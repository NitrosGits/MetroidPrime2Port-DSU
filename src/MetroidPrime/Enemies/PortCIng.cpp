// The REL's CIng (module 29, Ing.rel), ported for behaviour rather than bytes. The state machine is
// fn_29_B208's: 49 triggers, 29 states and 12 code functions, all registered under the REL's own
// names. Functions checked against the retail code carry `// @ fn_29_<offset>`; the attacks, the
// mini-portal and exit-host states and three code functions are minimal bodies marked [?]. The
// analysis is docs/module-analysis/IngFamily.md, section A (A.5 for this file).

#include "MetroidPrime/Enemies/CIng.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Particles/CGenDescription.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Enemies/CGenericFSM2.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Enemies/CTeamAiRole.hpp"
#include "MetroidPrime/Enemies/PortIngShared.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrIng.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIHint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include <stdio.h>
#include <stdlib.h>

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

namespace {

// The forms (mForm, retail 0x9d4). 0/1 are the host forms [?] (no body), 2 corporeal, 3 ing spot,
// 4 the flying body projectile, 5 evaporating; 6/7/8/9 are the four form changes' own forms.
enum {
  kForm_Corporeal = 2,
  kForm_IngSpot = 3,
  kForm_BodyProjectile = 4,
  kForm_Evaporate = 5,
  kForm_BecomingCorporeal = 6, // BecomeCorporeal's Activate, when it leaves the ing-spot form
  kForm_BecomingIngSpot = 7,   // BecomeIngSpot's Activate, when it leaves the corporeal form
  kForm_BecomingBodyProjectile = 8, // BecomeBodyProjectile's Activate
  kForm_BecomingWallProjectile = 9, // BecomeWallProjectile's Activate
};

// mData.mFlags210 (SLdrIng.ingFlagsIng).
enum {
  kFlag_AlwaysTargetable = 0x01,
  kFlag_NoPathFind = 0x02,
  kFlag_NoFormChange = 0x04,
  kFlag_NoGrapple = 0x08,
  kFlag_NoMiniPortal = 0x10,
  kFlag_NoBodyProjectile = 0x20,
  kFlag_NoArmSwipe = 0x40,
  kFlag_StartAsIngSpot = 0x80,
};

const float kFacingAngle = 0.349f; // rodata: 20 degrees
// rodata_104: the wall projectile's launch direction is clamped to this angle from the surface normal.
const float kWallLaunchAngle = 0.7853981f; // 45 degrees

bool DebugIng() {
  static int sEnabled = -1;
  if (sEnabled < 0) {
    const char* env = getenv("MP2_DEBUG_ING");
    sEnabled = env != nullptr && env[0] != '\0' && env[0] != '0' ? 1 : 0;
  }
  return sEnabled != 0;
}

CMaterialFilter SolidFilter() {
  return CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList());
}

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
, mBlobId(kInvalidUniqueId)
, mTeamId(kInvalidUniqueId)
, mTargetId(kInvalidUniqueId)
, mPrevTargetId(kInvalidUniqueId)
, mAttackPathResumeId(kInvalidUniqueId)
, mCoverHintId(kInvalidUniqueId)
, mLastCoverHintId(kInvalidUniqueId)
, mD76Id(kInvalidUniqueId)
, mLureSafeZoneId(kInvalidUniqueId)
, mDeathEffect(OptionalPart(data.mIngSpot.GetDeathEffect()))
// fn_29_D540 @0x0000E110: the tracker gets the "head" locator (0xd7b) and rodata 0x118/0x11c,
// 0.1f and 0.05f.
, mEyeSegId(PortHeadSegId(modelData))
, mLineOfSight(uid, mEyeSegId, 0.1f, 0.05f)
, mTargetPos(CVector3f::Zero())
, mSurfaceMoveDir(CVector3f::Zero())
, mPortalPlaneNormal(CVector3f::Zero())
, mPortalPlaneConstant(0.f)
, mModelScale(modelData.GetScale())
, mMiniPortalCount(0)
, mSafeZoneCount(0)
, mHeardShotTime(1.5f)
, mUnderFireTime(0.75f)
, mArmSwipeTime(1.f)
, mFormTime(0.f)
, mBodyState5Time(0.f)
, mFrustrationTimer(0.f)
, mBallCooldown(0.f)
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
, mPathArea(nullptr) {
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

CIng::~CIng() {}

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

void CIng::ReleaseCoverHint(CStateManager& mgr, TUniqueId& id) {
  if (CScriptAIHint* hint = GetCoverHint(mgr, id)) {
    hint->SetInUse(false);
    hint->SetTimeRemaining(0.f);
  }
  id = kInvalidUniqueId;
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

void CIng::UpdateValidTarget(CStateManager& mgr) {
  switch (mForm) {
  case 0:
  case 1:
    SetValidTarget(0, false);
    break;
  case kForm_IngSpot:
  case kForm_Evaporate:
    SetValidTarget(0, mgr.GetPlayerState(0)->GetActiveVisor(mgr) == CPlayerState::kPV_Dark ||
                          (mData.mFlags210 & kFlag_AlwaysTargetable) != 0);
    break;
  default:
    SetValidTarget(0, true);
    break;
  }
}

// [?] which loop plays in which form, and the emitter parameters: the idle loop in the ing-spot
// form while still, the move loop while moving on the surface.
void CIng::UpdateLoopSfx() {
  const bool spot = mForm == kForm_IngSpot && GetActive();
  const bool moving = spot && mSurfaceMoveDir != CVector3f::Zero();
  const ushort ids[2] = {mData.mIngSpot.GetIdleSound(), mData.mIngSpot.GetMoveSound()};
  const bool want[2] = {spot && !moving, moving};
  for (int i = 0; i < 2; ++i) {
    if (want[i] && !mLoopSfx[i] && ids[i] != CSfxManager::kInternalInvalidSfxId) {
      mLoopSfx[i] = CSfxManager::AddEmitter(ids[i], GetTranslation(), GetCurrentAreaId().Value(),
                                            true, true);
    } else if (!want[i] && mLoopSfx[i]) {
      CSfxManager::SfxStop(mLoopSfx[i]);
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

// ---- triggers -------------------------------------------------------------------------------

// @ fn_29_B1F4
bool CIng::StateOver(CStateManager&, const CTriggerData&) const { return mAnimationState.IsOver(); }

// fn_29_B1E8 (not yet exercised in a run)
bool CIng::IsAlert(CStateManager&, const CTriggerData&) const { return mAlert; }

// fn_29_B1C8 (not yet exercised in a run)
bool CIng::IsControllingHost(CStateManager&, const CTriggerData&) const {
  return mForm == 0 || mForm == 1;
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

// fn_29_AE68 (not yet exercised in a run)
bool CIng::AreaClear(CStateManager& mgr, const CTriggerData&) const {
  const float half = mModelScale.GetX() * 10.f;
  const CVector3f pos = GetTranslation();
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList,
                    CAABox(pos - CVector3f(half, half, half), pos + CVector3f(half, half, half)),
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

// fn_29_AD68 (not yet exercised in a run)
bool CIng::HasMiniPortals(CStateManager&, const CTriggerData&) const {
  return mMiniPortalCount > 0;
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

// fn_29_AD14 (not yet exercised in a run)
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

// fn_29_AC20 (not yet exercised in a run)
bool CIng::TargetInSafeZone(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  return target != nullptr && mgr.GetSafeZoneManager()->IsObjectInSafeZone(*target, mgr);
}

// @ fn_29_ABC8
bool CIng::TargetInLightSafeZone(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  return target != nullptr && mgr.GetSafeZoneManager()->IsObjectInHurtfulSafeZone(*target, mgr);
}

// fn_29_AB7C (not yet exercised in a run)
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

// fn_29_AA4C (not yet exercised in a run)
bool CIng::ShouldBecomeCorporeal(CStateManager&, const CTriggerData&) const {
  return mDesiredForm == kForm_Corporeal && mForm != kForm_Corporeal;
}

// fn_29_AA04 (not yet exercised in a run)
bool CIng::ShouldBecomeIngSpot(CStateManager&, const CTriggerData&) const {
  return mDesiredForm == kForm_IngSpot && mForm != kForm_IngSpot;
}

// @ fn_29_A8CC
bool CIng::ShouldArmSwipe(CStateManager& mgr, const CTriggerData&) const {
  if (mForm != kForm_Corporeal || (mData.mFlags210 & kFlag_NoArmSwipe) != 0 ||
      !(mArmSwipeTime > 1.f)) {
    return false;
  }
  // [?] the team gate is taken to be CanStartAttack(melee).
  if (!CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Melee, mgr, mTeamId,
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
  if ((mData.mFlags210 & (kFlag_NoBodyProjectile | kFlag_NoFormChange)) != 0 ||
      !mBodyProjectileAllowed ||
      !CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId,
                                        GetUniqueId())) {
    return false;
  }
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return false;
  }
  const CVector3f delta = target->GetTranslation() - GetTranslation();
  const float distSq = delta.MagSquared();
  const float minDist = mData.mBodyProjectile.mMinAttackDist;
  const float maxDist = mData.mBodyProjectile.mMaxAttackDist;
  return minDist * minDist <= distSq && distSq <= maxDist * maxDist && -15.f <= delta.GetZ() &&
         delta.GetZ() <= 3.f;
}

// @ fn_29_A664
bool CIng::ShouldOpenMiniPortal(CStateManager& mgr, const CTriggerData&) const {
  if ((mData.mFlags210 & kFlag_NoMiniPortal) != 0 || mFormTime < mData.mUnknownC620 ||
      !CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId,
                                        GetUniqueId())) {
    return false;
  }
  const float distSq = DistSqToTarget(mgr);
  const float minDist = mData.mMiniPortal.mMinAttackDist;
  const float maxDist = mData.mMiniPortal.mMaxAttackDist;
  return minDist * minDist <= distSq && distSq <= maxDist * maxDist;
}

// fn_29_A658 (not yet exercised in a run)
bool CIng::ProjectileSplat(CStateManager&, const CTriggerData&) const { return mSplat; }

// fn_29_A4EC (not yet exercised in a run)
bool CIng::ProjectileHitTarget(CStateManager& mgr, const CTriggerData&) const {
  const CPhysicsActor* target = TCastToConstPtr< CPhysicsActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return false;
  }
  if (const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId))) {
    if (player->GetMorphballTransitionState() == CPlayer::kMS_Morphed ||
        player->GetMorphballTransitionState() == CPlayer::kMS_Morphing) {
      return false;
    }
  }
  const float half = mModelScale.GetX() * 2.5f;
  const CVector3f pos = GetTranslation();
  return CAABox(pos - CVector3f(half, half, half), pos + CVector3f(half, half, half))
      .DoBoundsOverlap(target->GetBoundingBox());
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

// fn_29_A428 (not yet exercised in a run)
bool CIng::PointPathOver(CStateManager&, const CTriggerData&) const {
  return mIngSpotPathFindNavigation.IsPathOver(*this);
}

// fn_29_A400 (not yet exercised in a run)
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

// fn_29_A2D8 (not yet exercised in a run)
bool CIng::TargetIsBall(CStateManager& mgr, const CTriggerData&) const {
  const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId));
  return player != nullptr && player->GetMorphballTransitionState() == CPlayer::kMS_Morphed;
}

// fn_29_A224 (not yet exercised in a run)
bool CIng::InBallPursuitRange(CStateManager& mgr, const CTriggerData&) const {
  return (mData.mFlags210 & kFlag_NoGrapple) == 0 && mBallCooldown <= 0.f &&
         DistSqToTarget(mgr) < mData.mGrapple.mPursuitRange * mData.mGrapple.mPursuitRange;
}

// fn_29_A1C0 (not yet exercised in a run)
bool CIng::HasPathToTarget(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  return target != nullptr &&
         const_cast< CPathFindSearch& >(mPathFindSearch)
                 .PathExists(GetTranslation(), target->GetTranslation()) ==
             CPathFindSearch::kR_Success;
}

// fn_29_A100 (not yet exercised in a run)
bool CIng::InGrappleRange(CStateManager& mgr, const CTriggerData&) const {
  if ((mData.mFlags210 & kFlag_NoGrapple) != 0 || !mTouchBox) {
    return false;
  }
  const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId));
  if (player == nullptr || player->GetMorphballTransitionState() != CPlayer::kMS_Morphed ||
      mSurfaceAlignment.GetSurface().GetNormal().GetZ() < 0.9f) {
    return false;
  }
  return mTouchBox->DoBoundsOverlap(player->GetBoundingBox());
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

// fn_29_A090 (not yet exercised in a run)
void CIng::WaitForFSMTransition(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("WaitForFSMTransition");
  }
}

// [?] fn_29_9D68: leaving a host is not ported (no host form is ever entered here).
void CIng::ExitHost(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("ExitHost");
    mAnimationState.SetState(CAnimationState::kAS_Over);
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
    // [?] SetActive(*(+0xc50), mgr, true): that CCollisionActorManager is built by fn_29_3190,
    // which is not ported, so the port has no manager to set active.
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
      BodyController()->SetLocomotionType(pas::kLT_Combat);
    }
    Stop();
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    mPortalPlane = false;                                    // 0xe5d bit 6
    mStartAsIngSpot = false;                                 // 0xe5d bit 5
    mVerticalMovement = false;                               // CPatterned 0x34c bit 6
    AddMaterial(kMT_GroundCollider, mgr);                    // 0x25
    BodyController()->SetLocomotionType(pas::kLT_Combat);
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
      BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mForm = kForm_IngSpot;
    mDesiredForm = -1;
    mLineOfSight.SetSegment(CSegId::Null()); // 0xd8a = 99
    mVerticalMovement = true;                // CPatterned 0x34c bit 6
    RemoveMaterial(kMT_GroundCollider, mgr); // 0x25
    // [?] SetActive(*(+0xc50), mgr, false): no CCollisionActorManager in the port (fn_29_3190).
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

// fn_29_4468: the target's aim position (vtable slot 0x54, CActor::GetAimPosition, zero offset).
// False when the target id is empty; the caller then keeps whatever it had.
bool CIng::TargetAimPosition(CStateManager& mgr, CVector3f& out) {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return false;
  }
  out = target->GetAimPosition(mgr, 0.f);
  return true;
}

// fn_29_94E8 (not yet exercised in a run)
void CIng::BecomeBodyProjectile(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("BecomeBodyProjectile");
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6b4 = 1
    // Form 4, the flying body projectile, is only set by the kUE_BeginAction user anim event
    // (fn_29_C228), which the port's DoUserAnimEvent does not handle yet.
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
      BodyController()->SetLocomotionType(pas::kLT_Relaxed);
      if (!mTakeOff && mForm != kForm_BodyProjectile) {
        // Aim at the target's aim position, flattened to the actor's own height (0xdd0 = 0x5c).
        if (TargetAimPosition(mgr, mTargetPos)) { // fn_29_4468
          mTargetPos.SetZ(GetTranslation().GetZ());
          BodyController()->CommandMgr().SetTargetVector(mTargetPos - GetTranslation());
        }
      } else {
        // [?] SetActive(*(+0xc50), mgr, false): the CCollisionActorManager is built by fn_29_3190,
        // which is not ported.
        mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
      }
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    mVerticalMovement = true;                                // CPatterned 0x34c bit 6
    RemoveMaterial(kMT_GroundCollider, mgr);                 // 0x25
    // [?] SetActive(*(+0xc50), mgr, false): no CCollisionActorManager in the port (fn_29_3190).
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
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
    // The portal plane sits the bounding box's Y extent below the actor along the normal; retail's
    // scalarised dot pairs (y,nz)/(x,nx)/(z,ny), taken here as dot(translation - h*normal, normal).
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
      BodyController()->SetLocomotionType(pas::kLT_Relaxed);
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
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
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
    const ushort sfx = mData.mSwarm.mSoundExitHostSafeZone;
    if (sfx != CSfxManager::kInternalInvalidSfxId) {
      // [?] emitter parameters.
      CSfxManager::AddEmitter(sfx, GetTranslation(), GetCurrentAreaId().Value(), true, false);
    }
  } else if (msg == kStateMsg_Update) {
    if (!mHurtfulZone && !mEvaporate) {
      if (mStateTime > 1.f) {
        mAnimationState.SetState(CAnimationState::kAS_Over);
      }
    } else if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_KnockBack)) {
      // [?] the knock-back direction and severity.
      BodyController()->CommandMgr().DeliverCmd(
          CBCKnockBackCmd(GetTransform().GetForward(), pas::kS_One));
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// fn_29_8BAC (not yet exercised in a run)
void CIng::Taunt(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("Taunt");
    mAnimationState.SetState(CAnimationState::kAS_Ready);
  } else if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
      BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(pas::kTT_Two));
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mTaunt = false;
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

// fn_29_8684 (not yet exercised in a run)
void CIng::IngSpotPathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("IngSpotPathFind");
  }
  mRecalcPath = false;
  mMoving = true;
  mPathFindNavigation.PathFind(mgr, msg, dt, *this);
  ApplySeparation(mgr);
  const CVector3f move = BodyController()->CommandMgr().GetMoveVector();
  BodyController()->CommandMgr().ClearLocomotionCmds();
  if (move.IsMagnitudeSafe() && dt > 0.f) {
    float speed;
    if (mSafeZoneIngSpot) {
      speed = mData.mIngSpot.GetSpeedModifier();
    } else if (TargetIsBall(mgr, CTriggerData(0.f))) {
      speed = mData.mIngSpot.GetBallPursuitSpeed();
    } else {
      speed = mData.mIngSpot.GetMaxSpeed();
    }
    MoveOnSurface(move, speed, dt);
    BodyController()->FaceDirection(move.AsNormalized(), dt);
  }
}

// fn_29_8570 (not yet exercised in a run)
void CIng::IngSpotPointPathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("IngSpotPointPathFind");
  }
  mIngSpotPathFindNavigation.PathFind(mgr, msg, dt, *this);
  const CVector3f move = BodyController()->CommandMgr().GetMoveVector();
  BodyController()->CommandMgr().ClearLocomotionCmds();
  const float maxSpeed = mData.mIngSpot.GetMaxSpeed();
  const float speed = CVector3f::GetAngleDiff(CVector3f::Up(), GetTransform().GetUp()) /
                          3.1415927f * (mData.mIngSpot.GetMaxWallSpeed() - maxSpeed) +
                      maxSpeed;
  MoveOnSurface(move, speed, dt);
}

// fn_29_84A4 (not yet exercised in a run)
void CIng::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("Patrol");
    BodyController()->CommandMgr().SetSteeringBlendMode(ESteeringBlendMode::kSBM_FullSpeed);
    // [?] the speed range.
    BodyController()->CommandMgr().SetSteeringSpeedRange(1.f, 1.f);
  }
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);
}

// fn_29_82D4 (not yet exercised in a run)
void CIng::FollowAttackPattern(CStateManager& mgr, EStateMsg msg, float dt) {
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);
  if (msg == kStateMsg_Activate) {
    LogState("FollowAttackPattern");
    const TUniqueId dest = mAttackPathResumeId != kInvalidUniqueId
                               ? mAttackPathResumeId
                               : GetConnectedObject(mgr, kSS_Attack, kSM_Follow);
    mWaypointNavigation.SetDestination(dest);
    mFollowingAttackPath = true;
  } else if (msg == kStateMsg_Update) {
    const CScriptAIWaypoint* waypoint =
        TCastToConstPtr< CScriptAIWaypoint >(mgr.GetObjectById(mWaypointNavigation.GetDestination()));
    if (waypoint != nullptr) {
      if ((waypoint->GetFlags() & (0x40 | 0x80)) != 0) {
        mDesiredForm = kForm_IngSpot;
      } else if ((waypoint->GetFlags() & 0x100) != 0) {
        mDesiredForm = kForm_Corporeal;
      }
    }
    if (mForm == kForm_IngSpot) {
      const CVector3f move = BodyController()->CommandMgr().GetMoveVector();
      BodyController()->CommandMgr().ClearLocomotionCmds();
      const float speed = waypoint != nullptr ? waypoint->GetSpeed() : 1.f;
      MoveOnSurface(move, speed * mData.mIngSpot.GetMaxSpeed(), dt);
    }
  } else if (msg == kStateMsg_Deactivate) {
    mFollowingAttackPath = false;
    mAttackPathResumeId = mWaypointNavigation.GetDestination();
  }
}

// [?] fn_29_7E30: the body-projectile flight is not ported; the state ends at once.
void CIng::BodyProjectileFlight(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("BodyProjectileFlight");
    mAnimationState.SetState(CAnimationState::kAS_Over);
  }
}

// [?] fn_29_76B8: not ported.
void CIng::SuckEnergy(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("SuckEnergy");
    mAnimationState.SetState(CAnimationState::kAS_Over);
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
    mMoving = false;
    return;
  }
  const CVector3f delta = hint->GetTranslation() - GetTranslation();
  const float maxWall = mData.mIngSpot.GetMaxWallSpeed();
  if (!delta.IsMagnitudeSafe() || delta.Magnitude() <= dt * maxWall) {
    mMoving = false;
  } else {
    MoveOnSurface(delta, maxWall, dt);
    mMoving = true;
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
// The cover search MinorIng's fn_44_4E84 shares: the best free AI hint of type 5 in this area that
// is not the last one used and not in a safe zone, scored by distance with height above the target
// weighted.
void CIng::FindCoverPoint(CStateManager& mgr, EStateMsg msg, float) {
  if (msg != kStateMsg_Activate) {
    return;
  }
  LogState("FindCoverPoint");
  ReleaseCoverHint(mgr, mCoverHintId);
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return;
  }
  mTargetPos = target->GetAimPosition(mgr, 0.f);
  TUniqueId best = kInvalidUniqueId;
  float bestScore = FLT_MAX;
  const CObjectList& list = mgr.GetObjectListById(kOL_AiWaypoint);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CScriptAIHint* hint = TCastToConstPtr< CScriptAIHint >(list[i]);
    // [?] hint types 4 (wall) and 5 both count as cover for CIng (HasWallCoverPoint tests 4).
    if (hint == nullptr || !hint->GetActive() ||
        (hint->GetHintType() != static_cast< CScriptAIHint::EHintType >(5) &&
         hint->GetHintType() != static_cast< CScriptAIHint::EHintType >(4)) ||
        hint->GetInUse(GetUniqueId()) || hint->GetCurrentAreaId() != GetCurrentAreaId() ||
        hint->GetUniqueId() == mLastCoverHintId ||
        mgr.GetSafeZoneManager()->PointIsInSafeZone(mgr, hint->GetTranslation())) {
      continue;
    }
    const CVector3f delta = mTargetPos - hint->GetTranslation();
    const float up = CVector3f::Dot(CVector3f::Up(), delta);
    float score = delta.MagSquared();
    if (up > 0.f) {
      score += up * 4.f * up;
    }
    if (score < bestScore) {
      bestScore = score;
      best = hint->GetUniqueId();
    }
  }
  if (CScriptAIHint* hint = GetCoverHint(mgr, best)) {
    hint->SetInUse(true);
    mCoverHintId = best;
    mLastCoverHintId = best;
  }
}

// [?] fn_29_6A88: the arm swipe is not ported; the state ends at once.
void CIng::ArmSwipe(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("ArmSwipe");
    mArmSwipeTime = 0.f;
    mAnimationState.SetState(CAnimationState::kAS_Over);
  }
}

// fn_29_69A0 (not yet exercised in a run)
void CIng::FailSafeMode(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("FailSafeMode");
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    int idx = -1;
    if (mPathArea != nullptr &&
        mPointPathFindSearch.FindClosestPhysicalPoint(GetTranslation(), idx,
                                                      CPathFindPointSearchFilter(1000.f, 0, -1)) ==
            CPathFindPointSearch::kCPR_Success &&
        idx != -1) {
      SetTranslation(mPathArea->GetPoint(idx).GetPosition());
      mSurfaceAlignment.AlignNearPosition(*this, mgr, GetTranslation(), 1.f);
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// [?] fn_29_6578: not ported.
void CIng::GrappleBall(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("GrappleBall");
    mBallCooldown = mData.mGrapple.mPostWaitTime;
    mAnimationState.SetState(CAnimationState::kAS_Over);
  }
}

// [?] fn_29_61B4: not ported; no mini portal is ever found.
void CIng::FindMiniPortals(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("FindMiniPortals");
    mMiniPortalCount = 0;
  }
}

// [?] fn_29_6008: not ported.
void CIng::MiniPortalAttack(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("MiniPortalAttack");
    mAnimationState.SetState(CAnimationState::kAS_Over);
  }
}

// fn_29_5CB0 (not yet exercised in a run)
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
  if (mForm == kForm_IngSpot || mForm == kForm_BodyProjectile || mForm == kForm_Evaporate) {
    if (msg != kStateMsg_Activate) {
      return;
    }
    RemoveMaterial(kMT_Target, kMT_Orbit, mgr);
    if (mDeathEffect) {
      mgr.AddObject(rs_new CExplosion(
          *mDeathEffect, mgr.AllocateUniqueId(),
          CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId),
          rstl::string_l("Ing death Fx"), GetTransform(), 0, mModelScale, CColor::White(), -1));
    }
    const ushort sfx = mData.mIngSpot.GetDeathSound();
    if (sfx != CSfxManager::kInternalInvalidSfxId) {
      CSfxManager::AddEmitter(sfx, GetTranslation(), GetCurrentAreaId().Value(), true, false);
    }
    DeathDelete(mgr);
    return;
  }
  // Corporeal: burn away. [?] the burn's effects; only the shrink and the fade are ported.
  if (msg == kStateMsg_Activate) {
    mDeadFade = 2.f;
    RemoveMaterial(kMT_Target, kMT_Orbit, mgr);
  } else if (msg == kStateMsg_Update) {
    mAlphaDelta = 0.f;
    mSurfaceAlignment.AlignNearPosition(*this, mgr, GetTranslation(), dt);
    if (BodyController()->GetCurrentStateId() == pas::kAS_Death) {
      CVector3f scale = GetModelData()->GetScale();
      const float z = scale.GetZ() - dt / 0.75f;
      if (z > 0.f) {
        ModelData()->SetScale(CVector3f(scale.GetX(), scale.GetY(), z));
      } else {
        mDeadFade -= dt;
        mRenderModel = false;
      }
    }
    if (mDeadFade <= 0.f) {
      DeathDelete(mgr);
    }
  }
}

// fn_29_5C28 (not yet exercised in a run)
void CIng::Evaporate(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("Evaporate");
    mForm = kForm_Evaporate;
  } else if (msg == kStateMsg_Update) {
    if (mStateTime > 2.f) {
      DeathDelete(mgr);
    }
  }
}

// ---- code functions (the FSM2 runs them: CGenericFSM2State::SetCodeFunctions) -----------

// @ fn_29_5B60
void CIng::SetTargetDest(CStateManager& mgr, float) {
  LogState("SetTargetDest");
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return;
  }
  mTargetPos = target->GetTranslation();
  mFoundMovementPos = true;
  mPathFindNavigation.SetDestination(mTargetPos);
  JoinTeam(mgr);
}

// fn_29_5AC4 (not yet exercised in a run)
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

// [?] fn_29_59D0: taken to be the closest path point within 20 as the destination.
void CIng::SetRecoverDest(CStateManager&, float) {
  LogState("SetRecoverDest");
  mFoundMovementPos = false;
  int idx = -1;
  if (mPathArea != nullptr &&
      mPointPathFindSearch.FindClosestPhysicalPoint(GetTranslation(), idx,
                                                    CPathFindPointSearchFilter(20.f, 0, -1)) ==
          CPathFindPointSearch::kCPR_Success &&
      idx != -1) {
    mPathFindNavigation.SetDestination(mPathArea->GetPoint(idx).GetPosition());
    mFoundMovementPos = true;
  }
}

// fn_29_5820 (not yet exercised in a run)
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
  float bestDistSq = 400.f;
  int best = -1;
  int previous = -1;
  for (int i = 0; i < mPathArea->GetNumPoints(); ++i) {
    const CPFPoint& point = mPathArea->GetPoint(i);
    if (!mPathArea->PointPathExists(from, i)) {
      continue;
    }
    const float distSq = (cover - point.GetPosition()).MagSquared();
    if (distSq < bestDistSq &&
        mPathFindSearch.PathExists(point.GetPosition(), cover) == CPathFindSearch::kR_Success) {
      previous = best;
      best = i;
      bestDistSq = distSq;
    }
  }
  if (best == -1) {
    return;
  }
  if (best == from && previous != -1) {
    best = previous;
  }
  if (PortIngPointSearch(mPointPathFindSearch, *mPathArea, from, best)) {
    mFoundMovementPos = true;
  }
}

// fn_29_5714 (not yet exercised in a run)
void CIng::SetWallPointCoverDest(CStateManager& mgr, float) {
  LogState("SetWallPointCoverDest");
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
  int to = -1;
  mPointPathFindSearch.FindClosestPhysicalPoint(hint->GetTranslation(), to,
                                                CPathFindPointSearchFilter(20.f, 0, -1));
  if (to != -1 && PortIngPointSearch(mPointPathFindSearch, *mPathArea, from, to)) {
    mFoundMovementPos = true;
  }
}

// [?] fn_29_54E8: no host is ever entered, so there is no exit point.
void CIng::SetExitHostDest(CStateManager&, float) {
  LogState("SetExitHostDest");
  mFoundMovementPos = false;
}

// fn_29_53E0 (not yet exercised in a run)
void CIng::SetLuredDest(CStateManager& mgr, float) {
  LogState("SetLuredDest");
  CVector3f dest;
  mFoundMovementPos =
      PortIngLuredDest(mgr, mPathFindSearch, mLureSafeZoneId, GetTranslation(), dest);
  mPathFindNavigation.SetDestination(dest);
  mPathFindNavigation.SetFaceTarget(kInvalidUniqueId);
}

// fn_29_51D0 (not yet exercised in a run)
void CIng::SetLuredPointDest(CStateManager& mgr, float) {
  LogState("SetLuredPointDest");
  mFoundMovementPos = PortIngLuredPointDest(mgr, mPathFindSearch, mPointPathFindSearch, mPathArea,
                                            mLureSafeZoneId, GetTranslation());
}

// fn_29_5190 (not yet exercised in a run)
void CIng::StartRangedAttack(CStateManager& mgr, float) {
  CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId, GetUniqueId());
}

// fn_29_514C (not yet exercised in a run)
void CIng::EndRangedAttack(CStateManager& mgr, float) {
  CScriptTeamAiMgr::EndAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId, GetUniqueId(),
                              true);
}

// [?] fn_29_4F14: not ported.
void CIng::SplatOntoMesh(CStateManager&, float) {
  LogState("SplatOntoMesh");
  mSplat = false;
}

// [?] fn_29_4D18: not ported.
void CIng::ExpelGrappledBall(CStateManager&, float) {
  LogState("ExpelGrappledBall");
  mGrappled = false;
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
  CPatterned::Think(dt, mgr);
  mLineOfSight.Update(dt, mgr);
  UpdateTimers(dt, mgr);
  mSurfaceAlignment.Update(*this, mgr, dt);
  UpdateValidTarget(mgr);
  UpdateLoopSfx();
  // [?] the touch box's shape: MinorIng's puddle box (fn_44_3E8C).
  const float half = mModelScale.GetX() * 0.5625f;
  mTouchBox = CAABox(CVector3f(-half, -half, 0.f), CVector3f(half, half, mModelScale.GetX()))
                  .GetTransformedAABox(GetTransform());
  if (mLureSafeZoneId != kInvalidUniqueId) {
    const CScriptSafeZone* zone =
        TCastToConstPtr< CScriptSafeZone >(mgr.GetObjectById(mLureSafeZoneId));
    if (zone == nullptr || !zone->GetActive() ||
        zone->GetZoneType() != static_cast< CScriptSafeZone::EZoneType >(2)) {
      mLureSafeZoneId = kInvalidUniqueId;
    }
  }
  if (mD76Id != kInvalidUniqueId && mgr.GetObjectById(mD76Id) == nullptr) {
    mD76Id = kInvalidUniqueId;
  }
}

// @ fn_29_C9A4
void CIng::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  CPatterned::AcceptScriptMsg(mgr, msg);
  const TUniqueId sender = msg.GetSenderId();
  switch (msg.GetMessage()) {
  case kSM_Create:
    // fn_29_4C88
    EnsureBodyActive(mgr);
    mForm = mStartAsIngSpot ? kForm_IngSpot : kForm_Corporeal;
    if (mForm == kForm_IngSpot) {
      BodyController()->SetLocomotionType(pas::kLT_Relaxed);
      mSurfaceAlignment.AlignNearPosition(*this, mgr, GetTranslation(), 1.f);
    } else {
      BodyController()->SetLocomotionType(pas::kLT_Combat);
    }
    break;
  case kSM_Delete:
    ReleaseCoverHint(mgr, mCoverHintId);
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamId))) {
      team->QuitTeam(GetUniqueId());
    }
    StopSfx();
    break;
  case kSM_AreaLoaded:
    mPathArea =
        mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea;
    mPathFindSearch.SetArea(mPathArea);
    mPointPathFindSearch = CPathFindPointSearch(mPathArea);
    JoinTeam(mgr);
    break;
  case kSM_Alert:
    mAlert = true;
    break;
  case kSM_Activate:
    JoinTeam(mgr);
    break;
  case kSM_Deactivate:
    StopSfx();
    break;
  case kSM_XENZ:
    ++mSafeZoneCount;
    mHurtfulZone = mgr.GetSafeZoneManager()->IsObjectInHurtfulSafeZone(*this, mgr);
    break;
  case kSM_XEXZ:
    if (mSafeZoneCount > 0) {
      --mSafeZoneCount;
    }
    mHurtfulZone = mgr.GetSafeZoneManager()->IsObjectInHurtfulSafeZone(*this, mgr);
    break;
  default:
    break;
  }
  (void)sender;
}

// @ fn_29_C010
void CIng::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);
}

// @ fn_29_C0B4
void CIng::Render(const CStateManager& mgr) const {
  if (!mRenderModel) {
    return;
  }
  if (mForm == kForm_IngSpot || mForm == kForm_Evaporate) {
    if (mBodyState5Time < 2.5f) {
      CPatterned::Render(mgr);
    }
    return;
  }
  CPatterned::Render(mgr);
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

// @ fn_29_C228. The base class's handler runs only for the events this function does not handle
// (retail's local `handled` flag, set by every case below). The area 57 run delivered
// kUE_LoopedSoundStop (2), kUE_Activate (29), kUE_EffectOff (34) and kUE_Unknown37 (37).
void CIng::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                           float dt) {
  EnsureBodyActive(mgr); // host guard, not in retail
  bool handled = false;
  switch (type) {
  case kUE_Projectile: // 0
    // [?] the mini portal attack is not ported: while the bool at 0xd44 is set and the count at
    // 0xe28 is below 0xe00, retail takes that index's point (0xe04 + 12*i), makes a transform from
    // the actor's own (+0x24) with that translation, builds the mini portal's info (fn_29_12100,
    // the beam info at 0x940) and spawns a 0x238-byte CIngMiniPortalAttack (fn_29_11E8C) named
    // "Ing Mini Portal Attack", then adds it to the state manager. Handled all the same, so the
    // base class does not fire a generic projectile.
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

// @ fn_29_B294
bool CIng::Listen(CStateManager&, const CVector3f& position, EListenNoiseType type) {
  if (!GetAlive() || !GetActive()) {
    return false;
  }
  const float distSq = (position - GetTranslation()).MagSquared();
  if (type == kLNT_PathObstruction) {
    if (distSq < 1600.f) {
      mRecalcPath = true;
      return true;
    }
  } else if (distSq < mData.mHearingRadius * mData.mHearingRadius) {
    // [?] every other noise inside the hearing radius counts as a heard shot.
    mHeardShotTime = 0.f;
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
// parsed and then never read, here as in retail.
static CEntity* PortLoadIng(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
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
    fprintf(stderr, "[ing] load: %s\n", sldrThis.editorProperties.name.data());
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
