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
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"

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

void CSpacePirate::Run(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Run state (fn_72_BBA0)");
}

void CSpacePirate::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::PathFind state (fn_72_8A90)");
  CPatterned::PathFind(mgr, msg, dt);
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
    // TODO(fn_72_D800): retail also picks/keeps its guard position here (cover-point nudge plus
    // the line-of-sight test that drives x8f7_28_noPlayerLos); not ported yet.
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

void CSpacePirate::TargetPlayer(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::TargetPlayer state (fn_72_7868)");
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
