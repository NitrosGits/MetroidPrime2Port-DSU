// The Ing half of the Boost Guardian's state machine: the first states of the Ing REL's own state
// machine (Start, WaitForFSMTransition, ExitHost, BecomeCorporeal, BecomeIngSpot,
// BecomeBodyProjectile, BecomeWallProjectile) plus the two helpers they call.
//
// The behaviour is ported from the Ing host body
// (wt-mp2-ing/src/MetroidPrime/Enemies/PortCIng.cpp, branch decomp/ing), which is the Ing REL's
// own module 29 code.
//
// Every `fn_29_` address, every asm address and every guest byte offset quoted with one in the
// comments below is the ING REL's: this module (IngBoostBallGuardian) carries its own copy of the
// Ing code, so the module's own addresses are in docs/module-analysis/BoostGuardian_fsm.tsv. The
// module's own half of the state machine (the boost ball, the puddle form, the mini portals) is in
// PortCIngBoostBallGuardianStates2.cpp.
//
// What this module does not have, and what was dropped because of it:
//   - no lure safe zone (the Ing's mLureSafeZoneId, mSafeZoneCount and mEvaporate do not exist
//     here), so FaceLureOrTarget only turns to the target;
//   - no safe-zone ing-spot flag (the Ing's mSafeZoneIngSpot does not exist here);
//   - no grapple block (the Ing's mData.mGrapple does not exist here);
//   - no exit-host class yet (CIngExitHostEffect belongs to the Ing branch and the module's own
//     exit-host body is a later slice), so ExitHost and SpawnExitHostSmoke only log.

#include "MetroidPrime/Enemies/CIngBoostBallGuardian.hpp"

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
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CKnockBackInfo.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/Weapons/CWeapon.hpp"
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Enemies/CGenericFSM2.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Enemies/CTeamAiRole.hpp"
#include "MetroidPrime/Enemies/PortBoostGuardianShared.hpp"
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

// ---- states ---------------------------------------------------------------------------------

// The Ing's fn_29_A094.
void CIngBoostBallGuardian::Start(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("Start");
  }
}

// The Ing's fn_29_A090 (not yet exercised in a run).
void CIngBoostBallGuardian::WaitForFSMTransition(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("WaitForFSMTransition");
  }
}

// The Ing's fn_29_9D68. The exit host: the Ing it leaves behind is a swarm. Its Activate swaps the
// possessed-mold billboard for the exit-host effect (one rs_new of 0x5e8, the class's only
// allocation), Update rides that effect until it reports it has arrived, and Deactivate drops the
// Ing back onto the mesh as an ing spot wherever SetExitHostDest pointed. Retail's Activate adds
// kMT_Target (0x28), Deactivate adds back 0x21/0x3b/0x29.
//
// NOT PORTED YET: this module's own copy of the exit-host body (the module's own equivalent of the
// Ing's fn_29_9D68) and the exit-host class are not part of this branch - the module's exit host is
// a later slice. The Ing body's behaviour is in the Ing branch
// (wt-mp2-ing/src/MetroidPrime/Enemies/PortCIng.cpp), so this body only logs the state.
void CIngBoostBallGuardian::ExitHost(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("ExitHost");
  }
}

// The Ing's fn_29_1F1C (the 2026-10-20 run, /tmp/opencode/ing_run5.log: "state ExitHost" -> "exit
// host: smoke explosion" -> "left at (78.0, 368.0, 4.9)" once the exit-host effect's Think set
// the arrival flag): the exit host's smoke, spawned where the Ing lands. It only fires when the
// swarm block's second GEN token is loaded (retail's engaged byte at 0xcfc); then one CExplosion
// "IngExitHostSmokeFx" on the Ing's own transform at the model's scale (flags 0, white, player -1)
// and the swarm block's own sound (0x9cc = CIngData swarm +0x28).
//
// NOT PORTED YET, for the same reason as ExitHost: the exit-host class and the module's own
// exit-host body do not exist in this branch, so there is no smoke to spawn. The body only logs.
void CIngBoostBallGuardian::SpawnExitHostSmoke(CStateManager&) {
  if (DebugBg()) {
    fprintf(stderr, "[ibbg] %04x exit host: the exit-host body is a later slice\n", GetUniqueId().Value());
  }
}

// The Ing's fn_29_1A80: turn to the lure safe zone (0xd78) when one is set, else to the target
// (0xd6c). The offset is taken horizontally (z of the delta is zeroed) and only applied when its
// magnitude is safe; the actor's own z is kept. Called by BecomeCorporeal's Activate.
//
// This module has no lure safe zone (the Ing's mLureSafeZoneId does not exist here), so only the
// "else turn to the target" half is ported.
void CIngBoostBallGuardian::FaceLureOrTarget(CStateManager& mgr) {
  const CVector3f pos = GetTranslation();
  // Retail reads the object's +0x54 without a type test; the port asks for an actor (the target a
  // CActor).
  const CActor* other = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (other == nullptr) {
    return;
  }
  CVector3f delta = other->GetTranslation() - pos;
  delta.SetZ(0.f);
  if (delta.IsMagnitudeSafe()) {
    SetTransform(CTransform4f::LookAt(pos, pos + delta, CVector3f::Up()));
  }
}

// The Ing's fn_29_9A74.
void CIngBoostBallGuardian::BecomeCorporeal(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("BecomeCorporeal");
    // 0x0000B088: 0x11EF 0x80 = 0. Retail clears 0x11ED 0x02 (mProjectileFSMEntry) in the
    // Deactivate (0x0000B448), not here: the Update below still reads it.
    mCurrentlyInBoostPuddleForm = false;
    SetIngSpotManagerVulnerability(mgr, false); // 0x0000B094: fn_30_30E4
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
    mCollisionActorManager0->SetActive(mgr, true); // guest 0xD60
    mCollisionActorManager1->SetActive(mgr, false); // fn_30_B03C 0x0000B144: guest 0xD64
    mSurfaceAlignment.SetMode(CSurfaceAlignmentHelper::kM_WorldUp); // 0xca4 = 2
    mLineOfSight.SetSegment(mEyeSegId);                            // 0xd8a = 0xd7b ("head")
    FaceLureOrTarget(mgr);                                         // the Ing's fn_29_1A80
    mMoving = false;                                               // 0xe5d bit 1
    Stop();
    SetBodyManagerReflect(mgr, false); // 0x0000B17C: fn_30_3190
    break;
  case kStateMsg_Update:
    // CTryCommand::ShouldRetry(this+0x6b4, *BodyController(), 0xc): 0xc is the body state
    // kAS_Generate, i.e. the port's CAnimationState tracks the Generate animation.
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      // 0x0000B1A0: 0x11EE 0x04; 0x0000B1AC: 0x11ED 0x02. Type 2 (0x0000B1E8) or 0.
      const bool puddle = mBoostPuddleGenerate && !mProjectileFSMEntry;
      BodyController()->CommandMgr().DeliverCmd(
          CBCGenerateCmd(puddle ? pas::kGType_Two : pas::kGType_Zero, CVector3f::Zero()));
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) {
      BodyController()->SetLocomotionType(pas::kLT_Relaxed); // 0x0000B36C: li r4, 1
    }
    Stop();
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    mPortalPlane = false;                                    // 0xe5d bit 6
    mIngSpotBlobVisible = false;                                 // 0xe5d bit 5
    mVerticalMovement = false;                               // CPatterned 0x34c bit 6
    AddMaterial(kMT_GroundCollider, mgr);                    // 0x25
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);   // 0x0000B3BC: li r4, 1
    mForm = kForm_Corporeal;
    mDesiredForm = -1;
    mSurfaceAlignment.SetMode(CSurfaceAlignmentHelper::kM_None); // 0xca4 = 0
    mFormTime = 0.f;                                             // 0xe3c
    if (AnimationData() != nullptr) {
      // rodata_68E: "eyes"; the effect runs while the Ing is corporeal.
      AnimationData()->SetEffectState(rstl::string_l("eyes"), true, mgr);
    }
    mBoostPuddleGenerate = false;           // 0x0000B42C: 0x11EE 0x04 = 0
    mProjectileFSMEntry = false;            // 0x0000B448: 0x11ED 0x02 = 0
    mBoost11E0 = 0.f;                       // 0x0000B450
    mBoostNavPointId2 = kInvalidUniqueId;   // 0x0000B458: 0x1098
    break;
  default:
    break;
  }
}

// The Ing's fn_29_9750.
void CIngBoostBallGuardian::BecomeIngSpot(CStateManager& mgr, EStateMsg msg, float) {
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
    // 0x0000AC98: 0x11ED 0x02 clear and 0x0000ACA4: 0x11EE 0x08 set -> 0x0000ACB4: 0x11EF 0x80 = 1.
    if (!mProjectileFSMEntry && mBoostPuddleForm) {
      mCurrentlyInBoostPuddleForm = true;
      SetIngSpotManagerVulnerability(mgr, true); // 0x0000ACC8: fn_30_30E4
      SetBodyManagerReflect(mgr, true);          // 0x0000ACD8: fn_30_3190
      SetBoostPuddleMode(true);                  // 0x0000ACE8: fn_30_3068
    }
    mSurfaceAlignment.SetMode(CSurfaceAlignmentHelper::kM_NearbySurface); // 0xca4 = 1
    mBodyState5Time = 0.f;                                                // 0xe40
    mBoostModeRetreatActive = false; // 0x0000AD08: 0x11EE 0x80 = 0
    // This module has no safe-zone ing-spot flag (the Ing's mSafeZoneIngSpot does not exist here),
    // so the Ing's "0xe5e bit 5 = false" has nothing to clear.
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      // 0x0000AD30: 0x11EF 0x80 picks type 5 (0x0000AD6C) over 1 (0x0000AE40).
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(
          mCurrentlyInBoostPuddleForm ? pas::kGType_Five : pas::kGType_One, CVector3f::Zero()));
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) {
      BodyController()->SetLocomotionType(pas::kLT_Crouch); // 0x0000AEF0: li r4, 0
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    BodyController()->SetLocomotionType(pas::kLT_Crouch);    // 0x0000AF00: li r4, 0
    if (mForm == kForm_BecomingIngSpot) { // 0x0000AF14: form == 7
      mForm = kForm_IngSpot;
    }
    mDesiredForm = -1;
    mLineOfSight.SetSegment(CSegId::Null()); // 0xd8a = 99
    mVerticalMovement = true;                // CPatterned 0x34c bit 6
    RemoveMaterial(kMT_GroundCollider, mgr); // 0x25
    mCollisionActorManager0->SetActive(mgr, false); // guest 0xD60
    mCollisionActorManager1->SetActive(mgr, true); // fn_30_ABC4 0x0000AF70: guest 0xD64
    mPortalPlane = false;   // 0xe5d bit 6
    mIngSpotBlobVisible = true; // 0xe5d bit 5
    if (AnimationData() != nullptr) {
      AnimationData()->SetEffectState(rstl::string_l("eyes"), false, mgr);
    }
    break;
  default:
    break;
  }
}

// The Ing's fn_29_4468: the target's aim position (vtable slot 0x54, CActor::GetAimPosition, with
// the caller's prediction time in f1). False when the target id is empty; the caller then keeps
// whatever it had. Every caller passes rodata_6C (0.0f) except the body projectile's flight, which
// passes rodata_B4 (0.5f) - asm 0x00008054 loads it just before the call at 0x0000806C.
bool CIngBoostBallGuardian::TargetAimPosition(CStateManager& mgr, CVector3f& out, float prediction) {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mTargetId));
  if (target == nullptr) {
    return false;
  }
  out = target->GetAimPosition(mgr, prediction);
  return true;
}

// The Ing's fn_29_94E8 (not yet exercised in a run).
void CIngBoostBallGuardian::BecomeBodyProjectile(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("BecomeBodyProjectile");
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6b4 = 1
    // Form 4, the flying body projectile, is set by the kUE_BeginAction user anim event
    // (the Ing's fn_29_C228, handled in DoUserAnimEvent) once the anim reaches it; the state then
    // ends and the flight takes over.
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
      // The Ing's asm 0x00009628: SetLocomotionType(0), i.e. kLT_Crouch.
      BodyController()->SetLocomotionType(pas::kLT_Crouch);
      if (!mTakeOff && mForm != kForm_BodyProjectile) {
        // Aim at the target's aim position, flattened to the actor's own height (0xdd0 = 0x5c).
        if (TargetAimPosition(mgr, mTargetPos)) { // the Ing's fn_29_4468
          mTargetPos.SetZ(GetTranslation().GetZ());
          BodyController()->CommandMgr().SetTargetVector(mTargetPos - GetTranslation());
        }
      } else {
        mCollisionActorManager0->SetActive(mgr, false); // guest 0xD60
        mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
      }
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    mVerticalMovement = true;                                // CPatterned 0x34c bit 6
    RemoveMaterial(kMT_GroundCollider, mgr);                 // 0x25
    mCollisionActorManager0->SetActive(mgr, false); // guest 0xD60
    // The Ing's asm 0x000096F8: SetLocomotionType(0), i.e. kLT_Crouch.
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    if (AnimationData() != nullptr) {
      AnimationData()->SetEffectState(rstl::string_l("eyes"), false, mgr); // rodata_68E
    }
    break;
  default:
    break;
  }
}

// The Ing's fn_29_8FE8 (not yet exercised in a run).
void CIngBoostBallGuardian::BecomeWallProjectile(CStateManager& mgr, EStateMsg msg, float) {
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
    const CVector3f side = CVector3f::Cross(normal, CVector3f::Up()); // the Ing's fn_29_94A8
    if (side.IsMagnitudeSafe()) {
      SetTransform(CTransform4f::FromColumns(side.AsNormalized(), normal, CVector3f::Up(),
                                            GetTranslation() + normal));
    }
    mSurfaceAlignment.SetMode(CSurfaceAlignmentHelper::kM_None); // 0xca4 = 0
    mPortalPlane = true;                                         // 0xe5d bit 6
    // The portal plane sits the bounding box's Y extent below the actor along the normal; retail
    // scalarises dot(translation - h*normal, normal) into (x,nx),(y,ny),(z,nz) (the Ing's asm
    // 0x00009194).
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
      // The Ing's asm 0x0000928C: SetLocomotionType(0), i.e. kLT_Crouch.
      BodyController()->SetLocomotionType(pas::kLT_Crouch);
      mForm9Render = true; // 0xe5d bit 4
      if (mTakeOff || mForm == kForm_BodyProjectile) {
        mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
      } else if (TargetAimPosition(mgr, mTargetPos)) { // the Ing's fn_29_4468
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
    mIngSpotBlobVisible = false; // 0xe5d bit 5
    // The Ing's asm 0x000093C4: SetLocomotionType(0), i.e. kLT_Crouch.
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    if (AnimationData() != nullptr) {
      AnimationData()->SetEffectState(rstl::string_l("eyes"), false, mgr); // rodata_68E
    }
    break;
  default:
    break;
  }
}
