// Host body for the Splitter REL (module 75)'s command module - the "Quad" robot's detachable
// head, script type SPLU. Milestone 1: the actor loads, registers and thinks. The analysis is
// docs/module-analysis/Splitter.md; the module's entry points live in PortCSplitterMainChassis.cpp
// (the two files are the one REL's body).
//
// The accessors at the head of the module's .text are the REL's own one- or two-instruction bodies
// (fn_75_0, fn_75_8, fn_75_10, fn_75_18, fn_75_64, fn_75_6C, fn_75_74), so they are real rather than
// stand-ins; the bodies that belong to the state machine, the scanning, the laser pulse and sweep,
// the shields and the docking forward to CPatterned behind a once-only StandIn log, and the retail
// function each stands in for is named on it.
#include "MetroidPrime/Enemies/CSplitterCommandModule.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSplitterCommandModule.hpp"
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
  fprintf(stderr, "[splitter] %s is a port stand-in (see PortCSplitterCommandModule.cpp)\n",
          what);
}

} // namespace

// fn_75_6B08. The base call is the retail one: EPatternedAI 0x39 (li r4, 0x39 at 0x00006B68),
// flavor 0 (li r7, 0x0 at 0x00006B30), movement 1 / collider 1 / body 7 (the stack words at
// 0x00006B60..0x00006B6C - the head flies), all passed to CPatterned at 0x00006B78.
CSplitterCommandModule::CSplitterCommandModule(TUniqueId uid, const rstl::string& name,
                                                 const CEntityInfo& info, const CTransform4f& xf,
                                                 const CModelData& modelData,
                                                 const CActorParameters& actParms,
                                                 const CPatternedInfo& pInfo,
                                                 const CSplitterCommandModuleData& data)
: CPatterned(static_cast< EPatternedAI >(0x39), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Flyer, kCT_One, kBT_4WayBlended, actParms)
, mData(data) // guest 0x7C0: the parameter block fn_75_72BC copies in
// fn_75_6B08 @0x00006BA0: the CPathFindSearch (0xec) at 0xd6c, built with the null area, the
// flags 3, the patterned info's pathfinding index and the two radii lbl_75_rodata_34 (1.0f);
// fn_75_0 returns it.
, mPathFindSearch(nullptr, 3, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0, CPFRegion::kRP_Center)
// fn_75_6B08 @0x00006BC4: the CProjectileInfo (0x28) at 0xe5c, built from the data's laser pulse
// projectile (+0x20) and damage (+0x514 of the runtime struct); fn_75_8 returns it.
, mProjectileInfo(data.mLaserPulseProjectile, data.mLaserPulseDamage) {
  // The retail constructor also builds the second CProjectileInfo at 0xe84 (its asset id is
  // unknown), the shield vulnerability at 0xeac (a copy of the base class's) and the derived state
  // at 0xa7c..0xd6b. They belong to the state machine and the render paths this milestone does
  // not enter, so the port keeps the parameter block only.
}

CSplitterCommandModule::~CSplitterCommandModule() {}

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove all dereference the current state. The analysis does not say where retail
// activates it, so the port activates on kSM_Create and, as the other patterned actors in this
// port do, again on demand in every entry point that can run first.
void CSplitterCommandModule::EnsureBodyActive(CStateManager& mgr) {
  if (!BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

// vtable slot 0x14 = fn_75_6490. It tests the actor's active bit (the byte at 0x20, bit 0, read
// at 0x000064B4) and returns when it is clear, then runs CPatterned::Think; the rest of the body
// (the collision manager update, the docking link and the derived state) is the state machine's
// work.
void CSplitterCommandModule::Think(float dt, CStateManager& mgr) {
  StandIn("CSplitterCommandModule::Think (vtable slot 0x14, fn_75_6490)");
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
}

// vtable slot 0x18 = fn_75_654C (0x24C): it calls the base class's AcceptScriptMsg and then
// switches on the script message's four-character code.
void CSplitterCommandModule::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("CSplitterCommandModule::AcceptScriptMsg (vtable slot 0x18, fn_75_654C)");
  CPatterned::AcceptScriptMsg(mgr, msg);
  if (msg.GetMessage() == kSM_Create) {
    EnsureBodyActive(mgr);
  }
}

// vtable slot 0x24 = fn_75_633C: it renders only when the object's own flag byte says so (the
// byte at 0x420, bit 1, read at 0x00006348).
void CSplitterCommandModule::PreRender(CStateManager& mgr) {
  StandIn("CSplitterCommandModule::PreRender (vtable slot 0x24, fn_75_633C)");
  CPatterned::PreRender(mgr);
}

// vtable slot 0x28 = fn_75_6310: the same flag byte guards the base class's call.
void CSplitterCommandModule::AddToRenderer(const CStateManager& mgr) const {
  StandIn("CSplitterCommandModule::AddToRenderer (vtable slot 0x28, fn_75_6310)");
  CPatterned::AddToRenderer(mgr);
}

// vtable slot 0x2C = fn_75_6368: the same flag byte guards the base class's call.
void CSplitterCommandModule::Render(const CStateManager& mgr) const {
  StandIn("CSplitterCommandModule::Render (vtable slot 0x2C, fn_75_6368)");
  CPatterned::Render(mgr);
}

// vtable slot 0x3C = fn_75_D0: it calls the vtable's HealthInfo slot (0x38) and returns its
// result, which is what the base class's own accessor does.
const CHealthInfo* CSplitterCommandModule::GetHealthInfo() const {
  return CPatterned::GetHealthInfo();
}

// vtable slot 0x40 = fn_75_616C: it returns the object's own copy of the base class's damage
// vulnerability (the ctor copies it into 0xeac with fn_75_70A0).
const CDamageVulnerability* CSplitterCommandModule::GetDamageVulnerability() const {
  return CPatterned::GetDamageVulnerability();
}

// vtable slot 0x48 = fn_75_18: it takes the actor's bounding box and copies it into the optional
// (fn_75_7C44 copies the six words and sets the valid byte).
rstl::optional_object< CAABox > CSplitterCommandModule::GetTouchBounds() const {
  return rstl::optional_object< CAABox >(GetBoundingBox());
}

// vtable slot 0x54 = fn_75_5F8C (0x1E0): the aim position the head scans from.
CVector3f CSplitterCommandModule::GetAimPosition(const CStateManager& mgr, float dt) const {
  StandIn("CSplitterCommandModule::GetAimPosition (vtable slot 0x54, fn_75_5F8C)");
  return CPatterned::GetAimPosition(mgr, dt);
}

// vtable slot 0x70 = fn_75_6394.
CAABox CSplitterCommandModule::GetSortingBounds(const CStateManager& mgr) const {
  StandIn("CSplitterCommandModule::GetSortingBounds (vtable slot 0x70, fn_75_6394)");
  return CPatterned::GetSortingBounds(mgr);
}

// vtable slot 0x84 = fn_75_6178.
void CSplitterCommandModule::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                                           CStateManager& mgr) {
  StandIn("CSplitterCommandModule::CollidedWith (vtable slot 0x84, fn_75_6178)");
  CPatterned::CollidedWith(id, list, mgr);
}

// vtable slot 0x9C = fn_75_6174: the REL's whole body is a single `blr`, so the head ignores a
// knock-back.
void CSplitterCommandModule::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {}

// vtable slot 0xB0 = fn_75_5F18: it listens for the noise types the state machine asks for.
bool CSplitterCommandModule::Listen(CStateManager& mgr, const CVector3f& position,
                                     EListenNoiseType type) {
  StandIn("CSplitterCommandModule::Listen (vtable slot 0xB0, fn_75_5F18)");
  return CPatterned::Listen(mgr, position, type);
}

// vtable slot 0xF0 = fn_75_5E8C: it registers the module's three tables (17 triggers, 14 states
// and 7 codes) on the state machine. That is milestone 2, so only the base setup runs here.
void CSplitterCommandModule::SetupStateMachine(CStateManager& mgr) {
  StandIn("CSplitterCommandModule::SetupStateMachine (vtable slot 0xF0, fn_75_5E8C)");
  CPatterned::SetupStateMachine(mgr);
}

// vtable slot 0xF8 = fn_75_5E50: it is only possessed while it is docked to a main chassis (the
// main chassis' unique id at 0xefa).
bool CSplitterCommandModule::CanBeIngPossessed(CStateManager& mgr) const {
  StandIn("CSplitterCommandModule::CanBeIngPossessed (vtable slot 0xF8, fn_75_5E50)");
  return CPatterned::CanBeIngPossessed(mgr);
}

// ---------------------------------------------------------------------------------------------
// The module's SPLU loader (retail fn_75_FC).

// Retail fn_75_FC. The property ids and their order are the generated
// SLdrSplitterCommandModule reader's, which the analysis checked against the REL's switch id by
// id: the loader's dispatch tree reads all four blocks (0x255a4580 editor properties, 0xB3774750
// patterned AI, 0x7E397FED actor parameters, 0x560CF813 the command module data).
CEntity* PortLoadSplitterCommandModule(CStateManager& mgr, CInputStream& input,
                                       CEntityInfo& info) {
  SLdrSplitterCommandModule sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrSplitterCommandModule.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL reads the model copy's valid byte (the byte at 0x148 of its stack frame, read at
    // 0x000002D4) and returns null when it is clear.
    return nullptr;
  }

  // The parameter block, field by field: the loader's stack struct is copied into the object
  // (fn_75_73A0), so the host keeps the loader's own values, with the damage infos and the
  // vulnerabilities in their converted forms.
  const SLdrSplitterCommandModuleData& sldr = sldrThis.commandModuleProperties;
  CSplitterCommandModuleData data;
  data.mUnknownBD80FD94 = sldr.unknown_0xbd80fd94;
  data.mMaxLinearVelocity = sldr.maxLinearVelocity;
  data.mMaxTurnSpeed = sldr.maxTurnSpeed;
  data.mScanningTurnSpeed = sldr.scanningTurnSpeed;
  data.mMinLaserPulseAttackTime = sldr.minLaserPulseAttackTime;
  data.mMinLaserPulseRange = sldr.minLaserPulseRange;
  data.mMaxLaserPulseRange = sldr.maxLaserPulseRange;
  data.mMaxLaserPulseShots = sldr.maxLaserPulseShots;
  data.mLaserPulseProjectile = sldr.laserPulseProjectile;
  data.mLaserPulseDamage = LdrToDamageInfo(sldr.laserPulseDamage);
  data.mMinDodges = sldr.minDodges;
  data.mMaxDodges = sldr.maxDodges;
  data.mDodgeChance = sldr.dodgeChance;
  data.mResetShieldTime = sldr.resetShieldTime;
  data.mShieldHP = sldr.shieldHP;
  data.mLaserSweepTurnSpeed = sldr.laserSweepTurnSpeed;
  data.mLaserSweepDamage = LdrToDamageInfo(sldr.laserSweepDamage);
  data.mLaserSweepBeamInfo = sldr.laserSweepBeamInfo;
  data.mUnknown9EC51FE4 = sldr.unknown_0x9ec51fe4;
  data.mSoundLaserSweep = sldr.sound_LaserSweep;
  data.mSoundLaserChargeUp = sldr.sound_LaserChargeUp;
  data.mSoundDocking = sldr.sound_Docking;
  data.mSoundScanning = sldr.sound_Scanning;
  data.mSoundLightShield = sldr.sound_LightShield;
  data.mSoundDarkShield = sldr.sound_DarkShield;
  data.mSoundShieldOn = sldr.sound_ShieldOn;
  data.mLightShieldVulnerability = LdrToDamageVulnerability(sldr.lightShieldVulnerability);
  data.mDarkShieldVulnerability = LdrToDamageVulnerability(sldr.darkShieldVulnerability);

  return rs_new CSplitterCommandModule(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                                       LdrToEntityInfo(info, sldrThis.editorProperties),
                                       LdrToTransform4f(sldrThis.editorProperties), *modelData,
                                       LdrToActorParameters(sldrThis.actorInformation),
                                       LdrToPatternedInfo(sldrThis.patterned,
                                                          &sldr.ingPossessionData),
                                       data);
}
