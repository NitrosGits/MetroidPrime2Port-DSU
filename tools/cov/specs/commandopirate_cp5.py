# CommandoPirate (module 9) forcing spec, slice cp5: the AcceptScriptMsg (fn_9_CB40) arms the
# messages cannot reach on their own, forced per message: each message has its own call counter
# k and a cycle of write sets (k % len(cycle)), so a message's combinations do not depend on how
# the other messages interleave (cp4's period-4 phases over every call missed them). The cycles
# have length 3 against the driver's active/inactive alternation (period 2), so every pair of
# (active bit, forced set) comes round. Every write lasts the one call and is restored at its
# return; '!' writes (the message's own sender, a caller temporary) are not restored.
#
# Also: the XRDG's WeaponHurts (0x0000D204) answers true on every other ResistedDamage that gets
# that far (the shield's own resisted shots: the collision actor sends XRDG only when the weapon
# does *not* hit, CStateManager.cpp:2987, so the true arm needs the forced answer), and XOFF runs
# with the body frozen (0x0000CF5C; nothing in Echoes freezes a commando).
#
# Run (the coverage build; forced profiles first in a gcov-tool merge):
#   BUILD=build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh
#     tools/cov/specs/commandopirate_cp5.py <out> 3bfa3eff,05_temple_federationsite_dark --fast
#     --frames 1500 --driver 'python3 tools/drivers/commandopirate_cp5.py {port} <out>/drv.txt - msg'
# The breakpoints are this spec's own (force_ret.py runs it inside gdb); FORCE_RET stays empty.
import os

CLASS = 'CCommandoPirate'
FORCE_RET = []
SPEC = []

try:
    import gdb
except ImportError:  # read outside gdb
    gdb = None

THIS = '((CCommandoPirate*)%d)'
C63, C64, C65, C66 = '$this->xc63_', '$this->xc64_', '$this->xc65_', '$this->xc66_'
F0, F1 = '$this->mData.mFlags0', '$this->mData.mFlags1'
SENDER = '!((CScriptMsg*)%d)->mSenderId.value'
FROZEN = '$this->mBodyController.mPtr->mFrozen'

# message -> the cycle of write sets [(lvalue, value), ...]; 'HURTS' asks WeaponHurts for true.
CYCLES = {
    # ACTV (0x0000CD3C..0x0000CD58): the block's +0x91D 0x80 bit, and +0x920 != -1.
    'kSM_Activate': [[], [(F1, F1 + ' | 0x80')], [(F1, F1 + ' | 0x80'), ('$this->x920_', '0')]],
    # ESCP / ZERO (0x0000CF2C..0x0000CF3C, 0x0000CEF8..0x0000CF08): +0xC63 0x20.
    # ACTV's own 0x80 arm sets +0xC63 0x20 for good, so the bit is forced both ways.
    'kSM_Escape': [[], [(C63, C63 + ' & 0xdf')], [(C63, C63 + ' | 0x20')]],
    'kSM_SetToZero': [[], [(C63, C63 + ' & 0xdf')], [(C63, C63 + ' | 0x20')]],
    # XALD (0x0000CE04): the block's +0x91C 0x01 bit kills the actor, so only from the message's
    # FROM-th call on (the area's own load sends the first; the driver's two come last).
    'kSM_AreaLoaded': [[(F0, F0 + ' | 0x01')]],
    # XDMG (0x0000D0A0, 0x0000D0D0, 0x0000D150): +0xC65 0x04, +0xC66 0x40, +0xC64 0x10.
    # (+0xC64 0x10 is set from the spawn on, so it is forced both ways.)
    'kSM_Damage': [[], [(C65, C65 + ' | 0x04'), (C66, C66 + ' | 0x40'), (C64, C64 + ' & 0xef')],
                   [(C64, C64 + ' | 0x10')]],
    # XRDG (0x0000D168..0x0000D204): the shield up and the sender the shield's actor; the actor
    # cast failing (+0xBC6 kInvalidUniqueId with the console's invalid sender); WeaponHurts true.
    'kSM_ResistedDamage': [[], [(C65, C65 + ' | 0x04'), (SENDER, '$this->xbc6_.value'), 'HURTS'],
                           [(C65, C65 + ' | 0x04'), ('$this->xbc6_.value', '0xffff')],
                           [(C65, C65 + ' | 0x04'), (C64, C64 + ' | 0x10'), 'HURTS']],
    # XOFF (0x0000CF5C): the body frozen.
    'kSM_OffGround': [[], [(FROZEN, '1')], []],
}

# the first call a message's cycle applies to (earlier calls run unforced).
FROM = {'kSM_AreaLoaded': 2}

STATE = {'hurts': 0, 'counts': {}, 'log': 0, 'rdg': 0, 'live': 0}


def _log(msg):
    if STATE['log'] < 400:
        STATE['log'] += 1
        with open(os.path.join(os.environ['COV_OUT'], 'forced.log'), 'a') as fh:
            fh.write(msg + '\n')


if gdb is not None:
    ENUM = {}
    for name in CYCLES:
        ENUM[int(gdb.parse_and_eval(name))] = name

    class Restore(gdb.FinishBreakpoint):
        def __init__(self, frame, saved, hurts):
            super().__init__(frame, internal=True)
            self.saved, self.hurts = saved, hurts

        def stop(self):
            for lv, old in reversed(self.saved):
                try:
                    gdb.execute('set var %s = %s' % (lv, old))
                except gdb.error as e:
                    _log('restore failed %s: %s' % (lv, str(e)[:60]))
            STATE['hurts'] -= self.hurts
            return False

        def out_of_scope(self):
            STATE['hurts'] -= self.hurts
            _log('CB40 left without a return (not restored)')

    class MsgBp(gdb.Breakpoint):
        def stop(self):
            msg = int(gdb.parse_and_eval('((CScriptMsg*)$rdx)->m_msg'))
            name = ENUM.get(msg)
            if name is None:
                return False
            k = STATE['counts'].get(name, 0) + 1
            STATE['counts'][name] = k
            if name == 'kSM_ResistedDamage' and STATE['rdg'] < 40:
                STATE['rdg'] += 1
                this = THIS % int(gdb.parse_and_eval('$rdi'))
                _log('XRDG #%d sender %s xbc6 %s c65 %s' % (
                    k, str(gdb.parse_and_eval('((CScriptMsg*)$rdx)->mSenderId.value')),
                    str(gdb.parse_and_eval(this + '->xbc6_.value')),
                    str(int(gdb.parse_and_eval(this + '->xc65_')))))
            if k < FROM.get(name, 0):
                return False
            cycle = CYCLES[name]
            writes = cycle[k % len(cycle)]
            if name == 'kSM_ResistedDamage' and str(gdb.parse_and_eval(
                    '((CScriptMsg*)$rdx)->mSenderId.value == %s->xbc6_.value'
                    % (THIS % int(gdb.parse_and_eval('$rdi'))))) == 'true':
                # The shield actor's own XRDG (the driver's live shot as its last toucher): the
                # shield up, and WeaponHurts both ways.
                STATE['live'] += 1
                writes = [(C65, C65 + ' | 0x04')] + (['HURTS'] if STATE['live'] % 2 else [])
            if not writes:
                return False
            this = THIS % int(gdb.parse_and_eval('$rdi'))
            rdx = int(gdb.parse_and_eval('$rdx'))
            saved, hurts = [], 0
            for w in writes:
                if w == 'HURTS':
                    hurts = 1
                    continue
                lv, val = w
                keep = lv.startswith('!')
                lv = lv.lstrip('!').replace('$this', this)
                if '%d' in lv:
                    lv = lv % rdx
                val = val.replace('$this', this)
                try:
                    v = gdb.parse_and_eval(lv)
                    old = str(int(v))
                    gdb.execute('set var %s = %s' % (lv, val))
                    if not keep:
                        saved.append((lv, old))
                except gdb.error as e:
                    _log('set failed %s: %s' % (lv, str(e)[:60]))
            STATE['hurts'] += hurts
            try:
                Restore(gdb.newest_frame(), saved, hurts)
            except (gdb.error, ValueError) as e:
                STATE['hurts'] -= hurts
                _log('no restore: %s' % str(e)[:60])
            if name != 'kSM_OffGround' or k < 4:
                _log('%s #%d: %s' % (name, k, writes))
            return False

    class HurtsRet(gdb.FinishBreakpoint):
        def stop(self):
            gdb.execute('set $rax = 1')
            _log('WeaponHurts forced true')
            return False

    class HurtsBp(gdb.Breakpoint):
        def stop(self):
            if STATE['hurts'] > 0:
                try:
                    HurtsRet(gdb.newest_frame(), internal=True)
                except (gdb.error, ValueError) as e:
                    _log('no WeaponHurts return: %s' % str(e)[:60])
            return False

    class WaypointRet(gdb.FinishBreakpoint):
        def __init__(self, frame, uid):
            super().__init__(frame, internal=True)
            self.uid = uid

        def stop(self):
            gdb.execute('set $rax = %d' % self.uid)
            _log('WallHang ARRV/NEXT waypoint forced %d' % self.uid)
            return False

    class WaypointBp(gdb.Breakpoint):
        def stop(self):
            older = gdb.newest_frame().older()
            if older is None or 'WallHang' not in (older.name() or ''):
                return False
            STATE['wp'] = STATE.get('wp', 0) + 1
            path = os.path.join(os.environ['COV_OUT'], 'waypoint.txt')
            if STATE['wp'] % 2 == 0 and os.path.exists(path):
                try:
                    WaypointRet(gdb.newest_frame(), int(open(path).read()))
                except (gdb.error, ValueError) as e:
                    _log('no waypoint return: %s' % str(e)[:60])
            return False

    if os.environ.get('CP5_MODE', 'msg') == 'msg':
        MsgBp('CCommandoPirate::AcceptScriptMsg', internal=True)
        HurtsBp('CDamageVulnerability::WeaponHurts(CWeaponMode const&) const', internal=True)
    else:
        # CP5_MODE=states (the driver's states2): WallHang's ARRV/NEXT search (0x000073E8..
        # 0x00007414) answers a real waypoint on every other call.
        WaypointBp('CEntity::FindConnectedObject_if', internal=True)
