// The 37 states of the SpacePirate state machine (retail table fn_72_10E50 -> lbl_72_data_604).
// Milestone 2 is the skeleton: a state CPatterned also has forwards to it, every other one is a
// no-op behind a once-per-name StandIn that names the retail function. Later lanes port the bodies
// from docs/module-analysis/SpacePirate.md and build/G2ME01/SpacePirate/m2c/fn_72_<off>.c.
#include "MetroidPrime/Enemies/CSpacePirate.hpp"

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

void CSpacePirate::Taunt(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Taunt state (fn_72_B59C)");
}

void CSpacePirate::Cover(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Cover state (fn_72_A3C0)");
}

void CSpacePirate::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  PortSpacePirateStandIn("CSpacePirate::Dead state (fn_72_5C10)");
  CPatterned::Dead(mgr, msg, dt);
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
