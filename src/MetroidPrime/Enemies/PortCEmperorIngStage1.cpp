// Host body for the EmperorIngStage1 REL (module 16), milestone 1: the actor loads, stands and
// thinks. Every vtable override that belongs to retail's AI forwards to CPatterned behind a
// once-only StandIn log, and the REL function it stands in for is named on each. The state
// machine's tables are registered (fn_16_8AF4 installs the 6 triggers, 12 states and 3 code
// functions fn_16_BF88 builds), but the 21 functions they point at are stand-ins that log the
// retail function they stand in for and return the neutral value. The tentacles, the five attacks
// (melee, projectile, warp, beam, taunt) and the heart stage are milestone 2 - docs/module-
// analysis/EmperorIngStage1.md has the plan.
//
// Which methods the REL overrides is not guessed: its vtable lbl_16_data_3C8 lists them by
// address, and the slots line up with CPatterned's declaration order (the inherited entries are
// spelled out by name in the same table). The tiny accessors at the head of the module's .text
// (fn_16_B994, fn_16_B9A4, fn_16_B9AC, fn_16_B9B4, fn_16_B9BC, fn_16_B9CC, fn_16_B9C4, fn_16_B9DC,
// fn_16_B9E8, fn_16_B9F4 and fn_16_B994's neighbours in EmperorIngStage1Accessors.s) have
// four-to-twelve-instruction bodies, so they are real rather than stand-ins.
#include "MetroidPrime/Enemies/CEmperorIngStage1.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrEmperorIngStage1.hpp"
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
  fprintf(stderr, "[emp1] %s is a port stand-in (see PortCEmperorIngStage1.cpp)\n", what);
}

// The same, for the state machine's own functions: the message names the retail function the
// entry stands in for, so the log says which of the module's 21 FSM functions the run reached.
void StandInFn(const char* fn) {
  static const char* sSeen[32] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == fn) {
      return;
    }
  }
  if (sCount < 32) {
    sSeen[sCount++] = fn;
  }
  fprintf(stderr, "[emp1] stand-in %s\n", fn);
}

// The body's three FSM tables. Retail builds them in a static initialiser (fn_16_BF88, .ctors at
// 0x0000C2A4): the tables are lbl_16_data_19C (6 triggers, .data 0x19C), lbl_16_data_94 (12
// states, .data 0x94) and lbl_16_data_220 (3 codes, .data 0x220), each entry 16 bytes
// {name, 0, 0xFFFFFFFF, fn}, and the function pointers come from the three-word records
// {0, 0xFFFFFFFF, fn} in .data: the states' twelve at 0x4..0x88 (fn_16_BF88's stores at
// 0x0000C0C0..0x0000C124 and 0x0000C214), the triggers' six at 0x154..0x190 (0x0000C234..
// 0x0000C284) and the codes' three at 0x1FC..0x214 (0x0000C284..0x0000C294).
//
// On the host the table addresses are irrelevant: the state machine matches a node by its name,
// so the name strings are retail's and the entry's function is the module's member function.
// The members are cast the way the DigitalGuardian and SpacePirate ports do it.
#define EMP1_TRIGGER(name)                                                                          \
  { #name, reinterpret_cast< CPatterned::StateMachine::TriggerFunc >(&CEmperorIngStage1::name) }
#define EMP1_STATE(name)                                                                            \
  { #name, reinterpret_cast< CPatterned::StateMachine::StateFunc >(&CEmperorIngStage1::name) }
#define EMP1_CODE(name)                                                                             \
  { #name, reinterpret_cast< CPatterned::StateMachine::CodeFunc >(&CEmperorIngStage1::name) }

CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    EMP1_TRIGGER(AllTentaclesRetracted),
    EMP1_TRIGGER(HeartStageOver),
    EMP1_TRIGGER(IsDead),
    EMP1_TRIGGER(ShouldMeleeAttack),
    EMP1_TRIGGER(ShouldAttack),
    EMP1_TRIGGER(ShouldTaunt),
};

CPatterned::StateMachine::SStateFunction skStates[] = {
    EMP1_STATE(TrackPlayer),
    EMP1_STATE(BeginHeartStage),
    EMP1_STATE(HeartStage),
    EMP1_STATE(EndHeartStage),
    EMP1_STATE(Dead),
    EMP1_STATE(MeleeAttack),
    EMP1_STATE(Attack),
    EMP1_STATE(ProjectileAttack),
    EMP1_STATE(WarpAttack),
    EMP1_STATE(TauntAttack),
    EMP1_STATE(Taunt),
    EMP1_STATE(JumpRope),
};

CPatterned::StateMachine::SCodeFunction skCodes[] = {
    EMP1_CODE(BeginSubStage),
    EMP1_CODE(ResetTentacles),
    EMP1_CODE(ChooseRandomTentacleCount),
};

#undef EMP1_TRIGGER
#undef EMP1_STATE
#undef EMP1_CODE

} // namespace

// fn_16_900C. The base call is the retail one: EPatternedAI 0x0D (`li r4, 0xd` at 0x00009068),
// flavor 0 (`li r7, 0x0` at 0x0000905C), movement 1 (kMT_Flyer), collider 1 (kCT_One) and body 2
// (kBT_Restricted) - the three constants stored to the stack arguments at 0x00009060..0x00009070.
CEmperorIngStage1::CEmperorIngStage1(TUniqueId uid, const rstl::string& name,
                                     const CEntityInfo& info, const CTransform4f& xf,
                                     const CModelData& modelData,
                                     const CActorParameters& actParms,
                                     const CPatternedInfo& pInfo,
                                     const SLdrEmperorIngStage1Data& data)
: CPatterned(static_cast< EPatternedAI >(0x0d), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Flyer, kCT_One, kBT_Restricted, actParms)
, mHealthInfo(LdrToHealthInfo(data.tentacle.health))            // guest 0xAAC, fn_16_9AB0
, mNormalVulnerability(LdrToDamageVulnerability(                  // guest 0xACC, fn_16_98D0
      data.tentacle.normalVulnerability))
, mWarpVulnerability(LdrToDamageVulnerability(                    // guest 0xAFC, fn_16_98D0
      data.tentacle.warpAttackVulnerability))
, mMeleeVulnerability(LdrToDamageVulnerability(                   // guest 0xB2C, fn_16_98D0
      data.tentacle.meleeAttackVulnerability))
, mProjectileVulnerability(LdrToDamageVulnerability(              // guest 0xB5C, fn_16_98D0
      data.tentacle.projectileAttackVulnerability))
, mMeleeDamage(LdrToDamageInfo(data.melee.damage))               // guest 0xB8C
, mBeamDamage(LdrToDamageInfo(data.beamAttack.damage))            // guest 0xBC4
, mWarpStabDamage(LdrToDamageInfo(data.warp.stabDamage))         // guest 0xC10
, mHeartExposedTime(data.heartExposedTime)                        // guest 0xBA8
, mStayRetractedTime(data.tentacle.stayRetractedTime)            // guest 0xBB4
, mMaxTurnSpeedNormal(data.maxTurnSpeedNormal)                    // guest 0xBF0
, mMaxTurnSpeedMelee(data.maxTurnSpeedMelee)                      // guest 0xBF4
, mTurnSpeedAccel(data.turnSpeedAccel)                            // guest 0xBFC
, mTauntFrequency(data.tauntFrequency)                            // guest 0xC34
, mAttackIntervalMin(data.attackIntervalMin)                      // guest 0xCF4
, mAttackIntervalMax(data.attackIntervalMax)                      // guest 0xCF8
, x7c0_(0)                                                        // guest 0x7C0, fn_16_900C clears it
, x7c4_(0)                                                        // guest 0x7C4, cleared
, x7d4_(0)                                                        // guest 0x7D4, cleared
, x7d8_()                                                         // guest 0x7D8, the tentacle slots
, xa4c_(4.f)                                                      // guest 0xA4C, rodata_38C
, xa50_(0.f)                                                      // guest 0xA50, rodata_380
{
  // The retail constructor stops the actor drawing a shadow (SetDrawShadow(this, 0) at
  // 0x000095E0) and calls the DOL's EnableAllAnimReactions(&this->0x620, 0) at 0x000095EC; the
  // port's CAiKnockBackMgr has no such call yet, so that half is missing.
  // Everything else the retail constructor builds - the shock-wave block at guest 0xC40, the two
  // tokens at guest 0xBB8 and 0xBE0, the projectile info at guest 0xC80 and the beam info at
  // guest 0xCA8 - belongs to the attacks, which this milestone does not enter.
  SetDrawShadow(false);
}

CEmperorIngStage1::~CEmperorIngStage1() {}

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove / Death / KnockBack all dereference the current state. The analysis does not say
// where retail activates it, so the port activates on kSM_Create and, as the other patterned
// actors in this port do, again on demand in every entry point that can run first.
// The null test is a port limit, not retail's: CPatterned's constructor only builds the body
// controller when the patterned info has model data (CPatterned.cpp's HasModelData() test), so an
// actor without a model has no controller to activate.
void CEmperorIngStage1::EnsureBodyActive(CStateManager& mgr) {
  if (BodyController() != nullptr && !BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

// vtable slot 3 = fn_16_56D8 (0xC4): retail's Think. The state machine it drives (the heart
// stage, the tentacles and the attack selection) is milestone 2, so the base class's Think runs.
void CEmperorIngStage1::Think(float dt, CStateManager& mgr) {
  StandIn("CEmperorIngStage1::Think (vtable slot 3, fn_16_56D8)");
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
}

// vtable slot 4 = fn_16_7CE8 (0xC4): retail's message switch (the tentacle and heart messages).
void CEmperorIngStage1::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("CEmperorIngStage1::AcceptScriptMsg (vtable slot 4, fn_16_7CE8)");
  CPatterned::AcceptScriptMsg(mgr, msg);
  if (msg.GetMessage() == kSM_Create) {
    EnsureBodyActive(mgr);
  }
}

// vtable slot 7 = fn_16_52FC (0x20): the retail override is the base class's own PreRender.
void CEmperorIngStage1::PreRender(CStateManager& mgr) { CPatterned::PreRender(mgr); }

// vtable slot 9 = fn_16_531C (0x2B8): retail's render path, which sets up the actor's own
// model state before the base draws it.
void CEmperorIngStage1::Render(const CStateManager& mgr) const {
  StandIn("CEmperorIngStage1::Render (vtable slot 9, fn_16_531C)");
  CPatterned::Render(mgr);
}

// vtable slot 26 = fn_16_7F8C (0x2C4): retail's user-event switch, which drives the tentacles
// and the heart.
void CEmperorIngStage1::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                        EUserEventType type, float dt) {
  StandIn("CEmperorIngStage1::DoUserAnimEvent (vtable slot 26, fn_16_7F8C)");
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

// vtable slot 28 = fn_16_7EA8 (0xC4): retail's own sound handling for the boss.
void CEmperorIngStage1::ProcessSoundEvent(int sfxId, float weight, int flags, float fallOff,
                                          float maxDist, const CSegId& locator, ushort pitchStart,
                                          ushort pitchEnd, float pitchDuration, uchar minVol,
                                          uchar maxVol, float distanceSquared,
                                          const CVector3f& position, int aid, CStateManager& mgr,
                                          bool translateId) {
  StandIn("CEmperorIngStage1::ProcessSoundEvent (vtable slot 28, fn_16_7EA8)");
  CPatterned::ProcessSoundEvent(sfxId, weight, flags, fallOff, maxDist, locator, pitchStart,
                                pitchEnd, pitchDuration, minVol, maxVol, distanceSquared, position,
                                aid, mgr, translateId);
}

// vtable slot 40 = fn_16_B9FC: `li r3, 0x1` - the actor can be shot.
bool CEmperorIngStage1::CanBeShot(const CStateManager&, int) { return true; }

// vtable slot 41 = fn_16_BA04: `li r3, 0x0` - the actor does not listen for noises.
bool CEmperorIngStage1::IsListening() const { return false; }

// vtable slot 58 = fn_16_8AF4 (0x8AF4..0x8B80): CPatterned's own setup first (the call at
// 0x00008B08), then the module's three name tables are handed to the state machine at this+0x350
// - the triggers (lbl_16_data_19C, 6 entries) through the state manager's vtable slot +0x18 (the
// call at 0x00008B20), the states (lbl_16_data_94, 12 entries) through +0x14 (0x00008B40) and
// the codes (lbl_16_data_220, 3 entries) through +0x1C (0x00008B60).
void CEmperorIngStage1::SetupStateMachine(CStateManager& mgr) {
  CPatterned::SetupStateMachine(mgr);
  StateMachine* stateMachine = mStateMachine.get();
  stateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  stateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  // The host's FSM1 does not dispatch code nodes yet (TStateMachineState::SetCodeFunctions is
  // empty), so this only records the table retail registers.
  stateMachine->SetCodeFunctions(skCodes, ARRAY_SIZE(skCodes));
}

// ---------------------------------------------------------------------------------------------
// The body's FSM functions. The four triggers and the two codes the run reaches are retail's
// bodies; the rest are stand-ins that log the retail function they stand in for and return the
// neutral value (false for a trigger, nothing for a state or a code).

// fn_16_4BD4 (the trigger "AllTentaclesRetracted", lbl_16_data_19C entry 0): true while every
// live tentacle's state is 0, i.e. while they are all retracted. The loop runs over the live
// tentacles (the count at guest 0x7D4) and the state is the element's int at +0x28.
bool CEmperorIngStage1::AllTentaclesRetracted(CStateManager& mgr, const CTriggerData& data) const {
  for (int i = 0; i < x7d4_; ++i) {
    if (x7d8_[i].x28_ != 0) {
      return false;
    }
  }
  return true;
}

bool CEmperorIngStage1::HeartStageOver(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_16_4BB4");
  return false;
}

bool CEmperorIngStage1::IsDead(CStateManager& mgr, const CTriggerData& data) const {
  StandInFn("fn_16_4B6C");
  return false;
}

// fn_16_4B20 (the trigger "ShouldMeleeAttack", lbl_16_data_19C entry 3): the squared
// horizontal distance to the player against 900 (30 units). The player is the state manager's
// first player actor (retail CStateManager+0x14FC, mPlayers[0]) and the position is the
// actor's own (CActor::mPosition at +0x54, so the two floats are X and Y).
bool CEmperorIngStage1::ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const {
  const CVector3f& mine = GetTranslation();
  const CVector3f& theirs = mgr.GetPlayer(0)->GetTranslation();
  const float dx = mine.GetX() - theirs.GetX();
  const float dy = mine.GetY() - theirs.GetY();
  return dx * dx + dy * dy < 900.f;
}

// fn_16_4B04 (the trigger "ShouldAttack", lbl_16_data_19C entry 4): the attack timer (guest
// 0xA4C, 4.0f in the constructor) has run below zero.
bool CEmperorIngStage1::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  return xa4c_ < 0.f;
}

// fn_16_4AEC (the trigger "ShouldTaunt", lbl_16_data_19C entry 5): the taunt timer (guest
// 0xA50, 0.0f in the constructor) is past the taunt frequency (guest 0xC34).
bool CEmperorIngStage1::ShouldTaunt(CStateManager& mgr, const CTriggerData& data) const {
  return xa50_ > mTauntFrequency;
}

// The 12 states of lbl_16_data_94 (.data 0x94; fn_16_BF88's stores at 0x0000C0C0..0x0000C124 and
// 0x0000C214).
void CEmperorIngStage1::TrackPlayer(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_7C28");
}

void CEmperorIngStage1::BeginHeartStage(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_78AC");
}

void CEmperorIngStage1::HeartStage(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_774C");
}

void CEmperorIngStage1::EndHeartStage(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_71BC");
}

void CEmperorIngStage1::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_716C");
}

void CEmperorIngStage1::MeleeAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_6F28");
}

void CEmperorIngStage1::Attack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_6D94");
}

void CEmperorIngStage1::ProjectileAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_68F4");
}

void CEmperorIngStage1::WarpAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_601C");
}

void CEmperorIngStage1::TauntAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_5DBC");
}

void CEmperorIngStage1::Taunt(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_5C4C");
}

void CEmperorIngStage1::JumpRope(CStateManager& mgr, EStateMsg msg, float dt) {
  StandInFn("fn_16_5928");
}

// fn_16_4AE8 (the code "BeginSubStage", lbl_16_data_220 entry 0): retail's body is a single
// `blr` - the sub-stage itself is the state machine's business.
void CEmperorIngStage1::BeginSubStage(CStateManager& mgr, float dt) {}

// fn_16_1DFC: the tentacle's timer is cleared and its state becomes 1, unless it is already 2.
static void ResetTentacle(CEmperorIngStage1Tentacle& tentacle) {
  tentacle.x4_ = 0.f;
  if (tentacle.x28_ != 2) {
    tentacle.x28_ = 1;
  }
}

// fn_16_4A04 (the code "ResetTentacles", lbl_16_data_220 entry 1): every tentacle slot of the
// current sub-stage is reset (fn_16_1DFC clears its timer and sets its state to 1 unless it is
// already 2) and gets the retraction timer, then the live tentacles' flags are set (fn_16_2EE0)
// and the script is told which sub-stage this is.
void CEmperorIngStage1::ResetTentacles(CStateManager& mgr, float dt) {
  const int count = TentacleCount();
  for (int i = 0; i < count; ++i) {
    ResetTentacle(x7d8_[i]);
    x7d8_[i].x18_ = mStayRetractedTime;
  }
  SetTentacleFlags(1);
  if (x7c0_ == 2) {
    SendScriptMsgs(kSS_InternalState7, mgr, kInvalidUniqueId, kSM_None);
  } else if (x7c0_ >= 1) {
    SendScriptMsgs(kSS_InternalState6, mgr, kInvalidUniqueId, kSM_None);
  }
}

void CEmperorIngStage1::ChooseRandomTentacleCount(CStateManager& mgr, float dt) {
  StandInFn("fn_16_4954");
}

// fn_16_2F70: how many tentacle slots the current sub-stage uses - four, six or eight. The
// sub-stage is the int at guest 0x7C4.
int CEmperorIngStage1::TentacleCount() const {
  if (x7c4_ == 1) {
    return 6;
  }
  if (x7c4_ >= 1) {
    return 8;
  }
  if (x7c4_ >= 0) {
    return 4;
  }
  return 8;
}

// fn_16_2EE0: every live tentacle's flag byte (guest +0x24) gets its 0x80 bit set when the
// argument is non-zero and the tentacle is one of the current sub-stage's slots; the others get
// it cleared.
void CEmperorIngStage1::SetTentacleFlags(uint flag) {
  const bool set = (flag & 0xff) != 0;
  const int count = TentacleCount();
  for (int i = 0; i < x7d4_; ++i) {
    if (set && i < count) {
      x7d8_[i].x24_ |= 0x80;
    } else {
      x7d8_[i].x24_ &= ~0x80;
    }
  }
}

// ---------------------------------------------------------------------------------------------
// The module's loader (fn_16_A2A0) and entry points.

static CEntity* PortLoadEmperorIngStage1(CStateManager& mgr, CInputStream& input,
                                         CEntityInfo& info) {
  SLdrEmperorIngStage1 sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrEmperorIngStage1.inc"

  // The REL builds the model with LdrToModelData(scale, kInvalidAssetId, the patterned info's
  // animation set, true) at 0x0000A41C and moves the result into the object at 0x0000A434.
  rstl::optional_object< CModelData > modelData(
      LdrToModelData(sldrThis.editorProperties.transform.scale, kInvalidAssetId,
                     sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL reads the copy's valid byte (sp+0xF8 at 0x0000A440) and branches to its null
    // return at 0x0000A448 when the model is empty.
    return nullptr;
  }

  // The REL's own allocation is rs_new(0xD18, "CEmperorIngStage1") at 0x0000A4B8; the port
  // allocates sizeof(CEmperorIngStage1), which is larger because the host layout is its own.
  return rs_new CEmperorIngStage1(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                                  LdrToEntityInfo(info, sldrThis.editorProperties),
                                  LdrToTransform4f(sldrThis.editorProperties), *modelData,
                                  LdrToActorParameters(sldrThis.actorInformation),
                                  LdrToPatternedInfo(sldrThis.patterned, nullptr), sldrThis.data);
}

static SEmperorIngStage1_FuncPtrs REL_loader_EmperorIngStage1;

// Retail's RELMain calls fn_16_A270, which stores the loader fn_16_A2A0 into lbl_16_bss_0 (the
// module's SEmperorIngStage1_FuncPtrs) and calls the DOL's SetSEmperorIngStage1_FuncPtrs. The
// proxy in SEmperorIngStage1_FuncPtrs.cpp calls through it.
void RELMain() {
  REL_loader_EmperorIngStage1.mLoader = PortLoadEmperorIngStage1;
  SetSEmperorIngStage1_FuncPtrs(&REL_loader_EmperorIngStage1);
}

void RELExit() { SetSEmperorIngStage1_FuncPtrs(nullptr); }
