# PlantScarabSwarm (module 49) psc9 fixture: the constructor's own argument-driven arms
# (fn_49_2300, src/MetroidPrime/Enemies/CPlantScarabSwarm.cpp:24-75).  This file is BOTH the force.sh
# spec argument and COV_GDB_SCRIPT.
#
# The constructor is a real out-of-line function.  Its three source conditions sit at these offsets in
# the archived build (objdump -d --line-numbers; bound to the binary hash below):
#   +0x32e  cmpl $0xffffffff,0x0(%r13)      cpp:50  animRes.GetId() != kInvalidAssetId
#   +0x342  cmpl $0xffffffff,0x1a8(%rsp)    cpp:50  intoAttackAnimation != -1
#   +0x54f  cmpl $0xffffffff,0x1b0(%rsp)    cpp:65  attackAnimation != -1
# r13 holds the caller's CAnimRes* at +0x32e; the two int arguments are the stack slots the compares
# read.  Each phase patches exactly one of them for the call:
#   natural            no write                       (all conditions true)
#   invalidAnim        animRes.mAncsId 0xffffffff     (cpp:50 first operand false)
#   intoAttackMinus1   the intoAttackAnimation slot    (cpp:50 second operand false)
#   attackAnimMinus1   the attackAnimation slot        (cpp:65 operand false)
# The caller's CAnimRes object is patched only at +0x32e, i.e. AFTER the base CSwarmBasics constructor
# has consumed the valid resource, so the base ctor is never fed an invalid asset.
#
# Restoration contract (what the summary enforces):
#  - every ctor call gets its own record (id, phase, armed, obligations, returned); obligations are NOT
#    keyed by phase, so two calls of the same phase cannot share or lose a snapshot;
#  - a mutation is permitted ONLY when that call's FinishBreakpoint was created (armed); otherwise the
#    patch refuses and the summary fails;
#  - the 4 raw bytes are snapshotted and REGISTERED as an obligation BEFORE the write is attempted, and
#    the obligation stays even if the write fails or the post-write readback is unreadable;
#  - at the ctor's return the ORIGINAL bytes are written back and read back; the summary fails on any
#    obligation that is not restored, on any call that never returned, and on any missing phase write;
#  - a call that leaves without a return (out_of_scope) never writes expired stack storage; its
#    obligations are reported unrestored and the summary fails.
# Fail-closed guards: the binary hash must match (the offsets are one build's), the ctor symbol must be
# in the run's nm dump, and every failure path (no hit, install failure, unreadable address, refused
# mutation, restore readback) is a named reason in fixture.txt - status=OK only when everything above
# held for every call.
#
#   PSC9_PHASE_START=0 BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/psc9_ctor_args.py \
#     tools/cov/force.sh tools/cov/specs/psc9_ctor_args.py /tmp/opencode/psc9_rN \
#     3dfd2249,ok_swamp_hall --con tools/cov/specs/psc9_ctor_ok.con --fast --frames 1200
import hashlib
import os

import gdb

OUT = os.environ.get('COV_OUT', '/tmp')
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
EVIDENCE_SHA256 = 'cbab51dd3faee657cd725085e8d36d0074ba33f29b57980b5cd7b104772a2d15'
ANIM_ID_OFF = 0x32e
INTO_OFF = 0x342
ATTACK_OFF = 0x54f
INTO_SLOT = 0x1a8
ATTACK_SLOT = 0x1b0
PHASES = ['natural', 'invalidAnim', 'intoAttackMinus1', 'attackAnimMinus1']
WRITE_PHASES = ('invalidAnim', 'intoAttackMinus1', 'attackAnimMinus1')
try:
    PHASE_START = int(os.environ.get('PSC9_PHASE_START', '0'))
except ValueError:
    PHASE_START = 0
state = {'installed': 0, 'binary': 'unchecked', 'hits': 0, 'objects': 0, 'state_ok': 0,
         'calls': [], 'current': None, 'problems': []}


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
    except Exception as e:  # a broken expression must not abort the run
        return 'exc:' + repr(e)[:48]


def rd32(addr):
    v = ev('*(unsigned int*)0x%x' % addr)
    return v if isinstance(v, int) else None


def fmt(v):
    return '0x%x' % v if isinstance(v, int) else str(v)


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


class CtorReturn(gdb.FinishBreakpoint):
    """Per-call return hook: restore this call's obligations (never another call's) and check state."""

    def __init__(self, frame, this, call):
        super().__init__(frame, internal=True)
        self.this, self.call = this, call

    def stop(self):
        call = self.call
        call['returned'] = True
        state['objects'] += 1
        if state['current'] is call:
            state['current'] = None
        for ob in call['obligations']:
            addr, orig, what = ob['addr'], ob['original'], ob['what']
            now = rd32(addr)
            try:
                gdb.execute('set {unsigned int}0x%x = 0x%x' % (addr, orig))
                back = rd32(addr)
            except gdb.error as e:
                problem('hit %d: restore of %s at 0x%x failed: %s'
                        % (call['id'], what, addr, str(e)[:60]))
                continue
            if back is None:
                problem('hit %d: restore readback of %s at 0x%x unreadable (wanted %s)'
                        % (call['id'], what, addr, fmt(orig)))
            elif back == orig:
                ob['restored'] = True
                log('hit %d %s: restored ORIGINAL %s at 0x%x = %s (read back %s; before the restore it '
                    'was %s)' % (call['id'], call['phase'], what, addr, fmt(orig), fmt(back), fmt(now)))
            else:
                problem('hit %d: restore readback mismatch for %s at 0x%x: wanted %s read %s'
                        % (call['id'], what, addr, fmt(orig), fmt(back)))
        bad = self.check_state()
        if bad:
            for p in bad:
                problem('hit %d %s: object state %s' % (call['id'], call['phase'], p))
        else:
            state['state_ok'] += 1
            log('hit %d %s: object state OK (models=%s sharedModel=%s sharedDelta=%s bitBase=%s '
                'sharedBit=%s)' % ((call['id'], call['phase']) + tuple(
                    fmt(x) for x in self.snapshot())))
        return False

    def out_of_scope(self):
        self.call['returned'] = False
        state['unreturned'] = state.get('unreturned', 0) + 1
        n = len(self.call['obligations'])
        problem('hit %d: the ctor left without a return; %d obligation(s) NOT restored (expired stack '
                'storage, never written)' % (self.call['id'], n))

    def snapshot(self):
        obj = '((CPlantScarabSwarm*)0x%x)' % self.this
        return (ev(obj + '->mAttackModels.mCount'),
                ev(obj + '->mSharedAttackModel.mPtr != 0'),
                ev(obj + '->mSharedAttackDeltas.mPtr != 0'),
                ev(obj + '->mAttackModelBitBase'),
                ev(obj + '->mSharedAttackBit'))

    def check_state(self):
        models, shared, delta, base, bit = self.snapshot()
        if not all(isinstance(x, int) for x in (models, shared, delta, base, bit)):
            return ['unreadable (%s/%s/%s/%s/%s)' % (models, shared, delta, base, bit)]
        want = {
            'natural': (3, 1, 1, 3, 1),
            'invalidAnim': (0, 0, 0, 0, 0),
            'intoAttackMinus1': (0, 0, 0, 0, 0),
            'attackAnimMinus1': (3, 0, 0, 3, 0),
        }[self.call['phase']]
        got = (models, shared, delta, base, bit)
        if got != want:
            return ['got models/shared/delta/base/bit=%s want %s' % (got, want)]
        return []


class EntryBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*%s' % CTOR, internal=True)

    def stop(self):
        try:
            self._stop()
        except Exception as e:
            problem('entry exception on hit %d: %r' % (state['hits'] + 1, e))
        return False

    def _stop(self):
        state['hits'] += 1
        call = {'id': state['hits'],
                'phase': PHASES[(PHASE_START + state['hits'] - 1) % len(PHASES)],
                'armed': False, 'obligations': [], 'returned': False}
        state['calls'].append(call)
        state['current'] = call
        this = ev('$rdi')
        if not isinstance(this, int) or this == 0:
            problem('hit %d: this unresolved; no mutation will be attempted' % call['id'])
            return
        try:
            CtorReturn(gdb.newest_frame(), this, call)
            call['armed'] = True
        except Exception as e:
            problem('hit %d: FinishBreakpoint not created (%r); no mutation will be attempted'
                    % (call['id'], e))
            return
        log('ctor hit %d: this=0x%x phase=%s armed' % (call['id'], this, call['phase']))


class PatchBp(gdb.Breakpoint):
    def __init__(self, off, which):
        super().__init__('*%s+0x%x' % (CTOR, off), internal=True)
        self.which = which

    def stop(self):
        try:
            self._stop()
        except Exception as e:
            problem('%s: patch exception in %s: %r'
                    % ((state['current'] or {}).get('phase', '?'), self.which, e))
        return False

    def _stop(self):
        call = state['current']
        if call is None:
            problem('%s: patch ran with no live ctor call' % self.which)
            return
        phase = call['phase']
        if self.which == 'animid' and phase != 'invalidAnim':
            return
        if self.which == 'into' and phase != 'intoAttackMinus1':
            return
        if self.which == 'attack' and phase != 'attackAnimMinus1':
            return
        if not call['armed']:
            problem('hit %d %s: mutation refused, the call has no FinishBreakpoint'
                    % (call['id'], phase))
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
                problem('hit %d %s: rsp unresolved for the %s slot' % (call['id'], phase, self.which))
                return
            addr = rsp + slot
            how = '$rsp+0x%x' % slot
            what = 'intoAttackAnimation' if self.which == 'into' else 'attackAnimation'
        if not isinstance(addr, int) or addr == 0:
            problem('hit %d %s: %s address unresolved' % (call['id'], phase, self.which))
            return
        original = rd32(addr)
        if original is None:
            problem('hit %d %s: %s at 0x%x unreadable, nothing written' % (call['id'], phase, what, addr))
            return
        ob = {'addr': addr, 'original': original, 'what': what, 'restored': False}
        call['obligations'].append(ob)  # registered BEFORE the write; never discarded
        log('hit %d %s: snapshot %s at 0x%x = %s via %s (obligation registered before the write)'
            % (call['id'], phase, what, addr, fmt(original), how))
        try:
            gdb.execute('set {unsigned int}0x%x = 0x%x' % (addr, 0xffffffff))
        except gdb.error as e:
            problem('hit %d %s: write to %s at 0x%x failed: %s (snapshot retained for the return)'
                    % (call['id'], phase, what, addr, str(e)[:60]))
            return
        now = rd32(addr)
        if now is None:
            problem('hit %d %s: post-write readback of %s at 0x%x unreadable (snapshot retained)'
                    % (call['id'], phase, what, addr))
        elif now != 0xffffffff:
            problem('hit %d %s: write to %s did not take at 0x%x (read %s; snapshot retained)'
                    % (call['id'], phase, what, addr, fmt(now)))
        else:
            log('hit %d %s: %s %s -> 0xffffffff confirmed'
                % (call['id'], phase, what, fmt(original)))


def summarize():
    problems = list(state['problems'])
    calls = state['calls']
    if state['hits'] == 0:
        problems.append('no ctor hit')
    if state['installed'] != 1:
        problems.append('fixture not installed')
    unreturned = [c for c in calls if not c['returned']]
    if unreturned:
        problems.append('unreturned ctor call(s): %s' % [c['id'] for c in unreturned])
    obligations = [(c['id'], ob) for c in calls for ob in c['obligations']]
    unrestored = [(cid, ob['what'], hex(ob['addr'])) for cid, ob in obligations if not ob['restored']]
    if unrestored:
        problems.append('attempted write(s) not restored: %s' % unrestored)
    missing = [c['id'] for c in calls
               if c['returned'] and c['phase'] in WRITE_PHASES and not c['obligations']]
    if missing:
        problems.append('missing phase write for call(s): %s' % missing)
    if calls and state['state_ok'] != sum(1 for c in calls if c['returned']):
        problems.append('object-state oracle passed %d of %d returns'
                        % (state['state_ok'], sum(1 for c in calls if c['returned'])))
    status = 'OK' if not problems else 'FAIL'
    line = ('status=%s binary=%s installed=%d hits=%d objects=%d attempted=%d restored=%d '
            'unrestored=%d unreturned=%d state_ok=%d phase_start=%d reasons=%s\n'
            % (status, state['binary'], state['installed'], state['hits'], state['objects'],
               len(obligations), sum(1 for _c, ob in obligations if ob['restored']), len(unrestored),
               len(unreturned), state['state_ok'], PHASE_START,
               '; '.join(problems) if problems else '-'))
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
state['binary'] = _sha or ('unavailable: ' + str(_why))
if _sha != EVIDENCE_SHA256:
    problem('BINARY SHA MISMATCH: expected %s, got %s (%s) - the +0x32e/+0x342/+0x54f offsets and the '
            '0x1a8/0x1b0 stack slots are bound to the archived build, so the fixture is NOT installed'
            % (EVIDENCE_SHA256, _sha, _why))
elif not CTOR:
    problem('ctor symbol not found in COV_NM (no demangled "CPlantScarabSwarm::CPlantScarabSwarm(" '
            'entry) - fixture NOT installed')
else:
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
