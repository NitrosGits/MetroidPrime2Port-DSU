# lum2 sun: UpdateSunlight with the safe-zone surface (IsTouchingSunTrigger true at once) and the sunlight bit
# not transitioning (the `!mTransitioning` alpha arm), then with the dark visor active.
set $t = (CLumite*)$rdi
set $m = (CStateManager*)$rsi
set $lm_as = $t->mAttackState
set $lm_kind = $t->mSurface.mKind
set $lm_tr = $t->mTransitioning
set $lm_al = $t->mAlphaFactor
set $lm_in = $t->mInSunlight
set var $t->mAttackState = 1
set var $t->mSurface.mKind = 2
set var $t->mTransitioning = 0
set var $t->mAlphaFactor = 0.5f
call $t->UpdateSunlight(*$m, 0.016f)
printf "sun zone-surface: insun %d alpha %f\n", $t->mInSunlight, $t->mAlphaFactor
set var $t->mAttackState = $lm_as
set var $t->mSurface.mKind = $lm_kind
set var $t->mTransitioning = 1
set $lm_vis = $m->mPlayerStates[0]->mCurrentVisor
set var $m->mPlayerStates[0]->mCurrentVisor = CPlayerState::kPV_Dark
call $t->UpdateSunlight(*$m, 0.016f)
set var $m->mPlayerStates[0]->mCurrentVisor = $lm_vis
set var $t->mTransitioning = $lm_tr
set var $t->mAlphaFactor = $lm_al
set var $t->mInSunlight = $lm_in
printf "sun done\n"
