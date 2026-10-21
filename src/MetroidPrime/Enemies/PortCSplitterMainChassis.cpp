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

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateInfo.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "MetroidPrime/CObjectList.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "MetroidPrime/Weapons/CWeapon.hpp"
#include "MetroidPrime/CKnockBackInfo.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Enemies/CSplitterCommandModule.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Tweaks/CTweakBall.hpp"
#include "MetroidPrime/ScriptObjects/CScriptPlayerHint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSplitterMainChassis.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/TCastTo.hpp"

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
  fprintf(stderr, "[splitter] %s is a port stand-in (see PortCSplitterMainChassis.cpp)\n", what);
}

// The chassis's FSM names: the REL's lbl_75_data_720 (the 28 triggers),
// lbl_75_data_9E8 (the 22 states) and lbl_75_data_BCC (the 11 codes). The tables'
// entries are 16 bytes {const char* name, 0, 0xFFFFFFFF, fn} and the module's static
// initialiser (fn_75_FED0, in the REL's .ctors) fills the function word from the
// three-word records of .data 0x5D0..0x714, so the retail function of each name is
// known (docs/module-analysis/Splitter.md section b). The ported ones are the Start state
// (fn_75_DFE8), the Scanning state (fn_75_DCF8) and the IsDisabled (fn_75_E620),
// DeployFromHoldingTube (fn_75_E034), ShotAt (fn_75_E5B8), PatrolPathOver (fn_75_E478),
// LostHead (fn_75_E5AC), ShouldWaitForSnatch (fn_75_E5D0) and PrepareForDocking (fn_75_DFEC)
// triggers; each other entry below is a once-logging shell for the retail function it names.
#define SPLITTER_TRIGGER(name) \
  { #name, static_cast< CPatterned::StateMachine::TriggerFunc >(&CSplitterMainChassis::name) }
#define SPLITTER_STATE(name) \
  { #name, static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterMainChassis::name) }
#define SPLITTER_CODE(name) \
  { #name, static_cast< CPatterned::StateMachine::CodeFunc >(&CSplitterMainChassis::name) }
#define SPLITTER_TRIGGER_PORTED(name) \
  { #name, static_cast< CPatterned::StateMachine::TriggerFunc >(&CSplitterMainChassis::name) }
#define SPLITTER_STATE_PORTED(name) \
  { #name, static_cast< CPatterned::StateMachine::StateFunc >(&CSplitterMainChassis::name) }

CPatterned::StateMachine::STriggerFunction skTriggers[] = {
    SPLITTER_TRIGGER(StateOver),
    SPLITTER_TRIGGER(IsIntact),
    SPLITTER_TRIGGER_PORTED(IsDisabled),
    SPLITTER_TRIGGER_PORTED(ShouldWaitForSnatch),
    SPLITTER_TRIGGER_PORTED(ShotAt),
    SPLITTER_TRIGGER_PORTED(LostHead),
    SPLITTER_TRIGGER(HasHead),
    SPLITTER_TRIGGER(HasRetreatPoint),
    SPLITTER_TRIGGER_PORTED(HasTarget),
    SPLITTER_TRIGGER(CanReachTarget),
    SPLITTER_TRIGGER(HasLineOfSight),
    SPLITTER_TRIGGER(PathShagged),
    SPLITTER_TRIGGER(PathOver),
    SPLITTER_TRIGGER_PORTED(PatrolPathOver),
    SPLITTER_TRIGGER(ShouldLegStab),
    SPLITTER_TRIGGER(ShouldMorphballStab),
    SPLITTER_TRIGGER(ShouldSpinAttack),
    SPLITTER_TRIGGER(ShouldLaserSweep),
    SPLITTER_TRIGGER(ShouldDodge),
    SPLITTER_TRIGGER(IsSpinAttackOver),
    SPLITTER_TRIGGER(SpunIntoPlayer),
    SPLITTER_TRIGGER(SpunIntoGeometry),
    SPLITTER_TRIGGER(BreakOutOfSpin),
    SPLITTER_TRIGGER(TooManyBounces),
    SPLITTER_TRIGGER_PORTED(DeployFromHoldingTube),
    SPLITTER_TRIGGER(ShouldDeploy),
    SPLITTER_TRIGGER(DeployPathOver),
    SPLITTER_TRIGGER_PORTED(PrepareForDocking),
};

CPatterned::StateMachine::SStateFunction skStates[] = {
    SPLITTER_STATE_PORTED(Start),
    SPLITTER_STATE(Idle),
    SPLITTER_STATE(Inactive),
    SPLITTER_STATE(Activate),
    SPLITTER_STATE_PORTED(Patrol),
    SPLITTER_STATE_PORTED(Scanning),
    SPLITTER_STATE(PathFind),
    SPLITTER_STATE(LegStabAttack),
    SPLITTER_STATE(SpinAttack),
    SPLITTER_STATE(Dodge),
    SPLITTER_STATE(LaserSweepAttack),
    SPLITTER_STATE(SpinCollisionReaction),
    SPLITTER_STATE(SpinBounce),
    SPLITTER_STATE(SpinTelegraph),
    SPLITTER_STATE(SpinToIdle),
    SPLITTER_STATE(SpawnDrop),
    SPLITTER_STATE(HoldingTube),
    SPLITTER_STATE(WaitForDocking),
    SPLITTER_STATE(WaitForSnatch),
    SPLITTER_STATE(FollowDeployPath),
    SPLITTER_STATE(DeploymentLanding),
    SPLITTER_STATE(HeadExplosion),
};

CPatterned::StateMachine::SCodeFunction skCodeFuncs[] = {
    SPLITTER_CODE(SelectTarget),
    SPLITTER_CODE(SetTargetDest),
    SPLITTER_CODE(SetRetreatDest),
    SPLITTER_CODE(SetupSpinAttack),
    SPLITTER_CODE(EndSpinAttack),
    SPLITTER_CODE(SetupLaserSweep),
    SPLITTER_CODE(SetMorphballStabLeg),
    SPLITTER_CODE(FindBestStabLeg),
    SPLITTER_CODE(FindBestDodgeDirection),
    SPLITTER_CODE(BounceOffPlayer),
    SPLITTER_CODE(BounceOffGeometry),
};

#undef SPLITTER_TRIGGER
#undef SPLITTER_STATE
#undef SPLITTER_CODE
#undef SPLITTER_TRIGGER_PORTED
#undef SPLITTER_STATE_PORTED


// The chassis's own path update (fn_75_986C, 0x28C) is ported as
// CSplitterMainChassis::PortUpdatePath.

// The module's own calls in the main chassis's script-message handler (fn_75_F1C0). All six are
// over the 0x80 a slice takes, so each stays a named once-logging shell.

} // namespace

// The state machine names that are not ported yet: each stands in for the retail
// function the REL's initialiser puts in the table and logs it once, so the log
// says which retail body the state machine asked for.
#define SPLITTER_TRIGGER_STANDIN(name, fn)                                       \
  bool CSplitterMainChassis::name(CStateManager&, const CTriggerData&) const {    \
    StandIn(#fn " (the chassis's " #name " trigger)");                            \
    return false;                                                                  \
  }
#define SPLITTER_STATE_STANDIN(name, fn)                                          \
  void CSplitterMainChassis::name(CStateManager&, EStateMsg, float) {               \
    StandIn(#fn " (the chassis's " #name " state)");                              \
  }
#define SPLITTER_CODE_STANDIN(name, fn)                                           \
  void CSplitterMainChassis::name(CStateManager&, float) {                          \
    StandIn(#fn " (the chassis's " #name " code)");                               \
  }
SPLITTER_TRIGGER_STANDIN(IsIntact, fn_75_E62C)
SPLITTER_TRIGGER_STANDIN(HasHead, fn_75_E58C)
SPLITTER_TRIGGER_STANDIN(CanReachTarget, fn_75_E530)
SPLITTER_TRIGGER_STANDIN(PathShagged, fn_75_E4DC)
SPLITTER_TRIGGER_STANDIN(PathOver, fn_75_E494)
SPLITTER_TRIGGER_STANDIN(IsSpinAttackOver, fn_75_E084)
SPLITTER_TRIGGER_STANDIN(SpunIntoPlayer, fn_75_E078)
SPLITTER_TRIGGER_STANDIN(SpunIntoGeometry, fn_75_E06C)
SPLITTER_TRIGGER_STANDIN(BreakOutOfSpin, fn_75_E060)
SPLITTER_TRIGGER_STANDIN(TooManyBounces, fn_75_E040)
SPLITTER_TRIGGER_STANDIN(ShouldDeploy, fn_75_E028)
SPLITTER_TRIGGER_STANDIN(DeployPathOver, fn_75_E00C)
SPLITTER_STATE_STANDIN(Idle, fn_75_DFE4)
SPLITTER_STATE_STANDIN(Inactive, fn_75_DF88)
SPLITTER_STATE_STANDIN(Activate, fn_75_DDF8)
SPLITTER_STATE_STANDIN(SpinAttack, fn_75_D7D4)
SPLITTER_STATE_STANDIN(Dodge, fn_75_D61C)
SPLITTER_STATE_STANDIN(SpinCollisionReaction, fn_75_D2A0)
SPLITTER_STATE_STANDIN(SpinBounce, fn_75_D13C)
SPLITTER_STATE_STANDIN(SpinTelegraph, fn_75_D0A4)
SPLITTER_STATE_STANDIN(SpinToIdle, fn_75_CF44)
SPLITTER_STATE_STANDIN(SpawnDrop, fn_75_CCF0)
SPLITTER_STATE_STANDIN(HoldingTube, fn_75_CC98)
SPLITTER_STATE_STANDIN(WaitForDocking, fn_75_C9E4)
SPLITTER_STATE_STANDIN(WaitForSnatch, fn_75_C8F4)
SPLITTER_STATE_STANDIN(FollowDeployPath, fn_75_C51C)
SPLITTER_STATE_STANDIN(DeploymentLanding, fn_75_C034)
SPLITTER_CODE_STANDIN(SetRetreatDest, fn_75_BBC0)
SPLITTER_CODE_STANDIN(SetupSpinAttack, fn_75_BAA8)
SPLITTER_CODE_STANDIN(EndSpinAttack, fn_75_B9B0)
SPLITTER_CODE_STANDIN(BounceOffPlayer, fn_75_B1D8)
SPLITTER_CODE_STANDIN(BounceOffGeometry, fn_75_B104)
#undef SPLITTER_TRIGGER_STANDIN
#undef SPLITTER_STATE_STANDIN
#undef SPLITTER_CODE_STANDIN

// The state machine entries the area-4 run reaches, whose retail body is ported here.

// fn_75_DFE8 (the chassis's Start state, 0x4 bytes): the body is a single return, so the state
// does nothing.
void CSplitterMainChassis::Start(CStateManager&, EStateMsg, float) {}

// The squared distance from the chassis to its target (the id at 0xCC0), or -1.0f without one.
// The Should* triggers below each look the target up (GetObjectById, the null test) and build
// it inline from the two actors' positions (0x54, 0x58 and 0x5C).
// The -1 (no target) answer is unreachable from the chassis's triggers: they all run from
// PathFind, which the FSM only enters after SelectTarget (fn_75_BE28) stored the player's id at
// 0xCC0, the one writer of that id, and the player is never deleted.
static float PortTargetDistanceSq(const CSplitterMainChassis& self, const CStateManager& mgr) {
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(self.GetSyncId()));
  if (target == nullptr) {
    return -1.f;
  }
  return (target->GetTranslation() - self.GetTranslation()).MagSquared();
}

// fn_75_E56C (the chassis's HasRetreatPoint trigger, 0xE56C-0xE58C): the follow object's id at
// 0xCC6 (the connected object fn_75_AC68 keeps) is valid (0xE570-0xE584: the two subf and or,
// srwi 31).
// @ fn_75_E56C
bool CSplitterMainChassis::HasRetreatPoint(CStateManager&, const CTriggerData&) const {
  return mFollowObjectId != kInvalidUniqueId;
}

// fn_75_E098 (the chassis's ShouldDodge trigger, 0xE098-0xE0B0): the dodge direction at 0xD04
// is not -1 (0xE09C-0xE0A8: subfic -1, addi 1, or, srwi 31).
// @ fn_75_E098
bool CSplitterMainChassis::ShouldDodge(CStateManager&, const CTriggerData&) const {
  return mDodgeDirection != -1;
}

// fn_75_E524 (the chassis's HasLineOfSight trigger, 0xE524-0xE530): the CVisualKnowledge's
// (0xC70) flag byte at 0xCA8, its 0x80 bit - the host tracker's line-of-sight flag.
// @ fn_75_E524
bool CSplitterMainChassis::HasLineOfSight(CStateManager&, const CTriggerData&) const {
  return mVisualKnowledge.HasLineOfSight();
}

// fn_75_E2F0 (the chassis's ShouldMorphballStab trigger, 0xE2F0-0xE3A4): false unless the data's
// flag word (0x828) has its 0x4 bit (0xE304-0xE30C), the leg stab timer at 0xD10 is at most 0.0f
// (0xE310-0xE324, lbl_75_rodata_390) and the target at 0xCC0 is there (0xE328-0xE340); then the
// answer is the squared distance below the square of the data's leg stab min range (0x830,
// 0xE344-0xE384: fcmpo, the cr0 lt bit).
bool CSplitterMainChassis::ShouldMorphballStab(CStateManager& mgr, const CTriggerData&) const {
  if ((mData.mUnknownCEF5C2FE & 0x4) == 0 || mLegStabTimer > 0.f) {
    return false;
  }
  const float distSq = PortTargetDistanceSq(*this, mgr);
  if (distSq < 0.f) {
    return false;
  }
  return distSq < mData.mLegStabMinAttackRange * mData.mLegStabMinAttackRange;
}

// fn_75_E3A4 (the chassis's ShouldLegStab trigger, 0xE3A4-0xE478): the same three tests as
// ShouldMorphballStab (0xE3B8-0xE3F4); then the squared distance must be at least the square of
// the leg stab min range (0x830, 0xE43C-0xE444: cror eq, gt, eq) and at most the square of the
// max range (0x834, 0xE448-0xE450: cror eq, lt, eq).
bool CSplitterMainChassis::ShouldLegStab(CStateManager& mgr, const CTriggerData&) const {
  if ((mData.mUnknownCEF5C2FE & 0x4) == 0 || mLegStabTimer > 0.f) {
    return false;
  }
  const float distSq = PortTargetDistanceSq(*this, mgr);
  if (distSq < 0.f) {
    return false;
  }
  return distSq >= mData.mLegStabMinAttackRange * mData.mLegStabMinAttackRange &&
         distSq <= mData.mLegStabMaxAttackRange * mData.mLegStabMaxAttackRange;
}

// fn_75_E1D0 (the chassis's ShouldSpinAttack trigger, 0xE1D0-0xE2F0): false unless 0xD40's 0x80
// bit (the IM00/IM01 messages set it, 0xE1EC-0xE1F4) is set and the spin penalty timer at 0xD14
// is at most 0.0f (0xE1F8-0xE20C); in a team (0xCBE valid, 0xE210-0xE220) the team must also
// let a melee attack start (0xE244 CanStartAttack with 0). Then the target at 0xCC0 must be there
// (0xE250-0xE268) at a squared distance from the square of the data's spin attack min range
// (0x880) to the square of its max range (0x884), both ends included (0xE2B0-0xE2C4).
bool CSplitterMainChassis::ShouldSpinAttack(CStateManager& mgr, const CTriggerData&) const {
  if ((mFlagsD40 & 0x80) == 0 || mSpinPenaltyTimer > 0.f) {
    return false;
  }
  if (mTeamId != kInvalidUniqueId &&
      !CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Melee, mgr, mTeamId, GetUniqueId())) {
    return false;
  }
  const float distSq = PortTargetDistanceSq(*this, mgr);
  if (distSq < 0.f) {
    return false;
  }
  return distSq >= mData.mSpinAttackMinAttackRange * mData.mSpinAttackMinAttackRange &&
         distSq <= mData.mSpinAttackMaxAttackRange * mData.mSpinAttackMaxAttackRange;
}

// fn_75_E0B0 (the chassis's ShouldLaserSweep trigger, 0xE0B0-0xE1D0): false unless the data's
// flag word (0x828) has its 0x40 bit (0xE0CC-0xE0D4) and the laser sweep timer at 0xD18 is at
// most 0.0f (0xE0D8-0xE0EC); in a team (0xCBE valid) the team must let a projectile attack start
// (0xE124 CanStartAttack with 1). Then the target at 0xCC0 must be there at a squared distance
// from the square of the data's laser sweep min range (0x85C) to the square of its max range
// (0x860), both ends included (0xE190-0xE1A4). Every shipped placement sets the 0x40 bit (the
// flag words are 0x6E, 0x76, 0x7C and 0x7D), so only the timer half of the first test runs.
// @ fn_75_E0B0
bool CSplitterMainChassis::ShouldLaserSweep(CStateManager& mgr, const CTriggerData&) const {
  if ((mData.mUnknownCEF5C2FE & 0x40) == 0 || mLaserSweepTimer > 0.f) {
    return false;
  }
  if (mTeamId != kInvalidUniqueId &&
      !CScriptTeamAiMgr::CanStartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId,
                                        GetUniqueId())) {
    return false;
  }
  const float distSq = PortTargetDistanceSq(*this, mgr);
  if (distSq < 0.f) {
    return false;
  }
  return distSq >= mData.mLaserSweepMinAttackRange * mData.mLaserSweepMinAttackRange &&
         distSq <= mData.mLaserSweepMaxAttackRange * mData.mLaserSweepMaxAttackRange;
}

// fn_75_BC70 (the chassis's SetTargetDest code, 0xBC70-0xBE28): the destination starts as the
// chassis's own position (0xBC98-0xBCB0) and 0xD40's 0x20 bit is cleared (0xBCB4-0xBCBC). The
// target at 0xCC0 is looked up (0xBCC8); without one the position stays. With one: without line
// of sight (0xBCD4-0xBCDC: the CVisualKnowledge's flag byte at 0xCA8, its 0x80 bit) the target's
// position is the destination and the 0x20 bit is set (0xBCE0-0xBD04). With line of sight the
// team role is asked for (0xBD2C GetTeamAiRole with the team at 0xCBE and this actor's id); a
// role other than 2 (kTAR_Projectile, 0xBD38-0xBD40) - or none - takes the target's position and
// sets the bit (0xBDBC-0xBDE0), while the projectile role takes it only when the squared distance
// is at least the square of half the data's spin attack min plus max range (0x880 and 0x884,
// 0xBD44-0xBD98, lbl_75_rodata_3A8 0.5f, fcmpo with cror eq, gt, eq). Then the base's path
// navigation (0x6F4) gets the destination (0xBDEC SetDestination) and its face target (0x718)
// kInvalidUniqueId (0xBDF0-0xBE04), and the team join runs (0xBE08 fn_75_8DF8).
void CSplitterMainChassis::SetTargetDest(CStateManager& mgr, float) {
  CVector3f dest = GetTranslation();
  mFlagsD40 &= ~0x20;
  if (const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mSyncId))) {
    if (!mVisualKnowledge.HasLineOfSight()) {
      dest = target->GetTranslation();
      mFlagsD40 |= 0x20;
    } else {
      const CTeamAiRole* role = CScriptTeamAiMgr::GetTeamAiRole(mgr, mTeamId, GetUniqueId());
      if (role != nullptr && role->GetTeamAiRole() == CTeamAiRole::kTAR_Projectile) {
        const float range =
            0.5f * (mData.mSpinAttackMinAttackRange + mData.mSpinAttackMaxAttackRange);
        if ((target->GetTranslation() - GetTranslation()).MagSquared() >= range * range) {
          dest = target->GetTranslation();
          mFlagsD40 |= 0x20;
        }
      } else {
        dest = target->GetTranslation();
        mFlagsD40 |= 0x20;
      }
    }
  }
  mPathFindNavigation.SetDestination(dest);
  mPathFindNavigation.SetFaceTarget(kInvalidUniqueId);
  PortJoinTeam(mgr);
}

// fn_75_8DF8 (the chassis's team join, 0x8DF8-0x8EA4): only with no team at 0xCBE yet
// (0x8E1C-0x8E28) the associated team is asked for (0x8E38 GetAssociatedTeamId) and kept at
// 0xCBE (0x8E44); when that is valid (0x8E48-0x8E54) and casts to a CScriptTeamAiMgr
// (0x8E68-0x8E74) the chassis joins it with the roles 1, 2 and -1 (0x8E78-0x8E88 JoinTeam:
// kTAR_Melee, kTAR_Projectile, kTAR_Invalid). The cast cannot fail: GetAssociatedTeamId only
// answers ids its CTeamAiPredicate just cast to a CScriptTeamAiMgr.
// @ fn_75_8DF8
void CSplitterMainChassis::PortJoinTeam(CStateManager& mgr) {
  if (mTeamId != kInvalidUniqueId) {
    return;
  }
  mTeamId = CScriptTeamAiMgr::GetAssociatedTeamId(*this, mgr);
  if (mTeamId == kInvalidUniqueId) {
    return;
  }
  if (CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamId))) {
    team->JoinTeam(*this, CTeamAiRole::kTAR_Melee, CTeamAiRole::kTAR_Projectile,
                   CTeamAiRole::kTAR_Invalid);
  }
}

// fn_75_DC08 (the chassis's PathFind state, 0xDC08-0xDCF8). Activate (0xDC50): the body
// controller's command manager gets its blend mode (0xDC54-0xDC58, stw 1 at the body
// controller's 0x4C) and the steering speed range 0xD0C, 0xD0C (0xDC5C-0xDC6C); with 0xD40's
// 0x20 bit (a destination, 0xDC70-0xDC78) the path navigation's PathFind runs (0xDC90); then
// 0xD40's 0x10 bit is cleared (0xDC94-0xDCA0). Update (0xDCA8): with the 0x20 bit the PathFind
// runs (0xDCBC). Every message but deactivate (0xDCC0 cmpwi 2) then runs the chassis's own path
// update (0xDCD0 fn_75_986C).
void CSplitterMainChassis::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_FullSpeed);
    BodyController()->CommandMgr().SetSteeringSpeedRange(mSteeringSpeed, mSteeringSpeed);
    if ((mFlagsD40 & 0x20) != 0) {
      mPathFindNavigation.PathFind(mgr, msg, dt, *this);
    }
    mFlagsD40 &= ~0x10;
  } else if (msg == kStateMsg_Update) {
    if ((mFlagsD40 & 0x20) != 0) {
      mPathFindNavigation.PathFind(mgr, msg, dt, *this);
    }
  }
  if (msg != kStateMsg_Deactivate) {
    PortUpdatePath(mgr);
  }
}

// fn_75_E64C (the chassis's StateOver trigger, 0xE64C-0xE65C): a REL copy of the DOL's
// CPatterned::GetAnimOver (0x80151990) - the animation state at 0x6B4 is kAS_Over (3).
bool CSplitterMainChassis::StateOver(CStateManager& mgr, const CTriggerData& data) const {
  return GetAnimOver(mgr, data);
}

// fn_75_B66C (the chassis's SetMorphballStabLeg code, 0xB66C-0xB678): the stab leg at 0xD08
// is 0.
// @ fn_75_B66C
void CSplitterMainChassis::SetMorphballStabLeg(CStateManager&, float) { mStabLeg = 0; }

// fn_75_B57C (the chassis's FindBestStabLeg code, 0xB57C-0xB66C): the stab leg at 0xD08 starts
// as -1 (0xB588-0xB594). With the target at 0xCC0 (0xB5A8-0xB5B0) the offset to it is dotted
// with the transform's Y column (0x28, 0x38 and 0x48; 0xB5CC-0xB5FC) and X column (0x24, 0x34
// and 0x44; 0xB60C-0xB614 / 0xB634-0xB63C), each against 0.0f (lbl_75_rodata_390, fcmpo with
// ble): ahead and right is leg 1, ahead and left 2, behind and right 3, behind and left 4.
// The null-target arm is unreachable: the state follows ShouldLegStab, which needs the target,
// and the target id is only ever the player's (see PortTargetDistanceSq).
// @ fn_75_B57C
void CSplitterMainChassis::FindBestStabLeg(CStateManager& mgr, float) {
  mStabLeg = -1;
  const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mSyncId));
  if (target == nullptr) {  // 0xB5AC-0xB5B0
    return;
  }
  const CVector3f offset = target->GetTranslation() - GetTranslation();
  const bool right = CVector3f::Dot(GetTransform().GetColumn(kDX), offset) > 0.f;
  if (CVector3f::Dot(GetTransform().GetColumn(kDY), offset) > 0.f) {
    mStabLeg = right ? 1 : 2;
  } else {
    mStabLeg = right ? 3 : 4;
  }
}

// fn_75_909C (0x909C-0x91E4): whether a dodge of the given length along a direction stays
// clear. The ray runs from the bounding box's centre (0x90D8 GetBoundingBox, 0x90E4
// GetCenterPoint) to that point plus the direction times the length (0x90E8-0x913C); with the
// filter that includes kMT_Solid (lbl_75_data_CA4, 0x3B) and excludes kMT_CollisionActor
// (lbl_75_data_CA8, 0x2F) RayCollideWorld must answer true (0x9190-0x9198, the ray is clear) and
// the end must be on the path search's mesh (0x91A4 OnPath at 0xB7C, kR_Success).
// @ fn_75_909C
bool CSplitterMainChassis::PortDodgeIsClear(CStateManager& mgr, const CVector3f& dir,
                                            float length) {
  const CVector3f start = GetBoundingBox().GetCenterPoint();
  const CVector3f end = start + dir * length;
  const CMaterialFilter filter = CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid), CMaterialList(kMT_CollisionActor));
  return mgr.RayCollideWorld(start, end, filter, this) &&
         mPathFindSearch.OnPath(end) == CPathFindSearch::kR_Success;
}

// fn_75_91E4 (0x91E4-0x951C): the dodge direction away from a missile's offset. The base's
// FindBestStepDirection (0x9260) picks the axis: for kSD_Left the transform's Y column (the two
// answers 1 and 0), for kSD_Right the negated Y column (0 and 1), for kSD_Forward the negated X
// column (3 and 2) and otherwise the X column (2 and 3) (0x9264-0x9304). Every other listening
// AI actor of the same area (the state manager's list at 0x830, 0x9308-0x9438; the area id at
// 0x4) closer than the step distance at 0xD38 (squared, 0x9378) blocks a side when it lies within
// 60 degrees of it (lbl_75_rodata_398, 1.0471976f, GetAngleDiff): the axis side when its dot with
// the offset is at least 0.0f (0x93A0-0x93D4), the opposite side otherwise (0x93D8-0x9414). Each
// side still open must also be clear (fn_75_909C with the step distance, 0x943C-0x949C); with
// both open the random number's 0x4000 bit (0x94B4 Next, 0x94B8) closes the opposite side, else
// the axis side. The answer is the opposite side's value, else the axis side's, else -1.
int CSplitterMainChassis::PortFindDodgeDirection(CStateManager& mgr, const CVector3f& away) {
  const float rangeSq = mStepDistance * mStepDistance;
  CVector3f axis = GetTransform().GetColumn(kDX);
  int axisAnswer = 3;
  int oppositeAnswer = 2;
  switch (FindBestStepDirection(away)) {
  case pas::kSD_Left:
    axis = GetTransform().GetColumn(kDY);
    axisAnswer = 0;
    oppositeAnswer = 1;
    break;
  case pas::kSD_Right:
    axis = -GetTransform().GetColumn(kDY);
    axisAnswer = 1;
    oppositeAnswer = 0;
    break;
  case pas::kSD_Forward:
    axis = -axis;
    axisAnswer = 2;
    oppositeAnswer = 3;
    break;
  default:
    break;
  }
  bool axisOpen = true;
  bool oppositeOpen = true;
  CObjectList& list = mgr.ObjectListById(kOL_ListeningAi);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    // The retail reads the entry's position (0x54-0x5C) without a cast: the list holds AI actors.
    const CActor* other = TCastToConstPtr< CActor >(list[i]);
    if (other == nullptr || other == this || other->GetCurrentAreaId() != GetCurrentAreaId()) {  // 0x9320-0x933C
      continue;
    }
    const CVector3f offset = other->GetTranslation() - GetTranslation();
    if (offset.MagSquared() >= rangeSq) {
      continue;
    }
    if (CVector3f::Dot(offset, axis) >= 0.f) {
      if (axisOpen && CVector3f::GetAngleDiff(axis, offset) < 1.0471976f) {
        axisOpen = false;
      }
    } else if (oppositeOpen && CVector3f::GetAngleDiff(-axis, offset) < 1.0471976f) {
      oppositeOpen = false;
    }
  }
  if (axisOpen) {
    axisOpen = PortDodgeIsClear(mgr, axis, mStepDistance);
  }
  if (oppositeOpen) {
    oppositeOpen = PortDodgeIsClear(mgr, -axis, mStepDistance);
  }
  if (oppositeOpen && axisOpen) {
    if ((mgr.Random()->Next() & 0x4000) != 0) {
      oppositeOpen = false;
    } else {
      axisOpen = false;
    }
  }
  if (oppositeOpen) {
    return oppositeAnswer;
  }
  return axisOpen ? axisAnswer : -1;
}

// fn_75_B2A0 (the chassis's FindBestDodgeDirection code, 0xB2A0-0xB540): the dodge direction at
// 0xD04 starts as -1 (0xB2D4-0xB2DC). Only with the data's flag word (0x828) bit 0x10 or 0x20
// (0xB2E4-0xB2F4), the timer at 0xD34 below 1.0f (lbl_75_rodata_364, 0xB2F8-0xB308) and the dodge
// timer at 0xD1C at most 0.0f (0xB30C-0xB324): a Range(0.0f, 100.0f) roll (lbl_75_rodata_37C,
// 0xB33C) above the data's dodge chance (0x84C) restarts the dodge timer with the data's min
// dodge interval (0x848, 0xB500-0xB504). Otherwise the target at 0xCC0 must be the player
// (0xB364 TCastToPtr<CPlayer>) orbiting the chassis or its head (the player's 0x3B0 against 0x8
// and 0xC6C, 0xB370-0xB388); then the near list of projectiles (lbl_75_data_C88, 0x23) in a box
// 50.0f (lbl_75_rodata_3B8) around the chassis (0xB38C-0xB430) is searched for the first missile
// (the weapon's 0x160 is 6, kWT_Missile) in the chassis's area (0x4) (0xB44C-0xB484). For it the
// flag bit 0x20 sets the direction to 5 (0xB488-0xB498), and the bit 0x10 replaces it with
// fn_75_91E4's answer for the offset from the missile (0xB49C-0xB4DC).
void CSplitterMainChassis::FindBestDodgeDirection(CStateManager& mgr, float) {
  mDodgeDirection = -1;
  if ((mData.mUnknownCEF5C2FE & 0x30) == 0 || mTimerD34 >= 1.f || mDodgeTimer > 0.f) {
    return;
  }
  // Every shipped placement has a dodge chance of 100.0f (0x47be3298) and the flag bits 0x30
  // set, so neither this arm nor the flag half of the test above runs.
  if (mgr.Random()->Range(0.f, 100.f) > mData.mDodgeChance) {
    mDodgeTimer = mData.mMinDodgeInterval;
    return;
  }
  const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mSyncId));
  if (player == nullptr ||  // 0xB368-0xB36C
      (player->GetOrbitTargetId() != GetUniqueId() && player->GetOrbitTargetId() != mCommandModuleId)) {
    return;
  }
  const CVector3f pos = GetTranslation();
  const CAABox box(pos - CVector3f(50.f, 50.f, 50.f), pos + CVector3f(50.f, 50.f, 50.f));
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, box, CMaterialFilter::MakeInclude(CMaterialList(kMT_Projectile)),
                    nullptr);
  for (int i = 0; i < nearList.size(); ++i) {
    const CWeapon* weapon = TCastToConstPtr< CWeapon >(mgr.GetObjectById(nearList[i]));
    if (weapon == nullptr || weapon->GetType() != kWT_Missile ||  // 0xB464-0xB484
        weapon->GetCurrentAreaId() != GetCurrentAreaId()) {
      continue;
    }
    if ((mData.mUnknownCEF5C2FE & 0x20) != 0) {
      mDodgeDirection = 5;
    }
    if ((mData.mUnknownCEF5C2FE & 0x10) != 0) {
      mDodgeDirection = PortFindDodgeDirection(mgr, pos - weapon->GetTranslation());
    }
    return;
  }
}

// fn_75_B678 (the chassis's SetupLaserSweep code, 0xB678-0xB9AC): only with the head at 0xC6C
// (0xB6E0-0xB6EC TCastToPtr<CSplitterCommandModule>). The sweep starts at the head's laser
// origin (0xB6F8 fn_75_5A80) and aims 30.0f (lbl_75_rodata_384) along the chassis's Y column, or
// at the target's (0xCC0) aim position (0xB774, the CActor vtable word 0x54, with 0.0f). The
// offset and its flat form (z 0.0f) must both be IsMagnitudeSafe (0xB7B4-0xB7CC). An offset that
// climbs more than 45 degrees (lbl_75_rodata_3BC, 0.7853982f; z above 0.0f) - or drops more than
// 15 degrees (lbl_75_rodata_3C0, 0.2617994f) - is clamped to that angle from the flat direction
// (Slerp of the normalized flat and offset, times the offset's length; 0xB7F8-0xB8B8). The two
// sweep ends are the offset turned -45 and +45 degrees about z (FastCosR/FastSinR of 45 degrees,
// 0xB8BC-0xB92C) from the origin; the head takes them (0xB930 fn_75_5AD8) and the team starts a
// projectile attack (0xB958 StartAttack with 1).
void CSplitterMainChassis::SetupLaserSweep(CStateManager& mgr, float) {
  CSplitterCommandModule* head =
      TCastToPtr< CSplitterCommandModule >(mgr.ObjectById(mCommandModuleId));
  if (head == nullptr) {  // 0xB6E8-0xB6EC
    return;
  }
  const CVector3f origin = head->PortLaserOrigin(mgr);
  CVector3f aim = origin + 30.f * GetTransform().GetColumn(kDY);
  if (const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mSyncId))) {
    aim = target->GetAimPosition(mgr, 0.f);
  }
  const CVector3f offset = aim - origin;
  const CVector3f flat(offset.GetX(), offset.GetY(), 0.f);
  if (!offset.IsMagnitudeSafe() || !flat.IsMagnitudeSafe()) {
    return;
  }
  const float angle = CVector3f::GetAngleDiff(offset, flat);
  CVector3f dir = offset;
  const float limit = offset.GetZ() > 0.f ? 0.7853982f : 0.2617994f;
  if (angle > limit) {
    dir = CVector3f::Slerp(flat.AsNormalized(), offset.AsNormalized(),
                           CRelAngle::FromRadians(limit)) *
          offset.Magnitude();
  }
  const float c = CMath::FastCosR(0.7853982f);
  const float sn = CMath::FastSinR(0.7853982f);
  const CVector3f first(origin.GetX() + (c * dir.GetX() + sn * dir.GetY()),
                        origin.GetY() + (c * dir.GetY() - sn * dir.GetX()),
                        origin.GetZ() + dir.GetZ());
  const CVector3f second(origin.GetX() + (c * dir.GetX() - sn * dir.GetY()),
                         origin.GetY() + (c * dir.GetY() + sn * dir.GetX()),
                         origin.GetZ() + dir.GetZ());
  head->PortStartLaserSweep(first, second);
  CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId, GetUniqueId());
}

// fn_75_9AF8 (0x9AF8-0x9B98): every collision actor of the manager at 0xC68 (0x9B28
// GetCollisionDescFromIndex, to GetNumCollisionActors at 0x9B7C) that casts to a CCollisionActor
// (0x9B44-0x9B4C) gets the first vulnerability when it is the "target_LCTR" actor at 0xCC2
// (0x9B50-0x9B64), else the second (0x9B6C-0x9B70).
void CSplitterMainChassis::PortSetCollisionVulnerability(CStateManager& mgr,
                                                         const CDamageVulnerability& target,
                                                         const CDamageVulnerability& other) {
  for (uint i = 0; i < mCollisionActorManager->GetNumCollisionActors(); ++i) {
    const TUniqueId id = mCollisionActorManager->GetCollisionDescFromIndex(i).GetCollisionActorId();
    CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(id));
    if (actor == nullptr) {  // 0x9B48-0x9B4C
      continue;
    }
    actor->SetDamageVulnerability(id == mTargetActorId ? target : other);
  }
}

// fn_75_D3E0 (the chassis's LaserSweepAttack state, 0xD3E0-0xD618). Activate (0xD424): the
// animation state at 0x6B4 is kAS_Ready (1), 0xD41's 0x40 bit is set, and the "target_LCTR"
// collision actor gets the base's vulnerability (0xD444 CPatterned::GetDamageVulnerability) while
// the others get the DOL's ImmuneRicochetVulnerability (the host's ReflectVulnerabilty; 0xD438,
// 0xD458 fn_75_9AF8). Update (0xD460): when CTryCommand::ShouldRetry (the host's
// CAnimationState::CanIssueCommand) allows body state 9 (kAS_LoopAttack, 0xD468) a loop attack
// command of type 0 goes out (lbl_75_data_E0C, id 7, not waiting for the animation, no skip;
// 0xD478-0xD4F4). Then without the head at 0xC6C (0xD510-0xD518), or with its laser sweep stage
// at 0xF34 back to -1 (0xD51C-0xD524), the body state is told to exit (id 10, kBSC_ExitState;
// 0xD528-0xD548). Deactivate (0xD55C): the animation state is kAS_NotReady (0), the exit command
// goes out (0xD584), every collision actor is immune again (0xD59C-0xD5B8), the laser sweep timer
// at 0xD18 restarts with the data's laser sweep interval (0x858, 0xD5BC-0xD5CC), 0xD41's 0x40 bit
// is cleared and the team's projectile attack ends (0xD5FC EndAttack with 1, the team at 0xCBE,
// this actor and false).
// @ fn_75_D3E0
void CSplitterMainChassis::LaserSweepAttack(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mFlagsD41 |= 0x40;
    PortSetCollisionVulnerability(mgr, *CPatterned::GetDamageVulnerability(),
                                  CDamageVulnerability::ReflectVulnerabilty());
  } else if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_LoopAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCLoopAttackCmd(pas::kLAT_Zero));
    }
    const CSplitterCommandModule* head =
        TCastToConstPtr< CSplitterCommandModule >(mgr.GetObjectById(mCommandModuleId));
    if (head == nullptr || head->GetLaserSweepStage() == -1) {  // 0xD514-0xD524
      BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    PortSetCollisionVulnerability(mgr, CDamageVulnerability::ReflectVulnerabilty(),
                                  CDamageVulnerability::ReflectVulnerabilty());
    mLaserSweepTimer = mData.mLaserSweepInterval;
    mFlagsD41 &= ~0x40;
    CScriptTeamAiMgr::EndAttack(CScriptTeamAiMgr::kAT_Projectile, mgr, mTeamId, GetUniqueId(),
                                false);
  }
}

// fn_75_DA38 (the chassis's LegStabAttack state, 0xDA38-0xDBA8). Activate (0xDA70): with a stab
// leg from 0 to 4 at 0xD08 the animation state at 0x6B4 is kAS_Ready (1) and 0xD40's 0x1 bit is
// set (0xDA84-0xDA94), otherwise the state is kAS_Over (3, 0xDA9C-0xDAA0). Update (0xDAA8):
// when the DOL's CTryCommand::ShouldRetry (fn_801B73E4, the host's
// CAnimationState::CanIssueCommand) allows body state 7 (kAS_MeleeAttack, 0xDAB0-0xDAB4) a melee
// attack command goes to the body controller's command manager (0xDB2C DeliverCmd, id 5) with
// the severity lbl_75_rodata_2F8[leg] (the table 0, 1, 2, 3, 4; 0xDAD8-0xDAE0), a zero target
// and no target flag (0xDB1C-0xDB28). Deactivate (0xDB7C): the animation state is kAS_NotReady
// (0), the leg stab timer at 0xD10 gets the data's leg stab interval (0x82C) and 0xD40's 0x1 bit
// is cleared.
// @ fn_75_DA38
void CSplitterMainChassis::LegStabAttack(CStateManager&, EStateMsg msg, float) {
  static const pas::ESeverity skLegStabSeverity[5] = {pas::kS_Zero, pas::kS_One, pas::kS_Two,
                                                       pas::kS_Three, pas::kS_Four};
  if (msg == kStateMsg_Activate) {
    if (mStabLeg >= 0 && mStabLeg <= 4) {
      mAnimationState.SetState(CAnimationState::kAS_Ready);
      mFlagsD40 |= 0x1;
    } else {
      // Unreachable in the shipped data: FindBestStabLeg (B57C) and SetMorphballStabLeg (B66C)
      // leave a leg of 0-4; B57C's -1 is only on its null-target arm, which PathFind rules out.
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
  } else if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_MeleeAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCMeleeAttackCmd(skLegStabSeverity[mStabLeg]));
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mLegStabTimer = mData.mLegStabAttackInterval;
    mFlagsD40 &= ~0x1;
  }
}

// fn_75_DCF8 (the chassis's Scanning state, 0xDCF8-0xDCFC): the retail body is a single return,
// so the state does nothing. The scan timer at 0xD28 is not set here: the retail's ACTN script
// message sets it in the chassis's script-message handler (fn_75_F1C0, 0x0000F40C-0x0000F42C).
void CSplitterMainChassis::Scanning(CStateManager&, EStateMsg, float) {}

// fn_75_DCFC (the chassis's Patrol state, 0xDCFC-0xDDF8): the base's waypoint navigation (the
// CWaypointNavigation at 0x6B8 - its destination is at 0x6C0 and its last destination at 0x6C2)
// runs its own patrol first (0xDD20-0xDD28: the navigation, the state manager, the message, the
// frame time and the actor). With the activate message (0xDD2C cmpwi r31, 1 - 0xDD38 cmpwi
// r31, 0) the body controller's command manager gets its blend mode (0xDD4C stw 1 at the body
// controller's 0x4C - the command manager's 0x48) and the steering speed range (0xDD50 the
// chassis's 0xD0C, 0xDD60 SetSteeringSpeedRange). With the update message (0xDD30 beq) the two
// waypoint ids - the last destination at 0x6C2 (0xDD68 lhz) and the destination at 0x6C0
// (0xDD84 lhz) - are looked up (0xDD7C and 0xDD9C GetObjectById) and cast to a script waypoint
// (0xDD80, 0xDDA0); with both there (0xDDA4, 0xDDAC) the chassis's own path update runs
// (0xDDB8 fn_75_986C) and the command manager's face vector (0xDDC0-0xDDD8: the body
// controller's 0x10, 0x14 and 0x18 - the command manager's 0x0C, 0x10 and 0x14) is copied into
// the chassis's 0x6E0, 0x6E4 and 0x6E8.
void CSplitterMainChassis::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);
  if (msg == kStateMsg_Activate) {
    BodyController()->CommandMgr().SetSteeringBlendMode(kSBM_FullSpeed);
    BodyController()->CommandMgr().SetSteeringSpeedRange(mSteeringSpeed, mSteeringSpeed);
    return;
  }
  if (msg != kStateMsg_Update) {
    return;
  }
  const CScriptWaypoint* last =
      TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(mWaypointNavigation.GetLastDestination()));
  const CScriptWaypoint* dest =
      TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(mWaypointNavigation.GetDestination()));
  // The asm's own null tests (0xDDA4 cmplwi r31, 0 and 0xDDAC cmplwi r3, 0) are this check.
  if (last == nullptr || dest == nullptr) {
    return;
  }
  PortUpdatePath(mgr);
  mPatrolFacing = BodyController()->GetCommandMgr().GetFaceVector();
}

// fn_75_986C (the chassis's own path update, 0x986C-0x9AF8): the separation vector
// (fn_75_8EC8, 0x00009890) decides the first command - with a vector that is not all zeros
// (0x000098A8-0x000098C0: each component is compared with 0.0f) the command manager gets a
// locomotion command with the separation vector as its move vector, a zero face vector and the
// weight 0.5f (0x000098C8-0x00009914). Then the command manager's move vector (the body
// controller's 0x4, 0x8 and 0xC - the command manager's 0x0, 0x4 and 0x8) is copied
// (0x00009918-0x00009930), the locomotion commands are cleared (0x00009934) and the best step
// direction for that vector is asked for (0x00009940 FindBestStepDirection). With a safe
// magnitude (0x0000994C IsMagnitudeSafe) and a step direction other than 0 (0x00009958) the
// vector becomes the face vector - the step direction 1 negates it (0x000099A0), 2 negates its x
// (0x000099E8) and 3 negates its y (0x000099C8) - the steering speed range becomes 1.0f and
// 1.0f (0x00009A04) and the command manager gets the locomotion command with the move vector,
// the normalized face vector and the weight 1.0f (0x00009A1C-0x00009A70). Otherwise the
// steering speed range becomes the chassis's 0xD0C (0x00009A78) and the command manager gets
// the locomotion command with the move vector, a zero face vector and the weight 1.0f
// (0x00009A8C-0x00009ADC).
void CSplitterMainChassis::PortUpdatePath(CStateManager& mgr) {
  CBodyStateCmdMgr& cmdMgr = BodyController()->CommandMgr();
  const CVector3f separation = PortSeparationVector(mgr);
  if (separation.GetX() != 0.f || separation.GetY() != 0.f || separation.GetZ() != 0.f) {
    cmdMgr.DeliverCmd(CBCLocomotionCmd(separation, CVector3f::Zero(), 0.5f));
  }
  const CVector3f move = cmdMgr.GetMoveVector();
  cmdMgr.ClearLocomotionCmds();
  const pas::EStepDirection step = FindBestStepDirection(move);
  if (!move.IsMagnitudeSafe() || step == pas::kSD_Forward) {
    cmdMgr.SetSteeringSpeedRange(mSteeringSpeed, mSteeringSpeed);
    cmdMgr.DeliverCmd(CBCLocomotionCmd(move, CVector3f::Zero(), 1.f));
    return;
  }
  CVector3f face = CVector3f::Zero();
  switch (step) {
  case pas::kSD_Backward:
    face = -move;
    break;
  case pas::kSD_Left:
    face = CVector3f(-move.GetX(), move.GetY(), move.GetZ());
    break;
  case pas::kSD_Right:
    face = CVector3f(move.GetX(), -move.GetY(), move.GetZ());
    break;
  default:
    break;
  }
  cmdMgr.SetSteeringSpeedRange(1.f, 1.f);
  cmdMgr.DeliverCmd(CBCLocomotionCmd(move, face.AsNormalized(), 1.f));
}

// fn_75_8EC8 (the chassis's separation steering, 0x8EC8-0x909C): the state manager's list at
// 0x830 (0x00008EF8, the fifth list - kOL_ListeningAi) is walked from its first index
// (0x00008F20 lha 0x2008) to -1 (0x0000902C). Every CPatterned (0x00008F34) other than the
// chassis (0x00008F40) in the chassis's area (0x00008F50, the entity's 0x4) is kept unless it is
// a command module whose chassis id at 0xEFA is valid (0x00008F58-0x00008F74: a docked head is
// skipped). The chassis's steering (0x61C) gives the separation from the actor's position (its
// 0x54) with the radius 6.5f (lbl_75_rodata_394) times the model data's scale x
// (0x00008F78-0x00008FB4); a safe-magnitude result (0x00008FD4) is normalized and summed and the
// count (lbl_75_rodata_390, 0.0f) goes up by 1.0f (lbl_75_rodata_364). A count above 0.0f
// (0x0000903C fcmpo, 0x00009040 ble) divides the sum by it (0x0000904C fdivs).
// @ fn_75_8EC8
CVector3f CSplitterMainChassis::PortSeparationVector(CStateManager& mgr) {
  CObjectList& list = mgr.ObjectListById(kOL_ListeningAi);
  CVector3f sum = CVector3f::Zero();
  float count = 0.f;
  for (int idx = list.GetFirstObjectIndex(); idx != -1; idx = list.GetNextObjectIndex(idx)) {
    CPatterned* other = TCastToPtr< CPatterned >(list[idx]);
    if (other == nullptr || other == this || other->GetCurrentAreaId() != GetCurrentAreaId()) {
      continue;
    }
    const CSplitterCommandModule* head = TCastToConstPtr< CSplitterCommandModule >(other);
    if (head != nullptr && head->GetChassisId() != kInvalidUniqueId) {
      continue;
    }
    const CVector3f scale = GetModelData()->GetScale();
    const CVector3f separation =
        mSteeringBehaviors.Separation(*this, other->GetTranslation(), 6.5f * scale.GetX());
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

// fn_75_ACEC (the chassis's head placement, 0xACEC-0xAFE8): the head's id at 0xC6C (0x0000AD18
// lhz) decides. With no head id the body looks at the second link at 0xC6E (0x0000AF8C): an
// object that is not a command module clears it (0x0000AFC0). With a head id the head is looked
// up (0x0000AD40 ObjectById) and cast (0x0000AD44); with no head object the id at 0xC6C is
// cleared (0x0000AF74), the flag byte 0xD41's 0x02 bit is set (0x0000AF7C rlwimi with the word's
// bit 30) and the materials 0x28 and 0x29 (kMT_Target and kMT_Orbit) are added (0x0000AF84).
// With the head there the chassis's locator transform (the segment id at 0xCC8, 0x0000AD58
// GetLctrTransform) gives the position (the transform's translation) and the Z column, and the
// flag byte 0xD41's 0x80 (0x0000AD88 extrwi with the word's bit 24) and 0x08 (0x0000AD90, the
// word's bit 28) bits, the counter at 0xD00 (0x0000AD98) and the head's editor offset (0xF24-0xF2C,
// 0x0000ADA8-0x0000ADD0) pick the path: with the bits clear, the counter at 5 and a non-zero
// offset the head looks at the position plus that offset (0x0000AEDC, 0x0000AF34 LookAt);
// otherwise the head's Y column is slerped toward the chassis's (0x0000AE30 GetAngleDiff,
// 0x0000AE60 Slerp with the turn rate 12.566371 rad/s) and the head gets the chassis's
// transform (0x0000AE44 SetTransform).
void CSplitterMainChassis::PortPlaceHead(CStateManager& mgr, float dt) {
  if (mCommandModuleId == kInvalidUniqueId) {
    if (mSecondLinkId == kInvalidUniqueId) {
      return;
    }
    CEntity* entity = mgr.ObjectById(mSecondLinkId);
    // 0x0000AFB0: the retail's own null test of the cast (cmplwi r3, 0 - 0x0000AFB4 bne).
    if (TCastToConstPtr< CSplitterCommandModule >(entity) != nullptr) {
      return;
    }
    mSecondLinkId = kInvalidUniqueId;
    return;
  }
  CSplitterCommandModule* head =
      TCastToPtr< CSplitterCommandModule >(mgr.ObjectById(mCommandModuleId));
  // 0x0000AD4C: the retail's own null test of the cast (mr. r30, r3 - 0x0000AD50 beq).
  if (head == nullptr) {
    mCommandModuleId = kInvalidUniqueId;
    mFlagsD41 |= 0x2;
    AddMaterial(kMT_Target, kMT_Orbit, mgr);
    return;
  }
  const CTransform4f locator = GetLctrTransform(mLocatorSegment);
  const CVector3f position = locator.GetTranslation();
  const CVector3f zColumn = locator.GetColumn(kDZ);
  const CVector3f offset = head->GetEditorProperties();
  const bool offsetIsZero =
      offset.GetX() == 0.f && offset.GetY() == 0.f && offset.GetZ() == 0.f;
  if ((mFlagsD41 & 0x80) == 0 && (mFlagsD41 & 0x8) == 0 && mLandedCount == 5 && !offsetIsZero) {
    // 0x0000AEDC: the head looks at the position plus the head's editor offset.
    head->SetTransform(CTransform4f::LookAt(position, position + offset, zColumn));
    return;
  }
  // 0x0000ADE0: the head's Y column is slerped toward the chassis's, but only when the counter
  // at 0xD00 is 5 and the angle between them is at least the turn rate times the frame time
  // (0x0000AE34 fcmpo, 0x0000AE38 bge); otherwise the head takes the chassis's transform.
  if (mLandedCount == 5) {
    const CVector3f headColumn = head->GetTransform().GetColumn(kDY);
    const CVector3f chassisColumn = locator.GetColumn(kDY);
    const float turn = 12.566371f * dt;
    if (CVector3f::GetAngleDiff(headColumn, chassisColumn) >= turn) {
      const CVector3f slerped =
          CVector3f::Slerp(headColumn, chassisColumn, CRelAngle::FromRadians(turn));
      head->SetTransform(CTransform4f::LookAt(position, position + slerped, zColumn));
      return;
    }
  }
  head->SetTransform(locator);
}

// fn_75_A4E4 (the chassis's collision actor manager, 0xA4E4-0xA8FC): the six joints are built
// from the retail's rodata tables - 0x30C gives the two spheres ("Skeleton_Root" with the radius
// 1.5f and "target_LCTR" with the radius 0.38f) and 0x31C the four leg boxes (the knee and the
// ankle of each leg and the extent 0x3F4CCCCD, 0.8f) - the mass is 1000.0f (the rodata at 0x3AC)
// and the manager is made with them (0x0000A754), the old one deleted (0x0000A764) and the new
// one kept at 0xC68. Each leg is an OBBAutoSizeCollision (0x0000A6A8) with the bounds
// (0.8f, 0.8f, 0.8f) (0x0000A68C-0x0000A6A0), the orientation 1 (kOT_BetweenJoints, 0x0000A698
// li r7, 1) and the knee's name. Then every collision actor of the manager is walked
// (0x0000A774): the actor named
// "target_LCTR" (the rodata string at 0x34C) gives its id to the chassis's 0xCC2 (0x0000A7E8)
// and its extended touch bounds (the model data's 0x0, 0x4 and 0x8 times the rodata's 1.5f,
// 0x0000A7F0-0x0000A810), and every actor gets the ricochet vulnerability (0x0000A814). The
// module's own call (fn_75_A37C, 0x0000A840) copies the health info and the material filter (the
// include mask with the bit 59 - kMT_Solid - and the exclude mask with the bits 47, 28 and 32 -
// kMT_CollisionActor, kMT_AIPassthrough and kMT_Player - and the type 3) is set at the end
// (0x0000A8D0). The four masks are the data words 0x3B, 0x2F, 0x1C and 0x20 shifted into the
// list (0x0000A858 and the three after it); the data word 0x3B is the include mask on its own
// (0x0000A8B4) and the other three the exclude mask.
// @ fn_75_A4E4
void CSplitterMainChassis::PortCreateCollisionActorManager(CStateManager& mgr) {
  const CAnimData* anim = GetModelData()->GetAnimationData();
  rstl::vector< CJointCollisionDescription > joints;
  joints.reserve(6); // fn_75_7B1C reserve (0x0000A51C), fn_75_3970 its dtor (0x0000A8DC)
  // fn_75_38B0: the REL's description copy (0x0000A5B0)
  // fn_75_3830: the REL's rstl::vector::push_back (0x0000A5D4)
  joints.push_back_unsafe(CJointCollisionDescription::SphereCollision(
      anim->GetLocatorSegId(rstl::string_l("Skeleton_Root")), CVector3f::Zero(), 1.5f,
      rstl::string_l("Skeleton_Root"), 1000.f));
  joints.push_back_unsafe(CJointCollisionDescription::SphereCollision(
      anim->GetLocatorSegId(rstl::string_l("target_LCTR")), CVector3f::Zero(), 0.38f,
      rstl::string_l("target_LCTR"), 1000.f));
  struct FLegJoint {
    const char* mKnee;
    const char* mAnkle;
  };
  static const FLegJoint sLegs[4] = {
    {"L_F_knee_2", "L_F_ankle"},
    {"L_B_knee_2", "L_B_ankle"},
    {"R_F_knee_2", "R_F_ankle"},
    {"R_B_knee_2", "R_B_ankle"},
  };
  for (int i = 0; i < 4; ++i) {
    joints.push_back_unsafe(CJointCollisionDescription::OBBAutoSizeCollision(
        anim->GetLocatorSegId(rstl::string_l(sLegs[i].mKnee)),
        anim->GetLocatorSegId(rstl::string_l(sLegs[i].mAnkle)), CVector3f(0.8f, 0.8f, 0.8f),
        CJointCollisionDescription::kOT_BetweenJoints, rstl::string_l(sLegs[i].mKnee), 1000.f));
  }
  CCollisionActorManager* manager =
      rs_new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), joints, true);
  delete mCollisionActorManager;
  mCollisionActorManager = manager;
  for (uint i = 0; i < mCollisionActorManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc = mCollisionActorManager->GetCollisionDescFromIndex(i);
    const TUniqueId id = desc.GetCollisionActorId();
    CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(id));
    // 0x0000A7A0: the retail's own null test of the cast (mr. r24, r3 - 0x0000A7A4 beq).
    if (actor == nullptr) {
      continue;
    }
    if (strcmp(desc.GetName().c_str(), "target_LCTR") == 0) { // fn_75_3678, string == (0x0000A7C4)
      mTargetActorId = id;
      // 0x0000A7EC-0x0000A810: the actor's extended touch bounds (its 0x35C) are the model
      // data's scale times 1.5f (lbl_75_rodata_388).
      actor->SetExtendedTouchBounds(GetModelData()->GetScale() * 1.5f);
    }
    // 0x0000A814: every collision actor, not only "target_LCTR", gets the DOL's
    // ImmuneRicochetVulnerability (the host's ReflectVulnerabilty).
    actor->SetDamageVulnerability(CDamageVulnerability::ReflectVulnerabilty());
  }
  PortCopyHealthToCollisionActors(mgr); // fn_75_A37C
  SetMaterialFilter(CMaterialFilter(CMaterialList(0x0800000000000000ull),
                                    CMaterialList(0x0000800110000000ull),
                                    CMaterialFilter::kFT_IncludeExclude));
}

// fn_75_A37C (the chassis's health copy, 0xA37C-0xA48C): every collision actor of the manager
// at 0xC68 (0x0000A3B0 GetCollisionDescFromIndex, to GetNumCollisionActors at 0x0000A464) is
// looked up and cast (0x0000A3C8, 0x0000A3CC); one that is there gets the chassis's health info
// (the vtable's 0x3C GetHealthInfo, copied by fn_75_A490 into its 0x38 HealthInfo,
// 0x0000A3D8-0x0000A458).
// @ fn_75_A37C
void CSplitterMainChassis::PortCopyHealthToCollisionActors(CStateManager& mgr) {
  for (uint i = 0; i < mCollisionActorManager->GetNumCollisionActors(); ++i) {
    const CJointCollisionDescription& desc = mCollisionActorManager->GetCollisionDescFromIndex(i);
    CCollisionActor* actor =
        TCastToPtr< CCollisionActor >(mgr.ObjectById(desc.GetCollisionActorId()));
    // 0x0000A3D0: the retail's own null test of the cast (mr. r30, r3 - 0x0000A3D4 beq).
    if (actor == nullptr) {
      continue;
    }
    *actor->HealthInfo() = *GetHealthInfo(); // fn_75_A490, CHealthInfo's copy (0x0000A3F4)
  }
}

// fn_75_E620 (the chassis's IsDisabled trigger, 0xE620-0xE62C): the flag byte at 0xD40 gets its
// 0x40 bit tested (0xE624 extrwi with the word's bit 25 - the byte's 0x40).
bool CSplitterMainChassis::IsDisabled(CStateManager&, const CTriggerData&) const {
  return (mFlagsD40 & 0x40) != 0;
}

// fn_75_E034 (the chassis's DeployFromHoldingTube trigger, 0xE034-0xE040): the data's flag word
// at 0x828 (the loader's unknown_0xcef5c2fe) is read (0xE034 lwz) and its second bit is kept
// (0xE038 extrwi with the word's bit 30 - the byte's 0x02); that bit is the answer.
bool CSplitterMainChassis::DeployFromHoldingTube(CStateManager&, const CTriggerData&) const {
  return (mData.mUnknownCEF5C2FE & 0x2) != 0;
}

// fn_75_E5B8 (the chassis's ShotAt trigger, 0xE5B8-0xE5D0): the base's flag byte at 0x420 gets
// its 0x80 bit tested (0xE5BC extrwi with the word's bit 24 - the byte's 0x80, the host's
// GetHitByPlayerProjectile) and the result is made a 0/1 value (0xE5C0 neg, 0xE5C4 or, 0xE5C8
// srwi).
bool CSplitterMainChassis::ShotAt(CStateManager&, const CTriggerData&) const {
  return GetHitByPlayerProjectile();
}

// fn_75_E54C (the chassis's HasTarget trigger, 0xE54C-0xE568): the target id at 0xCC0 against
// kInvalidUniqueId (0xE550-0xE564: subf both ways, or, srwi by 31 - true when they differ).
// @ fn_75_E54C
bool CSplitterMainChassis::HasTarget(CStateManager&, const CTriggerData&) const {
  return mSyncId != kInvalidUniqueId;
}

// fn_75_BE28 (the chassis's SelectTarget code, 0xBE28-0xBEE8): the player's id (the manager's
// 0x14FC, its 0x8) becomes the target id at 0xCC0 (0xBE48-0xBE54) and goes to the line-of-sight
// tracker at 0xC70 (0xBE60 fn_801F585C, the host's SetTarget). With a head (the command module id
// at 0xC6C valid, 0xBE64-0xBE74) the alerted sound (the data's 0x8A0) starts as an emitter at the
// chassis's position, volume 0x7F, its area, acoustics on, not looped, medium priority
// (0xBE78-0xBEA0). The four timers are copied (0xBEA8 fn_75_8EA4) and the chassis sends its
// "ATTK" state with its own id and no message (0xBEAC-0xBED0, li r7 -1).
// fn_75_BE28 (not marked: the headless arm, no AddEmitter, never ran)
void CSplitterMainChassis::SelectTarget(CStateManager& mgr, float) {
  mSyncId = mgr.GetPlayer(0)->GetUniqueId();
  mVisualKnowledge.SetTarget(mSyncId); // fn_801F585C
  if (mCommandModuleId != kInvalidUniqueId) {
    CSfxManager::AddEmitter(static_cast< ushort >(mData.mSoundAlerted & 0xFFFF), GetTranslation(),
                            0x7F, GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
  }
  PortCopyTimers(); // fn_75_8EA4
  SendScriptMsgs(kSS_Attack, mgr, GetUniqueId(), kSM_None);
}

// fn_75_E478 (the chassis's PatrolPathOver trigger, 0xE478-0xE494): the id at 0x6C0 (the base's
// waypoint navigation destination) is compared with kInvalidUniqueId (0xE47C lhz, 0xE480 lhz,
// 0xE484 subf, 0xE488 cntlzw, 0xE48C srwi by 5): the trigger is true when the destination is
// invalid, that is when the patrol path is over.
bool CSplitterMainChassis::PatrolPathOver(CStateManager&, const CTriggerData&) const {
  return mWaypointNavigation.GetDestination() == kInvalidUniqueId;
}

// fn_75_E5AC (the chassis's LostHead trigger, 0xE5AC-0xE5B8): the flag byte at 0xD41 gets its
// second bit tested (0xE5B0 extrwi with the word's bit 30 - the byte's 0x02).
bool CSplitterMainChassis::LostHead(CStateManager&, const CTriggerData&) const {
  return (mFlagsD41 & 0x2) != 0;
}

// fn_75_E5D0 (the chassis's ShouldWaitForSnatch trigger, 0xE5D0-0xE620): the id at 0xCBC (the
// snatching swarm's id, which the retail's fn_75_E810 keeps) is compared with kInvalidUniqueId
// (0xE5E8 lhz, 0xE5F0 cmplw) and, when it is invalid, the base's IsBeingSnatched (0xE5F8) decides
// (0xE5FC clrlwi with the word's bit 24 - the byte's 0xFF): the trigger is true when either of
// the two says so.
bool CSplitterMainChassis::ShouldWaitForSnatch(CStateManager&, const CTriggerData&) const {
  return mSnatcherId != kInvalidUniqueId || IsBeingSnatched();
}

// fn_75_DFEC (the chassis's PrepareForDocking trigger, 0xDFEC-0xE00C): the id at 0xC6E (the
// second link) is compared with kInvalidUniqueId (0xDFF0 lhz, 0xDFF4 lhz, 0xDFF8 subf, 0xDFFC
// subf, 0xE000 or, 0xE004 srwi): the trigger is true when the id is valid.
bool CSplitterMainChassis::PrepareForDocking(CStateManager&, const CTriggerData&) const {
  return mSecondLinkId != kInvalidUniqueId;
}


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
  // fn_75_F5F8 @0x0000F93C-0x0000F964: guest 0xD0C is the walk speed (anim 1, 0x0000F948 li r4, 1)
  // divided by the run speed. The first call's anim register is not set in the dump, so the run
  // speed is the reading; the host's GetMaxSpeed() is that speed when the model has one and the
  // largest locomotion speed otherwise.
  const CBodyStateInfo& bodyInfo = BodyController()->GetBodyStateInfo();
  mSteeringSpeed = bodyInfo.GetLocomotionSpeed(pas::kLA_Walk) / bodyInfo.GetMaxSpeed();
  // fn_75_F5F8 @0x0000F970-0x0000F98C: guest 0xCC8 is the segment of the locator "attach_LCTR"
  // (the rodata string at 0x3B7) - fn_8002BE04 is CAnimData::GetLocatorSegId on the model's
  // animation data. The 0xFF the ctor stores first (0x0000F780) is overwritten here.
  mLocatorSegment = GetModelData()->GetAnimationData()->GetLocatorSegId(rstl::string_l("attach_LCTR"));
  // fn_75_F5F8 @0x0000FA70-0x0000FB08: guest 0xD38 is the step distance - the step animation
  // (kAS_Step with the parameters 3 and 1, fn_75_6F1C builds the CPASAnimParmData) times the
  // model's x scale (the model data at 0x60).
  const CPASAnimParmData stepParms(pas::kAS_Step, CPASAnimParm::FromEnum(3),
                                   CPASAnimParm::FromEnum(1));
  mStepDistance = GetModelData()->GetScale().GetX() * GetAnimationDistance(stepParms);
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

// fn_75_8EA4 (not marked: the run never reached the copy)
// The chassis copies four of its data's timers into its derived state (0x8EA4-0x8EC8): the leg
// stab interval (0x82C) to 0xD10, the spin attack interval penalty (0x874) to 0xD14, the laser
// sweep interval (0x858) to 0xD18 and the min dodge interval (0x848) to 0xD1C.
void CSplitterMainChassis::PortCopyTimers() {
  mLegStabTimer = mData.mLegStabAttackInterval;
  mSpinPenaltyTimer = mData.mSpinAttackIntervalPenalty;
  mLaserSweepTimer = mData.mLaserSweepInterval;
  mDodgeTimer = mData.mMinDodgeInterval;
}

// fn_75_E6E4 (not marked: the run never reached the accept)
// The chassis accepts a head (0xE6E4-0xE768): with the chassis's own command module id (0xC6C)
// still the invalid one (0xE700-0xE70C) the head's id goes there (0xE720), the id at 0xC6E is
// cleared (0xE72C), the flag byte 0xD40's 0x40 bit is cleared (0xE730-0xE738, the word's bit 25),
// the materials 0x28 and 0x29 (kMT_Target and kMT_Orbit) are removed (0xE73C) and the four
// timers are copied (0xE744 fn_75_8EA4); the answer is true (0xE748). With a command module
// already there the answer is false (0xE750).
bool CSplitterMainChassis::PortAcceptHead(CStateManager& mgr, TUniqueId headId) {
  if (mCommandModuleId != kInvalidUniqueId) {
    return false;
  }
  mCommandModuleId = headId;
  mSecondLinkId = kInvalidUniqueId;
  mFlagsD40 &= ~static_cast< uchar >(0x40);
  RemoveMaterial(kMT_Target, kMT_Orbit, mgr);
  PortCopyTimers();
  return true;
}

// fn_75_F0E8 (0xB8) - the main chassis's Think. The body follows the retail's: the active gate
// (the byte at 0x20, 0xF10C lbz and 0xF110 extrwi bit 24 - the byte's 0x80), the base's Think
// (0xF118), the docking link (the partner id at 0xC68, 0xF11C lwz and 0xF120 cmplwi 0: with no
// partner the retail looks for one, 0xF130 fn_75_A4E4, otherwise it updates the dock, 0xF144
// fn_80135D84), the line-of-sight tracker (the tracker at 0xC70, 0xF154 fn_801F55F0) and the
// module's own three calls (0xF164 fn_75_AFE8, 0xF174 fn_75_ACEC, 0xF180 fn_75_9520). The first
// and the last are ported (PortUpdateTimers, PortUpdateFade); the middle one is over 0x80 and
// stays a once-logging shell, so the function carries no port marker yet.
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
    PortCreateCollisionActorManager(mgr);
  } else {
    mCollisionActorManager->Update(dt, mgr, CCollisionActorManager::kUO_ObjectSpace);
  }
  mVisualKnowledge.Update(dt, mgr);
  PortUpdateTimers(mgr, dt);
  PortPlaceHead(mgr, dt);
  PortUpdateFade(dt);
}

// fn_75_AFE8 (not marked: the area-4 run never reached the death branch, and the D3C-up
// branch needs the sync id at 0xCC0 and the counter at 0xC48 - see the module note)
// The chassis's timers (0xAFE8-0xB104), the Think's call at 0x0000F164 (the manager, the frame
// time in f1 and the state manager in r4). The four timers the chassis copies from its data
// (fn_75_8EA4) are counted down (0x0000AFF8 0xD10, 0x0000B004 0xD14, 0x0000B010 0xD18,
// 0x0000B01C 0xD1C), the scan timer at 0xD28 too (0x0000B028), and three timers are counted up
// (0x0000B034 0xD34, 0x0000B040 0xD30, 0x0000B04C 0xD2C). The timer at 0xD3C is counted up
// only while the id at 0xCC0 is valid (0x0000B058 lhz against kInvalidUniqueId) and the counter
// at 0xC48 is not zero (0x0000B068 lwz, 0x0000B06C cmpwi); otherwise it is cleared (0x0000B084,
// lbl_75_rodata_390, 0.0f). The flag byte at 0xD41's 0x01 bit (0x0000B094 clrlwi with the word's
// bit 31 - the byte's 0x01) and the alive bit of the byte at 0x420 (0x0000B0A0 extrwi with the
// word's bit 25 - the byte's 0x40) gate the death: the timer at 0xD20 is counted down
// (0x0000B0B0) and, at zero or below (0x0000B0C0 fcmpo with the cror eq, lt, eq - the <= sense),
// the chassis dies with the transform's Y column (the vector at 0x28, 0x38 and 0x48) and the
// state 'RATL' (0x0000B0E0).
void CSplitterMainChassis::PortUpdateTimers(CStateManager& mgr, float dt) {
  mLegStabTimer -= dt;     // 0xD10 (0x0000AFF8)
  mSpinPenaltyTimer -= dt; // 0xD14 (0x0000B004)
  mLaserSweepTimer -= dt;  // 0xD18 (0x0000B010)
  mDodgeTimer -= dt;       // 0xD1C (0x0000B01C)
  mScanTimer -= dt;        // 0xD28 (0x0000B028)
  mTimerD34 += dt;         // 0xD34 (0x0000B034)
  mTimerD30 += dt;         // 0xD30 (0x0000B040)
  mSpinTimer += dt;        // 0xD2C (0x0000B04C)
  if (mSyncId != kInvalidUniqueId && mIntC48 != 0) { // 0xCC0 (0x0000B058), 0xC48 (0x0000B068)
    mTimerD3C += dt;                                   // 0xD3C (0x0000B074)
  } else {
    mTimerD3C = 0.f;                                   // 0xD3C (0x0000B084, lbl_75_rodata_390)
  }
  if ((mFlagsD41 & 0x1) != 0 && GetAlive()) {          // 0xD41 (0x0000B094), 0x420 (0x0000B0A0)
    mTimerD20 -= dt;                                   // 0xD20 (0x0000B0B0)
    if (mTimerD20 <= 0.f) {                            // 0x0000B0C0 (the <= sense)
      // 0x0000B0F0: the retail calls the base class's Death (not the module's override).
      CPatterned::Death(mgr, GetTransform().GetColumn(kDY), kSS_DeathRattle);
    }
  }
}

// fn_75_9520 (not marked: the area-4 run never reached the fade-down branch, which needs the
// scan timer at 0xD28 above zero - see the module note)
// The chassis's fade (0x9520-0x963C), the Think's call at 0x0000F180. The id at 0xCC0
// (0x00009540 lhz) picks the rate: a valid id gives lbl_75_rodata_39C (0.25f) (0x00009558) and
// the invalid one lbl_75_rodata_364 (1.0f) (0x00009564), and the step is the frame time divided
// by it (0x0000956C fdivs). While the scan timer at 0xD28 is above zero (0x0000957C fcmpo, the
// greater-than branch) or the id is valid, the weight at 0xCB8 is counted down and clamped at
// zero (0x0000958C-0x000095AC); otherwise it is counted up and clamped at one
// (0x000095B4-0x000095D4). The weight above zero adds the additive animation with the id at
// 0xCB4 (0x000095EC, 0x000095FC with the two flags clear); at zero or below the animation is
// deleted when it is active (0x00009604, 0x0000960C, 0x00009610 clrlwi with the word's bit 24 -
// the byte's 0xFF).
void CSplitterMainChassis::PortUpdateFade(float dt) {
  const float step = dt / (mSyncId != kInvalidUniqueId ? 0.25f : 1.f); // 0x00009558-0x0000956C
  if (mScanTimer > 0.f || mSyncId != kInvalidUniqueId) { // 0x0000957C, 0x00009584
    mFadeWeight -= step;                                 // 0xCB8 (0x0000958C)
    if (mFadeWeight <= 0.f) {                            // 0x0000959C (the <= sense)
      mFadeWeight = 0.f;                                 // 0x000095A8
    }
  } else {
    mFadeWeight += step;                                 // 0xCB8 (0x000095B4)
    if (mFadeWeight >= 1.f) {                            // 0x000095C4 (the >= sense)
      mFadeWeight = 1.f;                                 // 0x000095D0
    }
  }
  if (mFadeWeight > 0.f) {                               // 0x000095E4 (the greater-than sense)
    AnimationData()->AddAdditiveAnimation(mAdditiveAnimId, mFadeWeight, false, false);
    return;                                              // 0x00009600
  }
  if (AnimationData()->IsAdditiveAnimationActive(mAdditiveAnimId)) { // 0x0000960C, 0x00009610
    AnimationData()->DelAdditiveAnimation(mAdditiveAnimId);          // 0x00009618
  }
}

// @ fn_75_AC68
// The chassis's connected-object lookups (0xAC68-0xADEC): the object connected with the state
// "CONN" (0xAC7C) and the message "ATCH" (0xAC80) is checked (0xACA0 CheckConnectedObject) and
// its id is kept at 0xCC4 (0xACB4); then the object connected with the state "RTRT"
// (kSS_Retreat, 0xACC0) and the message "FOLW" (kSM_Follow, 0xACC4) is fetched (0xACC8
// GetConnectedObject) and its id is kept at 0xCC6 (0xACD0).
void CSplitterMainChassis::PortUpdateConnectedObjects(CStateManager& mgr) {
  mConnectedObjectId = CheckConnectedObject(mgr, kSS_Connect, kSM_Attach);
  mFollowObjectId = GetConnectedObject(mgr, kSS_Retreat, kSM_Follow);
}

// @ fn_75_8D50
// The chassis's team exit (0x8D50-0x8DF8): the team id at 0xCBE is looked up (0x8D90
// ObjectById) and cast to a CScriptTeamAiMgr (0x8D94); when the cast fails (0x8D9C) or the
// actor is not in that team (0x8DB0 IsPartOfTeam, 0x8DB4) nothing happens, else the actor
// leaves the team (0x8DD0 QuitTeam) and the id is cleared (0x8DDC).
void CSplitterMainChassis::PortLeaveTeam(CStateManager& mgr) {
  if (mTeamId == kInvalidUniqueId) {
    return;
  }
  CScriptTeamAiMgr* team = TCastToPtr< CScriptTeamAiMgr >(mgr.ObjectById(mTeamId));
  if (team == nullptr || !team->IsPartOfTeam(GetUniqueId())) {
    return;
  }
  team->QuitTeam(GetUniqueId());
  mTeamId = kInvalidUniqueId;
}

// @ fn_75_AAC4
// The chassis's player-hint release (0xAAC4-0xAB8C): the hint's id at 0xCC4 is looked up
// (0xAAF0 ObjectById) and cast to a CScriptPlayerHint (0xAAF4); when the cast fails (0xAAF8)
// or the hint's actor id (0x1AC) is not this actor's id (0xAB00-0xAB0C) nothing happens, else
// the hint's actor id is cleared (0xAB28) and the hint gets a "DECR" message (0xAB70
// DeliverScriptMsg: this actor is the sender, the player the originator, the hint the id).
void CSplitterMainChassis::PortReleasePlayerHint(CStateManager& mgr) {
  CScriptPlayerHint* hint = TCastToPtr< CScriptPlayerHint >(mgr.ObjectById(mConnectedObjectId));
  if (hint == nullptr || hint->GetActorId() != GetUniqueId()) {
    return;
  }
  hint->SetActorId(kInvalidUniqueId);
  const CPlayer* player = mgr.GetPlayer(0);
  mgr.DeliverScriptMsg(CScriptMsg(GetUniqueId(), hint->GetUniqueId(), kSM_Decrement,
                                  player != nullptr ? player->GetUniqueId() : kInvalidUniqueId,
                                  kSS_InvalidState));
}

// fn_75_AB8C (0xAB8C-0xAC6C): the chassis's player-hint grant. The hint at 0xCC4 (0xABB8
// ObjectById) cast to a CScriptPlayerHint (0xABBC-0xABC4) that is not already held by this actor
// (its 0x1AC; 0xABC8-0xABD4) is taken while the player (the manager's 0x14FC) is unmorphed or
// morphed (its 0x38C in 0..1; 0xABDC-0xABF0): the hint's actor id becomes this actor's (0xABF4)
// and the hint gets an "INCR" message (0xAC4C DeliverScriptMsg: this actor the sender, the player
// the originator).
void CSplitterMainChassis::PortTakePlayerHint(CStateManager& mgr) {
  CScriptPlayerHint* hint = TCastToPtr< CScriptPlayerHint >(mgr.ObjectById(mConnectedObjectId));
  if (hint == nullptr || hint->GetActorId() == GetUniqueId()) {  // 0xABC0-0xABD4
    return;
  }
  const CPlayer* player = mgr.GetPlayer(0);
  const CPlayer::EPlayerMorphBallState morph = player->GetMorphballTransitionState();
  if (morph >= CPlayer::kMS_Morphing || morph < CPlayer::kMS_Unmorphed) {  // 0xABE0-0xABF0
    return;
  }
  hint->SetActorId(GetUniqueId());
  mgr.DeliverScriptMsg(CScriptMsg(GetUniqueId(), hint->GetUniqueId(), kSM_Increment,
                                  player->GetUniqueId(), kSS_InvalidState));
}

// The inline both damage handlers use (0x9C2C-0x9C58, 0xA02C-0xA058): a player that is not in
// its spawn morph state (0x390 zero) and is morphed (0x38C == 1) with the boost shield on
// (CMorphBall::IsBoostShieldActive on its 0x1174).
static bool PortIsBoosting(const CPlayer& player) {
  const CPlayer::EPlayerMorphBallState morph =
      player.GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed
          ? player.GetMorphballTransitionState()
          : CPlayer::kMS_Unmorphed;
  return morph == CPlayer::kMS_Morphed && player.GetMorphBall()->IsBoostShieldActive();
}

// fn_75_9FA8 (0x9FA8-0xA0A0, the resisted/reflected damage message): only a live chassis
// (0x420's 0x40 bit; 0x9FC4-0x9FCC) reacts. While 0xD41's 0x80 bit is set (0x9FD0-0x9FD8), a
// sender that is a collision actor (0x9FEC-0x9FF8) whose last touched object (0xA004
// CCollisionActor::GetLastTouchedObject) is a boosting player (0xA020-0xA058) advances the spin
// counter at 0xCFC by the data's 0x888 (0xA05C-0xA06C) and sets 0xD40's 0x2 bit (0xA070-0xA078).
// Then the player hint is taken (0xA084 fn_75_AB8C).
void CSplitterMainChassis::PortResistedDamage(CStateManager& mgr, TUniqueId sender) {
  if (!mAlive) {  // 0x9FC8-0x9FCC
    return;
  }
  if ((mFlagsD41 & 0x80) != 0) {  // 0x9FD0-0x9FD8
    if (const CCollisionActor* actor = TCastToConstPtr< CCollisionActor >(mgr.ObjectById(sender))) {
      const CPlayer* player =
          TCastToConstPtr< CPlayer >(mgr.GetObjectById(actor->GetLastTouchedObject()));
      if (player != nullptr && PortIsBoosting(*player)) {  // 0xA024-0xA028
        mSpinCounter += mData.mUnknownD5F34476;
        mFlagsD40 |= 0x2;
      }
    }
  }
  PortTakePlayerHint(mgr);
}

// fn_75_9B9C (0x9B9C-0x9FA8, the hit-object message): a live chassis (0x9BC4-0x9BCC) without
// 0xD40's 0x40 bit (0x9BD0-0x9BD8) looks at the sender as a collision actor (0x9BEC-0x9BF8) and
// its last touched object (0x9C04 GetLastTouchedObject).
// - A player (0x9C1C-0x9C28): boosting (0x9C2C-0x9C58), when the sender is the "target_LCTR"
//   actor at 0xCC2 (0x9E30-0x9E3C) the morph ball takes the tweak's boost ball damage
//   (0x9E50 GetBoostBallDamage, 0x9EBC ApplyBoostBallDamage with 0.0f, lbl_75_rodata_390).
//   Otherwise, without 0xD40's 0x2 bit (0x9C5C-0x9C64), the damage is the contact damage
//   (the vtable word 0xE8, 0x9C7C), the leg stab damage (data 0x7C0) with 0xD40's 0x1 bit
//   (0x9CC8-0x9D1C) or else the spin attack damage (data 0x7DC) with 0xD41's 0x80 bit, which also
//   sets 0xD40's 0x8 bit (0x9D20-0x9D7C); once the damage wait at 0x440 has run out (<= 0.0f,
//   0x9D80-0x9D94) the player takes it (0x9E20 ApplyDamage: this actor, the player, this actor,
//   the filter including kMT_Solid (lbl_75_data_C9C, 0x3B), the translation) and the wait restarts
//   from 0x444 (0x9E24-0x9E28). Either way the player hint is taken (0x9EC8 fn_75_AB8C).
// - Anything else, while 0xD41's 0x80 bit is set (0x9ED0-0x9ED8) and the object is not a CAi
//   (0x9EEC-0x9EF8), takes the spin attack damage the same way (lbl_75_data_CA0, 0x3B; 0x9F84),
//   without the wait.
void CSplitterMainChassis::PortHitObject(CStateManager& mgr, TUniqueId sender) {
  if (!mAlive || (mFlagsD40 & 0x40) != 0) {  // 0x9BC8-0x9BD8
    return;
  }
  const CCollisionActor* actor = TCastToConstPtr< CCollisionActor >(mgr.GetObjectById(sender));
  if (actor == nullptr) {  // 0x9BF4-0x9BF8
    return;
  }
  const TUniqueId touched = actor->GetLastTouchedObject();
  const CMaterialFilter filter =
      CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList());
  if (CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(touched))) {  // 0x9C24-0x9C28
    if (PortIsBoosting(*player)) {
      if (sender == mTargetActorId) {
        player->GetMorphBall()->ApplyBoostBallDamage(mgr, mTargetActorId,
                                                     gpTweakBall->GetBoostBallDamage(), 0.f);
      }
    } else if ((mFlagsD40 & 0x2) == 0) {  // 0x9C5C-0x9C64
      CDamageInfo damage = GetContactDamage();
      if ((mFlagsD40 & 0x1) != 0) {  // 0x9CC8-0x9CD0
        damage = mData.mLegStabDamage;
      } else if ((mFlagsD41 & 0x80) != 0) {  // 0x9D20-0x9D28
        damage = mData.mSpinAttackDamage;
        mFlagsD40 |= 0x8;
      }
      if (mCurDamageRemTime <= 0.f) {
        mgr.ApplyDamage(GetUniqueId(), touched, GetUniqueId(), damage, filter, GetTranslation());
        mCurDamageRemTime = mDamageWaitTime;
      }
    }
    PortTakePlayerHint(mgr);
  } else if ((mFlagsD41 & 0x80) != 0) {  // 0x9ED0-0x9ED8
    // 0x9EF0 TCastToPtr<CAi>: the host has no such cast, so the type test is spelled out.
    const CEntity* object = mgr.GetObjectById(touched);
    if (object != nullptr && object->TypesMatch(kET_Ai) != nullptr) {  // 0x9EF4-0x9EF8
      return;
    }
    mgr.ApplyDamage(GetUniqueId(), touched, GetUniqueId(), mData.mSpinAttackDamage, filter,
                    GetTranslation());
  }
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
    PortReleasePlayerHint(mgr);
    PortLeaveTeam(mgr);
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
    PortLeaveTeam(mgr);
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
    // 0x0000F434-0x0000F464: the path area of the chassis's current area (the world at the
    // manager's 0x1604, its area list at 0x24 indexed by the area id at 0x4, the area's
    // post-constructed data at 0x104 and its path area at 0xEC) goes into the path search at
    // 0xB7C - without it every Search fails and PathFind never moves the chassis. Then the
    // module's own call runs (0x0000F468 fn_75_AC68).
    mPathFindSearch.SetArea(
        mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()).GetPostConstructed()->mPathArea);
    PortUpdateConnectedObjects(mgr);
    break;
  }
  case kSM_Damage: { // "XCMG" (0x0000F2DC)
    // 0x0000F470-0x0000F498: the module's own call (0x0000F488 fn_75_A0A0) and the base's flag
    // byte at 0x420 gets its 0x80 bit set.
    PortCollisionActorDamage(mgr, msg.GetSenderId());
    SetHitByPlayerProjectile(true);
    break;
  }
  case kSM_ResistedDamage:   // "XRDG" (0x0000F2EC)
  case kSM_ReflectedDamage: { // "XXDG" (0x0000F314)
    // 0x0000F4A0-0x0000F4C8: the module's own call (0x0000F4B8 fn_75_9FA8) and the base's flag
    // byte at 0x420 gets its 0x80 bit set.
    PortResistedDamage(mgr, msg.GetSenderId());
    SetHitByPlayerProjectile(true);
    break;
  }
  case kSM_HitObject: { // "XHIT" (0x0000F2A0)
    // 0x0000F4D0-0x0000F4E8: the module's own call (0x0000F4E8 fn_75_9B9C).
    PortHitObject(mgr, msg.GetSenderId());
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

// fn_75_A0A0 (0xA0A0-0xA37C): a collision actor reports damage. Only for a living chassis (the
// base's 0x420 0x40 bit, 0xA0C8-0xA0D8). The sender cast to a CCollisionActor (0xA0F0-0xA0F8)
// carries a copy of the chassis's health (fn_75_A37C), so the hit is the chassis's initial health
// less the actor's (0xA180): the chassis loses it (0xA28C). The hit's direction is the chassis's
// own translation and its damage a boost ball hit of that much (CWeaponMode 7, 0xA140-0xA1FC),
// unless the object the actor last touched (0xA104 GetLastTouchedObject) is a CWeapon (0xA208):
// then its translation, owner (0x15C) and current damage info (0x19C-0x1B6). The chassis takes the
// hit (0xA29C, vtable 0xA4 TakeDamage) and with its health at or below the hit (0xA2A0-0xA2A8)
// dies (0xA2CC, vtable 0x98 Death with kSS_DeathRattle) and records the actor as the cause
// (0xA2F4 UpdateHealthCause); it is knocked back (0xA320-0xA33C) and the actor's health is
// re-armed (0xA340). With or without the actor the hint grant follows (0xA34C fn_75_AB8C).
void CSplitterMainChassis::PortCollisionActorDamage(CStateManager& mgr, TUniqueId sender) {
  if (!mAlive) {  // 0xA0D4-0xA0D8
    return;
  }
  CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(sender));
  if (actor != nullptr) {  // 0xA0F4-0xA0F8
    const TUniqueId touched = actor->GetLastTouchedObject();
    CHealthInfo* actorHealth = actor->HealthInfo();
    CHealthInfo* health = HealthInfo();
    const float initial = health->GetInitialHP();
    const float hp = health->GetHP();
    const float amount = initial - actorHealth->GetHP();
    CVector3f direction = GetTranslation();
    TUniqueId attacker = kInvalidUniqueId;
    CDamageInfo damage(CWeaponMode(kWT_BoostBall), amount, 0.f, 0.f, false, false);
    if (const CWeapon* weapon = TCastToConstPtr< CWeapon >(mgr.GetObjectById(touched))) {
      direction = weapon->GetTranslation();
      attacker = weapon->GetOwnerId();
      damage = weapon->GetCurrentDamageInfo();
    }
    health->SetHP(hp - amount);
    TakeDamage(direction, amount);
    if (hp <= amount) {  // 0xA2A0-0xA2A8
      Death(mgr, direction, kSS_DeathRattle);
      mgr.RecordDamageSource(*this, actor->GetUniqueId(), damage, true, false);
    }
    KnockBack(mgr, CKnockBackInfo(direction, touched, attacker, damage, true));
    actorHealth->SetHP(initial);
  }
  PortTakePlayerHint(mgr);
}

// fn_75_BEEC (the chassis's HeadExplosion state, 0xBEEC-0xC034). Activate (0xBF24): the animation
// state is kAS_Ready and the flag byte 0xD41's 0x02 bit is cleared (0xBF34 rlwimi). Update
// (0xBF40): when CTryCommand::ShouldRetry (the host's CAnimationState::CanIssueCommand) allows
// body state 6 (kAS_KnockBack, 0xBF48-0xBF54) a knock-back command along the chassis's Y column
// (the transform's 0x28, 0x38, 0x48) with severity 8 goes out (lbl_75_data_5B8, 0xBF58-0xBFB8).
// Deactivate (0xC010): the animation state is kAS_NotReady and the spin penalty timer at 0xD14
// restarts with the data's spin attack interval penalty (0x874, 0xC018-0xC01C).
// @ fn_75_BEEC
void CSplitterMainChassis::HeadExplosion(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    mFlagsD41 &= ~0x02;
  } else if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_KnockBack)) {
      BodyController()->CommandMgr().DeliverCmd(
          CBCKnockBackCmd(GetTransform().GetColumn(kDY), pas::kS_Eight));
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mSpinPenaltyTimer = mData.mSpinAttackIntervalPenalty;
  }
}

// fn_75_E788 (0xE788-0xE810): a living (the base's 0x420 0x40 bit, 0xE788-0xE790), active (the
// entity's 0x20 0x80 bit, 0xE794-0xE79C) chassis with no head (0xC6C, 0xE7A4-0xE7B4) and no
// reservation (0xC6E, 0xE7B8-0xE7C0), landed (0xD00 at 5 or 0, 0xE7C4-0xE7D4), with 0xD41's 0x80
// and 0xD40's 0x01 bits clear (0xE7D8-0xE7EC) takes the head's reservation (0xE7F4); the result
// is whether the reservation is that head's (0xE7F8-0xE808).
bool CSplitterMainChassis::PortReserveDock(TUniqueId head) {
  if (mAlive && GetActive() && mCommandModuleId == kInvalidUniqueId &&
      mSecondLinkId == kInvalidUniqueId && (mLandedCount == 5 || mLandedCount == 0) &&
      (mFlagsD41 & 0x80) == 0 && (mFlagsD40 & 0x1) == 0) {  // 0xE788-0xE7EC
    mSecondLinkId = head;
  }
  return mSecondLinkId == head;
}

// fn_75_E768 (0xE768-0xE788): the reservation at 0xC6E is cleared when it is that head's.
void CSplitterMainChassis::PortReleaseDock(TUniqueId head) {
  if (mSecondLinkId == head) {
    mSecondLinkId = kInvalidUniqueId;
  }
}

// vtable slot 0x98 = fn_75_EB04 (0xEB04-0xEBF4): the base's Death (0xEB28), then every
// collision actor of the manager at 0xC68 (0xEB3C GetCollisionDescFromIndex, to
// GetNumCollisionActors at 0xEB80) that casts to a CCollisionActor takes the chassis's material
// filter (0xEB68-0xEB74); the chassis drops kMT_Target and kMT_Orbit (0xEB9C RemoveMaterial
// 0x28, 0x29) and, with its head there (0xEBB0-0xEBBC), resets the head's shield (0xEBC4
// fn_75_5D50) and lets the head go (0xEBD0: the id at 0xC6C cleared). The head's chassis check
// (fn_75_31A8) then sees the chassis no longer holds it and undocks.
void CSplitterMainChassis::Death(CStateManager& mgr, const CVector3f& direction,
                                  EScriptObjectState state) {
  CPatterned::Death(mgr, direction, state);
  for (uint i = 0; i < mCollisionActorManager->GetNumCollisionActors(); ++i) {
    const TUniqueId id = mCollisionActorManager->GetCollisionDescFromIndex(i).GetCollisionActorId();
    CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(id));
    if (actor == nullptr) {  // 0xEB5C-0xEB60
      continue;
    }
    actor->SetMaterialFilter(GetMaterialFilter());
  }
  RemoveMaterial(kMT_Target, kMT_Orbit, mgr);
  CSplitterCommandModule* head =
      TCastToPtr< CSplitterCommandModule >(mgr.ObjectById(mCommandModuleId));
  // The head-null return is not reached in play: the chassis's hp only drops through A0A0 on
  // target_LCTR, which is vulnerable only inside LaserSweepAttack (fn_75_D3E0 Activate); a dead
  // head is deleted by its Dead state (fn_75_5448, 0x547C DeathDelete), then PortPlaceHead
  // clears 0xC6C (0xAD50), LaserSweepAttack exits without a head (0xD510-0xD548) and its
  // Deactivate makes every collision actor immune again (0xD59C-0xD5B8), and a headless PathFind
  // leaves for SetRetreatDest through !HasHead (fn_75_E58C), never LaserSweepAttack.
  if (head == nullptr) {  // 0xEBB8-0xEBBC
    return;
  }
  head->PortResetShield(mgr);
  mCommandModuleId = kInvalidUniqueId;
}

// vtable slot 0x9C = fn_75_EBF4.
void CSplitterMainChassis::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  StandIn("CSplitterMainChassis::KnockBack (vtable slot 0x9C, fn_75_EBF4)");
  CPatterned::KnockBack(mgr, info);
}

// vtable slot 0xB0 = fn_75_EA70 (0xEA70-0xEB04): only a live chassis (0x420's 0x40 bit,
// mAlive; 0xEA70-0xEA7C) hears anything. A path obstruction (type 4, 0xEA80-0xEA84) closer than
// 20 units (squared against 400.0f, lbl_75_rodata_3CC; 0xEA98-0xEAD4) sets 0xD40's 0x10 bit
// (0xEAD8-0xEAE4) and is heard; the player's fire (type 0, 0xEA8C-0xEA90) clears the timer at
// 0xD34 (0xEAEC-0xEAF8, the time since the last shot that FindBestDodgeDirection tests) and is
// heard. The base's Listen is not called.
bool CSplitterMainChassis::Listen(CStateManager&, const CVector3f& position,
                                   EListenNoiseType type) {
  if (!mAlive) {  // 0xEA78-0xEA7C
    return false;
  }
  if (type == kLNT_PathObstruction) {
    if ((position - GetTranslation()).MagSquared() < 400.f) {
      mFlagsD40 |= 0x10;
      return true;
    }
  } else if (type == kLNT_PlayerFire) {
    mTimerD34 = 0.f;
    return true;
  }
  return false;
}

// vtable slot 0xF0 = fn_75_E9E4: it registers the module's three tables (28 triggers, 22 states
// and 11 codes) on the state machine. That is milestone 2, so only the base setup runs here.
// @ fn_75_E9E4
// The chassis's state machine (0xE9E4-0xEA70): the base's setup (0xE9F8) and then the module's
// own tables - the 28 triggers (lbl_75_data_720, 0xEA0C), the 22 states (lbl_75_data_9E8, 0xEA2C)
// and the 11 codes (lbl_75_data_BCC, 0xEA4C).
void CSplitterMainChassis::SetupStateMachine(CStateManager& mgr) {
  CPatterned::SetupStateMachine(mgr);
  mStateMachine->SetTriggerFunctions(skTriggers, ARRAY_SIZE(skTriggers));
  mStateMachine->SetStateFunctions(skStates, ARRAY_SIZE(skStates));
  mStateMachine->SetCodeFunctions(skCodeFuncs, ARRAY_SIZE(skCodeFuncs));
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
