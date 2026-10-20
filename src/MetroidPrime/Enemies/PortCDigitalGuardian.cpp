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

#include "Kyoto/Animation/CCharLayoutInfo.hpp"
#include "Kyoto/Animation/CSegId.hpp"
#include "Kyoto/Graphics/CColor.hpp"
#include "Kyoto/Graphics/CModelFlags.hpp"
#include "Kyoto/Particles/CParticleGen.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrDigitalGuardian.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

#include <float.h>
#include <stdio.h>

namespace {

// The module's render flag (lbl_14_bss_0): the module's render (fn_14_A3D4) sets it while it
// renders, and its RenderSystemsToBeDrawnLast (fn_14_A3A4) only forwards to the base's while
// it is set.
uchar sRendering = 0;

// The module's GX register colour (lbl_14_bss_30): the static initialiser (fn_14_D904) sets
// it to (0.25, 0.25, 0.25, 1).
const CColor sGXRegister1Color(0.25f, 0.25f, 0.25f, 1.f);

// The retail's GenerateParticlePOIMask (the call at 0xA420) is the host's
// CStateManager::GetCharacterRenderMaskAndTarget; the retail's name is kept here so the
// asm-vs-C++ check can match the call.
void GenerateParticlePOIMask(const CStateManager& mgr, uint& mask, uint& target) {
  mgr.GetCharacterRenderMaskAndTarget(mask, target);
}

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
, xFB0_(kInvalidUniqueId.value)
, xFB2_(kInvalidUniqueId.value)
, xFB4_(kInvalidUniqueId.value)
, xFB6_(kInvalidUniqueId.value)
, x10C4_(4)
, x10C8_{CSegId::Invalid(), CSegId::Invalid(), CSegId::Invalid(), CSegId::Invalid()}
, x1082_(0)
, x1168_(TUniqueId(0))
, x11B8_(0)
, x11E0_(0.f)
, x11E4_(0.f)
, x1274_(0.f)
, x1278_(CVector3f::Zero())
, x1284_(-1)
, x128C_(0)
, xE6C_() // 0xE6C: the leg model (x6864_ fills it in)
, xEBC_() // 0xEBC: the shin model (x6864_ fills it in)
, xF0C_() // 0xF0C: the knee model (x6864_ fills it in)
, xF5C_() // 0xF5C: the toe target's model (x6864_ fills it in)
, x11EC_() // 0x11EC: the transmission beacon's model (x6864_ fills it in)
, x10C0_(0xFF)
, x10C1_(0xFF)
, x10C2_(0xFF)
, x10C3_(0xFF)
, x10D4_(0xFF)
, x10D5_(0xFF)
, x10D6_(0xFF)
, x10D7_(0xFF)
, x10DC_{-1, -1, -1, -1}
, x123C_(nullptr)
, x1264_(0.f)
, x1270_(0)
, mData(data) // guest 0x7C0: the 0x640-byte parameter block fn_14_C68C copies in
{
  // The retail constructor also builds the secondary struct at 0xE00 from the patterned info
  // (fn_14_C57C), the four vulnerabilities at 0x1014/0x1044/0x1110/0x1188 and the 'PART' token at
  // 0x1100. They belong to the state machine and the render paths this milestone does not enter,
  // so the port keeps the parameter block only.

  // The locator ids the retail constructor resolves with GetLocatorSegId (the calls from 0xBAF0
  // on): 0x10B9 is "neck_SDK" (the store at 0xBAFC), 0x10C0..0x10C3 the four knee locators
  // ("L_front_knee_2", "L_back_knee_2", "R_front_knee_2", "R_back_knee_2" - the stores at
  // 0xBB30, 0xBB64, 0xBB98 and 0xBBCC) and 0x10D4..0x10D7 the four ankle locators
  // ("L_front_ankle", "L_back_ankle", "R_front_ankle", "R_back_ankle" - the stores at 0xBCD0,
  // 0xBD04, 0xBD38 and 0xBD6C). The ids at 0x10C8..0x10CB (the "*_col_LCTR" locators) belong
  // to the melee trigger and are not resolved yet.
  const CAnimData* anim = GetAnimationData();
  x10B9_ = anim->GetLocatorSegId(rstl::string_l("neck_SDK")).val();
  x10C0_ = anim->GetLocatorSegId(rstl::string_l("L_front_knee_2")).val();
  x10C1_ = anim->GetLocatorSegId(rstl::string_l("L_back_knee_2")).val();
  x10C2_ = anim->GetLocatorSegId(rstl::string_l("R_front_knee_2")).val();
  x10C3_ = anim->GetLocatorSegId(rstl::string_l("R_back_knee_2")).val();
  x10D4_ = anim->GetLocatorSegId(rstl::string_l("L_front_ankle")).val();
  x10D5_ = anim->GetLocatorSegId(rstl::string_l("L_back_ankle")).val();
  x10D6_ = anim->GetLocatorSegId(rstl::string_l("R_front_ankle")).val();
  x10D7_ = anim->GetLocatorSegId(rstl::string_l("R_back_ankle")).val();

  // fn_14_6864 (the call at 0xBD78): the module's five models.
  x6864_();
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

// fn_14_A688 (vtable slot 4): the base's PreThink (the call at 0xA694) and nothing else. The
// port keeps the body activation the base's PreThink does not do (CPatterned::PreThink only
// forwards to CEntity::PreThink), so the guardian still thinks.
void CDigitalGuardian::PreThink(float dt, CStateManager& mgr) {
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

// fn_14_A384 (vtable slot 9): the base's PreRender (the call at 0xA390) and nothing else.
void CDigitalGuardian::PreRender(CStateManager& mgr) {
  CPatterned::PreRender(mgr);
}

// fn_14_A004 (vtable slot 10, AddToRenderer): the base's AddToRenderer (the call at 0xA01C),
// the module's GX register colour (the call at 0xA020), the base's render (the direct call at
// 0xA04C - this is where the guardian's model is drawn; the REL's own Render does not draw it),
// the character's alpha buffer alpha (the call at 0xA058), which is pushed to the renderer when
// it is not -1 (the call at 0xA068). Then the module's own models are drawn (the calls at 0xA0A0,
// 0xA0B4, 0xA0E8 and 0xA0FC), the GX register colour goes back to black (the Black() call at
// 0xA0B8 and the renderer call at 0xA0BC), and the alpha is pushed again (the call at 0xA108; the
// cmpwi at 0xA100 skips it when the alpha is -1).
//
// The four module calls, by size and by what their pseudo-C does:
//   fn_14_1994 (0xD4): if the flag byte at guest this+0xEB8 is set, every locator id in the
//     array at this+0x10C0 is walked and the model at this+0xE6C is drawn at that locator's
//     scaled transform.
//   fn_14_18B4 (0xE0): the same walk, but when bit 4 of the flag byte at this+0x128C is clear
//     and the byte at this+0xF08 is set, drawing the model at this+0xEBC.
//   fn_14_17DC (0xD8): when bit 3 of the flag byte at this+0x128C is clear, the byte at
//     this+0x1238 is set and the float at this+0x1264 is positive, the model at this+0x11EC is
//     drawn at the transform fn_14_2578 builds; the echo emitter at this+0x123C is then drawn
//     through the renderer.
//   fn_14_1700 (0xDC): the locator ids at this+0x10D4 are walked and the model at this+0xF5C
//     is drawn at each of their scaled transforms.
//
// The four calls get the actor's transform (guest this+0x24, the CActor's mTransform) and the
// actor's draw flags (guest this+0xFC, the CActor's mDrawFlags) - the retail's r5 and r6.
void CDigitalGuardian::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
  gpRender->SetGXRegister1Color(sGXRegister1Color);
  CPatterned::Render(mgr);
  const int alpha = GetRenderAlphaBufferAlpha(mgr);
  if (alpha != -1) {
    gpRender->SetDestinationAlpha(alpha);
  }
  // fn_14_1994 (0xA0A0): the module's leg model render.
  x1994_(mgr, GetTransform(), GetModelFlags());
  // fn_14_18B4 (0xA0B4): the module's second model render.
  x18B4_(mgr, GetTransform(), GetModelFlags());
  gpRender->SetGXRegister1Color(CColor::Black());
  // fn_14_17DC (0xA0E8): the module's echo emitter render.
  x17DC_(mgr, GetTransform());
  // fn_14_1700 (0xA0FC): the module's toe model render.
  x1700_(mgr, GetTransform(), GetModelFlags());
  if (alpha != -1) {
    gpRender->SetDestinationAlpha(alpha);
  }
}

// The retail's per-segment rotation: the character's rest pose rotation for that segment. The
// retail reads it from the actor's model data (guest this+0x60, the CActor's mModelData), the
// anim data (that+0x10), the character layout (that+0x10C) and the layout's linear rotations
// (that+0x68), indexed with the segment id - the chain
// CScriptSpecialFunction::ThinkObjectFollowJoint walks as well.
//
// Port limit: the locator ids are still invalid in the port (the constructor does not resolve
// them - the model data's builder is not ported yet), and the retail would index the rotation
// array with 0xFF, so an invalid id has no rotation here.
CTransform4f CDigitalGuardian::SegmentRotation_(const CSegId& seg) const {
  if (seg == CSegId::Invalid()) {
    return CTransform4f::Identity();
  }
  const CModelData& model = *GetModelData();
  if (!model.HasAnimation()) {
    return CTransform4f::Identity();
  }
  const CCharLayoutInfo& layout = *model.GetAnimationData()->GetCharLayoutInfo();
  const rstl::vector< CQuaternion >& rotations = layout.GetLinearRotations();
  return rotations[seg.val()].BuildTransform4f();
}

// fn_14_1994 (0xD4 bytes): the leg model. The flag byte at guest this+0xEB8 has to be set (the
// test at 0x19B4); then each of the four locator ids at this+0x10C0 is walked (the loop at
// 0x19D4-0x1A50): the segment's rotation (the BuildTransform4f call at 0x19EC), the locator's
// scaled transform (the GetScaledLocatorTransform call at 0x19FC), the actor's transform times
// the locator's (the __ml__ at 0x1A0C), times the segment's rotation (the __ml__ at 0x1A1C), and
// the model at this+0xE6C is drawn there (the Render call at 0x1A40).
void CDigitalGuardian::x1994_(const CStateManager& mgr, const CTransform4f& xf,
                              const CModelFlags& flags) const {
  if (!xE6C_.valid()) {
    return;
  }
  // The ids are the bytes the retail loads (the lbz at 0x19D4): the first one is the byte the
  // load reads (the mask is the whole byte), the retail steps the pointer for the other three.
  const uchar ids[4] = {x10C0_ & 0xFF, x10C1_, x10C2_, x10C3_};
  for (int i = 0; i < 4; ++i) {
    const CSegId seg(ids[i]);
    const CTransform4f modelXf = xf * GetScaledLocatorTransform(seg) * SegmentRotation_(seg);
    xE6C_.data().Render(mgr, modelXf, GetActorLights(), flags);
  }
}

// fn_14_18B4 (0xE0 bytes): the shin model. The same walk as fn_14_1994 (the ids at this+0x10C0,
// the locator transform at 0x1928, the two __ml__ calls at 0x1938 and 0x1948), but the model at
// this+0xEBC is drawn (the Render call at 0x196C) and only when bit 4 of the flag byte at
// this+0x128C is clear (the extrwi at 0x19D8 - the byte's bit 4, the mask 1u << (31 - 27) =
// 0x10) and the byte at this+0xF08 is set (the test at 0x19E4).
void CDigitalGuardian::x18B4_(const CStateManager& mgr, const CTransform4f& xf,
                              const CModelFlags& flags) const {
  if ((x128C_ & 0x10) != 0 || !xEBC_.valid()) {
    return;
  }
  // The ids are the bytes the retail loads (the lbz at 0x1900): the first one is the byte the
  // load reads (the mask is the whole byte), the retail steps the pointer for the other three.
  const uchar ids[4] = {x10C0_ & 0xFF, x10C1_, x10C2_, x10C3_};
  for (int i = 0; i < 4; ++i) {
    const CSegId seg(ids[i]);
    const CTransform4f modelXf = xf * GetScaledLocatorTransform(seg) * SegmentRotation_(seg);
    xEBC_.data().Render(mgr, modelXf, GetActorLights(), flags);
  }
}

// fn_14_17DC (0xD8 bytes): the echo emitter's model. When bit 3 of the flag byte at this+0x128C
// is clear (the extrwi at 0x17FC), the byte at this+0x1238 is set (the test at 0x1808) and the
// float at this+0x1264 is positive (the fcmpo at 0x181C), the transform fn_14_2578 builds (the
// call at 0x182C) is copied (0x1838) and the model at this+0x11EC is drawn there (the Render
// call at 0x1870) with the flags the retail builds on the stack: the blend mode 2 (the stb at
// 0x1848), the shader set 0 (the stb at 0x1854), the flags 3 (the sth at 0x1860) and the packed
// colour at this+0x1270 (the stw at 0x1868).
//
// Then the echo emitter at this+0x123C, when it is there (the test at 0x1878), is drawn through
// the renderer: the retail calls the renderer's vtable slot at 0x44 (the lwz at 0x1954 and the
// bctrl at 0x1958). The slot is not identified, so the call is a stand-in.
void CDigitalGuardian::x17DC_(const CStateManager& mgr, const CTransform4f& xf) const {
  if ((x128C_ & 0x08) == 0 && x11EC_.valid() && x1264_ > 0.f) {
    CTransform4f modelXf = CTransform4f::Identity();
    x2578_(modelXf, xf);
    const CModelFlags flags(
        CModelFlags::kT_Two, 0,
        static_cast< CModelFlags::EFlags >(CModelFlags::kF_DepthCompare |
                                           CModelFlags::kF_DepthUpdate),
        CColor(x1270_));
    x11EC_.data().Render(mgr, modelXf, GetActorLights(), flags);
  }
  if (x123C_ != nullptr) {
    // The retail calls the renderer's vtable slot at 0x44 (the lwz at 0x1954 and the bctrl at
    // 0x1958) with the echo emitter in r4 (the lwz at 0x1874). The DOL's __vt__13CCubeRenderer
    // has CCubeRenderer::AddParticleGen(const CParticleGen&) in that slot (the entry at 0x44,
    // the 18th - the counting the SpacePirate lane used for 0x128), and the echo emitter at
    // 0x123C is a CElementGen - a CParticleGen.
    gpRender->AddParticleGen(*x123C_);
  }
}

// fn_14_1700 (0xDC bytes): the toe target's model. The four locator ids at this+0x10D4 are
// walked (the loop at 0x1738-0x17C4) and each one is drawn (the Render call at 0x17B0) unless
// the word at this+0x10DC is 3 (the cmpwi at 0x173C). The transform is the actor's, times the
// locator's scaled transform (the GetScaledLocatorTransform call at 0x176C), times the segment's
// rotation (the two __ml__ calls at 0x177C and 0x178C).
void CDigitalGuardian::x1700_(const CStateManager& mgr, const CTransform4f& xf,
                              const CModelFlags& flags) const {
  // The ids are the bytes the retail loads (the lbz at 0x1744): the first one is the byte the
  // load reads (the mask is the whole byte), the retail steps the pointer for the other three.
  const uchar ids[4] = {x10D4_ & 0xFF, x10D5_, x10D6_, x10D7_};
  // The retail draws the model without testing the optional's flag (0xFA8): its optional
  // default-constructs the model, so the draw is of an empty model. The host's optional leaves
  // the data unconstructed while it is invalid, so the draw is skipped instead.
  if (!xF5C_.valid()) {
    return;
  }
  for (int i = 0; i < 4; ++i) {
    if (x10DC_[i] == 3) {
      continue;
    }
    const CSegId seg(ids[i]);
    const CTransform4f modelXf = xf * GetScaledLocatorTransform(seg) * SegmentRotation_(seg);
    xF5C_.data().Render(mgr, modelXf, GetActorLights(), flags);
  }
}

// fn_14_2578 (0xF4 bytes): the echo emitter's transform. The retail walks the locator at guest
// this+0x10B9 (the GetScaledLocatorTransform call at 0x26A4), multiplies the actor's transform
// with it (the __ml__ at 0x26B4) and, when the byte at this+0x1238 is set, moves the result
// along its own z axis by (this+0x1264 - 0.5f) * (0.8f * the model's height) - the constants
// are .rodata 0x29C and 0x2A0 and the height comes from the model at this+0x11EC (the
// GetBounds call at 0x269C). 0xF4 bytes is over this slice's limit, so it is a stand-in.
void CDigitalGuardian::x2578_(CTransform4f& out, const CTransform4f& xf) const {
  StandInFn("fn_14_2578 (the echo emitter's transform, 0xF4 bytes)");
  out = CTransform4f::Identity();
}

// fn_14_6864 (0x1BC bytes): the constructor's tail builds the module's five models - the leg
// model at 0xE6C (the asset id at 0x8DC, the data block's mLegModel), the shin model at 0xEBC
// (0x8E0, mShinArmor), the knee model at 0xF0C (0x918, mKneeArmor), the transmission beacon's
// model at 0x11EC (0x94C, mTransmissionBeacon) and the toe target's model at 0xF5C (0x7E4,
// mToeTargetModel). Each one is built from a CStaticRes {the asset id, the actor's model scale}
// - the scale comes from the actor's model data at 0x60 (the three lfs at 0x68A0, 0x68A4 and
// 0x68A8) - and assigned into the member's optional (fn_14_6A20 at 0x68D4: the optional's
// assignment - fn_14_6A50 reads the valid flag at +0x4C, constructs the model when the optional
// was empty (fn_14_528) and copies it otherwise (fn_14_6A98); the host's optional_object does
// the same). The temporary is destroyed right after (the __dt__ at 0x68E0). An invalid asset id
// (the cmplwi at 0x689C) leaves the optional empty.
void CDigitalGuardian::x6864_() {
  const CVector3f scale = GetModelData()->GetScale();
  if (mData.mLegModel != kInvalidAssetId) {
    xE6C_ = CModelData(CStaticRes(mData.mLegModel, scale));
  }
  if (mData.mShinArmor != kInvalidAssetId) {
    xEBC_ = CModelData(CStaticRes(mData.mShinArmor, scale));
  }
  if (mData.mKneeArmor != kInvalidAssetId) {
    xF0C_ = CModelData(CStaticRes(mData.mKneeArmor, scale));
  }
  if (mData.mTransmissionBeacon != kInvalidAssetId) {
    x11EC_ = CModelData(CStaticRes(mData.mTransmissionBeacon, scale));
  }
  if (mData.mToeTargetModel != kInvalidAssetId) {
    xF5C_ = CModelData(CStaticRes(mData.mToeTargetModel, scale));
  }
}

// fn_14_A3D4 (vtable slot 11, Render): the module's render flag is set (the store at 0xA3FC)
// and cleared again (the store at 0xA44C) around the module's own RenderSystemsToBeDrawnLast
// (the virtual call at 0xA440 - vtable slot 47, the REL's fn_14_A3A4), which forwards to the
// base's while the flag is set. The character's render mask and target start at zero (the
// stores at 0xA400 and 0xA404) and are only filled when the particles are drawn (the flag byte
// at guest this+0x421, bit 0 - mDrawParticles, the clrlwi. at 0xA40C): the retail's
// GenerateParticlePOIMask (the call at 0xA420) is the host's
// CStateManager::GetCharacterRenderMaskAndTarget. The base's Render is never called: the
// retail's guardian draws its model through the render list (fn_14_A004), not through the
// render path.
void CDigitalGuardian::Render(const CStateManager& mgr) const {
  sRendering = 1;
  uint mask = 0;
  uint target = 0;
  if (mDrawParticles) {
    GenerateParticlePOIMask(mgr, mask, target);
  }
  RenderSystemsToBeDrawnLast(mgr, mask, target);
  sRendering = 0;
}

// vtable slot 13 = fn_14_A138.
void CDigitalGuardian::PreRenderAllViewports(CStateManager& mgr) {
  StandIn("CDigitalGuardian::PreRenderAllViewports (vtable slot 13, fn_14_A138)");
  CPatterned::PreRenderAllViewports(mgr);
}

// vtable slot 15 = fn_14_10C (0x2C): it calls the vtable's HealthInfo slot (0x38) and returns
// its result, which is what the base class's own accessor does.
const CHealthInfo* CDigitalGuardian::GetHealthInfo() const { return CPatterned::GetHealthInfo(); }

// fn_14_9FE4 (vtable slot 16): the "pass through" vulnerability (the call at 0x9FF0) is the
// answer - the guardian is not hurt by anything, the shots pass through it.
const CDamageVulnerability* CDigitalGuardian::GetDamageVulnerability() const {
  return &CDamageVulnerability::PassThroughVulnerabilty();
}

// fn_14_10 (vtable slot 18): the actor's bounding box (the GetBoundingBox call at 0x28) is the
// touch bounds, and the optional is always valid (the byte at 0x18 is set to 1 at 0x30).
rstl::optional_object< CAABox > CDigitalGuardian::GetTouchBounds() const {
  return rstl::optional_object< CAABox >(GetBoundingBox());
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

// fn_14_9DBC (vtable slot 29): when bit 6 (the mask 0x40, the extrwi at 0x9DCC) of the flag byte
// at guest this+0x128C is set and the scannable token (guest this+0xE68, the lwz at 0x9DD4) is
// there, that token's object is the answer (the lwz at 0x9DE0); otherwise the base's scannable
// info (the call at 0x9DE8). The token is locked in the constructor, which the port does not do
// yet, so only the base's answer is reachable.
CScannableObjectInfo* CDigitalGuardian::GetScannableObjectInfo() const {
  return CPatterned::GetScannableObjectInfo();
}

// vtable slot 41 = fn_14_78 (0x10): retail stores skDamageHitTime into the damage-cooldown timer
// at 0x448 and ignores the damage, exactly as CPatterned::TakeDamage does.
void CDigitalGuardian::TakeDamage(const CVector3f& direction, float magnitude) {
  CPatterned::TakeDamage(direction, magnitude);
}

// fn_14_F0 (vtable slot 45): the actor's translation (guest this+0x54, the three loads at
// 0xF0/0xF8/0x100) is the origin.
CVector3f CDigitalGuardian::GetOrigin(const CStateManager& mgr, const CTeamAiRole& role,
                                      const CVector3f& point) const {
  return GetTranslation();
}

// fn_14_A3A4 (vtable slot 47): the base's call, but only while the module's render flag is set
// (the byte at lbl_14_bss_0, the lbz at 0xA3B4). The flag is set by the module's render
// (fn_14_A3D4, the stores at 0xA3FC and 0xA44C), which is not ported yet, so it stays clear.
void CDigitalGuardian::RenderSystemsToBeDrawnLast(const CStateManager& mgr, uint mask,
                                                  uint target) const {
  if (sRendering) {
    CPatterned::RenderSystemsToBeDrawnLast(mgr, mask, target);
  }
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

// fn_14_9AA8: the base's animation state id (guest this+0x6B4) with 3. The asm tests it with
// "subfic r0, r0, 3; cntlzw r0, r0; srwi r3, r0, 5" (0x9AAC..0x9AB4), i.e. the answer is true
// when the id is 3 - the host enum's kAS_Over (the ctor's state id 0 is "not ready").
bool CDigitalGuardian::StateOver(CStateManager& mgr, const CTriggerData& data) const {
  return mAnimationState.GetState() == CAnimationState::kAS_Over;
}

// fn_14_9A9C: bit 7 of the flag byte at guest this+0x128C. The asm loads the byte (0x9A9C) and
// extracts it with "rlwinm r3, r0, 25, 31, 31" (0x9AA0), which keeps the byte's bit 7 (the
// mask 1u << (31 - 24) = 0x80).
bool CDigitalGuardian::IsIdle(CStateManager& mgr, const CTriggerData& data) const {
  return (x128C_ & 0x80) != 0;
}

// fn_14_9A60: the three floats at guest this+0x1278, 0x127C and 0x1280 against 0.0f
// (lbl_14_rodata_264, loaded at 0x9A68 and 0x9A9C); the answer is true when any of them is
// not zero (the three fcmpu/bne pairs at 0x9A70..0x9A8C).
bool CDigitalGuardian::HasFaceTarget(CStateManager& mgr, const CTriggerData& data) const {
  return x1278_.GetX() != 0.f || x1278_.GetY() != 0.f || x1278_.GetZ() != 0.f;
}

// fn_14_9A48: the word at guest this+0x1284 (0x9A48). The asm computes
// "~value | (value + 1)" ("subfic r3, r4, -1; addi r0, r4, 1; or r0, r3, r0" at 0x9A4C..0x9A54)
// and keeps its top bit ("srwi r3, r0, 31" at 0x9A58). That bit is clear only when the value
// is -1 (0xFFFFFFFF): ~(-1) is 0 and (-1) + 1 is 0, and for every other value one of the two
// terms has its top bit set. So the answer is "the knee index is set".
bool CDigitalGuardian::KneeDestroyed(CStateManager& mgr, const CTriggerData& data) const {
  return x1284_ != -1;
}

// fn_14_9A0C: the word at guest this+0x1284 with -1 (the cmpwi at 0x9A1C); when it is -1 the
// answer is false (the li at 0x9A34), otherwise it is "fn_14_705C() == 0" (the call at
// 0x9A24 and the cntlzw/srwi pair at 0x9A28..0x9A2C).
bool CDigitalGuardian::AllKneesDestroyed(CStateManager& mgr, const CTriggerData& data) const {
  if (x1284_ == -1) {
    return false;
  }
  return x705C_() == 0;
}

// fn_14_9994: the head module's uid (guest this+0x1082, the lhz at 0x99A8) is looked up in the
// state manager (the calls at 0x99B8 and 0x99BC); if the head exists and its flag byte
// (head+0x10F4) has bit 5 set (the rlwinm. at 0x99CC keeps the byte's bit 5, the mask
// 1u << (31 - 26) = 0x20, and the bne at 0x99D0 takes the "no" branch), the answer is false.
// Otherwise the answer is whether the float at guest this+0x1274 is at most zero (the fcmpo
// cr0 at 0x99E0, the "cror eq, lt, eq" at 0x99E4 and the mfcr/extrwi pair at 0x99E8..0x99EC:
// the mfcr puts CR0 in the top four bits of r0, so the extracted bit 29 is CR0's eq, which the
// cror has just made "f1 <= f0").
// The head module is WP3 and its loader is a stand-in, so no head can exist in the port and
// retail's head branch cannot be taken: the answer is the float test.
// Retail tests the head's flag byte first (head+0x10F4 bit 0x20, asm 0x99CC): with no head the
// TCastToPtr<CDigitalGuardianHead> is null and the branch falls through to the x1274 test.
bool CDigitalGuardian::ShouldProtectKnees(CStateManager& mgr, const CTriggerData& data) const {
  return x1274_ <= 0.0f;
}

// fn_14_9930: the head module's uid (guest this+0x1082, the lhz at 0x9944) is looked up; if the
// head exists and its flag byte (head+0x10F4) has bit 5 set (the extrwi. at 0x9968 keeps the
// byte's bit 5, the mask 1u << (31 - 26) = 0x20), the answer is false (the bne at 0x996C and
// the li at 0x997C). Otherwise the answer is bit 0 (the mask 1u << (31 - 30) = 0x1) of the flag
// byte at guest this+0x128C (the clrlwi at 0x9974).
// The head module is WP3 and its loader is a stand-in, so no head can exist in the port and
// retail's head branch cannot be taken.
bool CDigitalGuardian::ShouldVortexAttack(CStateManager& mgr, const CTriggerData& data) const {
  return (x128C_ & 0x1) != 0;
}

// fn_14_97F0: the head's flag byte (head+0x10F4, bit 5) is tested first (the extrwi. at
// 0x9854); with no head (WP3) that branch cannot be taken. Then the float at guest this+0x11E0
// must be at most zero (the fcmpo cr0 at 0x9868, the cror eq, lt, eq at 0x986C and the bne at
// 0x9870). Then the four locator ids at guest this+0x10C8 (the count at 0x10C4) are walked
// (the loop at 0x98E0..0x98EC): each locator's world position comes from GetLctrTransform
// (the call at 0x98A0) and the player's (state manager+0x14FC, the lwz at 0x987C) is compared
// with it in the horizontal plane; the squared distance (the fsubs/fmuls/fmadds at
// 0x98B0..0x98C8) must be below the far leg stab range squared (guest this+0x7C8 = the data
// block's +0x08, the fmuls at 0x9884).
bool CDigitalGuardian::ShouldMeleeAttack(CStateManager& mgr, const CTriggerData& data) const {
  if (x11E0_ > 0.0f) {
    return false;
  }
  const float range = mData.mFarLegStabRange;
  const float rangeSq = range * range;
  const CVector3f player = mgr.GetPlayer(0)->GetTranslation();
  for (int i = 0; i < x10C4_; ++i) {
    const CVector3f locator = GetLctrTransform(x10C8_[i]).GetTranslation();
    const float dx = player.GetX() - locator.GetX();
    const float dy = player.GetY() - locator.GetY();
    if (dx * dx + dy * dy < rangeSq) {
      return true;
    }
  }
  return false;
}

// fn_14_97A4: the head's flag byte (head+0x10F4, bit 5) is the whole answer (the extrwi at
// 0x97D4 keeps the byte's bit 5, the mask 1u << (31 - 26) = 0x20), but only when the head
// exists (the cmplwi at 0x97C8 and the beq at 0x97CC answer false without it).
// The head module is WP3 and its loader is a stand-in, so no head can exist in the port and
// retail's head branch cannot be taken: the answer is always false.
bool CDigitalGuardian::ShouldProjectileAttack(CStateManager& mgr, const CTriggerData& data) const {
  return false;
}

// fn_14_308C: the objects connected with the state kSS_InternalState2 (0x49533032) and the
// message kSM_Follow (0x464F4C57) are collected (the FindConnectedObjects call at 0x30CC; the
// retail's own vector copy and free, fn_14_31AC and fn_14_32AC, are the host's rstl::vector),
// and the closest active waypoint to the given position wins: each id is looked up (0x3108) and
// cast to CScriptWaypoint (0x310C); a waypoint that is not active (the byte at +0x20, bit 7,
// 0x3118) is skipped, and its squared distance to the position (the fsubs at 0x3130..0x3150)
// has to be below the best so far (3.4028235e+38f, lbl_14_rodata_2AC, loaded at 0x30F0). The
// winner's id (the u16 at +0x8, 0x3160) is the answer (the sth at 0x3180).
TUniqueId CDigitalGuardian::x308C_(CStateManager& mgr, const CVector3f& position) const {
  const rstl::vector< TUniqueId > objects =
      FindConnectedObjects(mgr, kSS_InternalState2, kSM_Follow);
  TUniqueId best = kInvalidUniqueId;
  float bestSq = FLT_MAX;
  for (uint i = 0; i < objects.size(); ++i) {
    const CScriptWaypoint* waypoint = TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(objects[i]));
    if (waypoint == nullptr || !waypoint->GetActive()) {
      continue;
    }
    const float sq = (waypoint->GetTranslation() - position).MagSquared();
    if (sq < bestSq) {
      bestSq = sq;
      best = waypoint->GetUniqueId();
    }
  }
  return best;
}

// fn_14_9678: the float at guest this+0x11E4 must be at most zero (the fcmpo cr0 at 0x96A0,
// the cror eq, lt, eq at 0x96A4 and the bne at 0x96A8). Then the closest active waypoint
// (fn_14_308C, the call at 0x96D8) is looked up (0x96F0); without it the answer is false
// (the cmplwi at 0x96F4). The target must be further than 100.0f (lbl_14_rodata_290, the
// fcmpo at 0x9734) from the guardian, and then the player must be closer to the target than to
// the guardian (the two distance sums at 0x973C..0x9774, compared at 0x9778).
bool CDigitalGuardian::ShouldJumpAttack(CStateManager& mgr, const CTriggerData& data) const {
  if (x11E4_ > 0.0f) {
    return false;
  }
  const CVector3f player = mgr.GetPlayer(0)->GetTranslation();
  const TUniqueId targetId = x308C_(mgr, player);
  const CEntity* target = mgr.GetObjectById(targetId);
  if (target == nullptr) {
    return false;
  }
  // The id comes from x308C_, which only ever returns waypoint ids, so the cast cannot fail.
  const CVector3f targetPos = TCastToConstPtr< CActor >(target)->GetTranslation();
  const CVector3f selfPos = GetTranslation();
  if ((targetPos - selfPos).MagSquared() <= 100.0f) {
    return false;
  }
  return (targetPos - player).MagSquared() < (selfPos - player).MagSquared();
}

// fn_14_966C: bit 2 (the mask 1u << (31 - 29) = 0x4) of the flag byte at guest this+0x128C (the
// lbz at 0x966C and the extrwi at 0x9670).
bool CDigitalGuardian::VortexStruckPlayer(CStateManager& mgr, const CTriggerData& data) const {
  return (x128C_ & 0x4) != 0;
}

// fn_14_9638: the word at guest this+0x11B8 must be 1 (the cmpwi at 0x963C and the bne at
// 0x9640), and then the float at guest this+0x11C8 (the data block's +0x100, i.e.
// mUnknown4F5D725C) must be at most zero (the fcmpo cr0 at 0x9650, the cror eq, lt, eq at
// 0x9654 and the mfcr/extrwi pair at 0x9658..0x965C).
bool CDigitalGuardian::BreakOutOfVortex(CStateManager& mgr, const CTriggerData& data) const {
  if (x11B8_ != 1) {
    return false;
  }
  return mData.mUnknown4F5D725C <= 0.0f;
}

// fn_14_705C: how many of the four knee ids (guest this+0xFB0, 0xFB2, 0xFB4 and 0xFB6) are not
// kInvalidUniqueId (the four lhz/cmplw pairs at 0x7060..0x709C).
int CDigitalGuardian::x705C_() const {
  int count = 0;
  if (xFB0_ != kInvalidUniqueId.value) {
    ++count;
  }
  if (xFB2_ != kInvalidUniqueId.value) {
    ++count;
  }
  if (xFB4_ != kInvalidUniqueId.value) {
    ++count;
  }
  if (xFB6_ != kInvalidUniqueId.value) {
    ++count;
  }
  return count;
}

// The 14 states of lbl_14_data_204 (.data 0x204; fn_14_D904's stores at 0xDCD0..0xDEB8).

// fn_14_9634: the body is empty (the single blr at 0x9634), like CPatterned::Start.
void CDigitalGuardian::Start(CStateManager& mgr, EStateMsg msg, float dt) {}

// fn_14_95B8: the state message picks the work (the cmpwi chain at 0x95C0..0x95E8): on Activate
// (0) the body controller's locomotion type becomes 1 (pas::kLT_Relaxed, the call at 0x95F8)
// and bit 7 (the mask 0x80) of the flag byte at guest this+0x128C is set (the rlwimi at 0x9604);
// on Deactivate (2) that bit is cleared (the rlwimi at 0x9618); Update (1) and every other
// message do nothing.
void CDigitalGuardian::Idle(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    BodyController()->SetLocomotionType(pas::kLT_Relaxed);
    x128C_ |= 0x80;
    break;
  case kStateMsg_Deactivate:
    x128C_ &= ~0x80;
    break;
  default:
    break;
  }
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

// fn_14_7B90: the number of knees that are set (fn_14_705C, the call at 0x7BA4) picks the
// float that is copied into the member at guest this+0x1274 (the store at 0x7BBC): the data
// block's +0x174 (mUnknown6A754EBD) when all four knees are set (the cmpwi with 4 at 0x7BA8),
// otherwise the block's +0x170 (mUnknownA324E26C) (the two loads at 0x7BB0/0x7BB8).
void CDigitalGuardian::ResetProtectKneesTime(CStateManager& mgr, float dt) {
  x1274_ = x705C_() >= 4 ? mData.mUnknown6A754EBD : mData.mUnknownA324E26C;
}

// fn_14_7B84: the jump timer (the float at guest this+0x938 = the data block's +0x178,
// mJumpTimer, the load at 0x7B84) is copied into the member at guest this+0x11E4 (the store at
// 0x7B88).
void CDigitalGuardian::ResetJumpTime(CStateManager& mgr, float dt) {
  x11E4_ = mData.mJumpTimer;
}

// fn_14_7B24: the minimum leg stab attack time (the float at guest this+0x7CC = the data
// block's +0x0C, mMinLegStabAttackTime, the load at 0x7B3C) goes into the member at guest
// this+0x11E0 (the store at 0x7B40) and the jump timer (guest this+0x938) into the member at
// guest this+0x11E4 (the load at 0x7B44 and the store at 0x7B48). Then the head module is
// asked to reset its attack times (the head's fn_14_13B08, the call at 0x7B6C): the head class
// is WP3 and its loader is a stand-in, so no head can exist in the port and that call cannot
// be made.
void CDigitalGuardian::ResetAttackTimes(CStateManager& mgr, float dt) {
  x11E0_ = mData.mMinLegStabAttackTime;
  x11E4_ = mData.mJumpTimer;
}

void CDigitalGuardian::SetProtectKneesFaceTarget(CStateManager& mgr, float dt) {
  StandInFn("fn_14_77EC");
}

void CDigitalGuardian::SetVortexAttackMode(CStateManager& mgr, float dt) {
  StandInFn("fn_14_775C");
}

// fn_14_76D0 is not ported yet: it calls fn_14_3A44 (the call at 0x7718), 0x108 bytes, which
// walks the four knees, resets their damage vulnerabilities (guest this+0x1014 and this+0x1044)
// and calls fn_14_CE94 - too big for one slice.
void CDigitalGuardian::SetNormalAttackMode(CStateManager& mgr, float dt) {
  StandInFn("fn_14_76D0");
}

// fn_14_768C: the head module is looked up by the uid at guest this+0x1082 (the lhz at 0x7698)
// and, if it exists, asked to disable its attacks with the argument 3 (the li r4, 3 at 0x76B8
// and the call at 0x76BC). The head class is WP3 and its loader is a stand-in, so no head can
// exist in the port and that call cannot be made.
void CDigitalGuardian::DisableHeadAttacks(CStateManager& mgr, float dt) {}

// fn_14_7648: the head module is looked up by the uid at guest this+0x1082 (the lhz at 0x7654)
// and, if it exists, asked to reset its attacks with the argument 4 (the li r4, 4 at 0x7674 and
// the call at 0x7678). The head class is WP3 and its loader is a stand-in, so no head can
// exist in the port and that call cannot be made.
void CDigitalGuardian::ResetHeadAttacks(CStateManager& mgr, float dt) {}

// fn_14_7600: the closest active waypoint to the player (the state manager's player at
// +0x14FC, the lwz at 0x761C, and its position at +0x54, the addi at 0x7624) is picked by
// fn_14_308C (the call at 0x7628) and its id is stored in the member at guest this+0x1168 (the
// store at 0x7630).
void CDigitalGuardian::SelectExitVortexTarget(CStateManager& mgr, float dt) {
  const CVector3f player = mgr.GetPlayer(0)->GetTranslation();
  x1168_ = x308C_(mgr, player);
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
