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
#include "MetroidPrime/CGameCollision.hpp"
#include "WorldFormat/CMetroidAreaCollider.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/CRandom16.hpp"
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
#include "MetroidPrime/Weapons/CEnergyProjectile.hpp"
#include "MetroidPrime/Weapons/CProjectileInfo.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Math/CAABox.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CAnimPlaybackParms.hpp"
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

// fn_10_17E90 (0x00017E90, 0x74 bytes): the health left in percent: lbl_10_rodata_3F0 = 100.0f
// times the health info's +4 (the health) over its +0 (the initial health), both through the
// vtable slot 0x3C (0x00017EB0/0x00017EC8).
// @ fn_10_17E90
float CDarkSamus::GetHealthPercent() const {
  const CHealthInfo* health = GetHealthInfo();
  return 100.f * (health->GetHP() / health->GetInitialHP());
}

// fn_10_194AC (0x000194AC, 0xA0 bytes): the stage after the current one: the stage object's first
// kSS_Connect link, any message (FindConnectedObject at 0x00019508). No stage object (0x000194E0)
// or no link (0x0001951C) answers the current stage's uid 0x974.
// @ fn_10_194AC
TUniqueId CDarkSamus::GetNextBattleStage(CStateManager& mgr) const {
  const CDarkSamusBattleStage* stage = GetBattleStage(mgr);
  if (stage == nullptr) { // 0x000194E0
    return mBattleStageUid;
  }
  const TUniqueId next = stage->FindConnectedObject(mgr, kSS_Connect, kSM_None);
  if (next != kInvalidUniqueId) {
    return next;
  }
  return mBattleStageUid;
}

// fn_10_B86C (0x0000B86C, 0x1CC bytes): walks the AI hint chains the actor links with
// kSS_InternalState6 (FindConnectedObjects at 0x0000B89C, any message). A chain starts at a hint of
// type 0x11 (0x158, `cmpwi r0, 0x11` at 0x0000B8E4) and each hint on it is activated (the vtable
// slot 0x1C, SetActive(true), at 0x0000B908) before the next is looked for among its kSS_Arrived
// links (0x0000B928): the first that is a hint. An active next hint whose 0x15C (the radius) is
// lbl_10_rodata_25C = 0.0f counts up (0x0000B99C), an inactive one resets the count (0x0000B9A4);
// four (`cmpwi r29, 0x4` at 0x0000B9CC) or no next hint ends the chain.
void CDarkSamus::ActivateHintChains(CStateManager& mgr) {
  const rstl::vector< TUniqueId > starts =
      FindConnectedObjects(mgr, kSS_InternalState6, kSM_None);
  for (rstl::vector< TUniqueId >::const_iterator it = starts.begin(); it != starts.end(); ++it) {
    CScriptAIHint* hint = TCastToPtr< CScriptAIHint >(mgr.ObjectById(*it));
    if (hint == nullptr ||                                   // 0x0000B8DC
        static_cast< int >(hint->GetHintType()) != 0x11) {   // 0x0000B8E8
      continue;
    }
    int count = 0;
    while (hint != nullptr) { // 0x0000B9F4
      hint->SetActive(true);
      CScriptAIHint* next = nullptr;
      const rstl::vector< TUniqueId > arrived =
          hint->FindConnectedObjects(mgr, kSS_Arrived, kSM_None);
      for (rstl::vector< TUniqueId >::const_iterator a = arrived.begin(); a != arrived.end();
           ++a) {
        CScriptAIHint* other = TCastToPtr< CScriptAIHint >(mgr.ObjectById(*a));
        if (other == nullptr) { // 0x0000B968
          continue;
        }
        if (other->GetActive() == true) {
          if (0.f == other->GetRadius()) {
            ++count;
          }
        } else {
          count = 0;
        }
        next = other;
        break;
      }
      if (count >= 4) {
        break;
      }
      hint = next;
    }
  }
}

// fn_10_193A0 (0x000193A0, 0x10C bytes): the stage advance. A stage (fn_10_17E14, 0x000193C4) with
// an advance threshold above lbl_10_rodata_3FC = 0.0f (its info's +0x0, 0x000193DC) that the
// health percent (fn_10_17E90) has fallen below (`bge` at 0x000193F0) gives way to the next stage
// (fn_10_194AC; the uid to 0x974 at 0x00019410, 0x978 counts up at 0x0001941C), and a next stage
// whose info's +0x35 is 1 (0x00019440) wakes the hint chains (fn_10_B86C). Then, with a stage
// (0x0001945C), 0x97C takes its flags: 0x80 = the info's +0x34 (usesAlternateScannableInfo),
// 0x40 = its +0x32 is not 1 (the subfic/subi/or at 0x0001947C).
// @ fn_10_193A0
void CDarkSamus::AdvanceBattleStage(CStateManager& mgr) {
  const CDarkSamusBattleStage* stage = GetBattleStage(mgr);
  if (stage != nullptr && stage->GetStageInfo().unknown_0xb5af7831 > 0.f && // 0x000193C8
      GetHealthPercent() < stage->GetStageInfo().unknown_0xb5af7831) {
    mBattleStageUid = GetNextBattleStage(mgr);
    ++mStageCount;
    if (GetBattleStage(mgr) != nullptr && // 0x00019428
        GetBattleStage(mgr)->GetStageInfo().unknown_0xaa85c885 == true) {
      ActivateHintChains(mgr);
    }
  }
  if (const CDarkSamusBattleStage* now = GetBattleStage(mgr)) {
    mUseAltScan = now->GetStageInfo().usesAlternateScannableInfo;
    mCanDie = now->GetStageInfo().unknown_0xc96b8223 != true;
  }
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

// fn_10_149B4 (0x000149B4, 0x4C bytes): the dash record's flag 0x80 (0xA08's
// `extrwi r0, r0, 1, 24` at 0x000149C4) breaks the dash; otherwise not while 0xA04 is above 0 (`cmpwi; ble` at
// 0x000149DC), else the base's GetAnimOver (0x000149EC).
// @ fn_10_149B4
bool CDarkSamus::BreakDash(CStateManager& mgr, const CTriggerData& data) const {
  if (mDash.mBlocked == true) {
    return true;
  }
  if (mDash.mStepsLeft > 0) {
    return false;
  }
  return GetAnimOver(mgr, data);
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

// fn_10_FAF4 (0x0000FAF4, 0x98 bytes): no area hint of type 0xE (fn_10_FC64, 0x0000FB14) is
// "reached"; otherwise the flat distance (z = lbl_10_rodata_3FC = 0.0f, 0x0000FB5C) from the hint
// to the actor below lbl_10_rodata_438 = 3.0f (the fcmpo at 0x0000FB6C, strict).
// @ fn_10_FAF4
bool CDarkSamus::ReachedPissedOff(CStateManager& mgr, const CTriggerData&) const {
  const CScriptAIHint* hint = FindAreaHint(mgr, 0xE);
  if (hint == nullptr) { // 0x0000FB14
    return true;
  }
  const CVector3f delta = GetTranslation() - hint->GetTranslation();
  return CVector3f(delta.GetX(), delta.GetY(), 0.f).Magnitude() < 3.f;
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

// fn_10_150DC (0x000150DC, 0x26C bytes): the Dash state. Activate (`mr. r31, r5` at 0x000150EC)
// clears the double-dash flag (0x40, 0x00015110) and sets it when the stage (fn_10_17E14, the null
// test at 0x00015120) rolls Range(0.0f, lbl_10_rodata_3F0 = 100.0f) below its chanceToDoubleDash
// (the stage info's +0x20, `fcmpo; bge` at 0x00015144). A double dash is fn_10_14D14 (0x00015178).
// Otherwise Activate (0x00015184) starts from a fresh dash record (fn_10_15348 on the stack, copied
// in at 0x000151A0..0x000151F0), pushes the best cover point (fn_10_1E3C0 / fn_10_1E26C at
// 0x000151F4/0x00015208) and picks a dash (fn_10_154FC at 0x00015298, eight weights of
// lbl_10_rodata_3E8 = 1.0f) toward the cover point (GetCoverPoint of 0xC24 at 0x0001524C; without
// one the actor's own position); a pick of weight 0.0f blocks the dash (0x80 at 0x000152B4) and
// runs the collision failsafe (0x000152C4). Then the stuck count as in fn_10_14FE0
// (0x000152C8..0x00015308). Every message, unless blocked (`extrwi.` at 0x00015310), runs the dash
// step (fn_10_12CE0 at 0x00015324).
void CDarkSamus::Dash(CStateManager& mgr, EStateMsg msg, float) {
  DsStateEntered("Dash", msg);
  if (msg == kStateMsg_Activate) {
    mDash.mDoubleDash = false;
    const CDarkSamusBattleStage* stage = GetBattleStage(mgr);
    if (stage != nullptr && // `mr. r28, r3; beq` at 0x0001511C/0x00015120
        mgr.Random()->Range(0.f, 100.f) < stage->GetStageInfo().chanceToDoubleDash) {
      mDash.mDoubleDash = true;
    }
  }
  if (mDash.mDoubleDash == true) {
    UpdateDoubleDash(mgr, msg);
    return;
  }
  if (msg == kStateMsg_Activate) {
    mDash = CDarkSamusDash();
    PushAttacker(FindBestCoverPoint(mgr));
    float weights[8] = {1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f};
    CVector3f target = GetTranslation();
    if (const CScriptCoverPoint* cover = GetCoverPoint(mgr, mLastAttackerUid)) {
      target = cover->GetTranslation();
    }
    if (0.f == PickDash(mgr, weights, &target)) {
      mDash.mBlocked = true;
      RunCollisionFailsafe(mgr);
    }
    if (mStillTime > 1.2f) {
      ++mDashStuckCount;
      if (mDashStuckCount >= 2) {
        ApplyDashStuckBlast(mgr);
      }
    } else {
      mDashStuckCount = 0;
    }
  }
  if (mDash.mBlocked == false) {
    UpdateDashStep(mgr, msg);
  }
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

// fn_10_11AD8 (0x00011AD8, 0x17C bytes): the MovingScatterShot state, the scatter shot fired
// from a side step. Activate (0x00011B2C) clears the start time (0xBA4 = lbl_10_rodata_3FC = 0.0f),
// the firing byte 0xBAD and the shot count 0xBA0 and forgets the projectile (0xBE4 =
// kInvalidUniqueId); Deactivate (0x00011B54) only forgets the projectile. Every message then turns
// toward the player at lbl_10_rodata_434 = 1.7453293f rad/s (fn_10_16B64, 0x00011B78), runs the fire
// timer (fn_10_11974, 0x00011B88) and the attack bookkeeping with id 0x10 (`li r4, 0x10`,
// fn_10_1B478 at 0x00011B9C), and last the step command (fn_10_21DF8 at 0x00011C14,
// pas::kAS_Step = `li r5, 0x3`): a break-dodge step (`li r7, 0x2`) to the right (`li r9, 0x3`)
// unless the side byte 0xBAC is set (`cmplwi r0, 0x0` at 0x00011BA8), then to the left (0x2).
// The shots start from DoUserAnimEvent's kUE_EventStart arm with the side 0xBAC (0x0001C264).
// @ fn_10_11AD8
void CDarkSamus::MovingScatterShot(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("MovingScatterShot", msg);
  switch (msg) {
  case kStateMsg_Activate:
    mScatterStartTime = 0.f;
    mScatterFiring = 0;
    mScatterShotCount = 0;
    mScatterProjectileUid = kInvalidUniqueId;
    break;
  case kStateMsg_Update:
    break;
  case kStateMsg_Deactivate:
    mScatterProjectileUid = kInvalidUniqueId;
    break;
  }
  TurnTowardPlayer(mgr, dt, 1.7453293f);
  UpdateScatterShotFire(mgr, dt);
  UpdateCurrentAttack(0x10, mgr, msg);
  DoStepCmd(msg, pas::kAS_Step,
            CBCStepCmd(mScatterSide != 0 ? pas::kSD_Left : pas::kSD_Right, pas::kStep_BreakDodge));
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

// fn_10_F930 (0x0000F930, 0x1C4 bytes): the PissedOff state. Activate (`mr. r31, r5` at
// 0x0000F950) clears 0xD59 and 0xD58 (0x0000F96C/0x0000F97C), sends kSS_InternalState0 ('IS00',
// 0x0000F994) and puts the actor on the area hint of type 0xE (fn_10_FC64, SetTranslation at
// 0x0000F9B8); Deactivate (0x0000F9C4) sends kSS_InternalState1 ('IS01', 0x0000F9EC). Every
// message runs the attack bookkeeping with 0x1000 (fn_10_1B478, 0x0000FA00), the shield flag
// (fn_10_105F4, 0x0000FA14) and the ground collision (fn_10_18F90, 0x0000FA24), turns toward the
// hint of type 0xF at lbl_10_rodata_430 = 5.2359877f rad/s (fn_10_16B8C, 0x0000FA58) or, with
// none, toward the player at lbl_10_rodata_434 = 1.7453293f (fn_10_16B64, 0x0000FA78), and runs
// the taunt command with taunt type 1 (fn_10_8698 with pas::kAS_Taunt = 0x10, the command
// {0x15 = kBSC_Taunt, 1} at 0x0000FA88..0x0000FAB4; see TurnToFacePlayer for the host form).
// @ fn_10_F930
void CDarkSamus::PissedOff(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("PissedOff", msg);
  if (msg == kStateMsg_Activate) {
    mStageChanged = 0;
    mPissedOff = 0;
    SendScriptMsgs(kSS_InternalState0, mgr, kInvalidUniqueId, kSM_None);
    if (const CScriptAIHint* hint = FindAreaHint(mgr, 0xE)) { // 0x0000F9AC
      SetTranslation(hint->GetTranslation());
    }
  } else if (msg == kStateMsg_Deactivate) {
    SendScriptMsgs(kSS_InternalState1, mgr, kInvalidUniqueId, kSM_None);
  }
  UpdateCurrentAttack(0x1000, mgr, msg);
  UpdateShieldFlag(msg);
  UpdateGroundCollision(mgr, msg);
  if (const CScriptAIHint* face = FindAreaHint(mgr, 0xF)) { // 0x0000FA3C
    TurnTowardPoint(dt, face->GetTranslation(), 5.2359877f);
  } else {
    TurnTowardPlayer(mgr, dt, 1.7453293f);
  }
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(pas::kTT_One));
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
      BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(pas::kTT_One));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  }
}

// fn_10_FC64 (0x0000FC64, 0xCC bytes): the first area hint of `type` - the AI waypoint list (the
// state manager's 0x838, its first index at 0x2008) is walked for an active (the byte at 0x20's
// bit 7, 0x0000FCC4) CScriptAIHint (the cast at 0x0000FCA0) in the actor's area (0x0000FCD4)
// whose hint type (0x158) is `type` (0x0000FCE0). None answers null (0x0000FD0C).
// @ fn_10_FC64
const CScriptAIHint* CDarkSamus::FindAreaHint(CStateManager& mgr, int type) const {
  CObjectList& list = mgr.ObjectListById(kOL_AiWaypoint);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CScriptAIHint* hint = TCastToPtr< CScriptAIHint >(list[i]);
    if (hint == nullptr || hint->GetActive() != true || // 0x0000FCA0/0x0000FCC4
        hint->GetCurrentAreaId() != GetCurrentAreaId()) {
      continue;
    }
    if (static_cast< int >(hint->GetHintType()) == type) {
      return hint;
    }
  }
  return nullptr;
}

// fn_10_105F4 (0x000105F4, 0x40 bytes): Activate sets the 0xCD0 byte (0x00010604) and copies
// lbl_10_bss_128 (a zero vector) to 0xD18 (0x0001060C..0x00010618); Deactivate (the `cmpwi r4,
// 0x2` at 0x00010620) clears the byte (0x0001062C). The byte picks the shielded collision response
// (fn_10_F84C) and the 0xCD4 vulnerability (fn_10_100DC, still a stand-in).
// @ fn_10_105F4
void CDarkSamus::UpdateShieldFlag(EStateMsg msg) {
  if (msg == kStateMsg_Activate) {
    mGuestCD0 = 1;
    mGuestD18 = CVector3f::Zero();
  } else if (msg == kStateMsg_Deactivate) {
    mGuestCD0 = 0;
  }
}

// fn_10_18F90 (0x00018F90, 0x3C bytes): Activate (0x00018FA0) runs fn_10_19014 - vertical
// movement on, off the ground and no kMT_GroundCollider (0x0001902C/0x00019040/0x00019048);
// Deactivate (0x00018FB0) runs fn_10_18FCC with 1 - vertical movement off, on the ground and the
// material back (0x00018FE0/0x00018FF8/0x00019000).
// @ fn_10_18F90
void CDarkSamus::UpdateGroundCollision(CStateManager& mgr, EStateMsg msg) {
  if (msg == kStateMsg_Activate) {
    mVerticalMovement = true;
    mOnGround = false;
    RemoveMaterial(kMT_GroundCollider, mgr);
  } else if (msg == kStateMsg_Deactivate) {
    mVerticalMovement = false;
    mOnGround = true;
    AddMaterial(kMT_GroundCollider, mgr);
  }
}

// fn_10_FD4C (0x0000FD4C, 0xCC bytes): a stage count (fn_10_17E48, 0x978) other than the one last
// seen (0xD54, the beq at 0x0000FD74) clears 0xD59 (0x0000FD80) and reads the last seen stage
// (0xD50): its info's +0x84 (unknown_0x911a2476, 0x0000FDB4) set gets the actor pissed off (0xD58
// = 1, 0x0000FDBC), clear - or no stage (0x0000FD9C) or no info (0x0000FDA8, the accessor is a
// member address and is never null) - sets 0xD59 (0x0000FDC8/0x0000FDEC). The current stage and
// count become the seen ones (0x0000FDD4..0x0000FDDC, 0x0000FDF4..0x0000FDFC).
// @ fn_10_FD4C
void CDarkSamus::WatchStageChange(CStateManager& mgr) {
  if (mStageCount == mSeenStageCount) {
    return;
  }
  mStageChanged = 0;
  const CDarkSamusBattleStage* seen =
      TCastToConstPtr< CDarkSamusBattleStage >(mgr.GetObjectById(mSeenStageUid));
  if (seen != nullptr) { // 0x0000FD9C
    if (seen->GetStageInfo().unknown_0x911a2476 == true) {
      mPissedOff = 1;
    } else {
      mStageChanged = 1;
    }
  } else {
    mStageChanged = 1;
  }
  mSeenStageUid = mBattleStageUid;
  mSeenStageCount = mStageCount;
}

// fn_10_175AC (0x000175AC, 0x1B4 bytes): the ScatterShot state. Every message runs the aim
// points (fn_10_10520, 0x000175DC) and the attack bookkeeping with the scatter shot's id 8
// (`li r4, 0x8`, fn_10_1B478 at 0x000175F0) first. Activate (0x00017618) clears the delay
// (0xBA4 = lbl_10_rodata_3FC = 0.0f), the firing byte 0xBAD and the shot count 0xBA0, forgets the
// projectile (0xBE4 = kInvalidUniqueId) and runs the collision failsafe (fn_10_1AEC0,
// 0x00017648). Update (0x00017650) turns toward the player at lbl_10_rodata_434 = 1.7453293f
// rad/s (fn_10_16B64) and runs the fire timer (fn_10_11974). Deactivate (0x00017680) forgets the
// projectile and runs the failsafe. Last, for every message, the projectile-attack command
// (fn_10_21F14 at 0x00017720, pas::kAS_ProjectileAttack = `li r5, 0x12`) with severity 1
// (`li r7, 0x1`), the target lbl_10_rodata_3E4 = 0.5f times the sum of the line's ends (0xB7C +
// 0xB94, 0x000176B8..0x000176E8) and no blend (`li r0, 0x0`). The state ends on the FSM's
// AnimOver, which the command's animation state drives.
void CDarkSamus::ScatterShot(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("ScatterShot", msg);
  UpdateAttackAimPoints(mgr, msg);
  UpdateCurrentAttack(8, mgr, msg);
  switch (msg) {
  case kStateMsg_Activate:
    mScatterStartTime = 0.f;
    mScatterFiring = 0;
    mScatterShotCount = 0;
    mScatterProjectileUid = kInvalidUniqueId;
    RunCollisionFailsafe(mgr);
    break;
  case kStateMsg_Update:
    TurnTowardPlayer(mgr, dt, 1.7453293f);
    UpdateScatterShotFire(mgr, dt);
    break;
  case kStateMsg_Deactivate:
    mScatterProjectileUid = kInvalidUniqueId;
    RunCollisionFailsafe(mgr);
    break;
  }
  DoProjectileAttackCmd(msg, pas::kAS_ProjectileAttack,
                        CBCProjectileAttackCmd(pas::kS_One,
                                               0.5f * (mScatterLineStart + mScatterLineEnd),
                                               false));
}

// fn_10_EDB8 (0x0000EDB8, 0x14 bytes): the side dash's weight of a slot, lbl_10_rodata_3BC[slot]:
// 0.6f, 0.7f, 1.0f, 1.0f, 0.8f, 0.8f, 0.9f, 0.9f (slots 2/3 first, the forward slots 0/1 last).
static float GetSideDashWeight(int slot) {
  static const float kWeights[8] = {0.6f, 0.7f, 1.f, 1.f, 0.8f, 0.8f, 0.9f, 0.9f};
  return kWeights[slot];
}

// fn_10_153EC (0x000153EC, 0x110 bytes): the SideDash state. Activate (`mr. r31, r5` at
// 0x000153FC) clears the dash record in place - the start and the destination to the zero vector,
// both slots to -1, the weight to lbl_10_rodata_3FC = 0.0f and the 0x80 / 0x40 flags (0x0001540C..
// 0x00015478; the steps-left word 0xA04 is left alone) - and picks a dash with no target (fn_10_154FC
// at 0x000154A8, `li r6, 0x0`) over fn_10_EDB8's weights (the loop at 0x0001547C); a pick of weight
// 0.0f (`fcmpu` at 0x000154B4) blocks the dash (0x80 at 0x000154C4). Every message, unless blocked
// (`extrwi.` at 0x000154D0), runs the dash step (fn_10_12CE0 at 0x000154E4).
void CDarkSamus::SideDash(CStateManager& mgr, EStateMsg msg, float) {
  DsStateEntered("SideDash", msg);
  if (msg == kStateMsg_Activate) {
    mDash.mStartPosition = CVector3f::Zero();
    mDash.mDestination = mDash.mStartPosition;
    mDash.mLastSlot = -1;
    mDash.mSlot = -1;
    mDash.mWeight = 0.f;
    mDash.mBlocked = false;
    mDash.mDoubleDash = false;
    float weights[8];
    for (int i = 0; i < 8; ++i) {
      weights[i] = GetSideDashWeight(i);
    }
    if (0.f == PickDash(mgr, weights, nullptr)) {
      mDash.mBlocked = true;
    }
  }
  if (mDash.mBlocked == false) {
    UpdateDashStep(mgr, msg);
  }
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
    mDash.mLastSlot = -1;
  } else {
    mPathFindNavigation.SetFaceTarget(mgr.GetPlayer(0)->GetUniqueId());
    mPathFindNavigation.PathFind(mgr, msg, dt, *this);
    Separate(mgr);
  }
  AccumulateDamage(msg, dt);
  UpdateGlideEmitter(msg);
  UpdateStillTimer(msg, dt);
}

// fn_10_FB8C (0x0000FB8C, 0xD8 bytes): the TravelToPissedOff state: the attack bookkeeping with
// 0x2000 (fn_10_1B478, 0x0000FBC4), the shield flag (fn_10_105F4, 0x0000FBD8), then the path to
// the area hint of type 0xE (fn_10_FC64, 0x0000FBE8): Activate (0x0000FBF8) sets the destination
// (fn_10_1C778, 0x0000FC0C), every other message faces the hint (the uid to the navigation's face
// target 0x718, 0x0000FC24) and runs the path find (0x0000FC30). No hint skips the path. Last the
// still timer (fn_10_1D794, 0x0000FC40).
// @ fn_10_FB8C
void CDarkSamus::TravelToPissedOff(CStateManager& mgr, EStateMsg msg, float dt) {
  DsStateEntered("TravelToPissedOff", msg);
  UpdateCurrentAttack(0x2000, mgr, msg);
  UpdateShieldFlag(msg);
  if (const CScriptAIHint* hint = FindAreaHint(mgr, 0xE)) {
    if (msg == kStateMsg_Activate) {
      SetPathFindDestination(mgr, hint->GetTranslation(), dt);
    } else {
      mPathFindNavigation.SetFaceTarget(hint->GetUniqueId());
      mPathFindNavigation.PathFind(mgr, msg, dt, *this);
    }
  }
  UpdateStillTimer(msg, dt);
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

// fn_10_EDE0 / fn_10_EB90 are defined with the dash below.
static pas::EStepDirection GetDashStepDirection(int slot);
static CVector3f GetDashDirection(uint slot, const CVector3f& f);

// fn_10_164C0 (0x000164C0, 0x140 bytes): is the line from the gun locator (0x8CC's translation,
// 0x8D8/0x8E8/0x8F8) plus offset to target clear? A static ray (RayStaticIntersection at
// 0x000165C8) over the whole line, the filter including lbl_10_data_C60 / C64 (0x3B kMT_Solid,
// 0x30 kMT_AIBlock; type 1 at 0x00016550 = include only); fn_10_2C8 is the module's copy of the
// CRayCastResult copy constructor, and the answer is the result not valid (0x000165DC).
// @ fn_10_164C0
bool CDarkSamus::IsGunLineClear(CStateManager& mgr, const CVector3f& offset,
                                const CVector3f& target) const {
  const CVector3f start = mGunLctrXf.GetTranslation() + offset;
  const CVector3f delta = target - start;
  const float length = delta.Magnitude();
  const CRayCastResult result = mgr.RayStaticIntersection(
      start, delta.AsNormalized(), length,
      CMaterialFilter::MakeInclude(CMaterialList(kMT_Solid, kMT_AIBlock)));
  return !result.IsValid();
}

// fn_10_16418 (0x00016418, 0xA8 bytes): the gun line (fn_10_164C0 at 0x000164A4) raised by
// lbl_10_rodata_40C = 5.0f in z to the player's aim position (the vtable call at 0x00016458,
// slot 0x54 = GetAimPosition, dt lbl_10_rodata_3FC = 0.0f).
// @ fn_10_16418
bool CDarkSamus::IsPlayerAimLineClear(CStateManager& mgr) const {
  const CVector3f aim = mgr.GetPlayer(0)->GetAimPosition(mgr, 0.f);
  return IsGunLineClear(mgr, CVector3f(0.f, 0.f, 5.f), aim);
}

// fn_10_16984 (0x00016984, 0x158 bytes): the ring ray's world-space offset, not normalized
// (jumptable_10_data_F94): 0 (0, 1, 0), 1 (1, 1, 0), 2 (1, 0, 0), 3 (1, -1, 0), 4 (0, -1, 0),
// 5 (-1, -1, 0), 6 (-1, 0, 0), 7 (-1, 1, 0); past 7 (`cmplwi r5, 0x7` at 0x00016984) the zero
// vector.
// @ fn_10_16984
static CVector3f GetRingRayOffset(int i) {
  switch (i) {
  case 0:
    return CVector3f(0.f, 1.f, 0.f);
  case 1:
    return CVector3f(1.f, 1.f, 0.f);
  case 2:
    return CVector3f(1.f, 0.f, 0.f);
  case 3:
    return CVector3f(1.f, -1.f, 0.f);
  case 4:
    return CVector3f(0.f, -1.f, 0.f);
  case 5:
    return CVector3f(-1.f, -1.f, 0.f);
  case 6:
    return CVector3f(-1.f, 0.f, 0.f);
  case 7:
    return CVector3f(-1.f, 1.f, 0.f);
  default:
    return CVector3f::Zero();
  }
}

// fn_10_16ADC (0x00016ADC, 0x8 bytes): the ring's ray count, 8.
// @ fn_10_16ADC
static int GetRingRayCount() { return 8; }

// fn_10_166AC (0x000166AC, 0x1B4 bytes): ring ray i: from the actor's position and to dest, both
// moved by radius along fn_10_16984(i); a line longer than lbl_10_rodata_3E4 = 0.5f (0x000167E4)
// is cast (RayStaticIntersection at 0x00016814) over reach times its length, the filter including
// lbl_10_data_C54 / C58 (0x3B kMT_Solid, 0x30 kMT_AIBlock) and excluding C5C (0x31 kMT_Platform).
// A valid hit (`cmplwi r0, 0x1` at 0x00016828) answers false, anything else true.
bool CDarkSamus::IsRingRayClear(CStateManager& mgr, const CVector3f& dest, int i, float reach,
                                float radius) const {
  const CVector3f offset = radius * GetRingRayOffset(i);
  const CVector3f start = GetTranslation() + offset;
  const CVector3f delta = (dest + offset) - start;
  const float length = delta.Magnitude();
  if (length > 0.5f) {
    const CRayCastResult result = mgr.RayStaticIntersection(
        start, delta.AsNormalized(), length * reach,
        CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid, kMT_AIBlock),
                                            CMaterialList(kMT_Platform)));
    if (result.IsValid() == true) {
      return false;
    }
  }
  return true;
}

// fn_10_16600 (0x00016600, 0xAC bytes): every ring ray (fn_10_166AC, i below fn_10_16ADC) clear;
// the first blocked one answers false (0x00016664).
// @ fn_10_16600
bool CDarkSamus::IsRingPathClear(CStateManager& mgr, const CVector3f& dest, float reach,
                                 float radius) const {
  for (int i = 0; i < GetRingRayCount(); ++i) {
    if (!IsRingRayClear(mgr, dest, i, reach, radius)) {
      return false;
    }
  }
  return true;
}

// fn_10_12B38 (0x00012B38, 0xF4 bytes): the dive's landing point: the player's position when the
// player is morphed (0x38C == 1, 0x00012BA8), else lbl_10_rodata_40C = 5.0f past the player along
// the flat line from the actor (z cleared at 0x00012B98, Normalize at 0x00012B9C).
// @ fn_10_12B38
CVector3f CDarkSamus::GetDiveTarget(CStateManager& mgr) const {
  const CPlayer* player = mgr.GetPlayer(0);
  const CVector3f& playerPos = player->GetTranslation();
  CVector3f dir(playerPos.GetX() - GetTranslation().GetX(),
                playerPos.GetY() - GetTranslation().GetY(), 0.f);
  dir.Normalize();
  if (static_cast< int >(player->GetMorphballTransitionState()) == 1) {
    return playerPos;
  }
  return playerPos + 5.f * dir;
}

// fn_10_141BC (0x000141BC, 0xC8 bytes): the player's distance (fn_10_17CEC) inside
// [minScale, maxScale] times the animation's distance (GetAnimationDistance at 0x00014224) times
// the scale's y; above the top (`bgt` at 0x00014238) or below the bottom (`bge` at 0x00014244)
// answers false.
// @ fn_10_141BC
bool CDarkSamus::IsPlayerInAnimReach(CStateManager& mgr, const CPASAnimParmData& parms,
                                     float minScale, float maxScale) const {
  const float dist = GetDistanceToPlayer(mgr);
  const float reach = GetAnimationDistance(parms) * GetModelData()->GetScale().GetY();
  if (dist > reach * maxScale || !(dist >= reach * minScale)) {
    return false;
  }
  return true;
}

// fn_10_13EE0 (0x00013EE0, 0xE0 bytes): the step animation's distance for a step direction
// (kAS_Step, FromEnum(dir), FromEnum(2) = kStep_BreakDodge; fn_10_13FFC at 0x00013F74 is the
// module's copy of the CPASAnimParmData constructor) times the scale's y.
// @ fn_10_13EE0
float CDarkSamus::GetBreakDodgeDistance(int dir) const {
  return GetAnimationDistance(CPASAnimParmData(pas::kAS_Step, CPASAnimParm::FromEnum(dir),
                                               CPASAnimParm::FromEnum(pas::kStep_BreakDodge))) *
         GetModelData()->GetScale().GetY();
}

// fn_10_13E2C (0x00013E2C, 0xB4 bytes): the dash slot's break-dodge step as an offset: the slot's
// direction (fn_10_EB90) from the transform's forward column (0x28/0x38/0x48, not flattened) times
// fn_10_13EE0 of the slot's step direction (fn_10_EDE0).
// @ fn_10_13E2C
CVector3f CDarkSamus::GetDodgeStepOffset(int slot) const {
  const float dist = GetBreakDodgeDistance(GetDashStepDirection(slot));
  return dist * GetDashDirection(slot, GetTransform().GetForward());
}

// fn_10_16B1C (0x00016B1C, 0x2C bytes): the point on the path (OnPath of 0x7C0 at 0x00016B2C
// answers kR_Success).
// @ fn_10_16B1C
bool CDarkSamus::IsOnPath(const CVector3f& point) const {
  return mPathFindSearch.OnPath(point) == CPathFindSearch::kR_Success;
}

// fn_10_1444C (0x0001444C, 0x198 bytes): from point back to the actor in lbl_10_rodata_41C =
// 0.1f steps (t from lbl_10_rodata_3E8 = 1.0f while above 0.0f), every point on the line must be
// on the mesh (GetHeightOfPointAboveMesh at 0x00014544, padding 0.0f; a failure answers false)
// within lbl_10_rodata_40C = 5.0f of height (0x0001455C/0x00014564).
bool CDarkSamus::IsLineOnMesh(const CVector3f& point, float height) const {
  const float top = 5.f + height;
  const float bottom = height - 5.f;
  for (float t = 1.f; t > 0.f; t -= 0.1f) {
    const CVector3f along = (1.f - t) * GetTranslation() + t * point;
    float pointHeight = 0.f;
    if (mPathFindSearch.GetHeightOfPointAboveMesh(along, pointHeight, 0.f) !=
        CPathFindSearch::kR_Success) {
      return false;
    }
    if (pointHeight > top || pointHeight < bottom) {
      return false;
    }
  }
  return true;
}

// fn_10_145E4 (0x000145E4, 0x64 bytes): the actor's own height (GetHeightOfPointAboveMesh at
// 0x0001461C, from its z, padding 0.0f, the result unread) and fn_10_1444C to point.
// @ fn_10_145E4
bool CDarkSamus::CanWalkToPoint(const CVector3f& point) const {
  float height = GetTranslation().GetZ();
  mPathFindSearch.GetHeightOfPointAboveMesh(GetTranslation(), height, 0.f);
  return IsLineOnMesh(point, height);
}

// fn_10_19B64 (0x00019B64, 0x670 bytes): the collector for the attacks' own reach checks.
// 0x1 goes when the player is beyond the min attack range (0x3DC, `ble` at 0x00019BE4); 0x100 when
// the aim line is blocked (fn_10_16418); 0x200/0x400/0x40/0x4 and 0x8/0x10/0x1/0x80 when the actor
// cannot walk to the player (fn_10_14648 at 0x00019C58); 0x20 when the ring path to the dive target
// (fn_10_12B38) is blocked (fn_10_16600 with lbl_10_rodata_534 = 0.75f and lbl_10_rodata_4DC =
// 1.1f); 0x4 and 0x80 when the player is outside the melee animations' reach (fn_10_141BC with
// kAS_MeleeAttack (FromEnum(2) / FromEnum(0xA), FromEnum of the body's locomotion type 0x580):
// lbl_10_rodata_538 = 0.7f .. lbl_10_rodata_53C = 1.3f, lbl_10_rodata_540 = 0.9f ..
// lbl_10_rodata_438 = 3.0f). For 0x10 the dodge side nearer the player (fn_10_13E2C of slots 2 and
// 3, the squared distances' `bge` at 0x0001A014) sets the side byte 0xBAC (1 for slot 2 at
// 0x0001A020, 0 for slot 3 at 0x0001A0D0); 0x10 goes unless that end point and the point
// lbl_10_rodata_53C = 1.3f times the offset out are on the path (fn_10_16B1C) and walkable
// (fn_10_145E4).
void CDarkSamus::CollectReachAttacks(CStateManager& mgr, SAttackWeights&, uint* mask) {
  const float dist = GetDistanceToPlayer(mgr);
  if ((*mask & 0x1) != 0 && dist > mMinAttackRange) { // `clrlwi.` at 0x00019BD4
    SetAttackBits(mask, 0, 0x1, 0xFFFF, 0xFFFF, 0xFFFF);
  }
  if ((*mask & 0x100) != 0 && !IsPlayerAimLineClear(mgr)) { // rlwinm 23,23 at 0x00019C10
    SetAttackBits(mask, 0, 0x100, 0xFFFF, 0xFFFF, 0xFFFF);
  }
  if (!CanWalkToPlayer(mgr)) {
    SetAttackBits(mask, 0, 0x200, 0x400, 0x40, 0x4);
    SetAttackBits(mask, 0, 0x8, 0x10, 0x1, 0x80);
  }
  if ((*mask & 0x20) != 0) { // rlwinm 26,26 at 0x00019CA8
    const CVector3f target = GetDiveTarget(mgr);
    if (!IsRingPathClear(mgr, target, 0.75f, 1.1f)) {
      SetAttackBits(mask, 0, 0x20, 0xFFFF, 0xFFFF, 0xFFFF);
    }
  }
  if ((*mask & 0x4) != 0) { // rlwinm 29,29 at 0x00019D2C
    const int loco = GetBodyController()->GetLocomotionType();
    const CPASAnimParmData parms(pas::kAS_MeleeAttack, CPASAnimParm::FromEnum(2),
                                 CPASAnimParm::FromEnum(loco));
    if (!IsPlayerInAnimReach(mgr, parms, 0.7f, 1.3f)) {
      SetAttackBits(mask, 0, 0x4, 0xFFFF, 0xFFFF, 0xFFFF);
    }
  }
  if ((*mask & 0x80) != 0) { // rlwinm 24,24 at 0x00019E14
    const int loco = GetBodyController()->GetLocomotionType();
    const CPASAnimParmData parms(pas::kAS_MeleeAttack, CPASAnimParm::FromEnum(0xA),
                                 CPASAnimParm::FromEnum(loco));
    if (!IsPlayerInAnimReach(mgr, parms, 0.9f, 3.f)) {
      SetAttackBits(mask, 0, 0x80, 0xFFFF, 0xFFFF, 0xFFFF);
    }
  }
  if ((*mask & 0x10) != 0) { // rlwinm 27,27 at 0x00019EFC
    const CVector3f playerPos = mgr.GetPlayer(0)->GetTranslation();
    const CVector3f offset2 = GetDodgeStepOffset(2);
    const CVector3f offset3 = GetDodgeStepOffset(3);
    const CVector3f end2 = GetTranslation() + offset2;
    const CVector3f end3 = GetTranslation() + offset3;
    const bool nearer2 = (end2 - playerPos).MagSquared() < (end3 - playerPos).MagSquared();
    const CVector3f& end = nearer2 ? end2 : end3;
    const CVector3f& offset = nearer2 ? offset2 : offset3;
    mScatterSide = nearer2 ? 1 : 0;
    if (!(IsOnPath(end) && IsOnPath(GetTranslation() + 1.3f * offset) && CanWalkToPoint(end))) {
      SetAttackBits(mask, 0, 0x10, 0xFFFF, 0xFFFF, 0xFFFF);
    }
  }
}

// fn_10_197D8 (0x000197D8, 0x140 bytes): the visible picker. An empty weight list and mask
// (0x000197E4), then the collectors in retail's order (fn_10_1A5BC, 1A554, 1A278, 1A440, 1A34C,
// 1A1D4, 19B64, 19A1C), the copy of the weights into 0xC54/0xC58 (0x000198C0..0x000198DC), the
// mask into 0xCB8 (0x000198F0) and the weighted pick (fn_10_19918) into 0xCBC (0x000198FC),
// which it answers.
int CDarkSamus::PickVisibleAttack(CStateManager& mgr) {
  SAttackWeights weights;
  weights.count = 0;
  uint mask = 0;
  CollectStageWeights(mgr, weights, &mask);
  CollectFadeAttacks(mgr, weights, &mask);
  CollectMorphballAttacks(mgr, weights, &mask);
  CollectRangeAttacks(mgr, weights, &mask);
  CollectHistoryPenalties(mgr, weights, &mask);
  CollectFacingAttacks(mgr, weights, &mask);
  CollectReachAttacks(mgr, weights, &mask);
  CollectLineAttacks(mgr, weights, &mask);
  mAttackWeights = weights;
  mAttackMask = mask;
  mWeightedPick = PickWeightedAttack(mgr, weights, mask);
  return mWeightedPick;
}

// fn_10_1969C (0x0001969C, 0x13C bytes): the faded picker - fn_10_1A5BC, 1A554, 1A440, 1A34C and
// 1A1D4, then a player the actor cannot walk to (fn_10_14648 false) takes away the boost ball
// and the sweep beam (fn_10_1A980 with 0x20000, 0x10000); the copy and the pick as in
// fn_10_197D8.
int CDarkSamus::PickFadedAttack(CStateManager& mgr) {
  SAttackWeights weights;
  weights.count = 0;
  uint mask = 0;
  CollectStageWeights(mgr, weights, &mask);
  CollectFadeAttacks(mgr, weights, &mask);
  CollectRangeAttacks(mgr, weights, &mask);
  CollectHistoryPenalties(mgr, weights, &mask);
  CollectFacingAttacks(mgr, weights, &mask);
  if (CanWalkToPlayer(mgr) == false) {
    SetAttackBits(&mask, 0, 0x20000, 0x10000, 0xFFFF, 0xFFFF);
  }
  mAttackWeights = weights;
  mAttackMask = mask;
  mWeightedPick = PickWeightedAttack(mgr, weights, mask);
  return mWeightedPick;
}

// fn_10_1954C (0x0001954C, 0x150 bytes): nothing while an attack runs (0x980 not 0xFFFF, the
// whole-word compare at 0x00019574), within lbl_10_rodata_530 = 0.25f of the last pick
// (0x00019594) or while the fight script holds the actor (0xD58 == 1, 0x000195A0). Out of close
// range (fn_10_1AA6C nonzero, 0x000195B0) the pick waits for 0x990 (0x000195C0); close by, it may
// come lbl_10_rodata_424 = 0.6f of the stage's shortest action gap early (the stage's +0x4,
// faded its +0xC, 0x00019604/0x00019618; no stage lbl_10_rodata_3E8 = 1.0f; the fnmsubs at
// 0x0001962C). A pick counts (0x988, 0x00019640), runs the faded picker when 0xDEC is set
// (0x00019648), stores the attack in 0x984 and the time in 0x98C (0x00019678).
void CDarkSamus::UpdateAttackPick(CStateManager& mgr) {
  if (mCurrentAttack != 0xFFFF) {
    return;
  }
  if (0.25f + mLastPickTime > mElapsedTime) {
    return;
  }
  if (mPissedOff == 1) {
    return;
  }
  if (GetPlayerRangeBand(mgr) != 0) {
    if (mNextActionTime > mElapsedTime) {
      return;
    }
  } else {
    float gap = 1.f;
    if (GetBattleStage(mgr) != nullptr) {
      if (mFadeOut == 1) {
        gap = GetBattleStage(mgr)->GetStageInfo().minTimeBetweenActionsWhenInvisible;
      } else {
        gap = GetBattleStage(mgr)->GetStageInfo().minTimeBetweenActions;
      }
    }
    if (-(0.6f * gap - mNextActionTime) > mElapsedTime) {
      return;
    }
  }
  mPickCount++;
  if (mFadeOut == 0) {
    mPickedAttack = PickVisibleAttack(mgr);
  } else {
    mPickedAttack = PickFadedAttack(mgr);
  }
  mLastPickTime = mElapsedTime;
}

// fn_10_1B5E4 (0x0001B5E4, 0x74 bytes): mAttackHistory's erase. A position outside
// [begin, end) (the cmplw pair at 0x0001B5EC/0x0001B604) answers end; otherwise the later ids move
// down one (0x0001B614..0x0001B630), the count drops (0x0001B63C) and the position is answered.
uint* CDarkSamus::EraseFinishedAttack(uint* pos) {
  if (pos < mAttackHistory || pos >= mAttackHistory + mAttackHistoryCount) {
    return mAttackHistory + mAttackHistoryCount;
  }
  for (uint* it = pos; it < mAttackHistory + mAttackHistoryCount; ++it) {
    it[0] = it[1];
  }
  mAttackHistoryCount--;
  return pos;
}

// fn_10_1B478 (0x0001B478, 0x16C bytes): the attack bookkeeping. Activate (the `cmpwi r6, 0x0`
// arm, 0x0001B4C8) makes `attack` the current attack (0x980, 0x0001B4CC), clears the pick (0x984
// = 0xFFFF, 0x0001B4E0) and draws the next pick's delay into 0x990 (0x0001B558) between the
// stage's minimum and maximum action gap (+0x4/+0x8, 0x0001B534/0x0001B544; +0xC/+0x10 when
// faded, 0xDEC == 1 at 0x0001B500; without a stage both are lbl_10_rodata_3FC = 0.0f) with the
// state manager's random (the CRandom16 at 0x16E4, 0x0001B54C). Deactivate (`cmpwi r6, 0x3` /
// the 0x0001B560 arm) trims the oldest finished attack when there are three (0x0001B564,
// fn_10_1B5E4 at 0x0001B574), pushes the current one (0x0001B590), counts it (0x0001B5A4), clears
// 0x980 to 0xFFFF (0x0001B5A8) and turns the delay into a time (0x990 += 0x8B0, 0x0001B5B8).
// Update does nothing.
void CDarkSamus::UpdateCurrentAttack(uint attack, CStateManager& mgr, EStateMsg msg) {
  switch (msg) {
  case kStateMsg_Activate: {
    mCurrentAttack = attack;
    mPickedAttack = 0xFFFF;
    float minGap = 0.f;
    float maxGap = minGap;
    if (GetBattleStage(mgr) != nullptr) {
      if (mFadeOut == 1) {
        minGap = GetBattleStage(mgr)->GetStageInfo().minTimeBetweenActionsWhenInvisible;
        maxGap = GetBattleStage(mgr)->GetStageInfo().maxTimeBetweenActionsWhenInvisible;
      } else {
        minGap = GetBattleStage(mgr)->GetStageInfo().minTimeBetweenActions;
        maxGap = GetBattleStage(mgr)->GetStageInfo().maxTimeBetweenActions;
      }
    }
    mNextActionTime = (maxGap - minGap) * mgr.Random()->Float() + minGap;
    break;
  }
  case kStateMsg_Deactivate:
    if (static_cast< int >(mAttackHistoryCount) >= 3) {
      EraseFinishedAttack(mAttackHistory);
    }
    // 0x0001B584: the placement-new null test of the push; the slot is never null.
    mAttackHistory[mAttackHistoryCount] = mCurrentAttack;
    mAttackHistoryCount++;
    mCurrentAttack = 0xFFFF;
    mNextActionTime += mElapsedTime;
    break;
  default:
    break;
  }
}

// fn_10_10520 (0x00010520, 0xD4 bytes): on Activate (0x00010528) the base's orbit position
// (CPatterned::GetOrbitPosition, called directly at 0x00010550) into 0xD28 and its aim position
// at lbl_10_rodata_3FC = 0.0f (CPatterned::GetAimPosition, 0x00010580) into 0xD34; on Deactivate
// (0x000105A0) both become the zero vector.
// @ fn_10_10520
void CDarkSamus::UpdateAttackAimPoints(CStateManager& mgr, EStateMsg msg) {
  if (msg == kStateMsg_Activate) {
    mAttackOrbitPos = CPatterned::GetOrbitPosition(mgr);
    mAttackAimPos = CPatterned::GetAimPosition(mgr, 0.f);
  } else if (msg == kStateMsg_Deactivate) {
    mAttackOrbitPos = CVector3f::Zero();
    mAttackAimPos = CVector3f::Zero();
  }
}

// fn_10_1AEC0 (0x0001AEC0, 0xF4 bytes): the motion volume at lbl_10_rodata_3FC = 0.0f
// (0x0001AEEC) builds an area collision cache (0x0001AF28, filled at 0x0001AF34) and a collider
// list (0x0001AF50), and CGameCollision::CollisionFailsafe runs with the actor's collision
// primitive (vtable 0x7C, 0x0001AF64), dt 0.0f, 0 ticks and 0.0f (0x0001AF8C); the cache's
// destructor (fn_10_1AFB4, 0x0001AF98) is the host's at the end of the scope.
// @ fn_10_1AEC0
void CDarkSamus::RunCollisionFailsafe(CStateManager& mgr) {
  const CAABox motionVolume = GetMotionVolume(0.f);
  CAreaCollisionCache cache(motionVolume);
  CGameCollision::BuildAreaCollisionCache(mgr, cache);
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildColliderList(nearList, *this, motionVolume);
  CGameCollision::CollisionFailsafe(mgr, cache, *this, *GetCollisionPrimitive(), nearList, 0.f, 0,
                                    0.f);
}

// fn_10_16B8C (0x00016B8C, 0x150 bytes): nothing when dt <= lbl_10_rodata_3FC = 0.0f (the cror at
// 0x00016BCC). Otherwise the flattened facing axis (the transform's second column with z = 0.0f,
// 0x00016BD4..0x00016BF0) and the flattened line to the point (0x00016C0C..0x00016C4C), each
// normalized when it can be (0x00016BF4/0x00016C50), give a rotation arc clamped to dt * rate
// (0x00016C68, 0x00016C80) that the actor turns by in one frame (0x00016CB0).
void CDarkSamus::TurnTowardPoint(float dt, const CVector3f& point, float rate) {
  if (dt <= 0.f) {
    return;
  }
  const CVector3f forward = GetTransform().GetForward();
  CVector3f flatForward(forward.GetX(), forward.GetY(), 0.f);
  if (flatForward.CanBeNormalized() == true) {
    flatForward.Normalize();
  }
  const CVector3f pos = GetTranslation();
  CVector3f toPoint(point.GetX() - pos.GetX(), point.GetY() - pos.GetY(), 0.f);
  if (toPoint.CanBeNormalized() == true) {
    toPoint.Normalize();
  }
  const CQuaternion arc =
      CQuaternion::ShortestRotationArcClamped(flatForward, toPoint, CRelAngle::FromRadians(dt * rate));
  RotateInOneFrameOR(arc, dt);
}

// fn_10_16B64 (0x00016B64, 0x28 bytes): fn_10_16B8C toward the player's position (the state
// manager's player at 0x14FC, its position at +0x54, 0x00016B70).
// @ fn_10_16B64
void CDarkSamus::TurnTowardPlayer(CStateManager& mgr, float dt, float rate) {
  TurnTowardPoint(dt, mgr.GetPlayer(0)->GetTranslation(), rate);
}

// fn_10_11814 (0x00011814, 0x160 bytes): the scatter shot's start, from DoUserAnimEvent's
// kUE_EventStart arm (0x0001C25C with side 0 for the ScatterShot attack 8, 0x0001C268 with the
// byte 0xBAC for MovingScatterShot's 0x10). The start time 0xBA4 is the elapsed time 0x8B0
// (0x00011858), the firing byte 0xBAD is set (0x00011868) and the fire timer 0xBA8 starts at
// lbl_10_rodata_474 = 0.05f (0x00011874). The aim 0xB88 is the player's position (the state
// manager's player at 0x14FC, +0x54, 0x00011878..0x00011890); the flattened direction to it
// (z = lbl_10_rodata_3FC = 0.0f, 0x000118B8) is normalized (0x000118BC) and the line's ends sit
// lbl_10_rodata_478 = 6.5f to either side of the aim (the perpendicular (y, -x), 0x000118E0..
// 0x00011908; z = aim z + 0.0f). Side 1 (`cmplwi r0, 0x1` at 0x000118EC) starts the line on the
// (y, -x) side, any other side on the (-y, x) side.
// @ fn_10_11814
void CDarkSamus::StartScatterShotFire(CStateManager& mgr, uchar side) {
  mScatterStartTime = mElapsedTime;
  mScatterFiring = 1;
  mScatterTimer = 0.05f;
  const CVector3f player = mgr.GetPlayer(0)->GetTranslation();
  mScatterAim = player;
  const CVector3f pos = GetTranslation();
  CVector3f dir(player.GetX() - pos.GetX(), player.GetY() - pos.GetY(), 0.f);
  dir.Normalize();
  const CVector3f right(player.GetX() + 6.5f * dir.GetY(), player.GetY() + 6.5f * -dir.GetX(),
                        player.GetZ() + 0.f);
  const CVector3f left(player.GetX() + 6.5f * -dir.GetY(), player.GetY() + 6.5f * dir.GetX(),
                       player.GetZ() + 0.f);
  if (side == 1) {
    mScatterLineStart = right;
    mScatterLineEnd = left;
  } else {
    mScatterLineStart = left;
    mScatterLineEnd = right;
  }
}

// fn_10_13230 (0x00013230, 0x1A0 bytes): the point the next shot aims at. The sweep fraction is
// the time since the start (0x8B0 - 0xBA4) over lbl_10_rodata_4A8 = 0.47f for the ScatterShot
// attack 8 (`cmpwi r0, 0x8` at 0x00013290), lbl_10_rodata_4AC = 0.67f for any other, clamped to
// lbl_10_rodata_3E8 = 1.0f (0x000132BC). The point is the line start plus the fraction of the line
// (0xB94 - 0xB7C), jittered by the state manager's random (0x16E4): x and y by
// lbl_10_rodata_3E0 = 2.0f * (lbl_10_rodata_408 = 4.0f * rand) - 4.0f (0x000132F0..0x0001334C), z
// by lbl_10_rodata_4B4 = 9.0f * rand + lbl_10_rodata_4B0 = -1.5f (0x00013350..0x00013378).
// @ fn_10_13230
CVector3f CDarkSamus::GetScatterShotAimPoint(CStateManager& mgr) const {
  float t = (mElapsedTime - mScatterStartTime) / (mCurrentAttack == 8 ? 0.47f : 0.67f);
  if (t > 1.f) {
    t = 1.f;
  }
  const float sx = mScatterLineStart.GetX();
  const float sy = mScatterLineStart.GetY();
  const float sz = mScatterLineStart.GetZ();
  const float dx = mScatterLineEnd.GetX() - sx;
  const float dy = mScatterLineEnd.GetY() - sy;
  const float dz = mScatterLineEnd.GetZ() - sz;
  const float x = 2.f * (4.f * mgr.Random()->Float()) + ((sx + t * dx) - 4.f);
  const float y = 2.f * (4.f * mgr.Random()->Float()) + ((sy + t * dy) - 4.f);
  const float z = 9.f * mgr.Random()->Float() + (-1.5f + (sz + t * dz));
  return CVector3f(x, y, z);
}

// fn_10_12F60 (0x00012F60, 0x2D0 bytes): one scatter shot. The shot's transform looks from the gun
// locator transform 0x8CC's translation at the aim point (fn_10_13230, 0x00012F98) with the
// locator's up (LookAt at 0x00013000). With a live projectile in 0xBE4 (0x0001301C;
// TCastToPtr<CScattershotProjectile> at 0x00013038, else 0xBE4 = kInvalidUniqueId) the shot is
// added to it (fn_10_C98 at 0x0001308C, from the locator's translation along the shot's forward).
// Otherwise the second projectile's chance 0xBDC - divided by lbl_10_rodata_3F0 = 100.0f when above
// lbl_10_rodata_3E8 = 1.0f (0x000130A0) - against the state manager's random decides 0xBAE
// (`fcmpo; mfcr; srwi 31` = rand < chance, 0x000130CC), which ProjectileInfo (vtable 0xDC) reads;
// when its weapon description is loaded (TCachedToken::TryCache, 0x000130E4..0x00013124) a new
// uid goes to 0xBE4 (0x00013150) and the 0x27B0-byte CScattershotProjectile (fn_10_2AD0 at
// 0x000131DC: the info's token, kWT_AI = `li r5, 0xb`, the transform, the info's damage at +0xC,
// the uid, the area 0x4, the owner 0x8 and lbl_10_rodata_488 = 45.0f) is added (0x000131F4), or
// 0xBE4 is cleared when the allocation fails (0x00013204).
//
// Port limit: CScattershotProjectile is the module's own CWeapon subclass (its vtable is
// lbl_10_data_40, its ~57 functions fill 0x0..0x44D0) and the port does not have it yet. The
// host's CEnergyProjectile stands in for it with the same description, weapon type, transform,
// damage, uid, area and owner (the impact arguments are CPatterned::LaunchProjectile's); it is one
// shot rather than a swarm, so fn_10_C98's added shots are a stand-in and the next projectile
// spawns once the current one is gone.
void CDarkSamus::FireScatterShotProjectile(CStateManager& mgr) {
  const CVector3f aim = GetScatterShotAimPoint(mgr);
  const CTransform4f xf = CTransform4f::LookAt(mGunLctrXf.GetTranslation(), aim, mGunLctrXf.GetUp());
  if (mScatterProjectileUid != kInvalidUniqueId) {
    if (TCastToPtr< CEnergyProjectile >(mgr.ObjectById(mScatterProjectileUid)) == nullptr) {
      mScatterProjectileUid = kInvalidUniqueId;
    } else {
      FsmStandIn("fn_10_C98", "CScattershotProjectile add shot");
      return;
    }
  }
  float chance = mScatterChance;
  if (chance > 1.f) {
    chance = chance / 100.f;
  }
  mScatterUseSecondInfo = mgr.Random()->Float() < chance;
  CProjectileInfo* info = ProjectileInfo();
  if (!info->Token().TryCache()) {
    return;
  }
  mScatterProjectileUid = mgr.AllocateUniqueId();
  CEnergyProjectile* projectile = rs_new CEnergyProjectile(
      true, ProjectileInfo()->Token(), kWT_AI, xf, kMT_Character, ProjectileInfo()->GetDamage(),
      mScatterProjectileUid, GetCurrentAreaId(), GetUniqueId(), kInvalidUniqueId,
      CWeapon::kPA_None, false, CVector3f(1.f, 1.f, 1.f), CImpactVisorEffect::None(), false, true,
      false, 1.f, 4.f, 4.f);
  if (projectile != nullptr) {
    mgr.AddObject(projectile);
    return;
  }
  mScatterProjectileUid = kInvalidUniqueId;
}

// fn_10_11974 (0x00011974, 0x164 bytes): only while the firing byte 0xBAD is set (0x0001199C).
// The aim point 0xB88 moves toward the player by lbl_10_rodata_47C = 6.0f * dt of the way
// (0x000119BC..0x00011A00) and the line's ends 0xB7C/0xB94 move with it (0x00011A04..0x00011A48);
// the timer 0xBA8 counts dt (0x00011A78). With a rate 0xBE0 above 0.0f (0x00011A84) one projectile
// (fn_10_12F60, 0x00011AA8) fires per lbl_10_rodata_3E8 = 1.0f / rate of the timer (0x00011A8C,
// the loop at 0x00011AB4).
void CDarkSamus::UpdateScatterShotFire(CStateManager& mgr, float dt) {
  if (mScatterFiring == 0) {
    return;
  }
  const float t = 6.f * dt;
  const float keep = 1.f - t;
  const CVector3f& player = mgr.GetPlayer(0)->GetTranslation();
  const float dx = (mScatterAim.GetX() * keep + player.GetX() * t) - mScatterAim.GetX();
  const float dy = (mScatterAim.GetY() * keep + player.GetY() * t) - mScatterAim.GetY();
  const float dz = (mScatterAim.GetZ() * keep + player.GetZ() * t) - mScatterAim.GetZ();
  const CVector3f delta(dx, dy, dz);
  mScatterLineStart += delta;
  mScatterLineEnd += delta;
  mScatterAim += delta;
  mScatterTimer += dt;
  if (mScatterRate <= 0.f) {
    return;
  }
  const float interval = 1.f / mScatterRate;
  while (mScatterTimer > interval) {
    mScatterTimer -= interval;
    FireScatterShotProjectile(mgr);
  }
}

// fn_10_15348 (0x00015348, 0xA4 bytes): the dash record, in the retail store order.
// @ fn_10_15348
CDarkSamusDash::CDarkSamusDash() {
  mDestination = CVector3f::Zero();
  mStartPosition = CVector3f::Zero();
  mStepsLeft = 0;
  mDoubleDash = false;
  mStartPosition = CVector3f::Zero();
  mDestination = mStartPosition;
  mLastSlot = -1;
  mSlot = -1;
  mWeight = 0.f;
  mBlocked = false;
}

// fn_10_EDE0 (0x0000EDE0, 0x14 bytes): the dash slot (0..7) to its step direction, the table
// lbl_10_rodata_37C = {0, 1, 2, 3, 6, 7, 8, 9} (no bounds test). Echoes' step directions go past
// the host enum's kSD_Down (5): 6..9 are the four diagonals, cast through as retail passes them.
// @ fn_10_EDE0
static pas::EStepDirection GetDashStepDirection(int slot) {
  static const int skDirections[8] = {0, 1, 2, 3, 6, 7, 8, 9};
  return static_cast< pas::EStepDirection >(skDirections[slot]);
}

// fn_10_EB90 (0x0000EB90, 0x228 bytes): the dash slot's direction from the flat facing axis f, the
// jump table jumptable_10_data_F74 (`cmplwi r4, 0x7; bgt` at 0x0000EB98 to the zero vector):
// 0 f, 1 -f (lbl_10_rodata_3EC = -1.0f), 2 (-f.y, f.x, 0), 3 (f.y, -f.x, 0), and the diagonals
// normalized (AsNormalized at 0x0000EC70/0x0000ECC4/0x0000ED2C/0x0000ED80): 4 the sum of f and
// slot 2's, 6 slot 2's minus f, 5 the sum of f and slot 3's, 7 slot 3's minus f (the `cmpwi` at
// 0x0000EC14/0x0000EC3C and 0x0000ECD0/0x0000ECF8 split the shared arms).
// @ fn_10_EB90
static CVector3f GetDashDirection(uint slot, const CVector3f& f) {
  switch (slot) {
  case 0:
    return f;
  case 1:
    return -1.f * f;
  case 2:
  case 4:
  case 6: {
    const float nfy = -f.GetY();
    if (slot == 2) {
      return CVector3f(nfy, f.GetX(), 0.f);
    }
    if (slot == 4) {
      return CVector3f(nfy + f.GetX(), f.GetX() + f.GetY(), 0.f + f.GetZ()).AsNormalized();
    }
    const CVector3f neg = -1.f * f;
    return CVector3f(nfy + neg.GetX(), f.GetX() + neg.GetY(), 0.f + neg.GetZ()).AsNormalized();
  }
  case 3:
  case 5:
  case 7: {
    const float nfx = -f.GetX();
    if (slot == 3) {
      return CVector3f(f.GetY(), nfx, 0.f);
    }
    if (slot == 5) {
      return CVector3f(f.GetY() + f.GetX(), nfx + f.GetY(), 0.f + f.GetZ()).AsNormalized();
    }
    const CVector3f neg = -1.f * f;
    return CVector3f(f.GetY() + neg.GetX(), nfx + neg.GetY(), 0.f + neg.GetZ()).AsNormalized();
  }
  default:
    return CVector3f::Zero();
  }
}

// fn_10_12C2C (0x00012C2C, 0xB4 bytes): the step command (fn_10_21DF8 at 0x00012D88,
// pas::kAS_Step = `li r5, 0x3` at 0x00012D6C) toward the dash slot's direction (fn_10_EDE0 of
// 0x9E0 at 0x00012D28, stored at the command's 0x10), a normal step (`li r0, 0x0` stored at 0x14).
// @ fn_10_12C2C
void CDarkSamus::DoDashStepCmd(EStateMsg msg) {
  DoStepCmd(msg, pas::kAS_Step, CBCStepCmd(GetDashStepDirection(mDash.mSlot), pas::kStep_Normal));
}

// fn_10_12CE0 (0x00012CE0, 0xF0 bytes): the attack bookkeeping with id 0x8000 (`lis r4, 0x1;
// addi r4, r4, -0x8000`, fn_10_1B478 at 0x00012D14), the collision failsafe (fn_10_1AEC0 at
// 0x00012D20), the step command of fn_10_12C2C (inlined, 0x00012D24..0x00012D88), and on Activate
// (`cmpwi r31, 0x0` at 0x00012D98) the slot 0x9E0 is copied to 0x9FC (0x00012DB0).
// @ fn_10_12CE0
void CDarkSamus::UpdateDashStep(CStateManager& mgr, EStateMsg msg) {
  UpdateCurrentAttack(0x8000, mgr, msg);
  RunCollisionFailsafe(mgr);
  DoDashStepCmd(msg);
  if (msg == kStateMsg_Activate) {
    mDash.mLastSlot = mDash.mSlot;
  }
}

// fn_10_14A00 (0x00014A00, 0x250 bytes): the blast when the dash is stuck. The damage
// (0x00014A3C..0x00014AF0): kWT_UnknownSource (`li r0, 0x13` into the mode's high half), not
// charged / comboed (0x8000 / 0x4000 cleared at 0x00014A78/0x00014A94), instant kill (0x2000 set
// at 0x00014AB0); damage and radius damage lbl_10_rodata_3F0 = 100.0f, radius
// lbl_10_rodata_4C8 = 15.0f + 0x968 (the int-to-float at 0x00014A68, fadds at 0x00014A9C),
// knockback lbl_10_rodata_3FC = 0.0f, no immunity (0x80 set at 0x00014AA8), no radius-damage flag
// (0x40 cleared at 0x00014ABC). The near list (0x00014BC0) is the box of half-size radius around
// the actor (0x00014B60..0x00014BA0) filtered to include lbl_10_data_C94 / C98 (0x3B kMT_Solid,
// 0x31 kMT_Platform) and exclude lbl_10_data_C9C (0x20 kMT_Player), type 3; every object found
// (ObjectById, the null test at 0x00014BE4) takes the radius damage from the actor's position
// with the actor's uid (ApplyRadiusDamage at 0x00014C0C). Last the collision failsafe
// (0x00014C30).
void CDarkSamus::ApplyDashStuckBlast(CStateManager& mgr) {
  const float radius = 15.f + static_cast< float >(mDashStuckCount);
  const CDamageInfo info(CWeaponMode(kWT_UnknownSource, false, false, true), 100.f, radius, 0.f,
                         true, false);
  const CMaterialFilter filter = CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid, kMT_Platform), CMaterialList(kMT_Player));
  const CVector3f& pos = GetTranslation();
  const CVector3f max(pos.GetX() + radius, pos.GetY() + radius, pos.GetZ() + radius);
  const CVector3f min(pos.GetX() + -radius, pos.GetY() + -radius, pos.GetZ() + -radius);
  const CAABox box(min, max);
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, box, filter, this);
  for (rstl::reserved_vector< TUniqueId, 1024 >::iterator it = nearList.begin();
       it != nearList.end(); ++it) {
    // The near list holds actors only; retail passes the entity on as one.
    CActor* actor = static_cast< CActor* >(mgr.ObjectById(*it));
    if (actor != nullptr) { // `mr. r6, r3; beq` at 0x00014BE4/0x00014BE8
      mgr.ApplyRadiusDamage(*this, GetTranslation(), *actor, GetUniqueId(), info);
    }
  }
  RunCollisionFailsafe(mgr);
}

// fn_10_16048 (0x00016048, 0x3D0 bytes): the flat line from the actor to dest (z of the delta
// cleared at 0x000160EC; not normalizable answers lbl_10_rodata_3FC = 0.0f at 0x00016128) is cast
// three times (the loop `cmpwi r27, 0x3` at 0x000163B0) from the actor's position raised by
// lbl_10_rodata_3E0 = 2.0f, offset lbl_10_rodata_4F0 = 1.5f along the zero vector (k 0), the right
// normal (dir.y, -dir.x, 0) (k 1, 0x00016210) and the left one (-dir.y, dir.x, 0) (k 2,
// 0x0001622C), over the delta's length. The static test (RayStaticLineOfSightTest on the actor's
// area at 0x000162A4) includes lbl_10_data_C68 (0x3B kMT_Solid) and excludes C6C / C70 (0x2F
// kMT_CollisionActor, 0x1C kMT_AIPassthrough); the dynamic one (RayDynamicLineOfSightTest at
// 0x00016380, the near list, the actor ignored) includes C74 / C78 (kMT_Solid, 0x30 kMT_AIBlock)
// and excludes C7C / C80 (0x20 kMT_Player, kMT_AIPassthrough). A blocked centre ray answers 0.0f
// (0x000162B0/0x0001638C); a blocked side ray takes lbl_10_rodata_4F4 = 0.49f off the answer,
// which starts at lbl_10_rodata_3E8 = 1.0f.
float CDarkSamus::GetDashPathClearance(
    CStateManager& mgr, const CVector3f& dest,
    const rstl::reserved_vector< TUniqueId, 1024 >& nearList) const {
  const CVector3f& pos = GetTranslation();
  const CVector3f delta(dest.GetX() - pos.GetX(), dest.GetY() - pos.GetY(), 0.f);
  const float length = delta.Magnitude();
  if (!delta.CanBeNormalized()) {
    return 0.f;
  }
  const CVector3f dir = delta.AsNormalized();
  const CMaterialFilter staticFilter = CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid), CMaterialList(kMT_CollisionActor, kMT_AIPassthrough));
  const CMaterialFilter dynamicFilter = CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid, kMT_AIBlock), CMaterialList(kMT_Player, kMT_AIPassthrough));
  float clearance = 1.f;
  for (int k = 0; k < 3; ++k) {
    CVector3f offset = CVector3f::Zero();
    if (k == 1) {
      offset = CVector3f(dir.GetY(), -dir.GetX(), 0.f);
    } else if (k == 2) {
      offset = CVector3f(-dir.GetY(), dir.GetX(), 0.f);
    }
    const CVector3f start(pos.GetX() + 0.f + 1.5f * offset.GetX(),
                          pos.GetY() + 0.f + 1.5f * offset.GetY(),
                          pos.GetZ() + 2.f + 1.5f * offset.GetZ());
    const CGameArea& area = mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId());
    if (!CGameCollision::RayStaticLineOfSightTest(area, start, dir, length, staticFilter)) {
      if (k == 0) {
        return 0.f;
      }
      clearance -= 0.49f;
      continue;
    }
    if (!CGameCollision::RayDynamicLineOfSightTest(mgr, start, dir, length, dynamicFilter,
                                                   nearList, this)) {
      if (k == 0) {
        return 0.f;
      }
      clearance -= 0.49f;
    }
  }
  return clearance;
}

// fn_10_154FC (0x000154FC, 0xB4C bytes): weighs the eight dash slots and picks one.
//
// The facing axis (the transform's forward column, z cleared at 0x000155FC, normalized or
// (lbl_10_rodata_3E8 = 1.0f, 0, 0) at 0x0001561C) and the squared distance to the player (mgr's
// 0x14FC). While steps are left (0xA04 > 0, 0x0001564C) the last slot and the slot opposite it
// (lbl_10_rodata_340 = {1, 0, 3, 2, 7, 6, 5, 4}) get no weight (0x00015718..), otherwise the last
// slot's weight is scaled by lbl_10_rodata_4D8 = 0.01f (0x000157D8..). 0x9E0 becomes 1
// (0x0001586C). The height of the actor above the path mesh (GetHeightOfPointAboveMesh at
// 0x00015888, padding 1.0f) and the near list (0x00015A58) around the forward step's reach (the
// animation distance of kAS_Step (FromEnum(0), FromEnum(0)) times the scale's y, times
// lbl_10_rodata_4DC = 1.1f, in x/y; lbl_10_rodata_4E0 = -2.0f .. lbl_10_rodata_47C = 6.0f in z),
// including lbl_10_data_C84 / C88 (kMT_Solid, kMT_AIBlock) and excluding C8C / C90 (kMT_Player,
// kMT_AIPassthrough). Per slot (0x00015A78, 8 slots at 0x00015E00): the slot's end point is the
// actor's position plus the slot's direction (fn_10_EB90) times its step's animation distance
// (kAS_Step (FromEnum(fn_10_EDE0(slot)), FromEnum(0))) times the scale's y. A path less than clear
// (fn_10_16048 below 1.0f, 0x00015B94) scales the weight by lbl_10_rodata_41C = 0.1f * clearance
// when the height query failed (0x00015B9C), drops the slot when the actor has stood still for
// less than lbl_10_rodata_42C = 1.2f (0x00015BCC), else scales it by lbl_10_rodata_4E4 = 0.02f *
// clearance. With steps left an end point nearer the player than the actor drops the slot
// (0x00015C58), else the weight is scaled by the squared distance gained (0x00015C70); without,
// the closest the flat line to the end point passes the player (t from lbl_10_rodata_474 = 0.05f
// by 0.05f while <= 1.0f, the start at lbl_10_rodata_4E8 = 8.01f) under lbl_10_rodata_414 = 8.0f
// scales it by lbl_10_rodata_3E4 = 0.5f * (closest * lbl_10_rodata_4EC = 0.125f). Then an end
// point nearer the player halves it (0x00015D50), and with a target (0x00015D6C) a distance under
// the base's max attack range (0x3E0) scales it by 1 - distance / range, else by 0.01f. Last the
// largest weight (from 0.0f) is looked for from a random slot on (Next, the signed modulo 8 at
// 0x00015EB0..); the slot (1 when none matches), its end point, the actor's position and its weight
// go into the dash record (0x00015F94..0x00015FD4), and the weight is the answer.
float CDarkSamus::PickDash(CStateManager& mgr, float* weights, const CVector3f* target) {
  static const int skOpposite[8] = {1, 0, 3, 2, 7, 6, 5, 4};
  const CVector3f playerPos = mgr.GetPlayer(0)->GetTranslation();
  const CVector3f& pos = GetTranslation();
  const CVector3f toPlayer = playerPos - pos;
  const float playerDistSq = toPlayer.MagSquared();
  const CVector3f forward = GetTransform().GetForward();
  CVector3f facing(forward.GetX(), forward.GetY(), 0.f);
  if (facing.CanBeNormalized() == true) {
    facing.Normalize();
  } else {
    facing = CVector3f(1.f, 0.f, 0.f);
  }

  CVector3f ends[8];
  int slots[8];
  for (int i = 0; i < 8; ++i) {
    ends[i] = CVector3f::Zero();
  }
  if (mDash.mStepsLeft > 0) {
    for (int i = 0; i < 8; ++i) {
      slots[i] = i;
      if (mDash.mLastSlot == i || mDash.mLastSlot == skOpposite[i]) {
        weights[i] = 0.f;
      }
    }
  } else {
    for (int i = 0; i < 8; ++i) {
      slots[i] = i;
      if (mDash.mLastSlot == i) {
        weights[i] = weights[i] * 0.01f;
      }
    }
  }

  mDash.mSlot = 1;
  float height = pos.GetZ();
  const CPathFindSearch::EResult heightResult =
      mPathFindSearch.GetHeightOfPointAboveMesh(pos, height, 1.f);

  const float scaleY = GetModelData()->GetScale().GetY();
  const float reach =
      1.1f * (GetAnimationDistance(CPASAnimParmData(pas::kAS_Step, CPASAnimParm::FromEnum(0),
                                                    CPASAnimParm::FromEnum(0))) *
              scaleY);
  const CVector3f boxMax(pos.GetX() + reach, pos.GetY() + reach, pos.GetZ() + 6.f);
  const CVector3f boxMin(pos.GetX() + -reach, pos.GetY() + -reach, pos.GetZ() + -2.f);
  const CMaterialFilter filter = CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid, kMT_AIBlock), CMaterialList(kMT_Player, kMT_AIPassthrough));
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, CAABox(boxMin, boxMax), filter, this);

  for (int i = 0; i < 8; ++i) {
    float& weight = weights[i];
    const float step = GetAnimationDistance(CPASAnimParmData(
                           pas::kAS_Step, CPASAnimParm::FromEnum(GetDashStepDirection(i)),
                           CPASAnimParm::FromEnum(0))) *
                       GetModelData()->GetScale().GetY();
    const CVector3f dir = GetDashDirection(i, facing);
    const CVector3f end(pos.GetX() + step * dir.GetX(), pos.GetY() + step * dir.GetY(),
                        pos.GetZ() + step * dir.GetZ());
    ends[i] = end;
    const float clearance = GetDashPathClearance(mgr, end, nearList);
    if (clearance < 1.f) {
      if (heightResult != CPathFindSearch::kR_Success) {
        weight = weight * (0.1f * clearance);
      } else if (mStillTime < 1.2f) {
        weight = 0.f;
        continue;
      } else {
        weight = weight * (0.02f * clearance);
      }
    }
    const float endDistSq = (end - playerPos).MagSquared();
    if (mDash.mStepsLeft > 0) {
      if (endDistSq < playerDistSq) {
        weight = 0.f;
        continue;
      }
      weight = weight * (endDistSq - playerDistSq);
    } else {
      float closest = 8.01f;
      for (float t = 0.05f; t <= 1.f; t += 0.05f) {
        const CVector3f along((1.f - t) * pos.GetX() + t * end.GetX() - playerPos.GetX(),
                              (1.f - t) * pos.GetY() + t * end.GetY() - playerPos.GetY(), 0.f);
        const float dist = along.Magnitude();
        if (dist < closest) {
          closest = dist;
        }
      }
      if (closest < 8.f) {
        weight = weight * (0.5f * (closest * 0.125f));
      }
    }
    if (endDistSq < playerDistSq) {
      weight = weight * 0.5f;
    }
    if (target != nullptr) { // `cmplwi r25, 0x0; beq` at 0x00015D6C/0x00015D70
      const float dist = (*target - end).Magnitude();
      if (dist < mMaxAttackRange) {
        weight = weight * (1.f - dist / mMaxAttackRange);
      } else {
        weight = weight * 0.01f;
      }
    }
  }

  float best = 0.f;
  for (int i = 0; i < 8; ++i) {
    if (weights[i] > best) {
      best = weights[i];
    }
  }
  const int start = mgr.Random()->Next();
  int pick = 1;
  for (int k = 0; k < 8; ++k) {
    const int idx = (start + k) % 8;
    if (best == weights[idx]) {
      pick = idx;
      break;
    }
  }
  mDash.mSlot = slots[pick];
  mDash.mDestination = ends[pick];
  mDash.mStartPosition = GetTranslation();
  mDash.mWeight = weights[pick];
  return weights[pick];
}

// fn_10_14FE0 (0x00014FE0, 0xFC bytes): the best cover point pushed (fn_10_1E3C0 / fn_10_1E26C at
// 0x00015008/0x0001501C), the dash picked with eight weights of lbl_10_rodata_3E8 = 1.0f and no
// target (fn_10_154FC at 0x0001505C, `li r6, 0x0`); a pick of weight 0.0f blocks the dash (0x80
// set at 0x00015078). Then the stuck count: standing still for more than lbl_10_rodata_42C = 1.2f
// (0x0001508C) counts 0x968 up and two or more (`cmplwi r0, 0x2` at 0x000150A4) fire the stuck
// blast (fn_10_14A00); otherwise the count is cleared (0x000150C0).
void CDarkSamus::PickNextDash(CStateManager& mgr) {
  PushAttacker(FindBestCoverPoint(mgr));
  float weights[8] = {1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f};
  if (0.f == PickDash(mgr, weights, nullptr)) {
    mDash.mBlocked = true;
  }
  if (mStillTime > 1.2f) {
    ++mDashStuckCount;
    if (mDashStuckCount >= 2) {
      ApplyDashStuckBlast(mgr);
    }
  } else {
    mDashStuckCount = 0;
  }
}

// fn_10_14E5C (0x00014E5C, 0x184 bytes): the dash slot's step animation (kAS_Step with
// FromEnum(fn_10_EDE0(0x9E0)) and FromEnum(0), fn_10_13FFC at 0x00014F04 is the module's copy of
// the CPASAnimParmData constructor) found in the body controller's PAS database
// (FindBestAnimation at 0x00014F28 with mgr's random and -1), set on the animation data
// (CAnimPlaybackParms(anim, -1, 1.0f, true), SetAnimation at 0x00014F7C, no query) and advanced by
// lbl_10_rodata_46C = 0.3f (AdvanceAnimation at 0x00014FB4, the actor's area, the tree advanced,
// camera distance 0.0f); last the collision failsafe (0x00014FC0).
// @ fn_10_14E5C
void CDarkSamus::PlayDashStepAnimation(CStateManager& mgr) {
  const CPASAnimParmData parms(pas::kAS_Step,
                               CPASAnimParm::FromEnum(GetDashStepDirection(mDash.mSlot)),
                               CPASAnimParm::FromEnum(0));
  const rstl::pair< float, int > best =
      GetBodyController()->GetPASDatabase().FindBestAnimation(parms, *mgr.Random(), -1);
  ModelData()->AnimationData()->SetAnimation(CAnimPlaybackParms(best.second, -1, 1.f, true),
                                             false);
  ModelData()->AdvanceAnimation(0.3f, mgr, GetCurrentAreaId(), true, 0.f);
  RunCollisionFailsafe(mgr);
}

// fn_10_14C50 (0x00014C50, 0xC4 bytes): a fresh dash record (the zero vectors, -1 slots, 0.0f
// weight, 0x80 cleared: 0x00014C88..0x00014CCC) whose last slot is the slot just dashed
// (0x00014CD0), a new pick (fn_10_14FE0), one step less (0x00014CE8), the step command delivered
// as on Activate (fn_10_12C2C with `li r4, 0x0` at 0x00014CEC) and its animation (fn_10_14E5C).
// @ fn_10_14C50
void CDarkSamus::StartNextDashStep(CStateManager& mgr) {
  const int slot = mDash.mSlot;
  mDash.mStartPosition = CVector3f::Zero();
  mDash.mDestination = mDash.mStartPosition;
  mDash.mLastSlot = -1;
  mDash.mSlot = -1;
  mDash.mWeight = 0.f;
  mDash.mBlocked = false;
  mDash.mLastSlot = slot;
  PickNextDash(mgr);
  mDash.mStepsLeft = mDash.mStepsLeft - 1;
  DoDashStepCmd(kStateMsg_Activate);
  PlayDashStepAnimation(mgr);
}

// fn_10_14D14 (0x00014D14, 0x148 bytes): the double dash. The attack bookkeeping with id 0x8000
// (fn_10_1B478 at 0x00014D48); on Activate (`cmpwi r31, 0x0` at 0x00014D4C) the dash record is
// reset as in fn_10_14C50 (0x00014D74..0x00014DBC), one step is left (0x00014DC0) and a dash is
// picked (fn_10_14FE0). Then the collision failsafe (0x00014DD0); unless blocked (0x80, the
// `extrwi.` at 0x00014DD8) the step command (fn_10_12C2C) and, while a step is left and the base's
// GetAnimOver answers (CTriggerData(0.0f), 0x00014E14), the next step (fn_10_14C50). Deactivate
// (`cmpwi r31, 0x2` at 0x00014E30) clears the steps left.
void CDarkSamus::UpdateDoubleDash(CStateManager& mgr, EStateMsg msg) {
  UpdateCurrentAttack(0x8000, mgr, msg);
  if (msg == kStateMsg_Activate) {
    mDash.mStartPosition = CVector3f::Zero();
    mDash.mDestination = mDash.mStartPosition;
    mDash.mLastSlot = -1;
    mDash.mSlot = -1;
    mDash.mWeight = 0.f;
    mDash.mBlocked = false;
    mDash.mStepsLeft = 1;
    PickNextDash(mgr);
  }
  RunCollisionFailsafe(mgr);
  if (mDash.mBlocked == false) {
    DoDashStepCmd(msg);
    if (mDash.mStepsLeft > 0 && GetAnimOver(mgr, CTriggerData(0.f)) == true) {
      StartNextDashStep(mgr);
    }
  }
  if (msg == kStateMsg_Deactivate) {
    mDash.mStepsLeft = 0;
  }
}

// fn_10_21DF8 (0x00021DF8, 0x11C bytes): the step command, the same shape as fn_10_21F14. Activate
// (0x00021E38) sets the animation state 0x6B4 to 1 (kAS_Ready) and delivers the command (DeliverCmd
// at 0x00021E50, the step slot copy at 0x00021E54..0x00021E88); Update (0x00021E90) delivers it
// again while CanIssueCommand(state) answers true (0x00021E98); Deactivate (0x00021EF4) sets the
// animation state to 0 (kAS_NotReady).
// @ fn_10_21DF8
void CDarkSamus::DoStepCmd(EStateMsg msg, pas::EAnimationState state, const CBCStepCmd& cmd) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(cmd);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), state)) {
      BodyController()->CommandMgr().DeliverCmd(cmd);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  default:
    break;
  }
}

// fn_10_21F14 (0x00021F14, 0x10C bytes): Activate (0x00021F54) sets the animation state to 1
// (0x6B4, the base's CAnimationState: kAS_Ready) and delivers the command (DeliverCmd at
// 0x00021F6C, the projectile-attack slot copy at 0x00021F78..0x00021F9C); Update (0x00021FA4)
// delivers it again while CanIssueCommand(state) answers true (0x00021FAC); Deactivate
// (0x00022000) sets the animation state to 0 (kAS_NotReady).
void CDarkSamus::DoProjectileAttackCmd(EStateMsg msg, pas::EAnimationState state,
                                       const CBCProjectileAttackCmd& cmd) {
  switch (msg) {
  case kStateMsg_Activate:
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    BodyController()->CommandMgr().DeliverCmd(cmd);
    break;
  case kStateMsg_Update:
    if (mAnimationState.CanIssueCommand(*BodyController(), state)) {
      BodyController()->CommandMgr().DeliverCmd(cmd);
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    break;
  default:
    break;
  }
}
