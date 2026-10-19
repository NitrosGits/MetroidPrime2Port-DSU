// The 37 states of the SpacePirate state machine (retail table fn_72_10E50 -> lbl_72_data_604).
// Milestone 2 is the skeleton: a state CPatterned also has forwards to it, every other one is a
// no-op behind a once-per-name StandIn that names the retail function. Later lanes port the bodies
// from docs/module-analysis/SpacePirate.md and build/G2ME01/SpacePirate/m2c/fn_72_<off>.c.
#include "MetroidPrime/Enemies/CSpacePirate.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CPASDatabase.hpp"
#include "MetroidPrime/CStateManager.hpp"
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

void CSpacePirate::Lurk(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Lurk state (fn_72_AF70)");
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
