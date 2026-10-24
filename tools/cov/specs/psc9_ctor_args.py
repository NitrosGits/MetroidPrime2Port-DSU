# PlantScarabSwarm (module 49) psc9 fixture: the constructor's own argument-driven arms
# (fn_49_2300, src/MetroidPrime/Enemies/CPlantScarabSwarm.cpp:24-75).  This file is BOTH the force.sh
# spec argument and COV_GDB_SCRIPT.  It writes ONE 4-byte scalar per phase, on the real call, and puts
# the ORIGINAL bytes back at the constructor's return, then reads them back to prove it.
#
# The constructor is a real out-of-line function.  Its three source conditions sit at these offsets in
# the archived build (objdump -d --line-numbers, bound to the binary hash below):
#   +0x32e  f4f6de  cmpl $0xffffffff,0x0(%r13)      cpp:50  animRes.GetId() != kInvalidAssetId
#   +0x342  f4f6f2  cmpl $0xffffffff,0x1a8(%rsp)    cpp:50  intoAttackAnimation != -1
#   +0x54f  f4f8ff  cmpl $0xffffffff,0x1b0(%rsp)    cpp:65  attackAnimation != -1
# r13 holds the caller's CAnimRes* at +0x32e (the compare dereferences it); the two int arguments are
# the stack slots the compares read.  Each phase patches exactly one of them for the call:
#   natural            no write at all                     (all conditions true)
#   invalidAnim        animRes.mAncsId 0xffffffff          (cpp:50 first operand false)
#   intoAttackMinus1   the intoAttackAnimation slot        (cpp:50 second operand false)
#   attackAnimMinus1   the attackAnimation slot            (cpp:65 operand false)
# The caller's CAnimRes object is patched only at +0x32e, i.e. AFTER the base CSwarmBasics constructor
# has consumed the valid resource, so the base ctor is never fed an invalid asset.  At the ctor's
# return the fixture restores the snapshot, reads it back, and checks the CONSTRUCTED OBJECT's state
# (attack-model count, shared model, bit base) - an output oracle, not an entry count.
#
# The breakpoints are addresses of one build, so the fixture hashes the binary first and refuses to
# install on any other (status=FAIL, reason=BINARY SHA MISMATCH).  Every failure path - no hit, no
# object state, unreadable slot, caught exception, restore mismatch - is a named FAIL in fixture.txt.
#
#   PSC9_PHASE_START=0 BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/psc9_ctor_args.py \
#     tools/cov/force.sh tools/cov/specs/psc9_ctor_args.py /tmp/opencode/psc9_r1 \
#     3dfd2249,ok_swamp_hall --con tools/cov/specs/psc9_ctor_ok.con --fast --frames 1200
import hashlib
import os
import re

import gdb

OUT = os.environ.get('COV_OUT', '/tmp')
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
EVIDENCE_SHA256 = 'cbab51dd3faee657cd725085e8d36d0074ba33f29b57980b5cd7b104772a2d15'
def ctor_symbol():
    """The ctor's mangled name, taken from the run's nm dump (COV_NM) by its demangled prefix."""
    nm = os.environ.get('COV_NM')
    if not nm or not os.path.exists(nm):
        return None
    with open(nm) as fh:
        for ln in fh:
            parts = ln.rstrip('\n').split('\t')
            if len(parts) == 2 and parts[1].startswith('CPlantScarabSwarm::CPlantScarabSwarm('):
                f = parts[0].split()
                if len(f) == 3 and f[1] in 'tTW':
                    return f[2]
    return None


CTOR = ctor_symbol()
ANIM_ID_OFF = 0x32e
INTO_OFF = 0x342
ATTACK_OFF = 0x54f
INTO_SLOT = 0x1a8
ATTACK_SLOT = 0x1b0
PHASES = ['natural', 'invalidAnim', 'intoAttackMinus1', 'attackAnimMinus1']
try:
    PHASE_START = int(os.environ.get('PSC9_PHASE_START', '0'))
except ValueError:
    PHASE_START = 0
state = {'installed': 0, 'binary': 'unchecked', 'hits': 0, 'objects': 0, 'writes': 0,
         'restored': 0, 'restore_bad': 0, 'state_ok': 0, 'problems': [], 'phase': None,
         'phase_idx': PHASE_START, 'pending': {}, 'checked': []}


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def problem(msg):
    state['problems'].append(msg)
    log('psc9 FAIL: ' + msg)


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return 'err:' + str(e)[:48]
    except Exception as e:
        return 'exc:' + repr(e)[:48]


def rd32(addr):
    v = ev('*(unsigned int*)0x%x' % addr)
    return v if isinstance(v, int) else None


class CtorReturn(gdb.FinishBreakpoint):
    """At the ctor's return: restore the phase's snapshot, read it back, check the object state."""

    def __init__(self, frame, this, phase):
        super().__init__(frame, internal=True)
        self.this, self.phase = this, phase

    def stop(self):
        state['objects'] += 1
        pend = state['pending'].pop(self.phase, [])
        for addr, original, what in pend:
            now = rd32(addr)
            try:
                gdb.execute('set {unsigned int}0x%x = 0x%x' % (addr, original))
                back = rd32(addr)
            except gdb.error as e:
                state['restore_bad'] += 1
                problem('%s: RESTORE FAILED for %s at 0x%x: %s' % (self.phase, what, addr, str(e)[:60]))
                continue
            if back == original:
                state['restored'] += 1
                log('%s: restored ORIGINAL %s at 0x%x = 0x%x (read back 0x%x; before the restore it '
                    'was 0x%x)' % (self.phase, what, addr, original, back, now))
            else:
                state['restore_bad'] += 1
                problem('%s: restore readback mismatch for %s: wanted 0x%x read 0x%x'
                        % (self.phase, what, original, back))
        problems = self.check_state()
        if problems:
            for p in problems:
                problem('%s: object state %s' % (self.phase, p))
        else:
            state['state_ok'] += 1
            log('%s: object state OK (models=%d sharedModel=%d sharedDelta=%d bitBase=%d sharedBit=%d)'
                % ((self.phase,) + self.snapshot()))
        return False

    def out_of_scope(self):
        problem('%s: the ctor left without a return; snapshot not restored' % self.phase)

    def snapshot(self):
        obj = '((CPlantScarabSwarm*)0x%x)' % self.this
        return (ev(obj + '->mAttackModels.mCount'),
                ev(obj + '->mSharedAttackModel.mPtr != 0'),
                ev(obj + '->mSharedAttackDeltas.mPtr != 0'),
                ev(obj + '->mAttackModelBitBase'),
                ev(obj + '->mSharedAttackBit'))

    def check_state(self):
        obj = '((CPlantScarabSwarm*)0x%x)' % self.this
        models = ev(obj + '->mAttackModels.mCount')
        shared = ev(obj + '->mSharedAttackModel.mPtr != 0')
        delta = ev(obj + '->mSharedAttackDeltas.mPtr != 0')
        bit = ev(obj + '->mSharedAttackBit')
        base = ev(obj + '->mAttackModelBitBase')
        if not all(isinstance(x, int) for x in (models, shared, delta, bit, base)):
            return ['unreadable (%s/%s/%s/%s/%s)' % (models, shared, delta, bit, base)]
        want = {
            'natural': (3, 1, 1, 3, 1),
            'invalidAnim': (0, 0, 0, 0, 0),
            'intoAttackMinus1': (0, 0, 0, 0, 0),
            'attackAnimMinus1': (3, 0, 0, 3, 0),
        }[self.phase]
        got = (models, shared, delta, base, bit)
        if got != want:
            return ['got models/shared/delta/base/bit=%s want %s (base must equal mModelDatas.size)'
                    % (got, want)]
        return []


class EntryBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*%s' % CTOR, internal=True)

    def stop(self):
        try:
            state['hits'] += 1
            state['phase'] = PHASES[(state['phase_idx'] + state['hits'] - 1) % len(PHASES)]
            this = ev('$rdi')
            if not isinstance(this, int) or this == 0:
                problem('hit %d: this unresolved' % state['hits'])
                return False
            CtorReturn(gdb.newest_frame(), this, state['phase'])
            log('ctor hit %d: this=0x%x phase=%s' % (state['hits'], this, state['phase']))
        except Exception as e:
            problem('entry fixture exception on hit %d: %r' % (state['hits'], e))
        return False


class PatchBp(gdb.Breakpoint):
    def __init__(self, off, which):
        super().__init__('*%s+0x%x' % (CTOR, off), internal=True)
        self.which = which

    def stop(self):
        try:
            self._stop()
        except Exception as e:
            problem('%s: patch exception in %s: %r' % (state['phase'], self.which, e))
        return False

    def _stop(self):
        phase = state['phase']
        if self.which == 'animid' and phase != 'invalidAnim':
            return
        if self.which == 'into' and phase != 'intoAttackMinus1':
            return
        if self.which == 'attack' and phase != 'attackAnimMinus1':
            return
        if self.which == 'animid':
            addr = ev('(long)&animRes')
            how = 'DWARF &animRes'
            if not isinstance(addr, int) or addr == 0:
                reg = ev('$r13')
                addr = reg if isinstance(reg, int) and reg else None
                how = '$r13 (disassembly: r13 holds the CAnimRes*)'
            what = 'animRes.mAncsId'
        else:
            slot = INTO_SLOT if self.which == 'into' else ATTACK_SLOT
            rsp = ev('$rsp')
            if not isinstance(rsp, int):
                problem('%s: rsp unresolved for the %s slot' % (phase, self.which))
                return
            addr = rsp + slot
            how = '$rsp+0x%x' % slot
            what = 'intoAttackAnimation' if self.which == 'into' else 'attackAnimation'
        if not isinstance(addr, int) or addr == 0:
            problem('%s: %s address unresolved' % (phase, self.which))
            return
        original = rd32(addr)
        if original is None:
            problem('%s: %s at 0x%x unreadable' % (phase, what, addr))
            return
        try:
            gdb.execute('set {unsigned int}0x%x = 0x%x' % (addr, 0xffffffff))
        except gdb.error as e:
            problem('%s: write to %s at 0x%x failed: %s' % (phase, what, addr, str(e)[:60]))
            return
        now = rd32(addr)
        if now != 0xffffffff:
            problem('%s: %s write did not take at 0x%x (read 0x%x)' % (phase, what, addr, now))
            return
        state['writes'] += 1
        state.setdefault('pending', {}).setdefault(phase, []).append((addr, original, what))
        log('%s: %s 0x%x -> 0x%x at 0x%x via %s (restored at the ctor return)'
            % (phase, what, original, now, addr, how))


def summarize():
    problems = list(state['problems'])
    if state['hits'] == 0:
        problems.append('no ctor hit')
    if state['objects'] == 0:
        problems.append('no ctor return reached')
    if state['phase'] in ('invalidAnim', 'intoAttackMinus1', 'attackAnimMinus1') and state['writes'] == 0:
        problems.append('phase %s never wrote its argument' % state['phase'])
    if state['restore_bad']:
        problems.append('%d restore failure(s)' % state['restore_bad'])
    if state['state_ok'] != state['objects']:
        problems.append('object-state oracle passed %d of %d returns' % (state['state_ok'], state['objects']))
    status = 'OK' if not problems else 'FAIL'
    line = ('status=%s binary=%s installed=%d hits=%d objects=%d writes=%d restored=%d '
            'restore_bad=%d state_ok=%d phase_start=%d reasons=%s\n'
            % (status, state['binary'], state['installed'], state['hits'], state['objects'],
               state['writes'], state['restored'], state['restore_bad'], state['state_ok'],
               PHASE_START, '; '.join(problems) if problems else '-'))
    with open(TABLE, 'a') as fh:
        fh.write(line)
    log('psc9 summary: ' + line.strip())
    return status, problems


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, 'rb') as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def binary_sha():
    try:
        ps = gdb.current_progspace()
        fn = ps.filename if ps is not None else None
    except gdb.error as e:
        return None, 'progspace error: %s' % str(e)[:60]
    if not fn:
        return None, 'no binary path in the progspace'
    path = fn if os.path.isabs(fn) else os.path.join(os.getcwd(), fn)
    try:
        return sha256_file(path), path
    except OSError as e:
        return None, 'cannot read %s: %s' % (path, str(e)[:60])


gdb.events.exited.connect(lambda event: summarize())
_sha, _why = binary_sha()
if _sha == EVIDENCE_SHA256 and not CTOR:
    state['binary'] = _sha
    problem('ctor symbol not found in COV_NM (no demangled "CPlantScarabSwarm::CPlantScarabSwarm(" '
            'entry) - fixture NOT installed')
elif _sha == EVIDENCE_SHA256:
    state['binary'] = _sha
    try:
        EntryBp()
        PatchBp(ANIM_ID_OFF, 'animid')
        PatchBp(INTO_OFF, 'into')
        PatchBp(ATTACK_OFF, 'attack')
        state['installed'] = 1
        log('psc9: ctor fixture installed on %s (binary %s, phase start %d)'
            % (CTOR[:40], _sha[:12], PHASE_START))
    except Exception as e:
        problem('breakpoint install failed on %s: %r' % (CTOR[:40], e))
else:
    state['binary'] = _sha or ('unavailable: ' + str(_why))
    problem('BINARY SHA MISMATCH: expected %s, got %s (%s) - the +0x32e/+0x342/+0x54f offsets and the '
            '0x1a8/0x1b0 stack slots are bound to the archived build, so the fixture is NOT installed'
            % (EVIDENCE_SHA256, _sha, _why))
