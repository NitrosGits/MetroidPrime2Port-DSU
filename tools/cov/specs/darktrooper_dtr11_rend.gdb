# dtr11: the render and scan arms. At ThinkRagDoll (a real ragdoll exists): prime it and call
# PreRender / PreRenderAllViewports (the primed arms); head segment = root segment makes the head
# direction zero (CanBeNormalized false); then GetScannableObjectInfo over the alive / active /
# transitioning / attack state / scan info combinations. Every member is written back.
set $dE = 1
set $t = (CDarkTrooper*)$rdi
set $m = (CStateManager*)$rsi
set $rp = $t->mRagDollHolder.mRagDoll.mPtr
set $pr = $rp->mPrimed
set $hs = $t->mHeadSegId
set $dc = $t->mDamageCooldownTimer
set $valive = $t->mAlive
set $vactive = $t->mActive
set $tr = $t->mTransitioning
set $as = $t->mAttackState
set $vsi = $t->mAttackScanInfo.mPtr
printf "rend11: this %p ragdoll %p primed %d scaninfo %p alive %d active %d trans %d state %d\n", $t, $rp, $pr, $vsi, $valive, $vactive, $tr, $as
set var $rp->mPrimed = 1
set var $t->mDamageCooldownTimer = 1.0f
call $t->PreRender(*$m)
printf "step 1 done\n"
call $t->PreRenderAllViewports(*$m)
printf "step 2 done\n"
set var $t->mDamageCooldownTimer = 0.0f
set var $t->mHeadSegId = $t->mRootSegId
call $t->PreRender(*$m)
printf "step 3 done\n"
set var $t->mHeadSegId = $hs
set var $t->mDamageCooldownTimer = $dc
# a ragdoll that is not primed: both calls fall through to CPatterned
set var $rp->mPrimed = 0
call $t->PreRender(*$m)
printf "step 4 done\n"
call $t->PreRenderAllViewports(*$m)
printf "step 5 done\n"
set var $rp->mPrimed = $pr
# scan: dead, inactive, transitioning (not state 3), transitioning in state 3, normal
set var $t->mAlive = 0
printf "scan dead %p\n", $t->GetScannableObjectInfo()
set var $t->mAlive = 1
set var $t->mActive = 0
printf "scan inactive %p\n", $t->GetScannableObjectInfo()
set var $t->mActive = 1
set var $t->mTransitioning = 1
set var $t->mAttackState = 0
printf "scan transitioning %p\n", $t->GetScannableObjectInfo()
set var $t->mAttackState = 3
printf "scan trans state3 %p\n", $t->GetScannableObjectInfo()
set var $t->mTransitioning = 0
printf "scan plain %p\n", $t->GetScannableObjectInfo()
# no attack scan info
set var $t->mAttackScanInfo.mPtr = 0
printf "scan no info %p\n", $t->GetScannableObjectInfo()
set var $t->mAttackScanInfo.mPtr = $vsi
set var $t->mAlive = $valive
set var $t->mActive = $vactive
set var $t->mTransitioning = $tr
set var $t->mAttackState = $as
printf "rend11 done\n"
