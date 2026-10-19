// Ingsmasher (CElitePirate, REL 15): the FSM movement states (patrol, pursue, turn, alert, wait, stun, attack pattern).
// Part of the module body in PortCElitePirate.cpp; analysis in docs/module-analysis/ElitePirate.md.
#include "MetroidPrime/Enemies/CElitePirate.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CSkinnedModel.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CPlane.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CAxisAngle.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CRumbleManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Cameras/CFirstPersonCamera.hpp"
#include "MetroidPrime/Collision/CJointCollisionDescription.hpp"
#include "MetroidPrime/Enemies/CElitePirateGrenadeLauncher.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrElitePirate.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIHint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CEnergyProjectile.hpp"
#include "MetroidPrime/Weapons/CGameProjectile.hpp"
#include "MetroidPrime/Weapons/CImpactVisorEffect.hpp"
#include "MetroidPrime/Weapons/WeaponTypes.hpp"
#include <float.h>
#include <math.h>
#include <stdio.h>

// @ fn_15_4790
// Records the attack kind the running state belongs to; on leaving it, pushes the kind onto a
// three-deep history (fn_15_47DC drops the oldest when full, fn_15_4848 being the erase) and resets.
void CElitePirate::SetAttackKind(int kind, EStateMsg msg) {
  if (msg == kStateMsg_Deactivate) {
    if (mAttackKindHistory.size() == 3) {
      mAttackKindHistory.erase(mAttackKindHistory.begin());
    }
    mAttackKindHistory.push_back(mAttackKind);
    mAttackKind = 0;
  } else {
    mAttackKind = kind;
  }
}

// @ fn_15_850C
// The shield (crouch) vulnerability and the absorb flag are up from a state's Activate to its Deactivate.
void CElitePirate::UpdateAbsorb(CStateManager& mgr, EStateMsg msg) {
  if (msg == kStateMsg_Activate) {
    SetShieldVulnerability(mgr, true);
    mAbsorbing = true;
  } else if (msg == kStateMsg_Deactivate) {
    SetShieldVulnerability(mgr, false);
    mAbsorbing = false;
  }
}

// @ fn_15_BEA0
// A taunt body command held for the state's duration: delivered on Activate, again whenever the body
// has left the taunt state while it is still wanted (retail's CTryCommand with state 0x10).
void CElitePirate::TauntCmd(EStateMsg msg, int tauntType) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(
        CBCTauntCmd(static_cast< pas::ETauntType >(tauntType)));
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCTauntCmd(static_cast< pas::ETauntType >(tauntType)));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// @ fn_15_BDFC
// The same retry shape for a locomotion command, with body state 8 (turn).
void CElitePirate::TurnCmd(EStateMsg msg, const CBCLocomotionCmd& cmd) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(cmd);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Turn)) {
      BodyController()->CommandMgr().DeliverCmd(cmd);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// @ fn_15_1A14
// True while the player stands on the current search path (retail: OnPath == success).
bool CElitePirate::IsPlayerOnPath(const CStateManager& mgr) const {
  return mPathFindSearch.OnPath(mgr.GetPlayer(0)->GetTranslation()) == CPathFindSearch::kR_Success;
}

// @ fn_15_13B8
// Walks the path back from its last point and claims (avoidance flag 4) the first free region at a
// segment midpoint other than the destination's own, so a second Ingsmasher routes around it.
void CElitePirate::ClaimPathRegion(CStateManager& mgr) {
  const rstl::reserved_vector< CVector3f, 16 >& waypoints = mPathFindSearch.GetWaypoints();
  if (waypoints.size() == 0) {
    return;
  }
  CPFArea* area = GetPathArea(mgr);
  const CPFRegion* lastRegion = area->FindClosestRegion(
      waypoints[waypoints.size() - 1], mPathFindSearch.GetRegionFlags(),
      mPathFindSearch.GetCreatureMask(), 2.f);
  for (int i = waypoints.size() - 1; i > 0; --i) {
    const CVector3f midpoint = 0.5f * (waypoints[i] + waypoints[i - 1]);
    CPFRegion* region = GetPathArea(mgr)->FindClosestRegion(
        midpoint, mPathFindSearch.GetRegionFlags(), mPathFindSearch.GetCreatureMask(), 2.f);
    if (region != nullptr && region != lastRegion && region->Data()->GetAvoidanceFlags() == 0) {
      region->Data()->SetAvoidanceFlags(region->Data()->GetAvoidanceFlags() | 4);
      mClaimedRegion = region->GetIndex();
      return;
    }
  }
}

// @ fn_15_1FC0
// Plans a path to `dest`, retrying with the same height, the midpoint to the previous goal, and the
// previous goal itself when the search fails; x810_ keeps the goal that was finally used.
void CElitePirate::PlanPath(CStateManager& mgr, float dt, const CVector3f& dest) {
  CVector3f goal = dest;
  mPathFindSearch.SetAvoidanceFilter(4);
  ReleaseClaimedRegion(mgr);
  mPathFindNavigation.SetDestination(goal);
  mPathFindNavigation.PathFind(mgr, kStateMsg_Activate, dt, *this);
  CTriggerData data(0);
  if (PathShagged(mgr, data)) {
    if (fabsf(dest.GetZ() - GetTranslation().GetZ()) > 2.f) {
      goal.SetZ(GetTranslation().GetZ());
      mPathFindNavigation.SetDestination(goal);
      mPathFindNavigation.PathFind(mgr, kStateMsg_Activate, dt, *this);
    }
    if (PathShagged(mgr, data)) {
      goal = CVector3f((dest.GetX() + x810_.GetX()) * 0.5f, (dest.GetY() + x810_.GetY()) * 0.5f,
                       (dest.GetZ() + x810_.GetZ()) * 0.5f);
      mPathFindNavigation.SetDestination(goal);
      mPathFindNavigation.PathFind(mgr, kStateMsg_Activate, dt, *this);
    }
    if (PathShagged(mgr, data)) {
      goal = x810_;
      mPathFindNavigation.SetDestination(goal);
      mPathFindNavigation.PathFind(mgr, kStateMsg_Activate, dt, *this);
    }
  }
  x810_ = goal;
  ClaimPathRegion(mgr);
}

// @ fn_15_5CBC
// Walks to `dest`: Activate plans the path; Update re-plans when the goal has moved 4 m, counts
// xa40_ (seconds stuck at the end of the path with the player off it), and steers along the path
// (or arrives at the goal when the search failed).
void CElitePirate::PathToward(CStateManager& mgr, EStateMsg msg, float dt, const CVector3f& dest) {
  if (msg == kStateMsg_Update) {
    const float dx = dest.GetX() - x810_.GetX();
    const float dy = dest.GetY() - x810_.GetY();
    if (dx * dx + dy * dy + 0.f > 16.f) {
      PlanPath(mgr, dt, dest);
    }
    const rstl::reserved_vector< CVector3f, 16 >& waypoints = mPathFindSearch.GetWaypoints();
    if (waypoints.size() > 0) {
      const CVector3f& last = waypoints[waypoints.size() - 1];
      const CVector3f toLast(GetTranslation().GetX() - last.GetX(),
                             GetTranslation().GetY() - last.GetY(), 0.f);
      if (toLast.Magnitude() < 4.f && !IsPlayerOnPath(mgr)) {
        xa40_ += dt;
        return;
      }
    }
    xa40_ = 0.f;
    CTriggerData data(0);
    CVector3f move;
    if (!PathShagged(mgr, data) && !mPathFindSearch.IsOver()) {
      mPathFindNavigation.PathFind(mgr, kStateMsg_Update, dt, *this);
      move = BodyController()->CommandMgr().GetMoveVector();
    } else {
      move = mSteeringBehaviors.Arrival(*this, dest, 7.f);
    }
    BodyController()->CommandMgr().DeliverCmd(CBCLocomotionCmd(move, CVector3f::Zero(), 1.f));
  } else if (msg == kStateMsg_Activate) {
    PlanPath(mgr, dt, dest);
    xa40_ = 0.f;
  }
}

void CElitePirate::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mHitByPlayerProjectile = false;
  }
  CPatterned::Patrol(mgr, msg, dt);
}

// Back to the patrol route: Activate aims at the patrol path's current point (or the leash
// position) and plans with the base class; Update steers to xb3c_ (the base PathFind, or an arrival
// when the search fails) and sets ReturnedToPatrol (xc10_04_) once the path is spent.
void CElitePirate::TargetPatrol(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    CTriggerData data(0);
    if (!PathShagged(mgr, data)) {
      if (!mPathFindSearch.IsOver()) {
        PathFind(mgr, msg, dt);
      } else {
        xc10_04_ = true;
      }
    } else {
      const CVector3f move = mSteeringBehaviors.Arrival(*this, xb3c_, 25.f);
      BodyController()->CommandMgr().DeliverCmd(CBCLocomotionCmd(move, CVector3f::Zero(), 1.f));
      xc10_04_ = true;
    }
  } else if (msg == kStateMsg_Activate) {
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    CTriggerData data(0);
    if (HasPatrolPath(mgr, data)) {
      SetDestPos(mWaypointNavigation.GetDestinationPosition());
    } else {
      SetDestPos(mLatestLeashPosition);
    }
    xb3c_ = mDestPos;
    if (GetSearchPath() != nullptr) {
      PathFind(mgr, msg, dt);
    }
    SetXDamaged(false);
    xc10_04_ = false;
    mCurPlayerLeashTime = 0.f;
  } else if (msg == kStateMsg_Deactivate) {
    mAlerted = false;
  }
}

// Chases the attack target (xa26_), or the last goal when it is gone. xa44_ marks when the chase began.
// @ fn_15_5BB4
void CElitePirate::Pursue(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    CVector3f dest = x810_;
    if (const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(xa26_))) {
      dest = target->GetTranslation();
    }
    PathToward(mgr, msg, dt, dest);
  } else if (msg == kStateMsg_Activate) {
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mAlerted = false;
    xa44_ = mTime;
    mKnockBackController.EnableAnimReaction(CKnockBackMgr::kAR_Flinch, true);
  }
}

// @ fn_15_5598
// Turns on the spot towards the attack target; xcb8_ holds the direction fixed at Activate.
void CElitePirate::Turn(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(xa26_));
    if (target == nullptr) {
      return;
    }
    const CVector3f toTarget = target->GetTranslation() - GetTranslation();
    if (!toTarget.CanBeNormalized()) {
      return;
    }
    xcb8_ = toTarget.AsNormalized();
  }
  CTriggerData data(0);
  if (ShouldAlert(mgr, data)) {
    UpdateAbsorb(mgr, msg);
  }
  TurnCmd(msg, CBCLocomotionCmd(CVector3f::Zero(), xcb8_.AsNormalized(), 1.f));
}

// @ fn_15_5340
void CElitePirate::Alert(CStateManager& mgr, EStateMsg msg, float dt) {
  SetAttackKind(1, msg);
  UpdateAbsorb(mgr, msg);
  TauntCmd(msg, xc3c_);
  if (msg == kStateMsg_Activate) {
    xc10_01_ = true;
    x810_ = GetTranslation();
  } else if (msg == kStateMsg_Deactivate) {
    SetXDamaged(false);
    SendLauncherMsg(mgr, true);
  }
}

void CElitePirate::InvulnAlert(CStateManager& mgr, EStateMsg msg, float dt) {
  UpdateAbsorb(mgr, msg);
  Alert(mgr, msg, dt);
}

void CElitePirate::Wait(CStateManager& mgr, EStateMsg msg, float) {
  UpdateAbsorb(mgr, msg);
  if (msg == kStateMsg_Activate) {
    BodyController()->CommandMgr().ClearLocomotionCmds();
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
  }
}

void CElitePirate::Stunned(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mShieldAlpha = 0.f;
  }
  TauntCmd(msg, 2);
}

// @ fn_15_6DDC
// Walks the waypoint chain linked by 'ATTK'/'FOLW'. A first waypoint behind the Ingsmasher counts
// as already in position.
void CElitePirate::FollowAttackPattern(CStateManager& mgr, EStateMsg msg, float dt) {
  SetAttackKind(3, msg);
  UpdateAbsorb(mgr, msg);
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);
  if (msg == kStateMsg_Activate) {
    mAlerted = false;
    const TUniqueId id = GetConnectedObject(mgr, kSS_Attack, kSM_Follow);
    mWaypointNavigation.SetDestination(id);
    if (const CScriptWaypoint* waypoint = TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(id))) {
      const CVector3f forward = GetTransform().GetForward();
      const CVector3f d = waypoint->GetTranslation() - GetTranslation();
      if (forward.GetX() * d.GetX() + forward.GetY() * d.GetY() + forward.GetZ() * d.GetZ() <= 0.f) {
        mWaypointNavigation.SetInPosition(true);
      }
    }
  }
}
