// Host body for the SandBoss REL (module 55, Amorbis): the FSM triggers, states and code functions
// (lbl_55_data_100 / lbl_55_data_340 / lbl_55_data_5AC, registered by fn_55_CAA4 in
// PortCSandBoss.cpp). Functions marked `// @ fn_55_X` were checked against retail's behaviour;
// `// fn_55_X (ported, not exercised yet)` follows the retail pseudocode but has not run in game;
// the rest are log-once stand-ins (triggers return false, states and codes do nothing).
// docs/module-analysis/SandBoss.md section 4 is the function map.
#include "MetroidPrime/Enemies/CSandBoss.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"

void PortSandBossStandIn(const char* what);
CSandBoss* CastSandBoss(CEntity* entity);
const CSandBoss* CastSandBoss(const CEntity* entity);

// Triggers

// @ fn_55_CA90
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

// lbl_55_rodata_0, indexed by mRound: the chance (percent) of picking out of the far three
// quarters of the stampede points instead of the nearest quarter.
static const float skStampedePickChance[4] = {70.f, 50.f, 33.f, 0.f};

// @ fn_55_9E3C
// Collects the cover points in reach of the player, sorts them by squared distance and keeps one:
// the nearest quarter (or, by the table above, the rest) for this round.
void CSandBoss::SelectStampedePoint(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Update) {
    if (mStampedePoint == kInvalidUniqueId) {
      const CVector3f playerPos = mgr.GetPlayer(0)->GetTranslation();          // mgr 0x14FC
      CObjectList& coverPoints = mgr.ObjectListById(kOL_AiWaypoint);           // mgr 0x838
      for (int i = coverPoints.GetFirstObjectIndex(); i != -1;
           i = coverPoints.GetNextObjectIndex(i)) {
        CScriptCoverPoint* cover = TCastToPtr< CScriptCoverPoint >(coverPoints[i]);
        if (cover == nullptr || !cover->GetActive() || cover->GetInUse(GetUniqueId()) ||
            cover->GetCurrentAreaId() != GetCurrentAreaId() ||
            cover->GetUniqueId() == mCoverPoint) { // 0xE90: not the point of the last round
          continue;
        }
        // Score against the waypoint the point leads to when it has one, the point itself otherwise.
        CVector3f goal = cover->GetTranslation();
        if (CScriptAIWaypoint* waypoint = TCastToPtr< CScriptAIWaypoint >(mgr.ObjectById(
                cover->FindConnectedObject(mgr, kSS_Arrived, kSM_Next)))) {
          goal = waypoint->GetTranslation();
        }
        mIdEntries.push_back(
            SSandBossIdEntry{cover->GetUniqueId(), (goal - playerPos).MagSquared()});
        if (mIdEntries.size() == mIdEntries.capacity()) { // 0x147C == 0x1480 (sixteen)
          break;
        }
      }
      if (mIdEntries.size() != 0) {
        SortStampedePoints(); // fn_55_10CB4
        const int count = mIdEntries.size();
        const int quarter = count / 4;
        int first = 0;
        int last = quarter;
        if (skStampedePickChance[mRound] < mgr.Random()->Range(0.f, 100.f)) { // mgr 0x16E4
          first = quarter;
          last = count - 1;
        }
        const SSandBossIdEntry& pick = mIdEntries[mgr.Random()->Range(first, last)];
        if (CScriptCoverPoint* cover =
                TCastToPtr< CScriptCoverPoint >(mgr.ObjectById(pick.mId))) {
          SetCoverPoint(cover, mStampedePoint); // 0xE8E
          mCoverPoint = mStampedePoint;         // 0xE90
        }
      }
    }
  } else if (msg == kStateMsg_Activate) {
    ReleaseCoverPoint(mgr, mStampedePoint, true);
    mIdEntries.clear(); // fn_55_A164 (0x147C = 0)
  }
}

// @ fn_55_9BB8
// Walks to the selected point at 100 units/s; on arrival sets BeginStampede once the delay the
// stampeding siblings imply has elapsed (0xF20 counts up in fn_55_86D0).
void CSandBoss::SeekStampedePoint(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (CScriptCoverPoint* cover = GetCoverPoint(mgr, mStampedePoint)) {
      const CVector3f delta = cover->GetTranslation() - GetTranslation();
      const float distance = delta.Magnitude();
      const float step = dt * 100.f;
      if (distance <= step) {
        if (GetStampedeDelay(mgr) <= mF20) { // 0xF20
          mBeginStampede = true;             // 0x165C b6
        }
      } else {
        SetTranslation(GetTranslation() + delta * (step / distance));
      }
    }
  } else if (msg == kStateMsg_Activate) {
    mBeginStampede = false; // 0x165C b6
  } else if (msg == kStateMsg_Deactivate) {
    // Face the waypoint the point leads to and remember its height for the stampede path (0xF28).
    if (CScriptCoverPoint* cover = GetCoverPoint(mgr, mStampedePoint)) {
      if (CScriptAIWaypoint* waypoint = TCastToPtr< CScriptAIWaypoint >(mgr.ObjectById(
              cover->FindConnectedObject(mgr, kSS_Arrived, kSM_Next)))) {
        // Level look: the waypoint's x/y at the point's own height.
        const CVector3f& from = cover->GetTranslation();
        const CVector3f& to = waypoint->GetTranslation();
        SetTransform(CTransform4f::LookAt(from, CVector3f(to.GetX(), to.GetY(), from.GetZ()),
                                          CVector3f::Up()));
        mF28 = waypoint->GetTranslation().GetZ();
      } else {
        mF28 = cover->GetTranslation().GetZ();
      }
    }
  }
}

// @ fn_55_98FC
// Stomps along the path: the body starts generating (the stampede gait) and StampedeHelper sheds
// armour pieces as the stampede HP drops. Activate also releases the siblings' wait timers.
void CSandBoss::Stampede(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) { // 0x6B4
      BodyController()->CommandMgr().DeliverCmd(CBCGenerateCmd(pas::kGType_Three, -1));
    }
    StampedeHelper(mgr, dt); // fn_55_7C68
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6B4 = 1
    mCollisionManager->SetActive(mgr, true);              // 0xD98
    AddMaterial(kMT_Target, kMT_Orbit, mgr);
    RemoveMaterial(kMT_GroundCollider, mgr);
    mPrevOnGround = true; // 0x34C b6 [name uncertain: CPatterned's bit order puts this here]
    for (int i = 0; i < mSiblings.size(); ++i) { // 0xDD8 / 0xDDC
      CSandBoss* sibling = CastSandBoss(mgr.ObjectById(mSiblings[i]));
      if (sibling != nullptr && sibling != this) {
        sibling->mF20 = 0.f; // 0xF20
      }
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6B4 = 0
    RemoveMaterial(kMT_Target, kMT_Orbit, mgr);
    AddMaterial(kMT_GroundCollider, mgr);
    mPrevOnGround = false; // 0x34C b6
    mF20 = 0.f;            // 0xF20
  }
}

// Code functions

// The armour setters swap the collision actors' damage vulnerability, the per-locator armour state
// (1 stampede set, 2 attached set) and the response types, then reset the hit colour.
// fn_55_986C (ported, not exercised yet)
void CSandBoss::SetTripleChargeArmor(CStateManager& mgr, float) {
  for (int i = 0; i < 8; ++i) {
    mArmorState[i] = 2; // 0x1458..0x1474
  }
  SetCollisionVulnerabilities(mgr, mReflectVulnerability1, mReflectVulnerability1); // fn_55_783C
  UpdateArmorResponseTypes(mgr);                                                  // fn_55_6698
  mDamageColor = CPatterned::skHitsWithoutDamageColor;                             // 0x450
  SetVisorOrbitableFlags(CVisorParameters::kVOF_All, true);
}

// fn_55_97DC (ported, not exercised yet)
void CSandBoss::SetDoubleChargeArmor(CStateManager& mgr, float) {
  for (int i = 0; i < 8; ++i) {
    mArmorState[i] = 2; // 0x1458..0x1474
  }
  SetCollisionVulnerabilities(mgr, mReflectVulnerability0, mReflectVulnerability0); // fn_55_783C
  UpdateArmorResponseTypes(mgr);                                                  // fn_55_6698
  mDamageColor = CPatterned::skHitsWithoutDamageColor;                             // 0x450
  SetVisorOrbitableFlags(CVisorParameters::kVOF_All, true);
}

// @ fn_55_974C
void CSandBoss::SetStampedeArmor(CStateManager& mgr, float) {
  for (int i = 0; i < 8; ++i) {
    mArmorState[i] = 1; // 0x1458..0x1474
  }
  SetCollisionVulnerabilities(mgr, mStampedeVulnerability, mStampedeVulnerability); // fn_55_783C
  UpdateArmorResponseTypes(mgr);                                                  // fn_55_6698
  mDamageColor = CPatterned::skHitsWithoutDamageColor;                            // 0x450
  SetVisorOrbitableFlags(CVisorParameters::kVOF_All, true);
}

// fn_55_96B0 (ported, not exercised yet)
void CSandBoss::SetAttachedArmor(CStateManager& mgr, float) {
  for (int i = 0; i < 8; ++i) {
    mArmorState[i] = 2; // 0x1458..0x1474
  }
  // The head keeps the normal vulnerability; the pieces are reflective.
  SetCollisionVulnerabilities(mgr, mNormalVulnerability,
                              CDamageVulnerability::ReflectVulnerabilty()); // fn_55_783C
  UpdateArmorResponseTypes(mgr);                                        // fn_55_6698
  mDamageColor = CPatterned::skHitsWithoutDamageColor;                  // 0x450
  SetVisorOrbitableFlags(CVisorParameters::kVOF_All, true);
}

// fn_55_9600 (ported, not exercised yet)
void CSandBoss::SetSuckAirArmor(CStateManager& mgr, float) {
  mArmorState[0] = 0; // 0x1458: the head loses its armour piece
  for (int i = 1; i < 8; ++i) {
    mArmorState[i] = 2; // 0x145C..0x1474
  }
  SetCollisionVulnerabilities(mgr, mSuckAirVulnerability,
                              CDamageVulnerability::ReflectVulnerabilty()); // fn_55_783C
  UpdateArmorResponseTypes(mgr);                                        // fn_55_6698
  mDamageColor = CPatterned::skDamageColor;                             // 0x450
  SetVisorOrbitableFlags(CVisorParameters::kVOF_All, false);
  SetVisorOrbitableFlags(CVisorParameters::kVOF_Echo, true);
}

// fn_55_952C (ported, not exercised yet)
// Excluding kMT_NoStepLogic from the boss's own filter copy stops the player being pushed by the
// stampede's collision actors.
void CSandBoss::RemovePlayerCollision(CStateManager& mgr, float) {
  CMaterialFilter filter = GetMaterialFilter();
  filter.ExcludeList().Add(kMT_NoStepLogic);
  for (uint i = 0; i < mCollisionManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc = mCollisionManager->GetCollisionDescFromIndex(i); // 0xD98
    if (CCollisionActor* colAct =
            TCastToPtr< CCollisionActor >(mgr.ObjectById(desc.GetCollisionActorId()))) {
      colAct->SetMaterialFilter(filter);
    }
  }
}

// fn_55_9438 (ported, not exercised yet)
void CSandBoss::RestorePlayerCollision(CStateManager& mgr, float) {
  CMaterialFilter filter = GetMaterialFilter();
  filter.ExcludeList().Remove(kMT_NoStepLogic);
  for (uint i = 0; i < mCollisionManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc = mCollisionManager->GetCollisionDescFromIndex(i);
    if (CCollisionActor* colAct =
            TCastToPtr< CCollisionActor >(mgr.ObjectById(desc.GetCollisionActorId()))) {
      colAct->SetMaterialFilter(filter);
    }
  }
  // Retail also sets bit 7 of the byte at 0x1144 of the player. CPlayer.hpp only names the two
  // morph-transition bits of that byte, so the store is left out until the bit is identified.
}

// fn_55_920C (ported, not exercised yet)
// Picks the active CScriptSafeZone (ATTK / ATCH) most nearly ahead of the boss (within 70 degrees)
// that is closest to the player.
void CSandBoss::SelectSafeZoneTarget(CStateManager& mgr, float) {
  const CVector3f playerPos = mgr.GetPlayer(0)->GetTranslation(); // mgr 0x14FC
  const CVector3f origin = GetTranslation();
  const CVector3f forward = GetTransform().GetForward(); // 0x28 / 0x38 / 0x48
  float bestDistanceSq = 3.4028235e38f;
  mSafeZoneTarget = kInvalidUniqueId; // 0xE88
  const rstl::vector< SConnection >& connections = GetConnectionList(); // 0x1C / 0x14
  for (int i = 0; i < connections.size(); ++i) {
    const SConnection& conn = connections[i];
    if (conn.state != kSS_Attack || conn.msg != kSM_Attach) { // 'ATTK' / 'ATCH'
      continue;
    }
    CScriptSafeZone* zone = TCastToPtr< CScriptSafeZone >(
        mgr.ObjectById(mgr.GetIdForScript(conn.objId)));
    if (zone == nullptr || !zone->GetActive()) {
      continue;
    }
    if (CVector3f::GetAngleDiff(forward, zone->GetTranslation() - origin) <= 1.2217304706573486f) {
      const float distanceSq = (zone->GetTranslation() - playerPos).MagSquared();
      if (distanceSq < bestDistanceSq) {
        mSafeZoneTarget = zone->GetUniqueId();
        bestDistanceSq = distanceSq;
      }
    }
  }
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

// fn_55_8F94 (ported, not exercised yet)
// Head-armour explosion at the Head_1 locator plus the IS02 waypoint taking the boss's transform.
void CSandBoss::PlayHeadArmorExplosion(CStateManager& mgr, float) {
  const CTransform4f xf = GetLctrTransform(mHeadLocator); // 0xF08
  if (mHeadArmorExplosion.valid()) {                      // 0xE50
    CExplosion* explosion = rs_new CExplosion(
        *mHeadArmorExplosion, mgr.AllocateUniqueId(),
        CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true),
        rstl::string_l("ArmorExplosionFx"), xf, 0, GetModelData()->GetScale(), CColor::White(), -1);
    if (explosion != nullptr) {
      mgr.AddObject(explosion);
    }
  }
  if (CScriptWaypoint* waypoint = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(mIS02Target))) {
    waypoint->SetTransform(GetTransform()); // 0x24
    SendScriptMsgs(kSS_InternalState2, mgr, GetUniqueId(), kSM_None);
  }
}

// fn_55_8F88 (ported, not exercised yet)
void CSandBoss::ResetHeadArmorHP(CStateManager&, float) { mHeadArmorHP = mData.headArmorHP; }

// fn_55_8E60 (ported, not exercised yet)
// Re-seeds the attack timers from the charge-beam data of the stage the attached count implies;
// two attached siblings is the double charge, three the triple charge, anything else the dark beam.
void CSandBoss::ResetAttackTimes(CStateManager& mgr, float) {
  const int attached = CountAttachedSiblings(mgr); // fn_55_595C
  const SLdrSandBossChargeBeamData& doubleCharge = mData.unknown_0x7619e561.doubleCharge; // 0x85C
  const SLdrSandBossChargeBeamData& tripleCharge = mData.unknown_0x7619e561.tripleCharge; // 0x8A4
  if (attached == 2) {
    mChargeBeamTimer = doubleCharge.minChargeBeamAttackTime +
                       mgr.Random()->Range(0.f, doubleCharge.chargeBeamAttackTimeVariance);
    mDarkBeamTimer = doubleCharge.minDarkBeamAttackTime +
                     mgr.Random()->Range(0.f, doubleCharge.darkBeamAttackTimeVariance);
  } else if (attached < 2 || attached > 3) {
    mChargeBeamTimer = 3.4028235e38f; // 0x7f7fffff: no charge beam
    mDarkBeamTimer = mData.minDarkBeamAttackTime + // 0x7F4
                     mgr.Random()->Range(0.f, mData.darkBeamAttackTimeVariance); // 0x7F8
  } else {
    mChargeBeamTimer = tripleCharge.minChargeBeamAttackTime +
                       mgr.Random()->Range(0.f, tripleCharge.chargeBeamAttackTimeVariance);
    mDarkBeamTimer = tripleCharge.minDarkBeamAttackTime +
                     mgr.Random()->Range(0.f, tripleCharge.darkBeamAttackTimeVariance);
  }
  m165D_b1 = false; // 0x165D b1
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
