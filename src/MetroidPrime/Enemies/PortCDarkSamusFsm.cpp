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
#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "MetroidPrime/StateMachineCommon.hpp"

#include <stdio.h>

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

} // namespace

// The two tables go into the machine under the REL's own names, which is how the machine finds the
// handler for a node: the FSM2 asset's node names are the REL's names.
void CDarkSamus::InstallStateMachineFunctions() {
  StateMachine* stateMachine = mStateMachine.get();
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
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

bool CDarkSamus::DoneTurning(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_F864", "trigger DoneTurning");
  return false;
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

bool CDarkSamus::StartsInTheAir(CStateManager&, const CTriggerData&) const {
  FsmStandIn("fn_10_F924", "trigger StartsInTheAir");
  return false;
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

void CDarkSamus::BoostBallFire(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_9724", "state BoostBallFire");
}

void CDarkSamus::BoostBallInto(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_B164", "state BoostBallInto");
}

void CDarkSamus::BoostBallRoll(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_A3B0", "state BoostBallRoll");
}

void CDarkSamus::FreezeBeam(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_16CDC", "state FreezeBeam");
}

void CDarkSamus::Dash(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_150DC", "state Dash");
}

void CDarkSamus::DiveAttack(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_11FA4", "state DiveAttack");
}

void CDarkSamus::DiveFromBoost(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_9424", "state DiveFromBoost");
}

void CDarkSamus::EnrageHitReact(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_7A14", "state EnrageHitReact");
}

void CDarkSamus::EnragedTravel(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_6E38", "state EnragedTravel");
}

void CDarkSamus::FadeIn(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_BD40", "state FadeIn");
}

void CDarkSamus::FadeOut(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_BC54", "state FadeOut");
}

void CDarkSamus::FallFromTheAir(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_F86C", "state FallFromTheAir");
}

void CDarkSamus::GetUp(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_18EAC", "state GetUp");
}

void CDarkSamus::GoToPhazonAttack(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_652C", "state GoToPhazonAttack");
}

void CDarkSamus::GoToPhazonEnrage(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_6460", "state GoToPhazonEnrage");
}

void CDarkSamus::HeavyHit(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_10C9C", "state HeavyHit");
}

void CDarkSamus::Interrupt(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_10FF4", "state Interrupt");
}

void CDarkSamus::MeleeAttack(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_17BC0", "state MeleeAttack");
}

void CDarkSamus::MeleeDash(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_17ABC", "state MeleeDash");
}

void CDarkSamus::MissileDeflect(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_178E4", "state MissileDeflect");
}

void CDarkSamus::MissileJump(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_177BC", "state MissileJump");
}

void CDarkSamus::MovingScatterShot(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_11AD8", "state MovingScatterShot");
}

void CDarkSamus::NormalMissileAttack(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_17448", "state NormalMissileAttack");
}

void CDarkSamus::Null(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_14448", "state Null");
}

// The base's Dead body is the host's death path (the die command, the fade and the material
// change); it stands in for the module's until that one is ported, so the actor can still die.
void CDarkSamus::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  FsmStandIn("fn_10_143A8", "state Dead");
  CPatterned::Dead(mgr, msg, dt);
}

// The base's Patrol body is the host's waypoint patrol; it stands in for the module's until
// that one is ported.
void CDarkSamus::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  FsmStandIn("fn_10_1142C", "state Patrol");
  CPatterned::Patrol(mgr, msg, dt);
}

void CDarkSamus::Pause(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_10710", "state Pause");
}

void CDarkSamus::PhazonAttack(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_81D4", "state PhazonAttack");
}

void CDarkSamus::PhazonEnraged(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_7BC0", "state PhazonEnraged");
}

void CDarkSamus::PhazonFall(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_6BA0", "state PhazonFall");
}

void CDarkSamus::PhazonMissiles(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_67B8", "state PhazonMissiles");
}

void CDarkSamus::PhazonRise(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_7054", "state PhazonRise");
}

void CDarkSamus::PhazonSuck(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_4AC8", "state PhazonSuck");
}

void CDarkSamus::PissedOff(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_F930", "state PissedOff");
}

void CDarkSamus::ScatterShot(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_175AC", "state ScatterShot");
}

void CDarkSamus::SideDash(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_153EC", "state SideDash");
}

// The base's Start body is empty, so it stands in for the module's until that one is ported.
void CDarkSamus::Start(CStateManager& mgr, EStateMsg msg, float dt) {
  FsmStandIn("fn_10_F920", "state Start");
  CPatterned::Start(mgr, msg, dt);
}

void CDarkSamus::SuperMissileAttack(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_16ECC", "state SuperMissileAttack");
}

void CDarkSamus::SweepBeam(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_CA0C", "state SweepBeam");
}

void CDarkSamus::Taunt(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_11544", "state Taunt");
}

void CDarkSamus::TravelToCoverPoints(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_1DF74", "state TravelToCoverPoints");
}

void CDarkSamus::TravelToPissedOff(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_FB8C", "state TravelToPissedOff");
}

void CDarkSamus::TurnToFacePlayer(CStateManager&, EStateMsg, float) {
  FsmStandIn("fn_10_FE18", "state TurnToFacePlayer");
}
