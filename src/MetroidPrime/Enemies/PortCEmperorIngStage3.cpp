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
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrEmperorIngStage3.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

#include "Collision/CCollisionPrimitive.hpp"
#include "Collision/CRayCastResult.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Math/CAABox.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"

#include <float.h>
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

// fn_18_39A0 (0x3C): the cross product of two vectors: the result is (a.y*b.z - a.z*b.y,
// a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x) (0x39CC..0x39D0).
CVector3f Cross3f(const CVector3f& a, const CVector3f& b) { return CVector3f::Cross(a, b); }

// fn_18_3AC (0x20): the copy of the ray's result. The host's CRayCastResult is copyable, so the
// copy is the assignment; the byte the retail reads at +0x28 is the result's mValid.
CRayCastResult CopyRayResult(const CRayCastResult& result) { return result; }

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
, x824_(0)                                                 // guest 0x824
, x825_(0)                                                 // guest 0x825
, x7c0_(nullptr)                                           // guest 0x7C0
, mVulnerabilityState(0)                                  // guest 0x7C4
, mX7c8_(0.f)                                             // guest 0x7C8
, mX7cc_(0.f)                                             // guest 0x7CC
, mX7d4_(0.f)                                             // guest 0x7D4
, mX7d0_(0.f)                                             // guest 0x7D0
, mX7e0_(10000.f)                                         // guest 0x7E0
, mAttackTimer(4.f)                                       // guest 0x7E4
, mAction(-1)                                             // guest 0x7E8
, mX810_(0.f)                                             // guest 0x810
, mX86c_(CDamageVulnerability::ImmuneVulnerabilty())     // guest 0x86C
, mX8ec_(0.f)                                             // guest 0x8EC
, mX8f0_(0.f)                                             // guest 0x8F0
, mX8f4_(0.f)                                             // guest 0x8F4
, mX818_(CVector3f::Zero())                               // guest 0x818
, mX864_(0.f)                                             // guest 0x864
, mX868_(0)                                               // guest 0x868
, mX6b4_(0)                                               // guest 0x6B4
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
  x824_ |= 0x02;
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

// fn_18_F8 (0x11C): the module's look check. The parameter is the angle in degrees the player
// may be off the actor's facing by. Retail: the vector from the actor's "DarkBeam_LCTR_SDK"
// locator (fn_18_DBC) to the player, normalised (0x170) and flattened to the XY plane (the
// zero at rodata 0x258, 0x198), has to be normalisable (0x260); the angle between it and the
// actor's facing (the transform's Y column, guest 0x28/0x38/0x48) is turned into degrees
// (rodata 0x25C = 57.295776, 0x1D8) and has to be below the parameter (0x1E0).
bool CEmperorIngStage3::RandomCheck(CStateManager& mgr, float maxAngleDeg) const {
  const CVector3f& mine = DarkBeamLocatorPosition();
  const CVector3f& theirs = mgr.GetPlayer(0)->GetTranslation();
  CVector3f direction(theirs.GetX() - mine.GetX(), theirs.GetY() - mine.GetY(),
                       theirs.GetZ() - mine.GetZ());
  direction.Normalize();
  direction.SetZ(0.f); // rodata 0x258
  if (!direction.CanBeNormalized()) {
    return false;
  }
  direction.Normalize();
  const float degrees =
      57.295776f * CVector3f::GetAngleDiff(direction, GetTransform().GetColumn(kDY));
  return degrees < maxAngleDeg;
}

// fn_18_DBC (0x40): the actor's "DarkBeam_LCTR_SDK" locator (rodata 0xA8) in world space: the
// actor's transform times the model's scaled locator transform, and the translation of the
// product (0xE08..0xE20).
CVector3f CEmperorIngStage3::DarkBeamLocatorPosition() const {
  const CTransform4f locator = GetScaledLocatorTransform(rstl::string_l("DarkBeam_LCTR_SDK"));
  return (GetTransform() * locator).GetTranslation();
}

// fn_18_2298 (0x24): the taunt average over the module's bone list. Its own callee fn_18_AB9C
// (0x64) walks the list at guest 0x840, which the port does not model, so this stays a
// stand-in: it names the retail function once and answers the neutral zero.
float CEmperorIngStage3::TauntAverage() const {
  StandInFn("fn_18_2298");
  return 0.f;
}

// fn_18_1A54 (0xE0): the look along the actor's facing. The ray starts at the furthest point of
// the actor's collision box along the facing (the actor's own transform, 0x1AE0) and runs the
// length the caller gives; the answer is "the way is clear", true when the ray hit nothing:
// the retail reads the result's byte at +0x28 and answers true when it is zero (0x1B5C).
bool CEmperorIngStage3::LineOfSight(CStateManager& mgr, const CVector3f& facing,
                                     float length) const {
  CVector3f direction(facing.GetX(), facing.GetY(), facing.GetZ());
  if (!direction.CanBeNormalized()) {
    return false; // 0x1AB0
  }
  direction.Normalize(); // 0x1AB8
  const CAABox bounds = GetCollisionPrimitive()->CalculateAABox(GetTransform()); // 0x1AE0
  const CVector3f start = bounds.FurthestPointAlongVector(direction);           // 0x1AFC
  // The retail's material filter is the type zero (always) with every material in the include
  // list (0x1B38..0x1B48), which is the host's default filter.
  const CRayCastResult result =
      mgr.RayStaticIntersection(start, direction, length, CMaterialFilter()); // 0x1B4C
  const CRayCastResult copy = CopyRayResult(result); // fn_18_3AC (0x20)
  return !copy.IsValid();              // 0x1B5C: the byte the copy has at +0x28
}

// fn_18_3804 (0x25C): the turn towards a position. The facing at guest 0x818 is cleared first
// (0x382C, 0x3840, 0x3848) and only written at the end (0x3970..0x3980): the angle between
// the actor's facing and the direction to the target (0x38A4), snapped to the nearest 45
// degrees (fn_18_3744 at 0x38AC), signed by the up vector's dot with the cross product of the
// two (fn_18_39A0 at 0x38E8, the dot at 0x3908..0x3920), and rotated around Z (0x3958).
void CEmperorIngStage3::FaceTarget(const CVector3f& target) {
  mX818_ = CVector3f::Zero(); // 0x382C, 0x3840, 0x3848
  const CVector3f& mine = GetTranslation();
  CVector3f toTarget(target.GetX() - mine.GetX(), target.GetY() - mine.GetY(),
                     target.GetZ() - mine.GetZ());
  if (!toTarget.CanBeNormalized()) {
    return; // 0x3878
  }
  toTarget.Normalize(); // 0x3880
  const CVector3f facing = GetTransform().GetColumn(kDY);
  const float angle = SnapAngleTo45(CVector3f::GetAngleDiff(facing, toTarget)); // fn_18_3744
  if (angle <= 0.f) {                                                          // rodata 0x258
    return;                                                                    // 0x38C0
  }
  const CVector3f cross = Cross3f(facing, toTarget); // fn_18_39A0
  const CVector3f& up = CVector3f::Up();
  const float turn = up.GetX() * cross.GetX() + up.GetY() * cross.GetY() +
                     up.GetZ() * cross.GetZ(); // 0x3908..0x3920
  const float signedAngle = turn <= 0.f ? -angle : angle; // 0x3924..0x3930
  const CTransform4f turn4f = CTransform4f::RotateZ(CRelAngle::FromRadians(signedAngle)); // 0x3958
  mX818_ = turn4f * facing;                                                            // 0x3968
}

// fn_18_3744 (0xC0): the angle snapped to the nearest 45 degrees. The table at rodata 0x244
// holds 0, pi/4, pi/2, 3pi/4 and pi, and the entry nearest the angle is the answer.
float CEmperorIngStage3::SnapAngleTo45(float angle) const {
  static const float kAngles[] = {0.f, 0.7853982f, 1.5707964f, 2.3561945f,
                                  3.1415927f}; // rodata 0x244
  float best = FLT_MAX; // rodata 0x2C8
  u32 index = 0;
  for (u32 i = 0; i < 5; ++i) {
    const float diff = CMath::AbsF(angle - kAngles[i]);
    if (diff < best) {
      best = diff;
      index = i;
    }
  }
  return kAngles[index];
}

// fn_18_3B14 (0x4C): the module's heart record: the holder's record 0xA (the first heart, the
// index at 0x3B2C) or 0xB (the second, 0x3B44); any other heart is no heart (0x3B50). The
// holder is null until the module creates it - the module's fn_18_3BB0 hands the DOL's
// fn_80136434 the ten leg bones of rodata 0x18 and gets the holder with the collision actors
// back - and that creator is still a stand-in, so the port finds no record.
const SEmperorIngHeartRecord* CEmperorIngStage3::HeartRecord(int heart) const {
  if (x7c0_ == nullptr) {
    return nullptr; // port limit: the holder is not created yet
  }
  if (heart == 0) {
    return &x7c0_->mRecords[0xA];
  }
  if (heart == 1) {
    return &x7c0_->mRecords[0xB];
  }
  return nullptr; // 0x3B50: any other heart is no heart
}

// fn_18_3ACC (0x48): the heart's collision actor: the unique id of the record (guest +0x3C,
// read at 0x3AE8) looked up in the state manager (0x3AFC).
CCollisionActor* CEmperorIngStage3::HeartCollisionActor(CStateManager& mgr, int heart) const {
  const SEmperorIngHeartRecord* record = HeartRecord(heart);
  if (record == nullptr) {
    return nullptr; // port limit: the holder is not created yet
  }
  return static_cast< CCollisionActor* >(mgr.ObjectById(TUniqueId(record->mUniqueId)));
}

// fn_18_22BC (0x24): the module's average over its bone list at guest 0x830. Its own callee
// fn_18_AB9C (0x64) walks the list, which the port does not model, so this stays a stand-in.
void CEmperorIngStage3::BoneAverage() const { StandInFn("fn_18_22BC"); }

// fn_18_2D1C (0x34): the vector from the actor to the first player, flattened to the XY plane
// (the zero at rodata 0x258).
CVector3f CEmperorIngStage3::PlayerDirection(CStateManager& mgr) const {
  const CVector3f& mine = GetTranslation();
  const CVector3f& theirs = mgr.GetPlayer(0)->GetTranslation();
  return CVector3f(theirs.GetX() - mine.GetX(), theirs.GetY() - mine.GetY(), 0.f);
}

// fn_18_2C98 (0x20): the actor's facing (the transform's Y column), flattened to the XY plane.
CVector3f CEmperorIngStage3::FlatFacing() const {
  const CVector3f facing = GetTransform().GetColumn(kDY);
  return CVector3f(facing.GetX(), facing.GetY(), 0.f);
}

// fn_18_1BD8 (0x5C): both hearts get the module's vulnerability (guest 0x86C). The two tests are
// a port limit: retail has the hearts (its holder is made in fn_18_3BB0), the port has no
// holder, so the lookup can find no actor.
void CEmperorIngStage3::SetHeartVulnerability(CStateManager& mgr) const {
  CCollisionActor* first = HeartCollisionActor(mgr, 0); // fn_18_3ACC
  if (first != nullptr) {
    first->SetDamageVulnerability(mX86c_); // 0x1C00
  }
  CCollisionActor* second = HeartCollisionActor(mgr, 1); // fn_18_3ACC
  if (second != nullptr) {
    second->SetDamageVulnerability(mX86c_); // 0x1C18
  }
}

// fn_18_1BB0 (0x28): the heart switch with the heart that is on (guest 0x868) and no
// vulnerability change.
void CEmperorIngStage3::SetCurrentHeart(CStateManager& mgr) { SetHeartState(mgr, mX868_, false); }

// fn_18_8C4C (0x13C): "ShouldMeleeAttack". The module's time since the last attack (guest
// 0x7D0) has to be at least 2.0 (rodata 0x274), the module's average over the bone list runs
// (fn_18_22BC) and the time has to be at least 10.0 (rodata 0x294). The vector to the player
// (fn_18_2D1C) has to be normalisable and shorter than 30.0 (rodata 0x2FC), and the player has
// to be in front of the actor: the cosine of the angle between the actor's flattened facing
// (fn_18_2C98) and the direction to the player has to be above 1.3089969 (rodata 0x318).
bool CEmperorIngStage3::ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const {
  if (mX7d0_ < 2.f) { // rodata 0x274
    return false;
  }
  BoneAverage(); // fn_18_22BC
  if (mX7d0_ < 10.f) { // rodata 0x294
    return false;
  }
  const CVector3f toPlayer = PlayerDirection(mgr); // fn_18_2D1C
  if (!toPlayer.CanBeNormalized()) {
    return false;
  }
  if (toPlayer.Magnitude() >= 30.f) { // rodata 0x2FC
    return false;
  }
  const CVector3f facing = FlatFacing(); // fn_18_2C98
  const float cosine = CMath::FastCosR(CVector3f::Dot(facing, toPlayer));
  return cosine > 1.3089969f; // rodata 0x318
}

// fn_18_8ACC (0x180): "ShouldGroundPound". The module's time since the last attack (guest
// 0x7D0) has to be at least 2.0 (rodata 0x274). The vector from the ing's "head" locator
// (rodata 0x704) to the player has to be normalisable, and the player has to be below the ing:
// the cosine of the angle between the direction to the player and the ing's down axis (the
// negated Z column of the transform) has to be above 0.7853982 (rodata 0x314).
bool CEmperorIngStage3::ShouldGroundPound(CStateManager& mgr, const CTriggerData& data) const {
  if (mX7d0_ < 2.f) { // rodata 0x274
    return false;
  }
  const CVector3f head =
      (GetTransform() * GetScaledLocatorTransform(rstl::string_l("head"))).GetTranslation();
  const CVector3f& theirs = mgr.GetPlayer(0)->GetTranslation();
  CVector3f toPlayer(theirs.GetX() - head.GetX(), theirs.GetY() - head.GetY(),
                     theirs.GetZ() - head.GetZ());
  if (!toPlayer.CanBeNormalized()) {
    return false;
  }
  toPlayer = toPlayer.AsNormalized();
  const CVector3f down = GetTransform().GetColumn(kDZ);
  const float cosine = CMath::FastCosR(-down.GetZ() * toPlayer.GetZ() +
                                       (-down.GetX() * toPlayer.GetX() +
                                        -down.GetY() * toPlayer.GetY()));
  return cosine > 0.7853982f; // rodata 0x314
}

// fn_18_5264 (0x2F8): the "EyeShut" state.
//  - Activate (0x52B0): the eye-shut time. The module's time (guest 0x8EC) times two percent
//    (rodata 0x274) over a hundred (rodata 0x27C), at most 2.5 (rodata 0x298), goes to guest
//    0x8F0 and the module's time is cleared; a random time between one (rodata 0x264) and 1.2
//    (rodata 0x2EC) times that, at most 5.0 (rodata 0x2BC), goes to guest 0x8F4; the eye-shut
//    time then is at least 2.5 (rodata 0x298) plus that random time; the flag at guest 0x6B4 is
//    set.
//  - Update (0x5370): the body's eye-shut commands. A stand-in: the body needs the module's own
//    command classes (the vtables at lbl_18_data_744, _750 and _78C) and the base member at
//    guest 0x350, which the port does not model.
//  - Deactivate (0x5518): the eye-shut timer (guest 0x7CC) is cleared, both hearts get the
//    module's vulnerability (fn_18_1BD8) and the heart switch runs with the heart that is on
//    (fn_18_1BB0); the flag at guest 0x6B4 is cleared.
void CEmperorIngStage3::EyeShut(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    float time = mX8ec_ * 2.f / 100.f; // rodata 0x274, 0x27C
    if (time > 2.5f) {               // rodata 0x298
      time = 2.5f;
    }
    mX8f0_ = time;
    mX8ec_ = 0.f;
    float range = 1.2f * mX8f0_; // rodata 0x2EC
    if (range > 5.f) {            // rodata 0x2BC
      range = 5.f;
    }
    mX8f4_ = mgr.Random()->Range(1.f, range); // rodata 0x264
    if (mX8f0_ < 2.5f + mX8f4_) { // rodata 0x298
      mX8f0_ = 2.5f + mX8f4_;
    }
    mX6b4_ = 1;
    break;
  }
  case kStateMsg_Update:
    StandInFn("fn_18_5264 update");
    break;
  case kStateMsg_Deactivate:
    mX7cc_ = 0.f;
    SetHeartVulnerability(mgr); // fn_18_1BD8
    SetCurrentHeart(mgr);      // fn_18_1BB0
    mX6b4_ = 0;
    break;
  default:
    break;
  }
}

// fn_18_1DF4 (0x4A0): the heart switch. Heart 0 is the enraged heart: the two hearts get the
// yellow health (guest 0x920, 0x1E8C) and the "EnragedHeart" effect; heart 1 the dark
// heart: the actor's own health (the vtable slot 0x3C, 0x1F80) and the "DarkHeart" effect;
// heart 2 the light heart: the same health (0x208C) and the "LightHeart" effect. The heart
// that is on is remembered at guest 0x868 (0x2164) and, when the caller asks for it, the two
// hearts get that heart's vulnerability (0x960 red, 0x9C0 dark, 0x990 light, 0x219C..0x21E4).
// The lookup of the two hearts finds none while the module's holder is not created, and
// retail returns early then (0x1E34), so the body below does not run on the host.
void CEmperorIngStage3::SetHeartState(CStateManager& mgr, int heart, bool applyVulnerability) {
  CCollisionActor* first = HeartCollisionActor(mgr, 0);  // fn_18_3ACC
  CCollisionActor* second = HeartCollisionActor(mgr, 1); // fn_18_3ACC
  if (first == nullptr) {
    return; // 0x1E34: the retail's own test of the first heart
  }
  bool enraged = false;
  bool light = false;
  bool dark = false;
  switch (heart) {
  case 0:
    if (heart != mX868_) {
      *first->HealthInfo() = mYellowHealth;  // 0x1E8C: the yellow health at guest 0x920
      *second->HealthInfo() = mYellowHealth; // 0x1FDC
    }
    enraged = true; // 0x1F40
    break;
  case 1:
    if (heart != mX868_) {
      *first->HealthInfo() = *GetHealthInfo();  // 0x1F80: the actor's own health
      *second->HealthInfo() = *GetHealthInfo(); // 0x2000
    }
    dark = true; // 0x204C
    break;
  case 2:
    if (heart != mX868_) {
      *first->HealthInfo() = *GetHealthInfo();  // 0x208C
      *second->HealthInfo() = *GetHealthInfo(); // 0x2108
    }
    light = true; // 0x2158
    break;
  default:
    break;
  }
  if (heart != 3) {
    mX868_ = heart; // 0x2164
  }
  if (applyVulnerability) {
    switch (mX868_) {
    case 0:
      first->SetDamageVulnerability(mRedVulnerability);  // 0x219C: guest 0x960
      second->SetDamageVulnerability(mRedVulnerability); // 0x21A8
      break;
    case 1:
      first->SetDamageVulnerability(mDarkVulnerability);  // 0x21B4: guest 0x9C0
      second->SetDamageVulnerability(mDarkVulnerability); // 0x21C0
      break;
    case 2:
      first->SetDamageVulnerability(mLightVulnerability);  // 0x21D0: guest 0x990
      second->SetDamageVulnerability(mLightVulnerability); // 0x21DC
      break;
    default:
      break;
    }
  }
  AnimationData()->SetEffectState(rstl::string_l("EnragedHeart"), enraged, mgr); // 0x2210
  AnimationData()->SetEffectState(rstl::string_l("LightHeart"), light, mgr);     // 0x2244
  AnimationData()->SetEffectState(rstl::string_l("DarkHeart"), dark, mgr);       // 0x2278
}

// The module's state machine functions. The eleven triggers whose retail body is small enough
// are ported; the rest are stand-ins that log the retail function they stand in for and return the
// neutral value (false for a trigger, nothing for a state or a code).
// fn_18_8DA4 (0x8DA4): "TooClose". The module's own distance (guest 0x7D4) has to be at least
// 2.0 (rodata 0x274) and the player then has to be nearer than 20 units (rodata 0x270). A vector
// that cannot be normalized counts as too close.
// fn_18_2D1C (0x34) is the vector from the actor to the first player, flattened to the XY plane;
// fn_18_2C98 (0x20) copies a vector the retail then never reads, so it is left out.
bool CEmperorIngStage3::TooClose(CStateManager& mgr, const CTriggerData& data) const {
  if (mX7d4_ < 2.f) { // rodata 0x274
    return false;
  }
  const CVector3f toPlayer = PlayerDirection(mgr); // fn_18_2D1C
  if (!toPlayer.CanBeNormalized()) {
    return true;
  }
  return toPlayer.Magnitude() < 20.f; // rodata 0x270
}

// fn_18_8D88 (0x8D88): the attack timer (guest 0x7E4) against zero (rodata 0x258).
bool CEmperorIngStage3::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  return mAttackTimer < 0.f;
}

// fn_18_8A4C (0x8A4C): "ShouldTaunt". The look check has to pass (rodata 0x2E4 = 60.0f),
// the module's taunt average runs and has to be above 50.0 (rodata 0x290), and the taunt
// timer (guest 0x7C8) has to be past the taunt frequency (guest 0x91C).
bool CEmperorIngStage3::ShouldTaunt(CStateManager& mgr, const CTriggerData& data) const {
  if (!RandomCheck(mgr, 60.f)) { // rodata 0x2E4
    return false;
  }
  if (TauntAverage() <= 50.f) { // fn_18_2298, rodata 0x290
    return false;
  }
  return mX7c8_ > mTauntFrequency;
}

// fn_18_897C (0x897C): the action (guest 0x7E8) is 0.
bool CEmperorIngStage3::ShouldStampede(CStateManager& mgr, const CTriggerData& data) const {
  return mAction == 0;
}

// fn_18_8A34 (0x8A34): the flag byte at guest 0x824, the bit 28 (the mask 0x08).
bool CEmperorIngStage3::ShouldDodge(CStateManager& mgr, const CTriggerData& data) const {
  return (x824_ & 0x08) != 0;
}

// fn_18_8A20 (0x8A20): the vulnerability state (guest 0x7C4) is 1.
bool CEmperorIngStage3::ShouldChangeVulnerabilityToVulnerable(CStateManager& mgr, const CTriggerData& data) const {
  return mVulnerabilityState == 1;
}

// fn_18_898C (0x898C): "ShouldChangeVulnerabilityToInvulnerable". The look check has to pass
// (rodata 0x2CC = 90.0f); in the vulnerable state (3) the trigger is true, in the invulnerable
// state (2) it needs the damage taken while vulnerable (guest 0x810) or the vulnerable damage
// threshold (guest 0x814).
bool CEmperorIngStage3::ShouldChangeVulnerabilityToInvulnerable(CStateManager& mgr,
                                                               const CTriggerData& data) const {
  if (!RandomCheck(mgr, 90.f)) { // rodata 0x2CC
    return false;
  }
  if (mVulnerabilityState == 3) {
    return true;
  }
  if (mVulnerabilityState != 2) {
    return false;
  }
  if (mX810_ < 0.f) { // rodata 0x258
    return true;
  }
  return mVulnerableDamageThreshold < 1.1920929e-07f; // rodata 0x310
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

// fn_18_85A4 (0x85A4): "ShouldEyeShut". The look check has to pass (rodata 0x2CC = 90.0f) and
// the eye-shut timer (guest 0x7CC) has to be below 8.0 (rodata 0x2A8).
bool CEmperorIngStage3::ShouldEyeShut(CStateManager& mgr, const CTriggerData& data) const {
  if (!RandomCheck(mgr, 90.f)) { // rodata 0x2CC
    return false;
  }
  return mX7cc_ < 8.f; // rodata 0x2A8
}

// fn_18_858C (0x858C): the flag byte at guest 0x825, the bit 29 (the mask 0x04).
bool CEmperorIngStage3::AbortedJumpSlide(CStateManager& mgr, const CTriggerData& data) const {
  return (x825_ & 0x04) != 0;
}

// fn_18_8518 (0x8518): "ShouldJumpAttack". The look check has to pass (rodata 0x30C = 35.0f),
// the jump-attack timer (guest 0x7E0) has to be at least 2.5 (rodata 0x298) and the flag byte at
// guest 0x824 has to have its bit 29 (the mask 0x04) set.
bool CEmperorIngStage3::ShouldJumpAttack(CStateManager& mgr, const CTriggerData& data) const {
  if (!RandomCheck(mgr, 35.f)) { // rodata 0x30C
    return false;
  }
  if (mX7e0_ < 2.5f) { // rodata 0x298
    return false;
  }
  return (x824_ & 0x04) != 0;
}

// fn_18_8500 (0x8500): the flag byte at guest 0x825, the bit 0 (the mask 0x01).
bool CEmperorIngStage3::ShouldStampedeReact(CStateManager& mgr, const CTriggerData& data) const {
  return (x825_ & 0x01) != 0;
}

// fn_18_7ADC (0x7ADC): the "Start" state. On the deactivation message it hands over to
// fn_18_1DF4 with the enraged heart (0) and the flag set (0x7AF8..0x7B00).
void CEmperorIngStage3::Start(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg != kStateMsg_Deactivate) {
    return;
  }
  SetHeartState(mgr, 0, true); // fn_18_1DF4
}

// fn_18_788C (0x250): the "Idle" state.
//  - Activate (0x78D0): the timer at guest 0x864 is set to the biggest float (rodata 0x2C8)
//    and the body's steering is set up: the walk speed over the run speed (the two
//    GetLocomotionSpeed calls, pas::kLA_Walk over pas::kLA_Run, 0x78E4..0x78FC), the steering
//    blend mode (the write of one at 0x7908) and the steering speed range (0x7918).
//  - Update (0x7920): the vector to the player; fn_18_1A54 (0xE0, a stand-in) looks along the
//    actor's facing for something to hit and fn_18_3804 (0x25C, a stand-in) turns the actor
//    towards the player; over fifty units (rodata 0x290) the timer is cleared (rodata 0x258);
//    the locomotion command carries the normalised direction to the player when the look hit
//    and the timer is below half a second (rodata 0x2A0), the zero vector otherwise; then the
//    timer grows by the frame time (0x7AB0).
void CEmperorIngStage3::Idle(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    mX864_ = FLT_MAX; // rodata 0x2C8
    const float run = BodyController()->GetBodyStateInfo().GetLocomotionSpeed(pas::kLA_Run);
    const float walk = BodyController()->GetBodyStateInfo().GetLocomotionSpeed(pas::kLA_Walk);
    const float ratio = walk / run; // 0x78FC
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_FullSpeed); // 0x7908
    BodyController()->CommandMgr().SetSteeringSpeedRange(ratio, ratio);  // 0x7918
    break;
  }
  case kStateMsg_Update: {
    const CVector3f& theirs = mgr.GetPlayer(0)->GetTranslation();
    const CVector3f& mine = GetTranslation();
    CVector3f toPlayer(theirs.GetX() - mine.GetX(), theirs.GetY() - mine.GetY(),
                       theirs.GetZ() - mine.GetZ());
    if (!toPlayer.CanBeNormalized()) {
      return; // 0x796C: the retail leaves the state without the timer
    }
    const bool hit = LineOfSight(mgr, GetTransform().GetColumn(kDY), 15.f); // fn_18_1A54
    FaceTarget(theirs);                                                   // fn_18_3804
    const float distance = toPlayer.Magnitude();
    const float scale = 1.f / distance;
    const CVector3f direction(toPlayer.GetX() * scale, toPlayer.GetY() * scale,
                              toPlayer.GetZ() * scale);
    if (distance > 50.f) { // rodata 0x290
      mX864_ = 0.f;        // rodata 0x258
    }
    if (hit && mX864_ < 0.5f) { // rodata 0x2A0
      BodyController()->CommandMgr().DeliverCmd(CBCLocomotionCmd(direction, mX818_, 1.f));
    } else {
      BodyController()->CommandMgr().DeliverCmd(CBCLocomotionCmd(CVector3f::Zero(), mX818_, 1.f));
    }
    mX864_ += dt; // 0x7AB0
    break;
  }
  default:
    break;
  }
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
