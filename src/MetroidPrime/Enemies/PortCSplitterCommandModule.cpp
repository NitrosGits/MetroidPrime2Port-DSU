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

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSplitterCommandModule.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

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
// HasDockingTarget (fn_75_57F0) and the HasTarget (fn_75_57B4) triggers, the Start state
// (fn_75_5564), the NotifyDocking code (fn_75_3EEC) and the RaiseShields code (fn_75_3B20);
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
    SPLITTER_TRIGGER(InDetectionRange),
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
    SPLITTER_STATE(Idle),
    SPLITTER_STATE(SpawnIdle),
    SPLITTER_STATE(Dead),
    SPLITTER_STATE(Scanning),
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

// The head's own move (fn_75_2154, 0xA8) - the Hover state asks it to move the head to a
// position; the port logs the position once and does not move.
void StandInFn_75_2154(const CVector3f& dest, float dt) {
  static bool sLogged = false;
  if (sLogged) {
    return;
  }
  sLogged = true;
  fprintf(stderr,
          "[splitter] fn_75_2154 (0xA8) would move the head to (%.3f, %.3f, %.3f) in %.3f s\n",
          dest.GetX(), dest.GetY(), dest.GetZ(), dt);
}

// The DOL's CSfxManager::UpdateEmitter (the sound the look-at plays).
void StandInSfxUpdateEmitter() { StandIn("the DOL's CSfxManager::UpdateEmitter"); }

// The DOL's CSfxManager::RemoveEmitter (the beam effect's sound).
void StandInSfxRemoveEmitter() { StandIn("the DOL's CSfxManager::RemoveEmitter"); }

// The retail's beam effect actor (0x1A0 bytes, a CActor) has no host class yet, so its creation
// (fn_75_11884) and the manager's add stay a once-logging shell. The values the retail would
// build are passed in, so the log says what was skipped.
void StandInFn_75_11884(const CEntityInfo& info, const rstl::string& name, float angle) {
  static bool sLogged = false;
  if (sLogged) {
    return;
  }
  sLogged = true;
  fprintf(stderr,
          "[splitter] fn_75_11884 (0x208) would create the beam effect actor '%s' in area %d "
          "with a %.4f rad rotation; the host has no class for it\n",
          name.c_str(), info.GetAreaId().Value(), angle);
}

} // namespace

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
SPLITTER_TRIGGER_STANDIN(StateOver, fn_75_5A40)
SPLITTER_TRIGGER_STANDIN(InDetectionRange, fn_75_5944)
SPLITTER_TRIGGER_STANDIN(HasDockingPath, fn_75_5864)
SPLITTER_TRIGGER_STANDIN(DockingPathOver, fn_75_57D4)
SPLITTER_TRIGGER_STANDIN(InHoverRange, fn_75_570C)
SPLITTER_TRIGGER_STANDIN(InLaserPulseRange, fn_75_565C)
SPLITTER_TRIGGER_STANDIN(ShouldDodge, fn_75_5644)
SPLITTER_TRIGGER_STANDIN(ShouldAttack, fn_75_5624)
SPLITTER_TRIGGER_STANDIN(ShouldFireAgain, fn_75_5610)
SPLITTER_TRIGGER_STANDIN(ShouldLaserSweep, fn_75_55F8)
SPLITTER_TRIGGER_STANDIN(PathOver, fn_75_55B0)
SPLITTER_TRIGGER_STANDIN(PathShagged, fn_75_5568)
SPLITTER_STATE_STANDIN(Idle, fn_75_5538)
SPLITTER_STATE_STANDIN(SpawnIdle, fn_75_5498)
SPLITTER_STATE_STANDIN(Dead, fn_75_5448)
SPLITTER_STATE_STANDIN(Scanning, fn_75_523C)
SPLITTER_STATE_STANDIN(FaceTarget, fn_75_50B0)
SPLITTER_STATE_STANDIN(PathFind, fn_75_4E90)
SPLITTER_STATE_STANDIN(Dodge, fn_75_4AC0)
SPLITTER_STATE_STANDIN(LaserPulse, fn_75_4864)
SPLITTER_STATE_STANDIN(LaserSweep, fn_75_44DC)
SPLITTER_STATE_STANDIN(LostChassisReaction, fn_75_42F8)
SPLITTER_STATE_STANDIN(SeekMainChassis, fn_75_4138)
SPLITTER_STATE_STANDIN(FollowDockingPath, fn_75_3FE0)
SPLITTER_CODE_STANDIN(SetTargetDest, fn_75_3D20)
SPLITTER_CODE_STANDIN(SetDockingDest, fn_75_3BEC)
SPLITTER_CODE_STANDIN(FindBestDodgeDirection, fn_75_3B34)
SPLITTER_CODE_STANDIN(ResetAttackTimes, fn_75_3ADC)
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
  StandInFn_75_2154(CVector3f(pos.GetX(), pos.GetY(), pos.GetZ() + (dz > 0.f ? step : -step)), dt);
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

// The head's own effect-state update (fn_75_1578, 0x218); its body is below, with the module's
// own calls.
void StandInFn_75_1578();

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
  StandInFn_75_1578();
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

// The DOL's CEntity virtual the projectile's despawn calls (the vtable word 0x24, 0x17C8-0x17DC
// of fn_75_1790): the port has no name for it, so the call stays a once-logging shell.
void StandInCEntityVirtual0x24() { StandIn("the DOL's CEntity virtual at the vtable word 0x24"); }

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
  // 0x11A4-0x11B4: the beam effect's id (0xF68) is looked up.
  CEntity* beam = mgr.ObjectById(mBeamEffectId);
  // 0x11B8-0x11E0: the module's own transform (fn_75_10D0) is built and copied and, when the
  // beam effect is there, set on it. The retail casts the object to a CActor without a check;
  // the port's beam effect is never created (fn_75_11884 is a shell), so the cast is the only
  // way to keep the call safe here.
  CTransform4f moduleTransform = GetTransform();
  PortModuleTransform(moduleTransform);
  if (CActor* beamActor = TCastToPtr< CActor >(beam)) {
    beamActor->SetTransform(moduleTransform);
  }
  // 0x1200-0x1208: the gate (fn_75_58E8) decides whether the body runs.
  if (!PortChassisTimerRunning(mgr)) {
    if (beam == nullptr) {
      // 0x1494-0x14B4: with no beam effect there, the retail clears the id at 0xF68 (0x14A0),
      // removes the sound emitter (0x14AC) and clears its handle at 0xF64 (0x14B4).
      mBeamEffectId = kInvalidUniqueId;
      StandInSfxRemoveEmitter();
    }
    // 0x143C-0x1490: with a beam effect there, the retail clears the beam's byte at 0x198
    // (0x1440) and, when the beam's own float at 0x188 is zero, asks the manager to delete the
    // object at 0xF68 (0x1480 DeleteObjectRequest). The port has no class for the beam effect
    // actor, so it cannot read that float and the delete stays out.
    return;
  }
  if (beam != nullptr) {
    // 0x13EC-0x142C: the beam effect is already there: its byte at 0x198 is set (0x13F8) and
    // the sound emitter is updated (0x142C).
    StandInSfxUpdateEmitter();
    return;
  }
  // 0x1214-0x1224: the base's field at 0x3D0 (its copy of the patterned info's detection range)
  // must be above zero.
  if (GetDetectionRange() <= 0.f) {
    return;
  }
  // port limit: the retail allocates a fresh id here (0x1230) and keeps it at 0xF68 (0x1244)
  // before it creates the actor. The host has no class for that actor, so the port allocates no
  // id: an id with no object behind it would be allocated again every frame and would eat the
  // manager's object ids.
  // 0x1248-0x13B4: the beam effect actor (0x1A0 bytes) is created with the CEntityInfo the
  // retail builds at 0x12A0 (the area id, NullConnectionList, true and kInvalidEditorId), the
  // name (0x12B8, the rodata string at 0xA0 + 0x230) and the rotation (0x12C4-0x1334: the
  // data's angle at 0x864 times one degree in radians), and it is added to the manager. The host
  // has no class for that actor, so the creation and the add stay a once-logging shell.
  const CEntityInfo info(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId);
  const rstl::string name = rstl::string_l("Splitter Beam Effect");
  const float angle = mData.mUnknown9EC51FE4.angle * 0.017453292f;
  StandInFn_75_11884(info, name, angle);
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
    StandInCEntityVirtual0x24();
    StandInSfxRemoveEmitter();
    mgr.DeleteObjectRequest(mLookAtTargetId);
  }
  mLookAtTargetId = kInvalidUniqueId;
  mLookAtSfxHandle = 0;
  if (GetFlag26()) {
    StandInFn_75_1578();
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
    // 0x6724-0x6758: the value the retail reads out of the manager's area list is written to
    // 0xD6C (the port has no name for that list, so the write stays out) and the module's own
    // call runs (0x6758 fn_75_3A8C, ported).
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
