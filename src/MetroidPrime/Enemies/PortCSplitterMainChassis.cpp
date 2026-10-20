// Host body for the Splitter REL (module 75), milestone 1: the actors load, register and think.
// The analysis is docs/module-analysis/Splitter.md. Which methods each class overrides is not
// guessed: the module's two vtables (lbl_75_data_CB8 for the main chassis, lbl_75_data_448 for
// the command module) list them by address and the slots line up with CPatterned's declaration
// order, the inherited entries keeping the DOL's own symbols.
//
// The accessors at the head of the module's .text are the REL's own one- or two-instruction bodies
// (fn_75_0/8/10/18/64/6C/74/7C/8C/98/A4/AC/D0 and, in the second .text, fn_75_81F8/8200/8208), so
// they are real rather than stand-ins; the bodies that belong to the state machine, the spin
// attack, the laser sweep and the docking forward to CPatterned behind a once-only StandIn log,
// and the retail function each stands in for is named on it.
#include "MetroidPrime/Enemies/CSplitterMainChassis.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Enemies/CSplitterCommandModule.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSplitterMainChassis.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/TCastTo.hpp"

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
  fprintf(stderr, "[splitter] %s is a port stand-in (see PortCSplitterMainChassis.cpp)\n", what);
}

// The module's own calls in the main chassis's Think and the DOL's two helpers it makes, as
// named once-logging shells: the name is the retail function and its size (the DOL's functions
// are not in the module, so they carry none). They are the docking link, the line-of-sight
// tracker and the derived state.
void StandInFn_75_A4E4() { StandIn("fn_75_A4E4 (0x418)"); }
void StandInFn_75_AFE8() { StandIn("fn_75_AFE8 (0x11C)"); }
void StandInFn_75_ACEC() { StandIn("fn_75_ACEC (0x2FC)"); }
void StandInFn_75_9520() { StandIn("fn_75_9520 (0x11C)"); }

// The module's own calls in the main chassis's script-message handler (fn_75_F1C0). All six are
// over the 0x80 a slice takes, so each stays a named once-logging shell.
void StandInFn_75_AAC4() { StandIn("fn_75_AAC4 (0xC8)"); }
void StandInFn_75_8D50() { StandIn("fn_75_8D50 (0xA8)"); }
void StandInFn_75_A0A0() { StandIn("fn_75_A0A0 (0x2DC)"); }
void StandInFn_75_9FA8() { StandIn("fn_75_9FA8 (0xF8)"); }
void StandInFn_75_9B9C() { StandIn("fn_75_9B9C (0x40C)"); }
void StandInFn_75_AC68() { StandIn("fn_75_AC68 (0x84)"); }

} // namespace

// The command module's loader lives in PortCSplitterCommandModule.cpp (the two files are the one
// REL's body).
CEntity* PortLoadSplitterCommandModule(CStateManager& mgr, CInputStream& input,
                                       CEntityInfo& info);

// fn_75_F5F8. The base call is the retail one: EPatternedAI 0x38 (li r4, 0x38 at 0x0000F654),
// flavor 0 (li r7, 0x0 at 0x0000F65C), movement 0 / collider 1 / body 1 (the stack words at
// 0x0000F648..0x0000F660), all passed to CPatterned at 0x0000F66C.
CSplitterMainChassis::CSplitterMainChassis(TUniqueId uid, const rstl::string& name,
                                             const CEntityInfo& info, const CTransform4f& xf,
                                             const CModelData& modelData,
                                             const CActorParameters& actParms,
                                             const CPatternedInfo& pInfo,
                                             const CSplitterMainChassisData& data)
: CPatterned(static_cast< EPatternedAI >(0x38), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mData(data) // guest 0x7C0: the parameter block fn_75_FB30 copies in
// fn_75_F5F8 @0x0000F690: the CPathFindSearch (0xec) at 0xb7c, built with the null area, the
// flags 1, the patterned info's pathfinding index and the two radii lbl_75_rodata_364 (1.0f);
// fn_75_81F8 returns it.
, mPathFindSearch(nullptr, 1, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0, CPFRegion::kRP_Center)
// port: writer fn_75_F5F8 @0x0000F6E4 (sth r6, 0xc6c(r29) - the retail's ctor copies
// kInvalidUniqueId, read at 0x0000F6CC, into the command module's id)
// port: writer fn_75_F5F8 @0x0000F6BC (stw r0, 0xc68(r29) with r0 = 0 - the retail's ctor
// leaves the collision actor manager empty)
, mCollisionActorManager(nullptr)
, mCommandModuleId(kInvalidUniqueId)
// port: the retail's CVisualKnowledge at 0xC70 is built by the ctor; the host's equivalent is
// CLineOfSightTracker, built here with the same arguments the other ports use.
, mVisualKnowledge(uid, CSegId(), 0.1f, 0.05f) {
  // The retail constructor also builds the parameter block's converted forms (fn_75_FB30), the
  // possession data (fn_75_77C0), the line-of-sight tracker at 0xc70 and the derived state at
  // 0xa50..0xb7b. They belong to the state machine and the render paths this milestone does not
  // enter, so the port keeps the parameter block only.
}

CSplitterMainChassis::~CSplitterMainChassis() {}

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove / Death / KnockBack all dereference the current state. The analysis does not say
// where retail activates it, so the port activates on kSM_Create and, as the other patterned
// actors in this port do, again on demand in every entry point that can run first. The body
// controller itself is always built by CPatterned's constructor (CPatterned.cpp's
// BuildBodyController call), so the port limit here is only the activation point, not the pointer.
void CSplitterMainChassis::EnsureBodyActive(CStateManager& mgr) {
  if (!BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

// vtable slot 0x10 = fn_75_F1A0: the REL's whole body is the base class's call.
void CSplitterMainChassis::PreThink(float dt, CStateManager& mgr) {
  EnsureBodyActive(mgr);
  CPatterned::PreThink(dt, mgr);
}

// fn_75_F0E8 (0xB8) - the main chassis's Think. The body follows the retail's: the active gate
// (the byte at 0x20, 0xF10C lbz and 0xF110 extrwi bit 24 - the byte's 0x80), the base's Think
// (0xF118), the docking link (the partner id at 0xC68, 0xF11C lwz and 0xF120 cmplwi 0: with no
// partner the retail looks for one, 0xF130 fn_75_A4E4, otherwise it updates the dock, 0xF144
// fn_80135D84), the line-of-sight tracker (the tracker at 0xC70, 0xF154 fn_801F55F0) and the
// module's own three calls (0xF164 fn_75_AFE8, 0xF174 fn_75_ACEC, 0xF180 fn_75_9520). Every one
// of them is over 0x80 or belongs to the DOL, so each is a once-logging shell here and the
// function carries no port marker yet.
// vtable slot 0x14.
void CSplitterMainChassis::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
  if (mCollisionActorManager == nullptr) {
    // port limit: the retail builds the collision actor manager in fn_75_A4E4 (0x418), which
    // is one of the module's own functions; the port makes an empty one instead,
    // as the command module does.
    StandInFn_75_A4E4();
    mCollisionActorManager = rs_new CCollisionActorManager(
        mgr, GetUniqueId(), GetCurrentAreaId(), rstl::vector< CJointCollisionDescription >(),
        true);
  } else {
    mCollisionActorManager->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  }
  mVisualKnowledge.Update(dt, mgr);
  StandInFn_75_AFE8();
  StandInFn_75_ACEC();
  StandInFn_75_9520();
}

// @ fn_75_AA68
// The chassis's locomotion type (0xAA68-0xAAC4): the data's flag word at 0x828 decides it. The
// word's bit 30 (the byte's 0x40, 0x0000AA78 rlwinm) gives kLT_Lurk (2, 0x0000AA88), the word's
// bit 31 (the byte's 0x80, 0x0000AA90 clrlwi) gives kLT_Crouch (0, 0x0000AAA0) and anything
// else kLT_Relaxed (1, 0x0000AAB0).
void CSplitterMainChassis::PortSetLocomotionType() {
  const int flags = mData.mUnknownCEF5C2FE;
  if ((flags & 0x40000000) != 0) {
    BodyController()->SetLocomotionType(pas::kLT_Lurk);
  } else if ((flags & 0x80000000) != 0) {
    BodyController()->SetLocomotionType(pas::kLT_Crouch);
  } else {
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
  }
}

// @ fn_75_F1C0
// The main chassis's script-message handler (0xF1C0-0xF560): the base's call (0x0000F1F4) and
// then a switch on the message's four-character code (0x0000F1F8-0x0000F33C). The retail's
// cases: XCRT (the locomotion type and the body controller's activation), XDEL (the collision
// actor manager's destruction and two module calls), ACTV and DCVT (the collision actor
// manager's activation), AUDT (its physics), ALRT (the base's flag byte), STRT (the flag byte
// at 0xD41), ACTN (the scan timer at 0xD28), XAND, XCMG, XRDG, XXDG, XHIT, XLND, IM00 and IM01.
void CSplitterMainChassis::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  CPatterned::AcceptScriptMsg(mgr, msg);
  // 0x0000F1E0-0x0000F1F0: the byte at 0x20's 0x80 bit (the word's bit 24) is the actor's
  // active flag.
  const bool active = GetActive();
  switch (msg.GetMessage()) {
  case kSM_Create: { // "XCRT" (0x0000F2C8)
    // 0x0000F328-0x0000F33C: the locomotion type (fn_75_AA68) and the body controller's
    // activation with the animation state -1.
    PortSetLocomotionType();
    BodyController()->Activate(mgr, pas::kAS_Invalid);
    break;
  }
  case kSM_Delete: { // "XDEL" (0x0000F2B4)
    // 0x0000F344-0x0000F36C: the collision actor manager is destroyed (0x0000F354 Destroy) and
    // the module's own two calls run (0x0000F360 fn_75_AAC4, 0x0000F36C fn_75_8D50).
    if (mCollisionActorManager != nullptr) {
      mCollisionActorManager->Destroy(mgr);
    }
    StandInFn_75_AAC4();
    StandInFn_75_8D50();
    break;
  }
  case kSM_Activate: { // "ACTV" (0x0000F220)
    // 0x0000F374-0x0000F390: with the actor inactive and a collision actor manager there, the
    // manager is activated (0x0000F390 SetActive with true).
    if (!active && mCollisionActorManager != nullptr) {
      mCollisionActorManager->SetActive(mgr, true);
    }
    break;
  }
  case kSM_Deactivate: { // "DCVT" (0x0000F20C)
    // 0x0000F398-0x0000F3C0: with the actor active and a collision actor manager there, the
    // manager is deactivated (0x0000F3B4 SetActive with false) and the module's own call runs
    // (0x0000F3C0 fn_75_8D50).
    if (active && mCollisionActorManager != nullptr) {
      mCollisionActorManager->SetActive(mgr, false);
    }
    StandInFn_75_8D50();
    break;
  }
  case kSM_AIUpdateDisabled: { // "AUDT" (0x0000F1F8)
    // 0x0000F3C8-0x0000F3DC: the collision actor manager's physics is switched off (0x0000F3DC
    // SetPhysicsActive with false).
    if (mCollisionActorManager != nullptr) {
      mCollisionActorManager->SetPhysicsActive(mgr, false);
    }
    break;
  }
  case kSM_Alert: { // "ALRT" (0x0000F244)
    // 0x0000F3E4-0x0000F3F0: the base's flag byte at 0x420 gets its 0x80 bit (the word's bit 24)
    // set - the Ing port maps that bit to the same flag.
    SetHitByPlayerProjectile(true);
    break;
  }
  case kSM_Start: { // "STRT" (0x0000F258)
    // 0x0000F3F8-0x0000F404: the flag byte at 0xD41 gets its 0x04 bit (the word's bit 29) set.
    mFlagsD41 |= 0x4;
    break;
  }
  case kSM_Action: { // "ACTN" (0x0000F234)
    // 0x0000F40C-0x0000F42C: the scan timer at 0xD28 is the data's scan duration (0x854) when
    // that is above zero and FLT_MAX otherwise.
    mScanTimer = mData.mScanDuration > 0.f ? mData.mScanDuration : FLT_MAX;
    break;
  }
  case kSM_AreaLoaded: { // "XAND" (0x0000F290)
    // 0x0000F434-0x0000F468: the value the retail reads out of the manager's area list is
    // written to 0xB7C and the module's own call runs (0x0000F468 fn_75_AC68). The port has no
    // name for that list, so the write stays out.
    StandInFn_75_AC68();
    break;
  }
  case kSM_Damage: { // "XCMG" (0x0000F2DC)
    // 0x0000F470-0x0000F498: the module's own call (0x0000F488 fn_75_A0A0) and the base's flag
    // byte at 0x420 gets its 0x80 bit set.
    StandInFn_75_A0A0();
    SetHitByPlayerProjectile(true);
    break;
  }
  case kSM_ResistedDamage:   // "XRDG" (0x0000F2EC)
  case kSM_ReflectedDamage: { // "XXDG" (0x0000F314)
    // 0x0000F4A0-0x0000F4C8: the module's own call (0x0000F4B8 fn_75_9FA8) and the base's flag
    // byte at 0x420 gets its 0x80 bit set.
    StandInFn_75_9FA8();
    SetHitByPlayerProjectile(true);
    break;
  }
  case kSM_HitObject: { // "XHIT" (0x0000F2A0)
    // 0x0000F4D0-0x0000F4E8: the module's own call (0x0000F4E8 fn_75_9B9C).
    StandInFn_75_9B9C();
    break;
  }
  case kSM_Landed: { // "XLND" (0x0000F300)
    // 0x0000F4F0-0x0000F4F4: the counter at 0xD00 is set to 5.
    mLandedCount = 5;
    break;
  }
  case kSM_InternalMessage0: { // "IM00" (0x0000F280)
    // 0x0000F4FC-0x0000F534: the flag byte at 0xD40 gets its 0x80 bit cleared and, when the
    // byte at 0xD41 has its 0x80 bit set, the timer at 0xD2C gets the data's spin attack max
    // time (0x88C) added and the counter at 0xCFC the data's unknown_0xd5f34476 (0x888).
    mFlagsD40 &= static_cast< uchar >(~0x80);
    if ((mFlagsD41 & 0x80) != 0) {
      mSpinTimer += mData.mSpinAttackMaxTime;
      mSpinCounter += mData.mUnknownD5F34476;
    }
    break;
  }
  case kSM_InternalMessage1: { // "IM01" (0x0000F26C)
    // 0x0000F53C-0x0000F548: the flag byte at 0xD40 gets its 0x80 bit set.
    mFlagsD40 |= 0x80;
    break;
  }
  default:
    break;
  }
}

// vtable slot 0x24 = fn_75_EF64: the REL's whole body is the base class's call.
void CSplitterMainChassis::PreRender(CStateManager& mgr) { CPatterned::PreRender(mgr); }

// vtable slot 0x28 = fn_75_EF44: the REL's whole body is the base class's call.
void CSplitterMainChassis::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
}

// vtable slot 0x2C = fn_75_EF84: the REL's whole body is the base class's call.
void CSplitterMainChassis::Render(const CStateManager& mgr) const { CPatterned::Render(mgr); }

// vtable slot 0x3C = fn_75_D0: it calls the vtable's HealthInfo slot (0x38) and returns its
// result, which is what the base class's own accessor does.
const CHealthInfo* CSplitterMainChassis::GetHealthInfo() const { return CPatterned::GetHealthInfo(); }

// vtable slot 0x40 = fn_75_EF24: it returns the static pass-through vulnerability, so the body
// cannot be damaged at all.
const CDamageVulnerability* CSplitterMainChassis::GetDamageVulnerability() const {
  return &CDamageVulnerability::PassThroughVulnerabilty();
}

// vtable slot 0x48 = fn_75_18: it takes the actor's bounding box and copies it into the optional
// (fn_75_7C44 copies the six words and sets the valid byte).
rstl::optional_object< CAABox > CSplitterMainChassis::GetTouchBounds() const {
  return rstl::optional_object< CAABox >(GetBoundingBox());
}

// vtable slot 0x70 = fn_75_EFA4.
CAABox CSplitterMainChassis::GetSortingBounds(const CStateManager& mgr) const {
  StandIn("CSplitterMainChassis::GetSortingBounds (vtable slot 0x70, fn_75_EFA4)");
  return CPatterned::GetSortingBounds(mgr);
}

// vtable slot 0x84 = fn_75_ECA4.
void CSplitterMainChassis::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                                        CStateManager& mgr) {
  StandIn("CSplitterMainChassis::CollidedWith (vtable slot 0x84, fn_75_ECA4)");
  CPatterned::CollidedWith(id, list, mgr);
}

// vtable slot 0x98 = fn_75_EB04.
void CSplitterMainChassis::Death(CStateManager& mgr, const CVector3f& direction,
                                  EScriptObjectState state) {
  StandIn("CSplitterMainChassis::Death (vtable slot 0x98, fn_75_EB04)");
  CPatterned::Death(mgr, direction, state);
}

// vtable slot 0x9C = fn_75_EBF4.
void CSplitterMainChassis::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  StandIn("CSplitterMainChassis::KnockBack (vtable slot 0x9C, fn_75_EBF4)");
  CPatterned::KnockBack(mgr, info);
}

// vtable slot 0xB0 = fn_75_EA70: it listens for the noise types the state machine asks for.
bool CSplitterMainChassis::Listen(CStateManager& mgr, const CVector3f& position,
                                   EListenNoiseType type) {
  StandIn("CSplitterMainChassis::Listen (vtable slot 0xB0, fn_75_EA70)");
  return CPatterned::Listen(mgr, position, type);
}

// vtable slot 0xF0 = fn_75_E9E4: it registers the module's three tables (28 triggers, 22 states
// and 11 codes) on the state machine. That is milestone 2, so only the base setup runs here.
void CSplitterMainChassis::SetupStateMachine(CStateManager& mgr) {
  StandIn("CSplitterMainChassis::SetupStateMachine (vtable slot 0xF0, fn_75_E9E4)");
  CPatterned::SetupStateMachine(mgr);
}

// vtable slot 0xF8 = fn_75_E99C: the Ing-possession gate, which reads the object's own flag bytes.
bool CSplitterMainChassis::CanBeIngPossessed(CStateManager& mgr) const {
  StandIn("CSplitterMainChassis::CanBeIngPossessed (vtable slot 0xF8, fn_75_E99C)");
  return CPatterned::CanBeIngPossessed(mgr);
}

// vtable slot 0x100 = fn_75_E90C.
void CSplitterMainChassis::SetIngPossessed(bool possessed, CStateManager& mgr) {
  StandIn("CSplitterMainChassis::SetIngPossessed (vtable slot 0x100, fn_75_E90C)");
  CPatterned::SetIngPossessed(possessed, mgr);
}

// @ fn_75_E87C
// vtable slot 0x104 (0x90). The base call (0xE8A0) and then, when the possession flag is set
// (0xE8A4 clrlwi. r0, r30, 24 - the flag's low byte), the command module's id (the u16 at
// 0xC6C, 0xE8AC lhz) is looked up (0xE8BC ObjectById, 0xE8C0 the CSplitterCommandModule cast)
// and told that it is possessed: the command module's vtable slot 0x104 is the DOL's
// SetIngPossessed(bool, float, CStateManager&) (0xE8CC-0xE8EC: true, mgr,
// &lbl_75_rodata_390 (0.0f), 0.0f). The id is still the invalid one - the state machine's
// writers of 0xC6C are missing from the port - so the lookup finds nothing.
void CSplitterMainChassis::SetIngPossessed(bool possessed, float duration, CStateManager& mgr) {
  CPatterned::SetIngPossessed(possessed, duration, mgr);
  if (possessed) {
    CEntity* entity = mgr.ObjectById(mCommandModuleId);
    if (CSplitterCommandModule* module = TCastToPtr< CSplitterCommandModule >(entity)) {
      module->SetIngPossessed(true, 0.f, mgr);
    }
  }
}

// vtable slot 0x108 = fn_75_E810.
void CSplitterMainChassis::SetAttackTarget(CStateManager& mgr, TUniqueId target) {
  StandIn("CSplitterMainChassis::SetAttackTarget (vtable slot 0x108, fn_75_E810)");
  CPatterned::SetAttackTarget(mgr, target);
}

// The module's auto-destruct starter (retail fn_75_5A54). The DOL reaches it through
// SSplitterMainChassis_FuncPtrs::mAutoDestruct, whose entry the REL's table fills with the fifth
// word (the table is {loader, loader, 0, 0xFFFFFFFF, fn_75_5A54}). What that function does is
// not understood yet (docs/module-analysis/Splitter.md section j), so this is a documented
// stand-in: it logs once and does nothing else.
void CSplitterMainChassis::PortAutoDestruct(float time) {
  StandIn("CSplitterMainChassis::AutoDestruct (the REL's fn_75_5A54 body is not understood yet)");
}

// ---------------------------------------------------------------------------------------------
// The module's loaders (fn_75_82B0 for the main chassis, fn_75_FC for the command module) and
// entry points.

// Retail fn_75_82B0. The property ids and their order are the generated
// SLdrSplitterMainChassis reader's, which the analysis checked against the REL's switch id by id:
// the loader's dispatch tree reads all four blocks (0x255a4580 editor properties, 0xB3774750
// patterned AI, 0x7E397FED actor parameters, 0x15E03A2F the chassis data).
static CEntity* PortLoadSplitterMainChassis(CStateManager& mgr, CInputStream& input,
                                             CEntityInfo& info) {
  SLdrSplitterMainChassis sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrSplitterMainChassis.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL reads the model copy's valid byte (the byte at 0x148 of its stack frame, read at
    // 0x00008488) and returns null when it is clear.
    return nullptr;
  }

  // The parameter block, field by field: the loader's stack struct is copied into the object
  // (fn_75_FC10), so the host keeps the loader's own values, with the damage infos and the
  // vulnerability in their converted forms.
  const SLdrSplitterMainChassisData& sldr = sldrThis.splitterMainChassisData;
  CSplitterMainChassisData data;
  data.mUnknownCEF5C2FE = sldr.unknown_0xcef5c2fe;
  data.mLegStabAttackInterval = sldr.legStabAttackInterval;
  data.mLegStabMinAttackRange = sldr.legStabMinAttackRange;
  data.mLegStabMaxAttackRange = sldr.legStabMaxAttackRange;
  data.mLegStabDamage = LdrToDamageInfo(sldr.legStabDamage);
  data.mMinDodgeInterval = sldr.minDodgeInterval;
  data.mDodgeChance = sldr.dodgeChance;
  data.mDeploymentSpeed = sldr.deploymentSpeed;
  data.mScanDuration = sldr.scanDuration;
  data.mLaserSweepInterval = sldr.laserSweepInterval;
  data.mLaserSweepMinAttackRange = sldr.laserSweepMinAttackRange;
  data.mLaserSweepMaxAttackRange = sldr.laserSweepMaxAttackRange;
  data.mSpinAttackLinearVelocity = sldr.spinAttackLinearVelocity;
  data.mSpinAttackLinearAcceleration = sldr.spinAttackLinearAcceleration;
  data.mSpinAttackLinearDeceleration = sldr.spinAttackLinearDeceleration;
  data.mSpinAttackTurnSpeed = sldr.spinAttackTurnSpeed;
  data.mSpinAttackInterval = sldr.spinAttackInterval;
  data.mSpinAttackIntervalPenalty = sldr.spinAttackIntervalPenalty;
  data.mSpinAttackTelegraphTime = sldr.spinAttackTelegraphTime;
  data.mSpinAttackMinAttackRange = sldr.spinAttackMinAttackRange;
  data.mSpinAttackMaxAttackRange = sldr.spinAttackMaxAttackRange;
  data.mUnknownD5F34476 = sldr.unknown_0xd5f34476;
  data.mSpinAttackMaxTime = sldr.spinAttackMaxTime;
  data.mSpinAttackDamage = LdrToDamageInfo(sldr.spinAttackDamage);
  data.mSoundAlerted = sldr.sound_Alerted;
  data.mSpinAttackVulnerability = LdrToDamageVulnerability(sldr.spinAttackVulnerability);

  return rs_new CSplitterMainChassis(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                                     LdrToEntityInfo(info, sldrThis.editorProperties),
                                     LdrToTransform4f(sldrThis.editorProperties), *modelData,
                                     LdrToActorParameters(sldrThis.actorInformation),
                                     LdrToPatternedInfo(sldrThis.patterned, &sldr.ingPossessionData),
                                     data);
}

static SSplitterMainChassis_FuncPtrs REL_loader_Splitter;

// Retail's RELMain calls fn_75_8254, which fills the module's table (fn_75_82B0, fn_75_FC, 0,
// 0xFFFFFFFF and fn_75_5A54) and registers it with the DOL's SetSSplitterMainChassis_FuncPtrs.
// The port's table has three slots: the two loaders and the auto-destruct starter.
void RELMain() {
  REL_loader_Splitter.mLoadMainChassis = PortLoadSplitterMainChassis;
  REL_loader_Splitter.mLoadCommandModule = PortLoadSplitterCommandModule;
  REL_loader_Splitter.mAutoDestruct = &CSplitterMainChassis::PortAutoDestruct;
  SetSSplitterMainChassis_FuncPtrs(&REL_loader_Splitter);
}

void RELExit() { SetSSplitterMainChassis_FuncPtrs(nullptr); }
