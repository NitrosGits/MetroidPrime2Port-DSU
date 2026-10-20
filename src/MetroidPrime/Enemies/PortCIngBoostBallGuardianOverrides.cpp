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
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CKnockBackInfo.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/Weapons/CWeapon.hpp"
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Enemies/CGenericFSM2.hpp"
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

// The Ing's Think (the Ing REL's fn_29_C654): the base class's machine, then the surface
// alignment, the collision actors, the line-of-sight tracker and the actor behind the attack
// target.
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
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
  if (!mCollisionActorManager.null()) {
    mCollisionActorManager->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  }
  mLineOfSight.Update(dt, mgr);
  UpdateTimers(dt, mgr);
  mSurfaceAlignment.Update(*this, mgr, dt);
  UpdateValidTarget(mgr);
  UpdateLoopSfx();
  // [?] the touch box's shape: MinorIng's puddle box (fn_44_3E8C).
  const float half = mModelScale.GetX() * 0.5625f;
  mTouchBox = CAABox(CVector3f(-half, -half, 0.f), CVector3f(half, half, mModelScale.GetX()))
                  .GetTransformedAABox(GetTransform());
  // [this module] the Ing's lure safe zone check is dropped: this module has no lure safe zone
  // (no mLureSafeZoneId), so there is nothing to invalidate here.
  if (mD76Id != kInvalidUniqueId && mgr.GetObjectById(mD76Id) == nullptr) {
    mD76Id = kInvalidUniqueId;
  }
  // Once-a-second position poll: the off-navmesh Ing repeats Lurk -> SetRecoverDest ->
  // FailSafeMode, and only a position trace says whether the fail-safe teleport moves it.
  if (DebugBg() && (mDebugPoll++ % 60) == 0) {
    const CVector3f pos = GetTranslation();
    fprintf(stderr, "[ibbg] %04x pos (%.1f, %.1f, %.1f) form %d state %s\n", GetUniqueId().Value(),
            pos.GetX(), pos.GetY(), pos.GetZ(), mForm, mLastLoggedState ? mLastLoggedState : "?");
  }
}

// The Ing's AcceptScriptMsg (the Ing REL's fn_29_C9A4): the message switch that creates the body
// and the collision actors, joins the team, relays the hits and cleans up on delete.
void CIngBoostBallGuardian::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
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
    // fn_29_4C88
    EnsureBodyActive(mgr);
    mForm = mStartAsIngSpot ? kForm_IngSpot : kForm_Corporeal;
    if (mForm == kForm_IngSpot) {
      BodyController()->SetLocomotionType(pas::kLT_Relaxed);
      mSurfaceAlignment.AlignNearPosition(*this, mgr, GetTranslation(), 1.f);
    } else {
      BodyController()->SetLocomotionType(pas::kLT_Combat);
    }
    CreateCollisionActorManager(mgr); // fn_29_3190
    break;
  case kSM_Delete:
    // Retail deletes the blob object first, then destroys the collision actor manager.
    if (!mCollisionActorManager.null()) {
      mCollisionActorManager->Destroy(mgr); // fn_29_C9A4's XDEL branch
    }
    ReleaseCoverHint(mgr, mCoverHintId);
    if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamId))) {
      team->QuitTeam(GetUniqueId());
    }
    StopSfx();
    break;
  case kSM_AreaLoaded:
    mPathArea =
        mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea;
    mPathFindSearch.SetArea(mPathArea);
    mPointPathFindSearch = CPathFindPointSearch(mPathArea);
    JoinTeam(mgr);
    break;
  case kSM_Alert:
    mAlert = true;
    break;
  case kSM_Activate:
    JoinTeam(mgr);
    // Retail handles ACTV only while the entity is active (CEntity 0x20 bit 7, the first bitfield =
    // mActive) and only turns the collision actors on for the corporeal form.
    if (GetActive() && !mCollisionActorManager.null()) {
      mCollisionActorManager->SetActive(mgr, mForm == kForm_Corporeal);
    }
    break;
  case kSM_Deactivate:
    if (!mCollisionActorManager.null()) {
      mCollisionActorManager->SetActive(mgr, false);
    }
    StopSfx();
    break;
  case kSM_AIUpdateDisabled:
    if (!mCollisionActorManager.null()) {
      mCollisionActorManager->SetPhysicsActive(mgr, false);
    }
    break;
  case kSM_HitObject:
    CollisionActorHitPlayer(mgr, sender); // fn_29_2ED8
    break;
  case kSM_Damage:
    // fn_29_2E8C: the ing-spot form reacts to the hit itself, every
    // other form relays it.
    if (mForm == kForm_IngSpot) {
      IngSpotDamage(mgr, sender); // fn_29_2B70
    } else {
      CollisionActorDamage(mgr, sender); // fn_29_2684
    }
    break;
  case kSM_ResistedDamage:
    // fn_29_C9A4's XRDG branch: while grappled in the ing-spot form the hit timer ticks down and
    // the hit effect plays; either way the actor is flagged as hit.
    if (mForm == kForm_IngSpot && mGrappled) {
      mHitReactionTime -= 0.5f;
      if (CEntity* fx = SpawnHitEffect(mgr, static_cast< bool >(mHeavyHitEffect))) { // fn_29_2414
        mgr.AddObject(fx);
      }
    }
    mHitByPlayerProjectile = true; // 0x420 bit 7
    break;
  case kSM_XENZ:
    // [this module] the Ing's ++mSafeZoneCount is dropped: this module has no safe-zone count (it
    // has no lure safe zone), so only the harmful-zone flag is kept.
    mHurtfulZone = mgr.GetSafeZoneManager()->IsObjectInHurtfulSafeZone(*this, mgr);
    break;
  case kSM_XEXZ:
    // [this module] the Ing's --mSafeZoneCount is dropped, for the same reason.
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
    // fn_29_C228 @0xC368: one mini portal per event, as long as the effect is loaded (retail
    // 0xd44) and FindMiniPortals queued a point still unused (0xe28 < 0xe00). The damage is the
    // mini-portal block's (0x940 = CIngData 0x7c0 + 0x170 + 0x10) scaled by dt and made
    // immunity-proof (retail sets bit 7 of damage+0x1a). The spawn's name is the rodata pointer
    // lbl_29_rodata_120 + 0x573, which the dump does not resolve, so the guardian's identical
    // spawn text stands in [?]. Handled all the same, so the base class does not fire a generic
    // projectile.
    if (mMiniPortalEffect && mMiniPortalIndex >= 0 && mMiniPortalIndex < mMiniPortalCount) {
      const int index = mMiniPortalIndex++; // 0xe28
      // [this module] the mini portal attack itself (the Ing REL's CIngMiniPortalAttack, built
      // from mData.mMiniPortal's damage, sound and beam info) is not written yet - the module's
      // own body is a later slice. The queue's index still advances, as in the Ing.
      (void)index;
    }
    handled = true;
    break;
  case kUE_TakeOff: // 0xd
    mTakeOff = true; // 0xe5c bit 0
    handled = true;
    break;
  case kUE_BeginAction: // 0x11
    if (mAlive) {                      // CPatterned 0x420 bit 6
      mForm = kForm_BodyProjectile; // 0x9d4 = 4 (the flying body projectile)
      mBodyProjectileTime = 0.f;    // 0xe58 = rodata_6C
    }
    handled = true;
    break;
  case kUE_Activate: // 0x1d
    mStartAsIngSpot = true; // 0xe5d bit 5
    handled = true;
    break;
  case kUE_Deactivate: // 0x1e
    mStartAsIngSpot = false; // 0xe5d bit 5
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
