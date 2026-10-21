# The arms of the fourteen bodies the helpers2 lane still had open, with the retail offsets.
# Run: tools/cov/run.sh tools/cov/specs/darksamus_helpers2_arms.py <out> "1baa96c2,10_cliff_rooftop"
#         --con tools/cov/specs/darksamus_rooftop.con --fast --frames 6400
CLASS = 'CDarkSamus'
SPEC = [
    ('1905C ClassifyLineToPlayer', 'CDarkSamus::ClassifyLineToPlayer(', [], 'calls ret max=4000'),
    ('16B1C IsOnPath', 'CDarkSamus::IsOnPath(', [], 'ret max=4000'),
    ('1DD14 Separate', 'CDarkSamus::Separate(', ['$this->mContactTime + 0.3 > $this->mElapsedTime'],
     'calls max=4000'),
    ('16B8C TurnTowardPoint', 'CDarkSamus::TurnTowardPoint(', ['$xmm0.v4_float[0] <= 0.0'], 'max=8000'),
    ('1D860 UpdateAdditiveAnimation', 'CDarkSamus::UpdateAdditiveAnimation(',
     ['$rdx == 0', '$this->mAdditiveAnim != -1'], 'max=4000'),
    ('1E740 ShouldForgetLastAttacker', 'CDarkSamus::ShouldForgetLastAttacker(',
     ['$this->mLastAttackerUid.value == 65535'], 'calls ret max=4000'),
    ('EFC8 IsInUnsafeSpot', 'CDarkSamus::IsInUnsafeSpot(', ['$this->mHasSafePos == 1'], 'calls ret max=4000'),
    ('1444C IsLineOnMesh', 'CDarkSamus::IsLineOnMesh(', [], 'calls ret max=4000'),
    ('1E3C0 FindBestCoverPoint', 'CDarkSamus::FindBestCoverPoint(', [], 'calls retp max=4000'),
    ('1C6C8 ProjectileInfo', 'CDarkSamus::ProjectileInfo(', [], 'retp max=4000'),
    ('1EE4C CanReflectHit', 'CDarkSamus::CanReflectHit(', [], 'ret max=4000'),
    ('14D14 UpdateDoubleDash', 'CDarkSamus::UpdateDoubleDash(',
     ['$rdx', '$this->mDash.mBlocked', '$this->mDash.mStepsLeft > 0'], 'calls max=4000'),
    ('19B64 CollectReachAttacks', 'CDarkSamus::CollectReachAttacks(',
     ['(*$rcx & 1) != 0', '(*$rcx & 0x100) != 0', '(*$rcx & 0x20) != 0',
      '(*$rcx & 4) != 0', '(*$rcx & 0x80) != 0', '(*$rcx & 0x10) != 0'], 'calls max=4000'),
    ('154FC PickDash', 'CDarkSamus::PickDash(', [], 'calls max=4000'),
    # a big body, to check that the call sites are the body's own
    ('12F60 FireScatterShotProjectile', 'CDarkSamus::FireScatterShotProjectile(',
     ['$this->mScatterProjectileUid.value == 65535'], 'calls max=4000'),
]
