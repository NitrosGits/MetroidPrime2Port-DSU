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
#include "MetroidPrime/Enemies/CSplitterMainChassis.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include "MetroidPrime/CCollisionActorManager.hpp"

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

// The module's own per-frame calls, as named once-logging stand-ins: the name is the retail
// function and its size. They are the line-of-sight tracker (fn_75_31A8) and the derived state
// (the other five) that the state machine reads, and all six are over the 0x80 a slice takes.
void StandInFn_75_3004() { StandIn("fn_75_3004 (0x1A4)"); }
void StandInFn_75_1188() { StandIn("fn_75_1188 (0x344)"); }

// The DOL's CSfxManager::UpdateEmitter (the sound the look-at plays).
void StandInSfxUpdateEmitter() { StandIn("the DOL's CSfxManager::UpdateEmitter"); }

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

// The module's auto-destruct (fn_75_5A54). Its body is not understood yet (the module note's
// section j), so it logs once and does nothing else.
void CSplitterCommandModule::PortAutoDestruct(float time) {
  StandIn("fn_75_5A54 (the module's auto-destruct)");
}

// The module's own calls in the command module's chassis check: both are over 0x80, so each
// stays a named once-logging shell.
void StandInFn_75_1578() { StandIn("fn_75_1578 (0x218)"); }
void StandInFn_75_2280() { StandIn("fn_75_2280 (0x170)"); }

// @ fn_75_32F4
// The command module's flash and death (0x32F4-0x3460): three timers are counted down (0xF10 at
// 0x332C, 0xF0C at 0x333C and 0xF14 at 0x3344) and two counted up (0xF18 at 0x3354 and 0xF1C at
// 0x3360). While the flash weight 0xF08 is above zero (0x336C-0x3370) it is counted down and
// clamped at zero (0x3374-0x338C) and the colour at 0xF04 is the Lerp of the white and the base
// colour (0x450) with the weight capped at 1.0f (0x3390-0x33A8, 0x33C0). Then the flag byte at
// 0xF6A (the word's bit 29, the byte's 0x04, 0x33CC-0x33D4) and the alive bit of the byte at
// 0x420 (the word's bit 25, the byte's 0x40, 0x33D8-0x33E0) gate the death timer 0xEF0, which is
// counted down (0x33EC-0x33F4) and, at zero or below (0x33FC-0x3404), kills the actor with the
// direction (0x28, 0x38 and 0x48) and the state 0x5241544C ('RATL', 0x340C-0x3434).
void CSplitterCommandModule::PortUpdateFlashAndDeath(CStateManager& mgr, float dt) {
  mCountdownTimerA -= dt;
  mCountdownTimerB -= dt;
  mChassisTimer -= dt;
  mCountupTimerA += dt;
  mCountupTimerB += dt;
  if (mFlashAlpha > 0.f) {
    mFlashAlpha -= dt;
    if (mFlashAlpha < 0.f) {
      mFlashAlpha = 0.f;
    }
    const float weight = mFlashAlpha < 1.f ? mFlashAlpha : 1.f;
    mFlashColor = CColor::Lerp(CColor::White(), mColor, weight);
  }
  if ((mAutoDestructFlags & 0x4) != 0 && GetAlive()) {
    mDeathTimer -= dt;
    if (mDeathTimer <= 0.f) {
      Death(mgr, mDeathDirection, kSS_DeathRattle);
    }
  }
}

// @ fn_75_31A8
// The command module's chassis check (0x31A8-0x32F4): the id at 0xEFA (0x31D8 lhz) is looked up
// (0x31E8 GetObjectById) and cast to the main chassis (0x31EC); a null cast or a chassis whose
// command module id (0xC6C) is not this actor (0x31F8-0x3204, the retail's compare is against
// this+0x8 - the entity's own id) goes to 0x3238, which clears the id at 0xEFA and the id at
// 0x12A (0x3240-0x324C) and ends. Ours: the copy at 0xEFE takes the chassis's 0xCC0 when the two
// differ (0x3208-0x3218), fn_75_1578 runs (0x3224) and the timer 0xF14 is set to 3.0f
// (0x322C-0x3230). With no chassis id, the data's flag bit 0x8 (0x3258 rlwinm 28-28), the timer
// <= 0.0f (0x326C-0x3274) and the id 0xEFC being invalid (0x327C-0x3280) gate fn_75_2280
// (0x3284) and the timer is set to 3.0f again (0x328C-0x3290). Then the id 0xEFC (0x32A4) is
// looked up and cast (0x32B4-0x32B8): a dead chassis (the byte at 0x420, the word's bit 25 = the
// byte's 0x40 - the host's GetAlive) clears it (0x32D0-0x32D8).
void CSplitterCommandModule::PortUpdateChassis(CStateManager& mgr) {
  if (mChassisId != kInvalidUniqueId) {
    const CSplitterMainChassis* chassis =
        TCastToConstPtr< CSplitterMainChassis >(mgr.GetObjectById(mChassisId));
    if (chassis == nullptr || chassis->GetCommandModuleId() != GetUniqueId()) {
      mChassisId = kInvalidUniqueId;
      mClearedId = kInvalidUniqueId;
      return;
    }
    if (mSyncedId != chassis->GetSyncId()) {
      mSyncedId = chassis->GetSyncId();
      StandInFn_75_1578();
    }
    mChassisTimer = 3.f;
    return;
  }
  if ((mData.mUnknownBD80FD94 & 0x8) != 0 && mChassisTimer <= 0.f &&
      mCheckedChassisId == kInvalidUniqueId) {
    StandInFn_75_2280();
    mChassisTimer = 3.f;
  }
  if (mCheckedChassisId != kInvalidUniqueId) {
    const CSplitterMainChassis* chassis =
        TCastToConstPtr< CSplitterMainChassis >(mgr.GetObjectById(mCheckedChassisId));
    if (chassis == nullptr || !chassis->GetAlive()) {
      mCheckedChassisId = kInvalidUniqueId;
    }
  }
}

// @ fn_75_21FC
// The auto-destruct timer (0x21FC-0x2280): the flag byte at 0xF6A (0x2210 lbz, 0x2214 clrlwi with
// the word's bit 31 - the byte's 0x01) gates it. With the flag set the timer at 0xEF4 (0x221C lfs)
// is advanced by dt (0x2224 fadds, 0x222C stfs) and when it reaches 5.0f (0x2228 lbl_75_rodata_58,
// 0x2234 fcmpo and 0x2238 cror eq,gt,eq - the >= sense) the module's auto-destruct runs with 0.0f
// (0x2240/0x2244 lbl_75_rodata_40, 0x2248 the call); the flag is cleared either way (0x224C-0x2258).
// With the flag clear the timer is reset to 0.0f (0x2260-0x2268).
void CSplitterCommandModule::PortAutoDestructTimer(float dt) {
  if (mAutoDestructFlags & 0x1) {
    mAutoDestructTimer += dt;
    if (mAutoDestructTimer >= 5.f) {
      PortAutoDestruct(0.f);
    }
    mAutoDestructFlags &= ~static_cast< uchar >(0x1);
  } else {
    mAutoDestructTimer = 0.f;
  }
}

// The DOL's CEntity virtual the look-at calls (the vtable slot 0x8C, 0x1924-0x192C): the DOL's
// slot 35 is not named in the port, so the call stays a once-logging shell.
void StandInCEntityVirtual0x8C() { StandIn("the DOL's CEntity virtual at the vtable slot 0x8C"); }

// @ fn_75_184C
// The command module's look-at (0x184C-0x1974): the target id (the u16 at 0xF02, 0x1874 lhz) is
// looked up (0x1888 ObjectById) and, when it is there, the module's locator transform (the
// segment id at 0xEF8, 0x18A0 GetLctrTransform) gives the position; the offset vector (0xF50,
// 0xF54 and 0xF58, added at 0x18E8-0x18F0) is added to it, the transform looks at that point
// (0x1900 LookAt with the up vector) and the result goes to the target's virtual at the vtable
// slot 0x8C (0x1924-0x192C). The sound (the handle at 0xF5C read at 0x1930, the target's position
// at 0x444 and the byte 0x7f, 0x194C UpdateEmitter) is the DOL's CSfxManager call, which the host
// spells differently, so it stays a shell too.
void CSplitterCommandModule::PortLookAtTarget(CStateManager& mgr, float dt) {
  CEntity* target = mgr.ObjectById(mLookAtTargetId);
  if (target == nullptr) {
    return;
  }
  const CTransform4f locator = GetLctrTransform(mLookAtSegment);
  const CVector3f position(locator.GetTranslation().GetX() + mLookAtOffset.GetX(),
                            locator.GetTranslation().GetY() + mLookAtOffset.GetY(),
                            locator.GetTranslation().GetZ() + mLookAtOffset.GetZ());
  const CTransform4f lookAt =
      CTransform4f::LookAt(locator.GetTranslation(), position, CVector3f::Up());
  StandInCEntityVirtual0x8C();
  StandInSfxUpdateEmitter();
}

// fn_75_6490 (0xBC) - the command module's Think. The body follows the retail's: the
// active gate, the base's Think, the collision actor manager's update and the module's
// own six calls in order (their sizes are in the table in
// docs/module-analysis/Splitter.md). The six are over 0x80 and each is a once-logging
// shell here, so this function carries no port marker yet.
// vtable slot 0x14 (0xBC). The active gate (the byte at 0x20, 0x64B4 lbz and 0x64B8 extrwi bit 24 -
// the byte's 0x80), CPatterned::Think (0x64C0), the collision actor manager's update (the manager at
// 0xE58, 0x64C8 lwz; the update options 0 at 0x64D0, which is kUO_ObjectSpace; 0x64D4 the update)
// and then the module's own six per-frame calls in order: fn_75_31A8 (0x64E0, 0x14C), fn_75_32F4
// (0x64F0, 0x16C), fn_75_3004 (0x64FC, 0x1A4), fn_75_184C (0x650C, 0x128), fn_75_1188 (0x651C, 0x344)
// and fn_75_21FC (0x652C, 0x84). All six are over 0x80 and belong to the line-of-sight tracker and
// the derived state, so they stay named once-logging stand-ins that return neutral values.
void CSplitterCommandModule::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
  // port limit: the retail builds the collision actor manager in its constructor, where it has
  // no CStateManager to hand it, so the port creates it on the first Think instead.
  if (mCollisionActorManager == nullptr) {
    mCollisionActorManager = rs_new CCollisionActorManager(
        mgr, GetUniqueId(), GetCurrentAreaId(), rstl::vector< CJointCollisionDescription >(), true);
  }
  mCollisionActorManager->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  PortUpdateChassis(mgr);
  PortUpdateFlashAndDeath(mgr, dt);
  StandInFn_75_3004();
  PortLookAtTarget(mgr, dt);
  StandInFn_75_1188();
  PortAutoDestructTimer(dt);
}

// vtable slot 0x18 = fn_75_654C (0x24C): it calls the base class's AcceptScriptMsg and then
// switches on the script message's four-character code.
void CSplitterCommandModule::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("CSplitterCommandModule::AcceptScriptMsg (vtable slot 0x18, fn_75_654C, 0x24C bytes)");
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
