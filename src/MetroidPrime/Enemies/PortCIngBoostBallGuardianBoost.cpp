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
// Two things this slice does not have:
//   - the nav-point finder fn_30_2F9C (the retreat's Activate needs it) and the module's second
//     class (a CPhysicsActor built by fn_30_3478, the boost attack's child), so the retreat's
//     Activate and the whole of BoostAttack are a later slice.
#include "MetroidPrime/Enemies/CIngBoostBallGuardian.hpp"

#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CTransform4f.hpp"
#include "Kyoto/Math/CVector3f.hpp"
#include "MetroidPrime/CStateManager.hpp"
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

// fn_30_B7AC: IsBoostPuddleForm (guest 0x11EE bit 3 and not guest 0x11ED bit 1).
bool CIngBoostBallGuardian::IsBoostPuddleForm(CStateManager& mgr, const CTriggerData& data) const {
  return mBoostPuddleForm && !x11EDBit1;
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

// ---- states ---------------------------------------------------------------------------------

// fn_30_79C0: BoostAttack.
//
// Retail's three cases:
//   - Activate: spawns the module's second class (a CPhysicsActor built by fn_30_3478) and calls
//     SetActive(false) on the collision manager at guest 0xD64;
//   - Update: copies the child's translation (its +0x54/+0x58/+0x5C, with z taken from the
//     parent's 0x1174) onto the actor, and sets bit 5 of 0x11EE (mBoostOver) when fn_30_13B98() is
//     not zero;
//   - Deactivate: sets bit 5 of 0x11EE (mBoostOver), stops the child and sets bit 0 of its byte
//     0x5D8, copies the child's translation onto the actor, sends the script state 'EXIT'
//     (0x45584954) with kInvalidUniqueId, randomises the float at 0x11E8 (mBoostRandomFloat) from
//     the array element at 0x11D0 (+0x10, +0x14), clears bit 0 of 0x11EE (mCollidedWithCrystal)
//     and zeroes the float at 0x11DC (mBoostTimer), sets the collision manager at 0xD64 active
//     and deletes the child by the id at 0x1094 (mBoostChildId).
//
// The second class is a later slice (the module note's WP4), so this body only logs: the
// module's own fn_30_79C0 does the work once the class exists.
void CIngBoostBallGuardian::BoostAttack(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("BoostAttack");
    if (DebugBg()) {
      fprintf(stderr, "[ibbg] %04x boost attack: the second class (fn_30_3478) is a later slice\n",
              GetUniqueId().Value());
    }
  }
}

// fn_30_7728: BoostModeRetreat.
//
// Retail's Activate clears bit 4 of 0x11EC (mBoostRetreatActive) and sets bit 7 of 0x11EE, then
// finds two navigation points with fn_30_2F9C (this module's own nav-point finder, called with
// 0xE and 0xF): the first point's id goes to 0x1096 (mBoostNavPointId) and its position becomes
// the navigation's destination, the second point's id goes to 0x1098 (mBoostNavPointId2), and the
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
//
// NOT YET: fn_30_2F9C (the nav-point finder) is a later slice, so the Activate case only logs
// and the nav-point work waits for it. The two flag bits retail writes here are written
// anyway: bit 4 of 0x11EC (mBoostRetreatActive) is cleared and bit 7 of 0x11EE
// (mBoostModeRetreatActive) is set - the latter is what this module's own IngSpotPathFind
// (fn_30_9DA0) reads to pick one of two speeds.
void CIngBoostBallGuardian::BoostModeRetreat(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("BoostModeRetreat");
    mBoostRetreatActive = false;      // guest 0x11EC bit 4
    mBoostModeRetreatActive = true;   // guest 0x11EE bit 7
    if (DebugBg()) {
      fprintf(stderr,
              "[ibbg] %04x boost retreat: the nav points (fn_30_2F9C) are a later slice\n",
              GetUniqueId().Value());
    }
    break;
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
