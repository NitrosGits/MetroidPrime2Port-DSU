// Host body for the SandBoss REL (module 55, Amorbis): the FSM triggers, states and code functions
// (lbl_55_data_100 / lbl_55_data_340 / lbl_55_data_5AC, registered by fn_55_CAA4 in
// PortCSandBoss.cpp). Functions marked `// @ fn_55_X` were checked against retail's behaviour;
// the rest are log-once stand-ins (triggers return false, states and codes do nothing) until the
// states slice lands. docs/module-analysis/SandBoss.md section 4 is the function map.
#include "MetroidPrime/Enemies/CSandBoss.hpp"

#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"

void PortSandBossStandIn(const char* what);

// Triggers

// @ fn_55_CA90
bool CSandBoss::StateOver(CStateManager&, const CTriggerData&) const {
  return mAnimationState.GetState() == CAnimationState::kAS_Over;
}

// @ fn_55_CA84
bool CSandBoss::IsUnderGround(CStateManager&, const CTriggerData&) const { return mUnderGround; }

// @ fn_55_CA70
bool CSandBoss::IsRecovered(CStateManager&, const CTriggerData&) const { return !mNotRecovered; }

// @ fn_55_CA1C
bool CSandBoss::CanStartNewRound(CStateManager&, const CTriggerData&) const {
  return mData.commandIndex <= mRound && mRound < 3;
}

// @ fn_55_CA10
bool CSandBoss::BeginStampede(CStateManager&, const CTriggerData&) const { return mBeginStampede; }

// @ fn_55_C9D8
bool CSandBoss::FoundStampedePoint(CStateManager& mgr, const CTriggerData&) const {
  return GetCoverPoint(mgr, mStampedePoint) != nullptr;
}

// @ fn_55_C9B8
bool CSandBoss::StampedeOver(CStateManager&, const CTriggerData&) const { return mStampedeHP <= 0.f; }

bool CSandBoss::ShouldDarkBeamAttack(CStateManager&, const CTriggerData&) const { // fn_55_C894
  PortSandBossStandIn("CSandBoss::ShouldDarkBeamAttack (fn_55_C894)");
  return false;
}

bool CSandBoss::ShouldRepeaterAttack(CStateManager&, const CTriggerData&) const { // fn_55_C750
  PortSandBossStandIn("CSandBoss::ShouldRepeaterAttack (fn_55_C750)");
  return false;
}

// @ fn_55_C730
bool CSandBoss::FoundSafeZoneTarget(CStateManager&, const CTriggerData&) const {
  return mSafeZoneTarget != kInvalidUniqueId;
}

bool CSandBoss::ShouldTripleCharge(CStateManager&, const CTriggerData&) const { // fn_55_C534
  PortSandBossStandIn("CSandBoss::ShouldTripleCharge (fn_55_C534)");
  return false;
}

bool CSandBoss::ShouldDoubleCharge(CStateManager&, const CTriggerData&) const { // fn_55_C378
  PortSandBossStandIn("CSandBoss::ShouldDoubleCharge (fn_55_C378)");
  return false;
}

bool CSandBoss::ShouldSnapJaws(CStateManager&, const CTriggerData&) const { // fn_55_C2A0
  PortSandBossStandIn("CSandBoss::ShouldSnapJaws (fn_55_C2A0)");
  return false;
}

// @ fn_55_C258
bool CSandBoss::ShouldDestroySphere(CStateManager& mgr, const CTriggerData&) const {
  return mAttachedToSphere && CountAttachedSiblings(mgr) == 1;
}

bool CSandBoss::IsFacingPlayer(CStateManager&, const CTriggerData&) const { // fn_55_C1CC
  PortSandBossStandIn("CSandBoss::IsFacingPlayer (fn_55_C1CC)");
  return false;
}

bool CSandBoss::IsBallInSuckRange(CStateManager&, const CTriggerData&) const { // fn_55_C188
  PortSandBossStandIn("CSandBoss::IsBallInSuckRange (fn_55_C188)");
  return false;
}

// @ fn_55_C174
bool CSandBoss::IsLastStage(CStateManager&, const CTriggerData&) const { return mRound == 2; }

// @ fn_55_C154
bool CSandBoss::HeadArmorDestroyed(CStateManager&, const CTriggerData&) const {
  return mHeadArmorHP <= 0.f;
}

// @ fn_55_C148
bool CSandBoss::ArmorDestroyed(CStateManager&, const CTriggerData&) const { return mArmorDestroyed; }

bool CSandBoss::SyncStampede(CStateManager&, const CTriggerData&) const { // fn_55_C0A0
  PortSandBossStandIn("CSandBoss::SyncStampede (fn_55_C0A0)");
  return false;
}

bool CSandBoss::SyncAttachToSphere(CStateManager&, const CTriggerData&) const { // fn_55_BFF8
  PortSandBossStandIn("CSandBoss::SyncAttachToSphere (fn_55_BFF8)");
  return false;
}

// States

void CSandBoss::Recover(CStateManager&, EStateMsg, float) { // fn_55_BF64
  PortSandBossStandIn("CSandBoss::Recover (fn_55_BF64)");
}

void CSandBoss::UnderGround(CStateManager&, EStateMsg, float) { // fn_55_BEA4
  PortSandBossStandIn("CSandBoss::UnderGround (fn_55_BEA4)");
}

void CSandBoss::Dead(CStateManager&, EStateMsg, float) { // fn_55_BDAC
  PortSandBossStandIn("CSandBoss::Dead (fn_55_BDAC)");
}

void CSandBoss::Deactivate(CStateManager&, EStateMsg, float) { // fn_55_BD30
  PortSandBossStandIn("CSandBoss::Deactivate (fn_55_BD30)");
}

void CSandBoss::AttachToSphere(CStateManager&, EStateMsg, float) { // fn_55_BAAC
  PortSandBossStandIn("CSandBoss::AttachToSphere (fn_55_BAAC)");
}

void CSandBoss::RotateToPlayer(CStateManager&, EStateMsg, float) { // fn_55_B98C
  PortSandBossStandIn("CSandBoss::RotateToPlayer (fn_55_B98C)");
}

void CSandBoss::SuckAir(CStateManager&, EStateMsg, float) { // fn_55_B718
  PortSandBossStandIn("CSandBoss::SuckAir (fn_55_B718)");
}

void CSandBoss::SuckBall(CStateManager&, EStateMsg, float) { // fn_55_B658
  PortSandBossStandIn("CSandBoss::SuckBall (fn_55_B658)");
}

void CSandBoss::BallInMouth(CStateManager&, EStateMsg, float) { // fn_55_B544
  PortSandBossStandIn("CSandBoss::BallInMouth (fn_55_B544)");
}

void CSandBoss::DestroySphere(CStateManager&, EStateMsg, float) { // fn_55_B2D8
  PortSandBossStandIn("CSandBoss::DestroySphere (fn_55_B2D8)");
}

void CSandBoss::JumpOffSphere(CStateManager&, EStateMsg, float) { // fn_55_B06C
  PortSandBossStandIn("CSandBoss::JumpOffSphere (fn_55_B06C)");
}

void CSandBoss::SpitOutBall(CStateManager&, EStateMsg, float) { // fn_55_AED8
  PortSandBossStandIn("CSandBoss::SpitOutBall (fn_55_AED8)");
}

void CSandBoss::DarkBeamAttack(CStateManager&, EStateMsg, float) { // fn_55_AC68
  PortSandBossStandIn("CSandBoss::DarkBeamAttack (fn_55_AC68)");
}

void CSandBoss::RepeaterAttack(CStateManager&, EStateMsg, float) { // fn_55_A9DC
  PortSandBossStandIn("CSandBoss::RepeaterAttack (fn_55_A9DC)");
}

void CSandBoss::DoubleCharge(CStateManager&, EStateMsg, float) { // fn_55_A6CC
  PortSandBossStandIn("CSandBoss::DoubleCharge (fn_55_A6CC)");
}

void CSandBoss::TripleCharge(CStateManager&, EStateMsg, float) { // fn_55_A360
  PortSandBossStandIn("CSandBoss::TripleCharge (fn_55_A360)");
}

void CSandBoss::SnapJaws(CStateManager&, EStateMsg, float) { // fn_55_A170
  PortSandBossStandIn("CSandBoss::SnapJaws (fn_55_A170)");
}

void CSandBoss::SelectStampedePoint(CStateManager&, EStateMsg, float) { // fn_55_9E3C
  PortSandBossStandIn("CSandBoss::SelectStampedePoint (fn_55_9E3C)");
}

void CSandBoss::SeekStampedePoint(CStateManager&, EStateMsg, float) { // fn_55_9BB8
  PortSandBossStandIn("CSandBoss::SeekStampedePoint (fn_55_9BB8)");
}

void CSandBoss::Stampede(CStateManager&, EStateMsg, float) { // fn_55_98FC
  PortSandBossStandIn("CSandBoss::Stampede (fn_55_98FC)");
}

// Code functions

void CSandBoss::SetTripleChargeArmor(CStateManager&, float) { // fn_55_986C
  PortSandBossStandIn("CSandBoss::SetTripleChargeArmor (fn_55_986C)");
}

void CSandBoss::SetDoubleChargeArmor(CStateManager&, float) { // fn_55_97DC
  PortSandBossStandIn("CSandBoss::SetDoubleChargeArmor (fn_55_97DC)");
}

void CSandBoss::SetStampedeArmor(CStateManager&, float) { // fn_55_974C
  PortSandBossStandIn("CSandBoss::SetStampedeArmor (fn_55_974C)");
}

void CSandBoss::SetAttachedArmor(CStateManager&, float) { // fn_55_96B0
  PortSandBossStandIn("CSandBoss::SetAttachedArmor (fn_55_96B0)");
}

void CSandBoss::SetSuckAirArmor(CStateManager&, float) { // fn_55_9600
  PortSandBossStandIn("CSandBoss::SetSuckAirArmor (fn_55_9600)");
}

void CSandBoss::RemovePlayerCollision(CStateManager&, float) { // fn_55_952C
  PortSandBossStandIn("CSandBoss::RemovePlayerCollision (fn_55_952C)");
}

void CSandBoss::RestorePlayerCollision(CStateManager&, float) { // fn_55_9438
  PortSandBossStandIn("CSandBoss::RestorePlayerCollision (fn_55_9438)");
}

void CSandBoss::SelectSafeZoneTarget(CStateManager&, float) { // fn_55_920C
  PortSandBossStandIn("CSandBoss::SelectSafeZoneTarget (fn_55_920C)");
}

// @ fn_55_9200
void CSandBoss::WaitStampede(CStateManager&, float) { mWaitMode = 1; }

// @ fn_55_91F4
void CSandBoss::WaitAttachToSphere(CStateManager&, float) { mWaitMode = 2; }

// @ fn_55_91E0
void CSandBoss::LockSphere(CStateManager&, float) { mSphereLocked = true; }

// @ fn_55_91CC
void CSandBoss::UnLockSphere(CStateManager&, float) { mSphereLocked = false; }

// @ fn_55_91B8
void CSandBoss::AllowSyncAttacks(CStateManager&, float) { mSyncAttacksAllowed = true; }

// @ fn_55_91A4
void CSandBoss::PreventSyncAttacks(CStateManager&, float) { mSyncAttacksAllowed = false; }

// @ fn_55_9190
void CSandBoss::BeginStampedeCode(CStateManager&, float) { mStampedeActive = true; }

// @ fn_55_917C
void CSandBoss::EndStampede(CStateManager&, float) { mStampedeActive = false; }

void CSandBoss::PlayHeadArmorExplosion(CStateManager&, float) { // fn_55_8F94
  PortSandBossStandIn("CSandBoss::PlayHeadArmorExplosion (fn_55_8F94)");
}

// @ fn_55_8F88
void CSandBoss::ResetHeadArmorHP(CStateManager&, float) { mHeadArmorHP = mData.headArmorHP; }

void CSandBoss::ResetAttackTimes(CStateManager&, float) { // fn_55_8E60
  PortSandBossStandIn("CSandBoss::ResetAttackTimes (fn_55_8E60)");
}

// @ fn_55_8E54
void CSandBoss::SetCinematicStateUnderGround(CStateManager&, float) { mCinematicState = 1; }

// @ fn_55_8E48
void CSandBoss::SetCinematicStateStampede(CStateManager&, float) { mCinematicState = 2; }

// @ fn_55_8E3C
void CSandBoss::SetCinematicStateNormal(CStateManager&, float) { mCinematicState = 3; }

// @ fn_55_8E30
void CSandBoss::SetCinematicStateStunned(CStateManager&, float) { mCinematicState = 4; }

// @ fn_55_8E24
void CSandBoss::SetCinematicStateChoking(CStateManager&, float) { mCinematicState = 5; }

// @ fn_55_8E18
void CSandBoss::SetCinematicStateExitSphere(CStateManager&, float) { mCinematicState = 6; }

// Helpers

void CSandBoss::SnapJawsCollisionSetup(CStateManager&) { // fn_55_78E0
  PortSandBossStandIn("CSandBoss::SnapJawsCollisionSetup (fn_55_78E0)");
}

bool CSandBoss::CheckBodyStateForDarkBeam(CStateManager&) const { // fn_55_8C88
  PortSandBossStandIn("CSandBoss::CheckBodyStateForDarkBeam (fn_55_8C88)");
  return false;
}
