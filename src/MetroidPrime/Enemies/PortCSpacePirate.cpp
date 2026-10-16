// Host body for the SpacePirate REL (module 72), milestone 1a: the actors load, stand and render.
// The analysis is docs/module-analysis/SpacePirate.md. Every vtable override below forwards to
// CPatterned behind a once-per-method StandIn log; the retail function it stands in for is named
// on each. There is no combat AI yet - CPatterned's default state machine drives the pirate.
#include "MetroidPrime/Enemies/CSpacePirate.hpp"

#include "Kyoto/Math/CMath.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CKnockBackInfo.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSpacePirate.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

#include <stdio.h>

namespace {

// Reports a stand-in the first time it runs, so a log lists what is still a shell.
void StandIn(const char* what) {
  static const char* sSeen[32] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == what) {
      return;
    }
  }
  if (sCount < 32) {
    sSeen[sCount++] = what;
  }
  fprintf(stderr, "[spacepirate] %s is a port stand-in (see PortCSpacePirate.cpp)\n", what);
}

} // namespace

CSpacePirate::CSpacePirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                           const CTransform4f& xf, const CModelData& modelData,
                           const CActorParameters& actParms, const CPatternedInfo& pInfo,
                           const CSpacePirateData& data)
// EPatternedAI 0x35, flavor 0, movement 0 (ground), collider 1, body 1 (bipedal): the retail
// constructor's arguments (fn_72_F49C).
: CPatterned(static_cast< EPatternedAI >(0x35), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mData(data)
, mHeadSegId(CSegId::Invalid()) // retail resolves "Head_1" here
, mBoneTracking(*GetModelData()->GetAnimationData(), rstl::string_l("Head_1"),
                CMath::Deg2Rad(80.f), CMath::Deg2Rad(180.f), kBTF_None)
, mEnableAim(false)
, mAllEnergyDrained(false)
, mAttachedActor(kInvalidUniqueId)
, mAttackTarget(kInvalidUniqueId)
, mAttackRemTime(1.f)
, xb30_(nullptr)
, mPortalPlane(0.f, CUnitVector3f(0.f, 0.f, 1.f)) {}

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

void CSpacePirate::SetupStateMachine(CStateManager& mgr) {
  // Retail (fn_72_F0A0) installs its own 39 triggers and 37 states; the base class's tables
  // (Patrol, Halt, PatternOver, AnimOver, ...) are what runs here.
  StandIn("CSpacePirate::SetupStateMachine (fn_72_F0A0)");
  CPatterned::SetupStateMachine(mgr);
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
