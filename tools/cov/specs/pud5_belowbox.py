# PuddleSpore (module 51) pud5 fixture for CPuddleSpore::WeaponHitsWeakSpot (fn_51_2DA4,
# src/MetroidPrime/Enemies/CPuddleSpore.cpp:256-266).  This file is BOTH the force.sh spec argument and
# COV_GDB_SCRIPT.  It makes exactly one bounded 4-byte edit per run, on a real invocation.
#
# The two arms pud5 left open are the geometric side where the hit point lies BELOW the touch box:
# retail 0x2E70 `fcmpo f1, -sine` / 0x2E74 `ble .L_00002E88` (dot <= -mHitDetectionSine -> return true).
# Every shot the driver fires comes from the platform or from above, so the dot is never <= -sine.
#
# The fixture moves the hit point for ONE real invocation: the referenced CVector3f (rsi = `position`,
# the projectile translation or GetCollisionResponseType's argument) gets its z word rewritten to
# `touch-box centre z - 1.0e6`, i.e. a shot from far below the box.  CUnitVector3f(position - centre) is
# then (+-eps, +-eps, -1), so dot = up . dir ~= -up.z.  Preconditions the fixture checks FROM THE LIVE
# OBJECT before it writes: the spore is open (the arm's own precondition), up.z (mTransform.m22) >= 0.9,
# and 0.999 * up.z >= mData.mHitDetectionSine, so dot <= -sine is guaranteed by arithmetic, not hoped
# for.  Otherwise the call is left natural and logged.  The write is the 4-byte position.z word;
# the original word is read first, recorded BEFORE the write, rolled back on any write/readback failure
# and written back and read back at the return (a FinishBreakpoint armed before the write).  The
# function's bool result is recorded; the below-box arm must return true.
#
# Window: only an invocation of THIS spore (first instance seen), only from the two real callers
# (CPuddleSpore::Touch / CPuddleSpore::GetCollisionResponseType), only while mState == kPS_Open.  The
# summary fails closed on: no forced call, forced call not returning true, unreadable/invalid bool
# result, write or readback failure, unrestored obligation, unreturned call, no natural return, install
# failure.  The edge evidence itself comes from the gcda: 250:1/2 and 251:1/4 may only be taken by
# the forced call (the unedited sine/geometry never reaches them in 36 natural calls).
#
# PUD5_SIDE=above (env) puts the hit point 1e6 ABOVE the centre instead: dot ~= +up.z >= sine, the
# retail 0x2E78/0x2E7C `bge` -> true arm (260:3/4).  Default is below.
#
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/specs/pud5_belowbox.py \
#     tools/cov/force.sh tools/cov/specs/pud5_belowbox.py /tmp/opencode/pud5_r1 \
#     3dfd2249,cbc72d79 --con tools/cov/specs/pud2_puddlespore.con --fast --frames 2150
import hashlib
import os
import struct

import gdb

OUT = os.environ.get('COV_OUT', '/tmp')
LOG = os.path.join(OUT, 'diag.log')
TABLE = os.path.join(OUT, 'fixture.txt')
FUNC = '_ZNK12CPuddleSpore18WeaponHitsWeakSpotERK9CVector3fRK11CWeaponMode'
ALLOWED_CALLERS = ('CPuddleSpore::Touch', 'CPuddleSpore::GetCollisionResponseType')
KPS_OPEN = 1
BELOW_DISTANCE = 1.0e6   # hit point this far under the touch-box centre (metres)
SIDE = os.environ.get('PUD5_SIDE', 'below')   # 'below' (dot <= -sine) or 'above' (dot >= sine, retail 0x2E78 bge)
SIGN = 1.0 if SIDE == 'above' else -1.0
UP_Z_MIN = 0.9           # the spore must be upright for the arithmetic below
state = {'installed': 0, 'binary': 'unchecked', 'calls': [], 'current': None, 'forced_id': None,
         'tracked_this': None, 'refused': [], 'problems': []}


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def problem(msg):
    state['problems'].append(msg)
    log('pud5 FAIL: ' + msg)


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return 'err:' + str(e)[:48]
    except Exception as e:
        return 'exc:' + repr(e)[:48]


def evf(expr):
    try:
        return float(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return 'err:' + str(e)[:48]
    except Exception as e:
        return 'exc:' + repr(e)[:48]


def f32_bits(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


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
        if call['mutation_attempted']:
            addr, orig = call['edit_addr'], call['edit_original']
            now = rd32(addr)
            try:
                gdb.execute('set {unsigned int}0x%x = 0x%x' % (addr, orig))
                back = rd32(addr)
            except gdb.error as e:
                problem('forced call: position.z restore failed: %s' % str(e)[:60])
                back = None
            if back == orig:
                call['restored'] = True
                log('forced call: restored ORIGINAL position.z word %s at 0x%x (read back %s; before '
                    'the restore it was %s)' % (fmt(orig), addr, fmt(back), fmt(now)))
            else:
                problem('forced call: position.z restore readback %s, wanted %s'
                        % (fmt(back), fmt(orig)))
        log('call %d: returned %s (attempted_mutation=%d caller=%s state=%s type_in=%s)'
            % (call['id'],
               'true' if call['ret'] == 1 else ('UNREADABLE' if call['ret'] is None
                                                else fmt(call['ret'])),
               int(call['mutation_attempted']), call['caller'], fmt(call['mstate']),
               fmt(call['type_in'])))
        return False

    def out_of_scope(self):
        self.call['returned'] = False
        problem('call %d: WeaponHitsWeakSpot left without a return' % self.call['id'])
        if self.call['mutation_attempted'] and not self.call['restored']:
            problem('call %d: attempted mutation not restored (the frame is gone; expired stack/heap '
                    'storage is never written)' % self.call['id'])


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
        pos = ev('$rsi')
        mstate = ev('((CPuddleSpore*)0x%x)->mState' % this) if isinstance(this, int) and this else '?'
        try:
            frame = gdb.newest_frame()
            older = frame.older() if frame is not None else None
            caller = (older.name() or '') if older is not None else ''
        except Exception:
            caller = ''
        call = {'id': len(state['calls']) + 1, 'accepted': False, 'armed': False,
                'mutation_attempted': False, 'write_confirmed': False, 'rollback_attempted': False,
                'rollback_confirmed': False, 'returned': False, 'ret': None, 'caller': caller,
                'mstate': mstate, 'edit_addr': (pos + 8) if isinstance(pos, int) else None, 'edit_original': None, 'restored': False,
                'type_in': ev('*(unsigned int*)0x%x' % mode) if isinstance(mode, int) and mode else '?'}
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
        call['accepted'] = True
        try:
            CallReturn(gdb.newest_frame(), call)
            call['armed'] = True
        except Exception as e:
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
        if not isinstance(pos, int) or pos == 0:
            problem('call %d: position address unresolved (%s)' % (call['id'], pos))
            return
        base = '((CPuddleSpore*)0x%x)' % this
        min_z, max_z = evf(base + '->mTouchBounds.min.mZ'), evf(base + '->mTouchBounds.max.mZ')
        up_z, sine = evf(base + '->mTransform.m22'), evf(base + '->mData.mHitDetectionSine')
        if not all(isinstance(v, float) for v in (min_z, max_z, up_z, sine)):
            problem('call %d: live geometry unreadable (min.z=%s max.z=%s up.z=%s sine=%s), nothing '
                    'written' % (call['id'], min_z, max_z, up_z, sine))
            return
        centre_z = 0.5 * (min_z + max_z)
        if up_z < UP_Z_MIN or 0.999 * up_z < sine:
            log('call %d: natural - preconditions not met (up.z=%.4f sine=%.4f), no write'
                % (call['id'], up_z, sine))
            return
        new_word = f32_bits(centre_z + SIGN * BELOW_DISTANCE)
        original = rd32(pos + 8)
        if original is None:
            problem('call %d: position.z word at 0x%x unreadable, nothing written'
                    % (call['id'], pos))
            return
        call['edit_original'] = original
        call['mutation_attempted'] = True          # BEFORE the write: the return must restore it
        state['forced_id'] = call['id']
        write_error = None
        try:
            gdb.execute('set {unsigned int}0x%x = 0x%x' % (pos + 8, new_word))
        except gdb.error as e:
            write_error = str(e)[:60]
            problem('call %d: position.z write raised: %s (attempted mutation recorded, rolling back)'
                    % (call['id'], write_error))
        now = rd32(pos + 8) if write_error is None else None
        if write_error is None and now is None:
            problem('call %d: post-write readback unreadable at 0x%x (attempted mutation recorded, '
                    'rolling back)' % (call["id"], pos + 8))
        elif write_error is None and now != new_word:
            problem('call %d: position.z write did not take (read %s; attempted mutation recorded, '
                    'rolling back)' % (call['id'], fmt(now)))
        if write_error is not None or now != new_word:
            call['write_confirmed'] = False
            rollback_ok = self.rollback(call)
            log('call %d: mutation failed%s; immediate rollback %s (the failure stays recorded and the '
                'return will still restore)'
                % (call['id'], '' if write_error is None else ' (exception)',
                   'restored the original' if rollback_ok else 'FAILED'))
            return
        call['write_confirmed'] = True
        log('call %d: FORCED position.z word %s -> %s (centre.z %.4f %+.0f; up.z=%.4f sine=%.4f) via '
            'caller %s state=%s; snapshot kept for the return'
            % (call['id'], fmt(original), fmt(new_word), centre_z, SIGN * BELOW_DISTANCE, up_z, sine, caller,
               fmt(mstate)))

    @staticmethod
    def rollback(call):
        """Put the original word back right away; report honestly, never claim success it did not see."""
        addr, orig = call['edit_addr'], call['edit_original']
        call['rollback_attempted'] = True
        try:
            gdb.execute('set {unsigned int}0x%x = 0x%x' % (addr, orig))
            back = rd32(addr)
        except gdb.error as e:
            call['rollback_confirmed'] = False
            problem('call %d: rollback write failed: %s' % (call['id'], str(e)[:60]))
            return False
        if back == orig:
            call['rollback_confirmed'] = True
            log('call %d: rollback read back the ORIGINAL 0x%x at 0x%x (failure still recorded)'
                % (call['id'], orig, addr))
            return True
        call['rollback_confirmed'] = False
        problem('call %d: rollback readback %s, wanted 0x%x' % (call['id'], fmt(back), orig))
        return False


def summarize():
    problems = list(state['problems'])
    calls = state['calls']
    if state['installed'] != 1:
        problems.append('fixture not installed')
    if not calls:
        problems.append('no WeaponHitsWeakSpot call at all')
    accepted = [c for c in calls if c['accepted']]
    armed = [c for c in accepted if c.get('armed')]
    forced = [c for c in calls if c['mutation_attempted']]
    if not forced:
        problems.append('the forced below-box call never happened (calls=%d, accepted=%d, '
                        'refused=%d)' % (len(calls), len(accepted), len(state['refused'])))
    # every armed call must return, natural ones included; a refused call is never completion evidence
    unreturned = [c['id'] for c in armed if not c['returned']]
    if unreturned:
        problems.append('accepted call(s) without a return: %s' % unreturned)
    unrestored = [c['id'] for c in forced if not c['restored']]
    if unrestored:
        problems.append('attempted mutation(s) not restored: %s' % unrestored)
    # A returned call whose bool result could not be read (or is not 0/1) is NOT evidence: fail closed.
    returned = [c for c in accepted if c['returned']]
    bad_ret = [c['id'] for c in returned if c['ret'] not in (0, 1)]
    if bad_ret:
        problems.append('accepted call(s) returned without a readable bool result: %s '
                        '(unreadable/invalid $rax cannot count as evidence)' % bad_ret)
    for c in forced:
        if c['returned'] and c['ret'] != 1:
            problems.append('call %d: forced call returned %s, the below-box arm must be observed to '
                            'return true' % (c['id'], fmt(c['ret'])))
    natural = [c for c in returned if not c['mutation_attempted'] and c['ret'] in (0, 1)]
    if not natural:
        problems.append('no accepted natural call returned a readable bool (the base/geometric paths '
                        'need one)')
    status = 'OK' if not problems else 'FAIL'
    line = ('status=%s binary=%s installed=%d calls=%d accepted=%d refused=%d armed=%d returned=%d '
            'attempted=%d restored=%d forced_ret=%s natural=%d callers=%s states=%s types=%s reason=%s\n'
            % (status, state['binary'], state['installed'], len(calls), len(accepted),
               len(state['refused']), len(armed), sum(1 for c in armed if c['returned']),
               len(forced), sum(1 for c in forced if c['restored']),
               ','.join(fmt(c['ret']) for c in forced), len(natural),
               sorted(set(c['caller'] for c in calls)), sorted(set(str(c['mstate']) for c in calls)),
               sorted(set(fmt(c['type_in']) for c in calls)),
               '; '.join(problems) if problems else '-'))
    with open(TABLE, 'a') as fh:
        fh.write(line)
    log('pud5 summary: ' + line.strip())
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
    log('pud5: fixture installed (binary %s, arm kPS_Open=%d, below distance %.0f)'
        % ((_sha or '?')[:12], KPS_OPEN, BELOW_DISTANCE))
except Exception as e:
    problem('breakpoint install failed (%s): %r' % (FUNC[:40], e))
