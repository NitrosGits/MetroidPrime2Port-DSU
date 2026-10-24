# lum2 basic: the arms that need only member writes: ProjectileInfo (neither state), GetScannableObjectInfo (alpha 0),
# Think (inactive; the StickToSurface condition combinations), StickToSurface (safe-zone surface), UpdateSunlight
# (dead early return), PreRender (not transitioning), FacingHint (zero direction), ResetAttackTimer / RandomVector
# (no random), the turn effect events (state 4), GetKneeName default.
set $t = (CLumite*)$rdi
set $m = (CStateManager*)$rsi
set $lm_as = $t->mAttackState
set $lm_al = $t->mAlphaFactor
set $lm_kind = $t->mSurface.mKind
set $lm_tr = $t->mTransitioning
set $lm_ld = $t->mHop.mLanded
set $lm_ra = $m->mRandomAvailable
printf "basic: this %p state %d alpha %f\n", $t, $lm_as, $lm_al
set var $t->mAttackState = 0
printf "ProjectileInfo none: %p\n", $t->ProjectileInfo()
set var $t->mAttackState = $lm_as
set var $t->mAlphaFactor = 0.0f
printf "scan alpha0: %p\n", $t->GetScannableObjectInfo()
set var $t->mAlphaFactor = $lm_al
set var $t->mActive = 0
call $t->Think(0.016f, *$m)
set var $t->mActive = 1
printf "think inactive done\n"
set var $t->mAttackState = 3
call $t->Think(0.016f, *$m)
set var $t->mAttackState = 0
set var $t->mHop.mLanded = 1
call $t->Think(0.016f, *$m)
set var $t->mHop.mLanded = 0
call $t->Think(0.016f, *$m)
set var $t->mAttackState = 1
call $t->Think(0.016f, *$m)
set var $t->mAttackState = 4
call $t->Think(0.016f, *$m)
set var $t->mAttackState = $lm_as
set var $t->mHop.mLanded = $lm_ld
printf "think combos done\n"
set var $t->mSurface.mKind = 2
call $t->StickToSurface(0.016f)
set var $t->mSurface.mKind = $lm_kind
printf "stick done\n"
set var $t->mAlive = 0
call $t->UpdateSunlight(*$m, 0.016f)
set var $t->mAlive = 1
printf "sunlight dead done\n"
set var $t->mTransitioning = 0
call $t->PreRender(*$m)
set var $t->mTransitioning = $lm_tr
printf "prerender done\n"
set $lm_v = (float*)malloc(16)
set var $lm_v[0] = $t->mTransform.m03
set var $lm_v[1] = $t->mTransform.m13
set var $lm_v[2] = $t->mTransform.m23
set $lm_pl = $t->mSurface.mPlane
set var $t->mSurface.mPlane.mNormal.mX = 0.0f
set var $t->mSurface.mPlane.mNormal.mY = 0.0f
set var $t->mSurface.mPlane.mNormal.mZ = 1.0f
set var $t->mSurface.mPlane.mConstant = $t->mTransform.m23
printf "FacingHint zero: %d\n", $t->FacingHint(*(CVector3f*)$lm_v)
set var $t->mSurface.mPlane = $lm_pl
set var $m->mRandomAvailable = 0
call $t->ResetAttackTimer(*$m)
call $t->RandomVector(*$m, 1.0f)
set var $m->mRandomAvailable = $lm_ra
printf "random done\n"
set var $t->mAttackState = 4
call $t->DoUserAnimEvent(*$m, *(CInt32POINode*)$m, kUE_EffectOn, 0.0f)
call $t->DoUserAnimEvent(*$m, *(CInt32POINode*)$m, kUE_EffectOff, 0.0f)
set var $t->mAttackState = $lm_as
printf "events done\n"
printf "kneename7 %d\n", $t->GetKneeName(7).mSize
printf "basic done\n"
