// Host body for the MinorIng REL (module 44): CMinorIng's state machine, ported for behaviour
// from the REL's pseudocode (docs/module-analysis/IngFamily.md section B.2). Functions marked
// `// @ fn_44_<off>` were checked against that retail function; `[?]` marks what is not proven.
// Not modelled on the host: the blob-effect actor and the collision actor retail creates on
// kSM_Create (fn_44_48F0/fn_44_319C), the touch AABBs (fn_44_3E8C), the inglet spawn in
// MassiveDeath and the damage-effect particles (fn_44_25E4/fn_44_29D4).
#include "MetroidPrime/Enemies/CMinorIng.hpp"

#include "Collision/CCollisionPrimitive.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Collision/CRayCastResult.hpp"
#include "Kyoto/Animation/CInt32POINode.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/Math/CPlane.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Enemies/CGenericFSM2.hpp"
#include "MetroidPrime/Enemies/PortIngShared.hpp"
#include "MetroidPrime/Enemies/CTeamAiRole.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrMinorIng.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIHint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CImpactVisorEffect.hpp"
#include "MetroidPrime/Weapons/CWeapon.hpp"
#include "WorldFormat/CCollisionSurface.hpp"

#include <stdio.h>
#include <stdlib.h>

typedef CPatterned::StateMachine::TriggerFunc TriggerFunc;
typedef CPatterned::StateMachine::StateFunc StateFunc;
typedef CPatterned::StateMachine::CodeFunc CodeFunc;

// Retail data_1A8 (34 triggers), data_47C (15 states) and data_5A8 (5 code functions), installed
// by fn_44_9758; the names are the REL's own strings.
static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"PathShagged", static_cast< TriggerFunc >(&CMinorIng::PathShagged)},
    {"ShouldMoveUnderBall", static_cast< TriggerFunc >(&CMinorIng::ShouldMoveUnderBall)},
    {"HasPatrolPath", static_cast< TriggerFunc >(&CMinorIng::HasPatrolPath)},
    {"PlayerLeashReached", static_cast< TriggerFunc >(&CMinorIng::PlayerLeashReached)},
    {"LeavePatrol", static_cast< TriggerFunc >(&CMinorIng::LeavePatrol)},
    {"LineOfSight", static_cast< TriggerFunc >(&CMinorIng::LineOfSight)},
    {"InMaxRange", static_cast< TriggerFunc >(&CMinorIng::InMaxRange)},
    {"ShouldFire", static_cast< TriggerFunc >(&CMinorIng::ShouldFire)},
    {"PuddleHitByWeapon", static_cast< TriggerFunc >(&CMinorIng::PuddleHitByWeapon)},
    {"CancelStun", static_cast< TriggerFunc >(&CMinorIng::CancelStun)},
    {"IsBeingKnockedback", static_cast< TriggerFunc >(&CMinorIng::IsBeingKnockedback)},
    {"TargetInSafeZone", static_cast< TriggerFunc >(&CMinorIng::TargetInSafeZone)},
    {"HasCoverPoint", static_cast< TriggerFunc >(&CMinorIng::HasCoverPoint)},
    {"PointPathOver", static_cast< TriggerFunc >(&CMinorIng::PointPathOver)},
    {"FoundPointPath", static_cast< TriggerFunc >(&CMinorIng::FoundPointPath)},
    {"IsPointPathFinding", static_cast< TriggerFunc >(&CMinorIng::IsPointPathFinding)},
    {"StunOver", static_cast< TriggerFunc >(&CMinorIng::StunOver)},
    {"ShouldPathFind", static_cast< TriggerFunc >(&CMinorIng::ShouldPathFind)},
    {"CancelAnim", static_cast< TriggerFunc >(&CMinorIng::CancelAnim)},
    {"HasAttackPattern", static_cast< TriggerFunc >(&CMinorIng::HasAttackPattern)},
    {"AttackPatternOver", static_cast< TriggerFunc >(&CMinorIng::AttackPatternOver)},
    {"IsFollowingAttackPath", static_cast< TriggerFunc >(&CMinorIng::IsFollowingAttackPath)},
    {"AllowAttackPatternProjectile",
     static_cast< TriggerFunc >(&CMinorIng::AllowAttackPatternProjectile)},
    {"AllowAttackPatternStun", static_cast< TriggerFunc >(&CMinorIng::AllowAttackPatternStun)},
    {"RecalculatePath", static_cast< TriggerFunc >(&CMinorIng::RecalculatePath)},
    {"InsideSafeZone", static_cast< TriggerFunc >(&CMinorIng::InsideSafeZone)},
    {"IsMoveAwayFromSafeZoneOver",
     static_cast< TriggerFunc >(&CMinorIng::IsMoveAwayFromSafeZoneOver)},
    {"FoundMovementPos", static_cast< TriggerFunc >(&CMinorIng::FoundMovementPos)},
    {"ShouldEvaporate", static_cast< TriggerFunc >(&CMinorIng::ShouldEvaporate)},
    {"IsInCorporealForm", static_cast< TriggerFunc >(&CMinorIng::IsInCorporealForm)},
    {"StayOnPointPathFinding", static_cast< TriggerFunc >(&CMinorIng::StayOnPointPathFinding)},
    {"AllowProjectileDuringStayOnPointPathFinding",
     static_cast< TriggerFunc >(&CMinorIng::AllowProjectileDuringStayOnPointPathFinding)},
    {"StateOver", static_cast< TriggerFunc >(&CMinorIng::StateOver)},
    {"IsLuredBySafeZone", static_cast< TriggerFunc >(&CMinorIng::IsLuredBySafeZone)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"PathFind", static_cast< StateFunc >(&CMinorIng::PathFind)},
    {"PointPathFind", static_cast< StateFunc >(&CMinorIng::PointPathFind)},
    {"Stunned", static_cast< StateFunc >(&CMinorIng::Stunned)},
    {"ProjectileAttack", static_cast< StateFunc >(&CMinorIng::ProjectileAttack)},
    {"Patrol", static_cast< StateFunc >(&CMinorIng::Patrol)},
    {"Dead", static_cast< StateFunc >(&CMinorIng::Dead)},
    {"SetWallPointCoverDest", static_cast< StateFunc >(&CMinorIng::SetWallPointCoverDest)},
    {"SetCoverDest", static_cast< StateFunc >(&CMinorIng::SetCoverDest)},
    {"IntoPuddleForm", static_cast< StateFunc >(&CMinorIng::IntoPuddleForm)},
    {"IntoCorporealForm", static_cast< StateFunc >(&CMinorIng::IntoCorporealForm)},
    {"Idle", static_cast< StateFunc >(&CMinorIng::Idle)},
    {"FollowAttackPattern", static_cast< StateFunc >(&CMinorIng::FollowAttackPattern)},
    {"MoveAwayFromSafeZone", static_cast< StateFunc >(&CMinorIng::MoveAwayFromSafeZone)},
    {"AlwaysPointPathFind", static_cast< StateFunc >(&CMinorIng::AlwaysPointPathFind)},
    {"WaitForFSMTransition", static_cast< StateFunc >(&CMinorIng::WaitForFSMTransition)},
};

static CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    {"SelectTarget", static_cast< CodeFunc >(&CMinorIng::SelectTarget)},
    {"FindCoverPoint", static_cast< CodeFunc >(&CMinorIng::FindCoverPoint)},
    {"SetPointCoverDest", static_cast< CodeFunc >(&CMinorIng::SetPointCoverDest)},
    {"SetLuredDest", static_cast< CodeFunc >(&CMinorIng::SetLuredDest)},
    {"SetLuredPointDest", static_cast< CodeFunc >(&CMinorIng::SetLuredPointDest)},
};

namespace {

bool DebugMinorIng() {
  static int sEnabled = -1;
  if (sEnabled < 0) {
    const char* env = getenv("MP2_DEBUG_MINORING");
    sEnabled = env != nullptr && env[0] != '\0' && env[0] != '0' ? 1 : 0;
  }
  return sEnabled != 0;
}

// Material filters from the REL's .data: data_0 (Touch), data_5F8/5FC (fn_44_88DC) and
// data_60C (fn_44_34B0) are material numbers shifted into masks: 0x3b Solid, 0x20 Player.
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
    fprintf(stderr, "[minoring] fsm %s %s:", kind, nodes[i].GetName().data());
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
    fprintf(stderr, "[minoring] fsm trigger %s%s(%g):", triggers[i].IsNegated() ? "!" : "",
            triggers[i].GetName().data(), triggers[i].GetArgument().GetFloat());
    for (int t = 0; t < out.size(); ++t) {
      fprintf(stderr, " %s->%s", out[t].mName.data(), Fsm2NodeName(out[t].mTarget));
    }
    fprintf(stderr, "\n");
  }
}

} // namespace

CMinorIng::CMinorIng(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                     const CTransform4f& xf, const CModelData& modelData,
                     const CPatternedInfo& pInfo, const CActorParameters& actParms,
                     const CMinorIngData& data)
// EPatternedAI 0x4b, flavor 0, movement 0 (ground), collider 1, body 1 (bipedal): the retail
// constructor's arguments to CPatterned (fn_44_9D58).
: CPatterned(static_cast< EPatternedAI >(0x4b), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mData(data)
, mProjectileInfo(data.mProjectile, data.mProjectileDamage)
, mPathFindSearch(nullptr, 0x301, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0, CPFRegion::kRP_Center)
, mPointPathFindSearch(nullptr)
, mSurfaceAlignment()
, mIngSpotPathFindNavigation()
, mSurfaceMoveDir(CVector3f::Zero())
, mLineOfSight(uid, CSegId(), 0.1f, 0.05f)
, mForm(kF_None)
, mTeamId(kInvalidUniqueId)
, mTargetId(kInvalidUniqueId)
, mCoverHintId(kInvalidUniqueId)
, mLastCoverId(kInvalidUniqueId)
, mAttackPathLastId(kInvalidUniqueId)
, mAttackPathId(kInvalidUniqueId)
, mSafeZoneId(kInvalidUniqueId)
, mBlobEffectId(kInvalidUniqueId)
, mCollisionActorId(kInvalidUniqueId)
, mTargetPos(CVector3f::Zero())
, mModelScale(modelData.GetScale())
, mAttackTimer(0.f)
, mLeashTimer(0.f)
, mStunTimer(0.f)
, mPuddleHitTimer(0.f)
, mHpTracker(GetHealthInfo() != nullptr ? GetHealthInfo()->GetHP() : 0.f)
, mKnockDamage(0.f)
, mTimerD1C(0.f)
, mMoveAwayTimer(0.f)
, mSafeZoneEnterCount(0)
, mEvaporate(0.f)
, mDamageFxTimer(0.f)
, mLastLoggedState(nullptr)
, mPathArea(nullptr)
, mHasPatrolPath(false)
, mPathFindActive(false)
, mPointPathFinding(false)
, mFoundMovementPos(false)
, mRecalculatePath(false)
, mIdle(false)
, mFollowingAttackPath(false)
, mAttackPatternProjectile(false)
, mPuddleHitByWeapon(false)
, mCancelStun(false)
, mKnockAnimCancel(false)
, mTouched(false)
, mShouldMoveUnderBall(false)
, mInsideSafeZone(false)
, mShouldEvaporate(false) {
  // fn_44_9D58 locks the projectile's WPSC token (0x918); without it LaunchProjectile's TryCache
  // never succeeds.
  mProjectileInfo.Token().Lock();
  // fn_44_9D58: the surface helper's mode word (0xc24) is 1, the path search's character
  // height/radius are scale.y*3 and scale.z*5.
  mSurfaceAlignment.SetMode(static_cast< CSurfaceAlignmentHelper::EMode >(1));
  mPathFindSearch.SetCharacterHeight(mModelScale.GetY() * 3.f);
  mPathFindSearch.SetCharacterRadius(mModelScale.GetZ() * 5.f);
  // Knock-back physics off and the knock-back animation reaction widened to (2, 2).
  KnockBackController().EnableKnockBackPhysics(false);
  KnockBackController().SetAnimReactionRange(CKnockBackMgr::kAR_KnockBack,
                                             CKnockBackMgr::kAR_KnockBack);
  // The lock-on locator ("lockon_target_SDK", 0xd3c) replaces mLockOnTarget when present, and
  // the line-of-sight tracker casts from "Eye_LCTR" (the REL's next string, bss_40) through
  // Solid minus Player/CollisionActor (bss_0 / DAT_80900d10 in fn_44_ABCC).
  if (const CAnimData* animData = GetModelData()->GetAnimationData()) {
    const CSegId lockOn = animData->GetLocatorSegId(rstl::string_l("lockon_target_SDK"));
    if (lockOn != CSegId::Null()) {
      mLockOnTarget = lockOn;
    }
    mLineOfSight.SetSegment(animData->GetLocatorSegId(rstl::string_l("Eye_LCTR")));
  }
  mLineOfSight.SetRayFilter(CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid), CMaterialList(kMT_Player, kMT_CollisionActor)));
}

CMinorIng::~CMinorIng() {}

void CMinorIng::LogState(const char* name) {
  if (DebugMinorIng() && mLastLoggedState != name) {
    mLastLoggedState = name;
    fprintf(stderr, "[minoring] %04x state %s form %d\n", GetUniqueId().Value(), name, mForm);
  }
}

// Retail activates the body controller on kSM_Create (fn_44_9150). Kept as a guard for the
// entry points that can run before it on the host.
void CMinorIng::EnsureBodyActive(CStateManager& mgr) {
  if (BodyController() != nullptr && !BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

// ---- helpers -------------------------------------------------------------------------------

// @ fn_44_8BF4
CVector3f CMinorIng::RaisedPoint(const CVector3f& pos) const {
  return CVector3f(pos.GetX(), pos.GetY(), pos.GetZ() + 1.f);
}

// @ fn_44_8B38
bool CMinorIng::ProjectOntoMesh(CVector3f& pos) {
  pos = RaisedPoint(pos);
  float height = 0.f;
  if (mPathFindSearch.GetHeightOfPointAboveMesh(pos, height, 0.f) != CPathFindSearch::kR_Success) {
    return false;
  }
  if (height > 0.f) {
    pos.SetZ(pos.GetZ() - (height - 0.5f));
  }
  return true;
}

// @ fn_44_5D70
bool CMinorIng::IsTargetInRange(CStateManager& mgr, const CVector3f& pos) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return false;
  }
  const bool moving = mPathFindActive || mPointPathFinding || mIdle;
  const float range = mMaxAttackRange - (moving ? mMaxAttackRange * 0.2f : 0.f);
  return (target->GetTranslation() - pos).MagSquared() < range * range;
}

// @ fn_44_5E94
bool CMinorIng::IsTargetInAttackAngle(CStateManager& mgr) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return false;
  }
  const CVector3f toSelf = -(target->GetTranslation() - GetTranslation());
  return CVector3f::GetAngleDiff(target->GetTransform().GetForward(), toSelf) <
         mData.mAttackAngleLimit * 0.017453292f;
}

// @ fn_44_88DC
bool CMinorIng::IsMoveClear(CStateManager& mgr, const CVector3f& offset, bool checkStatic) const {
  // [?] retail first calls fn_44_3670 (snap to the floor) from this const-looking check; the
  // host keeps that side effect out of it.
  if ((mData.mFlags156 & 0x20) != 0) {
    return true;
  }
  const CCollisionPrimitive* prim = GetCollisionPrimitive();
  if (prim == nullptr) {
    return true;
  }
  // [?] retail builds the transform with its own GetPrimitiveTransform (fn_44_4080, locator-based)
  CTransform4f xf = GetPrimitiveTransform();
  xf.SetTranslation(xf.GetTranslation() + offset);
  const CMaterialFilter filter =
      CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList(kMT_Player));
  if (checkStatic && CGameCollision::DetectStaticCollisionBoolean(mgr, *prim, xf, filter)) {
    return false;
  }
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, prim->CalculateAABox(xf), filter, this);
  if (nearList.empty()) {
    return !mgr.GetSafeZoneManager()->PointIsInSafeZone(mgr, xf.GetTranslation());
  }
  for (int i = 0; i < nearList.size(); ++i) {
    const CCollisionActor* colAct =
        TCastToConstPtr< CCollisionActor >(mgr.GetObjectById(nearList[i]));
    if (colAct == nullptr || colAct->GetOwnerId() != GetUniqueId()) {
      return false;
    }
  }
  return true;
}

// @ fn_44_2598
CScriptAIHint* CMinorIng::GetCoverHint(CStateManager& mgr, TUniqueId id) const {
  if (id == kInvalidUniqueId) {
    return nullptr;
  }
  return TCastToPtr< CScriptAIHint >(mgr.ObjectById(id));
}

// @ fn_44_24D4
void CMinorIng::ReleaseCoverHint(CStateManager& mgr, TUniqueId& id, bool keepTimer) {
  if (CScriptAIHint* hint = GetCoverHint(mgr, id)) {
    hint->SetInUse(false);
    if (!keepTimer) {
      hint->SetTimeRemaining(0.f); // hint+0x170
    }
    id = kInvalidUniqueId;
  }
}

// @ fn_44_343C
void CMinorIng::ResetAttackTimer(CStateManager& mgr) {
  mAttackTimer = GetAverageAttackTime();
  // retail tests bit 7 of mgr+0x16e8, the byte after the CRandom16 at 0x16e4.
  if (mgr.IsRandomAvailable()) {
    mAttackTimer += mgr.Random()->Float() * mAttackTimeVariation;
  }
}

// @ fn_44_3B0C
void CMinorIng::PlaySfx(int idx, bool start) {
  if (idx < 0 || idx >= 6) {
    return;
  }
  const bool looped = idx == 0 || idx == 1;
  if (!start) {
    if (looped) {
      CSfxManager::SfxStop(mLoopSfx[idx]);
      mLoopSfx[idx] = CSfxHandle();
    }
    return;
  }
  ushort id = CSfxManager::kInternalInvalidSfxId;
  switch (idx) {
  case 0:
    id = mData.mIngSpotIdleSound;
    break;
  case 1:
    id = mData.mIngSpotMoveSound;
    break;
  case 3:
    id = mData.mIngSpotDeathSound;
    break;
  case 4:
    id = mData.mIngSpotHitNormalSound;
    break;
  case 5:
    id = mData.mIngSpotHitHeavySound;
    break;
  default:
    break;
  }
  if (id == CSfxManager::kInternalInvalidSfxId) {
    return;
  }
  CAudioSys::C3DEmitterParmData parms(mData.mIngSpotUnknown7569, mData.mIngSpotSfxFallOff, 1, 0x7f,
                                      0x14);
  parms.mPos = GetTranslation();
  parms.mDir = CVector3f::Zero();
  parms.mSfxId = id;
  const CSfxHandle handle =
      CSfxManager::AddEmitter(parms, GetCurrentAreaId().Value(), true, looped,
                              CSfxManager::kMedPriority);
  if (looped) {
    mLoopSfx[idx] = handle;
  }
}

// @ fn_44_39C4
void CMinorIng::UpdateLoopSfx() {
  bool stopMove = true;
  bool stopIdle = true;
  if (mForm == kF_Puddle) {
    if (!mPathFindActive && !mPointPathFinding && !mFollowingAttackPath) {
      stopIdle = false;
      if (!mLoopSfx[0]) {
        PlaySfx(0, true);
      } else {
        CSfxManager::UpdateEmitter(mLoopSfx[0], GetTranslation(), GetTransform().GetForward(),
                                   0x7f);
      }
    } else {
      stopMove = false;
      if (!mLoopSfx[1]) {
        PlaySfx(1, true);
      } else {
        CSfxManager::UpdateEmitter(mLoopSfx[1], GetTranslation(), GetTransform().GetForward(),
                                   0x7f);
      }
    }
  }
  if (stopMove) {
    PlaySfx(1, false);
  }
  if (stopIdle) {
    PlaySfx(0, false);
  }
}

// @ fn_44_396C
void CMinorIng::StopSfx() {
  PlaySfx(0, false);
  PlaySfx(1, false);
}

// @ fn_44_3CB4
void CMinorIng::UpdateTimers(float dt) {
  if (mForm == kF_Puddle && mCurDamageRemTime > 0.f) {
    mCurDamageRemTime -= dt;
  }
  mAttackTimer -= dt;
  mPuddleHitTimer -= dt;
  mTimerD1C -= dt;
  mDamageFxTimer -= dt;
}

// @ fn_44_3E20
void CMinorIng::UpdateLocomotionType() {
  const pas::ELocomotionType type =
      (mForm == kF_Puddle || mForm == kF_IntoCorporeal || mForm == kF_IntoPuddle)
          ? pas::kLT_Relaxed
          : pas::kLT_Combat;
  if (type != BodyController()->GetLocomotionType()) {
    BodyController()->SetLocomotionType(type);
  }
}

// @ fn_44_43F4
void CMinorIng::SetForm(CStateManager& mgr, int form, bool setLocomotion) {
  if (form == mForm) {
    return;
  }
  const bool keepTargetable = (mData.mFlags156 & 0x04) != 0;
  if (form == kF_Puddle) {
    if (!keepTargetable) {
      RemoveMaterial(kMT_Target, kMT_SeekerTarget, mgr);
    }
    AddMaterial(kMT_NonSolidDamageable, mgr);
  } else {
    if (!keepTargetable) {
      AddMaterial(kMT_Target, kMT_SeekerTarget, mgr);
    }
    RemoveMaterial(kMT_NonSolidDamageable, mgr);
  }
  // [?] fn_44_305C activates the collision actor for forms 0/2/3; it does not exist on the host.
  if (form == kF_IntoCorporeal) {
    SnapToFloor(mgr);
  }
  const float fade = mPuddleHitByWeapon ? 0.021f : 0.33f;
  switch (form) {
  case kF_Corporeal:
  case kF_IntoCorporeal:
    mAlphaDelta = 1.f / fade;
    break;
  case kF_Puddle:
    mAlphaDelta = -(1.f / fade);
    break;
  case kF_IntoPuddle:
    mAlphaDelta = 0.f;
    break;
  default:
    break;
  }
  mForm = form;
  if (setLocomotion) {
    UpdateLocomotionType();
  }
}

// @ fn_44_3670
void CMinorIng::SnapToFloor(CStateManager& mgr) {
  // fn_44_34B0: a ray down the actor's up axis, from one unit above it, 10 units long, Solid.
  const CVector3f up = GetTransform().GetUp();
  const CRayCastResult result =
      mgr.RayStaticIntersection(GetTranslation() + up, up * -1.f, 10.f, SolidFilter());
  if (result.IsValid() && result.GetPoint() != CVector3f::Zero()) {
    SetTranslation(result.GetPoint());
  }
}

// @ fn_44_883C
bool CMinorIng::UpdateTargetPos(CStateManager& mgr) {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target != nullptr) {
    mTargetPos = target->GetAimPosition(mgr, 0.2f);
  }
  return target != nullptr;
}

// @ fn_44_5AC0
void CMinorIng::UpdateBallPursuit(CStateManager& mgr) {
  CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamId));
  const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mTargetId));
  // [?] retail reads the morph state at player+0x38c only when the word at +0x390 is 0.
  if (player != nullptr && player->GetMorphballTransitionState() == CPlayer::kMS_Morphed &&
      (player->GetTranslation() - GetTranslation()).MagSquared() <
          mData.mMorphballPursuitDistance * mData.mMorphballPursuitDistance) {
    const CTeamAiRole* role = CScriptTeamAiMgr::GetTeamAiRole(mgr, mTeamId, GetUniqueId());
    if (role == nullptr || role->GetTeamAiRole() != CTeamAiRole::kTAR_Initial) {
      if (mTeamId == kInvalidUniqueId ||
          (team != nullptr && team->IsMeleeAttacking(GetUniqueId())) ||
          CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Melee, mgr, mTeamId,
                                        GetUniqueId())) {
        mShouldMoveUnderBall = true;
        return;
      }
    }
  }
  if (team != nullptr && team->IsMeleeAttacking(GetUniqueId())) {
    EndAttack(mgr, false);
  }
  mShouldMoveUnderBall = false;
}

// @ fn_44_6FCC
void CMinorIng::UpdateValidTarget(CStateManager& mgr) {
  if (mForm == kF_Puddle) {
    SetValidTarget(0, mgr.GetPlayerState(0)->GetActiveVisor(mgr) == CPlayerState::kPV_Dark ||
                          (mData.mFlags156 & 0x04) != 0);
  } else {
    SetValidTarget(0, true);
  }
}

// @ fn_44_36FC
void CMinorIng::UpdateSafeZone(CStateManager& mgr) {
  if (mSafeZoneId == kInvalidUniqueId) {
    return;
  }
  const CScriptSafeZone* zone = TCastToConstPtr< CScriptSafeZone >(mgr.GetObjectById(mSafeZoneId));
  if (zone == nullptr || !zone->GetActive() ||
      zone->GetZoneType() != static_cast< CScriptSafeZone::EZoneType >(2)) {
    mSafeZoneId = kInvalidUniqueId;
  }
}

// @ fn_44_3780
void CMinorIng::PickSafeZone(CStateManager& mgr, const CVector3f& pos) {
  const float distSq = (pos - GetTranslation()).MagSquared();
  if (distSq > mData.mHearingRadius * mData.mHearingRadius) {
    return;
  }
  const TUniqueId zoneId = mgr.GetSafeZoneManager()->PointIsInWhichSafeZone(mgr, pos);
  if (mSafeZoneId == kInvalidUniqueId) {
    mSafeZoneId = zoneId;
    return;
  }
  const CScriptSafeZone* zone = TCastToConstPtr< CScriptSafeZone >(mgr.GetObjectById(mSafeZoneId));
  if (zone == nullptr || distSq < (zone->GetTranslation() - GetTranslation()).MagSquared()) {
    mSafeZoneId = zoneId;
  }
}

// @ fn_44_593C
void CMinorIng::JoinTeam(CStateManager& mgr) {
  if (mTeamId == kInvalidUniqueId) {
    mTeamId = CScriptTeamAiMgr::GetAssociatedTeamId(*this, mgr);
  }
  if (mTeamId != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamId))) {
      team->JoinTeam(*this, CTeamAiRole::kTAR_Projectile, CTeamAiRole::kTAR_Melee,
                     CTeamAiRole::kTAR_Invalid);
    }
  }
}

// @ fn_44_5844
void CMinorIng::EndAttack(CStateManager& mgr, bool projectile) {
  CScriptTeamAiMgr::EndAttack(projectile ? CScriptTeamAiMgr::kAT_Projectile
                                         : CScriptTeamAiMgr::kAT_Melee,
                              mgr, mTeamId, GetUniqueId(), true);
}

// @ fn_44_55E0
void CMinorIng::ApplySeparation(CStateManager& mgr) {
  CVector3f total = CVector3f::Zero();
  const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CPatterned* other = TCastToConstPtr< CPatterned >(list[i]);
    if (other != nullptr && other != this && other->GetCurrentAreaId() == GetCurrentAreaId()) {
      const CVector2f separation = mSteeringBehaviors.Separation2D(
          *this, CVector2f(other->GetTranslation().GetX(), other->GetTranslation().GetY()),
          mModelScale.GetX() * 10.f);
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

// @ fn_44_6900
void CMinorIng::MoveAlongPath(CStateManager& mgr, float dt) {
  CVector3f move = BodyController()->CommandMgr().GetMoveVector();
  BodyController()->CommandMgr().ClearLocomotionCmds();
  if (!move.IsMagnitudeSafe() || !(dt > 0.f)) {
    return;
  }
  const CVector3f face = move.AsNormalized();
  float speed = mData.mIngSpotMaxSpeed;
  if (mSafeZoneId == kInvalidUniqueId && mSafeZoneEnterCount == 0) {
    if (mShouldMoveUnderBall) {
      speed = mData.mIngSpotBallPursuitSpeed;
    }
  } else {
    speed *= mData.mSpeedModifier;
  }
  MoveOnSurface(speed, dt, move);
  BodyController()->FaceDirection(face, dt);
}

// @ fn_44_5384
void CMinorIng::MoveOnSurface(float speed, float dt, const CVector3f& moveIn) {
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
    const CVector3f next = pos + dir * (speed * dt);
    bool apply = true;
    if (!mPointPathFinding && !mFollowingAttackPath &&
        mPathFindSearch.OnPath(RaisedPoint(pos)) == CPathFindSearch::kR_Success &&
        mPathFindSearch.OnPath(RaisedPoint(next)) != CPathFindSearch::kR_Success) {
      apply = false;
    }
    if (apply) {
      SetTranslation(next);
      mSurfaceMoveDir = dir;
    }
  }
  Stop();
}

// ---- triggers ------------------------------------------------------------------------------

// @ fn_44_6184
bool CMinorIng::PathShagged(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::PathShagged(mgr, data) ||
         mPathFindSearch.OnPath(RaisedPoint(GetTranslation())) != CPathFindSearch::kR_Success;
}

// @ fn_44_5D34
bool CMinorIng::ShouldMoveUnderBall(CStateManager&, const CTriggerData&) const {
  return mShouldMoveUnderBall;
}

// @ fn_44_6178
bool CMinorIng::HasPatrolPath(CStateManager&, const CTriggerData&) const { return mHasPatrolPath; }

// @ fn_44_6144
bool CMinorIng::PlayerLeashReached(CStateManager&, const CTriggerData&) const {
  return mTargetId == kInvalidUniqueId || mPlayerLeashTime < mLeashTimer;
}

// @ fn_44_60F4
bool CMinorIng::LeavePatrol(CStateManager& mgr, const CTriggerData& data) const {
  return mTargetId != kInvalidUniqueId || InDetectionRange(mgr, data);
}

// @ fn_44_60C4
bool CMinorIng::LineOfSight(CStateManager&, const CTriggerData&) const {
  // 0xcc0 bit 7 and 0xcb8 are the tracker's flag and clear time (tracker at 0xc88).
  return mLineOfSight.HasLineOfSight() && mLineOfSight.GetClearTime() > 0.f;
}

// @ fn_44_5E70
bool CMinorIng::InMaxRange(CStateManager& mgr, const CTriggerData&) const {
  return IsTargetInRange(mgr, GetTranslation());
}

// @ fn_44_5F60
bool CMinorIng::ShouldFire(CStateManager& mgr, const CTriggerData&) const {
  const CTriggerData zero(0.f);
  if (mTouched || mAttackTimer > 0.f || !IsTargetInAttackAngle(mgr) || !InMaxRange(mgr, zero) ||
      !LineOfSight(mgr, zero) || !IsMoveClear(mgr, CVector3f::Zero(), true)) {
    return false;
  }
  const CTeamAiRole* role = CScriptTeamAiMgr::GetTeamAiRole(mgr, mTeamId, GetUniqueId());
  if (role != nullptr && role->GetTeamAiRole() == CTeamAiRole::kTAR_Initial) {
    return false;
  }
  if (mTeamId != kInvalidUniqueId &&
      !CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId,
                                     GetUniqueId())) {
    return false;
  }
  return true;
}

// @ fn_44_5D64
bool CMinorIng::PuddleHitByWeapon(CStateManager&, const CTriggerData&) const {
  return mPuddleHitByWeapon;
}

// @ fn_44_5D58
bool CMinorIng::CancelStun(CStateManager&, const CTriggerData&) const { return mCancelStun; }

// @ fn_44_5D40
bool CMinorIng::IsBeingKnockedback(CStateManager&, const CTriggerData&) const {
  return GetBodyController()->GetCurrentStateId() == pas::kAS_KnockBack;
}

// @ fn_44_4D84
bool CMinorIng::TargetInSafeZone(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  return target != nullptr && mgr.GetSafeZoneManager()->IsObjectInSafeZone(*target, mgr);
}

// @ fn_44_4D4C
bool CMinorIng::HasCoverPoint(CStateManager& mgr, const CTriggerData&) const {
  return GetCoverHint(mgr, mCoverHintId) != nullptr;
}

// @ fn_44_4A74
bool CMinorIng::PointPathOver(CStateManager&, const CTriggerData&) const {
  const bool over = mIngSpotPathFindNavigation.IsPathOver(*this);
  if (over) {
    const_cast< CMinorIng* >(this)->mPointPathFinding = false;
  }
  return over;
}

// @ fn_44_4A40
bool CMinorIng::FoundPointPath(CStateManager&, const CTriggerData&) const {
  return mIngSpotPathFindNavigation.HasPath(*this);
}

// @ fn_44_4A68
bool CMinorIng::IsPointPathFinding(CStateManager&, const CTriggerData&) const {
  return mPointPathFinding;
}

// @ fn_44_3E00
bool CMinorIng::StunOver(CStateManager&, const CTriggerData&) const { return mStunTimer <= 0.f; }

// @ fn_44_3D10
bool CMinorIng::ShouldPathFind(CStateManager& mgr, const CTriggerData& data) const {
  if (!InMaxRange(mgr, data) || !LineOfSight(mgr, data) || ShouldMoveUnderBall(mgr, data)) {
    return true;
  }
  return !mPointPathFinding &&
         mPathFindSearch.OnPath(RaisedPoint(GetTranslation())) == CPathFindSearch::kR_Success;
}

// @ fn_44_3DE0
bool CMinorIng::CancelAnim(CStateManager&, const CTriggerData&) const {
  return mKnockAnimCancel || mShouldEvaporate;
}

// @ fn_44_62A8
bool CMinorIng::HasAttackPattern(CStateManager& mgr, const CTriggerData&) const {
  return GetConnectedObject(mgr, kSS_Attack, kSM_Follow) != kInvalidUniqueId;
}

// @ fn_44_6268
bool CMinorIng::AttackPatternOver(CStateManager&, const CTriggerData&) const {
  if (mFollowingAttackPath) {
    const_cast< CMinorIng* >(this)->mFollowingAttackPath =
        mWaypointNavigation.GetDestination() != kInvalidUniqueId;
  }
  return !mFollowingAttackPath;
}

// @ fn_44_6210
bool CMinorIng::IsFollowingAttackPath(CStateManager&, const CTriggerData&) const {
  return mFollowingAttackPath;
}

// @ fn_44_6240
bool CMinorIng::AllowAttackPatternProjectile(CStateManager&, const CTriggerData&) const {
  return mAttackPatternProjectile || (mData.mFlags156 & 0x80) != 0;
}

// @ fn_44_6234
bool CMinorIng::AllowAttackPatternStun(CStateManager&, const CTriggerData&) const {
  return (mData.mFlags156 & 0x40) != 0;
}

// @ fn_44_6228
bool CMinorIng::RecalculatePath(CStateManager&, const CTriggerData&) const {
  return mRecalculatePath;
}

// @ fn_44_621C
bool CMinorIng::InsideSafeZone(CStateManager&, const CTriggerData&) const {
  return mInsideSafeZone;
}

// @ fn_44_6300
bool CMinorIng::IsMoveAwayFromSafeZoneOver(CStateManager& mgr, const CTriggerData& data) const {
  if (!mFoundMovementPos) {
    return false;
  }
  if (!mPointPathFinding) {
    const bool shagged = PathShagged(mgr, data);
    return CPatterned::PathOver(mgr, data) || shagged;
  }
  return PointPathOver(mgr, data);
}

// @ fn_44_3430
bool CMinorIng::FoundMovementPos(CStateManager&, const CTriggerData&) const {
  return mFoundMovementPos;
}

// @ fn_44_5D28
bool CMinorIng::ShouldEvaporate(CStateManager&, const CTriggerData&) const {
  return mShouldEvaporate;
}

// @ fn_44_5D08
bool CMinorIng::IsInCorporealForm(CStateManager&, const CTriggerData&) const {
  return mForm == kF_Corporeal || mForm == kF_IntoCorporeal;
}

// @ fn_44_5CFC
bool CMinorIng::StayOnPointPathFinding(CStateManager&, const CTriggerData&) const {
  return (mData.mFlags156 & 0x10) != 0;
}

// @ fn_44_5CF0
bool CMinorIng::AllowProjectileDuringStayOnPointPathFinding(CStateManager&,
                                                            const CTriggerData&) const {
  return (mData.mFlags156 & 0x08) != 0;
}

// @ fn_44_2038
bool CMinorIng::StateOver(CStateManager&, const CTriggerData&) const {
  return mAnimationState.GetState() == CAnimationState::kAS_Over;
}

// @ fn_44_5CAC
bool CMinorIng::IsLuredBySafeZone(CStateManager&, const CTriggerData&) const {
  return mSafeZoneId != kInvalidUniqueId && mForm >= kF_Corporeal && mForm < kF_IntoCorporeal;
}

// ---- states --------------------------------------------------------------------------------

// @ fn_44_8128
void CMinorIng::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (msg == kStateMsg_Activate) {
    if (!mIdle) {
      LogState("PathFind");
    }
    ReleaseCoverHint(mgr, mCoverHintId, false);
    mRecalculatePath = false;
    mPathFindActive = true;
    if (!mIdle && mSafeZoneId == kInvalidUniqueId) {
      SelectTarget(mgr, dt);
    }
  } else if (msg == kStateMsg_Update) {
    if (target == nullptr) {
      mTargetId = kInvalidUniqueId;
    }
  } else if (msg == kStateMsg_Deactivate) {
    mPathFindActive = false;
    mFoundMovementPos = false;
  }
  if (mPathFindActive && mFoundMovementPos) {
    mPathFindNavigation.PathFind(mgr, msg, dt, *this);
    ApplySeparation(mgr);
    MoveAlongPath(mgr, dt);
  }
}

// @ fn_44_7F98
void CMinorIng::PointPathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    if (!mIdle) {
      LogState("PointPathFind");
    }
    mPathFindActive = true;
    mPointPathFinding = true;
  } else if (msg == kStateMsg_Deactivate) {
    mPathFindActive = false;
    mFoundMovementPos = false;
    mLastCoverId = kInvalidUniqueId;
  }
  if (msg == kStateMsg_Activate || mIngSpotPathFindNavigation.HasPath(*this)) {
    mIngSpotPathFindNavigation.PathFind(mgr, msg, dt, *this);
    const CVector3f move = BodyController()->CommandMgr().GetMoveVector();
    BodyController()->CommandMgr().ClearLocomotionCmds();
    float speed = CVector3f::GetAngleDiff(CVector3f::Up(), GetTransform().GetUp()) / 3.1415927f *
                      (mData.mIngSpotMaxWallSpeed - mData.mIngSpotMaxSpeed) +
                  mData.mIngSpotMaxSpeed;
    if (mSafeZoneId != kInvalidUniqueId || mSafeZoneEnterCount != 0) {
      speed *= mData.mSpeedModifier;
    }
    MoveOnSurface(speed, dt, move);
  }
}

// @ fn_44_85DC
void CMinorIng::Stunned(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("Stunned");
    mStunTimer = mSafeZoneEnterCount == 0 ? mData.mBombStunDuration : 2.f;
    SetForm(mgr, kF_IntoCorporeal, false);
    mAnimationState.SetState(CAnimationState::kAS_Ready);
  } else if (msg == kStateMsg_Update) {
    mStunTimer -= dt;
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopReaction)) {
      BodyController()->CommandMgr().DeliverCmd(CBCLoopReactionCmd(pas::kRT_Three));
    }
    if (StunOver(mgr, CTriggerData(0.f))) {
      BodyController()->CommandMgr().DeliverCmd(kBSC_ExitState);
      SetForm(mgr, kF_IntoPuddle, mCancelStun);
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    SetForm(mgr, kF_Puddle, false);
    mPuddleHitByWeapon = false;
    mCancelStun = false;
    mPuddleHitTimer = 0.f;
    mTimerD1C = 1.f;
    ResetAttackTimer(mgr);
  }
}

// @ fn_44_777C
void CMinorIng::ProjectileAttack(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("ProjectileAttack");
    mAnimationState.SetState(CAnimationState::kAS_Ready);
  } else if (msg == kStateMsg_Update) {
    if (mForm == kF_Corporeal &&
        mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_ProjectileAttack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCProjectileAttackCmd(pas::kS_Zero, mTargetPos, false));
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    EndAttack(mgr, true);
    mAttackPatternProjectile = false;
  }
}

// @ fn_44_7740
void CMinorIng::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("Patrol");
    mTargetId = kInvalidUniqueId;
  }
  CPatterned::Patrol(mgr, msg, dt);
}

// @ fn_44_6EB8
void CMinorIng::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("Dead");
  }
  if (msg != kStateMsg_Update) {
    return;
  }
  mEvaporate = mEvaporate + dt;
  if (mEvaporate > 2.f) {
    mEvaporate = 2.f;
  }
  if (mShouldEvaporate) {
    // Retail waits while the blob-effect actor (0xcdc) still reports itself active; the host
    // never creates one, so the lookup always fails and the actor deletes itself.
    bool remove = true;
    if (const CActor* blob = TCastToConstPtr< CActor >(mgr.GetObjectById(mBlobEffectId))) {
      remove = !blob->GetActive();
    }
    if (remove) {
      mgr.DeleteObjectRequest(GetUniqueId());
    }
  }
}

// @ fn_44_5264
void CMinorIng::SetWallPointCoverDest(CStateManager& mgr, EStateMsg msg, float) {
  if (msg != kStateMsg_Activate) {
    return;
  }
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
  // [?] the second filter's third argument is not visible in the pseudocode; the register still
  // holds the -1 of the first.
  int to = -1;
  mPointPathFindSearch.FindClosestPhysicalPoint(hint->GetTranslation(), to,
                                                CPathFindPointSearchFilter(20.f, 0, -1));
  if (to != -1 && PortIngPointSearch(mPointPathFindSearch, *mPathArea, from, to)) {
    mFoundMovementPos = true;
  }
}

// @ fn_44_4C9C
void CMinorIng::SetCoverDest(CStateManager& mgr, EStateMsg msg, float) {
  if (msg != kStateMsg_Activate) {
    return;
  }
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

// @ fn_44_8460
void CMinorIng::IntoPuddleForm(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("IntoPuddleForm");
    SetForm(mgr, kF_IntoPuddle, true);
    mAnimationState.SetState(CAnimationState::kAS_Ready);
  } else if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCGenerateCmd(pas::kGType_One, CVector3f::Zero()));
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    SetForm(mgr, kF_Puddle, true);
    ResetAttackTimer(mgr);
  }
}

// @ fn_44_82A0
void CMinorIng::IntoCorporealForm(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("IntoCorporealForm");
    SetForm(mgr, kF_IntoCorporeal, true);
    mAnimationState.SetState(CAnimationState::kAS_Ready);
  } else if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCGenerateCmd(pas::kGType_Zero, CVector3f::Zero()));
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    SetForm(mgr, kF_Corporeal, true);
  }
}

// @ fn_44_7960
void CMinorIng::Idle(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("Idle");
    mIdle = true;
    if (!mPointPathFinding) {
      // A random point 2..10 units away at a random heading, on the mesh and clear.
      const float angle = mgr.Random()->Range(0.f, 360.f);
      const float dist = mgr.Random()->Range(2.f, 10.f);
      const CVector3f offset =
          CTransform4f::RotateZ(CRelAngle::FromDegrees(angle)) * (CVector3f::Forward() * dist);
      CVector3f point = GetTranslation() + offset + CVector3f::Up();
      if (ProjectOntoMesh(point) && IsMoveClear(mgr, point - GetTranslation(), false) &&
          mPathFindSearch.PathExists(RaisedPoint(GetTranslation()), RaisedPoint(point)) ==
              CPathFindSearch::kR_Success) {
        mPathFindNavigation.SetDestination(point);
        mFoundMovementPos = true;
      }
    } else if (CPFArea* area = mPathArea) {
      // A random neighbour of the closest physical point in the point graph.
      int closest = -1;
      mPointPathFindSearch.FindClosestPhysicalPoint(GetTranslation(), closest,
                                                    CPathFindPointSearchFilter(20.f, 0, -1));
      if (closest != -1) {
        const CPFPoint& from = area->GetPoint(closest);
        if (from.GetNumLinks() != 0) {
          const int link = from.GetLink(mgr.Random()->Range(0, from.GetNumLinks() - 1));
          if (link != -1) {
            const CPFPoint& to = area->GetPoint(link);
            if (IsMoveClear(mgr, to.GetPosition() - GetTranslation(), false) &&
                mPointPathFindSearch.Search(from, to) == CPathFindPointSearch::kR_Success) {
              mFoundMovementPos = true;
            }
          }
        }
      }
    }
  } else if (msg == kStateMsg_Deactivate) {
    mIdle = false;
  }
  if (!mPointPathFinding) {
    PathFind(mgr, msg, dt);
  } else {
    PointPathFind(mgr, msg, dt);
  }
}

// @ fn_44_6A40
void CMinorIng::FollowAttackPattern(CStateManager& mgr, EStateMsg msg, float dt) {
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);
  if (msg == kStateMsg_Activate) {
    LogState("FollowAttackPattern");
    const TUniqueId dest = mAttackPathId == kInvalidUniqueId
                               ? GetConnectedObject(mgr, kSS_Attack, kSM_Follow)
                               : mAttackPathId;
    mWaypointNavigation.SetDestination(dest);
    if (const CScriptWaypoint* waypoint =
            TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(dest))) {
      if (CVector3f::Dot(waypoint->GetTranslation() - GetTranslation(),
                         GetTransform().GetForward()) <= 0.f) {
        mWaypointNavigation.SetInPosition(true);
      }
    }
    mFollowingAttackPath = true;
  } else if (msg == kStateMsg_Update) {
    if (!mAttackPatternProjectile) {
      MoveAlongPath(mgr, dt);
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAttackPathLastId = mWaypointNavigation.GetLastDestination();
    mAttackPathId = mWaypointNavigation.GetDestination();
  }
}

// @ fn_44_6BD4
void CMinorIng::MoveAwayFromSafeZone(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("MoveAwayFromSafeZone");
    mInsideSafeZone = false;
    mFoundMovementPos = false;
    mPathFindSearch.SetFlags(mPathFindSearch.GetRegionFlags() & ~0x300u);
    if (mFollowingAttackPath && (mData.mFlags156 & 0x40) != 0) {
      mFollowingAttackPath = false;
    }
    mPointPathFinding = false;
    mMoveAwayTimer = 0.1f;
    for (int i = 0; i < 3; ++i) {
      FindCoverPoint(mgr, dt);
      if (!HasCoverPoint(mgr, CTriggerData(0.f))) {
        continue;
      }
      SetPointCoverDest(mgr, dt);
      if (FoundPointPath(mgr, CTriggerData(0.f))) {
        mFoundMovementPos = true;
        mPointPathFinding = true;
        break;
      }
      if (const CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId)) {
        // Retail also projects its own position onto the mesh here (fn_44_8B38) and never reads
        // the result.
        if (mPathFindSearch.PathExists(RaisedPoint(GetTranslation()), hint->GetTranslation()) ==
            CPathFindSearch::kR_Success) {
          mPathFindNavigation.SetDestination(hint->GetTranslation());
          mFoundMovementPos = true;
          break;
        }
      }
    }
    if (!mFoundMovementPos) {
      MassiveDeath(mgr); // vtable slot 0xd4
    }
  } else if (msg == kStateMsg_Update) {
    mMoveAwayTimer -= dt;
  } else if (msg == kStateMsg_Deactivate) {
    mFoundMovementPos = false;
    mMoveAwayTimer = 0.f;
    mPathFindSearch.SetFlags(mPathFindSearch.GetRegionFlags() | 0x300);
    ReleaseCoverHint(mgr, mCoverHintId, false);
  }
  if (mFoundMovementPos && (msg == kStateMsg_Activate || mMoveAwayTimer < 0.f)) {
    if (!mPointPathFinding) {
      PathFind(mgr, msg, dt);
    } else {
      PointPathFind(mgr, msg, dt);
    }
  }
}

// @ fn_44_7D20
void CMinorIng::AlwaysPointPathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  const CPathFindPointSearchFilter nearSelf(mData.mUnknownD6C8EAC2, 0, -1);
  const CPathFindPointSearchFilter nearTarget(mData.mUnknown2A5449BA, 0, -1);
  if (msg == kStateMsg_Update) {
    int from = -1;
    if (mPointPathFindSearch.FindClosestPhysicalPoint(GetTranslation(), from, nearSelf) ==
            CPathFindPointSearch::kCPR_Success &&
        from != -1) {
      CVector3f toTarget = mTargetPos - GetTranslation();
      if (toTarget.CanBeNormalized() &&
          toTarget.MagSquared() < mData.mUnknownD6C8EAC2 * mData.mUnknownD6C8EAC2) {
        toTarget.Normalize();
        BodyController()->CommandMgr().DeliverCmd(CBCLocomotionCmd(toTarget, toTarget, 2.f));
      }
    }
  } else if (msg == kStateMsg_Activate) {
    LogState("AlwaysPointPathFind");
    mPointPathFinding = true;
    mFoundMovementPos = false;
    int from = -1;
    mPointPathFindSearch.FindClosestPhysicalPoint(GetTranslation(), from, nearSelf);
    if (from != -1) {
      SelectTarget(mgr, dt);
      int to = -1;
      if (mPointPathFindSearch.FindClosestPhysicalPoint(mTargetPos, to, nearTarget) ==
              CPathFindPointSearch::kCPR_Success &&
          to != -1 && mPathArea != nullptr &&
          PortIngPointSearch(mPointPathFindSearch, *mPathArea, from, to)) {
        mFoundMovementPos = true;
      }
    }
  }
  if (mFoundMovementPos) {
    PointPathFind(mgr, msg, dt);
  }
}

// @ fn_44_6A3C
void CMinorIng::WaitForFSMTransition(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("WaitForFSMTransition");
  }
}

// ---- code functions (the FSM2 runs them: CGenericFSM2State::SetCodeFunctions) -----------

// @ fn_44_8C1C
void CMinorIng::SelectTarget(CStateManager& mgr, float) {
  if (!mPathFindActive) {
    mFoundMovementPos = false;
  }
  SetHitByPlayerProjectile(false);
  CPlayer* player = mgr.GetPlayer(0);
  if (player == nullptr) {
    return;
  }
  CVector3f dest = player->GetTranslation();
  mTargetPos = dest;
  mTargetId = player->GetUniqueId();
  mLineOfSight.SetTarget(mTargetId);
  if (ShouldPathFind(mgr, CTriggerData(0.f)) && ProjectOntoMesh(dest)) {
    mPathFindNavigation.SetDestination(dest);
    mPathFindNavigation.SetFaceTarget(player->GetUniqueId());
    mFoundMovementPos = true;
  }
}

// fn_44_4DDC: the target's aim position (vtable 0x54, dt 0) into `out`.
bool CMinorIng::TargetAimPosition(CStateManager& mgr, CVector3f& out) const {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return false;
  }
  out = target->GetAimPosition(mgr, 0.f);
  return true;
}

// @ fn_44_4E84
void CMinorIng::FindCoverPoint(CStateManager& mgr, float) {
  ReleaseCoverHint(mgr, mCoverHintId, false);
  if (!TargetAimPosition(mgr, mTargetPos)) {
    return;
  }
  // Retail keeps the four best candidates, sorted by score, in a static reserved_vector.
  struct SCandidate {
    SCandidate() : id(kInvalidUniqueId), score(0.f) {}
    TUniqueId id;
    float score;
  };
  SCandidate best[4];
  int count = 0;
  // [?] retail walks the object list at mgr+0x838, taken to be kOL_AiWaypoint (the list the other
  // AI-hint scans use).
  const CObjectList& list = mgr.GetObjectListById(kOL_AiWaypoint);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CScriptAIHint* hint = TCastToConstPtr< CScriptAIHint >(list[i]);
    // [?] hint type 5 has no name in CScriptAIHint::EHintType.
    if (hint == nullptr || !hint->GetActive() ||
        hint->GetHintType() != static_cast< CScriptAIHint::EHintType >(5) ||
        hint->GetInUse(GetUniqueId()) || hint->GetCurrentAreaId() != GetCurrentAreaId() ||
        hint->GetUniqueId() == mLastCoverId ||
        mgr.GetSafeZoneManager()->PointIsInSafeZone(mgr, hint->GetTranslation())) {
      continue;
    }
    const CVector3f delta = mTargetPos - hint->GetTranslation();
    const float up = CVector3f::Dot(CVector3f::Up(), delta);
    float score = delta.MagSquared();
    if (up > 0.f) {
      score += up * 4.f * up;
    }
    int slot;
    if (count < 4) {
      slot = count++;
    } else if (score < best[3].score) {
      slot = 3;
    } else {
      continue;
    }
    best[slot].id = hint->GetUniqueId();
    best[slot].score = score;
    for (; slot > 0 && best[slot].score < best[slot - 1].score; --slot) {
      const SCandidate swap = best[slot];
      best[slot] = best[slot - 1];
      best[slot - 1] = swap;
    }
  }
  if (count != 0) {
    if (CScriptAIHint* hint = TCastToPtr< CScriptAIHint >(mgr.ObjectById(best[0].id))) {
      hint->SetInUse(true); // fn_44_2550
      mCoverHintId = hint->GetUniqueId();
      mLastCoverId = mCoverHintId;
    }
  }
}

// @ fn_44_4AC4
void CMinorIng::SetPointCoverDest(CStateManager& mgr, float) {
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

// @ fn_44_21A8
void CMinorIng::SetLuredDest(CStateManager& mgr, float) {
  CVector3f dest;
  mFoundMovementPos =
      PortIngLuredDest(mgr, mPathFindSearch, mSafeZoneId, GetTranslation(), dest);
  mPathFindNavigation.SetDestination(dest);
  mPathFindNavigation.SetFaceTarget(kInvalidUniqueId);
}

// @ fn_44_22B0
void CMinorIng::SetLuredPointDest(CStateManager& mgr, float) {
  mFoundMovementPos = PortIngLuredPointDest(mgr, mPathFindSearch, mPointPathFindSearch, mPathArea,
                                            mSafeZoneId, GetTranslation());
}

// ---- overrides -----------------------------------------------------------------------------

// @ fn_44_8FCC
void CMinorIng::PreThink(float dt, CStateManager& mgr) {
  EnsureBodyActive(mgr);
  // [?] retail also requires bit 0x20 of the word at +0x68 to be clear.
  if (mForm == kF_Puddle) {
    const CPlane plane = mSurfaceAlignment.GetSurface().GetPlane();
    if (CVector3f::Dot(mSurfaceMoveDir, plane.GetNormal()) < 0.95f) {
      const CVector3f pos = GetTranslation();
      const float t = dt * 0.6f;
      const float dist = plane.GetHeight(pos) - 0.1f;
      const CVector3f onPlane = pos - plane.GetNormal() * dist;
      SetTranslation(pos * (1.f - t) + onPlane * t);
    }
    mSurfaceMoveDir = CVector3f::Zero();
  }
  CPatterned::PreThink(dt, mgr);
}

// @ fn_44_8E5C
void CMinorIng::Think(float dt, CStateManager& mgr) {
  if (GetActive()) {
    EnsureBodyActive(mgr);
    CPatterned::Think(dt, mgr);
    UpdateTargetPos(mgr);
    // [?] fn_44_30F4 moves the collision actor to the lock-on locator; there is none on the host.
    mLineOfSight.Update(dt, mgr);
    UpdateBallPursuit(mgr);
    UpdateTimers(dt);
    mSurfaceAlignment.Update(*this, mgr, dt);
    UpdateValidTarget(mgr);
    UpdateLoopSfx();
    mTouched = false;
    if (GetAlive()) {
      mKnockAnimCancel = false;
      if (mForm == kF_Puddle) {
        mKnockDamage = 0.f;
      } else if (mData.mUnknownA03E450C < mKnockDamage) {
        mKnockAnimCancel = true;
      }
      KnockBackController().EnableAnimReaction(CKnockBackMgr::kAR_KnockBack, !mKnockAnimCancel);
    }
    UpdateSafeZone(mgr);
  }
  // [?] fn_44_458C keeps the blob-effect actor on the Ing; there is none on the host.
}

// @ fn_44_9150
void CMinorIng::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  CPatterned::AcceptScriptMsg(mgr, msg);
  const TUniqueId sender = msg.GetSenderId();
  switch (msg.GetMessage()) {
  case kSM_Create:
    SnapToFloor(mgr);
    AddMaterial(kMT_NoStaticCollision, mgr);
    RemoveMaterial(kMT_Solid, mgr);
    BodyController()->Activate(mgr, pas::kAS_Invalid);
    // [?] fn_44_319C (collision actor) and fn_44_48F0 (blob effect from the ing-spot PART) are
    // not created on the host.
    SetForm(mgr, kF_Puddle, true);
    UpdateLoopSfx();
    ResetAttackTimer(mgr);
    break;
  case kSM_Deactivate:
    StopSfx();
    break;
  case kSM_Activate:
    JoinTeam(mgr);
    break;
  case kSM_AreaLoaded: {
    const rstl::vector< SConnection >& conns = GetConnectionList();
    for (int i = 0; i < conns.size(); ++i) {
      if (conns[i].state == kSS_Patrol && conns[i].msg == kSM_Follow) {
        mHasPatrolPath = true;
        break;
      }
    }
    CPFArea* area =
        mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea;
    mPathArea = area;
    mPathFindSearch.SetArea(area);
    mPointPathFindSearch = CPathFindPointSearch(area);
    JoinTeam(mgr);
    break;
  }
  case kSM_Increment: {
    const CScriptAIWaypoint* wp = TCastToConstPtr< CScriptAIWaypoint >(mgr.GetObjectById(sender));
    if (wp != nullptr && (wp->GetFlags() & 1) != 0) {
      mAttackPatternProjectile = true;
    }
    break;
  }
  case kSM_XENZ:
    if (TCastToConstPtr< CScriptSafeZone >(mgr.GetObjectById(sender)) != nullptr &&
        mMoveAwayTimer <= 0.f) {
      ++mSafeZoneEnterCount;
      mSafeZoneId = sender;
      mInsideSafeZone = true;
    }
    break;
  case kSM_XEXZ:
    if (TCastToConstPtr< CScriptSafeZone >(mgr.GetObjectById(sender)) != nullptr &&
        --mSafeZoneEnterCount == 0) {
      mSafeZoneId = kInvalidUniqueId;
      mInsideSafeZone = false;
    }
    break;
  case kSM_Damage:
    // [?] fn_44_25E4 (puddle: damage particles + hit sfx) and fn_44_29D4 (collision-actor relay).
    if (mForm == kF_Puddle && mDamageFxTimer <= 0.f) {
      PlaySfx(4, true);
      mDamageFxTimer = 0.111f;
    }
    break;
  case kSM_Delete:
    ReleaseCoverHint(mgr, mCoverHintId, false);
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamId))) {
      team->QuitTeam(GetUniqueId());
    }
    StopSfx();
    break;
  case kSM_ResistedDamage:
    if (mForm == kF_Puddle && mPuddleHitTimer <= 0.f) {
      const CWeapon* weapon = TCastToConstPtr< CWeapon >(mgr.GetObjectById(sender));
      if (weapon != nullptr &&
          TCastToConstPtr< CPlayer >(mgr.GetObjectById(weapon->GetOwnerId())) != nullptr) {
        const CWeaponTypeVulnerability vuln = GetDamageVulnerability()->GetVulnerability(
            weapon->GetCurrentDamageInfo().GetWeaponMode());
        if (!close_enough(vuln.mDamageMultiplier, 0.f) ||
            vuln.mEffect == CWeaponTypeVulnerability::kE_Immune) {
          mPuddleHitByWeapon = true;
        }
      }
    }
    break;
  default:
    break;
  }
}

void CMinorIng::PreRender(CStateManager& mgr) { CPatterned::PreRender(mgr); }

// @ fn_44_8E30
void CMinorIng::Render(const CStateManager& mgr) const {
  if (mForm != kF_Puddle) {
    CPatterned::Render(mgr);
  }
}

// @ fn_44_424C
void CMinorIng::Touch(CActor& actor, CStateManager& mgr) {
  EnsureBodyActive(mgr);
  if (mCurDamageRemTime <= 0.f && actor.GetUniqueId() == mTargetId) {
    const CDamageInfo damage = mForm == kF_Puddle ? mData.mIngSpotDamage : GetContactDamage();
    mgr.ApplyDamage(GetUniqueId(), mTargetId, GetUniqueId(), damage, SolidFilter(),
                    CVector3f::Zero());
    mTouched = true;
    mCurDamageRemTime = mDamageWaitTime;
  }
  CPatterned::Touch(actor, mgr);
}

// @ fn_44_76CC
void CMinorIng::Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) {
  EnsureBodyActive(mgr);
  // retail deactivates the blob effect first (fn_44_3004) while alive; there is none here.
  CPatterned::Death(mgr, direction, state);
}

// @ fn_44_8D2C
void CMinorIng::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  if ((mForm == kF_Puddle || mForm == kF_IntoPuddle) && GetHealthInfo()->GetHP() > 0.f) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::KnockBack(mgr, info);
}

// [?] fn_44_7058: retail also spawns the inglets (CBouncyGrenade, data at guest 0x8e0..0x8f8).
void CMinorIng::MassiveDeath(CStateManager& mgr) {
  PlaySfx(3, true);
  CPatterned::MassiveDeath(mgr);
}

// @ fn_44_6384
void CMinorIng::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                EUserEventType type, float dt) {
  if (type == kUE_Projectile) {
    const CTransform4f lctr = GetLctrTransform(node.GetLocatorName());
    const CEnergyProjectile* shot = LaunchProjectile(
        CTransform4f::LookAt(lctr.GetTranslation(), mTargetPos, CVector3f::Up()), mgr, 1,
        CWeapon::kPA_None, false, CImpactVisorEffect::None(), CVector3f(1.f, 1.f, 1.f));
    if (DebugMinorIng()) {
      fprintf(stderr, "[minoring] %04x projectile %s\n", GetUniqueId().Value(),
              shot != nullptr ? "launched" : "not launched");
    }
    return;
  }
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

// @ fn_44_8DBC
const CDamageVulnerability* CMinorIng::GetDamageVulnerability() const {
  if (mForm == kF_Puddle) {
    return &mData.mIngSpotVulnerability;
  }
  return CPatterned::GetDamageVulnerability();
}

// @ fn_44_38A8
bool CMinorIng::Listen(CStateManager& mgr, const CVector3f& position, EListenNoiseType type) {
  if (!GetAlive() || !GetActive()) {
    return false;
  }
  if (type == kLNT_PathObstruction) {
    if ((position - GetTranslation()).MagSquared() < 1600.f) {
      mRecalculatePath = true;
      return true;
    }
  } else if (type == kLNT_SafeZone) {
    PickSafeZone(mgr, position);
  }
  return false;
}

// @ fn_44_9758
void CMinorIng::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  stateMachine->SetCodeFunctions(skCodeFuncs, ARRAY_SIZE(skCodeFuncs));
  // Host: MP2_DEBUG_MINORING dumps the FSM2 graph once, so a log shows which triggers moved an
  // Ing between the states it prints.
  static bool sDumped = false;
  const CGenericFSM2* machine = GetStateMachine2();
  if (DebugMinorIng() && !sDumped && machine != nullptr) {
    sDumped = true;
    DumpFsm2(*machine);
  }
}

// Retail fn_44_110. The property ids and their order are the generated SLdrMinorIng reader's,
// which the analysis checked against the REL id by id: it reads all 20.
static CEntity* PortLoadMinorIng(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrMinorIng sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrMinorIng.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  const CIngSpotData ingSpot(sldrThis.ingSpot.blobEffect, sldrThis.ingSpot.hitNormalDamage,
                             sldrThis.ingSpot.hitHeavyDamage, sldrThis.ingSpot.death,
                             sldrThis.ingSpot.maxSpeed, sldrThis.ingSpot.maxWallSpeed,
                             sldrThis.ingSpot.ballPursuitSpeed, sldrThis.ingSpot.speedModifier,
                             sldrThis.ingSpot.turnSpeed,
                             LdrToDamageVulnerability(sldrThis.ingSpot.vulnerability),
                             static_cast< ushort >(sldrThis.ingSpot.sound_Idle),
                             static_cast< ushort >(sldrThis.ingSpot.sound_Move),
                             static_cast< ushort >(sldrThis.ingSpot.sound_HitNormalDamage),
                             static_cast< ushort >(sldrThis.ingSpot.sound_HitHeavyDamage),
                             static_cast< ushort >(sldrThis.ingSpot.sound_Death));

  // The three flag bytes: the REL's constructor takes them in an order that does not follow the
  // properties' declaration order, and the analysis does not pair the two, so the bits here follow
  // SLdrMinorIng's own field order. Nothing reads them in this milestone.
  uchar flags156 = 0;
  if (sldrThis.allowProjectileDuringAttackPattern) {
    flags156 |= 0x80;
  }
  if (sldrThis.unknown_0xbce16644) {
    flags156 |= 0x40;
  }
  if (sldrThis.unknown_0x142433d3) {
    flags156 |= 0x20;
  }
  if (sldrThis.stayOnPointPathFinding) {
    flags156 |= 0x10;
  }
  if (sldrThis.allowProjectileDuringStayOnPointPathFinding) {
    flags156 |= 0x08;
  }
  if (sldrThis.allowPuddleLockOn) {
    flags156 |= 0x04;
  }

  const CMinorIngData data(
      sldrThis.projectile, LdrToDamageInfo(sldrThis.projectileDamage),
      LdrToDamageInfo(sldrThis.ingSpot.damage), ingSpot, sldrThis.unknown_0xa03e450c,
      sldrThis.ingSpot.morphballPursuitDistance, sldrThis.ingSpot.bombStunDuration,
      sldrThis.ingSpot.unknown_0x7569fdba, sldrThis.ingSpot.sfxFallOff,
      sldrThis.attackAngleLimit, sldrThis.lineOfSightHeightOffset, sldrThis.unknown_0xd6c8eac2,
      sldrThis.unknown_0x2a5449ba, sldrThis.hearingRadius, sldrThis.ingSpot.speedModifier,
      sldrThis.unknown_0x3da35851.allowLockOn,
      static_cast< uchar >(sldrThis.unknown_0x09207f51), flags156, sldrThis.ingSpot.maxSpeed,
      sldrThis.ingSpot.maxWallSpeed, sldrThis.ingSpot.ballPursuitSpeed,
      LdrToDamageVulnerability(sldrThis.ingSpot.vulnerability),
      static_cast< ushort >(sldrThis.ingSpot.sound_Idle),
      static_cast< ushort >(sldrThis.ingSpot.sound_Move),
      static_cast< ushort >(sldrThis.ingSpot.sound_HitNormalDamage),
      static_cast< ushort >(sldrThis.ingSpot.sound_HitHeavyDamage),
      static_cast< ushort >(sldrThis.ingSpot.sound_Death));

  return rs_new CMinorIng(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                          LdrToEntityInfo(info, sldrThis.editorProperties),
                          LdrToTransform4f(sldrThis.editorProperties), *modelData,
                          LdrToPatternedInfo(sldrThis.patterned, nullptr),
                          LdrToActorParameters(sldrThis.actorInformation), data);
}

static SMinorIng_FuncPtrs REL_loader_MinorIng;

// Retail's RELMain stores fn_44_110 into the module's table and registers it with the DOL's
// SetSMinorIng_FuncPtrs (via fn_44_E0). The proxy in SMinorIng_FuncPtrs.cpp calls through it.
void RELMain() {
  REL_loader_MinorIng.mLoader = PortLoadMinorIng;
  SetSMinorIng_FuncPtrs(&REL_loader_MinorIng);
}

void RELExit() { SetSMinorIng_FuncPtrs(nullptr); }
