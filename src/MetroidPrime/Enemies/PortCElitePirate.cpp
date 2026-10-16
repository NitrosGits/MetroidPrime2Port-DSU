// Host body for the ElitePirate REL (module 15), milestone 1: the actors load, stand and render.
// The analysis is docs/module-analysis/ElitePirate.md. Every vtable override below forwards to
// CPatterned behind a once-per-method StandIn log; the retail function it stands in for is named on
// each. There is no combat AI yet - CPatterned's default state machine drives the pirate - and the
// shields, the shock waves, the rockets and the FSM (fn_15_C0D0, 0x808) are later milestones.
//
// Which methods CElitePirate overrides is not guessed: the REL's vtable lbl_15_data_634 lists them
// by address and its slots line up with CPatterned's declaration order (slot 27 GetSortingBounds,
// slot 28 DoUserAnimEvent, slot 29 GetScannableObjectInfo - the same numbering the Ing and
// SpacePirate analyses used). The entries the REL inherits keep the DOL's own symbols, so the
// table also says what is *not* overridden: slot 4 is CPatterned::PreThink. The tiny accessors at
// the head of the module's .text are decided by their two-to-twelve-instruction bodies, so they are
// real, not stand-ins.
#include "MetroidPrime/Enemies/CElitePirate.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrElitePirate.hpp"
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
  fprintf(stderr, "[elitepirate] %s is a port stand-in (see PortCElitePirate.cpp)\n", what);
}

} // namespace

CElitePirate::CElitePirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                           const CTransform4f& xf, const CModelData& modelData,
                           const CPatternedInfo& pInfo, const CActorParameters& actParms,
                           const CElitePirateData& data)
// EPatternedAI 0xc, flavor 0, movement 0 (ground), collider 1, body 1 (bipedal): the retail
// constructor's arguments to CPatterned (fn_15_A190, which passes r4 = 0xc and 0/1/1 on the stack).
: CPatterned(static_cast< EPatternedAI >(0xc), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mData(data)
// fn_15_A190 @0x0000A520: (null area, flags 0x201 or 1, the patterned info's pathfinding index at
// +0xec, 1.0f, 1.0f, no avoidance filter, kRP_Center). The two radii are lbl_15_rodata_DC, which is
// 1.0f; the flag's 0x200 bit comes from a field of the patterned info (r29+0x120) that this
// milestone passes as set.
, mPathFindSearch(nullptr, 0x201, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0, CPFRegion::kRP_Center)
// fn_15_A190 @0x0000A674: the CProjectileInfo (0x28) at 0xcd0, built from the data's rocket asset
// (+0x168) and damage (+0x16c); ProjectileInfo returns it.
, mProjectileInfo(data.mRocket, data.mRocketDamage) {
  // The retail constructor disables every knock-back anim reaction and then re-enables the first
  // (@0x0000A714: EnableAllAnimReactions(false), then EnableAnimReaction(kAR_Flinch, true)).
  // Everything else it builds - the CCollidableAABox at 0x9c0, the 'PART' token at 0x9e8, the FSM2
  // token object at 0xc30, the initial-state ids at 0xc3c/0xc40 and the shield/shock-wave members -
  // belongs to the state machine and the render paths this milestone does not enter.
  KnockBackController().EnableAllAnimReactions(false);
  KnockBackController().EnableAnimReaction(CKnockBackMgr::kAR_Flinch, true);
}

CElitePirate::~CElitePirate() {}

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove all dereference the current state. The analysis does not say where retail activates
// it, so the port activates on kSM_Create and, as the other patterned actors in this port do, again
// on demand in Think. (Retail does not override PreThink, Touch, Death or KnockBack - the REL's
// vtable keeps the base-class symbols in those slots - so those entry points cannot activate it.)
void CElitePirate::EnsureBodyActive(CStateManager& mgr) {
  if (BodyController() != nullptr && !BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

// vtable slot 5 = fn_15_9598 (0x2A8). It returns immediately unless the actor is active (a bit in
// this+0x20) and then runs the state machine, so it maps to Think.
void CElitePirate::Think(float dt, CStateManager& mgr) {
  StandIn("CElitePirate::Think (vtable slot 5, fn_15_9598)");
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
}

// vtable slot 6 = fn_15_8580 (0x804). It switches on the script message's four-character code,
// which is AcceptScriptMsg; the switch's own arms (shield, FSM, spawn) are milestone 2+.
void CElitePirate::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("CElitePirate::AcceptScriptMsg (vtable slot 6, fn_15_8580)");
  CPatterned::AcceptScriptMsg(mgr, msg);
  if (msg.GetMessage() == kSM_Create) {
    EnsureBodyActive(mgr);
  }
}

// vtable slot 9 = fn_15_83C0 (0x12C): forwards to CPatterned::PreRender and then positions the
// shield / visor-electric effect off an LCTR transform it looks up by name.
void CElitePirate::PreRender(CStateManager& mgr) {
  StandIn("CElitePirate::PreRender (vtable slot 9, fn_15_83C0)");
  CPatterned::PreRender(mgr);
}

// vtable slot 10 = fn_15_84EC (0x20): the REL's whole body is the base-class call, so this one is
// real rather than a stand-in.
void CElitePirate::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
}

// vtable slot 11 = fn_15_8294 (0x12C): the Echoed/dark render flavour (it reads the shield timer at
// 0xc64, the electric timer at 0xa3c and CColor::Black).
void CElitePirate::Render(const CStateManager& mgr) const {
  StandIn("CElitePirate::Render (vtable slot 11, fn_15_8294)");
  CPatterned::Render(mgr);
}

// vtable slot 28 = fn_15_7BFC (0x2F0): its jump table jumptable_15_data_864 dispatches the user
// events, so it is DoUserAnimEvent.
void CElitePirate::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                   EUserEventType type, float dt) {
  StandIn("CElitePirate::DoUserAnimEvent (vtable slot 28, fn_15_7BFC)");
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

// vtable slot 41 = fn_15_4C (0x10): retail's body stores one rodata constant (lbl_8041AAB8) into
// the damage cooldown timer at 0x448 and ignores the damage, exactly as CSpacePirate's does.
void CElitePirate::TakeDamage(const CVector3f& direction, float magnitude) {
  StandIn("CElitePirate::TakeDamage (vtable slot 41, fn_15_4C)");
  CPatterned::TakeDamage(direction, magnitude);
}

// Retail fn_15_178. Property ids and order are the generated SLdrElitePirate reader's, which the
// analysis checked against the REL's switch id by id: the loader's dispatch tree reads all 43.
static CEntity* PortLoadElitePirate(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrElitePirate sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrElitePirate.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL checks the optional's valid byte (r1+0x1fc) and returns null when it is empty.
    return nullptr;
  }

  CElitePirateData data;
  // data+0x00 is the loader's first integer argument (r4 = patterned+0x1e4), which the class's
  // constructor loads as the 'FSM2' token at 0xc30 - i.e. the patterned info's second state
  // machine asset. data+0x08 is a CDamageInfo copied out of the builder's damage argument, i.e.
  // the melee damage. The rest of the REL builder's parameter list could not be mapped back to
  // properties (see docs/module-analysis/ElitePirate.md), so the remaining fields are assigned by
  // name and nothing in this milestone reads them.
  data.mStateMachine = sldrThis.patterned.stateMachine2;
  data.mMeleeDamage = LdrToDamageInfo(sldrThis.meleeDamage);
  data.mMaxMeleeRange = sldrThis.maxMeleeRange;
  data.mMinShockwaveRange = sldrThis.minShockwaveRange;
  data.mMaxShockwaveRange = sldrThis.maxShockwaveRange;
  data.mMinRocketRange = sldrThis.minRocketRange;
  data.mMaxRocketRange = sldrThis.maxRocketRange;
  data.mUnknown5236C2B6 = sldrThis.unknown_0x5236c2b6;
  data.mUnknown01EAAB17 = sldrThis.unknown_0x01eaab17;
  data.mShieldedModel = sldrThis.shieldedModel;
  data.mShieldedSkinRules = sldrThis.shieldedSkinRules;
  data.mDarkShield = sldrThis.darkShield;
  data.mDarkShieldSound = static_cast< ushort >(sldrThis.darkShieldSound);
  data.mDarkShieldPop = sldrThis.darkShieldPop;
  data.mLightShield = sldrThis.lightShield;
  data.mLightShieldSound = static_cast< ushort >(sldrThis.lightShieldSound);
  data.mLightShieldPop = sldrThis.lightShieldPop;
  data.mTauntInterval = sldrThis.tauntInterval;
  data.mTauntVariance = sldrThis.tauntVariance;
  data.mSingleShockWaveInfo = CShockWaveInfo(sldrThis.singleShockWaveInfo);
  data.mDoubleShockWaveInfo = CShockWaveInfo(sldrThis.doubleShockWaveInfo);
  data.mUnknown28B39197 = sldrThis.unknown_0x28b39197;
  data.mUnknownE27DE71B = sldrThis.unknown_0xe27de71b;
  data.mUnknown665E7ACE = sldrThis.unknown_0x665e7ace;
  data.mUnknownACD4D06D = sldrThis.unknown_0xacd4d06d;
  data.mRocket = sldrThis.rocket;
  data.mRocketDamage = LdrToDamageInfo(sldrThis.rocketDamage);
  data.mUnknown624222F8 = sldrThis.unknown_0x624222f8;
  data.mUnknown31E43A1C = sldrThis.unknown_0x31e43a1c;
  data.mRepeatedAttackChance = sldrThis.repeatedAttackChance;
  data.mEnergyAbsorbDuration = sldrThis.energyAbsorbDuration;
  data.mEnergyAbsorbVariance = sldrThis.energyAbsorbVariance;
  data.mEnergyAttractionForce = sldrThis.energyAttractionForce;
  data.mAlwaysFF = sldrThis.alwaysFF;
  data.mAlwaysFF23F5E1EE = sldrThis.alwaysFF_0x23f5e1ee;
  data.mRocketLauncherActorInfo = LdrToActorParameters(sldrThis.rocketLauncherActorInfo);
  data.mRocketLauncherAnimInfo = LdrToAnimationParameters(sldrThis.rocketLauncherAnimInfo);
  data.mUnknown7E6E0D38 = LdrToAnimationParameters(sldrThis.unknown_0x7e6e0d38);
  data.mVisorElectricEffect = sldrThis.visorElectricEffect;
  data.mSoundVisorElectric = static_cast< ushort >(sldrThis.sound_VisorElectric);
  // data.mPartAsset is left invalid: the constructor reads data+0x34 as an asset it loads under
  // the 'PART' tag, but which property reaches that offset is not established.

  return rs_new CElitePirate(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                             LdrToEntityInfo(info, sldrThis.editorProperties),
                             LdrToTransform4f(sldrThis.editorProperties), *modelData,
                             LdrToPatternedInfo(sldrThis.patterned, &sldrThis.ingPossessionData),
                             LdrToActorParameters(sldrThis.actorInformation), data);
}

static SElitePirate_FuncPtrs REL_loader_ElitePirate;

// Retail's RELMain calls fn_15_148, which stores the loader fn_15_178 into lbl_15_bss_0 (the
// module's SElitePirate_FuncPtrs, a single loader slot) and calls the DOL's SetSElitePirate_FuncPtrs
// (fn_80218AD4). The proxy in SElitePirate_FuncPtrs.cpp calls through it.
void RELMain() {
  REL_loader_ElitePirate.mLoader = PortLoadElitePirate;
  SetSElitePirate_FuncPtrs(&REL_loader_ElitePirate);
}

void RELExit() { SetSElitePirate_FuncPtrs(nullptr); }
