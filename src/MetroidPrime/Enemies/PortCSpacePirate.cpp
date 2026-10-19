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
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
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
  xa10_unkTimer = 0.f;
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
  // TODO(fn_72_10568): 0xb34 is built by a helper taking (id, &id, rodata_B9C, -1, zero, 0, z, 3.0);
  // unidentified (CIkChain-sized), so it stays zeroed.
  memset(xb34_ikChain_, 0, sizeof(xb34_ikChain_));
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

void CSpacePirate::PreThink(float dt, CStateManager& mgr) {
  StandIn("CSpacePirate::PreThink (fn_72_24C4)");
  EnsureBodyActive(mgr);
  CPatterned::PreThink(dt, mgr);
}

void CSpacePirate::Think(float dt, CStateManager& mgr) {
  StandIn("CSpacePirate::Think (fn_72_1980)");
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
}

void CSpacePirate::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("CSpacePirate::AcceptScriptMsg (fn_72_E498)");
  CPatterned::AcceptScriptMsg(mgr, msg);
  if (msg.GetMessage() == kSM_Create) {
    EnsureBodyActive(mgr);
  }
}

void CSpacePirate::PreRender(CStateManager& mgr) {
  StandIn("CSpacePirate::PreRender (fn_72_38E4)");
  CPatterned::PreRender(mgr);
}

void CSpacePirate::Render(const CStateManager& mgr) const {
  StandIn("CSpacePirate::Render (fn_72_37C4)");
  CPatterned::Render(mgr);
}

void CSpacePirate::Touch(CActor& actor, CStateManager& mgr) {
  StandIn("CSpacePirate::Touch (fn_72_E3D8)");
  EnsureBodyActive(mgr);
  CPatterned::Touch(actor, mgr);
}

void CSpacePirate::Death(CStateManager& mgr, const CVector3f& direction,
                         EScriptObjectState state) {
  StandIn("CSpacePirate::Death (fn_72_C538)");
  EnsureBodyActive(mgr);
  CPatterned::Death(mgr, direction, state);
}

void CSpacePirate::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  StandIn("CSpacePirate::KnockBack (fn_72_C684)");
  EnsureBodyActive(mgr);
  CPatterned::KnockBack(mgr, info);
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

const CHealthInfo* CSpacePirate::GetHealthInfo() const {
  StandIn("CSpacePirate::GetHealthInfo (fn_72_64)");
  return CPatterned::GetHealthInfo();
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

void CSpacePirate::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                   EUserEventType type, float dt) {
  StandIn("CSpacePirate::DoUserAnimEvent (fn_72_3B28)");
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

bool CSpacePirate::Listen(CStateManager& mgr, const CVector3f& position, EListenNoiseType type) {
  StandIn("CSpacePirate::Listen (fn_72_E318)");
  return CPatterned::Listen(mgr, position, type);
}

CVector3f CSpacePirate::GetOrigin(const CStateManager& mgr, const CTeamAiRole& role,
                                  const CVector3f& aimPos) const {
  StandIn("CSpacePirate::GetOrigin (fn_72_E090)");
  return CPatterned::GetOrigin(mgr, role, aimPos);
}

uchar CSpacePirate::GetModelAlphau8(const CStateManager& mgr) const {
  StandIn("CSpacePirate::GetModelAlphau8 (fn_72_3AE8)");
  return CPatterned::GetModelAlphau8(mgr);
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
