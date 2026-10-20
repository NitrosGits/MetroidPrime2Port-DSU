// The REL's CIng (module 29, Ing.rel), ported for behaviour rather than bytes. The state machine is
// fn_29_B208's: 49 triggers, 29 states and 12 code functions, all registered under the REL's own
// names. Functions checked against the retail code carry `// @ fn_29_<offset>`; the remaining
// minimal bodies are marked [?]. The analysis is docs/module-analysis/IngFamily.md, section A (A.5
// for this file).

#include "MetroidPrime/Enemies/CIng.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Particles/CGenDescription.hpp"
#include "Kyoto/Text/CStringTable.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CAxisAngle.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CEchoEmitter.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CKnockBackInfo.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/Weapons/CWeapon.hpp"
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Enemies/CGenericFSM2.hpp"
#include "MetroidPrime/Enemies/CIngExitHostEffect.hpp"
#include "MetroidPrime/Enemies/CIngMiniPortalAttack.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Enemies/CTeamAiRole.hpp"
#include "MetroidPrime/Enemies/PortIngShared.hpp"
#include "MetroidPrime/HUD/CHUDMemoParms.hpp"
#include "MetroidPrime/HUD/CSamusHud.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/Player/CEnvironmentVariable.hpp"
#include "MetroidPrime/Player/CGameState.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrIng.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CHUDBillboardEffect.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIHint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef CPatterned::StateMachine::TriggerFunc TriggerFunc;
typedef CPatterned::StateMachine::StateFunc StateFunc;
typedef CPatterned::StateMachine::CodeFunc CodeFunc;

#define ING_TRIGGER(name) {#name, static_cast< TriggerFunc >(&CIng::name)}
#define ING_STATE(name) {#name, static_cast< StateFunc >(&CIng::name)}
#define ING_CODE(name) {#name, static_cast< CodeFunc >(&CIng::name)}

// fn_29_B208's three tables, in the REL's order; the names are its own strings.
static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    ING_TRIGGER(StateOver),          ING_TRIGGER(IsAlert),
    ING_TRIGGER(IsControllingHost),  ING_TRIGGER(IsFacingTarget),
    ING_TRIGGER(HasTarget),          ING_TRIGGER(HasNewTarget),
    ING_TRIGGER(HasLineOfSight),     ING_TRIGGER(UnderFire),
    ING_TRIGGER(HeardShot),          ING_TRIGGER(AreaClear),
    ING_TRIGGER(CoverLeash),         ING_TRIGGER(HasCoverPoint),
    ING_TRIGGER(HasWallCoverPoint),  ING_TRIGGER(HasMiniPortals),
    ING_TRIGGER(IsCorporeal),        ING_TRIGGER(IsIngSpot),
    ING_TRIGGER(IsBodyProjectile),   ING_TRIGGER(IsFrustrated),
    ING_TRIGGER(IsAggressive),       ING_TRIGGER(IsLuredBySafeZone),
    ING_TRIGGER(StillLuredBySafeZone), ING_TRIGGER(UseProjectileFSMEntry),
    ING_TRIGGER(TargetInSafeZone),   ING_TRIGGER(TargetInLightSafeZone),
    ING_TRIGGER(EmergePtInSafeZone), ING_TRIGGER(CanChangeForm),
    ING_TRIGGER(ShouldEvaporate),    ING_TRIGGER(ShouldFleeSafeZone),
    ING_TRIGGER(ShouldBecomeCorporeal), ING_TRIGGER(ShouldBecomeIngSpot),
    ING_TRIGGER(ShouldArmSwipe),     ING_TRIGGER(ShouldTaunt),
    ING_TRIGGER(ShouldBodyProjectile), ING_TRIGGER(ShouldOpenMiniPortal),
    ING_TRIGGER(ProjectileSplat),    ING_TRIGGER(ProjectileHitTarget),
    ING_TRIGGER(FoundMovementPos),   ING_TRIGGER(PathShagged),
    ING_TRIGGER(PathOver),           ING_TRIGGER(PointPathOver),
    ING_TRIGGER(FoundPointPath),     ING_TRIGGER(IsOffPath),
    ING_TRIGGER(HasAttackPattern),   ING_TRIGGER(AttackPatternOver),
    ING_TRIGGER(TargetIsBall),       ING_TRIGGER(InBallPursuitRange),
    ING_TRIGGER(HasPathToTarget),    ING_TRIGGER(InGrappleRange),
    ING_TRIGGER(CanGrappleTarget),
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    ING_STATE(Start),
    ING_STATE(WaitForFSMTransition),
    ING_STATE(ExitHost),
    ING_STATE(BecomeCorporeal),
    ING_STATE(BecomeIngSpot),
    ING_STATE(BecomeBodyProjectile),
    ING_STATE(BecomeWallProjectile),
    ING_STATE(Alert),
    ING_STATE(SafeZoneReaction),
    ING_STATE(Taunt),
    ING_STATE(Lurk),
    ING_STATE(PathFind),
    ING_STATE(IngSpotPathFind),
    ING_STATE(IngSpotPointPathFind),
    ING_STATE(Patrol),
    ING_STATE(FollowAttackPattern),
    ING_STATE(BodyProjectileFlight),
    ING_STATE(SuckEnergy),
    ING_STATE(SeekWallPoint),
    ING_STATE(SelectTarget),
    ING_STATE(FaceTarget),
    ING_STATE(FindCoverPoint),
    ING_STATE(ArmSwipe),
    ING_STATE(FailSafeMode),
    ING_STATE(GrappleBall),
    ING_STATE(FindMiniPortals),
    ING_STATE(MiniPortalAttack),
    ING_STATE(Dead),
    ING_STATE(Evaporate),
};

static CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    ING_CODE(SetTargetDest),     ING_CODE(SetCoverDest),      ING_CODE(SetRecoverDest),
    ING_CODE(SetPointCoverDest), ING_CODE(SetWallPointCoverDest), ING_CODE(SetExitHostDest),
    ING_CODE(SetLuredDest),      ING_CODE(SetLuredPointDest), ING_CODE(StartRangedAttack),
    ING_CODE(EndRangedAttack),   ING_CODE(SplatOntoMesh),     ING_CODE(ExpelGrappledBall),
};

#undef ING_TRIGGER
#undef ING_STATE
#undef ING_CODE

// Reports a stand-in the first time it runs, so a run's log lists the retail functions the host
// still substitutes. The name is the retail one and the number its size in the REL.
void PortIngStandIn(const char* what) {
  static const char* sSeen[64] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (strcmp(sSeen[i], what) == 0) {
      return;
    }
  }
  if (sCount < 64) {
    sSeen[sCount++] = what;
  }
  fprintf(stderr, "[ing] stand-in %s (see PortCIng.cpp)\n", what);
}

namespace {

// The forms (mForm, retail 0x9d4). 0 is the host form SuckEnergy sets (the Ing possesses the player
// and has no body of its own), 1 is the exit host form ExitHost's Activate sets, 2 corporeal, 3 ing
// spot, 4 the flying body projectile, 5 evaporating; 6/7/8/9 are the four form changes' own forms.
enum {
  kForm_Host = 0,
  kForm_ControllingHost = 1, // the second host form, ExitHost's Activate; nothing else reads it
  kForm_Corporeal = 2,
  kForm_IngSpot = 3,
  kForm_BodyProjectile = 4,
  kForm_Evaporate = 5,
  kForm_BecomingCorporeal = 6, // BecomeCorporeal's Activate, when it leaves the ing-spot form
  kForm_BecomingIngSpot = 7,   // BecomeIngSpot's Activate, when it leaves the corporeal form
  kForm_BecomingBodyProjectile = 8, // BecomeBodyProjectile's Activate
  kForm_BecomingWallProjectile = 9, // BecomeWallProjectile's Activate
};

// fn_29_BF80 (asm 0xBF8C-0xBFE0) and fn_29_C010 (asm 0xC024-0xC07C) gate the draw on the form the
// same way: the forms 2, 4 and 6..9 draw always, the forms 3 and 5 only while the body-state-5
// timer (0xe40) is below rodata_C8 (2.5f), and the forms 0, 1 and 10 and up never draw.
bool IngFormDraws(int form, float bodyState5Time) {
  if (form == kForm_Corporeal || form == kForm_BodyProjectile) {
    return true;
  }
  if (form >= kForm_BecomingCorporeal && form <= kForm_BecomingWallProjectile) {
    return true;
  }
  if (form == kForm_IngSpot || form == kForm_Evaporate) {
    return bodyState5Time < 2.5f; // lbl_29_rodata_C8
  }
  return false;
}

// mData.mFlags210 (SLdrIng.ingFlagsIng): the retail's flag word. The byte the REL reads at the
// actor's 0x9d0 is the word's most significant byte, so the byte mask m is the word mask
// (m << 24) - the retail's flag byte masks 0x01..0x80 (fn_29_EE68's eight rlwimi's at asm
// 0x0000EF4C-0x0000EFA8 put the ingFlags' bits 0..7 into the byte's bits 7..0).
enum {
  kFlag_AlwaysTargetable = 0x01000000,
  kFlag_NoPathFind = 0x02000000,
  kFlag_NoFormChange = 0x04000000,
  kFlag_NoGrapple = 0x08000000,
  kFlag_NoMiniPortal = 0x10000000,
  kFlag_NoBodyProjectile = 0x20000000,
  kFlag_NoArmSwipe = 0x40000000,
  kFlag_StartAsIngSpot = 0x80000000,
};

const float kFacingAngle = 0.349f; // rodata: 20 degrees
// rodata_104: the wall projectile's launch direction is clamped to this angle from the surface normal.
const float kWallLaunchAngle = 0.7853981f; // 45 degrees

// fn_29_3190's seven joint spheres: Ing.rel's own rodata_0 table, seven (locator, radius) pairs.
// Every radius is 1.5 (0x3FC00000); the pivot point is the zero vector and the mass is 1000.0
// (rodata_C4). The names are the Ing model's locators.
struct SIngJointSphere {
  const char* mLocator;
  float mRadius;
};
const SIngJointSphere skIngJointSpheres[] = {
    {"head", 1.5f},   {"Pelvis_SDK", 1.5f}, {"L_knee", 1.5f}, {"R_knee", 1.5f},
    {"F_elbow", 1.5f}, {"L_elbow", 1.5f},   {"R_elbow", 1.5f},
};
const float kIngJointMass = 1000.f; // rodata_C4

bool DebugIng() {
  static int sEnabled = -1;
  if (sEnabled < 0) {
    const char* env = getenv("MP2_DEBUG_ING");
    sEnabled = env != nullptr && env[0] != '\0' && env[0] != '0' ? 1 : 0;
  }
  return sEnabled != 0;
}

// Per-trigger debug throttle: one line in sixty (about a second), keyed by the caller's own
// counter. The body-projectile and grapple triggers are evaluated every frame while their state
// is active, so a raw print would bury the log.
bool IngLogTick(int& counter) { return DebugIng() && (counter++ % 60) == 0; }

CMaterialFilter SolidFilter() {
  return CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList());
}

// @ fn_29_3D74: the box between two points, three units taller at each end (retail's double 3.0).
CAABox PortSegmentBox(const CVector3f& a, const CVector3f& b) {
  return CAABox(
      CVector3f(CMath::Min(a.GetX(), b.GetX()), CMath::Min(a.GetY(), b.GetY()),
                CMath::Min(a.GetZ(), b.GetZ()) - 3.f),
      CVector3f(CMath::Max(a.GetX(), b.GetX()), CMath::Max(a.GetY(), b.GetY()),
                CMath::Max(a.GetZ(), b.GetZ()) + 3.f));
}

// @ fn_29_421C: which arm swings; true is the left (0xd80..), false the right (0xd7c..).
bool PortPickSwipeArm(CStateManager& mgr) {
  return mgr.Random()->Range(0.f, 100.f) >= 50.f;
}

// fn_29_61B4's search ring (rodata_44): eight angles in degrees, 180 first, taken in order.
const float skMiniPortalAngles[] = {180.f, 0.f, 135.f, 45.f, -135.f, -45.f, 90.f, -90.f};

// Retail keeps the next table index in the REL's bss (lbl_29_bss_64), not in the actor, so it is
// shared by every Ing and survives across state entries; the port does the same on purpose.
int sMiniPortalAngleIndex = 0;

const char* Fsm2NodeName(const CState2* node) {
  if (node == nullptr) {
    return "?";
  }
  switch (node->GetType()) {
  case CState2::kType_State:
    return static_cast< const CState2State* >(node)->GetName().data();
  case CState2::kType_Code:
    return static_cast< const CState2Code* >(node)->GetName().data();
  case CState2::kType_Trigger:
    return static_cast< const CState2Trigger* >(node)->GetName().data();
  case CState2::kType_SubMachine:
    return static_cast< const CState2SubMachine* >(node)->GetName().data();
  }
  return "?";
}

template < class Node >
void DumpFsm2Nodes(const char* kind, const rstl::vector< Node >& nodes) {
  for (int i = 0; i < nodes.size(); ++i) {
    const rstl::vector< SStateMachine2Transition >& out = nodes[i].GetTransitions();
    fprintf(stderr, "[ing] fsm %s %s:", kind, nodes[i].GetName().data());
    for (int t = 0; t < out.size(); ++t) {
      fprintf(stderr, " %s->%s", out[t].mName.data(), Fsm2NodeName(out[t].mTarget));
    }
    fprintf(stderr, "\n");
  }
}

void DumpFsm2(const CGenericFSM2& machine) {
  DumpFsm2Nodes("state", machine.GetStates());
  DumpFsm2Nodes("code", machine.GetCodes());
  const rstl::vector< CState2Trigger >& triggers = machine.GetTriggers();
  for (int i = 0; i < triggers.size(); ++i) {
    const rstl::vector< SStateMachine2Transition >& out = triggers[i].GetTransitions();
    fprintf(stderr, "[ing] fsm trigger %s%s(%g):", triggers[i].IsNegated() ? "!" : "",
            triggers[i].GetName().data(), triggers[i].GetArgument().GetFloat());
    for (int t = 0; t < out.size(); ++t) {
      fprintf(stderr, " %s->%s", out[t].mName.data(), Fsm2NodeName(out[t].mTarget));
    }
    fprintf(stderr, "\n");
  }
  // The sub-machines carry their own states, codes and triggers (the body projectile's and the
  // ing spot's); retail loads each as its own FSM2 resource, so the transition into one of them is
  // only visible once its contents are dumped too.
  const rstl::vector< CState2SubMachine >& subMachines = machine.GetSubMachines();
  for (int i = 0; i < subMachines.size(); ++i) {
    fprintf(stderr, "[ing] fsm submachine %s\n", subMachines[i].GetName().data());
    const CGenericFSM2* sub = subMachines[i].GetMachine();
    if (sub != nullptr) {
      DumpFsm2(*sub);
    }
  }
}

rstl::optional_object< TLockedToken< CGenDescription > > OptionalPart(CAssetId id) {
  if (id == kInvalidAssetId) {
    return rstl::optional_object< TLockedToken< CGenDescription > >();
  }
  return rstl::optional_object< TLockedToken< CGenDescription > >(
      TLockedToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', id))));
}

// The ctor's "head" locator (retail 0xd7b, the ctor's first GetLocatorSegId; fn_29_D540 reads
// rodata 0x64e through the locator name table). It is the line-of-sight tracker's segment (0xd8a)
// and BecomeCorporeal's Activate restores it.
CSegId PortHeadSegId(const CModelData& model) {
  const CAnimData* anim = model.GetAnimationData();
  return anim != nullptr ? anim->GetLocatorSegId(rstl::string_l("head")) : CSegId();
}

// The ctor's arm locators (fn_29_D540 @0x0000E4EC..0x0000E630), same lookup as the head's.
CSegId PortArmSegId(const CModelData& model, const char* name) {
  const CAnimData* anim = model.GetAnimationData();
  return anim != nullptr ? anim->GetLocatorSegId(rstl::string_l(name)) : CSegId();
}

} // namespace

CIng::CIng(TUniqueId uid, const rstl::string& name, const CEntityInfo& info, const CTransform4f& xf,
           const CModelData& modelData, const CActorParameters& actParms,
           const CPatternedInfo& pInfo, const CIngData& data)
// EPatternedAI 0x18, flavor 0, movement 0 (ground), collider 1, body 1 (bipedal): the retail
// constructor's arguments to CPatterned (fn_29_D540, which passes r4 = 0x18 and 0/1/1 on the stack).
: CPatterned(static_cast< EPatternedAI >(0x18), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mData(data)
// fn_29_D540 @0x0000D5F4: (null area, flags 0x301, the patterned info's pathfinding index at
// +0xec, 1.0f, 1.0f, no avoidance filter, kRP_Center). rodata_64, which the ctor loads for both
// radii, is 1.0f.
, mPathFindSearch(nullptr, 0x301, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0, CPFRegion::kRP_Center)
, mPointPathFindSearch(nullptr) // fn_29_D540 @0x0000D600, the DOL's fn_801F8818
, mSurfaceAlignment()           // fn_29_D540 @0x0000D610, the DOL's fn_801F7CA0
, mIngSpotPathFindNavigation()  // fn_29_D540 @0x0000D618, the DOL's fn_801F8258
, mForm(kForm_Corporeal)
, mDesiredForm(-1)
, mTouchBox()
, mHostId(kInvalidUniqueId)
, mPossessedEffectId(kInvalidUniqueId)                            // 0xcde
, mSwarmEffect(OptionalPart(data.mSwarm.mPART))                   // 0xce0, CIngData+0x1e4
, mSwarmEffect2(OptionalPart(data.mSwarm.mPART081E))              // 0xcf0, CIngData+0x1f4
, mBlobId(kInvalidUniqueId)
, mTeamId(kInvalidUniqueId)
, mTargetId(kInvalidUniqueId)
, mPrevTargetId(kInvalidUniqueId)
, mAttackPathResumeId(kInvalidUniqueId)
, mCoverHintId(kInvalidUniqueId)
, mLastCoverHintId(kInvalidUniqueId)
, mD76Id(kInvalidUniqueId)
, mLureSafeZoneId(kInvalidUniqueId)
, mNormalHitEffect(OptionalPart(data.mIngSpot.GetNormalHitEffect())) // 0xd08
, mHeavyHitEffect(OptionalPart(data.mIngSpot.GetHeavyHitEffect()))   // 0xd18
, mDeathEffect(OptionalPart(data.mIngSpot.GetDeathEffect()))         // 0xd28
, mMiniPortalEffect(OptionalPart(data.mMiniPortal.mEffect))           // 0xd38
, mSplatEffect(OptionalPart(data.mBodyProjectile.mSplatEffect))       // 0xd48, CIngData+0x8c
// fn_29_D540 @0x0000E110: the tracker gets the "head" locator (0xd7b) and rodata 0x118/0x11c,
// 0.1f and 0.05f.
, mEyeSegId(PortHeadSegId(modelData))
, mCollarSegId(CSegId::Invalid()) // 0xd7a: the ctor writes 0xff and nothing else does
// The ctor's arm locators: R_shoulder/R_elbow/R_forearm/R_wrist_end, then the L_ set. The pseudo-C
// puts the shoulders at CPatterned's 0x35f/0x360; the asm stores them here (0x0000E47C/0x0000E54C).
, mRightShoulderSegId(PortArmSegId(modelData, "R_shoulder")) // 0xd7c
, mRightElbowSegId(PortArmSegId(modelData, "R_elbow"))       // 0xd7d
, mRightForearmSegId(PortArmSegId(modelData, "R_forearm"))   // 0xd7e
, mRightWristSegId(PortArmSegId(modelData, "R_wrist_end"))   // 0xd7f
, mLeftShoulderSegId(PortArmSegId(modelData, "L_shoulder"))  // 0xd80
, mLeftElbowSegId(PortArmSegId(modelData, "L_elbow"))        // 0xd81
, mLeftForearmSegId(PortArmSegId(modelData, "L_forearm"))    // 0xd82
, mLeftWristSegId(PortArmSegId(modelData, "L_wrist_end"))    // 0xd83
, mLineOfSight(uid, mEyeSegId, 0.1f, 0.05f)
, mTargetPos(CVector3f::Zero())
, mSurfaceNormal(CVector3f::Zero())
, mSurfaceMoveDir(CVector3f::Zero())
, mPortalPlaneNormal(CVector3f::Zero())
, mPortalPlaneConstant(0.f)
, mSwipeArm(-1)
, mModelScale(modelData.GetScale())
, mMiniPortalCount(0)
, mMiniPortalPositions()
, mMiniPortalIndex(-1)
, mSafeZoneCount(0)
, mHeardShotTime(1.5f)
, mUnderFireTime(0.75f)
, mArmSwipeTime(1.f)
, mFormTime(0.f)
, mBodyState5Time(0.f)
, mFrustrationTimer(0.f)
, mBallCooldown(0.f)
, mHitReactionTime(0.f)
, mLightBlend(0.f)
, mDeadFade(0.f)
, mBodyProjectileTime(0.f)
, mEvaporate(false)
, mTaunt(false)
, mBodyProjectileAllowed(false)
, mAggressive(false)
, mFoundMovementPos(false)
, mRecalcPath(false)
, mRenderModel(true)
, mPortalPlane(false)
, mStartAsIngSpot((data.mFlags210 & kFlag_StartAsIngSpot) != 0)
, mForm9Render(false)
, mFollowingAttackPath(false)
, mAlert(false)
, mMoving(false)
, mGrappled(false)
, mProjectileFSMEntry(false)
, mSplat(false)
, mSafeZoneIngSpot(false)
, mHurtfulZone(false)
, mLastLoggedState(nullptr)
, mStateTime(0.f)
, mPathArea(nullptr)
, mDebugPoll(0) {
  // fn_29_E7C8 (0x110) builds the data block's sub-structs inside the constructor: the host takes
  // the loader's finished block (PortLoadIng below) and its own member constructors are that
  // step.
  // The retail constructor turns knock-back physics off on CPatterned's own manager (@0x0000E3DC)
  // and stops the actor drawing a shadow (@0x0000E634).
  KnockBackController().EnableKnockBackPhysics(false);
  SetDrawShadow(false);
  // [?] MinorIng's surface helper mode and search radii (fn_44_9D58); the same DOL helpers.
  mSurfaceAlignment.SetMode(static_cast< CSurfaceAlignmentHelper::EMode >(1));
  mPathFindSearch.SetCharacterHeight(mModelScale.GetY() * 3.f);
  mPathFindSearch.SetCharacterRadius(mModelScale.GetZ() * 5.f);
  mLineOfSight.SetRayFilter(CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid), CMaterialList(kMT_Player, kMT_CollisionActor)));
}

// fn_29_CF64 (0x1C4): the retail's destructor frees the data block (fn_29_10F8, 0x94) and the
// exit-host effect; the host's own member destructors are that step.
CIng::~CIng() {}

// fn_29_3190: the Ing's collision actors, one 1.5-radius sphere on each of seven joints, with the
// Ing's own health and vulnerability (fn_29_3014), the Unknown54 material and a Solid filter that
// excludes collision actors, AI passthrough and the player. The manager starts inactive; the form
// changes and the script messages set it active. The joint vector is the retail's fn_29_33CC
// (0x38, asm 0x33CC: the count at +4 goes up before the element at data + count * 0x68 is built
// by fn_29_3404 (0x20) and fn_29_3424 (0x28)); the host's vector push is the same step.
void CIng::CreateCollisionActorManager(CStateManager& mgr) {
  // The joint vector is the retail's own: fn_29_33CC (0x38) pushes an element, fn_29_344C (0xC0)
  // is the element's copy constructor (the two words, the two bytes, the eight floats, the locator
  // string, the halfword, the float and the matrix), fn_29_F1A0 (0xC0) grows the array and
  // fn_29_3568 (0x84) destroys it. The host's rstl::vector of CJointCollisionDescription is the
  // same structure, so the push below is that step.
  rstl::vector< CJointCollisionDescription > joints;
  joints.reserve(7);
  const CAnimData* anim = GetAnimationData();
  for (uint i = 0; i < 7; ++i) {
    const SIngJointSphere& sphere = skIngJointSpheres[i];
    const rstl::string name = rstl::string_l(sphere.mLocator);
    const CSegId seg = anim != nullptr ? anim->GetLocatorSegId(name) : CSegId();
    joints.push_back_unsafe(CJointCollisionDescription::SphereCollision(
        seg, CVector3f::Zero(), sphere.mRadius, name, kIngJointMass));
  }
  // The ctor's last argument is false (the new manager is inactive).
  mCollisionActorManager =
      rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), joints, false);
  SetupCollisionActors(mgr);       // fn_29_3014
  AddMaterial(kMT_Unknown54, mgr); // 0x36
  // data_A14 = Solid; data_A18|data_A1C|data_A20 = CollisionActor|AIPassthrough|Player.
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid),
      CMaterialList(kMT_CollisionActor, kMT_AIPassthrough, kMT_Player)));
}

// fn_29_3014: every collision actor takes the Ing's health info and CPatterned's own damage
// vulnerability. The health copy itself is the retail's fn_29_313C (0x54, asm 0x313C): the three
// floats, the word and the two halfwords of the health, then the two more halfwords and the byte -
// the host's `*actor->HealthInfo() = *HealthInfo()` is the same copy. Retail calls `GetDamageVulnerability__10CPatternedCFv` directly (not through the
// vtable), so the colliders get the base table (CAi's normal one) rather than CIng's form-dependent
// override - with the override the colliders inherit the trigger table, whose beams are
// kE_PassThrough, and no projectile can reach a corporeal Ing at all.
void CIng::SetupCollisionActors(CStateManager& mgr) {
  if (mCollisionActorManager.null()) {
    return;
  }
  const uint count = mCollisionActorManager->GetNumCollisionActors();
  for (uint i = 0; i < count; ++i) {
    const TUniqueId id =
        mCollisionActorManager->GetCollisionDescFromIndex(i).GetCollisionActorId();
    CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(id));
    if (actor == nullptr) {
      continue;
    }
    actor->SetDamageVulnerability(*CPatterned::GetDamageVulnerability());
    *actor->HealthInfo() = *HealthInfo();
  }
}

// fn_29_2AE0 (0x90): the retail's default damage info. It clears the four floats (the damage, the
// radius damage, the radius and the knock-back), the three sound ids and the two flag bits (the
// no-immunity bit 7 and the apply-radius-damage bit 6 of the byte at 0x1a), and sets the weapon
// mode's word to 0xffff0000 - the type 0xffff (none) in the high half and the low 13 bits, which
// the retail reads from an uninitialised stack word (asm 0x2AF0 loads 0x8(r1), the callee's own
// outgoing parameter save area), so the host leaves them zero.
// Skipped: the host's CDamageInfo has no setter for the three sound ids, so they stay 0xffff.
static CDamageInfo IngDefaultDamageInfo() {
  return CDamageInfo(CWeaponMode(kWT_None), 0.f, 0.f, 0.f, false, false);
}

// @ fn_29_2ED8 (kSM_HitObject): when a collision actor that last touched the player reports a hit
// and the contact damage is off cooldown, the player takes the Ing's contact damage.
void CIng::CollisionActorHitPlayer(CStateManager& mgr, TUniqueId sender) {
  const CCollisionActor* actor = TCastToConstPtr< CCollisionActor >(mgr.ObjectById(sender));
  if (actor == nullptr) {
    return;
  }
  const CPlayer* player = mgr.GetPlayer(0);
  if (player == nullptr || actor->GetLastTouchedObject() != player->GetUniqueId()) {
    return;
  }
  // xE5C_1 (0xe5c bit 1) suppresses the contact damage; fn_29_6A88 owns that bit.
  if (mCurDamageRemTime > 0.f || xE5C_1) {
    return;
  }
  mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(), GetContactDamage(),
                  SolidFilter(), CVector3f::Zero());
  mCurDamageRemTime = mDamageWaitTime;
}

// @ fn_29_2414: the ing-spot hit effect explosion, from the heavy token (the retail's
// fn_29_2098 (0x58) allocates it and fn_29_20F0 (0x54) frees it; the host's rs_new does both)
// when it is loaded and asked for, else the normal one. The caller adds it to the state manager.
CEntity* CIng::SpawnHitEffect(CStateManager& mgr, bool heavy) {
  const rstl::optional_object< TLockedToken< CGenDescription > >& effect =
      heavy ? mHeavyHitEffect : mNormalHitEffect;
  if (!effect) {
    if (DebugIng()) {
      fprintf(stderr, "[ing] %04x hit fx: the %s token is not loaded\n", GetUniqueId().Value(),
              heavy ? "heavy" : "normal");
    }
    return nullptr;
  }
  if (DebugIng()) {
    fprintf(stderr, "[ing] %04x hit fx: %s explosion\n", GetUniqueId().Value(),
            heavy ? "heavy" : "normal");
  }
  return rs_new CExplosion(*effect, mgr.AllocateUniqueId(),
                           CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true,
                                       kInvalidEditorId),
                           rstl::string_l("Ing Damage Fx"), GetTransform(), 0,
                           GetModelData()->GetScale(), CColor::White(), -1);
}

// @ fn_29_2B70: a shot on the ing-spot form. The hit resets the reaction timer and flags the spot,
// then the weapon picks the light or heavy effect (25 damage on the ing-spot vulnerability) and
// plays that hit's sound.
void CIng::IngSpotDamage(CStateManager& mgr, TUniqueId sender) {
  mHitReactionTime = -1.f; // 0xe4c
  mSafeZoneIngSpot = true; // 0xe5e bit 5
  const CWeapon* weapon = TCastToConstPtr< CWeapon >(mgr.GetObjectById(sender));
  if (weapon == nullptr) {
    return;
  }
  const bool heavy =
      !(weapon->GetCurrentDamageInfo().GetDamage(mData.mIngSpot.GetVulnerability()) < 25.f);
  if (DebugIng()) {
    // What a run cannot otherwise see: the spot form has no health of its own, so a landed shot
    // shows only as this line plus the effect it spawns.
    fprintf(stderr, "[ing] %04x spot hit heavy %d\n", GetUniqueId().Value(), heavy ? 1 : 0);
  }
  CEntity* explosion = SpawnHitEffect(mgr, heavy);
  CSfxManager::AddEmitter(heavy ? mData.mIngSpot.GetHeavyHitSound() : mData.mIngSpot.GetNormalHitSound(),
                          GetTranslation(), 0x7f, GetCurrentAreaId().Value(), true, false,
                          CSfxManager::kMedPriority);
  if (explosion != nullptr) {
    mgr.AddObject(explosion);
  }
}

// @ fn_29_2684: a weapon (or the player) hit a collision actor. The actor carries a copy of the Ing's
// initial health, so what it lost is the hit: the Ing's health drops by that much, it takes the hit
// (fn_29_B890 only records the hit time), dies below zero, and is knocked back along the shot. The
// actor's health is then re-armed.
void CIng::CollisionActorDamage(CStateManager& mgr, TUniqueId sender) {
  if (!mAlive) { // CPatterned 0x420 bit 6
    return;
  }
  CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(sender));
  if (actor == nullptr) {
    return;
  }
  const TUniqueId touched = actor->GetLastTouchedObject();
  CHealthInfo* actorHealth = actor->HealthInfo();
  CHealthInfo* health = HealthInfo();
  if (actorHealth == nullptr || health == nullptr) {
    return;
  }
  const float initial = health->GetInitialHP();
  const float hp = health->GetHP();
  const float amount = initial - actorHealth->GetHP();
  health->SetHP(hp - amount);

  // The hit comes from the weapon the actor last touched, or from the player (morph ball).
  CVector3f direction = CVector3f::Zero();
  TUniqueId attacker = kInvalidUniqueId;
  CDamageInfo damage = IngDefaultDamageInfo(); // fn_29_2AE0 (asm 0x2744)
  if (const CWeapon* weapon = TCastToConstPtr< CWeapon >(mgr.GetObjectById(touched))) {
    direction = weapon->GetTransform().GetForward();
    damage = weapon->GetCurrentDamageInfo();
    attacker = weapon->GetOwnerId();
  } else if (const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(touched))) {
    direction = GetTranslation() - player->GetTranslation();
    const CMorphBall* ball = player->GetMorphBall();
    if (ball != nullptr && ball->InScrewAttackMode()) {
      damage = CDamageInfo(CWeaponMode(kWT_ScrewAttack), amount, 0.f, 0.f, false, false);
    } else if (ball != nullptr && ball->IsBoosting()) {
      damage = CDamageInfo(CWeaponMode(kWT_BoostBall), amount, 0.f, 0.f, false, false);
    }
  }

  TakeDamage(direction, amount); // 0xa4 -> fn_29_B890
  if (hp <= amount) {
    Death(mgr, direction, kSS_DeathRattle); // 0x98 -> fn_29_B3C4, 0x5241544C
    // Retail's UpdateHealthCause(actor, touched, damage, true, false).
    mgr.RecordDamageSource(*this, touched, damage, true, false);
  }
  KnockBack(mgr, CKnockBackInfo(direction, touched, attacker, damage, true)); // 0x9c
  actorHealth->SetHP(initial);
}

// A body state is only ever installed by CBodyController::Activate; retail activates it on
// kSM_Create (fn_29_4C88). Kept as a guard for the entry points that can run first on the host.
void CIng::EnsureBodyActive(CStateManager& mgr) {
  if (BodyController() != nullptr && !BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

void CIng::LogState(const char* name) {
  mStateTime = 0.f;
  if (DebugIng() && mLastLoggedState != name) {
    mLastLoggedState = name;
    fprintf(stderr, "[ing] %04x state %s form %d\n", GetUniqueId().Value(), name, mForm);
  }
}

// ---- helpers --------------------------------------------------------------------------------

// fn_29_372C (its only caller is SetTargetDest, fn_29_5B60)
void CIng::JoinTeam(CStateManager& mgr) {
  if (mTeamId == kInvalidUniqueId) {
    mTeamId = CScriptTeamAiMgr::GetAssociatedTeamId(*this, mgr);
  }
  if (mTeamId != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamId))) {
      // [?] the roles are MinorIng's (fn_44_593C).
      team->JoinTeam(*this, CTeamAiRole::kTAR_Projectile, CTeamAiRole::kTAR_Melee,
                     CTeamAiRole::kTAR_Invalid);
    }
  }
}

// The DOL's surface move shared with MinorIng (fn_44_5384): drop the part of the move along the
// surface normal, step, and keep the step on the path mesh unless following a point path.
void CIng::MoveOnSurface(const CVector3f& moveIn, float speed, float dt) {
  const CVector3f pos = GetTranslation();
  const CVector3f normal = mSurfaceAlignment.GetSurface().GetNormal();
  CVector3f move = moveIn;
  const float along = CVector3f::Dot(move, normal);
  if (along * along < move.MagSquared() * 0.6f) {
    move = move - normal * along;
  }
  if (!move.IsMagnitudeSafe()) {
    mSurfaceMoveDir = CVector3f::Zero();
  } else {
    const CVector3f dir = move.AsNormalized();
    SetTranslation(pos + dir * (speed * dt));
    mSurfaceMoveDir = dir;
  }
  Stop();
}

void CIng::ApplySeparation(CStateManager& mgr) {
  CVector3f total = CVector3f::Zero();
  const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CPatterned* other = TCastToConstPtr< CPatterned >(list[i]);
    if (other != nullptr && other != this && other->GetCurrentAreaId() == GetCurrentAreaId()) {
      const CVector2f separation = mSteeringBehaviors.Separation2D(
          *this, CVector2f(other->GetTranslation().GetX(), other->GetTranslation().GetY()),
          mModelScale.GetX() * 15.f);
      const CVector3f separation3(separation.GetX(), separation.GetY(), 0.f);
      if (separation3.IsMagnitudeSafe()) {
        total += separation3;
      }
    }
  }
  if (total != CVector3f::Zero()) {
    BodyController()->CommandMgr().DeliverCmd(CBCLocomotionCmd(total, CVector3f::Zero(), 0.5f));
  }
}

float CIng::DistSqToTarget(const CStateManager& mgr) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return FLT_MAX;
  }
  return (target->GetTranslation() - GetTranslation()).MagSquared();
}

CScriptAIHint* CIng::GetCoverHint(CStateManager& mgr, TUniqueId id) const {
  if (id == kInvalidUniqueId) {
    return nullptr;
  }
  return TCastToPtr< CScriptAIHint >(mgr.ObjectById(id));
}

// fn_29_4560 (retail always passes 0xd72, mCoverHintId).
void CIng::ReleaseCoverHint(CStateManager& mgr, TUniqueId& id) {
  if (CScriptAIHint* hint = GetCoverHint(mgr, id)) {
    hint->SetInUse(false);
    hint->SetTimeRemaining(0.f);
  }
  id = kInvalidUniqueId;
}

// fn_29_4510: the picked hint goes in use and becomes both the current and the last cover hint.
void CIng::UseCoverHint(CScriptAIHint& hint) {
  hint.SetInUse(true);                    // asm 0x00004534
  mCoverHintId = hint.GetUniqueId();      // asm 0x0000453C: 0xd72
  mLastCoverHintId = mCoverHintId;        // asm 0x00004544: 0xd74
}

void CIng::UpdateTimers(float dt, CStateManager& mgr) {
  mHeardShotTime += dt;
  if (GetHitByPlayerProjectile()) {
    mUnderFireTime = 0.f;
    SetHitByPlayerProjectile(false);
  } else {
    mUnderFireTime += dt;
  }
  mArmSwipeTime += dt;
  mFormTime += dt;
  mStateTime += dt;
  if (mForm == kForm_Corporeal && PathShagged(mgr, CTriggerData(0.f))) {
    mFrustrationTimer += dt;
  } else {
    mFrustrationTimer = 0.f;
  }
  mBallCooldown -= dt;
  if (mForm == kForm_BodyProjectile) {
    mBodyProjectileTime += dt;
  } else {
    mBodyProjectileTime = 0.f;
  }
  if (mForm == kForm_IngSpot && mCurDamageRemTime > 0.f) {
    mCurDamageRemTime -= dt;
  }
}

// fn_29_426C (CIng::Think's call at asm 0x0000C7A0). The forms: the two host forms (0, 1) are
// never valid targets, the ing-spot and evaporating forms (3, 5) follow the Dark visor and the
// actor's own byte 0x9d0 bit 0, every other form is always a valid target.
void CIng::UpdateValidTarget(CStateManager& mgr) {
  switch (mForm) {
  case kForm_Host:            // 0
  case kForm_ControllingHost: // 1
    SetValidTarget(0, false); // asm 0x000042BC: SetValidTarget(this, 0, 0)
    break;
  case kForm_IngSpot:   // 3
  case kForm_Evaporate: // 5
    // asm 0x000042CC-0x000042EC: the Dark visor (3) forces the flag on - the compare's cntlzw
    // only leaves a non-zero byte for the visor 3 - otherwise the actor's own byte 0x9d0 bit 0
    // decides (asm 0x000042F0-0x00004300). That byte is the loader's data: the data block's
    // +0x210 (CIngData at 0x7c0), the most significant byte of the flag word mData.mFlags210, so
    // the byte mask 0x01 is the word mask 0x01000000 (kFlag_AlwaysTargetable) - the ingFlags'
    // bit 7, since fn_29_EE68 reverses the eight bits into that byte.
    if (mgr.GetPlayerState(0)->GetActiveVisor(mgr) == CPlayerState::kPV_Dark) {
      SetValidTarget(0, true);
    } else {
      SetValidTarget(0, (mData.mFlags210 & kFlag_AlwaysTargetable) != 0);
    }
    break;
  default:
    SetValidTarget(0, true); // asm 0x00004308: SetValidTarget(this, 0, 1)
    break;
  }
}

// fn_29_BCB4: CIng's GetAimPosition - CPhysicsActor's vtable slot 0x54. Retail works out the
// aim position from the form: the corporeal form (2) and the three form-change forms (6..8)
// predict their motion first, and the collar locator (0xd7a) then decides between the locator's
// aim and the base class's. The ctor writes 0xff (invalid) to 0xd7a and nothing else in the
// module ever writes it (asm 0x0000E170/0x0000E448), so retail's locator path is dead and the
// aim is always the base class's - the port keeps the structure anyway. (Grepped over the whole
// module: only the ctor's two stores at asm 0x0000E170/0x0000E448 write 0xd7a, and it is outside
// the loader's data block - CIngData covers 0x7c0..0x9d3.)
CVector3f CIng::GetAimPosition(const CStateManager& mgr, float dt) const {
  CVector3f aim = CVector3f::Zero(); // asm 0x0000BCFC: sZeroVector
  // asm 0x0000BD04-0x0000BD40: the corporeal form and the forms 6..8 predict; the prediction
  // only runs when the argument is above zero (rodata_6C).
  const bool predict = mForm == kForm_Corporeal ||
                       (mForm >= kForm_BecomingCorporeal && mForm <= kForm_BecomingWallProjectile);
  if (!predict) {
    return CPatterned::GetAimPosition(mgr, dt); // asm 0x0000BE4C
  }
  if (dt > 0.f) {
    aim = PredictMotion(dt).GetTranslation(); // asm 0x0000BD50/0x0000BD5C (fn_29_BEB4 copies it)
  }
  // asm 0x0000BD6C: the collar locator (0xd7a); without it retail falls back to the base class.
  if (mCollarSegId == CSegId::Invalid()) {
    return CPatterned::GetAimPosition(mgr, dt); // asm 0x0000BE38
  }
  // asm 0x0000BD90: the locator's transform, composed with the actor's own (0x24); the copy's
  // 0xc/0x1c/0x2c are the translation's three floats.
  const CTransform4f world =
      GetTransform() * GetModelData()->GetLocatorTransform(mCollarSegId); // asm 0x0000BDA0
  const CVector3f offset = world.GetTranslation();
  // asm 0x0000BDCC-0x0000BDE8: the model's scale (the model data's first three floats) times
  // the locator's world position.
  const CVector3f scale = GetModelData()->GetScale();
  aim += CVector3f(scale.GetX() * offset.GetX(), scale.GetY() * offset.GetY(),
                   scale.GetZ() * offset.GetZ());
  // asm 0x0000BDC0/0x0000BDF0: the forms 6 and 7 fall back to the base class when the aim is
  // below the actor's own height (0x5c).
  if ((mForm == kForm_BecomingCorporeal || mForm == kForm_BecomingIngSpot) &&
      aim.GetZ() < GetTranslation().GetZ()) {
    return CPatterned::GetAimPosition(mgr, dt); // asm 0x0000BE14
  }
  return aim;
}

// Which loop plays in which form, and the emitter parameters: the idle loop in the ing-spot form
// while still, the move loop while moving on the surface (fn_29_2270, read 2026-10-20).
void CIng::UpdateLoopSfx() {
  // fn_29_2270 (asm 0x2294-0x23F8): the ing spot's idle sound (0xd60) and move sound (0xd64). They
  // are added at the Ing's position (0x54) with the volume 0x7f and then follow it: the retail
  // updates each emitter with the position and the transform's first column (the right vector,
  // asm 0x22F0-0x2320). Outside the ing-spot form both emitters are removed.
  const bool spot = mForm == kForm_IngSpot;
  // The retail's condition is the moving flag (the bit 1 of the byte at 0xe5d, asm 0x22A0-0x22A8:
  // extrwi. r0, r0, 1, 30), not the move vector.
  const bool moving = spot && mMoving;
  const ushort ids[2] = {mData.mIngSpot.GetIdleSound(), mData.mIngSpot.GetMoveSound()};
  const bool want[2] = {spot && !moving, moving};
  for (int i = 0; i < 2; ++i) {
    if (want[i]) {
      if (!mLoopSfx[i] && ids[i] != CSfxManager::kInternalInvalidSfxId) {
        mLoopSfx[i] = CSfxManager::AddEmitter(ids[i], GetTranslation(), 0x7f,
                                              GetCurrentAreaId().Value(), true, true,
                                              CSfxManager::kMedPriority);
      }
      if (mLoopSfx[i]) {
        CSfxManager::UpdateEmitter(mLoopSfx[i], GetTranslation(), GetTransform().GetRight(), 0x7f);
      }
    } else if (mLoopSfx[i]) {
      CSfxManager::RemoveEmitter(mLoopSfx[i]);
      mLoopSfx[i] = CSfxHandle();
    }
  }
}

void CIng::StopSfx() {
  for (int i = 0; i < 2; ++i) {
    if (mLoopSfx[i]) {
      CSfxManager::SfxStop(mLoopSfx[i]);
      mLoopSfx[i] = CSfxHandle();
    }
  }
}

// fn_29_4604 (CIng::Think's call at asm 0x0000C794): the blob the Ing owns (mBlobId, 0xd5c)
// follows the surface the Ing stands on. Retail looks the blob up, works out an "align" flag from
// the form and the flags, and when it is set turns the blob to the surface normal, writes the
// blob's model scale and colour (0xfc/0x100..0x104), calls GeomBlobV2's fn_25_256C and sets the
// blob's byte 0x154 bit 5.
//
// The Ing never spawns the blob (mBlobId stays invalid - the member map in IngFamily.md A.3), so
// retail's whole body is unreachable in every run so far and the port stops where retail does:
// at the lookup. The blob's class has no host counterpart either (the model's stores, GeomBlobV2's
// fn_25_256C and the byte 0x154), so those three steps have no host equivalent.
void CIng::AlignBlobToSurface(CStateManager& mgr) {
  CEntity* blobEntity = mgr.ObjectById(mBlobId); // asm 0x00004634: the blob id (0xd5c)
  if (blobEntity == nullptr) {
    return;                                     // asm 0x0000463C: no blob, nothing to do
  }
  CActor* blob = static_cast< CActor* >(blobEntity);
  // asm 0x00004648-0x000046C8: the align flag. The ing-spot form always aligns; the corporeal
  // form only while it is dead (CPatterned 0x420 bit 6 = mAlive is clear) and it started as an
  // ing spot (0xe5d bit 5); the four form-change forms (6..9) only when it started as an ing
  // spot. Every other form does not align.
  float align = 0.f; // asm 0x00004648 (rodata_6C)
  if (mForm == kForm_IngSpot) {
    align = 1.f;     // asm 0x0000467C (rodata_64)
  } else if (mForm == kForm_Corporeal) {
    if (!mAlive && mStartAsIngSpot) {
      align = 1.f;   // asm 0x0000469C-0x000046B8
    }
  } else if (mForm >= kForm_BecomingCorporeal && mForm < 10) {
    if (mStartAsIngSpot) {
      align = 1.f;   // asm 0x00004684-0x00004694
    }
  }
  if (align > 0.f) { // asm 0x000046C4: fcmpo align, 0.0f (rodata_6C)
    const CVector3f normal = mSurfaceAlignment.GetSurface().GetNormal(); // asm 0x000046D4
    // asm 0x000046E8-0x00004758: the normal's z against 0.95f (rodata_D8). The compare is folded
    // into a double (2^52 or 2^52+1) and the difference against rodata_E0 (2^52) picks the up
    // vector: the Right one when the normal points up (z >= 0.95), the Up one otherwise.
    const CVector3f up = normal.GetZ() < 0.95f ? CVector3f::Up() : CVector3f::Right();
    const CVector3f pos = GetTranslation(); // asm 0x000046D8/0x000046EC/0x00004700: 0x54/0x58/0x5c
    const CVector3f look = pos + normal;    // asm 0x00004770-0x00004788
    blob->SetTransform(CTransform4f::LookAt(pos, look, up)); // asm 0x0000478C/0x00004798
  }
  // asm 0x000047EC: GeomBlobV2's fn_25_256C(blob, align) - no host counterpart.
  // asm 0x000047FC-0x0000480C: mDamageCooldownTimer (0x448) > 0 sets the blob's byte 0x154 bit 5
  // (0x20) - no host counterpart.
}

// ---- triggers -------------------------------------------------------------------------------

// @ fn_29_B1F4
bool CIng::StateOver(CStateManager&, const CTriggerData&) const { return mAnimationState.IsOver(); }

// fn_29_B1E8 (not yet exercised in a run)
bool CIng::IsAlert(CStateManager&, const CTriggerData&) const { return mAlert; }

// fn_29_B1C8 (not yet exercised in a run). The asm tests mHostId (0xcdc) against kInvalidUniqueId -
// the lhz/subf/or/srwi-31 idiom - and nothing else; it does not look at the form. That is what lets
// SuckEnergy, which sets form 0 and then clears the host once its suck time runs out, take the
// !IsControllingHost trigger into SetExitHostDest and on into ExitHost.
bool CIng::IsControllingHost(CStateManager&, const CTriggerData&) const {
  return mHostId != kInvalidUniqueId; // 0xcdc
}

// @ fn_29_B0E0
bool CIng::IsFacingTarget(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return true;
  }
  CVector3f delta = target->GetTranslation() - GetTranslation();
  delta = CVector3f(delta.GetX(), delta.GetY(), 0.f);
  return CVector3f::GetAngleDiff(GetTransform().GetForward(), delta) <= kFacingAngle;
}

// @ fn_29_B0A4
bool CIng::HasTarget(CStateManager& mgr, const CTriggerData&) const {
  return mgr.GetObjectById(mTargetId) != nullptr;
}

// @ fn_29_B04C
bool CIng::HasNewTarget(CStateManager& mgr, const CTriggerData& data) const {
  return HasTarget(mgr, data) && mPrevTargetId != mTargetId;
}

// @ fn_29_B01C
bool CIng::HasLineOfSight(CStateManager&, const CTriggerData&) const {
  return mLineOfSight.HasLineOfSight() && mLineOfSight.GetClearTime() > 0.5f;
}

// @ fn_29_B000
bool CIng::UnderFire(CStateManager&, const CTriggerData&) const { return mUnderFireTime < 0.75f; }

// @ fn_29_AFE4
bool CIng::HeardShot(CStateManager&, const CTriggerData&) const { return mHeardShotTime < 1.5f; }

// @ fn_29_AE68: the near box is the model scale (this+0x60) times 10 on each axis - rodata_90 is
// 10.0f and the asm scales scale.x/y/z separately (0x0000AEB4..0x0000AEF4). The include filter is
// the two data words 0x9F4/0x9F8 = 0x20/0x21 = kMT_Player/kMT_Character, type 1 = kFT_Include.
bool CIng::AreaClear(CStateManager& mgr, const CTriggerData&) const {
  const CVector3f half = mModelScale * 10.f;
  const CVector3f pos = GetTranslation();
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, CAABox(pos - half, pos + half),
                    CMaterialFilter::MakeInclude(CMaterialList(kMT_Player, kMT_Character)), this);
  return nearList.empty();
}

// @ fn_29_ADE8
bool CIng::CoverLeash(CStateManager& mgr, const CTriggerData&) const {
  const CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId);
  return hint == nullptr || (hint->GetTranslation() - GetTranslation()).MagSquared() >
                                mData.mUnknown5D0D * mData.mUnknown5D0D;
}

// @ fn_29_ADBC
bool CIng::HasCoverPoint(CStateManager& mgr, const CTriggerData&) const {
  return GetCoverHint(mgr, mCoverHintId) != nullptr;
}

// @ fn_29_AD7C
bool CIng::HasWallCoverPoint(CStateManager& mgr, const CTriggerData&) const {
  const CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId);
  return hint != nullptr && hint->GetHintType() == static_cast< CScriptAIHint::EHintType >(4);
}

// @ fn_29_AD68: the mini-portal count at 0xe00, tested with the neg/or/srwi-31 nonzero idiom
// (0x0000AD68), not a signed comparison.
bool CIng::HasMiniPortals(CStateManager&, const CTriggerData&) const {
  return mMiniPortalCount != 0;
}

// @ fn_29_AD54
bool CIng::IsCorporeal(CStateManager&, const CTriggerData&) const {
  return mForm == kForm_Corporeal;
}

// @ fn_29_AD40
bool CIng::IsIngSpot(CStateManager&, const CTriggerData&) const { return mForm == kForm_IngSpot; }

// @ fn_29_AD2C
bool CIng::IsBodyProjectile(CStateManager&, const CTriggerData&) const {
  return mForm == kForm_BodyProjectile;
}

// @ fn_29_AD14: the frustration time (0x7cc = CIngData+0xc) below the frustration timer (0xe44);
// the asm's fcmpo/extrwi-1 takes the GT bit (0x0000AD14).
bool CIng::IsFrustrated(CStateManager&, const CTriggerData&) const {
  return mData.mFrustrationTime < mFrustrationTimer;
}

// @ fn_29_ACEC
bool CIng::IsAggressive(CStateManager&, const CTriggerData&) const {
  return mAggressive || mSafeZoneCount > 0;
}

// @ fn_29_AC98
bool CIng::IsLuredBySafeZone(CStateManager&, const CTriggerData&) const {
  return mLureSafeZoneId != kInvalidUniqueId &&
         (mForm == kForm_Corporeal || mForm == kForm_IngSpot);
}

// fn_29_AC78 (not yet exercised in a run)
bool CIng::StillLuredBySafeZone(CStateManager&, const CTriggerData&) const {
  return mLureSafeZoneId != kInvalidUniqueId;
}

// @ fn_29_ACE0
bool CIng::UseProjectileFSMEntry(CStateManager&, const CTriggerData&) const {
  return mProjectileFSMEntry;
}

// @ fn_29_AC20: the target id at 0xd6c is looked up and the object is asked whether it sits in a
// safe zone; a missing object is false (0x0000AC20).
bool CIng::TargetInSafeZone(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  return target != nullptr && mgr.GetSafeZoneManager()->IsObjectInSafeZone(*target, mgr);
}

// @ fn_29_ABC8
bool CIng::TargetInLightSafeZone(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  return target != nullptr && mgr.GetSafeZoneManager()->IsObjectInHurtfulSafeZone(*target, mgr);
}

// @ fn_29_AB7C: the REL's own fn_29_45BC is the cover-hint lookup (mCoverHintId at 0xd72, then
// ObjectById + the CScriptAIHint cast), so GetCoverHint is the same thing; the hint's position
// (+0x54) is tested against the safe zone (0x0000AB7C).
bool CIng::EmergePtInSafeZone(CStateManager& mgr, const CTriggerData&) const {
  const CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId);
  return hint != nullptr &&
         mgr.GetSafeZoneManager()->PointIsInSafeZone(mgr, hint->GetTranslation());
}

// @ fn_29_AAF8
bool CIng::CanChangeForm(CStateManager&, const CTriggerData&) const {
  const bool locked = (mData.mFlags210 & kFlag_NoFormChange) != 0;
  switch (mForm) {
  case kForm_IngSpot:
    return !locked;
  case kForm_Corporeal:
    return !locked && mData.mUnknownC620 < mFormTime;
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
    return false;
  default:
    return true;
  }
}

// @ fn_29_AAA8
bool CIng::ShouldEvaporate(CStateManager&, const CTriggerData&) const {
  return mEvaporate &&
         (mForm == kForm_Corporeal || mForm == kForm_IngSpot || mForm == kForm_Evaporate);
}

// fn_29_AA94 (not yet exercised in a run)
bool CIng::ShouldFleeSafeZone(CStateManager&, const CTriggerData&) const {
  return mSafeZoneCount > 0;
}

// fn_29_AA4C (not yet exercised in a run): 0x9d8 == 2 and not fn_29_AD54 (asm 0x0000AA6C).
bool CIng::ShouldBecomeCorporeal(CStateManager& mgr, const CTriggerData& data) const {
  return mDesiredForm == kForm_Corporeal && !IsCorporeal(mgr, data);
}

// fn_29_AA04 (not yet exercised in a run): 0x9d8 == 3 and not fn_29_AD40 (asm 0x0000AA24).
bool CIng::ShouldBecomeIngSpot(CStateManager& mgr, const CTriggerData& data) const {
  return mDesiredForm == kForm_IngSpot && !IsIngSpot(mgr, data);
}

// @ fn_29_A8CC
bool CIng::ShouldArmSwipe(CStateManager& mgr, const CTriggerData&) const {
  if (mForm != kForm_Corporeal || (mData.mFlags210 & kFlag_NoArmSwipe) != 0 ||
      !(mArmSwipeTime > 1.f)) {
    return false;
  }
  // The team gate only applies inside a team (retail skips it when mTeamId is kInvalid, as
  // fn_29_A8CC's branch at 0x0000A924 does).
  if (mTeamId != kInvalidUniqueId &&
      !CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Melee, mgr, mTeamId,
                                        GetUniqueId())) {
    return false;
  }
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return false;
  }
  const CVector3f delta = target->GetTranslation() - GetTranslation();
  const float distSq = delta.MagSquared();
  return mMinAttackRange * mMinAttackRange <= distSq &&
         distSq <= mMaxAttackRange * mMaxAttackRange && fabsf(delta.GetZ()) < 3.f;
}

// @ fn_29_A8C0
bool CIng::ShouldTaunt(CStateManager&, const CTriggerData&) const { return mTaunt; }

// @ fn_29_A780
bool CIng::ShouldBodyProjectile(CStateManager& mgr, const CTriggerData&) const {
  // The team gate only applies inside a team: retail compares mTeamId with kInvalidUniqueId
  // and skips CanStartAttack when they are equal (asm 0x0000A8A4, the branch that lands at
  // .L_0000A7FC). An unconditional gate keeps the state off for every Ing placed without a team.
  if ((mData.mFlags210 & (kFlag_NoBodyProjectile | kFlag_NoFormChange)) != 0 ||
      !mBodyProjectileAllowed ||
      (mTeamId != kInvalidUniqueId &&
       !CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId,
                                         GetUniqueId()))) {
    return false;
  }
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  const CVector3f delta = target != nullptr ? target->GetTranslation() - GetTranslation()
                                            : CVector3f::Zero();
  const float distSq = delta.MagSquared();
  const float minDist = mData.mBodyProjectile.mMinAttackDist; // 0x85c
  const float maxDist = mData.mBodyProjectile.mMaxAttackDist; // 0x860
  const bool inRange = target != nullptr && minDist * minDist <= distSq &&
                       distSq <= maxDist * maxDist && -15.f <= delta.GetZ() &&
                       delta.GetZ() <= 3.f;
  static int sTick = 0;
  if (IngLogTick(sTick)) {
    fprintf(stderr,
            "[ing] %04x body proj trigger: target %04x dist %.1f [%.1f..%.1f] dz %.1f -> %d\n",
            GetUniqueId().Value(), mTargetId.Value(), CMath::SqrtF(distSq), minDist, maxDist,
            delta.GetZ(), inRange ? 1 : 0);
  }
  return inRange;
}

// @ fn_29_A664
bool CIng::ShouldOpenMiniPortal(CStateManager& mgr, const CTriggerData&) const {
  // Same team gate as fn_29_A780/A8CC: retail asks CanStartAttack only when mTeamId is set
  // (asm 0x0000A788, the compare that skips to .L_0000A6E0).
  if ((mData.mFlags210 & kFlag_NoMiniPortal) != 0 || mFormTime < mData.mUnknownC620 ||
      (mTeamId != kInvalidUniqueId &&
       !CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId,
                                         GetUniqueId()))) {
    return false;
  }
  const float distSq = DistSqToTarget(mgr);
  const float minDist = mData.mMiniPortal.mMinAttackDist;
  const float maxDist = mData.mMiniPortal.mMaxAttackDist;
  return minDist * minDist <= distSq && distSq <= maxDist * maxDist;
}

// @ fn_29_A658: the splat flag is bit 6 of the byte at 0xe5e; the asm's extrwi-25 pulls exactly
// that bit out (0x0000A658).
bool CIng::ProjectileSplat(CStateManager&, const CTriggerData&) const { return mSplat; }

// @ fn_29_A4EC: the hit box is the model scale (this+0x60) times 2.5 on each axis - rodata_C8 is
// 2.5f and the asm scales scale.x/y/z separately (0x0000A5A0..0x0000A5F4). A player that is
// morphed or morphing (0x38c is neither 0 nor 3) never counts as a hit (0x0000A524).
bool CIng::ProjectileHitTarget(CStateManager& mgr, const CTriggerData&) const {
  const CEntity* entity = mgr.GetObjectById(mTargetId);
  const CPhysicsActor* target = TCastToConstPtr< CPhysicsActor >(entity);
  if (target == nullptr) {
    return false;
  }
  if (const CPlayer* player = TCastToConstPtr< CPlayer >(entity)) {
    const CPlayer::EPlayerMorphBallState state = player->GetMorphballTransitionState();
    if (state != CPlayer::kMS_Unmorphed && state != CPlayer::kMS_Unmorphing) {
      return false;
    }
  }
  const CVector3f half = mModelScale * 2.5f;
  const CVector3f pos = GetTranslation();
  return CAABox(pos - half, pos + half).DoBoundsOverlap(target->GetBoundingBox());
}

// fn_29_A4E0 (not yet exercised in a run)
bool CIng::FoundMovementPos(CStateManager&, const CTriggerData&) const {
  return mFoundMovementPos;
}

// @ fn_29_A498
bool CIng::PathShagged(CStateManager& mgr, const CTriggerData& data) const {
  return mRecalcPath || CPatterned::PathShagged(mgr, data);
}

// @ fn_29_A450
bool CIng::PathOver(CStateManager& mgr, const CTriggerData& data) const {
  return !mFoundMovementPos || CPatterned::PathOver(mgr, data);
}

// @ fn_29_A428: the ing-spot navigation at 0xcb0 is asked whether the path is over (the DOL's
// fn_801F81C8) with this as the actor (0x0000A428).
bool CIng::PointPathOver(CStateManager&, const CTriggerData&) const {
  return mIngSpotPathFindNavigation.IsPathOver(*this);
}

// @ fn_29_A400: the same navigation object's HasPath (the DOL's fn_801F8178) (0x0000A400).
bool CIng::FoundPointPath(CStateManager&, const CTriggerData&) const {
  return mIngSpotPathFindNavigation.HasPath(*this);
}

// @ fn_29_A3C8
bool CIng::IsOffPath(CStateManager&, const CTriggerData&) const {
  return mPathFindSearch.OnPath(GetTranslation()) != CPathFindSearch::kR_Success;
}

// @ fn_29_A370
bool CIng::HasAttackPattern(CStateManager& mgr, const CTriggerData&) const {
  return const_cast< CIng* >(this)->GetConnectedObject(mgr, kSS_Attack, kSM_Follow) !=
         kInvalidUniqueId;
}

// fn_29_A340 (not yet exercised in a run)
bool CIng::AttackPatternOver(CStateManager&, const CTriggerData&) const {
  if (mFollowingAttackPath) {
    const_cast< CIng* >(this)->mFollowingAttackPath =
        mWaypointNavigation.GetDestination() != kInvalidUniqueId;
  }
  return !mFollowingAttackPath;
}

// @ fn_29_A2D8: the target is a ball that was not spawned as one - asm 0x0000A2E0 tests
// 0x390 (spawned state) first and only reads 0x38c (transition state) when it is 0.
bool CIng::TargetIsBall(CStateManager& mgr, const CTriggerData&) const {
  const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId));
  const bool isBall = player != nullptr &&
                      player->GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed &&
                      player->GetMorphballTransitionState() == CPlayer::kMS_Morphed;
  static int sTick = 0;
  if (IngLogTick(sTick)) {
    fprintf(stderr, "[ing] %04x target is ball: target %04x player %d morph %d -> %d\n",
            GetUniqueId().Value(), mTargetId.Value(), player != nullptr ? 1 : 0,
            player != nullptr ? int(player->GetMorphballTransitionState()) : -1,
            isBall ? 1 : 0);
  }
  return isBall;
}

// @ fn_29_A224: no-grapple flag (0x9d0 bit 3), ball cooldown at or below zero (0xe48) and the
// target inside mData.mGrapple.mPursuitRange (0x8fc), compared squared.
bool CIng::InBallPursuitRange(CStateManager& mgr, const CTriggerData&) const {
  const bool inRange = (mData.mFlags210 & kFlag_NoGrapple) == 0 && mBallCooldown <= 0.f &&
                       DistSqToTarget(mgr) <
                           mData.mGrapple.mPursuitRange * mData.mGrapple.mPursuitRange;
  static int sTick = 0;
  if (IngLogTick(sTick)) {
    fprintf(stderr, "[ing] %04x ball pursuit: distSq %.1f [%.1f] cooldown %.2f -> %d\n",
            GetUniqueId().Value(), DistSqToTarget(mgr), mData.mGrapple.mPursuitRange,
            mBallCooldown, inRange ? 1 : 0);
  }
  return inRange;
}

// @ fn_29_A1C0: the target id at 0xd6c is looked up and the path search at 0x9dc is asked for a
// path from this actor's position (0x54) to the target's (target+0x54); the retail's
// cntlzw/srwi-5 turns a zero result (kR_Success) into true (0x0000A1C0).
bool CIng::HasPathToTarget(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  return target != nullptr &&
         const_cast< CPathFindSearch& >(mPathFindSearch)
                 .PathExists(GetTranslation(), target->GetTranslation()) ==
             CPathFindSearch::kR_Success;
}

// @ fn_29_A100: no-grapple flag, the same spawned-then-transition ball test as TargetIsBall
// (asm 0x0000A144), the surface normal's z at or above rodata_108 (0.9) and the touch box (0xcc0)
// overlapping the player's bounds.
bool CIng::InGrappleRange(CStateManager& mgr, const CTriggerData&) const {
  const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId));
  const float normalZ = mSurfaceAlignment.GetSurface().GetNormal().GetZ();
  const bool overlap =
      player != nullptr && mTouchBox && mTouchBox->DoBoundsOverlap(player->GetBoundingBox());
  const bool inRange = (mData.mFlags210 & kFlag_NoGrapple) == 0 && mTouchBox && player != nullptr &&
                       player->GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed &&
                       player->GetMorphballTransitionState() == CPlayer::kMS_Morphed &&
                       normalZ >= 0.9f && overlap;
  static int sTick = 0;
  if (IngLogTick(sTick)) {
    fprintf(stderr,
            "[ing] %04x grapple range: touchbox %d player %04x morph %d normalZ %.2f overlap %d "
            "flags %02x -> %d\n",
            GetUniqueId().Value(), mTouchBox ? 1 : 0, mTargetId.Value(),
            player != nullptr ? int(player->GetMorphballTransitionState()) : -1, normalZ,
            overlap ? 1 : 0, mData.mFlags210, inRange ? 1 : 0);
  }
  return inRange;
}

// @ fn_29_A098
bool CIng::CanGrappleTarget(CStateManager& mgr, const CTriggerData&) const {
  if ((mData.mFlags210 & kFlag_NoGrapple) != 0) {
    return false;
  }
  const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId));
  return player != nullptr && player->GetAttachedActorId() == kInvalidUniqueId;
}

// ---- states ---------------------------------------------------------------------------------

// @ fn_29_A094
void CIng::Start(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("Start");
  }
}

// @ fn_29_A090: retail's state body is empty - a bare blr (0x0000A090), so the state just waits
// for the FSM to move on.
void CIng::WaitForFSMTransition(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("WaitForFSMTransition");
  }
}

// @ fn_29_9D68. The exit host: the Ing it leaves behind is a swarm. Its
// Activate swaps the possessed-mold billboard for a CIngExitHostEffect (one rs_new of 0x5e8, the
// class's only allocation), Update rides that effect until it reports it has arrived, and Deactivate
// drops the Ing back onto the mesh as an ing spot wherever SetExitHostDest pointed. Retail's Activate
// adds kMT_Target (0x28), Deactivate adds back 0x21/0x3b/0x29.
void CIng::ExitHost(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate: {
    LogState("ExitHost");
    AddMaterial(kMT_Target, mgr);  // 0x28
    mForm = kForm_ControllingHost; // 0x9d4 = 1
    // The mold billboard and any earlier swarm effect go. Retail allocates the new id before the
    // rs_new, so a failed build still consumes one.
    if (mPossessedEffectId != kInvalidUniqueId) {
      mgr.DeleteObjectRequest(mPossessedEffectId); // 0xcde
    }
    if (mD76Id != kInvalidUniqueId) {
      mgr.DeleteObjectRequest(mD76Id); // 0xd76
    }
    mD76Id = mgr.AllocateUniqueId(); // 0xd76
    // The ctor's argument order is the header's: the destination is the state's 0xdc8, the scale is
    // the model's, and the two assets and the five swarm-block values follow the member order of
    // CIngSwarmData (0x9a8..0x9c4 = mSRSC, mPART3DA2, the three unknown floats and the move sound).
    CIngExitHostEffect* effect = rs_new CIngExitHostEffect(
        mD76Id,
        CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId),
        GetTransform(), mTargetPos, GetModelData()->GetScale(), mData.mSwarm.mSRSC,
        mData.mSwarm.mPART3DA2, mData.mSwarm.mUnknown2327, mData.mSwarm.mUnknownCB39, mTargetId,
        mData.mSwarm.mUnknown587C, mData.mSwarm.mUnknown0BD7, mData.mSwarm.mSoundSwarmMove);
    if (effect == nullptr) {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
    } else {
      mgr.AddObject(*effect);
      mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6b4 = 1
    }
    // One emitter, chosen by whether the target is in a safe zone. Retail builds an empty
    // CTriggerData (its single float zeroed) on the stack for fn_29_AC20, which is why the port's
    // TargetInSafeZone takes an argument it never reads.
    const ushort sound = TargetInSafeZone(mgr, CTriggerData(0.f))
                             ? mData.mSwarm.mSoundExitHostSafeZone // swarm +0x24
                             : mData.mSwarm.mSoundExitHost;        // swarm +0x22
    CSfxManager::AddEmitter(sound, GetTranslation(), 0x7f, GetCurrentAreaId().Value(), true, false,
                            CSfxManager::kMedPriority);
    if (DebugIng()) {
      fprintf(stderr, "[ing] %04x exit host: swarm %04x, sound %04x\n", GetUniqueId().Value(),
              mD76Id.Value(), sound);
    }
    break;
  }
  case kStateMsg_Update: {
    // Retail looks the object up by id and reads its byte 0x5e2 with no type test; the class has
    // retail's vtable and no TypesMatch override, so the cast is only to reach IsDone().
    CIngExitHostEffect* effect = static_cast< CIngExitHostEffect* >(mgr.ObjectById(mD76Id));
    if (effect == nullptr || effect->IsDone()) {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
    } else {
      SetTranslation(effect->GetTranslation());
    }
    break;
  }
  case kStateMsg_Deactivate:
    // The Ing lands where SetExitHostDest pointed and becomes an ing spot: snapped to the surface
    // (CTerrainAlignment::AlignToSurface with 1.0f), solid again, and the grapple cooldown armed.
    SetTranslation(mTargetPos); // 0xdc8
    mSurfaceAlignment.AlignNearPosition(*this, mgr, mTargetPos, 1.f);
    AddMaterial(kMT_Character, kMT_Solid, kMT_Orbit, mgr); // 0x21/0x3b/0x29
    mBallCooldown = mData.mGrapple.mPostWaitTime;          // 0xe48 = 0x8f8
    SpawnExitHostSmoke(mgr);                               // fn_29_1F1C
    mForm = kForm_IngSpot;                                 // 0x9d4 = 3
    if (DebugIng()) {
      fprintf(stderr, "[ing] %04x exit host: left at (%.1f, %.1f, %.1f)\n", GetUniqueId().Value(),
              mTargetPos.GetX(), mTargetPos.GetY(), mTargetPos.GetZ());
    }
    break;
  default:
    break;
  }
}

// @ fn_29_1F1C (the 2026-10-20 run, /tmp/opencode/ing_run5.log: "state ExitHost" -> "exit host:
// smoke explosion" -> "left at (78.0, 368.0, 4.9)" once CIngExitHostEffect's Think set the
// arrival flag): the exit host's smoke, spawned where the Ing lands. It
// only fires when the swarm block's second GEN token is loaded (retail's engaged byte at 0xcfc);
// then one CExplosion "IngExitHostSmokeFx" on the Ing's own transform at the model's scale (flags
// 0, white, player -1) and the swarm block's own sound (0x9cc = CIngData swarm +0x28).
void CIng::SpawnExitHostSmoke(CStateManager& mgr) {
  const bool smokeLoaded = static_cast< bool >(mSwarmEffect2); // 0xcfc, the token's engaged byte
  if (DebugIng()) {
    fprintf(stderr, "[ing] %04x exit host: smoke %s\n", GetUniqueId().Value(),
            smokeLoaded ? "explosion" : "token not loaded");
  }
  if (!smokeLoaded) {
    return;
  }
  CExplosion* smoke = rs_new CExplosion(
      *mSwarmEffect2, mgr.AllocateUniqueId(),
      CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId),
      rstl::string_l("IngExitHostSmokeFx"), GetTransform(), 0, GetModelData()->GetScale(),
      CColor::White(), -1);
  if (smoke != nullptr) {
    mgr.AddObject(*smoke);
    CSfxManager::AddEmitter(mData.mSwarm.mSound, GetTranslation(), 0x7f,
                            GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
  }
}

// fn_29_1A80: turn to the lure safe zone (0xd78) when one is set, else to the target (0xd6c).
// The offset is taken horizontally (z of the delta is zeroed) and only applied when its magnitude
// is safe; the actor's own z is kept. Called by BecomeCorporeal's Activate.
void CIng::FaceLureOrTarget(CStateManager& mgr) {
  const CVector3f pos = GetTranslation();
  // Retail reads the object's +0x54 without a type test; the port asks for an actor (the lure is a
  // CScriptSafeZone, the target a CActor).
  const CActor* other = TCastToConstPtr< CScriptSafeZone >(mgr.GetObjectById(mLureSafeZoneId));
  if (other == nullptr) {
    other = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  }
  if (other == nullptr) {
    return;
  }
  CVector3f delta = other->GetTranslation() - pos;
  delta.SetZ(0.f);
  if (delta.IsMagnitudeSafe()) {
    SetTransform(CTransform4f::LookAt(pos, pos + delta, CVector3f::Up()));
  }
}

// @ fn_29_9A74
void CIng::BecomeCorporeal(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("BecomeCorporeal");
    mProjectileFSMEntry = false; // 0xe5e bit 7
    if (mForm == kForm_IngSpot) {
      mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6b4 = 1
      // 0xdf0/0xdfc: the horizontal plane through the actor. The normal is (0,0,1) (retail
      // normalises it) and the constant is dot(translation, normal) = translation.z.
      mPortalPlaneNormal = CVector3f::Up();
      mPortalPlaneConstant = CVector3f::Dot(GetTranslation(), mPortalPlaneNormal);
      mPortalPlane = true; // 0xe5d bit 6
      mForm = kForm_BecomingCorporeal;
    } else {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
    }
    if (!mCollisionActorManager.null()) {
      mCollisionActorManager->SetActive(mgr, true); // 0xc50
    }
    mSurfaceAlignment.SetMode(CSurfaceAlignmentHelper::kM_WorldUp); // 0xca4 = 2
    mLineOfSight.SetSegment(mEyeSegId);                            // 0xd8a = 0xd7b ("head")
    FaceLureOrTarget(mgr);                                         // fn_29_1A80
    mMoving = false;                                               // 0xe5d bit 1
    Stop();
    break;
  case kStateMsg_Update:
    // CTryCommand::ShouldRetry(this+0x6b4, *BodyController(), 0xc): 0xc is the body state
    // kAS_Generate, i.e. the port's CAnimationState tracks the Generate animation.
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCGenerateCmd(pas::kGType_Zero, CVector3f::Zero()));
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) {
      BodyController()->SetLocomotionType(pas::kLT_Relaxed); // 0x00009C9C: li r4, 1
    }
    Stop();
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    mPortalPlane = false;                                    // 0xe5d bit 6
    mStartAsIngSpot = false;                                 // 0xe5d bit 5
    mVerticalMovement = false;                               // CPatterned 0x34c bit 6
    AddMaterial(kMT_GroundCollider, mgr);                    // 0x25
    BodyController()->SetLocomotionType(pas::kLT_Relaxed); // 0x00009CEC: li r4, 1
    mForm = kForm_Corporeal;
    mDesiredForm = -1;
    mSurfaceAlignment.SetMode(CSurfaceAlignmentHelper::kM_None); // 0xca4 = 0
    mFormTime = 0.f;                                             // 0xe3c
    if (AnimationData() != nullptr) {
      // rodata_68E: "eyes"; the effect runs while the Ing is corporeal.
      AnimationData()->SetEffectState(rstl::string_l("eyes"), true, mgr);
    }
    break;
  default:
    break;
  }
}

// @ fn_29_9750
void CIng::BecomeIngSpot(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("BecomeIngSpot");
    if (mForm == kForm_Corporeal) {
      mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6b4 = 1
      mPortalPlaneNormal = CVector3f::Up();                 // 0xdf0
      mPortalPlaneConstant = CVector3f::Dot(GetTranslation(), mPortalPlaneNormal); // 0xdfc
      mPortalPlane = true;                                  // 0xe5d bit 6
      mForm = kForm_BecomingIngSpot;
    } else {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
    }
    mSurfaceAlignment.SetMode(CSurfaceAlignmentHelper::kM_NearbySurface); // 0xca4 = 1
    mBodyState5Time = 0.f;                                                // 0xe40
    mSafeZoneIngSpot = false;                                             // 0xe5e bit 5
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCGenerateCmd(pas::kGType_One, CVector3f::Zero()));
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) {
      BodyController()->SetLocomotionType(pas::kLT_Crouch); // 0x00009948: li r4, 0
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    BodyController()->SetLocomotionType(pas::kLT_Crouch); // 0x00009960: li r4, 0
    mForm = kForm_IngSpot;
    mDesiredForm = -1;
    mLineOfSight.SetSegment(CSegId::Null()); // 0xd8a = 99
    mVerticalMovement = true;                // CPatterned 0x34c bit 6
    RemoveMaterial(kMT_GroundCollider, mgr); // 0x25
    if (!mCollisionActorManager.null()) {
      mCollisionActorManager->SetActive(mgr, false); // 0xc50
    }
    mPortalPlane = false;   // 0xe5d bit 6
    mStartAsIngSpot = true; // 0xe5d bit 5
    if (AnimationData() != nullptr) {
      AnimationData()->SetEffectState(rstl::string_l("eyes"), false, mgr);
    }
    break;
  default:
    break;
  }
}

// fn_29_4468: the target's aim position (vtable slot 0x54, CActor::GetAimPosition, with the
// caller's prediction time in f1). False when the target id is empty; the caller then keeps
// whatever it had. Every caller passes rodata_6C (0.0f) except the body projectile's flight, which
// passes rodata_B4 (0.5f) - asm 0x00008054 loads it just before the call at 0x0000806C.
bool CIng::TargetAimPosition(CStateManager& mgr, CVector3f& out, float prediction) {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return false;
  }
  out = target->GetAimPosition(mgr, prediction);
  return true;
}

// @ fn_29_94E8: the Ing turns into a flying body projectile. Exercised in Agon
// 01_sand_entrance_dark (/tmp/opencode/ing_fs3.log, 2026-10-20): "state BecomeBodyProjectile
// form 2" -> the loop-attack command -> the form 4 flight (fn_29_7E30).
void CIng::BecomeBodyProjectile(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("BecomeBodyProjectile");
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6b4 = 1
    // Form 4, the flying body projectile, is set by the kUE_BeginAction user anim event
    // (fn_29_C228, handled in DoUserAnimEvent) once the anim reaches it; the state then ends and
    // the flight takes over.
    mForm = kForm_BecomingBodyProjectile; // 0x9d4 = 8
    mTakeOff = false;                                     // 0xe5c bit 0
    mDesiredForm = -1;                                    // 0x9d8
    break;
  case kStateMsg_Update:
    // The try command waits for the body state kAS_LoopAttack (9), then asks for the loop attack.
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      mVerticalMovement = true;               // CPatterned 0x34c bit 6
      RemoveMaterial(kMT_GroundCollider, mgr); // 0x25
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_Zero));
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_LoopAttack) {
      BodyController()->SetLocomotionType(pas::kLT_Crouch); // 0x00009628: li r4, 0
      if (!mTakeOff && mForm != kForm_BodyProjectile) {
        // Aim at the target's aim position, flattened to the actor's own height (0xdd0 = 0x5c).
        if (TargetAimPosition(mgr, mTargetPos)) { // fn_29_4468
          mTargetPos.SetZ(GetTranslation().GetZ());
          BodyController()->CommandMgr().SetTargetVector(mTargetPos - GetTranslation());
        }
      } else {
        if (!mCollisionActorManager.null()) {
          mCollisionActorManager->SetActive(mgr, false); // 0xc50
        }
        mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
      }
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    mVerticalMovement = true;                                // CPatterned 0x34c bit 6
    RemoveMaterial(kMT_GroundCollider, mgr);                 // 0x25
    if (!mCollisionActorManager.null()) {
      mCollisionActorManager->SetActive(mgr, false); // 0xc50
    }
    BodyController()->SetLocomotionType(pas::kLT_Crouch); // 0x000096FC: li r4, 0
    if (AnimationData() != nullptr) {
      AnimationData()->SetEffectState(rstl::string_l("eyes"), false, mgr); // rodata_68E
    }
    break;
  default:
    break;
  }
}

// fn_29_8FE8 (not yet exercised in a run)
void CIng::BecomeWallProjectile(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate: {
    LogState("BecomeWallProjectile");
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6b4 = 1
    mForm = kForm_BecomingWallProjectile;                 // 0x9d4 = 9
    mForm9Render = false;                                 // 0xe5d bit 4
    mTakeOff = false;                                     // 0xe5c bit 0
    mDesiredForm = -1;                                    // 0x9d8
    // Face out of the surface it is on: columns = (normal x up normalised, normal, up), origin one
    // normal's length off the wall.
    const CVector3f normal = mSurfaceAlignment.GetSurface().GetNormal();
    const CVector3f side = CVector3f::Cross(normal, CVector3f::Up()); // fn_29_94A8
    if (side.IsMagnitudeSafe()) {
      SetTransform(CTransform4f::FromColumns(side.AsNormalized(), normal, CVector3f::Up(),
                                            GetTranslation() + normal));
    }
    mSurfaceAlignment.SetMode(CSurfaceAlignmentHelper::kM_None); // 0xca4 = 0
    mPortalPlane = true;                                         // 0xe5d bit 6
    // The portal plane sits the bounding box's Y extent below the actor along the normal; retail
    // scalarises dot(translation - h*normal, normal) into (x,nx),(y,ny),(z,nz) (asm 0x00009194).
    const CAABox bounds = GetBoundingBox();
    const float height = bounds.GetMaxPoint().GetY() - bounds.GetMinPoint().GetY();
    mPortalPlaneNormal = normal;                                // 0xdf0
    mPortalPlaneConstant = CVector3f::Dot(GetTranslation() - normal * height, normal); // 0xdfc
    BodyController()->EnableAnimation(true);
    mMoving = false; // 0xe5d bit 1
    break;
  }
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_One));
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_LoopAttack) {
      BodyController()->SetLocomotionType(pas::kLT_Crouch); // 0x0000928C: li r4, 0
      mForm9Render = true; // 0xe5d bit 4
      if (mTakeOff || mForm == kForm_BodyProjectile) {
        mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
      } else if (TargetAimPosition(mgr, mTargetPos)) { // fn_29_4468
        mTargetPos.SetZ(GetTranslation().GetZ());
        const CVector3f normal = mSurfaceAlignment.GetSurface().GetNormal();
        CVector3f dir = mTargetPos - GetTranslation();
        // Within 45 degrees of the surface normal, else clamp the direction to 45 degrees off it.
        if (CVector3f::GetAngleDiff(dir, normal) > kWallLaunchAngle) {
          dir = CVector3f::Slerp(normal, dir.AsNormalized(),
                                 CRelAngle::FromRadians(kWallLaunchAngle));
        }
        BodyController()->CommandMgr().SetTargetVector(dir);
      }
    }
    break;
  case kStateMsg_Deactivate:
    mPortalPlane = false;    // 0xe5d bit 6
    mStartAsIngSpot = false; // 0xe5d bit 5
    BodyController()->SetLocomotionType(pas::kLT_Crouch); // 0x000093DC: li r4, 0 (li at 0x000093C4)
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    if (AnimationData() != nullptr) {
      AnimationData()->SetEffectState(rstl::string_l("eyes"), false, mgr); // rodata_68E
    }
    break;
  default:
    break;
  }
}

// @ fn_29_8EF0
void CIng::Alert(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("Alert");
    mPrevTargetId = mTargetId;
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    Stop();
  } else if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
      BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(pas::kTT_Four));
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mFormTime = 0.f;
  }
}

// fn_29_8CF4 (not yet exercised in a run)
void CIng::SafeZoneReaction(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("SafeZoneReaction");
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    // 0x00008D58: AddEmitter(sfx, position, 0x7f, area, true, false, kMedPriority). Retail has no
    // invalid-id test; the host's AddEmitter drops 0xffff by itself.
    CSfxManager::AddEmitter(mData.mSwarm.mSoundExitHostSafeZone, GetTranslation(), 0x7f,
                            GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
  } else if (msg == kStateMsg_Update) {
    // 0x00008D60: a harmful zone or the evaporating form knocks the Ing back; otherwise the state
    // ends after a second (the state time comes from the state machine at 0x350).
    if (!mHurtfulZone && !mEvaporate) {
      if (mStateTime > 1.f) { // rodata_64 = 1.0
        mAnimationState.SetState(CAnimationState::kAS_Over);
      }
    } else if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_KnockBack)) {
      // 0x00008D90: the knock-back direction is the actor's forward (0x28/0x38/0x48 = m01/m11/m21),
      // the severity 1 and the animation id -1.
      BodyController()->CommandMgr().DeliverCmd(
          CBCKnockBackCmd(GetTransform().GetForward(), pas::kS_One));
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// @ fn_29_8BAC: the Ing taunts the player. Exercised in Agon 01_sand_entrance_dark
// (/tmp/opencode/ing_fs3.log, 2026-10-20: six "state Taunt form" entries in the run).
void CIng::Taunt(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("Taunt");
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x00008BE4: 0x6b4 = 1
  } else if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) { // 0x00008BF4: 0x10
      BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(pas::kTT_Two)); // 0x00008C40: 0x15
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x00008C70: 0x6b4 = 0
    mTaunt = false; // 0x00008C78: 0xe5c bit 6
  }
}

// @ fn_29_8A2C
void CIng::Lurk(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("Lurk");
    if (DebugIng()) {
      fprintf(stderr, "[ing] %04x lurk: detection range %.1f height %.1f\n", GetUniqueId().Value(),
              mDetectionRange, mDetectionHeightRange);
    }
    if (mgr.GetObjectById(mTargetId) != nullptr) {
      mAlert = true;
    }
    CRandom16* random = mgr.Random();
    mTaunt = random->Range(0.f, 100.f) <= mData.mTauntChance;
    mAggressive = (mData.mFlags210 & kFlag_NoArmSwipe) == 0 &&
                  random->Range(0.f, 100.f) <= mData.mAggressiveness;
    mBodyProjectileAllowed =
        (mData.mFlags210 & kFlag_NoBodyProjectile) == 0 &&
        ((mData.mFlags210 & kFlag_NoMiniPortal) != 0 ||
         random->Range(0.f, 100.f) <= mData.mBodyProjectile.mOdds);
  } else if (msg == kStateMsg_Update) {
    if (mForm == kForm_IngSpot) {
      Stop();
    }
  }
}

// @ fn_29_8808
void CIng::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  const bool canPathFind = (mData.mFlags210 & kFlag_NoPathFind) == 0 && mFoundMovementPos;
  bool moved = false;
  if (msg == kStateMsg_Activate) {
    LogState("PathFind");
    const bool lured = mLureSafeZoneId != kInvalidUniqueId;
    // [?] the slow speed range when neither aggressive nor lured.
    BodyController()->CommandMgr().SetSteeringSpeedRange(mAggressive || lured ? 1.f : 0.5f,
                                                         mAggressive || lured ? 1.f : 0.5f);
    if (lured) {
      mPathFindSearch.SetFlags(1);
    }
    if (canPathFind) {
      mPathFindNavigation.PathFind(mgr, msg, dt, *this);
    }
    moved = true;
    mRecalcPath = false;
  } else if (msg == kStateMsg_Update) {
    if (canPathFind) {
      const bool hold = !mAggressive && mLineOfSight.HasLineOfSight() && mSafeZoneCount < 1 &&
                        mLureSafeZoneId == kInvalidUniqueId;
      if (!hold) {
        mPathFindNavigation.PathFind(mgr, msg, dt, *this);
        ApplySeparation(mgr);
        moved = true;
      }
    }
  } else if (msg == kStateMsg_Deactivate) {
    mPathFindSearch.SetFlags(0x301);
    moved = true;
  }
  if (!moved) {
    Stop();
  }
}

// @ fn_29_8684: the puddle's mesh path find, and the one the grapple runs through. Activate clears
// mRecalcPath and raises mMoving; the body (asm 0x000086DC) only paths when the properties allow
// path finding (0x9d0 bit 1) and SetCoverDest/SetRecoverDest found a destination (0xe5c bit 3).
// The speed is the safe-zone modifier or the maximum, and a ball target overrides both with the
// ball pursuit speed (asm 0x00008778-0x00008894). MoveOnSurface takes the raw separation vector,
// FaceDirection the normalized one.
void CIng::IngSpotPathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  // fn_29_37D8 (0x170): the retail's own path walk over the ing-spot's graph; over the size line,
  // so the host keeps its minimal body and logs the stand-in.
  PortIngStandIn("fn_29_37D8 path helper (0x170)");
  if (msg == kStateMsg_Activate) {
    LogState("IngSpotPathFind");
    mRecalcPath = false; // 0xe5c bit 2
    mMoving = true;      // 0xe5d bit 1
  }
  if ((mData.mFlags210 & kFlag_NoPathFind) != 0 || !mFoundMovementPos) {
    return;
  }
  mPathFindNavigation.PathFind(mgr, msg, dt, *this);
  ApplySeparation(mgr);
  const CVector3f move = BodyController()->CommandMgr().GetMoveVector();
  BodyController()->CommandMgr().ClearLocomotionCmds();
  if (move.IsMagnitudeSafe() && dt > 0.f) {
    float speed = mSafeZoneIngSpot ? mData.mIngSpot.GetSpeedModifier()
                                   : mData.mIngSpot.GetMaxSpeed();
    if (TargetIsBall(mgr, CTriggerData(0.f))) {
      speed = mData.mIngSpot.GetBallPursuitSpeed();
    }
    MoveOnSurface(move, speed, dt);
    BodyController()->FaceDirection(move.AsNormalized(), dt);
  }
}

// @ fn_29_8570: the point-graph variant, same two guards and the same Activate stores as
// fn_29_8684 (asm 0x0000858C-0x000086A8); the speed blends from the safe-zone-or-maximum base to
// the wall speed by the angle between the actor's up and world up (asm 0x00008600-0x00008740).
void CIng::IngSpotPointPathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("IngSpotPointPathFind");
    mRecalcPath = false; // 0xe5c bit 2
    mMoving = true;      // 0xe5d bit 1
  }
  if ((mData.mFlags210 & kFlag_NoPathFind) != 0 || !mFoundMovementPos) {
    return;
  }
  mIngSpotPathFindNavigation.PathFind(mgr, msg, dt, *this);
  const CVector3f move = BodyController()->CommandMgr().GetMoveVector();
  BodyController()->CommandMgr().ClearLocomotionCmds();
  const float baseSpeed = mSafeZoneIngSpot ? mData.mIngSpot.GetSpeedModifier()
                                           : mData.mIngSpot.GetMaxSpeed();
  const float speed = CVector3f::GetAngleDiff(CVector3f::Up(), GetTransform().GetUp()) /
                          3.1415927f * (mData.mIngSpot.GetMaxWallSpeed() - baseSpeed) +
                      baseSpeed;
  MoveOnSurface(move, speed, dt);
}

// fn_29_84A4 (not yet exercised in a run)
void CIng::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("Patrol");
    // 0x00008510: the steering blend mode 1 (kSBM_FullSpeed); the speed range is the locomotion's
    // walk speed over its run speed (0x00008508 divides the two). The host's GetLocomotionSpeed
    // returns 0.f when the locomotion state is missing, so the ratio is then inf, as in retail.
    BodyController()->CommandMgr().SetSteeringBlendMode(ESteeringBlendMode::kSBM_FullSpeed);
    const CBodyStateInfo& info = BodyController()->GetBodyStateInfo();
    const float speed =
        info.GetLocomotionSpeed(pas::kLA_Walk) / info.GetLocomotionSpeed(pas::kLA_Run);
    BodyController()->CommandMgr().SetSteeringSpeedRange(speed, speed);
  }
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);
}

// fn_29_82D4 (not yet exercised in a run)
void CIng::FollowAttackPattern(CStateManager& mgr, EStateMsg msg, float dt) {
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);
  // 0x00008310: in the ing-spot form the Ing walks the waypoint's own speed (0x158) over the ing
  // spot's maximum speed (0x87c), on every message - the waypoint is the navigation's destination
  // (0x6c0) and a missing one leaves the speed at 1.0 (rodata_64).
  if (mForm == kForm_IngSpot) {
    const CVector3f move = BodyController()->CommandMgr().GetMoveVector();
    BodyController()->CommandMgr().ClearLocomotionCmds();
    const CScriptAIWaypoint* waypoint =
        TCastToConstPtr< CScriptAIWaypoint >(mgr.GetObjectById(mWaypointNavigation.GetDestination()));
    const float speed = waypoint != nullptr ? waypoint->GetSpeed() : 1.f;
    MoveOnSurface(move, speed * mData.mIngSpot.GetMaxSpeed(), dt);
  }
  if (msg == kStateMsg_Activate) {
    LogState("FollowAttackPattern");
    // 0x000083AC: the resume id (0xd70) when it is set, else the "ATTK"/"FOLW" connection.
    const TUniqueId dest = mAttackPathResumeId != kInvalidUniqueId
                               ? mAttackPathResumeId
                               : GetConnectedObject(mgr, kSS_Attack, kSM_Follow);
    mWaypointNavigation.SetDestination(dest);
    mFollowingAttackPath = true; // 0x000083FC: 0xe5d bit 3
  } else if (msg == kStateMsg_Update) {
    // 0x00008410: the waypoint's flags (0x164) pick the form: 0x40 or 0x80 the ing spot (form 3),
    // 0x100 the corporeal one (form 2).
    const CScriptAIWaypoint* waypoint =
        TCastToConstPtr< CScriptAIWaypoint >(mgr.ObjectById(mWaypointNavigation.GetDestination()));
    if (waypoint != nullptr) {
      if ((waypoint->GetFlags() & (0x40 | 0x80)) != 0) {
        mDesiredForm = kForm_IngSpot;
      } else if ((waypoint->GetFlags() & 0x100) != 0) {
        mDesiredForm = kForm_Corporeal;
      }
    }
  } else if (msg == kStateMsg_Deactivate) {
    mFollowingAttackPath = false; // 0x00008468: 0xe5d bit 3
    mAttackPathResumeId = mWaypointNavigation.GetDestination(); // 0x00008478: 0xd70 = 0x6c0
  }
}

// @ fn_29_7E30: the body projectile in flight. The launch point and the flight direction are two
// file statics in the REL's bss, seeded on the first call (bss_10, bss_20) from Zero() and
// Forward() and shared by every Ing; each Activate rewrites both. Exercised in area 57
// (/tmp/opencode/ing_bp3.log): BecomeBodyProjectile -> BodyProjectileFlight form 8 (the drift
// branch, two frames) -> form 4 once the anim's kUE_BeginAction lands (35 units a second along the
// launch direction) -> SuckEnergy on the hit, with the emitter removed in Deactivate.
void CIng::BodyProjectileFlight(CStateManager& mgr, EStateMsg msg, float dt) {
  static bool sInit = false;
  static CVector3f sLaunchPoint;
  static CVector3f sFlightDir;
  if (!sInit) {
    sLaunchPoint = CVector3f::Zero();
    sFlightDir = CVector3f::Forward();
    sInit = true;
  }

  switch (msg) {
  case kStateMsg_Activate: {
    LogState("BodyProjectileFlight");
    mSplat = false;             // 0xe5e bit 6
    mProjectileFSMEntry = true; // 0xe5e bit 7
    sLaunchPoint = GetLctrTransform(mEyeSegId).GetTranslation(); // 0xd7b
    sFlightDir = CVector3f::Forward();
    if (TargetAimPosition(mgr, mTargetPos, 0.5f)) { // fn_29_4468, 0xdc8; f1 = rodata_B4
      // The aim position is dropped by the model's height (scale.z, the REL's only non-zero scale
      // term); a shot that points upwards keeps Forward() as its direction.
      const CVector3f scale = GetModelData()->GetScale(); // 0x60, model data's first CVector3f
      CVector3f aim = mTargetPos - sLaunchPoint;
      aim.SetZ(aim.GetZ() - scale.GetZ());
      if (aim.GetZ() < 0.f && aim.IsMagnitudeSafe()) {
        sFlightDir = GetTransform().TransposeRotate(aim.AsNormalized()); // 0x24
      }
    }
    mBodyProjectileSfx =
        CSfxManager::AddEmitter(mData.mBodyProjectile.mSoundProjectile, GetTranslation(), 0x7f,
                                GetCurrentAreaId().Value(), true, true,
                                CSfxManager::kMedPriority); // 0x850
    break;
  }
  case kStateMsg_Update: {
    // Retail calls UpdateEmitter unconditionally (asm 0x00008090), empty handle included.
    CSfxManager::UpdateEmitter(mBodyProjectileSfx, GetTranslation(),
                               GetTransform().GetForward(), 0x7f);
    static int sTick = 0;
    static int sLastForm = -1;
    if (mForm != sLastForm || IngLogTick(sTick)) {
      sLastForm = mForm;
      fprintf(stderr,
              "[ing] %04x flight: form %d time %.2f dt %.3f dir (%.2f, %.2f, %.2f) at "
              "(%.1f, %.1f, %.1f)\n",
              GetUniqueId().Value(), mForm, StateMachineState().GetTime(), dt, sFlightDir.GetX(),
              sFlightDir.GetY(), sFlightDir.GetZ(), GetTranslation().GetX(),
              GetTranslation().GetY(), GetTranslation().GetZ());
    }
    if (mForm == kForm_BodyProjectile) {
      if (dt > 0.f) {
        // Once the state has been running longer than the drop time the flight starts falling: the
        // steer vector loses gravity along z, and the whole velocity follows the steer vector.
        if (StateMachineState().GetTime() > mData.mBodyProjectile.mDropTime) { // 0x858
          sFlightDir.SetZ(sFlightDir.GetZ() - dt * GetGravityConstant());
        }
        MoveInOneFrameOR((dt * mData.mBodyProjectile.mSpeed) * sFlightDir, dt); // 0x854
      }
    } else if (dt > 0.f) {
      // Any other form: the horizontal speed along the steer vector, plus a slow drift (10 units a
      // second) back onto the launch point's height so the body still comes down where it started.
      CVector3f velocity = (dt * mData.mBodyProjectile.mSpeed) * sFlightDir;
      velocity.SetZ(0.f);
      CVector3f drift(0.f, 0.f, sLaunchPoint.GetZ() - GetTranslation().GetZ()); // 0x5c = z
      if (drift.IsMagnitudeSafe()) {
        velocity += drift.AsNormalized() * (dt * 10.f);
      }
      MoveInOneFrameOR(velocity, dt);
    }
    break;
  }
  case kStateMsg_Deactivate:
    // Command 10 (lbl_29_data_C50's id word) is kBSC_ExitState.
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    // Retail removes unconditionally and then stores 0 to 0xd58 (asm 0x0000828C..0x000082A0).
    CSfxManager::RemoveEmitter(mBodyProjectileSfx);
    mBodyProjectileSfx.Clear(); // 0xd58 = 0
    break;
  default:
    break;
  }
}

// @ fn_29_76B8: the Ing possesses the player. Activate drops the body (form 0), clears its
// particles, aborts the scripted body state, strips its materials, remembers the target as the host,
// spawns the possessed-mold billboard and the drape voice, deals the body projectile's contact
// damage and attaches to the player (with the HUD warning); Update rides on the host's transform and
// aim point, draining mUnknownA0D6 a second through a solid-only filter, until the body projectile's
// suck time runs out; Exit releases the player and the voice.
void CIng::SuckEnergy(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    LogState("SuckEnergy");
    mSuckTickLogged = false;
    mForm = kForm_Host; // 0x9d4 = 0
    AnimationData()->GetParticleDB().DestroyAllActiveParticles();
    AnimationData()->GetParticleDB().ClearAllNonPersistentEffects(&mgr);
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_AbortScripted)); // id word 13
    // Immediate values 0x21/0x25/0x3b/0x28/0x29.
    RemoveMaterial(kMT_Character, kMT_GroundCollider, kMT_Solid, kMT_Target, kMT_Orbit, mgr);
    mHostId = mTargetId; // 0xcdc <- 0xd6c
    if (mSwarmEffect) {  // 0xcec, the swarm GEN token's engaged byte
      mPossessedEffectId = mgr.AllocateUniqueId(); // 0xcde
      mgr.AddObject(rs_new CHUDBillboardEffect(
          rstl::optional_object< TToken< CGenDescription > >(*mSwarmEffect),
          rstl::optional_object_null(), mPossessedEffectId, true,
          rstl::string_l("Ing Possessed Mold"), CHUDBillboardEffect::GetNearClipDistance(mgr, 0),
          CHUDBillboardEffect::GetScaleForPOV(mgr), 0, CColor::White(), CVector3f::One(),
          CVector3f::Zero(), true)); // adjustDepthRange, unlike the Grenchler's visor effects
    }
    // 0x852: the body projectile's splat-wall sound, one shot at the Ing's own position.
    mSuckSfx = CSfxManager::AddEmitter(mData.mBodyProjectile.mSoundSplatWall, GetTranslation(), 0x7f,
                                       GetCurrentAreaId().Value(), true, false,
                                       CSfxManager::kMedPriority); // 0xd00
    // 0x82c (the body projectile's contact damage) with the default filter (kFT_Always) and the
    // actor transform's column 1 as the knock-back.
    mgr.ApplyDamage(GetUniqueId(), mHostId, GetUniqueId(), mData.mBodyProjectile.mContactDamage,
                    CMaterialFilter(), GetTransform().GetForward());
    if (CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mHostId))) {
      // 0x1144 bits 7/6 cleared (asm 0x00007A98/0x00007AB0): no morph transition of either kind
      // while the Ing rides the player.
      player->EnableLeaveMorphBall(false);
      player->EnableEnterMorphBall(false);
      player->AttachActorToPlayer(GetUniqueId(), false);
      CEnvironmentVariable* var =
          gpGameState->SystemOptions().EnvVars().FindEnvironmentVariable("IngAttachedWarningCount");
      if (var != nullptr && var->GetValue() != var->GetMaximum()) {
        var->Set(var->GetValue() + 1);
        CSamusHud::DisplayHudMemo(rstl::wstring_l(gpStringTable->GetString("IngAttachedWarning")),
                                  CHUDMemoParms(5.f, true, false, false,
                                                1 << player->GetPlayerIndex(), true));
      }
      if (DebugIng()) {
        fprintf(stderr, "[ing] %04x suck: host %04x attached (mold %04x), %.1fs, %.1f dmg/s\n",
                GetUniqueId().Value(), mHostId.Value(), mPossessedEffectId.Value(),
                mData.mBodyProjectile.mSuckTime, mData.mBodyProjectile.mUnknownA0D6);
      }
    } else if (DebugIng()) {
      fprintf(stderr, "[ing] %04x suck: host %04x is not a player\n", GetUniqueId().Value(),
              mHostId.Value());
    }
    // 0x9ca (the swarm block's mSoundInsideHost) at kMaxVolume, pan 0x40; the handle replaces the
    // one-shot above, exactly as retail's dead store to 0xd00 does.
    mSuckSfx = CSfxManager::SfxStart(mData.mSwarm.mSoundInsideHost, CAudioSys::kMaxVolume, 0x40,
                                     GetCurrentAreaId().Value(), true, false,
                                     CSfxManager::kMedPriority); // 0xd00
    break;
  }
  case kStateMsg_Update: {
    if (StateMachineState().GetTime() > mData.mBodyProjectile.mSuckTime) { // 0x864
      mHostId = kInvalidUniqueId; // 0xcdc
      break;
    }
    CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mHostId));
    if (player == nullptr) {
      mHostId = kInvalidUniqueId;
      break;
    }
    SetTransform(player->GetTransform());             // 0x7b30
    SetTranslation(player->GetAimPosition(mgr, 0.f)); // vtable 0x54, rodata_6C = 0.0f
    if (DebugIng() && !mSuckTickLogged) {
      mSuckTickLogged = true;
      const CHealthInfo* hp = player->GetHealthInfo();
      fprintf(stderr, "[ing] %04x suck tick: riding host %04x (mold %04x), %.2f dmg/frame, host hp %.0f\n",
              GetUniqueId().Value(), mHostId.Value(), mPossessedEffectId.Value(),
              dt * mData.mBodyProjectile.mUnknownA0D6, hp != nullptr ? hp->GetHP() : -1.f);
    }
    // 0x848: mUnknownA0D6 a second, no immunity. Retail's weapon-mode word comes from uninitialised
    // stack with its low bits cleared; kWT_AI is the 0xb<<16 half it does set.
    const CDamageInfo damage(CWeaponMode(kWT_AI), dt * mData.mBodyProjectile.mUnknownA0D6, 0.f, 0.f,
                             true, false);
    // The asm builds one included material, 1 << 0x3b (kMT_Solid), type 3 = kFT_IncludeExclude.
    mgr.ApplyDamage(GetUniqueId(), mHostId, GetUniqueId(), damage, SolidFilter(),
                    GetTransform().GetForward());
    break;
  }
  case kStateMsg_Deactivate: {
    if (DebugIng()) {
      fprintf(stderr, "[ing] %04x suck end: releasing %04x\n", GetUniqueId().Value(),
              mTargetId.Value());
    }
    if (CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mTargetId))) {
      // 0x1144 bits 7/6 set again (asm 0x00007D74/0x00007D88): the player may morph once the Ing
      // lets go.
      player->EnableLeaveMorphBall(true);
      player->EnableEnterMorphBall(true);
      if (player->GetAttachedActorId() == GetUniqueId()) { // 0x2e4
        player->DetachActorFromPlayer();
      }
    }
    CSfxManager::RemoveEmitter(mSuckSfx); // 0xd00
    mSuckSfx = CSfxHandle();
    mHostId = kInvalidUniqueId; // 0xcdc
    break;
  }
  default:
    break;
  }
}

// fn_29_75C0 (not yet exercised in a run)
void CIng::SeekWallPoint(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("SeekWallPoint");
  } else if (msg != kStateMsg_Update) {
    return;
  }
  const CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId);
  if (hint == nullptr) {
    return; // 0x000075F8: a missing hint leaves mMoving as it was
  }
  const CVector3f delta = hint->GetTranslation() - GetTranslation();
  const float maxWall = mData.mIngSpot.GetMaxWallSpeed(); // 0x880
  if (!delta.IsMagnitudeSafe() || delta.Magnitude() <= dt * maxWall) {
    mMoving = false; // 0x0000768C: 0xe5d bit 1
  } else {
    MoveOnSurface(delta.AsNormalized(), maxWall, dt); // 0x00007674: the direction is normalised
    mMoving = true; // 0x00007678: 0xe5d bit 1
  }
}

// @ fn_29_7518
void CIng::SelectTarget(CStateManager& mgr, EStateMsg msg, float) {
  if (msg != kStateMsg_Activate) {
    return;
  }
  LogState("SelectTarget");
  if (mgr.GetObjectById(mTargetId) == nullptr) {
    if (const CPlayer* player = mgr.GetPlayer(0)) {
      mTargetId = player->GetUniqueId();
    }
  }
  mLineOfSight.SetTarget(mTargetId);
  // [?] retail sends script messages here; which state/message pair was not read.
}

// @ fn_29_7244
void CIng::FaceTarget(CStateManager& mgr, EStateMsg msg, float dt) {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (msg == kStateMsg_Activate) {
    LogState("FaceTarget");
    mAnimationState.SetState(target == nullptr || IsFacingTarget(mgr, CTriggerData(0.f))
                                 ? CAnimationState::kAS_Over
                                 : CAnimationState::kAS_Ready);
    mMoving = false;
  } else if (msg == kStateMsg_Update) {
    if (target == nullptr) {
      mAnimationState.SetState(CAnimationState::kAS_Over);
      return;
    }
    CVector3f delta = target->GetTranslation() - GetTranslation();
    delta = CVector3f(delta.GetX(), delta.GetY(), 0.f);
    if (mForm == kForm_IngSpot || (mForm >= 6 && mForm <= 9)) {
      if (IsFacingTarget(mgr, CTriggerData(0.f)) || !delta.IsMagnitudeSafe()) {
        mAnimationState.SetState(CAnimationState::kAS_Over);
      } else {
        BodyController()->FaceDirection(delta.AsNormalized(), dt);
      }
    } else if (mForm == kForm_Corporeal) {
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Turn)) {
        if (delta.IsMagnitudeSafe()) {
          BodyController()->CommandMgr().DeliverCmd(
              CBCLocomotionCmd(CVector3f::Zero(), delta.AsNormalized(), 1.f));
        } else {
          mAnimationState.SetState(CAnimationState::kAS_Over);
        }
      }
    } else {
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// @ fn_29_6DA0
// Retail keeps the four lowest-scoring candidates in a file-static array (REL bss lbl_29_bss_40:
// a count plus four {TUniqueId, float} entries), insertion-sorted ascending, then takes one entry
// at random through the state manager's RNG. Cover hints are types 3 and 4 (asm 0x00006F2C:
// cmpwi 0x3 then cmpwi 0x4) - the port had guessed 4 and 5, which matched nothing in the areas the
// Ings actually live in; type 4 (wall, the kind HasWallCoverPoint tests) additionally needs the
// body projectile to be allowed by the properties (0x9d0 bit 5) and the target outside a hurtful
// safe zone.
void CIng::FindCoverPoint(CStateManager& mgr, EStateMsg msg, float) {
  if (msg != kStateMsg_Activate) { // asm 0x00006DCC
    return;
  }
  LogState("FindCoverPoint");
  ReleaseCoverHint(mgr, mCoverHintId); // fn_29_4560
  if (!TargetAimPosition(mgr, mTargetPos)) { // fn_29_4468, 0xdc8; f1 = rodata_6C (0.0)
    return;
  }
  // The candidate array lives in the REL's bss (lbl_29_bss_40), so it is shared by every Ing and
  // cleared on each call (asm 0x00006E8C); function-local statics do the same here.
  struct SCandidate {
    SCandidate() : id(kInvalidUniqueId), score(0.f) {}
    TUniqueId id;
    float score;
  };
  static SCandidate sCandidates[4];
  static int sCandidateCount = 0;
  sCandidateCount = 0;
  // Near/far score bounds, each the max of two ranges, squared (asm 0x00006E98/0x00006EB0): f31
  // from mMinAttackRange (0x3e0) vs body-projectile min (0x85c), f30 from body-projectile max
  // (0x860) vs mini-portal max (0x934 = CIngData 0x7c0 + 0x174).
  const float nearBound = mMinAttackRange >= mData.mBodyProjectile.mMinAttackDist
                              ? mMinAttackRange
                              : mData.mBodyProjectile.mMinAttackDist;
  const float f31 = nearBound * nearBound;
  const float farBound = mData.mBodyProjectile.mMaxAttackDist >= mData.mMiniPortal.mMaxAttackDist
                             ? mData.mBodyProjectile.mMaxAttackDist
                             : mData.mMiniPortal.mMaxAttackDist;
  const float f30 = farBound * farBound;
  // Evaluated once before the loop (asm 0x00006EE8, fn_29_ABC8): the target actor exists and sits
  // in a hurtful safe zone.
  const bool targetInHurtful = TargetInLightSafeZone(mgr, CTriggerData(0.f));
  int scanned = 0;
  // Debug: why the scan comes up empty (the puddle then never leaves FindCoverPoint, and the
  // grapple behind IngSpotPathFind stays unreachable).
  int inactive = 0, wrongType = 0, inUse = 0, wrongArea = 0, lastUsed = 0, safeZone = 0;
  int spotGated = 0, hurtful = 0;
  int typeHist[32] = {0};
  const CObjectList& list = mgr.GetObjectListById(kOL_AiWaypoint); // mgr+0x838
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CScriptAIHint* hint = TCastToConstPtr< CScriptAIHint >(list[i]);
    ++scanned;
    if (hint != nullptr) {
      const int type = static_cast< int >(hint->GetHintType());
      if (type >= 0 && type < 32) {
        ++typeHist[type];
      }
    }
    if (hint == nullptr) {
      continue;
    }
    if (!hint->GetActive()) { // asm 0x00006F20: byte 0x20 bit 0
      ++inactive;
      continue;
    }
    const int type = static_cast< int >(hint->GetHintType());
    if (type != 3 && type != 4) { // asm 0x00006F2C
      ++wrongType;
      continue;
    }
    if (hint->GetInUse(GetUniqueId())) { // asm 0x00006F50
      ++inUse;
      continue;
    }
    if (hint->GetCurrentAreaId() != GetCurrentAreaId()) { // asm 0x00006F60, area word 0x4
      ++wrongArea;
      continue;
    }
    if (hint->GetUniqueId() == mLastCoverHintId) { // asm 0x00006F74, 0xd74
      ++lastUsed;
      continue;
    }
    if (mgr.GetSafeZoneManager()->PointIsInSafeZone(mgr, hint->GetTranslation())) { // 0x00006F8C
      ++safeZone;
      continue;
    }
    if (type == 4) { // asm 0x00007074: wall hints only
      // asm 0x00007080 reads 0x9d0 bit 5, which is CIngData's +0x210 flag byte (0x7c0 + 0x210),
      // not the 0xe5d bitfield: an Ing whose properties forbid the body projectile never takes a
      // wall point either.
      if ((mData.mFlags210 & kFlag_NoBodyProjectile) != 0) {
        ++spotGated;
        continue;
      }
      if (targetInHurtful) { // asm 0x0000708C
        ++hurtful;
        continue;
      }
    }
    const CVector3f delta = mTargetPos - hint->GetTranslation(); // 0xdc8 - hint+0x54
    float score = delta.MagSquared();
    const float up = CVector3f::Dot(CVector3f::Up(), delta); // == delta.GetZ(), asm 0x000070D0
    if (type != 4 || up > 0.f) { // asm 0x000070E4: wall hints at/below the target skip the term
      score += 4.f * up * up; // rodata_AC
    }
    if (score < f31) { // asm 0x00007108
      score += f30; // asm 0x00007110
    }
    // Same insertion shape as CMinorIng's copy of this routine: store at the end (or over the
    // worst entry when full), then bubble down to keep the array ascending (asm 0x00007040).
    int slot;
    if (sCandidateCount < 4) { // asm 0x00007040
      slot = sCandidateCount++;
    } else if (score < sCandidates[3].score) {
      slot = 3;
    } else {
      continue;
    }
    sCandidates[slot].id = hint->GetUniqueId();
    sCandidates[slot].score = score;
    for (; slot > 0 && sCandidates[slot].score < sCandidates[slot - 1].score; --slot) {
      const SCandidate swap = sCandidates[slot];
      sCandidates[slot] = sCandidates[slot - 1];
      sCandidates[slot - 1] = swap;
    }
  }
  TUniqueId picked = kInvalidUniqueId;
  if (sCandidateCount != 0) { // asm 0x00007148
    const int index = mgr.Random()->Range(0, sCandidateCount - 1); // asm 0x00007234, mgr+0x16e4
    picked = sCandidates[index].id;
  }
  if (CScriptAIHint* hint = GetCoverHint(mgr, picked)) {
    UseCoverHint(*hint); // fn_29_4510
  }
  if (DebugIng()) {
    fprintf(stderr,
            "[ing] %04x cover scan: candidates %d, pick %04x: inactive %d wrongtype %d inuse %d "
            "area %d last %d safezone %d wallspot %d hurtful %d; types",
            GetUniqueId().Value(), scanned, picked.Value(), inactive, wrongType, inUse, wrongArea,
            lastUsed, safeZone, spotGated, hurtful);
    for (int t = 0; t < 32; ++t) {
      if (typeHist[t] != 0) {
        fprintf(stderr, " %d:%d", t, typeHist[t]);
      }
    }
    fprintf(stderr, "\n");
  }
}

// @ fn_29_3E1C: the segment test of one arm. Four boxes along the arm (shoulder-elbow,
// elbow-forearm, forearm-wrist, then the wrist extended six units along the forearm) are tested
// against the target's bounding box; retail's chained early-outs are one OR. A hit deals the arm
// swipe damage with the given knock-back and clears the contact-damage pause.
void CIng::ArmSwipeSegment(CStateManager& mgr, const CSegId& a, const CSegId& b, const CSegId& c,
                           const CSegId& d, const CVector3f& knockback) {
  if (!xE5C_1) { // 0xe5c bit 1: only while the swing is live
    return;
  }
  CPhysicsActor* target = TCastToPtr< CPhysicsActor >(mgr.ObjectById(mTargetId));
  if (target == nullptr) {
    return;
  }
  const CAABox bounds = target->GetBoundingBox();
  const CVector3f p0 = GetLctrTransform(a).GetTranslation();
  const CVector3f p1 = GetLctrTransform(b).GetTranslation();
  const CVector3f p2 = GetLctrTransform(c).GetTranslation();
  const CVector3f p3 = GetLctrTransform(d).GetTranslation();
  bool hit = bounds.DoBoundsOverlap(PortSegmentBox(p0, p1)) ||
             bounds.DoBoundsOverlap(PortSegmentBox(p1, p2)) ||
             bounds.DoBoundsOverlap(PortSegmentBox(p2, p3));
  if (!hit) {
    const CVector3f delta = p3 - p2;
    if (!delta.IsMagnitudeSafe()) { // retail gives up here rather than normalising
      return;
    }
    hit = bounds.DoBoundsOverlap(PortSegmentBox(p3, p3 + delta.AsNormalized() * 6.f));
    if (!hit) {
      return;
    }
  }
  if (DebugIng()) {
    fprintf(stderr, "[ing] %04x arm swipe hit: arm %d (%02x..%02x), target %04x, damage %.1f\n",
            GetUniqueId().Value(), mSwipeArm, static_cast< int >(a.val()),
            static_cast< int >(d.val()), mTargetId.Value(),
            mData.mArmSwipeDamage.GetDamage());
  }
  // rodata A10 = 0x3B (kMT_Solid) and the filter's type word 3 = kFT_IncludeExclude.
  mgr.ApplyDamage(GetUniqueId(), mTargetId, GetUniqueId(), mData.mArmSwipeDamage,
                  CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList()),
                  knockback);
  mCurDamageRemTime = mDamageWaitTime; // 0x440 = 0x444
  xE5C_1 = false;                      // 0xe5c bit 1
}

// @ fn_29_6A88
void CIng::ArmSwipe(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("ArmSwipe");
    if (!TargetAimPosition(mgr, mTargetPos)) { // fn_29_4468, 0xdc8
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
      break;
    }
    if (mTeamId != kInvalidUniqueId &&
        !CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Melee, mgr, mTeamId, GetUniqueId())) {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
      break;
    }
    mFormTime += mData.mUnknownC620;                       // 0xe3c += 0x7c8
    mAnimationState.SetState(CAnimationState::kAS_Ready);  // 0x6b4 = 1
    break;
  case kStateMsg_Update:
    // CTryCommand::ShouldRetry(this+0x6b4, *BodyController(), 7); 7 is pas::kAS_MeleeAttack.
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_MeleeAttack)) {
      mSwipeArm = PortPickSwipeArm(mgr) ? 1 : 0; // fn_29_421C, 0xdec
      // rodata_38 = {1, 2}: the melee attack's severity, indexed by the arm.
      BodyController()->CommandMgr().DeliverCmd(
          CBCMeleeAttackCmd(mSwipeArm == 0 ? pas::kS_One : pas::kS_Two));
      xE5C_1 = true; // 0xe5c bit 1: the contact damage pauses during the swing
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_MeleeAttack) {
      // The knock-back points sideways: the right arm pushes along -right, the left along +right
      // (retail reads m00/m10/m20, i.e. CTransform4f::GetRight()).
      const CVector3f right = GetTransform().GetRight();
      ArmSwipeSegment(mgr, mRightShoulderSegId, mRightElbowSegId, mRightForearmSegId,
                      mRightWristSegId, CVector3f::Zero() - right); // 0xd7c..0xd7f
      if (mSwipeArm == 1) {
        ArmSwipeSegment(mgr, mLeftShoulderSegId, mLeftElbowSegId, mLeftForearmSegId,
                        mLeftWristSegId, right); // 0xd80..0xd83
      }
    }
    break;
  case kStateMsg_Deactivate:
    mSwipeArm = -1;                                        // 0xdec
    xE5C_1 = false;                                        // 0xe5c bit 1
    mArmSwipeTime = 0.f;                                   // 0xe38
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    CScriptTeamAiMgr::EndAttack(CScriptTeamAiMgr::kAT_Melee, mgr, mTeamId, GetUniqueId(), false);
    break;
  default:
    break;
  }
}

// @ fn_29_69A0: the puddle is off the navmesh. Activate sets kAS_Ready, teleports the actor onto
// the closest physical point within 1000 (rodata_C4) and re-aligns to the surface with 1.0
// (rodata_64), then sets kAS_Over; Deactivate sets kAS_NotReady. Update does nothing (asm
// 0x00006A70 is the common tail).
void CIng::FailSafeMode(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("FailSafeMode");
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    int idx = -1;
    const CVector3f before = GetTranslation();
    CPathFindPointSearch::EClosestPointResult res = CPathFindPointSearch::kCPR_NoPoint;
    int numPoints = 0;
    // Port limit: an area with no path area leaves the host's CPFArea pointer null; the REL's search
    // always has one, so the guard only keeps the host's null area from being dereferenced.
    if (mPathArea != nullptr) {
      numPoints = mPathArea->GetNumPoints();
      res = mPointPathFindSearch.FindClosestPhysicalPoint(before, idx,
                                                          CPathFindPointSearchFilter(1000.f, 0, -1));
    }
    if (DebugIng()) {
      fprintf(stderr, "[ing] %04x failsafe: points %d res %d idx %d at (%.1f, %.1f, %.1f)\n",
              GetUniqueId().Value(), numPoints, static_cast< int >(res), idx, before.GetX(),
              before.GetY(), before.GetZ());
    }
    if (res == CPathFindPointSearch::kCPR_Success && idx != -1) {
      SetTranslation(mPathArea->GetPoint(idx).GetPosition());
      const CVector3f moved = GetTranslation();
      mSurfaceAlignment.AlignNearPosition(*this, mgr, GetTranslation(), 1.f);
      if (DebugIng()) {
        // Is the point the fail-safe picked on a walkable region at all? IsOffPath asks the ground
        // search, so this says why the Ing can stay off the path after the teleport.
        float height = 0.f;
        const bool onMesh =
            mPathFindSearch.GetHeightOfPointAboveMesh(moved, height, 0.f) == CPathFindSearch::kR_Success;
        const bool near1 = mPathFindSearch.NearlyOnPath(moved, 1.f) == CPathFindSearch::kR_Success;
        const bool near5 = mPathFindSearch.NearlyOnPath(moved, 5.f) == CPathFindSearch::kR_Success;
        fprintf(stderr,
                "[ing] %04x failsafe: moved %.2f to (%.1f, %.1f, %.1f), after align (%.1f, %.1f, %.1f)"
                " onmesh %d near1 %d near5 %d height %.2f\n",
                GetUniqueId().Value(), (moved - before).Magnitude(), moved.GetX(), moved.GetY(),
                moved.GetZ(), GetTranslation().GetX(), GetTranslation().GetY(), GetTranslation().GetZ(),
                onMesh ? 1 : 0, near1 ? 1 : 0, near5 ? 1 : 0, height);
      }
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// @ fn_29_6578: retail holds the morphed player for as long as the grapple lives. Exercised in
// Agon's 01_sand_entrance_dark (/tmp/opencode/ing_gr8.log): `grapple range ... overlap 1 -> 1` ->
// `state GrappleBall form 3` -> the ball is dragged from 0.5 to 0.0 units away while 5 damage a
// second lands and the hit reaction ramps 0.30 a second to 1.00 -> `state ExpelGrappledBall` at the
// 5 s hold time. Enter grabs the ball (attach, morph lock, looping voice), Update drags the Ing onto
// the ball at the ing-spot wall speed while draining mUnknown67F6 damage a second (no immunity,
// solid filter) and returns kAS_Over once the hold time runs out or a hit reaction (mHitReactionTime
// < 0) ends it, and Exit releases the ball, stops the voice and arms the grapple cooldown.
void CIng::GrappleBall(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    LogState("GrappleBall");
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6b4 = 1
    mGrappled = true;                                     // 0xe5d bit 0
    mHitReactionTime = 0.f;                               // 0xe4c
    CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mTargetId));
    if (player != nullptr) {
      // 0x1144 bits 7/6 (asm 0x00006610): no morph transition of either kind while the ball is
      // held, then the Ing rides the ball.
      player->EnableLeaveMorphBall(false);
      player->EnableEnterMorphBall(false);
      player->AttachActorToPlayer(GetUniqueId(), false);
    }
    // 0x9ca: the fourth of the swarm block's u16 sounds, which this header calls mSoundInsideHost.
    mGrappleSfx = CSfxManager::AddEmitter(mData.mSwarm.mSoundInsideHost, GetTranslation(), 0x7f,
                                          GetCurrentAreaId().Value(), true, true,
                                          CSfxManager::kMedPriority); // 0xd04
    mMoving = false;                                              // 0xe5d bit 1
    break;
  }
  case kStateMsg_Update: {
    // The voice follows the Ing: its position, its forward as the emitter's direction.
    CSfxManager::UpdateEmitter(mGrappleSfx, GetTranslation(), GetTransform().GetForward(), 0x7f);
    if (StateMachineState().GetTime() > mData.mGrapple.mMaxHoldTime || // 0x8f4
        mHitReactionTime < 0.f) {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
      break;
    }
    CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mTargetId));
    if (player == nullptr ||
        player->GetSpawnedMorphballState() != CPlayer::kMS_Unmorphed || // 0x390
        player->GetMorphballTransitionState() != CPlayer::kMS_Morphed) { // 0x38c
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
      break;
    }
    player->Stop();
    const CVector3f delta = player->GetTranslation() - GetTranslation();
    if (!delta.IsMagnitudeSafe()) {
      Stop();
    } else {
      MoveOnSurface(delta.AsNormalized(), mData.mIngSpot.GetMaxWallSpeed(), dt); // fn_29_3948, 0x880
    }
    mgr.ApplyDamage(GetUniqueId(), mTargetId, GetUniqueId(),
                    CDamageInfo(CWeaponMode(kWT_AI), dt * mData.mGrapple.mUnknown67F6, 0.f, 0.f,
                                true, false), // 0x8cc, no immunity
                    SolidFilter(), CVector3f::Zero());
    static int sTick = 0;
    if (IngLogTick(sTick)) {
      fprintf(stderr,
              "[ing] %04x grapple: held %.2f/%.2f s, reaction %.2f, %.1f dmg/s, ball %.1f away\n",
              GetUniqueId().Value(), StateMachineState().GetTime(), mData.mGrapple.mMaxHoldTime,
              mHitReactionTime, mData.mGrapple.mUnknown67F6, delta.Magnitude());
    }
    const float drain = mHitReactionTime + 0.3f * dt; // reaction ramp, rodata_FC
    mHitReactionTime = drain > 1.f ? 1.f : drain;    // 0xe4c
    break;
  }
  case kStateMsg_Deactivate: {
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    mGrappled = false;                                       // 0xe5d bit 0
    mBallCooldown = mData.mGrapple.mPostWaitTime;            // 0xe48 = 0x8f8
    CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mTargetId));
    if (player != nullptr) {
      // Both bits back on (asm 0x0000692C).
      player->EnableLeaveMorphBall(true);
      player->EnableEnterMorphBall(true);
      if (player->GetAttachedActorId() == GetUniqueId()) { // 0x2e4
        player->DetachActorFromPlayer();
      }
    }
    CSfxManager::RemoveEmitter(mGrappleSfx); // 0xd04
    mGrappleSfx = CSfxHandle();
    break;
  }
  default:
    break;
  }
}

// @ fn_29_61B4: two rays a frame, out from the head locator to the
// target's aim point, until three hits are stored or the shared angle index runs out (the ring is
// eight angles, so a FindMiniPortals entry never searches more than four frames). The filter and
// the geometry are the Space Jump Guardian's (fn_29_1E6C's FindMiniPortals), which is verified.
void CIng::FindMiniPortals(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("FindMiniPortals");
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6b4 = 1
    mMiniPortalCount = 0;                                 // 0xe00 = 0
    sMiniPortalAngleIndex = 0;
    // fn_29_4468: no target, no portals - the state is over at once.
    if (!TargetAimPosition(mgr, mTargetPos)) {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
    }
    break;
  case kStateMsg_Update: {
    if (mMiniPortalCount >= 3 || sMiniPortalAngleIndex >= 8) {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
      break;
    }
    // The ray runs from 3 model-scales in front of the head locator, plus the ring's offset.
    const CVector3f origin = GetLctrTransform(mEyeSegId).GetTranslation(); // 0xd7b
    const CVector3f start = origin + mModelScale.GetY() * (3.f * GetTransform().GetForward());
    const CMaterialFilter filter = CMaterialFilter::MakeIncludeExclude(
        CMaterialList(kMT_Solid),
        CMaterialList(kMT_Character, kMT_Player, kMT_CollisionActor, kMT_ProjectilePassthrough));
    for (int i = 0; i < 2; ++i) {
      // Retail takes the pair's angle without re-testing the bound, so its last pair reads one
      // float past the table when the index is already 8 (the bss guard is tested once, above).
      if (sMiniPortalAngleIndex >= 8) {
        break;
      }
      const float angle = skMiniPortalAngles[sMiniPortalAngleIndex++] * 0.017453292f;
      const float c = CMath::FastCosR(angle);
      const float s = CMath::FastSinR(angle);
      const CVector3f offset =
          2.f * CVector3f(mModelScale.GetX() * c, mModelScale.GetY() * 0.f, mModelScale.GetZ() * s);
      const CVector3f end = start + GetTransform().Rotate(offset);
      if (mgr.RayCollideWorld(end, mTargetPos, filter, this)) { // 0xdc8
        mMiniPortalPositions[mMiniPortalCount++] = end; // 0xe04 + 12 * count
        if (mMiniPortalCount == 3) {
          mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
          return;
        }
      }
    }
    break;
  }
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    break;
  default:
    break;
  }
}

// @ fn_29_6008: asks the body for one projectile attack; the animation's
// kUE_Projectile events then spawn the portals FindMiniPortals queued (fn_29_C228).
void CIng::MiniPortalAttack(CStateManager&, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("MiniPortalAttack");
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6b4 = 1
    mMiniPortalIndex = 0;                                 // 0xe28 = 0
    break;
  case kStateMsg_Update:
    // ShouldRetry(0x6b4, body, 0x12): 0x12 is pas::kAS_ProjectileAttack.
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_ProjectileAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCProjectileAttackCmd(
          pas::kS_One, GetTranslation() + GetTransform().GetForward(), false));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    mMiniPortalIndex = -1;                                   // 0xe28 = -1
    break;
  default:
    break;
  }
}

// @ fn_29_5CB0: the Ing dies. Exercised in Agon 01_sand_entrance_dark
// (/tmp/opencode/ing_fs5.log, 2026-10-20): the console's `damage <uid> 100` three times took the
// Ing's 300 HP to zero and the state ran (three "state Dead form 3" entries) - the ing-spot
// branch, the one that drops the target and orbit materials, explodes the death effect and plays
// the death sound. The corporeal branch (the burn) wants the Ing killed while it is still form 2.
void CIng::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("Dead");
  }
  if (mForm == 0 || mForm == 1) {
    if (msg == kStateMsg_Activate) {
      DeathDelete(mgr);
    }
    return;
  }
  // 0x00005CFC: the ing spot, the body projectile and the evaporating form die on the Activate
  // message: the target and orbit materials go, the death effect explodes, the death sound plays.
  if (mForm == kForm_IngSpot || mForm == kForm_BodyProjectile || mForm == kForm_Evaporate) {
    if (msg != kStateMsg_Activate) {
      return;
    }
    RemoveMaterial(kMT_Target, kMT_Orbit, mgr); // 0x00005D18: 0x28/0x29
    if (mDeathEffect) {
      mgr.AddObject(rs_new CExplosion(
          *mDeathEffect, mgr.AllocateUniqueId(),
          CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId),
          rstl::string_l("Ing death Fx"), GetTransform(), 0, mModelScale, CColor::White(), -1));
    }
    // 0x00005E68: AddEmitter(sfx, position, 0x7f, area, true, false, kMedPriority)
    CSfxManager::AddEmitter(mData.mIngSpot.GetDeathSound(), GetTranslation(), 0x7f,
                            GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
    DeathDelete(mgr);
    return;
  }
  // 0x00005E8C: the corporeal form (2) and the form changes (6 and up) burn away; the base class
  // gets its own death message first.
  CPatterned::Dead(mgr, msg, dt);
  // 0x00005EA4/0x00005EAC: two of CPatterned's flags skip the burn: the byte at 0x421's bits 1 and
  // 2, which are the host's mBurning and mLaggedBurnDeath. The Ing's own death handler tests the
  // same pair at 0x0000B478 before it clears the vertical movement bit, and so does the host's
  // CPatterned death path (src/MetroidPrime/Enemies/CPatterned.cpp:433).
  if (mBurning || mLaggedBurnDeath) {
    return;
  }
  if (msg == kStateMsg_Activate) {
    mDeadFade = 2.f; // 0x00005EE4: rodata_74
    RemoveMaterial(kMT_Target, kMT_Orbit, mgr); // 0x00005EE8: 0x28/0x29
    mDamageCooldownTimer = CPatterned::skDamageHitTime; // 0x00005EF4: 0x448
  } else if (msg == kStateMsg_Update) {
    mAlphaDelta = 0.f; // 0x00005F14: 0x40c
    mSurfaceAlignment.AlignNearPosition(*this, mgr, GetTranslation(), dt); // 0x00005F1C
    if (BodyController()->GetCurrentStateId() == pas::kAS_Death) { // 0x00005F28
      const CVector3f scale = GetModelData()->GetScale();
      const float z = scale.GetZ() - dt / 0.75f; // rodata_F4
      // The scale is always written, its z clamped at zero (0x00005F74).
      ModelData()->SetScale(CVector3f(scale.GetX(), scale.GetY(), z > 0.f ? z : 0.f));
      if (z <= 0.f) {
        mDeadFade -= dt; // 0x00005FA0
        mRenderModel = false; // 0x00005FAC: 0xe5d bit 7
        mStartAsIngSpot = false; // 0x00005FB8: 0xe5d bit 5
      }
      if (mDeadFade <= 0.f) { // 0x00005FCC
        DeathDelete(mgr);
      }
    }
  }
}

// fn_29_5C28 (not yet exercised in a run)
void CIng::Evaporate(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("Evaporate");
    mForm = kForm_Evaporate; // 0x00005C5C: 0x9d4 = 5
  } else if (msg == kStateMsg_Update) {
    // 0x00005C68: the state time (the state machine at 0x350) past two seconds (rodata_74).
    if (mStateTime > 2.f) {
      DeathDelete(mgr);
    }
  }
}

// ---- code functions (the FSM2 runs them: CGenericFSM2State::SetCodeFunctions) -----------

// @ fn_29_5B60: SetCoverDest's shape with the target - the Ing's own position unless the target
// exists (no type check, asm 0x00005BB4), then the face target at kInvalidUniqueId and the team.
void CIng::SetTargetDest(CStateManager& mgr, float) {
  LogState("SetTargetDest");
  CVector3f dest = GetTranslation(); // asm 0x00005B84
  mFoundMovementPos = false;         // asm 0x00005BA4
  if (const CEntity* target = mgr.GetObjectById(mTargetId)) {        // asm 0x00005BB4
    dest = static_cast< const CActor* >(target)->GetTranslation(); // asm 0x00005BC0
    mFoundMovementPos = true;                                        // asm 0x00005BE0
  }
  mPathFindNavigation.SetDestination(dest);            // asm 0x00005BF0
  mPathFindNavigation.SetFaceTarget(kInvalidUniqueId); // asm 0x00005C08: 0x718
  JoinTeam(mgr);                                       // fn_29_372C
}

// @ fn_29_5AC4: destination = the cover hint's translation when there is one, else the Ing's own
// position; mFoundMovementPos only in the first case; then SetDestination and the face target at
// kInvalidUniqueId (0x718), asm 0x00005B00-0x00005B48.
void CIng::SetCoverDest(CStateManager& mgr, float) {
  LogState("SetCoverDest");
  CVector3f dest = GetTranslation();
  mFoundMovementPos = false;
  if (const CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId)) {
    dest = hint->GetTranslation();
    mFoundMovementPos = true;
  }
  mPathFindNavigation.SetDestination(dest);
  mPathFindNavigation.SetFaceTarget(kInvalidUniqueId);
}

// @ fn_29_59D0: two closest-point queries from the Ing's own position - filter (20, 0, -1) then
// (20, 1, from), asm 0x000059E0 and 0x00005A44 - and a point-graph search between the two results
// (0x00005A94). The second filter's connected point is the *first* query's index (the asm keeps it
// in r5 from 0x00005A2C), so the second point is one the graph connects to the first - without
// that the pair is unrelated and the search below fails. It sets no destination:
// mFoundMovementPos is what IngSpotPathFind reads.
void CIng::SetRecoverDest(CStateManager&, float) {
  LogState("SetRecoverDest");
  mFoundMovementPos = false;
  mPointPathFindSearch.ClearWaypoints(); // 0xacc = 0, asm 0x00005A08
  int from = -1;
  if (mPointPathFindSearch.FindClosestPhysicalPoint(GetTranslation(), from,
                                                    CPathFindPointSearchFilter(20.f, 0, -1)) !=
          CPathFindPointSearch::kCPR_Success ||
      from == -1) {
    return;
  }
  int to = -1;
  if (mPointPathFindSearch.FindClosestPhysicalPoint(GetTranslation(), to,
                                                    CPathFindPointSearchFilter(20.f, 1, from)) !=
          CPathFindPointSearch::kCPR_Success ||
      to == -1) {
    return;
  }
  if (mPathArea != nullptr && PortIngPointSearch(mPointPathFindSearch, *mPathArea, from, to)) {
    mFoundMovementPos = true;
  }
}

// @ fn_29_5820: the point-graph route to the cover hint. Filter (20, 0, -1) from the Ing's own
// position, then the nearest flag-1 point that the graph connects to it, sits under 400
// (rodata_F0) from the hint and has a mesh path to it (asm 0x00005844-0x00005998), then the point
// search between the two.
void CIng::SetPointCoverDest(CStateManager& mgr, float) {
  LogState("SetPointCoverDest");
  mFoundMovementPos = false;
  mPointPathFindSearch.ClearWaypoints();
  const CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId);
  if (hint == nullptr || mPathArea == nullptr) {
    return;
  }
  int from = -1;
  mPointPathFindSearch.FindClosestPhysicalPoint(GetTranslation(), from,
                                                CPathFindPointSearchFilter(20.f, 0, -1));
  if (from == -1) {
    return;
  }
  const CVector3f cover = hint->GetTranslation();
  float bestDistSq = 400.f; // rodata_F0
  int best = -1;
  for (int i = 0; i < mPathArea->GetNumPoints(); ++i) {
    const CPFPoint& point = mPathArea->GetPoint(i);
    // asm 0x000058E0: only points whose flags word has bit 0 set are cover points.
    if ((point.GetFlags() & 1) == 0 || !mPathArea->PointPathExists(from, i)) {
      continue;
    }
    const float distSq = (cover - point.GetPosition()).MagSquared();
    if (distSq < bestDistSq &&
        mPathFindSearch.PathExists(point.GetPosition(), cover) == CPathFindSearch::kR_Success) {
      best = i;
      bestDistSq = distSq;
    }
  }
  if (best == -1) {
    return;
  }
  if (PortIngPointSearch(mPointPathFindSearch, *mPathArea, from, best)) {
    mFoundMovementPos = true;
  }
}

// fn_29_5714 (not yet exercised in a run): the point-graph route to the cover hint. Two closest-point
// queries - the Ing's own position with filter (20, 0, -1) (asm 0x00005758-0x00005768), then the
// hint's position with filter (20, 0, from) (asm 0x00005778-0x000057A0) - and a point-graph search
// between the two results (asm 0x000057CC-0x000057EC). It sets no destination: mFoundMovementPos is
// what IngSpotPathFind reads.
void CIng::SetWallPointCoverDest(CStateManager& mgr, float) {
  LogState("SetWallPointCoverDest");
  mFoundMovementPos = false;
  mPointPathFindSearch.ClearWaypoints(); // 0xacc = 0, asm 0x0000573C
  const CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId); // fn_29_45BC, id at 0xd72
  // Retail tests the hint only (fn_29_45BC returns null when the id is not a CScriptAIHint); the
  // host can run without a path area, so the point queries are skipped when mPathArea is null.
  if (hint == nullptr || mPathArea == nullptr) {
    return;
  }
  int from = -1;
  mPointPathFindSearch.FindClosestPhysicalPoint(GetTranslation(), from,
                                                CPathFindPointSearchFilter(20.f, 0, -1));
  if (from == -1) {
    return;
  }
  int to = -1;
  // asm 0x00005778: the second filter's connected point is the index the first query found, so only
  // points the graph connects to the Ing's own point are candidates.
  mPointPathFindSearch.FindClosestPhysicalPoint(hint->GetTranslation(), to,
                                                CPathFindPointSearchFilter(20.f, 0, from));
  if (to != -1 && PortIngPointSearch(mPointPathFindSearch, *mPathArea, from, to)) {
    mFoundMovementPos = true;
  }
}

// @ fn_29_54E8. Where the exit host lands: the AI hint that best matches
// the way the player is facing. Retail starts from the player's own position (uid +8, transform
// +0x24) and takes mgr+0x14fc as the player with no null test, which the port keeps as a guard.
// The score is (GetAngleDiff(forward, hint - Ing) + 1) * |hint - Ing|^2, and the comparison pair
// keeps a 400.0f (rodata_F0, i.e. 20 units squared) petal around zero: under it the *smaller*
// non-negative score wins, over it the larger one does. Retail writes mTargetPos (0xdc8) and
// nothing else - in particular it does not touch mFoundMovementPos, so neither does this.
void CIng::SetExitHostDest(CStateManager& mgr, float) {
  LogState("SetExitHostDest");
  const CPlayer* player = mgr.GetPlayer(0); // mgr+0x14fc
  if (player == nullptr) {
    return;
  }
  const CVector3f forward = player->GetTransform().GetForward();
  float best = 0.f; // rodata_6C
  CVector3f bestPos = player->GetTranslation();
  CObjectList& list = mgr.ObjectListById(kOL_AiWaypoint);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    CScriptAIHint* hint = TCastToPtr< CScriptAIHint >(list[i]);
    // Retail compares the object's own area word against the Ing's (there is no active/in-use test)
    // and skips hint type 4, a type with no name in CScriptAIHint::EHintType.
    if (hint == nullptr || hint->GetCurrentAreaId() != GetCurrentAreaId() ||
        static_cast< int >(hint->GetHintType()) == 4) {
      continue;
    }
    const CVector3f delta = hint->GetTranslation() - GetTranslation();
    const float score = (CVector3f::GetAngleDiff(forward, delta) + 1.f) * delta.MagSquared();
    if ((score < best && score >= 400.f) || (score > best && best <= 400.f)) {
      best = score;
      bestPos = hint->GetTranslation();
    }
  }
  mTargetPos = bestPos; // 0xdc8
}

// fn_29_53E0 (not yet exercised in a run): the lure point is the nav-mesh point below the lure safe
// zone (fn_29_1C98, the shared PortIngLuredDest does it); the path search's flags go to 1 for the
// mesh-path query and back to 0x301 after it (asm 0x00005458-0x000054B8). Only when that path
// exists does the lure point become the destination and mFoundMovementPos get set (0xe5c bit 3);
// otherwise the destination stays the Ing's own position (asm 0x00005414-0x000054C4).
void CIng::SetLuredDest(CStateManager& mgr, float) {
  LogState("SetLuredDest");
  CVector3f dest;
  mFoundMovementPos =
      PortIngLuredDest(mgr, mPathFindSearch, mLureSafeZoneId, GetTranslation(), dest);
  mPathFindNavigation.SetDestination(dest);
  mPathFindNavigation.SetFaceTarget(kInvalidUniqueId); // 0x718, asm 0x000054C8
}

// fn_29_51D0 (not yet exercised in a run): the point-graph route to the lure point - the flag-1
// point nearest the Ing that the graph connects to the Ing's own point and that has a mesh path to
// the lure point (asm 0x000052BC-0x0000531C), then the point search between the two (asm
// 0x00005350-0x0000536C). The shared PortIngLuredPointDest is the same routine as the MinorIng's
// copy (fn_44_22B0), whose asm matches this one field for field.
void CIng::SetLuredPointDest(CStateManager& mgr, float) {
  LogState("SetLuredPointDest");
  mFoundMovementPos = PortIngLuredPointDest(mgr, mPathFindSearch, mPointPathFindSearch, mPathArea,
                                            mLureSafeZoneId, GetTranslation());
}

// fn_29_5190 (not yet exercised in a run): the Ing starts a projectile attack on its own team: the
// attack type 1 (kAT_Projectile), the team id at 0xd6a and the Ing's own id (asm 0x000051A4-0x000051B8).
void CIng::StartRangedAttack(CStateManager& mgr, float) {
  CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId, GetUniqueId());
}

// fn_29_514C (not yet exercised in a run): the team attack ends, but the Ing keeps its team role:
// retail's last argument to EndAttack is 0 (asm 0x00005154), not a role clear.
void CIng::EndRangedAttack(CStateManager& mgr, float) {
  CScriptTeamAiMgr::EndAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId, GetUniqueId(),
                              false);
}

// @ fn_29_4F14: the body projectile hit the mesh, so the Ing splats back
// into its ing-spot form: form 3, the scripted body state aborted, the splat-wall sound once at the
// Ing's own position (its handle is discarded), then - when the splat token at 0xd48 is engaged - an
// explosion on the Ing's transform at the model's scale ("Ing Splat Wall Fx").
void CIng::SplatOntoMesh(CStateManager& mgr, float) {
  LogState("SplatOntoMesh");
  mForm = kForm_IngSpot; // 0x9d4 = 3
  BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_AbortScripted)); // id word 13
  // The normal comes from the surface the body projectile hit (0xdd4/0xdd8/0xddc, written by
  // fn_29_B534's CollidedWith); it is zero until then. rodata_64 is 1.0f.
  mSurfaceAlignment.OrientToSurfaceNormal(*this, mSurfaceNormal, 1.f);
  // 0x852: the body projectile's splat-wall sound. Retail stores no handle, so neither do we.
  CSfxManager::AddEmitter(mData.mBodyProjectile.mSoundSplatWall, GetTranslation(), 0x7f,
                          GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
  const bool splatFx = static_cast< bool >(mSplatEffect); // 0xd54, the token's engaged byte
  if (DebugIng()) {
    fprintf(stderr, "[ing] %04x splat: splat fx %s\n", GetUniqueId().Value(),
            splatFx ? "explosion" : "token not loaded");
  }
  if (splatFx) {
    mgr.AddObject(rs_new CExplosion(*mSplatEffect, mgr.AllocateUniqueId(),
                                    CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList,
                                                true, kInvalidEditorId),
                                    rstl::string_l("Ing Splat Wall Fx"), GetTransform(), 0,
                                    GetModelData()->GetScale(), CColor::White(), -1));
  }
}

// @ fn_29_4D18: the grapple's expiry, reached in the same run (`expel: hit reaction 1.00, impulse
// 6000.0, dir (0.11, -0.32, 0.94)` - 30 spit force times the player's 200 mass, up the surface
// normal slerped 20 degrees toward the Ing). The grapple's expiry: one last mExitGrappleDamage on the
// held ball (unless a hit reaction already cancelled it), then the ball is spat away - up the
// surface normal slerped 20 degrees (rodata_E8) toward the Ing's forward, times mass * mSpitForce
// so its speed is the property's - and the reaction timer resets.
void CIng::ExpelGrappledBall(CStateManager& mgr, float) {
  LogState("ExpelGrappledBall");
  CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mTargetId));
  if (player != nullptr && player->GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed &&
      player->GetMorphballTransitionState() == CPlayer::kMS_Morphed) {
    if (mHitReactionTime >= 0.f) { // 0xe4c
      mgr.ApplyDamage(GetUniqueId(), mTargetId, GetUniqueId(), mData.mGrapple.mExitGrappleDamage,
                      SolidFilter(), CVector3f::Zero()); // 0x8d0
      mCurDamageRemTime = mDamageWaitTime;               // 0x440 = 0x444
    }
    player->Stop();
    const CVector3f dir = CVector3f::Slerp(CVector3f::Up(), GetTransform().GetForward(),
                                           CRelAngle::FromRadians(0.34906584f)); // rodata_E8
    player->ApplyImpulseWR(dir * (mData.mGrapple.mSpitForce * player->GetMass()), // 0x8ec, 0x158
                           CAxisAngle::Identity());
    player->SetMoveState(NPlayer::kMS_ApplyJump, mgr);
    CSfxManager::AddEmitter(mData.mGrapple.mSoundExitGrapple, GetTranslation(), 0x7f,
                            GetCurrentAreaId().Value(), true, false,
                            CSfxManager::kMedPriority); // 0x8f0
    if (DebugIng()) {
      fprintf(stderr, "[ing] %04x expel: hit reaction %.2f, impulse %.1f, dir (%.2f, %.2f, %.2f)\n",
              GetUniqueId().Value(), mHitReactionTime, mData.mGrapple.mSpitForce * player->GetMass(),
              dir.GetX(), dir.GetY(), dir.GetZ());
    }
  }
  mHitReactionTime = 0.f; // 0xe4c
  mMoving = false;        // 0xe5d bit 1
}

// fn_29_3C58: the touch box is a fixed 5x5x1.5 box on the Ing's position, neither scaled nor
// rotated (rodata_CC -2.5, rodata_C8 2.5, rodata_6C 0, rodata_8C 1.5; asm 0x00003C88-0x00003CE4).
void CIng::UpdateTouchBox() {
  const CVector3f pos = GetTranslation();
  mTouchBox = CAABox(pos + CVector3f(-2.5f, -2.5f, 0.f), pos + CVector3f(2.5f, 2.5f, 1.5f));
}

// fn_29_1C2C: the lure zone is dropped unless it is an active safe zone of type 2 (0x3f0); no
// kInvalidUniqueId guard, the lookup fails for it (asm 0x00001C50-0x00001C80).
void CIng::ValidateLureZone(CStateManager& mgr) {
  const CScriptSafeZone* zone =
      TCastToConstPtr< CScriptSafeZone >(mgr.GetObjectById(mLureSafeZoneId));
  if (zone == nullptr || !zone->GetActive() ||
      zone->GetZoneType() != static_cast< CScriptSafeZone::EZoneType >(2)) {
    mLureSafeZoneId = kInvalidUniqueId;
  }
}

// ---- overrides ------------------------------------------------------------------------------

// @ fn_29_C814
void CIng::PreThink(float dt, CStateManager& mgr) {
  EnsureBodyActive(mgr);
  CPatterned::PreThink(dt, mgr);
}

// @ fn_29_C654
void CIng::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  // fn_29_3B50 (asm 0x3B7C-0x3BA8): 0xe40 is the time the body has spent in the locomotion state
  // (the body controller's state id 5, read at 0x37c); it is cleared whenever the body is in any
  // other state. The retail then only updates the body controller and the animation while the
  // timer is below rodata_C8 (2.5f) - the host's CPatterned::Think does both unconditionally, so
  // only the timer is ported here; the draw gate in PreRender and AddToRenderer reads it.
  if (BodyController()->GetCurrentStateId() == pas::kAS_Locomotion) {
    mBodyState5Time += dt;
  } else {
    mBodyState5Time = 0.f;
  }
  CPatterned::Think(dt, mgr);
  // The retail's Think (fn_29_C654) calls three more module functions the host has no body for:
  // fn_29_3B50 (0x108) before the state machine thinks (asm 0xC704), fn_29_2270 (0x1A4) before
  // the touch box (asm 0xC7A8) and fn_29_4960 (0x148) after it (asm 0xC7C0). The first two are
  // ported (the body-state-5 timer above, the looped sounds in UpdateLoopSfx); the third is the
  // actor's light, which the host does not have.
  if (!mCollisionActorManager.null()) {
    mCollisionActorManager->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  }
  mLineOfSight.Update(dt, mgr);
  // The retail's Think (fn_29_C654) also calls fn_29_432C (0x13C, asm 0xC770), the module's own
  // timers: the heard-shot and the arm-swipe and the form timers rise by dt (0xe30, 0xe38, 0xe3c),
  // the body-projectile timer only rises in the form 4 (0xe58), the frustration timer only rises
  // in the form 2 and only while the path is shagged (0xe44), the ball cooldown falls (0xe48), the
  // ing-spot's damage cooldown falls in the form 3 (0x440) and the under-fire timer follows the
  // hit flag (the bit 7 of the byte at 0x420). The host's UpdateTimers is that step.
  UpdateTimers(dt, mgr);
  mSurfaceAlignment.Update(*this, mgr, dt);
  AlignBlobToSurface(mgr); // fn_29_4604, asm 0x0000C794
  UpdateValidTarget(mgr);
  UpdateLoopSfx(); // fn_29_2270 (0x1A4): the ing spot's looped sounds
  UpdateTouchBox();      // fn_29_3C58, asm 0x0000C7B0
  PortIngStandIn("fn_29_4960 light blend (0x148)");
  ValidateLureZone(mgr); // fn_29_1C2C, asm 0x0000C7CC
  if (mgr.GetObjectById(mD76Id) == nullptr) { // asm 0x0000C7E0: no kInvalidUniqueId guard
    mD76Id = kInvalidUniqueId;
  }
  // Once-a-second position poll: the off-navmesh Ing repeats Lurk -> SetRecoverDest ->
  // FailSafeMode, and only a position trace says whether the fail-safe teleport moves it.
  if (DebugIng() && (mDebugPoll++ % 60) == 0) {
    const CVector3f pos = GetTranslation();
    fprintf(stderr, "[ing] %04x pos (%.1f, %.1f, %.1f) form %d state %s\n", GetUniqueId().Value(),
            pos.GetX(), pos.GetY(), pos.GetZ(), mForm, mLastLoggedState ? mLastLoggedState : "?");
  }
}

// @ fn_29_C9A4
void CIng::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const bool wasActive = GetActive(); // asm 0x0000C9C8: CEntity 0x20 bit 7 before the base class
  CPatterned::AcceptScriptMsg(mgr, msg);
  const TUniqueId sender = msg.GetSenderId();
  if (DebugIng()) {
    // Which damage messages reach the Ing, and from whom: the collider relay is the only route a
    // projectile has to a corporeal Ing (pool this with MP2_DEBUG_ING=1).
    switch (msg.GetMessage()) {
    case kSM_Damage:          // XDMG in retail's naming
    case kSM_HitObject:
    case kSM_ResistedDamage:  // XRDG
      fprintf(stderr, "[ing] %04x msg %d from %04x\n", GetUniqueId().Value(),
              static_cast< int >(msg.GetMessage()), sender.Value());
      break;
    default:
      break;
    }
  }
  switch (msg.GetMessage()) {
  case kSM_Create:
    // fn_29_4C88
    EnsureBodyActive(mgr);
    mForm = mStartAsIngSpot ? kForm_IngSpot : kForm_Corporeal;
    if (mForm == kForm_IngSpot) {
      BodyController()->SetLocomotionType(pas::kLT_Crouch); // 0x00004CD0: li r4, 0
      mSurfaceAlignment.AlignNearPosition(*this, mgr, GetTranslation(), 1.f);
    } else {
      BodyController()->SetLocomotionType(pas::kLT_Relaxed); // 0x00004CFC: li r4, 1
    }
    CreateCollisionActorManager(mgr); // fn_29_3190
    // The retail also spawns the ing spot's blob object (fn_29_4830, asm 0xCB84 - module 25's
    // class, which the host does not have) and builds the actor's light (fn_29_4B4C, asm 0xCBA8 -
    // a CActorLights the host's CIng has no room for). Both stay stand-ins.
    PortIngStandIn("fn_29_4830 blob spawn (0x130)");
    PortIngStandIn("fn_29_4B4C light (0x13C)");
    break;
  case kSM_Delete:
    // Retail deletes the blob object (0xd5c, never spawned on the host), stops the sounds,
    // destroys the collision actor manager, then deletes the light (0xd68, unported) and the
    // exit effect.
    StopSfx(); // fn_29_2194, asm 0x0000CBC8
    if (!mCollisionActorManager.null()) {
      mCollisionActorManager->Destroy(mgr); // asm 0x0000CBD4
    }
    if (mD76Id != kInvalidUniqueId) { // asm 0x0000CBF8
      mgr.DeleteObjectRequest(mD76Id);
    }
    ReleaseCoverHint(mgr, mCoverHintId); // fn_29_4560
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamId))) {
      team->QuitTeam(GetUniqueId()); // fn_29_3684
    }
    break;
  case kSM_AreaLoaded:
    mPathArea =
        mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea;
    mPathFindSearch.SetArea(mPathArea);
    mPointPathFindSearch = CPathFindPointSearch(mPathArea);
    break; // retail joins the team only in SetTargetDest (fn_29_372C has no other caller)
  case kSM_Alert:
    // asm 0x0000CD44: ALRT sets CPatterned 0x420 bit 7; mAlert (0xe5d bit 2) is Lurk's.
    SetHitByPlayerProjectile(true);
    break;
  case kSM_Activate:
    // Retail handles ACTV only when the entity was inactive before the base class ran (asm
    // 0x0000CC3C) and turns the collision actors on only for the corporeal form. The light at
    // 0xd68 (asm 0x0000CC6C) is unported.
    if (!wasActive && !mCollisionActorManager.null()) {
      mCollisionActorManager->SetActive(mgr, mForm == kForm_Corporeal); // asm 0x0000CC50
    }
    break;
  case kSM_Deactivate:
    if (!wasActive) { // asm 0x0000CC9C: only when it was active
      break;
    }
    if (!mCollisionActorManager.null()) {
      mCollisionActorManager->SetActive(mgr, false); // asm 0x0000CCAC
    }
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamId))) {
      team->QuitTeam(GetUniqueId()); // fn_29_3684, asm 0x0000CCB8
    }
    StopSfx(); // fn_29_2194, asm 0x0000CD18
    break;
  case kSM_AIUpdateDisabled:
    if (!mCollisionActorManager.null()) {
      mCollisionActorManager->SetPhysicsActive(mgr, false);
    }
    break;
  case kSM_HitObject:
    CollisionActorHitPlayer(mgr, sender); // fn_29_2ED8
    break;
  case kSM_Damage:
    // @ fn_29_2E8C: the ing-spot form reacts to the hit itself, every
    // other form relays it.
    if (mForm == kForm_IngSpot) {
      IngSpotDamage(mgr, sender); // fn_29_2B70
    } else {
      CollisionActorDamage(mgr, sender); // fn_29_2684
    }
    SetHitByPlayerProjectile(true); // asm 0x0000CDA8: 0x420 bit 7
    break;
  case kSM_ResistedDamage:
    // fn_29_C9A4's XRDG branch: while grappled in the ing-spot form the hit timer ticks down and
    // the hit effect plays; either way the actor is flagged as hit.
    if (mForm == kForm_IngSpot && mGrappled) {
      mHitReactionTime -= 0.5f;
      if (CEntity* fx = SpawnHitEffect(mgr, static_cast< bool >(mHeavyHitEffect))) { // fn_29_2414
        mgr.AddObject(fx);
      }
    }
    mHitByPlayerProjectile = true; // 0x420 bit 7
    break;
  case kSM_XENZ:
    ++mSafeZoneCount;
    if (mForm == kForm_IngSpot) {
      mSafeZoneIngSpot = true; // asm 0x0000CE2C: 0x9d4 == 3 -> 0xe5e |= 0x20
    }
    mHurtfulZone = mgr.GetSafeZoneManager()->IsObjectInHurtfulSafeZone(*this, mgr);
    break;
  case kSM_XEXZ:
    --mSafeZoneCount; // asm 0x0000CE74: unconditional
    mHurtfulZone = mgr.GetSafeZoneManager()->IsObjectInHurtfulSafeZone(*this, mgr);
    break;
  default:
    break;
  }
  (void)sender;
}

// @ fn_29_B784
void CIng::UpdateHitDamageTime(float dt) {
  // The retail's own hit-damage timer (asm 0xB7A4-0xB870): the timer at 0x448 (CPatterned's
  // mDamageCooldownTimer) falls by dt and stops at zero, the damage colour is the black to
  // skDamageColor lerp by the timer over skDamageHitTime, and the flag bit 5 of the byte at 0x154
  // follows the timer.
  float timer = mDamageCooldownTimer - dt;
  if (timer <= 0.f) {
    timer = 0.f;
  }
  mDamageCooldownTimer = timer;
  CEchoEmitter* emitter = EchoEmitter();
  if (timer > 0.f) {
    const float t = timer / CPatterned::skDamageHitTime;
    const CColor& color = CColor::Lerp(CColor::Black(), CPatterned::skDamageColor, t);
    mColor.Set(color.GetRedu8(), color.GetGreenu8(), color.GetBlueu8(), mColor.GetAlphau8());
    SetDamageHighlight(true);
    if (emitter) {
      emitter->SetDamageExplicit(t);
    }
  } else {
    SetDamageHighlight(false);
    if (emitter) {
      emitter->SetDamageExplicit(0.f);
    }
  }
}

// @ fn_29_C010
void CIng::PreRender(CStateManager& mgr) {
  // The retail gates the base PreRender on the form (asm 0xC024-0xC07C): the forms 2, 4 and 6..9
  // always pre-render, the forms 3 and 5 only while the body-state-5 timer (0xe40) is below
  // rodata_C8 (2.5f), and the forms 0, 1 and 10 and up never.
  if (IngFormDraws(mForm, mBodyState5Time)) {
    CPatterned::PreRender(mgr);
  }
  // The portal plane (0xe5d bit 6) sets the draw flags' kF_Unknown80: asm 0xC080-0xC09C reads the
  // byte at 0xe5d and tests bit 25 of the word (extrwi. r0, r0, 1, 25), which is bit 6 of that
  // byte, and `ori r3, r0, 0x80` adds 0x80 to the flags word at 0x102 (CModelFlags::mFlags).
  if (mPortalPlane) {
    const CModelFlags flags = GetModelFlags();
    SetModelFlags(CModelFlags(flags, flags.GetOtherFlags() | 0x80));
  }
  // Skipped: the retail also stores a word into the draw flags' first field (0xfc) - asm 0xC094
  // loads it from 0x8(r1), the callee's own outgoing parameter save area, which nothing writes, so
  // the retail stores an uninitialised word there. The host's CModelFlags has that word (x0_) but
  // no setter for it and nothing reads it.
}

// @ fn_29_BF80
void CIng::AddToRenderer(const CStateManager& mgr) const {
  // The same form gate as the PreRender above (asm 0xBF8C-0xBFEC): the forms 2, 4 and 6..9 are
  // added to the renderer, the forms 3 and 5 only while 0xe40 is below rodata_C8 (2.5f), and the
  // forms 0, 1 and 10 and up are not.
  if (IngFormDraws(mForm, mBodyState5Time)) {
    CPatterned::AddToRenderer(mgr);
  }
}

// @ fn_29_C0B4
void CIng::Render(const CStateManager& mgr) const {
  // The portal plane (0xe5d bit 6) sets the model's world-space portal plane (asm 0xC0D0-0xC0E8);
  // the plane is the one at 0xdf0 (the normal) and 0xdfc (the constant).
  if (mPortalPlane) {
    GetModelData()->SetupWorldSpacePortalPlane(
        GetTransform(), CPlane(mPortalPlaneConstant, CUnitVector3f(mPortalPlaneNormal)));
  }
  const int form = mForm;
  if (form == kForm_Corporeal || form == kForm_BodyProjectile ||
      (form >= kForm_BecomingCorporeal && form <= kForm_BecomingWallProjectile)) {
    // The forms 2, 4 and 6..9 (asm 0xC0EC-0xC120).
    if (form == kForm_BecomingWallProjectile && !mForm9Render) {
      return; // asm 0xC120-0xC130: the form 9 needs the flag bit 4 of the byte at 0xe5d
    }
    if (mRenderModel) {
      // asm 0xC134-0xC14C: the flag bit 7 of the byte at 0xe5d draws the model.
      CPatterned::Render(mgr);
      return;
    }
    // asm 0xC150-0xC1A0: with the model's own draw off, only the particle systems the render
    // mask picks are drawn - and only when the actor draws particles (the bit 0 of the byte at
    // 0x421).
    if (mDrawParticles) {
      uint mask = 0;
      uint target = 0;
      mgr.GetCharacterRenderMaskAndTarget(mask, target);
      CPatterned::RenderSystemsToBeDrawnFirst(mgr, mask, target);
      CPatterned::RenderSystemsToBeDrawnLast(mgr, mask, target);
    }
    return;
  }
  if (form == kForm_IngSpot || form == kForm_Evaporate) {
    // The forms 3 and 5 (asm 0xC1A4-0xC210): the ing spot draws only while the body-state-5 timer
    // (0xe40) is below rodata_C8 (2.5f) and the actor draws particles (the bit 0 of the byte at
    // 0x421).
    if (mBodyState5Time < 2.5f && mDrawParticles) {
      // The retail draws no model in this form: the ing spot's visible body is the separate blob
      // object (0xd5c) that module 25's class spawns (fn_29_4830), and the host has no blob, so
      // the model's draw is kept here (with its own particles) to keep the ing spot visible.
      CPatterned::Render(mgr);
    }
  }
}

// fn_29_BF20 (not yet exercised in a run)
const CDamageVulnerability* CIng::GetDamageVulnerability() const {
  if (mForm == kForm_BodyProjectile) {
    return CPatterned::GetDamageVulnerability();
  }
  if (mForm == kForm_IngSpot) {
    return mGrappled ? &mData.mGrapple.mVulnerability : &mData.mIngSpot.GetVulnerability();
  }
  return &mData.mTriggerVulnerability;
}

// @ fn_29_B970
rstl::optional_object< CAABox > CIng::GetTouchBounds() const {
  if (mForm == kForm_IngSpot || mForm == kForm_Evaporate) {
    return mTouchBox;
  }
  if (mForm == 0 || mForm == 1) {
    return rstl::optional_object< CAABox >();
  }
  return CPatterned::GetTouchBounds();
}

// fn_29_BB8C (not yet exercised in a run)
void CIng::Touch(CActor& actor, CStateManager& mgr) {
  EnsureBodyActive(mgr);
  if (mForm == kForm_IngSpot && !mGrappled && actor.GetUniqueId() == mTargetId &&
      mCurDamageRemTime <= 0.f) {
    // [?] the damage: the patterned contact damage.
    mgr.ApplyDamage(GetUniqueId(), mTargetId, GetUniqueId(), GetContactDamage(), SolidFilter(),
                    CVector3f::Zero());
    mCurDamageRemTime = mDamageWaitTime;
  }
  CPatterned::Touch(actor, mgr);
}

// fn_29_B534: CIng's CollidedWith - CPhysicsActor's vtable slot 0x80 (the REL's own override).
// The flying body projectile (form 4) remembers the surface it hit: the first collision whose
// material list holds one of the six splat surfaces gives the normal (0xdd4/0xdd8/0xddc) and
// sets the splat flag (0xe5e bit 6).
void CIng::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list, CStateManager& mgr) {
  CPatterned::CollidedWith(id, list, mgr); // asm 0x0000B550
  if (mForm != kForm_BodyProjectile) {
    return; // asm 0x0000B558: cmpwi mForm (0x9d4), 4
  }
  if (mBodyProjectileTime < 0.5f) {
    return; // asm 0x0000B56C: fcmpo mBodyProjectileTime (0xe58), 0.5f (rodata_B4)
  }
  // The mask retail builds once into lbl_29_bss_8 (asm 0x0000B588-0x0000B6F0) from the six bit
  // indices in lbl_29_data_9DC..0x9F0 (0x1d, 0x1e, 0x1f, 0x3b, 0x2b, 0x2a): Ceiling, Wall, Floor,
  // Solid, Immovable and Occluder. Retail ANDs the mask's low word with the collision's material
  // list low word (0x30) and its high word with the high word (0x34), which is the same test as
  // the whole 64-bit mask against the list's value.
  const u64 kSplatSurfaces = (u64(1) << kMT_Ceiling) | (u64(1) << kMT_Wall) |
                             (u64(1) << kMT_Floor) | (u64(1) << kMT_Solid) |
                             (u64(1) << kMT_Immovable) | (u64(1) << kMT_Occluder);
  const int count = list.GetCount(); // asm 0x0000B6FC: the list's count
  for (int i = 0; i < count; ++i) {
    const CCollisionInfo& info = list[i]; // asm 0x0000B718: the entries are 0x60 apart
    if ((info.GetMaterialLeft().GetValue() & kSplatSurfaces) == 0) {
      continue; // asm 0x0000B720-0x0000B734: the masks do not meet
    }
    mSurfaceNormal = info.GetNormalLeft(); // asm 0x0000B738-0x0000B750: 0x40/0x44/0x48 -> 0xdd4/0xdd8/0xddc
    mSplat = true;                          // asm 0x0000B758: rlwimi, the word's bit 25 = the byte's bit 6 (0x40)
    break;                                  // asm 0x0000B760: the first hit wins
  }
}

// @ fn_29_C228. The base class's handler runs only for the events this function does not handle
// (retail's local `handled` flag, set by every case below). The area 57 run delivered
// kUE_LoopedSoundStop (2), kUE_Activate (29), kUE_EffectOff (34) and kUE_Unknown37 (37).
void CIng::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                           float dt) {
  EnsureBodyActive(mgr); // host guard, not in retail
  bool handled = false;
  switch (type) {
  case kUE_Projectile: // 0
    // fn_29_C228 @0xC368: one mini portal per event, as long as the effect is loaded (retail
    // 0xd44) and FindMiniPortals queued a point still unused (0xe28 < 0xe00). The damage is the
    // mini-portal block's (0x940 = CIngData 0x7c0 + 0x170 + 0x10) scaled by dt and made
    // immunity-proof (retail sets bit 7 of damage+0x1a). The spawn's name is the rodata pointer
    // lbl_29_rodata_120 + 0x573, which the dump does not resolve, so the guardian's identical
    // spawn text stands in [?]. Handled all the same, so the base class does not fire a generic
    // projectile.
    if (mMiniPortalEffect && mMiniPortalIndex >= 0 && mMiniPortalIndex < mMiniPortalCount) {
      const int index = mMiniPortalIndex++; // 0xe28
      CTransform4f xf = GetTransform();
      xf.SetTranslation(mMiniPortalPositions[index]); // 0xe04 + 12 * index
      CDamageInfo damage(mData.mMiniPortal.mDamage);
      damage.MultiplyDamage(dt);
      damage.SetNoImmunity(true);
      // CIngMiniPortalInfo's floats are retail's rodata_64 = 1 (aim time), rodata_74 = 2
      // (lifetime), rodata_70 = 150 (sfx max distance) and rodata_64 = 1 (sfx falloff).
      const CIngMiniPortalInfo info(mTargetId, 1.f, 2.f, *mMiniPortalEffect,
                                    mData.mMiniPortal.mSound, 150.f, 1.f, damage,
                                    mData.mMiniPortal.mBeamInfo);
      CIngMiniPortalAttack* portal = rs_new CIngMiniPortalAttack(
          mgr.AllocateUniqueId(), rstl::string_l("Ing Mini Portal Attack"),
          CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true), xf, GetUniqueId(),
          GetModelData()->GetScale(), info);
      mgr.AddObject(portal);
    }
    handled = true;
    break;
  case kUE_TakeOff: // 0xd
    mTakeOff = true; // 0xe5c bit 0
    handled = true;
    break;
  case kUE_BeginAction: // 0x11
    if (mAlive) {                      // CPatterned 0x420 bit 6
      mForm = kForm_BodyProjectile; // 0x9d4 = 4 (the flying body projectile)
      mBodyProjectileTime = 0.f;    // 0xe58 = rodata_6C
    }
    handled = true;
    break;
  case kUE_Activate: // 0x1d
    mStartAsIngSpot = true; // 0xe5d bit 5
    handled = true;
    break;
  case kUE_Deactivate: // 0x1e
    mStartAsIngSpot = false; // 0xe5d bit 5
    handled = true;
    break;
  case kUE_EffectOn: // 0x21
    if (AnimationData() != nullptr) {
      AnimationData()->SetEffectState(rstl::string_l("eyes"), true, mgr); // rodata_40 -> rodata_68E
    }
    handled = true;
    break;
  case kUE_EffectOff: // 0x22
    if (AnimationData() != nullptr) {
      AnimationData()->SetEffectState(rstl::string_l("eyes"), false, mgr);
    }
    handled = true;
    break;
  default:
    break;
  }
  if (!handled) {
    CPatterned::DoUserAnimEvent(mgr, node, type, dt);
  }
}

// fn_29_B3C4 (not yet exercised in a run)
void CIng::Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) {
  EnsureBodyActive(mgr);
  StopSfx();
  CPatterned::Death(mgr, direction, state);
}

// fn_29_B8C0 (not yet exercised in a run)
void CIng::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  EnsureBodyActive(mgr);
  CPatterned::KnockBack(mgr, info);
}

// fn_29_B890 (not yet exercised in a run)
void CIng::TakeDamage(const CVector3f& direction, float magnitude) {
  if (mForm == 0 || mForm == 1 || mForm == kForm_Evaporate) {
    return;
  }
  CPatterned::TakeDamage(direction, magnitude);
}

// @ fn_29_B294 (with its helper fn_29_1DF4, 0x128)
bool CIng::Listen(CStateManager& mgr, const CVector3f& position, EListenNoiseType type) {
  // The retail's gate (asm 0xB2A8-0xB2B0): the Ing must be alive (the byte at 0x420, bit 6).
  if (!GetAlive() || !GetActive()) {
    return false;
  }
  const CVector3f delta = position - GetTranslation();
  const float distSq = delta.MagSquared();
  if (type == kLNT_PathObstruction) {
    // asm 0xB2D8-0xB328: within rodata_110 (40 units, so 1600 squared) the path is shagged.
    if (distSq < 1600.f) {
      mRecalcPath = true; // 0xe5c bit 2
      return true;
    }
    return false;
  }
  if (type == kLNT_PlayerFire) {
    // asm 0xB32C-0xB3A4: inside the hearing radius (CIngData+0x0, the actor's 0x7c0) and not too
    // far above the Ing, the shot is heard.
    if (distSq >= mData.mHearingRadius * mData.mHearingRadius) {
      return false;
    }
    // The retail also rejects the noise when it is more than 0x3d4 squared above the Ing (asm
    // 0xB378-0xB390); 0x3d4 is CPatterned's mCurPlayerLeashTime.
    if (mCurPlayerLeashTime != 0.f &&
        delta.GetZ() >= mCurPlayerLeashTime * mCurPlayerLeashTime) {
      return false;
    }
    mHeardShotTime = 0.f; // 0xe30
    return true;
  }
  if (type == kLNT_SafeZone) {
    // fn_29_1DF4 (asm 0x1DF4-0x1EF4): the safe zone the noise is in becomes the Ing's lure zone
    // (0xd78), unless the Ing already has one that is nearer. The retail returns false here.
    if (distSq >= mData.mHearingRadius * mData.mHearingRadius) {
      return false;
    }
    const TUniqueId zoneId =
        mgr.GetSafeZoneManager()->PointIsInWhichSafeZone(mgr, position);
    if (mLureSafeZoneId == kInvalidUniqueId) {
      mLureSafeZoneId = zoneId;
      return false;
    }
    const CScriptSafeZone* zone =
        TCastToConstPtr< CScriptSafeZone >(mgr.GetObjectById(mLureSafeZoneId));
    if (zone) {
      const float zoneDistSq = (zone->GetTranslation() - GetTranslation()).MagSquared();
      if (distSq >= zoneDistSq) {
        return false; // the Ing keeps the nearer zone
      }
    }
    mLureSafeZoneId = zoneId;
  }
  return false;
}

// @ fn_29_B208
void CIng::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  stateMachine->SetCodeFunctions(skCodeFuncs, ARRAY_SIZE(skCodeFuncs));
  // Host: MP2_DEBUG_ING dumps the FSM2 graph once.
  static bool sDumped = false;
  const CGenericFSM2* machine = GetStateMachine2();
  if (DebugIng() && !sDumped && machine != nullptr) {
    sDumped = true;
    DumpFsm2(*machine);
  }
}

// Retail fn_29_130. The property ids and their order are the generated SLdrIng reader's, which the
// analysis checked against the REL id by id: it reads all 69. facePlateModel (0xbdcce71b) is
// parsed and then never read, here as in retail. The model data's move is the retail's fn_29_1294
// (0x40, asm 0x1294) with fn_29_12D4 (0x20) and fn_29_12F4 (0x28); the host's LdrToModelData and
// its optional are the same step.
static CEntity* PortLoadIng(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  // fn_29_16AC (0x3D4): the retail's own data-block builder; over the size line, so the host keeps
  // its own construction of the five sub-structs and logs the stand-in.
  PortIngStandIn("fn_29_16AC data block (0x3D4)");
  SLdrIng sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrIng.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL checks the optional's valid byte (sp+0x3f4) and returns null when the model is empty.
    if (DebugIng()) {
      fprintf(stderr, "[ing] load: %s has no model; not created\n", sldrThis.editorProperties.name.data());
    }
    return nullptr;
  }

  // The five sub-structs CIngData embeds, in the order fn_29_130 builds them: fn_29_F12C (the
  // body-projectile block), the CIngSpotData constructor, fn_29_F090 (grapple), fn_29_F000
  // (mini-portal) and fn_29_EFC0 (swarm / exit-host).
  const CIngBodyProjectileData bodyProjectile(
      LdrToDamageInfo(sldrThis.bodyProjectileContactDamage), sldrThis.unknown_0xa0d63374,
      sldrThis.bodyProjectileSuckTime, sldrThis.bodyProjectileSpeed,
      sldrThis.bodyProjectileDropTime, sldrThis.bodyProjectileMinAttackDist,
      sldrThis.bodyProjectileMaxAttackDist, sldrThis.bodyProjectileOdds,
      sldrThis.bodyProjectileSplatEffect, static_cast< ushort >(sldrThis.sound_BodyProjectile),
      static_cast< ushort >(sldrThis.sound_BodyProjectileSplatWall));

  const CIngSpotData ingSpot(sldrThis.ingSpotBlobEffect, sldrThis.ingSpotHitNormalDamage,
                             sldrThis.ingSpotHitHeavyDamage, sldrThis.ingSpotDeath,
                             sldrThis.ingSpotMaxSpeed, sldrThis.ingSpotMaxWallSpeed,
                             sldrThis.ingSpotBallPursuitSpeed, sldrThis.unknown_0x50398a06,
                             sldrThis.ingSpotTurnSpeed,
                             LdrToDamageVulnerability(sldrThis.ingSpotVulnerability),
                             static_cast< ushort >(sldrThis.sound_IngSpotIdle),
                             static_cast< ushort >(sldrThis.sound_IngSpotMove),
                             static_cast< ushort >(sldrThis.sound_HitNormalDamage),
                             static_cast< ushort >(sldrThis.sound_HitHeavyDamage),
                             static_cast< ushort >(sldrThis.sound_IngSpotDeath));

  const CIngGrappleData grapple(sldrThis.unknown_0x67f6c10e,
                                LdrToDamageInfo(sldrThis.exitGrappleDamage),
                                sldrThis.exitGrappleSpitForce, sldrThis.maxGrappleBallHoldTime,
                                sldrThis.postGrappleBallWaitTime, sldrThis.grappleBallPursuitRange,
                                LdrToDamageVulnerability(sldrThis.grappleBallVulnerability),
                                static_cast< ushort >(sldrThis.sound_ExitGrapple),
                                static_cast< ushort >(sldrThis.sound_Grapple));

  const CIngMiniPortalData miniPortal(sldrThis.miniPortalMinAttackDist,
                                      sldrThis.miniPortalMaxAttackDist, sldrThis.miniPortalEffect,
                                      static_cast< ushort >(sldrThis.sound_MiniPortal),
                                      LdrToDamageInfo(sldrThis.miniPortalProjectileDamage),
                                      sldrThis.miniPortalBeamInfo);

  const CIngSwarmData swarm(
      sldrThis.pART, sldrThis.sRSC, sldrThis.pART_0x3da219c7, sldrThis.unknown_0x23271976,
      sldrThis.pART_0x081e9e6c, sldrThis.unknown_0xcb39eccb, sldrThis.unknown_0x587ca175,
      sldrThis.unknown_0x0bd7d5a9, static_cast< ushort >(sldrThis.sound_SwarmMove),
      static_cast< ushort >(sldrThis.sound_ExitHost),
      static_cast< ushort >(sldrThis.sound_ExitHostSafeZone),
      static_cast< ushort >(sldrThis.sound_InsideHost), static_cast< ushort >(sldrThis.sound));

  // fn_29_EE68's own argument order: (ingFlags, hearing, armSwipe, triggerVulnerability,
  // lightColor, the five sub-structs, 0x5d0d, 0xc620, frustration, taunt, aggressiveness,
  // lightAttenuation last).
  const CIngData data(sldrThis.ingFlagsIng, sldrThis.hearingRadius,
                      LdrToDamageInfo(sldrThis.armSwipeDamage),
                      LdrToDamageVulnerability(sldrThis.triggerVulnerability), sldrThis.lightColor,
                      bodyProjectile, ingSpot, grapple, miniPortal, swarm,
                      sldrThis.unknown_0x5d0d2c40, sldrThis.unknown_0xc620183a,
                      sldrThis.frustrationTime, sldrThis.tauntChance, sldrThis.aggressiveness,
                      sldrThis.lightAttenuation);

  if (DebugIng()) {
    fprintf(stderr,
            "[ing] load: %s flags %02x hearing %.1f aggr %.1f taunt %.1f frust %.1f\n"
            "[ing] load: body proj dist [%.1f..%.1f] odds %.1f speed %.1f drop %.1f suck %.1f\n"
            "[ing] load: grapple pursuit %.1f hold %.1f postwait %.1f spit %.1f drain %.1f\n"
            "[ing] load: mini portal dist [%.1f..%.1f]\n"
            "[ing] load: light beam spot x%.2f trigger x%.2f base x%.2f\n",
            sldrThis.editorProperties.name.data(), int(data.mFlags210), data.mHearingRadius,
            data.mAggressiveness, data.mTauntChance, data.mFrustrationTime,
            data.mBodyProjectile.mMinAttackDist, data.mBodyProjectile.mMaxAttackDist,
            data.mBodyProjectile.mOdds, data.mBodyProjectile.mSpeed,
            data.mBodyProjectile.mDropTime, data.mBodyProjectile.mSuckTime,
            data.mGrapple.mPursuitRange, data.mGrapple.mMaxHoldTime,
            data.mGrapple.mPostWaitTime, data.mGrapple.mSpitForce,
            data.mGrapple.mUnknown67F6, data.mMiniPortal.mMinAttackDist,
            data.mMiniPortal.mMaxAttackDist,
            data.mIngSpot.GetVulnerability().GetVulnerability(CWeaponMode(kWT_Light)).mDamageMultiplier,
            data.mTriggerVulnerability.GetVulnerability(CWeaponMode(kWT_Light)).mDamageMultiplier,
            CDamageVulnerability::NormalVulnerabilty()
                .GetVulnerability(CWeaponMode(kWT_Light))
                .mDamageMultiplier);
  }
  return rs_new CIng(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                     LdrToEntityInfo(info, sldrThis.editorProperties),
                     LdrToTransform4f(sldrThis.editorProperties), *modelData,
                     LdrToActorParameters(sldrThis.actorInformation),
                     LdrToPatternedInfo(sldrThis.patterned, nullptr), data);
}

static SIng_FuncPtrs REL_loader_Ing;

// Retail's RELMain calls fn_29_100, which stores the loader fn_29_130 into lbl_29_bss_6C (the
// module's SIng_FuncPtrs) and calls the DOL's SetSIng_FuncPtrs. The proxy in SIng_FuncPtrs.cpp
// calls through it.
void RELMain() {
  REL_loader_Ing.mLoader = PortLoadIng;
  SetSIng_FuncPtrs(&REL_loader_Ing);
}

void RELExit() { SetSIng_FuncPtrs(nullptr); }
