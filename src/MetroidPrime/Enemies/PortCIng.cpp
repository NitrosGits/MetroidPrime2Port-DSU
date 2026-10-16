// Host body for the Ing REL (module 29), milestone 3 of docs/module-analysis/IngFamily.md section D:
// the actors load, stand and render. Every vtable override below forwards to CPatterned behind a
// once-per-method StandIn log; the REL function it stands in for is named on each. There is no
// combat AI yet - CPatterned's default state machine drives the Ing - and the state machine
// (fn_29_F2C8), the form changes, the attacks and the mini-portal/exit-host classes are not part of
// this milestone.
//
// Which methods CIng overrides is not guessed: the REL's vtable lbl_29_data_A24 lists them by
// address, and its slots line up with CPatterned's own declaration order (the entries the REL
// inherits are spelled out by name in the same table, e.g. slot 28 DoUserAnimEvent between slot 27
// GetSortingBounds and slot 29 GetScannableObjectInfo). The tiny accessors at the head of the
// module's .text are decided by their four-to-twelve-instruction bodies, so they are real, not
// stand-ins.
#include "MetroidPrime/Enemies/CIng.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrIng.hpp"
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
  fprintf(stderr, "[ing] %s is a port stand-in (see PortCIng.cpp)\n", what);
}

} // namespace

CIng::CIng(TUniqueId uid, const rstl::string& name, const CEntityInfo& info, const CTransform4f& xf,
           const CModelData& modelData, const CActorParameters& actParms,
           const CPatternedInfo& pInfo, const CIngData& data)
// EPatternedAI 0x18, flavor 0, movement 0 (ground), collider 1, body 1 (bipedal): the retail
// constructor's arguments to CPatterned (fn_29_D540, which passes r4 = 0x18 and 0/1/1 on the stack).
: CPatterned(static_cast< EPatternedAI >(0x18), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mData(data)
// fn_29_D540 @0x0000D5F4: (null area, flags 0x301, the patterned info's pathfinding index at
// +0xec, 1.0f, 1.0f, no avoidance filter, kRP_Center). rodata_64, which the ctor loads for both
// radii, is 1.0f.
, mPathFindSearch(nullptr, 0x301, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0, CPFRegion::kRP_Center)
, mPointPathFindSearch(nullptr) // fn_29_D540 @0x0000D600, the DOL's fn_801F8818
, mSurfaceAlignment()           // fn_29_D540 @0x0000D610, the DOL's fn_801F7CA0
, mIngSpotPathFindNavigation()  // fn_29_D540 @0x0000D618, the DOL's fn_801F8258
{
  // The retail constructor turns knock-back physics off on CPatterned's own manager (@0x0000E3DC)
  // and stops the actor drawing a shadow (@0x0000E634). Everything else it builds - the CVisual
  // Knowledge, the locator segment ids and the six locked tokens - belongs to the state machine and
  // the render paths this milestone does not enter.
  KnockBackController().EnableKnockBackPhysics(false);
  SetDrawShadow(false);
}

CIng::~CIng() {}

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove / Death / KnockBack all dereference the current state. The analysis does not say
// where retail activates it, so the port activates on kSM_Create and, as the other patterned actors
// in this port do, again on demand in every entry point that can run first.
void CIng::EnsureBodyActive(CStateManager& mgr) {
  if (BodyController() != nullptr && !BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

// vtable slot 4 = fn_29_C814 (0x190). The analysis maps neither slot to a method; the slot order
// puts fn_29_C814 at PreThink, fn_29_C654 at Think and fn_29_C9A4 at AcceptScriptMsg, which matches
// their sizes (0x190 / 0x1C0 / 0x514, the last the largest).
void CIng::PreThink(float dt, CStateManager& mgr) {
  StandIn("CIng::PreThink (vtable slot 4, fn_29_C814)");
  EnsureBodyActive(mgr);
  CPatterned::PreThink(dt, mgr);
}

void CIng::Think(float dt, CStateManager& mgr) {
  StandIn("CIng::Think (vtable slot 5, fn_29_C654)");
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
}

void CIng::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("CIng::AcceptScriptMsg (vtable slot 6, fn_29_C9A4)");
  CPatterned::AcceptScriptMsg(mgr, msg);
  if (msg.GetMessage() == kSM_Create) {
    EnsureBodyActive(mgr);
  }
}

void CIng::PreRender(CStateManager& mgr) {
  StandIn("CIng::PreRender (vtable slot 9, fn_29_C010)");
  CPatterned::PreRender(mgr);
}

void CIng::Render(const CStateManager& mgr) const {
  StandIn("CIng::Render (vtable slot 11, fn_29_C0B4)");
  CPatterned::Render(mgr);
}

void CIng::Touch(CActor& actor, CStateManager& mgr) {
  StandIn("CIng::Touch (vtable slot 19, fn_29_BB8C)");
  EnsureBodyActive(mgr);
  CPatterned::Touch(actor, mgr);
}

void CIng::Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) {
  StandIn("CIng::Death (vtable slot 38, fn_29_B3C4)");
  EnsureBodyActive(mgr);
  CPatterned::Death(mgr, direction, state);
}

void CIng::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  StandIn("CIng::KnockBack (vtable slot 39, fn_29_B8C0)");
  EnsureBodyActive(mgr);
  CPatterned::KnockBack(mgr, info);
}

void CIng::TakeDamage(const CVector3f& direction, float magnitude) {
  StandIn("CIng::TakeDamage (vtable slot 41, fn_29_B890)");
  CPatterned::TakeDamage(direction, magnitude);
}

bool CIng::Listen(CStateManager& mgr, const CVector3f& position, EListenNoiseType type) {
  StandIn("CIng::Listen (vtable slot 44, fn_29_B294)");
  return CPatterned::Listen(mgr, position, type);
}

void CIng::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type,
                           float dt) {
  // Retail fn_29_C228 (0x3B8) switches on the user-event type through the jump table
  // jumptable_29_data_B6C and spawns a CIngMiniPortalAttack for one of them, which is milestone 4.
  StandIn("CIng::DoUserAnimEvent (vtable slot 28, fn_29_C228)");
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

void CIng::SetupStateMachine(CStateManager& mgr) {
  // Retail (fn_29_B208) installs its own tables, which fn_29_F2C8 fills in: 49 triggers, 29 states
  // and 12 code functions. The base class's tables are what runs here.
  StandIn("CIng::SetupStateMachine (fn_29_B208)");
  CPatterned::SetupStateMachine(mgr);
}

// Retail fn_29_130. The property ids and their order are the generated SLdrIng reader's, which the
// analysis checked against the REL id by id: it reads all 69. facePlateModel (0xbdcce71b) is
// parsed and then never read, here as in retail.
static CEntity* PortLoadIng(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrIng sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrIng.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL checks the optional's valid byte (sp+0x3f4) and returns null when the model is empty.
    return nullptr;
  }

  // The five sub-structs CIngData embeds, in the order fn_29_130 builds them: fn_29_F12C (the
  // body-projectile block), the CIngSpotData constructor, fn_29_F090 (grapple), fn_29_F000
  // (mini-portal) and fn_29_EFC0 (swarm / exit-host).
  const CIngBodyProjectileData bodyProjectile(
      LdrToDamageInfo(sldrThis.bodyProjectileContactDamage), sldrThis.unknown_0xa0d63374,
      sldrThis.bodyProjectileSuckTime, sldrThis.bodyProjectileSpeed,
      sldrThis.bodyProjectileDropTime, sldrThis.bodyProjectileMinAttackDist,
      sldrThis.bodyProjectileMaxAttackDist, sldrThis.bodyProjectileOdds,
      sldrThis.bodyProjectileSplatEffect, static_cast< ushort >(sldrThis.sound_BodyProjectile),
      static_cast< ushort >(sldrThis.sound_BodyProjectileSplatWall));

  const CIngSpotData ingSpot(sldrThis.ingSpotBlobEffect, sldrThis.ingSpotHitNormalDamage,
                             sldrThis.ingSpotHitHeavyDamage, sldrThis.ingSpotDeath,
                             sldrThis.ingSpotMaxSpeed, sldrThis.ingSpotMaxWallSpeed,
                             sldrThis.ingSpotBallPursuitSpeed, sldrThis.unknown_0x50398a06,
                             sldrThis.ingSpotTurnSpeed,
                             LdrToDamageVulnerability(sldrThis.ingSpotVulnerability),
                             static_cast< ushort >(sldrThis.sound_IngSpotIdle),
                             static_cast< ushort >(sldrThis.sound_IngSpotMove),
                             static_cast< ushort >(sldrThis.sound_HitNormalDamage),
                             static_cast< ushort >(sldrThis.sound_HitHeavyDamage),
                             static_cast< ushort >(sldrThis.sound_IngSpotDeath));

  const CIngGrappleData grapple(sldrThis.unknown_0x67f6c10e,
                                LdrToDamageInfo(sldrThis.exitGrappleDamage),
                                sldrThis.exitGrappleSpitForce, sldrThis.maxGrappleBallHoldTime,
                                sldrThis.postGrappleBallWaitTime, sldrThis.grappleBallPursuitRange,
                                LdrToDamageVulnerability(sldrThis.grappleBallVulnerability),
                                static_cast< ushort >(sldrThis.sound_ExitGrapple),
                                static_cast< ushort >(sldrThis.sound_Grapple));

  const CIngMiniPortalData miniPortal(sldrThis.miniPortalMinAttackDist,
                                      sldrThis.miniPortalMaxAttackDist, sldrThis.miniPortalEffect,
                                      static_cast< ushort >(sldrThis.sound_MiniPortal),
                                      LdrToDamageInfo(sldrThis.miniPortalProjectileDamage),
                                      sldrThis.miniPortalBeamInfo);

  const CIngSwarmData swarm(
      sldrThis.pART, sldrThis.sRSC, sldrThis.pART_0x3da219c7, sldrThis.unknown_0x23271976,
      sldrThis.pART_0x081e9e6c, sldrThis.unknown_0xcb39eccb, sldrThis.unknown_0x587ca175,
      sldrThis.unknown_0x0bd7d5a9, static_cast< ushort >(sldrThis.sound_SwarmMove),
      static_cast< ushort >(sldrThis.sound_ExitHost),
      static_cast< ushort >(sldrThis.sound_ExitHostSafeZone),
      static_cast< ushort >(sldrThis.sound_InsideHost), static_cast< ushort >(sldrThis.sound));

  // fn_29_EE68's own argument order: (ingFlags, hearing, armSwipe, triggerVulnerability,
  // lightColor, the five sub-structs, 0x5d0d, 0xc620, frustration, taunt, aggressiveness,
  // lightAttenuation last).
  const CIngData data(sldrThis.ingFlagsIng, sldrThis.hearingRadius,
                      LdrToDamageInfo(sldrThis.armSwipeDamage),
                      LdrToDamageVulnerability(sldrThis.triggerVulnerability), sldrThis.lightColor,
                      bodyProjectile, ingSpot, grapple, miniPortal, swarm,
                      sldrThis.unknown_0x5d0d2c40, sldrThis.unknown_0xc620183a,
                      sldrThis.frustrationTime, sldrThis.tauntChance, sldrThis.aggressiveness,
                      sldrThis.lightAttenuation);

  return rs_new CIng(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                     LdrToEntityInfo(info, sldrThis.editorProperties),
                     LdrToTransform4f(sldrThis.editorProperties), *modelData,
                     LdrToActorParameters(sldrThis.actorInformation),
                     LdrToPatternedInfo(sldrThis.patterned, nullptr), data);
}

static SIng_FuncPtrs REL_loader_Ing;

// Retail's RELMain calls fn_29_100, which stores the loader fn_29_130 into lbl_29_bss_6C (the
// module's SIng_FuncPtrs) and calls the DOL's SetSIng_FuncPtrs. The proxy in SIng_FuncPtrs.cpp
// calls through it.
void RELMain() {
  REL_loader_Ing.mLoader = PortLoadIng;
  SetSIng_FuncPtrs(&REL_loader_Ing);
}

void RELExit() { SetSIng_FuncPtrs(nullptr); }
