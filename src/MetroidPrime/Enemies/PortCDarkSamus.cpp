// Host body for the DarkSamus REL (module 10), milestone 1: the actor loads, registers and
// thinks. Every vtable override that belongs to retail's AI forwards to CPatterned behind a
// once-only stand-in log, and the REL function it stands in for is named on each. The state
// machine, the melee / dive / scatter-shot / missile attacks, the boost ball, the freeze beam,
// the sweep beam and the phazon enrage are later milestones - docs/module-analysis/DarkSamus.md
// section i has the plan.
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

#include <math.h>
#include <stdio.h>

namespace {

// Reports a stand-in the first time it runs, so a log lists what is still a shell. The retail
// function is the key, so the same function is reported once even if several call sites reach it.
void StandIn(const char* fn, const char* what) {
  static const char* sSeen[48] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == fn) {
      return;
    }
  }
  if (sCount < 48) {
    sSeen[sCount++] = fn;
  }
  fprintf(stderr, "[darksamus] stand-in %s (%s)\n", fn, what);
}

} // namespace

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
, mGuestD18(CVector3f::Zero())                  // 0x0000E5C8 (lbl_10_bss_128)
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

// vtable slot 5 = fn_10_1B1FC (0x27C). Retail runs the state machine's own tick first (it asks
// the machine for a state through the vtable slot 0x28), then aims at the player (the vector at
// 0x6E0), then runs CPatterned::Think and afterwards the module's own AI.
void CDarkSamus::Think(float dt, CStateManager& mgr) {
  StandIn("fn_10_1B1FC", "vtable slot 5, Think");
  // 0x0001B2DC: retail grows the elapsed time at 0x8B0 by dt in every Think, also while the
  // actor is inactive, so the port grows it before the active check.
  mElapsedTime += dt;
  if (!GetActive()) {
    return;
  }
  CPatterned::Think(dt, mgr);
  // 0x0001B2E4: the module's own AI runs only while the actor is alive (the guest's 0x420 bit 6,
  // the extrwi at 0x0001B2E8 - the host's GetAlive()). Of its calls the port has the stage
  // advance (fn_10_193A0, 0x0001B308), the attack pick (fn_10_1954C, 0x0001B314), the head aim
  // (fn_10_10A64, 0x0001B330) and the stage watch (fn_10_FD4C, 0x0001B33C); the ten others
  // (fn_10_1ACA0 .. fn_10_1B154) are not ported, so the body stays a stand-in rather than a
  // retail-order Think.
  if (GetAlive()) {
    AdvanceBattleStage(mgr);
    UpdateAttackPick(mgr);
    UpdateHeadAim(mgr);    // fn_10_10A64 at 0x0001B330 (fn_10_1ACA0 before it is not ported)
    WatchStageChange(mgr); // 0x0001B33C
  }
  // 0x0001B3DC: alive or not, the visibility fade, the env fx faded out while not fading
  // (0xDEC == 0, 0x0001B3F0) and alive (0x0001B40C), the targetability, and the melee pulse and
  // the boost ball's effects with dt.
  UpdateFade(dt);                                  // fn_10_1D6CC at 0x0001B3E4
  if (mFadeOut == 0 && GetAlive()) {
    mgr.EnvFxManager()->FadeDensity(0.f, 4000);    // lbl_10_rodata_3FC, lbl_10_rodata_254 (0x0001B42C)
  }
  UpdateTargetable(mgr);                           // fn_10_1D5C4 at 0x0001B438
  UpdateMeleePulseAndEffects(mgr, dt);             // fn_10_12DD0 at 0x0001B448
}

// vtable slot 6 = fn_10_1E89C (0x5B0): the script-message switch. The four-character codes it
// compares are 'XCRT' (0x0001E91C, the arm that also activates the body controller, 0x0001E9A4),
// 'ACTV' (0x0001E908), 'ALRT' (0x0001E8F4), 'DCTV' (0x0001E8E0), 'XALD' (0x0001E930), 'XDEL'
// (0x0001E8B8), 'XDMG' (0x0001E968), 'XLAU' (0x0001E958), 'XLND' (0x0001E944), 'XLSG'
// (0x0001E98C) and 'XOFF' (0x0001E978).
// fn_10_1E864 (0x0001E864, 0x38 bytes): the path search's area - the actor's area (the guest's
// this+4) in the world's area list (the state manager's 0x1604, the list at 0x24 and the entry's
// +4 at 0x0001E884), its post-constructed data (+0x104) and that data's path area (+0xEC), stored
// into the search (the guest's 0x7C0, 0x0001E890).
// @ fn_10_1E864
void CDarkSamus::SetPathSearchArea(CStateManager& mgr) {
  mPathFindSearch.SetArea(
      mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea);
}

void CDarkSamus::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("fn_10_1E89C", "vtable slot 6, AcceptScriptMsg");
  // The 'XDMG' arm (0x58444D47, the host's kSM_Damage; the compare at 0x0001E96C) is the
  // projectile that hit the actor: the sender must be a CGameProjectile (the cast at
  // 0x0001EA40), the vulnerability (the vtable's slot 16, fn_10_100DC, at 0x0001EA5C) must be
  // hurt by the projectile's damage info (fn_800DB834, the host's WeaponHurts, at 0x0001EA64),
  // and the last hit must be lbl_10_rodata_438 = 3.0f old (0x0001EAB4). The hit direction (the
  // projectile's position minus the actor's, 0x0001EA70..0x0001EAA4) goes to 0x8C0/0x8C4/0x8C8.
  // Then fn_10_1EE4C (0x0001EED8) decides whether the missile can be sent back: then 0xC0C
  // takes the elapsed time (0x0001EAF8) and 0xC10/0xC14/0xC18 the flattened direction
  // (0x0001EB04..0x0001EB18), normalized (0x0001EB30).
  if (msg.GetMessage() == kSM_Damage) {
    const CGameProjectile* proj =
        TCastToConstPtr< CGameProjectile >(mgr.GetObjectById(msg.GetSenderId()));
    if (proj != nullptr &&  // the retail's own null test, 0x0001EA48
        GetDamageVulnerability()->WeaponHurts(                // 0x0001EA5C, 0x0001EA64
            proj->GetCurrentDamageInfo().GetWeaponMode()) &&
        3.f + (mGuestC0C > mGuestC20 ? mGuestC0C : mGuestC20) <= mElapsedTime) { // 0x0001EAB4
      mHitDirection = proj->GetTranslation() - GetTranslation(); // 0x0001EA70..0x0001EAA4
      if (CanReflectHit(mgr, proj->GetCurrentDamageInfo(), proj->GetUniqueId())) { // 0x0001EED8
        mGuestC0C = mElapsedTime; // 0x0001EAF8
        mHitDirectionFlat = mHitDirection;
        mHitDirectionFlat.SetZ(0.f); // 0x0001EB18
        if (mHitDirectionFlat.CanBeNormalized() == true) {  // 0x0001EB1C
          mHitDirectionFlat.Normalize(); // 0x0001EB30
        }
      }
    }
  }
  // The 'ALRT' arm (0x414C5254, the host's kSM_Alert; the compare at 0x0001E8FC) sets the
  // alerted flag: the byte 0x90C's bit 7 (the rlwimi at 0x0001EA24 keeps the mask 0x80, the
  // PowerPC bit 24 being 1u << (31 - 24)).
  if (msg.GetMessage() == kSM_Alert) {
    mFlags90C |= 0x80;
  }
  // The 'XCRT' arm (0x58435254, the host's kSM_Create; the compare at 0x0001E91C) loads the body
  // controller (this+0x48C at 0x0001E99C), passes -1 for the animation state (0x0001E9A0) and
  // calls CBodyController::Activate (0x0001E9A4). Every arm then falls into the common tail,
  // which forwards the message to CPatterned (the call at 0x0001EE2C), so the message is
  // forwarded here too - after the activation, in retail's order.
  if (msg.GetMessage() == kSM_Create) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
  // The 'XALD' arm (0x58414C44, kSM_AreaLoaded; the compare at 0x0001E938) links the fight
  // script's object: the object connected with the state 'IS05' (kSS_InternalState5, the lis/addi
  // at 0x0001EB60/0x0001EB70) and any message (-1, 0x0001EB74). When the uid is valid (the cmplw
  // at 0x0001EB8C) and the object is a CScriptDarkSamusBattleStage (the cast at 0x0001EBA4, the
  // null test at 0x0001EBA8), its uid goes to 0x974 and to 0xD50 and the stage count becomes 1
  // (0x0001EBB8..0x0001EBC4). Then the path search's area (fn_10_1E864 at 0x0001EBD0); the
  // collision primitive (0x0001EBE0..), the boss parameters (0x0001ECDC) and the material filter
  // (0x0001ED50) after it are not in the port yet.
  if (msg.GetMessage() == kSM_AreaLoaded) {
    const TUniqueId stageUid =
        FindConnectedObject(mgr, kSS_InternalState5, kSM_None);
    if (stageUid != kInvalidUniqueId) {
      if (const CDarkSamusBattleStage* stage =
              TCastToConstPtr< CDarkSamusBattleStage >(mgr.GetObjectById(stageUid))) {
        mBattleStageUid = stage->GetUniqueId();
        mSeenStageUid = mBattleStageUid;
        mStageCount = 1;
      }
    }
    SetPathSearchArea(mgr);
  }
  // The 'XLAU' arm (kSM_Launching, 0x0001ED58): the base first takes a kSM_OffGround from the
  // same sender (0x0001EDA0), then the dive is in flight (0x40, 0x0001EDAC) from the actor's
  // position and the bubble comes on (fn_10_12EF8 at 0x0001EDD4); the original message goes to
  // the base in the common tail.
  if (msg.GetMessage() == kSM_Launching) {
    CPatterned::AcceptScriptMsg(mgr, CScriptMsg(msg.GetSenderId(), GetUniqueId(), kSM_OffGround,
                                                kInvalidUniqueId,
                                                static_cast< EScriptObjectState >(-1)));
    mDive.mLaunched = true;
    mDive.mStart = GetTranslation();
    StartDiveBubble(mgr);
  }
  // The 'XLND' arm (kSM_Landed, 0x0001EDDC): in the dive (0x20, 0x0001EDE4), after a bounce
  // (0x0001EDF4) the dive is done and vertical movement off (0x0001EE00/0x0001EE0C); the message
  // reaches the base only when the dive is done (`extrwi. 1,24` at 0x0001EE18).
  if (msg.GetMessage() == kSM_Landed && mCurrentAttack == 0x20) {
    if (mDive.mBounced == true) {
      mDive.mDone = true;
      mVerticalMovement = false;
    }
    if (!mDive.mDone) {
      return;
    }
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
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

// vtable slot 18 = fn_10_F5C0 (0x2C): it builds the box from CPhysicsActor::GetBoundingBox.
rstl::optional_object< CAABox > CDarkSamus::GetTouchBounds() const {
  StandIn("fn_10_F5C0", "vtable slot 18, GetTouchBounds");
  return CPatterned::GetTouchBounds();
}

// vtable slot 20 = fn_10_10358 (0x12C).
CVector3f CDarkSamus::GetOrbitPosition(const CStateManager& mgr) const {
  StandIn("fn_10_10358", "vtable slot 20, GetOrbitPosition");
  return CPatterned::GetOrbitPosition(mgr);
}

// vtable slot 21 = fn_10_10210 (0x148).
CVector3f CDarkSamus::GetAimPosition(const CStateManager& mgr, float dt) const {
  StandIn("fn_10_10210", "vtable slot 21, GetAimPosition");
  return CPatterned::GetAimPosition(mgr, dt);
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

// vtable slot 31 = fn_10_1AAC4 (0x1DC): the collision primitive the attacks use.
const CCollisionPrimitive* CDarkSamus::GetCollisionPrimitive() const {
  StandIn("fn_10_1AAC4", "vtable slot 31, GetCollisionPrimitive");
  return CPhysicsActor::GetCollisionPrimitive();
}

// vtable slot 33 = fn_10_1C7FC (0x1A0): what the actor collides with.
void CDarkSamus::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                              CStateManager& mgr) {
  StandIn("fn_10_1C7FC", "vtable slot 33, CollidedWith");
  // The dive's arm (fn_10_1C99C at 0x0001C85C) with the collider cast to an actor
  // (0x0001C838); fn_10_1D0A0 before it and fn_10_1CF84, fn_10_A734 and fn_10_1D194 after it are
  // not in the port yet, so the base takes the rest.
  CheckDiveCollision(mgr, TCastToPtr< CActor >(mgr.ObjectById(id)), list);
  CPatterned::CollidedWith(id, list, mgr);
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
