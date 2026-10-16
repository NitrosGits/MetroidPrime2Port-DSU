// Host body for the MinorIng REL (module 44), milestone 1: the actors load, stand and render.
// The analysis is docs/module-analysis/IngFamily.md section B. Every vtable override below
// forwards to CPatterned behind a once-per-method StandIn log; the analysis maps the loader
// (fn_44_110), the data constructor (fn_44_9B4) and the actor constructor (fn_44_9D58) but not
// the individual vtable slots, so each stand-in names its own method only. There is no combat AI
// yet: CPatterned's default state machine drives the Ing.
#include "MetroidPrime/Enemies/CMinorIng.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrMinorIng.hpp"
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
  fprintf(stderr, "[minoring] %s is a port stand-in (see PortCMinorIng.cpp)\n", what);
}

} // namespace

CMinorIng::CMinorIng(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                     const CTransform4f& xf, const CModelData& modelData,
                     const CPatternedInfo& pInfo, const CActorParameters& actParms,
                     const CMinorIngData& data)
// EPatternedAI 0x4b, flavor 0, movement 0 (ground), collider 1, body 1 (bipedal): the retail
// constructor's arguments to CPatterned (fn_44_9D58).
: CPatterned(static_cast< EPatternedAI >(0x4b), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mData(data)
, mProjectileInfo(data.mProjectile, data.mProjectileDamage)
, mPathFindSearch(nullptr, 0x301, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0, CPFRegion::kRP_Center)
, mPointPathFindSearch(nullptr)
, mSurfaceAlignment()
, mIngSpotPathFindNavigation() {
  // The retail constructor turns knock-back physics off and widens the knock-back animation
  // reaction to (2, 2), i.e. kAR_KnockBack both ways, on CPatterned's own manager at 0x620.
  KnockBackController().EnableKnockBackPhysics(false);
  KnockBackController().SetAnimReactionRange(CKnockBackMgr::kAR_KnockBack,
                                             CKnockBackMgr::kAR_KnockBack);
}

CMinorIng::~CMinorIng() {}

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove / Death / KnockBack all dereference the current state. The analysis does not say
// where retail activates it, so the port activates on kSM_Create and, as the other patterned
// actors in this port do, again on demand in every entry point that can run first.
void CMinorIng::EnsureBodyActive(CStateManager& mgr) {
  if (BodyController() != nullptr && !BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

void CMinorIng::PreThink(float dt, CStateManager& mgr) {
  StandIn("CMinorIng::PreThink");
  EnsureBodyActive(mgr);
  CPatterned::PreThink(dt, mgr);
}

void CMinorIng::Think(float dt, CStateManager& mgr) {
  StandIn("CMinorIng::Think");
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
}

void CMinorIng::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("CMinorIng::AcceptScriptMsg");
  CPatterned::AcceptScriptMsg(mgr, msg);
  if (msg.GetMessage() == kSM_Create) {
    EnsureBodyActive(mgr);
  }
}

void CMinorIng::PreRender(CStateManager& mgr) {
  StandIn("CMinorIng::PreRender");
  CPatterned::PreRender(mgr);
}

void CMinorIng::Render(const CStateManager& mgr) const {
  StandIn("CMinorIng::Render");
  CPatterned::Render(mgr);
}

void CMinorIng::Touch(CActor& actor, CStateManager& mgr) {
  StandIn("CMinorIng::Touch");
  EnsureBodyActive(mgr);
  CPatterned::Touch(actor, mgr);
}

void CMinorIng::Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) {
  StandIn("CMinorIng::Death");
  EnsureBodyActive(mgr);
  CPatterned::Death(mgr, direction, state);
}

void CMinorIng::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  StandIn("CMinorIng::KnockBack");
  EnsureBodyActive(mgr);
  CPatterned::KnockBack(mgr, info);
}

void CMinorIng::SetupStateMachine(CStateManager& mgr) {
  // Retail (fn_44_ABCC) installs its own 34 triggers and 20 states or code functions (an Idle, a
  // Patrol, a PathFind, a ProjectileAttack and a Dead among them); the base class's tables are
  // what runs here.
  StandIn("CMinorIng::SetupStateMachine (fn_44_ABCC)");
  CPatterned::SetupStateMachine(mgr);
}

// Retail fn_44_110. The property ids and their order are the generated SLdrMinorIng reader's,
// which the analysis checked against the REL id by id: it reads all 20.
static CEntity* PortLoadMinorIng(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrMinorIng sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrMinorIng.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    return nullptr;
  }

  const CIngSpotData ingSpot(sldrThis.ingSpot.blobEffect, sldrThis.ingSpot.hitNormalDamage,
                             sldrThis.ingSpot.hitHeavyDamage, sldrThis.ingSpot.death,
                             sldrThis.ingSpot.maxSpeed, sldrThis.ingSpot.maxWallSpeed,
                             sldrThis.ingSpot.ballPursuitSpeed, sldrThis.ingSpot.speedModifier,
                             sldrThis.ingSpot.turnSpeed,
                             LdrToDamageVulnerability(sldrThis.ingSpot.vulnerability),
                             static_cast< ushort >(sldrThis.ingSpot.sound_Idle),
                             static_cast< ushort >(sldrThis.ingSpot.sound_Move),
                             static_cast< ushort >(sldrThis.ingSpot.sound_HitNormalDamage),
                             static_cast< ushort >(sldrThis.ingSpot.sound_HitHeavyDamage),
                             static_cast< ushort >(sldrThis.ingSpot.sound_Death));

  // The three flag bytes: the REL's constructor takes them in an order that does not follow the
  // properties' declaration order, and the analysis does not pair the two, so the bits here follow
  // SLdrMinorIng's own field order. Nothing reads them in this milestone.
  uchar flags156 = 0;
  if (sldrThis.allowProjectileDuringAttackPattern) {
    flags156 |= 0x80;
  }
  if (sldrThis.unknown_0xbce16644) {
    flags156 |= 0x40;
  }
  if (sldrThis.unknown_0x142433d3) {
    flags156 |= 0x20;
  }
  if (sldrThis.stayOnPointPathFinding) {
    flags156 |= 0x10;
  }
  if (sldrThis.allowProjectileDuringStayOnPointPathFinding) {
    flags156 |= 0x08;
  }
  if (sldrThis.allowPuddleLockOn) {
    flags156 |= 0x04;
  }

  const CMinorIngData data(
      sldrThis.projectile, LdrToDamageInfo(sldrThis.projectileDamage),
      LdrToDamageInfo(sldrThis.ingSpot.damage), ingSpot, sldrThis.unknown_0xa03e450c,
      sldrThis.ingSpot.morphballPursuitDistance, sldrThis.ingSpot.bombStunDuration,
      sldrThis.ingSpot.unknown_0x7569fdba, sldrThis.ingSpot.sfxFallOff,
      sldrThis.attackAngleLimit, sldrThis.lineOfSightHeightOffset, sldrThis.unknown_0xd6c8eac2,
      sldrThis.unknown_0x2a5449ba, sldrThis.hearingRadius, sldrThis.ingSpot.speedModifier,
      sldrThis.unknown_0x3da35851.allowLockOn,
      static_cast< uchar >(sldrThis.unknown_0x09207f51), flags156);

  return rs_new CMinorIng(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                          LdrToEntityInfo(info, sldrThis.editorProperties),
                          LdrToTransform4f(sldrThis.editorProperties), *modelData,
                          LdrToPatternedInfo(sldrThis.patterned, nullptr),
                          LdrToActorParameters(sldrThis.actorInformation), data);
}

static SMinorIng_FuncPtrs REL_loader_MinorIng;

// Retail's RELMain stores fn_44_110 into the module's table and registers it with the DOL's
// SetSMinorIng_FuncPtrs (via fn_44_E0). The proxy in SMinorIng_FuncPtrs.cpp calls through it.
void RELMain() {
  REL_loader_MinorIng.mLoader = PortLoadMinorIng;
  SetSMinorIng_FuncPtrs(&REL_loader_MinorIng);
}

void RELExit() { SetSMinorIng_FuncPtrs(nullptr); }
