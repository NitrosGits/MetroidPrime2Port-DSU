// Host body for the DarkSamus REL (module 10): the constructor and the vtable overrides; the
// state machine and the attacks are in PortCDarkSamusFsm*.cpp / PortCDarkSamusHelpers*.cpp
// (docs/module-analysis/DarkSamus.md).
//
// Which methods the REL overrides is not guessed: its vtable lbl_10_data_CBC lists them by address
// and the slots line up with the CEntity -> CActor -> CPhysicsActor -> CAi -> CPatterned chain
// (docs/module-analysis/vtable-offsets.md); the entries the REL inherits keep the DOL's own
// symbols, so the table also says what is *not* overridden (slot 4 is CPatterned::PreThink, slot
// 19 Touch, slot 38 Death, slot 39 KnockBack, slot 60 SetupStateMachine). The tiny accessors at the
// head of the module's .text (fn_10_CE4C..fn_10_CEB4) have four-to-twelve-instruction bodies, so
// they are real rather than stand-ins.
#include "MetroidPrime/Enemies/CDarkSamus.hpp"

#include "Kyoto/Animation/CInt32POINode.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CEnvFxManager.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Enemies/CDarkSamusBattleStage.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "Kyoto/Graphics/CGX.hpp"
#include "Kyoto/Graphics/CGraphics.hpp"
#include "Kyoto/Animation/CSkinnedModel.hpp"
#include "MetroidPrime/CSimpleShadow.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "Kyoto/Particles/CParticleSwoosh.hpp"
#include "Kyoto/Animation/CCharLayoutInfo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrDarkSamus.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/Weapons/CGameProjectile.hpp"
#include "MetroidPrime/ScriptObjects/CScriptPlatform.hpp"
#include "Kyoto/Text/CStringTable.hpp"
#include "Collision/CCollisionInfoList.hpp"
#include "Collision/CMaterialFilter.hpp"

#include <math.h>
#include <stdio.h>

// fn_10_1FA14. The base call is the retail one: EPatternedAI 7 (`li r4, 0x7` at 0x0001FB00),
// flavor 0 (`li r7, 0x0` at 0x0001FB08), movement 0 / collider 1 / body 8 (the constants stored to
// the stack arguments at 0x0001FA84..0x0001FABC), all passed to CPatterned at 0x0001FB2C. The
// body type 8 is the host's kBT_4WayBlended (include/Kyoto/Animation/CharacterCommon.hpp).
CDarkSamus::CDarkSamus(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                       const CTransform4f& xf, const CModelData& modelData,
                       const CPatternedInfo& pInfo, const CActorParameters& actParms,
                       const CDarkSamusData& data)
: CPatterned(static_cast< EPatternedAI >(7), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_4WayBlended, actParms)
, mPathFindSearch(nullptr, 0x401, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0,
                  CPFRegion::kRP_Center) // 0x0001FB44: fn_8013E6A0, the CPathFindSearch at 0x7C0
, mData(data)
, mGuestCD0(0)
, mFlags90C(0)
, mTurnDone(0)
, mPissedOff(0)
, mFadeOut(0)
, mBattleStageUid(kInvalidUniqueId)
, mStageCount(0)
, mUseAltScan(false)                            // 0x0001FCD0
, mCanDie(true)                                 // 0x0001FCDC
, mPhazonHealthMark(0.f)                        // 0x00020214
, mPhazonPhase(false)                           // 0x00020224
, mGuestD24(0.f)                                // 0x0000E5DC
, mGuestCD4(CDamageVulnerability::ReflectVulnerabilty())            // 0x0000E58C
, mEnrageVulnerability(CDamageVulnerability::ReflectVulnerabilty()) // 0x00020FC4
, mSeenStageUid(kInvalidUniqueId)
, mSeenStageCount(1)                            // 0x0002001C
, mStageChanged(0)                              // 0x00020028
, mGuestD18(0.3f, 0.6f, 1.f)                  // 0x0000E5C8 (lbl_10_bss_128)
, mSweepBeamWeaponMode(CWeaponMode(kWT_Phazon))       // lbl_10_bss_114 (fn_10_CDB8, 0x0000CE30)
, mLastAttackerUid(kInvalidUniqueId)
, mAttackerCount(0)
, mAttackerUids{kInvalidUniqueId, kInvalidUniqueId}
, mPickedAttack(0xFFFF)
, mCurrentAttack(0xFFFF)
, mAttackHistory{0, 0, 0}
, mAttackHistoryCount(0)
, mAccumulatedDamage(0.f)
, mLastHealth(-1.f)
, mDamageDecay(1.f)
, mDamageThreshold(data.mDamageInterruptThreshold)
, mStillTime(0.f)
, mDashStuckCount(0)
, mStillPosition(CVector3f(0.f, 0.f, 0.f))
, mLastWallHitTime(0.f)
, mElapsedTime(0.f)
, mPathFindDestination(CVector3f(0.f, 0.f, 0.f))
, mAdditiveAnim(-1)
, mGlideSfxId(data.mGlideSound == -1 ? CSfxManager::kInternalInvalidSfxId
                                    : static_cast< ushort >(data.mGlideSound))
, mGlideSfx()
, mSavedSpeed(0.f)
, mDash()
, mGuestC00(0.f)
, mSafePos(CVector3f(0.f, 0.f, 0.f))
, mHasSafePos(0)
, mSeparationDir(CVector3f(0.f, 0.f, 0.f))
, mContactTime(0.f)
, mContactPoint(CVector3f(0.f, 0.f, 0.f))
, mLastMeleeTime(0.f)
, mAttackMask(0)
, mWeightedPick(0xFFFF)
, mPickCount(0)
, mLastPickTime(-1000.f)   // lbl_10_rodata_410, `lfs f0` at 0x0001FC9C, stored at 0x0001FCF0
, mNextActionTime(-1000.f) // the same f0, 0x0001FCF4
, mAttackOrbitPos(CVector3f(0.f, 0.f, 0.f))
, mAttackAimPos(CVector3f(0.f, 0.f, 0.f))
, mScatterLineStart(CVector3f(0.f, 0.f, 0.f)) // fn_10_215FC, 0x00021660..0x000216A4
, mScatterAim(CVector3f(0.f, 0.f, 0.f))
, mScatterLineEnd(CVector3f(0.f, 0.f, 0.f))
, mScatterShotCount(0)
, mScatterStartTime(0.f)
, mScatterTimer(0.f)
, mScatterFiring(0)
, mScatterChance(data.mUnknown378285B9)         // 0x000216C4
, mScatterRate(data.mScatterShotProjectilesPerSecond) // 0x000216C8
, mScatterProjectileUid(kInvalidUniqueId)      // 0x000216D0
, mScatterSide(0)                               // 0x000216B4
, mScatterUseSecondInfo(0)                      // 0x000216BC
, mDive(data.mDiveAttackDamage, data.mDiveAttackImpulseHorizontal,
        data.mDiveAttackImpulseVertical)         // fn_10_E784 at 0x0001FE64
, mGuestE04(0.f) // not written by the constructor; fn_10_18628 is the only writer
, mGuestE08(0.f)
, mGuestE0C(0.f)
, mBoostBallUid(kInvalidUniqueId)                  // 0x000089FC
, mBoostBallRollSfxId(data.mBoostBallRollSound == -1 // 0x00008978
                          ? CSfxManager::kInternalInvalidSfxId
                          : static_cast< ushort >(data.mBoostBallRollSound))
, mBoostBallRollSfx()                                // 0x00008A50
, mBoostBallFlag40(false)
, mBoostHintUid(kInvalidUniqueId)                    // 0x0002011C
, mBoostHintDist(1000.f)                             // 0x0002012C
, mGuestEC8(0)                                       // 0x00020138
, mGuestECC(CVector3f(0.f, 0.f, 0.f))                // 0x00020140
, mBoostHintValue(1.f)                               // 0x00020154
// fn_10_8DB4 at 0x00020060; the two sound ids are kInternalInvalidSfxId for -1 (0x0002000C,
// 0x0002004C).
, mSweepBeamPart(data.mSweepSwoosh)
, mSweepBeamGen()
, mSweepBeamDamageInfo(data.mSweepBeamDamage)
, mSweepBeamDir(CVector3f::Zero())
, mSweepBeamPrevDir(CVector3f::Zero())
, mSweepBeamEnd(CVector3f::Zero())
, mSweepBeamLength(0.f)
, mSweepBeamSpeed(1.f)
, mSweepBeamHitPlayer(false)
, mSweepBeamOn(false)
, mSweepBeamResponse(gpSimplePool->GetObj(SObjectTag('CRSC', data.mSweepBeamCollisionEffect)))
, mSweepBeamSfxId(data.mSweepBeamSound == -1 ? CSfxManager::kInternalInvalidSfxId
                                              : static_cast< ushort >(data.mSweepBeamSound))
, mSweepBeamSfx()
, mSweepBeamEndSfxId(data.mUnknown0EF8DC15 == -1
                         ? CSfxManager::kInternalInvalidSfxId
                         : static_cast< ushort >(data.mUnknown0EF8DC15))
, mSweepBeamEndSfx()
, mDodgeTime(0.f)                               // 0x00020248
, mDodgeDir(CVector3f::Zero())                  // 0x00020254..0x00020264
, mGunLctrXf(CTransform4f::Identity())          // 0x0001FBC0
, mRootPos(CVector3f(0.f, 0.f, 0.f))             // 0x0001FBDC..0x0001FBEC
, mReflectedUids()                               // 0x0001FE9C
, mLastReflectTime(-1000.f)                      // lbl_10_rodata_410, 0x0001FEE0
, mGuestC0C(0.f) // f3 = lbl_10_rodata_3FC = 0.0f (lfs at 0x0001FEB8), stfs at 0x0001FEE4
, mGuestC20(-1000.f)                             // lbl_10_rodata_410, 0x0001FF04
, mRicochetSfxId(data.mMissileRicochetSound == -1 // 0x0000DA94, stored at 0x0001FF48
                     ? CSfxManager::kInternalInvalidSfxId
                     : static_cast< ushort >(data.mMissileRicochetSound))
, mGuestA40(0)
, mGuestA80(0)
, mGuestAB8(0)
, mGuestC30(0.f)
, mGuestEA4(0.f)
, mGuest9FC(0)
, mGuest1048(0.f)
, mGuest104C(0.f)
, mGuest1050(0.f)
, mBss118(160.f)
, mReflectChanceCount(0)                         // 0x00020270
, mReflectChances() {
  // fn_10_215FC (the call at 0x0001FE80) builds the two projectile infos: fn_10_20C2C the first
  // from scatterShotProjectile (0x0002163C), then the second from scatterShotProjectile2
  // (0x000216E4), both with scatterShotDamage, each only for a valid asset and locked after.
  if (data.mScatterShotProjectile != kInvalidAssetId) {
    mScatterShotInfo = CProjectileInfo(data.mScatterShotProjectile, data.mScatterShotDamage);
    mScatterShotInfo.data().Token().Lock();
  }
  if (data.mScatterShotProjectile2 != kInvalidAssetId) {
    mScatterShotInfo2 = CProjectileInfo(data.mScatterShotProjectile2, data.mScatterShotDamage);
    mScatterShotInfo2.data().Token().Lock();
  }
  // The melee record's tokens (fn_10_EA24 at 0x0001FCFC: meleeAttackFX at +0x20 0x0000EAB8,
  // pART at +0x2C), the missile info 0xA0C (fn_10_21A14 at 0x0001FD1C, normalMissileProjectile /
  // normalMissileDamage), the super missile info 0xA4C (fn_10_2199C at 0x0001FDB4) and the freeze
  // beam's token 0xA88 (fn_10_217FC at 0x0001FDC8), each only for a valid asset on the host.
  if (data.mMeleeAttackFX != kInvalidAssetId) {
    mMeleeAttackFXToken = TLockedToken< CGenDescription >(
        gpSimplePool->GetObj(SObjectTag('PART', data.mMeleeAttackFX)));
  }
  if (data.mPart != kInvalidAssetId) {
    mMeleePartToken =
        TLockedToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', data.mPart)));
  }
  if (data.mNormalMissileProjectile != kInvalidAssetId) {
    mMissileInfo = CProjectileInfo(data.mNormalMissileProjectile, data.mNormalMissileDamage);
    mMissileInfo.data().Token().Lock();
  }
  if (data.mSuperMissileProjectile != kInvalidAssetId) {
    mSuperMissileInfo = CProjectileInfo(data.mSuperMissileProjectile, data.mSuperMissileDamage);
    mSuperMissileInfo.data().Token().Lock();
  }
  if (data.mFreezeBeamProjectile != kInvalidAssetId) {
    mFreezeBeamToken = TToken< CWeaponDescription >(
        gpSimplePool->GetObj(SObjectTag('WPSC', data.mFreezeBeamProjectile)));
  }
  // The flash record 0xDF0 (fn_10_EDF4 at 0x000200B4): tXTR's token and the size.
  mFlashSize = data.mUnknown2C6A3344;
  if (data.mTXTR != kInvalidAssetId) {
    mFlashTexture =
        TLockedToken< CTexture >(gpSimplePool->GetObj(SObjectTag('TXTR', data.mTXTR)));
  }
  // The phazon infos 0xF80 (fn_10_20F50 at 0x000201D8) and 0x1064 (fn_10_20BFC at 0x0002022C).
  if (data.mPhazonProjectile != kInvalidAssetId) {
    mPhazonShotInfo = CProjectileInfo(data.mPhazonProjectile, data.mPhazonProjectileDamage);
    mPhazonShotInfo.data().Token().Lock();
  }
  if (data.mPhazonSuperMissileProjectile != kInvalidAssetId) {
    mPhazonSuperMissileInfo =
        CProjectileInfo(data.mPhazonSuperMissileProjectile, data.mPhazonSuperMissileDamage);
    mPhazonSuperMissileInfo.data().Token().Lock();
  }
  // The invulnerable model 0xD64 (0x000202D8..0x00020408): CMDL mInvulnerableModel (0xD5C,
  // 0x000202E4), CSKR mInvulnerableSkinRules (0xD60, 0x00020358) and the character's layout info
  // (0x000203B8), built and locked whatever the ids.
  mInvulnerableSkinnedModel = TLockedToken< CSkinnedModel >(
      rs_new CSkinnedModel(gpSimplePool->GetObj(SObjectTag('CMDL', data.mInvulnerableModel)),
                           gpSimplePool->GetObj(SObjectTag('CSKR', data.mInvulnerableSkinRules)),
                           AnimationData()->GetModelData()->GetLayoutInfo()));
  mAttackWeights.count = 0;
  // The path search's flags are 0x401 (`li r5, 0x401` at 0x0001FB54), its index the patterned
  // info's pathfinding index (`lwz r6, 0xec(r30)` at 0x0001FB40), its radii 1.0f (lbl_10_rodata_3E8,
  // loaded at 0x0001FB38) and its root position kRP_Center (`li r8, 0x0` at 0x0001FB5C); the area
  // is null (`li r4, 0x0` at 0x0001FB50).
  //
  // The retail constructor turns knock-back physics off on CPatterned's own manager (the
  // CAiKnockBackMgr at 0x620, `addi r3, r31, 0x620` at 0x00020244, the false at 0x0002024C, the
  // call at 0x000202B0) and clears five flag bits in the byte at 0x681 (the rlwimi pairs at
  // 0x00020274..0x000202AC, i.e. the bits 25, 27, 28, 29 and 30 of that byte).
  // Everything else it builds - the identity transform at 0x8CC, the 'SCAN' token at 0x92C, the
  // sixteen sub-objects at 0x94C..0x1064 and the CMdl/Cskr token pair at 0xD64 - belongs to the
  // state machine and the render paths this milestone does not enter.
  //
  // The scalars the three TravelToCoverPoints triggers read are set to retail's defaults: the
  // attack words at 0x980 and 0x984 are 0xFFFF (0x0001FCE4/0x0001FCE8), the count at 0x994 is 0
  // (0x0001FCF8), the accumulated damage at 0xCC0 is 0.0f (0x0001FF58), the last health at 0xCC4
  // is -1.0f (0x0001FF5C), the damage threshold at 0xCC8 is the constructor's 8th float argument
  // (0x0001FF60 - the loader's damageInterruptThreshold, the property id 0x8f70d3f2), the damage
  // decay at 0xCCC is 1.0f (0x0001FF64), the still timer at 0x964 is 0.0f (0x0001FCB0) and its
  // reference position at 0x958/0x95C/0x960 is the zero vector (0x0001FC64..0x0001FC74), the wall
  // collision time at 0xC44 is 0.0f (0x0001FF38) and the elapsed time at 0x8B0 is 0.0f
  // (0x0001FB88).
  KnockBackController().EnableKnockBackPhysics(false);
  ConstructPhazonBeam(); // fn_10_45F8 at 0x000201B4
  mHeadSegId = GetModelData()->GetAnimationData()->GetCharLayoutInfo()->GetSegIdFromString(
      rstl::string_l("Head_1")); // lbl_10_rodata_572, 0x0001FFB4..0x0001FFF0
  // 0x0002048C: a valid alternate scannable info is locked into 0x92C (`addis r0, r29, 0x1;
  // cmplwi r0, 0xffff`, i.e. the id is not -1).
  if (mData.mAlternateScannableInfo != kInvalidAssetId) {
    mAltScanInfo = rs_new TLockedToken< CScannableObjectInfo >(
        gpSimplePool->GetObj(SObjectTag('SCAN', mData.mAlternateScannableInfo)));
  }
}

CDarkSamus::~CDarkSamus() {}

// vtable slot 5 = fn_10_1B1FC (0x27C): Think. Until the state machine has a state (the machine at
// 0x350, its vtable 0x28 at 0x0001B234) the first Think only sets the machine up (fn_10_18D78 at
// 0x0001B248, the host's InitializeStateMachine) and returns. Then the waypoint navigation faces the
// player: the flat direction (z = lbl_10_rodata_3FC = 0.0f at 0x0001B294), normalized when it can
// be (0x0001B2A4), goes to the base's 0x6E0 (0x0001B2B4..0x0001B2C4). CPatterned::Think runs
// (0x0001B2D4; it returns at once for an inactive actor, and retail has no active test of its own),
// and the elapsed time 0x8B0 grows by dt after it (0x0001B2DC).
// @ fn_10_1B1FC
void CDarkSamus::Think(float dt, CStateManager& mgr) {
  if (!StateMachineState().HasState()) {
    InitializeStateMachine(mgr);
    return;
  }
  CVector3f toPlayer = (mgr.GetPlayer(0)->GetTranslation() - GetTranslation()).DropZ();
  if (toPlayer.CanBeNormalized()) {
    toPlayer.Normalize();
    mWaypointNavigation.SetFaceVector(toPlayer);
  }
  CPatterned::Think(dt, mgr);
  mElapsedTime += dt;
  // 0x0001B2E4: the module's own AI runs only while the actor is alive (the guest's 0x420 bit 6,
  // the extrwi at 0x0001B2E8 - the host's GetAlive()).
  if (GetAlive()) {
    AdvanceBattleStage(mgr);           // fn_10_193A0 at 0x0001B308
    UpdateAttackPick(mgr);             // fn_10_1954C at 0x0001B314
    UpdateDiveContact(mgr);            // fn_10_1ACA0 at 0x0001B320
    UpdateHeadAim(mgr);                // fn_10_10A64 at 0x0001B330
    WatchStageChange(mgr);             // fn_10_FD4C at 0x0001B33C
    UpdateSilhouetteFade(mgr, dt);     // fn_10_F710 at 0x0001B34C
    UpdateInvulnerableFade(dt);        // fn_10_F654 at 0x0001B358
    UpdateMeleeHit(mgr);               // fn_10_1AE08 at 0x0001B364
    if (mGuestC00 > 0.f) {             // 0x0001B378
      ReflectProjectiles(mgr);         // fn_10_1B658 at 0x0001B384
    }
    UpdateStageFlash(mgr);             // fn_10_1B0E0 at 0x0001B390
    // 0x0001B3A8: in an unsafe spot (fn_10_EFC8) with a safe position kept (0x928 == 1,
    // 0x0001B3B4) the actor goes back there (SetTranslation at 0x0001B3C0), the collision
    // failsafe runs (fn_10_1AEC0 at 0x0001B3CC) and the model is hidden (0xDE8 = 0.0f, 0x0001B3D8).
    if (IsInUnsafeSpot(mgr) && mHasSafePos == 1) {
      SetTranslation(mSafePos);
      RunCollisionFailsafe(mgr);
      mGuestDE8 = 0.f;
    }
  }
  // 0x0001B3DC: alive or not, the visibility fade, the env fx faded out while not fading
  // (0xDEC == 0, 0x0001B3F0) and alive (0x0001B40C), the targetability, the melee pulse and the
  // boost ball's effects with dt, and the health threshold's message.
  UpdateFade(dt);                                  // fn_10_1D6CC at 0x0001B3E4
  if (mFadeOut == 0 && GetAlive()) {
    mgr.EnvFxManager()->FadeDensity(0.f, 4000);    // lbl_10_rodata_3FC, lbl_10_rodata_254 (0x0001B42C)
  }
  UpdateTargetable(mgr);                           // fn_10_1D5C4 at 0x0001B438
  UpdateMeleePulseAndEffects(mgr, dt);             // fn_10_12DD0 at 0x0001B448
  CheckHealthThreshold(mgr);                       // fn_10_1B154 at 0x0001B454
}

// fn_10_1E864 (0x0001E864, 0x38 bytes): the path search's area - the actor's area (the guest's
// this+4) in the world's area list (the state manager's 0x1604, the list at 0x24 and the entry's
// +4 at 0x0001E884), its post-constructed data (+0x104) and that data's path area (+0xEC), stored
// into the search (the guest's 0x7C0, 0x0001E890).
// @ fn_10_1E864
void CDarkSamus::SetPathSearchArea(CStateManager& mgr) {
  mPathFindSearch.SetArea(
      mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea);
}

// vtable slot 6 = fn_10_1E89C (0x5B0): the script-message switch (the compare tree at
// 0x0001E8B8..0x0001E998). The arms that do something: 'XCRT' (0x0001E99C), 'ACTV' (0x0001E9AC),
// 'ALRT' (0x0001EA1C), 'XDMG' (0x0001EA30), 'XDEL' and 'DCTV' (both 0x0001EB38), 'XALD'
// (0x0001EB60), 'XLAU' (0x0001ED58), 'XLND' and 'XLSG' (both 0x0001EDDC); 'XOFF' and every other
// message only reach the common tail, CPatterned::AcceptScriptMsg (0x0001EE2C), which the
// landing arms skip while a dive is not done (0x0001EE1C).
// @ fn_10_1E89C
void CDarkSamus::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case kSM_Create: // 'XCRT', 0x0001E928
    BodyController()->Activate(mgr, pas::kAS_Invalid); // 0x0001E9A0 (li r5, -0x1), 0x0001E9A4
    break;
  case kSM_Activate: { // 'ACTV', 0x0001E914
    // The boss bar's health: the constructor's float 0x908 (the loader's unknown_0x74fa22f0, its
    // only writer `stfs f24, 0x908` at 0x0001FBF0) when it is above lbl_10_rodata_3FC = 0.0f
    // (0x0001E9B8, `ble`), the health info's (vtable 0x3C, 0x0001E9D4) otherwise. The name is
    // "BossDarkSamus" (lbl_10_rodata_560 + 0x55F = .rodata 0xABF, 0x0001E9F8).
    const float hp = mData.mUnknown74FA22F0 > 0.f ? mData.mUnknown74FA22F0 // 0x0001E9B8
                                                   : GetHealthInfo()->GetHP(); // 0x0001E9D8
    mgr.SetBossParams(GetUniqueId(), hp,
                      gpStringTable->GetStringIndex("BossDarkSamus")); // 0x0001EA00, 0x0001EA14
    break;
  }
  case kSM_Alert: // 'ALRT', 0x0001E900
    // The alerted flag: the byte 0x90C's bit 0x80 (`rlwimi r0, r3, 7, 24, 24` at 0x0001EA24).
    mFlags90C |= 0x80;
    break;
  case kSM_Damage: { // 'XDMG', 0x0001E970
    // The sender must be a CGameProjectile (0x0001EA40, the null test 0x0001EA48) whose damage
    // (its 0x19C) hurts the vulnerability (vtable 0x40 at 0x0001EA5C, WeaponHurts at 0x0001EA64,
    // `beq` on false at 0x0001EA6C).
    const CGameProjectile* proj =
        TCastToConstPtr< CGameProjectile >(mgr.GetObjectById(msg.GetSenderId()));
    if (proj == nullptr) { // 0x0001EA48
      break;
    }
    if (!GetDamageVulnerability()->WeaponHurts(proj->GetCurrentDamageInfo().GetWeaponMode())) {
      break; // 0x0001EA6C
    }
    // The hit direction goes to 0x8C0/0x8C4/0x8C8 before the time test (0x0001EA98..0x0001EAA4).
    mHitDirection = proj->GetTranslation() - GetTranslation();
    // Only while lbl_10_rodata_3E8 = 1.0f has passed since 0xC20 (`fadds` 0x0001EAB0, `fcmpo` +
    // `bge` skip at 0x0001EAB4/0x0001EAB8).
    if (1.f + mGuestC20 >= mElapsedTime) {
      break;
    }
    // fn_10_1EE4C (0x0001EAD8) with the projectile's damage (0x19C) and uid; on true (`cmplwi
    // r0, 0x1` at 0x0001EAE0) 0xC0C takes the elapsed time (0x0001EAF8) and 0xC10..0xC18 the hit
    // direction with z = lbl_10_rodata_3FC = 0.0f (0x0001EB04..0x0001EB18), normalized when it
    // can be (0x0001EB24, 0x0001EB30).
    if (CanReflectHit(mgr, proj->GetCurrentDamageInfo(), proj->GetUniqueId()) == true) {
      mGuestC0C = mElapsedTime;
      mHitDirectionFlat = mHitDirection;
      mHitDirectionFlat.SetZ(0.f);
      if (mHitDirectionFlat.CanBeNormalized() == true) {
        mHitDirectionFlat.Normalize();
      }
    }
    break;
  }
  case kSM_Delete:     // 'XDEL', 0x0001E8D8
  case kSM_Deactivate: // 'DCTV', 0x0001E8EC
    DeleteBoostBall(mgr);  // fn_10_9AC4 at 0x0001EB40
    DeleteGuestActor(mgr); // fn_10_4868 at 0x0001EB4C
    DeleteGuestUids(mgr);  // fn_10_69C0 at 0x0001EB58
    break;
  case kSM_AreaLoaded: { // 'XALD', 0x0001E93C
    // The object connected with 'IS05' (kSS_InternalState5, 0x0001EB60/0x0001EB70) for any
    // message (-1, 0x0001EB74): a valid uid (0x0001EB8C) that is a CScriptDarkSamusBattleStage
    // (0x0001EBA4, the null test 0x0001EBA8) goes to 0x974 and 0xD50 and the stage count 0x978
    // becomes 1 (0x0001EBB8..0x0001EBC4).
    const TUniqueId stageUid = FindConnectedObject(mgr, kSS_InternalState5, kSM_None);
    if (stageUid != kInvalidUniqueId) {
      if (const CDarkSamusBattleStage* stage =
              TCastToConstPtr< CDarkSamusBattleStage >(mgr.GetObjectById(stageUid))) {
        mBattleStageUid = stage->GetUniqueId();
        mSeenStageUid = mBattleStageUid;
        mStageCount = 1;
      }
    }
    SetPathSearchArea(mgr); // fn_10_1E864 at 0x0001EBD0
    // The dive record's collidable 0xB28 takes the animation's bounds (0x0001EBE0) and the
    // actor's material list (this+0x68, 0x0001EBEC) (the stores at 0x0001EC38..0x0001EC68).
    mDive.mCollidable = CCollidableAABox(GetAnimationData()->GetBoundingBox(), GetMaterialList());
    // Active (the byte 0x20's bit 0x80, `extrwi r3, r0, 1, 24` at 0x0001EC74, `cmplwi 0x1` at
    // 0x0001EC84): the boss bar with the health info's health (vtable 0x3C, 0x0001EC9C) and
    // "BossDarkSamus" (0x0001ECC4).
    if (GetActive() == true) {
      mgr.SetBossParams(GetUniqueId(), GetHealthInfo()->GetHP(),
                        gpStringTable->GetStringIndex("BossDarkSamus")); // 0x0001ECDC
    }
    // The material filter (0x0001ED50): include 1 << lbl_10_data_C14 = 0x3B (kMT_Solid,
    // 0x0001ED2C), exclude 1 << lbl_10_data_C18 = 0x2F (kMT_CollisionActor, 0x0001ECF4) |
    // 1 << lbl_10_data_C1C = 0x1C (kMT_AIPassthrough, 0x0001ED10), type 3 (`li r0, 0x3` at
    // 0x0001ED30, kFT_IncludeExclude).
    SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
        CMaterialList(kMT_Solid), CMaterialList(kMT_CollisionActor, kMT_AIPassthrough)));
    break;
  }
  case kSM_Launching: // 'XLAU', 0x0001E960
    // The base first takes a 'XOFF' (kSM_OffGround, 0x0001ED6C/0x0001ED70) from the message's
    // sender to this actor, originator kInvalidUniqueId, state -1 (built at 0x0001ED74..
    // 0x0001ED9C, the call at 0x0001EDA0). Then the dive is in flight (0xB20 bit 0x40,
    // `rlwimi r0, r3, 6, 25, 25` at 0x0001EDAC) from the actor's position (0xACC..0xAD4,
    // 0x0001EDC0..0x0001EDD0) and the bubble comes on (fn_10_12EF8 at 0x0001EDD4).
    CPatterned::AcceptScriptMsg(mgr, CScriptMsg(msg.GetSenderId(), GetUniqueId(), kSM_OffGround,
                                                kInvalidUniqueId,
                                                static_cast< EScriptObjectState >(-1)));
    mDive.mLaunched = true;
    mDive.mStart = GetTranslation();
    StartDiveBubble(mgr);
    break;
  case kSM_Landed:               // 'XLND', 0x0001E950
  case kSM_LandedOnStaticGround: // 'XLSG', 0x0001E994
    // Only in the dive (0x980 == 0x20, 0x0001EDE0/0x0001EDE4): after a bounce (0xB20 bit 0x02,
    // `extrwi r0, r4, 1, 30` at 0x0001EDEC, `cmplwi r0, 0x1` at 0x0001EDF0) the dive is done
    // (bit 0x80, `rlwimi r4, r0, 7, 24, 24` at 0x0001EE00) and the vertical movement off (0x34C
    // bit 0x40 cleared, `rlwimi r0, r3, 6, 25, 25` at 0x0001EE0C); the base is skipped while the
    // dive is not done (`extrwi. r0, r0, 1, 24` + `beq` at 0x0001EE18/0x0001EE1C).
    if (mCurrentAttack == 0x20) {
      if (mDive.mBounced == true) {
        mDive.mDone = true;
        mVerticalMovement = false;
      }
      if (!mDive.mDone) {
        return;
      }
    }
    break;
  default:
    break;
  }
  CPatterned::AcceptScriptMsg(mgr, msg); // 0x0001EE2C
}

// The state machine's setup (vtable slot 60 stays CPatterned::SetupStateMachine, so this is the
// module's own hook). Retail's fn_10_18D78 - called from the first Think (fn_10_1B1FC, when the
// machine has no state yet) - calls CPatterned's InitializeStateMachine and then hands the machine
// its own two tables: lbl_10_data_450 with the 49 triggers and lbl_10_data_964 with the 43 states.
// The host's machine asks for the tables through this virtual (CPatterned::InitializeStateMachine
// calls it), so the module's tables go in here, after the base's own.
// @ fn_10_18D78
void CDarkSamus::SetupStateMachine(CStateManager& mgr) {
  CPatterned::SetupStateMachine(mgr);
  InstallStateMachineFunctions();
}

// vtable slot 9 = fn_10_18AB8 (0x2C0): the pre-render pass. With 0xD14 above lbl_10_rodata_3FC =
// 0.0f (0x00018AF0) the flag byte 0x90C's bit 0x20 is set
// (`rlwimi r0, r3, 5, 26, 26` at 0x00018AFC); while it is set (`extrwi. r0, r0, 1, 26` at 0x00018B08) the renderer allocates the
// phazon suit mask (0x00018B1C). The base's pass (0x00018B28); the colour's alpha byte 0x44F is
// lbl_10_rodata_52C = 255.0f times 0.0f with a boost ball (fn_10_9624 at 0x00018B30) or 0xDE8
// (0x00018B4C) (the u8 store at 0x00018B5C). With the last echo visor (0xDED bit 0x40,
// `extrwi r0, r0, 1, 25` at 0x00018B6C) and the damage timer 0x448 above 0.0f (0x00018B88) the
// draw flags 0xFC are (2, 0, 3, Lerp(Black, White, min(0x448 / skDamageHitTime, 1.0f)))
// (`bge` at 0x00018BA4, 0x00018BE4..0x00018BF4). Then the gun locator's transform 0x8CC
// (0x00018C18) and the root bone's position 0x8FC (0x00018C50..0x00018C6C), fn_10_10864
// (0x00018C7C), the simple shadow 0xC0 from the model's bounds with 0xDE8 above 0.0f
// (0x00018C90, 0x00018CB4), the visor bits 0xDED 0x80 (dark, cntlzw(3 - visor) by
// `rlwimi r3, r5, 2, 24, 24` at 0x00018CD8) and 0x40 (echo, cntlzw(1 - visor) >> 5 by
// `rlwimi r0, r4, 6, 25, 25` at 0x00018CF0), fn_10_12DD0 (0x00018D04), and with 0xFC0 above 0.0f
// (0x00018D18) the colour's red, green and blue bytes 0x44C..0x44E from Lerp(Black, White, 0xFC0)
// (0x00018D38..0x00018D50).
// Run (darksamus_uevent.py, dsfsm2-runs/s11/u8): every retail arm (key 78 a boost ball); the one
// untaken branch is CStateManager::GetPlayerState's host-only fallback (TARGET_PC).
// @ fn_10_18AB8
void CDarkSamus::PreRender(CStateManager& mgr) {
  if (mGuestD14 > 0.f) { // 0x00018AF0
    mFlags90C |= 0x20;   // 0x00018AFC
  }
  if ((mFlags90C & 0x20) != 0) { // 0x00018B08
    gpRender->AllocatePhazonSuitMaskTexture();
  }
  CPatterned::PreRender(mgr);
  const float visible = HasBoostBall() == true ? 0.f : mGuestDE8; // 0x00018B38
  mColor.SetAlpha(static_cast< uchar >(255.f * visible));       // lbl_10_rodata_52C, 0x00018B58
  if (mEchoVisorDED40 == true && mDamageCooldownTimer > 0.f) {  // 0x00018B70, 0x00018B88
    float t = mDamageCooldownTimer / skDamageHitTime;           // 0x00018B9C
    if (t >= 1.f) {                                             // 0x00018BA4
      t = 1.f;
    }
    SetModelFlags(CModelFlags(CModelFlags::kT_Two, 0, CModelFlags::EFlags(3),
                              CColor::Lerp(CColor::Black(), CColor::White(), t)));
  }
  mGunLctrXf = GetLctrTransform(rstl::string_l("GUN_Particle_LCTR"));
  mRootPos = GetLctrTransform(rstl::string_l("Skeleton_Root")).GetTranslation();
  TwistHead(); // fn_10_10864 at 0x00018C7C
  if (mGuestDE8 > 0.f) { // 0x00018C90
    Shadow()->Calculate(GetModelData()->GetBounds(), GetTransform(), mgr);
  }
  const CPlayerState::EPlayerVisor visor = mgr.GetPlayerState()->GetActiveVisor(mgr);
  mDarkVisorDED80 = visor == CPlayerState::kPV_Dark; // 0x00018CC4..0x00018CD8
  mEchoVisorDED40 = visor == CPlayerState::kPV_Echo; // 0x00018CD0..0x00018CF0
  UpdateMeleePulseAndEffects(mgr, 0.f); // fn_10_12DD0 at 0x00018D04, dt lbl_10_rodata_3FC (0x00018CF4)
  if (mGuestFC0 > 0.f) { // 0x00018D18
    const CColor lerped = CColor::Lerp(CColor::Black(), CColor::White(), mGuestFC0);
    mColor.SetRed(lerped.GetRedu8());     // 0x00018D48
    mColor.SetGreen(lerped.GetGreenu8()); // 0x00018D4C
    mColor.SetBlue(lerped.GetBlueu8());   // 0x00018D50
  }
}

// vtable slot 10 = fn_10_17F04 (0x2D8): the base's pass, then - alive only (0x420 bit 0x40,
// 0x00017F2C) - the effects. With the stage's fourth bool (+0x33, 0x00017F64) set and the dark
// visor (3) active none of them (0x00017F74). Else the sweep beam 0xD7C, the boost ball swoosh
// 0xE3C and, without 0xEBC bit 0x40, the swoosh 0xE4C (gpRender vtable 0x44); the phazon beam
// 0xEF8 when there is one (fn_10_4AA8, 0x00018000); the enrage sphere 0x1028 when its bounds
// (vtable 0x70) are set, sorted at lbl_10_rodata_3E4 = 0.5 * (the player's eye + the actor's
// position) with the box of lbl_10_rodata_41C = 0.1 around the root 0x8FC (vtable 0x48,
// 0x00018184). Last, with a fade factor (fn_10_185CC) over lbl_10_rodata_3FC = 0 and the dark
// visor, EnsureRendered (0x000181BC).
// Run (darksamus_render.py): every arm by a keyed force (not alive, the stage's bool with either
// visor, no stage, each effect lent the beam, 0x1028's empty and set bounds, the fade window); the
// beam is drawn (screenshot of the link build). Not a retail arm: the host GetPlayerState()'s
// fallback to mPlayerStates[0] (mPlayerState is always set; CPatterned::AddToRenderer reads it
// unchecked just before).
// @ fn_10_17F04
void CDarkSamus::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
  if (!GetAlive()) {
    return;
  }
  const CPlayerState::EPlayerVisor visor = mgr.GetPlayerState()->GetActiveVisor(mgr);
  const CDarkSamusBattleStage* stage = GetBattleStage(const_cast< CStateManager& >(mgr));
  if (stage == nullptr || !stage->GetUnknown53FDCB5B() || visor != CPlayerState::kPV_Dark) {
    if (mSweepBeamGen.get() != nullptr) {
      gpRender->AddParticleGen(*mSweepBeamGen);
    }
    if (mBoostBallSwooshE3C != nullptr) {
      gpRender->AddParticleGen(*mBoostBallSwooshE3C);
      if (!mBoostBallFlag40 && mBoostBallSwooshE4C != nullptr) {
        gpRender->AddParticleGen(*mBoostBallSwooshE4C);
      }
    }
    if (HasPhazonBeam()) {
      gpRender->AddParticleGen(*mGuestEF8);
    }
    if (mGuest1028.get() != nullptr) {
      const rstl::optional_object< CAABox > bounds = mGuest1028->GetBounds();
      if (bounds) {
        const CVector3f pos = 0.5f * (mgr.GetPlayer(0)->GetEyePosition() + GetTranslation());
        const CVector3f half = 0.1f * CVector3f::One();
        gpRender->AddParticleGen(*mGuest1028, pos, CAABox(mRootPos - half, mRootPos + half));
      }
    }
  }
  if (GetFadeFactor() > 0.f &&
      mgr.GetPlayerState()->GetActiveVisor(mgr) == CPlayerState::kPV_Dark) {
    EnsureRendered(mgr);
  }
}

// fn_10_4AA8 (0x00004AA8, 0x14 bytes): the beam 0xEF8 is not null (`neg; or; srwi 31`).
// @ fn_10_4AA8
bool CDarkSamus::HasPhazonBeam() const { return mGuestEF8.get() != nullptr; }

// fn_10_185CC (0x000185CC, 0x5C bytes): fn_10_18628's window (0xE04 the start, 0xE08 the hold,
// 0xE0C the fade): 0 (lbl_10_rodata_3FC) once the elapsed time 0x8B0 is past all three
// (0x000185E8), 1 - the time past the hold / the fade while fading (0x00018614), else 1
// (lbl_10_rodata_3E8). Run (darksamus_render.py): the window forced holding and fading; 0 natural.
// @ fn_10_185CC
float CDarkSamus::GetFadeFactor() const {
  if (mElapsedTime > mGuestE04 + (mGuestE08 + mGuestE0C)) {
    return 0.f;
  }
  const float hold = mGuestE04 + mGuestE08;
  if (mElapsedTime > hold) {
    return 1.f - (mElapsedTime - hold) / mGuestE0C;
  }
  return 1.f;
}

// vtable slot 11 = fn_10_18964 (0x154): with a stage (fn_10_17E14, 0x0001899C) whose +0x33 is
// set (0x000189B4) and the last visor the dark one (0xDED bit 0x80,
// `extrwi r0, r0, 1, 24` at 0x000189C0) only the flash below. Otherwise, without a boost ball (fn_10_9624, 0x000189D8): when
// 0xDE8 is above 0.0f (0x000189EC) or the visor is the dark (3, 0x000189F0) or echo one (1,
// 0x000189F8), the silhouette pass fn_10_18238 if 0xD14 is above 0.0f (0x00018A10), the visor is
// the combat one (0x00018A14) and 0x90C bit 0x20 is set (0x00018A20), else the base's Render
// (0x00018A40); then the invulnerable model fn_10_183E8 (0x00018A48). The boost ball's element
// generator 0xE64 renders when there is one (vtable 0x10, 0x00018A64). Last, the flash
// (fn_10_1890C, 0x00018A98) while the fade factor (fn_10_185CC) is above 0.0f (0x00018A7C) and
// the visor is the dark one (0x00018A8C).
// Run (darksamus_uevent.py, dsfsm2-runs/s10 u4+u9+u10): every retail arm (key 63 the stage's
// +0x33 under the dark and the combat visor, 56 no boost ball, 50 0xD14 with the combat and dark
// visors, 54 0xDE8 = 0 under dark/echo). Not run: the host GetActiveVisor inline's null-camera
// branch; 0xD14 > 0 with 0x90C bit 0x20 clear (PreRender sets the bit first, 0x00018B08 path);
// the 0xE64 generator (always null on the host, its spawn is unported).
// @ fn_10_18964
void CDarkSamus::Render(const CStateManager& mgr) const {
  const CPlayerState::EPlayerVisor visor = mgr.GetPlayerState()->GetActiveVisor(mgr);
  CStateManager& smgr = const_cast< CStateManager& >(mgr);
  if (GetBattleStage(smgr) == nullptr || GetBattleStage(smgr)->GetUnknown53FDCB5B() != true ||
      mDarkVisorDED80 != true) {
    if (!HasBoostBall()) {
      if (mGuestDE8 > 0.f || visor == CPlayerState::kPV_Dark || visor == CPlayerState::kPV_Echo) {
        if (mGuestD14 > 0.f && visor == CPlayerState::kPV_Combat && (mFlags90C & 0x20) != 0) {
          RenderSilhouetteModel(mgr);
        } else {
          CPatterned::Render(mgr);
        }
      }
      RenderInvulnerableModel();
    }
    if (mBoostBallElementGen.get() != nullptr) {
      mBoostBallElementGen->Render();
    }
  }
  if (GetFadeFactor() > 0.f &&
      mgr.GetPlayerState()->GetActiveVisor(mgr) == CPlayerState::kPV_Dark) {
    RenderFlash();
  }
}

// fn_10_181E0 (0x000181E0, 0x58 bytes): the silhouette colour - the lesser of 0xDE8 and 0xD14
// (`bge` at 0x000181F8 keeps 0xD14 when 0xDE8 is not below it) times 0xD18's x, y and z
// (0x00018214..0x00018220), alpha lbl_10_rodata_3E4 = 0.5f.
// Run (darksamus_uevent.py, dsfsm2-runs/s10 u4+u9+u10): both sides of the min (keys 50, 54).
// @ fn_10_181E0
CColor CDarkSamus::GetSilhouetteColor() const {
  float scale = mGuestDE8;
  if (scale >= mGuestD14) { // 0x000181F8
    scale = mGuestD14;
  }
  return CColor(scale * mGuestD18.GetX(), scale * mGuestD18.GetY(), scale * mGuestD18.GetZ(),
                0.5f);
}

// fn_10_18238 (0x00018238, 0x1B0 bytes): the phazon silhouette. The screen goes to the spare
// buffer (CopyScreenTex(3, true, mpSpareBuffer, 4 = GX_TF_RGB565, false), 0x00018280), the
// destination alpha is on at 0xFF (0x0001828C), the model matrix is the transform times the
// model's scale (0x000182C8..0x00018304), the animated model (kWM_Normal, 0x00018338) is drawn
// with the flags (1, 0, 3, Black) (0x00018318..0x0001832C, 0x00018348), the renderer's
// silhouette at blur lbl_10_rodata_3E4 = 0.5f with fn_10_181E0's colour, no texture and
// (0.0f, 0.0f, 0.0f, White) (0x00018394), then the base's Render (0x000183C4).
// Run (darksamus_uevent.py, dsfsm2-runs/s10 u4+u9+u10): every line; the untaken
// branches are the optional_object temporary's exception edges (no retail arm).
// @ fn_10_18238
void CDarkSamus::RenderSilhouetteModel(const CStateManager& mgr) const {
  CCubeRenderer* render = gpRender;
  render->CopyScreenTex(3, true, CGraphics::GetDolphinSpareBuffer(), GX_TF_RGB565, false);
  CGX::SetDstAlpha(true, 0xFF);
  CTransform4f xf = GetTransform();
  const CVector3f& scale = GetModelData()->GetScale();
  xf = xf * CTransform4f::Scale(scale.GetX(), scale.GetY(), scale.GetZ());
  gpRender->SetModelMatrix(xf);
  GetModelData()->GetAnimationData()->Render(
      GetModelData()->PickAnimatedModel(CModelData::kWM_Normal),
      CModelFlags(CModelFlags::kT_One, 0, CModelFlags::EFlags(3), CColor::Black()));
  render->RenderSilhouette(0.5f, GetSilhouetteColor(),
                           rstl::optional_object< TCachedToken< CTexture > >(), 0.f, 0.f, 0.f,
                           CColor::White());
  CPatterned::Render(mgr);
}

// fn_10_183E8 (0x000183E8, 0x1E4 bytes): the invulnerable model 0xD6C, not with the last visor the
// echo one (0xDED bit 0x40, `extrwi r0, r0, 1, 25` at 0x00018404). The model matrix is the
// transform times the model's scale (0x00018434..0x00018464). The alpha: with 0xD24 above 0.0f
// (0x00018478), the wave (1 + sin(lbl_10_rodata_518 = 16.0f * 0x8B0)) * 0.5f (0x0001848C..
// 0x000184B0) and below lbl_10_rodata_3E8 = 1.0f (0x000184B4) lbl_10_rodata_51C = 0.4f * 0xD24 +
// lbl_10_rodata_428 = 0.2f (0x000184C8), else 1.0f in the phazon phase (0x1060 bit 0x80 at
// 0x000184D4) or during the attacks 0x400000 / 0x800000 (`subis` at 0x000184E4 / 0x000184F0),
// else lbl_10_rodata_524 = 0.2f * the wave + lbl_10_rodata_520 = 0.4f (0x00018518); at 0.0f or
// less the wave of lbl_10_rodata_528 = 14.0f (0x00018530) and lbl_10_rodata_41C = 0.1f * (1 +
// wave) * 0.5f + 0.1f (0x00018558). The alpha is held to 0xDE8 (0x00018564) and the model drawn
// with the flags (5, 0, 3, (1, 1, 1, alpha)) (0x0001858C..0x000185A0, 0x000185B0).
// Run (darksamus_uevent.py, dsfsm2-runs/s10 u4+u9+u10): every arm (keys 51/52 0xD24 at
// 0.5 / 1, 53 the phazon phase, the attacks by the dt key, `visor echo`).
// @ fn_10_183E8
void CDarkSamus::RenderInvulnerableModel() const {
  if (mEchoVisorDED40 == true) { // 0x0001840C
    return;
  }
  gpRender->SetModelMatrix(GetTransform() * CTransform4f::Scale(GetModelData()->GetScale()));
  float alpha;
  if (mGuestD24 > 0.f) { // 0x00018478
    const float wave = (1.f + static_cast< float >(sin(16.f * mElapsedTime))) * 0.5f;
    if (mGuestD24 < 1.f) { // 0x000184B4
      alpha = 0.40000004f * mGuestD24 + 0.2f;
    } else if (mPhazonPhase == true || mCurrentAttack == 0x400000 ||
               mCurrentAttack == 0x800000) { // 0x000184DC, 0x000184EC, 0x000184F8
      alpha = 1.f;
    } else {
      alpha = 0.20000002f * wave + 0.4f;
    }
  } else {
    const float wave = (1.f + static_cast< float >(sin(14.f * mElapsedTime))) * 0.5f;
    alpha = 0.1f * wave + 0.1f;
  }
  if (alpha > mGuestDE8) { // 0x00018564
    alpha = mGuestDE8;
  }
  GetModelData()->GetAnimationData()->Render(
      **(*mInvulnerableSkinnedModel),
      CModelFlags(CModelFlags::kT_Blend, 0, CModelFlags::EFlags(3), CColor(1.f, 1.f, 1.f, alpha)));
}

// vtable slot 15 = fn_10_0 (0x2C): it calls the vtable's HealthInfo slot (0x38) and returns its
// result, which is what the base class's own accessor does.
// @ fn_10_0
// Run 2026-10-21 (Sanctuary 10_cliff_rooftop, boss u04f7): every health query in the fight goes
// through here, so the body ran. See docs/module-analysis/DarkSamus_helpers.md.
const CHealthInfo* CDarkSamus::GetHealthInfo() const { return CPatterned::GetHealthInfo(); }

// vtable slot 16 = fn_10_100DC (0x134): the vulnerability others read. During the phazon enrage
// (attack 0x800000) every weapon but phazon does nothing: the patterned vulnerability is copied
// to 0xFD0 (fn_10_FFB4, its operator=, 0x00010128) and every normal type but kWT_Phazon, every
// charged and every combo slot is set to (lbl_10_rodata_3FC = 0.0f, kE_Normal, false).
const CDamageVulnerability* CDarkSamus::GetDamageVulnerability() const {
  if (mGuestCD0 == 1) { // 0x000100FC
    return &mGuestCD4;
  }
  if (mCurrentAttack == 0x800000) { // 0x00010110 (subis/cmplwi)
    mEnrageVulnerability = *CPatterned::GetDamageVulnerability(); // 0x00010128
    for (int i = 0; i < 0x15; ++i) {                                // 0x00010168
      if (i != kWT_Phazon) {                                        // 0x00010138 (10)
        mEnrageVulnerability.SetVulnerability(
            i, CWeaponTypeVulnerability(0.f, CWeaponTypeVulnerability::kE_Normal, false));
      }
    }
    for (int i = 0; i < 4; ++i) { // 0x000101A4
      mEnrageVulnerability.SetChargedVulnerability(
          i, CWeaponTypeVulnerability(0.f, CWeaponTypeVulnerability::kE_Normal, false));
    }
    for (int i = 0; i < 4; ++i) { // 0x000101E0
      mEnrageVulnerability.SetComboVulnerability(
          i, CWeaponTypeVulnerability(0.f, CWeaponTypeVulnerability::kE_Normal, false));
    }
    return &mEnrageVulnerability;
  }
  return CPatterned::GetDamageVulnerability(); // 0x000101F0
}

// vtable slot 18 = fn_10_F5C0 (0x2C) only calls fn_10_F5EC (0x0000F5D4), which fills the
// optional with CPhysicsActor::GetBoundingBox (0x0000F604) and sets its valid byte
// (`li r0, 0x1` at 0x0000F608, `stb r0, 0x18(r31)` at 0x0000F60C): the touch bounds are always
// the bounding box.
// @ fn_10_F5C0
// @ fn_10_F5EC
rstl::optional_object< CAABox > CDarkSamus::GetTouchBounds() const {
  return GetBoundingBox();
}

// vtable slot 20 = fn_10_10358 (0x12C). With a boost ball (fn_10_9624, `cmplwi r0, 0x1` at
// 0x00010388) whose actor exists (fn_10_996C, the null test 0x000103A0) the ball's own orbit
// position (vtable 0x50, 0x000103C8). Otherwise CPatterned's (0x000103DC) with the phazon bob
// (fn_10_10484 at 0x00010400); when the attack's orbit position 0xD28 is not the zero vector (the
// three `fcmpu` at 0x00010414..0x00010438) its z (0xD30) replaces the result's (0x00010454..).
// @ fn_10_10358
CVector3f CDarkSamus::GetOrbitPosition(const CStateManager& mgr) const {
  if (HasBoostBall() == true) {
    if (GetBoostBallActor(const_cast< CStateManager& >(mgr)) != nullptr) {
      return GetBoostBallActor(const_cast< CStateManager& >(mgr))->GetOrbitPosition(mgr);
    }
  }
  CVector3f pos = CPatterned::GetOrbitPosition(mgr);
  AddPhazonBob(pos);
  if (!(mAttackOrbitPos.GetX() == CVector3f::Zero().GetX() &&
        mAttackOrbitPos.GetY() == CVector3f::Zero().GetY() &&
        mAttackOrbitPos.GetZ() == CVector3f::Zero().GetZ())) {
    pos.SetZ(mAttackOrbitPos.GetZ());
  }
  return pos;
}

// vtable slot 21 = fn_10_10210 (0x148): GetOrbitPosition's shape with the ball's aim position
// (vtable 0x54, 0x00010290), CPatterned::GetAimPosition (0x000102A8), the phazon bob (fn_10_10484
// at 0x000102CC) and the attack's aim position 0xD34 (the `fcmpu` at 0x000102E0..0x00010300; its
// z 0xD3C at 0x00010314).
// @ fn_10_10210
CVector3f CDarkSamus::GetAimPosition(const CStateManager& mgr, float dt) const {
  if (HasBoostBall() == true) {
    if (GetBoostBallActor(const_cast< CStateManager& >(mgr)) != nullptr) {
      return GetBoostBallActor(const_cast< CStateManager& >(mgr))->GetAimPosition(mgr, dt);
    }
  }
  CVector3f pos = CPatterned::GetAimPosition(mgr, dt);
  AddPhazonBob(pos);
  if (!(mAttackAimPos.GetX() == CVector3f::Zero().GetX() &&
        mAttackAimPos.GetY() == CVector3f::Zero().GetY() &&
        mAttackAimPos.GetZ() == CVector3f::Zero().GetZ())) {
    pos.SetZ(mAttackAimPos.GetZ());
  }
  return pos;
}

// fn_10_10484 (0x00010484, 0x9C bytes): while the phazon volley's time 0xFCC is above
// lbl_10_rodata_3FC = 0.0f (0x000104A4) and lbl_10_rodata_440 = 5.3f has not passed since it
// (0x000104BC), z grows by lbl_10_rodata_448 = 12.0f * sin(lbl_10_rodata_444 = pi * (1 - (1 - t)^2))
// with t = (elapsed - 0xFCC) / 5.3 (0x000104C4..0x00010508; 1.0f is lbl_10_rodata_3E8).
// @ fn_10_10484
void CDarkSamus::AddPhazonBob(CVector3f& pos) const {
  if (mGuestFCC > 0.f && 5.3f + mGuestFCC > mElapsedTime) { // 0x000104A8, 0x000104C0
    const float u = 1.f - (mElapsedTime - mGuestFCC) / 5.3f;   // 0x000104C4..0x000104E0
    const float s = static_cast< float >(sin(3.1415927f * (1.f - u * u))); // 0x000104E4..0x000104F4
    pos.SetZ(pos.GetZ() + 12.f * s);                           // 0x00010500..0x00010508
  }
}

// vtable slot 24 = fn_10_F84C (0x18): the byte at guest this+0xCD0 picks the response - 0x6A when
// it is 1, 0x2E otherwise (`cmplwi r0, 0x1` at 0x0000F854, the two `li` at 0x0000F850/0x0000F85C).
EWeaponCollisionResponseTypes CDarkSamus::GetCollisionResponseType(const CVector3f& position,
                                                                   const CVector3f& direction,
                                                                   const CWeaponMode& mode,
                                                                   int attributes) const {
  if (mGuestCD0 == 1) {
    return static_cast< EWeaponCollisionResponseTypes >(0x6A);
  }
  return static_cast< EWeaponCollisionResponseTypes >(0x2E);
}

// fn_10_1BDF0 (0x0001BDF0, 0x14 bytes): the melee reach, the minimum attack range 0x3DC plus
// lbl_10_rodata_544 = 2.5f.
// Run (darksamus_uevent.py): the DamageOn event (`uevent 8`) with the melee attacks.
// @ fn_10_1BDF0
float CDarkSamus::GetMeleeReach() const { return 2.5f + mMinAttackRange; }

// fn_10_1BE04 (0x0001BE04, 0x7C bytes): the environment effects' density to lbl_10_rodata_3FC =
// 0.0f over lbl_10_rodata_254 = 4000 (FadeDensity at 0x0001BE38, the manager at 0x1630), then
// the Exited links with no message (kSS_Exited = 'EXIT', r7 = -1, 0x0001BE64).
// Run (darksamus_uevent.py): the FadeIn event (`uevent 14`) with 0x40000 and 0x400000.
// @ fn_10_1BE04
void CDarkSamus::FadeEnvFxOut(CStateManager& mgr) {
  mgr.EnvFxManager()->FadeDensity(0.f, 4000);
  SendScriptMsgs(kSS_Exited, mgr, kInvalidUniqueId, kSM_None);
}

// fn_10_1BE80 (0x0001BE80, 0x7C bytes): the density back to lbl_10_rodata_3E8 = 1.0f over
// lbl_10_rodata_250 = 4000, then the Entered links (kSS_Entered = 'ENTR', 0x0001BEE0).
// Run (darksamus_uevent.py): the FadeOut event (`uevent 15`) with 0x80000 and 0x400000.
// @ fn_10_1BE80
void CDarkSamus::FadeEnvFxIn(CStateManager& mgr) {
  mgr.EnvFxManager()->FadeDensity(1.f, 4000);
  SendScriptMsgs(kSS_Entered, mgr, kInvalidUniqueId, kSM_None);
}

// vtable slot 28 = fn_10_1BEFC (0x7CC): the user-anim-event switch (the attacks' timings) on the
// event type (`cmplwi r31, 0x2e` at 0x0001BF28, jumptable_10_data_FB4). The cases that switch on
// the current attack 0x980 (0, 8, 9, 12, 17, 18, 27, 28, 33, 34) return without the base
// (0x0001C6A0); the rest and every other type end in CPatterned::DoUserAnimEvent (0x0001C69C).
// Run (darksamus_uevent.py, `uevent` with the attack forced by the dt key; ALL-ARMS on
// /tmp/opencode/dsfsm2/s9/u3): every type the switch names and every arm above; the deflect's
// slow-down forced (keys 42/43: a `shoot` power shot read as kWT_Missile with no stage, at 30 and
// 20 units), Landing's dive bounce forced (key 40) and type 46 with a dash step left (key 41).
// @ fn_10_1BEFC
void CDarkSamus::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                                 float dt) {
  const int attack = mCurrentAttack;
  switch (type) {
  case kUE_Projectile: // 0x0001C134
    switch (attack) {
    case 1:
    case 2:
    case 4:
      SpawnMeleePulse(mgr); // fn_10_13AB4 at 0x0001C1B8
      break;
    case 0x40:
    case 0x80:
      FireMissile(mgr); // fn_10_1353C at 0x0001C1C8
      break;
    case 0x100:
      FireSuperMissile(mgr); // fn_10_133D0 at 0x0001C1D0
      break;
    case 0x20000:
    case 0x100000:
      FireFreezeBeam(mgr); // fn_10_9164 at 0x0001C1E0
      break;
    case 0x800000:
      FirePhazonVolley(mgr); // fn_10_5840 at 0x0001C1E8
      break;
    default:
      break;
    }
    return;
  case kUE_DamageOn: // 0x0001BFAC: the melee attacks 1, 2 and 4 hit within reach (strict)
    if ((attack == 1 || attack == 2 || attack == 4) && GetDistanceToPlayer(mgr) < GetMeleeReach()) {
      ApplyMeleeDamage(mgr); // fn_10_13998 at 0x0001BFF8
    }
    return;
  case kUE_DamageOff: // 0x0001C000
    return;
  case kUE_BeginAction: // 0x0001C004
    if (attack == 0x10000) {
      StartSweepBeam(); // fn_10_C85C at 0x0001C098
    } else if (attack == 2 && mGuestC04 == 0.f && // 0x0001C030
               AnyIncomingMissiles(mgr, CTriggerData(0)) == true && // vtable 0x14C, 0x0001C050
               GetIncomingProjectileDistance(mgr) > 28.f) { // lbl_10_rodata_510, 0x0001C074
      // The deflect's slow-down: its start time and the body's time scale lbl_10_rodata_4D8 =
      // 0.01f (0x0001C090).
      mGuestC04 = mElapsedTime;
      BodyController()->SetTimeScale(0.01f);
    }
    return;
  case kUE_EndAction: // 0x0001C0A0: the shot counters
    switch (attack) {
    case 0x10000:
      StopSweepBeam(); // fn_10_C798 at 0x0001C11C
      break;
    case 0x40:
      ++mGuestA38;
      break;
    case 0x100:
      ++mGuestA78;
      break;
    case 0x100000:
      ++mGuestAB0;
      break;
    case 0x400000:
      ++mGuestF48;
      break;
    default:
      break;
    }
    return;
  case kUE_EventStart: // 0x0001C1F0
    switch (attack) {
    case 8:
      StartScatterShotFire(mgr, 0); // fn_10_11814 at 0x0001C25C
      break;
    case 0x10:
      StartScatterShotFire(mgr, mScatterSide); // 0x0001C268
      break;
    case 0x100:
      mGuestA84 = 1; // 0x0001C23C
      break;
    case 0x100000:
      mGuestAB8 |= 0x40; // 0x0001C24C
      break;
    case 0x400000:
      CreatePhazonBeam();   // fn_10_6118 at 0x0001C270
      StartPhazonBeamSfx(); // fn_10_8178 at 0x0001C278
      break;
    default:
      break;
    }
    return;
  case kUE_EventStop: // 0x0001C280
    mGuestA84 = 0;
    mGuestAB8 &= ~0x40;
    mScatterFiring = 0;
    mScatterStartTime = 0.f;
    ReleasePhazonBeam();   // fn_10_58C0 at 0x0001C2A4
    DeleteGuestActor(mgr); // fn_10_4868 at 0x0001C2B0
    StopGuestSfx();        // fn_10_812C at 0x0001C2B8
    return;
  case kUE_Landing: // 0x0001C2C0: the missiles' and the freeze beam's records, the dive's landing
    switch (attack) {
    case 0x40:
      mGuestA40 = 1; // 0x0001C30C
      break;
    case 0x100:
      mGuestA80 = 1; // 0x0001C31C
      SendScriptMsgs(kSS_Attack, mgr, kInvalidUniqueId, kSM_None); // 0x0001C340
      break;
    case 0x100000:
      mGuestAB8 |= 0x80; // 0x0001C350
      SendScriptMsgs(kSS_Attack, mgr, kInvalidUniqueId, kSM_None); // 0x0001C37C
      break;
    case 0x20: // 0x0001C384
      mDive.mLanding = true;
      if (mDive.mBounced == true) { // 0x0001C3A0
        SetGrounded(mgr, true);     // fn_10_18FCC at 0x0001C3A8
        mDive.Reset();              // fn_10_E910 at 0x0001C3B0
        mDive.mLaunched = true;
        mDive.mDone = true;
      }
      break;
    case 0x400000:
      mGuestF5C80 = true; // 0x0001C3DC
      break;
    default:
      break;
    }
    return;
  case kUE_EffectOn: // 0x0001BF54
    if (attack == 1 || attack == 2 || attack == 4) {
      mGuestC00 = mMinAttackRange; // 0x0001BF94
    } else if (attack == 0x1000) {
      mStageChanged = 1; // 0x0001BF88
    }
    return;
  case kUE_EffectOff: // 0x0001BF9C
    mGuestC00 = 0.f;
    return;
  case kUE_FadeIn: // 0x0001C3E8: the cloaked attacks show again
    if (attack == 0x40000 || attack == 0x400000 || attack == 0x800000) {
      mFadeOut = 0;
      // lbl_10_rodata_3FC = 0.0f for 0x40000, else lbl_10_rodata_E0 = 5.0f; lbl_10_rodata_258 = 5.0f
      SetGuestE04(attack == 0x40000 ? 0.f : 5.f, 5.f);
      FadeEnvFxOut(mgr); // fn_10_1BE04 at 0x0001C46C
    }
    break;
  case kUE_FadeOut: // 0x0001C474
    if (attack == 0x80000 || attack == 0x400000 || attack == 0x800000) {
      mFadeOut = 1;
      SetGuestE04(attack == 0x80000 ? 0.f : 5.f, 5.f);
      FadeEnvFxIn(mgr); // fn_10_1BE80 at 0x0001C4F8
    }
    break;
  case kUE_ScreenShake: { // 0x0001C618: at the event's locator, from no one
    const CVector3f pos = GetLctrTransform(node.GetLocatorName()).GetTranslation();
    ApplyScreenShake(mgr, pos, kInvalidUniqueId); // 0x0001C660
    break;
  }
  case kUE_BecomeRagDoll: // 0x0001C500: the boost ball's record reset
    if (attack == 0x20000) {
      mBoostBallUid = kInvalidUniqueId; // 0x0001C530
      mBoostBallFlag20 = false;         // 0xEBC: 0x20 cleared, then 0x40 and 0x80 copied
      mBoostBallFlag40 = false;         // down from it (0x0001C540..0x0001C564)
      mBoostBallFlag80 = false;
      mGuestEA8 = -1000.f; // lbl_10_rodata_410
      mGuestEAC = CVector3f::Zero();
      mGuestEB8 = 0.f;
      SpawnBoostBall(mgr); // fn_10_9E00 at 0x0001C588
      mGuestEA8 = mElapsedTime; // 0x0001C594
      mGuestEAC = GetTranslation();
      mGuestEB8 = (GetTranslation() - mgr.GetPlayer(0)->GetTranslation()).Magnitude(); // 0x0001C5F4
    }
    break;
  case kUE_Unknown37: // 0x0001C5FC
    ClearEnrageSphere(); // fn_10_77B8 at 0x0001C5FC
    break;
  case kUE_Unknown38: // 0x0001C604
    mGuestF5C40 = true;
    break;
  case kUE_Unknown46: // 0x0001C678: the double dash's next step
    if (mDash.mStepsLeft > 0) {
      StartNextDashStep(mgr); // fn_10_14C50 at 0x0001C684
    }
    break;
  default:
    if (static_cast< int >(type) == 43) { // 0x0001C668: lbl_10_rodata_540 = 0.9f
      mGuestD24 = 0.9f;
    }
    break;
  }
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

// vtable slot 29 = fn_10_F40C (0x7C): the alternate scannable info. Alive (0x420 bit 0x40,
// 0x0000F430), active (0x20 bit 0x80, 0x0000F44C), on a stage that asks for it (0x97C bit 0x80,
// 0x0000F45C) and with the token at 0x92C (0x0000F468) the actor answers that token's object
// (+0x8, 0x0000F46C); otherwise CPatterned::GetScannableObjectInfo (0x0000F474).
CScannableObjectInfo* CDarkSamus::GetScannableObjectInfo() const {
  if (GetAlive() == true && GetActive() == true && mUseAltScan == true &&
      !mAltScanInfo.null()) {
    return **mAltScanInfo;
  }
  return CPatterned::GetScannableObjectInfo();
}

// vtable slot 31 = fn_10_1AAC4 (0x1DC). In the dive (0x980 == 0x20, `cmpwi r0, 0x20` at
// 0x0001AAE4) before any bounce (0xB20 bit 0x02, `extrwi. r0, r0, 1, 30` at 0x0001AAF0) and
// within lbl_10_rodata_518 = 16.0f of the dive's target (0xAC0 minus the position, 0x0001AB2C;
// `bge` out at 0x0001AB3C), the dive record's collidable 0xB28 takes the animation's bounds
// (0x0001AB84) grown in x and y by lbl_10_rodata_548 = 1.8f - shrunk by
// (distance - lbl_10_rodata_408 = 4.0f) / lbl_10_rodata_448 = 12.0f beyond 4.0f (0x0001AB50..
// 0x0001AB74) - and in z by lbl_10_rodata_3FC = 0.0f (0x0001ABC0/0x0001ABC8), with the actor's
// material list (this+0x68, 0x0001ABEC); the stores at 0x0001AC3C..0x0001AC68, and that
// collidable is the answer (0x0001AC74). Otherwise CPhysicsActor's (0x0001AC80).
// @ fn_10_1AAC4
const CCollisionPrimitive* CDarkSamus::GetCollisionPrimitive() const {
  if (mCurrentAttack == 0x20 && !mDive.mBounced) { // 0x0001AAE8, 0x0001AAF4
    const float dist = (mDive.mTarget - GetTranslation()).Magnitude(); // 0x0001AB2C
    if (dist < 16.f) {                                                // 0x0001AB3C
      float grow = 1.8f;                                               // 0x0001AB4C
      if (dist > 4.f) {                                                // 0x0001AB54
        grow *= 1.f - (dist - 4.f) / 12.f;                             // 0x0001AB5C..0x0001AB74
      }
      const CAABox bounds = GetAnimationData()->GetBoundingBox();     // 0x0001AB84
      const CAABox box(CVector3f(bounds.GetMinPoint().GetX() - grow,
                                 bounds.GetMinPoint().GetY() - grow,
                                 bounds.GetMinPoint().GetZ() - 0.f),
                       CVector3f(bounds.GetMaxPoint().GetX() + grow,
                                 bounds.GetMaxPoint().GetY() + grow,
                                 bounds.GetMaxPoint().GetZ() + 0.f)); // 0x0001ABE4
      mDive.mCollidable = CCollidableAABox(box, GetMaterialList());
      return &mDive.mCollidable; // 0x0001AC74
    }
  }
  return CPhysicsActor::GetCollisionPrimitive(); // 0x0001AC80
}

// vtable slot 33 = fn_10_1C7FC (0x1A0). The collider cast to an actor (0x0001C838), then
// fn_10_1D0A0 (0x0001C848), the dive's arm fn_10_1C99C (0x0001C85C), fn_10_1CF84 (0x0001C86C) and
// fn_10_A734 (0x0001C87C). In the attacks 0x80 or 0x4 (0x0001C884/0x0001C890), with a boost ball
// (fn_10_9624, 0x0001C8A4) or in the dive (0x20, 0x0001C8AC) in flight (0xB20 bit 0x40,
// `extrwi r0, r0, 1, 25` at 0x0001C8B8, `cmplwi r0, 0x1` at 0x0001C8BC) the platform crush runs
// (fn_10_1D194 at 0x0001C8D4), and the base is skipped while the actor is nearer the dive's start
// (0xACC) than its target (0xAC0) is (the two Magnitudes at 0x0001C930/0x0001C93C, `blt` out at
// 0x0001C944). Then more than two stuck dashes (0x968 > 2, `cmplwi r0, 0x2` + `ble` at
// 0x0001C94C/0x0001C950) crush (0x0001C964) instead of the base (CPatterned at 0x0001C97C).
// @ fn_10_1C7FC
void CDarkSamus::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                              CStateManager& mgr) {
  CActor* collider = TCastToPtr< CActor >(mgr.ObjectById(id));
  RecordPlatformContact(list);                 // fn_10_1D0A0
  CheckDiveCollision(mgr, collider, list);     // fn_10_1C99C
  RecordWallHit(id, list);                     // fn_10_1CF84
  CheckBoostBallWallHit(mgr, list);            // fn_10_A734
  if (mCurrentAttack == 0x80 || mCurrentAttack == 0x4 || HasBoostBall() == true ||
      (mCurrentAttack == 0x20 && mDive.mLaunched == true)) {
    CrushPlatform(mgr, id, list); // 0x0001C8D4
    const float targetDist = (mDive.mTarget - mDive.mStart).Magnitude(); // 0x0001C930
    if ((GetTranslation() - mDive.mStart).Magnitude() < targetDist) {     // 0x0001C93C/0x0001C944
      return;
    }
  }
  if (mDashStuckCount > 2) { // 0x0001C950
    CrushPlatform(mgr, id, list);
  } else {
    CPatterned::CollidedWith(id, list, mgr);
  }
}

// fn_10_1D0A0 (0x0001D0A0, 0xF4 bytes): the first contact whose left material has
// 1 << lbl_10_data_C20 = 0x31 (kMT_Platform, 0x0001D0DC..0x0001D114) ends the scan; unless it is
// also 1 << lbl_10_data_C24 = 0x1F (kMT_Floor, 0x0001D12C, `bne` out at 0x0001D148) the contact
// time 0xC34 takes the elapsed time and 0xC38..0xC40 the contact's point (0x0001D14C..0x0001D168).
// @ fn_10_1D0A0
void CDarkSamus::RecordPlatformContact(const CCollisionInfoList& list) {
  for (int i = 0; i < list.GetCount(); ++i) { // 0x0001D17C
    const CCollisionInfo& info = list[i];
    if (info.GetMaterialLeft().HasMaterial(kMT_Platform)) {
      if (!info.GetMaterialLeft().HasMaterial(kMT_Floor)) {
        mContactTime = mElapsedTime;
        mContactPoint = info.GetPoint();
      }
      return;
    }
  }
}

// fn_10_1CF84 (0x0001CF84, 0x11C bytes): for a collision with no object (the id is
// kInvalidUniqueId, 0x0001CFA4/0x0001CFA8) with no attack (0x980 == 0xFFFF, `addis r0, r3, 0x0;
// cmplwi r0, 0xffff` at 0x0001CFB0/0x0001CFB4) or in 0x2000 (`cmpwi r3, 0x2000` at 0x0001CFBC),
// the first contact whose left material has 1 << lbl_10_data_C28 = 0x1E (kMT_Wall, 0x0001CFF0) or
// else 1 << lbl_10_data_C2C = 0x1D (kMT_Ceiling, 0x0001D040) sets the wall hit time 0xC44 to the
// elapsed time (0x0001D02C / 0x0001D074) and ends the scan.
// @ fn_10_1CF84
void CDarkSamus::RecordWallHit(const TUniqueId& id, const CCollisionInfoList& list) {
  if (id != kInvalidUniqueId) { // 0x0001CFA8
    return;
  }
  if (mCurrentAttack != 0xFFFF && mCurrentAttack != 0x2000) { // 0x0001CFB8, 0x0001CFC0
    return;
  }
  for (int i = 0; i < list.GetCount(); ++i) { // 0x0001D088
    const CMaterialList& mat = list[i].GetMaterialLeft();
    if (mat.HasMaterial(kMT_Wall)) { // 0x0001D024
      mLastWallHitTime = mElapsedTime;
      return;
    }
    if (mat.HasMaterial(kMT_Ceiling)) { // 0x0001D06C
      mLastWallHitTime = mElapsedTime;
      return;
    }
  }
}

// fn_10_A734 (0x0000A734, 0xE0 bytes): with a boost ball (fn_10_9624, `clrlwi.` + `beq` at
// 0x0000A750/0x0000A754) the first contact whose left material has 1 << lbl_10_data_1F8 = 0x1E
// (kMT_Wall) or 1 << lbl_10_data_1FC = 0x1D (kMT_Ceiling) (the `or` mask at 0x0000A7A0..
// 0x0000A7C4) sets 0xEBC's bit 0x80 (`rlwimi r0, r3, 7, 24, 24` at 0x0000A7E4) and ends the scan.
// @ fn_10_A734
void CDarkSamus::CheckBoostBallWallHit(CStateManager& mgr, const CCollisionInfoList& list) {
  if (!HasBoostBall()) { // 0x0000A754
    return;
  }
  const CMaterialList wallOrCeiling(kMT_Wall, kMT_Ceiling);
  for (int i = 0; i < list.GetCount(); ++i) { // 0x0000A7FC
    if ((list[i].GetMaterialLeft().GetValue() & wallOrCeiling.GetValue()) != 0) { // 0x0000A7D8
      mBoostBallFlag80 = true;
      return;
    }
  }
}

// fn_10_1D194 (0x0001D194, 0x430 bytes): the collider must be a CScriptPlatform (0x0001D1D4, the
// null test 0x0001D1DC). No contact may have kMT_Floor on its left (the low word's 0x80000000,
// `lis r5, 0x8000` at 0x0001D1EC, `beq` out at 0x0001D234); the first contact with kMT_Platform
// (the high word's 0x20000 = bit 49, `lis r4, 0x2` at 0x0001D244) crushes the platform with three
// ApplyDamage calls (0x0001D380, 0x0001D498, 0x0001D594) from this actor to the platform, each
// of lbl_10_rodata_558 = 1000.0f damage and radius damage, 0.0f radius and knockback
// (lbl_10_rodata_3FC), the sound ids 0xFFFF (`lis 1; subi 1`), no-immunity set (the byte's 0x80,
// `rlwimi 7, 24, 24`) and radius damage off (0x40 cleared, `rlwimi 6, 25, 25`), the filter
// include 1 << lbl_10_data_200 = 0x3B (kMT_Solid) / exclude nothing / type 3 (`li r10, 0x3`) and
// the zero vector: kWT_Power (type 0, 0x0001D294..0x0001D2E4), kWT_Power comboed (the 0x4000 bit,
// `rlwimi r7, r0, 14, 17, 17` at 0x0001D3F4) and kWT_UnknownSource (0x13, `li r0, 0x13` at
// 0x0001D4A0) instant-kill (the 0x2000 bit, `rlwimi r7, r0, 13, 18, 18` at 0x0001D4EC).
// @ fn_10_1D194
void CDarkSamus::CrushPlatform(CStateManager& mgr, const TUniqueId& id,
                               const CCollisionInfoList& list) {
  if (TCastToPtr< CScriptPlatform >(mgr.ObjectById(id)) == nullptr) { // 0x0001D1DC
    return;
  }
  for (int i = 0; i < list.GetCount(); ++i) { // 0x0001D23C
    if (list[i].GetMaterialLeft().HasMaterial(kMT_Floor)) { // 0x0001D234
      return;
    }
  }
  for (int i = 0; i < list.GetCount(); ++i) { // 0x0001D5A0
    if (list[i].GetMaterialLeft().HasMaterial(kMT_Platform)) { // 0x0001D288
      const CMaterialFilter filter =
          CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList());
      mgr.ApplyDamage(GetUniqueId(), id, GetUniqueId(),
                      CDamageInfo(CWeaponMode(kWT_Power), 1000.f, 0.f, 0.f, true, false), filter,
                      CVector3f::Zero()); // 0x0001D380
      mgr.ApplyDamage(GetUniqueId(), id, GetUniqueId(),
                      CDamageInfo(CWeaponMode(kWT_Power, false, true, false), 1000.f, 0.f, 0.f,
                                  true, false),
                      filter, CVector3f::Zero()); // 0x0001D498
      mgr.ApplyDamage(GetUniqueId(), id, GetUniqueId(),
                      CDamageInfo(CWeaponMode(kWT_UnknownSource, false, false, true), 1000.f, 0.f,
                                  0.f, true, false),
                      filter, CVector3f::Zero()); // 0x0001D594
      return;
    }
  }
}

// vtable slot 40 = fn_10_FF20 (0x94): the damage vulnerability the actor is hit with - the phazon
// sub-object at 0xCD4 when the byte at 0xCD0 is 1; during the phazon enrage the patterned
// vulnerability copied to 0xFD0 with combo slot 3 set to (0.0f, kE_Normal, false); the CAi one
// otherwise.
CDamageVulnerability* CDarkSamus::DamageVulnerability() {
  if (mGuestCD0 == 1) { // 0x0000FF38
    return &mGuestCD4;
  }
  if (mCurrentAttack == 0x800000) { // 0x0000FF4C (subis/cmplwi)
    mEnrageVulnerability = *CPatterned::GetDamageVulnerability(); // 0x0000FF64
    mEnrageVulnerability.SetComboVulnerability(                     // 0x0000FF90
        3, CWeaponTypeVulnerability(0.f, CWeaponTypeVulnerability::kE_Normal, false));
    return &mEnrageVulnerability;
  }
  return CAi::DamageVulnerability(); // 0x0000FF9C
}

// vtable slot 41 = fn_10_F488 (0x138): the damage handling. The hit cooldown 0x448 =
// skDamageHitTime (0x0000F4AC, CPatterned::TakeDamage). Active (0x0000F4C8), a health below
// lbl_10_rodata_3E8 = 1.0f (0x0000F4EC) is held at 1.0f (the vtable slot 0x38, 0x0000F528) when
// the stage forbids dying (0x97C bit 0x40 clear, 0x0000F4F8) or in the phazon phase (0x1060 bit
// 0x80, 0x0000F508). In the phazon phase (0x0000F538) a health below 0x1054 -
// lbl_10_rodata_E4 = 60.0f (0x0000F568) is set to that mark - lbl_10_rodata_41C = 0.1f, the
// 0xCD0 byte to 1 and 0xD24 to 1.0f (0x0000F598..0x0000F5A0).
void CDarkSamus::TakeDamage(const CVector3f& direction, float magnitude) {
  CPatterned::TakeDamage(direction, magnitude);
  if (GetActive() != true) {
    return;
  }
  if (GetHealthInfo()->GetHP() < 1.f) {
    if (mCanDie == false || mPhazonPhase == true) {
      HealthInfo()->SetHP(1.f);
    }
  }
  if (mPhazonPhase == true) {
    const float mark = mPhazonHealthMark - 60.f;
    if (GetHealthInfo()->GetHP() < mark) {
      HealthInfo()->SetHP(mark - 0.1f);
      mGuestCD0 = 1;
      mGuestD24 = 1.f;
    }
  }
}

// vtable slot 45 = fn_10_CEB4 (0x1C): it copies the three floats out of this+0x54/0x58/0x5C, the
// actor's translation.
CVector3f CDarkSamus::GetOrigin(const CStateManager& mgr, const CTeamAiRole& role,
                                const CVector3f& point) const {
  return GetTranslation();
}

// vtable slot 55 = fn_10_1C6C8 (0xB0): the projectile info the attacks launch.
CProjectileInfo* CDarkSamus::ProjectileInfo() {
  // The switch on the current attack 0x980: the scatter shots 8 and 0x10 (0x0001C718) take the
  // second info 0xBB0 when 0xBAE is 1, the first 0xB50 otherwise; the missiles 0x40/0x80 the
  // info 0xA0C (0x0001C734), 0x100 the super missile's 0xA4C (0x0001C73C), 0x800000 the phazon
  // infos by 0xFB8: 0 the 0xF80 one (0x0001C760), 1 the 0x1064 one (0x0001C768), else null. Port limit: an info whose asset was invalid is never built on the host, so
  // it answers null too.
  switch (mCurrentAttack) {
  case 8:
  case 0x10:
    if (mScatterUseSecondInfo == 1) {
      return mScatterShotInfo2 ? &mScatterShotInfo2.data() : nullptr;
    }
    return mScatterShotInfo ? &mScatterShotInfo.data() : nullptr;
  case 0x40:
  case 0x80:
    return mMissileInfo ? &mMissileInfo.data() : nullptr;
  case 0x100:
    return mSuperMissileInfo ? &mSuperMissileInfo.data() : nullptr;
  case 0x800000:
    if (mGuestFB8 == 0) {
      return mPhazonShotInfo ? &mPhazonShotInfo.data() : nullptr;
    }
    if (mGuestFB8 == 1) {
      return mPhazonSuperMissileInfo ? &mPhazonSuperMissileInfo.data() : nullptr;
    }
    return nullptr;
  default:
    return nullptr;
  }
}

// vtable slot 73 = fn_10_CE94 (0x8): it returns this+0x754, the base class's own member.
const rstl::optional_object< TCachedToken< CGenDescription > >&
CDarkSamus::GetDeathExplosionParticle() const {
  return CPatterned::GetDeathExplosionParticle();
}

// CDarkSamus::TypesMatch is the one the DOL asks for (vtable slot 3, TypesMatch__10CDarkSamusCFi);
// it is implemented with the shared macro in src/MetroidPrime/TypesMatch.cpp, as for the other
// port bodies.

// ---------------------------------------------------------------------------------------------
// The module's loader (fn_10_CF44) and entry points.

static CEntity* PortLoadDarkSamus(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrDarkSamus sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrDarkSamus.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL checks the copy's valid byte (sp+0x36c at 0x0000DA3C) and returns null when the
    // model is empty.
    return nullptr;
  }

  // The 63 property ids the REL's own switch reads (fn_10_CF44's dispatch tree) are exactly the
  // ids of the generated SLdrDarkSamus reader, so the reader above already read all of them. The
  // host keeps the 60 data ones in one block; the fields are named after the property they come
  // from and nothing in this milestone reads them.
  CDarkSamusData data;
  data.mUnknown72EDEB7D = sldrThis.unknown_0x72edeb7d;
  data.mUnknown74FA22F0 = sldrThis.unknown_0x74fa22f0;
  data.mGlideSound = sldrThis.glideSound;
  data.mMissileRicochetSound = sldrThis.missileRicochetSound;
  data.mStartsInTheAir = sldrThis.startsInTheAir;
  data.mTXTR = sldrThis.tXTR;
  data.mUnknown2C6A3344 = sldrThis.unknown_0x2c6a3344;
  data.mMeleeAttackDamage = LdrToDamageInfo(sldrThis.meleeAttackDamage);
  data.mMeleeAttackFX = sldrThis.meleeAttackFX;
  data.mPart = sldrThis.pART;
  data.mDiveAttackDamage = LdrToDamageInfo(sldrThis.diveAttackDamage);
  data.mDiveAttackImpulseVertical = sldrThis.diveAttackImpulseVertical;
  data.mDiveAttackImpulseHorizontal = sldrThis.diveAttackImpulseHorizontal;
  data.mDiveAttackEffect = sldrThis.diveAttackEffect;
  data.mScatterShotProjectile = sldrThis.scatterShotProjectile;
  data.mScatterShotProjectile2 = sldrThis.scatterShotProjectile2;
  data.mScatterShotDamage = LdrToDamageInfo(sldrThis.scatterShotDamage);
  data.mUnknown378285B9 = sldrThis.unknown_0x378285b9;
  data.mScatterShotProjectilesPerSecond = sldrThis.scatterShotProjectilesPerSecond;
  data.mNormalMissileProjectile = sldrThis.normalMissileProjectile;
  data.mNormalMissileDamage = LdrToDamageInfo(sldrThis.normalMissileDamage);
  data.mUnknown1F1EF7A9 = sldrThis.unknown_0x1f1ef7a9;
  data.mUnknownC4A1A44E = sldrThis.unknown_0xc4a1a44e;
  data.mSuperMissileProjectile = sldrThis.superMissileProjectile;
  data.mSuperMissileDamage = LdrToDamageInfo(sldrThis.superMissileDamage);
  data.mFreezeBeamProjectile = sldrThis.freezeBeamProjectile;
  data.mFreezeBeamDamage = LdrToDamageInfo(sldrThis.freezeBeamDamage);
  data.mFreezeBeamVisorTexture = sldrThis.freezeBeamVisorTexture;
  data.mDamageInterruptThreshold = sldrThis.damageInterruptThreshold;
  data.mUnknownF317F4D5 = sldrThis.unknown_0xf317f4d5;
  data.mSweepSwoosh = sldrThis.sweepSwoosh;
  data.mSweepBeamDamage = LdrToDamageInfo(sldrThis.sweepBeamDamage);
  data.mSweepBeamCollisionEffect = sldrThis.sweepBeamCollisionEffect;
  data.mSweepBeamSound = sldrThis.sweepBeamSound;
  data.mUnknown0EF8DC15 = sldrThis.unknown_0x0ef8dc15;
  data.mInvulnerableModel = sldrThis.invulnerableModel;
  data.mInvulnerableSkinRules = sldrThis.invulnerableSkinRules;
  data.mBoostBallModel = LdrToAnimationParameters(sldrThis.boostBallModel);
  data.mBoostBallDamage = LdrToDamageInfo(sldrThis.boostBallDamage);
  data.mBoostBallGlow = sldrThis.boostBallGlow;
  data.mBoostBallTrailSwoosh = sldrThis.boostBallTrailSwoosh;
  data.mSWHC = sldrThis.sWHC;
  data.mBoostBallRollSound = sldrThis.boostBallRollSound;
  data.mBoostBallHitPlayerSound = sldrThis.boostBallHitPlayerSound;
  data.mBoostBallCollision = sldrThis.boostBallCollision;
  data.mBoostBallCollisionSoundOneShot = sldrThis.boostBallCollisionSound_OneShot;
  data.mIceSpreadFX = sldrThis.iceSpreadFX;
  data.mIceSpreadSound = sldrThis.iceSpreadSound;
  data.mPart908B06E9 = sldrThis.pART_0x908b06e9;
  data.mPart494DE4A4 = sldrThis.pART_0x494de4a4;
  data.mSound = sldrThis.sound;
  data.mDamageInfo = LdrToDamageInfo(sldrThis.damageInfo);
  data.mPartE701DAEA = sldrThis.pART_0xe701daea;
  data.mPhazonProjectile = sldrThis.phazonProjectile;
  data.mPhazonSuperMissileProjectile = sldrThis.phazonSuperMissileProjectile;
  data.mPhazonSuperMissileDamage = LdrToDamageInfo(sldrThis.phazonSuperMissileDamage);
  data.mPhazonProjectileDamage = LdrToDamageInfo(sldrThis.phazonProjectileDamage);
  data.mPhazonEnrageSphere = sldrThis.phazonEnrageSphere;
  data.mPhazonEnrageSphereContactDamage = LdrToDamageInfo(sldrThis.phazonEnrageSphereContactDamage);
  data.mAlternateScannableInfo = sldrThis.alternateScannableInfo;

  return rs_new CDarkSamus(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                           LdrToEntityInfo(info, sldrThis.editorProperties),
                           LdrToTransform4f(sldrThis.editorProperties), *modelData,
                           LdrToPatternedInfo(sldrThis.patterned, nullptr),
                           LdrToActorParameters(sldrThis.actorInformation), data);
}

static SDarkSamus_FuncPtrs REL_loader_DarkSamus;

// Retail's RELMain calls fn_10_CF14, which stores the loader fn_10_CF44 into lbl_10_bss_110 (the
// module's SDarkSamus_FuncPtrs) and calls the DOL's SetSDarkSamus_FuncPtrs (fn_802188E4). The
// proxy in SDarkSamus_FuncPtrs.cpp calls through it.
void RELMain() {
  REL_loader_DarkSamus.mLoader = PortLoadDarkSamus;
  SetSDarkSamus_FuncPtrs(&REL_loader_DarkSamus);
}

void RELExit() { SetSDarkSamus_FuncPtrs(nullptr); }
