# dtr9: calls at a CDarkTrooper::Think entry (this rdi, mgr rsi).
set $dA = 1
set $t = (CDarkTrooper*)$rdi
set $m = (CStateManager*)$rsi
printf "think: this %p mgr %p\n", $t, $m
set $tr = $t->mTransitioning
set $valive = $t->mAlive
set var $t->mTransitioning = 0
set var $t->mAlive = 1
call $t->GetContactDamage()
set var $t->mTransitioning = 1
call $t->GetContactDamage()
set var $t->mTransitioning = 0
set var $t->mAlive = 0
call $t->GetContactDamage()
set var $t->mTransitioning = $tr
set var $t->mAlive = $valive
printf "contact done\n"
set $hx = $t->mHeadDirection.mX
set $hy = $t->mHeadDirection.mY
set $hz = $t->mHeadDirection.mZ
call $t->GetIngSnatchingNormal(0.0f)
set var $t->mHeadDirection.mX = 0
set var $t->mHeadDirection.mY = 0
set var $t->mHeadDirection.mZ = 1
call $t->GetIngSnatchingNormal(0.0f)
set var $t->mHeadDirection.mX = 1
set var $t->mHeadDirection.mY = 2
set var $t->mHeadDirection.mZ = 3
call $t->GetIngSnatchingNormal(0.0f)
set var $t->mHeadDirection.mX = $hx
set var $t->mHeadDirection.mY = $hy
set var $t->mHeadDirection.mZ = $hz
printf "snatch done\n"
set $lk = $t->mLockOnTarget.mId
call $t->GetAimPosition(*$m, 0.5f)
call $t->GetAimPosition(*$m, 0.0f)
set var $t->mLockOnTarget.mId = 0xff
call $t->GetAimPosition(*$m, 0.5f)
call $t->GetAimPosition(*$m, 0.0f)
set var $t->mLockOnTarget.mId = $lk
printf "aim done\n"
set $mi = $t->mMarkedRegionIndex
set var $t->mMarkedRegionIndex = -1
call $t->UnmarkPathRegion(*$m)
set var $t->mMarkedRegionIndex = 0
call $t->UnmarkPathRegion(*$m)
set var $t->mMarkedRegionIndex = 0
call $t->UnmarkPathRegion(*$m)
set var $t->mMarkedRegionIndex = 0
set $pnull = 1
call $t->UnmarkPathRegion(*$m)
set $pnull = 0
set var $t->mMarkedRegionIndex = $mi
printf "unmark done\n"
# Sleep Activate: the Range(0,1) < 0.5 draw, repeated (locomotion type forced to relaxed)
set $lt = $t->mLocomotionType
set var $t->mLocomotionType = pas::kLT_Relaxed
call $t->Sleep(*$m, 0, 0.0f)
call $t->Sleep(*$m, 0, 0.0f)
call $t->Sleep(*$m, 0, 0.0f)
call $t->Sleep(*$m, 0, 0.0f)
call $t->Sleep(*$m, 0, 0.0f)
call $t->Sleep(*$m, 0, 0.0f)
call $t->Sleep(*$m, 0, 0.0f)
call $t->Sleep(*$m, 0, 0.0f)
set var $t->mLocomotionType = $lt
printf "sleep done\n"
# Dead: Activate with a scannable info / without, then the avoid-down-frames arm.
set $ad = $t->mAvoidDownFrames
set $scan = 1
call $t->Dead(*$m, 0, 0.0f)
set $scan = 2
call $t->Dead(*$m, 0, 0.0f)
set $scan = 0
set var $t->mAvoidDownFrames = 1
call $t->Dead(*$m, 0, 0.0f)
call $t->Dead(*$m, 1, 0.0f)
set var $t->mAvoidDownFrames = $ad
printf "dead done\n"
