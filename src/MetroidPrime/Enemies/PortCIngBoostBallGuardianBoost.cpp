// The boost half of the Boost Guardian's state machine: the six boost triggers and the two boost
// states.
//
// The behaviour is ported from this module's own bodies - the IngBoostBallGuardian REL (module
// 30), not the Ing REL (module 29). The names are the REL's own strings, the addresses are the
// module's own (docs/module-analysis/BoostGuardian_fsm.tsv has the name-to-address table,
// docs/module-analysis/BoostGuardian.md section b.2 what each body does).
//
// The guest offsets quoted below are the REL's own (its object is 0x11F0 bytes). The host layout
// is its own: the flag bytes are one-bit members (include/MetroidPrime/Enemies/
// CIngBoostBallGuardian.hpp), so a retail bit test reads the member that holds that bit.
//
// The boost attack's child is the module's second class, the boost ball
// (PortCBoostBallGuardianBall.cpp; BoostGuardian.md section f).
#include "MetroidPrime/Enemies/CIngBoostBallGuardian.hpp"

#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CTransform4f.hpp"
#include "Kyoto/Math/CVector3f.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Collision/CMaterialList.hpp"
#include "Collision/CRayCastResult.hpp"
#include "Kyoto/CRandom16.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/PortCBoostBallGuardianBall.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Enemies/PortBoostGuardianShared.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIHint.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include <stdio.h>

// ---- triggers -------------------------------------------------------------------------------

// fn_30_B788: HasCollidedWithCrystal (guest 0x11EE bit 0).
bool CIngBoostBallGuardian::HasCollidedWithCrystal(CStateManager& mgr,
                                                   const CTriggerData& data) const {
  return mCollidedWithCrystal;
}

// fn_30_B794: ShouldBoost (guest 0x11EE bit 4).
bool CIngBoostBallGuardian::ShouldBoost(CStateManager& mgr, const CTriggerData& data) const {
  return mShouldBoost;
}

// fn_30_B7A0: IsCurrentlyInBoostPuddleForm (guest 0x11EF bit 7).
bool CIngBoostBallGuardian::IsCurrentlyInBoostPuddleForm(CStateManager& mgr,
                                                        const CTriggerData& data) const {
  return mCurrentlyInBoostPuddleForm;
}

// fn_30_B7AC: IsBoostPuddleForm (guest 0x11EE bit 3 and not guest 0x11ED bit 1, the bit
// UseProjectileFSMEntry fn_30_C1A4 reads).
bool CIngBoostBallGuardian::IsBoostPuddleForm(CStateManager& mgr, const CTriggerData& data) const {
  // 0x0000B7B4: 0x11EE 0x08; 0x0000B7C0: 0x11ED 0x02.
  return mBoostPuddleForm && !mProjectileFSMEntry;
}

// fn_30_B7D4: IsBoostOver (guest 0x11EE bit 5).
bool CIngBoostBallGuardian::IsBoostOver(CStateManager& mgr, const CTriggerData& data) const {
  return mBoostOver;
}

// fn_30_B76C: BoostModeRetreatOver (the u16 at guest 0x1096 is kInvalidUniqueId).
bool CIngBoostBallGuardian::BoostModeRetreatOver(CStateManager& mgr,
                                                 const CTriggerData& data) const {
  return mBoostNavPointId == kInvalidUniqueId;
}

// fn_30_2F9C: the first AI hint of the waypoint list (mgr+0x838, 0x00002FC0) that is active
// (byte 0x20 bit 0x80, 0x00002FE4), in this actor's area (+0x04, 0x00003000) and of the given
// type (+0x158, 0x00003010); null when there is none (0x00003044).
// @ fn_30_2F9C
CScriptAIHint* CIngBoostBallGuardian::FindBoostNavPoint(CStateManager& mgr, const CActor& actor,
                                                       int hintType) {
  CObjectList& list = mgr.ObjectListById(kOL_AiWaypoint);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    CScriptAIHint* hint = TCastToPtr< CScriptAIHint >(list[i]);
    if (hint != nullptr && hint->GetActive() && hint->GetCurrentAreaId() == actor.GetCurrentAreaId() &&
        static_cast< int >(hint->GetHintType()) == hintType) {
      return hint;
    }
  }
  return nullptr;
}

// ---- states ---------------------------------------------------------------------------------

// fn_30_340C: the player's aim position (vtable +0x54, no prediction), its z replaced by the
// actor's.
// @ fn_30_340C
void CIngBoostBallGuardian::UpdateBoostAimPos(CStateManager& mgr) {
  mBoostAimPos = mgr.GetPlayer(0)->GetAimPosition(mgr, 0.f); // mgr+0x14FC
  mBoostAimPos.SetZ(GetTranslation().GetZ());               // 0x00003460
}

// fn_30_324C: the ground under the actor - a 10-unit ray down the actor's up axis from one unit
// above it, against solid geometry only (lbl_30_data_9BC); zero when it hits nothing.
// @ fn_30_324C
CVector3f CIngBoostBallGuardian::FindBoostSpawnPos(CStateManager& mgr) const {
  const CVector3f up = GetTransform().GetColumn(kDZ);
  const CRayCastResult result =
      mgr.RayStaticIntersection(GetTranslation() + up, -up, 10.f,
                                CMaterialFilter::MakeInclude(CMaterialList(kMT_Solid)));
  return result.IsValid() ? result.GetPoint() : CVector3f::Zero(); // 0x00003340
}

// fn_30_3478: spawns the boost ball facing the player and keeps its id.
// @ fn_30_3478
void CIngBoostBallGuardian::SpawnBoostBall(CStateManager& mgr) {
  UpdateBoostAimPos(mgr);
  // 0x000034CC/0x000034D0: both ends of the look in the ground plane.
  CVector3f from = GetTranslation();
  from.SetZ(0.f);
  mBoostAimPos.SetZ(0.f);
  const CTransform4f xf = CTransform4f::LookAt(from, mBoostAimPos, CVector3f::Up());
  SendScriptMsgs(kSS_Entered, mgr, kInvalidUniqueId, kSM_None); // 0x0000350C: 'ENTR'
  mShouldBoost = false;                                         // 0x00003518: 0x11EE 0x10 = 0
  const CModelData modelData(CStaticRes(mBoost.mModel, mBoost.mScale)); // 0x00003548
  // 0x00003568: a copy of this actor's health info with the current hp reset to the initial.
  CHealthInfo health = *HealthInfo();
  health.SetHP(health.GetInitialHP());
  mBoostSpawnPos = FindBoostSpawnPos(mgr); // 0x00003594: 0x116C
  const CMaterialList matList(kMT_CollisionActor, kMT_Immovable); // lbl_30_data_9B4/9B8
  CBoostBallGuardianBall* ball = rs_new CBoostBallGuardianBall(
      mgr.AllocateUniqueId(), rstl::string_l("BoostBallGuardian"),
      CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId), xf,
      mBoostSpawnPos, modelData, CActorParameters(), GetUniqueId(), mBoost, mPhaseIndex,
      mBoost.mVulnerability, mModifyTargets, &matList, &health); // 0x00003704: fn_30_15924
  if (ball != nullptr) {
    mgr.AddObject(ball);                       // 0x00003744
    ball->RemoveMaterial(kMT_Projectile, mgr); // 0x00003754
    mBoostChildId = ball->GetUniqueId();       // 0x00003760: 0x1094
    mBoostOver = false;                        // 0x00003768: 0x11EE 0x20 = 0
  }
}

// fn_30_79C0: BoostAttack. The state ends on IsBoostOver, which the ball's shrink raises
// (fn_30_13B98, 0x5D8 bit 0x04), or on HasCollidedWithCrystal.
// @ fn_30_79C0
void CIngBoostBallGuardian::BoostAttack(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("BoostAttack");
    SpawnBoostBall(mgr); // 0x00007A04
    // 0x00007A14: retail calls the 0xD64 manager unchecked; the host does not build it yet.
    if (mCollisionActorManager1 != nullptr) {
      mCollisionActorManager1->SetActive(mgr, false);
    }
    break;
  case kStateMsg_Update:
    if (CBoostBallGuardianBall* ball =
            TCastToPtr< CBoostBallGuardianBall >(mgr.ObjectById(mBoostChildId))) {
      if (ball->IsDone()) {
        mBoostOver = true; // 0x00007A50: 0x11EE 0x20 = 1
      }
      // 0x00007A58: follow the ball, at the spawn height (0x1174).
      CVector3f pos = ball->GetTranslation();
      pos.SetZ(mBoostSpawnPos.GetZ());
      SetTranslation(pos);
    }
    break;
  case kStateMsg_Deactivate:
    mBoostOver = true; // 0x00007A90
    if (CBoostBallGuardianBall* ball =
            TCastToPtr< CBoostBallGuardianBall >(mgr.ObjectById(mBoostChildId))) {
      ball->Stop();          // 0x00007AB8
      ball->SetHalted(true); // 0x00007AC4: 0x5D8 0x01 = 1
      CVector3f pos = ball->GetTranslation();
      pos.SetZ(mBoostSpawnPos.GetZ());
      SetTranslation(pos);                                        // 0x00007AF4
      SendScriptMsgs(kSS_Exited, mgr, kInvalidUniqueId, kSM_None); // 0x00007B20: 'EXIT'
      mBoostRandomFloat = mgr.Random()->Range(CurrentPhase().mNextBoostTimeMin,
                                              CurrentPhase().mNextBoostTimeMax); // 0x00007B34
    }
    if (mCollidedWithCrystal) { // 0x00007B40
      mBoostTimer = 0.f;
      mCollidedWithCrystal = false;
    }
    if (mCollisionActorManager1 != nullptr) {
      mCollisionActorManager1->SetActive(mgr, true); // 0x00007B70
    }
    mgr.DeleteObjectRequest(mBoostChildId); // 0x00007B84
    mBoostChildId = kInvalidUniqueId;
    break;
  default:
    break;
  }
}

// fn_30_7728: BoostModeRetreat.
//
// Retail's Activate clears bit 4 of 0x11EC (mFoundMovementPos) and sets bit 7 of 0x11EE, then
// finds two navigation points with fn_30_2F9C (this module's own nav-point finder, called with
// 0xE and 0xF): the first point's id goes to 0x1096 (mBoostNavPointId) and its position becomes
// the navigation's destination and bit 4 of 0x11EC is set again, the second point's id goes to 0x1098 (mBoostNavPointId2), and the
// navigation's face target (guest 0x718) is cleared.
//
// Retail's Update drops the first nav point (0x1096) when the actor is within 1.0 of it, and drops
// it too when the point is gone.
//
// Retail's Deactivate turns to the second nav point (0x1098) in the horizontal plane (both z's are
// zeroed, the actor keeps its own translation), clears bit 7 of 0x11EE and sends the script state
// 'CTIM' (0x4354494D) with kInvalidUniqueId.
//
// Every message falls into this module's own IngSpotPathFind (fn_30_9DA0) afterwards.
// @ fn_30_7728
void CIngBoostBallGuardian::BoostModeRetreat(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    LogState("BoostModeRetreat");
    mFoundMovementPos = false;        // 0x00007784: 0x11EC 0x10 = 0
    mBoostModeRetreatActive = true;   // 0x000077A0: 0x11EE 0x80 = 1
    if (const CScriptAIHint* point = FindBoostNavPoint(mgr, 0xE)) { // 0x000077A8
      mFoundMovementPos = true;    // 0x000077BC: 0x11EC 0x10 = 1
      mBoostNavPointId = point->GetUniqueId(); // 0x000077D0: 0x1096
      mPathFindNavigation.SetDestination(point->GetTranslation()); // 0x000077D4
      mPathFindNavigation.SetFaceTarget(kInvalidUniqueId);         // 0x000077E0: 0x718
    }
    if (const CScriptAIHint* point = FindBoostNavPoint(mgr, 0xF)) { // 0x000077F0
      mBoostNavPointId2 = point->GetUniqueId(); // 0x00007800: 0x1098
    }
    if (DebugBg()) {
      fprintf(stderr, "[ibbg] %04x boost retreat: nav points %04x / %04x\n",
              GetUniqueId().Value(), mBoostNavPointId.Value(), mBoostNavPointId2.Value());
    }
    break;
  }
  case kStateMsg_Update: {
    const CScriptAIHint* hint =
        TCastToConstPtr< CScriptAIHint >(mgr.GetObjectById(mBoostNavPointId));
    if (hint == nullptr) {
      mBoostNavPointId = kInvalidUniqueId;
    } else {
      const CVector3f delta = GetTranslation() - hint->GetTranslation();
      if (delta.MagSquared() < 1.0f) {
        mBoostNavPointId = kInvalidUniqueId;
      }
    }
    break;
  }
  case kStateMsg_Deactivate: {
    const CScriptAIHint* hint =
        TCastToConstPtr< CScriptAIHint >(mgr.GetObjectById(mBoostNavPointId2));
    if (hint != nullptr) {
      const CVector3f pos = GetTranslation();
      const CVector3f target = hint->GetTranslation();
      const CTransform4f look = CTransform4f::LookAt(CVector3f(pos.GetX(), pos.GetY(), 0.f),
                                                    CVector3f(target.GetX(), target.GetY(), 0.f),
                                                    CVector3f::Up());
      SetTransform(CQuaternion::FromMatrix(look).BuildTransform4f(pos));
    }
    // Guest 0x11EE bit 7 (mBoostModeRetreatActive) is cleared here in retail.
    mBoostModeRetreatActive = false;
    // The script state is the REL's 'CTIM' (0x4354494D), which the host names kSS_CameraTime.
    SendScriptMsgs(kSS_CameraTime, mgr, kInvalidUniqueId, kSM_None);
    break;
  }
  }
  // Retail ends every message with this module's own IngSpotPathFind (fn_30_9DA0).
  IngSpotPathFind(mgr, msg, dt);
}
