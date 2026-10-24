# dtr9: DoUserAnimEvent called again at a DoUserAnimEvent entry (this rdi, mgr rsi, node rdx).
set $dU = 1
set $t = (CDarkTrooper*)$rdi
set $m = (CStateManager*)$rsi
set $n = (CInt32POINode*)$rdx
printf "event: this %p\n", $t
set $fm = $t->mFiresMissiles
set $as = $t->mAttackState
set $sf = $t->mSmallShotAttack.mShotsFired
set $sl = $t->mSmallShotAttack.mShotLimit
set $mf = $t->mMissileAttack.mShotsFired
set $ml = $t->mMissileAttack.mShotLimit
set $los = $t->mLineOfSightTracker.mHasLineOfSight
set $tt = $t->mTransitioning
# BecomeRagDoll with mFiresMissiles set (no CreateRagDoll)
set var $t->mFiresMissiles = 1
call $t->DoUserAnimEvent(*$m, *$n, (EUserEventType)19, 0.0f)
set var $t->mFiresMissiles = $fm
# Landing: attack state neither 5 nor 1
set var $t->mAttackState = 2
call $t->DoUserAnimEvent(*$m, *$n, (EUserEventType)12, 0.0f)
set var $t->mAttackState = 5
call $t->DoUserAnimEvent(*$m, *$n, (EUserEventType)12, 0.0f)
set var $t->mAttackState = 1
call $t->DoUserAnimEvent(*$m, *$n, (EUserEventType)12, 0.0f)
# Activate: attack state < 3, 3..4, >= 5
set var $t->mAttackState = 2
call $t->DoUserAnimEvent(*$m, *$n, (EUserEventType)29, 0.0f)
set var $t->mAttackState = 3
call $t->DoUserAnimEvent(*$m, *$n, (EUserEventType)29, 0.0f)
set var $t->mAttackState = 5
call $t->DoUserAnimEvent(*$m, *$n, (EUserEventType)29, 0.0f)
set var $t->mTransitioning = $tt
printf "event simple done\n"
# EndAction, state 0 (neither case), then the small-shot and missile counters
set var $t->mAttackState = 0
call $t->DoUserAnimEvent(*$m, *$n, (EUserEventType)18, 0.0f)
set var $t->mAttackState = 5
set var $t->mSmallShotAttack.mShotsFired = 0
set var $t->mSmallShotAttack.mShotLimit = 9
set var $t->mLineOfSightTracker.mHasLineOfSight = 1
call $t->DoUserAnimEvent(*$m, *$n, (EUserEventType)18, 0.0f)
set var $t->mLineOfSightTracker.mHasLineOfSight = 0
call $t->DoUserAnimEvent(*$m, *$n, (EUserEventType)18, 0.0f)
set var $t->mSmallShotAttack.mShotsFired = 20
set var $t->mLineOfSightTracker.mHasLineOfSight = 1
call $t->DoUserAnimEvent(*$m, *$n, (EUserEventType)18, 0.0f)
printf "event small done\n"
set var $t->mAttackState = 1
set var $t->mMissileAttack.mShotsFired = 0
set var $t->mMissileAttack.mShotLimit = 9
set var $t->mLineOfSightTracker.mHasLineOfSight = 1
call $t->DoUserAnimEvent(*$m, *$n, (EUserEventType)18, 0.0f)
set var $t->mLineOfSightTracker.mHasLineOfSight = 0
call $t->DoUserAnimEvent(*$m, *$n, (EUserEventType)18, 0.0f)
set var $t->mMissileAttack.mShotsFired = 20
set var $t->mLineOfSightTracker.mHasLineOfSight = 1
call $t->DoUserAnimEvent(*$m, *$n, (EUserEventType)18, 0.0f)
printf "event missile done\n"
set var $t->mAttackState = $as
set var $t->mSmallShotAttack.mShotsFired = $sf
set var $t->mSmallShotAttack.mShotLimit = $sl
set var $t->mMissileAttack.mShotsFired = $mf
set var $t->mMissileAttack.mShotLimit = $ml
set var $t->mLineOfSightTracker.mHasLineOfSight = $los
