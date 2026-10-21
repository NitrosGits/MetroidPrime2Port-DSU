// The Dark Samus's state machine: the 49 triggers and the 43 states the REL hands to the machine,
// and the bodies behind them.
//
// fn_10_18D78 is the module's own setup (called from the first Think, fn_10_1B1FC, when the
// machine has no state yet): it calls CPatterned's InitializeStateMachine and then hands the
// machine two tables - lbl_10_data_450 with 49 entries (the triggers) and lbl_10_data_964 with 43
// entries (the states). The names in the tables are the module's own strings (the .rodata at
// lbl_10_rodata_579 and lbl_10_rodata_871) and the machine finds a node's handler by name.
//
// Which REL function implements which name: every entry has a 12-byte record beside it (the 49 at
// lbl_10_data_204, the 43 at lbl_10_data_760) and the .ctors initialiser fn_10_221C0 copies the
// record into the entry (entry+4 = record[0], entry+8 = record[1], entry+12 = record[2]). The
// record's second word is the byte offset of the handler inside lbl_10_data_CBC - the blob that
// holds the actor's vtable (words 0..81) and then the 92 handlers (words 82..173): the triggers
// take the words 82..130 and the states 131..173. That is what tells the two halves apart - the
// first 49 handlers return a bool and take (mgr, float), the last 43 return nothing and take
// (mgr, EStateMsg, float) - and it lines up with the two tables' names.
//
// The bodies below are stand-ins: each logs the first time it runs and does nothing else. The
// tables, the names and the name-to-function mapping are retail's; the behaviour is not yet.
#include "MetroidPrime/Enemies/CDarkSamus.hpp"

#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CGenericFSM2.hpp"
#include "MetroidPrime/Enemies/CDarkSamusBattleStage.hpp"
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIHint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptRepulsor.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/PathFinding/CPathFindRegion.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CPASDatabase.hpp"
#include "MetroidPrime/StateMachineCommon.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Collision/CRayCastResult.hpp"

#include <stdio.h>
#include <stdlib.h>

#include "types.h"

namespace {

typedef CPatterned::StateMachine::TriggerFunc TriggerFunc;
typedef CPatterned::StateMachine::StateFunc StateFunc;
typedef CPatterned::StateMachine::STriggerFunction STriggerFunction;
typedef CPatterned::StateMachine::SStateFunction SStateFunction;

#define DS_TRIGGER(name) {#name, static_cast< TriggerFunc >(&CDarkSamus::name)}
#define DS_STATE(name) {#name, static_cast< StateFunc >(&CDarkSamus::name)}

// The once-only log the milestone-1 body uses: one line per retail function, the first time it
// runs, so a log lists what is still a shell.
void FsmStandIn(const char* fn, const char* what) {
  static const char* sSeen[128] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == fn) {
      return;
    }
  }
  if (sCount < 128) {
    sSeen[sCount++] = fn;
  }
  fprintf(stderr, "[darksamus] fsm stand-in %s (%s)\n", fn, what);
}

// fn_10_18D78's trigger table (lbl_10_data_450), in the REL's order. The comment is the REL's
// handler, read out of the record beside the entry.
const STriggerFunction skTriggers[] = {

    DS_TRIGGER(Alerted), // fn_10_F840
    DS_TRIGGER(Arrived), // fn_10_62B4
    DS_TRIGGER(AnyIncomingMissiles), // fn_10_1BB08
    DS_TRIGGER(BreakFreezeBeam), // fn_10_16B48
    DS_TRIGGER(BreakDash), // fn_10_149B4
    DS_TRIGGER(BreakDiveAttack), // fn_10_11C54
    DS_TRIGGER(BreakNormalMissile), // fn_10_16B5C
    DS_TRIGGER(BreakPause), // fn_10_106F8
    DS_TRIGGER(BreakPhazonAttack), // fn_10_4ABC
    DS_TRIGGER(BreakSuperMissile), // fn_10_16B54
    DS_TRIGGER(CanMeleeAttack), // fn_10_10634
    DS_TRIGGER(CanMissileDeflect), // fn_10_14284
    DS_TRIGGER(DiveAfterBoost), // fn_10_95A8
    DS_TRIGGER(DoneRolling), // fn_10_9314
    DS_TRIGGER(DoneTurning), // fn_10_F864
    DS_TRIGGER(FacingPlayer), // fn_10_142CC
    DS_TRIGGER(HitDuringEnrage), // fn_10_7B68
    DS_TRIGGER(OutOfMeleeRange), // fn_10_17CB8
    DS_TRIGGER(PickedBoostBall), // fn_10_116F8
    DS_TRIGGER(PickedFreezeBeam), // fn_10_11760
    DS_TRIGGER(PickedDiveAttack), // fn_10_1174C
    DS_TRIGGER(PickedMelee), // fn_10_117D8
    DS_TRIGGER(PickedMeleeDash), // fn_10_11710
    DS_TRIGGER(PickedMissileJump), // fn_10_1178C
    DS_TRIGGER(PickedMovingSShot), // fn_10_11724
    DS_TRIGGER(PickedNormalMissile), // fn_10_117A0
    DS_TRIGGER(PickedPause), // fn_10_11800
    DS_TRIGGER(PickedPhazonAttack), // fn_10_8680
    DS_TRIGGER(PickedPhazonEnrage), // fn_10_8114
    DS_TRIGGER(PickedScatterShot), // fn_10_11738
    DS_TRIGGER(PickedSuperMissile), // fn_10_11778
    DS_TRIGGER(PickedSweepBeam), // fn_10_CDA0
    DS_TRIGGER(PickedTaunt), // fn_10_117EC
    DS_TRIGGER(PlayerIsMorphball), // fn_10_112E8
    DS_TRIGGER(PlayerTooClose), // fn_10_10658
    DS_TRIGGER(ReachedPissedOff), // fn_10_FAF4
    DS_TRIGGER(ReadyForMissiles), // fn_10_6A40
    DS_TRIGGER(ShouldEnragedMelee), // fn_10_6B78
    DS_TRIGGER(ShouldFadeIn), // fn_10_BB78
    DS_TRIGGER(ShouldFadeOut), // fn_10_BBE4
    DS_TRIGGER(ShouldGetPissedOff), // fn_10_FD30
    DS_TRIGGER(ShouldLeaveSphere), // fn_10_6A60
    DS_TRIGGER(ShouldReact), // fn_10_11158
    DS_TRIGGER(ShouldRiseAgain), // fn_10_6AE8
    DS_TRIGGER(SkipPhazonAttack), // fn_10_110E0
    DS_TRIGGER(StartsInTheAir), // fn_10_F924
    DS_TRIGGER(TooMuchDamage), // fn_10_10698
    DS_TRIGGER(WallCollision), // fn_10_F800
    DS_TRIGGER(WasHeavyHit), // fn_10_112AC
};

// fn_10_18D78's state table (lbl_10_data_964), in the REL's order.
const SStateFunction skStates[] = {
    DS_STATE(BoostBallFire), // fn_10_9724
    DS_STATE(BoostBallInto), // fn_10_B164
    DS_STATE(BoostBallRoll), // fn_10_A3B0
    DS_STATE(FreezeBeam), // fn_10_16CDC
    DS_STATE(Dash), // fn_10_150DC
    DS_STATE(DiveAttack), // fn_10_11FA4
    DS_STATE(DiveFromBoost), // fn_10_9424
    DS_STATE(EnrageHitReact), // fn_10_7A14
    DS_STATE(EnragedTravel), // fn_10_6E38
    DS_STATE(FadeIn), // fn_10_BD40
    DS_STATE(FadeOut), // fn_10_BC54
    DS_STATE(FallFromTheAir), // fn_10_F86C
    DS_STATE(GetUp), // fn_10_18EAC
    DS_STATE(GoToPhazonAttack), // fn_10_652C
    DS_STATE(GoToPhazonEnrage), // fn_10_6460
    DS_STATE(HeavyHit), // fn_10_10C9C
    DS_STATE(Interrupt), // fn_10_10FF4
    DS_STATE(MeleeAttack), // fn_10_17BC0
    DS_STATE(MeleeDash), // fn_10_17ABC
    DS_STATE(MissileDeflect), // fn_10_178E4
    DS_STATE(MissileJump), // fn_10_177BC
    DS_STATE(MovingScatterShot), // fn_10_11AD8
    DS_STATE(NormalMissileAttack), // fn_10_17448
    DS_STATE(Null), // fn_10_14448
    DS_STATE(Dead), // fn_10_143A8
    DS_STATE(Patrol), // fn_10_1142C
    DS_STATE(Pause), // fn_10_10710
    DS_STATE(PhazonAttack), // fn_10_81D4
    DS_STATE(PhazonEnraged), // fn_10_7BC0
    DS_STATE(PhazonFall), // fn_10_6BA0
    DS_STATE(PhazonMissiles), // fn_10_67B8
    DS_STATE(PhazonRise), // fn_10_7054
    DS_STATE(PhazonSuck), // fn_10_4AC8
    DS_STATE(PissedOff), // fn_10_F930
    DS_STATE(ScatterShot), // fn_10_175AC
    DS_STATE(SideDash), // fn_10_153EC
    DS_STATE(Start), // fn_10_F920
    DS_STATE(SuperMissileAttack), // fn_10_16ECC
    DS_STATE(SweepBeam), // fn_10_CA0C
    DS_STATE(Taunt), // fn_10_11544
    DS_STATE(TravelToCoverPoints), // fn_10_1DF74
    DS_STATE(TravelToPissedOff), // fn_10_FB8C
    DS_STATE(TurnToFacePlayer), // fn_10_FE18
};

// ---- the FSM2 probe (MP2_DS_FSM) --------------------------------------------------------------
//
// The transitions are not in the REL's .data - they come from the actor's 'FSM2' asset - so the
// machine is what tells which trigger leads to which state. The probe has the shape of the Boost
// Guardian's MP2_BG_FSM probe: one line per node and per transition, once per process.

// The node a transition points at.
struct SDsFsmNode {
  const char* mKind; // "state", "trigger", "code", "submachine" or "none".
  int mIndex;        // Index in the machine's own list, -1 when the target is not in it.
  const char* mName; // The node's name.
};

// Finds the list the target belongs to (by its type) and then the element the target points at.
SDsFsmNode DsFsmNodeOf(const CGenericFSM2& machine, const CState2* target) {
  SDsFsmNode node = { "none", -1, "" };
  if (target == nullptr) {
    return node;
  }
  switch (target->GetType()) {
  case CState2::kType_State: {
    const rstl::vector< CState2State >& states = machine.GetStates();
    for (int i = 0; i < states.size(); ++i) {
      if (static_cast< const CState2* >( &states[i] ) == target) {
        node.mKind = "state";
        node.mIndex = i;
        node.mName = states[i].GetName().data();
        return node;
      }
    }
    break;
  }
  case CState2::kType_Trigger: {
    const rstl::vector< CState2Trigger >& triggers = machine.GetTriggers();
    for (int i = 0; i < triggers.size(); ++i) {
      if (static_cast< const CState2* >( &triggers[i] ) == target) {
        node.mKind = "trigger";
        node.mIndex = i;
        node.mName = triggers[i].GetName().data();
        return node;
      }
    }
    break;
  }
  case CState2::kType_Code: {
    const rstl::vector< CState2Code >& codes = machine.GetCodes();
    for (int i = 0; i < codes.size(); ++i) {
      if (static_cast< const CState2* >( &codes[i] ) == target) {
        node.mKind = "code";
        node.mIndex = i;
        node.mName = codes[i].GetName().data();
        return node;
      }
    }
    break;
  }
  case CState2::kType_SubMachine: {
    const rstl::vector< CState2SubMachine >& subMachines = machine.GetSubMachines();
    for (int i = 0; i < subMachines.size(); ++i) {
      if (static_cast< const CState2* >( &subMachines[i] ) == target) {
        node.mKind = "submachine";
        node.mIndex = i;
        node.mName = subMachines[i].GetName().data();
        return node;
      }
    }
    break;
  }
  }
  return node;
}

// The state a node's own transitions lead to (the first one that is a state).
const char* DsFsmStateOf(const CState2* node) {
  if (node == nullptr) {
    return "none";
  }
  const rstl::vector< SStateMachine2Transition >* transitions = nullptr;
  switch (node->GetType()) {
  case CState2::kType_Trigger:
    transitions = &static_cast< const CState2Trigger* >( node )->GetTransitions();
    break;
  case CState2::kType_Code:
    transitions = &static_cast< const CState2Code* >( node )->GetTransitions();
    break;
  default:
    break;
  }
  if (transitions != nullptr) {
    for (int i = 0; i < transitions->size(); ++i) {
      const CState2* target = (*transitions)[i].mTarget;
      if (target != nullptr && target->GetType() == CState2::kType_State) {
        return static_cast< const CState2State* >( target )->GetName().data();
      }
    }
  }
  return "none";
}

// One line per transition of a node: the trigger it waits for (with the parameter the trigger
// carries) and the state that trigger leads to.
void DsDumpFsmTransitions(const CGenericFSM2& machine,
                          const rstl::vector< SStateMachine2Transition >& transitions,
                          const char* indent) {
  for (int i = 0; i < transitions.size(); ++i) {
    const SStateMachine2Transition& transition = transitions[i];
    const CState2* target = transition.mTarget;
    if (target == nullptr) {
      fprintf(stderr, "[dsfsm] %s-> none\n", indent);
      continue;
    }
    if (target->GetType() == CState2::kType_Trigger) {
      const CState2Trigger* trigger = static_cast< const CState2Trigger* >( target );
      fprintf(stderr, "[dsfsm] %strigger %s (parm=%f negate=%d) -> state %s\n", indent,
              trigger->GetName().data(), trigger->GetArgument().GetFloat(),
              trigger->IsNegated() ? 1 : 0, DsFsmStateOf(target));
    } else {
      const SDsFsmNode node = DsFsmNodeOf(machine, target);
      fprintf(stderr, "[dsfsm] %s%s %s\n", indent, node.mKind, node.mName);
      if (node.mKind[0] == 'c' || node.mKind[0] == 's') {
        // A code or a sub-machine carries its own transitions: the state it leads to is in them.
        DsDumpFsmTransitions(machine,
                             node.mKind[0] == 'c'
                                 ? static_cast< const CState2Code* >( target )->GetTransitions()
                                 : static_cast< const CState2SubMachine* >( target )->GetTransitions(),
                             indent);
      }
    }
  }
}

// One line per node and one line per transition, for every list of the machine. A sub-machine
// that carries its own machine is dumped one level deeper.
void DsDumpFsmMachine(const CGenericFSM2& machine, const char* indent, const char* childIndent) {
  fprintf(stderr, "[dsfsm] %smachine: %d states, %d triggers, %d codes, %d sub-machines\n", indent,
          machine.GetStates().size(), machine.GetTriggers().size(), machine.GetCodes().size(),
          machine.GetSubMachines().size());

  const rstl::vector< CState2State >& states = machine.GetStates();
  for (int i = 0; i < states.size(); ++i) {
    fprintf(stderr, "[dsfsm] %sstate %d %s\n", indent, i, states[i].GetName().data());
    DsDumpFsmTransitions(machine, states[i].GetTransitions(), childIndent);
  }

  const rstl::vector< CState2Trigger >& triggers = machine.GetTriggers();
  for (int i = 0; i < triggers.size(); ++i) {
    fprintf(stderr, "[dsfsm] %strigger %d %s (parm=%f negate=%d)\n", indent, i,
            triggers[i].GetName().data(), triggers[i].GetArgument().GetFloat(),
            triggers[i].IsNegated() ? 1 : 0);
  }

  const rstl::vector< CState2Code >& codes = machine.GetCodes();
  for (int i = 0; i < codes.size(); ++i) {
    fprintf(stderr, "[dsfsm] %scode %d %s\n", indent, i, codes[i].GetName().data());
    DsDumpFsmTransitions(machine, codes[i].GetTransitions(), childIndent);
  }

  const rstl::vector< CState2SubMachine >& subMachines = machine.GetSubMachines();
  for (int i = 0; i < subMachines.size(); ++i) {
    fprintf(stderr, "[dsfsm] %ssubmachine %d %s\n", indent, i, subMachines[i].GetName().data());
    DsDumpFsmTransitions(machine, subMachines[i].GetTransitions(), childIndent);
    const CGenericFSM2* sub = subMachines[i].GetMachine();
    if (sub != nullptr) {
      fprintf(stderr, "[dsfsm] %ssubmachine %d machine:\n", indent, i);
      DsDumpFsmMachine(*sub, "    ", "      ");
    }
  }
}

// Dumps the machine once per process. The machine is the actor's 'FSM2' asset; its nodes carry
// the names the REL's own tables (fn_10_18D78) use, so the dump gives the machine's shape and
// the names the module looks its handlers up by.
void DsDumpFsmOnce(CDarkSamus* darksamus) {
  static bool sDumped = false;
  if (sDumped) {
    return;
  }
  sDumped = true;
  const CGenericFSM2* machine = darksamus->GetStateMachine2();
  if (machine == nullptr) {
    fprintf(stderr, "[dsfsm] no FSM2 machine\n");
    return;
  }
  DsDumpFsmMachine(*machine, "", "  ");
}

// The state change itself: the machine sends the state its body the Activate message when it
// enters it.
void DsStateEntered(const char* name, EStateMsg msg) {
  if (msg == kStateMsg_Activate && getenv("MP2_DS_FSM") != nullptr) {
    fprintf(stderr, "[dsfsm] state change: %s\n", name);
  }
}

} // namespace

// The two tables go into the machine under the REL's own names, which is how the machine finds the
// handler for a node: the FSM2 asset's node names are the REL's names.
void CDarkSamus::InstallStateMachineFunctions() {
  StateMachine* stateMachine = mStateMachine.get();
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));

  // The transitions are only in the actor's 'FSM2' asset, so the probe (MP2_DS_FSM) dumps the
  // machine the asset holds: every state with the trigger of each of its transitions, the
  // parameter that trigger carries and the state it leads to. Once per process.
  if (getenv("MP2_DS_FSM") != nullptr) {
    DsDumpFsmOnce(this);
  }
}

// fn_10_142F4: is the player within `angle` of the actor's facing axis? The direction to the
// player and the forward axis (the guest's 0x28/0x38/0x48 = the transform's forward column) are
// flattened into the XY plane (the two zero stores at 0x00014364/0x00014368), normalized (the two
// calls at 0x0001436C/0x00014374) and compared (the fcmpo at 0x00014384; the cr bit 31 at
// 0x0001438C is the "less than" of the compare, so the limit is strict).
bool CDarkSamus::IsPlayerInFront(CStateManager& mgr, float angle) const {
  const CVector3f toPlayer = mgr.GetPlayer(0)->GetTranslation() - GetTranslation();
  const CVector3f forward = GetTransform().GetForward();
  const CVector3f flatToPlayer = toPlayer.DropZ().AsNormalized();
  const CVector3f flatForward = CVector3f(forward.GetX(), forward.GetY(), 0.f).AsNormalized();
  return CVector3f::GetAngleDiff(flatForward, flatToPlayer) < angle;
}

// fn_10_17E14: the fight script's object - the one with the uid at the guest's 0x974 - and
// module 11's accessor on it (fn_11_B70, which returns the object's 0x24, the stage info).
// fn_10_17DE0 looks the uid up in the state manager (GetObjectById at 0x00017DFC) and casts the
// result to CScriptDarkSamusBattleStage (the cast at 0x00017E00); a null object (no script) ends
// the helper with a null (the cmplwi at 0x00017E24). The uid is written by AcceptScriptMsg's
// 'XALD' arm (0x0001EBB8, the first stage) and by fn_10_193A0 (0x00019410, the next stage);
// until the area is loaded the lookup finds nothing and the triggers below answer false.
const CDarkSamusBattleStage* CDarkSamus::GetBattleStage(CStateManager& mgr) const {
  return TCastToConstPtr< CDarkSamusBattleStage >(mgr.GetObjectById(mBattleStageUid));
}

// fn_10_1D794 (0x0001D794, 0xC8 bytes): the still timer (the guest's 0x964) and the position it
// measures from (the guest's 0x958/0x95C/0x960).
//
// On kStateMsg_Activate (the guest's argument r4 against 0, 0x0001D7B0) the timer is cleared and
// the reference position becomes the actor's own position (the base class' position, the guest's
// 0x54/0x58/0x5C): 0x0001D810..0x0001D834. On any other message the distance between the actor's
// position and the reference position is taken (the three fsubs at 0x0001D7D0..0x0001D7E4 and
// Magnitude at 0x0001D7FC): when it is more than 0.6 (lbl_10_rodata_424, the ble at 0x0001D80C) the
// actor has walked away, so the timer is cleared and the reference position becomes the actor's
// position again (0x0001D810); when it is not, the timer grows by dt (0x0001D838).
void CDarkSamus::UpdateStillTimer(EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    mStillTime = 0.f;
    mStillPosition = GetTranslation();
    return;
  }
  const CVector3f delta = GetTranslation() - mStillPosition;
  if (delta.Magnitude() <= 0.6f) {
    mStillTime += dt;
    return;
  }
  mStillTime = 0.f;
  mStillPosition = GetTranslation();
}

// fn_10_1C778 (0x0001C778, 0xDC bytes): the destination of the path find (the guest's
// 0x8B4/0x8B8/0x8BC) and the path find itself.
//
// The vector is written into the three floats (the three stfs at 0x0001C7A4..0x0001C7BC), then
// the base class' path find navigation (the guest's 0x6F4) gets the destination (the
// SetDestination call at 0x0001C7C0) and is told to find a path (the PathFind call at
// 0x0001C7D8) with the message kStateMsg_Activate (the `li r5, 0x0` at 0x0001C7D4) and the
// float argument.
void CDarkSamus::SetPathFindDestination(CStateManager& mgr, const CVector3f& destination,
                                        float dt) {
  mPathFindDestination = destination;
  mPathFindNavigation.SetDestination(destination);
  mPathFindNavigation.PathFind(mgr, kStateMsg_Activate, dt, *this);
}

// fn_10_1E26C (0x0001E26C, 0xDC bytes): the uid of the attacker that hit the actor last (the
// guest's 0xC24) and the two uids before it (the count at 0xC28, the uids at 0xC2C).
//
// The uid is stored (0x0001E288) and an invalid one ends the function (the cmplw against
// kInvalidUniqueId at 0x0001E294). When there are two uids already the oldest is dropped
// (fn_10_1E2E8 at 0x0001E2B0: it shifts the rest one down and counts one less), and then the uid
// is appended (0x0001E2C4) and the count grows (0x0001E2D0).
void CDarkSamus::PushAttacker(TUniqueId uid) {
  mLastAttackerUid = uid;
  if (mLastAttackerUid == kInvalidUniqueId) {
    return;
  }
  if (mAttackerCount >= 2) {
    EraseFirstAttacker();
  }
  mAttackerUids[mAttackerCount] = mLastAttackerUid;
  mAttackerCount++;
}

// fn_10_1E2E8 (0x0001E2E8, 0xDC bytes): the oldest uid is dropped and the rest move one down -
// the guest's loop copies every uid one slot down (the lhz/sth at 0x0001E318/0x0001E31C) and the
// count grows one less (0x0001E33C/0x0001E344).
void CDarkSamus::EraseFirstAttacker() {
  mAttackerUids[0] = mAttackerUids[1];
  mAttackerCount--;
}

// fn_10_1E360 (0x0001E360, 0x60 bytes): the point on the player the cover points are scored
// against - the player's aim position (the state manager's 0x14FC and its vtable slot 0x54) a
// lbl_10_rodata_55C = 0.65f ahead.
// @ fn_10_1E360
CVector3f CDarkSamus::GetPlayerAimPosition(CStateManager& mgr) const {
  return mgr.GetPlayer(0)->GetAimPosition(mgr, 0.65f);
}

// fn_10_1E3C0 (0x0001E3C0, 0x104 bytes): the best cover point to travel to.
//
// The AI waypoint list (the state manager's 0x838, its first index at 0x2008) is walked
// (0x0001E41C..0x0001E4A0): each cover point (the TCastToPtr at 0x0001E428) that is active (the
// byte at 0x20's bit 7, 0x0001E434) and in the actor's area (0x0001E458) is scored against the
// player's aim position (fn_10_1E360 at 0x0001E3F8, fn_10_1E4C4 at 0x0001E46C); the best score
// above lbl_10_rodata_410 = -1000.0f (the fcmpo at 0x0001E470, strict) wins. No cover point
// answers kInvalidUniqueId (the lhz at 0x0001E3E8).
TUniqueId CDarkSamus::FindBestCoverPoint(CStateManager& mgr) const {
  TUniqueId best = kInvalidUniqueId;
  float bestScore = -1000.f;
  const CVector3f target = GetPlayerAimPosition(mgr);
  CObjectList& list = mgr.ObjectListById(kOL_AiWaypoint);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CScriptCoverPoint* cover = TCastToPtr< CScriptCoverPoint >(list[i]);
    if (cover == nullptr || cover->GetActive() != true ||
        cover->GetCurrentAreaId() != GetCurrentAreaId()) {
      continue;
    }
    const float score = ScoreCoverPoint(*cover, target);
    if (score > bestScore) { // 0x0001E470
      bestScore = score;
      best = cover->GetUniqueId();
    }
  }
  return best;
}

// fn_10_1E4C4 (0x0001E4C4, 0x27C bytes): how good a cover point is against `target`.
//
// The score starts at lbl_10_rodata_404 = 10.0f and loses 10 for each recent attacker uid (the
// guest's 0xC28 count, the uids at 0xC2C) that is this cover point (0x0001E510..0x0001E528). A
// cover point between the attack ranges (the 3D distance to the target, Magnitude at
// 0x0001E588, strictly above 0x3DC and below 0x3E0) gains lbl_10_rodata_40C = 5.0f. The flat
// (z = lbl_10_rodata_3FC = 0.0f) directions from the target to the cover point and to the actor
// (each normalized when it can be, 0x0001E5B4/0x0001E610) more than lbl_10_rodata_43C = pi / 2
// apart (the fcmpo at 0x0001E63C) lose lbl_10_rodata_4C8 = 15.0f, and the flat forward axis (the
// guest's 0x28/0x38/0x48) and the flat direction from the actor to the cover point more than
// pi / 2 apart (0x0001E704) lose 10.
float CDarkSamus::ScoreCoverPoint(const CScriptCoverPoint& cover, const CVector3f& target) const {
  float score = 10.f;
  for (uint i = 0; i < mAttackerCount; ++i) {
    if (mAttackerUids[i] == cover.GetUniqueId()) { // 0x0001E518
      score -= 10.f;
    }
  }
  const CVector3f toCover = cover.GetTranslation() - target;
  CVector3f flatToCover(toCover.GetX(), toCover.GetY(), 0.f);
  const float dist = toCover.Magnitude();
  if (dist > mMinAttackRange && dist < mMaxAttackRange) { // 0x0001E590/0x0001E59C
    score += 5.f;
  }
  if (flatToCover.CanBeNormalized()) {
    flatToCover.Normalize();
  }
  const CVector3f toSelf = GetTranslation() - target;
  CVector3f flatToSelf(toSelf.GetX(), toSelf.GetY(), 0.f);
  if (flatToSelf.CanBeNormalized()) {
    flatToSelf.Normalize();
  }
  if (CVector3f::GetAngleDiff(flatToCover, flatToSelf) > 1.5707964f) { // 0x0001E63C
    score -= 15.f;
  }
  const CVector3f forward = GetTransform().GetForward();
  CVector3f flatForward(forward.GetX(), forward.GetY(), 0.f);
  if (flatForward.CanBeNormalized()) {
    flatForward.Normalize();
  }
  const CVector3f selfToCover = cover.GetTranslation() - GetTranslation();
  CVector3f flatSelfToCover(selfToCover.GetX(), selfToCover.GetY(), 0.f);
  if (flatSelfToCover.CanBeNormalized()) {
    flatSelfToCover.Normalize();
  }
  if (CVector3f::GetAngleDiff(flatForward, flatSelfToCover) > 1.5707964f) { // 0x0001E704
    score -= 10.f;
  }
  return score;
}

// fn_10_1E740 (0x0001E740, 0x120 bytes): should the actor forget the uid of the attacker that hit
// it last (the guest's 0xC24)?
//
// Without an attacker there is nothing to forget (the lhz against kInvalidUniqueId at
// 0x0001E760..0x0001E774). A shagged path (PathShagged at 0x0001E788, its result's low byte
// against 1 at 0x0001E78C..0x0001E794) forgets the attacker, and so does a path find that is
// over: the guest's 0x888 - the search's current waypoint - against the guest's 0x7C4 - the
// search's waypoint count - minus one (0x0001E798..0x0001E7A8). An attacker without a cover
// point forgets it too (the null check at 0x0001E7CC). Otherwise the distance between the cover
// point and the actor (the three fsubs at 0x0001E7DC..0x0001E804) is squared (the three fmuls
// at 0x0001E810..0x0001E828) and compared with lbl_10_rodata_408 = 4.0f (the fcmpo at
// 0x0001E840); the mfcr/srwi at 0x0001E844..0x0001E848 keeps the compare's "less than" bit, so
// the attacker is forgotten once it is within 2 units.
bool CDarkSamus::ShouldForgetLastAttacker(CStateManager& mgr) const {
  if (mLastAttackerUid == kInvalidUniqueId) {
    return false;
  }
  if (PathShagged(mgr, CTriggerData(0.f))) {
    return true;
  }
  const CPathFindSearch* search = GetSearchPath();
  if (search->GetCurrentWaypoint() >= search->GetWaypoints().size() - 1) {
    return true;
  }
  const CScriptCoverPoint* cover = GetCoverPoint(mgr, mLastAttackerUid);
  if (cover == nullptr) {
    return true;
  }
  return (cover->GetTranslation() - GetTranslation()).MagSquared() < 4.0f;
}

// fn_10_1D9A4 (0x0001D9A4, 0x370 bytes): the point the actor is pushed away from.
//
// The actor's connections are walked (the begin/end at 0x1C/0x14, 0x0001D9CC/0x0001DB2C): each
// one's editor id is resolved (GetIdForScript at 0x0001D9F8, the invalid uid skipped at
// 0x0001DA0C) and cast to a repulsor (0x0001DA20). An active repulsor (the byte at 0x20's bit 7,
// the extrwi at 0x0001DA30) in the actor's area (0x0001DA40) without the repel-player flag (the
// flags at 0x164 & 1, 0x0001DA4C) is measured: the offset from the actor to it (0x0001DA54..),
// with the height dropped for a cylinder (the shape against 1 at 0x0001DA88, lbl_10_rodata_3FC =
// 0.0f), squared against the radius squared (the fcmpo at 0x0001DAD4; a point outside is
// skipped). A repulsor that holds the actor answers its position (0x0001DB0C), and `strength`,
// when not null (0x0001DADC), gets 1 - distance / radius (lbl_10_rodata_3E8 = 1.0f, 0x0001DB04).
//
// Without one, the area's path find area (the world's area of the actor's area id, its post
// constructed data's 0xEC, 0x0001DB44..0x0001DB68) is looked at; without a path area the answer
// is zero (0x0001DB6C). The actor's position lifted by lbl_10_rodata_3E4 = 0.5f (and
// lbl_10_rodata_3FC = 0.0f on x and y) is taken into the area's frame (the translation at
// 0x1C8/0x1D8/0x1E8 subtracted, TransposeRotate at 0x0001DBF4) and the regions there are found
// with the search's region flags and creature mask (0x0001DC1C..0x0001DC44, the last argument
// true); the first region with kPFO_Unknown2 obstructions (GetObstructionCount(2) > 0 at
// 0x0001DC60) answers its centroid (the region's 0x28, 0x0001DC6C). Otherwise the last contact
// point answers for lbl_10_rodata_46C = 0.3 seconds after the contact (0x0001DCAC..0x0001DCB4)
// and the zero vector after that (0x0001DCD4).
CVector3f CDarkSamus::GetRepulsionPoint(CStateManager& mgr, float* strength) const {
  const rstl::vector< SConnection >& connections = GetConnectionList();
  for (rstl::vector< SConnection >::const_iterator it = connections.begin();
       it != connections.end(); ++it) {
    const TUniqueId uid = mgr.GetIdForScript(it->objId);
    if (uid == kInvalidUniqueId) {
      continue;
    }
    const CScriptRepulsor* repulsor = TCastToConstPtr< CScriptRepulsor >(mgr.GetObjectById(uid));
    if (repulsor == nullptr || !repulsor->GetActive() ||
        repulsor->GetCurrentAreaId() != GetCurrentAreaId() ||
        (repulsor->GetFlags() & CScriptRepulsor::kF_RepelPlayer) != 0) { // 0x0001DA4C
      continue;
    }
    CVector3f delta = repulsor->GetTranslation() - GetTranslation();
    if (repulsor->GetShape() == CScriptRepulsor::kS_Cylinder) { // 0x0001DA88
      delta.SetZ(0.f);
    }
    const float radius = repulsor->GetRadius();
    if (delta.MagSquared() > repulsor->GetRadius() * radius) { // 0x0001DAD4
      continue;
    }
    if (strength != nullptr) { // 0x0001DADC
      *strength = 1.f - delta.Magnitude() / repulsor->GetRadius();
    }
    return repulsor->GetTranslation();
  }

  CPFArea* area =
      mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea;
  if (area == nullptr) { // 0x0001DB6C
    return CVector3f::Zero();
  }
  const CTransform4f& areaXf = area->GetTransform();
  const CVector3f position = GetTranslation() + CVector3f(0.f, 0.f, 0.5f);
  const CVector3f local = areaXf.TransposeRotate(position - areaXf.GetTranslation());
  rstl::reserved_vector< CPFRegion*, 8 > regions;
  const uint creatureMask = mPathFindSearch.GetCreatureMask();
  area->FindRegions(regions, local, mPathFindSearch.GetRegionFlags(), creatureMask, true);
  for (int i = 0; i < regions.size(); ++i) {
    CPFRegion* region = regions[i];
    if (region->GetObstructionCount(kPFO_Unknown2) > 0) { // 0x0001DC64
      return region->GetCentroid();
    }
  }
  if (0.3f + mContactTime > mElapsedTime) { // 0x0001DCB0
    return mContactPoint;
  }
  return CVector3f::Zero();
}

// fn_10_1DD14 (0x0001DD14, 0x260 bytes): the separation.
//
// The steering direction (the guest's 0x910) is cleared (0x0001DD50..0x0001DD68) and the
// repulsion point is asked for with the strength 0 (lbl_10_rodata_3FC, 0x0001DD6C); the zero
// vector ends the function (the three fcmpu at 0x0001DD8C..0x0001DDB0). The point is taken at
// the actor's height (0x0001DDD4). Retail then builds the flat direction from the actor to its
// path find navigation's target (the guest's 0x700, the navigation's mPathTarget) or, when that
// cannot be normalized, the actor's forward axis (0x0001DDC4..0x0001DE3C), but no later
// instruction reads it (the stack slot at 0x3C is dead), so the port leaves it out. The speed is
// lbl_10_rodata_408 = 4.0f, lbl_10_rodata_414 = 8.0f for 0.3 seconds after the last contact
// (0x0001DE5C), and scales with the strength below lbl_10_rodata_46C = 0.3 (0x0001DE78..). The
// base's steering separation from the point (the radius lbl_10_rodata_448 = 12.0f, 0x0001DEA4)
// is flattened (0x0001DEC4) and, when it can be normalized (0x0001DED8), becomes a locomotion
// command (its direction normalized, the zero face vector and the speed, 0x0001DF38) and the
// steering direction (0x0001DF40..0x0001DF50).
void CDarkSamus::Separate(CStateManager& mgr) {
  mSeparationDir = CVector3f::Zero();
  float strength = 0.f;
  CVector3f point = GetRepulsionPoint(mgr, &strength);
  if (point == CVector3f::Zero()) {
    return;
  }
  point.SetZ(GetTranslation().GetZ());
  float speed = 4.f;
  if (0.3f + mContactTime > mElapsedTime) { // 0x0001DE5C
    speed = 8.f;
  }
  if (strength < 0.3f) { // 0x0001DE78
    speed *= strength / 0.3f;
  }
  CVector3f separation = mSteeringBehaviors.Separation(*this, point, 12.f);
  separation.SetZ(0.f);
  if (separation.CanBeNormalized()) { // 0x0001DED8
    separation.Normalize();
    BodyController()->CommandMgr().DeliverCmd(
        CBCLocomotionCmd(separation.AsNormalized(), CVector3f::Zero(), speed));
    mSeparationDir = separation;
  }
}

// fn_10_1D860 (0x0001D860, 0x144 bytes): the additive animation of the state.
//
// kStateMsg_Activate (the r5 against 0 at 0x0001D868) builds the parms - the state
// kAS_AdditiveReaction (0x18, the li r4 at 0x0001D8F4), the enum 5 (FromEnum at 0x0001D8C4) and
// seven empty parms (NoParameter, 0x0001D888..) - through fn_10_13FFC (0x0001D8F8, the module's
// copy of the CPASAnimParmData constructor: it stores the state and appends the eight parms)
// and asks the animation's PAS database for the best animation (FindBestAnimation with the
// state manager's random generator and -1, 0x0001D918). A score above 0 (lbl_10_rodata_3FC =
// 0.0f, the fcmpo at 0x0001D92C) keeps the animation's index (0x0001D934) and adds it with the
// weight lbl_10_rodata_3E8 = 1.0f, active and fading out (the li r5/r6 = 1 and
// AddAdditiveAnimation at 0x0001D958). kStateMsg_Deactivate (the r5 against 2 at 0x0001D960)
// removes the animation when there is one (the index against -1, 0x0001D96C/0x0001D970;
// DelAdditiveAnimation at 0x0001D980) and forgets it (0x0001D988). kStateMsg_Update does nothing.
void CDarkSamus::UpdateAdditiveAnimation(CStateManager& mgr, EStateMsg msg) {
  if (msg == kStateMsg_Activate) {
    const CPASAnimParmData parms(pas::kAS_AdditiveReaction, CPASAnimParm::FromEnum(5));
    const rstl::pair< float, int > best =
        GetAnimationData()->GetPASDatabase().FindBestAnimation(parms, *mgr.Random(), -1);
    if (best.first > 0.f) { // 0x0001D92C
      mAdditiveAnim = best.second;
      AnimationData()->AddAdditiveAnimation(mAdditiveAnim, 1.f, true, true);
    }
  } else if (msg == kStateMsg_Deactivate) {
    if (mAdditiveAnim != -1) { // 0x0001D970
      AnimationData()->DelAdditiveAnimation(mAdditiveAnim);
      mAdditiveAnim = -1;
    }
  }
}

// fn_10_EFC8 (0x0000EFC8, 0x138 bytes): is the actor where it must not take cover?
//
// With a safe position (the guest's 0x928 against 1 at 0x0000EFF0) the actor is when it is more
// than lbl_10_rodata_404 = 10.0f below it (the fcmpo at 0x0000F00C, strict). Otherwise the AI
// waypoint list (the state manager's 0x838) is walked (0x0000F028..0x0000F0D4): an active AI hint
// (the TCastToPtr at 0x0000F034, the byte at 0x20's bit 7) in the actor's area (0x0000F054) of
// type 0x1C (the word at 0x158, 0x0000F060) whose radius (0x15C) holds the actor (the 3D
// distance, Magnitude at 0x0000F0A0, strictly less) says it is.
bool CDarkSamus::IsInUnsafeSpot(CStateManager& mgr) const {
  if (mHasSafePos == 1 && GetTranslation().GetZ() < mSafePos.GetZ() - 10.f) { // 0x0000F00C
    return true;
  }
  CObjectList& list = mgr.ObjectListById(kOL_AiWaypoint);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CScriptAIHint* hint = TCastToPtr< CScriptAIHint >(list[i]);
    if (hint == nullptr || !hint->GetActive() ||
        hint->GetCurrentAreaId() != GetCurrentAreaId()) {
      continue;
    }
    // 0x1C is not in CScriptAIHint::EHintType yet.
    if (static_cast< int >(hint->GetHintType()) != 0x1C) { // 0x0000F060
      continue;
    }
    if ((GetTranslation() - hint->GetTranslation()).Magnitude() < hint->GetRadius()) {
      return true; // 0x0000F0A4
    }
  }
  return false;
}

// fn_10_11334 (0x00011334, 0xF8 bytes): the looping glide emitter at the actor.
//
// kStateMsg_Activate (0x00011354) starts it unless the sound is kInternalInvalidSfxId (the
// cmplw at 0x00011378): AddEmitter at the actor's position in its area, with acoustics and
// looping (the li r7/r8 = 1 at 0x00011394/0x00011398) and kMedPriority (the lha at 0x00011388),
// and keeps the handle (0x000113A4). kStateMsg_Update starts it first when there is no handle
// (the recursive call with kStateMsg_Activate at 0x000113BC) and then, with a handle, moves it
// to the actor (UpdateEmitter with the zero direction and CAudioSys::kMaxVolume, 0x000113EC).
// kStateMsg_Deactivate (the case 2 at 0x00011368) removes the emitter when there is one
// (0x0001140C) and clears the handle (0x00011414). Other messages do nothing.
void CDarkSamus::UpdateGlideEmitter(EStateMsg msg) {
  switch (msg) {
  case kStateMsg_Activate:
    if (mGlideSfxId == CSfxManager::kInternalInvalidSfxId) { // 0x00011378
      return;
    }
    mGlideSfx = CSfxManager::AddEmitter(mGlideSfxId, GetTranslation(),
                                        GetCurrentAreaId().Value(), true, true,
                                        CSfxManager::kMedPriority);
    break;
  case kStateMsg_Update:
    if (!mGlideSfx) { // 0x000113B0
      UpdateGlideEmitter(kStateMsg_Activate);
    }
    if (!mGlideSfx) { // 0x000113C4
      return;
    }
    CSfxManager::UpdateEmitter(mGlideSfx, GetTranslation(), CVector3f::Zero(),
                               CAudioSys::kMaxVolume);
    break;
  case kStateMsg_Deactivate:
    if (!mGlideSfx) { // 0x000113F8
      return;
    }
    CSfxManager::RemoveEmitter(mGlideSfx);
    mGlideSfx = CSfxHandle();
    break;
  default:
    break;
  }
}

// fn_10_128D8 (0x000128D8, 0x88 bytes): the first position the actor is safe at.
//
// Nothing happens once there is one (the guest's 0x928 at 0x000128F0) or while the actor is
// unsafe (fn_10_EFC8 at 0x000128F8). Otherwise the actor's position is kept (0x91C/0x920/0x924)
// and the flag is set (0x0001292C); the guest checks the flag again (0x0001290C) and keeps the
// position without setting it when it is already set, which the first check makes unreachable.
void CDarkSamus::UpdateSafePosition(CStateManager& mgr) {
  if (mHasSafePos != 0) {
    return;
  }
  if (IsInUnsafeSpot(mgr)) {
    return;
  }
  mSafePos = GetTranslation();
  mHasSafePos = 1;
}

// fn_10_1E178 (0x0001E178, 0xF0 bytes): the damage the actor has taken (the guest's 0xCC0).
//
// The health info's current health (the base's GetHealthInfo - the vtable slot 0x3C - and the
// float at its +4) is compared with the health of the last call (the guest's 0xCC4): when they
// differ (the fcmpu and the beq at 0x0001E1B8) and the message (the guest's argument r4) is not
// kStateMsg_Activate
// and the last health was above zero (the fcmpo and the ble at 0x0001E1C8), the health that is
// gone (0xCC4 - the current health) is added to the damage (0x0001E1F4..0x0001E200), and the last
// health is updated (0x0001E218). Then the damage decays by the rate at 0xCCC (1.0f) times dt
// (0x0001E22C..0x0001E238) and never goes below zero (0x0001E248).
void CDarkSamus::AccumulateDamage(EStateMsg msg, float dt) {
  const CHealthInfo* health = GetHealthInfo();
  const float current = health->GetHP();
  if (current != mLastHealth) {
    if (mLastHealth > 0.f && msg != kStateMsg_Activate) {
      mAccumulatedDamage += (mLastHealth - current);
    }
    mLastHealth = current;
  }
  mAccumulatedDamage -= mDamageDecay * dt;
  if (mAccumulatedDamage < 0.f) {
    mAccumulatedDamage = 0.f;
  }
}

// ---- the trigger bodies (stand-ins) ------------------------------------------------------------

bool CDarkSamus::Alerted(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_F840", "trigger Alerted");
  return false;
}

bool CDarkSamus::Arrived(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_62B4", "trigger Arrived");
  return false;
}

bool CDarkSamus::AnyIncomingMissiles(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_1BB08", "trigger AnyIncomingMissiles");
  return false;
}

bool CDarkSamus::BreakFreezeBeam(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_16B48", "trigger BreakFreezeBeam");
  return false;
}

bool CDarkSamus::BreakDash(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_149B4", "trigger BreakDash");
  return false;
}

bool CDarkSamus::BreakDiveAttack(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_11C54", "trigger BreakDiveAttack");
  return false;
}

bool CDarkSamus::BreakNormalMissile(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_16B5C", "trigger BreakNormalMissile");
  return false;
}

bool CDarkSamus::BreakPause(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_106F8", "trigger BreakPause");
  return false;
}

bool CDarkSamus::BreakPhazonAttack(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_4ABC", "trigger BreakPhazonAttack");
  return false;
}

bool CDarkSamus::BreakSuperMissile(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_16B54", "trigger BreakSuperMissile");
  return false;
}

bool CDarkSamus::CanMeleeAttack(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_10634", "trigger CanMeleeAttack");
  return false;
}

// fn_10_14284 (0x00014284): the fight script's object (fn_10_17E14) - without one the trigger
// answers false (the cmplwi at 0x00014294) - and the float at the stage info's +0x24 (the lfs at
// 0x000142A8) against zero (lbl_10_rodata_3FC): the trigger answers when the float is below
// zero (the fcmpo at 0x000142B0 and the extrwi at 0x000142B8, which keeps the compare's "less
// than" bit).
bool CDarkSamus::CanMissileDeflect(CStateManager& mgr, const CTriggerData&) const {
  const CDarkSamusBattleStage* stage = GetBattleStage(mgr);
  if (stage == nullptr) {
    return false;
  }
  return stage->GetMissileDeflectTime() < 0.f;
}

bool CDarkSamus::DiveAfterBoost(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_95A8", "trigger DiveAfterBoost");
  return false;
}

bool CDarkSamus::DoneRolling(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_9314", "trigger DoneRolling");
  return false;
}

// fn_10_F864: the trigger is the byte at 0xD4C (the lbz at 0x0000F864) - the flag the
// TurnToFacePlayer state sets when the actor faces the player.
// @ fn_10_F864
bool CDarkSamus::DoneTurning(CStateManager&, const CTriggerData&) const {
  return mTurnDone != 0;
}

bool CDarkSamus::FacingPlayer(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_142CC", "trigger FacingPlayer");
  return false;
}

bool CDarkSamus::HitDuringEnrage(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_7B68", "trigger HitDuringEnrage");
  return false;
}

bool CDarkSamus::OutOfMeleeRange(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_17CB8", "trigger OutOfMeleeRange");
  return false;
}

// fn_10_116F8 (0x000116F8): the word at 0x984 - the fight script's picked attack id - against
// 0x20000, the id of the boost ball. The body is the cntlzw/srwi idiom over `lis r0,0x2`
// (0x20000) - `subf` - `cntlzw` - `srwi r3,r0,5` (0x000116FC..0x0001170C), which makes the result
// 1 only for an exact match.
// @ fn_10_116F8
bool CDarkSamus::PickedBoostBall(CStateManager&, const CTriggerData&) const {
  return mPickedAttack == 0x20000;
}

// fn_10_11760 (0x00011760): the word at 0x984 against 0x100000, the id of the freeze beam. The
// body is the cntlzw/srwi idiom over `lis r0,0x10` (0x100000) (0x00011764..0x00011774). Not
// marked: the freeze beam is not one of the TravelToCoverPoints state's transitions, so the body
// did not run in the walk.
bool CDarkSamus::PickedFreezeBeam(CStateManager&, const CTriggerData&) const {
  return mPickedAttack == 0x100000;
}

// fn_10_1174C (0x0001174C): the word at 0x984 - the fight script's picked attack id - against 0x20, the
// id of the dive attack. The body is the cntlzw/srwi idiom (the four instructions after the load),
// which makes the result 1 only for an exact match.
// @ fn_10_1174C
bool CDarkSamus::PickedDiveAttack(CStateManager&, const CTriggerData&) const {
  return mPickedAttack == 0x20;
}

// fn_10_117D8 (0x000117D8): the word at 0x984 - the fight script's picked attack id - against 1, the
// id of the melee attack. The body is the cntlzw/srwi idiom (the four instructions after the load),
// which makes the result 1 only for an exact match.
// @ fn_10_117D8
bool CDarkSamus::PickedMelee(CStateManager&, const CTriggerData&) const {
  return mPickedAttack == 1;
}

// fn_10_11710 (0x00011710): the word at 0x984 - the fight script's picked attack id - against 4, the
// id of the melee dash. The body is the cntlzw/srwi idiom (the four instructions after the load),
// which makes the result 1 only for an exact match.
// @ fn_10_11710
bool CDarkSamus::PickedMeleeDash(CStateManager&, const CTriggerData&) const {
  return mPickedAttack == 4;
}

// fn_10_1178C (0x0001178C): the word at 0x984 - the fight script's picked attack id - against 0x80, the
// id of the missile jump. The body is the cntlzw/srwi idiom (the four instructions after the load),
// which makes the result 1 only for an exact match.
// @ fn_10_1178C
bool CDarkSamus::PickedMissileJump(CStateManager&, const CTriggerData&) const {
  return mPickedAttack == 0x80;
}

// fn_10_11724 (0x00011724): the word at 0x984 - the fight script's picked attack id - against 0x10, the
// id of the moving scatter shot. The body is the cntlzw/srwi idiom (the four instructions after the load),
// which makes the result 1 only for an exact match.
// @ fn_10_11724
bool CDarkSamus::PickedMovingSShot(CStateManager&, const CTriggerData&) const {
  return mPickedAttack == 0x10;
}

// fn_10_117A0 (0x000117A0): the byte at 0xDEC (the fade flag) first - when it is 1 the
// trigger answers for the id 0x200000 (the compare at 0x000117B0, `lis r0,0x20`), otherwise
// for the normal missile's id 0x40 (the compare at 0x000117C4).
bool CDarkSamus::PickedNormalMissile(CStateManager&, const CTriggerData&) const {
  if (mFadeOut == 1) {
    return mPickedAttack == 0x200000;
  }
  return mPickedAttack == 0x40;
}

// fn_10_11800 (0x00011800): the word at 0x984 - the fight script's picked attack id - against 0x400, the
// id of the pause. The body is the cntlzw/srwi idiom (the four instructions after the load),
// which makes the result 1 only for an exact match.
// @ fn_10_11800
bool CDarkSamus::PickedPause(CStateManager&, const CTriggerData&) const {
  return mPickedAttack == 0x400;
}

// fn_10_8680 (DOL, the REL's table only names it): the word at 0x984 against 0x400000, the id of
// the phazon attack. The body is the cntlzw/srwi idiom over `lis r0,0x40` (0x400000) -
// `subf` - `cntlzw` - `srwi r3,r0,5` (0x00008680..0x00008698). The DOL's copy is not in the
// port's sources, so the body is ported here against the host's member.
// @ fn_10_8680
bool CDarkSamus::PickedPhazonAttack(CStateManager&, const CTriggerData&) const {
  return mPickedAttack == 0x400000;
}

// fn_10_8114 (DOL, the REL's table only names it): the word at 0x984 against 0x800000, the id of
// the phazon enrage. The body is the cntlzw/srwi idiom over `lis r0,0x80` (0x800000) (0x00008114
// ..0x0000812C). The DOL's copy is not in the port's sources, so the body is ported here.
// @ fn_10_8114
bool CDarkSamus::PickedPhazonEnrage(CStateManager&, const CTriggerData&) const {
  return mPickedAttack == 0x800000;
}

// fn_10_11738 (0x00011738): the word at 0x984 - the fight script's picked attack id - against 0x8, the
// id of the scatter shot. The body is the cntlzw/srwi idiom (the four instructions after the load),
// which makes the result 1 only for an exact match.
// @ fn_10_11738
bool CDarkSamus::PickedScatterShot(CStateManager&, const CTriggerData&) const {
  return mPickedAttack == 0x8;
}

// fn_10_11778 (0x00011778): the word at 0x984 - the fight script's picked attack id - against 0x100, the
// id of the super missile. The body is the cntlzw/srwi idiom (the four instructions after the load),
// which makes the result 1 only for an exact match.
// @ fn_10_11778
bool CDarkSamus::PickedSuperMissile(CStateManager&, const CTriggerData&) const {
  return mPickedAttack == 0x100;
}

// fn_10_CDA0 (DOL, the REL's table only names it): the word at 0x984 against 0x10000, the id of
// the sweep beam. The body is the cntlzw/srwi idiom over `lis r0,0x1` (0x10000) - `subf` -
// `cntlzw` - `srwi r3,r0,5` (0x0000CDA0..0x0000CDB8). The DOL's copy is not in the port's
// sources, so the body is ported here against the host's member.
// @ fn_10_CDA0
bool CDarkSamus::PickedSweepBeam(CStateManager&, const CTriggerData&) const {
  return mPickedAttack == 0x10000;
}

// fn_10_117EC (0x000117EC): the word at 0x984 - the fight script's picked attack id - against 0x200, the
// id of the taunt. The body is the cntlzw/srwi idiom (the four instructions after the load),
// which makes the result 1 only for an exact match.
// @ fn_10_117EC
bool CDarkSamus::PickedTaunt(CStateManager&, const CTriggerData&) const {
  return mPickedAttack == 0x200;
}

bool CDarkSamus::PlayerIsMorphball(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_112E8", "trigger PlayerIsMorphball");
  return false;
}

bool CDarkSamus::PlayerTooClose(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_10658", "trigger PlayerTooClose");
  return false;
}

bool CDarkSamus::ReachedPissedOff(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_FAF4", "trigger ReachedPissedOff");
  return false;
}

bool CDarkSamus::ReadyForMissiles(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_6A40", "trigger ReadyForMissiles");
  return false;
}

bool CDarkSamus::ShouldEnragedMelee(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_6B78", "trigger ShouldEnragedMelee");
  return false;
}

bool CDarkSamus::ShouldFadeIn(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_BB78", "trigger ShouldFadeIn");
  return false;
}

// fn_10_BBE4 (0x0000BBE4): the fight script's object (fn_10_17E14) - without one the trigger
// answers false (the cmplwi at 0x0000BC04) - its byte at the stage info's +0x31 (the lbz at
// 0x0000BC18), which must be 1 (the cmplwi at 0x0000BC1C), and then the actor's own fade flag
// (the guest's 0xDEC, the lbz at 0x0000BC24), which must be 0.
bool CDarkSamus::ShouldFadeOut(CStateManager& mgr, const CTriggerData&) const {
  const CDarkSamusBattleStage* stage = GetBattleStage(mgr);
  if (stage == nullptr || !stage->GetFadeOutFlag()) {
    return false;
  }
  return mFadeOut == 0;
}

// fn_10_FD30 (0x0000FD30): the byte at 0x420's bit 6 (0x40 - the extrwi at 0x0000FD34 keeps
// the byte's bit 25, which is 1u << (31 - 25) = 0x40) and then the byte at 0xD58. The bit is
// the base class' own flag: fn_10_F40C (the scannable info) and fn_10_17F04 (the render
// pass) test it together with the frozen and the visible flags, so it is the alive flag
// the host keeps in GetAlive(). The byte at 0xD58 is the flag the PissedOff state clears
// (fn_10_F930) and the fight script sets.
bool CDarkSamus::ShouldGetPissedOff(CStateManager&, const CTriggerData&) const {
  if (GetAlive()) {
    return mPissedOff != 0;
  }
  return false;
}

bool CDarkSamus::ShouldLeaveSphere(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_6A60", "trigger ShouldLeaveSphere");
  return false;
}

// fn_10_11158 (0x00011158, 0x154 bytes): the trigger's switch over the attack the actor is
// performing (the word at 0x980, read at 0x00011218) stands behind four of the module's own
// triggers, called through the handler table the vptr points at (the words at 0x208, 0x174,
// 0x184 and 0x14C of lbl_10_data_CBC - docs/module-analysis/DarkSamus_fsm.tsv): the word at
// 0x208 is WasHeavyHit (fn_10_112AC) and answers the trigger at once when it is true (the
// li r3, 1 at 0x00011198), and then the words at 0x174 (CanMissileDeflect, fn_10_14284),
// 0x184 (FacingPlayer, fn_10_142CC) and 0x14C (AnyIncomingMissiles, fn_10_1BB08) must all
// answer true (the cmplwi against 1 at 0x000111C0, 0x000111E4 and 0x00011210) or the trigger
// answers false (0x0001128C).
//
// The four answers are read as one byte: `clrlwi r0, r3, 24` clears the HIGH 24 bits and keeps
// the low byte (the mask is 0xFF, not 0xFFFFFF), and `cmplwi r0, 1` compares that byte with 1.
// The triggers return a bool, so the test is the bool itself.
//
// The switch over the attack id: 0x8 the scatter shot, 0x40 the normal missile, 0x200 the taunt,
// 0x400 the pause, 0x10000 the sweep beam and 0x200000 the fade variant of the normal missile
// answer true (0x0001127C), 0xFFFF (no attack) answers the fade flag at 0xDEC (0x00011284) and
// everything else answers false.
bool CDarkSamus::ShouldReact(CStateManager& mgr, const CTriggerData& data) const {
  if (WasHeavyHit(mgr, data)) {
    return true;
  }
  if (!CanMissileDeflect(mgr, data) || !FacingPlayer(mgr, data) ||
      !AnyIncomingMissiles(mgr, data)) {
    return false;
  }
  switch (mCurrentAttack) {
  case 0x8:      // the scatter shot
  case 0x40:     // the normal missile
  case 0x200:    // the taunt
  case 0x400:    // the pause
  case 0x10000:  // the sweep beam
  case 0x200000: // the fade variant of the normal missile
    return true;
  case 0xFFFF:   // no attack
    return mFadeOut != 0;
  default:
    return false;
  }
}

bool CDarkSamus::ShouldRiseAgain(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_6AE8", "trigger ShouldRiseAgain");
  return false;
}

bool CDarkSamus::SkipPhazonAttack(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_110E0", "trigger SkipPhazonAttack");
  return false;
}

// fn_10_F924: the trigger is the flag byte's bit 6 - the extrwi at 0x0000F928 keeps the byte's
// bit 25, the mask 0x40 (the PowerPC bit 25 is 1u << (31 - 25) = 0x40).
// @ fn_10_F924
bool CDarkSamus::StartsInTheAir(CStateManager&, const CTriggerData&) const {
  return (mFlags90C & 0x40) != 0;
}

// fn_10_10698 (0x00010698, 0x5C bytes): the trigger answers when the damage the actor has taken
// (the float at 0xCC0) is above the threshold (the float at 0xCC8): the fcmpo at 0x000106E8
// and the extrwi at 0x000106F0, which takes the CR0 bit 1 - the "greater than" of an fcmpo
// (bit 0 is the "less than") - so the test is 0xCC0 > 0xCC8.
//
// Two guards come first. The attack the actor is performing (the word at 0x980) must not be
// 0x8000: the cmplwi at 0x000106A0 compares the whole word with 0x8000 - the `addis r0, r4, 0x0`
// at 0x0001069C is a plain copy of the word into the register cmplwi destroys, and cmplwi
// compares the 32-bit register against the 16-bit immediate, so the test is the whole word
// against 0x00008000. And when the actor has a history (the count at 0x994, the ids at 0x998),
// the last id of the history (the word at 0x998 + (count - 1) * 4, the slwi/add at
// 0x000106BC..0x000106C8) must not be 0x8000 either (the copy at 0x000106CC and the cmplwi at
// 0x000106D0).
bool CDarkSamus::TooMuchDamage(CStateManager&, const CTriggerData&) const {
  if (mCurrentAttack == 0x8000) {
    return false;
  }
  if (mAttackHistoryCount > 0 && mAttackHistory[mAttackHistoryCount - 1] == 0x8000) {
    return false;
  }
  return mAccumulatedDamage > mDamageThreshold;
}

// fn_10_F800 (0x0000F800, 0x40 bytes): the trigger answers when the actor has stood still for
// more than 1.2 seconds (the float at 0x964 against lbl_10_rodata_42C = 1.2f: the fcmpo at
// 0x0000F80C and the ble at 0x0000F810, so the test is 0x964 > 1.2f) - and otherwise when more
// than 0.2 seconds have passed since the last wall collision (the float at 0xC44 plus
// lbl_10_rodata_428 = 0.2f against the elapsed time at 0x8B0: the fadds at 0x0000F82C, the
// fcmpo at 0x0000F830 and the extrwi at 0x0000F838, which takes the CR0 bit 1 - the "greater
// than" of an fcmpo).
bool CDarkSamus::WallCollision(CStateManager&, const CTriggerData&) const {
  if (mStillTime > 1.2f) {
    return true;
  }
  return (0.2f + mLastWallHitTime) > mElapsedTime;
}

bool CDarkSamus::WasHeavyHit(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_112AC", "trigger WasHeavyHit");
  return false;
}

// ---- the state bodies (stand-ins) --------------------------------------------------------------

void CDarkSamus::BoostBallFire(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_9724", "state BoostBallFire");
  DsStateEntered("BoostBallFire", msg);
}

void CDarkSamus::BoostBallInto(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_B164", "state BoostBallInto");
  DsStateEntered("BoostBallInto", msg);
}

void CDarkSamus::BoostBallRoll(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_A3B0", "state BoostBallRoll");
  DsStateEntered("BoostBallRoll", msg);
}

void CDarkSamus::FreezeBeam(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_16CDC", "state FreezeBeam");
  DsStateEntered("FreezeBeam", msg);
}

void CDarkSamus::Dash(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_150DC", "state Dash");
  DsStateEntered("Dash", msg);
}

void CDarkSamus::DiveAttack(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_11FA4", "state DiveAttack");
  DsStateEntered("DiveAttack", msg);
}

void CDarkSamus::DiveFromBoost(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_9424", "state DiveFromBoost");
  DsStateEntered("DiveFromBoost", msg);
}

void CDarkSamus::EnrageHitReact(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_7A14", "state EnrageHitReact");
  DsStateEntered("EnrageHitReact", msg);
}

void CDarkSamus::EnragedTravel(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_6E38", "state EnragedTravel");
  DsStateEntered("EnragedTravel", msg);
}

void CDarkSamus::FadeIn(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_BD40", "state FadeIn");
  DsStateEntered("FadeIn", msg);
}

void CDarkSamus::FadeOut(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_BC54", "state FadeOut");
  DsStateEntered("FadeOut", msg);
}

void CDarkSamus::FallFromTheAir(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_F86C", "state FallFromTheAir");
  DsStateEntered("FallFromTheAir", msg);
}

void CDarkSamus::GetUp(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_18EAC", "state GetUp");
  DsStateEntered("GetUp", msg);
}

void CDarkSamus::GoToPhazonAttack(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_652C", "state GoToPhazonAttack");
  DsStateEntered("GoToPhazonAttack", msg);
}

void CDarkSamus::GoToPhazonEnrage(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_6460", "state GoToPhazonEnrage");
  DsStateEntered("GoToPhazonEnrage", msg);
}

void CDarkSamus::HeavyHit(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_10C9C", "state HeavyHit");
  DsStateEntered("HeavyHit", msg);
}

void CDarkSamus::Interrupt(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_10FF4", "state Interrupt");
  DsStateEntered("Interrupt", msg);
}

void CDarkSamus::MeleeAttack(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_17BC0", "state MeleeAttack");
  DsStateEntered("MeleeAttack", msg);
}

void CDarkSamus::MeleeDash(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_17ABC", "state MeleeDash");
  DsStateEntered("MeleeDash", msg);
}

void CDarkSamus::MissileDeflect(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_178E4", "state MissileDeflect");
  DsStateEntered("MissileDeflect", msg);
}

void CDarkSamus::MissileJump(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_177BC", "state MissileJump");
  DsStateEntered("MissileJump", msg);
}

void CDarkSamus::MovingScatterShot(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_11AD8", "state MovingScatterShot");
  DsStateEntered("MovingScatterShot", msg);
}

void CDarkSamus::NormalMissileAttack(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_17448", "state NormalMissileAttack");
  DsStateEntered("NormalMissileAttack", msg);
}

void CDarkSamus::Null(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_14448", "state Null");
  DsStateEntered("Null", msg);
}

// The base's Dead body is the host's death path (the die command, the fade and the material
// change); it stands in for the module's until that one is ported, so the actor can still die.
void CDarkSamus::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  FsmStandIn("fn_10_143A8", "state Dead");
  DsStateEntered("Dead", msg);
  CPatterned::Dead(mgr, msg, dt);
}

// The base's Patrol body is the host's waypoint patrol; it stands in for the module's until
// that one is ported.
void CDarkSamus::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  FsmStandIn("fn_10_1142C", "state Patrol");
  DsStateEntered("Patrol", msg);
  CPatterned::Patrol(mgr, msg, dt);
}

void CDarkSamus::Pause(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_10710", "state Pause");
  DsStateEntered("Pause", msg);
}

void CDarkSamus::PhazonAttack(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_81D4", "state PhazonAttack");
  DsStateEntered("PhazonAttack", msg);
}

void CDarkSamus::PhazonEnraged(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_7BC0", "state PhazonEnraged");
  DsStateEntered("PhazonEnraged", msg);
}

void CDarkSamus::PhazonFall(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_6BA0", "state PhazonFall");
  DsStateEntered("PhazonFall", msg);
}

void CDarkSamus::PhazonMissiles(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_67B8", "state PhazonMissiles");
  DsStateEntered("PhazonMissiles", msg);
}

void CDarkSamus::PhazonRise(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_7054", "state PhazonRise");
  DsStateEntered("PhazonRise", msg);
}

void CDarkSamus::PhazonSuck(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_4AC8", "state PhazonSuck");
  DsStateEntered("PhazonSuck", msg);
}

void CDarkSamus::PissedOff(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_F930", "state PissedOff");
  DsStateEntered("PissedOff", msg);
}

void CDarkSamus::ScatterShot(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_175AC", "state ScatterShot");
  DsStateEntered("ScatterShot", msg);
}

void CDarkSamus::SideDash(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_153EC", "state SideDash");
  DsStateEntered("SideDash", msg);
}

// The base's Start body is empty, so it stands in for the module's until that one is ported.
// fn_10_F920: the state body is a single blr (0x0000F920) - the state does nothing but mark the
// entry.
// @ fn_10_F920
void CDarkSamus::Start(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("Start", msg);
  CPatterned::Start(mgr, msg, dt);
}

void CDarkSamus::SuperMissileAttack(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_16ECC", "state SuperMissileAttack");
  DsStateEntered("SuperMissileAttack", msg);
}

void CDarkSamus::SweepBeam(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_CA0C", "state SweepBeam");
  DsStateEntered("SweepBeam", msg);
}

void CDarkSamus::Taunt(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_11544", "state Taunt");
  DsStateEntered("Taunt", msg);
}

// fn_10_1DF74 (0x0001DF74, 0x204 bytes): the state that glides the actor from cover point to
// cover point.
//
// The additive animation runs first (fn_10_1D860 at 0x0001DFA8). kStateMsg_Activate keeps the speed
// (the base's 0x3C8 into the guest's 0x96C, 0x0001DFC4), speeds up by lbl_10_rodata_4DC = 1.1f
// (0x0001DFD8) and keeps the safe position (fn_10_128D8 at 0x0001DFDC). kStateMsg_Deactivate puts
// the speed back (0x0001DFF0), stops the glide emitter (fn_10_11334 at 0x0001E004), drops the cover
// point (0x0001E010) and is done. Any other message (and Activate) clears the byte at 0xCD0
// (0x0001E028) and the float at 0xC00 (lbl_10_rodata_3FC = 0.0f, 0x0001E030), forgets the cover
// point when fn_10_1E740 says so (0x0001E050) and, without one, pushes the best one (fn_10_1E3C0
// and fn_10_1E26C at 0x0001E074/0x0001E088). A cover point that is gone (GetCoverPoint at
// 0x0001E0A0) ends the call. On Activate the wall hit time becomes lbl_10_rodata_410 = -1000.0f
// (0x0001E0BC). On Activate or a new cover point (the uid read at 0x0001E034 against the current
// one, 0x0001E0CC) the path find heads for the cover point (fn_10_1C778 at 0x0001E0E4) and the word
// at 0x9FC becomes -1 (0x0001E0EC); otherwise the path find faces the player (the player's uid into
// the navigation's face target, 0x0001E110) and runs (0x0001E114), and the actor separates
// (fn_10_1DD14 at 0x0001E120). Then the damage (fn_10_1E178), the glide emitter (fn_10_11334) and
// the still timer (fn_10_1D794) are updated with the message.
void CDarkSamus::TravelToCoverPoints(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("TravelToCoverPoints", msg);
  UpdateAdditiveAnimation(mgr, msg);
  if (msg == kStateMsg_Activate) {
    mSavedSpeed = mSpeed;
    mSpeed = 1.1f * mSpeed;
    UpdateSafePosition(mgr);
  } else if (msg == kStateMsg_Deactivate) {
    mSpeed = mSavedSpeed;
  }
  if (msg == kStateMsg_Deactivate) {
    UpdateGlideEmitter(msg);
    mLastAttackerUid = kInvalidUniqueId;
    return;
  }
  mGuestCD0 = 0;
  mGuestC00 = 0.f;
  const TUniqueId prev = mLastAttackerUid;
  if (ShouldForgetLastAttacker(mgr) == true) { // 0x0001E044
    mLastAttackerUid = kInvalidUniqueId;
  }
  if (mLastAttackerUid == kInvalidUniqueId) { // 0x0001E064
    PushAttacker(FindBestCoverPoint(mgr));
  }
  const CScriptCoverPoint* cover = GetCoverPoint(mgr, mLastAttackerUid);
  if (cover == nullptr) { // 0x0001E0A8
    return;
  }
  if (msg == kStateMsg_Activate) {
    mLastWallHitTime = -1000.f;
  }
  if (msg == kStateMsg_Activate || prev != mLastAttackerUid) { // 0x0001E0C4/0x0001E0D0
    SetPathFindDestination(mgr, cover->GetTranslation(), dt);
    mGuest9FC = -1;
  } else {
    mPathFindNavigation.SetFaceTarget(mgr.GetPlayer(0)->GetUniqueId());
    mPathFindNavigation.PathFind(mgr, msg, dt, *this);
    Separate(mgr);
  }
  AccumulateDamage(msg, dt);
  UpdateGlideEmitter(msg);
  UpdateStillTimer(msg, dt);
}

void CDarkSamus::TravelToPissedOff(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_FB8C", "state TravelToPissedOff");
  DsStateEntered("TravelToPissedOff", msg);
}

// fn_10_FE18: the TurnToFacePlayer state. The activate half (0x0000FE40) sets the flag byte's
// bit 7 and clears the turn flag, and the turn ends at once when the player is within 90 degrees
// of the actor's facing axis (fn_10_142F4, the call at 0x0000FE60); otherwise the taunt command
// (fn_10_8698, the call at 0x0000FEBC) runs for every message until the base's AnimOver (the
// call at 0x0000FEEC) answers true - the animation state the command advances is the byte the
// base's GetAnimOver reads, so the taunt ends the turn when it is over.
// @ fn_10_FE18
void CDarkSamus::TurnToFacePlayer(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("TurnToFacePlayer", msg);
  if (msg == kStateMsg_Activate) {
    // 0x0000FE40: the flag byte's bit 7 (the rlwimi over the byte's bit 24) and the turn flag
    // cleared (the stb at 0x0000FE5C).
    mFlags90C |= 0x80;
    mTurnDone = 0;
    // 0x0000FE60: fn_10_142F4 - the player within 90 degrees (rodata_43C = 1.5707964) of the
    // actor's facing axis ends the turn at once.
    if (IsPlayerInFront(mgr, 1.5707964f)) {
      mTurnDone = 1;
    }
  }
  if (mTurnDone == 0) {
    // fn_10_8698: the taunt command, which the state runs for every message while the turn is
    // not done. The guest's byte at 0x6B4 is the base's animation state - the byte fn_10_8698
    // stores 1 to on Activate (0x000087E0) and 0 to on Deactivate (0x00008820), and the DOL's
    // ShouldRetry (the call at 0x00008710, with 0x10 = pas::kAS_Taunt) advances it to kAS_Repeat
    // and then to kAS_Over. It is the member the base's AnimOver reads
    // (CPatterned::GetAnimOver = mAnimationState.IsOver()), so the host drives it the way the
    // other ported actors do (CDarkCommando::Lurk).
    switch (msg) {
    case kStateMsg_Activate:
      mAnimationState.SetState(CAnimationState::kAS_Ready);
      BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(pas::kTT_Six));
      break;
    case kStateMsg_Update:
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
        BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(pas::kTT_Six));
      }
      break;
    case kStateMsg_Deactivate:
      mAnimationState.SetState(CAnimationState::kAS_NotReady);
      break;
    }
    // 0x0000FEEC: fn_801519A4, the base's AnimOver - the animation over ends the turn.
    if (AnimOver(mgr, CTriggerData(0))) {
      mTurnDone = 1;
    }
  }
}

// ---------------------------------------------------------------------------------------------
// The attack picker's leaves. An attack is a bit (1 << i, i < 24 - the `cmpwi 0x18` loops) of a
// mask the collectors below clear or set, and a weight from the stage info.

// fn_10_1AA30 (0x0001AA30, 0x2C bytes): `set` (the `clrlwi 24` at 0x0001AA30) == 1 ors the bit
// into the mask (0x0001AA40), anything else clears it (the andc at 0x0001AA50).
void CDarkSamus::SetAttackBit(uint* mask, uchar set, uint bit) {
  if (set == 1) {
    *mask |= bit;
  } else {
    *mask &= ~bit;
  }
}

// fn_10_1A980 (0x0001A980, 0xB0 bytes): the first bit always, the next three up to the first
// that is 0xFFFF (the whole-word compares at 0x0001A9B0/0x0001A9D8/0x0001AA00).
void CDarkSamus::SetAttackBits(uint* mask, uchar set, uint a, uint b, uint c, uint d) {
  SetAttackBit(mask, set, a);
  if (b != 0xFFFF) {
    SetAttackBit(mask, set, b);
    if (c != 0xFFFF) {
      SetAttackBit(mask, set, c);
      if (d != 0xFFFF) {
        SetAttackBit(mask, set, d);
      }
    }
  }
}

// fn_10_1A8F8 (0x0001A8F8, 0x88 bytes): every one of the 24 bits (0x0001A964) except the four
// given (the cmpw chain at 0x0001A930..0x0001A948).
void CDarkSamus::SetAttackBitsExcept(uint* mask, uchar set, uint a, uint b, uint c, uint d) {
  for (int i = 0; i < 24; ++i) {
    const uint bit = 1u << i;
    if (bit != a && bit != b && bit != c && bit != d) {
      SetAttackBit(mask, set, bit);
    }
  }
}

// fn_10_1AA5C (0x0001AA5C, 0x10 bytes): the bit is 2 (the subfic/cntlzw at 0x0001AA5C).
bool CDarkSamus::IsAttackTwo(uint bit) { return bit == 2; }

// fn_10_10BBC (0x00010BBC): the index of the bit (the first i < 24 with 1 << i == bit), else
// 0xFFFF.
int CDarkSamus::GetAttackBitIndex(uint bit) {
  for (int i = 0; i < 24; ++i) {
    if ((1u << i) == bit) {
      return i;
    }
  }
  return 0xFFFF;
}

// fn_10_1A6C4 (0x0001A6C4, 0x234 bytes): the stage info's weight of an attack (the switch's lfs
// offsets: 1 0x44, 4 0x48, 8 0x4C, 0x10 0x50, 0x20 0x54, 0x40 0x5C, 0x80 0x60, 0x100 0x64,
// 0x200 0x3C, 0x400 0x38, 0x10000 0x70, 0x20000 0x74, 0x100000 0x68, 0x200000 0x6C, 0x400000
// 0x7C, 0x800000 0x80); every other id answers lbl_10_rodata_3FC = 0.0f.
float CDarkSamus::GetAttackWeight(const SLdrDSStageInfo& stage, int attack) {
  switch (attack) {
  case 0x1: return stage.meleeAttack;                   // 0x0001A86C
  case 0x4: return stage.meleeDash;                     // 0x0001A874
  case 0x8: return stage.scatterShot;                   // 0x0001A87C
  case 0x10: return stage.movingScatterShot;            // 0x0001A884
  case 0x20: return stage.diveAttack;                   // 0x0001A88C
  case 0x40: return stage.normalMissile;                // 0x0001A894
  case 0x80: return stage.missileJump;                  // 0x0001A89C
  case 0x100: return stage.superMissile;                // 0x0001A8A4
  case 0x200: return stage.taunt;                       // 0x0001A8AC
  case 0x400: return stage.pause;                       // 0x0001A8B4
  case 0x10000: return stage.sweepBeam;                 // 0x0001A8BC
  case 0x20000: return stage.boostBall;                 // 0x0001A8C4
  case 0x100000: return stage.freezeBeamWhenInvisible;  // 0x0001A8CC
  case 0x200000: return stage.normalMissileWhenInvisible; // 0x0001A8D4
  case 0x400000: return stage.phazonAttack;             // 0x0001A8DC
  case 0x800000: return stage.phazonEnrage;             // 0x0001A8E4
  default: return 0.f;                                  // 0x0001A8EC
  }
}

// fn_10_17D50 (0x00017D50, 0x90 bytes): no stage answers false (0x00017D78); 0x800 is the stage's
// lookAround byte (the lbz 0x40 at 0x00017DA0), 2 is false (0x00017D98), every other attack is
// on when its weight is above lbl_10_rodata_3FC = 0.0f (the fcmpo's gt bit, 0x00017DC4).
bool CDarkSamus::IsAttackEnabled(CStateManager& mgr, int attack) const {
  const CDarkSamusBattleStage* stage = GetBattleStage(mgr);
  if (stage == nullptr) {
    return false;
  }
  switch (attack) {
  case 0x800:
    return stage->GetStageInfo().lookAround;
  case 0x2:
    return false;
  default:
    return GetAttackWeight(stage->GetStageInfo(), attack) > 0.f;
  }
}

// fn_10_11308 (0x00011308, 0x2C bytes): the player (the state manager's 0x14FC) is morphed - its
// 0x390 is 0 and its 0x38C is 1 (the subfic/cntlzw at 0x00011324).
bool CDarkSamus::IsPlayerMorphed(CStateManager& mgr) {
  const CPlayer* player = mgr.GetPlayer(0);
  const int state = player->GetSpawnedMorphballState() == 0 // 0x00011310
                        ? static_cast< int >(player->GetMorphballTransitionState())
                        : 0;
  return state == 1;
}

// fn_10_17CEC (0x00017CEC, 0x64 bytes): the distance from the player (0x14FC) to the actor
// (the fsubs of 0x54/0x58/0x5C and Magnitude at 0x00017D3C).
float CDarkSamus::GetDistanceToPlayer(CStateManager& mgr) const {
  return (GetTranslation() - mgr.GetPlayer(0)->GetTranslation()).Magnitude();
}

// fn_10_1AA6C (0x0001AA6C, 0x58 bytes): 0 below the attack range (0x3DC, the bge at 0x0001AA8C),
// 2 above it (0x3E0, the ble at 0x0001AAA0), 1 inside.
int CDarkSamus::GetPlayerRangeBand(CStateManager& mgr) const {
  const float dist = GetDistanceToPlayer(mgr);
  if (dist < mMinAttackRange) {
    return 0;
  }
  if (dist > mMaxAttackRange) {
    return 2;
  }
  return 1;
}

// fn_10_1A5BC (0x0001A5BC, 0x108 bytes): one weight per bit (0x0001A6A8). An attack with no stage
// (0x0001A5F8), the attack 2 (fn_10_1AA5C, 0x0001A610) or one the stage turns off (fn_10_17D50,
// 0x0001A628) gets lbl_10_data_C4C = 0; the others get their stage weight (fn_10_1A6C4, the
// fctiwz at 0x0001A670) and their bit is added to the mask (the add at 0x0001A69C).
void CDarkSamus::CollectStageWeights(CStateManager& mgr, SAttackWeights& weights,
                                     uint* mask) const {
  for (int i = 0; i < 24; ++i) {
    const uint bit = 1u << i;
    if (GetBattleStage(mgr) == nullptr || IsAttackTwo(bit) == true ||
        !IsAttackEnabled(mgr, bit)) {
      weights.weights[weights.count] = 0; // lbl_10_data_C4C
      weights.count++;
    } else {
      weights.weights[weights.count] =
          static_cast< int >(GetAttackWeight(GetBattleStage(mgr)->GetStageInfo(), bit));
      weights.count++;
      *mask += bit;
    }
  }
}

// fn_10_1A554 (0x0001A554, 0x68 bytes): faded (0xDEC == 1, 0x0001A564) the mask keeps only the
// freeze beam, the melee and the phazon attacks (fn_10_1A8F8 with 0x100000, 1, 0x400000,
// 0x800000); visible it loses the freeze beam and the fade missile (0x100000, 0x200000).
void CDarkSamus::CollectFadeAttacks(CStateManager&, SAttackWeights&, uint* mask) const {
  if (mFadeOut == 1) {
    SetAttackBitsExcept(mask, 0, 0x100000, 1, 0x400000, 0x800000);
  } else {
    SetAttackBits(mask, 0, 0x100000, 0x200000, 0xFFFF, 0xFFFF);
  }
}

// fn_10_1A278 (0x0001A278, 0xD4 bytes): a morphed player (fn_10_11308, 0x0001A29C) takes away the
// scatter shots and the dive (8, 0x10, 0x20), the missile jump, the super missile, the taunt and
// the sweep beam (0x80, 0x100, 0x200, 0x10000) and turns the normal missile on (0x40, set 1 at
// 0x0001A2FC); otherwise the melee dash (4) goes.
void CDarkSamus::CollectMorphballAttacks(CStateManager& mgr, SAttackWeights&, uint* mask) const {
  if (IsPlayerMorphed(mgr) == true) {
    SetAttackBits(mask, 0, 0x8, 0x10, 0x20, 0xFFFF);
    SetAttackBits(mask, 0, 0x80, 0x100, 0x200, 0x10000);
    SetAttackBits(mask, 1, 0x40, 0xFFFF, 0xFFFF, 0xFFFF);
  } else {
    SetAttackBits(mask, 0, 0x4, 0xFFFF, 0xFFFF, 0xFFFF);
  }
}

// fn_10_1A440 (0x0001A440, 0x114 bytes): by the range band (fn_10_1AA6C): close (0) keeps only
// the melee, the attack 2 and the look-around (1, 2, 0x800); in range (1) loses the melee; far
// (2) loses 1, 4, 8, 0x10, then 0x20, 0x40, 0x100, then 0x200000, 0x100000.
void CDarkSamus::CollectRangeAttacks(CStateManager& mgr, SAttackWeights&, uint* mask) const {
  switch (GetPlayerRangeBand(mgr)) {
  case 0:
    SetAttackBitsExcept(mask, 0, 0x1, 0x2, 0x800, 0xFFFF);
    break;
  case 1:
    SetAttackBits(mask, 0, 0x1, 0xFFFF, 0xFFFF, 0xFFFF);
    break;
  case 2:
    SetAttackBits(mask, 0, 0x1, 0x4, 0x8, 0x10);
    SetAttackBits(mask, 0, 0x20, 0x40, 0x100, 0xFFFF);
    SetAttackBits(mask, 0, 0x200000, 0x100000, 0xFFFF, 0xFFFF);
    break;
  }
}

// fn_10_1A34C (0x0001A34C, 0xF4 bytes): the finished attacks (0x998, the count at 0x994), newest
// first, lower their weights - the newest by 15, the others by 5 (0x0001A388/0x0001A390) - except
// the melee, the melee dash and the missile jump (1, 4, 0x80: 0x0001A398..0x0001A3A8). A melee as
// the newest one within lbl_10_rodata_544 = 2.5f of 0x9C0 (the fcmpo at 0x0001A3F0) clears the
// melee bit.
void CDarkSamus::CollectHistoryPenalties(CStateManager&, SAttackWeights& weights,
                                         uint* mask) const {
  const int last = static_cast< int >(mAttackHistoryCount) - 1;
  for (int i = last; i >= 0; --i) {
    const int penalty = i == last ? -15 : -5;
    const int attack = static_cast< int >(mAttackHistory[i]);
    if (attack != 1 && attack != 4 && attack != 0x80) {
      weights.weights[GetAttackBitIndex(attack)] += penalty;
    }
    if (attack == 1 && i == last && mLastMeleeTime + 2.5f > mElapsedTime) {
      SetAttackBits(mask, 0, 0x1, 0xFFFF, 0xFFFF, 0xFFFF);
    }
  }
}

// fn_10_1A1D4 (0x0001A1D4, 0xA4 bytes): the look-around (0x800) goes; a player outside
// lbl_10_rodata_4C4 = 0.17453292f (10 degrees) of the facing (fn_10_142F4, 0x0001A234) leaves only
// the melee (fn_10_1A8F8 with 1).
void CDarkSamus::CollectFacingAttacks(CStateManager& mgr, SAttackWeights&, uint* mask) const {
  SetAttackBits(mask, 0, 0x800, 0xFFFF, 0xFFFF, 0xFFFF);
  if (!IsPlayerInFront(mgr, 0.17453292f)) {
    SetAttackBitsExcept(mask, 0, 0x1, 0xFFFF, 0xFFFF, 0xFFFF);
  }
}

// fn_10_19918 (0x00019918, 0x104 bytes): the weighted pick. The masked attacks' weights are
// raised to at least 1 (0x00019968), the others zeroed (0x00019978); with no weight left the pick
// is 0xFFFF (0x0001999C). A draw of the state manager's random (the CRandom16 at 0x16E4,
// Next at 0x000199A8) modulo the total (the divw/mullw at 0x000199AC) walks the positive weights
// until it is used up (the subf. at 0x000199D8) and picks that attack's bit.
int CDarkSamus::PickWeightedAttack(CStateManager& mgr, SAttackWeights& weights, uint mask) const {
  int total = 0;
  for (int i = 0; i < weights.count; ++i) {
    if ((mask & (1u << i)) != 0) {
      if (weights.weights[i] <= 0) {
        weights.weights[i] = 1;
      }
      total += weights.weights[i];
    } else {
      weights.weights[i] = 0;
    }
  }
  if (total == 0) {
    return 0xFFFF;
  }
  int draw = mgr.Random()->Next() % total;
  for (int i = 0; i < weights.count; ++i) {
    if (weights.weights[i] > 0) {
      draw -= weights.weights[i];
      if (draw <= 0) {
        return 1 << i;
      }
    }
  }
  return 0xFFFF;
}

// fn_10_1905C (0x0001905C, 0x3C4 bytes): the flat direction to the player (the z cleared at
// 0x000190F8; not normalizable answers 0, 0x00019154). The rays start lbl_10_rodata_414 = 8.0f
// short of the player (at least at the actor, the bge at 0x0001911C) and are
// lbl_10_rodata_40C = 5.0f longer than the player's distance; one per height of
// lbl_10_rodata_334 = {1.0f, 2.5f, 4.0f} (the cmpwi 3 at 0x00019318), filtered to include
// only the material lbl_10_data_C50 = 0x31 (the __shl2i at 0x00019170, the filter type 1 at
// 0x000191C8). The nearest hit (from the unshifted start) that is not 8 short of the player
// (the blt at 0x000192F0) is kept (lbl_10_rodata_3EC = -1.0f = none, 0x00019300); none is 0
// (0x0001932C), short of the player 1 (0x0001933C), else 2.
int CDarkSamus::ClassifyLineToPlayer(CStateManager& mgr) const {
  CVector3f dir = mgr.GetPlayer(0)->GetTranslation() - GetTranslation();
  dir.SetZ(0.f);
  const float dist = dir.Magnitude();
  float back = dist - 8.f;
  if (back < 0.f) { // 0x0001911C
    back = 0.f;
  }
  const float length = (5.f + dist) - back;
  if (dir.CanBeNormalized() != true) { // 0x00019144
    return 0;
  }
  dir.Normalize();
  const CMaterialFilter filter =
      CMaterialFilter::MakeInclude(CMaterialList(static_cast< EMaterialTypes >(0x31)));
  static const float heights[3] = {1.f, 2.5f, 4.f}; // lbl_10_rodata_334
  float best = -1.f;
  for (int i = 0; i < 3; ++i) {
    const CVector3f origin = GetTranslation() + CVector3f(0.f, 0.f, heights[i]);
    const CVector3f start = origin + back * dir;
    rstl::reserved_vector< TUniqueId, 1024 > nearList;
    mgr.BuildNearList(nearList, start, dir.AsNormalized(), length, filter, nullptr);
    TUniqueId hitId = kInvalidUniqueId;
    const CRayCastResult result =
        mgr.RayWorldIntersection(hitId, start, dir, length, filter, nearList);
    if (result.IsValid() == true) { // 0x000192A0
      const float hitDist = (result.GetPoint() - origin).Magnitude();
      if (!(hitDist < dist - 8.f) && (best < 0.f || hitDist < best)) { // 0x000192F0..0x00019308
        best = hitDist;
      }
    }
  }
  if (best < 0.f) { // 0x0001932C
    return 0;
  }
  if (best < dist) { // 0x0001933C
    return 1;
  }
  return 2;
}

// fn_10_19A1C (0x00019A1C, 0x148 bytes): a repulsion point (fn_10_1D9A4, any but the zero vector
// - the fcmpu chain at 0x00019A70..0x00019A8C) leaves only the melee (fn_10_1A8F8 with 1,
// 0x00019AC0). Otherwise the line to the player (fn_10_1905C, 0x00019AD0): blocked short of it
// (1) adds 10 to the normal and the super missile's weights (0x40, 0x100: 0x00019AEC..), blocked
// beyond it (2) adds 10 to the dive attack's (0x20, 0x00019B28).
void CDarkSamus::CollectLineAttacks(CStateManager& mgr, SAttackWeights& weights,
                                    uint* mask) const {
  const CVector3f point = GetRepulsionPoint(mgr, nullptr);
  if (!(point.GetX() == CVector3f::Zero().GetX() && point.GetY() == CVector3f::Zero().GetY() &&
        point.GetZ() == CVector3f::Zero().GetZ())) {
    SetAttackBitsExcept(mask, 0, 0x1, 0xFFFF, 0xFFFF, 0xFFFF);
    return;
  }
  switch (ClassifyLineToPlayer(mgr)) {
  case 1:
    weights.weights[GetAttackBitIndex(0x40)] += 10;  // 0x00019B04
    weights.weights[GetAttackBitIndex(0x100)] += 10; // 0x00019B1C
    break;
  case 2:
    weights.weights[GetAttackBitIndex(0x20)] += 10; // 0x00019B3C
    break;
  }
}

// fn_10_14648 (0x00014648, 0x2F4 bytes): no path area answers false (0x000146E0). The regions
// at the player (0.5 up, lbl_10_rodata_3E4, in the area's frame) are searched for the first
// obstructed one (the obstruction 2 count above 0, 0x000147C4) that holds the point (0x000147DC);
// no region at all makes "off the mesh" answer true (0x0001479C). From the player back to the
// actor in lbl_10_rodata_428 = 0.2f steps (t from 1, 0x00014918), every point 0.5 up that is not
// inside that region (the world-space point against the region, 0x000148A4) must be on the
// mesh (an off-mesh point answers the "no region" flag, 0x000148D8) within lbl_10_rodata_40C =
// 5.0f of the actor's own height (0x000148FC/0x00014908).
bool CDarkSamus::CanWalkToPlayer(CStateManager& mgr) const {
  const CVector3f playerPos = mgr.GetPlayer(0)->GetTranslation();
  CPFRegion* inside = nullptr;
  bool noRegion = false;
  CPFArea* area =
      mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea;
  if (area == nullptr) { // 0x000146DC
    return false;
  }
  const CTransform4f& areaXf = area->GetTransform();
  const CVector3f local =
      areaXf.TransposeRotate(playerPos + CVector3f(0.f, 0.f, 0.5f) - areaXf.GetTranslation());
  rstl::reserved_vector< CPFRegion*, 8 > regions;
  const uint creatureMask = mPathFindSearch.GetCreatureMask();
  area->FindRegions(regions, local, mPathFindSearch.GetRegionFlags(), creatureMask, true);
  if (regions.size() == 0) { // 0x00014794
    noRegion = true;
  } else {
    for (int i = 0; i < regions.size(); ++i) {
      CPFRegion* region = regions[i];
      if (region->GetObstructionCount(kPFO_Unknown2) > 0 &&
          region->IsPointInside(local) == true) { // 0x000147C4/0x000147DC
        inside = region;
        break;
      }
    }
  }
  float height = GetTranslation().GetZ();
  mPathFindSearch.GetHeightOfPointAboveMesh(GetTranslation(), height, 0.f); // 0x0001481C
  for (float t = 1.f; t > 0.f; t -= 0.2f) {
    const CVector3f point = (1.f - t) * GetTranslation() + t * playerPos + CVector3f(0.f, 0.f, 0.5f);
    if (inside == nullptr || inside->IsPointInside(point) != true) {
      float pointHeight = 0.f;
      if (mPathFindSearch.GetHeightOfPointAboveMesh(point, pointHeight, 0.f) !=
          CPathFindSearch::kR_Success) { // 0x000148D8
        return noRegion;
      }
      if (pointHeight > 5.f + height || pointHeight < height - 5.f) { // 0x000148FC/0x00014908
        return false;
      }
    }
  }
  return true;
}
