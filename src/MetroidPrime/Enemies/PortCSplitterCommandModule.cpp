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
#include "MetroidPrime/Collision/CJointCollisionDescription.hpp"

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

// The head's FSM names: the REL's lbl_75_data_CC (the 17 triggers), lbl_75_data_284 (the
// 14 states) and lbl_75_data_3B8 (the 7 codes). The retail's tables hold the names only -
// the transitions live in the area script - so the function pointers are null.
CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    {"StateOver", nullptr},
    {"InDetectionRange", nullptr},
    {"IsScanning", nullptr},
    {"IsInitiallyDocked", nullptr},
    {"IsDocked", nullptr},
    {"HasDockingPath", nullptr},
    {"HasDockingTarget", nullptr},
    {"DockingPathOver", nullptr},
    {"HasTarget", nullptr},
    {"InHoverRange", nullptr},
    {"InLaserPulseRange", nullptr},
    {"ShouldDodge", nullptr},
    {"ShouldAttack", nullptr},
    {"ShouldFireAgain", nullptr},
    {"ShouldLaserSweep", nullptr},
    {"PathOver", nullptr},
    {"PathShagged", nullptr},
};

CPatterned::StateMachine::SStateFunction skStates[] = {
    {"Start", nullptr},
    {"Idle", nullptr},
    {"SpawnIdle", nullptr},
    {"Dead", nullptr},
    {"Scanning", nullptr},
    {"FaceTarget", nullptr},
    {"PathFind", nullptr},
    {"Hover", nullptr},
    {"Dodge", nullptr},
    {"LaserPulse", nullptr},
    {"LaserSweep", nullptr},
    {"LostChassisReaction", nullptr},
    {"SeekMainChassis", nullptr},
    {"FollowDockingPath", nullptr},
};

CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    {"NotifyDocking", nullptr},
    {"SelectTarget", nullptr},
    {"SetTargetDest", nullptr},
    {"SetDockingDest", nullptr},
    {"FindBestDodgeDirection", nullptr},
    {"RaiseShields", nullptr},
    {"ResetAttackTimes", nullptr},
};

// The module's own per-frame calls, as named once-logging stand-ins: the name is the retail
// function and its size. They are the line-of-sight tracker (fn_75_31A8) and the derived state
// (the other five) that the state machine reads, and all six are over the 0x80 a slice takes.
void StandInFn_75_10D0() { StandIn("fn_75_10D0 (0xB8, the module's own transform)"); }

// The module's own calls in the command module's script-message handler (fn_75_654C). Both are
// over 0x80, so each stays a named once-logging shell.

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

// The module's own calls in the collision update: both are over 0x80, so each stays a named
// once-logging shell. fn_75_2BE0 returns the neutral value (no object) and fn_75_2CE0 does
// nothing.
CEntity* PortFn_75_2BE0() {
  StandIn("fn_75_2BE0 (0xE0)");
  return nullptr;
}
void PortFn_75_2CE0(CStateManager& mgr, int mode, float value) {
  StandIn("fn_75_2CE0 (0x324)");
}
// The vtable word 0x4C of the object fn_75_2BE0 returns, and the CCollisionActor's virtual at
// the vtable word 0x38 (its 0x4 field is what the body reads). The port has no names for either
// DOL virtual yet, so both stay shells; the second one answers the neutral 0.0f.
void StandInVTable0x4C() { StandIn("the vtable word 0x4C of fn_75_2BE0's result"); }
float StandInCollisionActorVirtual0x38() {
  StandIn("the CCollisionActor virtual at the vtable word 0x38");
  return 0.f;
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
  CEntity* sound = PortFn_75_2BE0();
  if (sound != nullptr) {
    StandInVTable0x4C();
  }
  StandInSfxUpdateEmitter();
  const CEntity* actor = mgr.ObjectById(mSoundTargetId);
  if (actor == nullptr) {
    return;
  }
  if (TCastToConstPtr< CCollisionActor >(actor) == nullptr) {
    return;
  }
  if (mCollisionActorManager != nullptr) {
    mCollisionActorManager->SetActive(mgr, mChassisId == kInvalidUniqueId);
  }
  if (mCollisionState > 0 && mCollisionState < 3) {
    if ((mAutoDestructFlags & 0x8) != 0) {
      if (StandInCollisionActorVirtual0x38() <= 0.f) {
        mCollisionState = 0;
        mCountdownTimerA = mData.mResetShieldTime;
        PortFn_75_2CE0(mgr, 0, mData.mResetShieldTime);
      }
    }
  } else if ((mAutoDestructFlags & 0x8) != 0 && mCountdownTimerA <= 0.f) {
    mCollisionState = mPreviousCollisionState == 1 ? 2 : 1;
    mPreviousCollisionState = mCollisionState;
    PortFn_75_2CE0(mgr, 1, static_cast< float >(mCollisionState));
  }
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
bool CSplitterCommandModule::PortChassisTimerRunning(CStateManager& mgr) {
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
  // beam effect is there, set on it. The transform's own body is a once-logging shell.
  StandInFn_75_10D0();
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
// Finally the module's own call runs (0x3654 fn_75_2CE0, over 0x80).
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
  PortFn_75_2CE0(mgr, 0, 0.f);
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
