# ds46 forced calls of fn_10_154FC PickDash, run once at a CDarkSamus::Think entry
# (tools/cov/specs/darksamus_ds46_pickdash.py). Every member written here is written back.
# Arms: steps left / none with the last slot (2: slot 2 itself, slot 3 by its opposite); the
# facing flat (forward column with no x/y); the actor next to the player (end points nearer
# the player, the closest-pass scale under 8); a target near (1 - d / range) and far (0.01); the
# clearance under 1 with the height query failed, with the actor still (dropped) and not
# (0.02); weights all negative (no slot matches the best 0.0f: slot 1).
set $t = (CDarkSamus*)$rdi
set $m = (CStateManager*)$rsi
printf "ds46: this %p mgr %p range %f still %f\n", $t, $m, $t->mMaxAttackRange, $t->mStillTime
set unwind-on-signal on
ds45-scratch
set $w = (float*)$scratch
set $tg = (CVector3f*)($scratch + 0x40)
set $pp = $m->mPlayers[0]->mPosition
set $dash = $t->mDash
set $still = $t->mStillTime
set $pos = $t->mPosition
set $dxf = $t->mTransform

define ds46-w
  set $i = 0
  while $i < 8
    set var $w[$i] = $arg0
    set $i = $i + 1
  end
end
define ds46-call
  set $tgp = (CVector3f*)$arg0
  if $tgp != 0
    printf "target %p (%f %f %f)\n", $tgp, $tgp->mX, $tgp->mY, $tgp->mZ
  end
  set $r = $t->PickDash(*$m, $w, $tgp)
  printf "154FC %s: answer %f slot %d steps %d last %d w %f %f %f %f %f %f %f %f\n", $arg1, $r, $t->mDash.mSlot, $t->mDash.mStepsLeft, $t->mDash.mLastSlot, $w[0], $w[1], $w[2], $w[3], $w[4], $w[5], $w[6], $w[7]
end
define ds46-dash
  set var $t->mDash = $dash
  set var $t->mDash.mStepsLeft = $arg0
  set var $t->mDash.mLastSlot = $arg1
end

# natural position, no steps, last slot 2, no target
ds46-w 1.0
ds46-dash 0 2
ds46-call 0 "far_nosteps_last2"
ds46-w 1.0
ds46-dash 1 2
ds46-call 0 "far_steps_last2"

# the actor 4 units from the player: end points nearer the player, the closest pass under 8; the
# target at the actor (one step from each end: 1 - d / range), then 100000 off (0.01)
set var $t->mPosition.mX = $pp.mX + 4.0
set var $t->mPosition.mY = $pp.mY
set var $t->mPosition.mZ = $pp.mZ
set var $tg->mX = $t->mPosition.mX
set var $tg->mY = $t->mPosition.mY
set var $tg->mZ = $t->mPosition.mZ
ds46-w 1.0
ds46-dash 0 2
ds46-call $tg "near_nosteps_target_near"
ds46-w 1.0
ds46-dash 2 2
ds46-call $tg "near_steps_target_near"
set var $tg->mX = $tg->mX + 100000.0
ds46-w 1.0
ds46-dash 0 2
ds46-call $tg "near_nosteps_target_far"
set var $t->mPosition = $pos

# the facing flat: the forward column (m01, m11, m21) with no x/y
set var $t->mTransform.m01 = 0.0
set var $t->mTransform.m11 = 0.0
set var $t->mTransform.m21 = 1.0
ds46-w 1.0
ds46-dash 0 -1
ds46-call 0 "facing_flat"
set var $t->mTransform = $dxf

# the clearance forced under 1: height query failed; succeeded and still; succeeded, moving
set $clrv = 0.5
set $hval = 1
set $hgate = 1
set $clrgate = -1
ds46-w 1.0
ds46-dash 0 -1
ds46-call 0 "clear_half_height_failed"
set $hval = 0
set $hgate = 1
set var $t->mStillTime = 0.0
ds46-w 1.0
ds46-dash 0 -1
ds46-call 0 "clear_half_still"
set $hval = 0
set $hgate = 1
set var $t->mStillTime = 2.0
ds46-w 1.0
ds46-dash 0 -1
ds46-call 0 "clear_half_moving"
set $clrgate = 0
set $hgate = 0
set var $t->mStillTime = $still

# weights all negative (the clearance forced clear, 1.0f): the best stays 0.0f, no slot matches,
# slot 1
set $clrv = 1.0
set $clrgate = -1
ds46-w -1.0
ds46-dash 0 -1
ds46-call 0 "all_negative"
set $clrgate = 0

set var $t->mDash = $dash
set var $t->mStillTime = $still
set var $t->mPosition = $pos
set var $t->mTransform = $dxf
printf "ds46 done\n"
