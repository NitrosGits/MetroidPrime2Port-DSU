# Grenchler (module 27) slice gren5: null/edge arms of the small functions, forced per call
# (tools/cov/force_ret.py). Guardian room driver, merge with the gren4 runs:
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 \
#     tools/cov/force.sh tools/cov/specs/grenchler_force5.py <out> 3dfd2249,08_swamp_grappleguardian_dark \
#     --con tools/cov/specs/grenchler_guardian.con --fast --frames 3200
CLASS = 'CGrenchler'
FORCE_RET = [
    ('PreRender null eye glow / null collision manager', 'CGrenchler::PreRender(', 3,
     [(0, [('$this->mEffectA.mEyeGlow.mGen.mItem', '0')]),
      (1, [('$this->mCollisionManager.mPtr', '0')])],
     'guardian PreRender without eye CElementGen, then without collision manager'),
    ('PreRenderAllViewports null collision manager', 'CGrenchler::PreRenderAllViewports(', 3,
     [(0, [('$this->mCollisionManager.mPtr', '0')])],
     'guardian without collision manager: bounds untouched'),
]
FORCE_INNER = [
    ('PreRenderAllViewports null actor', 'CGrenchler::PreRenderAllViewports(', 'CStateManager::GetObjectById(', 3,
     {1: [('$rax', '0')]}, 'collision actor lookup fails: joint skipped'),
]
CALLS = [('PullPlayer direct calls', 'CGrenchler::PullPlayer(',
          '((CGrenchler*)$rdi)->mIsGrappleGuardian == 0', [
    # player airborne, nothing normalisable: SetMoveState, no pull rotation
    ([], '$this->PullPlayer(0.016f, *$mgr, $this->mManeuverTarget)',
     [('CPlayer::IsOnGround(', 0), ('CVector3f::CanBeNormalized(', 0)]),
    # on ground, facing not normalisable; pull direction normalisable
    ([], '$this->PullPlayer(0.016f, *$mgr, $this->mManeuverTarget)',
     [('CPlayer::IsOnGround(', 1), ('CVector3f::CanBeNormalized(', [0, 1])]),
    # on ground, facing normalisable; pull direction not
    ([], '$this->PullPlayer(0.016f, *$mgr, $this->mManeuverTarget)',
     [('CPlayer::IsOnGround(', 1), ('CVector3f::CanBeNormalized(', [1, 0])]),
], 'PullPlayer with IsOnGround/CanBeNormalized stubbed'),
    ('Maneuver direct calls', 'CGrenchler::Maneuver(', '((CGrenchler*)$rdi)->mIsGrappleGuardian == 0', [
    # a message no case handles
    ([], '$this->Maneuver(*$mgr, 3, 0.016f)', []),
    # Update with a finished path that is not shagged: the other arm of the && chain
    ([('$this->mPathFindSearch.mCurWaypoint', '100')], '$this->Maneuver(*$mgr, 1, 0.016f)',
     [('CPatterned::PathShagged(', 0)]),
    ([], '$this->Maneuver(*$mgr, 1, 0.016f)', [('CPatterned::PathShagged(', 1)]),
    ], 'Maneuver with PathShagged/IsOver stubbed'),
    ('DestroyTail direct calls', 'CGrenchler::PreRender(', '((CGrenchler*)$rdi)->mIsGrappleGuardian == 0', [
    ([], '$this->DestroyTail(*$mgr)', []),
    ([('$this->mIsGrappleGuardian', '1')], '$this->DestroyTail(*$mgr)', []),
    ([], '$this->DestroyTail(*$mgr)', [('CPatterned::IsIngPossessed(', 1)]),
    ], 'DestroyTail as a normal enemy, as the guardian, and ing possessed'),
    ('Pursue direct calls', 'CGrenchler::Pursue(', '((CGrenchler*)$rdi)->mIsGrappleGuardian == 0', [
    ([('$this->mSubmerged', '!$this->mSubmerged')], '$this->Pursue(*$mgr, 1, 0.016f)', []),
    ([('$this->mIsGrappleGuardian', '1'), ('$this->mPredictedLeashTime', '6.f'),
      ('$this->mLastLeashTeleportTime', '-10.f')], '$this->Pursue(*$mgr, 1, 0.016f)', []),
    ([('$this->mIsGrappleGuardian', '1'), ('$this->mPredictedLeashTime', '0.f')],
     '$this->Pursue(*$mgr, 1, 0.016f)', []),
    ([('$this->mIsGrappleGuardian', '1'), ('$this->mPredictedLeashTime', '6.f'),
      ('$this->mLastLeashTeleportTime', '$this->mElapsedTime')], '$this->Pursue(*$mgr, 1, 0.016f)', []),
    ([('$this->mStuckCheckTimer', '1.f'), ('$this->mReflectInfo.mJumpEndTime', '-100.f')],
     '$this->Pursue(*$mgr, 1, 0.016f)',
     [('CGrenchler::IsNearPath(', 1), ('CGrenchler::FindJumpTarget(', 0)]),
    ([], '$this->Pursue(*$mgr, 3, 0.016f)', []),
    ([], '$this->Pursue(*$mgr, 1, 0.016f)',
     [('CGrenchler::IsNearPath(', 0), ('CGrenchler::FindJumpTarget(', 1)]),
    ([('$this->mStuckCheckTimer', '1.f'), ('$this->mReflectInfo.mJumpEndTime', '-100.f')],
     '$this->Pursue(*$mgr, 1, 0.016f)',
     [('CGrenchler::IsNearPath(', 1), ('CGrenchler::FindJumpTarget(', 1)]),
    ], 'Pursue: water toggle, guardian leash teleport arms, stuck check'),
]
WATER = ('*(unsigned short*)&$this->mWaterId', '459')  # u0459 (decimal uid), a02's CScriptWater in 01_swamp_entrance
JUMP = '$this->Jump(*$mgr, 1, 0.f)'
CALLS.append(('Jump direct calls', 'CGrenchler::PreRender(', '((CGrenchler*)$rdi)->mIsGrappleGuardian == 0', [
    ([], '$this->Jump(*$mgr, 3, 0.f)', []),
    # animation over: mLanded set
    ([('*(int*)&$this->mAnimationState', '3')], JUMP, []),
    # started submerged, now surfaced: water crossed; water found: splash
    ([('$this->mReflectInfo.mWaterCrossed', '0'), ('$this->mReflectInfo.mStartedSubmerged', '1'), WATER],
     JUMP, [('CGrenchler::IsDeeplySubmerged(', 0)]),
    # started submerged, still deep: not crossed
    ([('$this->mReflectInfo.mWaterCrossed', '0'), ('$this->mReflectInfo.mStartedSubmerged', '1'), WATER],
     JUMP, [('CGrenchler::IsDeeplySubmerged(', 1)]),
    # not started submerged, now deep: slowed, crossed
    ([('$this->mReflectInfo.mWaterCrossed', '0'), ('$this->mReflectInfo.mStartedSubmerged', '0'), WATER],
     JUMP, [('CGrenchler::IsDeeplySubmerged(', 1)]),
    # not started submerged, still shallow: not crossed
    ([('$this->mReflectInfo.mWaterCrossed', '0'), ('$this->mReflectInfo.mStartedSubmerged', '0')],
     JUMP, [('CGrenchler::IsDeeplySubmerged(', 0)]),
    # crossed already, water known
    ([('$this->mReflectInfo.mWaterCrossed', '1'), WATER], JUMP, []),
], 'Jump: default message, animation over, water crossing from submerged, splash'))
# F5=<substring of a CALLS label>: arm one call group per run (two armed groups at once lost the run)
_only = __import__('os').environ.get('F5')
if _only:
    CALLS = [c for c in CALLS if _only in c[0]]
    FORCE_RET, FORCE_INNER = [], []
