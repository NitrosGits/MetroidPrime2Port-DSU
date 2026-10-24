# dtr11: FireSmallShot (fn_12_46B4) with the gate, the jitter and the geometry written, every step
# a call. LaunchProjectile returns at once ($skiplp). The three Range(-0.3, 0.3) draws are read
# first (z, y, x) from the seed, which is put back, so the gun can sit exactly on the target (the
# first CanBeNormalized false) or at a known offset (the angle test).
set $dS = 1
set $skiplp = 1
set $t = (CDarkTrooper*)$rdi
set $m = (CStateManager*)$rsi
set $pl = $m->mPlayers[0]
set $sv = $t->mSmallShotAttack
set $mt = $t->mGunTransform
set $at = $t->mTransform
set $mb = $pl->mMorphBallState
set $mn = $t->mRangedAttackMinRange
set $mx = $t->mRangedAttackMaxRange
printf "fss11: this %p valid %d morph %d\n", $t, $t->mSmallShotAttack.mProjectile.m_valid, $mb

define fss_geom
  # args: $gdx $gdy (gun - target, xy), forward in $fx $fy $fz
  set $sd = $m->mRandom.mSeed
  set $jz = $m->mRandom.Range(-0.3f, 0.3f)
  set $jy = $m->mRandom.Range(-0.3f, 0.3f)
  set $jx = $m->mRandom.Range(-0.3f, 0.3f)
  set var $m->mRandom.mSeed = $sd
  set $tb = $t->GetTargetPosition(*$m)
  set var $t->mGunTransform.m03 = (float)($tb.mX + $jx + $gdx)
  set var $t->mGunTransform.m13 = (float)($tb.mY + $jy + $gdy)
  set var $t->mGunTransform.m01 = $fx
  set var $t->mGunTransform.m11 = $fy
  set var $t->mGunTransform.m21 = $fz
end

# 1: no projectile (346 false arm)
set var $t->mSmallShotAttack.mProjectile.m_valid = 0
call $t->FireSmallShot(*$m)
printf "step 1 done\n"
set var $t->mSmallShotAttack.mProjectile.m_valid = 1
set var $t->mRangedAttackMinRange = 0
set var $t->mRangedAttackMaxRange = 1e9
# actor facing the target: forward = toward the target xy
set $vdx = $m->mPlayers[0]->mTransform.m03 - $t->mTransform.m03
set $dy = $m->mPlayers[0]->mTransform.m13 - $t->mTransform.m13
set $vdl = ($vdx * $vdx + $dy * $dy)
set $vdl = sqrt($vdl)
set var $t->mTransform.m01 = $vdx / $vdl
set var $t->mTransform.m11 = $dy / $vdl
set var $t->mTransform.m21 = 0
# 2: unmorphed (348 true), gun 5 along +x from the target, forward +x: angle 0 -> LaunchProjectile; facing+range true
set var $pl->mMorphBallState = 0
set $gdx = 5.0
set $gdy = 0.0
set $fx = 1.0
set $fy = 0.0
set $fz = 0.0
fss_geom
call $t->FireSmallShot(*$m)
printf "step 2 done\n"
# 3: morphed (348 false arm), same geometry
set var $pl->mMorphBallState = 1
fss_geom
call $t->FireSmallShot(*$m)
printf "step 3 done\n"
set var $pl->mMorphBallState = 0
# 4: gun exactly on the target xy: toTarget cannot be normalized (366 false)
set $gdx = 0.0
fss_geom
call $t->FireSmallShot(*$m)
printf "step 4 done\n"
# 5: forward straight up: after z = 0 it cannot be normalized (370 false)
set $gdx = 5.0
set $fx = 0.0
set $fz = 1.0
fss_geom
call $t->FireSmallShot(*$m)
printf "step 5 done\n"
# 6: forward opposite: angle pi > pi/4 (370 angle false)
set $fx = -1.0
set $fz = 0.0
fss_geom
call $t->FireSmallShot(*$m)
printf "step 6 done\n"
# 7: aligned, not facing the player (actor forward away): 384 facing false -> EndSmallShotAttack
set $fx = 1.0
fss_geom
set var $t->mTransform.m01 = -$vdx / $vdl
set var $t->mTransform.m11 = -$dy / $vdl
call $t->FireSmallShot(*$m)
printf "step 7 done\n"
# 8: facing, out of range (min above the distance): 384 range false
set var $t->mTransform.m01 = $vdx / $vdl
set var $t->mTransform.m11 = $dy / $vdl
set var $t->mRangedAttackMinRange = 1e9
set var $t->mRangedAttackMaxRange = 2e9
fss_geom
call $t->FireSmallShot(*$m)
printf "step 8 done\n"
# 9: facing and in range: nothing ends
set var $t->mRangedAttackMinRange = 0
set var $t->mRangedAttackMaxRange = 1e9
fss_geom
call $t->FireSmallShot(*$m)
printf "step 9 done\n"
set var $t->mSmallShotAttack = $sv
set var $t->mGunTransform = $mt
set var $t->mTransform = $at
set var $pl->mMorphBallState = $mb
set var $t->mRangedAttackMinRange = $mn
set var $t->mRangedAttackMaxRange = $mx
set $skiplp = 0
printf "fss11 done\n"
