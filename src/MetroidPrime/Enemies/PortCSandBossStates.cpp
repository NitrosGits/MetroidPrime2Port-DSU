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
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/Player/CStaticInterference.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"

CSandBoss* CastSandBoss(CEntity* entity);
const CSandBoss* CastSandBoss(const CEntity* entity);

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

// @ fn_55_C894
// Dark-beam gate, and the first thing it checks is that no charge probe is running. A non-leader
// just reports its own probing bit, since the leader's probe is what asks it. The leader fires at
// once when it is the only attached worm and its dark-beam timer has run out; with more worms
// attached it also needs a beam cooldown spent, and then fn_55_42A8's pick decides: this worm
// answers true when it is the pick and otherwise makes the picked worm run its own probe
// (fn_55_8CB4).
bool CSandBoss::ShouldDarkBeamAttack(CStateManager& mgr, const CTriggerData&) const {
  if (mProbingDoubleCharge || mProbingTripleCharge) { // 0x165E b7 / b6
    return false;
  }
  if (!IsAttachedLeader(mgr)) { // fn_55_5A84
    return mProbingDarkBeam;    // 0x165E b5
  }
  if (CountAttachedSiblings(mgr) == 1) { // fn_55_595C
    return mDarkBeamTimer <= 0.f;        // 0xF1C
  }
  if (mChargeBeamCooldown <= 0 || mDarkBeamTimer > 0.f) { // 0xF44, 0xF1C
    return false;
  }
  const TUniqueId pick = ShouldDarkBeamAttackCheck(mgr); // fn_55_42A8
  if (pick == GetUniqueId()) {
    return true;
  }
  if (pick != kInvalidUniqueId) {
    if (CSandBoss* other = CastSandBoss(mgr.ObjectById(pick))) {
      other->ProbeDarkBeam(mgr); // fn_55_8CB4
    }
  }
  return false;
}

// @ fn_55_C750
// Repeater-barrage gate: with no active safe zone among this worm's connections it fires at once
// (fn_55_1B94 counts them); otherwise the player must be within 70 degrees of the forward axis and
// a roll out of 100 decides. Retail keeps the roll above a per-worm threshold, which is
// mData.unknown_0x2b42dddf (0x810) while the player stands in a safe zone and
// mData.unknown_0x1562e0d6 (0x814) outside one [inferred: field identity, the loader's defaults are
// 90 and 10], so the barrage is the ten-percent answer to a covered player.
bool CSandBoss::ShouldRepeaterAttack(CStateManager& mgr, const CTriggerData&) const {
  if (ShouldRepeaterAttackCheck(mgr) == 0) { // fn_55_1B94
    return true;
  }
  const CPlayer* player = mgr.GetPlayer(0); // mgr 0x14FC
  if (player == nullptr) {
    return false;
  }
  const CVector3f delta = player->GetTranslation() - GetTranslation(); // player 0x54..0x5C
  const CVector3f flatDelta(CVector2f(delta.GetX(), delta.GetY()), 0.f);
  if (CVector3f::GetAngleDiff(GetTransform().GetColumn(kDY), flatDelta) > 1.2217304706573486f) {
    return false; // 70 degrees
  }
  const float threshold = mgr.GetSafeZoneManager()->IsObjectInSafeZone(*player, mgr)
                              ? mData.unknown_0x2b42dddf  // 0x810
                              : mData.unknown_0x1562e0d6; // 0x814
  return threshold < mgr.Random()->Range(0.f, 100.f); // mgr 0x16E4, 0x40590000 = 100.0
}

// @ fn_55_C730
bool CSandBoss::FoundSafeZoneTarget(CStateManager&, const CTriggerData&) const {
  return mSafeZoneTarget != kInvalidUniqueId;
}

// @ fn_55_C534
// Triple charge gate: all three worms attached and the leader; while another worm is the probe target
// it just reports its probing bit. The leader needs its attack timer run out, sync attacks allowed
// and no beam cooldown; then every attached sibling must pass its own triple probe, otherwise the
// siblings probed so far are told to leave their loop attack and the trigger fails.
bool CSandBoss::ShouldTripleCharge(CStateManager& mgr, const CTriggerData&) const {
  if (CountAttachedSiblings(mgr) != 3) { // fn_55_595C
    return false;
  }
  if (!IsAttachedLeader(mgr)) { // fn_55_5A84
    return mProbingTripleCharge; // 0x165E b6
  }
  if (mChargeBeamTimer > 0.f || !mSyncAttacksAllowed || mChargeBeamCooldown >= 1) { // 0xF18, 0x165E b4
    return false;
  }
  for (int i = 0; i < mSiblings.size(); ++i) {
    const CSandBoss* sibling = CastSandBoss(mgr.GetObjectById(mSiblings[i]));
    if (sibling != nullptr && sibling != this && sibling->mAttachedToSphere && // 0x165D b7
        !sibling->mSyncAttacksAllowed) {                                       // 0x165E b4
      return false;
    }
  }
  int failed = mSiblings.size();
  for (int i = 0; i < mSiblings.size(); ++i) {
    CSandBoss* sibling = CastSandBoss(mgr.ObjectById(mSiblings[i]));
    if (sibling != nullptr && sibling != this && sibling->mAttachedToSphere &&
        !sibling->ProbeTripleCharge(mgr)) { // fn_55_8D24
      failed = i;
      break;
    }
  }
  if (failed == mSiblings.size()) {
    return true;
  }
  for (int i = 0; i < failed; ++i) {
    CSandBoss* sibling = CastSandBoss(mgr.ObjectById(mSiblings[i]));
    if (sibling != nullptr && sibling != this && sibling->mAttachedToSphere) {
      sibling->BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
      sibling->mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6B4 = 3
    }
  }
  return false;
}

// @ fn_55_C378
// Double charge gate: two attached worms and the leader; a probing worm reports its sync flag. The
// leader needs its attack timer run out, sync attacks allowed and no beam cooldown, and the player
// must be in front of it along the local Y axis; the first attached sibling then answers the same
// question about its own facing and, if it also faces the player, its double probe decides.
bool CSandBoss::ShouldDoubleCharge(CStateManager& mgr, const CTriggerData&) const {
  if (CountAttachedSiblings(mgr) != 2) { // fn_55_595C
    return false;
  }
  if (!IsAttachedLeader(mgr)) { // fn_55_5A84
    return mProbingDoubleCharge ? mSyncAttacksAllowed : false; // 0x165E b7 / b4
  }
  if (mChargeBeamTimer > 0.f || !mSyncAttacksAllowed || mChargeBeamCooldown >= 1) { // 0xF18, 0x165E b4
    return false;
  }
  const CPlayer* player = mgr.GetPlayer(0);
  if (player == nullptr) {
    return false;
  }
  const CVector3f toPlayer = player->GetTranslation() - GetTranslation(); // player + 0x54
  if (CVector3f::Dot(GetTransform().GetColumn(kDY), toPlayer) <= 0.f) {    // this + 0x28/0x38/0x48
    return false;
  }
  for (int i = 0; i < mSiblings.size(); ++i) {
    CSandBoss* sibling = CastSandBoss(mgr.ObjectById(mSiblings[i]));
    if (sibling != nullptr && sibling != this && sibling->mAttachedToSphere) { // 0x165D b7
      if (CVector3f::Dot(sibling->GetTransform().GetColumn(kDY), toPlayer) > 0.f) {
        return sibling->ProbeDoubleCharge(mgr); // fn_55_8D94
      }
      return false;
    }
  }
  return false;
}

// @ fn_55_C2A0
// Snap-jaw bite gate: no charge probe running, the player inside the patterned attack-range band
// (compared squared, CPatterned mMinAttackRange / mMaxAttackRange at 0x3DC / 0x3E0 [inferred: field
// identity]) and within 30 degrees of the forward axis.
bool CSandBoss::ShouldSnapJaws(CStateManager& mgr, const CTriggerData&) const {
  if (mProbingDoubleCharge) { // 0x165E b7
    return false;
  }
  const CPlayer* player = mgr.GetPlayer(0); // mgr 0x14FC
  if (player == nullptr) {
    return false;
  }
  const CVector3f delta = player->GetTranslation() - GetTranslation(); // player 0x54..0x5C
  const float distanceSq = delta.MagSquared();
  if (distanceSq < mMinAttackRange * mMinAttackRange ||
      distanceSq > mMaxAttackRange * mMaxAttackRange) {
    return false;
  }
  return CVector3f::GetAngleDiff(GetTransform().GetColumn(kDY), delta) < 0.5235987901687622f;
}

// @ fn_55_C258
bool CSandBoss::ShouldDestroySphere(CStateManager& mgr, const CTriggerData&) const {
  return mAttachedToSphere && CountAttachedSiblings(mgr) == 1;
}

// @ fn_55_C1CC
// IsFacingPlayer: the player is inside 20 degrees of the forward axis (the local Y column, this
// +0x28/0x38/0x48), measured in three dimensions against the player's translation (player
// +0x54..0x5C). The constant is lbl_55_rodata_134 = 0.34906584.
//
// Retail (fn_55_C1CC): 0xC1D8 loads the player (mgr + 0x14FC), 0xC1E0..0xC1FC subtracts the two
// translations (this +0x54/0x58/0x5C minus player +0x54/0x58/0x5C), 0xC210..0xC218 reads the
// facing column (this +0x48 / +0x38 / +0x28) and 0xC22C calls GetAngleDiff with it, 0xC234
// loads lbl_55_rodata_134 (0.34906584) and 0xC238..0xC244 turns the compare into "<=" (the
// `cror eq, lt, eq` at 0xC23C keeps the less-than-or-equal bit).
// The trigger sits in the FSM table (lbl_55_data_AC) but the shipped asset references it in no
// transition (§6 of the note), so the only way to run it is a direct call: the port's run does
// that (forced arm, see docs/module-analysis/SandBoss.md §7.4). The null-player guard has no
// retail counterpart - retail dereferences the player at 0xC1E4 without a check - and is dead:
// the game always has a player in this area.
bool CSandBoss::IsFacingPlayer(CStateManager& mgr, const CTriggerData&) const {
  const CPlayer* player = mgr.GetPlayer(0); // mgr 0x14FC
  if (player == nullptr) {
    return false;
  }
  const CVector3f delta = player->GetTranslation() - GetTranslation(); // player 0x54..0x5C
  return CVector3f::GetAngleDiff(GetTransform().GetColumn(kDY), delta) <= 0.34906584f;
}

// @ fn_55_C188
// The suck-air armour is on, the player is morphed, and the ball is within the mouth's range and
// facing cone (fn_55_5B38).
bool CSandBoss::IsBallInSuckRange(CStateManager& mgr, const CTriggerData&) const {
  const CPlayer* player = mgr.GetPlayer(0); // mgr 0x14FC
  if (!mSuckAir || player == nullptr ||
      player->GetMorphballTransitionState() != CPlayer::kMS_Morphed) { // player 0x38C == 1
    return false;
  }
  return IsBallInRange(*player); // fn_55_5B38
}

// @ fn_55_C174
bool CSandBoss::IsLastStage(CStateManager&, const CTriggerData&) const { return mRound == 2; }

// @ fn_55_C154
bool CSandBoss::HeadArmorDestroyed(CStateManager&, const CTriggerData&) const {
  return mHeadArmorHP <= 0.f;
}

// @ fn_55_C148
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

// @ fn_55_BFF8
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

// @ fn_55_BF64
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

// @ fn_55_BDAC
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

// @ fn_55_BD30
void CSandBoss::Deactivate(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Update && mStateMachine->GetTime() > 3.f) {
    mgr.DeleteObjectRequest(GetUniqueId());
  }
}

// @ fn_55_BAAC
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
    mChargeBeamCooldown = 0; // 0xF44
    m165D_b1 = true; // 0x165D b1
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6B4 = 0
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    mAttachedToSphere = true; // 0x165D b7
    mArmorDestroyed = false;  // 0x165D b6
  }
}

// @ fn_55_B98C
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

// @ fn_55_B718
// SuckAir: once mData.suckAirTime has passed the animation is over; until then anything other than
// Locomotion is aborted with an AbortScripted command, and an unmorphed player in front of the mouth
// (fn_55_5B38) is pulled toward it at 150 units a second, with collision impulses, the minimal
// acceleration timer and the HUD static interference. Start readies the animation, drops into
// locomotion 5 and marks this worm not recovered (0x165D b5).
void CSandBoss::SuckAir(CStateManager& mgr, EStateMsg msg, float dt) {
  CPlayer* player = mgr.GetPlayer(0); // mgr 0x14FC
  if (msg == kStateMsg_Update) {
    if (mData.suckAirTime <= mStateMachine->GetTime()) {      // 0x81C
      mAnimationState.SetState(CAnimationState::kAS_Over);     // 0x6B4 = 3
      return;
    }
    if (BodyController()->GetCurrentStateId() != pas::kAS_Locomotion) { // body 0x37C == 5
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_AbortScripted)); // 0xD
    }
    if (mSuckAir && player->GetMorphballTransitionState() != CPlayer::kMS_Morphed && // 0x165D b4
        IsBallInRange(*player)) {                                                   // fn_55_5B38
      const CVector3f mouth = GetLctrTransform(mArmorLocators[0]).GetTranslation(); // 0xF00
      const CVector3f toMouth = mouth - player->GetTranslation();
      if (toMouth.IsMagnitudeSafe()) {
        // Mass scaled, so the pull speed does not depend on the player's mass (retail's 0x158).
        const CVector3f impulse = toMouth.AsNormalized() * (dt * (player->GetMass() * 150.f));
        player->ApplyImpulseWR(impulse, CAxisAngle::Identity());
        player->UseCollisionImpulses();
        player->SetMinimalAccelerationTimer(dt * 2.f);
        // HUD static interference; magnitude and duration read as 0.1 from the decompiler's literals.
        if (CPlayerState* playerState = mgr.PlayerState(0)) {
          playerState->StaticInterference().AddSource(GetUniqueId(), 0.1f, 0.1f);
        }
      }
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);      // 0x6B4 = 1
    BodyController()->SetLocomotionType(pas::kLT_Internal5);   // 5
    mNotRecovered = true;                                     // 0x165D b5
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);   // 0x6B4 = 0
  }
}

// @ fn_55_B658
// SuckBall: each update pulls the morphed ball to the mouth (fn_55_62E4) and ends the state once
// it arrives. Start readies the animation, drops into locomotion 5 and takes the player's solid
// material away; stop drops the animation back to not-ready.
void CSandBoss::SuckBall(CStateManager& mgr, EStateMsg msg, float dt) {
  CPlayer* player = mgr.GetPlayer(0); // mgr 0x14FC
  if (msg == kStateMsg_Update) {
    if (SphereStateHelper62E4(dt, *player)) { // fn_55_62E4
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6B4 = 3
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6B4 = 1
    BodyController()->SetLocomotionType(pas::kLT_Internal5); // 5
    // Retail also clears bit 7 of the player's byte at 0x1144. CPlayer.hpp only names the two
    // morph-transition bits of that byte, so the store is left out until the bit is identified.
    player->RemoveMaterial(kMT_Solid, mgr); // 0x3B
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6B4 = 0
  }
}

// @ fn_55_B544
// BallInMouth: every update parks the ball on the mouth locator (fn_55_6250), runs the choke helper
// (fn_55_5E5C) and spits the ball out once mData.spitMorphballTime has passed. Start readies the
// animation, sends kSS_Entered for the player, drops into locomotion 6 and attaches the ball actor
// to the player; stop drops the animation back to not-ready and clears the suck-air armour.
void CSandBoss::BallInMouth(CStateManager& mgr, EStateMsg msg, float) {
  CPlayer* player = mgr.GetPlayer(0); // mgr 0x14FC
  if (msg == kStateMsg_Update) {
    SphereStateHelper6250(*player); // fn_55_6250
    Helper5E5C(mgr);                // fn_55_5E5C
    if (mData.spitMorphballTime <= mStateMachine->GetTime()) { // 0x824
      mAnimationState.SetState(CAnimationState::kAS_Over);      // 0x6B4 = 3
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6B4 = 1
    SendScriptMsgs(kSS_Entered, mgr, player->GetUniqueId(), kSM_None); // 'ENTR' on the player
    BodyController()->SetLocomotionType(pas::kLT_Internal6);           // 6
    player->AttachActorToPlayer(GetUniqueId(), false);
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6B4 = 0
    mSuckAir = false;                                        // 0x165D b4
  }
}

// @ fn_55_B2D8
// DestroySphere: each update re-parks the held ball on the mouth, keeps the generate command going
// (locomotion 0 once the body is in kAS_Generate) and pulses the choke flash for the first five
// seconds. Start readies the animation, frees the sphere (fn_55_51BC) and sends kSS_Retreat; stop
// tells every sibling to increment its round (this is what advances mRound in AcceptScriptMsg),
// drops the animation, crouches and clears the attached-to-sphere flag.
void CSandBoss::DestroySphere(CStateManager& mgr, EStateMsg msg, float) {
  CPlayer* player = mgr.GetPlayer(0); // mgr 0x14FC
  if (msg == kStateMsg_Update) {
    if (player->GetAttachedActorId() == GetUniqueId()) { // player 0x2E4
      SphereStateHelper6250(*player);                    // fn_55_6250
    }
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCGenerateCmd(pas::kGType_Five, CVector3f::Zero())); // 0xF, type 5
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) {
      BodyController()->SetLocomotionType(pas::kLT_Crouch); // 0
    }
    const float stateTime = mStateMachine->GetTime();
    if (stateTime < 5.f) {
      SphereStateHelper6434(stateTime); // fn_55_6434
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6B4 = 1
    DestroySphereHelper(mgr);                            // fn_55_51BC
    SendScriptMsgs(kSS_Retreat, mgr, GetUniqueId(), kSM_None);
  } else if (msg == kStateMsg_Deactivate) {
    for (int i = 0; i < mSiblings.size(); ++i) { // 0xDD8 / 0xDDC
      mgr.DeliverScriptMsg(
          CScriptMsg(GetUniqueId(), mSiblings[i], kSM_Increment, kInvalidUniqueId));
    }
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6B4 = 0
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    mAttachedToSphere = false; // 0x165D b7
  }
}

// @ fn_55_B06C
// JumpOffSphere: the same update shape as DestroySphere with generate type 1. Start readies the
// animation, marks the held ball (0x165D b3), breaks the player's orbit on this worm and drops the
// orbit material; stop drops the animation, crouches, clears the attached-to-sphere and held-ball
// flags and tells every sibling to increment its round.
void CSandBoss::JumpOffSphere(CStateManager& mgr, EStateMsg msg, float) {
  CPlayer* player = mgr.GetPlayer(0); // mgr 0x14FC
  if (msg == kStateMsg_Update) {
    if (player->GetAttachedActorId() == GetUniqueId()) { // player 0x2E4
      SphereStateHelper6250(*player);                    // fn_55_6250
    }
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCGenerateCmd(pas::kGType_One, CVector3f::Zero())); // 0xF, type 1
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_Generate) {
      BodyController()->SetLocomotionType(pas::kLT_Crouch); // 0
    }
    const float stateTime = mStateMachine->GetTime();
    if (stateTime < 5.f) {
      SphereStateHelper6434(stateTime); // fn_55_6434
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6B4 = 1
    mHoldingBall = true;                                  // 0x165D b3
    player->SetOrbitRequestForTarget(GetUniqueId(), CPlayer::kOR_ActivateOrbitSource, mgr); // 8
    RemoveMaterial(kMT_Orbit, mgr); // 0x29
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6B4 = 0
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
    mAttachedToSphere = false; // 0x165D b7
    mHoldingBall = false;      // 0x165D b3
    mgr.DeliverScriptMsg(
        CScriptMsg(GetUniqueId(), GetUniqueId(), kSM_Increment, kInvalidUniqueId));
  }
}

// @ fn_55_AED8
// SpitOutBall: while the player still holds on, each update re-parks the ball on the mouth
// (fn_55_6250) and runs the choke helper (fn_55_5E5C), then asks for the generate command (type 9).
// Start readies the animation and raises 0x165E b3 (which fn_55_5FEC reads to throw the held bombs
// with the ball); stop drops the animation and clears it.
void CSandBoss::SpitOutBall(CStateManager& mgr, EStateMsg msg, float) {
  CPlayer* player = mgr.GetPlayer(0); // mgr 0x14FC
  if (msg == kStateMsg_Update) {
    if (player->GetAttachedActorId() == GetUniqueId()) { // player 0x2E4
      SphereStateHelper6250(*player);                    // fn_55_6250
      Helper5E5C(mgr);                                   // fn_55_5E5C
    }
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Generate)) {
      // Retail stores generate type 9; pas::EGenerateType stops at kGType_Eight.
      BodyController()->CommandMgr().DeliverCmd(
          CBCGenerateCmd(static_cast< pas::EGenerateType >(9), CVector3f::Zero()));
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6B4 = 1
    m165E_b3 = true;                                     // 0x165E b3
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6B4 = 0
    m165E_b3 = false;                                       // 0x165E b3
  }
}

// @ fn_55_AC68
// DarkBeamAttack: start aims the head bone tracking at the safe-zone target (turn rate +pi/2),
// readies the animation and raises the probe result bit (0x165C b3) so this worm owns the shot; each
// update re-issues the projectile-attack command toward the target's position while the animation
// can take it, and ends the state once the target is gone; stop aborts a running projectile attack,
// turns tracking off (+pi turn rate), drops the flag, clears the target and resyncs the group's
// attack timers (fn_55_1C70 with the retail delta -1).
void CSandBoss::DarkBeamAttack(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_ProjectileAttack)) { // 0x12
      CEntity* target = mgr.ObjectById(mSafeZoneTarget); // 0xE88
      if (target == nullptr) {
        mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6B4 = 3
      } else if (const CActor* actor = TCastToPtr< CActor >(target)) { // [inferred: the actor cast]
        BodyController()->CommandMgr().DeliverCmd(
            CBCProjectileAttackCmd(pas::kS_Zero, actor->GetTranslation(), false)); // 6, 0x54
      }
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6B4 = 1
    mBoneTracking.SetActive(true);                        // 0xD9C
    mBoneTracking.SetAngSpeed(1.5707964f);                // 0xDBC = +pi/2
    mBoneTracking.SetTarget(mSafeZoneTarget);             // 0xE88
    mDarkBeamResult = true;                               // 0x165C b3
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6B4 = 0
    if (BodyController()->GetCurrentStateId() == pas::kAS_ProjectileAttack) { // 0x37C == 0x12
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_AbortScripted)); // 0xD
    }
    mBoneTracking.SetActive(false);      // 0xD9C
    mBoneTracking.SetAngSpeed(3.1415927f); // 0xDBC = +pi
    mDarkBeamResult = false;            // 0x165C b3
    mSafeZoneTarget = kInvalidUniqueId; // 0xE88
    SyncAttacker(mgr, -1);              // fn_55_1C70
  }
}

// @ fn_55_A9DC
// RepeaterAttack: while the body can take the loop attack the mouth keeps asking for it; between
// shots the worm faces the player -- but only while it is the unlocked leader and fewer than three
// dark beams have gone out (0xF48 counts up in fn_55_44CC) -- and past that it exits the loop. Start
// readies the animation, aims the head bone tracking at the player (+pi/2) and raises both the
// tracking and the busy bits; stop drops them, turns tracking off (+pi) and resyncs the group's
// attack timers (fn_55_1C70 with the retail delta -1).
void CSandBoss::RepeaterAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) { // 9
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_One, true)); // 7
    } else if (mF48 < 3) { // 0xF48
      const CPlayer* player = mgr.GetPlayer(0); // mgr 0x14FC
      if (player != nullptr && NoSiblingHasSphereLocked(mgr) && IsAttachedLeader(mgr)) { // fn_55_59E4/5A84
        FaceTowardPoint(dt, player->GetTranslation()); // player + 0x54, fn_55_1F84
      }
    } else {
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState)); // 0xA
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);  // 0x6B4 = 1
    mBoneTracking.SetActive(true);                        // 0xD9C
    mBoneTracking.SetAngSpeed(1.5707964f);                // 0xDBC = +pi/2
    mBoneTracking.SetTarget(mgr.GetPlayer(0)->GetUniqueId()); // player + 0x8
    m165C_b2 = true;                                     // 0x165C b2
    mDarkBeamResult = true;                              // 0x165C b3
    mF48 = 0;                                            // 0xF48
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6B4 = 0
    if (BodyController()->GetCurrentStateId() == pas::kAS_LoopAttack) { // 0x37C == 9
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState)); // 0xA
    }
    mBoneTracking.SetActive(false);        // 0xD9C
    mBoneTracking.SetAngSpeed(3.1415927f); // 0xDBC = +pi
    m165C_b2 = false;                      // 0x165C b2
    mDarkBeamResult = false;               // 0x165C b3
    SyncAttacker(mgr, -1);                 // fn_55_1C70
  }
}

// @ fn_55_A6CC
// Double charge state: start opens the mouth loop attack, flags the probe result, seeds the sweep and
// retarget timers and the turn speed; update keeps the loop attack commanded and, once the body is
// in it and this worm leads, either ends the volley (state time over duration, or fewer than two
// live beams) or sweeps the two beam ends; deactivate clears the flag, restores the turn speed and
// ends the volley.
void CSandBoss::DoubleCharge(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_Zero));
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_LoopAttack && IsAttachedLeader(mgr)) {
      if (!mChargeBeams.empty()) { // 0xE94
        // Face a point one unit off this worm, a turn step either side of its local Y axis
        // depending on the sweep direction.
        const float turn = dt * (mData.unknown_0x7619e561.doubleCharge.turnSpeed * 0.017453292f); // 0x88C
        const float angle =
            mChargeBeamSweepDir == 1 ? 1.5707964f - turn : turn + 1.5707964f; // 0xF3C
        const CVector3f offset = GetTransform().Rotate(
            CVector3f(CMath::FastCosR(angle), CMath::FastSinR(angle), 0.f));
        FaceTowardPoint(dt, GetTranslation() + offset); // fn_55_1F84
      }
      if (mStateMachine->GetTime() > mData.unknown_0x7619e561.doubleCharge.duration || // 0x86C
          (CountActiveChargeBeams(mgr) < 2 && !mChargeBeams.empty())) {                // fn_55_26C0
        EndChargeBeamVolley(mgr); // fn_55_31E8
      } else {
        UpdateDoubleChargeSweep(mgr, dt);                          // fn_55_37F4
        UpdateChargeBeamRetarget(mgr, dt, mData.unknown_0x7619e561.doubleCharge); // fn_55_3324
      }
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6B4 = 1
    mDoubleChargeResult = true;                          // 0x165C b1
    BodyController()->SetTurnSpeed(mData.unknown_0x7619e561.doubleCharge.turnSpeed); // 0x88C
    mChargeBeamSweepPhase = -1.5707964f; // 0xF38
    mChargeBeamSweepDir = 1;             // 0xF3C
    mChargeBeamRetargetTimer = 0.f;      // 0xF40
    UpdateChargeBeamRetarget(mgr, dt, mData.unknown_0x7619e561.doubleCharge); // fn_55_3324
  } else if (msg == kStateMsg_Deactivate) {
    mDoubleChargeResult = false;                  // 0x165C b1
    mChargeBeamCooldown =                         // 0xF44 <- 0x898
        mData.unknown_0x7619e561.doubleCharge.unknown_0x8d4f3b88;
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6B4 = 0
    EndChargeBeamVolley(mgr);                               // fn_55_31E8
    BodyController()->SetTurnSpeed(mPInfoTurnSpeed);        // 0xDE8
  }
}

// @ fn_55_A360
// Triple charge state: the same shape with the triple charge's durations, radii, turn speed and
// loader data; three beams is the full set, so the volley ends as soon as fewer than three live.
void CSandBoss::TripleCharge(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_Zero));
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_LoopAttack && IsAttachedLeader(mgr)) {
      if (!mChargeBeams.empty()) { // 0xE94
        // Face a point one unit off this worm, a turn step either side of its local Y axis
        // depending on the sweep direction.
        const float turn = dt * (mData.unknown_0x7619e561.tripleCharge.turnSpeed * 0.017453292f); // 0x8D4
        const float angle =
            mChargeBeamSweepDir == 1 ? 1.5707964f - turn : turn + 1.5707964f; // 0xF3C
        const CVector3f offset = GetTransform().Rotate(
            CVector3f(CMath::FastCosR(angle), CMath::FastSinR(angle), 0.f));
        FaceTowardPoint(dt, GetTranslation() + offset); // fn_55_1F84
      }
      if (mStateMachine->GetTime() > mData.unknown_0x7619e561.tripleCharge.duration || // 0x8B4
          (CountActiveChargeBeams(mgr) < 3 && !mChargeBeams.empty())) {                // fn_55_26C0
        EndChargeBeamVolley(mgr); // fn_55_31E8
      } else {
        UpdateTripleChargeSweep(mgr, dt);                          // fn_55_3434
        UpdateChargeBeamRetarget(mgr, dt, mData.unknown_0x7619e561.tripleCharge); // fn_55_3324
      }
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6B4 = 1
    mTripleChargeResult = true;                          // 0x165C b0
    mChargeBeamSweepPhase = -1.5707964f; // 0xF38
    mChargeBeamSweepDir = 1;             // 0xF3C
    mChargeBeamRetargetTimer = 0.f;      // 0xF40
    BodyController()->SetTurnSpeed(mData.unknown_0x7619e561.tripleCharge.turnSpeed); // 0x8D4
    UpdateChargeBeamRetarget(mgr, dt, mData.unknown_0x7619e561.tripleCharge); // fn_55_3324
  } else if (msg == kStateMsg_Deactivate) {
    mTripleChargeResult = false;                  // 0x165C b0
    mChargeBeamCooldown =                         // 0xF44 <- 0x8E0
        mData.unknown_0x7619e561.tripleCharge.unknown_0x8d4f3b88;
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6B4 = 0
    EndChargeBeamVolley(mgr);                               // fn_55_31E8
    BodyController()->SetTurnSpeed(mPInfoTurnSpeed);        // 0xDE8
  }
}

// @ fn_55_A170
// SnapJaws: the bite is one melee-attack command. Start readies the animation, arms the snap-jaw
// contact damage (0x165C b5, read by fn_55_79B0) and widens the collision actors' touch bounds
// (fn_55_78E0) so the closed jaws reach; stop drops the contact bit, puts the touch bounds back and,
// if the body is still mid-bite, moves it on to its next state.
void CSandBoss::SnapJaws(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_MeleeAttack)) { // 7
      BodyController()->CommandMgr().DeliverCmd(CBCMeleeAttackCmd(pas::kS_One));    // 5
    }
  } else if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6B4 = 1
    mSnapJawContact = true;                               // 0x165C b5
    // lbl_55_bss_8 is a lone 4.5f in the REL's bss [inferred: the other two floats read as 0].
    SnapJawsCollisionSetup(mgr, CVector3f(4.5f, 0.0f, 0.0f)); // fn_55_78E0
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6B4 = 0
    mSnapJawContact = false;                                 // 0x165C b5
    SnapJawsCollisionSetup(mgr, CVector3f::Zero());          // fn_55_78E0
    if (BodyController()->GetCurrentStateId() == pas::kAS_MeleeAttack) { // 0x37C == 7
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_NextState)); // 0xC
    }
  }
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
    // 0x34C |= 0x40 (rlwimi 25,25): bit 1 of the byte is mVerticalMovement (0x08 is mOnGround,
    // what fn_55_8C reads), so the stampede leap's vertical root motion is applied.
    mVerticalMovement = true; // 0x99A0
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
    mVerticalMovement = false; // 0x34C &= ~0x40 (0x9B2C)
    mF20 = 0.f;            // 0xF20
  }
}

// Code functions

// The armour setters swap the collision actors' damage vulnerability, the per-locator armour state
// (1 stampede set, 2 attached set) and the response types, then reset the hit colour.
// @ fn_55_986C
void CSandBoss::SetTripleChargeArmor(CStateManager& mgr, float) {
  for (int i = 0; i < 8; ++i) {
    mArmorState[i] = 2; // 0x1458..0x1474
  }
  SetCollisionVulnerabilities(mgr, mReflectVulnerability1, mReflectVulnerability1); // fn_55_783C
  UpdateArmorResponseTypes(mgr);                                                  // fn_55_6698
  mDamageColor = CPatterned::skHitsWithoutDamageColor;                             // 0x450
  SetVisorOrbitableFlags(CVisorParameters::kVOF_All, true);
}

// @ fn_55_97DC
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

// @ fn_55_96B0
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

// @ fn_55_9600
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

// @ fn_55_952C
// Each collision actor's own filter gains kMT_Player in its exclude list (0x95A8:
// `ori r0, r5, 0x1` on the high word at +8 is bit 32), so the worm's joints stop colliding with the
// player while the ball is being sucked in. The filter is the collider's, not the boss's: copying the boss's filter (which
// also excludes kMT_Player/kMT_Character) left every joint deaf to the player's shots after the
// first DestroySphere.
void CSandBoss::RemovePlayerCollision(CStateManager& mgr, float) {
  for (uint i = 0; i < mCollisionManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc = mCollisionManager->GetCollisionDescFromIndex(i); // 0xD98
    if (CCollisionActor* colAct =
            TCastToPtr< CCollisionActor >(mgr.ObjectById(desc.GetCollisionActorId()))) {
      CMaterialFilter filter = colAct->GetMaterialFilter();
      filter.ExcludeList().Add(kMT_Player);
      colAct->SetMaterialFilter(filter);
    }
  }
}

// @ fn_55_9438
// The inverse of fn_55_952C on each collider's own filter (0x9494..: `and -2` on the word at +8).
void CSandBoss::RestorePlayerCollision(CStateManager& mgr, float) {
  for (uint i = 0; i < mCollisionManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc = mCollisionManager->GetCollisionDescFromIndex(i);
    if (CCollisionActor* colAct =
            TCastToPtr< CCollisionActor >(mgr.ObjectById(desc.GetCollisionActorId()))) {
      CMaterialFilter filter = colAct->GetMaterialFilter();
      filter.ExcludeList().Remove(kMT_Player);
      colAct->SetMaterialFilter(filter);
    }
  }
  // Retail also sets bit 7 of the byte at 0x1144 of the player. CPlayer.hpp only names the two
  // morph-transition bits of that byte, so the store is left out until the bit is identified.
}

// @ fn_55_920C
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

// @ fn_55_8F94
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

// @ fn_55_8F88
void CSandBoss::ResetHeadArmorHP(CStateManager&, float) { mHeadArmorHP = mData.headArmorHP; }

// @ fn_55_8E60
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

// @ fn_55_8E3C
void CSandBoss::SetCinematicStateNormal(CStateManager&, float) { mCinematicState = 3; }

// @ fn_55_8E30
void CSandBoss::SetCinematicStateStunned(CStateManager&, float) { mCinematicState = 4; }

// @ fn_55_8E24
void CSandBoss::SetCinematicStateChoking(CStateManager&, float) { mCinematicState = 5; }

// @ fn_55_8E18
void CSandBoss::SetCinematicStateExitSphere(CStateManager&, float) { mCinematicState = 6; }

// Helpers

// @ fn_55_78E0
// Sets the touch-bounds expansion (the member retail writes at actor+0x35C, folded into
// CCollisionActor::GetTouchBounds) on every collision actor this worm owns: the snap jaws widen it
// while they are closed and put it back to zero when they open.
void CSandBoss::SnapJawsCollisionSetup(CStateManager& mgr, const CVector3f& bounds) {
  for (uint i = 0; i < mCollisionManager->GetNumCollisionActors(); ++i) { // 0xD98
    const CJointCollisionDescription& desc = mCollisionManager->GetCollisionDescFromIndex(i);
    const TUniqueId id = desc.GetCollisionActorId(); // desc + 0x3C
    if (CCollisionActor* colAct = TCastToPtr< CCollisionActor >(mgr.ObjectById(id))) {
      colAct->SetExtendedTouchBounds(bounds);
    }
  }
}

// @ fn_55_8C88
// A worm may be picked for the dark beam only while it has no dark-beam answer yet and its body is
// walking (retail's body state id 5 = pas::kAS_Locomotion); fn_55_42A8 asks this when it scores the
// attached siblings.
bool CSandBoss::CheckBodyStateForDarkBeam(CStateManager&) const {
  return !mDarkBeamResult && // 0x165C b3
         GetBodyController()->GetCurrentStateId() == pas::kAS_Locomotion; // 0x37C == 5
}
