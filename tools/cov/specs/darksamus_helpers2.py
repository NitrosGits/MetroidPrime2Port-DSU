# DarkSamus helpers2 coverage spec (module 10, the helpers at 0xA000 and up).
#
# Run: tools/cov/run.sh tools/cov/specs/darksamus_helpers2.py <out> "1baa96c2,10_cliff_rooftop"
#         --con tools/cov/specs/darksamus_rooftop.con --fast --frames 6000
#
# The labels carry the retail offsets (the REL's own function offsets, 0xA000 and up). An entry
# breaks at the port's body; the expressions are the branch conditions, evaluated on the entry
# registers, so every distinct key is one arm: a key that never shows is an arm that never ran.
# `$this` is the actor ($rdi); a state's EStateMsg is $rdx (the second argument) or $rsi (the
# first); the first float argument is $xmm0.v4_float[0]; the third argument is $rcx.
#
# Bodies that return a struct by value have their `this` in $rsi (the hidden return pointer
# takes $rdi), so no `$this` expression is used for them - only their call sites ('calls').
CLASS = 'CDarkSamus'
SPEC = [
    # ---- the hint chain (the boost ball's hints) -------------------------------------------
    ('A06C UpdateBoostBallAtActor', 'CDarkSamus::UpdateBoostBallAtActor(', [], 'calls max=2000'),
    ('B458 FindArrivedHint', 'CDarkSamus::FindArrivedHint(', [], 'calls retp max=2000'),
    ('B6C4 FindBoostBallHint', 'CDarkSamus::FindBoostBallHint(', [], 'calls retp max=2000'),
    ('B86C ActivateHintChains', 'CDarkSamus::ActivateHintChains(', [], 'calls max=2000'),
    ('BA38 BoostChainHasInactiveHint', 'CDarkSamus::BoostChainHasInactiveHint(', [], 'calls ret max=2000'),
    # ---- the sweep beam ----------------------------------------------------------------------
    ('C104 SweepBeamDamage', 'CDarkSamus::SweepBeamDamage(', [], 'calls max=2000'),
    ('C2B0 UpdateSweepBeamAim', 'CDarkSamus::UpdateSweepBeamAim(', [], 'calls max=2000'),
    ('C470 UpdateSweepBeamEnd', 'CDarkSamus::UpdateSweepBeamEnd(', [], 'calls max=2000'),
    # ---- the boss's own helpers ----------------------------------------------------------------
    ('EFC8 IsInUnsafeSpot', 'CDarkSamus::IsInUnsafeSpot(', ['$this->mHasSafePos == 1'], 'calls ret max=2000'),
    ('F40C GetScannableObjectInfo', 'CDarkSamus::GetScannableObjectInfo(', [], 'retp max=2000'),
    ('F488 TakeDamage', 'CDarkSamus::TakeDamage(', [], 'calls max=2000'),
    ('F5C0 GetTouchBounds', 'CDarkSamus::GetTouchBounds(', [], 'retp max=500'),
    ('FF20 DamageVulnerability', 'CDarkSamus::DamageVulnerability(', [], 'retp max=2000'),
    ('100DC GetDamageVulnerability', 'CDarkSamus::GetDamageVulnerability(', ['$this->mGuestCD0 == 1'], 'retp max=2000'),
    ('10210 GetAimPosition', 'CDarkSamus::GetAimPosition(', [], 'max=500'),
    ('10358 GetOrbitPosition', 'CDarkSamus::GetOrbitPosition(', [], 'max=500'),
    ('11334 UpdateGlideEmitter', 'CDarkSamus::UpdateGlideEmitter(', ['$rdx == 0'], 'calls max=4000'),
    ('11974 UpdateScatterShotFire', 'CDarkSamus::UpdateScatterShotFire(',
     ['$this->mScatterFiring == 0', '$this->mScatterRate <= 0.0'], 'calls max=4000'),
    ('11C60 UpdateDiveFlight', 'CDarkSamus::UpdateDiveFlight(', [], 'calls max=2000'),
    ('11DDC SendDiveAttack', 'CDarkSamus::SendDiveAttack(', ['$this->mDive.mAttackSent'], 'calls max=2000'),
    ('11EB8 UpdateDiveStuck', 'CDarkSamus::UpdateDiveStuck(', ['$this->mDive.mStuckTime > 0.5'], 'calls max=2000'),
    ('128D8 UpdateSafePosition', 'CDarkSamus::UpdateSafePosition(', ['$this->mHasSafePos != 0'], 'calls max=2000'),
    ('12960 FindDiveLanding', 'CDarkSamus::FindDiveLanding(', [], 'calls max=2000'),
    ('12F60 FireScatterShotProjectile', 'CDarkSamus::FireScatterShotProjectile(',
     ['$this->mScatterProjectileUid == 65535'], 'calls max=2000'),
    ('1444C IsLineOnMesh', 'CDarkSamus::IsLineOnMesh(', [], 'calls ret max=2000'),
    ('14A00 ApplyDashStuckBlast', 'CDarkSamus::ApplyDashStuckBlast(', [], 'calls max=2000'),
    ('14D14 UpdateDoubleDash', 'CDarkSamus::UpdateDoubleDash(',
     ['$rdx == 0', '$this->mDash.mBlocked', '$this->mDash.mStepsLeft > 0'], 'calls max=2000'),
    ('14FE0 PickNextDash', 'CDarkSamus::PickNextDash(',
     ['$this->mStillTime > 1.2', '$this->mDashStuckCount >= 2'], 'calls max=2000'),
    ('154FC PickDash', 'CDarkSamus::PickDash(', [], 'calls max=2000'),
    ('16B8C TurnTowardPoint', 'CDarkSamus::TurnTowardPoint(', ['$xmm0.v4_float[0] <= 0.0'], 'max=8000'),
    ('1707C UpdateDodge', 'CDarkSamus::UpdateDodge(',
     ['$this->mDodgeTime > 0.0', '$this->mDodgeTime <= 0.0'], 'calls max=2000'),
    ('17F04 AddToRenderer', 'CDarkSamus::AddToRenderer(', [], 'max=500'),
    ('18964 Render', 'CDarkSamus::Render(', [], 'max=500'),
    ('18AB8 PreRender', 'CDarkSamus::PreRender(', [], 'max=500'),
    ('18D78 SetupStateMachine', 'CDarkSamus::SetupStateMachine(', [], 'max=200'),
    ('19B64 CollectReachAttacks', 'CDarkSamus::CollectReachAttacks(',
     ['(*$rcx & 1) != 0', '(*$rcx & 0x100) != 0', '(*$rcx & 0x20) != 0',
      '(*$rcx & 4) != 0', '(*$rcx & 0x80) != 0', '(*$rcx & 0x10) != 0'], 'calls max=2000'),
    ('1AAC4 GetCollisionPrimitive', 'CDarkSamus::GetCollisionPrimitive(', [], 'retp max=500'),
    ('1B1FC Think', 'CDarkSamus::Think(', [], 'max=500'),
    ('1C6C8 ProjectileInfo', 'CDarkSamus::ProjectileInfo(', [], 'retp max=2000'),
    ('1CDD0 BounceDive', 'CDarkSamus::BounceDive(', [], 'calls max=2000'),
    ('1D794 UpdateStillTimer', 'CDarkSamus::UpdateStillTimer(', ['$rsi == 0'], 'max=4000'),
    ('1D860 UpdateAdditiveAnimation', 'CDarkSamus::UpdateAdditiveAnimation(',
     ['$rdx == 0', '$this->mAdditiveAnim != -1'], 'max=4000'),
    ('1D9A4 GetRepulsionPoint', 'CDarkSamus::GetRepulsionPoint(', [], 'calls max=2000'),
    ('1DD14 Separate', 'CDarkSamus::Separate(', ['$this->mContactTime + 0.3 > $this->mElapsedTime'],
     'calls max=2000'),
    ('1E178 AccumulateDamage', 'CDarkSamus::AccumulateDamage(',
     ['$rsi == 0', '$this->mLastHealth > 0.0', '$this->mAccumulatedDamage < 0.0'], 'max=4000'),
    ('1E26C PushAttacker', 'CDarkSamus::PushAttacker(',
     ['$this->mLastAttackerUid == 65535', '$this->mAttackerCount >= 2'], 'max=4000'),
    ('1E2E8 EraseFirstAttacker', 'CDarkSamus::EraseFirstAttacker(', [], 'max=4000'),
    ('1E3C0 FindBestCoverPoint', 'CDarkSamus::FindBestCoverPoint(', [], 'calls retp max=2000'),
    ('1E4C4 ScoreCoverPoint', 'CDarkSamus::ScoreCoverPoint(', [], 'calls max=2000'),
    ('1E740 ShouldForgetLastAttacker', 'CDarkSamus::ShouldForgetLastAttacker(',
     ['$this->mLastAttackerUid == 65535'], 'calls ret max=2000'),
    ('1E89C AcceptScriptMsg', 'CDarkSamus::AcceptScriptMsg(', [], 'max=500'),
    ('1EE4C CanReflectHit', 'CDarkSamus::CanReflectHit(', [], 'ret max=2000'),
    # ---- the five ids the DarkSamus2 lane dropped --------------------------------------------
    ('16418 IsPlayerAimLineClear', 'CDarkSamus::IsPlayerAimLineClear(', [], 'calls ret max=2000'),
    ('164C0 (no body yet)', 'CDarkSamus::ClassifyLineToPlayer(', [], 'max=200'),
    ('1905C ClassifyLineToPlayer', 'CDarkSamus::ClassifyLineToPlayer(', [], 'calls ret max=2000'),
    ('16B1C IsOnPath', 'CDarkSamus::IsOnPath(', [], 'ret max=2000'),
    ('1A554 CollectFadeAttacks', 'CDarkSamus::CollectFadeAttacks(', ['$this->mFadeOut == 1'],
     'calls max=2000'),
]
