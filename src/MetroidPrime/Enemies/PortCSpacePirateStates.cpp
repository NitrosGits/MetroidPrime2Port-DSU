// The 37 states of the SpacePirate state machine (retail table fn_72_10E50 -> lbl_72_data_604).
// Milestone 2 is the skeleton: a state CPatterned also has forwards to it, every other one is a
// no-op behind a once-per-name StandIn that names the retail function. Later lanes port the bodies
// from docs/module-analysis/SpacePirate.md and build/G2ME01/SpacePirate/m2c/fn_72_<off>.c.
#include "MetroidPrime/Enemies/CSpacePirate.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CPASDatabase.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAiJumpPoint.hpp"
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

void CSpacePirate::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Attack state (fn_72_9F1C)");
}

void CSpacePirate::Crouch(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Crouch state (fn_72_98D8)");
}

void CSpacePirate::CoverAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::CoverAttack state (fn_72_A310)");
}

void CSpacePirate::Halt(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Halt state (fn_72_BBB0)");
}

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

void CSpacePirate::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Patrol state (fn_72_BFCC)");
  CPatterned::Patrol(mgr, msg, dt);
}

void CSpacePirate::TargetPatrol(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::TargetPatrol state (fn_72_BCB0)");
}

void CSpacePirate::Shuffle(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Shuffle state (fn_72_8670)");
}

void CSpacePirate::TurnAround(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::TurnAround state (fn_72_8464)");
}

void CSpacePirate::Dodge(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Dodge state (fn_72_7DFC)");
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

void CSpacePirate::Cover(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Cover state (fn_72_A3C0)");
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

void CSpacePirate::TargetCover(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::TargetCover state (fn_72_78DC)");
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

void CSpacePirate::Approach(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Approach state (fn_72_9478)");
}

void CSpacePirate::WallHang(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::WallHang state (fn_72_69AC)");
}

void CSpacePirate::WallDetach(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::WallDetach state (fn_72_68E4)");
}

void CSpacePirate::GetUp(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::GetUp state (fn_72_B3D4)");
}

void CSpacePirate::Generate(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Generate state (fn_72_C1A4)");
}

void CSpacePirate::Skid(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Skid state (fn_72_94D0)");
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

void CSpacePirate::Jump(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Jump state (fn_72_5310)");
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
