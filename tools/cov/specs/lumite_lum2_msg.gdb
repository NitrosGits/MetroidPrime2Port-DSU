# lum2 msg: AcceptScriptMsg with a fabricated CScriptMsg: AreaLoaded on an inactive Lumite (the GetActive operand),
# OffGround with the body controller frozen (the percentage-frozen operand).
set $t = (CLumite*)$rdi
set $m = (CStateManager*)$rsi
set $lm_msg = (CScriptMsg*)malloc(32)
set var $lm_msg->mSenderId.value = 0xffff
set var $lm_msg->m_originator.value = 0xffff
set var $lm_msg->m_id.value = 0xffff
set var $lm_msg->m_state = 0
set $lm_bc = $t->mBodyController.mPtr
set $lm_fd = $lm_bc->mFrozenDur
set $lm_tf = $lm_bc->mTimeFrozen
set $lm_vm = $t->mVerticalMovement
set var $t->mActive = 0
set var $lm_msg->m_msg = kSM_AreaLoaded
call $t->AcceptScriptMsg(*$m, *$lm_msg)
set var $t->mActive = 1
printf "msg areaLoaded inactive done\n"
set var $lm_bc->mFrozenDur = 10.0f
set var $lm_bc->mTimeFrozen = 1.0f
set var $t->mVerticalMovement = 0
set var $lm_msg->m_msg = kSM_OffGround
call $t->AcceptScriptMsg(*$m, *$lm_msg)
set var $lm_bc->mFrozenDur = $lm_fd
set var $lm_bc->mTimeFrozen = $lm_tf
set var $t->mVerticalMovement = $lm_vm
printf "msg offground frozen done\n"
