// The 37 states of the SpacePirate state machine (retail table fn_72_10E50 -> lbl_72_data_604).
// Milestone 2 is the skeleton: a state CPatterned also has forwards to it, every other one is a
// no-op behind a once-per-name StandIn that names the retail function. Later lanes port the bodies
// from docs/module-analysis/SpacePirate.md and build/G2ME01/SpacePirate/m2c/fn_72_<off>.c.
#include "MetroidPrime/Enemies/CSpacePirate.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CPASDatabase.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAiJumpPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include <float.h>

void PortSpacePirateStandIn(const char* what);

void CSpacePirate::Ambushing(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Ambushing state (fn_72_7630)");
}

void CSpacePirate::WarpIn(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::WarpIn state (fn_72_7078)");
}

void CSpacePirate::WarpOut(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::WarpOut state (fn_72_6E6C)");
}

void CSpacePirate::PostWarpOut(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::PostWarpOut state (fn_72_6DA4)");
}

// Retail fn_72_9F1C is MP1's CSpacePirate::Attack with Echoes' own members. Activate stores
// kAS_Ready and combat locomotion, caches GetTargetPos (CB30) in xbc4_ and the delta from that point
// to the actor's own bounding-box centre in x908, zeroes the steering speed (rodata 0xB9C) and
// clears the melee bit; the close test is TooClose with a zero CTriggerData, and only enables the
// melee bit plus the blade. Unlike MP1 the turn test is not an else: retail always runs it, asking
// for a turn (zero move vector, the cached delta as the face vector, speed 1 = rodata 0xB54) when the
// forward axis dots the normalized delta below 0.8 (rodata 0xB64). Update runs the melee / target
// vector / blade / cloak block only while x8f6 bit 0x20 is set, and then always calls D800 and D720;
// Deactivate clears both x8f6 bits and returns the animation to kAS_NotReady.
void CSpacePirate::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    xbc4_ = GetTargetPos(mgr);
    x908_targetDelta = xbc4_ - GetBoundingBox().GetCenterPoint();
    x904_steeringSpeed = 0.f;
    x8f6_26_enableMeleeAttack = false;
    if (!x8f5_24_noMeleeAttack) {
      if (TooClose(mgr, CTriggerData(0.f))) {
        x8f6_26_enableMeleeAttack = true;
        x8f8_25_appliedBladeDamage = false;
      }
    }
    if (CVector3f::Dot(GetTransform().GetForward(), x908_targetDelta.AsNormalized()) < 0.8f) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCLocomotionCmd(CVector3f::Zero(), x908_targetDelta, 1.f));
    }
    x8f6_31_inAttackState = true;
    xb90_maxCloakAlpha = 0.75f;
    break;
  }
  case kStateMsg_Update:
    if (x8f6_26_enableMeleeAttack) {
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_MeleeAttack)) {
        BodyController()->CommandMgr().DeliverCmd(CBCMeleeAttackCmd(pas::kS_One));
      }
      BodyController()->CommandMgr().SetTargetVector(x908_targetDelta);
      CheckBlade(mgr);
      if (x8f5_27_shadowPirate) {
        if (mAnimationState.GetState() == CAnimationState::kAS_Over) {
          mAlphaDelta = -0.4f;
        } else {
          mAlphaDelta = 1.f;
          xb90_maxCloakAlpha = 0.75f;
        }
      }
    }
    UpdateGuardPosition(mgr, dt);
    UpdateHeldPosition(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    x8f6_26_enableMeleeAttack = false;
    x8f6_31_inAttackState = false;
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// Retail fn_72_98D8 (0xCC): Activate drops the body into crouch locomotion (asm passes 0), turns the
// held cover point's forward axis (cover +0x28/0x38/0x48, the transform's forward column) into the
// target delta, zeroes the steering speed (rodata 0xB9C) and clears the cover direction to -1; Update
// feeds that delta to the body and then runs D800 with r3 = this. Unlike MP1's Crouch there is no
// TargetPlayer call (the asm has only SetLocomotionType and GetCoverPoint) and no Deactivate case:
// anything below 1 or above 1 falls straight to the epilogue.
void CSpacePirate::Crouch(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    if (CScriptCoverPoint* cover = GetCoverPoint(mgr, x900_coverPoint)) {
      x908_targetDelta = cover->GetTransform().GetForward();
    }
    x904_steeringSpeed = 0.f;
    xa68_coverDir = -1;
    break;
  case kStateMsg_Update:
    BodyController()->CommandMgr().SetTargetVector(x908_targetDelta);
    UpdateGuardPosition(mgr, dt);
    break;
  }
}

// Retail fn_72_A310 (0xB0): Activate delivers a bare CBodyStateCmd of type 0xB = kBSC_LeanFromCover
// (the asm builds {vtable, 0xB} on the stack and calls CBodyStateCmdMgr::DeliverCmd on body +0x4) and
// marks the in-attack bit; Update is only D800 with r3 = this; Deactivate clears that bit. MP1's
// CoverAttack differs only in calling UpdateCantSeePlayer by name.
void CSpacePirate::CoverAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_LeanFromCover));
    x8f6_31_inAttackState = true;
    break;
  case kStateMsg_Update:
    UpdateGuardPosition(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    x8f6_31_inAttackState = false;
    break;
  }
}

// Retail fn_72_BBB0 is a bare `x904 = 0` (rodata 0xB9C): it is not a message handler at all, so
// every call, Activate included, stops the steering speed that Think feeds to the body.
void CSpacePirate::Halt(CStateManager&, EStateMsg, float) { x904_steeringSpeed = 0.f; }

// @ fn_72_BBA0
// Retail is 16 bytes and checks no message at all (rodata 0xB54 = 1.0): every call, Update
// included, puts the steering speed back to full.
void CSpacePirate::Run(CStateManager&, EStateMsg, float) { x904_steeringSpeed = 1.f; }

// @ fn_72_8A90
// Activate turns the current destination into a path: the cover point it holds (if any) first,
// otherwise whatever dest the previous state set, and when there is no path at all it hunts for an
// unused jump point it can cross to (MP1's shape plus Echoes' jump-point type and apex fields).
// Update leaves path following to CPatterned, aims the body at the player and starts the jump once
// the jump point is within a body-length, then refreshes the cover range.
void CSpacePirate::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    xb14_jumpPoint = kInvalidUniqueId;
    if (CScriptCoverPoint* cover = GetCoverPoint(mgr, x900_coverPoint)) {
      mReflectedDestPos = GetTranslation();
      mInPosition = false;
      mDestObj = cover->GetUniqueId();
      mDestPos = cover->GetTranslation();
    }
    if (GetSearchPath()->Search(GetTranslation(), mDestPos) == CPathFindSearch::kR_Success) {
      mReflectedDestPos = GetTranslation();
      mDestPos = GetSearchPath()->GetPoint();
      mInPosition = false;
      BodyController()->CommandMgr().DeliverCmd(
          CBCLocomotionCmd(mDestPos - GetTranslation(), CVector3f::Zero(), 1.f));
    } else {
      // Score every free jump point by the walk there, the jump itself and the waypoint beyond
      // it; a point whose far side has no path still counts, with a 1000 penalty.
      CScriptAiJumpPoint* best = nullptr;
      float minDistSq = FLT_MAX;
      CObjectList& list = mgr.ObjectListById(kOL_AiWaypoint);
      for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
        CScriptAiJumpPoint* jumpPoint = TCastToPtr< CScriptAiJumpPoint >(list[i]);
        if (jumpPoint == nullptr || !jumpPoint->GetActive() || jumpPoint->GetType() != 0 ||
            jumpPoint->GetInUse(GetUniqueId()) ||
            jumpPoint->GetJumpTarget() != kInvalidUniqueId ||
            jumpPoint->GetCurrentAreaId() != GetCurrentAreaId()) {
          continue;
        }
        const CVector3f toJump = jumpPoint->GetTranslation() - GetTranslation();
        const float distSq = toJump.MagSquared();
        if (distSq <= 25.f ||
            CVector3f::Dot(jumpPoint->GetTransform().GetForward(), toJump) <= 0.f) {
          continue;
        }
        CScriptWaypoint* waypoint =
            TCastToPtr< CScriptWaypoint >(mgr.ObjectById(jumpPoint->GetJumpPoint()));
        if (waypoint == nullptr) {
          continue;
        }
        // Only take a jump that leads towards the destination (the two height deltas agree).
        if ((mDestPos.GetZ() - GetTranslation().GetZ()) *
                (waypoint->GetTranslation().GetZ() - jumpPoint->GetTranslation().GetZ()) <= 0.f) {
          continue;
        }
        const CVector3f toDest = mDestPos - waypoint->GetTranslation();
        float score = distSq + 4.f * toJump.GetZ() * toJump.GetZ() + toDest.MagSquared() +
                      9.f * toDest.GetZ() * toDest.GetZ();
        if (score >= minDistSq ||
            GetSearchPath()->PathExists(GetTranslation(), jumpPoint->GetTranslation()) !=
                CPathFindSearch::kR_Success) {
          continue;
        }
        bool good = false;
        if (GetSearchPath()->PathExists(waypoint->GetTranslation(), mDestPos) !=
            CPathFindSearch::kR_Success) {
          score += 1000.f;
        } else {
          good = true;
        }
        if (score < minDistSq) {
          minDistSq = score;
          best = jumpPoint;
          if (good) {
            break;
          }
        }
      }
      if (best != nullptr) {
        mDestPos = best->GetTranslation();
        if (GetSearchPath()->Search(GetTranslation(), mDestPos) == CPathFindSearch::kR_Success) {
          mReflectedDestPos = GetTranslation();
          mDestPos = GetSearchPath()->GetPoint();
          mInPosition = false;
          xb14_jumpPoint = best->GetUniqueId();
          xaf8_jumpHeight = best->GetJumpApex();
          if (CScriptWaypoint* waypoint =
                  TCastToPtr< CScriptWaypoint >(mgr.ObjectById(best->GetJumpPoint()))) {
            xafc_patrolDestPos = waypoint->GetTranslation();
            BodyController()->CommandMgr().DeliverCmd(
                CBCLocomotionCmd(mDestPos, CVector3f::Zero(), 1.f));
          }
        }
      }
    }
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_FullSpeed);
    if (mEnableAim) {
      x904_steeringSpeed = 1.f;
    }
    x8f9_27_inRange = false;
    x8fa_24_normalDodge = true;
    break;
  }
  case kStateMsg_Update: {
    CPatterned::PathFind(mgr, msg, dt);
    // Retail reads the cached player straight out of the state manager (its +0x14fc, the member
    // TargetPlayer also uses) and steers the body at it.
    if (const CPlayer* player = mgr.Player(0)) {
      BodyController()->CommandMgr().SetTargetVector(player->GetTranslation() - GetTranslation());
    }
    if (xb14_jumpPoint != kInvalidUniqueId) {
      if (CScriptAiJumpPoint* jumpPoint =
              TCastToPtr< CScriptAiJumpPoint >(mgr.ObjectById(xb14_jumpPoint))) {
        const float maxSpeed = BodyController()->GetBodyStateInfo().GetMaxSpeed();
        const float jumpDistance = maxSpeed * ((1.5f * dt + 0.1f) * GetModelData()->GetScale().GetY()) +
                                   xa70_intoJumpDist;
        if ((GetTranslation() - jumpPoint->GetTranslation()).MagSquared() <
            jumpDistance * jumpDistance) {
          mAnimationState.SetState(CAnimationState::kAS_Ready);
          if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Jump)) {
            BodyController()->CommandMgr().DeliverCmd(
                CBCJumpCmd(mDestPos, pas::kJT_Normal, pas::kJS_IntoJump, 0,
                           CBCJumpCmd::kFF_AmbushJump));
          }
          x8fa_29_ = true;
        }
      }
    }
    // @ fn_72_D40C: retail avoids the other ai actors here, between the jump point test and the
    // cover-range block.
    AvoidActors(mgr);
    if (!x8f9_27_inRange) {
      if (CScriptCoverPoint* cover = GetCoverPoint(mgr, x900_coverPoint)) {
        const float maxSpeed = BodyController()->GetBodyStateInfo().GetMaxSpeed();
        xa1c_coverRange =
            maxSpeed * ((1.5f * dt + 0.1f) * GetModelData()->GetScale().GetY());
        if (cover->ShouldWallHang()) {
          xa1c_coverRange += xa70_intoJumpDist;
        }
        x8f9_27_inRange = (GetTranslation() - cover->GetTranslation()).MagSquared() <
                          xa1c_coverRange * xa1c_coverRange;
      }
    }
    UpdateGuardPosition(mgr, dt);
    UpdateHeldPosition(mgr, dt);
    break;
  }
  case kStateMsg_Deactivate:
    CPatterned::PathFind(mgr, msg, dt);
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    xb14_jumpPoint = kInvalidUniqueId;
    x8f9_27_inRange = false;
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_Normal);
    break;
  }
}

// @ fn_72_BFCC
// Retail is the patrol state plus the guard duties of a pirate that is on patrol duty. Ahead of
// anything else: Activate drops to relaxed locomotion and scales the steering speed by walk/run
// (rodata-free, the body state info's own speeds); Update refreshes the guard position; Deactivate
// returns to combat locomotion and, once ever (0x8f9 bit 0x01), fires kSS_Attack. Then, only when
// x8f7 bit 0x80 (enablePatrol) is set, every message also goes to CPatterned::Patrol: Activate
// blends at full speed and divides the turn speed by 1.25 (rodata C10), Update avoids the other
// actors and records the waypoint navigation's destination (0x6b8 + 0x1c = 0x6d4) in
// xafc_patrolDestPos, Deactivate restores the normal blend and multiplies the turn speed back.
// Traced 2026-10-20 on 08_sand_casualpirates: Activate/Update/Deactivate of the outer half run for
// the load state; no pirate there has a kSS_Patrol/kSM_Follow connection, so the enablePatrol half
// is translated from the asm only (see SpacePirate.md section 6).
void CSpacePirate::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  CBodyController* body = BodyController();
  switch (msg) {
  case kStateMsg_Activate:
    body->SetLocomotionType(pas::kLT_Relaxed);
    x904_steeringSpeed =
        body->GetBodyStateInfo().GetLocomotionSpeed(pas::kLA_Walk) /
        body->GetBodyStateInfo().GetLocomotionSpeed(pas::kLA_Run);
    break;
  case kStateMsg_Update:
    UpdateGuardPosition(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    body->SetLocomotionType(pas::kLT_Combat);
    if (!x8f9_31_sentAttackMsg) {
      x8f9_31_sentAttackMsg = true;
      SendScriptMsgs(kSS_Attack, mgr, kInvalidUniqueId, kSM_None);
    }
    break;
  }
  if (x8f7_24_enablePatrol) {
    CPatterned::Patrol(mgr, msg, dt);
    switch (msg) {
    case kStateMsg_Activate:
      body->CommandMgr().SetSteeringBlendMode(kSBM_FullSpeed);
      body->SetTurnSpeed(body->GetTurnSpeed() / 1.25f);
      break;
    case kStateMsg_Update:
      AvoidActors(mgr);
      xafc_patrolDestPos = mWaypointNavigation.GetDestinationPosition();
      break;
    case kStateMsg_Deactivate:
      body->CommandMgr().SetSteeringBlendMode(kSBM_Normal);
      body->SetTurnSpeed(1.25f * body->GetTurnSpeed());
      break;
    }
  }
}

// Retail fn_72_BCB0. Every message forwards to CPatterned::Patrol first (asm 0xBCE4). Activate
// takes the kSS_Attack/kSM_Follow connected waypoint as the nav destination and, when the dot of the
// forward axis with (waypoint - translation) is exactly zero (asm 0xBD90: fcmpo/cror/bne), marks the
// nav in-position; Update
// marks it in-position and sets the jump height when the AI waypoint's flags bit1/bit2 ask for a
// jump/drop and the pirate is within a body-length, aims the cmd-manager target at the player while
// the body state is kAS_Jump and the waypoint has no kSS_Arrived/kSM_Next connection, then caches
// the nav destination position in xafc_patrolDestPos; Deactivate restores the normal blend. Retail
// writes the nav destination (this+0x6c0 = nav+0x08), unlike MP1 which wrote mDestObj.
void CSpacePirate::TargetPatrol(CStateManager& mgr, EStateMsg msg, float dt) {
  CPatterned::Patrol(mgr, msg, dt);
  switch (msg) {
  case kStateMsg_Activate: {
    x904_steeringSpeed = 1.f;
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_FullSpeed);
    const TUniqueId id = GetConnectedObject(mgr, kSS_Attack, kSM_Follow);
    if (CScriptWaypoint* waypoint = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(id))) {
      mWaypointNavigation.SetDestination(id);
      if (CVector3f::Dot(GetTransform().GetForward(),
                         waypoint->GetTranslation() - GetTranslation()) == 0.f) {
        mWaypointNavigation.SetInPosition(true);
      }
    }
    break;
  }
  case kStateMsg_Update: {
    CScriptAIWaypoint* waypoint = TCastToPtr< CScriptAIWaypoint >(
        mgr.ObjectById(mWaypointNavigation.GetDestination()));
    if (waypoint != nullptr) {
      const uint flags = waypoint->GetFlags();
      const bool jump = (flags >> 1) & 1;
      const bool drop = (flags >> 2) & 1;
      if (jump || drop) {
        const float maxSpeed = BodyController()->GetBodyStateInfo().GetMaxSpeed();
        const float jumpDistance = maxSpeed * ((1.5f * dt + 0.1f) * GetModelData()->GetScale().GetY()) +
                                   xa70_intoJumpDist;
        if ((GetTranslation() - waypoint->GetTranslation()).MagSquared() <
            jumpDistance * jumpDistance) {
          mWaypointNavigation.SetInPosition(true);
          xaf8_jumpHeight = jump ? 3.f : 0.f;
        }
      }
    }
    if (BodyController()->GetCurrentStateId() == pas::kAS_Jump) {
      bool setTarget = true;
      if (waypoint != nullptr) {
        if (waypoint->CheckConnectedObject(mgr, kSS_Arrived, kSM_Next) != kInvalidUniqueId) {
          setTarget = false;
        }
      }
      if (setTarget) {
        BodyController()->CommandMgr().SetTargetVector(GetTargetPos(mgr) - GetTranslation());
      }
    }
    xafc_patrolDestPos = mWaypointNavigation.GetDestinationPosition();
    break;
  }
  case kStateMsg_Deactivate:
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_Normal);
    break;
  }
}

namespace {
// Retail fn_72_D608 is MP1's CPatterned::Random2f, which this tree does not have. Two random
// components in the XY plane - the *first* CRandom16 draw is y and the second is x, both minus
// rodata 0xB44 = 0.5, while z is rodata 0xB9C = 0 (asm 0xD644-0xD688: the first draw lands at
// sp+0xc and the second at sp+0x8, which is the vector base) - then the x term is forced to
// rodata 0xBC8 = 0.001 when |x| is under 0.001 (0xD68C-0xD69C) so the normalize cannot collapse
// the direction, the vector is normalized in place (0xD6A0), and a third draw lerps the length
// minLen + (maxLen - minLen) * rand (0xD6C0 fmadds) which then scales the unit vector
// (0xD6C4-0xD6E8). Retail leaves that scaled length in the return register unused.
CVector3f ShuffleRandomVector(CStateManager& mgr, float minLen, float maxLen) {
  const float y = mgr.Random()->Float() - 0.5f;
  float x = mgr.Random()->Float() - 0.5f;
  if (x > -0.001f && x < 0.001f) {
    x = 0.001f;
  }
  CVector3f vec(x, y, 0.f);
  vec.Normalize();
  return (minLen + (maxLen - minLen) * mgr.Random()->Float()) * vec;
}
} // namespace

// Retail fn_72_8670 (asm 0x8670-0x8B14) is MP1's CSpacePirate::Shuffle with Echoes' own pieces:
// Activate runs only for kStateMsg_Activate (0x86B8-0x86CC jumps straight past it otherwise) and
// first clears the body's steering blend to normal (0x4c = 0). The close check is rodata 0xB9C = 0
// handed to CPatterned::TooClose as a CTriggerData, gated on x8f4 bit 0x08 being clear
// (`extrwi. r0, r0, 1, 28`); when it fires the destination is the target position (CB30) plus
// mMinAttackRange (0x3dc) along the normalized away-from-target vector plus fn_72_D608(mgr, 0, 5)
// - rodata 0xB9C/0xB78 at 0x8730-0x8748 - and x8f6 bit 0x02 is set (0x880C `rlwimi ..,1,30,30`).
// Otherwise the destination is the target position plus maxRange*(rand+1) along that vector and
// 2*maxRange*(rand-0.5) along Up x (own translation - target) (fn_72_8A44 = CVector3f::Cross,
// rodata 0xB60 = 2, 0x8850-0x8948), and the bit is cleared. Both branches set mDestObj
// (0x330 = kInvalidUniqueId) and the steering speed to rodata 0xB54 = 1 (0x8968). Echoes writes no
// behaviour orientation here, unlike MP1. Then CPatterned::PathFind runs for every message
// (0x8974) and the body controller's command target (cmdMgr +0x34/0x38/0x3c = SetTargetVector) is
// set to the state manager's player (+0x14fc, +0x54) minus this translation, also for every
// message; only then does the second switch avoid the other actors on Update (fn_72_D40C) and
// clear x8f6 bit 0x02 on Deactivate.
void CSpacePirate::Shuffle(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_Normal);
    const CVector3f targetPos = GetTargetPos(mgr);
    if (!x8f4_28_noShuffleCloseCheck && TooClose(mgr, CTriggerData(0.f))) {
      SetDestPos(GetTranslation() +
                 mMinAttackRange * (GetTranslation() - targetPos).AsNormalized() +
                 ShuffleRandomVector(mgr, 0.f, 5.f));
      mDestObj = kInvalidUniqueId;
      x8f6_30_shuffleClose = true;
    } else {
      const CVector3f fromTarget = GetTranslation() - targetPos;
      const CVector3f side = CVector3f::Cross(CVector3f::Up(), fromTarget);
      const float range = mMaxAttackRange;
      const float distance = range * mgr.Random()->Float() + range;
      const float sideDistance = 2.f * mMaxAttackRange * (mgr.Random()->Float() - 0.5f);
      SetDestPos(targetPos + distance * fromTarget.AsNormalized() +
                 sideDistance * side.AsNormalized());
      mDestObj = kInvalidUniqueId;
      x8f6_30_shuffleClose = false;
    }
    x904_steeringSpeed = 1.f;
  }
  CPatterned::PathFind(mgr, msg, dt);
  // Retail dereferences the state manager's player without a check; the port keeps the guard the
  // other states use and simply leaves the previous target vector when there is no player.
  if (const CPlayer* player = mgr.Player(0)) {
    BodyController()->CommandMgr().SetTargetVector(player->GetTranslation() - GetTranslation());
  }
  switch (msg) {
  case kStateMsg_Update:
    AvoidActors(mgr);
    break;
  case kStateMsg_Deactivate:
    x8f6_30_shuffleClose = false;
    break;
  }
}

// Retail fn_72_8464 rotates the pirate in place towards the target position it remembers.
// Activate caches GetTargetPos in xbc4_ (asm 0x8504-0x8520: the CB30 result is stored to
// 0xbc4/0xbc8/0xbcc, then the delta to the actor's own 0x54 translation is built with rodata
// 0xB9C = 0 stored over its Z, so the test is horizontal). The forward axis it dots that with is
// read at 0x28/0x38/0x48 off the actor - transform column 1, i.e. GetTransform().GetForward() -
// and the cut-off is rodata 0xB64 = 0.8 (asm 0x855C fcmpo): a target less than 0.8 ahead asks for
// a fresh turn. Update (0x8588) waits for CanIssueCommand(kAS_Turn) and then, only while the full
// 3-D delta to the cached spot is magnitude-safe (this one is not flattened), delivers
// CBCLocomotionCmd(zero move, normalized delta as the face vector, rodata 0xB54 = 1.0 weight; the
// zero vector is stored first at 0x50-0x58, the normalized delta at 0x5c-0x64) and otherwise
// gives up on the turn, kAS_Over. The guard position runs for every message; Deactivate returns
// the animation to kAS_NotReady.
void CSpacePirate::TurnAround(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    xbc4_ = GetTargetPos(mgr);
    CVector3f delta = xbc4_ - GetTranslation();
    delta.SetZ(0.f);
    if (CVector3f::Dot(GetTransform().GetForward(), delta.AsNormalized()) < 0.8f) {
      mAnimationState.SetState(CAnimationState::kAS_Ready);
    }
    break;
  }
  case kStateMsg_Update: {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Turn)) {
      const CVector3f delta = xbc4_ - GetTranslation();
      if (delta.IsMagnitudeSafe()) {
        BodyController()->CommandMgr().DeliverCmd(
            CBCLocomotionCmd(CVector3f::Zero(), delta.AsNormalized(), 1.f));
      } else {
        mAnimationState.SetState(CAnimationState::kAS_Over);
      }
    }
    UpdateGuardPosition(mgr, dt);
    break;
  }
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// Retail fn_72_7DFC is MP1's CSpacePirate::Dodge with TryCommand(TryDodge/TryRollingDodge/
// TryBreakDodge) replaced by a direct CBCStepCmd; the asm's three deliveries are the step types
// 1/3/2 at 0x7FC4/0x80A8/0x8158 and all sit behind CAnimationState::CanIssueCommand(kAS_Step) at
// fn_801B73E4. Activate clears the break-dodge bit (0x7E50 rlwimi over 0x8f9 bit 0x04) and then,
// only while neither normalDodge (0x8fa bit 0x80) nor noBreakDodge (0x8f5 bit 0x04) is set and the
// delay timer has run out (0x7E7C fcmpo against rodata 0xB9C = 0), rolls MP1's HP-scaled chance -
// rodata 0xBFC = 0.15 times (0xB54 = 1.0 + 0xB50 = 4.0 * lostHP / xa18_) - and re-arms xb94 from
// Range(0x870, 0x874), the CSpacePirateData break-dodge min/max. It then stores GetStrafeDir at
// xb18 over xb20 when the bit is set and xb1c otherwise, and asks for kAS_Ready only when that
// direction is valid. Update draws the 50/50 (rodata 0xB44) before the CanIssueCommand test, so the
// draw happens even when the body cannot step. The break-dodge half then drops the orbit material
// and asks the player to break its orbit once the state is older than 0.5 s.
void CSpacePirate::Dodge(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    x8f9_29_enableBreakDodge = false;
    if (!x8fa_24_normalDodge && !x8f5_29_noBreakDodge && xb94_dodgeDelayTimer <= 0.f) {
      const float chance = 0.15f * (1.f + (4.f * (xa18_ - HealthInfo()->GetHP())) / xa18_);
      if (mgr.Random()->Float() < chance) {
        x8f9_29_enableBreakDodge = true;
      }
      xb94_dodgeDelayTimer =
          mgr.Random()->Range(mData.mBreakDodgeMinTime, mData.mBreakDodgeMaxTime);
    }
    xb18_dodgeDir =
        GetStrafeDir(mgr, x8f9_29_enableBreakDodge ? xb20_breakDodgeDist : xb1c_dodgeDist);
    if (xb18_dodgeDir != pas::kSD_Invalid) {
      mAnimationState.SetState(CAnimationState::kAS_Ready);
    }
    break;
  case kStateMsg_Update:
    if (!x8f9_29_enableBreakDodge) {
      const bool normalStep = x8fa_24_normalDodge || mgr.Random()->Float() < 0.5f;
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) {
        BodyController()->CommandMgr().DeliverCmd(
            CBCStepCmd(static_cast< pas::EStepDirection >(xb18_dodgeDir),
                       normalStep ? pas::kStep_Dodge : pas::kStep_RollDodge));
      }
    } else {
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) {
        BodyController()->CommandMgr().DeliverCmd(
            CBCStepCmd(static_cast< pas::EStepDirection >(xb18_dodgeDir), pas::kStep_BreakDodge));
      }
      if (GetMaterialList().HasMaterial(kMT_Orbit) && mStateMachine->GetTime() > 0.5f) {
        RemoveMaterial(kMT_Orbit, mgr);
        // Retail dereferences the state manager's player without a check, as the shuffle path does.
        if (CPlayer* player = mgr.Player(0)) {
          player->SetOrbitRequestForTarget(GetUniqueId(), CPlayer::kOR_ActivateOrbitSource, mgr);
        }
      }
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    x8f8_29_noPlayerDodge = true;
    if (!GetMaterialList().HasMaterial(kMT_Orbit)) {
      AddMaterial(kMT_Orbit, mgr);
    }
    break;
  }
}

// Retail fn_72_5280: the leash timer accumulates while the body is not fully frozen (rodata B54 = 1.0,
// `fcmpu` + `beq` on GetPercentageFrozen) and not being electrocuted (body +0x5b8 > 0, i.e.
// IsElectrocuting). MP1 spells the same test !IsFrozen() && !IsElectrocuting().
void CSpacePirate::UpdateLeashTimer(float dt) {
  if (BodyController()->GetPercentageFrozen() != 1.f && !BodyController()->IsElectrocuting()) {
    xbb0_leashTimer += dt;
  }
}

// Retail fn_72_D720: how long the target has stayed put; ShouldJumpBack reads the timer.
void CSpacePirate::UpdateHeldPosition(CStateManager& mgr, float dt) {
  const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mAttackTarget));
  if (player == nullptr) {
    xbac_holdPositionTime = 0.f;
    return;
  }
  const CVector2f pos = player->GetTranslation().ToVec2f();
  if ((pos - CVector2f(xba4_heldPositionX, xba8_heldPositionY)).MagSquared() < 3.f) {
    xbac_holdPositionTime += dt;
  } else {
    xba4_heldPositionX = pos.GetX();
    xba8_heldPositionY = pos.GetY();
    xbac_holdPositionTime = 0.f;
  }
}

// @ fn_72_D800
// Ten times a second retail re-derives the point the pirate guards from and tests
// the line to the player's aim position. Behind cover (a cover point is held) the eye moves two
// units along the body's right axis, the sign taken from xa68_coverDir; in the open it is nudged
// 1.1 units across the look direction, a peek. A blocked ray restarts xa78_, so x8f7_28_noPlayerLos
// stays set for the 0.5 s (CSpacePirateData +0xc4) after the pirate loses sight of the player.
void CSpacePirate::UpdateGuardPosition(CStateManager& mgr, float dt) {
  xa80_cantSeePlayerCycleCounter += dt;
  xa78_ += dt;
  if (xa80_cantSeePlayerCycleCounter > 0.1f) {
    xa80_cantSeePlayerCycleCounter = 0.f;
    CVector3f guardPos = GetTranslation();
    guardPos.SetZ(guardPos.GetZ() + xa74_eyeHeight);
    // Retail: the attack target when it is a player, otherwise (single player) the state manager's
    // cached player at +0x14fc.
    const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mAttackTarget));
    if (player == nullptr && !mgr.IsMultiplayer()) {
      player = mgr.GetPlayer(0);
    }
    if (player != nullptr) {
      const CVector3f aimPos = player->GetAimPosition(mgr, 0.f);
      if (GetCoverPoint(mgr, x900_coverPoint) != nullptr) {
        if (xa68_coverDir == 0) {
          guardPos = guardPos - 2.f * GetTransform().GetRight();
        } else if (xa68_coverDir == 1) {
          guardPos = guardPos + 2.f * GetTransform().GetRight();
        }
      } else {
        const CVector3f side =
            CVector3f::Cross((aimPos - guardPos).AsNormalized(), CVector3f::Up());
        guardPos = guardPos + 1.1f * side;
      }
      if (!LineOfSightTest(mgr, guardPos, aimPos,
                           CMaterialList(kMT_Player, kMT_ProjectilePassthrough))) {
        xa78_ = 0.f;
      }
    }
  }
  x8f7_28_noPlayerLos = xa78_ < 0.5f;
}

// @ fn_72_D40C
// This is MP1's CSpacePirate::AvoidActors, confirmed offset by offset on the retail body: it walks
// the listening-AI object list (state manager +0x830), and for every other CPatterned in the same
// area (entities' +0x4 area id) asks CSteeringBehaviors::Separation (the pattern's steering object,
// this+0x61c, at CSpacePirateData +0xc0 = avoidDistance). A usable separation is normalized and
// delivered as a locomotion command with a zero look direction and speed 0.5 (rodata 0xB44); then
// with both pirates' steering delay timers at zero (xa10, rodata 0xB9C) and this one facing the
// other, which must also move along its own forward (velocity WR dot forward > 0), the timer is set
// to 1 (rodata 0xB54). No message switch: it runs from PathFind's Update only.
void CSpacePirate::AvoidActors(CStateManager& mgr) {
  CObjectList& list = mgr.ObjectListById(kOL_ListeningAi);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    CPatterned* ai = TCastToPtr< CPatterned >(list[i]);
    if (ai == nullptr || ai == this || ai->GetCurrentAreaId() != GetCurrentAreaId()) {
      continue;
    }
    const CVector3f separation =
        mSteeringBehaviors.Separation(*this, ai->GetTranslation(), mData.mAvoidDistance);
    if (!separation.IsMagnitudeSafe()) {
      continue;
    }
    BodyController()->CommandMgr().DeliverCmd(
        CBCLocomotionCmd(separation.AsNormalized(), CVector3f::Zero(), 0.5f));
    if (xa10_steeringDelayTimer == 0.f) {
      CSpacePirate* pirate = TCastToPtr< CSpacePirate >(ai);
      if (pirate != nullptr && pirate->xa10_steeringDelayTimer == 0.f) {
        const CVector3f delta = pirate->GetTranslation() - GetTranslation();
        if (CVector3f::Dot(GetTransform().GetForward(), delta) > 0.f &&
            CVector3f::Dot(pirate->GetVelocityWR(), pirate->GetTransform().GetForward()) > 0.f) {
          xa10_steeringDelayTimer = 1.f;
        }
      }
    }
  }
}

// Retail fn_72_CC80 is MP1's CSpacePirate::CheckBlade: the swoosh locator's own box (the model's
// scale halved around the locator translation, rodata 0xB44 = 0.5) tested against the attack
// target's bounds, which once over the overlap damages that actor with the data's blade damage and
// marks the blade spent. Echoes differs from MP1 in the guard: it bails on x8f8 bit 0x20
// (alwaysAggressive, `rlwinm. r0,r0,26,31,31` at 0xCCA4) rather than on the appliedBladeDamage bit,
// and only *sets* x8f8 bit 0x40 at the end (0xCE0C `rlwimi ..,1,6,25`).
void CSpacePirate::CheckBlade(CStateManager& mgr) {
  if (x8f8_26_alwaysAggressive) {
    return;
  }
  if (xa89_swooshSeg == CSegId::Invalid()) {
    return;
  }
  CPhysicsActor* actor = TCastToPtr< CPhysicsActor >(mgr.ObjectById(mAttackTarget));
  if (actor == nullptr) {
    return;
  }
  const CTransform4f swoosh = GetLctrTransform(xa89_swooshSeg);
  const CVector3f scale = GetModelData()->GetScale();
  const CVector3f half(scale.GetX() / 2.f, scale.GetY() / 2.f, scale.GetZ() / 2.f);
  const CAABox box(swoosh.GetTranslation() - half, swoosh.GetTranslation() + half);
  if (!box.DoBoundsOverlap(actor->GetBoundingBox())) {
    return;
  }
  mgr.ApplyDamage(GetUniqueId(), actor->GetUniqueId(), GetUniqueId(), mData.mBladeDamage,
                  CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList()),
                  CVector3f::Zero());
  x8f8_25_appliedBladeDamage = true;
}

// @ fn_72_AF70
void CSpacePirate::Lurk(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    ReleaseCoverPoint(mgr, x900_coverPoint, true);
    x904_steeringSpeed = 0.f;
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    x8f7_28_noPlayerLos = true;
    xa7c_timeNoPlayerLos = 0.f;
    // Three 0-100 rolls against the data's own percentages: this lurk is aggressive, checks for
    // cover and dodges only as often as its properties say.
    x8f8_26_alwaysAggressive = mgr.Random()->Range(0.f, 100.f) < mData.mAggressiveness;
    x8f8_27_coverCheck = mgr.Random()->Range(0.f, 100.f) < mData.mCoverCheck;
    x8f8_28_enableDodge = mgr.Random()->Range(0.f, 100.f) < mData.mDodgeCheck;
    mEnableAim = true;
    BodyController()->AbortScriptedAnimations();
    if (mAttackTarget == kInvalidUniqueId) {
      mAttackTarget = ChooseTeamTarget(mgr);
      UpdateTeamTarget(mgr);
      mBoneTracking.SetActive(true);
      mBoneTracking.SetTarget(mAttackTarget);
    }
    if (x8f4_29_onlyAttackInRange) {
      mBurstFire.SetBurstType(4);
      BodyController()->SetLocomotionType(pas::kLT_Combat);
    }
    x8fa_24_normalDodge = false;
    break;
  case kStateMsg_Update: {
    CBodyController* body = BodyController();
    if (body->HasBodyState(pas::kAS_Turn)) {
      // Face the target, and repeat the turn command each time the turn animation comes round.
      if (mAnimationState.GetState() != CAnimationState::kAS_NotReady &&
          mAnimationState.CanIssueCommand(*body, pas::kAS_Turn)) {
        const CVector3f toTarget = xbc4_ - GetTranslation();
        if (toTarget.IsMagnitudeSafe()) {
          body->CommandMgr().DeliverCmd(
              CBCLocomotionCmd(CVector3f::Zero(), toTarget.AsNormalized(), 1.f));
        }
      }
      if (mAnimationState.GetState() != CAnimationState::kAS_Repeat) {
        xbc4_ = GetTargetPos(mgr);
        CVector3f delta = xbc4_ - GetTranslation();
        delta.SetZ(0.f);
        // More than ~25 degrees off the forward axis: ask for another turn.
        if (CVector3f::Dot(GetTransform().GetForward(), delta.AsNormalized()) < 0.9f) {
          mAnimationState.SetState(CAnimationState::kAS_Ready);
        }
      }
    }
    // A sat-up pirate stands to attack, and sits down again once the shot is nearly due.
    if (x8f5_26_seated && x8f9_28_satUp) {
      if (mAttackRemTime > GetAverageAttackTime() &&
          body->GetLocomotionType() == pas::kLT_Combat) {
        body->SetLocomotionType(pas::kLT_Internal5);
      } else if (mAttackRemTime < 0.5f * GetAverageAttackTime() &&
                 body->GetLocomotionType() == pas::kLT_Internal5) {
        body->SetLocomotionType(pas::kLT_Combat);
      }
    }
    UpdateGuardPosition(mgr, dt);
    UpdateHeldPosition(mgr, dt);
    break;
  }
  case kStateMsg_Deactivate:
    x8f8_26_alwaysAggressive = false;
    x8f8_29_noPlayerDodge = false;
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// Retail fn_72_DFD4: find the squad this pirate belongs to (the field starts invalid) and take a
// role in it - melee or projectile, the same role SquadReset later gives up.
void CSpacePirate::SquadJoin(CStateManager& mgr) {
  if (xba0_teamAiMgrId == kInvalidUniqueId) {
    xba0_teamAiMgrId = CScriptTeamAiMgr::GetAssociatedTeamId(*this, mgr);
  }
  if (xba0_teamAiMgrId == kInvalidUniqueId) {
    return;
  }
  CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(xba0_teamAiMgrId));
  if (team != nullptr) {
    team->JoinTeam(*this,
                   x8f4_27_melee ? CTeamAiRole::kTAR_Melee : CTeamAiRole::kTAR_Projectile,
                   CTeamAiRole::kTAR_Unknown, CTeamAiRole::kTAR_Invalid);
  }
}

// @ fn_72_B59C
void CSpacePirate::Taunt(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    mEnableAim = true;
    BodyController()->AbortScriptedAnimations();
    SquadJoin(mgr);
    if (mAttackTarget == kInvalidUniqueId) {
      mAttackTarget = ChooseTeamTarget(mgr);
      UpdateTeamTarget(mgr);
    }
    mBoneTracking.SetActive(true);
    mBoneTracking.SetTarget(mAttackTarget);
    if (!BodyController()->HasBodyState(pas::kAS_Taunt)) {
      // No taunt animation: just the alert sound.
      CSfxManager::AddEmitter(mData.mAlertSfx, GetTranslation(), GetCurrentAreaId().Value(), true,
                              false, CSfxManager::kMedPriority);
      break;
    }
    if (!x8f5_27_shadowPirate) {
      bool findOtherPirate = true;
      // A melee pirate has a two-pirate taunt animation: when it exists, taunt the pair.
      if (x8f4_27_melee) {
        const CPASAnimParmData parms(pas::kAS_Taunt, CPASAnimParm::FromEnum(2));
        const rstl::pair< float, int > anim =
            BodyController()->GetPASDatabase().FindBestAnimation(parms, *mgr.Random(), -1);
        if (anim.first > 0.f) {
          findOtherPirate = false;
          xa28_taunt = pas::kTT_Two;
        }
      }
      if (findOtherPirate) {
        // Taunt a squad mate within hearing range, or taunt on its own when there is none.
        bool withOtherPirate = false;
        const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
        for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
          const CSpacePirate* pirate = TCastToConstPtr< CSpacePirate >(list[i]);
          if (pirate == nullptr || pirate == this || pirate->mEnableAim ||
              !pirate->GetAlive() || pirate->GetCurrentAreaId() != GetCurrentAreaId()) {
            continue;
          }
          if ((pirate->GetTranslation() - GetTranslation()).MagSquared() <
              mData.mHearingRadius * mData.mHearingRadius) {
            withOtherPirate = true;
          }
        }
        xa28_taunt = withOtherPirate ? pas::kTT_Zero : pas::kTT_One;
      }
    } else {
      xa28_taunt = x8f5_28_alertBeforeCloak ? pas::kTT_One : pas::kTT_Zero;
    }
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    break;
  }
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCTauntCmd(static_cast< pas::ETauntType >(xa28_taunt)));
    }
    break;
  case kStateMsg_Deactivate:
    // Retail calls the player out after a paired taunt (the noise is what Listen hears).
    if (xa28_taunt == pas::kTT_Zero) {
      mgr.InformListeners(GetTranslation(), kLNT_PlayerFire);
    }
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// Retail fn_72_A3C0 is MP1's CSpacePirate::Cover with Echoes' own members. Activate bails while the
// body is already in pas::kAS_Cover (body +0x37c), otherwise takes the cover point x900_coverPoint,
// stores the point's attack direction bit 1 (fn_800EC6D8, extrwi r0,r3,1,30 = MP1's `(dir >> 1) & 1`)
// in xa68_coverDir, sets the animation ready, moves the dest pos to the point's translation (+0x54)
// and - when fn_801B73E4 (CAnimationState::CanIssueCommand(&x6b4, body, 0x13)) allows it - delivers the
// CBCCoverCmd (type 0x17) to the body's CommandMgr with the cover direction, target and the negated
// cover forward (+0x28/0x38/0x48). That guarded delivery is Echoes inlining MP1's
// TryCommand(mgr, pas::kAS_Cover, &CPatterned::TryCover, x79c_coverDir); Update repeats it, then
// unconditionally aims the body at the negated cover forward (body +0x34/0x38/0x3c) and always runs
// D800. Deactivate stores kAS_NotReady into x6b4.
void CSpacePirate::Cover(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    if (BodyController()->GetCurrentStateId() != pas::kAS_Cover) {
      if (CScriptCoverPoint* cover = GetCoverPoint(mgr, x900_coverPoint)) {
        xa68_coverDir = (static_cast< uint >(cover->GetAttackDirection()) >> 1) & 1;
        mAnimationState.SetState(CAnimationState::kAS_Ready);
        SetDestPos(cover->GetTranslation());
        if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Cover)) {
          BodyController()->CommandMgr().DeliverCmd(
              CBCCoverCmd(static_cast< pas::ECoverDirection >(xa68_coverDir),
                          cover->GetTranslation(), -cover->GetTransform().GetForward()));
        }
      }
    }
    break;
  case kStateMsg_Update:
    if (CScriptCoverPoint* cover = GetCoverPoint(mgr, x900_coverPoint)) {
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Cover)) {
        BodyController()->CommandMgr().DeliverCmd(
            CBCCoverCmd(static_cast< pas::ECoverDirection >(xa68_coverDir),
                        cover->GetTranslation(), -cover->GetTransform().GetForward()));
      }
      BodyController()->CommandMgr().SetTargetVector(-cover->GetTransform().GetForward());
    }
    UpdateGuardPosition(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// @ fn_72_5C10
void CSpacePirate::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  CPatterned::Dead(mgr, msg, dt);
  switch (msg) {
  case kStateMsg_Activate:
    mBoneTracking.SetActive(false);
    // Retail calls fn_72_E314(this, mgr, 0) here, which is an empty function (0xe314..0xe318 is a
    // bare blr), so there is nothing to port.
    SquadReset(mgr); // fn_72_DEDC
    break;
  case kStateMsg_Update:
    // The corpse loses its target/orbit and ground collision and stops dead once the body has
    // finished the death animation (pas::kAS_Death is 4 in retail, read straight off the controller).
    if (BodyController()->GetCurrentStateId() == pas::kAS_Death) {
      RemoveMaterial(kMT_Target, kMT_Orbit, mgr);
      RemoveMaterial(kMT_GroundCollider, kMT_Solid, kMT_AIBlock, mgr);
      AddMaterial(kMT_ProjectilePassthrough, mgr);
      SetMomentumWR(CVector3f::Zero());
      CPhysicsActor::Stop();
    }
    break;
  }
}

// Retail fn_72_78DC (0x94): Activate only, like TargetPlayer above (asm compares the message with 0
// and bails on anything else). It aims the dest at the held cover point: the point's unique id and
// its translation (cover +0x54), then unconditionally re-seeds the reflected dest from its own
// translation and clears the in-position bit.
void CSpacePirate::TargetCover(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {
    return;
  }
  if (CScriptCoverPoint* cover = GetCoverPoint(mgr, x900_coverPoint)) {
    mDestObj = x900_coverPoint;
    SetDestPos(cover->GetTranslation());
  }
  mReflectedDestPos = GetTranslation();
  mInPosition = false; // asm: rlwimi clears 0x34C bit 0x80, the bit declared first
}

// @ fn_72_7868
// Activate only (asm compares the message with 0 and bails on anything else). Retail takes the
// first player out of the state manager: its unique id (player + 0x8) and its translation
// (player + 0x54), then marks itself out of position so the path finder re-seeds the segment.
void CSpacePirate::TargetPlayer(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Activate) {
    return;
  }
  const CPlayer* player = mgr.Player(0);
  if (player == nullptr) {
    return;
  }
  mDestObj = player->GetUniqueId();
  SetDestPos(player->GetTranslation());
  mReflectedDestPos = GetTranslation();
  mInPosition = false; // asm: rlwimi clears 0x34C bit 0x80, the bit declared first
}

// Retail fn_72_9478: Activate sets the normal steering blend and then a steering speed of one;
// Update only avoids the other actors. There is no Deactivate case: the asm's unused branches
// fall through to the shared epilogue.
void CSpacePirate::Approach(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_Normal);
    x904_steeringSpeed = 1.f;
    break;
  case kStateMsg_Update:
    AvoidActors(mgr);
    break;
  }
}

// Retail fn_72_69AC (0x33C): Activate stores kAS_Ready, sets 0x8f7 bit 0x04 and, when the pirate has
// a cover point, walks that point's kSS_Arrived/kSM_Next connection with an explicit
// CValidEntityPredicate and stores the waypoint's own unique id (waypoint +0x8), not the
// GetIdForScript result MP1 stores. The cover point's forward axis becomes the target delta. It then
// sets the combat bit, switches bone tracking off and sets 0x8fa bit 0x02. Update skips the hung
// command while the body still has to turn (more than 15 degrees, rodata 0xBF8) towards that axis;
// the turn is the usual zero move vector with the axis as the face vector and speed 1 (rodata 0xB54).
// Only once the body state is kAS_WallHang does it read the state's vtable slot 0x28 and store the
// negation of its byte into 0x8fa bit 0x02, and it always ends by setting the burst type to 1.
void CSpacePirate::WallHang(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    x8f7_29_inWallHang = true;
    if (CScriptCoverPoint* cover = GetCoverPoint(mgr, x900_coverPoint)) {
      const TUniqueId id = cover->CheckConnectedObject(mgr, kSS_Arrived, kSM_Next);
      if (CScriptWaypoint* waypoint = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(id))) {
        mDestObj = waypoint->GetUniqueId();
        SetDestPos(waypoint->GetTranslation());
        mReflectedDestPos = GetTranslation();
        mInPosition = false;
      }
      x908_targetDelta = cover->GetTransform().GetForward();
    }
    x8f6_31_inAttackState = true;
    mBoneTracking.SetActive(false);
    x8fa_30_ = true;
    break;
  case kStateMsg_Update: {
    bool ready = true;
    if (mAnimationState.GetState() == CAnimationState::kAS_Ready) {
      if (CVector3f::GetAngleDiff(GetTransform().GetForward(), x908_targetDelta) > 0.26179939f) {
        BodyController()->CommandMgr().DeliverCmd(
            CBCLocomotionCmd(CVector3f::Zero(), x908_targetDelta, 1.f));
        ready = false;
      }
    }
    if (ready && mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_WallHang)) {
      BodyController()->CommandMgr().DeliverCmd(CBCWallHangCmd(mDestObj));
    }
    if (BodyController()->GetCurrentStateId() == pas::kAS_WallHang) {
      x8fa_30_ = !BodyController()->GetBodyStateInfo().GetCurrentState()->UnkVtable2C();
    }
    mBurstFire.SetBurstType(1);
    break;
  }
  case kStateMsg_Deactivate:
    x8f7_29_inWallHang = false;
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    x8f6_31_inAttackState = false;
    mBoneTracking.SetActive(true);
    x8fa_30_ = false;
    break;
  }
}

// Retail fn_72_68E4 (0xC8): Activate sets 0x8f7 bit 0x04 and both 0x8fa bits 0x02 and 0x01, Update
// delivers a bare kBSC_ExitState command to the body and Deactivate clears those three bits again.
// Messages outside 1..3 and negative ones fall straight to the epilogue.
void CSpacePirate::WallDetach(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    x8f7_29_inWallHang = true;
    x8fa_30_ = true;
    x8fa_31_ = true;
    break;
  case kStateMsg_Update:
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    break;
  case kStateMsg_Deactivate:
    x8f7_29_inWallHang = false;
    x8fa_30_ = false;
    x8fa_31_ = false;
    break;
  }
}

// Retail fn_72_B3D4: Activate re-arms the animation state and the squad, zeroes the leash timer and
// sets 0x8fa bit 0x40. While the body lies on the ground and the path finder has no source point at
// its own translation the pirate is left to die; otherwise it gets up as soon as the animation state
// allows a kAS_Getup command. Both paths feed the leash timer. Deactivate undoes the flags.
void CSpacePirate::GetUp(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    SquadReset(mgr); // fn_72_DEDC
    xbb0_leashTimer = 0.f;
    x8fa_25_ = true;
    break;
  case kStateMsg_Update:
    if (BodyController()->GetCurrentStateId() == pas::kAS_LieOnGround &&
        mPathFindSearch.Search(GetTranslation(), GetTranslation()) ==
            CPathFindSearch::kR_NoSourcePoint) {
      SetPendingDeath(true);
    } else if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Getup)) {
      BodyController()->CommandMgr().DeliverCmd(CBCGetupCmd(pas::kGetup_Zero));
    }
    UpdateLeashTimer(dt); // 5280
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    x8fa_25_ = false;
    break;
  }
}

void CSpacePirate::Generate(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Generate state (fn_72_C1A4)");
}

// Retail fn_72_94D0: Activate stores rodata B50 = 4.0 into the strafe delay and flags the attack
// state; Update keeps a step going in xb08_skidDir (body 0x37c compared with 3 = kAS_Step);
// Deactivate clears the flag. Only messages 0-2 are handled.
void CSpacePirate::Skid(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    xb0c_strafeDelayTimer = 4.f;
    x8f6_31_inAttackState = true;
    break;
  case kStateMsg_Update:
    if (BodyController()->GetCurrentStateId() != pas::kAS_Step) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCStepCmd(static_cast< pas::EStepDirection >(xb08_skidDir), pas::kStep_Normal));
    }
    break;
  case kStateMsg_Deactivate:
    x8f6_31_inAttackState = false;
    break;
  }
}

void CSpacePirate::DoubleSnap(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::DoubleSnap state (fn_72_99F0)");
}

void CSpacePirate::JumpBack(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::JumpBack state (fn_72_63C8)");
}

void CSpacePirate::Bounce(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Bounce state (fn_72_5D0C)");
}

void CSpacePirate::PathFindEx(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::PathFindEx state (fn_72_5EA4)");
}

void CSpacePirate::Enraged(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Enraged state (fn_72_A2B8)");
}

// Retail fn_72_5310: Activate arms the animation state and zeroes the leash timer. Update delivers
// the loop jump towards mDestPos (as built by fn_72_5510: 0x11 kBSC_Jump, jump type 0, variant 0,
// waypoint2 = zero vector, initial state 2 = kJS_Loop, facing flags 2 = kFF_AmbushJump, no wall
// jump) once the animation allows kAS_Jump, sets 0x8f7 bit 0x02, and feeds the leash timer.
void CSpacePirate::Jump(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    xbb0_leashTimer = 0.f;
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Jump)) {
      x8f7_30_jumpVelSet = true;
      BodyController()->CommandMgr().DeliverCmd(
          CBCJumpCmd(mDestPos, pas::kJT_Normal, pas::kJS_Loop, 0, CBCJumpCmd::kFF_AmbushJump));
    }
    UpdateLeashTimer(dt); // 5280
    break;
  case kStateMsg_Deactivate:
    x8f7_30_jumpVelSet = false;
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

void CSpacePirate::Deactivate(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Deactivate state (fn_72_5BF4)");
}

void CSpacePirate::LaunchGrenade(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::LaunchGrenade state (fn_72_58D8)");
}

void CSpacePirate::Captured(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Captured state (fn_72_5610)");
}

void CSpacePirate::RemoveFromWorld(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::RemoveFromWorld state (fn_72_5B88)");
}
