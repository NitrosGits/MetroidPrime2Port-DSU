// Host body for the SandBoss REL (module 55, Amorbis): the FSM triggers, states and code functions
// (lbl_55_data_100 / lbl_55_data_340 / lbl_55_data_5AC, registered by fn_55_CAA4 in
// PortCSandBoss.cpp). Functions marked `// @ fn_55_X` were checked against retail's behaviour;
// the rest are log-once stand-ins (triggers return false, states and codes do nothing) until the
// states slice lands. docs/module-analysis/SandBoss.md section 4 is the function map.
#include "MetroidPrime/Enemies/CSandBoss.hpp"

#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"

void PortSandBossStandIn(const char* what);
const CSandBoss* CastSandBoss(const CEntity* entity);

// Triggers

// fn_55_CA90 (ported, not exercised yet)
bool CSandBoss::StateOver(CStateManager&, const CTriggerData&) const {
  return mAnimationState.GetState() == CAnimationState::kAS_Over;
}

// @ fn_55_CA84
bool CSandBoss::IsUnderGround(CStateManager&, const CTriggerData&) const { return mUnderGround; }

// fn_55_CA70 (ported, not exercised yet)
bool CSandBoss::IsRecovered(CStateManager&, const CTriggerData&) const { return !mNotRecovered; }

// @ fn_55_CA1C
bool CSandBoss::CanStartNewRound(CStateManager&, const CTriggerData&) const {
  return mData.commandIndex <= mRound && mRound < 3;
}

// fn_55_CA10 (ported, not exercised yet)
bool CSandBoss::BeginStampede(CStateManager&, const CTriggerData&) const { return mBeginStampede; }

// @ fn_55_C9D8
bool CSandBoss::FoundStampedePoint(CStateManager& mgr, const CTriggerData&) const {
  return GetCoverPoint(mgr, mStampedePoint) != nullptr;
}

// fn_55_C9B8 (ported, not exercised yet)
bool CSandBoss::StampedeOver(CStateManager&, const CTriggerData&) const { return mStampedeHP <= 0.f; }

bool CSandBoss::ShouldDarkBeamAttack(CStateManager&, const CTriggerData&) const { // fn_55_C894
  PortSandBossStandIn("CSandBoss::ShouldDarkBeamAttack (fn_55_C894)");
  return false;
}

bool CSandBoss::ShouldRepeaterAttack(CStateManager&, const CTriggerData&) const { // fn_55_C750
  PortSandBossStandIn("CSandBoss::ShouldRepeaterAttack (fn_55_C750)");
  return false;
}

// fn_55_C730 (ported, not exercised yet)
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

// fn_55_C258 (ported, not exercised yet)
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

// fn_55_C174 (ported, not exercised yet)
bool CSandBoss::IsLastStage(CStateManager&, const CTriggerData&) const { return mRound == 2; }

// fn_55_C154 (ported, not exercised yet)
bool CSandBoss::HeadArmorDestroyed(CStateManager&, const CTriggerData&) const {
  return mHeadArmorHP <= 0.f;
}

// fn_55_C148 (ported, not exercised yet)
bool CSandBoss::ArmorDestroyed(CStateManager&, const CTriggerData&) const { return mArmorDestroyed; }

// The sync triggers hold until every sibling is waiting in the same mode (or not waiting at all).
// @ fn_55_C0A0
bool CSandBoss::SyncStampede(CStateManager& mgr, const CTriggerData&) const {
  for (int i = 0; i < mSiblings.size(); ++i) {
    const CSandBoss* sibling = CastSandBoss(mgr.GetObjectById(mSiblings[i]));
    if (sibling != nullptr && sibling->mWaitMode != 1 && sibling->mWaitMode != 0) {
      return false;
    }
  }
  return true;
}

// fn_55_BFF8 (ported, not exercised yet)
bool CSandBoss::SyncAttachToSphere(CStateManager& mgr, const CTriggerData&) const {
  for (int i = 0; i < mSiblings.size(); ++i) {
    const CSandBoss* sibling = CastSandBoss(mgr.GetObjectById(mSiblings[i]));
    if (sibling != nullptr && sibling->mWaitMode != 2 && sibling->mWaitMode != 0) {
      return false;
    }
  }
  return true;
}

// States

// fn_55_BF64 (ported, not exercised yet)
void CSandBoss::Recover(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mSuckAir = false;
  } else if (msg == kStateMsg_Update) {
    if (mStateMachine->GetTime() > 2.5f) {
      mNotRecovered = false;
    }
  }
}

// @ fn_55_BEA4
void CSandBoss::UnderGround(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    mCollisionManager->SetActive(mgr, false);
    mStampedeHP = mData.stampedeProperties.breakStampedeHP;
    mUnderGround = true;
    RemoveMaterial(kMT_Target, kMT_Orbit, mgr);
  } else if (msg == kStateMsg_Deactivate) {
    mUnderGround = false;
    mNotRecovered = false;
  }
}

// fn_55_BDAC (ported, not exercised yet)
void CSandBoss::Dead(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6B4 = 1
    DestroySphereHelper(mgr);                            // fn_55_51BC
    SendScriptMsgs(kSS_DeathRattle, mgr, GetUniqueId(), kSM_None);
    SendScriptMsgs(kSS_Dead, mgr, GetUniqueId(), kSM_None);
    HandleObjectDrop(mgr); // fn_55_5FEC
    mgr.SetBossParams(kInvalidUniqueId, 0.f, 0);
    mgr.DeleteObjectRequest(GetUniqueId());
    mSkipParticleRender = true; // 0x165D b2
  }
}

// fn_55_BD30 (ported, not exercised yet)
void CSandBoss::Deactivate(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Update && mStateMachine->GetTime() > 3.f) {
    mgr.DeleteObjectRequest(GetUniqueId());
  }
}

// fn_55_BAAC (ported, not exercised yet)
void CSandBoss::AttachToSphere(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Update) {
    // 0x7C4 is mData.commandIndex: the wait is half a second per command index.
    if (AttachToSphereHelper() <= mStateMachine->GetTime()) { // fn_55_5C30
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
        BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Four, -1));
      } else if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) {
        BodyController()->SetLocomotionType(pas::kLT_Relaxed);
      }
    }
  } else if (msg == kStateMsg_Activate) {
    if (CActor* sphere = TCastToPtr< CActor >(mgr.ObjectById(mSphere))) { // 0xE8A
      SetTranslation(sphere->GetTranslation());
      SetTransform(mSpawnRotation.BuildTransform4f(GetTranslation())); // 0x14D0
    }
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6B4 = 1
    mCollisionManager->SetActive(mgr, true);              // 0xD98
    SetCollisionVulnerabilities(mgr, CDamageVulnerability::ReflectVulnerabilty(),
                                CDamageVulnerability::ReflectVulnerabilty()); // fn_55_783C
    AddMaterial(kMT_Target, kMT_Orbit, mgr);
    mF44 = 0;        // 0xF44
    m165D_b1 = true; // 0x165D b1
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6B4 = 0
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mAttachedToSphere = true; // 0x165D b7
    mArmorDestroyed = false;  // 0x165D b6
  }
}

// fn_55_B98C (ported, not exercised yet)
void CSandBoss::RotateToPlayer(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    const CVector3f target = mgr.GetPlayer(0)->GetTranslation(); // mgr 0x14FC = mPlayers[0]
    if (NoSiblingHasSphereLocked(mgr) && IsAttachedLeader(mgr)) { // fn_55_59E4/5A84
      RotateToPlayerStep(mgr, target, dt);                        // fn_55_22E4
    }
    RotateToPlayerHelper(target); // fn_55_83AC
  } else if (msg == kStateMsg_Activate) {
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mBoneTracking.SetAngSpeed(1.5707964f); // 0xDBC (mBoneTracking + 0x20)
    mBoneTracking.SetTarget(mgr.GetPlayer(0)->GetUniqueId());
  } else if (msg == kStateMsg_Deactivate) {
    mBoneTracking.SetActive(false);
    mBoneTracking.SetAngSpeed(3.1415927f); // 0xDBC
  }
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

// fn_55_91F4 (ported, not exercised yet)
void CSandBoss::WaitAttachToSphere(CStateManager&, float) { mWaitMode = 2; }

// fn_55_91E0 (ported, not exercised yet)
void CSandBoss::LockSphere(CStateManager&, float) { mSphereLocked = true; }

// fn_55_91CC (ported, not exercised yet)
void CSandBoss::UnLockSphere(CStateManager&, float) { mSphereLocked = false; }

// fn_55_91B8 (ported, not exercised yet)
void CSandBoss::AllowSyncAttacks(CStateManager&, float) { mSyncAttacksAllowed = true; }

// fn_55_91A4 (ported, not exercised yet)
void CSandBoss::PreventSyncAttacks(CStateManager&, float) { mSyncAttacksAllowed = false; }

// @ fn_55_9190
void CSandBoss::BeginStampedeCode(CStateManager&, float) { mStampedeActive = true; }

// fn_55_917C (ported, not exercised yet)
void CSandBoss::EndStampede(CStateManager&, float) { mStampedeActive = false; }

void CSandBoss::PlayHeadArmorExplosion(CStateManager&, float) { // fn_55_8F94
  PortSandBossStandIn("CSandBoss::PlayHeadArmorExplosion (fn_55_8F94)");
}

// fn_55_8F88 (ported, not exercised yet)
void CSandBoss::ResetHeadArmorHP(CStateManager&, float) { mHeadArmorHP = mData.headArmorHP; }

void CSandBoss::ResetAttackTimes(CStateManager&, float) { // fn_55_8E60
  PortSandBossStandIn("CSandBoss::ResetAttackTimes (fn_55_8E60)");
}

// @ fn_55_8E54
void CSandBoss::SetCinematicStateUnderGround(CStateManager&, float) { mCinematicState = 1; }

// @ fn_55_8E48
void CSandBoss::SetCinematicStateStampede(CStateManager&, float) { mCinematicState = 2; }

// fn_55_8E3C (ported, not exercised yet)
void CSandBoss::SetCinematicStateNormal(CStateManager&, float) { mCinematicState = 3; }

// fn_55_8E30 (ported, not exercised yet)
void CSandBoss::SetCinematicStateStunned(CStateManager&, float) { mCinematicState = 4; }

// fn_55_8E24 (ported, not exercised yet)
void CSandBoss::SetCinematicStateChoking(CStateManager&, float) { mCinematicState = 5; }

// fn_55_8E18 (ported, not exercised yet)
void CSandBoss::SetCinematicStateExitSphere(CStateManager&, float) { mCinematicState = 6; }

// Helpers

void CSandBoss::SnapJawsCollisionSetup(CStateManager&) { // fn_55_78E0
  PortSandBossStandIn("CSandBoss::SnapJawsCollisionSetup (fn_55_78E0)");
}

bool CSandBoss::CheckBodyStateForDarkBeam(CStateManager&) const { // fn_55_8C88
  PortSandBossStandIn("CSandBoss::CheckBodyStateForDarkBeam (fn_55_8C88)");
  return false;
}
