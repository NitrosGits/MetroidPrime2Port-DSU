// The Boost Guardian's state machine tables.
//
// fn_30_C6CC (the REL's SetupStateMachine) calls CPatterned's own setup and then hands the machine
// three tables: the 49 triggers (lbl_30_data_250), the 28 states (lbl_30_data_6B0) and the 9 code
// functions (lbl_30_data_8DC). The names are the REL's own strings and the order is the table
// order; the .ctors initialiser fn_30_110C4 is what fills in each entry's function pointer
// (docs/module-analysis/BoostGuardian_fsm.tsv has the name-to-address table).
//
// The Ing half of the names is the Ing REL's - this module carries its own copy of the Ing code -
// so the behaviour of the Ing half comes from the Ing host body
// (/home/odran/.local/share/opencode/worktree/3e7f972e5f2a67a4bf166b19e99a591913d8d4a4/
// wt-mp2-ing/src/MetroidPrime/Enemies/PortCIng.cpp, branch decomp/ing) and the boost half from this
// module's own bodies.
#include "MetroidPrime/Enemies/CIngBoostBallGuardian.hpp"

#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "types.h"

typedef CPatterned::StateMachine::TriggerFunc TriggerFunc;
typedef CPatterned::StateMachine::StateFunc StateFunc;
typedef CPatterned::StateMachine::CodeFunc CodeFunc;

#define BG_TRIGGER(name) {#name, static_cast< TriggerFunc >(&CIngBoostBallGuardian::name)}
#define BG_STATE(name) {#name, static_cast< StateFunc >(&CIngBoostBallGuardian::name)}
#define BG_CODE(name) {#name, static_cast< CodeFunc >(&CIngBoostBallGuardian::name)}

namespace {

// fn_30_C6CC's trigger table (lbl_30_data_250), in the REL's order.
CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    BG_TRIGGER(StateOver),
    BG_TRIGGER(IsAlert),
    BG_TRIGGER(IsControllingHost),
    BG_TRIGGER(IsFacingTarget),
    BG_TRIGGER(HasTarget),
    BG_TRIGGER(HasNewTarget),
    BG_TRIGGER(HasLineOfSight),
    BG_TRIGGER(UnderFire),
    BG_TRIGGER(HeardShot),
    BG_TRIGGER(AreaClear),
    BG_TRIGGER(CoverLeash),
    BG_TRIGGER(HasCoverPoint),
    BG_TRIGGER(HasWallCoverPoint),
    BG_TRIGGER(HasMiniPortals),
    BG_TRIGGER(IsCorporeal),
    BG_TRIGGER(IsIngSpot),
    BG_TRIGGER(IsBodyProjectile),
    BG_TRIGGER(IsFrustrated),
    BG_TRIGGER(IsAggressive),
    BG_TRIGGER(UseProjectileFSMEntry),
    BG_TRIGGER(TargetInSafeZone),
    BG_TRIGGER(TargetInLightSafeZone),
    BG_TRIGGER(CanChangeForm),
    BG_TRIGGER(ShouldFleeSafeZone),
    BG_TRIGGER(ShouldBecomeCorporeal),
    BG_TRIGGER(ShouldBecomeIngSpot),
    BG_TRIGGER(ShouldArmSwipe),
    BG_TRIGGER(ShouldTaunt),
    BG_TRIGGER(ShouldBodyProjectile),
    BG_TRIGGER(ShouldOpenMiniPortal),
    BG_TRIGGER(ProjectileSplat),
    BG_TRIGGER(ProjectileHitTarget),
    BG_TRIGGER(FoundMovementPos),
    BG_TRIGGER(PathShagged),
    BG_TRIGGER(PathOver),
    BG_TRIGGER(PointPathOver),
    BG_TRIGGER(FoundPointPath),
    BG_TRIGGER(IsOffPath),
    BG_TRIGGER(HasAttackPattern),
    BG_TRIGGER(AttackPatternOver),
    BG_TRIGGER(TargetIsBall),
    BG_TRIGGER(HasPathToTarget),
    BG_TRIGGER(CanGrappleTarget),
    BG_TRIGGER(IsBoostPuddleForm),
    BG_TRIGGER(IsCurrentlyInBoostPuddleForm),
    BG_TRIGGER(IsBoostOver),
    BG_TRIGGER(ShouldBoost),
    BG_TRIGGER(HasCollidedWithCrystal),
    BG_TRIGGER(BoostModeRetreatOver),
};

// fn_30_C6CC's state table (lbl_30_data_6B0), in the REL's order.
CPatterned::StateMachine::SStateFunction skStates[] = {
    BG_STATE(Start),
    BG_STATE(WaitForFSMTransition),
    BG_STATE(ExitHost),
    BG_STATE(BecomeCorporeal),
    BG_STATE(BecomeIngSpot),
    BG_STATE(BecomeBodyProjectile),
    BG_STATE(BecomeWallProjectile),
    BG_STATE(Alert),
    BG_STATE(Taunt),
    BG_STATE(Lurk),
    BG_STATE(PathFind),
    BG_STATE(IngSpotPathFind),
    BG_STATE(IngSpotPointPathFind),
    BG_STATE(Patrol),
    BG_STATE(FollowAttackPattern),
    BG_STATE(BodyProjectileFlight),
    BG_STATE(SuckEnergy),
    BG_STATE(SeekWallPoint),
    BG_STATE(SelectTarget),
    BG_STATE(FaceTarget),
    BG_STATE(FindCoverPoint),
    BG_STATE(ArmSwipe),
    BG_STATE(FailSafeMode),
    BG_STATE(FindMiniPortals),
    BG_STATE(MiniPortalAttack),
    BG_STATE(Dead),
    BG_STATE(BoostAttack),
    BG_STATE(BoostModeRetreat),
};

// fn_30_C6CC's code table (lbl_30_data_8DC), in the REL's order.
CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    BG_CODE(SetTargetDest),
    BG_CODE(SetCoverDest),
    BG_CODE(SetRecoverDest),
    BG_CODE(SetPointCoverDest),
    BG_CODE(SetWallPointCoverDest),
    BG_CODE(SetExitHostDest),
    BG_CODE(StartRangedAttack),
    BG_CODE(EndRangedAttack),
    BG_CODE(SplatOntoMesh),
};

} // namespace

// The three tables go into the machine under their own names, which is how the machine finds the
// handler for a node: the FSM2 resource's node names are the REL's names.
void CIngBoostBallGuardian::InstallStateMachineFunctions() {
  StateMachine* stateMachine = mStateMachine.get();
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  stateMachine->SetCodeFunctions(skCodeFuncs, ARRAY_SIZE(skCodeFuncs));
}
