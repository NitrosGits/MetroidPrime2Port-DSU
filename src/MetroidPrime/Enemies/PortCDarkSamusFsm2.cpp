// The Dark Samus's state machine, part two: the missile, hit, patrol, taunt and phazon states
// (fn_10_10C9C, 10FF4, 1142C, 11544, 16ECC, 17448, 178E4, 18EAC and the phazon family 4AC8,
// 6460, 652C, 67B8, 6BA0, 7054, 7BC0, 81D4). The tables and the rest of the machine are in
// PortCDarkSamusFsm.cpp.
//
// The module helpers these states call that are not ported yet (fn_10_1BC58, 65CC, 6374, 7F98,
// 69C0, 746C, 7610, 786C, 452C, 5974, 48C8, 4868, 812C) are stand-in calls: Fsm2StandIn logs
// the first call and the call returns the neutral value. A state that calls one stays unmarked.
// The body-state command helpers fn_10_8698 (taunt), fn_10_8870 (loop attack) and fn_10_22020
// (getup) are the same three-message switch as fn_10_21DF8 (DoStepCmd): written inline, as the
// base's PissedOff and TurnToFacePlayer write fn_10_8698.
#include "MetroidPrime/Enemies/CDarkSamus.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/Math/CAABox.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CUnitVector3f.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"

#include <stdio.h>
#include <stdlib.h>

#include "types.h"

namespace {

// The once-only log of a stand-in call: one line per retail function, the first time it runs.
void Fsm2StandIn(const char* fn, const char* what) {
  static const char* sSeen[32] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == fn) {
      return;
    }
  }
  if (sCount < 32) {
    sSeen[sCount++] = fn;
  }
  fprintf(stderr, "[darksamus] fsm stand-in %s (%s)\n", fn, what);
}

// The state change log of PortCDarkSamusFsm.cpp's DsStateEntered (MP2_DS_FSM).
void DsStateEntered(const char* name, EStateMsg msg) {
  if (msg == kStateMsg_Activate && getenv("MP2_DS_FSM") != nullptr) {
    fprintf(stderr, "[dsfsm] state change: %s\n", name);
  }
}

// fn_10_65CC (not ported): the phazon waypoint of a type; the stand-in has none (the zero vector,
// the value fn_10_7054 tests for).
CVector3f StandInPhazonWaypoint(int type) {
  (void)type;
  Fsm2StandIn("fn_10_65CC", "phazon waypoint");
  return CVector3f::Zero();
}

// fn_10_1BC58 (not ported): the distance to the nearest incoming projectile; with none it answers
// lbl_10_rodata_554 = FLT_MAX, which the stand-in always answers.
float StandInIncomingProjectileDistance() {
  Fsm2StandIn("fn_10_1BC58", "incoming projectile distance");
  return 3.4028235e38f;
}

} // namespace

// fn_10_10C9C (0x00010C9C, 0x2C8 bytes): the HeavyHit state.
// Activate (`mr. r31, r5` at 0x00010CE0): the kBSC_NextState command (`li r0, 0xc`, 0x00010D0C),
// then the step direction 0xC1C. The table is the flat forward (the transform's 0x28/0x38/0x48
// with z = lbl_10_rodata_3FC = 0, normalized when it can be, 0x00010D50), its negation
// (lbl_10_rodata_3EC = -1.0f, 0x00010D98), the negated flat right (0x24/0x34/0x44, 0x00010DEC)
// and the flat right, against lbl_10_rodata_36C = {0, 1, 2, 3}; 0xC1C starts at 1
// (0x00010E70) and the first entry whose angle to the hit direction 0xC10 is inside
// (lbl_10_rodata_468 = -0.7853982, lbl_10_rodata_464 = 0.7853982) (0x00010E80/0x00010E88) sets it.
// Every message: the step command, pas::kAS_Step (`li r5, 0x3`) with the direction 0xC1C and the
// step type 3 (`li r7, 0x3`), fn_10_21DF8 at 0x00010F18. Deactivate (`cmpwi r31, 0x2` at
// 0x00010F28) clears 0xC0C (0x00010F44) and takes the elapsed time into 0xC20 (0x00010F4C).
//   arm                                           | run
//   Activate / not (0x00010CE8)                   | both
//   forward can / cannot be normalized (10D5C)    | can only
//   right can / cannot be normalized (10DC4)      | can only
//   table entry i hits (10E84/10E8C), i = 0..3    | i = 0 only
//   no entry hits (loop ends, 10EB0)              | no
//   Deactivate / not (0x00010F38)                 | both
// Unmarked: the cannot-normalize arms and the entries 1..3 never ran (tools/cov/specs/
// darksamus_fsm2.py on 10_cliff_rooftop, 4500 frames).
void CDarkSamus::HeavyHit(CStateManager&, EStateMsg msg, float) {
  DsStateEntered("HeavyHit", msg);
  if (msg == kStateMsg_Activate) {
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_NextState)); // 0x00010D0C
    const CVector3f fwd3 = GetTransform().GetForward();
    CVector3f forward(fwd3.GetX(), fwd3.GetY(), 0.f);
    if (forward.CanBeNormalized()) { // 0x00010D5C
      forward.Normalize();
    }
    const CVector3f right3 = GetTransform().GetRight();
    CVector3f right(right3.GetX(), right3.GetY(), 0.f);
    if (right.CanBeNormalized()) { // 0x00010DC4
      right.Normalize();
    }
    const CVector3f table[4] = {forward, -1.f * forward, -1.f * right, right};
    static const int kDirs[4] = {0, 1, 2, 3}; // lbl_10_rodata_36C
    mGuestC1C = 1;                            // 0x00010E70
    for (int i = 0; i < 4; ++i) {
      const float angle = CVector3f::GetAngleDiff(table[i], mHitDirectionFlat);
      if (angle < 0.7853982f && angle > -0.7853982f) { // 0x00010E80 / 0x00010E88
        mGuestC1C = kDirs[i];                        // 0x00010E9C
        break;
      }
    }
  }
  DoStepCmd(msg, pas::kAS_Step,
            CBCStepCmd(static_cast< pas::EStepDirection >(mGuestC1C),
                       static_cast< pas::EStepType >(3))); // 0x00010F18
  if (msg == kStateMsg_Deactivate) { // 0x00010F28
    mGuestC0C = 0.f;                 // 0x00010F44
    mGuestC20 = mElapsedTime;        // 0x00010F4C
  }
}

// fn_10_10FF4 (0x00010FF4, 0xEC bytes): the Interrupt state, no message test. The last finished
// attack (the history 0x998 at the count 0x994 - 1, or 0xFFFF with none, 0x00011010) picks the
// command: 0x20, 0x40, 0x100, 0x100000, 0x200000 the kBSC_ExitState (`li r5, 0xa`, 0x00011090),
// 0x200 and 0x800 the kBSC_AbortScripted (`li r5, 0xd`, 0x00011098), anything else the
// kBSC_NextState (`li r5, 0xc`, 0x00011030); the accumulated damage 0xCC0 is cleared first
// (lbl_10_rodata_3FC, 0x00011034) and the command is delivered (0x000110B8).
//   arm                                      | run
//   history empty / not (0x00011010)         | not; empty forced once (gdb, count 0)
//   0x20 / 0x40 / 0x100 -> 0xA               | 0x40, 0x100 natural; 0x20 forced once
//   0x100000 / 0x200000 -> 0xA               | each forced once (gdb, history[count-1])
//   0x200 -> 0xD (Taunt), 0x800 -> 0xD       | 0x200 natural; 0x800 forced once
//   other -> 0xC                             | natural (0x1, 0x8000)
// Run: tools/cov/specs/darksamus_fsm2.py (natural) and darksamus_fsm2_force.py (the gdb writes)
// on 10_cliff_rooftop; both exit 0, 0 caught.
// @ fn_10_10FF4
void CDarkSamus::Interrupt(CStateManager&, EStateMsg msg, float) {
  DsStateEntered("Interrupt", msg);
  uint last = 0xFFFF;              // `subi r6, r4, 0x1` at 0x00011004
  if (mAttackHistoryCount > 0) {   // 0x00011010
    last = mAttackHistory[mAttackHistoryCount - 1];
  }
  mAccumulatedDamage = 0.f;        // 0x00011034
  EBodyStateCmd cmd = kBSC_NextState;
  switch (last) {
  case 0x20:
  case 0x40:
  case 0x100:
  case 0x100000:
  case 0x200000:
    cmd = kBSC_ExitState; // 0x00011090
    break;
  case 0x200:
  case 0x800:
    cmd = kBSC_AbortScripted; // 0x00011098
    break;
  default:
    break;
  }
  BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(cmd)); // 0x000110B8
}

// fn_10_1142C (0x0001142C, 0x118 bytes): the Patrol state. The flat direction to the player (the
// player at the manager's 0x14FC, z = lbl_10_rodata_3FC, 0x0001149C) goes, normalized, into the
// base's 0x6E0 (the waypoint navigation's face vector: the navigation at 0x6B8, +0x28) when it can
// be normalized (0x000114AC); Deactivate (`cmpwi r31, 0x2` at 0x000114D0) forgets the last
// attacker (0xC24 = kInvalidUniqueId). Then CPatterned::Patrol (0x000114F4), the separation
// (fn_10_1DD14), the additive animation (fn_10_1D860) and the glide emitter (fn_10_11334).
// Unmarked: never run - Patrol is not a state of the 10_cliff_rooftop FSM asset (`fsm` answers
// "not entered (unknown name?)").
void CDarkSamus::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("Patrol", msg);
  CVector3f toPlayer = (mgr.GetPlayer(0)->GetTranslation() - GetTranslation()).DropZ();
  if (toPlayer.CanBeNormalized()) { // 0x000114AC
    toPlayer.Normalize();
    mWaypointNavigation.SetFaceVector(toPlayer); // 0x000114BC..0x000114CC
  }
  if (msg == kStateMsg_Deactivate) { // 0x000114D0
    mLastAttackerUid = kInvalidUniqueId;
  }
  CPatterned::Patrol(mgr, msg, dt);       // 0x000114F4
  Separate(mgr);                          // 0x00011500
  UpdateAdditiveAnimation(mgr, msg);      // 0x00011510
  UpdateGlideEmitter(msg);                // 0x0001151C
}

// fn_10_11544 (0x00011544, 0x1B4 bytes): the Taunt state. The attack bookkeeping with 0x200
// (fn_10_1B478, 0x00011580). Activate (`cmpwi r31, 0x0` at 0x00011584) picks a new taunt type
// 0xBE8 from lbl_10_rodata_360 = {9, 0xA, 0xB} without the current one: the first two that
// differ from it are kept (0x000115AC..0x00011610) and Range(lbl_10_rodata_3FC = 0,
// lbl_10_rodata_3E8 = 1) below lbl_10_rodata_3E4 = 0.5 picks the first (0x00011638), else the
// second. Every message: the damage accumulation (fn_10_1E178, 0x00011660), the turn toward the
// player at lbl_10_rodata_470 = 1.2217305 rad/s (fn_10_16B64, 0x0001167C) and the taunt command
// (fn_10_8698, pas::kAS_Taunt = `li r5, 0x10`, {0x15 = kBSC_Taunt, 0xBE8}, 0x000116B8).
//   arm                                        | run
//   Activate / not (0x00011584)                | both
//   current 9 / 0xA / 0xB skipped              | all three (9 forced once: gdb 0xBE8 = 9)
//   Range < 0.5 / >= 0.5 (0x00011638)          | both (11 activations)
//   taunt cmd: Update CanIssueCommand yes / no | both; Deactivate run
// Run: tools/cov/specs/darksamus_fsm2_force.py (Taunt alternating with Interrupt), exit 0.
// @ fn_10_11544
void CDarkSamus::Taunt(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("Taunt", msg);
  UpdateCurrentAttack(0x200, mgr, msg);
  if (msg == kStateMsg_Activate) {
    static const int kTaunts[3] = {9, 0xA, 0xB}; // lbl_10_rodata_360
    int picks[2] = {0, 0};
    int count = 0;
    for (int i = 0; i < 3 && count < 2; ++i) { // 0x000115AC..0x00011610
      if (mGuestBE8 != kTaunts[i]) {
        picks[count++] = kTaunts[i];
      }
    }
    if (mgr.Random()->Range(0.f, 1.f) < 0.5f) { // 0x00011638
      mGuestBE8 = picks[0];
    } else {
      mGuestBE8 = picks[1];
    }
  }
  AccumulateDamage(msg, dt);                // 0x00011660
  TurnTowardPlayer(mgr, dt, 1.2217305f);    // 0x0001167C
  const CBCTauntCmd taunt(static_cast< pas::ETauntType >(mGuestBE8));
  switch (msg) { // fn_10_8698
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(taunt);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
      BodyController()->CommandMgr().DeliverCmd(taunt);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  default:
    break;
  }
}

// fn_10_16ECC (0x00016ECC, 0x1B0 bytes): the SuperMissileAttack state. The attack bookkeeping with
// 0x100 (0x00016F08), the shield flag (fn_10_105F4), the ground collision (fn_10_18F90) and the
// collision failsafe (fn_10_1AEC0). Activate (`cmpwi r31, 0x0` at 0x00016F3C) clears the shots
// fired 0xA78, the shots to fire 0xA7C and the break byte 0xA80, then 0xA7C = 1 (0x00016F58);
// otherwise, with the shots fired at least 0xA7C (0x00016F68), the kBSC_ExitState command
// (0x00016F90) ends the call. While the byte 0xA84 is 1 (0x00016FB0) the actor turns toward the
// player at lbl_10_rodata_4F8 = 2.7925267 rad/s (fn_10_16B64) and dodges (fn_10_1707C with
// lbl_10_rodata_4FC = 22.0f, lbl_10_rodata_3E4 = 0.5f). Then the loop attack command
// (fn_10_8870, pas::kAS_LoopAttack = `li r5, 0x9`, type 0, not waiting for the anim, 0x0001703C).
// 0xA78, 0xA80 (the BreakSuperMissile byte) and 0xA84 are written by the anim-event handler
// fn_10_1BEFC.
//   arm                                        | run
//   Activate / not (0x00016F3C)                | both
//   shots fired >= to fire: exit (0x00016F68)  | forced once (gdb 0xA78 = 0xA7C)
//   0xA84 == 1 / not (0x00016FB0)              | 0 natural; 1 forced (gdb, 10 Updates)
//   loop cmd: Update CanIssueCommand yes / no  | both; Deactivate run
// Run: darksamus_fsm2.py and darksamus_fsm2_force.py on 10_cliff_rooftop, exit 0.
// @ fn_10_16ECC
void CDarkSamus::SuperMissileAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("SuperMissileAttack", msg);
  UpdateCurrentAttack(0x100, mgr, msg);
  UpdateShieldFlag(msg);
  UpdateGroundCollision(mgr, msg);
  RunCollisionFailsafe(mgr);
  if (msg == kStateMsg_Activate) {
    mGuestA7C = 0;
    mGuestA78 = 0;
    mGuestA80 = 0;
    mGuestA7C = 1; // 0x00016F58
  } else if (mGuestA78 >= mGuestA7C) { // 0x00016F68
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState)); // 0x00016F90
    return;
  }
  if (mGuestA84 == 1) { // 0x00016FB0
    TurnTowardPlayer(mgr, dt, 2.7925267f);   // 0x00016FD0
    UpdateDodge(mgr, dt, 22.f, 0.5f);         // 0x00016FF0
  }
  const CBCLoopAttackCmd attack(static_cast< pas::ELoopAttackType >(0), false);
  switch (msg) { // fn_10_8870
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(attack);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(attack);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  default:
    break;
  }
}

// fn_10_17448 (0x00017448, 0x164 bytes): the NormalMissileAttack state. The attack bookkeeping
// with 0x40 (0x00017484). Activate (`cmpwi r30, 0x0` at 0x00017488) clears the shots to fire
// 0xA3C, the shots fired 0xA38 and the break byte 0xA40 and rolls 0xA3C = Range(0xA44, 0xA48)
// (the int Range at 0x000174AC; 0xA44/0xA48 are the constructor's copies of the data's
// unknown_0x1f1ef7a9 / unknown_0xc4a1a44e, the record at 0xA0C + 0x38/+0x3C); otherwise, with
// the shots fired at least 0xA3C (0x000174C0), the kBSC_ExitState command (0x000174E8) ends the
// call. Then the turn toward the player at lbl_10_rodata_504 = 1.9198622 rad/s (0x0001751C) and
// the loop attack command (fn_10_8870, pas::kAS_LoopAttack, type 1, 0x0001756C). 0xA38 and 0xA40
// (the BreakNormalMissile byte) are written by the anim-event handler fn_10_1BEFC.
//   arm                                        | run
//   Activate / not (0x00017488)                | both
//   shots fired >= to fire: exit (0x000174C0)  | forced once (gdb 0xA38 = 0xA3C)
//   loop cmd: Update CanIssueCommand yes / no  | both; Deactivate run
// Run: darksamus_fsm2.py and darksamus_fsm2_force.py on 10_cliff_rooftop, exit 0.
// @ fn_10_17448
void CDarkSamus::NormalMissileAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("NormalMissileAttack", msg);
  UpdateCurrentAttack(0x40, mgr, msg);
  if (msg == kStateMsg_Activate) {
    mGuestA3C = 0;
    mGuestA38 = 0;
    mGuestA40 = 0;
    mGuestA3C = mgr.Random()->Range(mData.mUnknown1F1EF7A9, mData.mUnknownC4A1A44E); // 0x000174AC
  } else if (mGuestA38 >= mGuestA3C) { // 0x000174C0
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState)); // 0x000174E8
    return;
  }
  TurnTowardPlayer(mgr, dt, 1.9198622f); // 0x0001751C
  const CBCLoopAttackCmd attack(static_cast< pas::ELoopAttackType >(1), false);
  switch (msg) { // fn_10_8870
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(attack);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(attack);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  default:
    break;
  }
}

// fn_10_178E4 (0x000178E4, 0x1D8 bytes): the MissileDeflect state. The attack bookkeeping with 2
// (0x00017914). Activate (0x0001793C): 0xBFC bit 0x80 = the nearest incoming projectile
// (fn_10_1BC58) closer than lbl_10_rodata_50C = 35.0f (the fcmpo's "less than" bit, 0x00017950),
// 0xC04 = 0 (0x00017978) and the melee contact reset (fn_10_EA08). Update (0x00017984): with 0xC04
// set (0x00017990) and either its time plus lbl_10_rodata_408 = 4.0f behind the elapsed time
// (0x000179A8) or a projectile closer than lbl_10_rodata_510 = 28.0f (0x000179C4), 0xC04 = 0 and
// the body controller's time scale (0x5C8) back to 1.0f (0x000179E4). Deactivate (0x000179EC):
// the time scale back to 1.0f, 0xC00 = 0 and the last attacker forgotten. Every message: the melee
// attack command (fn_10_8764) with severity 6 when 0xBFC bit 0x80 is set (0x00017A28), else 1.
void CDarkSamus::MissileDeflect(CStateManager& mgr, EStateMsg msg, float) {
  DsStateEntered("MissileDeflect", msg);
  UpdateCurrentAttack(0x2, mgr, msg);
  switch (msg) {
  case kStateMsg_Activate:
    mGuestBFC80 = StandInIncomingProjectileDistance() < 35.f; // 0x00017950
    mGuestC04 = 0.f;                                          // 0x00017978
    mMeleeContact.Reset();                                    // 0x0001797C
    break;
  case kStateMsg_Update:
    if (mGuestC04 != 0.f) { // 0x00017990
      if (mGuestC04 + 4.f < mElapsedTime ||                   // 0x000179A8
          StandInIncomingProjectileDistance() < 28.f) {        // 0x000179C4
        mGuestC04 = 0.f;
        BodyController()->SetTimeScale(1.f); // 0x000179E4
      }
    }
    break;
  case kStateMsg_Deactivate:
    BodyController()->SetTimeScale(1.f); // 0x00017A08
    mGuestC00 = 0.f;
    mLastAttackerUid = kInvalidUniqueId;
    break;
  default:
    break;
  }
  RunMeleeAttackCmd(msg, pas::kAS_MeleeAttack,
                    CBCMeleeAttackCmd(static_cast< pas::ESeverity >(mGuestBFC80 ? 6 : 1)));
}

// fn_10_18EAC (0x00018EAC, 0x88 bytes): the GetUp state. The ground collision (fn_10_18F90,
// 0x00018EC8) and the getup command (fn_10_22020, pas::kAS_Getup = `li r5, 0x1`, {kBSC_Getup,
// type 0}, 0x00018F00).
// Unmarked: never run - GetUp is not a state of the 10_cliff_rooftop FSM asset.
void CDarkSamus::GetUp(CStateManager& mgr, EStateMsg msg, float) {
  DsStateEntered("GetUp", msg);
  UpdateGroundCollision(mgr, msg);
  const CBCGetupCmd getup(static_cast< pas::EGetupType >(0));
  switch (msg) { // fn_10_22020
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(getup);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Getup)) {
      BodyController()->CommandMgr().DeliverCmd(getup);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  default:
    break;
  }
}

// The taunt command of the phazon states (fn_10_8698 with pas::kAS_Taunt).
void CDarkSamus::RunPhazonTauntCmd(EStateMsg msg, int type) {
  const CBCTauntCmd taunt(static_cast< pas::ETauntType >(type));
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(taunt);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
      BodyController()->CommandMgr().DeliverCmd(taunt);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  default:
    break;
  }
}

// fn_10_4AC8 (0x00004AC8, 0xD4 bytes): the PhazonSuck state. The attack bookkeeping with
// 0x400000 (`lis r4, 0x40`, 0x00004AF8), the shield flag (fn_10_105F4), the collision failsafe
// (fn_10_1AEC0) and the taunt command with type 3 (`li r0, 0x3`, fn_10_8698 at 0x00004B54);
// Activate (`cmpwi r31, 0x0` at 0x00004B64) keeps the actor's height (0x5C) in 0xF58.
// Unmarked: the taunt command's Update re-delivery (CanIssueCommand true) never ran (129 Updates).
void CDarkSamus::PhazonSuck(CStateManager& mgr, EStateMsg msg, float) {
  DsStateEntered("PhazonSuck", msg);
  UpdateCurrentAttack(0x400000, mgr, msg);
  UpdateShieldFlag(msg);
  RunCollisionFailsafe(mgr);
  RunPhazonTauntCmd(msg, 3);
  if (msg == kStateMsg_Activate) {
    mGuestF58 = GetTranslation().GetZ(); // 0x00004B7C
  }
}

// fn_10_6460 (0x00006460, 0xCC bytes): the GoToPhazonEnrage state. Activate (`mr. r31, r5` at
// 0x00006480) takes the phazon waypoint of type 7 (fn_10_65CC) into 0xEE4; every message stores
// lbl_10_rodata_198 = 0.1f + (lbl_10_rodata_190 = 6.0f + the elapsed time) to 0xF64
// (0x000064EC), runs the waypoint path (fn_10_6374, 0x000064F0) and the shield flag
// (fn_10_105F4, 0x00006504).
void CDarkSamus::GoToPhazonEnrage(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("GoToPhazonEnrage", msg);
  if (msg == kStateMsg_Activate) {
    mGuestEE4 = StandInPhazonWaypoint(7); // 0x000064A4
  }
  mGuestF64 = 0.1f + (6.f + mElapsedTime); // 0x000064EC
  (void)mgr;
  (void)dt;
  Fsm2StandIn("fn_10_6374", "waypoint path");
  UpdateShieldFlag(msg); // 0x00006504
}

// fn_10_652C (0x0000652C, 0xA0 bytes): the GoToPhazonAttack state. Activate takes the phazon
// waypoint of type 6 (fn_10_65CC, 0x00006570) into 0xEE4 and the elapsed time into 0xF64
// (0x00006590); every message runs the waypoint path (fn_10_6374, 0x000065A4).
void CDarkSamus::GoToPhazonAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("GoToPhazonAttack", msg);
  if (msg == kStateMsg_Activate) {
    mGuestEE4 = StandInPhazonWaypoint(6); // 0x00006570
    mGuestF64 = mElapsedTime;             // 0x00006590
  }
  (void)mgr;
  (void)dt;
  Fsm2StandIn("fn_10_6374", "waypoint path");
}

// fn_10_67B8 (0x000067B8, 0x1C4 bytes): the PhazonMissiles state. Activate (`mr. r31, r5` at
// 0x000067D8): fn_10_18628 with lbl_10_rodata_258 = 5.0f and lbl_10_rodata_14C = 1.0f
// (0x000067FC); the pattern index 0xFBC steps and wraps at 21 (0x00006810); the pattern
// lbl_10_rodata_E8[0xFBC] picks the volley: 0 makes 0xFB8 = 0xFB4 = 1 (0x00006840), else
// 0xFB8 = 0 and 0xFB4 = Range(9, 14) (0x00006860); the volley count 0x105C steps (0x00006870) and
// with 0xFB8 clear (0x0000687C) 0xFC8 = the elapsed time, 0xFCC = lbl_10_rodata_140 = -1000.0f and
// fn_10_69C0 runs (0x000068A0). Every message: fn_10_7F98 (0x000068B4) and the melee attack
// command with severity 7 when 0xFB8 is clear, else 4 (0x000068C4, fn_10_8764 at 0x00006924).
void CDarkSamus::PhazonMissiles(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("PhazonMissiles", msg);
  if (msg == kStateMsg_Activate) {
    static const int kPattern[21] = {0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1,
                                     0, 1, 0, 0, 0, 0, 1, 0, 1, 0}; // lbl_10_rodata_E8
    SetGuestE04(5.f, 1.f); // 0x000067FC
    ++mGuestFBC;
    if (mGuestFBC >= 21) { // 0x00006810
      mGuestFBC = 0;
    }
    if (kPattern[mGuestFBC] == 0) { // 0x00006834
      mGuestFB8 = 1;
      mGuestFB4 = 1;
    } else {
      mGuestFB8 = 0;
      mGuestFB4 = mgr.Random()->Range(9, 14); // 0x00006860
    }
    ++mGuest105C;          // 0x00006870
    if (mGuestFB8 == 0) {  // 0x0000687C
      mGuestFC8 = mElapsedTime;
      mGuestFCC = -1000.f;
      Fsm2StandIn("fn_10_69C0", "phazon missiles reset");
    }
  }
  (void)dt;
  Fsm2StandIn("fn_10_7F98", "phazon enrage update");
  RunMeleeAttackCmd(msg, pas::kAS_MeleeAttack,
                    CBCMeleeAttackCmd(static_cast< pas::ESeverity >(mGuestFB8 == 0 ? 7 : 4)));
}

// fn_10_6BA0 (0x00006BA0, 0x208 bytes): the PhazonFall state. The collision failsafe, the ground
// collision, the attack bookkeeping with 0x800000 (0x00006BF4), fn_10_746C and fn_10_7610; the
// phazon blend 0xFC0 loses dt / lbl_10_rodata_158 = 1.2f, clamped to [0, 1] (0x00006C2C..
// 0x00006C58). Activate (0x00006C7C): locomotion type 1, the phazon phase bit (0x1060 bit 0x80) set
// and the shield byte 0xCD0 cleared. Update (0x00006CA4): the shield byte set once the model's
// z scale is at most 0xFAC + lbl_10_rodata_1A8 = 0.01f (0x00006CD4). Deactivate (0x00006CE8): the
// shield byte and 0xFC0 cleared, the health (HealthInfo's +0x4) into 0x1054, 0xC00 = 0 and
// fn_10_4868. Every message: the taunt command with type 8 (fn_10_8698 at 0x00006D6C).
void CDarkSamus::PhazonFall(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("PhazonFall", msg);
  RunCollisionFailsafe(mgr);
  UpdateGroundCollision(mgr, msg);
  UpdateCurrentAttack(0x800000, mgr, msg);
  Fsm2StandIn("fn_10_746C", "phazon scale");
  Fsm2StandIn("fn_10_7610", "phazon blend");
  float blend = mGuestFC0 - dt / 1.2f; // 0x00006C2C
  if (blend < 0.f) {
    blend = 0.f;
  } else if (blend > 1.f) {
    blend = 1.f;
  }
  mGuestFC0 = blend; // 0x00006C58
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Relaxed); // 0x00006C84
    mPhazonPhase = true;                                     // 0x00006C98
    mGuestCD0 = 0;
    break;
  case kStateMsg_Update:
    if (GetModelData()->GetScale().GetZ() <= mGuestFAC + 0.01f) { // 0x00006CD4
      mGuestCD0 = 1;
    }
    break;
  case kStateMsg_Deactivate:
    mGuestCD0 = 0;
    mGuestFC0 = 0.f;
    mPhazonHealthMark = HealthInfo()->GetHP(); // 0x00006D20
    mGuestC00 = 0.f;
    Fsm2StandIn("fn_10_4868", "phazon fall end");
    break;
  default:
    break;
  }
  RunPhazonTauntCmd(msg, 8);
}

// fn_10_7054 (0x00007054, 0x2B0 bytes): the PhazonRise state. The collision failsafe, the ground
// collision, the shield flag (fn_10_105F4 with the zero vector, 0x000070A8) and the attack
// bookkeeping with 0x800000. Activate (0x000070E4): 0xFBC = Range(0, 20); the base bounding box
// kept once in 0x102C (0x1044 bit 0x80, 0x000070FC); the phazon phase bit cleared (0x00007154),
// 0xFB8 = 1, 0x105C = 0 and 0x1058 = Range(6, 8) (0x00007174). Update (0x00007180): toward the
// phazon waypoint of type 7 (fn_10_65CC), unless it is the zero vector (0x000071D4): the flat
// direction, normalized when it can be (0x00007218), times dt, is the one-frame move
// (MoveInOneFrameWR at 0x00007278, the host's MoveToInOneFrameWR). Deactivate (0x00007280): 0xFC0
// cleared. Every message: the taunt command with type 7 (0x000072C4).
void CDarkSamus::PhazonRise(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("PhazonRise", msg);
  RunCollisionFailsafe(mgr);
  UpdateGroundCollision(mgr, msg);
  UpdateShieldFlag(msg);
  UpdateCurrentAttack(0x800000, mgr, msg);
  switch (msg) {
  case kStateMsg_Activate:
    mGuestFBC = mgr.Random()->Range(0, 20); // 0x000070F0
    if (!mGuest1044) {                      // 0x000070FC
      mGuest102C = GetBaseBoundingBox();
      mGuest1044 = true;
    }
    mPhazonPhase = false; // 0x00007154
    mGuestFB8 = 1;
    mGuest105C = 0;
    mGuest1058 = mgr.Random()->Range(6, 8); // 0x00007174
    break;
  case kStateMsg_Update: {
    const CVector3f waypoint = StandInPhazonWaypoint(7);
    if (waypoint == CVector3f::Zero()) { // 0x000071D4
      break;
    }
    CVector3f dir = (waypoint - GetTranslation()).DropZ();
    if (dir.CanBeNormalized()) { // 0x00007218
      dir.Normalize();
      MoveToInOneFrameWR(GetTranslation() + dt * dir, dt); // 0x00007278
    }
    break;
  }
  case kStateMsg_Deactivate:
    mGuestFC0 = 0.f; // 0x00007288
    break;
  default:
    break;
  }
  RunPhazonTauntCmd(msg, 7);
}

// fn_10_7BC0 (0x00007BC0, 0x154 bytes): the PhazonEnraged state. Locomotion type 0xA every message
// (0x00007BF8). Activate: 0xC00 = lbl_10_rodata_1D4 = 10.0f, 0xFB0 = Range(lbl_10_rodata_1CC = 0.2,
// lbl_10_rodata_1D0 = 4.5) (0x00007C34), the health into 0xFC4 (0x00007C54) and fn_10_786C. Update
// (0x00007C60): 0xFB0 loses dt and the blend 0xFC0 gains dt / lbl_10_rodata_1B8 = 5.0f, clamped
// to [0, 1] (0x00007CB8). Every message: fn_10_7F98 (0x00007CCC) and the locomotion commands
// cleared (0x00007CD8).
void CDarkSamus::PhazonEnraged(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("PhazonEnraged", msg);
  BodyController()->SetLocomotionType(static_cast< pas::ELocomotionType >(0xA)); // 0x00007BF8
  if (msg == kStateMsg_Activate) {
    mGuestC00 = 10.f;
    mGuestFB0 = mgr.Random()->Range(0.2f, 4.5f); // 0x00007C30
    mGuestFC4 = HealthInfo()->GetHP();           // 0x00007C54
    Fsm2StandIn("fn_10_786C", "phazon enrage start");
  } else if (msg == kStateMsg_Update) {
    mGuestFB0 -= dt;
    float blend = mGuestFC0 + dt / 5.f; // 0x00007C90
    if (blend < 0.f) {
      blend = 0.f;
    } else if (blend > 1.f) {
      blend = 1.f;
    }
    mGuestFC0 = blend; // 0x00007CB8
  }
  Fsm2StandIn("fn_10_7F98", "phazon enrage update");
  BodyController()->CommandMgr().ClearLocomotionCmds(); // 0x00007CD8
}

// fn_10_81D4 (0x000081D4, 0x4AC bytes): the PhazonAttack state. The attack bookkeeping with
// 0x400000, the collision failsafe and the ground collision; off Activate with 0xF5C bit 0x40 set
// (0x00008248) the shield byte 0xCD0 is cleared, else the shield flag runs (fn_10_105F4).
// Activate (0x00008294): fn_10_452C on 0xEF0, 0xF4C = Range(2, 3), 0xF14 = the gun locator's
// forward (0x8D0/0x8E0/0x8F0) and 0xF10 = lbl_10_rodata_1AC = 0.5f * (lbl_10_rodata_1D4 = 10.0f +
// the flat distance to the player) (0x0000832C). Update (0x00008334): fn_10_5974 and fn_10_48C8;
// the flat forward and the flat direction to the player, both normalizable (0x00008388,
// 0x000083E8), turn the actor (LookAt, RotateInOneFrameOR at 0x000084C4) by lbl_10_rodata_1EC
// (1 degree) * dt * lbl_10_rodata_1E0 = 35, doubled (lbl_10_rodata_1E4) past lbl_10_rodata_1E8 =
// 0.87266463 rad (0x00008430); the emitter 0xF60, when there is one and 0xF20 is not the zero
// vector (0x00008514), moves to 0xF20 (0x00008534); with the shots 0xF48 at least 0xF4C
// (0x00008548) the kBSC_ExitState command (0x0000856C) ends the call. Deactivate (0x00008588):
// fn_10_812C. Then the loop attack command with type 3 (fn_10_8870 at 0x000085DC).
void CDarkSamus::PhazonAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("PhazonAttack", msg);
  UpdateCurrentAttack(0x400000, mgr, msg);
  RunCollisionFailsafe(mgr);
  UpdateGroundCollision(mgr, msg);
  if (msg != kStateMsg_Activate && mGuestF5C40) { // 0x00008248
    mGuestCD0 = 0;
  } else {
    UpdateShieldFlag(msg); // 0x0000826C
  }
  switch (msg) {
  case kStateMsg_Activate: {
    Fsm2StandIn("fn_10_452C", "phazon attack reset");
    mGuestF4C = mgr.Random()->Range(2, 3); // 0x000082A8
    mGuestF14 = mGunLctrXf.GetForward();   // 0x000082CC..0x000082D4
    const CVector3f toPlayer =
        (mgr.GetPlayer(0)->GetTranslation() - GetTranslation()).DropZ();
    mGuestF10 = 0.5f * (10.f + toPlayer.Magnitude()); // 0x0000832C
    break;
  }
  case kStateMsg_Update: {
    Fsm2StandIn("fn_10_5974", "phazon attack update");
    Fsm2StandIn("fn_10_48C8", "phazon attack aim");
    const CVector3f fwd = GetTransform().GetForward();
    CVector3f flatFwd(fwd.GetX(), fwd.GetY(), 0.f);
    if (flatFwd.CanBeNormalized()) { // 0x00008388
      flatFwd.Normalize();
      CVector3f toPlayer = (mgr.GetPlayer(0)->GetTranslation() - GetTranslation()).DropZ();
      if (toPlayer.CanBeNormalized()) { // 0x000083E8
        toPlayer.Normalize();
        float rate = 35.f;
        if (CVector3f::GetAngleDiff(fwd, toPlayer) > 0.87266463f) { // 0x00008430
          rate *= 2.f;
        }
        const CQuaternion q = CQuaternion::LookAt(
            CUnitVector3f(flatFwd.GetX(), flatFwd.GetY(), flatFwd.GetZ()),
            CUnitVector3f(toPlayer.GetX(), toPlayer.GetY(), toPlayer.GetZ()),
            CRelAngle::FromRadians(0.017453292f * (dt * rate)));
        RotateInOneFrameOR(q, dt); // 0x000084C4
      }
    }
    if (mGuestF60 && mGuestF20 != CVector3f::Zero()) { // 0x000084D0 / 0x00008514
      CSfxManager::UpdateEmitter(mGuestF60, mGuestF20, CVector3f::Up(), 0x7F); // 0x00008534
    }
    if (static_cast< int >(mGuestF48) >= mGuestF4C) { // 0x00008548
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState)); // 0x0000856C
      return;
    }
    break;
  }
  case kStateMsg_Deactivate:
    Fsm2StandIn("fn_10_812C", "phazon attack end");
    break;
  default:
    break;
  }
  const CBCLoopAttackCmd attack(static_cast< pas::ELoopAttackType >(3), false);
  switch (msg) { // fn_10_8870
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(attack);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(attack);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  default:
    break;
  }
}
