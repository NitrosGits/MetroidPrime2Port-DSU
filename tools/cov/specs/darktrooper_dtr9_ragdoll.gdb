# dtr9: at a ThinkRagDoll entry (this rdi, dt xmm0, mgr rsi; a ragdoll exists): CreateRagDoll with
# one present, ThinkRagDoll with the ragdoll nulled, and the flag combinations of the IsOver arm.
set $dR = 1
set $t = (CDarkTrooper*)$rdi
set $m = (CStateManager*)$rsi
printf "ragdoll: this %p ptr %p\n", $t, $t->mRagDollHolder.mRagDoll.mPtr
call $t->CreateRagDoll(*$m)
set $rp = $t->mRagDollHolder.mRagDoll.mPtr
set var $t->mRagDollHolder.mRagDoll.mPtr = 0
call $t->ThinkRagDoll(0.016f, *$m)
set var $t->mRagDollHolder.mRagDoll.mPtr = $rp
set $ad = $t->mAvoidDownFrames
set $fd = $t->mFadeToDeath
set $ua = $t->mUpdatingAnimation
set var $t->mAvoidDownFrames = 0
set var $t->mFadeToDeath = 0
set var $t->mUpdatingAnimation = 0
call $t->ThinkRagDoll(0.016f, *$m)
set var $t->mUpdatingAnimation = 1
call $t->ThinkRagDoll(0.016f, *$m)
set var $t->mFadeToDeath = 1
call $t->ThinkRagDoll(0.016f, *$m)
set var $t->mAvoidDownFrames = 1
set var $t->mFadeToDeath = 0
call $t->ThinkRagDoll(0.016f, *$m)
set var $t->mAvoidDownFrames = $ad
set var $t->mFadeToDeath = $fd
set var $t->mUpdatingAnimation = $ua
printf "ragdoll done\n"
