# Rezbit (REL 53) rez4 call-scoped forcing for the leaf/trigger arms rez3 left: Listen, Render,
# DoUserAnimEvent, Think, AcceptScriptMsg, ShouldVirusAttack, StartVirusAttack, FireEnergyBolt,
# UpdateLaser, SetupLaserTargets, UpdatePatrolMovement. Driver tools/drivers/rezbit_forced3.con.
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py MP2_CONSOLE_WAIT=120 \
#     tools/cov/force.sh tools/cov/specs/rezbit_force2.py <out> 1baa96c2,02_cliff_reactorcore \
#     --con tools/drivers/rezbit_forced3.con --fast --frames 7000
# x86-64 SysV: rdi this, rsi mgr, rdx 3rd arg, ecx 4th arg.
CLASS = 'CRezbit'
TID = '$this->mTargetId.value'
SELF = '$this->mUniqueId.value'
M = '$this->mData.'
NOSH = ('$this->mShieldActorId.value', '0xffff')
PL = '((CStateManager*)$rsi)->mPlayers[0]'
FORCE_RET = [
    ('Listen', 'CRezbit::Listen(', 5,
     [(0, [('$this->mAlive', '0')]),
      (1, [('!$ecx', '1')]),
      (2, [('$this->mAlert', '1')]),
      (3, [('$this->mAlert', '0'), (M + 'hearingRadius', '0.0')]),
      (4, [('$this->mAlert', '0'), (M + 'hearingRadius', '1.0e9')])],
     'dead; other noise type; already alert; out of radius; in radius'),
    ('UpdateLaser', 'CRezbit::UpdateLaser(', 2,
     [(0, [('$this->mLaserId.value', '0xffff')])], 'no laser object'),
    ('SetupLaserTargets', 'CRezbit::SetupLaserTargets(', 3,
     [(0, [(TID, '0xffff')]), (1, [(TID, SELF)]), (2, [])],
     'no target; target is itself (unsafe perpendicular); normal'),
    ('AcceptScriptMsg shield', 'CRezbit::AcceptScriptMsg(', 4,
     [(0, [('$this->mActive', '0')]), (1, [NOSH]),
      (2, [('$this->mActive', '0'), NOSH]), (3, [])],
     'Activate while inactive; no shield actor; both; plain'),
    ('FireEnergyBolt', 'CRezbit::FireEnergyBolt(', 7,
     [(0, [(TID, '0xffff')]),
      (1, [('$this->mBoltCount', '1'), ('((float*)&$this->mLastBoltDirection)[0]', '1.0'),
           ('((float*)&$this->mLastBoltDirection)[1]', '0.0'), ('((float*)&$this->mLastBoltDirection)[2]', '0.0')]),
      (2, [('$this->mBoltCount', '1'), ('((float*)&$this->mLastBoltDirection)[0]', '-1.0'),
           ('((float*)&$this->mLastBoltDirection)[1]', '0.0'), ('((float*)&$this->mLastBoltDirection)[2]', '0.0')]),
      (3, [('$this->mBoltCount', '1'), ('((float*)&$this->mLastBoltDirection)[0]', '0.0'),
           ('((float*)&$this->mLastBoltDirection)[1]', '1.0'), ('((float*)&$this->mLastBoltDirection)[2]', '0.0')]),
      (4, [('$this->mBoltCount', '1'), ('((float*)&$this->mLastBoltDirection)[0]', '0.0'),
           ('((float*)&$this->mLastBoltDirection)[1]', '0.0'), ('((float*)&$this->mLastBoltDirection)[2]', '0.0')]),
      (5, [(TID, SELF)]),
      (6, [('$this->mBoltCount', '2')])],
     'no target; last bolt direction variants; self target; second bolt'),
]
VIRUS = [('$this->mVirusTimer', '0.0'), NOSH, (TID, PL + '->mUniqueId.value'), ('$this->mTeamAiMgrId.value', '0xffff'),
         (M + 'virusMinAttackDist', '0.0'), (M + 'virusMaxAttackDist', '1.0e9')]
FORCE_RET += [
    ('DoUserAnimEvent', 'CRezbit::DoUserAnimEvent(', 3,
     [(0, [('!$ecx', '33')]), (1, [('!$ecx', '34')]), (2, [('!$ecx', '17')])],
     'rewrite the event type'),
    ('ShouldVirusAttack', 'CRezbit::ShouldVirusAttack(', 6,
     [(0, VIRUS + [('!' + PL + '->mSpawnedMorphBallState', '0')]),
      (1, VIRUS + [('!' + PL + '->mSpawnedMorphBallState', '1')]),
      (2, VIRUS + [(M + 'virusMinAttackDist', '1.0e9')]),
      (3, VIRUS + [(M + 'virusMaxAttackDist', '0.0')]),
      (4, VIRUS + [(TID, '0xffff')]),
      (5, VIRUS + [('$this->mVirusTimer', '1.0e9')])],
     'unmorphed / morphed / inside min / beyond max / no target / timer'),
    ('StartVirusAttack', 'CRezbit::StartVirusAttack(', 6,
     [(0, [(TID, PL + '->mUniqueId.value'), ('!' + PL + '->mSpawnedMorphBallState', '0')]),
      (1, [(TID, PL + '->mUniqueId.value'), ('!' + PL + '->mSpawnedMorphBallState', '1')]),
      (2, [(TID, '0xffff')]),
      (3, [(TID, SELF)]),
      (4, [(TID, PL + '->mUniqueId.value')]),
      (5, [])],
     'player unmorphed / morphed / no target / self / plain'),
    ('UpdatePatrolMovement', 'CRezbit::UpdatePatrolMovement(', 3,
     [(0, [('$this->mHasPathDestination', '0')]), (1, [('$this->mHasPathDestination', '1')]), (2, [])],
     'patrol variants'),
]
FORCE_INNER = [
    ('StartVirusAttack camera', 'CRezbit::StartVirusAttack(', 'CCameraManager::IsInCinematicCamera(', 3,
     {0: [('$rax', '1')]}, 'cinematic camera'),
    ('StartVirusAttack ray', 'CRezbit::StartVirusAttack(', 'CStateManager::RayCollideWorld(', 2,
     {0: [('$rax', '1')], 1: [('$rax', '0')]}, 'ray blocked / clear'),
    ('ShouldVirusAttack rezbit state', 'CRezbit::ShouldVirusAttack(', 'CPlayer::GetRezbitState(', 1,
     {0: [('$rax', '0')]}, 'player not infected'),
    ('UpdatePatrolMovement step', 'CRezbit::UpdatePatrolMovement(', 'CPatterned::FindBestStepDirection(', 4,
     {0: [('$rax', '0')], 1: [('$rax', '1')], 2: [('$rax', '2')], 3: [('$rax', '3')]}, 'step directions'),
]

# REZ_SKIP=label,label drops entries (hot per-frame functions make a gdb stop per frame: split runs).
import os
_skip = [x for x in os.environ.get('REZ_SKIP', '').split(',') if x]
FORCE_RET = [e for e in FORCE_RET if not any(k in e[0] for k in _skip)]
FORCE_INNER = [e for e in FORCE_INNER if not any(k in e[0] for k in _skip)]
