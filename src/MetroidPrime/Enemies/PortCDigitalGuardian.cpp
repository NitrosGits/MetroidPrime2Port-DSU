// Host body for the DigitalGuardian REL (module 14), milestone 1: the actor loads, registers and
// thinks. Every vtable override that belongs to retail's AI forwards to CPatterned behind a
// once-only StandIn log, and the REL function it stands in for is named on each. The state
// machine's tables are registered (fn_14_9DFC installs the 12 triggers, 14 states and 13 codes),
// but the 39 functions they point at are stand-ins that log the retail function they stand in
// for and return the neutral value. The vortex (fn_14_458C), the knee armour, the transmission
// beacon, the toe targets and the head module (CDigitalGuardianHead, guest 0x10F8) are not
// written yet - docs/module-analysis/DigitalGuardian.md section i has the plan.
//
// Which methods the REL overrides is not guessed: its vtable lbl_14_data_484 lists them by address
// (/tmp/opencode/pseudo/DigitalGuardian and the module's asm dump) and the slots line up with
// CPatterned's declaration order, the inherited entries being spelled out by name in the same
// table. The tiny accessors at the head of the module's .text (fn_14_0, fn_14_8, fn_14_78,
// fn_14_88, fn_14_90, fn_14_98, fn_14_A0, fn_14_A8, fn_14_B0, fn_14_C0, fn_14_CC, fn_14_D8,
// fn_14_E0, fn_14_E8) have four-to-twelve-instruction bodies, so they are real rather than
// stand-ins.
#include "MetroidPrime/Enemies/CDigitalGuardian.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrDigitalGuardian.hpp"
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
  fprintf(stderr, "[dgrd] %s is a port stand-in (see PortCDigitalGuardian.cpp)\n", what);
}

// The same, for the state machine's own functions: the message names the retail function the
// entry stands in for, so the log says which of the module's 39 FSM functions the run reached.
void StandInFn(const char* fn) {
  static const char* sSeen[64] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == fn) {
      return;
    }
  }
  if (sCount < 64) {
    sSeen[sCount++] = fn;
  }
  fprintf(stderr, "[dg] stand-in %s\n", fn);
}

// The body's three FSM tables. Retail builds them in a static initialiser (fn_14_D904, .ctors):
// the tables are lbl_14_data_9C (12 triggers, .data 0x9C), lbl_14_data_204 (14 states, .data
// 0x204) and lbl_14_data_380 (13 codes, .data 0x380), each entry 16 bytes
// {name, 0, 0xFFFFFFFF, fn}, and the function pointers come from the three-word records
// {0, 0xFFFFFFFF, fn} in .data 0xC..0x374 (fn_14_D904's stores at 0xDB50..0xDCC8 for the
// triggers, 0xDCD0..0xDEB8 for the states and 0xDEBC..0xE00C for the codes).
//
// On the host the table addresses are irrelevant: the state machine matches a node by its name,
// so the name strings are retail's and the entry's function is the module's member function.
// The members are cast the way CAIMannedTurret and the SpacePirate port do it.
#define DG_TRIGGER(name)                                                                           \
  { #name, reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CDigitalGuardian::name) }
#define DG_STATE(name)                                                                             \
  { #name, reinterpret_cast< CPatterned::StateMachine::StateFunc >(&CDigitalGuardian::name) }
#define DG_CODE(name)                                                                              \
  { #name, reinterpret_cast< CPatterned::StateMachine::CodeFunc >(&CDigitalGuardian::name) }

CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    DG_TRIGGER(StateOver),           DG_TRIGGER(IsIdle),
    DG_TRIGGER(HasFaceTarget),       DG_TRIGGER(KneeDestroyed),
    DG_TRIGGER(AllKneesDestroyed),   DG_TRIGGER(ShouldProtectKnees),
    DG_TRIGGER(ShouldVortexAttack),  DG_TRIGGER(ShouldMeleeAttack),
    DG_TRIGGER(ShouldProjectileAttack), DG_TRIGGER(ShouldJumpAttack),
    DG_TRIGGER(VortexStruckPlayer),  DG_TRIGGER(BreakOutOfVortex),
};

CPatterned::StateMachine::SStateFunction skStates[] = {
    DG_STATE(Start),               DG_STATE(Idle),
    DG_STATE(FaceTarget),          DG_STATE(VortexAttack),
    DG_STATE(VortexEndAttack),     DG_STATE(VortexRetreat),
    DG_STATE(ExitVortexAttack),    DG_STATE(MeleeAttackTelegraph),
    DG_STATE(MeleeAttack),         DG_STATE(ProjectileAttack),
    DG_STATE(JumpAttack),          DG_STATE(KneeDestroyedReaction),
    DG_STATE(CrippledReaction),    DG_STATE(Crippled),
};

CPatterned::StateMachine::SCodeFunction skCodes[] = {
    DG_CODE(SetupBossEnergyBar),      DG_CODE(ResetProtectKneesTime),
    DG_CODE(ResetJumpTime),           DG_CODE(ResetAttackTimes),
    DG_CODE(SetProtectKneesFaceTarget), DG_CODE(SetVortexAttackMode),
    DG_CODE(SetNormalAttackMode),     DG_CODE(DisableHeadAttacks),
    DG_CODE(ResetHeadAttacks),        DG_CODE(SelectExitVortexTarget),
    DG_CODE(SelectBestMeleeAttackLeg), DG_CODE(SelectMeleeAttackRange),
    DG_CODE(SetJumpFaceTarget),
};

#undef DG_TRIGGER
#undef DG_STATE
#undef DG_CODE

} // namespace

// fn_14_B100. The base call is the retail one: EPatternedAI 9 (li r4, 0x9 at 0x0000B154),
// flavor 0 (li r7, 0x0 at 0x0000B13C), movement 0 / collider 1 / body 1 (the stack words at
// 0x0000B150..0x0000B15C), all passed to CPatterned at 0x0000B164.
CDigitalGuardian::CDigitalGuardian(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                                   const CTransform4f& xf, const CModelData& modelData,
                                   const CActorParameters& actParms, const CPatternedInfo& pInfo,
                                   const CDigitalGuardianData& data)
: CPatterned(static_cast< EPatternedAI >(9), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mData(data) // guest 0x7C0: the 0x640-byte parameter block fn_14_C68C copies in
{
  // The retail constructor also builds the secondary struct at 0xE00 from the patterned info
  // (fn_14_C57C), the four vulnerabilities at 0x1014/0x1044/0x1110/0x1188, the five models at
  // 0xE6C.. (fn_14_6864) and the 'PART' token at 0x1100. They belong to the state machine and the
  // render paths this milestone does not enter, so the port keeps the parameter block only.
}

CDigitalGuardian::~CDigitalGuardian() {}

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove / Death / KnockBack all dereference the current state. The analysis does not say
// where retail activates it, so the port activates on kSM_Create and, as the other patterned
// actors in this port do, again on demand in every entry point that can run first. The body
// controller itself is always built by CPatterned's constructor (CPatterned.cpp's BuildBodyController
// call), so the port limit here is only the activation point, not the pointer.
void CDigitalGuardian::EnsureBodyActive(CStateManager& mgr) {
  if (!BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

// vtable slot 4 = fn_14_A688.
void CDigitalGuardian::PreThink(float dt, CStateManager& mgr) {
  StandIn("CDigitalGuardian::PreThink (vtable slot 4, fn_14_A688)");
  EnsureBodyActive(mgr);
  CPatterned::PreThink(dt, mgr);
}

// vtable slot 5 = fn_14_A5BC. It tests the actor's active bit (the byte at 0x20, bit 0, read at
// 0x0000A5E0) and returns when it is clear, then runs CPatterned::Think.
void CDigitalGuardian::Think(float dt, CStateManager& mgr) {
  StandIn("CDigitalGuardian::Think (vtable slot 5, fn_14_A5BC)");
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
}

// vtable slot 6 = fn_14_A6A8 (0x4F0), the biggest body function: it switches on the script
// message's four-character code.
void CDigitalGuardian::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("CDigitalGuardian::AcceptScriptMsg (vtable slot 6, fn_14_A6A8)");
  CPatterned::AcceptScriptMsg(mgr, msg);
  if (msg.GetMessage() == kSM_Create) {
    EnsureBodyActive(mgr);
  }
}

// vtable slot 9 = fn_14_A384.
void CDigitalGuardian::PreRender(CStateManager& mgr) {
  StandIn("CDigitalGuardian::PreRender (vtable slot 9, fn_14_A384)");
  CPatterned::PreRender(mgr);
}

// vtable slot 10 = fn_14_A004.
void CDigitalGuardian::AddToRenderer(const CStateManager& mgr) const {
  StandIn("CDigitalGuardian::AddToRenderer (vtable slot 10, fn_14_A004)");
  CPatterned::AddToRenderer(mgr);
}

// vtable slot 11 = fn_14_A3D4.
void CDigitalGuardian::Render(const CStateManager& mgr) const {
  StandIn("CDigitalGuardian::Render (vtable slot 11, fn_14_A3D4)");
  CPatterned::Render(mgr);
}

// vtable slot 13 = fn_14_A138.
void CDigitalGuardian::PreRenderAllViewports(CStateManager& mgr) {
  StandIn("CDigitalGuardian::PreRenderAllViewports (vtable slot 13, fn_14_A138)");
  CPatterned::PreRenderAllViewports(mgr);
}

// vtable slot 15 = fn_14_10C (0x2C): it calls the vtable's HealthInfo slot (0x38) and returns
// its result, which is what the base class's own accessor does.
const CHealthInfo* CDigitalGuardian::GetHealthInfo() const { return CPatterned::GetHealthInfo(); }

// vtable slot 16 = fn_14_9FE4.
const CDamageVulnerability* CDigitalGuardian::GetDamageVulnerability() const {
  StandIn("CDigitalGuardian::GetDamageVulnerability (vtable slot 16, fn_14_9FE4)");
  return CPatterned::GetDamageVulnerability();
}

// vtable slot 18 = fn_14_10: it builds the box from CPhysicsActor::GetBoundingBox.
rstl::optional_object< CAABox > CDigitalGuardian::GetTouchBounds() const {
  StandIn("CDigitalGuardian::GetTouchBounds (vtable slot 18, fn_14_10)");
  return CPatterned::GetTouchBounds();
}

// vtable slot 21 = fn_14_9E88.
CVector3f CDigitalGuardian::GetAimPosition(const CStateManager& mgr, float dt) const {
  StandIn("CDigitalGuardian::GetAimPosition (vtable slot 21, fn_14_9E88)");
  return CPatterned::GetAimPosition(mgr, dt);
}

// vtable slot 28 = fn_14_A468: it dispatches through the jump table jumptable_14_data_5CC, whose
// 25 entries are the toe-target effect handlers.
void CDigitalGuardian::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                       EUserEventType type, float dt) {
  StandIn("CDigitalGuardian::DoUserAnimEvent (vtable slot 28, fn_14_A468)");
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

// vtable slot 29 = fn_14_9DBC.
CScannableObjectInfo* CDigitalGuardian::GetScannableObjectInfo() const {
  StandIn("CDigitalGuardian::GetScannableObjectInfo (vtable slot 29, fn_14_9DBC)");
  return CPatterned::GetScannableObjectInfo();
}

// vtable slot 41 = fn_14_78 (0x10): retail stores skDamageHitTime into the damage-cooldown timer
// at 0x448 and ignores the damage, exactly as CPatterned::TakeDamage does.
void CDigitalGuardian::TakeDamage(const CVector3f& direction, float magnitude) {
  CPatterned::TakeDamage(direction, magnitude);
}

// vtable slot 45 = fn_14_F0: it returns the base's origin vector (the three floats at 0x454).
CVector3f CDigitalGuardian::GetOrigin(const CStateManager& mgr, const CTeamAiRole& role,
                                      const CVector3f& point) const {
  StandIn("CDigitalGuardian::GetOrigin (vtable slot 45, fn_14_F0)");
  return CPatterned::GetOrigin(mgr, role, point);
}

// vtable slot 47 = fn_14_A3A4.
void CDigitalGuardian::RenderSystemsToBeDrawnLast(const CStateManager& mgr, uint mask,
                                                  uint target) const {
  StandIn("CDigitalGuardian::RenderSystemsToBeDrawnLast (vtable slot 47, fn_14_A3A4)");
  CPatterned::RenderSystemsToBeDrawnLast(mgr, mask, target);
}

// vtable slot 60 = fn_14_9DFC (0x9DFC..0x9FE4): the base's setup first (the call at 0x9ED0), then
// the module's three tables on the state manager at this+0x350 - the triggers (lbl_14_data_9C,
// 12 = 0xC) through the vtable slot at +0x18 (the call at 0x9EE8), the states (lbl_14_data_204,
// 14 = 0xE) through +0x14 (0x9F10) and the codes (lbl_14_data_380, 13 = 0xD) through +0x1C
// (0x9F30).
void CDigitalGuardian::SetupStateMachine(CStateManager& mgr) {
  CPatterned::SetupStateMachine(mgr);
  StateMachine* stateMachine = mStateMachine.get();
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  // The host's FSM1 does not dispatch code nodes yet (TStateMachineState::SetCodeFunctions is
  // empty), so this only records the table retail registers.
  stateMachine->SetCodeFunctions(skCodes, ARRAY_SIZE(skCodes));
}

// ---------------------------------------------------------------------------------------------
// The body's FSM functions. Retail's entries are the module's own functions; their bodies are
// not ported in this slice, so each one logs the retail function it stands in for and returns
// the neutral value (false for a trigger, nothing for a state or a code).
//
// The 12 triggers of lbl_14_data_9C (.data 0x9C; fn_14_D904's stores at 0xDB50..0xDCC8).
bool CDigitalGuardian::StateOver(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_14_9AA8");
  return false;
}

bool CDigitalGuardian::IsIdle(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_14_9A9C");
  return false;
}

bool CDigitalGuardian::HasFaceTarget(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_14_9A60");
  return false;
}

bool CDigitalGuardian::KneeDestroyed(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_14_9A48");
  return false;
}

bool CDigitalGuardian::AllKneesDestroyed(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_14_9A0C");
  return false;
}

bool CDigitalGuardian::ShouldProtectKnees(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_14_9994");
  return false;
}

bool CDigitalGuardian::ShouldVortexAttack(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_14_9930");
  return false;
}

bool CDigitalGuardian::ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_14_97F0");
  return false;
}

bool CDigitalGuardian::ShouldProjectileAttack(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_14_97A4");
  return false;
}

bool CDigitalGuardian::ShouldJumpAttack(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_14_9678");
  return false;
}

bool CDigitalGuardian::VortexStruckPlayer(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_14_966C");
  return false;
}

bool CDigitalGuardian::BreakOutOfVortex(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_14_9638");
  return false;
}

// The 14 states of lbl_14_data_204 (.data 0x204; fn_14_D904's stores at 0xDCD0..0xDEB8).
void CDigitalGuardian::Start(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_14_9634");
}

void CDigitalGuardian::Idle(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_14_95B8");
}

void CDigitalGuardian::FaceTarget(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_14_94BC");
}

void CDigitalGuardian::VortexAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_14_9260");
}

void CDigitalGuardian::VortexEndAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_14_9164");
}

void CDigitalGuardian::VortexRetreat(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_14_9070");
}

void CDigitalGuardian::ExitVortexAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_14_8E44");
}

void CDigitalGuardian::MeleeAttackTelegraph(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_14_8CCC");
}

void CDigitalGuardian::MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_14_89D4");
}

void CDigitalGuardian::ProjectileAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_14_8794");
}

void CDigitalGuardian::JumpAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_14_8580");
}

void CDigitalGuardian::KneeDestroyedReaction(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_14_8090");
}

void CDigitalGuardian::CrippledReaction(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_14_7D8C");
}

void CDigitalGuardian::Crippled(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_14_7CB4");
}

// The 13 codes of lbl_14_data_380 (.data 0x380; fn_14_D904's stores at 0xDEBC..0xE00C).
void CDigitalGuardian::SetupBossEnergyBar(CStateManager& mgr, float dt) {
  StandInFn("fn_14_7BD4");
}

void CDigitalGuardian::ResetProtectKneesTime(CStateManager& mgr, float dt) {
  StandInFn("fn_14_7B90");
}

void CDigitalGuardian::ResetJumpTime(CStateManager& mgr, float dt) {
  StandInFn("fn_14_7B84");
}

void CDigitalGuardian::ResetAttackTimes(CStateManager& mgr, float dt) {
  StandInFn("fn_14_7B24");
}

void CDigitalGuardian::SetProtectKneesFaceTarget(CStateManager& mgr, float dt) {
  StandInFn("fn_14_77EC");
}

void CDigitalGuardian::SetVortexAttackMode(CStateManager& mgr, float dt) {
  StandInFn("fn_14_775C");
}

void CDigitalGuardian::SetNormalAttackMode(CStateManager& mgr, float dt) {
  StandInFn("fn_14_76D0");
}

void CDigitalGuardian::DisableHeadAttacks(CStateManager& mgr, float dt) {
  StandInFn("fn_14_768C");
}

void CDigitalGuardian::ResetHeadAttacks(CStateManager& mgr, float dt) {
  StandInFn("fn_14_7648");
}

void CDigitalGuardian::SelectExitVortexTarget(CStateManager& mgr, float dt) {
  StandInFn("fn_14_7600");
}

void CDigitalGuardian::SelectBestMeleeAttackLeg(CStateManager& mgr, float dt) {
  StandInFn("fn_14_7504");
}

void CDigitalGuardian::SelectMeleeAttackRange(CStateManager& mgr, float dt) {
  StandInFn("fn_14_73D4");
}

void CDigitalGuardian::SetJumpFaceTarget(CStateManager& mgr, float dt) {
  StandInFn("fn_14_7208");
}

// vtable slot 71 = fn_14_9CA4.
CAABox CDigitalGuardian::GetScanVisorRenderBounds(const CStateManager& mgr) const {
  StandIn("CDigitalGuardian::GetScanVisorRenderBounds (vtable slot 71, fn_14_9CA4)");
  return CPatterned::GetScanVisorRenderBounds(mgr);
}

// vtable slot 72 = fn_14_9ABC.
void CDigitalGuardian::ScanVisorRender(const CStateManager& mgr, const CTransform4f& xf,
                                       const CModelFlags& flags) const {
  StandIn("CDigitalGuardian::ScanVisorRender (vtable slot 72, fn_14_9ABC)");
  CPatterned::ScanVisorRender(mgr, xf, flags);
}

// vtable slot 73 = fn_14_D8 (0x8): it returns the base member at 0x754, which is what the base
// class's own accessor returns.
const rstl::optional_object< TCachedToken< CGenDescription > >&
CDigitalGuardian::GetDeathExplosionParticle() const {
  return CPatterned::GetDeathExplosionParticle();
}

// ---------------------------------------------------------------------------------------------
// The module's loaders (fn_14_1B8 and fn_14_DF7C) and entry points.

// Retail fn_14_1B8. The property ids and their order are the generated SLdrDigitalGuardian
// reader's, which the analysis checked against the REL's switch id by id: the loader's dispatch
// tree reads all four blocks (0x255A4580 editor properties, 0xB3774750 patterned AI, 0x7E397FED
// actor parameters, 0xC55918CC guardian data).
static CEntity* PortLoadDigitalGuardian(CStateManager& mgr, CInputStream& input,
                                         CEntityInfo& info) {
  SLdrDigitalGuardian sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrDigitalGuardian.inc"

  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL checks the optional's valid byte (sp+0xF8, read at 0x0000036C) and the model copy's
    // own valid byte (sp+0x148, read at 0x00000384), and returns null when the model is empty.
    return nullptr;
  }

  // The parameter block, field by field: the loader's stack struct is copied into the object
  // verbatim by fn_14_C68C, so the host keeps the loader's own values.
  const SLdrDigitalGuardianData& sldr = sldrThis.digitalGuardianProperties;
  CDigitalGuardianData data;
  data.mScannableInfoCrippled = sldr.scannableInfoCrippled;
  data.mNearLegStabRange = sldr.nearLegStabRange;
  data.mFarLegStabRange = sldr.farLegStabRange;
  data.mMinLegStabAttackTime = sldr.minLegStabAttackTime;
  data.mLegStabDamage = sldr.legStabDamage;
  data.mToeTargetHP = sldr.toeTargetHP;
  data.mToeTargetModel = sldr.toeTargetModel;
  data.mToeTargetExplosion = sldr.toeTargetExplosion;
  data.mSoundToeTarget = sldr.sound_ToeTarget;
  data.mSoundToeTargetAttack = sldr.sound_ToeTargetAttack;
  data.mSoundToeTargetExplosion = sldr.sound_ToeTargetExplosion;
  data.mSoundToeTargetHit = sldr.sound_ToeTargetHit;
  data.mSoundShockWave = sldr.sound_ShockWave;
  data.mShockWaveInfo = sldr.shockWaveInfo;
  data.mVortexAttackDuration = sldr.vortexAttackDuration;
  data.mVortexAttractionForce = sldr.vortexAttractionForce;
  data.mMaxVortexAttractionDistance = sldr.maxVortexAttractionDistance;
  data.mVortexLinearVelocity = sldr.vortexLinearVelocity;
  data.mVortexLinearAcceleration = sldr.vortexLinearAcceleration;
  data.mVortexDamage = sldr.vortexDamage;
  data.mUnknownFB5263E8 = sldr.unknown_0xfb5263e8;
  data.mUnknown6AAF33E3 = sldr.unknown_0x6aaf33e3;
  data.mUnknown4F5D725C = sldr.unknown_0x4f5d725c;
  data.mSoundVortexFlash = sldr.sound_VortexFlash;
  data.mLegModel = sldr.legModel;
  data.mShinArmor = sldr.shinArmor;
  data.mKneeArmorHP = sldr.kneeArmorHP;
  data.mSoundKneeArmorHit = sldr.sound_KneeArmorHit;
  data.mSoundKneeVulnerable = sldr.sound_KneeVulnerable;
  data.mKneeArmor = sldr.kneeArmor;
  data.mKneeEchoParameters = sldr.kneeEchoParameters;
  data.mUnknownA324E26C = sldr.unknown_0xa324e26c;
  data.mUnknown6A754EBD = sldr.unknown_0x6a754ebd;
  data.mJumpTimer = sldr.jumpTimer;
  data.mUnknown8106CDA9 = sldr.unknown_0x8106cda9;
  data.mUnknown9E1B8105 = sldr.unknown_0x9e1b8105;
  data.mUnknownA08FCC70 = sldr.unknown_0xa08fcc70;
  data.mUnknown3254A16B = sldr.unknown_0x3254a16b;
  data.mTransmissionBeacon = sldr.transmissionBeacon;
  data.mTransmissionBeaconFx = sldr.transmissionBeaconFx;
  data.mTransmissionBeaconEchoParameters = sldr.transmissionBeaconEchoParameters;
  data.mSoundTransmissionBeacon = sldr.sound_TransmissionBeacon;
  data.mSoundBeaconExtract = sldr.sound_BeaconExtract;
  data.mSoundBeaconRetract = sldr.sound_BeaconRetract;
  data.mTransmissionBeaconHP = sldr.transmissionBeaconHP;
  data.mTransmissionBeaconExplosion = sldr.transmissionBeaconExplosion;
  data.mTransmissionBeaconEchoHit = sldr.transmissionBeaconEchoHit;
  data.mSoundBeaconExplode = sldr.sound_BeaconExplode;
  data.mSoundBeaconHit = sldr.sound_BeaconHit;
  data.mKneeVulnerability = sldr.kneeVulnerability;
  data.mVortexVulnerability = sldr.vortexVulnerability;
  data.mToeTargetVulnerability = sldr.toeTargetVulnerability;

  return rs_new CDigitalGuardian(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                                  LdrToEntityInfo(info, sldrThis.editorProperties),
                                  LdrToTransform4f(sldrThis.editorProperties), *modelData,
                                  LdrToActorParameters(sldrThis.actorInformation),
                                  LdrToPatternedInfo(sldrThis.patterned, nullptr), data);
}

// The head module (CDigitalGuardianHead, guest 0x10F8, script type DGHD) is not ported yet: its
// own class, its 0x5B0-byte parameter block and its 31 vtable overrides are the WP3 work
// (docs/module-analysis/DigitalGuardian.md section i). The loader slot is filled so the module's
// table is complete, and it logs once and returns null, which the script loader helper reads as
// "skip this object" (src/MetroidPrime/CScriptObjectLoaderHelper.cpp skips the rest of the
// object's bytes).
static CEntity* PortLoadDigitalGuardianHead(CStateManager& mgr, CInputStream& input,
                                             CEntityInfo& info) {
  StandIn("CDigitalGuardianHead loader (module 14's DGHD actor is not ported yet)");
  return nullptr;
}

static SDigitalGuardian_FuncPtrs REL_loader_DigitalGuardian;

// Retail's RELMain calls fn_14_17C, which stores the two loaders (fn_14_1B8 and fn_14_DF7C) into
// lbl_14_bss_C (the module's SDigitalGuardian_FuncPtrs) and calls the DOL's
// SetSDigitalGuardian_FuncPtrs (fn_8021F9B0). The proxy in SDigitalGuardian_FuncPtrs.cpp calls
// through it.
void RELMain() {
  REL_loader_DigitalGuardian.mLoadDigitalGuardian = PortLoadDigitalGuardian;
  REL_loader_DigitalGuardian.mLoadDigitalGuardianHead = PortLoadDigitalGuardianHead;
  SetSDigitalGuardian_FuncPtrs(&REL_loader_DigitalGuardian);
}

void RELExit() { SetSDigitalGuardian_FuncPtrs(nullptr); }
