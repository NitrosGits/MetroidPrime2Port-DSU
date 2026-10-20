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
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/StateMachineCommon.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"

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

bool CDarkSamus::CanMissileDeflect(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_14284", "trigger CanMissileDeflect");
  return false;
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

bool CDarkSamus::PickedBoostBall(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_116F8", "trigger PickedBoostBall");
  return false;
}

bool CDarkSamus::PickedFreezeBeam(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_11760", "trigger PickedFreezeBeam");
  return false;
}

bool CDarkSamus::PickedDiveAttack(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_1174C", "trigger PickedDiveAttack");
  return false;
}

bool CDarkSamus::PickedMelee(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_117D8", "trigger PickedMelee");
  return false;
}

bool CDarkSamus::PickedMeleeDash(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_11710", "trigger PickedMeleeDash");
  return false;
}

bool CDarkSamus::PickedMissileJump(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_1178C", "trigger PickedMissileJump");
  return false;
}

bool CDarkSamus::PickedMovingSShot(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_11724", "trigger PickedMovingSShot");
  return false;
}

bool CDarkSamus::PickedNormalMissile(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_117A0", "trigger PickedNormalMissile");
  return false;
}

bool CDarkSamus::PickedPause(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_11800", "trigger PickedPause");
  return false;
}

bool CDarkSamus::PickedPhazonAttack(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_8680", "trigger PickedPhazonAttack");
  return false;
}

bool CDarkSamus::PickedPhazonEnrage(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_8114", "trigger PickedPhazonEnrage");
  return false;
}

bool CDarkSamus::PickedScatterShot(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_11738", "trigger PickedScatterShot");
  return false;
}

bool CDarkSamus::PickedSuperMissile(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_11778", "trigger PickedSuperMissile");
  return false;
}

bool CDarkSamus::PickedSweepBeam(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_CDA0", "trigger PickedSweepBeam");
  return false;
}

bool CDarkSamus::PickedTaunt(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_117EC", "trigger PickedTaunt");
  return false;
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

bool CDarkSamus::ShouldFadeOut(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_BBE4", "trigger ShouldFadeOut");
  return false;
}

bool CDarkSamus::ShouldGetPissedOff(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_FD30", "trigger ShouldGetPissedOff");
  return false;
}

bool CDarkSamus::ShouldLeaveSphere(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_6A60", "trigger ShouldLeaveSphere");
  return false;
}

bool CDarkSamus::ShouldReact(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_11158", "trigger ShouldReact");
  return false;
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

bool CDarkSamus::TooMuchDamage(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_10698", "trigger TooMuchDamage");
  return false;
}

bool CDarkSamus::WallCollision(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_F800", "trigger WallCollision");
  return false;
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

void CDarkSamus::TravelToCoverPoints(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_1DF74", "state TravelToCoverPoints");
  DsStateEntered("TravelToCoverPoints", msg);
}

void CDarkSamus::TravelToPissedOff(CStateManager&, EStateMsg msg, float) {
  FsmStandIn("fn_10_FB8C", "state TravelToPissedOff");
  DsStateEntered("TravelToPissedOff", msg);
}

// fn_10_FE18: the TurnToFacePlayer state.
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
    // fn_10_8698: the taunt command. The guest's byte at 0x6B4 is the pending flag and 0x10 the
    // animation state the retry asks the body controller for (kAS_Taunt).
    switch (msg) {
    case kStateMsg_Activate:
      mTauntPending = 1;
      BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(pas::kTT_Six));
      break;
    case kStateMsg_Update:
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
        BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(pas::kTT_Six));
      }
      break;
    case kStateMsg_Deactivate:
      mTauntPending = 0;
      break;
    }
    // 0x0000FEEC: fn_801519A4, the base's AnimOver - the animation over ends the turn.
    if (AnimOver(mgr, CTriggerData(0))) {
      mTurnDone = 1;
    }
  }
}
