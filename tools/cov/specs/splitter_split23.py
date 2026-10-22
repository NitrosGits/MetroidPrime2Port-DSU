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

# fn_75_E1D0: every chassis of the layer is in a team, so phase 2 takes it out (0xCBE invalid:
# 0xE220 skips the query).  Phase 0 puts the chassis in a team (its own id, so the real query
# says no) and the return override below makes the team's CanStartAttack (0xE244) answer yes.
_SPIN.append((2, [('$this->mFlagsD40', '$this->mFlagsD40 | 0x80'), ('$this->mSpinPenaltyTimer', '0.f'),
                  ('$this->mTeamId.value', '0xffff')]))
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
# The chassis objects: $split23_c532 / $split23_c521 are recorded at their Think (below).
C532 = '((CSplitterMainChassis*)$split23_c532)'
C521 = '((CSplitterMainChassis*)$split23_c521)'
_DOCK += [
    # 15: u0532 as it is (alive and active with its head: 0x5848 bne; after the kill at f6000 it
    # is dead: 0x5828); 20: u0521 with no head (0xC6C invalid: true); 25: u0521 inactive (the
    # entity's active bit: 0x5834); 30: u0521 not alive (0x5828).
    (15, [(HD + '->mCheckedChassisId.value', '0x532')]),
    (20, [(HD + '->mCheckedChassisId.value', '0x521'), (C521 + '->mCommandModuleId.value', '0xffff')]),
    (25, [(HD + '->mCheckedChassisId.value', '0x521'), ('((CEntity*)$split23_c521)->mActive', '0')]),
    (30, [(HD + '->mCheckedChassisId.value', '0x521'), (C521 + '->mAlive', '0')]),
]
_COLL = [
    # 0: state 1 with the bit set and the collision actor's HealthInfo overridden (RET_OVERRIDE)
    # to a zeroed CHealthInfo: the health 0.0f takes the shield down (0x3110 falls through).
    (0, [(HD + '->mCollisionState', '1'), (HD + '->mAutoDestructFlags', HD + '->mAutoDestructFlags | 8')]),
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
    # fn_75_B2A0's gates in front of fn_75_91E4: the placements of 914f1381 have neither flag bit
    # 0x10 nor 0x20 (0xB2E4-0xB2F4), so every call sets both and clears the two timers (0xB2F8,
    # 0xB30C); the missile search and the orbit test stay the game's.
    ('fn_75_B2A0', 'CSplitterMainChassis::FindBestDodgeDirection(', 1,
     [(0, [('$this->mData.mUnknownCEF5C2FE', '$this->mData.mUnknownCEF5C2FE | 0x30'),
           ('$this->mTimerD34', '0.f'), ('$this->mDodgeTimer', '0.f')])], 'dodge gates'),
    ('fn_75_50B0', 'CSplitterCommandModule::FaceTarget(', P, _FACE, 'target, magnitude, dt'),
    ('fn_75_57F0', 'CSplitterCommandModule::HasDockingTarget(', P, _DOCK, 'the checked chassis'),
    ('fn_75_3004', 'CSplitterCommandModule::PortUpdateCollision(', P, _COLL, 'actor, manager, state'),
    # fn_75_A0A0 PortCollisionActorDamage (the con's sends at f2300-f2340 and f9000-f9060, a shot
    # in flight): call 1 (no sender) is the null-actor arm; 2: a dead chassis (0xA0D8); 11 (the
    # f9004 send, after the docking checks): health 0.0f, so the hit kills it (0xA2A8 falls
    # through to Death).  The weapon arm comes from the GetLastTouchedObject override below
    # (calls 3-6, a shot in flight); calls 7-8 (the shots are gone) are the plain hit.
    ('fn_75_A0A0', 'CSplitterMainChassis::PortCollisionActorDamage(', 12,
     [(2, [('$this->mAlive', '0')]), (11, [('$this->mHealthInfo.mHealth', '0.f')])], 'dead, lethal'),
]
# gcov gives the arms; the gdb counters would only slow the run down.
SPEC = []

# (caller prefix, callee prefix, period, [values]): on every period-th call of the caller the
# next call of the callee made from it returns values[k % len(values)] (k counts the overrides).
# The callee's breakpoint exists only between the caller's entry and that call, so the host's
# other callers (every actor asks FindBestStepDirection) cost nothing.
RET_OVERRIDE = [
    # fn_75_E1D0 0xE244: CanStartAttack says yes for the forced team (phase 0 above).
    ('CSplitterMainChassis::ShouldSpinAttack(', 'CScriptTeamAiMgr::CanStartAttack(', P, [1]),
    # fn_75_986C 0x9940: each step direction (0x9958-0x9998: 0, 1, 2, 3, 4 and -1).
    ('CSplitterMainChassis::PortUpdatePath(', 'CPatterned::FindBestStepDirection(', 5,
     [0, 1, 2, 3, 4, -1]),
    # fn_75_3004 0x30F8: the health block is CVector3f::sZeroVector (12 zero bytes: the host
    # CHealthInfo's health at +4 reads 0.0f); only read, never written.
    ('CSplitterCommandModule::PortUpdateCollision(', 'CCollisionActor::HealthInfo(', P,
     ['(long)&CVector3f::sZeroVector']),
    # fn_75_986C 0x98A8-0x98C0: a separation with x 0.0f and y, then only z, not zero (the
    # vector comes back in xmm0 {x, y} and xmm1 {z}); 0x994C: an unsafe move vector.
    ('CSplitterMainChassis::PortUpdatePath(', 'CSplitterMainChassis::PortSeparationVector(', 7,
     ['cmd:set $xmm0.v4_float[0] = 0\nset $xmm0.v4_float[1] = 1\nset $xmm1.v4_float[0] = 0',
      'cmd:set $xmm0.v4_float[0] = 0\nset $xmm0.v4_float[1] = 0\nset $xmm1.v4_float[0] = 1']),
    ('CSplitterMainChassis::PortUpdatePath(', 'CVector3f::IsMagnitudeSafe(', 11, [0]),
    # fn_75_A0A0 0xA104: the actor last touched a live CWeapon (a CEnergyProjectile in flight,
    # $split23_wid, recorded at its Think and cleared in ~CWeapon); every second call keeps the
    # game's answer (no weapon).
    ('CSplitterMainChassis::PortCollisionActorDamage(', 'CCollisionActor::GetLastTouchedObject(', 2,
     ['($split23_wid != 0xffff) ? $split23_wid : $rax']),
]

if gdb.convenience_variable('split23_ret') is None:
    gdb.set_convenience_variable('split23_ret', 1)

    def _log(msg):
        with open(os.path.join(os.environ['COV_OUT'], 'forced.log'), 'a') as fh:
            fh.write(msg + '\n')

    def _addrs(prefix):
        out = []
        for ln in open(os.environ['COV_NM']):
            p = ln.rstrip('\n').split('\t')
            if len(p) == 2:
                f = p[0].split()
                if len(f) == 3 and f[1] in 'tTW' and p[1].startswith(prefix) and '.cold' not in p[1] \
                        and 'clone' not in p[1]:
                    out.append(f[2])
        return out

    class _RetFinish(gdb.FinishBreakpoint):
        def __init__(self, frame, value, label):
            super().__init__(frame, internal=True)
            self.value, self.label = value, label

        def stop(self):
            if isinstance(self.value, str) and self.value.startswith('cmd:'):
                for c in self.value[4:].split('\n'):
                    gdb.execute(c)
            else:
                gdb.execute('set $rax = %s' % (self.value,))
            _log('%s: returned %s' % (self.label, self.value))
            return False

    class _CalleeBp(gdb.Breakpoint):
        def __init__(self, where, owner):
            super().__init__(where, internal=True)
            self.owner = owner
            self.enabled = False

        def stop(self):
            # Only a direct call from the caller (PortSeparationVector asks IsMagnitudeSafe too).
            try:
                up = gdb.newest_frame().older()
                if up is not None and up.name() and \
                        up.name().split('(')[0] != self.owner.caller.split('(')[0]:
                    return False
            except gdb.error:
                pass
            self.enabled = False
            o = self.owner
            v = o.values[o.k % len(o.values)]
            o.k += 1
            try:
                _RetFinish(gdb.newest_frame(), v, o.label)
            except (gdb.error, ValueError) as e:
                _log('%s: no finish (%s)' % (o.label, str(e)[:60]))
            return False

    class _CallerBp(gdb.Breakpoint):
        def __init__(self, where, callees, period, values, label, caller):
            super().__init__(where, internal=True)
            self.period, self.values, self.label, self.n, self.k = period, values, label, 0, 0
            self.caller = caller
            self.callees = [_CalleeBp('*' + m, self) for m in callees]

        def stop(self):
            self.n += 1
            for c in self.callees:
                c.enabled = self.n % self.period == 0
            return False

    class _RecordBp(gdb.Breakpoint):
        # Records each chassis's address under $split23_c<uid> (the forces above write through it).
        def stop(self):
            this = int(gdb.parse_and_eval('$rdi'))
            uid = int(gdb.parse_and_eval('((CSplitterMainChassis*)%d)->mUniqueId.value' % this))
            name = 'split23_c%x' % uid
            if gdb.convenience_variable(name) is None:
                gdb.set_convenience_variable(name, this)
                _log('recorded %s = %#x' % (name, this))
            return False

    for _m in _addrs('CSplitterMainChassis::Think('):
        _RecordBp('*' + _m, internal=True)

    gdb.set_convenience_variable('split23_wid', 0xffff)

    class _WeaponBp(gdb.Breakpoint):
        # The live projectile for the GetLastTouchedObject override: set at its Think, cleared
        # when that weapon is destroyed.
        def __init__(self, where, alive):
            super().__init__(where, internal=True)
            self.alive = alive

        def stop(self):
            uid = int(gdb.parse_and_eval('((CEntity*)$rdi)->mUniqueId.value'))
            if self.alive:
                gdb.set_convenience_variable('split23_wid', uid)
            elif int(gdb.convenience_variable('split23_wid')) == uid:
                gdb.set_convenience_variable('split23_wid', 0xffff)
            return False

    for _m in _addrs('CEnergyProjectile::Think('):
        _WeaponBp('*' + _m, True)
    for _m in _addrs('CWeapon::~CWeapon('):
        _WeaponBp('*' + _m, False)

    for _caller, _callee, _period, _values in RET_OVERRIDE:
        _cs = _addrs(_callee)
        for _m in _addrs(_caller):
            _CallerBp('*' + _m, _cs, _period, _values, _callee.split('(')[0], _caller)
        if not _cs:
            _log('ret override: no symbol for %s' % _callee)
