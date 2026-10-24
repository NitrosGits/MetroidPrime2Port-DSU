# PuddleSpore (module 51) pud2 fixture for CPuddleSpore::WeaponHitsWeakSpot (fn_51_2DA4,
# src/MetroidPrime/Enemies/CPuddleSpore.cpp:246-256).  This file is BOTH the force.sh spec argument and
# COV_GDB_SCRIPT.  It makes exactly one bounded 4-byte argument edit per run, on a real invocation.
#
# Source: `if (IsOpen() && GetDamageVulnerability()->WeaponHits(mode, false)) { ... }` then the
# dot-product reject, else `return true`.
# Retail (fn_51_2DA4): 0x2DC8 bl IsOpen / 0x2DD0 beq -> tail, 0x2DE0 vtable+0x40 (GetDamageVulnerability),
# 0x2DF0 bl CDamageVulnerability::WeaponHits / 0x2DF8 beq -> tail (the arm this slice is after), then
# GetCenterPoint / the up dot / the two sine compares.  Host: IsOpen call + `test %al,%al` then the
# virtual call at `*0x78(%rax)`; the CWeaponMode reference is the third argument, rdx.
#
# The missing arm is "open AND WeaponHits false": every shipped player projectile the fight produces
# (power, dark, charged, missile) is accepted by the open vulnerability.  The fixture therefore rewrites
# the REFERENCED CWeaponMode for one call to the value a default-constructed CWeaponMode has -
# include/MetroidPrime/Weapons/WeaponTypes.hpp:32-47, `explicit CWeaponMode(EWeaponType type = kWT_None)`
# with kWT_None = -1, so the packed word is 0x0000ffff - and that type is what
# CDamageVulnerability::WeaponHits rejects first (`IsInvalidWeaponType`: type < kWT_Power,
# CDamageVulnerability.cpp:7,109-111).  No fabricated return, no replacement function, no enum value
# outside the shipped one.
#
# Window and obligations: the write happens only on an invocation of THIS spore (the first instance seen),
# only when the caller frame is one of the two real callers (CPuddleSpore::Touch or
# CPuddleSpore::GetCollisionResponseType), and only while mState == kPS_Open - the arm's own
# precondition.  A FinishBreakpoint is armed before the write; at the return the original 4 bytes are
# written back and read back, and the function's return value ($rax) is recorded, so the skip arm is
# evidenced by a real return and its result (true), not by the entry alone.  Any other caller or object
# is refused and logged.  The summary fails closed on: no forced call, forced call returning false,
# write/readback failure, unrestored obligation, unreturned call, no natural call, install failure.
#
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/pud2_weaponhitsweakspot.py \
#     tools/cov/force.sh tools/cov/specs/pud2_weaponhitsweakspot.py /tmp/opencode/pud2_r1 \
#     3dfd2249,cbc72d79 --con tools/cov/specs/pud2_puddlespore.con --fast --frames 2000
import hashlib
import os

import gdb

OUT = os.environ.get('COV_OUT', '/tmp')
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
FUNC = '_ZNK12CPuddleSpore18WeaponHitsWeakSpotERK9CVector3fRK11CWeaponMode'
ALLOWED_CALLERS = ('CPuddleSpore::Touch', 'CPuddleSpore::GetCollisionResponseType')
KPS_OPEN = 1
NONE_WEAPON_WORD = 0x0000ffff  # CWeaponMode() : mWeaponType = uint(kWT_None = -1), all flags 0
state = {'installed': 0, 'binary': 'unchecked', 'calls': [], 'current': None, 'forced_id': None,
         'tracked_this': None, 'refused': [], 'problems': []}


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def problem(msg):
    state['problems'].append(msg)
    log('pud2 FAIL: ' + msg)


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return 'err:' + str(e)[:48]
    except Exception as e:
        return 'exc:' + repr(e)[:48]


def fmt(v):
    return '0x%x' % v if isinstance(v, int) else str(v)


def rd32(addr):
    v = ev('*(unsigned int*)0x%x' % addr)
    return v if isinstance(v, int) else None


class CallReturn(gdb.FinishBreakpoint):
    def __init__(self, frame, call):
        super().__init__(frame, internal=True)
        self.call = call

    def stop(self):
        call = self.call
        call['returned'] = True
        rax = ev('$rax')
        call['ret'] = rax & 0xff if isinstance(rax, int) else None
        if state['current'] is call:
            state['current'] = None
        if call['forced']:
            addr, orig = call['mode_addr'], call['mode_original']
            now = rd32(addr)
            try:
                gdb.execute('set {unsigned int}0x%x = 0x%x' % (addr, orig))
                back = rd32(addr)
            except gdb.error as e:
                problem('forced call: mode restore failed: %s' % str(e)[:60])
                back = None
            if back == orig:
                call['restored'] = True
                log('forced call: restored ORIGINAL CWeaponMode word %s at 0x%x (read back %s; before '
                    'the restore it was %s)' % (fmt(orig), addr, fmt(back), fmt(now)))
            else:
                problem('forced call: CWeaponMode restore readback %s, wanted %s'
                        % (fmt(back), fmt(orig)))
        log('call %d: returned %s (forced=%d caller=%s state=%s type_in=%s)'
            % (call['id'], 'true' if call['ret'] == 1 else fmt(call['ret']), int(call['forced']),
               call['caller'], fmt(call['mstate']), fmt(call['type_in'])))
        return False

    def out_of_scope(self):
        problem('call %d: WeaponHitsWeakSpot left without a return' % self.call['id'])


class EntryBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('*' + FUNC, internal=True)

    def stop(self):
        try:
            self._stop()
        except Exception as e:
            problem('entry exception on call %d: %r' % (len(state['calls']) + 1, e))
        return False

    def _stop(self):
        this = ev('$rdi')
        mode = ev('$rdx')
        mstate = ev('((CPuddleSpore*)0x%x)->mState' % this) if isinstance(this, int) and this else '?'
        try:
            frame = gdb.newest_frame()
            older = frame.older() if frame is not None else None
            caller = (older.name() or '') if older is not None else ''
        except Exception:
            caller = ''
        call = {'id': len(state['calls']) + 1, 'forced': False, 'returned': False, 'ret': None,
                'caller': caller, 'mstate': mstate, 'mode_addr': mode, 'mode_original': None,
                'type_in': ev('*(unsigned int*)0x%x' % mode) if isinstance(mode, int) and mode else '?',
                'restored': False}
        state['calls'].append(call)
        state['current'] = call
        if state['tracked_this'] is None and any(c in caller for c in ALLOWED_CALLERS):
            state['tracked_this'] = this
            log('tracking spore this=0x%x (first allowed caller %s)' % (this, caller))
        allowed = any(c in caller for c in ALLOWED_CALLERS)
        if not allowed:
            state['refused'].append(call['id'])
            log('call %d: refused - caller %s is outside the window %s'
                % (call['id'], caller or '(unknown)', list(ALLOWED_CALLERS)))
            return
        if state['tracked_this'] is not None and this != state['tracked_this']:
            state['refused'].append(call['id'])
            log('call %d: refused - this=0x%x is not the tracked spore 0x%x'
                % (call['id'], this, state['tracked_this']))
            return
        try:
            CallReturn(gdb.newest_frame(), call)
        except Exception as e:
            call['armed'] = False
            problem('call %d: FinishBreakpoint not created (%r); no write attempted' % (call['id'], e))
            return
        if state['forced_id'] is not None:
            log('call %d: natural (type_in=%s state=%s)' % (call['id'], fmt(call['type_in']),
                                                            fmt(mstate)))
            return
        if mstate != KPS_OPEN:
            log('call %d: natural - mState %s is not kPS_Open, the forced arm needs the open state'
                % (call['id'], fmt(mstate)))
            return
        if not isinstance(mode, int) or mode == 0:
            problem('call %d: CWeaponMode address unresolved (%s)' % (call['id'], mode))
            return
        original = rd32(mode)
        if original is None:
            problem('call %d: CWeaponMode word at 0x%x unreadable' % (call['id'], mode))
            return
        call['mode_original'] = original
        try:
            gdb.execute('set {unsigned int}0x%x = 0x%x' % (mode, NONE_WEAPON_WORD))
        except gdb.error as e:
            problem('call %d: CWeaponMode write failed: %s (snapshot kept)' % (call['id'], str(e)[:60]))
            return
        now = rd32(mode)
        if now != NONE_WEAPON_WORD:
            problem('call %d: CWeaponMode write did not take (read %s)' % (call['id'], fmt(now)))
            return
        call['forced'] = True
        state['forced_id'] = call['id']
        log('call %d: FORCED CWeaponMode %s -> %s (kWT_None default CWeaponMode()) via caller %s '
            'state=%s; snapshot kept for the return'
            % (call['id'], fmt(original), fmt(NONE_WEAPON_WORD), caller, fmt(mstate)))


def summarize():
    problems = list(state['problems'])
    calls = state['calls']
    if state['installed'] != 1:
        problems.append('fixture not installed')
    if not calls:
        problems.append('no WeaponHitsWeakSpot call at all')
    forced = [c for c in calls if c['forced']]
    if not forced:
        problems.append('the forced open+WeaponHits-false call never happened (calls=%d, refused=%d)'
                        % (len(calls), len(state['refused'])))
    unreturned = [c['id'] for c in calls if c['forced'] and not c['returned']]
    if unreturned:
        problems.append('forced call(s) without a return: %s' % unreturned)
    for c in forced:
        if c['returned'] and c['ret'] != 1:
            problems.append('call %d: forced call returned %s, the skip arm must return true'
                            % (c['id'], fmt(c['ret'])))
        if c['returned'] and not c['restored']:
            problems.append('call %d: CWeaponMode not restored' % c['id'])
    natural = [c for c in calls if not c['forced'] and c['returned']]
    if not natural:
        problems.append('no natural call returned (the base/geometric paths need one)')
    status = 'OK' if not problems else 'FAIL'
    line = ('status=%s binary=%s installed=%d calls=%d refused=%d forced=%d forced_ret=%s '
            'natural=%d callers=%s states=%s types=%s reason=%s\n'
            % (status, state['binary'], state['installed'], len(calls), len(state['refused']),
               len(forced), ','.join(fmt(c['ret']) for c in forced), len(natural),
               sorted(set(c['caller'] for c in calls)), sorted(set(str(c['mstate']) for c in calls)),
               sorted(set(fmt(c['type_in']) for c in calls)),
               '; '.join(problems) if problems else '-'))
    with open(TABLE, 'a') as fh:
        fh.write(line)
    log('pud2 summary: ' + line.strip())
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
        return None, 'no binary path'
    path = fn if os.path.isabs(fn) else os.path.join(os.getcwd(), fn)
    try:
        return sha256_file(path), path
    except OSError as e:
        return None, 'cannot read %s: %s' % (path, str(e)[:60])


gdb.events.exited.connect(lambda event: summarize())
_sha, _why = binary_sha()
state['binary'] = _sha or ('unavailable: ' + str(_why))
try:
    EntryBp()
    state['installed'] = 1
    log('pud2: fixture installed (binary %s, arm kPS_Open=%d, forced word %s)'
        % ((_sha or '?')[:12], KPS_OPEN, fmt(NONE_WEAPON_WORD)))
except Exception as e:
    problem('breakpoint install failed (%s): %r' % (FUNC[:40], e))
