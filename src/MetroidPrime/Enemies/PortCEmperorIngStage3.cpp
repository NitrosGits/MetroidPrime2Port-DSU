// Host body for the EmperorIngStage3 REL (module 18), milestone 1: the mutated Emperor Ing
// loads, stands and thinks. Every vtable override that belongs to retail's AI forwards to
// CPatterned behind a once-only StandIn log, and the REL function it stands in for is named on
// each. The state machine (fn_18_943C installs the 21 triggers, 21 states and 2 code functions),
// the four attacks (melee, stampede, jump slide, ground pound), the two beams, the portal attack
// and the light swarm are milestone 2 - docs/module-analysis/EmperorIngStage3.md has the plan.
//
// Which methods the REL overrides is not guessed: its vtable lbl_18_data_4F0 lists them by
// address, and the slots line up with CPatterned's declaration order (the inherited entries are
// spelled out by name in the same table). The tiny accessors at the head of the module's .text
// (fn_18_0, fn_18_8, fn_18_44, fn_18_4C, fn_18_54, fn_18_5C, fn_18_64, fn_18_6C, fn_18_7C,
// fn_18_88, fn_18_90 and fn_18_9044) have four-to-twelve-instruction bodies, so they are real
// rather than stand-ins.
#include "MetroidPrime/Enemies/CEmperorIngStage3.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrEmperorIngStage3.hpp"
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
  fprintf(stderr, "[emp3] %s is a port stand-in (see PortCEmperorIngStage3.cpp)\n", what);
}

// The same, for the state machine's own functions: the message names the retail function the
// entry stands in for, so the log says which of the module's FSM functions the run reached.
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
  fprintf(stderr, "[emp3] stand-in %s\n", fn);
}

// The body's three FSM tables. Retail builds them in a static initialiser (fn_18_E180): the
// tables are lbl_18_data_100 (21 states, .data 0x100), lbl_18_data_34C (21 triggers, .data 0x34C)
// and lbl_18_data_4B4 (2 codes, .data 0x4B4), each entry 16 bytes {name, 0, 0xFFFFFFFF, fn}, and
// the function pointers come from the three-word records {0, 0xFFFFFFFF, fn} in .data: the states'
// twenty-one at 0x4..0xF4, the triggers' twenty-one at 0x250..0x344 and the codes' two at
// 0x49C..0x4A8.
//
// On the host the table addresses are irrelevant: the state machine matches a node by its name,
// so the name strings are retail's and the entry's function is the module's member function.
// The members are cast the way the DigitalGuardian, SpacePirate and EmperorIngStage1 ports do it.
#define EMP3_TRIGGER(name)                                                                          \
  { #name, reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CEmperorIngStage3::name) }
#define EMP3_STATE(name)                                                                            \
  { #name, reinterpret_cast< CPatterned::StateMachine::StateFunc >(&CEmperorIngStage3::name) }
#define EMP3_CODE(name)                                                                             \
  { #name, reinterpret_cast< CPatterned::StateMachine::CodeFunc >(&CEmperorIngStage3::name) }

CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    EMP3_TRIGGER(TooClose),
    EMP3_TRIGGER(ShouldAttack),
    EMP3_TRIGGER(ShouldMeleeAttack),
    EMP3_TRIGGER(ShouldTaunt),
    EMP3_TRIGGER(ShouldStampede),
    EMP3_TRIGGER(ShouldDodge),
    EMP3_TRIGGER(ShouldChangeVulnerabilityToVulnerable),
    EMP3_TRIGGER(ShouldChangeVulnerabilityToInvulnerable),
    EMP3_TRIGGER(ShouldDarkBeamAttack),
    EMP3_TRIGGER(ShouldLightBeamAttack),
    EMP3_TRIGGER(ShouldLightSwarmAttack),
    EMP3_TRIGGER(ShouldGroundPound),
    EMP3_TRIGGER(ShouldGenerateDarkFliers),
    EMP3_TRIGGER(ShouldPortalAttack),
    EMP3_TRIGGER(ShouldJumpSlideAttack),
    EMP3_TRIGGER(ShouldGrowHeart),
    EMP3_TRIGGER(JumpSlideAttackOver),
    EMP3_TRIGGER(ShouldEyeShut),
    EMP3_TRIGGER(AbortedJumpSlide),
    EMP3_TRIGGER(ShouldJumpAttack),
    EMP3_TRIGGER(ShouldStampedeReact),
};

CPatterned::StateMachine::SStateFunction skStates[] = {
    EMP3_STATE(Start),
    EMP3_STATE(Idle),
    EMP3_STATE(MeleeAttack),
    EMP3_STATE(GainDistance),
    EMP3_STATE(Stampede),
    EMP3_STATE(Dodge),
    EMP3_STATE(Taunt),
    EMP3_STATE(ChangeVulnerability),
    EMP3_STATE(EyeShutResetVulnerability),
    EMP3_STATE(DarkBeamAttack),
    EMP3_STATE(LightBeamAttack),
    EMP3_STATE(LightSwarmAttack),
    EMP3_STATE(GroundPound),
    EMP3_STATE(GenerateDarkFliers),
    EMP3_STATE(PortalAttack),
    EMP3_STATE(JumpSlideAttack),
    EMP3_STATE(GrowHeart),
    EMP3_STATE(EyeShut),
    EMP3_STATE(JumpAttack),
    EMP3_STATE(StampedeReact),
    EMP3_STATE(Dead),
};

CPatterned::StateMachine::SCodeFunction skCodes[] = {
    EMP3_CODE(ResetAttackTimer),
    EMP3_CODE(ChoseAction),
};

#undef EMP3_TRIGGER
#undef EMP3_STATE
#undef EMP3_CODE

} // namespace

// fn_18_9A8C. The base call is the retail one: EPatternedAI 0x0F (`li r4, 0xf` at 0x9AF0),
// flavor 0 (`li r7, 0x0` at 0x9AC8), movement 0 / collider 1 / body 1 (the constants stored
// to the stack arguments at 0x9AD8..0x9AE8 - kMT_Ground, kCT_One and kBT_BiPedal).
CEmperorIngStage3::CEmperorIngStage3(TUniqueId uid, const rstl::string& name,
                                     const CEntityInfo& info, const CTransform4f& xf,
                                     const CModelData& modelData,
                                     const CActorParameters& actParms,
                                     const CPatternedInfo& pInfo,
                                     const SLdrEmperorIngStage3Data& data)
: CPatterned(static_cast< EPatternedAI >(0x0f), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mVulnerableDamageThreshold(data.vulnerableDamageThreshold) // guest 0x814
, mTauntFrequency(data.tauntFrequency)                       // guest 0x91C
, mYellowHealth(LdrToHealthInfo(data.yellowHealth))           // guest 0x920
, mHealth(LdrToHealthInfo(data.health))                       // guest 0x940
, mRedVulnerability(LdrToDamageVulnerability(                  // guest 0x960
      data.redVulnerability))
, mLightVulnerability(LdrToDamageVulnerability(                // guest 0x990
      data.lightVulnerability))
, mDarkVulnerability(LdrToDamageVulnerability(                 // guest 0x9C0
      data.darkVulnerability))
, mMeleeDamage(LdrToDamageInfo(data.meleeDamage))             // guest 0x9F0
, mStampedeDamage(LdrToDamageInfo(data.stampedeDamage))       // guest 0xA0C
, mJumpSlideDamage(LdrToDamageInfo(data.jumpSlideDamage))     // guest 0xA28
, mGroundPoundDamage(LdrToDamageInfo(data.groundPoundDamage)) // guest 0xA44
, mFlags(0)                                                 // guest 0x824
, x7c0_(0)                                                // guest 0x7C0
, mVulnerabilityState(0)                                  // guest 0x7C4
, mAttackTimer(4.f)                                       // guest 0x7E4
, mAction(-1)                                             // guest 0x7E8
{
  // The rest of the retail constructor belongs to the attacks and the state machine: the two beam
  // infos (0xA88 and 0xAFC), the projectile infos (0xA60 and 0xAD4), the four stage records
  // (0xB54, 0xBC0, 0xC2C, 0xC98), the light swarm (0xD34) and the portal data (0xE90). The
  // constructor ends with SetDrawShadow(this, 0) (0xA2D0).
  SetDrawShadow(false);
}

CEmperorIngStage3::~CEmperorIngStage3() {}

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove / Death / KnockBack all dereference the current state. The analysis does not say
// where retail activates it, so the port activates on kSM_Create and, as the other patterned
// actors in this port do, again on demand in every entry point that can run first.
// The null test is a port limit, not retail's: CPatterned's constructor only builds the body
// controller when the patterned info has model data (CPatterned.cpp's HasModelData() test), so an
// actor without a model has no controller to activate.
void CEmperorIngStage3::EnsureBodyActive(CStateManager& mgr) {
  if (BodyController() != nullptr && !BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

// vtable slot 3 = fn_18_47D4: retail's Think, which drives the state machine (the four forms and
// the attack selection). That is milestone 2, so the base class's Think runs.
void CEmperorIngStage3::Think(float dt, CStateManager& mgr) {
  StandIn("CEmperorIngStage3::Think (vtable slot 3, fn_18_47D4)");
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
}

// vtable slot 4 = fn_18_7B14: retail's message switch (the forms and the attacks).
void CEmperorIngStage3::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("CEmperorIngStage3::AcceptScriptMsg (vtable slot 4, fn_18_7B14)");
  CPatterned::AcceptScriptMsg(mgr, msg);
  if (msg.GetMessage() == kSM_Create) {
    EnsureBodyActive(mgr);
  }
}

// vtable slot 7 = fn_18_4650: the base's pre-render and then the damage flash (the model colour
// lerped between black and CPatterned's damage colour by the damage timer).
void CEmperorIngStage3::PreRender(CStateManager& mgr) {
  StandIn("CEmperorIngStage3::PreRender (vtable slot 7, fn_18_4650)");
  CPatterned::PreRender(mgr);
}

// vtable slot 9 = fn_18_47B4: retail's render path.
void CEmperorIngStage3::Render(const CStateManager& mgr) const {
  StandIn("CEmperorIngStage3::Render (vtable slot 9, fn_18_47B4)");
  CPatterned::Render(mgr);
}

// vtable slot 15 = fn_18_B4: a call through the vtable slot 0x38, i.e. CActor::HealthInfo().
const CHealthInfo* CEmperorIngStage3::GetHealthInfo() const {
  return const_cast< CEmperorIngStage3* >(this)->HealthInfo();
}

// vtable slot 18 = fn_18_8: the actor's own bounding box, in an optional.
rstl::optional_object< CAABox > CEmperorIngStage3::GetTouchBounds() const {
  return GetBoundingBox();
}

// vtable slot 21 = fn_18_8E84: retail's aim position.
CVector3f CEmperorIngStage3::GetAimPosition(const CStateManager& mgr, float param) const {
  StandIn("CEmperorIngStage3::GetAimPosition (vtable slot 21, fn_18_8E84)");
  return CPatterned::GetAimPosition(mgr, param);
}

// vtable slot 27 = fn_18_45B4: retail's sorting bounds.
CAABox CEmperorIngStage3::GetSortingBounds(const CStateManager& mgr) const {
  StandIn("CEmperorIngStage3::GetSortingBounds (vtable slot 27, fn_18_45B4)");
  return CPatterned::GetSortingBounds(mgr);
}

// vtable slot 28 = fn_18_9048: retail's user-event switch (the attacks and the forms).
void CEmperorIngStage3::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                        EUserEventType type, float dt) {
  StandIn("CEmperorIngStage3::DoUserAnimEvent (vtable slot 28, fn_18_9048)");
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

// vtable slot 41 = fn_18_9044: the retail body is a single `blr` - the boss takes its damage in
// the state machine, not here.
void CEmperorIngStage3::TakeDamage(const CVector3f& direction, float magnitude) {}

// vtable slot 42 = fn_18_90: `li r3, 0x1` - the actor can be shot.
bool CEmperorIngStage3::CanBeShot(const CStateManager&, int) { return true; }

// vtable slot 43 = fn_18_0: `li r3, 0x1` - the actor listens for noises.
bool CEmperorIngStage3::IsListening() const { return true; }

// vtable slot 44 = fn_18_8E50: a player fire (the noise type 0) sets the flag byte at guest
// 0x824 (its bit 30, the mask 0x02) and the actor says it heard it; every other noise is
// ignored.
bool CEmperorIngStage3::Listen(CStateManager& mgr, const CVector3f& position,
                                EListenNoiseType type) {
  if (type != kLNT_PlayerFire) {
    return false;
  }
  mFlags |= 0x02;
  return true;
}

// vtable slot 45 = fn_18_98: retail returns a vector the manager holds at +0x54; the base class's
// origin (the actor's own translation) is close enough for this milestone.
CVector3f CEmperorIngStage3::GetOrigin(const CStateManager& mgr, const CTeamAiRole& role,
                                       const CVector3f& aimPos) const {
  StandIn("CEmperorIngStage3::GetOrigin (vtable slot 45, fn_18_98)");
  return CPatterned::GetOrigin(mgr, role, aimPos);
}

// vtable slot 50 = fn_18_44: the model alpha byte at guest 0x44F, CPatterned::mColor's alpha
// (CColor at 0x44C; the module never writes it).
uchar CEmperorIngStage3::GetModelAlphau8(const CStateManager& mgr) const { return mColor.GetAlphau8(); }

// vtable slot 55 = fn_18_4C: `li r3, 0x0` - the actor has no projectile info.
CProjectileInfo* CEmperorIngStage3::ProjectileInfo() { return nullptr; }

// vtable slot 56 = fn_18_54: `li r3, 0x0` - the actor has no path search.
CPathFindSearch* CEmperorIngStage3::GetSearchPath() { return nullptr; }

// vtable slot 57 = fn_18_5C: `li r3, 0x0` - the actor has no point path search.
CPathFindPointSearch* CEmperorIngStage3::GetPointSearchPath() { return nullptr; }

// vtable slot 61 = fn_18_64: `li r3, 0x0` - the actor has no rag doll.
CRagDoll* CEmperorIngStage3::GetRagDoll() const { return nullptr; }

// vtable slot 66 = fn_18_6C: stores kInvalidUniqueId, i.e. the actor never reports an attack
// target through this accessor.
TUniqueId CEmperorIngStage3::GetAttackTarget() const { return kInvalidUniqueId; }

// vtable slot 68 = fn_18_7C: the on-ground bit of the byte at guest 0x34C, which is what the base
// class keeps in mOnGround.
bool CEmperorIngStage3::IsOnGround() const { return CPatterned::IsOnGround(); }

// vtable slot 69 = fn_18_8E78: the gravity constant, rodata 0x2A4 = 200.0f.
float CEmperorIngStage3::GetGravityConstant() const { return 200.f; }

// vtable slot 60 = fn_18_943C: CPatterned's own setup first (the call at 0x9450), then the
// module's three name tables are handed to the state machine - the triggers (lbl_18_data_34C, 21
// entries) through the machine's vtable slot +0x18 (0x9470), the states (lbl_18_data_100, 21
// entries) through +0x14 (0x9490) and the codes (lbl_18_data_4B4, 2 entries) through +0x1C
// (0x94B0). That FSM is milestone 2, so only the base setup runs here.
void CEmperorIngStage3::SetupStateMachine(CStateManager& mgr) {
  CPatterned::SetupStateMachine(mgr);
  StateMachine* stateMachine = mStateMachine.get();
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  // The host's FSM1 does not dispatch code nodes yet (TStateMachineState::SetCodeFunctions is
  // empty), so this only records the table retail registers.
  stateMachine->SetCodeFunctions(skCodes, ARRAY_SIZE(skCodes));
}

// The module's state machine functions. The eleven triggers whose retail body is small enough
// are ported; the rest are stand-ins that log the retail function they stand in for and return the
// neutral value (false for a trigger, nothing for a state or a code).
bool CEmperorIngStage3::TooClose(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_18_8DA4");
  return false;
}

// fn_18_8D88 (0x8D88): the attack timer (guest 0x7E4) against zero (rodata 0x258).
bool CEmperorIngStage3::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mAttackTimer < 0.f;
}

bool CEmperorIngStage3::ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_18_8C4C");
  return false;
}

bool CEmperorIngStage3::ShouldTaunt(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_18_8A4C");
  return false;
}

// fn_18_897C (0x897C): the action (guest 0x7E8) is 0.
bool CEmperorIngStage3::ShouldStampede(CStateManager& mgr, const CTriggerData& data) const {
  return mAction == 0;
}

// fn_18_8A34 (0x8A34): the flag byte at guest 0x824, its bit 28 (the mask 0x08).
bool CEmperorIngStage3::ShouldDodge(CStateManager& mgr, const CTriggerData& data) const {
  return (mFlags & 0x08) != 0;
}

// fn_18_8A20 (0x8A20): the vulnerability state (guest 0x7C4) is 1.
bool CEmperorIngStage3::ShouldChangeVulnerabilityToVulnerable(CStateManager& mgr, const CTriggerData& data) const {
  return mVulnerabilityState == 1;
}

bool CEmperorIngStage3::ShouldChangeVulnerabilityToInvulnerable(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_18_898C");
  return false;
}

// fn_18_8968 (0x8968): the action (guest 0x7E8) is 3.
bool CEmperorIngStage3::ShouldDarkBeamAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mAction == 3;
}

// fn_18_8954 (0x8954): the action (guest 0x7E8) is 4.
bool CEmperorIngStage3::ShouldLightBeamAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mAction == 4;
}

bool CEmperorIngStage3::ShouldLightSwarmAttack(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_18_87F0");
  return false;
}

bool CEmperorIngStage3::ShouldGroundPound(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_18_8ACC");
  return false;
}

bool CEmperorIngStage3::ShouldGenerateDarkFliers(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_18_868C");
  return false;
}

// fn_18_8678 (0x8678): the action (guest 0x7E8) is 5.
bool CEmperorIngStage3::ShouldPortalAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mAction == 5;
}

// fn_18_8664 (0x8664): the action (guest 0x7E8) is 1.
bool CEmperorIngStage3::ShouldJumpSlideAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mAction == 1;
}

// fn_18_8648 (0x8648): the vulnerable damage threshold (guest 0x814) against rodata
// 0x310 (1.1920929e-07f).
bool CEmperorIngStage3::ShouldGrowHeart(CStateManager& mgr, const CTriggerData& data) const {
  return mVulnerableDamageThreshold < 1.1920929e-07f;
}

bool CEmperorIngStage3::JumpSlideAttackOver(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_18_8600");
  return false;
}

bool CEmperorIngStage3::ShouldEyeShut(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_18_85A4");
  return false;
}

// fn_18_858C (0x858C): the flag byte at guest 0x825, its bit 29 (the mask 0x04 of that
// byte, the word's 0x400).
bool CEmperorIngStage3::AbortedJumpSlide(CStateManager& mgr, const CTriggerData& data) const {
  return (mFlags & 0x400) != 0;
}

bool CEmperorIngStage3::ShouldJumpAttack(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_18_8518");
  return false;
}

// fn_18_8500 (0x8500): the flag byte at guest 0x825 with its bit 31 cleared (the mask 0x7F
// of that byte, the word's 0x7F00) is not zero.
bool CEmperorIngStage3::ShouldStampedeReact(CStateManager& mgr, const CTriggerData& data) const {
  return (mFlags & 0x7f00) != 0;
}

void CEmperorIngStage3::Start(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_7ADC");
}

void CEmperorIngStage3::Idle(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_788C");
}

void CEmperorIngStage3::MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_76D0");
}

void CEmperorIngStage3::GainDistance(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_7504");
}

void CEmperorIngStage3::Stampede(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_71D8");
}

void CEmperorIngStage3::Dodge(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_6E80");
}

void CEmperorIngStage3::Taunt(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_6D84");
}

void CEmperorIngStage3::ChangeVulnerability(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_6A40");
}

void CEmperorIngStage3::EyeShutResetVulnerability(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_6B80");
}

void CEmperorIngStage3::DarkBeamAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_6904");
}

void CEmperorIngStage3::LightBeamAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_67C0");
}

void CEmperorIngStage3::LightSwarmAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_6698");
}

void CEmperorIngStage3::GroundPound(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_6390");
}

void CEmperorIngStage3::GenerateDarkFliers(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_6258");
}

void CEmperorIngStage3::PortalAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_60AC");
}

void CEmperorIngStage3::JumpSlideAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_5844");
}

void CEmperorIngStage3::GrowHeart(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_5614");
}

void CEmperorIngStage3::EyeShut(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_5264");
}

void CEmperorIngStage3::JumpAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_4EB4");
}

void CEmperorIngStage3::StampedeReact(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_4CC8");
}

void CEmperorIngStage3::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_18_4C3C");
}

void CEmperorIngStage3::ResetAttackTimer(CStateManager& mgr, float dt) {
  StandInFn("fn_18_84A8");
}

void CEmperorIngStage3::ChoseAction(CStateManager& mgr, float dt) {
  StandInFn("fn_18_8200");
}

// ---------------------------------------------------------------------------------------------
// The module's loader (fn_18_C380) and entry points.

static CEntity* PortLoadEmperorIngStage3(CStateManager& mgr, CInputStream& input,
                                          CEntityInfo& info) {
  SLdrEmperorIngStage3 sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrEmperorIngStage3.inc"

  // The REL builds the model with LdrToModelData(scale, the asset id, the patterned info's
  // animation set, true) at 0xC510 and moves the result into the object at 0xC51C.
  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL reads the copy's valid byte (sp+0x148 at 0xC538) and branches to its null
    // return when the model is empty.
    return nullptr;
  }

  // The REL's own allocation is rs_new(0xED0, "CEmperorIngStage3") at 0xC598; the port
  // allocates sizeof(CEmperorIngStage3), which is larger because the host layout is its own.
  return rs_new CEmperorIngStage3(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                                  LdrToEntityInfo(info, sldrThis.editorProperties),
                                  LdrToTransform4f(sldrThis.editorProperties), *modelData,
                                  LdrToActorParameters(sldrThis.actorInformation),
                                  LdrToPatternedInfo(sldrThis.patterned, nullptr), sldrThis.data);
}

static SEmperorIngStage3_FuncPtrs REL_loader_EmperorIngStage3;

// Retail's RELMain calls fn_18_C350, which stores the loader fn_18_C380 into lbl_18_bss_30 (the
// module's SEmperorIngStage3_FuncPtrs) and calls the DOL's SetSEmperorIngStage3_FuncPtrs. The
// proxy in SEmperorIngStage3_FuncPtrs.cpp calls through it.
void RELMain() {
  REL_loader_EmperorIngStage3.mLoader = PortLoadEmperorIngStage3;
  SetSEmperorIngStage3_FuncPtrs(&REL_loader_EmperorIngStage3);
}

void RELExit() { SetSEmperorIngStage3_FuncPtrs(nullptr); }
