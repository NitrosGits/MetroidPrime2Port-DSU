# ds43 forced calls, run once at a CDarkSamus::Think entry (tools/cov/specs/darksamus_ds43_calls.py).
# Every member written here is written back before the next body.
set $t = (CDarkSamus*)$rdi
set $m = (CStateManager*)$rsi
set $st = (CDarkSamusBattleStage*)$t->GetBattleStage(*$m)
printf "ds43: this %p mgr %p stage %p\n", $t, $m, $st
set $suid = $t->mBattleStageUid.value
set $tx = $t->mTransform.m03
set $ty = $t->mTransform.m13
set $tz = $t->mTransform.m23
set $safe = $t->mSafePos
set $cd0 = $t->mGuestCD0
set $d24 = $t->mGuestD24
set $e04 = $t->mGuestE04
set $e08 = $t->mGuestE08
set $e0c = $t->mGuestE0C

# fn_10_F488 TakeDamage: inactive; HP 0.5 allowed to die; HP 0.5 not allowed to die (held at 1);
# phazon phase below the mark (mark - 0.1, 0xCD0, 0xD24); phazon phase above the mark.
set $hp = $t->mHealthInfo.mHealth
set $cand = $t->mCanDie
set $pp = $t->mPhazonPhase
set $mark = $t->mPhazonHealthMark
set var $t->mActive = 0
call $t->TakeDamage($t->mSafePos, 1.0)
printf "F488 inactive hp %f\n", $t->mHealthInfo.mHealth
set var $t->mActive = 1
set var $t->mHealthInfo.mHealth = 0.5
set var $t->mCanDie = 1
set var $t->mPhazonPhase = 0
call $t->TakeDamage($t->mSafePos, 1.0)
printf "F488 can die hp %f (0.5)\n", $t->mHealthInfo.mHealth
set var $t->mHealthInfo.mHealth = 0.5
set var $t->mCanDie = 0
call $t->TakeDamage($t->mSafePos, 1.0)
printf "F488 cannot die hp %f (1)\n", $t->mHealthInfo.mHealth
set var $t->mHealthInfo.mHealth = 0.5
set var $t->mCanDie = 1
set var $t->mPhazonPhase = 1
set var $t->mPhazonHealthMark = 100.0
call $t->TakeDamage($t->mSafePos, 1.0)
printf "F488 phazon below hp %f (39.9) cd0 %d d24 %f\n", $t->mHealthInfo.mHealth, $t->mGuestCD0, $t->mGuestD24
set var $t->mHealthInfo.mHealth = 90.0
call $t->TakeDamage($t->mSafePos, 1.0)
printf "F488 phazon above hp %f (90)\n", $t->mHealthInfo.mHealth
set var $t->mHealthInfo.mHealth = $hp
set var $t->mCanDie = $cand
set var $t->mPhazonPhase = $pp
set var $t->mPhazonHealthMark = $mark
set var $t->mGuestCD0 = $cd0
set var $t->mGuestD24 = $d24

# fn_10_100DC GetDamageVulnerability: 0xCD0 == 1; the enrage attack; neither.
set $ca = $t->mCurrentAttack
set var $t->mGuestCD0 = 1
print $t->GetDamageVulnerability() == &$t->mGuestCD4
set var $t->mGuestCD0 = 0
set var $t->mCurrentAttack = 0x800000
print $t->GetDamageVulnerability() == &$t->mEnrageVulnerability
set var $t->mCurrentAttack = 0x20
print $t->GetDamageVulnerability()
set var $t->mCurrentAttack = $ca
set var $t->mGuestCD0 = $cd0

# fn_10_F100 RollReflectChanceForHit: eleven new uids (the eleventh erases the first), then a hit.
set $rcn = $t->mReflectChanceCount
set $rc = $t->mReflectChances
set $bb = $t->mBoostBallUid.value
set var $t->mReflectChanceCount = 0
set $i = 0
while $i < 11
  set var $t->mBoostBallUid.value = 0x7001 + $i
  call $t->RollReflectChanceForHit(*$m, $t->mBoostBallUid)
  set $i = $i + 1
end
printf "F100 count %d (10)\n", $t->mReflectChanceCount
call $t->RollReflectChanceForHit(*$m, $t->mBoostBallUid)
printf "F100 hit: count %d (10)\n", $t->mReflectChanceCount

# fn_10_1EE4C CanReflectHit: too soon (C0C picked); not a missile (C20 picked); no stage; plain
# missile under/over +0x28; comboed under/over +0x2C.
set $c0c = $t->mGuestC0C
set $c20 = $t->mGuestC20
set $dmg = $t->mSweepBeamDamageInfo
set $m28 = $st->mStageInfo.missiles.unknown_0xbfb7ca5c
set $m2c = $st->mStageInfo.missiles.unknown_0x1e2debc6
set var $t->mGuestC0C = 1.0e9
set var $t->mGuestC20 = 0.0
print $t->CanReflectHit(*$m, $t->mSweepBeamDamageInfo, $t->mBoostBallUid)
set var $t->mGuestC0C = -200.0
set var $t->mGuestC20 = -100.0
set var $t->mSweepBeamDamageInfo.mWeaponMode.mWeaponType = 5
print $t->CanReflectHit(*$m, $t->mSweepBeamDamageInfo, $t->mBoostBallUid)
set var $t->mSweepBeamDamageInfo.mWeaponMode.mWeaponType = 6
set var $t->mSweepBeamDamageInfo.mWeaponMode.mComboed = 0
set var $t->mBattleStageUid.value = 0xFFFF
print $t->CanReflectHit(*$m, $t->mSweepBeamDamageInfo, $t->mBoostBallUid)
set var $t->mBattleStageUid.value = $suid
set var $st->mStageInfo.missiles.unknown_0xbfb7ca5c = 101.0
print $t->CanReflectHit(*$m, $t->mSweepBeamDamageInfo, $t->mBoostBallUid)
set var $st->mStageInfo.missiles.unknown_0xbfb7ca5c = -1.0
print $t->CanReflectHit(*$m, $t->mSweepBeamDamageInfo, $t->mBoostBallUid)
set var $t->mSweepBeamDamageInfo.mWeaponMode.mComboed = 1
set var $st->mStageInfo.missiles.unknown_0x1e2debc6 = 101.0
print $t->CanReflectHit(*$m, $t->mSweepBeamDamageInfo, $t->mBoostBallUid)
set var $st->mStageInfo.missiles.unknown_0x1e2debc6 = -1.0
print $t->CanReflectHit(*$m, $t->mSweepBeamDamageInfo, $t->mBoostBallUid)
set var $st->mStageInfo.missiles.unknown_0xbfb7ca5c = $m28
set var $st->mStageInfo.missiles.unknown_0x1e2debc6 = $m2c
set var $t->mSweepBeamDamageInfo = $dmg
set var $t->mGuestC0C = $c0c
set var $t->mGuestC20 = $c20
set var $t->mBoostBallUid.value = $bb
set $i = 0
while $i < 10
  set var $t->mReflectChances[$i] = $rc[$i]
  set $i = $i + 1
end
set var $t->mReflectChanceCount = $rcn

# fn_10_11EB8 UpdateDiveStuck: moved; still under 0.5 s; past 0.5 s (fn_10_1CDD0 with no normal).
# fn_10_1CDD0 BounceDive with a flat normal. fn_10_11DDC SendDiveAttack: already sent; bounced;
# no stage; stage +0x59 clear; stage +0x59 set.
set $dv = $t->mDive
set $vm = $t->mVerticalMovement
set $og = $t->mOnGround
set $osg = $t->mOnStaticGround
set var $t->mDive.mLastPos.mX = $tx + 10.0
call $t->UpdateDiveStuck(*$m, 0.3)
printf "11EB8 moved: stuck %f (0)\n", $t->mDive.mStuckTime
call $t->UpdateDiveStuck(*$m, 0.3)
printf "11EB8 still: stuck %f (0.3) bounced %d\n", $t->mDive.mStuckTime, $t->mDive.mBounced
call $t->UpdateDiveStuck(*$m, 0.3)
printf "11EB8 stuck: bounced %d (1) dir %f %f %f\n", $t->mDive.mBounced, $t->mDive.mBounceDir.mX, $t->mDive.mBounceDir.mY, $t->mDive.mBounceDir.mZ
set var $t->mSafePos.mX = 1.0
set var $t->mSafePos.mY = 0.0
set var $t->mSafePos.mZ = 0.0
call $t->BounceDive(*$m, $t->mSafePos)
printf "1CDD0 normal: dir %f %f %f\n", $t->mDive.mBounceDir.mX, $t->mDive.mBounceDir.mY, $t->mDive.mBounceDir.mZ
set var $t->mSafePos = $safe
set $f59 = $st->mStageInfo.unknown_0xf5cf3c0f
set var $t->mDive.mAttackSent = 1
call $t->SendDiveAttack(*$m)
set var $t->mDive.mAttackSent = 0
set var $t->mDive.mBounced = 1
set var $t->mGuestCD0 = 1
call $t->SendDiveAttack(*$m)
printf "11DDC bounced: cd0 %d (0)\n", $t->mGuestCD0
set var $t->mDive.mAttackSent = 0
set var $t->mDive.mBounced = 0
set var $t->mBattleStageUid.value = 0xFFFF
set var $t->mGuestCD0 = 1
call $t->SendDiveAttack(*$m)
printf "11DDC no stage: cd0 %d (0)\n", $t->mGuestCD0
set var $t->mBattleStageUid.value = $suid
set var $t->mDive.mAttackSent = 0
set var $st->mStageInfo.unknown_0xf5cf3c0f = 0
set var $t->mGuestCD0 = 1
call $t->SendDiveAttack(*$m)
printf "11DDC +0x59 clear: cd0 %d (0)\n", $t->mGuestCD0
set var $t->mDive.mAttackSent = 0
set var $st->mStageInfo.unknown_0xf5cf3c0f = 1
set var $t->mGuestCD0 = 1
call $t->SendDiveAttack(*$m)
printf "11DDC +0x59 set: cd0 %d (1) e08 %f e0c %f\n", $t->mGuestCD0, $t->mGuestE08, $t->mGuestE0C
set var $st->mStageInfo.unknown_0xf5cf3c0f = $f59

# fn_10_12960 FindDiveLanding: the target on the actor (no flat line); every ring path blocked.
set var $t->mSafePos.mX = $tx
set var $t->mSafePos.mY = $ty
set var $t->mSafePos.mZ = $tz
print $t->FindDiveLanding(*$m, &$t->mSafePos)
set var $t->mSafePos.mX = $tx + 3.0
set var $t->mSafePos.mY = $ty
set var $t->mSafePos.mZ = $tz
set $ring0 = 1
print $t->FindDiveLanding(*$m, &$t->mSafePos)
set $ring0 = 0
print $t->FindDiveLanding(*$m, &$t->mSafePos)
set var $t->mSafePos = $safe
set var $t->mDive = $dv
set var $t->mVerticalMovement = $vm
set var $t->mOnGround = $og
set var $t->mOnStaticGround = $osg
set var $t->mGuestCD0 = $cd0
set var $t->mGuestE04 = $e04
set var $t->mGuestE08 = $e08
set var $t->mGuestE0C = $e0c

# fn_10_150DC Dash (Activate): no stage; a double dash; stuck twice (fn_10_14A00); stuck once;
# not stuck; then Update while blocked.
set $dash = $t->mDash
set $still = $t->mStillTime
set $stuck = $t->mDashStuckCount
set $cdd = $st->mStageInfo.chanceToDoubleDash
set var $t->mBattleStageUid.value = 0xFFFF
set var $t->mStillTime = 0.0
call $t->Dash(*$m, (EStateMsg)0, 0.016)
set var $t->mBattleStageUid.value = $suid
set var $st->mStageInfo.chanceToDoubleDash = 101.0
call $t->Dash(*$m, (EStateMsg)0, 0.016)
printf "150DC double: %d (1)\n", $t->mDash.mDoubleDash
set var $st->mStageInfo.chanceToDoubleDash = -1.0
set var $t->mStillTime = 2.0
set var $t->mDashStuckCount = 1
call $t->Dash(*$m, (EStateMsg)0, 0.016)
printf "150DC stuck twice: count %d (2)\n", $t->mDashStuckCount
set var $t->mDashStuckCount = 0
call $t->Dash(*$m, (EStateMsg)0, 0.016)
printf "150DC stuck once: count %d (1)\n", $t->mDashStuckCount
set var $t->mStillTime = 0.0
call $t->Dash(*$m, (EStateMsg)0, 0.016)
printf "150DC not stuck: count %d (0)\n", $t->mDashStuckCount
set var $t->mDash.mDoubleDash = 0
set var $t->mDash.mBlocked = 1
call $t->Dash(*$m, (EStateMsg)1, 0.016)
set var $st->mStageInfo.chanceToDoubleDash = $cdd
set var $t->mDash = $dash
set var $t->mStillTime = $still
set var $t->mDashStuckCount = $stuck

# fn_10_1D860 UpdateAdditiveAnimation: Activate with a score of 0; Deactivate with no animation.
set $aa = $t->mAdditiveAnim
set $fba0 = 1
call $t->UpdateAdditiveAnimation(*$m, (EStateMsg)0)
set $fba0 = 0
printf "1D860 score 0: anim %d (%d)\n", $t->mAdditiveAnim, $aa
set var $t->mAdditiveAnim = -1
call $t->UpdateAdditiveAnimation(*$m, (EStateMsg)2)
printf "1D860 none: anim %d (-1)\n", $t->mAdditiveAnim
set var $t->mAdditiveAnim = $aa

# fn_10_1E3C0 FindBestCoverPoint: the last scored cover point inactive, then in another area.
set $cov = 0
set $capcov = 1
print $t->FindBestCoverPoint(*$m)
set $capcov = 0
printf "1E3C0 cover %p\n", $cov
if $cov != 0
  set $c = (CScriptCoverPoint*)$cov
  set $cact = $c->mActive
  set $carea = $c->mAreaId.value
  set var $c->mActive = 0
  print $t->FindBestCoverPoint(*$m)
  set var $c->mActive = $cact
  set var $c->mAreaId.value = $carea + 77
  print $t->FindBestCoverPoint(*$m)
  set var $c->mAreaId.value = $carea
end

# fn_10_150DC Dash (Activate): no cover point; a pick of weight 0 (blocked, collision failsafe).
set $dash = $t->mDash
set $still = $t->mStillTime
set $stuck = $t->mDashStuckCount
set $lau = $t->mLastAttackerUid.value
set $acn = $t->mAttackerCount
set $au0 = $t->mAttackerUids[0].value
set $au1 = $t->mAttackerUids[1].value
set $cdd = $st->mStageInfo.chanceToDoubleDash
set var $st->mStageInfo.chanceToDoubleDash = -1.0
set var $t->mStillTime = 0.0
set $fbc0 = 1
call $t->Dash(*$m, (EStateMsg)0, 0.016)
set $fbc0 = 0
printf "150DC no cover: last %x (ffff)\n", $t->mLastAttackerUid.value
set $pd0 = 1
call $t->Dash(*$m, (EStateMsg)0, 0.016)
set $pd0 = 0
printf "150DC pick 0: blocked %d (1)\n", $t->mDash.mBlocked
set var $st->mStageInfo.chanceToDoubleDash = $cdd
set var $t->mDash = $dash
set var $t->mStillTime = $still
set var $t->mDashStuckCount = $stuck
set var $t->mLastAttackerUid.value = $lau
set var $t->mAttackerCount = $acn
set var $t->mAttackerUids[0].value = $au0
set var $t->mAttackerUids[1].value = $au1

# fn_10_B6C4 FindBoostBallHint: the first linked hint (radius widened so it holds the actor)
# inactive; of another type; its chain answering true; no object for any uid.
set $hp = 0
set $caphint = 1
print $t->FindBoostBallHint(*$m)
set $caphint = 0
printf "B6C4 hint %p\n", $hp
if $hp != 0
  set $h = (CScriptAIHint*)$hp
  printf "B6C4 type %d radius %f active %d\n", $h->mHintType, $h->mRadius, $h->mActive
  set $hrad = $h->mRadius
  set $hact = $h->mActive
  set $htype = $h->mHintType
  set var $h->mRadius = 1.0e6
  set var $h->mActive = 1
  print $t->FindBoostBallHint(*$m)
  set var $h->mActive = 0
  print $t->FindBoostBallHint(*$m)
  set var $h->mActive = 1
  set var $h->mHintType = (CScriptAIHint::EHintType)0
  print $t->FindBoostBallHint(*$m)
  set var $h->mHintType = $htype
  set $bchi = 1
  print $t->FindBoostBallHint(*$m)
  set $bchi = 0
  set var $h->mRadius = $hrad
  set var $h->mActive = $hact
end
set $gnull = 1
print $t->FindBoostBallHint(*$m)
set $gnull = 0

# fn_10_C2B0 UpdateSweepBeamAim: a gun facing straight up (no flat direction, so the end point is
# the gun and the direction cannot be normalized); an end sound handle (one no emitter owns, so
# UpdateEmitter returns at its handle check).
set $gxf = $t->mGunLctrXf
set $send = $t->mSweepBeamEnd
set $sdir = $t->mSweepBeamDir
set $slen = $t->mSweepBeamLength
set $esfx = $t->mSweepBeamEndSfx.mID
set var $t->mGunLctrXf.m01 = 0.0
set var $t->mGunLctrXf.m11 = 0.0
set var $t->mGunLctrXf.m21 = 1.0
call $t->UpdateSweepBeamAim(*$m, 0.016)
printf "C2B0 up: dir %f %f %f len %f\n", $t->mSweepBeamDir.mX, $t->mSweepBeamDir.mY, $t->mSweepBeamDir.mZ, $t->mSweepBeamLength
set var $t->mGunLctrXf = $gxf
set var $t->mSweepBeamEndSfx.mID = 0xFFFFF000
call $t->UpdateSweepBeamAim(*$m, 0.016)
set var $t->mSweepBeamEndSfx.mID = $esfx

# fn_10_C104 SweepBeamDamage: no beam effect asset.
set $part = $t->mSweepBeamPart
set var $t->mSweepBeamPart = 0
call $t->SweepBeamDamage(*$m)
set var $t->mSweepBeamPart = $part
set var $t->mSweepBeamEnd = $send
set var $t->mSweepBeamDir = $sdir
set var $t->mSweepBeamLength = $slen

# fn_10_B86C ActivateHintChains: never called on the rooftop (only AdvanceBattleStage, for a stage
# whose info +0x35 is 1); called once.
call $t->ActivateHintChains(*$m)
printf "B86C called\n"
printf "ds43: done\n"
