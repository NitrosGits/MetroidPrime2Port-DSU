// Host body for the SpacePirate REL (module 72), milestones 1a-2: the actors load, stand and render;
// the 39 triggers / 37 states are declared and installed (PortCSpacePirate{Triggers,States}.cpp).
// The analysis is docs/module-analysis/SpacePirate.md. Every vtable override below forwards to
// CPatterned behind a once-per-method StandIn log; the retail function it stands in for is named
// on each. There is no combat AI yet - CPatterned's default state machine drives the pirate.
#include "MetroidPrime/Enemies/CSpacePirate.hpp"

#include "Kyoto/Math/CMath.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "MetroidPrime/CActorLights.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CKnockBackInfo.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/CActorModelParticles.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CTeamAiRole.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTargetingPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/Weapons/CEnergyProjectile.hpp"
#include "MetroidPrime/Weapons/CGameProjectile.hpp"
#include "MetroidPrime/Weapons/CImpactVisorEffect.hpp"
#include "MetroidPrime/Weapons/CWeapon.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSpacePirate.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

#include <float.h>
#include <stdio.h>
#include <string.h>

namespace {

static const SBurst skBursts0[] = {
    {20, {3, 4, 5, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {20, {2, 3, 4, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {20, {6, 5, 4, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {20, {1, 2, 3, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {20, {7, 6, 5, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.0f, 0.0f},
};

static const SBurst skBursts1[] = {
    {15, {5, 3, 2, 1, -1, 0, 0, 0}, 0.1f, 0.05f},
    {20, {1, 2, 3, 4, -1, 0, 0, 0}, 0.1f, 0.05f},
    {20, {7, 6, 5, 4, -1, 0, 0, 0}, 0.1f, 0.05f},
    {15, {3, 4, 5, 6, -1, 0, 0, 0}, 0.1f, 0.05f},
    {15, {6, 5, 4, 3, -1, 0, 0, 0}, 0.1f, 0.05f},
    {15, {2, 3, 4, 5, -1, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.0f, 0.0f},
};

static const SBurst skBursts2[] = {
    {40, {1, 2, 3, 4, 5, 6, -1, 0}, 0.1f, 0.05f},
    {40, {7, 6, 5, 4, 3, 2, -1, 0}, 0.1f, 0.05f},
    {10, {2, 3, 4, 5, 4, 3, -1, 0}, 0.1f, 0.05f},
    {10, {6, 5, 4, 3, 4, 5, -1, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.0f, 0.0f},
};

static const SBurst skBursts3[] = {
    {20, {16, 4, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {40, {5, 7, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {40, {1, 10, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.0f, 0.0f},
};

static const SBurst skBursts4[] = {
    {15, {16, 1, 3, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {20, {3, 4, 6, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {25, {7, 5, 4, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {25, {2, 6, 4, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {15, {7, 5, 3, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.0f, 0.0f},
};

static const SBurst skBursts5[] = {
    {35, {7, 13, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {35, {9, 1, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {30, {16, 12, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.0f, 0.0f},
};

static const SBurst skBursts6[] = {
    {10, {16, 15, 13, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {20, {13, 12, 10, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {30, {9, 11, 12, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {30, {14, 10, 12, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {10, {9, 11, 13, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.0f, 0.0f},
};

static const SBurst skBursts7[] = {
    {26, {16, 8, 11, 14, -1, 0, 0, 0}, 0.1f, 0.05f},
    {26, {16, 13, 11, 12, -1, 0, 0, 0}, 0.1f, 0.05f},
    {16, {9, 11, 13, 10, -1, 0, 0, 0}, 0.1f, 0.05f},
    {16, {14, 13, 12, 11, -1, 0, 0, 0}, 0.1f, 0.05f},
    {8, {10, 11, 12, 13, -1, 0, 0, 0}, 0.1f, 0.05f},
    {8, {6, 8, 11, 13, -1, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.0f, 0.0f},
};

static const SBurst skBursts8[] = {
    {40, {1, 16, 14, 12, 10, 11, -1, 0}, 0.1f, 0.05f},
    {40, {9, 11, 12, 13, 11, 7, -1, 0}, 0.1f, 0.05f},
    {10, {8, 10, 11, 12, 13, 12, -1, 0}, 0.1f, 0.05f},
    {10, {15, 13, 12, 10, 12, 9, -1, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.0f, 0.0f},
};

static const SBurst skBursts9[] = {
    {40, {7, 13, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {40, {9, 1, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {20, {16, 12, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.0f, 0.0f},
};

static const SBurst skBursts10[] = {
    {30, {9, 11, 13, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {10, {13, 12, 10, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {15, {9, 11, 12, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {15, {14, 10, 12, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {30, {16, 15, 13, -1, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.0f, 0.0f},
};

static const SBurst skBursts11[] = {
    {35, {7, 13, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {35, {9, 1, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {30, {16, 12, -1, 0, 0, 0, 0, 0}, 0.1f, 0.05f},
    {0, {0, 0, 0, 0, 0, 0, 0, 0}, 0.0f, 0.0f},
};

// fn_72_854: the table CBurstFire is built from; its order is retail's.
static const SBurst* skBursts[] = {skBursts0, skBursts1, skBursts2, skBursts3, skBursts4, skBursts5,
                                   skBursts6, skBursts7, skBursts8, skBursts9, skBursts10,
                                   skBursts11, nullptr};

} // namespace

// Reports a stand-in the first time it runs, so a log lists what is still a shell. Shared with
// PortCSpacePirateStates.cpp and PortCSpacePirateTriggers.cpp.
void PortSpacePirateStandIn(const char* what) {
  static const char* sSeen[256] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == what || strcmp(sSeen[i], what) == 0) {
      return;
    }
  }
  if (sCount < 256) {
    sSeen[sCount++] = what;
  }
  fprintf(stderr, "[spacepirate] %s is a port stand-in (see PortCSpacePirate.cpp)\n", what);
}

namespace {
void StandIn(const char* what) { PortSpacePirateStandIn(what); }
} // namespace

// Retail keeps the pirates that are in a charge/melee attack in this module-level list (bss_40);
// DoubleSnap's Activate drops the pirate from it.
rstl::list< TUniqueId > CSpacePirate::mChargePlayerList;

CSpacePirate::CSpacePirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                           const CTransform4f& xf, const CModelData& modelData,
                           const CActorParameters& actParms, const CPatternedInfo& pInfo,
                           const CSpacePirateData& data)
// EPatternedAI 0x35, flavor 0, movement 0 (ground), collider 1, body 1 (bipedal): the retail
// constructor's arguments (fn_72_F49C).
: CPatterned(static_cast< EPatternedAI >(0x35), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mData(data) // fn_72_106D4
, x900_coverPoint(kInvalidUniqueId)
, x902_previousCoverPoint(kInvalidUniqueId)
, mPathFindSearch(nullptr, 1, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0,
                  static_cast< CPFRegion::ERootPosition >(0))
, mHeadSegId(GetModelData()->GetAnimationData()->GetLocatorSegId(rstl::string_l("Head_1")))
, mBoneTracking(*GetModelData()->GetAnimationData(), rstl::string_l("Head_1"), 1.2217305f,
                3.1415927f, kBTF_None)
, mAttachedActor(kInvalidUniqueId)
, mAttackRemTime(1.f)
, mAttackTarget(kInvalidUniqueId)
, mBurstFire(skBursts, static_cast< int >(data.mFirstBurstCount))
, xb14_jumpPoint(kInvalidUniqueId)
, xb30_(nullptr)
, xba0_teamAiMgrId(kInvalidUniqueId)
, mPortalPlane(0.f, CUnitVector3f(CVector3f::Forward())) {
  // Flag bits 0x8f4_24..0x8f6_24, unpacked from the data's flags word (fn_72_F49C); every other
  // bit of 0x8f6..0x8fa starts cleared. 0x8f9_29 is the one bit the retail ctor skips.
  const uint flags = data.mFlags;
  x8f4_24_pendingAmbush = (flags & 3) != 0;
  x8f4_25_ceilingAmbush = (flags & 0x2) != 0;
  x8f4_26_nonAggressive = (flags & 0x4) != 0;
  x8f4_27_melee = (flags & 0x8) != 0;
  x8f4_28_noShuffleCloseCheck = (flags & 0x10) != 0;
  x8f4_29_onlyAttackInRange = (flags & 0x20) != 0;
  x8f4_30_ = (flags & 0x40) != 0;
  x8f4_31_noKnockbackImpulseReset = (flags & 0x80) != 0;
  x8f5_24_noMeleeAttack = (flags & 0x200) != 0;
  x8f5_25_breakAttack = (flags & 0x400) != 0;
  x8f5_26_seated = (flags & 0x1000) != 0;
  x8f5_27_shadowPirate = (flags & 0x2000) != 0;
  x8f5_28_alertBeforeCloak = (flags & 0x4000) != 0;
  x8f5_29_noBreakDodge = (flags & 0x8000) != 0;
  x8f5_30_floatingCorpse = (flags & 0x10000) != 0;
  x8f5_31_ragdollNoAiCollision = (flags & 0x20000) != 0;
  x8f6_24_trooper = (flags & 0x40000) != 0;
  x8f6_25_hearNoise = false;
  x8f6_26_enableMeleeAttack = false;
  x8f6_27_ = false;
  x8f6_28_ = false;
  x8f6_29_enableRetreat = false;
  x8f6_30_shuffleClose = false;
  x8f6_31_inAttackState = false;
  x8f7_24_enablePatrol = false;
  mEnableAim = false;
  x8f7_26_hearPlayerFire = false;
  x8f7_27_inProjectilePath = false;
  x8f7_28_noPlayerLos = false;
  x8f7_29_inWallHang = false;
  x8f7_30_jumpVelSet = false;
  x8f7_31_prevInCineCam = false;
  x8f8_24_pendingFrenzyChance = false;
  x8f8_25_appliedBladeDamage = false;
  x8f8_26_alwaysAggressive = false;
  x8f8_27_coverCheck = false;
  x8f8_28_enableDodge = false;
  x8f8_29_noPlayerDodge = false;
  mAllEnergyDrained = false;
  x8f8_31_mayStartAttack = false;
  x8f9_24_ = false;
  x8f9_25_useJumpBackJump = false;
  x8f9_26_started = false;
  x8f9_27_inRange = false;
  x8f9_28_satUp = false;
  x8f9_29_enableBreakDodge = false; // TODO(fn_72_F49C): retail leaves this bit untouched
  x8f9_30_closeMelee = false;
  x8f9_31_sentAttackMsg = false;
  x8fa_24_normalDodge = false;
  x8fa_25_ = false;
  x8fa_26_ = false;
  x8fa_27_ = false;
  x8fa_28_ = false;
  x8fa_29_ = false;
  x8fa_30_ = false;
  x8fa_31_ = false;

  x8fc_frenzyFrames = 0;
  x904_steeringSpeed = 1.f;
  x908_targetDelta = CVector3f::Forward();
  x914_coverPointRearDir = CVector3f::Zero();
  xa0c_unkTimer = 0.f;
  xa10_steeringDelayTimer = 0.f;
  xa14_ = 0;
  // TODO(fn_72_F49C): 0xa18 = pInfo+0x58 (MP1's x750_initialHP is the patterned health); not
  // resolved against CPatternedInfo's layout.
  xa18_ = 0.f;
  xa1c_coverRange = 0.f;
  xa24_ = 0;
  xa28_taunt = -1;
  xa68_coverDir = -1;
  xa70_intoJumpDist = 1.f;
  xa74_eyeHeight = 2.f;
  xa78_ = 0.f;
  xa7c_timeNoPlayerLos = 0.f;
  xa80_cantSeePlayerCycleCounter = 0;
  mBurstFire.SetBurstType(1);
  xaf8_jumpHeight = 3.f;
  xafc_patrolDestPos = CVector3f::Zero();
  xb08_skidDir = -1;
  xb0c_strafeDelayTimer = 0.f;
  xb10_meleeSeverity = -1;
  xb18_dodgeDir = -1;
  xb1c_dodgeDist = 3.f;
  xb20_breakDodgeDist = 3.f;
  xb24_timeSinceHitByPlayer = FLT_MAX;
  xb28_lowHealthFrenzyTimer = FLT_MAX;
  xb2c_ragdollDelayTimer = 0.f;
  xb7c_cloakDelayTimer = 0.f;
  xb80_electricParticleTimer = 0.f;
  xb84_cloakStepTime = 0.f;
  xb88_shadowPirateAlpha = 0.5f;
  xb8c_minCloakAlpha = data.mCloakOpacity;
  xb90_maxCloakAlpha = data.mMaxCloakOpacity;
  xb94_dodgeDelayTimer = data.mBreakDodgeMinTime;
  xb98_aimDelayTimer = data.mGunTrackDelay;
  xb9c_ = 0.f;
  xba4_heldPositionX = 0.f;
  xba8_heldPositionY = 0.f;
  xbac_holdPositionTime = 0.f;
  xbb0_leashTimer = 0.f;
  xbb4_ = CVector3f::Zero();
  xbc0_playerIndex = -1;
  xbc4_ = CVector3f::Zero();
  xbd0_ = 0.f;
  xbfc_ = false;
  xc00_ = 0;
  xc04_warpPoint = -1;
  xc54_ = false;
  xc58_ = 0;

  // TODO(fn_72_4F28): ctor-time init from the launcher block (data+0xc8): grenade launcher /
  // bouncy-grenade setup; not ported with the grenade states.
  if (data.mProjectile != kInvalidAssetId) {
    mProjectileInfo = CProjectileInfo(data.mProjectile, data.mProjectileDamage);
  }
  mBurstFire.SetBurstType(1);

  const CVector3f scale = GetModelData()->GetScale();
  mBoneTracking.SetDisableTrackingDistance(25.f * scale.GetZ());

  const CAnimData* animData = GetModelData()->GetAnimationData();
  xa87_elbowSeg = animData->GetLocatorSegId(rstl::string_l("R_elbow"));
  xa88_wristSeg = animData->GetLocatorSegId(rstl::string_l("R_wrist"));
  xa89_swooshSeg = animData->GetLocatorSegId(rstl::string_l("Swoosh_LCTR"));
  xa86_gunSeg = animData->GetLocatorSegId(rstl::string_l("gun_LCTR"));
  xa8a_lHipSeg = animData->GetLocatorSegId(rstl::string_l("L_hip"));
  xa8b_rHipSeg = animData->GetLocatorSegId(rstl::string_l("R_hip"));
  xa8c_collarSeg = animData->GetLocatorSegId(rstl::string_l("Collar"));

  if (!x8f4_29_onlyAttackInRange) {
    const CPASAnimParmData jump(pas::kAS_Jump, CPASAnimParm::FromEnum(0), CPASAnimParm::FromEnum(0),
                                CPASAnimParm::FromEnum(0));
    xa70_intoJumpDist = scale.GetY() * GetAnimationDistance(jump);
    const CPASAnimParmData dodge(pas::kAS_Step, CPASAnimParm::FromEnum(3),
                                 CPASAnimParm::FromEnum(1));
    xb1c_dodgeDist = scale.GetX() * GetAnimationDistance(dodge);
    const CPASAnimParmData breakDodge(pas::kAS_Step, CPASAnimParm::FromEnum(3),
                                      CPASAnimParm::FromEnum(2));
    xb20_breakDodgeDist = scale.GetX() * GetAnimationDistance(breakDodge);
  } else {
    // TODO(fn_72_F49C): retail sets bit 0x80 of the body controller's byte at +0x56c here
    // (MP1: BodyStateInfo().SetLocoAnimChangeAtEndOfAnimOnly(true)); no such setter in this tree.
  }

  const CAABox& baseBox = GetBaseBoundingBox();
  xa74_eyeHeight = 0.6f * (baseBox.GetMaxPoint().GetZ() - baseBox.GetMinPoint().GetZ());

  if (HasActorLights()) {
    ActorLights()->SetAmbienceGenerated(false);
  }
  // TODO(fn_72_F49C): `this->x682 |= 0x80` (a CPatterned bit, likely the knock-back controller's
  // locomotion-during-electrocution flag); not mapped to a member.
  if (!BodyController()->HasBodyState(static_cast< pas::EAnimationState >(0x16))) {
    x8f4_27_melee = true;
  }
  // TODO(fn_72_F49C): if pInfo+0x2a4 bit 0x80, retail builds a CEchoEmitter (0x70, DOL
  // fn_802455CC) from segs Head_1/L_ankle/R_ankle/Swoosh_LCTR/... and calls SetEchoEmitter; the
  // CPatternedInfo flag is unnamed here, so no emitter is made.
  // TODO(fn_72_F49C): if x8f5_30_floatingCorpse, retail clears bit 0x40 of the CActor flag byte at
  // +0x154; unmapped.
}

CSpacePirate::~CSpacePirate() {}

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove / Death / KnockBack all dereference the current state. Retail activates it from
// its own AcceptScriptMsg or Think (fn_72_E498 / fn_72_1980; the analysis does not say which), so
// the port activates on kSM_Create and, as the FlyingPirate does, again on demand in every entry
// point that can run first.
void CSpacePirate::EnsureBodyActive(CStateManager& mgr) {
  if (BodyController() != nullptr && !BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

bool CSpacePirate::PortAttachActor(TUniqueId id) {
  // Retail fn_72_E0D8; the wisp tentacle attaches the actor it is dragging.
  StandIn("CSpacePirate::AttachActorToPirate (fn_72_E0D8)");
  mAttachedActor = id;
  return true;
}

void CSpacePirate::PortDetachActor() {
  StandIn("CSpacePirate::DetachActorFromPirate (fn_72_E0AC)");
  mAttachedActor = kInvalidUniqueId;
}

// @ fn_72_24C4
void CSpacePirate::PreThink(float dt, CStateManager& mgr) {
  EnsureBodyActive(mgr);
  // Retail skips the head tracking while the ragdoll is primed (xb30_ is never set yet).
  if (xb30_ == nullptr) {
    mBoneTracking.PreThink(*ModelData()->AnimationData());
  }
  CPatterned::PreThink(dt, mgr);
}

namespace {
// MP2_DEBUG_SPACEPIRATE=1: one line per entry of the ported entry points (arg is a detail value).
void TraceCall(const CSpacePirate& self, const char* what, int arg) {
  static const bool sOn = getenv("MP2_DEBUG_SPACEPIRATE") != nullptr;
  if (sOn) {
    fprintf(stderr, "[spacepirate] %u %s %d\n", self.GetUniqueId().Value(), what, arg);
  }
}

// MP2_DEBUG_SPACEPIRATE=1: one line per state change of each pirate.
void TraceState(CSpacePirate& self, const char* name) {
  static const bool sOn = getenv("MP2_DEBUG_SPACEPIRATE") != nullptr;
  if (!sOn) {
    return;
  }
  static unsigned short sIds[256];
  static const char* sLast[256];
  static int sCount = 0;
  const unsigned short id = self.GetUniqueId().Value();
  int i = 0;
  while (i < sCount && sIds[i] != id) {
    ++i;
  }
  if (i == sCount) {
    if (sCount == 256) {
      return;
    }
    sIds[sCount] = id;
    sLast[sCount++] = nullptr;
  }
  if (sLast[i] == name || (sLast[i] && name && strcmp(sLast[i], name) == 0)) {
    return;
  }
  fprintf(stderr, "[spacepirate] %u state %s -> %s\n", (unsigned)id, sLast[i] ? sLast[i] : "(none)",
          name ? name : "(none)");
  sLast[i] = name;
}
} // namespace

// @ fn_72_1980
void CSpacePirate::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CBodyController* body = BodyController();
  const bool inCineCam = mgr.GetCameraManager(0)->IsInCinematicCamera();
  if (inCineCam && !x8f7_31_prevInCineCam) {
    SetCinematicCollision(mgr);
  } else if (!inCineCam && x8f7_31_prevInCineCam && !x8f5_31_ragdollNoAiCollision) {
    SetNonCinematicCollision(mgr);
  }
  x8f7_31_prevInCineCam = inCineCam;
  const float steeringSpeed = xa10_steeringDelayTimer != 0.f ? 0.f : x904_steeringSpeed;
  body->CommandMgr().SetSteeringSpeedRange(steeringSpeed, steeringSpeed);
  xa0c_unkTimer = CMath::Max(0.f, xa0c_unkTimer - dt);
  if (mAlive) {
    xb24_timeSinceHitByPlayer += dt;
    xb28_lowHealthFrenzyTimer += dt;
    if (x8f7_27_inProjectilePath) {
      xb28_lowHealthFrenzyTimer = 0.f;
      x8f7_27_inProjectilePath = false;
    }
    if (mHitByPlayerProjectile) {
      xb24_timeSinceHitByPlayer = 0.f;
      mHitByPlayerProjectile = false;
    }
  }
  UpdateCloak(dt, mgr);
  if (body->GetPercentageFrozen() < 1.f) {
    if (mAlive) {
      xa10_steeringDelayTimer = CMath::Max(0.f, xa10_steeringDelayTimer - dt);
      if (x8f7_28_noPlayerLos) {
        xa7c_timeNoPlayerLos += dt;
      } else {
        xa7c_timeNoPlayerLos = 0.f;
      }
      xb0c_strafeDelayTimer = CMath::Max(0.f, xb0c_strafeDelayTimer - dt);
      xb94_dodgeDelayTimer = CMath::Max(0.f, xb94_dodgeDelayTimer - dt);
      CheckForProjectiles(mgr);
      if (mEnableAim) {
        mAttackTarget = ChooseTarget(mgr);
        UpdateTeamTarget(mgr);
      }
    }
    UpdateAttacks(dt, mgr);
    UpdateAimBodyState(dt, mgr);
    xb34_ikChain.Update(dt);
  }
  CPatterned::Think(dt, mgr);
  if (body->GetPercentageFrozen() != 1.f) {
    mBoneTracking.Think(dt);
  }
  TraceState(*this, StateMachineState().GetName());
}

void CSpacePirate::SetCinematicCollision(CStateManager& mgr) {
  RemoveMaterial(kMT_AIBlock, mgr);
  CMaterialList include = GetMaterialFilter().GetIncludeList();
  include.Remove(kMT_AIBlock);
  SetMaterialFilter(
      CMaterialFilter::MakeIncludeExclude(include, GetMaterialFilter().GetExcludeList()));
}

void CSpacePirate::SetNonCinematicCollision(CStateManager& mgr) {
  AddMaterial(kMT_AIBlock, mgr);
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
      GetMaterialFilter().GetIncludeList().Union(CMaterialList(kMT_AIBlock)),
      GetMaterialFilter().GetExcludeList()));
}

// @ fn_72_DB48
bool CSpacePirate::LineOfSightTest(CStateManager& mgr, const CVector3f& start,
                                   const CVector3f& end, const CMaterialList& exclude) {
  const CMaterialFilter filter =
      CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), exclude);
  return mgr.RayCollideWorld(start, end, filter, this);
}

// Shadow pirates fade in and out; the cloak alpha lives in the actor's colour.
void CSpacePirate::UpdateCloak(float dt, CStateManager& mgr) {
  if (!x8f5_27_shadowPirate) {
    return;
  }
  if (mAlive) {
    if (xb7c_cloakDelayTimer > 0.f) {
      xb7c_cloakDelayTimer -= dt;
      if (xb7c_cloakDelayTimer <= 0.f) {
        mAlphaDelta = -0.4f;
      }
    }
  } else {
    xb8c_minCloakAlpha = 0.f;
    xb90_maxCloakAlpha = 1.f;
  }
  if (xb80_electricParticleTimer > 0.f) {
    xb80_electricParticleTimer -= dt;
    if (xb80_electricParticleTimer <= 0.f && !BodyController()->IsElectrocuting()) {
      mgr.ActorModelParticles()->StopElectric(*this);
    }
  }
  if (BodyController()->GetPercentageFrozen() != 1.f) {
    mAlphaDelta = 2.f;
  }
  if (mAlphaDelta < 0.f && mColor.GetAlpha() < xb8c_minCloakAlpha) {
    mColor.SetAlpha(xb8c_minCloakAlpha);
    mAlphaDelta = 0.f;
    RemoveMaterial(kMT_Target, mgr);
  }
  if (mAlphaDelta > 0.f && mColor.GetAlpha() > xb90_maxCloakAlpha) {
    mColor.SetAlpha(xb90_maxCloakAlpha);
    AddMaterial(kMT_Target, mgr);
  }
  xb84_cloakStepTime -= dt;
  if (xb84_cloakStepTime < 0.f) {
    const float random = mgr.Random()->Float();
    xb84_cloakStepTime = (1.f - random) * 0.08f;
    if (mAlphaDelta < 0.f) {
      xb88_shadowPirateAlpha = mColor.GetAlpha();
      if (mAlive) {
        xb88_shadowPirateAlpha -= random * (mColor.GetAlpha() - xb8c_minCloakAlpha);
      }
    } else if (mAlphaDelta > 0.f) {
      xb88_shadowPirateAlpha =
          mColor.GetAlpha() + random * (xb90_maxCloakAlpha - mColor.GetAlpha());
    } else {
      xb88_shadowPirateAlpha = mColor.GetAlpha();
    }
  }
}

bool CSpacePirate::CheckTargetable(CStateManager& mgr) { return GetModelAlphau8(mgr) > 127; }

// Ends the pirate's claim on a squad attack slot.
void CSpacePirate::SquadReset(CStateManager& mgr) {
  CScriptTeamAiMgr::EndAttack(x8f4_27_melee ? CScriptTeamAiMgr::kAT_Melee
                                            : CScriptTeamAiMgr::kAT_Projectile,
                              mgr, xba0_teamAiMgrId, GetUniqueId(), !x8f4_27_melee);
}

// Tells the squad manager who this member is shooting at.
void CSpacePirate::UpdateTeamTarget(CStateManager& mgr) {
  if (xba0_teamAiMgrId == kInvalidUniqueId) {
    return;
  }
  if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(xba0_teamAiMgrId))) {
    team->SetMemberTargetId(GetUniqueId(), mAttackTarget);
  }
}

TUniqueId CSpacePirate::ChooseTeamTarget(CStateManager& mgr) {
  if (xba0_teamAiMgrId != kInvalidUniqueId) {
    CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(xba0_teamAiMgrId));
    if (team != nullptr && team->IsTeamMemberInRange(mgr, *this, 20.f)) {
      return team->FindBestIndividualAttackTarget(mgr, *this);
    }
  }
  return CScriptTeamAiMgr::ChoosePlayer(mgr, *this);
}

// Keeps the current target unless it has gone inactive, in which case the squad picks one; a distant target is dropped for a player within 10 units.
TUniqueId CSpacePirate::ChooseTarget(CStateManager& mgr) {
  float nearest = 10.f;
  float currentDist = 0.f;
  TUniqueId nearestId = kInvalidUniqueId;
  TUniqueId result = mAttackTarget;
  for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
    const CPlayer* player = mgr.GetPlayer(i);
    if (player == nullptr) {
      continue;
    }
    if (player->GetUniqueId() == mAttackTarget) {
      if (!player->GetActive()) {
        result = ChooseTeamTarget(mgr);
      } else {
        currentDist = (player->GetTranslation() - GetTranslation()).Magnitude();
      }
    } else {
      const float dist = (player->GetTranslation() - GetTranslation()).Magnitude();
      if (dist < nearest) {
        nearest = dist;
        nearestId = player->GetUniqueId();
      }
    }
  }
  if (currentDist > 30.f && nearest < 10.f) {
    result = nearestId;
  }
  return result;
}

// Whether the next shot is a frenzy burst; a roll of 2..4 shots is queued and counted down.
bool CSpacePirate::ShouldFrenzy(CStateManager& mgr) {
  bool reset = false;
  if (x8f8_24_pendingFrenzyChance) {
    x8f8_24_pendingFrenzyChance = false;
    if (mgr.Random()->Next() % 100 < 25) {
      reset = true;
    }
  }
  // Retail also resets while the global charge-player list is non-empty (no port equivalent yet).
  const CPlayer* target = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mAttackTarget));
  if (target != nullptr && target->GetMorphballTransitionState() == CPlayer::kMS_Morphed) {
    reset = true;
  }
  if (GetHealthInfo()->GetHP() < 0.3f * xa18_ && mgr.Random()->Next() % 100 < 60 &&
      xb28_lowHealthFrenzyTimer < 0.5f) {
    reset = true;
  }
  if (reset) {
    x8fc_frenzyFrames = mgr.Random()->Range(2, 4);
  }
  return --x8fc_frenzyFrames >= 0;
}

// Where the pirate looks and aims: its target, or a point ahead if the target is gone.
CVector3f CSpacePirate::GetTargetPos(CStateManager& mgr) {
  CEntity* entity = mgr.ObjectById(mAttackTarget);
  if (const CPlayer* player = TCastToPtr< CPlayer >(entity)) {
    return player->GetTranslation();
  }
  if (const CActor* actor = TCastToPtr< CActor >(entity)) {
    if (actor->GetActive()) {
      return actor->GetTranslation();
    }
  }
  mAttackTarget = ChooseTeamTarget(mgr);
  UpdateTeamTarget(mgr);
  mBoneTracking.SetTarget(mAttackTarget);
  return GetTranslation() + 10.f * GetTransform().GetForward();
}

// Sets x8f7_27_inProjectilePath when a shot is flying at the pirate.
void CSpacePirate::CheckForProjectiles(CStateManager& mgr) {
  if (!x8f7_26_hearPlayerFire) {
    return;
  }
  const CVector3f extent(5.f, 5.f, 5.f);
  const CAABox bounds(xbb4_ - extent, xbb4_ + extent);
  x8f7_27_inProjectilePath = false;
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, bounds, CMaterialFilter::MakeInclude(CMaterialList(kMT_Projectile)),
                    nullptr);
  for (int i = 0; i < nearList.size() && !x8f7_27_inProjectilePath; ++i) {
    const CGameProjectile* projectile =
        TCastToConstPtr< CGameProjectile >(mgr.GetObjectById(nearList[i]));
    if (projectile == nullptr) {
      continue;
    }
    CVector3f delta = GetBoundingBox().GetCenterPoint() - projectile->GetTranslation();
    if (!delta.IsMagnitudeSafe()) {
      x8f7_27_inProjectilePath = true;
    } else if (CVector3f::Dot(GetTransform().GetForward(), delta) < 0.f) {
      delta.Normalize();
      CVector3f travel = projectile->GetTranslation() - projectile->GetPreviousPos();
      if (travel.IsMagnitudeSafe()) {
        travel.Normalize();
        if (CVector3f::Dot(travel, delta) > 0.939f) {
          x8f7_27_inProjectilePath = true;
        }
      }
    }
  }
  x8f7_26_hearPlayerFire = false;
}

// Fires one shot from the gun locator at the target (or straight ahead if the pirate is dying).
// @ fn_72_45F8
bool CSpacePirate::FireProjectile(float dt, CStateManager& mgr) {
  bool fired = false;
  const CTransform4f gunXf = GetLctrTransform(xa86_gunSeg);
  if (!mAlive) {
    LaunchProjectile(gunXf, mgr, 6, CWeapon::kPA_None, false, CImpactVisorEffect::None(),
                     CVector3f(1.f, 1.f, 1.f));
    fired = true;
  } else if (const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(mAttackTarget))) {
    CVector3f pos = actor->GetTranslation();
    bool isPlayer = false;
    if (const CPlayer* player = TCastToConstPtr< CPlayer >(actor)) {
      if (player->GetMorphballTransitionState() == CPlayer::kMS_Morphed) {
        isPlayer = true;
      } else {
        pos = ProjectileInfo()->PredictInterceptPos(gunXf.GetTranslation(),
                                                    player->GetAimPosition(mgr, 0.f), *player,
                                                    true, dt);
      }
    }
    CVector3f gunToPos = pos - gunXf.GetTranslation();
    const float distance = gunToPos.Magnitude();
    gunToPos *= 1.f / distance;
    const float dot = CVector3f::Dot((GetLctrTransform(xa88_wristSeg).GetTranslation() -
                                      GetLctrTransform(xa87_elbowSeg).GetTranslation())
                                         .AsNormalized(),
                                     gunToPos);
    if ((dot > 0.707f || (distance < 6.f && dot > 0.5f)) &&
        (isPlayer || LineOfSightTest(mgr, gunXf.GetTranslation(), pos,
                                     CMaterialList(kMT_Player, kMT_ProjectilePassthrough)))) {
      pos += GetTransform().Rotate(mBurstFire.GetDistanceCompensatedError(distance, 6.f));
      const CTransform4f shotXf = CTransform4f::LookAt(gunXf.GetTranslation(), pos, CVector3f::Up());
      LaunchProjectile(shotXf, mgr, 6, CWeapon::kPA_None, false, CImpactVisorEffect::None(),
                       CVector3f(1.f, 1.f, 1.f));
      fired = true;
    }
  }
  if (fired) {
    CSfxManager::AddEmitter(mData.mProjectileSfx, GetTranslation(), GetCurrentAreaId().Value(),
                            true, false, CSfxManager::kMedPriority);
  }
  return fired;
}

// Squad bookkeeping and the burst-fire pattern. Retail's switch has a second case (an inert
// pirate that only counts its attack timer down) that no state reaches.
// @ fn_72_28F0
void CSpacePirate::UpdateAttacks(float dt, CStateManager& mgr) {
  bool reset = true;
  const bool canShoot =
      !mAlive || (BodyController()->GetBodyStateInfo().GetCurrentState()->CanShoot() &&
                  mEnableAim && !x8f4_27_melee && !BodyController()->IsFrozen() &&
                  !x8f4_25_ceilingAmbush && !x8f9_26_started &&
                  !BodyController()->IsElectrocuting());
  if (canShoot && mBurstFire.GetBurstType() != -1) {
    const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mAttackTarget));
    if (mAlive && (!x8f4_29_onlyAttackInRange ||
                   (player != nullptr &&
                    (player->GetTranslation() - GetTranslation()).MagSquared() <
                        mLeashRadius * mLeashRadius))) {
      reset = false;
      mAttackRemTime -= dt;
      if (mAttackRemTime < 0.f) {
        const CTeamAiRole* role =
            CScriptTeamAiMgr::GetTeamAiRole(mgr, xba0_teamAiMgrId, GetUniqueId());
        if ((role == nullptr || role->GetTeamAiRole() == CTeamAiRole::kTAR_Projectile) &&
            (xba0_teamAiMgrId == kInvalidUniqueId ||
             CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr,
                                           xba0_teamAiMgrId, GetUniqueId()))) {
          if (ShouldFrenzy(mgr)) {
            mBurstFire.SetBurstType(2);
          }
          if (x8f5_26_seated) {
            mBurstFire.SetBurstType(5);
          }
          // A player facing away from this pirate gets the "behind" bursts (6..12).
          if (player != nullptr &&
              CVector3f::Dot(GetTranslation() - player->GetTranslation(),
                             player->GetTransform().GetForward()) < 0.f &&
              mBurstFire.GetBurstType() < 6) {
            mBurstFire.SetBurstType(mBurstFire.GetBurstType() + 6);
          }
          mBurstFire.Start(mgr);
          mAttackRemTime = mgr.Random()->Float() * mAttackTimeVariation + GetAverageAttackTime();
          if (player != nullptr) {
            const CVector3f fromPlayer =
                (GetGunEyePos() - player->GetAimPosition(mgr, 0.f)).AsNormalized();
            if (CVector3f::Dot(fromPlayer, player->GetTransform().GetForward()) < 0.9f) {
              // Another armed pirate in the same area is already shooting: wait a little longer.
              const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
              for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
                const CSpacePirate* other = TCastToConstPtr< CSpacePirate >(list[i]);
                if (other != nullptr && other != this && other->mEnableAim &&
                    other->GetCurrentAreaId() == GetCurrentAreaId()) {
                  mAttackRemTime += 0.2f;
                }
              }
            }
          }
        }
      }
    }
    mBurstFire.Update(mgr, dt);
    if (mBurstFire.ShouldFire()) {
      if (player != nullptr && player->GetSidewaysDashing() && mgr.Random()->Float() < 0.5f) {
        mBurstFire.SetAvoidAccuracy(true);
      }
      FireProjectile(dt, mgr);
      mBurstFire.SetAvoidAccuracy(false);
      mBurstFire.SetTimeToNextShot(mData.mIntraBurstVariation * (mgr.Random()->Float() - 0.5f) +
                                   mData.mIntraBurstTime);
    } else if (!mBurstFire.IsBurstSet()) {
      reset = true;
    }
  }
  if (reset) {
    SquadReset(mgr);
  }
  for (uint i = 0; i < mgr.GetNumPlayers(); ++i) {
    SetValidTarget(static_cast< int >(i), CheckTargetable(mgr));
  }
}

// Points the additive aim animation at the target while the pirate may aim, and eases it back to
// straight ahead (then idle) once it may not. Retail also passes the wall-hang flag as the aim
// command's type; CBCAdditiveAimCmd has no setter for it yet.
// @ fn_72_253C
void CSpacePirate::UpdateAimBodyState(float dt, CStateManager& mgr) {
  CBodyController* body = BodyController();
  CBodyStateCmdMgr& cmds = body->CommandMgr();
  if (mAlive && mEnableAim && !body->IsFrozen() && !body->IsElectrocuting() && !x8f4_27_melee &&
      xb30_ == nullptr && (!x8f5_26_seated || x8f9_28_satUp) && !x8fa_30_) {
    xb98_aimDelayTimer = CMath::Max(0.f, xb98_aimDelayTimer - dt);
    if (xb98_aimDelayTimer == 0.f) {
      cmds.DeliverCmd(CBCAdditiveAimCmd());
      const CVector3f gunPos = GetLctrTransform(xa86_gunSeg).GetTranslation();
      CVector3f offset = x8f7_29_inWallHang ? CVector3f::Zero() : GetTranslation() - gunPos;
      CVector3f aim = GetTransform().TransposeMultiply(GetTargetPos(mgr) + offset);
      if (x8f7_29_inWallHang) {
        aim.SetX(-aim.GetX());
        aim.SetY(-aim.GetY());
      }
      cmds.DeliverAdditiveTargetVector(aim);
      xb9c_ = 0.5f;
    }
  } else if (xb9c_ > 0.f) {
    xb9c_ -= dt;
    cmds.DeliverAdditiveTargetVector(GetTransform().GetForward());
    if (xb9c_ <= 0.f) {
      xb9c_ = 0.f;
      cmds.DeliverCmd(kBSC_AdditiveIdle);
    }
  }
}

// Retail fn_72_E498, in two passes: the wall-hang / ceiling-ambush filters and the activation
// bookkeeping run before CPatterned's handler, the per-message state after it.
// @ fn_72_E498
void CSpacePirate::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const EScriptObjectMessage message = msg.GetMessage();
  if (x8f7_29_inWallHang || x8f4_25_ceilingAmbush) {
    switch (message) {
    case kSM_OffGround: {
      CBodyController* body = BodyController();
      const pas::EAnimationState bodyState = body->GetCurrentStateId();
      if ((x8f7_29_inWallHang && bodyState == pas::kAS_WallHang &&
           !body->GetBodyStateInfo().GetCurrentState()->ApplyGravity()) ||
          (x8f4_25_ceilingAmbush &&
           (bodyState == pas::kAS_Locomotion ||
            (bodyState == pas::kAS_Jump &&
             !body->GetBodyStateInfo().GetCurrentState()->IsInAir(*body))))) {
        Stop();
        SetMomentumWR(CVector3f::Zero());
        return;
      }
      break;
    }
    case kSM_Landed:
      xb24_timeSinceHitByPlayer = FLT_MAX;
      break;
    default:
      break;
    }
  }
  if (message == kSM_Alert || message == kSM_Activate) {
    if (GetActive()) {
      if (x8f4_29_onlyAttackInRange) {
        x8f8_31_mayStartAttack = true;
      } else {
        mHitByPlayerProjectile = true;
      }
    } else if (x8f4_25_ceilingAmbush) {
      RemoveMaterial(kMT_GroundCollider, mgr);
      mOnGround = false;
    }
    if (message == kSM_Activate && x8f6_24_trooper) {
      // TODO(fn_72_E498): retail also stores an alpha into the colour byte here when the trooper
      // is still inactive with no warp point; the value is lost in the pseudocode.
      x8fa_26_ = true;
    }
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
  switch (message) {
  case kSM_AreaLoaded: {
    const rstl::vector< SConnection >& connections = GetConnectionList();
    for (AUTO(it, connections.begin()); it != connections.end(); ++it) {
      if (it->state == kSS_Retreat && it->msg == kSM_Next) {
        TUniqueId id = mgr.GetIdForScript(it->objId);
        if (CScriptCoverPoint* cover = TCastToPtr< CScriptCoverPoint >(mgr.ObjectById(id))) {
          cover->Reserve(GetUniqueId());
        }
      } else if (it->state == kSS_Patrol && it->msg == kSM_Follow) {
        x8f7_24_enablePatrol = true;
        // The patrol state's CPatterned half (move along the waypoints) runs only under this bit.
        TraceCall(*this, "enablePatrol", 1);
      }
    }
    mPathFindSearch.SetArea(
        mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea);
    if (x8f5_30_floatingCorpse) {
      xb2c_ragdollDelayTimer = 0.01f;
      RemoveMaterial(kMT_Character, kMT_Solid, kMT_Target, kMT_Orbit, mgr);
      mAlive = false;
      HealthInfo()->SetHP(-1.f);
    }
    break;
  }
  case kSM_Decrement:
    // TODO(fn_72_E498): retail clears the ragdoll's no-over-timer and small-movement flags;
    // there is no ragdoll yet (xb30_ stays null).
    break;
  case kSM_Create: {
    if (x8f4_25_ceilingAmbush && x8f5_27_shadowPirate) {
      mColor.SetAlpha(mData.mCloakOpacity);
      mAlphaDelta = -1.f;
    }
    xa24_ = mgr.Random()->Next() % 6;
    CMaterialList include = GetMaterialFilter().GetIncludeList();
    CMaterialList exclude = GetMaterialFilter().GetExcludeList();
    CMaterialList passthrough(kMT_AIPassthrough);
    include.Remove(passthrough);
    exclude.Add(passthrough);
    SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(include, exclude));
    break;
  }
  case kSM_SetToZero:
    if (GetActive()) {
      x8f6_29_enableRetreat = true;
      TrooperWarpOut(mgr, false);
    }
    break;
  case kSM_OffGround:
    if (BodyController()->GetPercentageFrozen() != 1.f) {
      float momentum = GetGravityConstant() * GetMass();
      if (x8f4_25_ceilingAmbush) {
        momentum *= 3.f;
      }
      SetMomentumWR(CVector3f(0.f, 0.f, -momentum));
    }
    if (BodyController()->GetCurrentStateId() == pas::kAS_Step) {
      SetVelocityWR(CVector3f(0.f, 0.f, GetVelocityWR().GetZ()));
    }
    mBurstFire.SetBurstType(3);
    break;
  case kSM_Launching:
    if (BodyController()->GetCurrentStateId() != pas::kAS_Jump) {
      CScriptMsg offGround(msg.GetSenderId(), kInvalidUniqueId, kSM_OffGround);
      CPatterned::AcceptScriptMsg(mgr, offGround);
      SetMomentumWR(CVector3f(0.f, 0.f, -GetGravityConstant() * GetMass()));
      SetVelocityForJump();
    }
    break;
  case kSM_Landed:
    mBurstFire.SetBurstType(x8f4_29_onlyAttackInRange ? 4 : 1);
    x8f7_30_jumpVelSet = false;
    x8fa_29_ = false;
    if (x8f5_27_shadowPirate && GetVelocityWR().GetZ() < -1.f) {
      mAlphaDelta = 1.f;
      xb7c_cloakDelayTimer += -0.05f * GetVelocityWR().GetZ();
      xb7c_cloakDelayTimer = CMath::Clamp(0.f, xb7c_cloakDelayTimer, 1.f);
      xb90_maxCloakAlpha = 0.5f;
      if (mAlive) {
        mgr.ActorModelParticles()->StartElectric(*this);
        xb80_electricParticleTimer = 1.f + xb7c_cloakDelayTimer;
      }
    }
    break;
  case kSM_Action:
    if (CScriptTargetingPoint* point =
            TCastToPtr< CScriptTargetingPoint >(mgr.ObjectById(msg.GetSenderId()))) {
      if (point->GetActive()) {
        mBoneTracking.SetTarget(msg.GetSenderId());
        mAttackTarget = msg.GetSenderId();
        UpdateTeamTarget(mgr);
        mHitByPlayerProjectile = true;
      } else {
        mAttackTarget = ChooseTeamTarget(mgr);
        UpdateTeamTarget(mgr);
        mBoneTracking.SetTarget(mAttackTarget);
      }
      mAttackRemTime = 0.f;
    }
    break;
  case kSM_Delete:
  case kSM_Deactivate:
    SquadRemove(mgr);
    // TODO(fn_72_E498): retail also drops the pirate from the module's charge-player list.
    break;
  case kSM_Start:
    x8f9_26_started = false;
    break;
  case kSM_Stop:
    x8f9_26_started = true;
    break;
  case kSM_Escape:
    TrooperWarpOut(mgr, true);
    break;
  default:
    break;
  }
}

// Retail fn_72_DF2C: leave the team manager the pirate joined.
void CSpacePirate::SquadRemove(CStateManager& mgr) {
  if (xba0_teamAiMgrId == kInvalidUniqueId) {
    return;
  }
  if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(xba0_teamAiMgrId))) {
    if (team->IsPartOfTeam(GetUniqueId())) {
      team->QuitTeam(GetUniqueId());
      xba0_teamAiMgrId = kInvalidUniqueId;
    }
  }
}

// Retail fn_72_51C8: a live trooper with no warp point and no pending warp enters "WarpOut".
void CSpacePirate::TrooperWarpOut(CStateManager& mgr, bool flag) {
  if (mAlive && !x8fa_25_ && xc04_warpPoint == -1 && x8f6_24_trooper) {
    mStateMachine->SetState(mgr, *this, rstl::string_l("WarpOut"));
    x8fa_27_ = flag;
  }
}

// Retail fn_72_E168 (MP1 SetVelocityForJump): ballistic launch to the patrol destination.
void CSpacePirate::SetVelocityForJump() {
  if (x8f7_30_jumpVelSet || x8fa_31_) {
    return;
  }
  const CVector3f& pos = GetTranslation();
  const float gravity = GetGravityConstant();
  const float peak = xaf8_jumpHeight + (pos.GetZ() < xafc_patrolDestPos.GetZ()
                                            ? xafc_patrolDestPos.GetZ()
                                            : pos.GetZ());
  const float vz = CMath::SqrtF(2.f * gravity * (peak - pos.GetZ()));
  const float rate = 1.f / (vz / gravity +
                            CMath::SqrtF(2.f * (peak - xafc_patrolDestPos.GetZ()) / gravity));
  SetVelocityWR(CVector3f(rate * (xafc_patrolDestPos.GetX() - pos.GetX()),
                          rate * (xafc_patrolDestPos.GetY() - pos.GetY()), vz));
  x8f7_30_jumpVelSet = true;
  x8fa_29_ = true;
}

void CSpacePirate::PreRender(CStateManager& mgr) {
  StandIn("CSpacePirate::PreRender (fn_72_38E4)");
  CPatterned::PreRender(mgr);
}

void CSpacePirate::Render(const CStateManager& mgr) const {
  StandIn("CSpacePirate::Render (fn_72_37C4)");
  CPatterned::Render(mgr);
}

// Retail fn_72_E3D8 adds a force-field trigger's push to the ragdoll's torso impulse once the
// ragdoll is primed; with no ragdoll (xb30_ is never set) it is CPatterned::Touch.
void CSpacePirate::Touch(CActor& actor, CStateManager& mgr) { CPatterned::Touch(actor, mgr); }

// Retail fn_72_C538.
// @ fn_72_C538
void CSpacePirate::Death(CStateManager& mgr, const CVector3f& direction,
                         EScriptObjectState state) {
  if (!mAlive) {
    return;
  }
  CPatterned::Death(mgr, direction, state);
  TraceCall(*this, "Death", static_cast< int >(mAttachedActor != kInvalidUniqueId));
  if (mAttachedActor != kInvalidUniqueId) {
    BodyController()->CommandMgr().DeliverCmd(
        CBCKnockDownCmd(GetTransform().GetForward(), pas::kS_Two));
  }
}

// Retail fn_72_C684.
// @ fn_72_C684
void CSpacePirate::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  if ((x8f4_25_ceilingAmbush && mAlive) || xb30_ != nullptr) {
    return;
  }
  mKnockBackController.EnableKnockBackPhysics(!x8f4_31_noKnockbackImpulseReset);
  mKnockBackController.EnableAnimReaction(CKnockBackMgr::kAR_KnockBack, IsOnGround());
  bool enableFreeze = true;
  if (IsIngPossessed() ||
      (x8f5_27_shadowPirate && !info.GetDamageInfo().GetWeaponMode().IsCharged() &&
       !info.GetDamageInfo().GetWeaponMode().IsComboed())) {
    enableFreeze = false;
  }
  mKnockBackController.EnableFreeze(enableFreeze);
  CPatterned::KnockBack(mgr, info);
  if (x8f5_27_shadowPirate) {
    if (mAlive) {
      const float power = info.GetDamageInfo().GetKnockBackPower(*GetDamageVulnerability(), 0.f);
      if (power >= 4.f && BodyController()->GetPercentageFrozen() != 1.f) {
        mAlphaDelta = 1.f;
        xb7c_cloakDelayTimer = CMath::Clamp(0.f, xb7c_cloakDelayTimer + 0.1f * power, 1.f);
        xb90_maxCloakAlpha = 0.5f;
        mgr.ActorModelParticles()->StartElectric(*this);
        xb80_electricParticleTimer = 1.f + xb7c_cloakDelayTimer;
      }
    } else {
      mAlphaDelta = 1.f;
      xb90_maxCloakAlpha = 1.f;
      xb8c_minCloakAlpha = 0.f;
      mgr.ActorModelParticles()->StartElectric(*this);
      xb80_electricParticleTimer = 2.f;
    }
  }
  const CKnockBackMgr::EAnimReaction reaction = mKnockBackController.GetActiveReaction();
  TraceCall(*this, mAlive ? "KnockBack alive reaction" : "KnockBack dead reaction",
            static_cast< int >(reaction));
  if (mAlive) {
    if (reaction == CKnockBackMgr::kAR_Hurled) {
      mStateMachine->SetState(mgr, *this, rstl::string_l("GetUpNow"));
      CSfxManager::AddEmitter(mData.mHurledSfx, GetTranslation(), 127,
                              GetCurrentAreaId().Value(), false, false, CSfxManager::kMedPriority);
    }
  } else if (!x8f5_30_floatingCorpse && reaction == CKnockBackMgr::kAR_Hurled) {
    const CKnockBackMgr::EFollowUp followUp = mKnockBackController.GetFollowUp();
    if (followUp != CKnockBackMgr::kFU_ExplodeDeath && followUp != CKnockBackMgr::kFU_IceDeath &&
        followUp != CKnockBackMgr::kFU_BurnDeath &&
        followUp != CKnockBackMgr::kFU_LaggedBurnDeath) {
      CSfxManager::AddEmitter(mData.mDeathSfx, GetTranslation(), 127, GetCurrentAreaId().Value(),
                              false, false, CSfxManager::kMedPriority);
    }
  }
}

namespace {

// Retail fn_72_10E50 builds these two tables once; the order is the table's. Own members are
// cast the way CAIMannedTurret does it (the base-class pointer-to-member types differ only by
// the class), which is fine for non-virtual members of a single-inheritance class.
#define SP_TRIGGER(name)                                                                           \
  { #name, reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CSpacePirate::name) }
#define SP_STATE(name)                                                                             \
  { #name, reinterpret_cast< CPatterned::StateMachine::StateFunc >(&CSpacePirate::name) }

CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    SP_TRIGGER(Stuck),           SP_TRIGGER(PatternShagged),  SP_TRIGGER(HearShot),
    SP_TRIGGER(HearPlayer),      SP_TRIGGER(AggressionCheck), SP_TRIGGER(CoverCheck),
    SP_TRIGGER(CoverFind),       SP_TRIGGER(CoverBlown),      SP_TRIGGER(CoverNearlyBlown),
    SP_TRIGGER(CoveringFire),    SP_TRIGGER(ShouldAttack),    SP_TRIGGER(LineOfSight),
    SP_TRIGGER(PatternOver),     SP_TRIGGER(SpotPlayer),      SP_TRIGGER(ShouldDodge),
    SP_TRIGGER(ShouldRetreat),   SP_TRIGGER(InRange),         SP_TRIGGER(ShouldCrouch),
    SP_TRIGGER(ShouldMove),      SP_TRIGGER(ShotAt),          SP_TRIGGER(Attacked),
    SP_TRIGGER(HasTargetingPoint), SP_TRIGGER(ShouldWallHang), SP_TRIGGER(AnimOver),
    SP_TRIGGER(ShouldStrafe),    SP_TRIGGER(ShouldSpecialAttack), SP_TRIGGER(StartAttack),
    SP_TRIGGER(BreakAttack),     SP_TRIGGER(LostInterest),    SP_TRIGGER(BounceFind),
    SP_TRIGGER(OffLine),         SP_TRIGGER(Landed),          SP_TRIGGER(ShouldJumpBack),
    SP_TRIGGER(Leash),           SP_TRIGGER(HasAttackPattern), SP_TRIGGER(IsAmbushing),
    SP_TRIGGER(ShouldWarpIn),    SP_TRIGGER(ShouldLaunchGrenade), SP_TRIGGER(InProjectileRange),
};

CPatterned::StateMachine::SStateFunction skStates[] = {
    SP_STATE(Ambushing), SP_STATE(WarpIn),      SP_STATE(WarpOut),    SP_STATE(PostWarpOut),
    SP_STATE(Attack),    SP_STATE(Crouch),      SP_STATE(CoverAttack), SP_STATE(Halt),
    SP_STATE(Run),       SP_STATE(PathFind),    SP_STATE(Patrol),     SP_STATE(TargetPatrol),
    SP_STATE(Shuffle),   SP_STATE(TurnAround),  SP_STATE(Dodge),      SP_STATE(Lurk),
    SP_STATE(Taunt),     SP_STATE(Cover),       SP_STATE(Dead),       SP_STATE(TargetCover),
    SP_STATE(TargetPlayer), SP_STATE(Approach), SP_STATE(WallHang),   SP_STATE(WallDetach),
    SP_STATE(GetUp),     SP_STATE(Generate),    SP_STATE(Skid),       SP_STATE(DoubleSnap),
    SP_STATE(JumpBack),  SP_STATE(Bounce),      SP_STATE(PathFindEx), SP_STATE(Enraged),
    SP_STATE(Jump),      SP_STATE(Deactivate),  SP_STATE(LaunchGrenade), SP_STATE(Captured),
    SP_STATE(RemoveFromWorld),
};

#undef SP_TRIGGER
#undef SP_STATE

} // namespace

// Retail fn_72_F0A0: CPatterned's tables first, then its own 39 triggers (0x27) and 37 states
// (0x25). The state bodies live in PortCSpacePirateStates.cpp, the triggers in
// PortCSpacePirateTriggers.cpp.
void CSpacePirate::SetupStateMachine(CStateManager& mgr) {
  StateMachine* stateMachine = mStateMachine.get();
  CPatterned::SetupStateMachine(mgr);
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
}

void CSpacePirate::SetAttackTarget(CStateManager& mgr, TUniqueId target) {
  StandIn("CSpacePirate::SetAttackTarget (fn_72_E120)");
  CPatterned::SetAttackTarget(mgr, target);
  mAttackTarget = target;
}

void CSpacePirate::PreRenderAllViewports(CStateManager& mgr) {
  StandIn("CSpacePirate::PreRenderAllViewports (fn_72_3A0C)");
  CPatterned::PreRenderAllViewports(mgr);
}

// @ fn_72_64
const CHealthInfo* CSpacePirate::GetHealthInfo() const {
  return const_cast< CSpacePirate* >(this)->HealthInfo(); // fn_72_64
}

const CDamageVulnerability* CSpacePirate::GetDamageVulnerability() const {
  StandIn("CSpacePirate::GetDamageVulnerability (fn_72_3AB4)");
  return CPatterned::GetDamageVulnerability();
}

const CDamageVulnerability* CSpacePirate::GetDamageVulnerability(const CVector3f& position,
                                                                 const CVector3f& direction,
                                                                 const CDamageInfo& damage) const {
  StandIn("CSpacePirate::GetDamageVulnerability(pos,dir,dmg) (fn_72_3A88)");
  return CPatterned::GetDamageVulnerability(position, direction, damage);
}

rstl::optional_object< CAABox > CSpacePirate::GetTouchBounds() const {
  StandIn("CSpacePirate::GetTouchBounds (fn_72_17C8)");
  return CPatterned::GetTouchBounds();
}

CAABox CSpacePirate::GetSortingBounds(const CStateManager& mgr) const {
  StandIn("CSpacePirate::GetSortingBounds (fn_72_35CC)");
  return CPatterned::GetSortingBounds(mgr);
}

// Retail fn_72_3B28: a jump table over the event type; every other event is CPatterned's.
// @ fn_72_3B28
void CSpacePirate::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                   EUserEventType type, float dt) {
  bool handled = false;
  TraceCall(*this, "UserAnimEvent", static_cast< int >(type));
  switch (type) {
  case kUE_Projectile:
    // TODO(fn_72_4174): retail spawns the launcher's grenade (a 0x390-byte actor) here.
    --xc58_;
    handled = true;
    break;
  case kUE_BeginAction:
    RemoveMaterial(kMT_Solid, mgr);
    mAllEnergyDrained = true;
    handled = true;
    break;
  case kUE_EndAction:
    x8f9_30_closeMelee = false;
    handled = true;
    break;
  case kUE_DeGenerate:
  case kUE_BecomeRagDoll:
    if (x8f4_29_onlyAttackInRange || GetHealthInfo()->GetHP() <= 0.f) {
      xb2c_ragdollDelayTimer = mgr.Random()->Float() * 0.05f + 0.001f;
    }
    handled = true;
    break;
  case kUE_IkLock:
    if (!xb34_ikChain.GetActive()) {
      const CAnimData& animData = *GetModelData()->GetAnimationData();
      const CSegId bone = animData.GetLocatorSegId(node.GetLocatorName());
      if (bone != CSegId::Invalid()) {
        CTransform4f xf = GetLctrTransform(bone);
        xb34_ikChain.Activate(animData, bone, xf);
        x8f9_28_satUp = true;
      }
    }
    handled = true;
    break;
  case kUE_IkRelease:
    xb34_ikChain.Deactivate();
    handled = true;
    break;
  case kUE_ScreenShake:
    SendScriptMsgs(kSS_Play, mgr, kSM_None);
    handled = true;
    break;
  case kUE_FadeOut:
    if (x8f5_27_shadowPirate) {
      mAlphaDelta = -0.8f;
      mgr.ActorModelParticles()->StartElectric(*this);
      xb80_electricParticleTimer = 1.f;
    }
    handled = true;
    break;
  case kUE_BreakLockOn:
    handled = mAlive;
    break;
  default:
    break;
  }
  if (!handled) {
    CPatterned::DoUserAnimEvent(mgr, node, type, dt);
  }
}

// Retail fn_72_E318.
bool CSpacePirate::Listen(CStateManager& mgr, const CVector3f& position, EListenNoiseType type) {
  bool heard = false;
  if (mAlive) {
    const CVector3f delta = position - GetTranslation();
    const float radius = mData.mHearingRadius;
    if (delta.MagSquared() < radius * radius &&
        (mDetectionHeightRange == 0.f ||
         delta.GetZ() * delta.GetZ() < mDetectionHeightRange * mDetectionHeightRange)) {
      heard = true;
      x8f6_25_hearNoise = true;
    }
    if (type == kLNT_PlayerFire) {
      x8f7_26_hearPlayerFire = true;
      xbb4_ = position;
    }
  }
  return heard;
}

CVector3f CSpacePirate::GetOrigin(const CStateManager& mgr, const CTeamAiRole& role,
                                  const CVector3f& aimPos) const {
  StandIn("CSpacePirate::GetOrigin (fn_72_E090)");
  return CPatterned::GetOrigin(mgr, role, aimPos);
}

// Retail fn_72_3AE8.
uchar CSpacePirate::GetModelAlphau8(const CStateManager& mgr) const {
  if (x8f5_27_shadowPirate) {
    return static_cast< uchar >(255.f * xb88_shadowPirateAlpha);
  }
  return mColor.GetAlphau8();
}

CProjectileInfo* CSpacePirate::ProjectileInfo() {
  // Retail returns this+0xbd4 whether or not the ctor built it; the host keeps it optional.
  return mProjectileInfo.valid() ? &mProjectileInfo.data() : nullptr;
}

CAABox CSpacePirate::GetScanVisorRenderBounds(const CStateManager& mgr) const {
  StandIn("CSpacePirate::GetScanVisorRenderBounds (fn_72_343C)");
  return CPatterned::GetScanVisorRenderBounds(mgr);
}

void CSpacePirate::ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                                   const CModelFlags& flags) const {
  StandIn("CSpacePirate::ScanVisorRender (fn_72_339C)");
  CPatterned::ScanVisorRender(mgr, xf, flags);
}

bool CSpacePirate::TryToBeCaptured(CStateManager& mgr) {
  StandIn("CSpacePirate::TryToBeCaptured (fn_72_18BC)");
  return CPatterned::TryToBeCaptured(mgr);
}

// Retail fn_72_140. Property ids and order are the generated SLdrSpacePirate reader's, which the
// analysis checked against the REL id by id.
static CEntity* PortLoadSpacePirate(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrSpacePirate sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrSpacePirate.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  CSpacePirateData data;
  data.mAggressiveness = sldrThis.aggressiveness;
  data.mCoverCheck = sldrThis.coverCheck;
  data.mSearchRadius = sldrThis.searchRadius;
  data.mFallBackCheck = sldrThis.fallBackCheck;
  data.mFallBackRadius = sldrThis.fallBackRadius;
  data.mHearingRadius = sldrThis.hearingRadius;
  data.mFlags = sldrThis.flags;
  data.mUnknownCe670970 = sldrThis.unknown_0xce670970;
  data.mProjectile = sldrThis.projectile;
  data.mProjectileDamage = LdrToDamageInfo(sldrThis.projectileDamage);
  data.mProjectileSfx = static_cast< ushort >(sldrThis.sound_Projectile);
  data.mBladeDamage = LdrToDamageInfo(sldrThis.bladeDamage);
  data.mKneelAttackChance = sldrThis.kneelAttackChance;
  data.mKneelAttackShot = sldrThis.kneelAttackShot;
  data.mKneelAttackDamage = LdrToDamageInfo(sldrThis.kneelAttackDamage);
  data.mDodgeCheck = sldrThis.dodgeCheck;
  data.mImpactSfx = static_cast< ushort >(sldrThis.sound_Impact);
  data.mIntraBurstTime = sldrThis.intraBurstShotTime;
  data.mIntraBurstVariation = sldrThis.intraBurstShotVariation;
  data.mAlertSfx = static_cast< ushort >(sldrThis.sound_Alert);
  data.mGunTrackDelay = sldrThis.gunTrackDelay;
  data.mFirstBurstCount = static_cast< uint >(sldrThis.unknown_0x1b454a27);
  data.mCloakOpacity = sldrThis.cloakOpacity;
  data.mMaxCloakOpacity = sldrThis.maxCloakOpacity;
  data.mBreakDodgeMinTime = sldrThis.breakDodgeMinTime;
  data.mBreakDodgeMaxTime = sldrThis.breakDodgeMaxTime;
  data.mHurledSfx = static_cast< ushort >(sldrThis.sound_Hurled);
  data.mDeathSfx = static_cast< ushort >(sldrThis.sound_Death);
  data.mAvoidDistance = sldrThis.avoidDistance;
  data.mEquippedWeapon = static_cast< uint >(sldrThis.weaponData.equippedWeapon);
  data.mGrenadeLauncher = sldrThis.weaponData.grenadeLauncher;
  data.mGrenadeMinAttackDist = sldrThis.weaponData.grenadeMinAttackDist;
  data.mGrenadeMaxAttackDist = sldrThis.weaponData.grenadeMaxAttackDist;

  return rs_new CSpacePirate(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                             LdrToEntityInfo(info, sldrThis.editorProperties),
                             LdrToTransform4f(sldrThis.editorProperties), *modelData,
                             LdrToActorParameters(sldrThis.actorInformation),
                             LdrToPatternedInfo(sldrThis.patterned, &sldrThis.ingPossessionData),
                             data);
}

static SSpacePirate_FuncPtrs REL_loader_SpacePirate;

// Retail's RELMain calls fn_72_D4, which fills the table the same way and registers it with the
// DOL's SetSSpacePirate_FuncPtrs. The PMFs point at the Port* bodies: AttachActorToPirate and
// DetachActorFromPirate are the proxies that call through this very table.
void RELMain() {
  REL_loader_SpacePirate.mLoadSpacePirate = PortLoadSpacePirate;
  REL_loader_SpacePirate.mAttachActor = &CSpacePirate::PortAttachActor;
  REL_loader_SpacePirate.mDetachActor = &CSpacePirate::PortDetachActor;
  SetSSpacePirate_FuncPtrs(&REL_loader_SpacePirate);
}

void RELExit() { SetSSpacePirate_FuncPtrs(nullptr); }
