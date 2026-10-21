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
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/Enemies/CSplitterMainChassis.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CParticleGenInfo.hpp"
#include "MetroidPrime/Collision/CJointCollisionDescription.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Math/CRelAngle.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Weapons/CWeaponAssetInfo.hpp"
#include "MetroidPrime/Weapons/CBeamInfo.hpp"
#include "MetroidPrime/Weapons/CPlasmaProjectile.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSplitterCommandModule.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

#include "Kyoto/CRandom16.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Graphics/CTexture.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CModelData.hpp"

#include <math.h>
#include <stdio.h>

namespace {

// Reports a stand-in the first time it runs, so a log lists what is still a shell.
void StandIn(const char* what) {
  static const char* sSeen[256] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == what) {
      return;
    }
  }
  if (sCount < 256) {
    sSeen[sCount++] = what;
  }
  fprintf(stderr, "[splitter] %s is a port stand-in (see PortCSplitterCommandModule.cpp)\n",
          what);
}

// The head's FSM names: the REL's lbl_75_data_CC (the 17 triggers),
// lbl_75_data_284 (the 14 states) and lbl_75_data_3B8 (the 7 codes). The tables'
// entries are 16 bytes {const char* name, 0, 0xFFFFFFFF, fn} and the module's static
// initialiser (fn_75_7C80, in the REL's .ctors) fills the function word from the
// three-word records of .data 0x0..0x2D0, so the retail function of each name is
// known (docs/module-analysis/Splitter.md section b). The ported ones are the IsScanning
// trigger (fn_75_58E8), the IsInitiallyDocked (fn_75_58DC), the IsDocked (fn_75_58BC), the
// HasDockingTarget (fn_75_57F0), the HasTarget (fn_75_57B4) and the InDetectionRange
// (fn_75_5944) triggers, the Start (fn_75_5564) and the Scanning (fn_75_523C) states, the NotifyDocking code (fn_75_3EEC) and the RaiseShields code (fn_75_3B20);
// the rest are once-logging shells for the retail function they name.
#define SPLITTER_TRIGGER(name) \
  { #name, static_cast< CPatterned::StateMachine::TriggerFunc >(&CSplitterCommandModule::name) }
#define SPLITTER_STATE(name) \
  { #name, static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterCommandModule::name) }
#define SPLITTER_CODE(name) \
  { #name, static_cast< CPatterned::StateMachine::CodeFunc >(&CSplitterCommandModule::name) }
#define SPLITTER_TRIGGER_PORTED(name) \
  { #name, static_cast< CPatterned::StateMachine::TriggerFunc >(&CSplitterCommandModule::name) }
#define SPLITTER_STATE_PORTED(name) \
  { #name, static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterCommandModule::name) }
#define SPLITTER_TRIGGER_PORTED(name) \
  { #name, static_cast< CPatterned::StateMachine::TriggerFunc >(&CSplitterCommandModule::name) }
#define SPLITTER_CODE_PORTED(name) \
  { #name, static_cast< CPatterned::StateMachine::CodeFunc >(&CSplitterCommandModule::name) }


CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    SPLITTER_TRIGGER(StateOver),
    SPLITTER_TRIGGER_PORTED(InDetectionRange),
    SPLITTER_TRIGGER_PORTED(IsScanning),
    SPLITTER_TRIGGER_PORTED(IsInitiallyDocked),
    SPLITTER_TRIGGER_PORTED(IsDocked),
    SPLITTER_TRIGGER(HasDockingPath),
    SPLITTER_TRIGGER_PORTED(HasDockingTarget),
    SPLITTER_TRIGGER(DockingPathOver),
    SPLITTER_TRIGGER_PORTED(HasTarget),
    SPLITTER_TRIGGER(InHoverRange),
    SPLITTER_TRIGGER(InLaserPulseRange),
    SPLITTER_TRIGGER(ShouldDodge),
    SPLITTER_TRIGGER(ShouldAttack),
    SPLITTER_TRIGGER(ShouldFireAgain),
    SPLITTER_TRIGGER(ShouldLaserSweep),
    SPLITTER_TRIGGER(PathOver),
    SPLITTER_TRIGGER(PathShagged),
};

CPatterned::StateMachine::SStateFunction skStates[] = {
    SPLITTER_STATE_PORTED(Start),
    SPLITTER_STATE_PORTED(Idle),
    SPLITTER_STATE(SpawnIdle),
    SPLITTER_STATE(Dead),
    SPLITTER_STATE_PORTED(Scanning),
    SPLITTER_STATE(FaceTarget),
    SPLITTER_STATE(PathFind),
    SPLITTER_STATE_PORTED(Hover),
    SPLITTER_STATE(Dodge),
    SPLITTER_STATE(LaserPulse),
    SPLITTER_STATE(LaserSweep),
    SPLITTER_STATE(LostChassisReaction),
    SPLITTER_STATE(SeekMainChassis),
    SPLITTER_STATE(FollowDockingPath),
};

CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    SPLITTER_CODE_PORTED(NotifyDocking),
    SPLITTER_CODE_PORTED(SelectTarget),
    SPLITTER_CODE(SetTargetDest),
    SPLITTER_CODE(SetDockingDest),
    SPLITTER_CODE(FindBestDodgeDirection),
    SPLITTER_CODE_PORTED(RaiseShields),
    SPLITTER_CODE(ResetAttackTimes),
};

#undef SPLITTER_TRIGGER
#undef SPLITTER_STATE
#undef SPLITTER_CODE
#undef SPLITTER_TRIGGER_PORTED
#undef SPLITTER_STATE_PORTED
#undef SPLITTER_TRIGGER_PORTED
#undef SPLITTER_CODE_PORTED


// The module's own per-frame calls, as named once-logging stand-ins: the name is the retail
// function and its size.

// The module's own calls in the command module's script-message handler (fn_75_654C). Both are
// over 0x80, so each stays a named once-logging shell.

// The head's own move (fn_75_2154, 0xA8) is ported as
// CSplitterCommandModule::PortMoveTo.

// The DOL's CSfxManager::UpdateEmitter (the sound the look-at plays).
void StandInSfxUpdateEmitter() { StandIn("the DOL's CSfxManager::UpdateEmitter"); }


} // namespace

// The head's beam effect (0x1A0 bytes; the REL has no type name for it, so the name is a guess):
// a CActor with no model whose vtable (lbl_75_data_E34) keeps the CActor slots except the
// dtor (fn_75_11804), Think (fn_75_10DC4), PreRender (fn_75_10F1C), AddToRenderer
// (fn_75_10EF0), Render (fn_75_10778) and PreRenderAllViewports (fn_75_1167C). The slot after
// HealthInfo is fn_75_D0, the REL's copy of the DOL's CActor::GetHealthInfo (not ported here).
// A cone opens from 1 degree to the data's angle in front of the head while the head scans.
class CSplitterBeamEffect : public CActor {
public:
  CSplitterBeamEffect(TUniqueId uid, const CEntityInfo& info, const rstl::string& name,
                      const CTransform4f& xf, short textureHeight, float angle, float length,
                      const CColor& cloudColor1, const CColor& cloudColor2,
                      const CColor& addColor1, const CColor& addColor2, float cloudScale,
                      float fadeOffSize, float openSpeed);
  ~CSplitterBeamEffect() override;

  void Think(float dt, CStateManager& mgr) override;
  void PreRender(CStateManager& mgr) override;
  void AddToRenderer(const CStateManager& mgr) const override;
  void Render(const CStateManager& mgr) const override;
  void PreRenderAllViewports(CStateManager& mgr) override;

  void SetOpening(bool opening) { mOpening = opening; }
  bool IsOpening() const { return mOpening; }
  float GetOpenFraction() const { return mOpenFraction; }

private:
  float mMaxAngle;        // 0x158
  float mLength;          // 0x15C
  CColor mCloudColor1;    // 0x160
  CColor mCloudColor2;    // 0x164
  CColor mAddColor1;      // 0x168
  CColor mAddColor2;      // 0x16C
  float mCloudScale;      // 0x170
  float mFadeOffSize;     // 0x174
  float mOpenSpeed;       // 0x178
  CTexture* mTexture;     // 0x17C
  float mAngle;           // 0x180
  int mUnknown184;        // 0x184
  float mOpenFraction;    // 0x188
  CRandom16 mRandom;      // 0x18C
  float mUnknown190;      // 0x190
  float mUnknown194;      // 0x194
  bool mOpening;          // 0x198
};

namespace {
// lbl_75_bss_38, set by the REL's static initialiser fn_75_11AAC from lbl_75_rodata_884: the
// cone's closed angle, one degree in radians.
const float kBeamEffectClosedAngle = 0.017453292f;

// The wrap both the head (0x12F4-0x1330) and the beam (0x10E9C-0x10ECC) do: x minus whole turns
// (fctiwz of x / 2pi, lbl_75_rodata_78 / 7C), plus a turn when that is below 0.0f.
float PortWrapTurn(float x) {
  float r = x - static_cast< float >(static_cast< int >(x * 0.15915494f)) * 6.2831855f;
  if (r < 0.f) {
    r += 6.2831855f;
  }
  return r;
}

void StandInBeamEffect(const char* what) {
  static const char* sLogged[4];
  static int sCount;
  for (int i = 0; i < sCount; ++i) {
    if (sLogged[i] == what) {
      return;
    }
  }
  if (sCount < 4) {
    sLogged[sCount++] = what;
  }
  fprintf(stderr, "[splitter] %s is a port stand-in (see PortCSplitterCommandModule.cpp)\n", what);
}
} // namespace

// @ fn_75_11884
// The ctor (0x11884-0x11A8C): a CActor (0x1197C) with an empty model (fn_75_11A8C, the
// CModelData default ctor), the material list holding the material lbl_75_data_E30 names (0,
// kMT_NoStepLogic; 0x11910-0x11954), default CActorParameters (0x11908) and no next draw node
// (0x118F8-0x11904). The fields take the arguments (0x11990-0x119DC); a kTF_IA8 texture of width 4
// and the given height with one mip is made (0x119A8-0x119FC); the angle starts closed
// (lbl_75_bss_38, 0x11A04-0x11A24), the open fraction and 0x184 at 0.0f/0 (0x11A2C-0x11A30), the
// random source is seeded with 99 (0x11A34), 0x190/0x194 are 0.0f and the cone opens (0x11A54).
CSplitterBeamEffect::CSplitterBeamEffect(TUniqueId uid, const CEntityInfo& info,
                                         const rstl::string& name, const CTransform4f& xf,
                                         short textureHeight, float angle, float length,
                                         const CColor& cloudColor1, const CColor& cloudColor2,
                                         const CColor& addColor1, const CColor& addColor2,
                                         float cloudScale, float fadeOffSize, float openSpeed)
: CActor(uid, name, info, 0, xf, CModelData(), CMaterialList(kMT_NoStepLogic), CActorParameters(),
         kInvalidUniqueId)
, mMaxAngle(angle)
, mLength(length)
, mCloudColor1(cloudColor1)
, mCloudColor2(cloudColor2)
, mAddColor1(addColor1)
, mAddColor2(addColor2)
, mCloudScale(cloudScale)
, mFadeOffSize(fadeOffSize)
, mOpenSpeed(openSpeed)
, mTexture(new CTexture(kTF_IA8, 4, textureHeight, 1))
, mAngle(kBeamEffectClosedAngle)
, mUnknown184(0)
, mOpenFraction(0.f)
, mRandom(99)
, mUnknown190(0.f)
, mUnknown194(0.f)
, mOpening(true) {}

// @ fn_75_11804
// The dtor (0x11804-0x11884): the texture is scheduled for deletion (0x11834) and freed
// (0x11848), then the CActor dtor runs (0x11854).
CSplitterBeamEffect::~CSplitterBeamEffect() {
  mTexture->ScheduleDeletion();
  delete mTexture;
}

// fn_75_10DC4 (Think, 0x10DC4-0x10EF0): after CActor::Think (0x10DE4), an active beam
// (0x10DE8-0x10DF0) moves its open fraction by dt times the open speed toward 1.0f while opening
// (lbl_75_rodata_848, 0x10E00-0x10E24) or toward 0.0f otherwise (lbl_75_rodata_85C,
// 0x10E2C-0x10E50), and its angle becomes the closed angle lerped to the full one by that
// fraction, wrapped to one turn (0x10E54-0x10ED0).
void CSplitterBeamEffect::Think(float dt, CStateManager& mgr) {
  CActor::Think(dt, mgr);
  if (!GetActive()) {
    return;
  }
  if (mOpening) {
    const float open = dt * mOpenSpeed + mOpenFraction;
    mOpenFraction = 1.f < open ? 1.f : open;
  } else {
    const float open = mOpenFraction - dt * mOpenSpeed;
    mOpenFraction = 0.f < open ? open : 0.f;
  }
  mAngle = PortWrapTurn((mMaxAngle - kBeamEffectClosedAngle) * mOpenFraction +
                        kBeamEffectClosedAngle);
}

// fn_75_10F1C (PreRender, 0x10F1C-0x1167C): draws the visible area geometry and the near actors
// flat into the EFB and copies it into the texture (GXCopyTex) for the cone's distortion. Not
// ported yet: a once-logging stand-in.
void CSplitterBeamEffect::PreRender(CStateManager&) { StandInBeamEffect("fn_75_10F1C (the beam effect's PreRender, 0x760)"); }

// fn_75_10EF0 (AddToRenderer, 0x10EF0-0x10F1C): unless the actor is out of the frustum (0x150's
// low bit, 0x10EFC-0x10F04) it is rendered (0x10F08 EnsureRendered).
void CSplitterBeamEffect::AddToRenderer(const CStateManager& mgr) const {
  if (!GetPreRenderClipped()) {
    EnsureRendered(mgr);
  }
}

// fn_75_10778 (Render, 0x10778-0x10DC4): the cone as a textured fan with the cloud and add
// colours. Not ported yet: a once-logging stand-in.
void CSplitterBeamEffect::Render(const CStateManager&) const { StandInBeamEffect("fn_75_10778 (the beam effect's Render, 0x64C)"); }

// fn_75_1167C (PreRenderAllViewports, 0x1167C-0x11804): the bounds start at the translation
// (0x116A4-0x116CC) and take the two cone edges, (0, length, +-length * tan(angle / 2))
// (lbl_75_rodata_840 0.5f, 0x116D0-0x11754) through the transform; they are both the render
// bounds (0xCC) and the other bounds (0xE4) (0x11758-0x117B4). With render bounds dirty
// (0x117B8-0x117C0) the portal state is updated and the flag cleared (0x117CC-0x117DC).
void CSplitterBeamEffect::PreRenderAllViewports(CStateManager& mgr) {
  CAABox bounds(GetTranslation(), GetTranslation());
  const float half = mLength * static_cast< float >(tan(0.5f * mAngle));
  bounds.AccumulateBounds(GetTransform() * CVector3f(0.f, mLength, half));
  bounds.AccumulateBounds(GetTransform() * CVector3f(0.f, mLength, -half));
  SetRenderBounds(bounds);
  SetOtherBounds(bounds);
  if (GetRenderBoundsDirty()) {
    UpdatePortalSystemState(mgr);
    SetRenderBoundsDirty(false);
  }
}

// The state machine names that are not ported yet: each stands in for the retail
// function the REL's initialiser puts in the table and logs it once, so the log
// says which retail body the state machine asked for.
#define SPLITTER_TRIGGER_STANDIN(name, fn)                                       \
  bool CSplitterCommandModule::name(CStateManager&, const CTriggerData&) const {  \
    StandIn(#fn " (the head's " #name " trigger)");                            \
    return false;                                                                  \
  }
#define SPLITTER_STATE_STANDIN(name, fn)                                          \
  void CSplitterCommandModule::name(CStateManager&, EStateMsg, float) {             \
    StandIn(#fn " (the head's " #name " state)");                                \
  }
#define SPLITTER_CODE_STANDIN(name, fn)                                           \
  void CSplitterCommandModule::name(CStateManager&, float) {                        \
    StandIn(#fn " (the head's " #name " code)");                                 \
  }
SPLITTER_TRIGGER_STANDIN(HasDockingPath, fn_75_5864)
SPLITTER_TRIGGER_STANDIN(DockingPathOver, fn_75_57D4)
SPLITTER_TRIGGER_STANDIN(PathOver, fn_75_55B0)
SPLITTER_TRIGGER_STANDIN(PathShagged, fn_75_5568)
SPLITTER_STATE_STANDIN(SpawnIdle, fn_75_5498)
SPLITTER_STATE_STANDIN(SeekMainChassis, fn_75_4138)
SPLITTER_STATE_STANDIN(FollowDockingPath, fn_75_3FE0)
SPLITTER_CODE_STANDIN(SetDockingDest, fn_75_3BEC)
#undef SPLITTER_TRIGGER_STANDIN
#undef SPLITTER_STATE_STANDIN
#undef SPLITTER_CODE_STANDIN

// The state machine entries the area-4 run reaches, whose retail body is ported here.

// fn_75_5564 (the head's Start state, 0x4 bytes): the body is a single return, so the state
// does nothing.
void CSplitterCommandModule::Start(CStateManager&, EStateMsg, float) {}

// fn_75_58DC (the head's IsInitiallyDocked trigger, 0x58DC-0x58E8): the SPLU parameter block's
// first word (0x7C0, the loader's unknown_0xbd80fd94) is read (0x58DC lwz) and its low bit is
// kept (0x58E0 clrlwi with the word's bit 31 - the byte's 0x01); that bit is the answer.
bool CSplitterCommandModule::IsInitiallyDocked(CStateManager&, const CTriggerData&) const {
  return (mData.mUnknownBD80FD94 & 0x1) != 0;
}

// fn_75_58BC (the head's IsDocked trigger, 0x58BC-0x58DC): the id at 0xEFA (the chassis the
// head is docked to) is compared with kInvalidUniqueId (0x58C0 lhz, 0x58C4 lhz, 0x58C8 subf,
// 0x58CC subf, 0x58D0 or, 0x58D4 srwi by 31): the trigger is true when the id is valid.
bool CSplitterCommandModule::IsDocked(CStateManager&, const CTriggerData&) const {
  return mChassisId != kInvalidUniqueId;
}

// fn_75_3B20 (the head's RaiseShields code, 0x3B20-0x3B34): the flag byte at 0xF6A gets its
// 0x08 bit set (0x3B28 rlwimi with the word's bit 28 - the byte's 0x08) and the body ends.
void CSplitterCommandModule::RaiseShields(CStateManager&, float) {
  mAutoDestructFlags |= 0x8;
}

// fn_75_57B4 (the head's HasTarget trigger, 0x57B4-0x57D4): the id at 0xEFE (the copy of the
// chassis's 0xCC0) is compared with kInvalidUniqueId (0x57B8 lhz, 0x57BC lhz, 0x57C0 subf,
// 0x57C4 subf, 0x57C8 or, 0x57CC srwi): the trigger is true when the id is valid.
bool CSplitterCommandModule::HasTarget(CStateManager&, const CTriggerData&) const {
  return mSyncedId != kInvalidUniqueId;
}

// fn_75_57F0 (the head's HasDockingTarget trigger, 0x57F0-0x5864): the id at 0xEFC (the second
// chassis the head checks) is looked up (0x580C GetObjectById) and cast to the main chassis
// (0x5810); a null cast (0x5814, 0x581C) gives false, and so do a chassis that is not alive
// (0x5820 lbz, 0x5824 extrwi with the word's bit 25 - the byte's 0x40, the host's GetAlive) and
// one that is not active (0x582C lbz, 0x5830 extrwi with the word's bit 24 - the byte's 0x80,
// the host's GetActive). What is left is the chassis's own command module id (0xC6C, 0x583C lhz):
// the trigger is true when the chassis has no command module yet (0x5848 bne).
bool CSplitterCommandModule::HasDockingTarget(CStateManager& mgr, const CTriggerData&) const {
  const CSplitterMainChassis* chassis =
      TCastToConstPtr< CSplitterMainChassis >(mgr.GetObjectById(mCheckedChassisId));
  if (chassis == nullptr) {
    return false;
  }
  if (!chassis->GetAlive()) {
    return false;
  }
  if (!chassis->GetActive()) {
    return false;
  }
  return chassis->GetCommandModuleId() == kInvalidUniqueId;
}


// fn_75_4D04 (the head's Hover state, 0x4D04-0x4F90): with the activate message (0x4D2C cmpwi
// r5, 1 - 0x4D40 cmpwi r5, 0) the body controller's locomotion type becomes 3 (0x4D50 li r4, 3
// - the host's kLT_Combat) (0x4D54 SetLocomotionType). With the update message and a frame time
// above zero (0x4D5C-0x4D68: lbl_75_rodata_40 is 0.0f) the head's synced id at 0xEFE (0x4D6C
// lhz) is looked up (0x4D7C GetObjectById) and that actor's aim position is asked for
// (0x4D90-0x4DAC: the vtable's word 0x54 - the host's GetAimPosition - the state manager and
// 0.0f); with an actor there (0x4D80 cmplwi) the head faces it (0x4DB0-0x4DF4: the direction is
// the aim position's x and y less the head's own 0x54 and 0x58, its z is 0.0f - 0x4DD8
// lbl_75_rodata_40 - and FaceDirection takes it with the frame time). The height difference
// (0x4DB0-0x4E04: the aim position's z less the head's own 0x5C) decides the step: when it is
// bigger than the frame time times the data's max linear velocity at 0x7C4 (0x4DFC-0x4E0C) the
// head moves up or down by that step (0x4E10-0x4E54) and the module's own move runs
// (0x4E64 fn_75_2154).
void CSplitterCommandModule::Hover(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    BodyController()->SetLocomotionType(pas::kLT_Combat);
    return;
  }
  if (msg != kStateMsg_Update) {
    return;
  }
  if (dt <= 0.f) {
    return;
  }
  // The retail calls the vtable's word 0x54 on the entity it looked up (0x4D90-0x4DAC); the
  // host's GetAimPosition is CActor's, so the port casts - the asm's test is the null check
  // below (0x4D80 cmplwi r3, 0).
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mSyncedId));
  if (target == nullptr) {
    return;
  }
  const CVector3f aim = target->GetAimPosition(mgr, 0.f);
  const CVector3f pos = GetTranslation();
  BodyController()->FaceDirection(
      CVector3f(aim.GetX() - pos.GetX(), aim.GetY() - pos.GetY(), 0.f), dt);
  const float dz = aim.GetZ() - pos.GetZ();
  const float step = dt * mData.mMaxLinearVelocity;
  if (fabsf(dz) <= step) {
    return;
  }
  PortMoveTo(CVector3f(pos.GetX(), pos.GetY(), pos.GetZ() + (dz > 0.f ? step : -step)), dt);
}

// fn_75_2154 (the head's own move, 0x2154-0x21FC): the auto-destruct timer at 0xEF4 (0x2170
// lfs) decides - above zero (0x2180 fcmpo against 0.0f, 0x2184 ble) the head is teleported to
// the destination (0x2188 SetTranslation); otherwise the difference between the destination and
// the head's own position (0x54, 0x58 and 0x5C - 0x2190-0x21BC) is moved through the head's
// transform (0x21B0 the transform at 0x24, 0x21CC TransposeRotate) and the result moves the
// head in one frame (0x21DC MoveInOneFrameOR).
void CSplitterCommandModule::PortMoveTo(const CVector3f& dest, float dt) {
  if (mAutoDestructTimer > 0.f) {
    SetTranslation(dest);
    return;
  }
  const CVector3f delta = dest - GetTranslation();
  MoveInOneFrameOR(GetTransform().TransposeRotate(delta), dt);
}

// fn_75_5538 (the head's Idle state, 0x5538-0x5564): the state message is compared with 2
// (0x5538 cmpwi r5, 2 - 0x553C bgelr) and with 0 (0x5540 cmpwi r5, 0 - 0x5544 bltlr), so the
// body only runs for the update and the activate message - the retail's deactivate (2) and
// the messages above it return. The body then copies the head's transform's Y column (the
// floats at 0x28, 0x38 and 0x48 - 0x5548-0x5550) into the vector at 0xF24-0xF2C
// (0x5554-0x555C), which is the vector the retail's constructor fills from the editor
// properties (fn_75_6B08) and the head's other states overwrite.
void CSplitterCommandModule::Idle(CStateManager&, EStateMsg msg, float) {
  if (msg != kStateMsg_Update && msg != kStateMsg_Activate) {
    return;
  }
  mEditorProperties = GetTransform().GetColumn(kDY);
}

// The two state machine entries whose retail body is ported: the IsScanning trigger
// (fn_75_58E8, the chassis's scan timer) and the NotifyDocking code (fn_75_3EEC, the head's
// dock to the chassis). The retail runs them as state machine nodes, by name.
bool CSplitterCommandModule::IsScanning(CStateManager& mgr, const CTriggerData&) const {
  return PortChassisTimerRunning(mgr);
}

void CSplitterCommandModule::NotifyDocking(CStateManager& mgr, float) {
  PortDockToChassis(mgr);
}

// fn_75_5944 (the head's InDetectionRange trigger, 0x5944-0x5A3C): false unless the IsScanning
// trigger (0x5968 fn_75_58E8) and the base's InDetectionRange (0x5980) both answer true. Then
// the player (the manager's 0x14FC) less the head's position, with z set to 0.0f (0x598C-0x59D0),
// is dotted with the head's transform's Y column (0x28, 0x38 and 0x48 - 0x59D8-0x5A04) and the
// answer is that dot above the distance times the base's detection cosine at 0x3D8
// (0x59EC-0x5A18: fcmpo and the cr0 gt bit).
// The shipped head's detection angle is 180 degrees (the cosine at 0x3D8 reads -1.0, area 5 run
// 2026-10-21). The dot is never below -dist, so the cone can fail only for a player exactly
// behind the head; that arm is unreachable in shipped data and never ran. The other two false
// arms ran: the timer off, and the timer on with the base range false (the player 41 units away).
// @ fn_75_5944
bool CSplitterCommandModule::InDetectionRange(CStateManager& mgr, const CTriggerData& data) const {
  if (!PortChassisTimerRunning(mgr) || !CPatterned::InDetectionRange(mgr, data)) {
    return false;
  }
  CVector3f delta = mgr.GetPlayer(0)->GetTranslation() - GetTranslation();
  delta.SetZ(0.f);
  const float dist = delta.Magnitude();
  return CVector3f::Dot(delta, GetTransform().GetColumn(kDY)) > dist * mDetectionAngle;
}

// fn_75_523C (the head's Scanning state, 0x523C-0x5444). The message picks the arm
// (0x5260-0x5288): 0 (activate, 0x528C) clears the base's flag byte 0x420's 0x80 bit (the
// host's hit-by-player-projectile flag, 0x528C-0x5298), zeroes the vector at 0xF24-0xF2C
// (0x529C-0x52B4, sZeroVector) and runs the effect-state update (0x52B8 fn_75_1578); 2
// (deactivate, 0x5420) runs only the update; 1 (update, 0x52C0) does the scan; others return.
// The scan: the chassis at 0xEFA is looked up and cast (0x52C0-0x52D8); with one, the head's own
// 0x80 bit set (0x52E0-0x52E8) or the InDetectionRange trigger with the value 0.0f
// (0x52EC-0x5310, fn_75_5944) sets the chassis's 0x80 bit (0x5314-0x5320), which its ShotAt
// trigger (fn_75_E5B8) reads. Then, chassis or not, the turn is the frame time times the data's
// scanning turn speed at 0x7CC (0x5324-0x5334), and the head's Y and X columns flattened to z
// 0.0f (0x28/0x38 and 0x24/0x34 - 0x5324-0x53A4) must both be IsMagnitudeSafe (0x53A8-0x53C0);
// the vector at 0xF24 becomes Slerp(Y normalized, X normalized, turn * 0.017453292f)
// (0x53C4-0x5418, the degrees to radians constant lbl_75_rodata_74).
// Not marked (2026-10-21), three arms never ran:
// - The own-0x80 shortcut. Only the head's ALRT case (0x6710) sets the bit, plus the base's
//   Damage/Touch with a player projectile. Shots reach the head through its collision actors,
//   so the sender is never a projectile. Area 5 has no Alert link to the head, but a DOL sender
//   isn't ruled out.
// - The null chassis. Reachable: the parent's IsDocked check runs after this update, and the
//   chassis can die while the head is docked.
// - The unsafe-axis guard. Not proven unreachable.
void CSplitterCommandModule::Scanning(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate:
    SetHitByPlayerProjectile(false);
    mEditorProperties = CVector3f::Zero();
    PortUpdateEffectStates(mgr);
    break;
  case kStateMsg_Update: {
    CSplitterMainChassis* chassis = TCastToPtr< CSplitterMainChassis >(mgr.ObjectById(mChassisId));
    if (chassis != nullptr &&
        (GetHitByPlayerProjectile() || InDetectionRange(mgr, CTriggerData(0.f)))) {
      chassis->SetHitByPlayerProjectile(true);
    }
    const float turn = dt * mData.mScanningTurnSpeed;
    const CVector3f yAxis = GetTransform().GetColumn(kDY);
    const CVector3f xAxis = GetTransform().GetColumn(kDX);
    const CVector3f forward(CVector2f(yAxis.GetX(), yAxis.GetY()), 0.f);
    const CVector3f right(CVector2f(xAxis.GetX(), xAxis.GetY()), 0.f);
    if (forward.IsMagnitudeSafe() && right.IsMagnitudeSafe()) {
      mEditorProperties = CVector3f::Slerp(forward.AsNormalized(), right.AsNormalized(),
                                           CRelAngle::FromRadians(turn * 0.017453292f));
    }
    break;
  }
  case kStateMsg_Deactivate:
    PortUpdateEffectStates(mgr);
    break;
  default:
    break;
  }
}

// fn_75_50B0 (the head's FaceTarget state, 0x50B0-0x523C). Activate (0x50E8): the vector at
// 0xF24 becomes the head's transform's Y column (0x28, 0x38 and 0x48). Update (0x5104), only
// with a frame time above 0.0f (0x510C ble): the target at 0xEFE is looked up (0x5124); the
// offset to it with z set to 0.0f (0x5130-0x516C) must be IsMagnitudeSafe (0x5170). The turn
// is the frame time times the data's max turn speed (0x7C8, 0x519C-0x51A0) in degrees
// (lbl_75_rodata_74, 0.017453292f): with the angle between the Y column and the offset
// (0x51A4 GetAngleDiff) below it the vector at 0xF24 is the offset normalized (0x51BC-0x51DC),
// else the Slerp from the Y column toward the normalized offset by the turn (0x51E4-0x521C).
// Deactivate and other messages do nothing.
void CSplitterCommandModule::FaceTarget(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    mEditorProperties = GetTransform().GetColumn(kDY);
    return;
  }
  if (msg != kStateMsg_Update || dt <= 0.f) {
    return;
  }
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mSyncedId));
  if (target == nullptr) {
    return;
  }
  CVector3f offset = target->GetTranslation() - GetTranslation();
  offset.SetZ(0.f);
  if (!offset.IsMagnitudeSafe()) {
    return;
  }
  const CVector3f forward = GetTransform().GetColumn(kDY);
  const float turn = 0.017453292f * (dt * mData.mMaxTurnSpeed);
  if (CVector3f::GetAngleDiff(forward, offset) < turn) {
    mEditorProperties = offset.AsNormalized();
  } else {
    mEditorProperties =
        CVector3f::Slerp(forward, offset.AsNormalized(), CRelAngle::FromRadians(turn));
  }
}

// fn_75_5A80 (0x5A80-0x5AD8): the head's laser origin - the translation of the locator at
// 0xEF8 (0x5A9C GetLctrTransform, 0x5AAC-0x5AC0).
// @ fn_75_5A80
CVector3f CSplitterCommandModule::PortLaserOrigin(CStateManager&) const {
  return GetLctrTransform(mLookAtSegment).GetTranslation();
}

// fn_75_5AD8 (0x5AD8-0x5B24): only for a docked head (the chassis id at 0xEFA valid,
// 0x5ADC-0x5AE8) the laser sweep stage at 0xF34 is 0 and the two sweep ends are kept at 0xF38
// and 0xF44 (0x5AEC-0x5B20).
// @ fn_75_5AD8
void CSplitterCommandModule::PortStartLaserSweep(const CVector3f& first, const CVector3f& second) {
  if (mChassisId == kInvalidUniqueId) {
    return;
  }
  mLaserSweepStage = 0;
  mLaserSweepStart = first;
  mLaserSweepEnd = second;
}

// fn_75_44DC (the head's LaserSweep state, 0x44DC-0x4864). Activate (0x4528): the look-at
// vector at 0xF50 becomes the transform's Y column; with the target at 0xEFE (0x4550) the sweep
// ends swap when the end at 0xF44 is closer in angle to that vector than the start at 0xF38
// (0x455C-0x4600, both offsets from the head's position); then the data's laser charge-up sound
// (0x888) plays from the head (0x462C AddEmitter: volume 0x7F, the area at 0x4, acoustics,
// not looped, kMedPriority). Update (0x4634), only with a frame time above 0.0f: the current
// end (0xF38 for stage 0, else 0xF44) is offset from the locator at 0xEF8 (0x4680
// GetLctrTransform); its flat form (z 0.0f) must be IsMagnitudeSafe, else the stage ends (-1,
// 0x482C). The turn is the frame time times the data's laser sweep turn speed (0x808) in degrees
// (lbl_75_rodata_74). Within it of the Y column (0x4710 GetAngleDiff) the vector at 0xF24 is the
// normalized flat offset and 0xF50 the normalized offset; stage 0 then moves to 1 and fires
// (0x4790 fn_75_1974, the laser shot at that end), any other stage ends (-1). Otherwise both
// vectors Slerp toward those by the turn (0x47A4-0x4824). Deactivate (0x4838): the projectile
// despawns (fn_75_1790) and the stage ends (-1).
void CSplitterCommandModule::LaserSweep(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    mLookAtOffset = GetTransform().GetColumn(kDY);
    if (mgr.GetObjectById(mSyncedId) != nullptr) {  // 0x4554-0x4558
      const float startAngle =
          CVector3f::GetAngleDiff(mLookAtOffset, mLaserSweepStart - GetTranslation());
      if (CVector3f::GetAngleDiff(mLookAtOffset, mLaserSweepEnd - GetTranslation()) <
          startAngle) {
        const CVector3f start = mLaserSweepStart;
        mLaserSweepStart = mLaserSweepEnd;
        mLaserSweepEnd = start;
      }
    }
    CSfxManager::AddEmitter(static_cast< ushort >(mData.mSoundLaserChargeUp & 0xFFFF),
                            GetTranslation(), 0x7F, GetCurrentAreaId().Value(), true, false,
                            CSfxManager::kMedPriority);
  } else if (msg == kStateMsg_Update) {
    if (dt <= 0.f) {
      return;
    }
    const CVector3f end = mLaserSweepStage == 0 ? mLaserSweepStart : mLaserSweepEnd;
    const CVector3f offset = end - GetLctrTransform(mLookAtSegment).GetTranslation();
    const CVector3f flat(offset.GetX(), offset.GetY(), 0.f);
    if (!flat.IsMagnitudeSafe()) {
      mLaserSweepStage = -1;
      return;
    }
    const CVector3f forward = GetTransform().GetColumn(kDY);
    const float turn = 0.017453292f * (dt * mData.mLaserSweepTurnSpeed);
    if (CVector3f::GetAngleDiff(forward, flat) < turn) {
      mEditorProperties = flat.AsNormalized();
      mLookAtOffset = offset.AsNormalized();
      if (mLaserSweepStage == 0) {
        mLaserSweepStage = 1;
        PortFireLaserSweep(mgr, end);
      } else {
        mLaserSweepStage = -1;
      }
    } else {
      mEditorProperties =
          CVector3f::Slerp(forward, flat.AsNormalized(), CRelAngle::FromRadians(turn));
      mLookAtOffset = CVector3f::Slerp(mLookAtOffset.AsNormalized(), offset.AsNormalized(),
                                       CRelAngle::FromRadians(turn));
    }
  } else if (msg == kStateMsg_Deactivate) {
    PortDespawnProjectile(mgr);
    mLaserSweepStage = -1;
  }
}

// fn_75_1974 (the head's laser shot, 0x1974-0x1C0C): the laser sweep beam's info (data 0x81C)
// becomes a CBeamInfo with the attributes 0x91 (0x199C fn_80239B64, the DOL's TLdrToBeamInfo;
// 0x19A8 fn_75_1C0C copies it). A new id (0x19B4 AllocateUniqueId) gets the 0x6A8-byte
// CPlasmaProjectile "LaserSweepBeam" (lbl_75_rodata_A0 + 0x245; 0x19D8 new, 0x1AA0 ctor): the
// weapon at 0xE84, weapon type 2, the identity transform, material 0x14 (kMT_ProjectilePassthrough),
// the converted sweep damage at 0xCF0, the area at 0x4, this actor as owner, a default
// CWeaponAssetInfo (8 entries of -1, 0x19E4-0x1A20), not drawn owner first and the attributes
// 0x21000. When it exists (0x1AB8): it fires (the vtable word 0x94, Fire with false; 0x1B2C) along
// LookAt from the locator at 0xEF8 (0x1ACC GetLctrTransform) toward the given end with the up
// vector (0x1B08), takes the chassis id at 0xEFA as its next draw node (its 0x12A, 0x1B48) and is
// added (0x1B4C AddObject). Then the data's laser sweep sound (0x884) loops from the beam's
// collision point (its 0x444-0x44C, CBeamProjectile::GetCurrentPos) with the 3D parameters 150.0f
// and 0.1f (lbl_75_rodata_2C / lbl_75_rodata_48), flags 1, volume 0x7F and priority 0x23, the up
// vector as direction, the area at 0x4, acoustics on and looped (0x1BD8 AddEmitter); its handle
// goes to 0xF5C and the beam's id to 0xF02 (0x1BDC-0x1BE8). Last the effect states update
// (0x1BF4 fn_75_1578), with or without the beam.
// @ fn_75_1974
void CSplitterCommandModule::PortFireLaserSweep(CStateManager& mgr, const CVector3f& end) {
  const CBeamInfo beamInfo = TLdrToBeamInfo(mData.mLaserSweepBeamInfo, 0x91);
  const TUniqueId beamId = mgr.AllocateUniqueId();
  CPlasmaProjectile* beam = rs_new CPlasmaProjectile(
      mLaserSweepProjectileInfo.Token(), rstl::string_l("LaserSweepBeam"), EWeaponType(2),
      beamInfo, CTransform4f::Identity(), kMT_ProjectilePassthrough, mData.mLaserSweepDamage,
      beamId, GetCurrentAreaId(), GetUniqueId(), CWeaponAssetInfo(), false, 0x21000);
  // The null arm is an allocation failure only (rs_new); it never happens in a run.
  if (beam != nullptr) {  // 0x1AB8-0x1ABC
    beam->Fire(CTransform4f::LookAt(GetLctrTransform(mLookAtSegment).GetTranslation(), end,
                                    CVector3f::Up()),
               mgr, false);
    beam->SetNextDrawNode(mChassisId);
    mgr.AddObject(*beam);
    CAudioSys::C3DEmitterParmData parms(150.f, 0.1f, 1, 0x7F, 0x23);
    parms.mPos = beam->GetCurrentPos();
    parms.mDir = CVector3f::Up();
    parms.mSfxId = static_cast< ushort >(mData.mSoundLaserSweep & 0xFFFF);  // 0x1BD0-0x1BD4
    mLookAtSfxHandle = CSfxManager::AddEmitter(parms, GetCurrentAreaId().Value(), true, true,
                                               CSfxManager::kMedPriority);
    mLookAtTargetId = beamId;
  }
  PortUpdateEffectStates(mgr);
}

// fn_75_5A40 (the head's StateOver trigger, 0x5A40-0x5A50): a REL copy of the DOL's
// CPatterned::GetAnimOver (0x80151990) - the animation state at 0x6B4 is kAS_Over (3).
bool CSplitterCommandModule::StateOver(CStateManager& mgr, const CTriggerData& data) const {
  return GetAnimOver(mgr, data);
}

// fn_75_55F8 (the head's ShouldLaserSweep trigger, 0x55F8-0x5610): the laser sweep stage at
// 0xF34 is not -1 (0x55FC-0x5608: subfic -1, addi 1, or, srwi 31).
// @ fn_75_55F8
bool CSplitterCommandModule::ShouldLaserSweep(CStateManager&, const CTriggerData&) const {
  return mLaserSweepStage != -1;
}

// The head's own effect-state update (fn_75_1578, 0x218) is ported as
// CSplitterCommandModule::PortUpdateEffectStates.

// fn_75_3E60 (the head's SelectTarget code, 0x3E60-0x3EEC): with the head's synced id at 0xEFE
// invalid (0x3E84 lhz, 0x3E8C cmplw, 0x3E90 bne) the id of the actor the state manager holds at
// 0x14FC (0x3E94 lwz - the player, the actor the head aims at) is taken (0x3E98 lhz of that
// actor's own id at 0x8) and kept at 0xEFE (0x3E9C sth); then fn_75_1578 runs (0x3EA8) and the
// head sends the state 'ATTK' to its connections (0x3EB0 lis 0x4154, 0x3EC0 addi 0x544b,
// 0x3ED0 bl SendScriptMsgs).
//
// The retail's call is SendScriptMsgs(this, 0x4154544B, mgr, id, -1): the state, the state
// manager, the head's own unique id (0x3EAC lhz 0x8(r30) - the entity's own id - copied to the
// stack at 0x3ECC sth) and the message 0xFFFFFFFF (0x3EC8 li r7, -1). The retail hands that id
// over by address (0x3EC4 addi r6, sp+0xc), because its TUniqueId is a 16-bit class; the host's
// CEntity::SendScriptMsgs takes it by value, so the port passes the id itself. The host's names
// for the two constants: kSS_Attack (0x4154544B) and kSM_None (0xFFFFFFFF). The message
// argument is the one a connection's own message must differ from, so "none" reaches every
// connection but the ones that ask for no message.
void CSplitterCommandModule::SelectTarget(CStateManager& mgr, float) {
  if (mSyncedId == kInvalidUniqueId) {
    mSyncedId = mgr.GetPlayer(0)->GetUniqueId();
  }
  PortUpdateEffectStates(mgr);
  SendScriptMsgs(kSS_Attack, mgr, GetUniqueId(), kSM_None);
}


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
, mProjectileInfo(data.mLaserPulseProjectile, data.mLaserPulseDamage)
// fn_75_6B08 @0x00006BD8-0x00006BEC: the second CProjectileInfo at 0xe84, built from the laser
// sweep beam's weapon system (data +0x60, the beam info's second word) and a default damage
// (fn_75_722C: weapon -1, every amount 0.0f, the sounds 0xFFFF); fn_75_1974 fires it.
, mLaserSweepProjectileInfo(data.mLaserSweepBeamInfo.weaponSystem, CDamageInfo()) {
  // fn_75_6B08 @0x00006DEC-0x00006E5C: the material filter - the include mask is the data word
  // 0x3B (lbl_75_data_428, kMT_Solid), the exclude mask the data words 0x2F and 0x21
  // (lbl_75_data_42C and lbl_75_data_430, kMT_CollisionActor and kMT_Character) and the type 3
  // (0x00006E3C, the include/exclude one). Excluding kMT_Character keeps the docked head out of
  // the chassis's collider list (CSortedListManager asks both actors' filters).
  SetMaterialFilter(CMaterialFilter(CMaterialList(kMT_Solid),
                                    CMaterialList(kMT_CollisionActor, kMT_Character),
                                    CMaterialFilter::kFT_IncludeExclude));
  // fn_75_6B08 @0x00006E60-0x00006EF8: the dodge distance at 0xEE8 is the model's scale x times
  // the animation distance of a right (3) dodge (1) step (body state 3, fn_75_6F1C).
  mDodgeDistance = GetModelData()->GetScale().GetX() *
                   GetAnimationDistance(CPASAnimParmData(pas::kAS_Step, CPASAnimParm::FromEnum(3),
                                                         CPASAnimParm::FromEnum(1)));
  // The retail constructor also builds the shield vulnerability at 0xeac (a copy of the base class's) and the derived state
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

// The vtable word 0x4C of the object fn_75_2BE0 returns. The port has no name for that DOL
// virtual yet, so it stays a shell. The CCollisionActor's virtual at the vtable word 0x38 (the
// float at its +0x4) is CCollisionActor::HealthInfo() (found while porting fn_75_2CE0, which
// calls it), so the body asks the host for it now.
void StandInVTable0x4C() { StandIn("the vtable word 0x4C of fn_75_2BE0's result"); }

// fn_75_2CE0 (not marked: the run only reached the null collision-actor lookup)
// The head's shield (0x2CE0-0x3004). The flag (the third argument, r5) is the "a shield goes up"
// one: the asm tests its low byte (0x2D0C clrlwi with the word's bit 24 - the byte's 0xFF). With
// it set the shield-on sound (the sound id at 0x89C) is added (0x2D1C-0x2E44: the volume 0x7F,
// the area at 0x4, useAcoustics true, looped false) and its handle is dropped - the retail writes
// it to a stack slot it never reads. Then the tracked emitter (the handle at 0xF60) is removed and
// cleared (0x2D48-0x2D6C). The beam collision actor (the id at 0xF00) is looked up (0x2D7C
// ObjectById) and cast (0x2D80); with no actor the body ends (0x2D88). Its health info's health
// (the float at +0x4, 0x2DA0) is set to the shield's HP (the float at 0x804). The state at 0xEE0
// (0x2DA8-0x2DC0) then picks one of three:
//   1 (0x2DC4): the character material (0x21) goes to the collision actor and the material 0x36 to
//     the head, the light shield's vulnerability (0xD0C) goes to the collision actor, the
//     "LightShield" effect is on and the "DarkShield" effect is off, and the light shield's sound
//     (the sound id at 0x894) is added as a looping emitter whose handle goes to 0xF60
//     (0x2E78-0x2E80).
//   2 (0x2E88): the same with the material removed from the collision actor, the dark shield's
//     vulnerability (0xD3C), the effects swapped and the dark shield's sound (the sound id at
//     0x898).
//   otherwise (0x2F4C): both materials are removed, the collision actor's vulnerability becomes the
//     pass-through one (fn_800DBB50) and both effects are off.
void CSplitterCommandModule::PortUpdateShield(CStateManager& mgr, bool shieldUp) {
  if (shieldUp) {
    // 0x2D1C-0x2E44: the shield-on sound. The retail's emitter position is the vector at 0x54
    // and its area the word at 0x4; the host's are the actor's translation and its area.
    CSfxManager::AddEmitter(static_cast< ushort >(mData.mSoundShieldOn & 0xFFFF), GetTranslation(),
                            0x7F, GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
  }
  // 0x2D48-0x2D6C: the tracked emitter (the handle at 0xF60) is removed and the handle cleared.
  if (mSoundHandle) {
    CSfxManager::RemoveEmitter(mSoundHandle);
    mSoundHandle.Clear();
  }
  // 0x2D6C-0x2D88: the beam collision actor (the id at 0xF00) is looked up and cast; with no
  // actor the body ends.
  CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(mSoundTargetId));
  if (actor == nullptr) {
    return;
  }
  // 0x2D8C-0x2DA0: the collision actor's health info (the vtable word 0x38) and its health (the
  // float at +0x4) get the shield's HP (the float at 0x804).
  actor->HealthInfo()->SetHP(mData.mShieldHP);
  switch (mCollisionState) {
  case 1: {
    // 0x2DC4-0x2EA4: the character material (0x21) goes to the collision actor and the material
    // 0x36 to the head.
    actor->AddMaterial(kMT_Character, mgr);
    AddMaterial(kMT_Unknown54, mgr);
    // 0x2EA8-0x2EB0: the light shield's vulnerability (0xD0C) goes to the collision actor.
    actor->SetDamageVulnerability(mData.mLightShieldVulnerability);
    // 0x2EB4-0x2F0C: the "LightShield" effect (rodata 0x10) is on and the "DarkShield" effect
    // (rodata 0x14) is off.
    AnimationData()->SetEffectState(rstl::string_l("LightShield"), true, mgr);
    AnimationData()->SetEffectState(rstl::string_l("DarkShield"), false, mgr);
    // 0x2F14-0x2F80: the light shield's sound (the sound id at 0x894) is added as a looping
    // emitter and its handle goes to 0xF60.
    mSoundHandle = CSfxManager::AddEmitter(static_cast< ushort >(mData.mSoundLightShield & 0xFFFF),
                                           GetTranslation(), 0x7F, GetCurrentAreaId().Value(), true,
                                           true, CSfxManager::kMedPriority);
    break;
  }
  case 2: {
    // 0x2E88-0x2EA4: the character material (0x21) is removed from the collision actor and the
    // material 0x36 is added to the head.
    actor->RemoveMaterial(kMT_Character, mgr);
    AddMaterial(kMT_Unknown54, mgr);
    // 0x2EA8-0x2EB0: the dark shield's vulnerability (0xD3C) goes to the collision actor.
    actor->SetDamageVulnerability(mData.mDarkShieldVulnerability);
    // 0x2EB4-0x2F0C: the effects are swapped.
    AnimationData()->SetEffectState(rstl::string_l("LightShield"), false, mgr);
    AnimationData()->SetEffectState(rstl::string_l("DarkShield"), true, mgr);
    // 0x2F14-0x2F80: the dark shield's sound (the sound id at 0x898) is added as a looping
    // emitter and its handle goes to 0xF60.
    mSoundHandle = CSfxManager::AddEmitter(static_cast< ushort >(mData.mSoundDarkShield & 0xFFFF),
                                           GetTranslation(), 0x7F, GetCurrentAreaId().Value(), true,
                                           true, CSfxManager::kMedPriority);
    break;
  }
  default: {
    // 0x2F4C-0x2F6C: both materials are removed.
    actor->RemoveMaterial(kMT_Character, mgr);
    RemoveMaterial(kMT_Unknown54, mgr);
    // 0x2F6C-0x2F78: the collision actor's vulnerability becomes the pass-through one.
    actor->SetDamageVulnerability(CDamageVulnerability::PassThroughVulnerabilty());
    // 0x2F7C-0x2FD8: both effects are off.
    AnimationData()->SetEffectState(rstl::string_l("LightShield"), false, mgr);
    AnimationData()->SetEffectState(rstl::string_l("DarkShield"), false, mgr);
    break;
  }
  }
}

// fn_75_2BE0 (not marked: the run only reached the state-0 return)
// The head's shield particle effect (0x2BE0-0x2BC4): the state at 0xEE0 picks the name - 1 gives
// "LightShield" (rodata 0x10, 0x2C18-0x2C34), any other state gives "DarkShield" (rodata 0x14,
// 0x2C38-0x2C50) and the state 0 gives nothing (0x2C00-0x2C04, the return at 0x2CA0). The name is
// the animation data's first particle effect (0x2C88 GetFirstParticleEffect), which is what the
// body returns.
CParticleGenInfo* CSplitterCommandModule::PortGetShieldEffect() {
  if (mCollisionState == 0) {
    return nullptr;
  }
  const char* name = mCollisionState == 1 ? "LightShield" : "DarkShield";
  return AnimationData()->GetFirstParticleEffect(rstl::string_l(name));
}

// @ fn_75_10D0
// The module's own transform (0x10D0-0x1188): the actor's own transform (the guest's at 0x24) is
// copied (0x10F0-0x10F4), the "Beam_LCTR" locator's transform (rodata 0x293, 0x1118
// GetLocatorTransform) gives the point, the point is moved through the copy (0x1140 the
// operator*) and the result's translation goes into the copy (0x1154-0x115C), which is the
// result.
void CSplitterCommandModule::PortModuleTransform(CTransform4f& out) const {
  CTransform4f copy = GetTransform();
  const CVector3f moved = copy * GetLocatorTransform(rstl::string_l("Beam_LCTR")).GetTranslation();
  copy.SetTranslation(moved);
  out = copy;
}

// fn_75_3004 (0x1A4) - the command module's collision update. The body: the module's own call
// fn_75_2BE0 (0x3024) gives an object whose vtable word 0x4C is called with the colour at 0xF04
// (0x3030-0x3040); the DOL's CSfxManager::UpdateEmitter runs with the handle at 0xF60, the
// position at 0x54 and the byte 0x7f (0x3044-0x3074); the id at 0xF00 is looked up (0x3088) and
// cast to a CCollisionActor (0x308C); the manager at 0xE58, when there is one, is deactivated
// when this actor has no chassis (0x3098-0x30C0 - the DOL's
// CCollisionActorManager::SetActive, with the flag built by the cntlzw idiom at 0x30B4-0x30BC);
// the state in 0xEE0 (0..2) with the flag byte 0xF6A's 0x08 set asks the collision actor for the
// field at its 0x4 (0x30DC-0x3100) and, at zero or below, clears the state, sets the timer 0xF10
// to the data's 0x40 and runs fn_75_2CE0 (0x3114-0x3130); a state out of range with the flag set
// and the timer at zero or below advances the state to 1 or 2 (0x3138-0x3170) and runs
// fn_75_2CE0 with it (0x3174-0x3188).
void CSplitterCommandModule::PortUpdateCollision(CStateManager& mgr) {
  CParticleGenInfo* effect = PortGetShieldEffect();
  if (effect != nullptr) {
    StandInVTable0x4C();
  }
  StandInSfxUpdateEmitter();
  CEntity* entity = mgr.ObjectById(mSoundTargetId);
  if (entity == nullptr) {
    return;
  }
  CCollisionActor* actor = TCastToPtr< CCollisionActor >(entity);
  if (actor == nullptr) {
    return;
  }
  if (mCollisionActorManager != nullptr) {
    mCollisionActorManager->SetActive(mgr, mChassisId == kInvalidUniqueId);
  }
  if (mCollisionState > 0 && mCollisionState < 3) {
    if ((mAutoDestructFlags & 0x8) != 0) {
      // 0x30E8-0x3110: the collision actor's virtual at the vtable word 0x38 is
      // CCollisionActor::HealthInfo() and the float at its +0x4 is the health; the shield goes
      // down when that health is zero or below.
      if (actor->HealthInfo()->GetHP() <= 0.f) {
        mCollisionState = 0;
        mCountdownTimerA = mData.mResetShieldTime;
        // 0x3124-0x3130: the retail calls fn_75_2CE0 with the flag clear (the shield goes down).
        PortUpdateShield(mgr, false);
      }
    }
  } else if ((mAutoDestructFlags & 0x8) != 0 && mCountdownTimerA <= 0.f) {
    mCollisionState = mPreviousCollisionState == 1 ? 2 : 1;
    mPreviousCollisionState = mCollisionState;
    // 0x317C-0x3188: the retail calls fn_75_2CE0 with the flag set (a shield goes up).
    PortUpdateShield(mgr, true);
  }
}

// fn_75_3EEC (not marked: the run only reached the null chassis lookup)
// The head's "dock to the chassis" (0x3EEC-0x3FE0): the id at 0xEFC (the connected object the
// XAND message found) is looked up (0x3F18 ObjectById) and cast to a main chassis (0x3F1C); with
// no chassis the body ends (0x3F24). The chassis is asked to accept this head (0x3F3C
// fn_75_E6E4, with the head's own id at 0x8); when it does not (0x3F40, the low byte) the body
// ends. Then the chassis's id goes to 0xEFA (0x3F50) - the id fn_75_31A8 and the beam-effect
// gate fn_75_58E8 read - the id at 0xEFC is cleared (0x3F58) and the id goes to 0x12A (0x3F60).
// The docking sound (the sound id at 0x88C) is added when the flag byte 0xF6A's 0x02 bit is clear
// (0x3F6C extrwi, the word's bit 30) and the data's flag word at 0x7C0 has its 0x80 bit clear
// (0x3F78 clrlwi, the word's bit 31); the flag byte's 0x02 bit is cleared either way (0x3FB4
// rlwimi, the word's bit 30). The locomotion type becomes 1 (kLT_Relaxed, 0x3FC4
// SetLocomotionType).
void CSplitterCommandModule::PortDockToChassis(CStateManager& mgr) {
  CSplitterMainChassis* chassis =
      TCastToPtr< CSplitterMainChassis >(mgr.ObjectById(mCheckedChassisId));
  if (chassis == nullptr) {
    return;
  }
  if (!chassis->PortAcceptHead(mgr, GetUniqueId())) {
    return;
  }
  mChassisId = mCheckedChassisId;
  mCheckedChassisId = kInvalidUniqueId;
  mClearedId = mChassisId;
  if ((mAutoDestructFlags & 0x2) == 0 && (mData.mUnknownBD80FD94 & 0x80000000) == 0) {
    // 0x3F80-0x3FA8: the docking sound; its handle is dropped (the retail writes it to a stack
    // slot it never reads).
    CSfxManager::AddEmitter(static_cast< ushort >(mData.mSoundDocking & 0xFFFF), GetTranslation(),
                            0x7F, GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
  }
  mAutoDestructFlags &= ~static_cast< uchar >(0x2);
  BodyController()->SetLocomotionType(pas::kLT_Relaxed);
}

// @ fn_75_32F4
// The command module's flash and death (0x32F4-0x3460): three timers are counted down (0xF10 at
// 0x332C, 0xF0C at 0x333C and 0xF14 at 0x3344) and two counted up (0xF18 at 0x3354 and 0xF1C at
// 0x3360). While the flash weight 0xF08 is above zero (0x336C-0x3370) it is counted down and
// clamped at zero (0x3374-0x338C) and the colour at 0xF04 is the Lerp of the white and the base
// colour (0x450) with the weight capped at 1.0f (0x3390-0x33A8, 0x33C0). Then the flag byte at
// 0xF6A (the word's bit 29, the byte's 0x04, 0x33CC-0x33D4) and the alive bit of the byte at
// 0x420 (the word's bit 25, the byte's 0x40, 0x33D8-0x33E0) gate the death timer 0xEF0, which is
// counted down (0x33EC-0x33F4) and, at zero or below (0x33FC-0x3404), kills the actor with the
// direction (the transform's Y column, the CActor's 0x28, 0x38 and 0x48) and the state
// 0x5241544C ('RATL', 0x340C-0x3434).
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
      Death(mgr, GetTransform().GetColumn(kDY), kSS_DeathRattle);
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
      PortUpdateEffectStates(mgr);
    }
    mChassisTimer = 3.f;
    return;
  }
  if ((mData.mUnknownBD80FD94 & 0x8) != 0 && mChassisTimer <= 0.f &&
      mCheckedChassisId == kInvalidUniqueId) {
    PortFindChassis(mgr);
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

// The DOL's CEntity virtual the projectile's despawn calls (the vtable word 0x24, 0x17C8-0x17DC
// of fn_75_1790): the port has no name for it, so the call stays a once-logging shell.

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

// @ fn_75_58E8
// The gate the head's beam-effect spawner asks first (0x58E8-0x5944): the chassis id at 0xEFA
// (0x58F4 lhz) is looked up (0x5904 GetObjectById) and cast to the main chassis (0x5908
// TCastToPtr); a null cast gives false (0x5930 li r3, 0) and the chassis's scan timer at 0xD28
// decides the rest (0x5918 lfs, 0x5920 fcmpo against zero and 0x5928 extrwi, which takes cr0's
// greater-than bit).
bool CSplitterCommandModule::PortChassisTimerRunning(CStateManager& mgr) const {
  const CSplitterMainChassis* chassis =
      TCastToConstPtr< CSplitterMainChassis >(mgr.GetObjectById(mChassisId));
  if (chassis == nullptr) {
    return false;
  }
  return chassis->GetScanTimer() > 0.f;
}

// @ fn_75_1188
// The head's beam-effect spawner (0x1188-0x14CC). The beam effect's id (0xF68) is looked up
// (0x11B0 ObjectById), the module's own transform (fn_75_10D0, 0x11C0) is built and copied
// (0x11CC) and set on the beam effect (0x11E0 SetTransform), and the gate (fn_75_58E8, 0x1200)
// decides whether the body runs at all (0x1204-0x1208). With a beam effect already there the
// body goes to 0x13EC, which sets the beam's byte at 0x198 (0x13F8) and updates the sound
// (0x142C UpdateEmitter). Otherwise the base's field at 0x3D0 must be above zero (0x1214-0x1224),
// a fresh id is allocated (0x1230 AllocateUniqueId) and kept at 0xF68 (0x1244), and the
// 0x1A0-byte beam effect actor is created (0x1258), built (0x12A0 the CEntityInfo, 0x12B8 the
// name, 0x12C4-0x1334 the rotation) and added to the manager (0x13B4 AddObject).
void CSplitterCommandModule::PortSpawnBeamEffect(CStateManager& mgr) {
  // 0x11A4-0x11B4: the beam effect's id (0xF68) is looked up; the id only ever holds the beam
  // effect, so the retail's unchecked use as one is kept.
  CSplitterBeamEffect* beam = static_cast< CSplitterBeamEffect* >(mgr.ObjectById(mBeamEffectId));
  // 0x11B8-0x11E0: the module's own transform (fn_75_10D0) is built and set on the beam.
  CTransform4f moduleTransform = GetTransform();
  PortModuleTransform(moduleTransform);
  if (beam != nullptr) {  // port limit: the retail calls SetTransform (0x11E0) with no test
    beam->SetTransform(moduleTransform);
  }
  // 0x1200-0x1208: the gate (fn_75_58E8) decides whether the body runs.
  if (!PortChassisTimerRunning(mgr)) {
    if (beam == nullptr) {  // 0x1434-0x1438
      // 0x1494-0x14B4: the id at 0xF68 is cleared and the sound removed, its handle cleared.
      mBeamEffectId = kInvalidUniqueId;
      CSfxManager::RemoveEmitter(mBeamSfxHandle);
      mBeamSfxHandle = CSfxHandle();
      return;
    }
    // 0x143C-0x1490: the beam closes (0x198 cleared, 0x1440); once it is fully closed (its
    // 0x188 is 0.0f, lbl_75_rodata_40, 0x1450-0x1460) it is deleted and the id cleared.
    beam->SetOpening(false);
    if (!beam->IsOpening() && beam->GetOpenFraction() == 0.f) {
      mgr.DeleteObjectRequest(mBeamEffectId);
      mBeamEffectId = kInvalidUniqueId;
    }
    return;
  }
  if (beam == nullptr) {  // 0x120C-0x1210
    // 0x1214-0x1224: the base's field at 0x3D0 (its copy of the patterned info's detection
    // range) must be above zero.
    if (GetDetectionRange() <= 0.f) {
      return;
    }
    // 0x1230-0x1244: a fresh id, kept at 0xF68. 0x1248-0x1380: the beam effect is built with the
    // CEntityInfo the retail makes at 0x12A0 (the area id, NullConnectionList, true and
    // kInvalidEditorId), the name (0x12B8, the rodata string at 0xA0 + 0x230), the module
    // transform, a texture height of 0x40 (0x136C), the data's angle (0x864) in radians wrapped
    // to one turn (0x12C4-0x1334), the detection range as the length (0x1370) and the data's
    // colours (0x868-0x874), cloud scale, fade-off size and open speed (0x878-0x880). It is
    // added to the manager (0x13B4 AddObject) and its looped sound (the data's 0x890) starts at
    // the head (0x13B8-0x13E8: volume 0x7F, the area, acoustics, looped, medium priority).
    mBeamEffectId = mgr.AllocateUniqueId();
    const CEntityInfo info(GetCurrentAreaId(), CEntity::NullConnectionList, true,
                           kInvalidEditorId);
    const rstl::string name = rstl::string_l("Splitter Beam Effect");
    const SLdrUnknownStruct42& cone = mData.mUnknown9EC51FE4;
    beam = new CSplitterBeamEffect(mBeamEffectId, info, name, moduleTransform, 0x40,
                                   PortWrapTurn(cone.angle * 0.017453292f), GetDetectionRange(),
                                   cone.cloudColor1, cone.cloudColor2, cone.addColor1,
                                   cone.addColor2, cone.cloudScale, cone.fadeOffSize,
                                   cone.openSpeed);
    mgr.AddObject(beam);
    // 0x13BC-0x13CC: the sound word at 0x890, its low half (clrlwi r4, r0, 16).
    mBeamSfxHandle = CSfxManager::AddEmitter(static_cast< ushort >(mData.mSoundScanning & 0xFFFF),
                                             GetTranslation(), 0x7F, GetCurrentAreaId().Value(),
                                             true, true, CSfxManager::kMedPriority);
  }
  // 0x13EC-0x142C: the beam opens (0x198 set, 0x13F8) and the sound follows the head, facing
  // along its Y column (0x13FC-0x142C UpdateEmitter, volume 0x7F).
  beam->SetOpening(true);
  CSfxManager::UpdateEmitter(mBeamSfxHandle, GetTranslation(), GetTransform().GetColumn(kDY),
                             0x7F);
}

// fn_75_1578 (the head's effect-state update, 0x1578-0x178C): the anim data (the model data at
// 0x60 and its 0x10 - 0x1594 lwz, 0x159C lwz) gets two effect states. The counter at 0xEDC
// (0x1590 lwz) picks the branch: 1 or 2 (0x1598 cmpwi r0, 2 - 0x15A0 beq, 0x15A4 cmpwi r0, 1)
// turns both off ("AlertEye" and "LaserMuzzle", 0x15AC-0x160C). Otherwise the laser-pulse
// projectile's id at 0xF02 (0x1614 lhz) decides: a valid id (the head is firing) turns
// "LaserMuzzle" on and "AlertEye" off (0x1628-0x1688). With no projectile the head's synced id
// at 0xEFE (0x168C lhz) decides: a valid id turns "AlertEye" on (0x16B4-0x1714); with no id
// either the IsScanning trigger (fn_75_58E8, 0x16A8) turns "AlertEye" on or both stay off
// (0x1718-0x1774). The two names are the rodata strings at 0x18 ("LaserMuzzle") and 0x1C (the
// "AlertEye" blob at 0x2C0).
void CSplitterCommandModule::PortUpdateEffectStates(CStateManager& mgr) {
  bool alertEye = false;
  bool laserMuzzle = false;
  if (mShieldState != 1 && mShieldState != 2) {
    if (mLookAtTargetId != kInvalidUniqueId) {
      laserMuzzle = true;
    } else if (mSyncedId != kInvalidUniqueId || PortChassisTimerRunning(mgr)) {
      alertEye = true;
    }
  }
  AnimationData()->SetEffectState(rstl::string_l("AlertEye"), alertEye, mgr);
  AnimationData()->SetEffectState(rstl::string_l("LaserMuzzle"), laserMuzzle, mgr);
}

// fn_75_5D50 (0x5D50-0x5DE0): the head's own vulnerability copy at 0xEAC takes the base's again
// (0x5D6C GetDamageVulnerability, 0x5D78 fn_75_5B9C, the REL copy of the CDamageVulnerability
// assignment); the port keeps no copy, the head's vulnerability slot (fn_75_616C) returns the
// base's, so there is nothing to copy. The head becomes orbitable (0x5D88
// SetVisorOrbitableFlags 0xF, true); with the shield state at 0xEDC at 2 (0x5D90 cmpwi) the timer
// at 0xF1C is zeroed (0x5DA0); the shield state and the timer at 0xF18 are zeroed (0x5DB4,
// 0x5DBC) and the effect states are updated (0x5DC0 fn_75_1578).
void CSplitterCommandModule::PortResetShield(CStateManager& mgr) {
  SetVisorOrbitableFlags(static_cast< CVisorParameters::EVisorOrbitableFlags >(0xF), true);
  if (mShieldState == 2) {
    mCountupTimerB = 0.f;
  }
  mShieldState = 0;
  mCountupTimerA = 0.f;
  PortUpdateEffectStates(mgr);
}

// fn_75_42F8 (the head's LostChassisReaction state, 0x42F8-0x4438). Activate (0x4330): the
// animation state is kAS_Ready (0x4334) and the flag byte 0xF6A's 0x20 bit is set (0x433C rlwimi).
// Update (0x4348): the head turns upright (fn_75_2994) and, when CTryCommand::ShouldRetry
// (the host's CAnimationState::CanIssueCommand) allows body state 6 (kAS_KnockBack, 0x4354-0x4360),
// a knock-back command along the head's Y column (the transform's 0x28, 0x38, 0x48) with
// severity 2 goes out (lbl_75_data_5B8, 0x4364-0x43C4). Deactivate (0x441C): the animation state is kAS_NotReady.
// @ fn_75_42F8
void CSplitterCommandModule::LostChassisReaction(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mAutoDestructFlags |= 0x20;
  } else if (msg == kStateMsg_Update) {
    PortTurnUpright(dt);
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_KnockBack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCKnockBackCmd(GetTransform().GetColumn(kDY), pas::kS_Two));
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// fn_75_2994 (the head's turn upright, 0x2994-0x2BE0): only with the flag byte 0xF6A's 0x20 bit
// (0x29BC-0x29C8). The turn is the frame time times 2 pi (lbl_75_rodata_7C). With the Z column
// (0x2C, 0x3C, 0x4C) within it of the up vector (0x2A38 GetAngleDiff, 0x2A3C-0x2A44) and the Y
// column's x or y non-zero (0x2A48-0x2A7C) the head looks along its flat Y column (0x2AC0
// LookAt, 0x2ACC SetTransform) and the 0x20 bit is cleared (0x2AD8). Otherwise the Z column is
// slerped toward the up vector by the turn (0x2B10) and the rotation arc from the Z column to it
// (0x2B38) is applied after the head's rotation (0x2B64 FromMatrix, 0x2B74 operator*,
// 0x2BA4 BuildTransform4f at the translation, 0x2BB0 SetTransform).
void CSplitterCommandModule::PortTurnUpright(float dt) {
  if ((mAutoDestructFlags & 0x20) == 0) {  // 0x29BC-0x29C8
    return;
  }
  const CVector3f pos = GetTranslation();
  const float turn = 6.2831855f * dt;
  const CVector3f up = CVector3f::Up();
  const CVector3f zColumn = GetTransform().GetColumn(kDZ);
  const CVector3f yColumn = GetTransform().GetColumn(kDY);
  if (CVector3f::GetAngleDiff(up, zColumn) <= turn &&
      (yColumn.GetX() != 0.f || yColumn.GetY() != 0.f)) {
    SetTransform(CTransform4f::LookAt(
        pos, CVector3f(pos.GetX() + yColumn.GetX(), pos.GetY() + yColumn.GetY(), pos.GetZ()), up));
    mAutoDestructFlags &= ~0x20;
    return;
  }
  const CVector3f toward =
      CVector3f::Slerp(zColumn.AsNormalized(), up.AsNormalized(), CRelAngle::FromRadians(turn));
  const CQuaternion arc = CQuaternion::ShortestRotationArc(zColumn, toward);
  SetTransform((GetRotation() * arc).BuildTransform4f(pos));
}

// fn_75_23F0 (the head's separation, 0x23F0-0x25C4): the chassis's fn_75_8EC8 with the head's
// radius - the listening-AI list at 0x830 walked from 0x2008 (0x2448) to -1; every CPatterned
// (0x245C) other than the head (0x2468) in its area (0x2470-0x247C) that is not a docked head
// (0x2480-0x249C) gives the steering separation (0x24DC) from its position with 5.0f
// (lbl_75_rodata_58) times the model data's scale x; safe ones (0x24FC) are normalized and summed
// and counted; a count above 0.0f (0x2564) divides the sum (0x2574).
CVector3f CSplitterCommandModule::PortSeparationVector(CStateManager& mgr) const {
  CObjectList& list = mgr.ObjectListById(kOL_ListeningAi);
  CVector3f sum = CVector3f::Zero();
  float count = 0.f;
  for (int idx = list.GetFirstObjectIndex(); idx != -1; idx = list.GetNextObjectIndex(idx)) {
    const CPatterned* other = TCastToConstPtr< CPatterned >(list[idx]);
    if (other == nullptr || other == this || other->GetCurrentAreaId() != GetCurrentAreaId()) {  // 0x2460, 0x2468, 0x2470-0x247C
      continue;
    }
    const CSplitterCommandModule* head = TCastToConstPtr< CSplitterCommandModule >(other);
    if (head != nullptr && head->GetChassisId() != kInvalidUniqueId) {  // 0x2484-0x249C
      continue;
    }
    const CVector3f separation = mSteeringBehaviors.Separation(
        *this, other->GetTranslation(), 5.f * GetModelData()->GetScale().GetX());
    if (separation.IsMagnitudeSafe()) {
      sum += separation.AsNormalized();
      count += 1.f;
    }
  }
  if (count > 0.f) {
    sum *= 1.f / count;
  }
  return sum;
}

// fn_75_2280 (the head's chassis search, 0x2280-0x23F0): the listening-AI list at 0x830 is walked
// (0x22D8 lha 0x2008, to -1); a main chassis (0x22EC) in the head's area (0x22F8-0x2304) that
// takes the head's reservation (0x2318 fn_75_E788) and is nearer than the best so far (squared,
// from FLT_MAX lbl_75_rodata_88; 0x2324-0x234C) releases the old pick's reservation (0x2350-0x2380
// ObjectById, cast, fn_75_E768) and becomes the pick at 0xEFC (0x238C).
void CSplitterCommandModule::PortFindChassis(CStateManager& mgr) {
  CObjectList& list = mgr.ObjectListById(kOL_ListeningAi);
  const CVector3f pos = GetTranslation();
  float best = 3.4028235e38f;
  for (int idx = list.GetFirstObjectIndex(); idx != -1; idx = list.GetNextObjectIndex(idx)) {
    CSplitterMainChassis* chassis = TCastToPtr< CSplitterMainChassis >(list[idx]);
    if (chassis == nullptr || chassis->GetCurrentAreaId() != GetCurrentAreaId()) {  // 0x22EC cast, 0x22F8-0x2304
      continue;
    }
    if (!chassis->PortReserveDock(GetUniqueId())) {
      continue;
    }
    const float distSq = (chassis->GetTranslation() - pos).MagSquared();
    if (distSq >= best) {
      continue;
    }
    if (CSplitterMainChassis* old =
            TCastToPtr< CSplitterMainChassis >(mgr.ObjectById(mCheckedChassisId))) {
      old->PortReleaseDock(GetUniqueId());
    }
    best = distSq;
    mCheckedChassisId = chassis->GetUniqueId();
  }
}

// fn_75_3ADC (the head's ResetAttackTimes code, 0x3ADC-0x3B20): the countdown at 0xF0C takes the
// data's minimum laser pulse attack time (0x7D0) and the shot count at 0xF20 a random 1 to the
// data's maximum laser pulse shots (0x7DC; 0x3B04 CRandom16::Range on the manager's 0x16E4).
// @ fn_75_3ADC
void CSplitterCommandModule::ResetAttackTimes(CStateManager& mgr, float) {
  mCountdownTimerB = mData.mMinLaserPulseAttackTime;
  mLaserPulseShots = mgr.Random()->Range(1, mData.mMaxLaserPulseShots);
}

// fn_75_3D20 (the head's SetTargetDest code, 0x3D20-0x3E60): the destination starts at the head's
// position and the flag byte 0xF6A's 0x80 bit is cleared (0x3D7C). With the target at 0xEFE
// (0x3D8C GetObjectById) the destination is its aim position (0x3DE4, vtable 0x54 with 0.0f)
// plus 4.0f (lbl_75_rodata_5C) up and the 0x80 bit is set (0x3E14). The path finder at 0x6F4
// takes the destination (0x3E24 SetDestination) and the target as its face target (0x3E2C, 0x718).
void CSplitterCommandModule::SetTargetDest(CStateManager& mgr, float) {
  CVector3f dest = GetTranslation();
  mAutoDestructFlags &= ~0x80;
  if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mSyncedId))) {
    // The id is the chassis's target (0xCC0), always an actor; retail does no cast.
    dest = target->GetAimPosition(mgr, 0.f) + CVector3f::Up() * 4.f;
    mAutoDestructFlags |= 0x80;
  }
  mPathFindNavigation.SetDestination(dest);
  mPathFindNavigation.SetFaceTarget(mSyncedId);
}

// fn_75_4E90 (the head's PathFind state, 0x4E90-0x50B0): with 0xF6A's 0x80 bit the path finder at
// 0x6F4 runs first (0x4EEC CPathFindNavigation::PathFind). Activate (0x4F14): the 0x40 bit is
// cleared. Update (0x4F28): the body's move vector (the body controller's command manager, its
// 0x4) plus the separation (0x4F50 fn_75_23F0) must be IsMagnitudeSafe (0x4F88); it is scaled by
// the frame time times the data's maximum linear velocity (0x7C4) and the head moves there
// (0x4FE4 fn_75_2154). The facing is that step, or with the path finder's face target (0x718,
// 0x5008 GetObjectById) the flat offset to it (z 0.0f; 0x5014-0x504C); the body faces it
// (0x505C FaceDirection). Deactivate (0x5064): the 0x80 bit is set.
void CSplitterCommandModule::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  if ((mAutoDestructFlags & 0x80) != 0) {  // 0x4EC8-0x4ED8
    mPathFindNavigation.PathFind(mgr, msg, dt, *this);
  }
  if (msg == kStateMsg_Activate) {
    mAutoDestructFlags &= ~0x40;
  } else if (msg == kStateMsg_Update) {
    const CVector3f move =
        BodyController()->CommandMgr().GetMoveVector() + PortSeparationVector(mgr);
    if (!move.IsMagnitudeSafe()) {
      return;
    }
    const CVector3f step = move * (dt * mData.mMaxLinearVelocity);
    const CVector3f pos = GetTranslation();
    PortMoveTo(pos + step, dt);
    CVector3f face = step;
    if (const CActor* target = static_cast< const CActor* >(
            mgr.GetObjectById(mPathFindNavigation.GetFaceTarget()))) {
      const CVector3f delta = target->GetTranslation() - GetTranslation();
      face = CVector3f(delta.GetX(), delta.GetY(), 0.f);
    }
    BodyController()->FaceDirection(face, dt);
  } else if (msg == kStateMsg_Deactivate) {
    mAutoDestructFlags |= 0x80;
  }
}

// fn_75_5624 (the head's ShouldAttack trigger, 0x5624-0x5644): the countdown at 0xF0C is at or
// below 0.0f (0x5630 fcmpo, 0x5634 cror lt|eq).
// @ fn_75_5624
bool CSplitterCommandModule::ShouldAttack(CStateManager&, const CTriggerData&) const {
  return mCountdownTimerB <= 0.f;
}

// fn_75_4864 (the head's LaserPulse state, 0x4864-0x4A5C). Activate (0x48B0): the animation
// state is kAS_Ready (0x48B8), the dodge count at 0xEEC is 0 (0x48BC) and the shot count at 0xF20
// drops by one (0x48C0-0x48C8). Update (0x48D0): when CTryCommand::ShouldRetry (the host's
// CAnimationState::CanIssueCommand) allows body state 0x12 (kAS_ProjectileAttack, 0x48D8-0x48E4)
// a projectile attack command (lbl_75_data_5AC, kind 6) with severity 1 (0x4914) at the position
// plus the Y column (0x48E8-0x4930) without horizon blending (0x4918) goes out (0x4958).
// Otherwise, with a positive frame time (0x49B0 fcmpo against 0.0f lbl_75_rodata_40) and the
// target at 0xEFE (0x49C8 GetObjectById, 0x49CC), the head faces the flat offset (z 0.0f, 0x4A2C)
// to the target's aim position (0x49F8, vtable 0x54 with 0.0f; 0x4A34 FaceDirection).
// Deactivate (0x4A3C): the animation state is kAS_NotReady.
void CSplitterCommandModule::LaserPulse(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mDodgeCount = 0;
    --mLaserPulseShots;
  } else if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_ProjectileAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCProjectileAttackCmd(
          pas::kS_One, GetTranslation() + GetTransform().GetColumn(kDY), false));
    } else if (dt > 0.f) {
      if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mSyncedId))) {
        // The id is the chassis's target (0xCC0), always an actor; retail does no cast.
        const CVector3f aim = target->GetAimPosition(mgr, 0.f);
        BodyController()->FaceDirection(
            CVector3f(aim.GetX() - GetTranslation().GetX(), aim.GetY() - GetTranslation().GetY(), 0.f),
            dt);
      }
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// fn_75_5448 (the head's Dead state, 0x5448-0x5498): Activate only (0x5450-0x5468): the laser
// pulse projectile despawns (0x5470 fn_75_1790) and the head is deleted (0x547C
// CPatterned::DeathDelete).
void CSplitterCommandModule::Dead(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    PortDespawnProjectile(mgr);
    DeathDelete(mgr);
  }
}

// fn_75_4AC0 (the head's Dodge state, 0x4AC0-0x4CA8). Activate (0x4B0C): the animation state is
// kAS_Ready (0x4B10) and the flag byte 0xF6A gets its 0x10 bit (0x4B14-0x4B1C rlwimi). Update
// (0x4B24): when CTryCommand::ShouldRetry (CAnimationState::CanIssueCommand) allows body state 3
// (kAS_Step, 0x4B2C-0x4B38) a step command (lbl_75_data_5A0, kind 1) in the dodge direction at
// 0xF30 (0x4B4C) of type 1 (kStep_Dodge, 0x4B54) goes out (0x4B9C). Otherwise, with a positive
// frame time (0x4BFC fcmpo against 0.0f) and the target at 0xEFE (0x4C14 GetObjectById, 0x4C18),
// the head faces the flat offset (z 0.0f, 0x4C64) to the target's position (0x4C20-0x4C54;
// 0x4C6C FaceDirection). Deactivate (0x4C74): kAS_NotReady and the 0x10 bit cleared.
void CSplitterCommandModule::Dodge(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mAutoDestructFlags |= 0x10;
  } else if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Step)) {
      BodyController()->CommandMgr().DeliverCmd(CBCStepCmd(
          static_cast< pas::EStepDirection >(mDodgeDirection), pas::kStep_Dodge));
    } else if (dt > 0.f) {
      if (const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mSyncedId))) {
        // The id is the chassis's target (0xCC0), always an actor; retail does no cast.
        const CVector3f delta = target->GetTranslation() - GetTranslation();
        BodyController()->FaceDirection(CVector3f(delta.GetX(), delta.GetY(), 0.f), dt);
      }
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mAutoDestructFlags &= ~0x10;
  }
}

// fn_75_5610 (the head's ShouldFireAgain trigger, 0x5610-0x5624): the shot count at 0xF20 is
// above 0 (0x5614-0x561C neg/andc/srwi).
// @ fn_75_5610
bool CSplitterCommandModule::ShouldFireAgain(CStateManager&, const CTriggerData&) const {
  return mLaserPulseShots > 0;
}

// fn_75_5644 (the head's ShouldDodge trigger, 0x5644-0x565C): the dodge direction at 0xF30 is not
// -1 (0x5648-0x5654 subfic/addi/or/srwi).
// @ fn_75_5644
bool CSplitterCommandModule::ShouldDodge(CStateManager&, const CTriggerData&) const {
  return mDodgeDirection != -1;
}

// fn_75_3B34 (the head's FindBestDodgeDirection code, 0x3B34-0x3BEC): the dodge direction at 0xF30
// is -1 (0x3B54). With the data word 0x7C0's 0x2 bit (0x3B5C) and the dodge count at 0xEEC below
// the data's maximum dodges (0x7F8, 0x3B6C): below the minimum dodges (0x7F4, 0x3B78) or a roll of
// 0.0f to 100.0f (lbl_75_rodata_40/64, 0x3B98) at or below the dodge chance (0x7FC, 0x3BA0-0x3BA8)
// picks a direction (0x3BB4 fn_75_270C) and counts the dodge (0x3BC0); otherwise the count goes
// to the maximum (0x3BCC-0x3BD0).
void CSplitterCommandModule::FindBestDodgeDirection(CStateManager& mgr, float) {
  mDodgeDirection = -1;
  if ((mData.mUnknownBD80FD94 & 0x2) == 0 || mDodgeCount >= mData.mMaxDodges) {  // 0x3B5C, 0x3B6C
    return;
  }
  if (mDodgeCount < mData.mMinDodges || mgr.Random()->Range(0.f, 100.f) <= mData.mDodgeChance) {
    mDodgeDirection = PortDodgeDirection(mgr);
    ++mDodgeCount;
  } else {
    mDodgeCount = mData.mMaxDodges;
  }
}

// fn_75_270C (the dodge direction pick, 0x270C-0x2994): the right (the X column, 0x24/0x34/0x44)
// and left sides start open. The listening-AI list at 0x830 is walked (0x2780 lha 0x2008, to -1);
// every entry (0x2794) other than the head (0x279C) in its area (0x27A4-0x27B0) nearer than the
// dodge distance at 0xEE8 (squared, 0x27EC) closes the side it lies on: a dot with the X column
// at or above 0.0f (0x2814-0x281C) closes the right within 60 degrees of it (lbl_75_rodata_8C,
// 0x2830-0x2840), below it the left within 60 degrees of -X (0x2874-0x2884). An open side needs a
// clear dodge path (0x28C8 / 0x290C fn_75_25C4). With both open a random bit 0x4000 (0x2928
// CRandom16::Next, 0x292C) keeps the right, else the left. The result is 2 (left, 0x2948), 3
// (right, 0x2958) or -1 (0x2768).
int CSplitterCommandModule::PortDodgeDirection(CStateManager& mgr) {
  const float dist = mDodgeDistance;
  const CVector3f pos = GetTranslation();
  const CVector3f right = GetTransform().GetColumn(kDX);
  bool leftOpen = true;
  bool rightOpen = true;
  CObjectList& list = mgr.ObjectListById(kOL_ListeningAi);
  for (int idx = list.GetFirstObjectIndex(); idx != -1; idx = list.GetNextObjectIndex(idx)) {
    // The list holds AIs, always actors; retail reads the position with no cast (0x27B4).
    const CActor* other = static_cast< const CActor* >(list[idx]);
    // 0x2794 (null), 0x279C (the head itself), 0x27A4-0x27B0 (the area).
    if (other == nullptr || other == this || other->GetCurrentAreaId() != GetCurrentAreaId()) {
      continue;
    }
    const CVector3f delta = other->GetTranslation() - pos;
    if (delta.MagSquared() >= dist * dist) {
      continue;
    }
    if (CVector3f::Dot(delta, right) >= 0.f) {
      if (rightOpen && CVector3f::GetAngleDiff(right, delta) < 1.0471976f) {
        rightOpen = false;
      }
    } else if (leftOpen && CVector3f::GetAngleDiff(-right, delta) < 1.0471976f) {
      leftOpen = false;
    }
  }
  if (rightOpen) {
    rightOpen = PortDodgeClear(mgr, right, mDodgeDistance);
  }
  if (leftOpen) {
    leftOpen = PortDodgeClear(mgr, -right, mDodgeDistance);
  }
  if (leftOpen && rightOpen) {
    if ((mgr.Random()->Next() & 0x4000) != 0) {  // 0x2928-0x292C
      leftOpen = false;
    } else {
      rightOpen = false;
    }
  }
  if (leftOpen) {
    return pas::kSD_Left;
  }
  if (rightOpen) {
    return pas::kSD_Right;
  }
  return pas::kSD_Invalid;
}

// fn_75_25C4 (a clear dodge path, 0x25C4-0x270C): from the bounding box's center (0x2600-0x260C)
// to the center plus the direction times the distance (0x2610-0x2664), a ray with the solid
// material (lbl_75_data_440, 0x3B) included and the collision actor one (lbl_75_data_444, 0x2F)
// excluded (type 3) hits no world (0x26B8 RayCollideWorld, 0x26C0) and the end lies on the path
// search at 0xD6C (0x26CC OnPath, 0x26D0 against kR_Success).
bool CSplitterCommandModule::PortDodgeClear(CStateManager& mgr, const CVector3f& dir, float dist) {
  const CVector3f center = GetBoundingBox().GetCenterPoint();
  const CVector3f end = center + dir * dist;
  const CMaterialFilter filter = CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid), CMaterialList(kMT_CollisionActor));
  return mgr.RayCollideWorld(center, end, filter, this) &&
         mPathFindSearch.OnPath(end) == CPathFindSearch::kR_Success;
}

// fn_75_565C (the head's InLaserPulseRange trigger, 0x565C-0x570C): with the target at 0xEFE
// (0x5680) its squared distance (0x568C-0x56CC) lies between the data's minimum (0x7D4) and
// maximum (0x7D8) laser pulse ranges squared (0x56D0-0x56E4).
bool CSplitterCommandModule::InLaserPulseRange(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mSyncedId));
  if (target == nullptr) {  // 0x5684-0x5688
    return false;
  }
  const float distSq = (target->GetTranslation() - GetTranslation()).MagSquared();
  return distSq >= mData.mMinLaserPulseRange * mData.mMinLaserPulseRange &&
         distSq <= mData.mMaxLaserPulseRange * mData.mMaxLaserPulseRange;
}

// fn_75_570C (the head's InHoverRange trigger, 0x570C-0x57B4): with the target at 0xEFE (0x5730)
// its squared distance is within half (lbl_75_rodata_44) the sum of the two laser pulse ranges,
// squared (0x573C-0x5794).
bool CSplitterCommandModule::InHoverRange(CStateManager& mgr, const CTriggerData&) const {
  const CActor* target = static_cast< const CActor* >(mgr.GetObjectById(mSyncedId));
  if (target == nullptr) {  // 0x5734
    return false;
  }
  const float distSq = (target->GetTranslation() - GetTranslation()).MagSquared();
  const float range = 0.5f * (mData.mMinLaserPulseRange + mData.mMaxLaserPulseRange);
  return distSq <= range * range;
}

// fn_75_6490 (0xBC) - the command module's Think. The body follows the retail's: the
// active gate, the base's Think, the collision actor manager's update and the module's
// own six calls in order (their sizes are in the table in
// docs/module-analysis/Splitter.md). Five of the six are over 0x80 and each is a once-logging
// shell here; fn_75_1188 (the beam-effect spawner) is ported, so this function carries no port
// marker yet.
// vtable slot 0x14 (0xBC). The active gate (the byte at 0x20, 0x64B4 lbz and 0x64B8 extrwi bit 24 -
// the byte's 0x80), CPatterned::Think (0x64C0), the collision actor manager's update (the manager at
// 0xE58, 0x64C8 lwz; the update options 0 at 0x64D0, which is kUO_ObjectSpace; 0x64D4 the update)
// and then the module's own six per-frame calls in order: fn_75_31A8 (0x64E0, 0x14C), fn_75_32F4
// (0x64F0, 0x16C), fn_75_3004 (0x64FC, 0x1A4), fn_75_184C (0x650C, 0x128), fn_75_1188 (0x651C, 0x344)
// and fn_75_21FC (0x652C, 0x84). Five of them are over 0x80 and belong to the line-of-sight
// tracker and the derived state, so they stay named once-logging shells that return neutral
// values; fn_75_1188 is ported (PortSpawnBeamEffect).
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
  // The retail's state machine calls fn_75_3EEC (the docking) as its NotifyDocking code, so
  // the port runs it from there and not from the Think.
  PortUpdateChassis(mgr);
  PortUpdateFlashAndDeath(mgr, dt);
  PortUpdateCollision(mgr);
  PortLookAtTarget(mgr, dt);
  PortSpawnBeamEffect(mgr);
  PortAutoDestructTimer(dt);
}

// @ fn_75_1790
// The head's laser-pulse projectile despawn (0x1790-0x184C): the projectile's id at 0xF02 is
// looked up (0x17BC ObjectById) and, when it is there, its virtual at the vtable word 0x24 is
// called with the manager and false (0x17C8-0x17DC), the sound handle at 0xF5C is removed
// (0x17EC RemoveEmitter) and the object is deleted (0x1800 DeleteObjectRequest). Then the id
// at 0xF02 is cleared (0x1814), the handle at 0xF5C is zeroed (0x1818) and, when the base's
// flag byte at 0x420 has its 0x20 bit (the word's bit 25, 0x1820 extrwi) set, the module's own
// call runs (0x1830 fn_75_1578).
void CSplitterCommandModule::PortDespawnProjectile(CStateManager& mgr) {
  CEntity* projectile = mgr.ObjectById(mLookAtTargetId);
  if (projectile != nullptr) {
    // The vtable word 0x90 with false (0x17C8-0x17DC): the DOL's CPlasmaProjectile::ResetBeam, a
    // host virtual. The id only ever holds the laser sweep beam (fn_75_1974).
    if (CBeamProjectile* beam = TCastToPtr< CBeamProjectile >(projectile)) {
      beam->ResetBeam(mgr, false);
    }
    CSfxManager::RemoveEmitter(mLookAtSfxHandle);
    mgr.DeleteObjectRequest(mLookAtTargetId);
  }
  mLookAtTargetId = kInvalidUniqueId;
  mLookAtSfxHandle.Clear();
  if (GetFlag26()) {
    PortUpdateEffectStates(mgr);
  }
}

// @ fn_75_3460
// The head's collision actor manager (0x3460-0x3674): the head's one joint sphere - the
// "Skeleton_Root" locator (rodata 0x0) with the radius 3.0f (rodata 0x4) and the mass 1000.0f
// (rodata 0x94) - is built (0x34C0-0x3514 SphereCollision) and the manager is made with it
// (0x35A4), the old one deleted (0x35B4) and the new one kept at 0xE58 (0x35B8). Then every
// collision actor of the manager is walked (0x35C4-0x3644): the description's id (its 0x3C) is
// looked up (0x35E8 ObjectById) and cast to a CCollisionActor (0x35EC); when the description's
// name is "Beam_LCTR" (0x3614, the retail's string compare) the id goes to 0xF00 (0x3630).
// Finally the module's own call runs (0x3654 fn_75_2CE0, the shield, ported).
void CSplitterCommandModule::PortCreateCollisionActorManager(CStateManager& mgr) {
  rstl::vector< CJointCollisionDescription > joints;
  joints.reserve(1);
  const CAnimData* anim = GetModelData()->GetAnimationData();
  const CSegId seg = anim->GetLocatorSegId(rstl::string_l("Skeleton_Root"));
  joints.push_back_unsafe(CJointCollisionDescription::SphereCollision(
      seg, CVector3f::Zero(), 3.f, rstl::string_l("Skeleton_Root"), 1000.f));
  CCollisionActorManager* manager =
      rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), joints, false);
  delete mCollisionActorManager;
  mCollisionActorManager = manager;
  for (uint i = 0; i < mCollisionActorManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc = mCollisionActorManager->GetCollisionDescFromIndex(i);
    const TUniqueId id = desc.GetCollisionActorId();
    if (TCastToPtr< CCollisionActor >(mgr.ObjectById(id)) == nullptr) {
      continue;
    }
    if (strcmp(desc.GetName().c_str(), "Beam_LCTR") == 0) {
      mSoundTargetId = id;
    }
  }
  // 0x3648-0x3654: the retail calls fn_75_2CE0 with the flag clear (no shield goes up).
  PortUpdateShield(mgr, false);
}

// @ fn_75_3A8C
// The command module's connected-object lookup (0x3A8C-0x3ADC): the object connected with the
// state "CONN" (0x3AA4) and the message "ATCH" (0x3AA8) is looked up (0x3ABC FindConnectedObject)
// and its id is kept at 0xEFC (0x3AC4 sth).
void CSplitterCommandModule::PortUpdateConnectedObject(CStateManager& mgr) {
  mCheckedChassisId = FindConnectedObject(mgr, kSS_Connect, kSM_Attach);
}

// @ fn_75_2CC0
// The command module's flash start (0x2CC0-0x2CE0): when the message's sender id (the u16 at
// the message's 0x0, 0x2CC0 lhz) is the id the module keeps at 0xF00 (0x2CC4 lhz), the flash
// weight at 0xF08 is set to 1.0f (0x2CD8 stfs, lbl_75_rodata_34).
void CSplitterCommandModule::PortStartFlashIfTarget(const CScriptMsg& msg) {
  if (msg.GetSenderId() == mSoundTargetId) {
    mFlashAlpha = 1.f;
  }
}

// vtable slot 0x18 = fn_75_654C (0x24C): it calls the base class's AcceptScriptMsg and then
// switches on the script message's four-character code.
// @ fn_75_654C
// The command module's script-message handler (0x654C-0x6798): the base's call (0x6578) and a
// switch on the message's four-character code. The retail's cases: XCRT (the body controller's
// activation, the module's own call and the random roll for the state at 0xEE4), XDEL (the
// collision actor manager's destruction, the beam effect's deletion and the module's own call),
// ACTV and DCVT (the collision actor manager's activation), AUDT (its physics), ALRT (the
// base's flag byte), XAND (the connected-object lookup) and XCMG (the flash start).
void CSplitterCommandModule::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  CPatterned::AcceptScriptMsg(mgr, msg);
  switch (msg.GetMessage()) {
  case kSM_Create: { // "XCRT" (0x65F8)
    // 0x6614-0x666C: the body controller's activation with the animation state -1 (0x6620), the
    // module's own call (0x662C fn_75_3460) and the random roll (0x6644 Range over the
    // manager's random, 0.0f to 100.0f): the state at 0xEE4 is 2 when the roll is 50.0f or more
    // and 1 otherwise (0x6668 stw).
    BodyController()->Activate(mgr, pas::kAS_Invalid);
    PortCreateCollisionActorManager(mgr);
    mPreviousCollisionState = mgr.Random()->Range(0.f, 100.f) >= 50.f ? 2 : 1;
    break;
  }
  case kSM_Delete: { // "XDEL" (0x65DC)
    // 0x6670-0x66B4: the collision actor manager is destroyed (0x6680 Destroy), the beam effect
    // at 0xF68 is deleted (0x66A8 DeleteObjectRequest) and the module's own call runs (0x66B4
    // fn_75_1790).
    if (mCollisionActorManager != nullptr) {
      mCollisionActorManager->Destroy(mgr);
    }
    if (mBeamEffectId != kInvalidUniqueId) {
      mgr.DeleteObjectRequest(mBeamEffectId);
    }
    PortDespawnProjectile(mgr);
    break;
  }
  case kSM_Activate: { // "ACTV" (0x65B8)
    // 0x66BC-0x66D0: the collision actor manager is activated (0x66D0 SetActive with true).
    if (mCollisionActorManager != nullptr) {
      mCollisionActorManager->SetActive(mgr, true);
    }
    break;
  }
  case kSM_Deactivate: { // "DCVT" (0x6590)
    // 0x66D8-0x66EC: the collision actor manager is deactivated (0x66EC SetActive with false).
    if (mCollisionActorManager != nullptr) {
      mCollisionActorManager->SetActive(mgr, false);
    }
    break;
  }
  case kSM_AIUpdateDisabled: { // "AUDT" (0x657C)
    // 0x66F4-0x6708: the collision actor manager's physics is switched off (0x6708
    // SetPhysicsActive with false).
    if (mCollisionActorManager != nullptr) {
      mCollisionActorManager->SetPhysicsActive(mgr, false);
    }
    break;
  }
  case kSM_Alert: { // "ALRT" (0x65A4)
    // 0x6710-0x671C: the base's flag byte at 0x420 gets its 0x80 bit (the word's bit 24) set.
    SetHitByPlayerProjectile(true);
    break;
  }
  case kSM_AreaLoaded: { // "XAND" (0x65CC)
    // 0x6724-0x6754: the path area of the head's current area (the world at the manager's
    // 0x1604, its area list at 0x24 indexed by the area id at 0x4, the area's post-constructed
    // data at 0x104 and its path area at 0xEC) goes into the path search at 0xD6C; then the
    // module's own call runs (0x6758 fn_75_3A8C, ported).
    mPathFindSearch.SetArea(
        mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea);
    PortUpdateConnectedObject(mgr);
    break;
  }
  case kSM_Damage: { // "XCMG" (0x6604)
    // 0x6760-0x6774: the module's own call (0x6774 fn_75_2CC0, ported) with the message's
    // sender id.
    PortStartFlashIfTarget(msg);
    break;
  }
  default:
    break;
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
// @ fn_75_5E8C
// The head's state machine (0x5E8C-0x5F18): the base's setup (0x5EA0) and then the module's own
// tables - the 17 triggers (lbl_75_data_CC, 0x5EB4), the 14 states (lbl_75_data_284, 0x5ED4) and
// the 7 codes (lbl_75_data_3B8, 0x5EF4).
void CSplitterCommandModule::SetupStateMachine(CStateManager& mgr) {
  CPatterned::SetupStateMachine(mgr);
  mStateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  mStateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  mStateMachine->SetCodeFunctions(skCodeFuncs, ARRAY_SIZE(skCodeFuncs));
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
