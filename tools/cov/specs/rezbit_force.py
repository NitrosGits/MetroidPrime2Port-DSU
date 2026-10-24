# Rezbit (REL 53) call-scoped forcing (tools/cov/force_ret.py: each write lasts one call, restored on
# return): the state-body and trigger arms the fight cannot reach. Same area/layers as rezbit.py.
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py MP2_CONSOLE_WAIT=120 \
#     tools/cov/force.sh tools/cov/specs/rezbit_force.py <out> 1baa96c2,02_cliff_reactorcore \
#     --con tools/drivers/rezbit_forced2.con --fast --frames 7000
# Merge its gcov dir with the natural runs (rezbit_states.con, rezbit_arms.con, tail21_rezbit*).
# x86-64 SysV entry registers: rdi this, rsi mgr, edx msg (state bodies) / ecx state (Death).
CLASS = 'CRezbit'
TID = '$this->mTargetId.value'
SELF = '$this->mUniqueId.value'
M = '$this->mData.'
FORCE_RET = [
    ('Death 205 state invalid', 'CRezbit::Death(', 2,
     [(1, [('!$ecx', '0xffffffff')])],
     'first Death call: the script state argument is kSS_InvalidState (the SendScriptMsgs skip)'),
    ('Alert 385/392/394', 'CRezbit::Alert(', 3,
     [(0, [('$this->mAlertDelay', '1.0e9')]),
      (1, [(TID, '0xffff')]),
      (2, [(TID, SELF)])],
     'phase 0: alert delay not elapsed; 1: no target object; 2: target is the Rezbit itself (zero toTarget, not magnitude safe)'),
    ('PathFind 444/446', 'CRezbit::PathFind(', 2,
     [(0, [('$this->mHasPathDestination', '0'), (TID, '0xffff')]),
      (1, [('$this->mHasPathDestination', '0'), (TID, SELF)])],
     'no path destination and: phase 0 no target object; 1 target is itself (zero toTarget)'),
    ('Strafe rez1', 'CRezbit::Strafe(', 1,
     [(0, [('$this->mRezState', '1')])],
     'derezzed state: the BreakDodge step and the derez strafe interval'),
    ('Strafe 469 no target', 'CRezbit::Strafe(', 2,
     [(0, [(TID, '0xffff')])],
     'no target object in the Step state'),
    ('Dead laser sfx', 'CRezbit::Dead(', 1,
     [(0, [('*(unsigned*)&$this->mLaserSfx', '0x1234')])],
     'a live laser sfx handle at death (SpawnLaser leaves none by then)'),
    ('SelectStrafeDirection', 'CRezbit::SelectStrafeDirection(', 3,
     [(0, [('$this->mRezState', '1')]),
      (1, [(M + 'unknown_0x94980a67', '-1.0')]),
      (2, [('$this->mRezState', '1'), (M + 'strafeDerezChance', '-1.0')])],
     'phase 0: derez chance; 1: roll misses (rez 0 interval); 2: derez, roll misses'),
    ('SetTargetDest', 'CRezbit::SetTargetDest(', 4,
     [(0, [(TID, '0xffff')]),
      (1, [('$this->mRezState', '1'), (M + 'virusMaxAttackDist', '1.0e9')]),
      (2, [(M + 'cuttingLaserMaxAttackDist', '0.0'), (M + 'energyBoltMaxAttackDist', '0.0')]),
      (3, [(M + 'cuttingLaserMaxAttackDist', '1.0e9'), (M + 'energyBoltMaxAttackDist', '1.0e9')])],
     'phase 0: no target; 1: derez, in range; 2: out of range; 3: in range (line of sight decides)'),
    ('RaiseShields', 'CRezbit::RaiseShields(', 3,
     [(0, [('$this->mShieldBroken', '1')]),
      (1, [('$this->mShieldActorId.value', '0xffff')]),
      (2, [('*(unsigned*)&$this->mShieldSfx', '0x1234')])],
     'phase 0: shield broken; 1: no shield actor; 2: shield sfx already playing'),
]

# Triggers: driver rezbit_forced2.con calls each with `trigcall` many times; the outer phase sets a
# base that makes every condition true and breaks one of them; the callee answers (team manager,
# camera, player rez state) alternate with their own period.
NOSH = ('$this->mShieldActorId.value', '0xffff')
FORCE_RET += [
    ('ShouldBecomeDeRezzed', 'CRezbit::ShouldBecomeDeRezzed(', 6,
     [(0, [('$this->mAttackKind', '-1'), ('$this->mDerezHealth', '1.0e9'), ('$this->mAlert', '1'), ('$this->mRezState', '0')]),
        (1, [('$this->mAttackKind', '0'), ('$this->mDerezHealth', '1.0e9'), ('$this->mAlert', '1'), ('$this->mRezState', '0')]),
        (2, [('$this->mAttackKind', '-1'), ('$this->mDerezHealth', '-1.0'), ('$this->mAlert', '1'), ('$this->mRezState', '0')]),
        (3, [('$this->mAttackKind', '-1'), ('$this->mDerezHealth', '1.0e9'), ('$this->mAlert', '0'), ('$this->mRezState', '0')]),
        (4, [('$this->mAttackKind', '-1'), ('$this->mDerezHealth', '1.0e9'), ('$this->mAlert', '1'), ('$this->mRezState', '1')])],
     'phase 0: every condition true; 1: attack kind set; 2: health above derez health; 3: not alert; 4: already derezzed'),
    ('ShouldStrafe', 'CRezbit::ShouldStrafe(', 5,
     [(0, [('$this->mHasPathDestination', '0'), ('$this->mStrafeTimer', '0.0'), ('$this->mLineOfSight.mClearTime', '3.0'), NOSH]),
      (1, [('$this->mHasPathDestination', '1'), ('$this->mStrafeTimer', '0.0'), ('$this->mLineOfSight.mClearTime', '3.0')]),
      (2, [('$this->mHasPathDestination', '0'), ('$this->mStrafeTimer', '1.0e9'), ('$this->mLineOfSight.mClearTime', '3.0')]),
      (3, [('$this->mHasPathDestination', '0'), ('$this->mStrafeTimer', '0.0'), ('$this->mLineOfSight.mClearTime', '0.0')]),
      (4, [('$this->mHasPathDestination', '0'), ('$this->mStrafeTimer', '0.0'), ('$this->mLineOfSight.mClearTime', '3.0'),
           ('$this->mShieldActorId.value', SELF)])],
     'phase 0: all true, no shield; 1: path destination; 2: strafe timer running; 3: line of sight not clear; 4: shield actor active'),
]
LASER = [('$this->mAttackTimer', '0.0'), ('$this->mAttackSelection', '1'), ('$this->mLaserId.value', '0xffff'), NOSH,
         ('$this->mTeamAiMgrId.value', '0xffff'), (M + 'cuttingLaserMinAttackDist', '0.0'), (M + 'cuttingLaserMaxAttackDist', '1.0e9')]
BOLT = [('$this->mAttackTimer', '0.0'), ('$this->mAttackSelection', '2'), NOSH, ('$this->mTeamAiMgrId.value', '0xffff'),
        (M + 'energyBoltMinAttackDist', '0.0'), (M + 'energyBoltMaxAttackDist', '1.0e9')]
VIRUS = [('$this->mVirusTimer', '0.0'), NOSH, (TID, '((CStateManager*)$rsi)->mPlayers[0]->mUniqueId.value'), ('$this->mTeamAiMgrId.value', '0xffff'),
         (M + 'virusMinAttackDist', '0.0'), (M + 'virusMaxAttackDist', '1.0e9')]


def variants(base, pre, mn, mx):
    """base writes with one entry overridden per phase (later writes win)."""
    ph = [(0, base)]
    ph.append((1, base + [(TID, '0xffff')]))
    ph.append((2, base + [('$this->mTeamAiMgrId.value', SELF)]))
    ph.append((3, base + [('$this->mTeamAiMgrId.value', SELF)]))
    ph.append((4, base + [(M + mx, '0.0')]))
    ph.append((5, base + [(M + mn, '1.0e9')]))
    ph.append((6, base + [('$this->mShieldActorId.value', SELF)]))
    ph.append((7, base + [('$this->mAttackTimer', '1.0e9')]))
    return ph


FORCE_RET += [
    ('ShouldLaserAttack', 'CRezbit::ShouldLaserAttack(', 11,
     variants(LASER, 0, 'cuttingLaserMinAttackDist', 'cuttingLaserMaxAttackDist')
     + [(8, LASER + [('$this->mAttackSelection', '2')]), (9, LASER + [('$this->mLaserId.value', SELF)])],
     'phase 0 all true; 1 no target; 2/3 team manager set (answer alternates); 4 beyond max; 5 inside min; 6 shield active; 7 timer; 8 other selection; 9 laser running'),
    ('ShouldEnergyBoltAttack', 'CRezbit::ShouldEnergyBoltAttack(', 10,
     variants(BOLT, 0, 'energyBoltMinAttackDist', 'energyBoltMaxAttackDist')
     + [(8, BOLT + [('$this->mAttackSelection', '1')])],
     'phase 0 all true; 1 no target; 2/3 team manager set; 4 beyond max; 5 inside min; 6 shield active; 7 timer; 8 other selection'),
    ('ShouldVirusAttack', 'CRezbit::ShouldVirusAttack(', 10,
     variants(VIRUS, 0, 'virusMinAttackDist', 'virusMaxAttackDist')
     + [(8, VIRUS + [('!((CStateManager*)$rsi)->mPlayers[0]->mSpawnedMorphBallState', '1')]),
        (9, VIRUS + [('!((CStateManager*)$rsi)->mPlayers[0]->mSpawnedMorphBallState', '0')])],
     'phase 0 all true; 1 no target; 2/3 team manager set; 4 beyond max; 5 inside min; 6/7 shield/timer (not read); 8 player morphed (kept); 9 player unmorphed again'),
]
FORCE_INNER = [
    ('Death HasState false', 'CRezbit::Death(', 'CGenericFSM2State<CPatterned>::HasState(', 2,
     {0: [('$rax', '0')]}, 'the state machine has no state at death (202:1)'),
]
for fn in ('ShouldLaserAttack', 'ShouldEnergyBoltAttack', 'ShouldVirusAttack'):
    FORCE_INNER += [
        (fn + ' team', 'CRezbit::%s(' % fn, 'CScriptTeamAiMgr::CanStartAttack(', 2,
         {0: [('$rax', '1')], 1: [('$rax', '0')]}, 'team manager grants / refuses the attack'),
        (fn + ' camera', 'CRezbit::%s(' % fn, 'CCameraManager::IsInCinematicCamera(', 4,
         {0: [('$rax', '1')]}, 'cinematic camera running'),
    ]
FORCE_INNER += [
    ('ShouldVirusAttack rez', 'CRezbit::ShouldVirusAttack(', 'CPlayer::GetRezbitState(', 7,
     {0: [('$rax', '1')]}, 'player already rezbit-infected'),
]
