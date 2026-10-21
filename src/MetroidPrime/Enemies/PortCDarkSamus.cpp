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

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Enemies/CDarkSamusBattleStage.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrDarkSamus.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/Weapons/CGameProjectile.hpp"

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
, mGuestC0C(-1.f)                                // lbl_10_rodata_3EC, 0x0001FEE4
, mGuestC20(-1000.f)                             // lbl_10_rodata_410, 0x0001FF04
, mRicochetSfxId(data.mMissileRicochetSound == -1 // 0x0000DA94, stored at 0x0001FF48
                     ? CSfxManager::kInternalInvalidSfxId
                     : static_cast< ushort >(data.mMissileRicochetSound))
, mGuestA40(0)
, mGuestA80(0)
, mGuestAB8(0)
, mGuestC30(0.f)
, mGuestF64(0.f)
, mGuestEE4(0.f)
, mGuestEE8(0.f)
, mGuestEEC(0.f)
, mGuestEA4(0.f)
, mGuestEA8(0.f)
, mGuestEAC(0.f)
, mGuestEB0(0.f)
, mGuestEB4(0.f)
, mGuestEB8(0.f)
, mGuestFB0(0.f)
, mGuestFC4(0.f)
, mGuest1058(0)
, mGuest105C(0)
, mGuestAB0(0)
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
  // advance (fn_10_193A0, 0x0001B308) and the attack pick (fn_10_1954C, 0x0001B314) and the stage
  // watch (fn_10_FD4C, 0x0001B33C); the eleven others (fn_10_1ACA0 .. fn_10_1B154) are not ported, so the body stays a stand-in rather
  // than a retail-order Think.
  if (GetAlive()) {
    AdvanceBattleStage(mgr);
    UpdateAttackPick(mgr);
    WatchStageChange(mgr); // 0x0001B33C (fn_10_1ACA0 and fn_10_10A64 before it are not ported)
  }
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
void CDarkSamus::SetupStateMachine(CStateManager& mgr) {
  CPatterned::SetupStateMachine(mgr);
  InstallStateMachineFunctions();
}

// vtable slot 9 = fn_10_18AB8 (0x2C0): the pre-render pass (the locator-driven effect setup).
void CDarkSamus::PreRender(CStateManager& mgr) {
  StandIn("fn_10_18AB8", "vtable slot 9, PreRender");
  CPatterned::PreRender(mgr);
  // 0x00018C18: the gun locator's transform the scatter shot fires from (0x8CC), then the root
  // bone's position (lbl_10_rodata_560 + 0x551, 0x00018C50) the projectile reflect measures from
  // (0x8FC, 0x00018C64..0x00018C6C). The rest of the pass (the effects and the colour lerps around
  // it) is not in the port yet.
  mGunLctrXf = GetLctrTransform(rstl::string_l("GUN_Particle_LCTR"));
  mRootPos = GetLctrTransform(rstl::string_l("Skeleton_Root")).GetTranslation();
}

// vtable slot 10 = fn_10_17F04 (0x2D8).
void CDarkSamus::AddToRenderer(const CStateManager& mgr) const {
  StandIn("fn_10_17F04", "vtable slot 10, AddToRenderer");
  CPatterned::AddToRenderer(mgr);
}

// vtable slot 11 = fn_10_18964 (0x154).
void CDarkSamus::Render(const CStateManager& mgr) const {
  StandIn("fn_10_18964", "vtable slot 11, Render");
  CPatterned::Render(mgr);
}

// vtable slot 15 = fn_10_0 (0x2C): it calls the vtable's HealthInfo slot (0x38) and returns its
// result, which is what the base class's own accessor does.
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

// vtable slot 28 = fn_10_1BEFC (0x7CC): the user-anim-event switch (the attacks' timings).
void CDarkSamus::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                                 float dt) {
  StandIn("fn_10_1BEFC", "vtable slot 28, DoUserAnimEvent");
  // The switch on the event type (`cmplwi r31, 0x2e` at 0x0001BF28, jumptable_10_data_FB4).
  // kUE_EventStart (case 27, 0x0001C1F0) switches on the current attack 0x980 and never reaches
  // the base: the scatter shots start (fn_10_11814) with side 0 for 8 (0x0001C25C) and the byte
  // 0xBAC for 0x10 (0x0001C268). Its 0x100 (0xA84 = 1), 0x100000 (0xAB8 bit 0x40) and 0x400000
  // (fn_10_6118, fn_10_8178) arms are not in the port yet.
  // kUE_Projectile (case 0, 0x0001C134) switches on the current attack 0x980 and never reaches
  // the base either: the melee hit fn_10_13AB4 for 1..4 except 3 (0x0001C15C/0x0001C168,
  // 0x0001C1B8), the missile fn_10_1353C for 0x40/0x80 (0x0001C1C8), fn_10_133D0 for 0x100
  // (0x0001C1D0), fn_10_9164 for 0x20000/0x100000 (0x0001C1E0) and fn_10_5840 for 0x800000
  // (0x0001C1E8); any other attack does nothing. None of those five is in the port yet, so the
  // melee and missile events launch nothing - the base's LaunchProjectile would read the null
  // ProjectileInfo those attacks answer.
  if (type == kUE_Projectile) {
    return;
  }
  if (type == kUE_EventStart) {
    switch (mCurrentAttack) {
    case 8:
      StartScatterShotFire(mgr, 0);
      break;
    case 0x10:
      StartScatterShotFire(mgr, mScatterSide);
      break;
    default:
      break;
    }
    return;
  }
  // kUE_BeginAction (case 17, 0x0001C004) and kUE_EndAction (case 18, 0x0001C0A0) switch on the
  // current attack 0x980 and never reach the base: the sweep beam (0x10000, `lis r0, 0x1` at
  // 0x0001C008 / 0x0001C0A4) starts its effect (fn_10_C85C at 0x0001C098) and releases it
  // (fn_10_C798 at 0x0001C11C). BeginAction's 0x2 arm (0x0001C018) and EndAction's 0x40, 0x100,
  // 0x100000 counters (0xA38/0xA78/0xAB0) and 0x400000 arm are not in the port yet.
  if (type == kUE_BeginAction) {
    if (mCurrentAttack == 0x10000) {
      StartSweepBeam();
    }
    return;
  }
  if (type == kUE_EndAction) {
    if (mCurrentAttack == 0x10000) {
      StopSweepBeam();
    }
    return;
  }
  // kUE_Landing (case 12, 0x0001C2C0) switches on the current attack too; the dive's arm
  // (0x20, 0x0001C384) starts the landing (0x10) and, after a bounce (0x0001C3A0), puts the actor
  // on the ground (fn_10_18FCC with 1), resets the dive record and ends it (0x40 and 0x80). Its
  // 0x40, 0x100, 0x100000 and 0x400000 arms are not in the port yet.
  if (type == kUE_Landing && mCurrentAttack == 0x20) {
    mDive.mLanding = true;
    if (mDive.mBounced == true) {
      SetGrounded(mgr, true);
      mDive.Reset();
      mDive.mLaunched = true;
      mDive.mDone = true;
    }
    return;
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
  // second info 0xBB0 when 0xBAE is 1, the first 0xB50 otherwise. The missiles' 0x40/0x80
  // (0xA0C), 0x100 (0xA4C) and 0x800000 (0xF80 / 0x1064 by 0xFB8) infos are not in the port yet,
  // so those arms answer null as the default does (0x0001C770). Port limit: an info whose asset
  // was invalid is never built on the host, so it answers null too.
  switch (mCurrentAttack) {
  case 8:
  case 0x10:
    if (mScatterUseSecondInfo == 1) {
      return mScatterShotInfo2 ? &mScatterShotInfo2.data() : nullptr;
    }
    return mScatterShotInfo ? &mScatterShotInfo.data() : nullptr;
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
