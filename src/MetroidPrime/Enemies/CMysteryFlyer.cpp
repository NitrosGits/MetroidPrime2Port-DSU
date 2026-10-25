#include "MetroidPrime/Enemies/CMysteryFlyer.hpp"

#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CObjectList.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrMysteryFlyer.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "REL/REL_Setup.h"

// Retail `__sinit_CMysteryFlyer_cpp` (0x2BF8-0x2F4C) writes exactly these three tables and nothing
// else: `lis` 0x2C24/0x2C0C/0x2E10 take the table addresses (.data 0x78, 0x190, 0x248), then 66 word
// copies (`stw` 0x2CCC-0x2F3C) put each entry's member pointer at +4/+8/+0xC as {adjust 0, vtoff -1
// (non-virtual), fn = address} - every entry is a direct pointer, no vtoff anywhere. Trigger names
// .rodata 0x2C/0x36/0x41/0x52/0x61/0x6E/0x74/0x83/0x8C/0x92 -> .text 0x2260/0x2224/0x21DC/0x21AC/
// 0x20C4/0x20A4/0x202C/0x1FFC/0x1FE0/0x1FD4; state names .rodata 0x9E/0xA4/0xAD/0xB8/0xBF/0xD0/0xD6/
// 0xDF/0xE6/0xEC -> .text 0x1FD0/0x1E14/0x1BEC/0x1B24/0x1A04/0x1998/0x1850/0x162C/0x13F0/0x15F4;
// code function names .rodata 0xF1/0xFD -> .text 0x1368/0x1348. Same names, order and targets as
// below; all 22 bodies already marked, and CMysteryFlyer.hpp declares no virtual at all, so the host
// arrays are direct pointers too (binary: skTriggers 0x2270160, skStates 0x2270060, skCodeFuncs
// 0x2270020, adjust 0). No call, no branch; the only other store is the register save stmw r14
// 0x2BFC (frame 0x90, restored lmw 0x2F40).
// @ fn_45_2BF8
static CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"StateOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMysteryFlyer::StateOver)},
    {"NearPatrol",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMysteryFlyer::NearPatrol)},
    {"InDetectionRange",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMysteryFlyer::InDetectionRange)},
    {"AttackPathOver",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMysteryFlyer::AttackPathOver)},
    {"ShouldAttack",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMysteryFlyer::ShouldAttack)},
    {"Leash", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMysteryFlyer::Leash)},
    {"ShouldApproach",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMysteryFlyer::ShouldApproach)},
    {"PathOver", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMysteryFlyer::PathOver)},
    {"Stuck", static_cast< CPatterned::StateMachine::TriggerFunc >(&CMysteryFlyer::Stuck)},
    {"LineOfSight",
     static_cast< CPatterned::StateMachine::TriggerFunc >(&CMysteryFlyer::LineOfSight)},
};

static CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Start", static_cast< CPatterned::StateMachine::StateFunc >(&CMysteryFlyer::Start)},
    {"Generate", static_cast< CPatterned::StateMachine::StateFunc >(&CMysteryFlyer::Generate)},
    {"GoToPatrol", static_cast< CPatterned::StateMachine::StateFunc >(&CMysteryFlyer::GoToPatrol)},
    {"Patrol", static_cast< CPatterned::StateMachine::StateFunc >(&CMysteryFlyer::Patrol)},
    {"FollowAttackPath",
     static_cast< CPatterned::StateMachine::StateFunc >(&CMysteryFlyer::FollowAttackPath)},
    {"Hover", static_cast< CPatterned::StateMachine::StateFunc >(&CMysteryFlyer::Hover)},
    {"Approach", static_cast< CPatterned::StateMachine::StateFunc >(&CMysteryFlyer::Approach)},
    {"Attack", static_cast< CPatterned::StateMachine::StateFunc >(&CMysteryFlyer::Attack)},
    {"Dodge", static_cast< CPatterned::StateMachine::StateFunc >(&CMysteryFlyer::Dodge)},
    {"Dead", static_cast< CPatterned::StateMachine::StateFunc >(&CMysteryFlyer::Dead)},
};

static CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    {"ResetAttack", static_cast< CPatterned::StateMachine::CodeFunc >(&CMysteryFlyer::ResetAttack)},
    {"CalcLineOfSight",
     static_cast< CPatterned::StateMachine::CodeFunc >(&CMysteryFlyer::CalcLineOfSight)},
};

// @ fn_45_2924
CMysteryFlyer::CMysteryFlyer(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                             const CTransform4f& xf, const CModelData& modelData,
                             const CActorParameters& actorParams,
                             const CPatternedInfo& patternedInfo, const SLdrMysteryFlyerData& data)
: CPatterned(kPAI_MysteryFlyer, uid, name, kFT_Zero, info, xf, modelData, patternedInfo, kMT_Flyer,
             kCT_One, kBT_AiMovedFlyer, actorParams)
, mData(data)
, mGenerateAnimId(patternedInfo.GetAnimationParameters().GetInitialAnimation())
, mPatrolWaypointId(kInvalidUniqueId)
, mShotProjectileInfo(data.shotProjectile, LdrToDamageInfo(data.shotDamage))
, mMoving(false)
, mHasLineOfSight(false)
, mAlerted(false)
, mHasAttackPath(false)
, mPathFindSearch(nullptr, 3, patternedInfo.GetPathfindingIndex(), 0.f, 0.f, 0,
                  CPFRegion::kRP_Center)
, mAttackInterval(1.f)
, mAttackTimer(0.f)
, mMoveDirection(CVector3f::Forward())
, mCurrentSpeed(0.f)
, mTeamAiMgrId(kInvalidUniqueId)
, mHoverHeight(data.hoverHeight)
, mStuckReferencePos(CVector3f::Zero())
, mStuckTimer(0.f)
, mLineOfSightTimer(10.f) {
  mShotProjectileInfo.Token().Lock();
}

// @ fn_45_2618
CMysteryFlyer::~CMysteryFlyer() {}

// @ fn_45_227C
void CMysteryFlyer::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  stateMachine->SetCodeFunctions(skCodeFuncs, ARRAY_SIZE(skCodeFuncs));
}

// @ fn_45_2308
void CMysteryFlyer::Think(float dt, CStateManager& mgr) {
  CPatterned::Think(dt, mgr);
  if (GetActive()) {
    UpdateMovement(mgr, dt);
    mLineOfSightTimer += dt;
  }
}

// fn_45_23B8
void CMysteryFlyer::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const EScriptObjectMessage message = msg.GetMessage();
  CPatterned::AcceptScriptMsg(mgr, msg);
  switch (message) {
  case kSM_Create:
    BodyController()->Activate(mgr, pas::kAS_Invalid);
    BodyController()->SetPlayDeathAnims(false);
    mHoverHeight += mgr.Random()->Float();
    break;
  case kSM_AreaLoaded:
    mPathFindSearch.SetArea(
        mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea);
    JoinTeam(mgr);
    mTeamAiMgrId = CScriptTeamAiMgr::GetAssociatedTeamId(*this, mgr);
    break;
  case kSM_Damage:
    if (BodyController()->HasBodyState(pas::kAS_KnockBack)) {
      if (BodyController()->GetCurrentStateId() != pas::kAS_KnockBack) {
        BodyController()->CommandMgr().DeliverCmd(
            CBCKnockBackCmd(GetTransform().GetForward(), pas::kS_Zero));
      }
    }
    // fallthrough
  case kSM_Alert:
    mAlerted = true;
    break;
  }
}

// @ fn_45_237C
void CMysteryFlyer::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                    EUserEventType type, float dt) {
  if (!(type == kUE_FadeIn ? true : false)) {
    CPatterned::DoUserAnimEvent(mgr, node, type, dt);
  }
}

// @ fn_45_2274
CProjectileInfo* CMysteryFlyer::ProjectileInfo() { return &mShotProjectileInfo; }

// @ fn_45_1FD0
void CMysteryFlyer::Start(CStateManager& mgr, EStateMsg msg, float dt) {}

// fn_45_1E14
// @ fn_45_1E14
void CMysteryFlyer::Generate(CStateManager& mgr, EStateMsg msg, float dt) {
  if (mData.needsToGenerate) {
    switch (msg) {
    case kStateMsg_Activate:
      mAnimationState.SetState(CAnimationState::kAS_Ready);
      break;
    case kStateMsg_Update:
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
        BodyController()->CommandMgr().DeliverCmd(
            CBCGenerateCmd(pas::kGType_Zero, mGenerateAnimId));
      }
      break;
    case kStateMsg_Deactivate:
      mAnimationState.SetState(CAnimationState::kAS_NotReady);
      break;
    }
  }
}

// fn_45_1BEC
// @ fn_45_1BEC
void CMysteryFlyer::GoToPatrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    QuitTeam(mgr);
    mPatrolWaypointId = FindConnectedObject(mgr, kSS_Patrol, kSM_Follow);
    if (mPatrolWaypointId != kInvalidUniqueId) {
      if (const CScriptWaypoint* waypoint =
              TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(mPatrolWaypointId))) {
        CVector3f offset = waypoint->GetTranslation() - GetTranslation();
        if (offset.CanBeNormalized()) {
          offset = 3.f * offset.Normalize();
        }
        mPathFindNavigation.SetDestination(waypoint->GetTranslation() - offset);
        mWaypointNavigation.SetLastDestination(
            waypoint->FindConnectedObject(mgr, kSS_Arrived, kSM_Next));
      } else {
        mPatrolWaypointId = kInvalidUniqueId;
      }
    }
    mMoving = true;
    break;
  }
  if (mPatrolWaypointId != kInvalidUniqueId) {
    mPathFindNavigation.PathFind(mgr, msg, dt, *this);
  }
  switch (msg) {
  case kStateMsg_Update:
    UpdateSteering(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    mMoving = false;
    break;
  }
}

// @ fn_45_1B24
void CMysteryFlyer::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mWaypointNavigation.SetLastDestination(kInvalidUniqueId);
    mMoving = true;
    break;
  }
  CPatterned::Patrol(mgr, msg, dt);
  switch (msg) {
  case kStateMsg_Update:
    UpdateSteering(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    mMoving = false;
    break;
  }
}

// fn_45_1A04
// @ fn_45_1A04
void CMysteryFlyer::FollowAttackPath(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    const TUniqueId attackPath = FindConnectedObject(mgr, kSS_Attack, kSM_Follow);
    mHasAttackPath = false;
    if (attackPath != kInvalidUniqueId) {
      mHasAttackPath = true;
      mWaypointNavigation.SetLastDestination(attackPath);
      mMoving = true;
    }
    break;
  }
  }
  if (mHasAttackPath) {
    CPatterned::Patrol(mgr, msg, dt);
  }
  switch (msg) {
  case kStateMsg_Update:
    UpdateSteering(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    mMoving = false;
    break;
  }
}

// @ fn_45_1998
void CMysteryFlyer::Hover(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    JoinTeam(mgr);
    break;
  case kStateMsg_Update:
    FaceTarget(mgr, dt);
    mAttackTimer += dt;
    break;
  }
}

// @ fn_45_1850
void CMysteryFlyer::Approach(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    const CTransform4f& playerXf = mgr.GetPlayer(0)->GetTransform();
#ifdef TARGET_PC
    // fn_45_1850: retail sums left to right, (translation + range * forward) + height * up:
    // 0x18E4/0x18EC/0x18F4 fadds f5/f3/f1 = translation (0x40/0x50/0x30) + x3E0 * forward,
    // then 0x1908-0x1910 fadds += x924 * sUpVector.
    mPathFindNavigation.SetDestination(
        (playerXf.GetTranslation() + mMaxAttackRange * playerXf.GetForward()) +
        mHoverHeight * CVector3f::Up());
#else
    mPathFindNavigation.SetDestination(
        playerXf.GetTranslation() +
        (mMaxAttackRange * playerXf.GetForward() + mHoverHeight * CVector3f::Up()));
#endif
    mMoving = true;
    break;
  }
  case kStateMsg_Update:
    mAttackTimer += dt;
    FaceTarget(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    mMoving = false;
    break;
  }
  mPathFindNavigation.PathFind(mgr, msg, dt, *this);
}

// @ fn_45_162C
void CMysteryFlyer::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamAiMgrId,
                                  GetUniqueId());
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_ProjectileAttack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCProjectileAttackCmd(pas::kS_Zero, mgr.GetPlayer(0)->GetAimPosition(mgr, 0.f), false));
    }
    FaceTarget(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    CScriptTeamAiMgr::EndAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamAiMgrId, GetUniqueId(),
                                true);
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// @ fn_45_13F0
void CMysteryFlyer::Dodge(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) {
      BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(
          static_cast< pas::EStepDirection >(mgr.Random()->Next() % 10), pas::kStep_Normal));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// @ fn_45_15F4
void CMysteryFlyer::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    MassiveDeath(mgr);
    break;
  }
}

// @ fn_45_1368
void CMysteryFlyer::ResetAttack(CStateManager& mgr, float dt) {
  const float attackTimeVariation = mAttackTimeVariation;
  const float averageAttackTime = GetAverageAttackTime();
  mAttackInterval = averageAttackTime + (mgr.Random()->Float() - 0.5f) * attackTimeVariation;
  mAttackTimer = 0.f;
}

// @ fn_45_1348
void CMysteryFlyer::CalcLineOfSight(CStateManager& mgr, float dt) { UpdateLineOfSight(mgr); }

// @ fn_45_2260
bool CMysteryFlyer::StateOver(CStateManager& mgr, const CTriggerData& data) const {
  return mAnimationState.IsOver();
}

// fn_45_2224
// @ fn_45_2224
bool CMysteryFlyer::NearPatrol(CStateManager& mgr, const CTriggerData& data) const {
  bool nearPatrol = true;
  if (mPatrolWaypointId != kInvalidUniqueId) {
    nearPatrol = mPathFindSearch.IsOver();
  }
  return nearPatrol;
}

// fn_45_21DC
// @ fn_45_21DC
bool CMysteryFlyer::InDetectionRange(CStateManager& mgr, const CTriggerData& data) const {
  return mAlerted || CPatterned::InDetectionRange(mgr, data);
}

// fn_45_21AC
// @ fn_45_21AC
bool CMysteryFlyer::AttackPathOver(CStateManager& mgr, const CTriggerData& data) const {
  return !mHasAttackPath || mWaypointNavigation.GetDestination() == kInvalidUniqueId;
}

// fn_45_20C4
// @ fn_45_20C4
bool CMysteryFlyer::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  bool shouldAttack = false;
  if (mAttackTimer > mAttackInterval) {
    const float distance = (mgr.GetPlayer(0)->GetTranslation() - GetTranslation()).Magnitude();
    shouldAttack = distance > mMinAttackRange && distance < mMaxAttackRange;
    if (shouldAttack && mTeamAiMgrId != kInvalidUniqueId) {
      shouldAttack = CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr,
                                                      mTeamAiMgrId, GetUniqueId());
    }
  }
  return shouldAttack;
}

// fn_45_20A4
// @ fn_45_20A4
bool CMysteryFlyer::Leash(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::Leash(mgr, data);
}

// @ fn_45_202C
bool CMysteryFlyer::ShouldApproach(CStateManager& mgr, const CTriggerData& data) const {
  const CVector3f toPlayer = mgr.GetPlayer(0)->GetTranslation() - GetTranslation();
  const float maxAttackRange = mMaxAttackRange;
  return toPlayer.Magnitude() > maxAttackRange;
}

// fn_45_1FFC
// @ fn_45_1FFC
bool CMysteryFlyer::PathOver(CStateManager& mgr, const CTriggerData& data) const {
  return mPathFindSearch.GetResult() || mPathFindSearch.IsOver();
}

// @ fn_45_1FE0
bool CMysteryFlyer::Stuck(CStateManager& mgr, const CTriggerData& data) const {
  return mStuckTimer > 0.5f;
}

// fn_45_1FD4
// @ fn_45_1FD4
bool CMysteryFlyer::LineOfSight(CStateManager& mgr, const CTriggerData& data) const {
  return mHasLineOfSight;
}

// fn_45_C10
void CMysteryFlyer::JoinTeam(CStateManager& mgr) {
  if (mTeamAiMgrId == kInvalidUniqueId) {
    mTeamAiMgrId = CScriptTeamAiMgr::GetAssociatedTeamId(*this, mgr);
  }
  if (mTeamAiMgrId != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamAiMgrId))) {
      team->JoinTeam(*this, CTeamAiRole::kTAR_Projectile, CTeamAiRole::kTAR_Unknown,
                     CTeamAiRole::kTAR_Invalid);
    }
  }
}

// fn_45_B68
// @ fn_45_B68
void CMysteryFlyer::QuitTeam(CStateManager& mgr) {
  if (mTeamAiMgrId != kInvalidUniqueId) {
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamAiMgrId))) {
      if (team->IsPartOfTeam(GetUniqueId())) {
        team->QuitTeam(GetUniqueId());
        mTeamAiMgrId = kInvalidUniqueId;
      }
    }
  }
}

// @ fn_45_F44
void CMysteryFlyer::FaceTarget(CStateManager& mgr, float dt) {
  CVector3f toPlayer = mgr.GetPlayer(0)->GetTranslation() - GetTranslation();
  toPlayer.SetZ(0.f);
  BodyController()->FaceDirection(toPlayer, dt);
}

// @ fn_45_A84
bool CMysteryFlyer::UpdateLineOfSight(CStateManager& mgr) {
  if (mLineOfSightTimer > 0.7f) {
    mLineOfSightTimer = 0.f;
    mHasLineOfSight =
        mgr.RayCollideWorld(GetTranslation(), mgr.GetPlayer(0)->GetAimPosition(mgr, 0.f),
                            CMaterialFilter::MakeExclude(CMaterialList(kMT_Player)), this);
  }
  return mHasLineOfSight;
}

// fn_45_CBC
// @ fn_45_CBC
CVector3f CMysteryFlyer::CalculateSeparation(CStateManager& mgr) {
  CVector3f total = CVector3f::Zero();
  float count = 0.f;
  const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CPatterned* other = TCastToConstPtr< CPatterned >(list[i]);
    if (other != nullptr && other != this && other->GetCurrentAreaId() == GetCurrentAreaId()) {
      const CVector3f separation =
          mSteeringBehaviors.Separation(*this, other->GetTranslation(), mData.separationDistance);
      if (separation.IsMagnitudeSafe()) {
        total += separation.AsNormalized();
        count += 1.f;
      }
    }
  }

  const CVector3f toPlayer = mgr.GetPlayer(0)->GetTranslation() - GetTranslation();
  const float maxAttackRange = mMaxAttackRange;
  if (toPlayer.Magnitude() < maxAttackRange) {
    const CVector3f separation =
        mSteeringBehaviors.Separation(*this, mgr.GetPlayer(0)->GetTranslation(), mMinAttackRange);
    if (separation.IsMagnitudeSafe()) {
      total += separation.AsNormalized();
      count += 1.f;
    }
    const float heightError = mHoverHeight + toPlayer.GetZ();
    const float absHeightError = CMath::AbsF(heightError);
    if (absHeightError > 1.f) {
      total.SetZ(total.GetZ() + heightError / absHeightError);
      count += 1.f;
    }
  }

  if (count > 0.f) {
    total *= 1.f / count;
  }
  return total;
}

// @ fn_45_1070
void CMysteryFlyer::UpdateMovement(CStateManager& mgr, float dt) {
  CVector3f desired = BodyController()->GetCommandMgr().GetMoveVector();
  desired = desired + CalculateSeparation(mgr);

  const float acceleration = dt * mData.hoverSpeed;
  if (mMoving) {
    mCurrentSpeed = CMath::Min(mData.hoverSpeed, mCurrentSpeed + acceleration);
  } else {
    mCurrentSpeed = rstl::max_val(0.f, mCurrentSpeed - acceleration);
    if (mCurrentSpeed == 0.f) {
      mMoveDirection = GetTransform().GetForward();
    }
  }

  if (mCurrentSpeed > 0.f && desired.IsMagnitudeSafe()) {
    mMoveDirection.Normalize();
    mMoveDirection = CVector3f::Slerp(mMoveDirection, desired.AsNormalized(),
                                      CRelAngle::FromDegrees(360.f * dt));
    const float distance = mCurrentSpeed * dt;
    MoveToInOneFrameWR(GetTranslation() + mMoveDirection * distance, dt);
    if (mMoving &&
        (GetTranslation() - mStuckReferencePos).Magnitude() < mCurrentSpeed * (0.2f * dt)) {
      mStuckTimer += dt;
    } else {
      mStuckTimer = 0.f;
      mStuckReferencePos = GetTranslation();
    }
  } else {
    mStuckTimer = 0.f;
    mStuckReferencePos = GetTranslation();
  }
}

// @ fn_45_FB0
void CMysteryFlyer::UpdateSteering(CStateManager& mgr, float dt) {
  CVector3f direction = GetTransform().GetForward();
  const TUniqueId destinationId = mWaypointNavigation.GetDestination();
  if (const CEntity* destination = mgr.GetObjectById(destinationId)) {
    const CActor* actor = static_cast< const CActor* >(destination);
    direction = actor->GetTranslation() - GetTranslation();
    direction.SetZ(0.f);
  }
  BodyController()->FaceDirection(direction, dt);
}

// fn_45_170
CEntity* LoadMysteryFlyer(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrMysteryFlyer sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrMysteryFlyer.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  return rs_new CMysteryFlyer(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), LdrToTransform4f(sldrThis.editorProperties),
      *modelData, LdrToActorParameters(sldrThis.actorInformation),
      LdrToPatternedInfo(sldrThis.patterned, nullptr), sldrThis.mysteryFlyerProperties);
}

#ifndef MONOLITHIC
// @ fn_45_140
static void SetFuncPtrs() {
  static SMysteryFlyer_FuncPtrs funcPtrs;
  funcPtrs.mLoader = &LoadMysteryFlyer;
  SetSMysteryFlyer_FuncPtrs(&funcPtrs);
}

// @ fn_45_120
extern "C" void RELMain() { SetFuncPtrs(); }

// @ fn_45_FC
extern "C" void RELExit() { SetSMysteryFlyer_FuncPtrs(nullptr); }
#endif
