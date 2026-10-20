// The Boost Guardian's Ing half: the base-class overrides the Ing REL carries its own copies of.
//
// This module (IngBoostBallGuardian, REL module 30) is the Ing REL with the boost half added, so
// it carries its own copy of the Ing's code. The behaviour below is ported from the Ing host body
// (branch decomp/ing, src/MetroidPrime/Enemies/PortCIng.cpp, lines 2791..3113).
//
// Every `fn_29_` address in the comments below is the ING REL's address, not this module's: the
// names and the code are the Ing's, and the module's own addresses are in
// docs/module-analysis/BoostGuardian_fsm.tsv (and docs/module-analysis/BoostGuardian_functions.tsv
// for the rest). The function-level addresses of the Ing bodies are given in the leading comment
// of each function, in the same wording as the Ing file.
//
// The parts the Ing has and this module does not are marked "[this module]": the lure safe zone,
// the safe-zone count, the grapple block (there is no CIngGrappleData in this module's data block)
// and the mini-portal attack class. The comments of the Ing body are kept as they are.
#include "MetroidPrime/Enemies/CIngBoostBallGuardian.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Particles/CGenDescription.hpp"
#include "Kyoto/Text/CStringTable.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CAxisAngle.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CGameLight.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "Kyoto/Graphics/CLight.hpp"
#include "Kyoto/Graphics/CModelFlags.hpp"
#include "MetroidPrime/Enemies/PortCBoostBallGuardianBall.hpp"
#include "MetroidPrime/Enemies/CCollisionTracker.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CKnockBackInfo.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/Weapons/CWeapon.hpp"
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Enemies/CGenericFSM2.hpp"
#include "MetroidPrime/Enemies/CIngMiniPortalAttack.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Enemies/CTeamAiRole.hpp"
#include "MetroidPrime/Enemies/PortBoostGuardianShared.hpp"
#include "MetroidPrime/HUD/CHUDMemoParms.hpp"
#include "MetroidPrime/HUD/CSamusHud.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/Player/CEnvironmentVariable.hpp"
#include "MetroidPrime/Player/CGameState.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrIng.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CHUDBillboardEffect.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIHint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include <stdio.h>
#include <stdlib.h>

namespace {

// The Ing's own file-scope solid filter helper (the Ing host body's helper) is shared with the
// other files of this module: PortBoostGuardianShared.hpp has SolidFilter().

} // namespace

// The Ing's PreThink (the Ing REL's fn_29_C814): the body state is installed first, then the base
// class's own PreThink runs.
void CIngBoostBallGuardian::PreThink(float dt, CStateManager& mgr) {
  EnsureBodyActive(mgr);
  CPatterned::PreThink(dt, mgr);
}

// The module's own Think. The form picks the base think (the host forms, the ing-spot forms'
// fn_30_55D8, or CPatterned's), then the managers, the line of sight, the timers, the surface
// alignment, the puddle effects, the target, the loop sound, the touch box, the hit light, the
// actor behind 0x1092 and the phase advance (0x0000DB18): the damage taken (1 - hp / initial hp)
// picks one of the three phases (the count at 0xF2C) in equal steps, and a new phase sets the
// speed (0x3C8) to its locomotion speed scale (+0x30).
// @ fn_30_D94C
void CIngBoostBallGuardian::Think(float dt, CStateManager& mgr) {
  // The probe's own activation: the Boost Guardian is an inactive actor until the fight's script
  // starts it (the console's `objs` lists it as inactive), so the probe wakes it up the way the
  // Sand Boss probe does. Without this the state machine is never set up and the probe's dump
  // never runs.
  if (getenv("MP2_BG_FSM") != nullptr && !GetActive()) {
    mgr.SendScriptMsg(this, GetUniqueId(), kSM_Activate);
  }
  if (!GetActive()) {
    return;
  }
  // [port] the host body is activated before the first think (see EnsureBodyActive).
  EnsureBodyActive(mgr);
  switch (mForm) {
  case kForm_Host:
  case kForm_ControllingHost:
    // 0x0000D9B0: the host forms run only the actor and the state machine.
    CActor::Think(dt, mgr);
    mStateMachine->Update(mgr, *this, dt);
    SetDrawShadow(false);
    return;
  case kForm_IngSpot:
  case kForm_Evaporate:
    CActor::Think(dt, mgr); // 0x0000D9F0
    UpdateIngSpotBody(dt, mgr);
    SetDrawShadow(false);
    UpdateHitDamageTime(dt); // vtable +0xEC
    break;
  default:
    CPatterned::Think(dt, mgr); // 0x0000DA38
    SetDrawShadow(mAlive);
    break;
  }
  // 0x0000DA58: both managers, guest 0xD60 and 0xD64.
  mCollisionActorManager0->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  mCollisionActorManager1->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  mLineOfSight.Update(dt, mgr);
  UpdateTimers(dt, mgr);
  mSurfaceAlignment.Update(*this, mgr, dt);
  UpdatePuddleEffects(dt, mgr);
  UpdateValidTarget(mgr);
  UpdateLoopSfx();
  UpdateTouchBox();
  UpdateHitLight(dt, mgr);
  if (mgr.GetObjectById(mD76Id) == nullptr) { // 0x0000DB00: no kInvalidUniqueId guard
    mD76Id = kInvalidUniqueId;
  }
  const CHealthInfo* health = GetHealthInfo();
  const float damage = 1.f - health->GetHP() / health->GetInitialHP();
  const int count = 3;
  const float step = 1.f / static_cast< float >(count);
  for (int i = 0; i < count; ++i) {
    const float lo = step * static_cast< float >(i);
    if (damage >= lo && damage <= lo + step) {
      if (mPhaseIndex != i) {
        mPhaseIndex = i;
        mSpeed = CurrentPhase().mLocomotionSpeedScale;
        if (DebugBg()) {
          fprintf(stderr, "[ibbg] %04x phase %d (damage %.2f) speed %.2f\n",
                  GetUniqueId().Value(), i, damage, mSpeed);
        }
      }
      break;
    }
  }
  // Once-a-second position poll: the off-navmesh Ing repeats Lurk -> SetRecoverDest ->
  // FailSafeMode, and only a position trace says whether the fail-safe teleport moves it.
  if (DebugBg() && (mDebugPoll++ % 60) == 0) {
    const CVector3f pos = GetTranslation();
    fprintf(stderr, "[ibbg] %04x pos (%.1f, %.1f, %.1f) form %d state %s\n", GetUniqueId().Value(),
            pos.GetX(), pos.GetY(), pos.GetZ(), mForm, mLastLoggedState ? mLastLoggedState : "?");
  }
}

// @ fn_30_55D8: the ing-spot forms' think. 0x11BC counts the time in the locomotion body state;
// the body and the animation run only for its first 2.5 s (rodata_E0).
void CIngBoostBallGuardian::UpdateIngSpotBody(float dt, CStateManager& mgr) {
  if (mBodyController->GetCurrentStateId() != pas::kAS_Locomotion) {
    mBodyState5Time = 0.f;
  } else {
    mBodyState5Time += dt;
  }
  if (mBodyState5Time < 2.5f) {
    mBodyController->Update(dt, mgr);
    UpdateAnimation(dt, mgr, true);
  }
  if (!mStateMachine->HasState()) {
    InitializeStateMachine(mgr);
  } else {
    mStateMachine->Update(mgr, *this, dt);
  }
  mWaypointNavigation.Update(dt);
}

// @ fn_30_56E0: an axis-aligned box on the translation, 2.75 across (rodata_E4) and 1.0 up and
// down (rodata_64).
void CIngBoostBallGuardian::UpdateTouchBox() {
  const CVector3f pos = GetTranslation();
  const CVector3f max(pos.GetX() + 2.75f, pos.GetY() + 2.75f, pos.GetZ() + 1.f);
  const CVector3f min(pos.GetX() - 2.75f, pos.GetY() - 2.75f, pos.GetZ() - 1.f);
  mTouchBox = CAABox(min, max);
}

static bool BgTrackerEmitting(CCollisionTracker* tracker) {
  return tracker != nullptr && tracker->GetParticleSystem() != nullptr &&
         tracker->GetParticleSystem()->GetParticleCount() > 0;
}

// @ fn_30_6108
// The two blob trackers (ing 0x1074, boost 0x1076): one is driven on the surface under the guardian
// and the other is turned off. The driven one is the ing tracker in the boost puddle form while it
// still emits, else the ing tracker unless the boost tracker still emits.
void CIngBoostBallGuardian::UpdatePuddleEffects(float, CStateManager& mgr) {
  CCollisionTracker* ing = static_cast< CCollisionTracker* >(mgr.ObjectById(mIngBlobTrackerId));
  const bool ingEmitting = BgTrackerEmitting(ing);
  CCollisionTracker* boost =
      static_cast< CCollisionTracker* >(mgr.ObjectById(mBoostBlobTrackerId));
  const bool boostEmitting = BgTrackerEmitting(boost);
  const bool useIng = mCurrentlyInBoostPuddleForm ? ingEmitting : !boostEmitting; // 0x11EF 0x80
  CCollisionTracker* driven = useIng ? ing : boost;
  CCollisionTracker* other = useIng ? boost : ing;
  if (driven != nullptr) {
    float rate = 0.f;
    switch (mForm) {
    case kForm_IngSpot: // 0x00006274
      rate = 1.f;
      if (CBoostBallGuardianBall* ball =
              TCastToPtr< CBoostBallGuardianBall >(mgr.ObjectById(mBoostChildId))) {
        switch (ball->GetState()) { // ball 0x2D4
        case 0:
          rate *= ball->GetShrink(); // ball 0x530
          break;
        case 1:
          rate = 0.f;
          break;
        case 2:
          rate = 1.f;
          break;
        default:
          break;
        }
      }
      break;
    case kForm_BecomingCorporeal:
    case kForm_BecomingIngSpot:
    case kForm_BecomingBodyProjectile:
    case kForm_BecomingWallProjectile: // 0x000062EC
      if (mIngSpotBlobVisible) {           // 0x11ED 0x40
        rate = 1.f;
      }
      break;
    case kForm_Corporeal: // 0x00006304
      if (!GetAlive() && mIngSpotBlobVisible) {
        rate = 1.f;
      }
      break;
    default:
      break;
    }
    if (rate > 0.f) {
      const CVector3f normal = mSurfaceAlignment.GetSurface().GetNormal(); // 0xD68
      const CVector3f pos = GetTranslation();
      const CVector3f& up = normal.GetZ() < 0.95f ? CVector3f::Up() : CVector3f::Right();
      driven->SetTransform(CTransform4f::LookAt(pos, pos + normal, up));
      driven->SetModelFlags(CModelFlags(CModelFlags::kT_Two, 0, CModelFlags::EFlags(3),
                                        CColor(mColor.GetRedu8(), mColor.GetGreenu8(),
                                               mColor.GetBlueu8(), 0xff)));
    }
    driven->SetParticleEmissionRateScalar(rate);
  }
  if (other != nullptr) {
    other->SetParticleEmissionRateScalar(0.f);
  }
}

// @ fn_30_65C8
// The "Ing Light" (0x1084) on the head locator (0x10AD): it fades in (0x11C4) while the guardian is
// alive and corporeal and out otherwise.
void CIngBoostBallGuardian::UpdateHitLight(float dt, CStateManager& mgr) {
  if (GetAlive() && mForm == kForm_Corporeal) { // 0x420 0x40, 0xAE4
    mLightBlend = CMath::Min(1.f, mLightBlend + dt);
  } else {
    mLightBlend = CMath::Max(0.f, mLightBlend - dt);
  }
  if (CGameLight* light = TCastToPtr< CGameLight >(mgr.ObjectById(mHitLightId))) {
    const CColor color = CColor::Lerp(CColor::Black(), mData.mLightColor, mLightBlend); // 0x8F8
    CLight lightData = CLight::BuildPoint(GetLctrTransform(mEyeSegId).GetTranslation(), color);
    lightData.SetAttenuation(1.f / mData.mLightAttenuation, 0.f, 0.f); // 0x8FC
    light->SetLight(lightData);
  }
}


// fn_30_DDA0 (this module).
// The Ing's AcceptScriptMsg (the Ing REL's fn_29_C9A4): the message switch that creates the body
// and the collision actors, joins the team, relays the hits and cleans up on delete.
void CIngBoostBallGuardian::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  // fn_30_DDA0 (0x0000DDC4..0x0000DE08): the active bit (CEntity 0x20 bit 7) before the base
  // handles the message, as a float compared with 0 (f31 vs lbl_30_rodata_70). ACTV acts only on
  // an inactive entity, DCTV only on an active one.
  const bool wasActive = GetActive();
  CPatterned::AcceptScriptMsg(mgr, msg);
  const TUniqueId sender = msg.GetSenderId();
  if (DebugBg()) {
    // Which damage messages reach the Ing, and from whom: the collider relay is the only route a
    // projectile has to a corporeal Ing (pool this with MP2_DEBUG_BG=1).
    switch (msg.GetMessage()) {
    case kSM_Damage:          // XDMG in retail's naming
    case kSM_HitObject:
    case kSM_ResistedDamage:  // XRDG
      fprintf(stderr, "[ibbg] %04x msg %d from %04x\n", GetUniqueId().Value(),
              static_cast< int >(msg.GetMessage()), sender.Value());
      break;
    default:
      break;
    }
  }
  switch (msg.GetMessage()) {
  case kSM_Create:
    // fn_30_68F0 tests the data byte's kFlag_StartAsIngSpot (0x0000690C/0x00006914 for the form,
    // 0x00006924/0x00006928 for the locomotion), not the guest bit 0x11ED 0x40.
    mForm = (mData.mFlags210 & kFlag_StartAsIngSpot) != 0 ? kForm_IngSpot : kForm_Corporeal;
    if ((mData.mFlags210 & kFlag_StartAsIngSpot) != 0) {
      BodyController()->SetLocomotionType(pas::kLT_Crouch); // 0x00006934: li r4, 0
      mSurfaceAlignment.AlignNearPosition(*this, mgr, GetTranslation(), 1.f);
    } else {
      BodyController()->SetLocomotionType(pas::kLT_Relaxed); // 0x00006960: li r4, 1
    }
    // 0x0000DF0C: the body is activated after the form picks its locomotion (kAS_Invalid). The
    // [port] guard skips it when a think already activated the body (see EnsureBodyActive).
    EnsureBodyActive(mgr);
    CreateCollisionActorManager(mgr); // 0x0000DF18: fn_30_4E90
    CreateIngSpotManager(mgr);        // 0x0000DF24: fn_30_4A38
    mIngBlobTrackerId = CreateBlobTracker( // 0x0000DF90: fn_30_648C
        mgr, TLockedToken< CGenDescription >(
                 gpSimplePool->GetObj(SObjectTag('PART', mData.mIngSpot.GetBlobEffect()))));
    mBoostBlobTrackerId = CreateBlobTracker( // 0x0000E01C: fn_30_648C
        mgr, TLockedToken< CGenDescription >(
                 gpSimplePool->GetObj(SObjectTag('PART', mBoost.mBlobEffect))));
    CreateHitLight(mgr); // 0x0000E048: fn_30_67B4
    // 0x0000E06C: the (kSS_Modify, kSM_Activate) connections, which the boost ball gets.
    // 0x0000E074/0x0000E080: the vector's assignment and the temporary's destructor
    mModifyTargets = FindConnectedObjects(mgr, kSS_Modify, kSM_Activate); // fn_30_E430, fn_30_E4EC
    break;
  case kSM_Delete:
    mgr.DeleteObjectRequest(mIngBlobTrackerId);   // 0x0000E098
    mgr.DeleteObjectRequest(mBoostBlobTrackerId); // 0x0000E0AC
    StopSfx();                                    // 0x0000E0B4: fn_30_3A60
    mCollisionActorManager0->Destroy(mgr); // 0x0000E0C0
    mCollisionActorManager1->Destroy(mgr); // 0x0000E0CC
    mgr.DeleteObjectRequest(mHitLightId);  // 0x0000E0E0
    ReleaseOwnCoverHint(mgr);              // 0x0000E114: fn_30_6064
    LeaveTeam(mgr);                        // 0x0000E120: fn_30_510C
    break;
  case kSM_AreaLoaded:
    mPathArea =
        mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea;
    mPathFindSearch.SetArea(mPathArea);
    mPointPathFindSearch = CPathFindPointSearch(mPathArea);
    JoinTeam(mgr);
    break;
  case kSM_Alert:
    // 0x0000E2F4: ALRT sets CPatterned 0x420 bit 7; mAlert (0x11ED 0x08) is Lurk's (0x0000A12C).
    SetHitByPlayerProjectile(true);
    break;
  case kSM_Activate:
    JoinTeam(mgr);
    // 0x0000E134: only when it was inactive; each manager is on for its own form.
    if (!wasActive) {
      mCollisionActorManager0->SetActive(mgr, mForm == kForm_Corporeal); // 0x0000E150
      mCollisionActorManager1->SetActive(mgr, mForm == kForm_IngSpot); // 0x0000E16C
      if (CEntity* light = mgr.ObjectById(mHitLightId)) {
        light->SetActive(true); // 0x0000E19C
      }
      mgr.SetBossParams(GetUniqueId(), GetHealthInfo()->GetInitialHP(), // 0x0000E1F0
                        gpStringTable->GetStringIndex("BossBoostBallGuardian"));
    }
    break;
  case kSM_Deactivate:
    // 0x0000E204: only when it was active.
    if (wasActive) {
      mCollisionActorManager0->SetActive(mgr, false); // 0x0000E214
      mCollisionActorManager1->SetActive(mgr, false); // 0x0000E224
      LeaveTeam(mgr);                                 // 0x0000E230: fn_30_510C
      if (CEntity* light = mgr.ObjectById(mHitLightId)) {
        light->SetActive(false); // 0x0000E260
      }
      if (CEntity* tracker = mgr.ObjectById(mIngBlobTrackerId)) {
        static_cast< CCollisionTracker* >(tracker)->SetParticleEmissionRateScalar(0.f); // 0x0000E288: fn_25_256C
      }
      if (CEntity* tracker = mgr.ObjectById(mBoostBlobTrackerId)) {
        static_cast< CCollisionTracker* >(tracker)->SetParticleEmissionRateScalar(0.f); // 0x0000E2B0: fn_25_256C
      }
      StopSfx(); // 0x0000E2B8: fn_30_3A60
    }
    break;
  case kSM_AIUpdateDisabled:
    if (mCollisionActorManager0 != nullptr) {
      mCollisionActorManager0->SetPhysicsActive(mgr, false);
    }
    if (mCollisionActorManager1 != nullptr) {
      mCollisionActorManager1->SetPhysicsActive(mgr, false); // 0x0000E2EC
    }
    break;
  case kSM_HitObject:
    CollisionActorHitPlayer(mgr, sender); // 0x0000E3AC: fn_30_47E8
    break;
  case kSM_Damage:
    ColliderDamage(mgr, sender);   // 0x0000E354: fn_30_479C
    mHitByPlayerProjectile = true; // 0x0000E364: 0x420 bit 7
    break;
  case kSM_ResistedDamage:
    if (mForm == kForm_IngSpot) {
      PuddleColliderResistedDamage(mgr, sender); // 0x0000E390: fn_30_4324
    }
    break;
  case kSM_XENZ:
    ++mSafeZoneCount; // 0x0000E3C4: 0x115C
    mHurtfulZone = mgr.GetSafeZoneManager()->IsObjectInHurtfulSafeZone(*this, mgr); // 0x11EE 0x40
    break;
  case kSM_XEXZ:
    --mSafeZoneCount; // 0x0000E3F0
    mHurtfulZone = mgr.GetSafeZoneManager()->IsObjectInHurtfulSafeZone(*this, mgr);
    break;
  default:
    break;
  }
  (void)sender;
}

// The Ing's PreRender (the Ing REL's fn_29_C010).
void CIngBoostBallGuardian::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);
}

// The Ing's Render (the Ing REL's fn_29_C0B4): the ing-spot and evaporating forms are drawn only
// for the first two and a half seconds of the body state.
void CIngBoostBallGuardian::Render(const CStateManager& mgr) const {
  if (!mRenderModel) {
    return;
  }
  if (mForm == kForm_IngSpot || mForm == kForm_Evaporate) {
    if (mBodyState5Time < 2.5f) {
      CPatterned::Render(mgr);
    }
    return;
  }
  CPatterned::Render(mgr);
}

// The Ing's GetDamageVulnerability (the Ing REL's fn_29_BF20).
const CDamageVulnerability* CIngBoostBallGuardian::GetDamageVulnerability() const {
  if (mForm == kForm_BodyProjectile) {
    return CPatterned::GetDamageVulnerability();
  }
  if (mForm == kForm_IngSpot) {
    // The Ing returns the grapple block's vulnerability while it holds the player; this module has
    // no grapple block (its data block has no grapple properties), so the ing-spot vulnerability
    // stands in for the grappled case too.
    return &mData.mIngSpotVulnerability;
  }
  // The Ing's fallback is its trigger vulnerability (mData.mTriggerVulnerability). This module's
  // data block has no trigger vulnerability - SLdrIngBoostBallGuardianData has no such property -
  // so the ing-spot vulnerability stands in.
  return &mData.mIngSpotVulnerability;
}

// The Ing's GetTouchBounds (the Ing REL's fn_29_B970): the ing-spot and evaporating forms use the
// puddle box, the two host forms have none, the rest use the patterned bounds.
rstl::optional_object< CAABox > CIngBoostBallGuardian::GetTouchBounds() const {
  if (mForm == kForm_IngSpot || mForm == kForm_Evaporate) {
    return mTouchBox;
  }
  if (mForm == 0 || mForm == 1) {
    return rstl::optional_object< CAABox >();
  }
  return CPatterned::GetTouchBounds();
}

// The Ing's Touch (the Ing REL's fn_29_BB8C): the ing-spot form's contact damage.
void CIngBoostBallGuardian::Touch(CActor& actor, CStateManager& mgr) {
  EnsureBodyActive(mgr);
  if (mForm == kForm_IngSpot && !mGrappled && actor.GetUniqueId() == mTargetId &&
      mCurDamageRemTime <= 0.f) {
    // [?] the damage: the patterned contact damage.
    mgr.ApplyDamage(GetUniqueId(), mTargetId, GetUniqueId(), GetContactDamage(), SolidFilter(),
                    CVector3f::Zero());
    mCurDamageRemTime = mDamageWaitTime;
  }
  CPatterned::Touch(actor, mgr);
}

// @ fn_30_CA18
// The hit tint: the damage colour while the cooldown runs, else the hits-without-damage colour
// while a resisted bomb hit (0x1080) or a safe zone holds it. Retail sets no damage highlight and
// drives no echo emitter here, unlike CPatterned's.
void CIngBoostBallGuardian::UpdateHitDamageTime(float dt) {
  mDamageCooldownTimer = 0.f >= mDamageCooldownTimer - dt ? 0.f : mDamageCooldownTimer - dt;
  mBoostHitTimer = 0.f >= mBoostHitTimer - dt ? 0.f : mBoostHitTimer - dt;
  if (mSafeZoneCount > 0) {
    mBoostHitTimer = CPatterned::skDamageHitTime; // 0x0000CA90
  }
  if (mDamageCooldownTimer > 0.f) {
    const float t = mDamageCooldownTimer / CPatterned::skDamageHitTime;
    const CColor color = CColor::Lerp(CColor::Black(), CPatterned::skDamageColor, t);
    mColor.Set(color.GetRedu8(), color.GetGreenu8(), color.GetBlueu8(), mColor.GetAlphau8());
  } else if (mBoostHitTimer > 0.f) {
    const float t = mBoostHitTimer / CPatterned::skDamageHitTime;
    const CColor color = CColor::Lerp(CColor::Black(), CPatterned::skHitsWithoutDamageColor, t);
    mColor.Set(color.GetRedu8(), color.GetGreenu8(), color.GetBlueu8(), mColor.GetAlphau8());
  }
}

// fn_30_D4F8 (this module).
// The Ing's DoUserAnimEvent (the Ing REL's fn_29_C228). The base class's handler runs only for the
// events this function does not handle (retail's local `handled` flag, set by every case below).
// The area 57 run delivered kUE_LoopedSoundStop (2), kUE_Activate (29), kUE_EffectOff (34) and
// kUE_Unknown37 (37).
void CIngBoostBallGuardian::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                             EUserEventType type, float dt) {
  EnsureBodyActive(mgr); // host guard, not in retail
  bool handled = false;
  switch (type) {
  case kUE_Projectile: // 0
    // [this module] fn_30_D4F8 @0xD65C: one mini portal per event, as long as the effect is loaded
    // (0x1050, engaged flag 0x105C) and FindMiniPortals queued a point still unused (0x1158 <
    // 0x1130, points at 0x1134 + 12 * index). The damage is the mini-portal block's (0xA8C) scaled
    // by dt and made immunity-proof (bit 7 of damage+0x1a); sound 0x89C, beam info 0x8B0, the
    // target 0x1088. The spawn's name is rodata_12C + 0x608, "Ing Mini Portal Attack". Handled
    // all the same, so the base class does not fire a generic projectile.
    if (mMiniPortalEffect && mMiniPortalIndex >= 0 && mMiniPortalIndex < mMiniPortalCount) {
      CTransform4f xf = GetTransform();
      xf.SetTranslation(mMiniPortalPositions[mMiniPortalIndex++]);
      CDamageInfo damage(mData.mMiniPortal.mDamage);
      damage.MultiplyDamage(dt);
      damage.SetNoImmunity(true);
      // rodata_64 1.0, rodata_78 2.0, rodata_74 150.0, then 1.0 again.
      const CIngMiniPortalInfo portalInfo(mTargetId, 1.f, 2.f, *mMiniPortalEffect,
                                          mData.mMiniPortal.mSound, 150.f, 1.f, damage,
                                          mData.mMiniPortal.mBeamInfo);
      CIngMiniPortalAttack* portal = rs_new CIngMiniPortalAttack(
          mgr.AllocateUniqueId(), rstl::string_l("Ing Mini Portal Attack"),
          CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true), xf, GetUniqueId(),
          GetModelData()->GetScale(), portalInfo);
      mgr.AddObject(portal);
    }
    handled = true;
    break;
  case kUE_TakeOff: // [this module] fn_30_D4F8 @0xD578
    mTakeOff = true; // 0x11EC 0x02
    handled = true;
    break;
  case kUE_BeginAction: // [this module] fn_30_D4F8 @0xD590: the form only; the Ing's reset of
                        // the body-projectile time is not in this module
    if (mAlive) {                   // CPatterned 0x420 bit 6
      mForm = kForm_BodyProjectile; // 0xAE4 = 4
    }
    handled = true;
    break;
  case kUE_Activate: // [this module] fn_30_D4F8 @0xD5AC
    mIngSpotBlobVisible = true; // 0x11ED 0x40
    handled = true;
    break;
  case kUE_Deactivate: // [this module] fn_30_D4F8 @0xD5C4
    mIngSpotBlobVisible = false; // 0x11ED 0x40
    handled = true;
    break;
  case kUE_EffectOn: // 0x21
    if (AnimationData() != nullptr) {
      AnimationData()->SetEffectState(rstl::string_l("eyes"), true, mgr); // rodata_40 -> rodata_68E
    }
    handled = true;
    break;
  case kUE_EffectOff: // 0x22
    if (AnimationData() != nullptr) {
      AnimationData()->SetEffectState(rstl::string_l("eyes"), false, mgr);
    }
    handled = true;
    break;
  case kUE_Unknown37: // [this module] fn_30_D4F8 @0xD548: body hits stop being lethal
    mBodyHitsLethal = false; // 0x11EF 0x10 = 0
    handled = true;
    break;
  case kUE_Unknown38: // [this module] fn_30_D4F8 @0xD560
    mBodyHitsLethal = true; // 0x11EF 0x10 = 1
    handled = true;
    break;
  default:
    break;
  }
  if (!handled) {
    CPatterned::DoUserAnimEvent(mgr, node, type, dt);
  }
}

// The Ing's Death (the Ing REL's fn_29_B3C4).
void CIngBoostBallGuardian::Death(CStateManager& mgr, const CVector3f& direction,
                                  EScriptObjectState state) {
  EnsureBodyActive(mgr);
  StopSfx();
  CPatterned::Death(mgr, direction, state);
}

// The Ing's KnockBack (the Ing REL's fn_29_B8C0).
void CIngBoostBallGuardian::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  EnsureBodyActive(mgr);
  CPatterned::KnockBack(mgr, info);
}

// The Ing's TakeDamage (the Ing REL's fn_29_B890): the two host forms and the evaporating form
// take no damage.
void CIngBoostBallGuardian::TakeDamage(const CVector3f& direction, float magnitude) {
  if (mForm == 0 || mForm == 1 || mForm == kForm_Evaporate) {
    return;
  }
  CPatterned::TakeDamage(direction, magnitude);
}

// The Ing's Listen (the Ing REL's fn_29_B294): a path obstruction inside 40 units asks for a path
// recalculation, every other noise inside the hearing radius counts as a heard shot.
bool CIngBoostBallGuardian::Listen(CStateManager&, const CVector3f& position,
                                   EListenNoiseType type) {
  if (!GetAlive() || !GetActive()) {
    return false;
  }
  const float distSq = (position - GetTranslation()).MagSquared();
  if (type == kLNT_PathObstruction) {
    if (distSq < 1600.f) {
      mRecalcPath = true;
      return true;
    }
  } else if (distSq < mData.mHearingRadius * mData.mHearingRadius) {
    // [?] every other noise inside the hearing radius counts as a heard shot.
    mHeardShotTime = 0.f;
  }
  return false;
}
