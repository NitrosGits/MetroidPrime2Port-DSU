// Host body for the IngBoostBallGuardian REL (module 30), milestone 1: the actor loads, stands
// and renders. Every vtable override that belongs to retail's AI forwards to CPatterned behind a
// once-only StandIn log, and the REL function it stands in for is named on each. The state machine
// (fn_30_C6CC installs the 49 triggers, 28 states and 9 codes), the boost ball, the puddle form,
// the mini portals and the second class (a CPhysicsActor spawned by fn_30_3478) are not written
// yet - docs/module-analysis/BoostGuardian.md section i has the plan.
//
// Which methods the REL overrides is not guessed: its vtable lbl_30_data_9C0 lists them by address
// (/tmp/opencode/ibbg_vtable.md) and the slots line up with CPatterned's declaration order, the
// inherited entries being spelled out by name in the same table. The tiny accessors at the head of
// the module's .text (fn_30_0, fn_30_8, fn_30_10, fn_30_34, fn_30_3C, fn_30_58, fn_30_6C) have
// four-to-twelve-instruction bodies, so they are real rather than stand-ins.
#include "MetroidPrime/Enemies/CIngBoostBallGuardian.hpp"

#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrIngBoostBallGuardian.hpp"
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
  fprintf(stderr, "[ibbg] %s is a port stand-in (see PortCIngBoostBallGuardian.cpp)\n", what);
}

} // namespace

// fn_30_EC6C. The base call is the retail one: EPatternedAI 0x19 (`li r4, 0x19` at 0x0000ECC0),
// flavor 0 (`li r7, 0x0` at 0x0000ECAC), movement 0 / collider 1 / body 1 (the constants stored to
// the stack arguments at 0x0000ECBC..0x0000ECC8). The path searches are fn_30_EC6C's own calls:
// CPFArea null, flags 0x301, the patterned info's pathfinding index (+0xEC), radii 1.0f (rodata_64)
// and region 0 (0x0000ED14..0x0000ED24).
CIngBoostBallGuardian::CIngBoostBallGuardian(TUniqueId uid, const rstl::string& name,
                                             const CEntityInfo& info, const CTransform4f& xf,
                                             const CModelData& modelData,
                                             const CActorParameters& actParms,
                                             const CPatternedInfo& pInfo,
                                             const CIngBoostBallGuardianData& data,
                                             const CBoostBallGuardianData& boost)
: CPatterned(static_cast< EPatternedAI >(0x19), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mData(data)      // guest 0x7C0, copied by fn_30_1056C
, mGuestAE4(2)     // 0x0000ED00
, mGuestAE8(-1)    // 0x0000ED10
, mPathFindSearch(nullptr, 0x301, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0,
                  CPFRegion::kRP_Center) // 0x0000ED24
, mPointPathFindSearch(nullptr)          // 0x0000ED30, the DOL's CPathFindPointSearch ctor
, mCollisionActorManager0(nullptr)       // 0x0000ED3C
, mCollisionActorManager1(nullptr)       // 0x0000ED40
, mSurfaceAlignment()                    // 0x0000ED44
, mIngSpotPathFindNavigation()           // 0x0000ED4C
, mBoost(boost)                          // guest 0xDEC, copied by fn_30_10250
, mVulnerability(mBoost.mVulnerability)  // 0x0000FD48 builds a CWeaponTypeVulnerability-derived
                                         // CDamageVulnerability; its source is still open, this
                                         // uses the boost ball's until the asm says otherwise
{
  // The retail constructor turns knock-back physics off on CPatterned's own manager
  // (@0x0000FF40) and leaves the shadow off; everything else it builds - the locator segment ids
  // at 0x10AC, the nine locked tokens at 0x0FDC and the line-of-sight tracker at 0x10B8 - belongs
  // to the state machine and the render paths this milestone does not enter.
  KnockBackController().EnableKnockBackPhysics(false);
  SetDrawShadow(false);
}

CIngBoostBallGuardian::~CIngBoostBallGuardian() {}

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove / Death / KnockBack all dereference the current state. The analysis does not say
// where retail activates it, so the port activates on kSM_Create and, as the other patterned
// actors in this port do, again on demand in every entry point that can run first.
void CIngBoostBallGuardian::EnsureBodyActive(CStateManager& mgr) {
  if (BodyController() != nullptr && !BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

// vtable slot 2 = fn_30_DC10 (0x190).
void CIngBoostBallGuardian::PreThink(float dt, CStateManager& mgr) {
  StandIn("CIngBoostBallGuardian::PreThink (vtable slot 2, fn_30_DC10)");
  EnsureBodyActive(mgr);
  CPatterned::PreThink(dt, mgr);
}

// vtable slot 3 = fn_30_D94C (0x2c4): surfaces alignment, the collision actors, the line-of-sight
// tracker and the actor behind the current attack target.
void CIngBoostBallGuardian::Think(float dt, CStateManager& mgr) {
  StandIn("CIngBoostBallGuardian::Think (vtable slot 3, fn_30_D94C)");
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
}

// vtable slot 4 = fn_30_DDA0 (0x690): the message switch that activates the body, spawns the
// collision actors, sends SetBossParams and deletes the actor when the boss dies.
void CIngBoostBallGuardian::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("CIngBoostBallGuardian::AcceptScriptMsg (vtable slot 4, fn_30_DDA0)");
  CPatterned::AcceptScriptMsg(mgr, msg);
  if (msg.GetMessage() == kSM_Create) {
    EnsureBodyActive(mgr);
  }
}

// vtable slot 7 = fn_30_D2E0 (0xa4), gated on the actor state; slot 8 = fn_30_D250, slot 9 =
// fn_30_D384 (the render override that sets up the world-space portal plane).
void CIngBoostBallGuardian::PreRender(CStateManager& mgr) {
  StandIn("CIngBoostBallGuardian::PreRender (vtable slot 7, fn_30_D2E0)");
  CPatterned::PreRender(mgr);
}

void CIngBoostBallGuardian::Render(const CStateManager& mgr) const {
  StandIn("CIngBoostBallGuardian::Render (vtable slot 9, fn_30_D384)");
  CPatterned::Render(mgr);
}

// vtable slot 31 = fn_30_C850 (0x1c8): what the boost ball hits.
void CIngBoostBallGuardian::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                                         CStateManager& mgr) {
  StandIn("CIngBoostBallGuardian::CollidedWith (vtable slot 31, fn_30_C850)");
  CPatterned::CollidedWith(id, list, mgr);
}

// vtable slot 58 = fn_30_C6CC (0x8c): CPatterned's own setup, then the three name tables are
// handed to the machine - lbl_30_data_250 (49 triggers), lbl_30_data_6B0 (28 states) and
// lbl_30_data_8DC (9 codes). Those are the module's FSM; it is milestone 2, so only the base
// setup runs here.
void CIngBoostBallGuardian::SetupStateMachine(CStateManager& mgr) {
  StandIn("CIngBoostBallGuardian::SetupStateMachine (vtable slot 58, fn_30_C6CC)");
  CPatterned::SetupStateMachine(mgr);
}

// vtable slot 17 = fn_30_CE50 (0x174): the contact-damage half of Touch, applied to the player.
void CIngBoostBallGuardian::Touch(CActor& actor, CStateManager& mgr) {
  StandIn("CIngBoostBallGuardian::Touch (vtable slot 17, fn_30_CE50)");
  CPatterned::Touch(actor, mgr);
}

void CIngBoostBallGuardian::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                            EUserEventType type, float dt) {
  StandIn("CIngBoostBallGuardian::DoUserAnimEvent (fn_30_D4F8's tail call)");
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

// vtable slot 36 = fn_30_76F4 (0x34): the retail body forces the attack state to 2 when it is 7
// and then calls CPatterned::Death.
void CIngBoostBallGuardian::Death(CStateManager& mgr, const CVector3f& direction,
                                  EScriptObjectState state) {
  StandIn("CIngBoostBallGuardian::Death (vtable slot 36, fn_30_76F4)");
  CPatterned::Death(mgr, direction, state);
}

// vtable slot 37 = fn_30_CB84 (0xb0): enables the knock-back reaction and forwards.
void CIngBoostBallGuardian::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  StandIn("CIngBoostBallGuardian::KnockBack (vtable slot 37, fn_30_CB84)");
  CPatterned::KnockBack(mgr, info);
}

// vtable slot 67 = fn_30_3C: stores kInvalidUniqueId, i.e. the actor never reports an attack
// target through this accessor.
TUniqueId CIngBoostBallGuardian::GetAttackTarget() const { return kInvalidUniqueId; }

CEntity* CIngBoostBallGuardian::TypesMatch(int typeId) const {
  return typeId == kET_IngBoostBallGuardian ? const_cast< CIngBoostBallGuardian* >(this) : nullptr;
}

// ---------------------------------------------------------------------------------------------
// The module's loader (fn_30_130) and entry points.

static CEntity* PortLoadIngBoostBallGuardian(CStateManager& mgr, CInputStream& input,
                                             CEntityInfo& info) {
  SLdrIngBoostBallGuardian sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrIngBoostBallGuardian.inc"

  rstl::optional_object< CModelData > modelData(LdrToModelData(
      sldrThis.editorProperties.transform.scale, kInvalidAssetId,
      sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL checks the copy's valid byte (sp+0x36c at 0x000002A4) and returns null when the
    // model is empty.
    return nullptr;
  }

  // The Ing half first (loader fn_30_10C24 builds it into sp+0x8bc), then the boost half
  // (fn_30_1988 into sp+0x41c).
  const SLdrIngBoostBallGuardianData& ing = sldrThis.guardianProperties;
  const SLdrBoostBallGuardianData& boostSldr = sldrThis.boostBallGuardian;

  const CIngSpotData ingSpot(
      ing.ingSpotBlobEffect, ing.ingSpotHitNormalDamage, ing.ingSpotHitHeavyDamage, ing.ingSpotDeath,
      ing.ingSpotMaxSpeed, ing.ingSpotMaxWallSpeed, ing.ingSpotBallPursuitSpeed,
      ing.unknown_0x50398a06, ing.ingSpotTurnSpeed,
      LdrToDamageVulnerability(ing.ingSpotVulnerability),
      static_cast< ushort >(ing.sound_IngSpotIdle),
      static_cast< ushort >(ing.sound_IngSpotMove),
      static_cast< ushort >(ing.sound_HitNormalDamage),
      static_cast< ushort >(ing.sound_HitHeavyDamage),
      static_cast< ushort >(ing.sound_IngSpotDeath));

  const CIngBodyProjectileData bodyProjectile(
      LdrToDamageInfo(ing.bodyProjectileContactDamage), ing.unknown_0xa0d63374,
      ing.bodyProjectileSuckTime, ing.bodyProjectileSpeed, ing.bodyProjectileDropTime,
      ing.bodyProjectileMinAttackDist, ing.bodyProjectileMaxAttackDist, ing.bodyProjectileOdds,
      ing.bodyProjectileSplatEffect, static_cast< ushort >(ing.sound_BodyProjectile),
      static_cast< ushort >(ing.sound_BodyProjectileSplatWall));

  const CIngMiniPortalData miniPortal(ing.miniPortalMinAttackDist, ing.miniPortalMaxAttackDist,
                                      ing.miniPortalEffect,
                                      static_cast< ushort >(ing.sound_MiniPortal),
                                      LdrToDamageInfo(ing.miniPortalProjectileDamage),
                                      ing.miniPortalBeamInfo);

  const CIngSwarmData swarm(ing.pART, ing.sRSC, ing.pART_0x3da219c7, ing.unknown_0x23271976,
                            ing.pART_0x081e9e6c, ing.unknown_0xcb39eccb, ing.unknown_0x587ca175,
                            ing.unknown_0x0bd7d5a9, static_cast< ushort >(ing.sound_SwarmMove),
                            static_cast< ushort >(ing.sound_ExitHost),
                            static_cast< ushort >(ing.sound_ExitHostSafeZone),
                            static_cast< ushort >(ing.sound_InsideHost),
                            static_cast< ushort >(ing.sound));

  const CIngBoostBallGuardianData data(
      ing.unknown_0x01789abd, ing.hearingRadius, ing.unknown_0x5d0d2c40, ing.unknown_0xc620183a,
      ing.frustrationTime, ing.tauntChance, ing.aggressiveness, ing.lightColor,
      ing.lightAttenuation, LdrToDamageInfo(ing.armSwipeDamage),
      LdrToDamageVulnerability(ing.ingSpotVulnerability), bodyProjectile, ingSpot, miniPortal,
      swarm);

  const auto phase = [](const SLdrIngBoostBallGuardianStruct& s) {
    return CIngBoostBallGuardianPhase(s.boostPuddleModeTime, s.unknown_0xabe99de0,
                                      s.breakBoostPuddleModeDamage, s.breakBoostModeDamage,
                                      s.nextBoostTimeMin, s.nextBoostTimeMax, s.unknown_0x285d67ad,
                                      s.unknown_0x6d5f242f, s.unknown_0xe3ff2ed6,
                                      s.unknown_0xa6fd6d54, s.unknown_0xecb314b6,
                                      s.unknown_0xa9b15734, s.locomotionSpeedScale,
                                      s.ingSpotSpeedScale);
  };

  const CBoostBallGuardianData boost(
      boostSldr.boostBallScale, boostSldr.boostBallMass, boostSldr.unknown_0xbea96fb7,
      boostSldr.boostBallMinDamageSpeed, boostSldr.unknown_0x7b21e31a, boostSldr.boostBallModel,
      boostSldr.boostBallChargeUpEffect, boostSldr.boostBallShieldEffect,
      boostSldr.boostBallTrailEffect, static_cast< ushort >(boostSldr.sound_Bounce),
      static_cast< ushort >(boostSldr.sound_IntoBall),
      static_cast< ushort >(boostSldr.sound_OutofBall),
      static_cast< ushort >(boostSldr.sound), static_cast< ushort >(boostSldr.sound_Boost),
      static_cast< ushort >(boostSldr.sound_Rolling),
      LdrToDamageVulnerability(boostSldr.boostBallVulnerability), boostSldr.unknown_0xee69b993,
      boostSldr.unknown_0x4b2de673, boostSldr.searchConeAngle, boostSldr.unknown_0xb0e85d53,
      LdrToDamageInfo(boostSldr.boostBallDamageToMorphBall),
      LdrToDamageInfo(boostSldr.boostBallDamageToFirstPerson),
      LdrToDamageInfo(boostSldr.ingSpotDamageToMorphBall),
      LdrToDamageInfo(boostSldr.ingSpotDamageToFirstPerson), boostSldr.boostIngSpotBlobEffect,
      boostSldr.boostIngSpotHitNormalDamage, boostSldr.boostIngSpotHitHeavyDamage,
      phase(boostSldr.ingBoostBallGuardianStruct),
      phase(boostSldr.ingBoostBallGuardianStruct_0xfe18a18f),
      phase(boostSldr.ingBoostBallGuardianStruct_0xc2784287));

  return rs_new CIngBoostBallGuardian(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                                      LdrToEntityInfo(info, sldrThis.editorProperties),
                                      LdrToTransform4f(sldrThis.editorProperties), *modelData,
                                      LdrToActorParameters(sldrThis.actorInformation),
                                      LdrToPatternedInfo(sldrThis.patterned, nullptr), data, boost);
}

static SIngBoostBallGuardian_FuncPtrs REL_loader_IngBoostBallGuardian;

// Retail's RELMain calls fn_30_100, which stores the loader fn_30_130 into lbl_30_bss_6C (the
// module's SIngBoostBallGuardian_FuncPtrs) and calls the DOL's SetSIngBoostBallGuardian_FuncPtrs
// (fn_8022FFC4). The proxy in SIngBoostBallGuardian_FuncPtrs.cpp calls through it.
void RELMain() {
  REL_loader_IngBoostBallGuardian.mLoader = PortLoadIngBoostBallGuardian;
  SetSIngBoostBallGuardian_FuncPtrs(&REL_loader_IngBoostBallGuardian);
}

void RELExit() { SetSIngBoostBallGuardian_FuncPtrs(nullptr); }
