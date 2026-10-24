# Grenchler (module 27) gren7: UpdateEffects C2EC forced arms (own run: the other force7 groups act on the first Grenchler
# and leave the console ids unusable). Driver tools/cov/specs/grenchler_gren7e.con (an electric beam + swoosh on u05c4 first).
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 tools/cov/force.sh \
#     tools/cov/specs/grenchler_force7e.py <out> 3dfd2249,08_swamp_grappleguardian_dark --con tools/cov/specs/grenchler_gren7e.con --fast --frames 700
CLASS = 'CGrenchler'
FORCE_RET = []
CALLS = []

# ---- UpdateEffects C2EC (guardian room: the swoosh generators come from `gren.swoosh 3`)
_UE = '$this->UpdateEffects(0.016f, *$mgr)'
_GUARD = [('$this->mIsGrappleGuardian', '1')]
_PID = '*(unsigned short*)&$this->mEffectA.mBeamProjectileId'
CALLS.append(('UpdateEffects direct calls', 'CGrenchler::PreThink(',
          '((CGrenchler*)$rdi)->mIsGrappleGuardian == 1 && ((CGrenchler*)$rdi)->mEffectC.mSwoosh.mGen.mItem != 0', [
    ([('$this->mEffectA.mSurfaceRing.mGen.mItem', '0'), ('$this->mEffectD.mWaterSplash.mGen.mItem', '0')], _UE, []),   # no splash / ring generators
    ([], '$this->UpdateEffects(0.0f, *$mgr)', [('CGrenchler::IsWadingInWater(', 1)]),                                  # wading with dt == 0
    ([(_PID, '64515')], _UE, []),                                                                                        # projectile id with no object
    ([(_PID, '*(unsigned short*)&$this->mUniqueId'), ('$this->mActive', '0')], _UE, []),                                # projectile entity inactive
    ([('$this->mEffectC.mSoundHandle.mID', '5')], _UE, []),                                                              # swoosh sound handle live
    ([('$this->mEffectC.mSoundHandle.mID', '0')], _UE, []),
    ([('$this->mEffectC.mTraceState', '2'), ('$this->mCurrentAction', '0')], _UE, []),                                  # beam stopped on a wall
    ([('$this->mEffectC.mTraceState', '3'), ('$this->mCurrentAction', '0')], _UE, []),
    ([], _UE, [('CVector3f::CanBeNormalized(', 0)]),                                                                     # beam particle step not normalisable
    ([('$this->mBodyController.mPtr->mFrozen', '1')], _UE, []),                                                          # frozen: StopElectricBeam
    ([('$this->mElectricSfx.mID', '7')] + _GUARD, _UE, []),                                                              # guardian, not electric, live sound
    ([('$this->mTailDestroyed', '1'), ('$this->mAlive', '0')] + _GUARD, _UE, []),                                        # guardian, tail gone but dead
    ([('$this->mEffectA.mEyeGlow.mGen.mItem', '0')] + _GUARD, _UE, []),                                                  # guardian without eye glow
], 'UpdateEffects: no generators, dt 0 wading, dead projectile ids, sound handles, wall trace states, frozen, guardian sound / death / eye glow'))
