# ds45 forced calls for the ds44 bodies, run once at a CDarkSamus::Think entry
# (tools/cov/specs/darksamus_ds44_calls.py). Every member written here is written back.
set $t = (CDarkSamus*)$rdi
set $m = (CStateManager*)$rsi
printf "ds44: this %p mgr %p\n", $t, $m
set $tx = $t->mPosition.mX
set $ty = $t->mPosition.mY
set $tz = $t->mPosition.mZ
set $ca = $t->mCurrentAttack
set $ecc = $t->mGuestECC
set unwind-on-signal on

# fn_10_B86C ActivateHintChains: the walk recorded (every ObjectById result and the mActive it
# had); the first start null; the first start of another type; the first arrived lookup null (no
# next hint: the walk ends); the first arrived hint inactive; the first arrived hint with a radius.
ds45-recclear
set $rec = 1
call $t->ActivateHintChains(*$m)
set $rec = 0
printf "B86C natural: %d lookups\n", $recn()
set $i = 0
while $i < $recn()
  set $o = $recat($i)
  if $isa($o, "CScriptAIHint")
    set $h = (CScriptAIHint*)$o
    printf "B86C lookup %d: hint %04x type %d radius %f active %d\n", $i, $h->mUniqueId.value, $h->mHintType, $h->mRadius, $h->mActive
  else
    printf "B86C lookup %d: %p not a hint\n", $i, $o
  end
  set $i = $i + 1
end
set $onull = 1
call $t->ActivateHintChains(*$m)
printf "B86C start null done\n"
set $h0 = $recat(0)
set $h1 = $recat(1)
if $isa($h0, "CScriptAIHint")
  set $h = (CScriptAIHint*)$h0
  set $htype = $h->mHintType
  set var $h->mHintType = (CScriptAIHint::EHintType)0
  call $t->ActivateHintChains(*$m)
  set var $h->mHintType = $htype
  printf "B86C start type 0 done\n"
end
set $onull = 2
call $t->ActivateHintChains(*$m)
printf "B86C arrived null done\n"
if $isa($h1, "CScriptAIHint")
  set $h = (CScriptAIHint*)$h1
  set $hrad = $h->mRadius
  set var $h->mActive = 0
  call $t->ActivateHintChains(*$m)
  printf "B86C arrived inactive done (now %d)\n", $h->mActive
  set var $h->mRadius = 5.0
  call $t->ActivateHintChains(*$m)
  set var $h->mRadius = $hrad
  printf "B86C arrived radius 5 done\n"
end
set $onull = 0
ds45-recrestore

# fn_10_12F60 FireScatterShotProjectile: the uid of an object that is not a scatter projectile
# (the actor's own); then no projectile and a chance above 1 (50 -> 0.5). The current attack is
# the scatter shot's 8, so ProjectileInfo (fn_10_1C6C8) answers the scatter infos.
set var $t->mCurrentAttack = 8
set $spu = $t->mScatterProjectileUid
set $sch = $t->mScatterChance
set $ssi = $t->mScatterUseSecondInfo
set var $t->mScatterProjectileUid = $t->mUniqueId
call $t->FireScatterShotProjectile(*$m)
printf "12F60 not scatter: uid %04x (new)\n", $t->mScatterProjectileUid.value
set var $t->mScatterProjectileUid.value = 0xFFFF
set var $t->mScatterChance = 50.0
call $t->FireScatterShotProjectile(*$m)
printf "12F60 chance 50: second %d uid %04x\n", $t->mScatterUseSecondInfo, $t->mScatterProjectileUid.value
# a chance of 0 (not above 1: the first info) whose weapon token is neither cached nor locked
# (TryCache false: the shot is dropped).
set var $t->mScatterProjectileUid.value = 0xFFFF
set var $t->mScatterChance = 0.0
set var $t->mScatterUseSecondInfo = 0
set $pi = (CProjectileInfo*)$t->ProjectileInfo()
set $pitem = $pi->mWeaponDescription.mItem
set $plock = $pi->mWeaponDescription.mLockHeld
set var $pi->mWeaponDescription.mItem = 0
set var $pi->mWeaponDescription.mLockHeld = 0
call $t->FireScatterShotProjectile(*$m)
printf "12F60 no cache: uid %04x (ffff)\n", $t->mScatterProjectileUid.value
set var $pi->mWeaponDescription.mItem = $pitem
set var $pi->mWeaponDescription.mLockHeld = $plock
set var $t->mScatterChance = $sch
set var $t->mScatterUseSecondInfo = $ssi
set var $t->mScatterProjectileUid = $spu
set var $t->mCurrentAttack = $ca

# fn_10_B164 BoostBallInto: Activate with no boost hint (then no hint uid: toward the player);
# Activate with no arrived hint; Activate with the arrived hint's value parm 0, 3 and 1; a message
# with a hint uid that is not an actor (the battle stage's).
set $bhu = $t->mBoostHintUid
set $bhd = $t->mBoostHintDist
set $bhv = $t->mBoostHintValue
set $ec8 = $t->mGuestEC8
set $f40 = $t->mBoostBallFlag40
set $rsfx = $t->mBoostBallRollSfx
set $fbh0 = 1
call $t->BoostBallInto(*$m, (EStateMsg)0, 0.016)
printf "B164 no boost hint: hint %04x (ffff)\n", $t->mBoostHintUid.value
set $fbh0 = 0
set $fah0 = 1
call $t->BoostBallInto(*$m, (EStateMsg)0, 0.016)
printf "B164 no arrived hint: hint %04x (ffff)\n", $t->mBoostHintUid.value
set $fah0 = 0
set $ahint = 0
set $capah = 1
call $t->BoostBallInto(*$m, (EStateMsg)0, 0.016)
set $capah = 0
printf "B164 arrived hint %p value %f\n", $ahint, $t->mBoostHintValue
if $ahint != 0
  set $ahp = (CScriptAIHint*)$ahint
  set $avp = $ahp->mValueParm
  set var $ahp->mValueParm = 0.0
  call $t->BoostBallInto(*$m, (EStateMsg)0, 0.016)
  printf "B164 value 0: %f (1)\n", $t->mBoostHintValue
  set var $ahp->mValueParm = 3.0
  call $t->BoostBallInto(*$m, (EStateMsg)0, 0.016)
  printf "B164 value 3: %f (2)\n", $t->mBoostHintValue
  set var $ahp->mValueParm = 1.5
  call $t->BoostBallInto(*$m, (EStateMsg)0, 0.016)
  printf "B164 value 1.5: %f (1.5)\n", $t->mBoostHintValue
  set var $ahp->mValueParm = $avp
end
set var $t->mBoostHintUid = $t->mBattleStageUid
call $t->BoostBallInto(*$m, (EStateMsg)2, 0.016)
set var $t->mBoostHintUid.value = 0xFFFF
call $t->BoostBallInto(*$m, (EStateMsg)2, 0.016)
printf "B164 stale / invalid hint uid done\n"
set var $t->mBoostHintUid = $bhu
set var $t->mBoostHintDist = $bhd
set var $t->mBoostHintValue = $bhv
set var $t->mGuestEC8 = $ec8
set var $t->mGuestECC = $ecc
set var $t->mBoostBallFlag40 = $f40
set var $t->mBoostBallRollSfx = $rsfx
set var $t->mCurrentAttack = $ca

# fn_10_C470 UpdateSweepBeamEnd: the natural ray with the beam on (the response data kept); a
# forced hit 10 units out with the beam on (decal as the data has it); the same with the decal
# entry made invalid; a forced hit with the beam off.
set $send = $t->mSweepBeamEnd
set $son = $t->mSweepBeamOn
set var $t->mSweepBeamOn = 1
set $crd = 0
set $capcrd = 1
set $rs = 1
set $rsn = 0
set $rs1_back = 0
call $t->UpdateSweepBeamEnd(*$m)
printf "C470 natural on: end %f %f %f\n", $t->mSweepBeamEnd.mX, $t->mSweepBeamEnd.mY, $t->mSweepBeamEnd.mZ
set $rsn = 0
set $rs1_back = 90
call $t->UpdateSweepBeamEnd(*$m)
set $capcrd = 0
printf "C470 hit on: end %f %f %f crd %p\n", $t->mSweepBeamEnd.mX, $t->mSweepBeamEnd.mY, $t->mSweepBeamEnd.mZ, $crd
if $crd != 0
  set $cr = (CCollisionResponseData*)$crd
  printf "C470 decals %d valid1 %d\n", $cr->mDecalTokens.mCount, $cr->mDecalTokens.mItems[1].m_valid
  # the type-1 decal is invalid in the shipped CRSC: swap a valid entry into slot 1 for one
  # forced hit (a swap, so every token's lock count stays as it was), then swap it back.
  ds45-decalswap
  if $dswap >= 0
    set $rsn = 0
    call $t->UpdateSweepBeamEnd(*$m)
    printf "C470 hit on, decal valid (slot %d) done\n", $dswap
    ds45-decalswap
  end
end
# the gun facing straight up: no flat direction.
set $gxf = $t->mGunLctrXf
set var $t->mGunLctrXf.m01 = 0.0
set var $t->mGunLctrXf.m11 = 0.0
set var $t->mGunLctrXf.m21 = 1.0
call $t->UpdateSweepBeamEnd(*$m)
set var $t->mGunLctrXf = $gxf
set var $t->mSweepBeamOn = 0
set $rsn = 0
call $t->UpdateSweepBeamEnd(*$m)
printf "C470 hit off done\n"
set $rs = 0
set var $t->mSweepBeamOn = $son
set var $t->mSweepBeamEnd = $send

# fn_10_1905C ClassifyLineToPlayer: the actor on the player's xy (no flat line: 0); 20 units out
# on x with the forced hits (dist+2, dist-2, behind the start) -> 1; (dist-2, dist+2, none) -> 1;
# (dist+2, none, none) -> 2; and the natural rays from there.
set $px = $m->mPlayers[0]->mPosition.mX
set $py = $m->mPlayers[0]->mPosition.mY
set var $t->mPosition.mX = $px
set var $t->mPosition.mY = $py
printf "1905C on the player: ds %f %f player %f %f\n", $t->mPosition.mX, $t->mPosition.mY, $px, $py
print $t->ClassifyLineToPlayer(*$m)
# the actor's x made +inf (bits 0x7f800000): the flat line has an inf component (0x00019144).
set var *(unsigned int*)&$t->mPosition.mX = 0x7f800000
printf "1905C inf x: ds %f\n", $t->mPosition.mX
print $t->ClassifyLineToPlayer(*$m)
set var $t->mPosition.mX = $px + 20.0
set $rw = 1
set $rwn = 0
set $rw1_back = 3
set $rw2_back = 7
set $rw3_back = -1
print $t->ClassifyLineToPlayer(*$m)
set $rwn = 0
set $rw1_back = 7
set $rw2_back = 3
set $rw3_back = 0
print $t->ClassifyLineToPlayer(*$m)
set $rwn = 0
set $rw1_back = 3
set $rw2_back = 0
print $t->ClassifyLineToPlayer(*$m)
set $rw = 0
print $t->ClassifyLineToPlayer(*$m)
set var $t->mPosition.mX = $tx
set var $t->mPosition.mY = $ty

# fn_10_1707C UpdateDodge (dt 0.1, the impulse 22 and time 0.5 of 0x00016E3C): a running dodge
# (0.3 left); one that runs out (0.05 left) into a clear line 20 units out (no dodge); then from
# no dodge, with the gun-line answers forced in call order: the first side line blocked and the
# right dodge clear; both side lines' second blocked and only the left dodge clear; every line
# blocked (no dodge); the actor 2 units out (too close) with the right dodge clear; the actor's x
# +inf (no flat line).
set $ddt = $t->mDodgeTime
set $ddd = $t->mDodgeDir
set $dimp = $t->mImpulse
set var $t->mPosition.mX = $px + 20.0
set var $t->mPosition.mY = $py
set var $t->mDodgeTime = 0.3
call $t->UpdateDodge(*$m, 0.1, 22.0, 0.5)
printf "1707C running: %f\n", $t->mDodgeTime
set var $t->mDodgeTime = 0.05
ds45-glc 11
call $t->UpdateDodge(*$m, 0.1, 22.0, 0.5)
printf "1707C ran out, clear: %f (0)\n", $t->mDodgeTime
ds45-glc 01
call $t->UpdateDodge(*$m, 0.1, 22.0, 0.5)
printf "1707C right: %f dir %f %f\n", $t->mDodgeTime, $t->mDodgeDir.mX, $t->mDodgeDir.mY
set var $t->mDodgeTime = 0.0
ds45-glc 1001
call $t->UpdateDodge(*$m, 0.1, 22.0, 0.5)
printf "1707C left: %f dir %f %f\n", $t->mDodgeTime, $t->mDodgeDir.mX, $t->mDodgeDir.mY
set var $t->mDodgeTime = 0.0
ds45-glc 000
call $t->UpdateDodge(*$m, 0.1, 22.0, 0.5)
printf "1707C none: %f (0)\n", $t->mDodgeTime
set var $t->mDodgeTime = 0.0
set var $t->mPosition.mX = $px + 2.0
ds45-glc 1
call $t->UpdateDodge(*$m, 0.1, 22.0, 0.5)
printf "1707C close: %f dir %f %f\n", $t->mDodgeTime, $t->mDodgeDir.mX, $t->mDodgeDir.mY
set var $t->mDodgeTime = 0.0
set var *(unsigned int*)&$t->mPosition.mX = 0x7f800000
ds45-glc
call $t->UpdateDodge(*$m, 0.1, 22.0, 0.5)
printf "1707C inf: %f (0)\n", $t->mDodgeTime
set var $t->mPosition.mX = $tx
set var $t->mPosition.mY = $ty
set var $t->mDodgeTime = $ddt
set var $t->mDodgeDir = $ddd
set var $t->mImpulse = $dimp

# fn_10_1C99C CheckDiveCollision on a list built here (normals zero, so a bounce does not turn
# the actor): the five guards (not the dive, not launched, bounced, landing, done) with a wall
# contact; a wall with no collider (bounce); a ceiling whose collider (the player) the dive's
# weapon hits (no bounce); a wall it does not hit (bounce); a wall-and-floor contact (no bounce,
# the floor lands); platform-and-floor, nothing, then the player (hit, lands); the same once the
# player was hit; a contact with no listed material; an empty list. Materials (bit numbers):
# ceiling 29, wall 30, floor 31, player 32, platform 49.
# Scratch memory: ds45-scratch (the .py) zeroes 0x3000 bytes under the stack into $scratch.
ds45-scratch
define ds45-dive
  set var $t->mCurrentAttack = 0x20
  set var $t->mDive.mLaunched = 1
  set var $t->mDive.mBounced = 0
  set var $t->mDive.mLanding = 0
  set var $t->mDive.mDone = 0
  set var $t->mDive.mHitFloor = 0
  set var $t->mDive.mHitPlayer = $arg0
end
define ds45-ci
  set var ((CCollisionInfo*)$dcl->mList.mData)[$arg0].mMaterialLeft.mValue = $arg1
end
define ds45-dcall
  call $t->CheckDiveCollision(*$m, (CActor*)$arg0, *$dcl)
  printf "1C99C %s: bounced %d floor %d hitplayer %d\n", $arg1, $t->mDive.mBounced, $t->mDive.mHitFloor, $t->mDive.mHitPlayer
end
set $dive = $t->mDive
set $dvm = $t->mVerticalMovement
set $dog = $t->mOnGround
set $dosg = $t->mOnStaticGround
set $dxf = $t->mTransform
set $dcl = (CCollisionInfoList*)$scratch
printf "1C99C list %p size %d\n", $dcl, sizeof(CCollisionInfoList)
set var $dcl->mList.mCount = 1
ds45-ci 0 0x40000000
ds45-dive 0
set var $t->mCurrentAttack = 0x40
ds45-dcall 0 "not_the_dive"
ds45-dive 0
set var $t->mDive.mLaunched = 0
ds45-dcall 0 "not_launched"
ds45-dive 0
set var $t->mDive.mBounced = 1
ds45-dcall 0 "bounced"
ds45-dive 0
set var $t->mDive.mLanding = 1
ds45-dcall 0 "landing"
ds45-dive 0
set var $t->mDive.mDone = 1
ds45-dcall 0 "done"
ds45-dive 0
ds45-dcall 0 "wall_no_collider_bounce"
ds45-ci 0 0x20000000
ds45-dive 0
set $whv = 1
set $wh = 1
ds45-dcall $m->mPlayers[0] "ceiling_weapon_hits_none"
ds45-ci 0 0x40000000
ds45-dive 0
set $whv = 0
set $wh = 1
ds45-dcall $m->mPlayers[0] "wall_weapon_does_not_hit_bounce"
set $wh = 0
ds45-ci 0 0xC0000000
ds45-dive 0
ds45-dcall 0 "wall_and_floor_floor"
set var $dcl->mList.mCount = 3
ds45-ci 0 0x2000080000000
ds45-ci 1 0
ds45-ci 2 0x100000000
ds45-dive 0
ds45-dcall 0 "platform_none_player_hit_floor"
ds45-dive 1
ds45-dcall 0 "the_same_player_hit_before_floor"
set var $dcl->mList.mCount = 1
ds45-ci 0 0x1
ds45-dive 0
ds45-dcall 0 "stone_only_none"
set var $dcl->mList.mCount = 0
ds45-dive 0
ds45-dcall 0 "empty_none"
set var $t->mDive = $dive
set var $t->mVerticalMovement = $dvm
set var $t->mOnGround = $dog
set var $t->mOnStaticGround = $dosg
set var $t->mTransform = $dxf
set var $t->mCurrentAttack = $ca

# fn_10_19B64 CollectReachAttacks (the direct bool callees forced in call order, $seqcaller 1):
# every mask bit with every check failing and the player beyond the min range; every check
# passing, the player inside it; no mask bit (only the walk check runs); the 0x10 arm with the
# player put beyond each dodge side (slot 2 nearer, then slot 3), each with the first path test
# failing, the second failing and the walk test failing.
set $seqcaller = 1
set $rw8 = (CDarkSamus::SAttackWeights*)($scratch + 0x1000)
set $rmask = (unsigned int*)($scratch + 0x2000)
set $mar = $t->mMinAttackRange
set $ssd = $t->mScatterSide
set var $t->mMinAttackRange = 0.0
set var *$rmask = 0xFFFFFFFF
ds45-seq aim 0
ds45-seq walk 0
ds45-seq ring 0
ds45-seq reach 00
ds45-seq onpath 0
call $t->CollectReachAttacks(*$m, *$rw8, $rmask)
printf "19B64 all fail: mask %08x\n", *$rmask
set var $t->mMinAttackRange = 1.0e9
set var *$rmask = 0xFFFFFFFF
ds45-seq aim 1
ds45-seq walk 1
ds45-seq ring 1
ds45-seq reach 11
ds45-seq onpath 11
ds45-seq walkpt 1
call $t->CollectReachAttacks(*$m, *$rw8, $rmask)
printf "19B64 all pass: mask %08x\n", *$rmask
set var *$rmask = 0xFFFFFFFF
ds45-seq reach 00
call $t->CollectReachAttacks(*$m, *$rw8, $rmask)
printf "19B64 walkable, out of reach: mask %08x\n", *$rmask
set var *$rmask = 0
ds45-seq walk 1
call $t->CollectReachAttacks(*$m, *$rw8, $rmask)
printf "19B64 no bits: mask %08x\n", *$rmask
set $o2 = $t->GetDodgeStepOffset(2)
set $o3 = $t->GetDodgeStepOffset(3)
set $pp = $m->mPlayers[0]->mPosition
printf "19B64 offsets %f %f / %f %f\n", $o2.mX, $o2.mY, $o3.mX, $o3.mY
set $side = 2
while $side <= 3
  if $side == 2
    set var $m->mPlayers[0]->mPosition.mX = $t->mPosition.mX + 10.0 * $o2.mX
    set var $m->mPlayers[0]->mPosition.mY = $t->mPosition.mY + 10.0 * $o2.mY
  else
    set var $m->mPlayers[0]->mPosition.mX = $t->mPosition.mX + 10.0 * $o3.mX
    set var $m->mPlayers[0]->mPosition.mY = $t->mPosition.mY + 10.0 * $o3.mY
  end
  set var $m->mPlayers[0]->mPosition.mZ = $t->mPosition.mZ
  set $k = 0
  while $k < 4
    set var *$rmask = 0x10
    ds45-seq walk 1
    if $k == 0
      ds45-seq onpath 0
    end
    if $k == 1
      ds45-seq onpath 10
    end
    if $k == 2
      ds45-seq onpath 11
      ds45-seq walkpt 0
    end
    if $k == 3
      ds45-seq onpath 11
      ds45-seq walkpt 1
    end
    call $t->CollectReachAttacks(*$m, *$rw8, $rmask)
    printf "19B64 side %d case %d: scatter side %d mask %08x\n", $side, $k, $t->mScatterSide, *$rmask
    set $k = $k + 1
  end
  set $side = $side + 1
end
set var $m->mPlayers[0]->mPosition = $pp
ds45-seq aim
ds45-seq walk
ds45-seq ring
ds45-seq reach
ds45-seq onpath
ds45-seq walkpt
set $seqcaller = 0
set var $t->mMinAttackRange = $mar
set var $t->mScatterSide = $ssd

# fn_10_CA0C SweepBeam: Activate; Update with the beam on and a sound; Update with the beam off and
# no sound; Deactivate with both handles set, then with both zero; an unknown message (3).
set $sbs = $t->mSweepBeamSpeed
set $sps = $t->mSpeed
set $sbon = $t->mSweepBeamOn
set $sbhp = $t->mSweepBeamHitPlayer
set $sbd = $t->mSweepBeamDir
set $sbpd = $t->mSweepBeamPrevDir
set $sbsfx = $t->mSweepBeamSfx.mID
set $sbesfx = $t->mSweepBeamEndSfx.mID
set var $t->mSweepBeamSfx.mID = 0
set var $t->mSweepBeamEndSfx.mID = 0
call $t->SweepBeam(*$m, (EStateMsg)0, 0.016)
printf "CA0C activate: sfx %x end %x speed %f kept %f\n", $t->mSweepBeamSfx.mID, $t->mSweepBeamEndSfx.mID, $t->mSpeed, $t->mSweepBeamSpeed
set $h1 = $t->mSweepBeamSfx.mID
set $h2 = $t->mSweepBeamEndSfx.mID
if $h1 == 0
  set var $t->mSweepBeamSfx.mID = 0x7fff0fff
end
set var $t->mSweepBeamOn = 1
call $t->SweepBeam(*$m, (EStateMsg)1, 0.016)
printf "CA0C update on: on %d\n", $t->mSweepBeamOn
set var $t->mSweepBeamOn = 0
set var $t->mSweepBeamSfx.mID = 0
call $t->SweepBeam(*$m, (EStateMsg)1, 0.016)
printf "CA0C update off\n"
set var $t->mSweepBeamSfx.mID = $h1
set var $t->mSweepBeamEndSfx.mID = $h2
if $h1 == 0 || $h2 == 0
  printf "CA0C: AddEmitter gave a null handle (%x %x)\n", $h1, $h2
end
# The rooftop boss's sweep sounds give null handles: stand-ins that StopSound ignores (index
# 0xFFF is past the channel's slots) take the remove arms.
if $h1 == 0
  set var $t->mSweepBeamSfx.mID = 0x7fff0fff
end
if $h2 == 0
  set var $t->mSweepBeamEndSfx.mID = 0x7fff0fff
end
call $t->SweepBeam(*$m, (EStateMsg)2, 0.016)
printf "CA0C deactivate set: sfx %x end %x speed %f\n", $t->mSweepBeamSfx.mID, $t->mSweepBeamEndSfx.mID, $t->mSpeed
call $t->SweepBeam(*$m, (EStateMsg)2, 0.016)
call $t->SweepBeam(*$m, (EStateMsg)3, 0.016)
printf "CA0C deactivate zero, msg 3 done\n"
set var $t->mSweepBeamSpeed = $sbs
set var $t->mSpeed = $sps
set var $t->mSweepBeamOn = $sbon
set var $t->mSweepBeamHitPlayer = $sbhp
set var $t->mSweepBeamDir = $sbd
set var $t->mSweepBeamPrevDir = $sbpd
set var $t->mSweepBeamSfx.mID = $sbsfx
set var $t->mSweepBeamEndSfx.mID = $sbesfx

# fn_10_CEB4 GetOrigin: no team holds the rooftop boss, so one forced call (the role and point
# are unused: the actor itself and its 0xECC vector stand in).
set $rv = $t->GetOrigin(*$m, *(CTeamAiRole*)$t, $t->mGuestECC)
printf "CEB4 -> %f %f %f pos %f %f %f\n", $rv.mX, $rv.mY, $rv.mZ, $t->mPosition.mX, $t->mPosition.mY, $t->mPosition.mZ

# fn_10_1D9A4 GetRepulsionPoint: natural; every connection's uid invalid; a repulsor (the actor's
# own, else the first in the object list answered for every connection): holding the actor with
# and without a strength, a cylinder, repel-player, another area, inactive, outside; no path
# area; a region with an obstruction; a contact under and over 0.3 s ago.
ds45-findrep
printf "1D9A4 crep %p anyrep %p uid %04x\n", $crep, $anyrep, $anyrepuid
set $ct = $t->mContactTime
set $rv = $t->GetRepulsionPoint(*$m, (float*)0)
printf "1D9A4 -> %f %f %f\n", $rv.mX, $rv.mY, $rv.mZ
set $gifsuid = 0xFFFF
set $gifs = -1
set $rv = $t->GetRepulsionPoint(*$m, (float*)0)
printf "1D9A4 -> %f %f %f\n", $rv.mX, $rv.mY, $rv.mZ
set $gifs = 0
set $rp = $crep
if $rp == 0 && $anyrep != 0
  set $rp = $anyrep
  set $gifsuid = $anyrepuid
  set $gifs = -1
end
if $rp != 0
  set $r = (CScriptRepulsor*)$rp
  set $ract = $r->mActive
  set $rarea = $r->mAreaId.value
  set $rflags = $r->mFlags
  set $rshape = $r->mShape
  set $rrad = $r->mRadius
  set var $r->mActive = 1
  set var $r->mAreaId.value = $t->mAreaId.value
  set var $r->mFlags = 0
  set var $r->mShape = (CScriptRepulsor::EShape)0
  set var $r->mRadius = 1.0e4
  set $rv = $t->GetRepulsionPoint(*$m, (float*)0)
  printf "1D9A4 -> %f %f %f\n", $rv.mX, $rv.mY, $rv.mZ
  set $rv = $t->GetRepulsionPoint(*$m, &$t->mGuestECC.mX)
  printf "1D9A4 -> %f %f %f\n", $rv.mX, $rv.mY, $rv.mZ
  printf "1D9A4 strength %f\n", $t->mGuestECC.mX
  set var $r->mShape = (CScriptRepulsor::EShape)1
  set $rv = $t->GetRepulsionPoint(*$m, (float*)0)
  printf "1D9A4 -> %f %f %f\n", $rv.mX, $rv.mY, $rv.mZ
  set var $r->mShape = (CScriptRepulsor::EShape)0
  set var $r->mFlags = 1
  set $rv = $t->GetRepulsionPoint(*$m, (float*)0)
  printf "1D9A4 -> %f %f %f\n", $rv.mX, $rv.mY, $rv.mZ
  set var $r->mFlags = 0
  set var $r->mAreaId.value = $t->mAreaId.value + 77
  set $rv = $t->GetRepulsionPoint(*$m, (float*)0)
  printf "1D9A4 -> %f %f %f\n", $rv.mX, $rv.mY, $rv.mZ
  set var $r->mAreaId.value = $t->mAreaId.value
  set var $r->mActive = 0
  set $rv = $t->GetRepulsionPoint(*$m, (float*)0)
  printf "1D9A4 -> %f %f %f\n", $rv.mX, $rv.mY, $rv.mZ
  set var $r->mActive = 1
  set var $r->mRadius = 0.001
  set $rv = $t->GetRepulsionPoint(*$m, (float*)0)
  printf "1D9A4 -> %f %f %f\n", $rv.mX, $rv.mY, $rv.mZ
  set var $r->mActive = $ract
  set var $r->mAreaId.value = $rarea
  set var $r->mFlags = $rflags
  set var $r->mShape = $rshape
  set var $r->mRadius = $rrad
  set var $t->mGuestECC = $ecc
end
set $gifs = 0
set $ds_pc = $m->mWorld.mPtr->mAreas.mItems[$t->mAreaId.value].mItem->mPostConst.mPtr
set $pa = $ds_pc->mPathArea
set var $ds_pc->mPathArea = 0
set $rv = $t->GetRepulsionPoint(*$m, (float*)0)
printf "1D9A4 -> %f %f %f\n", $rv.mX, $rv.mY, $rv.mZ
set var $ds_pc->mPathArea = $pa
set $obs1 = -1
set $rv = $t->GetRepulsionPoint(*$m, (float*)0)
printf "1D9A4 -> %f %f %f\n", $rv.mX, $rv.mY, $rv.mZ
set $obs1 = 0
set var $t->mContactTime = $t->mElapsedTime
set $rv = $t->GetRepulsionPoint(*$m, (float*)0)
printf "1D9A4 -> %f %f %f\n", $rv.mX, $rv.mY, $rv.mZ
set var $t->mContactTime = -1000.0
set $rv = $t->GetRepulsionPoint(*$m, (float*)0)
printf "1D9A4 -> %f %f %f\n", $rv.mX, $rv.mY, $rv.mZ
set var $t->mContactTime = $ct
set var $t->mCurrentAttack = $ca
printf "ds44: done\n"
