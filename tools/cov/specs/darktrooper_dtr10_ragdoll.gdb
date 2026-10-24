# dtr10: ThinkRagDoll with the fluid id forced: a real non-water object (the null-cast arm) and
# invalid again; the 731 flag combinations of the IsOver arm are dtr9's. Water arms: dead by data.
set $dR = 1
set $t = (CDarkTrooper*)$rdi
set $m = (CStateManager*)$rsi
set $fc = $t->mFluidIds.mCount
set $f0 = *(unsigned short*)$t->mFluidIds.mData
printf "ragdoll10: this %p fluids %d ptr %p\n", $t, $fc, $t->mRagDollHolder.mRagDoll.mPtr
set var $t->mFluidIds.mCount = 1
set var *(unsigned short*)$t->mFluidIds.mData = (unsigned short)$t->mUniqueId.value
call $t->ThinkRagDoll(0.016f, *$m)
printf "step 1 done\n"
set var $t->mFluidIds.mCount = $fc
set var *(unsigned short*)$t->mFluidIds.mData = $f0
set $rp = $t->mRagDollHolder.mRagDoll.mPtr
set var $t->mRagDollHolder.mRagDoll.mPtr = 0
call $t->ThinkRagDoll(0.016f, *$m)
printf "step 2 done\n"
set var $t->mRagDollHolder.mRagDoll.mPtr = $rp
set $ad = $t->mAvoidDownFrames
set $fd = $t->mFadeToDeath
set $ua = $t->mUpdatingAnimation
# IsOver and not continuing: both flag arms of the fade-to-death test, then IsOver and continuing
set var $rp->mOver = 1
set var $rp->mContinueSmallMovements = 0
set var $t->mAvoidDownFrames = 0
set var $t->mFadeToDeath = 0
call $t->ThinkRagDoll(0.016f, *$m)
printf "step 3 done\n"
printf "fade after plain: %d\n", $t->mFadeToDeath
set var $rp->mOver = 1
set var $t->mFadeToDeath = 1
call $t->ThinkRagDoll(0.016f, *$m)
printf "step 4 done\n"
set var $rp->mOver = 1
set var $t->mAvoidDownFrames = 1
set var $t->mFadeToDeath = 0
call $t->ThinkRagDoll(0.016f, *$m)
printf "step 5 done\n"
set var $rp->mOver = 1
set var $rp->mContinueSmallMovements = 1
call $t->ThinkRagDoll(0.016f, *$m)
printf "step 6 done\n"
# not over: animation update off / on
set var $rp->mOver = 0
set var $t->mUpdatingAnimation = 0
call $t->ThinkRagDoll(0.016f, *$m)
printf "step 7 done\n"
set var $t->mUpdatingAnimation = 1
call $t->ThinkRagDoll(0.016f, *$m)
printf "step 8 done\n"
set var $t->mAvoidDownFrames = $ad
set var $t->mFadeToDeath = $fd
set var $t->mUpdatingAnimation = $ua
printf "ragdoll10 done\n"
