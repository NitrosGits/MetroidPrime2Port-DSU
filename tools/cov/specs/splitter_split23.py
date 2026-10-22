# Splitter (module 75) split23 lane: the chassis's spin trigger's team arm, the head's FaceTarget,
# HasDockingTarget and collision update, the chassis's path update.  Run with
# COV_GDB_SCRIPT=tools/cov/force_ret.py and BUILD=build-port-cov (every write lasts one call):
#
#   COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/splitter_split23.py <out> 1baa96c2,914f1381 \
#     --con tools/cov/specs/splitter_split23.con --fast --frames 9000
#
# Besides force_ret.py's member writes this spec sets a callee's return value (RET_OVERRIDE
# below): a gdb FinishBreakpoint at the callee's entry, armed only when the caller is the named
# body, writes $rax when the callee returns.  The team query and the step direction are host
# (DOL) calls, so no member write can steer them.
import os
import gdb

_S22 = os.path.join(os.path.dirname(os.path.abspath(os.environ['COV_SPEC'])), 'splitter_split22.py')
exec(compile(open(_S22).read(), _S22, 'exec'))

HD = '((CSplitterCommandModule*)$this)'
P = 40

# fn_75_E1D0: phase 0 puts the chassis in a team (its own id, so the real query says no) and the
# return override below makes the team's CanStartAttack (0xE244) answer yes: 0xE24C falls through
# to the target lookup.
_SPIN.append((0, [('$this->mFlagsD40', '$this->mFlagsD40 | 0x80'), ('$this->mSpinPenaltyTimer', '0.f'),
                  ('$this->mTeamId.value', '$this->mUniqueId.value'),
                  ('$this->mData.mSpinAttackMinAttackRange', '0.f'),
                  ('$this->mData.mSpinAttackMaxAttackRange', '100.f')]))

_FACE = [
    # fn_75_50B0 FaceTarget (update only, $rdx == 1): 5: the target 0xEFE invalid (0x512C);
    # 10: the target is the head itself (a zero offset: 0x5178 IsMagnitudeSafe fails);
    # 15: a zero frame time (0x5110 ble).
    (5, [(HD + '->mSyncedId.value', '$rdx == 1 ? 0xffff : ' + HD + '->mSyncedId.value')]),
    (10, [(HD + '->mSyncedId.value', '$rdx == 1 ? ' + HD + '->mUniqueId.value : ' + HD + '->mSyncedId.value')]),
    (15, [('$xmm0.v4_float[0]', '0')]),
]
_DOCK = [
    # fn_75_57F0 HasDockingTarget: 5: the id 0xEFC invalid (0x581C, the cast's null arm); 10: the
    # head's own id (not a chassis: the same arm); 15/20: the two chassis (CHASSIS_IDS, from the
    # run's `objs splitter`; alive, active, with or without a head).
    (5, [(HD + '->mCheckedChassisId.value', '0xffff')]),
    (10, [(HD + '->mCheckedChassisId.value', HD + '->mUniqueId.value')]),
]
CHASSIS_IDS = []  # filled in after the first run
for i, cid in enumerate(CHASSIS_IDS):
    _DOCK.append((15 + 5 * i, [(HD + '->mCheckedChassisId.value', str(cid))]))
_COLL = [
    # fn_75_3004 PortUpdateCollision: 5: no collision actor (0xF00 invalid, 0x3094);
    # 10: no manager (0x30A0); 15: state 1 with 0xF6A's 0x08 set (the health test 0x3108);
    # 20: state 2 with the bit clear (0x30E4 straight to the shield-down block);
    # 25: state 0, the bit set, the timer at 0 after a 1 (the shield goes up as 2);
    # 30: the same after a 0 (up as 1); 35: state 3, the bit set, the timer 5 (0x3158 fails).
    (5, [(HD + '->mSoundTargetId.value', '0xffff')]),
    (10, [(HD + '->mCollisionActorManager', '0')]),
    (15, [(HD + '->mCollisionState', '1'), (HD + '->mAutoDestructFlags', HD + '->mAutoDestructFlags | 8')]),
    (20, [(HD + '->mCollisionState', '2'), (HD + '->mAutoDestructFlags', HD + '->mAutoDestructFlags & 0xf7')]),
    (25, [(HD + '->mCollisionState', '0'), (HD + '->mAutoDestructFlags', HD + '->mAutoDestructFlags | 8'),
          (HD + '->mCountdownTimerA', '0.f'), (HD + '->mPreviousCollisionState', '1')]),
    (30, [(HD + '->mCollisionState', '0'), (HD + '->mAutoDestructFlags', HD + '->mAutoDestructFlags | 8'),
          (HD + '->mCountdownTimerA', '0.f'), (HD + '->mPreviousCollisionState', '0')]),
    (35, [(HD + '->mCollisionState', '3'), (HD + '->mAutoDestructFlags', HD + '->mAutoDestructFlags | 8'),
          (HD + '->mCountdownTimerA', '5.f')]),
]
FORCE_RET += [
    ('fn_75_50B0', 'CSplitterCommandModule::FaceTarget(', P, _FACE, 'target, magnitude, dt'),
    ('fn_75_57F0', 'CSplitterCommandModule::HasDockingTarget(', P, _DOCK, 'the checked chassis'),
    ('fn_75_3004', 'CSplitterCommandModule::PortUpdateCollision(', P, _COLL, 'actor, manager, state'),
]
SPEC += [
    ('50B0 FaceTarget', 'CSplitterCommandModule::FaceTarget(', ['$rdx'], 'calls max=2000'),
    ('57F0 HasDockingTarget', 'CSplitterCommandModule::HasDockingTarget(', ['$rdi != 0'], 'ret'),
    ('986C PortUpdatePath', 'CSplitterMainChassis::PortUpdatePath(', ['$rdi != 0'], 'calls max=2000'),
    ('91E4 PortFindDodgeDirection', 'CSplitterMainChassis::PortFindDodgeDirection(', ['$rdi != 0'], 'calls'),
    ('A0A0 PortCollisionActorDamage', 'CSplitterMainChassis::PortCollisionActorDamage(', ['$rdi != 0'], 'calls'),
]

# (callee prefix, caller prefix, period, [values]): the n-th call of the callee from the caller
# returns values[(n // period) % len(values)] on every period-th call (n % period == 0).
RET_OVERRIDE = [
    # fn_75_E1D0 0xE244: only the forced team (the chassis's own id, edx == ecx) is overridden.
    ('CScriptTeamAiMgr::CanStartAttack(', 'CSplitterMainChassis::ShouldSpinAttack(', 1, [1]),
    # fn_75_986C 0x9940: each step direction (0x9958-0x9998: 0, 1, 2, 3, 4 and -1).
    ('CPatterned::FindBestStepDirection(', 'CSplitterMainChassis::PortUpdatePath(', 7,
     [0, 1, 2, 3, 4, -1]),
]

if gdb.convenience_variable('split23_ret') is None:
    gdb.set_convenience_variable('split23_ret', 1)
    import os as _os

    def _log(msg):
        with open(_os.path.join(_os.environ['COV_OUT'], 'forced.log'), 'a') as fh:
            fh.write(msg + '\n')

    class _RetFinish(gdb.FinishBreakpoint):
        def __init__(self, frame, value, label):
            super().__init__(frame, internal=True)
            self.value, self.label = value, label

        def stop(self):
            gdb.execute('set $rax = %d' % self.value)
            _log('%s: returned %d' % (self.label, self.value))
            return False

    class _RetBp(gdb.Breakpoint):
        def __init__(self, where, caller, period, values, team):
            super().__init__(where, internal=True)
            self.caller, self.period, self.values, self.team, self.n = caller, period, values, team, 0

        def stop(self):
            try:
                older = gdb.newest_frame().older()
                name = older.name() if older else ''
            except gdb.error:
                return False
            if not name or not name.startswith(self.caller):
                return False
            if self.team and (int(gdb.parse_and_eval('$edx')) & 0xffff) != (int(gdb.parse_and_eval('$ecx')) & 0xffff):
                return False
            self.n += 1
            if self.n % self.period:
                return False
            v = self.values[(self.n // self.period) % len(self.values)]
            try:
                _RetFinish(gdb.newest_frame(), v, self.caller)
            except (gdb.error, ValueError) as e:
                _log('ret override: %s' % str(e)[:60])
            return False

    _syms = []
    for _ln in open(_os.environ['COV_NM']):
        _p = _ln.rstrip('\n').split('\t')
        if len(_p) == 2:
            _f = _p[0].split()
            if len(_f) == 3 and _f[1] in 'tTW':
                _syms.append((_f[2], _p[1]))
    for _callee, _caller, _period, _values in RET_OVERRIDE:
        _hits = [m for m, d in _syms if d.startswith(_callee) and '.cold' not in d and 'clone' not in d]
        if not _hits:
            _log('ret override: no symbol for %s' % _callee)
        for _m in _hits:
            _RetBp('*' + _m, _caller, _period, _values, 'CanStartAttack' in _callee)
