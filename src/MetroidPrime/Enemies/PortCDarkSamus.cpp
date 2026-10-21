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
, mFirstBattleStageUid(kInvalidUniqueId)
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
, mStillPosition(CVector3f(0.f, 0.f, 0.f))
, mLastWallHitTime(0.f)
, mElapsedTime(0.f)
, mPathFindDestination(CVector3f(0.f, 0.f, 0.f))
, mAdditiveAnim(-1)
, mGlideSfxId(data.mGlideSound == -1 ? CSfxManager::kInternalInvalidSfxId
                                    : static_cast< ushort >(data.mGlideSound))
, mGlideSfx()
, mSavedSpeed(0.f)
, mGuest9FC(-1)
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
, mNextActionTime(-1000.f) { // the same f0, 0x0001FCF4
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
  // the extrwi at 0x0001B2E8 - the host's GetAlive()). Of its calls the port has the attack pick
  // (fn_10_1954C, the call at 0x0001B314); the stage advance before it (fn_10_193A0, 0x0001B308)
  // and the twelve after it (fn_10_1ACA0 .. fn_10_1B154) are not ported, so the body stays a
  // stand-in rather than a retail-order Think.
  if (GetAlive()) {
    UpdateAttackPick(mgr);
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
        mFirstBattleStageUid = mBattleStageUid;
        mStageCount = 1;
      }
    }
    SetPathSearchArea(mgr);
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

// vtable slot 16 = fn_10_100DC (0x134): the phazon-phase vulnerability.
const CDamageVulnerability* CDarkSamus::GetDamageVulnerability() const {
  StandIn("fn_10_100DC", "vtable slot 16, GetDamageVulnerability");
  return CPatterned::GetDamageVulnerability();
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
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

// vtable slot 29 = fn_10_F40C (0x7C): the alternate scannable info (the 'SCAN' token at 0x92C
// when the actor is alive, frozen and visible).
CScannableObjectInfo* CDarkSamus::GetScannableObjectInfo() const {
  StandIn("fn_10_F40C", "vtable slot 29, GetScannableObjectInfo");
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
  CPatterned::CollidedWith(id, list, mgr);
}

// vtable slot 40 = fn_10_FF20 (0x94): the damage vulnerability the actor is hit with - the phazon
// sub-object at 0xCD4 when the byte at 0xCD0 is 1, the combo vulnerability at 0xFD0 otherwise.
CDamageVulnerability* CDarkSamus::DamageVulnerability() {
  StandIn("fn_10_FF20", "vtable slot 40, DamageVulnerability");
  return CPatterned::DamageVulnerability();
}

// vtable slot 41 = fn_10_F488 (0x138): the damage handling (the interrupt threshold at 0x908).
void CDarkSamus::TakeDamage(const CVector3f& direction, float magnitude) {
  StandIn("fn_10_F488", "vtable slot 41, TakeDamage");
  CPatterned::TakeDamage(direction, magnitude);
}

// vtable slot 45 = fn_10_CEB4 (0x1C): it copies the three floats out of this+0x54/0x58/0x5C, the
// actor's translation.
CVector3f CDarkSamus::GetOrigin(const CStateManager& mgr, const CTeamAiRole& role,
                                const CVector3f& point) const {
  return GetTranslation();
}

// vtable slot 55 = fn_10_1C6C8 (0xB0): the projectile info the attacks launch.
CProjectileInfo* CDarkSamus::ProjectileInfo() {
  StandIn("fn_10_1C6C8", "vtable slot 55, ProjectileInfo");
  return CPatterned::ProjectileInfo();
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
