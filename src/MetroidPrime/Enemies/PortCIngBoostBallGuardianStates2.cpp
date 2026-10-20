// The Boost Guardian's Ing half of the state machine: the states the Ing REL's own machine holds,
// ported from the Ing host body (branch decomp/ing, src/MetroidPrime/Enemies/PortCIng.cpp). The
// module's own tables - the 49 triggers, the 28 states and the 9 code functions of fn_30_C6CC - are
// in PortCIngBoostBallGuardianFsm.cpp.
//
// Every fn_29_ address in the comments below is the ING REL's address: this module
// (IngBoostBallGuardian) carries its own copy of the Ing code, so the behaviour is the Ing's and
// only the state names are shared. The module's own addresses are in
// docs/module-analysis/BoostGuardian_fsm.tsv. The bodies are ported for behaviour, not bytes.
//
// The Ing members this module does not have (include/MetroidPrime/Enemies/CIngBoostBallGuardian.hpp
// has no counterpart), and what was dropped for them:
//   - the lure safe zone (the Ing's mLureSafeZoneId) and the safe-zone count (mSafeZoneCount):
//     PathFind's "lured" speed and its hold test lose those two terms.
//   - the safe-zone ing-spot flag (the Ing's mSafeZoneIngSpot): the ing-spot states move at the
//     ing spot's maximum speed.
//   - the grapple block (mData.mGrapple) and the grapple's state: this module's machine has no
//     such node, so GrappleBall is not here.
//   - the Ing's 0xe5c bit 1 (xE5C_1, the arm swipe's live flag): the header does not declare it,
//     so the arm swipe's segment test has no live-flag guard (the state calls it only while the
//     body is in the melee-attack state).

#include "MetroidPrime/Enemies/CIngBoostBallGuardian.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Particles/CGenDescription.hpp"
#include "Kyoto/Text/CStringTable.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CAxisAngle.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CKnockBackInfo.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/Weapons/CWeapon.hpp"
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Enemies/CGenericFSM2.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Enemies/CTeamAiRole.hpp"
#include "MetroidPrime/Enemies/PortBoostGuardianShared.hpp"
#include "MetroidPrime/HUD/CHUDMemoParms.hpp"
#include "MetroidPrime/HUD/CSamusHud.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/Player/CEnvironmentVariable.hpp"
#include "MetroidPrime/Player/CGameState.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrIng.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CHUDBillboardEffect.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIHint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include <stdio.h>
#include <stdlib.h>

namespace {

// The Ing's own helpers (the Ing host body's anonymous namespace), kept here because the Ing's
// shared header is not part of this module. The solid filter itself is shared with the other
// files of this module: PortBoostGuardianShared.hpp has SolidFilter().

// The Ing's fn_29_3D74: the box between two points, three units taller at each end (retail's
// double 3.0, passed in f1; fn_30_57E0 itself takes the pad as an argument).
// @ fn_30_57E0
CAABox BgSegmentBox(const CVector3f& a, const CVector3f& b) {
  return CAABox(
      CVector3f(CMath::Min(a.GetX(), b.GetX()), CMath::Min(a.GetY(), b.GetY()),
                CMath::Min(a.GetZ(), b.GetZ()) - 3.f),
      CVector3f(CMath::Max(a.GetX(), b.GetX()), CMath::Max(a.GetY(), b.GetY()),
                CMath::Max(a.GetZ(), b.GetZ()) + 3.f));
}

// fn_30_5C80 (the Ing's fn_29_421C): which arm swings; true is the left (0xd80..), false the right (0xd7c..).
// Range(rodata_70 = 0, rodata_CC = 100) >= rodata_B4 = 50 (0x00005CA4..0x00005CB4).
bool BgPickSwipeArm(CStateManager& mgr) { return mgr.Random()->Range(0.f, 100.f) >= 50.f; }

// fn_29_61B4's search ring (rodata_44): eight angles in degrees, 180 first, taken in order.
const float skBgMiniPortalAngles[] = {180.f, 0.f, 135.f, 45.f, -135.f, -45.f, 90.f, -90.f};

// Retail keeps the next table index in the REL's bss (lbl_29_bss_64), not in the actor, so it is
// shared by every Ing and survives across state entries; the port does the same on purpose.
int sBgMiniPortalAngleIndex = 0;

} // namespace

// fn_29_8EF0
// @ fn_30_A364
void CIngBoostBallGuardian::Alert(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("Alert");
    mPrevTargetId = mTargetId;
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    Stop();
  } else if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) {
      BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(pas::kTT_Four));
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
    mFormTime = 0.f;
  }
}

// fn_29_8BAC
// @ fn_30_A21C
void CIngBoostBallGuardian::Taunt(CStateManager&, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("Taunt");
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x00008BE4: 0x6b4 = 1
  } else if (msg == kStateMsg_Update) {
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Taunt)) { // 0x00008BF4: 0x10
      BodyController()->CommandMgr().DeliverCmd(CBCTauntCmd(pas::kTT_Two)); // 0x00008C40: 0x15
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x00008C70: 0x6b4 = 0
    mTaunt = false; // 0x00008C78: 0xe5c bit 6
  }
}

// fn_29_8A2C
// @ fn_30_A0D8
void CIngBoostBallGuardian::Lurk(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("Lurk");
    if (DebugBg()) {
      fprintf(stderr, "[ibbg] %04x lurk: detection range %.1f height %.1f\n", GetUniqueId().Value(),
              mDetectionRange, mDetectionHeightRange);
    }
    if (mTargetId != kInvalidUniqueId) { // 0x0000A11C: the id alone, no lookup
      mAlert = true;                     // 0x0000A12C: 0x11ED 0x08 = 1
    }
    CRandom16* random = mgr.Random();
    // 0x0000A158 cror eq,lt,eq: <= ; 0x0000A164: 0x11EC 0x80.
    mTaunt = random->Range(0.f, 100.f) <= mData.mTauntChance;
    // 0x0000A174: data 0x40; 0x0000A1A0 <= ; 0x0000A1B8: 0x11EC 0x20.
    mAggressive = (mData.mFlags210 & kFlag_NoArmSwipe) == 0 &&
                  random->Range(0.f, 100.f) <= mData.mAggressiveness;
    // 0x0000A1D8 <= ; 0x0000A1E4: 0x11EC 0x40. Unlike the Ing, no data flag gates this roll.
    mBodyProjectileAllowed = random->Range(0.f, 100.f) <= mData.mBodyProjectile.mOdds;
  } else if (msg == kStateMsg_Update) {
    if (mForm == kForm_IngSpot) {
      Stop();
    }
  }
}

// fn_29_8808
// @ fn_30_9F50
void CIngBoostBallGuardian::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  const bool canPathFind = (mData.mFlags210 & kFlag_NoPathFind) == 0 && mFoundMovementPos;
  bool moved = false;
  if (msg == kStateMsg_Activate) {
    LogState("PathFind");
    // fn_30_9F50 0x00009FE8: steering blend mode 1; 0x00009FC4: 0x11EC 0x20 (mAggressive) picks
    // 1.0 over the walk/run speed ratio (0x00009FBC).
    BodyController()->CommandMgr().SetSteeringBlendMode(ESteeringBlendMode::kSBM_FullSpeed);
    const CBodyStateInfo& info = BodyController()->GetBodyStateInfo();
    const float speed = mAggressive ? 1.f
                                    : info.GetLocomotionSpeed(pas::kLA_Walk) /
                                          info.GetLocomotionSpeed(pas::kLA_Run);
    BodyController()->CommandMgr().SetSteeringSpeedRange(speed, speed);
    if (canPathFind) { // 0x00009FFC: data 0x04; 0x0000A008: 0x11EC 0x10
      mPathFindNavigation.PathFind(mgr, msg, dt, *this);
      moved = true; // 0x0000A028
    }
    mRecalcPath = false; // 0x0000A034: 0x11EC 0x08 = 0
  } else if (msg == kStateMsg_Update) {
    if (canPathFind) { // 0x0000A044: data 0x04; 0x0000A050: 0x11EC 0x10
      // 0x0000A058: 0x11EC 0x20, 0x0000A070: HasLineOfSight (fn_30_C4E0), 0x0000A07C: it also
      // paths while the safe-zone count (0x115C) is > 0.
      const bool hold = !mAggressive && mLineOfSight.HasLineOfSight() && mSafeZoneCount <= 0;
      if (!hold) {
        mPathFindNavigation.PathFind(mgr, msg, dt, *this);
        moved = true; // 0x0000A0A0
      }
    }
  }
  // Deactivate (0x00009F8C) and every other message only Stop() (0x0000A0B0).
  if (!moved) {
    Stop();
  }
}

// fn_30_9DA0 (the Ing's fn_29_8684): the puddle's mesh path find. Activate clears mRecalcPath and
// raises mMoving; the body only paths when the properties allow path finding and a destination was
// found. Unlike the Ing there is no separation and no ball-pursuit speed: the speed is the ing
// spot's maximum, or the fast speed (data +0x14, unknown_0x50398a06) while retreating or in the
// boost puddle form with a boost pending and a cover hint, times the phase's ingSpotSpeedScale; the
// actor faces its move only while no retreat point is held.
// @ fn_30_9DA0
void CIngBoostBallGuardian::IngSpotPathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("IngSpotPathFind");
    mRecalcPath = false; // 0x00009DE0: 0x11EC 0x08 = 0
    mMoving = true;      // 0x00009DF0: 0x11ED 0x04 = 1
  }
  // 0x00009DFC: data 0x04; 0x00009E08: 0x11EC 0x10.
  if ((mData.mFlags210 & kFlag_NoPathFind) != 0 || !mFoundMovementPos) {
    return;
  }
  mPathFindNavigation.PathFind(mgr, msg, dt, *this); // 0x00009E20: 0x6F4
  // 0x00009E2C: 0x11EE 0x80; 0x00009E3C: 0x11EF 0x80; 0x00009E44: 0x11EE 0x10; 0x00009E54: fn_30_60C0.
  const bool fast = mBoostModeRetreatActive ||
                    (mCurrentlyInBoostPuddleForm && mShouldBoost &&
                     GetCoverHint(mgr, mCoverHintId) != nullptr);
  const CVector3f move = BodyController()->CommandMgr().GetMoveVector();
  BodyController()->CommandMgr().ClearLocomotionCmds();
  if (move.IsMagnitudeSafe() && dt > 0.f) { // 0x00009EA8: rodata_70 (0.0)
    const CVector3f dir = move.AsNormalized();
    // 0x00009EE4: data 0x14 or 0x00009EEC: data 0x08, times 0x00009EF8: *(0x11D0)+0x34.
    const float speed = (fast ? mData.mIngSpot.GetSpeedModifier() : mData.mIngSpot.GetMaxSpeed()) *
                        CurrentPhase().mIngSpotSpeedScale;
    MoveOnSurface(move, speed, dt); // 0x00009F00: fn_30_53D0
    if (mBoostNavPointId2 == kInvalidUniqueId) { // 0x00009F10: 0x1098
      BodyController()->FaceDirection(dir, dt);
    }
  }
}

// fn_29_8570: the point-graph variant, same two guards and the same Activate stores as
// fn_29_8684 (asm 0x0000858C-0x000086A8); the speed blends from the safe-zone-or-maximum base to
// the wall speed by the angle between the actor's up and world up (asm 0x00008600-0x00008740).
// @ fn_30_9C7C
void CIngBoostBallGuardian::IngSpotPointPathFind(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("IngSpotPointPathFind");
    mRecalcPath = false; // 0x00009CAC: 0x11EC 0x08 = 0
    mMoving = true;      // 0x00009CBC: 0x11ED 0x04 = 1
  }
  // 0x00009CC8: data 0x04; 0x00009CD4: 0x11EC 0x10.
  if ((mData.mFlags210 & kFlag_NoPathFind) != 0 || !mFoundMovementPos) {
    return;
  }
  mIngSpotPathFindNavigation.PathFind(mgr, msg, dt, *this);
  const CVector3f move = BodyController()->CommandMgr().GetMoveVector();
  BodyController()->CommandMgr().ClearLocomotionCmds();
  // 0x00009D38: the boost retreat (0x11EE 0x80) takes data 0x14 over data 0x08 as the floor
  // speed, blended toward the wall speed (data 0x0C) by the tilt off world up, then scaled by the
  // phase (0x00009D64: *(0x11D0)+0x34) like IngSpotPathFind.
  const float baseSpeed = mBoostModeRetreatActive ? mData.mIngSpot.GetSpeedModifier()
                                                  : mData.mIngSpot.GetMaxSpeed();
  const float speed = (CVector3f::GetAngleDiff(CVector3f::Up(), GetTransform().GetUp()) /
                           3.1415927f * (mData.mIngSpot.GetMaxWallSpeed() - baseSpeed) +
                       baseSpeed) *
                      CurrentPhase().mIngSpotSpeedScale;
  MoveOnSurface(move, speed, dt); // 0x00009D80: fn_30_53D0
}

// fn_30_9BB0 (this module).
// fn_29_84A4
// @ fn_30_9BB0
void CIngBoostBallGuardian::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("Patrol");
    // 0x00008510: the steering blend mode 1 (kSBM_FullSpeed); the speed range is the locomotion's
    // walk speed over its run speed (0x00008508 divides the two). The host's GetLocomotionSpeed
    // returns 0.f when the locomotion state is missing, so the ratio is then inf, as in retail.
    BodyController()->CommandMgr().SetSteeringBlendMode(ESteeringBlendMode::kSBM_FullSpeed);
    const CBodyStateInfo& info = BodyController()->GetBodyStateInfo();
    const float speed =
        info.GetLocomotionSpeed(pas::kLA_Walk) / info.GetLocomotionSpeed(pas::kLA_Run);
    BodyController()->CommandMgr().SetSteeringSpeedRange(speed, speed);
  }
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);
}

// fn_30_99D4 (this module).
// fn_29_82D4.
// @ fn_30_99D4
void CIngBoostBallGuardian::FollowAttackPattern(CStateManager& mgr, EStateMsg msg, float dt) {
  mWaypointNavigation.Patrol(mgr, msg, dt, *this);
  // 0x00009A10: in the ing-spot form the Ing walks the phase's speed scale (0x00009A64:
  // *(0x11D0)+0x34) times the waypoint's own speed (0x158) times the ing spot's maximum speed
  // (0x00009A84: 0x7c8, data 0x08), on every message - the waypoint is the navigation's destination
  // (0x6c0) and a missing one leaves the speed at 1.0 (rodata_64).
  if (mForm == kForm_IngSpot) {
    const CVector3f move = BodyController()->CommandMgr().GetMoveVector();
    BodyController()->CommandMgr().ClearLocomotionCmds();
    const CScriptAIWaypoint* waypoint =
        TCastToConstPtr< CScriptAIWaypoint >(mgr.GetObjectById(mWaypointNavigation.GetDestination()));
    const float speed = waypoint != nullptr ? waypoint->GetSpeed() : 1.f;
    MoveOnSurface(move, CurrentPhase().mIngSpotSpeedScale * speed * mData.mIngSpot.GetMaxSpeed(),
                  dt); // 0x00009A90: fn_30_53D0
  }
  if (msg == kStateMsg_Activate) {
    LogState("FollowAttackPattern");
    // 0x000083AC: the resume id (0xd70) when it is set, else the "ATTK"/"FOLW" connection.
    const TUniqueId dest = mAttackPathResumeId != kInvalidUniqueId
                               ? mAttackPathResumeId
                               : GetConnectedObject(mgr, kSS_Attack, kSM_Follow);
    mWaypointNavigation.SetDestination(dest);
    mFollowingAttackPath = true; // 0x000083FC: 0xe5d bit 3
  } else if (msg == kStateMsg_Update) {
    // The waypoint's flags (the guest's +0x164) pick the form: 0x40 or 0x80 the ing spot (form 3),
    // 0x100 the corporeal one (form 2). The asm tests them with rlwinm (0x00009B44, 0x00009B4C and
    // 0x00009B60), and PowerPC numbers the bits from the MSB: rlwinm. r0, r3, 0, N, N tests
    // 1u << (31 - N), so (25,25) is bit 6 (0x40), (24,24) is bit 7 (0x80) and (23,23) is bit 8
    // (0x100).
    const CScriptAIWaypoint* waypoint =
        TCastToConstPtr< CScriptAIWaypoint >(mgr.ObjectById(mWaypointNavigation.GetDestination()));
    if (waypoint != nullptr) {
      if ((waypoint->GetFlags() & (0x40 | 0x80)) != 0) {
        mDesiredForm = kForm_IngSpot;
      } else if ((waypoint->GetFlags() & 0x100) != 0) {
        mDesiredForm = kForm_Corporeal;
      } // area 28 has no waypoint with neither flag, so the fall-through never runs there
    }
  } else if (msg == kStateMsg_Deactivate) {
    mFollowingAttackPath = false; // 0x00008468: 0xe5d bit 3
    mAttackPathResumeId = mWaypointNavigation.GetDestination(); // 0x00008478: 0xd70 = 0x6c0
  }
}

// fn_30_953C (this module).
// fn_29_7E30: the body projectile in flight. The launch point and the flight direction are two
// file statics in the REL's bss, seeded on the first call (bss_10, bss_20) from Zero() and
// Forward() and shared by every Ing; each Activate rewrites both. Exercised in area 57
// (/tmp/opencode/ing_bp3.log): BecomeBodyProjectile -> BodyProjectileFlight form 8 (the drift
// branch, two frames) -> form 4 once the anim's kUE_BeginAction lands (35 units a second along the
// launch direction) -> SuckEnergy on the hit, with the emitter removed in Deactivate.
// @ fn_30_953C
void CIngBoostBallGuardian::BodyProjectileFlight(CStateManager& mgr, EStateMsg msg, float dt) {
  static bool sInit = false;
  static CVector3f sLaunchPoint;
  static CVector3f sFlightDir;
  if (!sInit) {
    sLaunchPoint = CVector3f::Zero();
    sFlightDir = CVector3f::Forward();
    sInit = true;
  }

  switch (msg) {
  case kStateMsg_Activate: {
    LogState("BodyProjectileFlight");
    mSplat = false;             // 0xe5e bit 6
    mProjectileFSMEntry = true; // 0xe5e bit 7
    sLaunchPoint = GetLctrTransform(mEyeSegId).GetTranslation(); // 0xd7b
    sFlightDir = CVector3f::Forward();
    if (TargetAimPosition(mgr, mTargetPos, 0.5f)) { // fn_29_4468, 0xdc8; f1 = rodata_80 (0.5)
      // The aim position is dropped by the model's height (scale.z, the REL's only non-zero scale
      // term); a shot that points upwards keeps Forward() as its direction.
      const CVector3f scale = GetModelData()->GetScale(); // 0x60, model data's first CVector3f
      CVector3f aim = mTargetPos - sLaunchPoint;
      aim.SetZ(aim.GetZ() - scale.GetZ());
      if (aim.GetZ() < 0.f && aim.IsMagnitudeSafe()) {
        sFlightDir = GetTransform().TransposeRotate(aim.AsNormalized()); // 0x24
      }
    }
    mBodyProjectileSfx =
        CSfxManager::AddEmitter(mData.mBodyProjectile.mSoundProjectile, GetTranslation(), 0x7f,
                                GetCurrentAreaId().Value(), true, true,
                                CSfxManager::kMedPriority); // 0x850
    break;
  }
  case kStateMsg_Update: {
    // Retail calls UpdateEmitter unconditionally (asm 0x00008090), empty handle included.
    CSfxManager::UpdateEmitter(mBodyProjectileSfx, GetTranslation(),
                               GetTransform().GetForward(), 0x7f);
    static int sTick = 0;
    static int sLastForm = -1;
    if (mForm != sLastForm || BgLogTick(sTick)) {
      sLastForm = mForm;
      fprintf(stderr,
              "[ibbg] %04x flight: form %d time %.2f dt %.3f dir (%.2f, %.2f, %.2f) at "
              "(%.1f, %.1f, %.1f)\n",
              GetUniqueId().Value(), mForm, StateMachineState().GetTime(), dt, sFlightDir.GetX(),
              sFlightDir.GetY(), sFlightDir.GetZ(), GetTranslation().GetX(),
              GetTranslation().GetY(), GetTranslation().GetZ());
    }
    if (mForm == kForm_BodyProjectile) {
      if (dt > 0.f) {
        // Once the state has been running longer than the drop time the flight starts falling: the
        // steer vector loses gravity along z, and the whole velocity follows the steer vector.
        if (StateMachineState().GetTime() > mData.mBodyProjectile.mDropTime) { // 0x858
          sFlightDir.SetZ(sFlightDir.GetZ() - dt * GetGravityConstant());
        }
        MoveInOneFrameOR((dt * mData.mBodyProjectile.mSpeed) * sFlightDir, dt); // 0x854
      }
    } else if (dt > 0.f) {
      // Any other form: the horizontal speed along the steer vector, plus a slow drift (10 units a
      // second) back onto the launch point's height so the body still comes down where it started.
      CVector3f velocity = (dt * mData.mBodyProjectile.mSpeed) * sFlightDir;
      velocity.SetZ(0.f);
      CVector3f drift(0.f, 0.f, sLaunchPoint.GetZ() - GetTranslation().GetZ()); // 0x5c = z
      if (drift.IsMagnitudeSafe()) {
        velocity += drift.AsNormalized() * (dt * 10.f);
      }
      MoveInOneFrameOR(velocity, dt);
    }
    break;
  }
  case kStateMsg_Deactivate:
    // Command 10 (lbl_29_data_C50's id word) is kBSC_ExitState.
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
    // Retail removes unconditionally and then stores 0 to 0xd58 (asm 0x0000828C..0x000082A0).
    CSfxManager::RemoveEmitter(mBodyProjectileSfx);
    mBodyProjectileSfx.Clear(); // 0xd58 = 0
    break;
  default:
    break;
  }
}

// fn_30_8DD4 (this module).
// fn_29_76B8: the Ing possesses the player. Activate drops the body (form 0), clears its
// particles, aborts the scripted body state, strips its materials, remembers the target as the host,
// spawns the possessed-mold billboard and the drape voice, deals the body projectile's contact
// damage and attaches to the player (with the HUD warning); Update rides on the host's transform and
// aim point, draining mUnknownA0D6 a second through a solid-only filter, until the body projectile's
// suck time runs out; Exit releases the player and the voice.
// @ fn_30_8DD4
void CIngBoostBallGuardian::SuckEnergy(CStateManager& mgr, EStateMsg msg, float dt) {
  switch (msg) {
  case kStateMsg_Activate: {
    LogState("SuckEnergy");
    mSuckTickLogged = false;
    mForm = kForm_Host; // 0x9d4 = 0
    AnimationData()->GetParticleDB().DestroyAllActiveParticles();
    AnimationData()->GetParticleDB().ClearAllNonPersistentEffects(&mgr);
    BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_AbortScripted)); // id word 13
    // Immediate values 0x21/0x25/0x3b/0x28/0x29.
    RemoveMaterial(kMT_Character, kMT_GroundCollider, kMT_Solid, kMT_Target, kMT_Orbit, mgr);
    mHostId = mTargetId; // 0xcdc <- 0xd6c
    if (mSwarmEffect) {  // 0xcec, the swarm GEN token's engaged byte
      mPossessedEffectId = mgr.AllocateUniqueId(); // 0xcde
      mgr.AddObject(rs_new CHUDBillboardEffect(
          rstl::optional_object< TToken< CGenDescription > >(*mSwarmEffect),
          rstl::optional_object_null(), mPossessedEffectId, true,
          rstl::string_l("Ing Possessed Mold"), CHUDBillboardEffect::GetNearClipDistance(mgr, 0),
          CHUDBillboardEffect::GetScaleForPOV(mgr), 0, CColor::White(), CVector3f::One(),
          CVector3f::Zero(), true)); // adjustDepthRange, unlike the Grenchler's visor effects
    }
    // 0x852: the body projectile's splat-wall sound, one shot at the Ing's own position.
    mSuckSfx = CSfxManager::AddEmitter(mData.mBodyProjectile.mSoundSplatWall, GetTranslation(), 0x7f,
                                       GetCurrentAreaId().Value(), true, false,
                                       CSfxManager::kMedPriority); // 0xd00
    // 0x82c (the body projectile's contact damage) with the default filter (kFT_Always) and the
    // actor transform's column 1 as the knock-back.
    mgr.ApplyDamage(GetUniqueId(), mHostId, GetUniqueId(), mData.mBodyProjectile.mContactDamage,
                    CMaterialFilter(), GetTransform().GetForward());
    if (CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mHostId))) {
      // 0x1144 bits 7/6 cleared (asm 0x00007A98/0x00007AB0): no morph transition of either kind
      // while the Ing rides the player.
      player->EnableLeaveMorphBall(false);
      player->EnableEnterMorphBall(false);
      player->AttachActorToPlayer(GetUniqueId(), false);
      CEnvironmentVariable* var =
          gpGameState->SystemOptions().EnvVars().FindEnvironmentVariable("IngAttachedWarningCount");
      if (var != nullptr && var->GetValue() != var->GetMaximum()) {
        var->Set(var->GetValue() + 1);
        CSamusHud::DisplayHudMemo(rstl::wstring_l(gpStringTable->GetString("IngAttachedWarning")),
                                  CHUDMemoParms(5.f, true, false, false,
                                                1 << player->GetPlayerIndex(), true));
      }
      if (DebugBg()) {
        fprintf(stderr, "[ibbg] %04x suck: host %04x attached (mold %04x), %.1fs, %.1f dmg/s\n",
                GetUniqueId().Value(), mHostId.Value(), mPossessedEffectId.Value(),
                mData.mBodyProjectile.mSuckTime, mData.mBodyProjectile.mUnknownA0D6);
      }
    } else if (DebugBg()) {
      fprintf(stderr, "[ibbg] %04x suck: host %04x is not a player\n", GetUniqueId().Value(),
              mHostId.Value());
    }
    // 0x9ca (the swarm block's mSoundInsideHost) at kMaxVolume, pan 0x40; the handle replaces the
    // one-shot above, exactly as retail's dead store to 0xd00 does.
    mSuckSfx = CSfxManager::SfxStart(mData.mSwarm.mSoundInsideHost, CAudioSys::kMaxVolume, 0x40,
                                     GetCurrentAreaId().Value(), true, false,
                                     CSfxManager::kMedPriority); // 0xd00
    break;
  }
  case kStateMsg_Update: {
    if (StateMachineState().GetTime() > mData.mBodyProjectile.mSuckTime) { // 0x864
      mHostId = kInvalidUniqueId; // 0xcdc
      break;
    }
    CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mHostId));
    if (player == nullptr) {
      mHostId = kInvalidUniqueId;
      break;
    }
    SetTransform(player->GetTransform());             // 0x7b30
    SetTranslation(player->GetAimPosition(mgr, 0.f)); // vtable 0x54, rodata_6C = 0.0f
    if (DebugBg() && !mSuckTickLogged) {
      mSuckTickLogged = true;
      const CHealthInfo* hp = player->GetHealthInfo();
      fprintf(stderr, "[ibbg] %04x suck tick: riding host %04x (mold %04x), %.2f dmg/frame, host hp %.0f\n",
              GetUniqueId().Value(), mHostId.Value(), mPossessedEffectId.Value(),
              dt * mData.mBodyProjectile.mUnknownA0D6, hp != nullptr ? hp->GetHP() : -1.f);
    }
    // 0x848: mUnknownA0D6 a second, no immunity. Retail's weapon-mode word comes from uninitialised
    // stack with its low bits cleared; kWT_AI is the 0xb<<16 half it does set.
    const CDamageInfo damage(CWeaponMode(kWT_AI), dt * mData.mBodyProjectile.mUnknownA0D6, 0.f, 0.f,
                             true, false);
    // The asm builds one included material, 1 << 0x3b (kMT_Solid), type 3 = kFT_IncludeExclude.
    mgr.ApplyDamage(GetUniqueId(), mHostId, GetUniqueId(), damage, SolidFilter(),
                    GetTransform().GetForward());
    break;
  }
  case kStateMsg_Deactivate: {
    if (DebugBg()) {
      fprintf(stderr, "[ibbg] %04x suck end: releasing %04x\n", GetUniqueId().Value(),
              mTargetId.Value());
    }
    if (CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mTargetId))) {
      // 0x1144 bits 7/6 set again (asm 0x00007D74/0x00007D88): the player may morph once the Ing
      // lets go.
      player->EnableLeaveMorphBall(true);
      player->EnableEnterMorphBall(true);
      if (player->GetAttachedActorId() == GetUniqueId()) { // 0x2e4
        player->DetachActorFromPlayer();
      }
    }
    CSfxManager::RemoveEmitter(mSuckSfx); // 0xd00
    mSuckSfx = CSfxHandle();
    mHostId = kInvalidUniqueId; // 0xcdc
    break;
  }
  default:
    break;
  }
}

// fn_29_75C0
// Activate and Update both move (0x00008CE4: msg < 2).
// @ fn_30_8CC0
void CIngBoostBallGuardian::SeekWallPoint(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("SeekWallPoint");
  } else if (msg != kStateMsg_Update) {
    return;
  }
  const CScriptAIHint* hint = GetCoverHint(mgr, mCoverHintId);
  if (hint == nullptr) {
    return; // 0x00008D04: a missing hint leaves mMoving as it was
  }
  const CVector3f delta = hint->GetTranslation() - GetTranslation();
  // 0x00008D4C: the phase's speed scale (*(0x11D0)+0x34) times ingSpotMaxWallSpeed (0x7cc).
  const float maxWall = CurrentPhase().mIngSpotSpeedScale * mData.mIngSpot.GetMaxWallSpeed();
  if (!delta.IsMagnitudeSafe() || delta.Magnitude() <= dt * maxWall) {
    mMoving = false; // 0x00008DA8: 0x11ED 0x04
  } else {
    MoveOnSurface(delta.AsNormalized(), maxWall, dt); // 0x00008D88: fn_30_53D0
    mMoving = true; // 0x00008D94: 0x11ED 0x04
  }
}

// fn_29_7518
// @ fn_30_8C18
void CIngBoostBallGuardian::SelectTarget(CStateManager& mgr, EStateMsg msg, float) {
  if (msg != kStateMsg_Activate) {
    return;
  }
  LogState("SelectTarget");
  // 0x00008C58: a live target keeps everything as it is - no line-of-sight retarget, no message.
  if (mgr.GetObjectById(mTargetId) != nullptr) {
    return;
  }
  mTargetId = mgr.GetPlayer(0)->GetUniqueId(); // 0x00008C6C: mgr+0x14FC, no null test
  mLineOfSight.SetTarget(mTargetId);           // 0x00008C78: 0x10B8
  SendScriptMsgs(kSS_Attack, mgr, kInvalidUniqueId, kSM_None); // 0x00008CA4: 'ATTK', -1
}

// fn_29_7244
// @ fn_30_8998
void CIngBoostBallGuardian::FaceTarget(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("FaceTarget");
    // 0x000089DC: the target's aim position into 0x10F8 (fn_30_5F6C, rodata_70 = 0.0), levelled
    // to the Ing's own height (0x000089F4); more than 20 degrees off the forward axis
    // (0x00008A50, rodata_104) leaves turning to do.
    if (TargetAimPosition(mgr, mTargetPos)) {
      mTargetPos.SetZ(GetTranslation().GetZ());
      mAnimationState.SetState(CVector3f::GetAngleDiff(GetTransform().GetForward(),
                                                       mTargetPos - GetTranslation()) > kFacingAngle
                                   ? CAnimationState::kAS_Ready
                                   : CAnimationState::kAS_Over);
    } else {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x00008A6C
    }
    mMoving = false; // 0x00008A7C: 0x11ED 0x04 = 0
    break;
  case kStateMsg_Update:
    // Update turns toward the point Activate stored, not the live target.
    switch (mForm) {
    case kForm_Corporeal: // 0x00008AB8: a turn command once the body can take one
      if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_Turn)) {
        const CVector3f delta = mTargetPos - GetTranslation();
        if (delta.IsMagnitudeSafe()) {
          BodyController()->CommandMgr().DeliverCmd(
              CBCLocomotionCmd(CVector3f::Zero(), delta.AsNormalized(), 1.f)); // rodata_64
        } else {
          mAnimationState.SetState(CAnimationState::kAS_Over); // 0x00008B74
        }
      }
      break;
    case kForm_IngSpot: // 0x00008B80: forms 3 and 6-9 snap to face the point (LookAt, world up)
    case 6:
    case 7:
    case 8:
    case 9:
      if ((mTargetPos - GetTranslation()).IsMagnitudeSafe()) {
        SetTransform(CTransform4f::LookAt(GetTranslation(), mTargetPos, CVector3f::Up()));
        mAnimationState.SetState(CAnimationState::kAS_Over);
      }
      break;
    default: // 0x00008BF0
      mAnimationState.SetState(CAnimationState::kAS_Over);
      break;
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x00008BFC
    break;
  default:
    break;
  }
}

// The Ing's fn_29_6DA0.
// Retail keeps the four lowest-scoring candidates in a file-static array (REL bss lbl_30_bss_40:
// a count plus four {TUniqueId, float} entries), insertion-sorted ascending, then takes one entry
// at random through the state manager's RNG. Unlike the Ing's copy, this module takes any hint
// type but the boost nav points (14 and 15, asm 0x000086EC) and never tests the hint against the
// safe zones; a wall hint (type 4) still needs the body projectile allowed by the properties (data
// 0x20, 0x00008700) and the target outside every safe zone (TargetInSafeZone, 0x0000866C).
// @ fn_30_8520
void CIngBoostBallGuardian::FindCoverPoint(CStateManager& mgr, EStateMsg msg, float) {
  if (msg != kStateMsg_Activate) { // asm 0x0000854C
    return;
  }
  LogState("FindCoverPoint");
  ReleaseOwnCoverHint(mgr);                  // fn_30_6064, asm 0x00008554
  if (!TargetAimPosition(mgr, mTargetPos)) { // fn_30_5F6C, 0x10F8; f1 = rodata_70 (0.0)
    return;
  }
  struct SCandidate {
    SCandidate() : id(kInvalidUniqueId), score(0.f) {}
    TUniqueId id;
    float score;
  };
  static SCandidate sCandidates[4];
  static int sCandidateCount = 0;
  sCandidateCount = 0; // asm 0x00008614
  // Near/far score bounds, each the max of two ranges, squared (asm 0x00008620/0x0000863C): near
  // from mMinAttackRange (0x3E0) vs the body projectile's minimum (0x87C), far from its maximum
  // (0x880) vs the mini portal's (0x894).
  const float nearBound = mMinAttackRange >= mData.mBodyProjectile.mMinAttackDist
                              ? mMinAttackRange
                              : mData.mBodyProjectile.mMinAttackDist;
  const float f31 = nearBound * nearBound;
  const float farBound = mData.mBodyProjectile.mMaxAttackDist >= mData.mMiniPortal.mMaxAttackDist
                             ? mData.mBodyProjectile.mMaxAttackDist
                             : mData.mMiniPortal.mMaxAttackDist;
  const float f30 = farBound * farBound;
  // Evaluated once before the loop (fn_30_C14C with a zeroed CTriggerData).
  const bool targetInSafeZone = TargetInSafeZone(mgr, CTriggerData(0.f));
  // Debug: why the scan comes up empty.
  int scanned = 0, inactive = 0, inUse = 0, wrongArea = 0, lastUsed = 0, boostNav = 0;
  int spotGated = 0, safeZone = 0;
  const CObjectList& list = mgr.GetObjectListById(kOL_AiWaypoint); // mgr+0x838
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CScriptAIHint* hint = TCastToConstPtr< CScriptAIHint >(list[i]);
    if (hint == nullptr) {
      continue;
    }
    ++scanned;
    if (!hint->GetActive()) { // asm 0x000086A0: byte 0x20 bit 7
      ++inactive;
      continue;
    }
    if (hint->GetInUse(GetUniqueId())) { // asm 0x000086B8
      ++inUse;
      continue;
    }
    if (hint->GetCurrentAreaId() != GetCurrentAreaId()) { // asm 0x000086CC, area word 0x4
      ++wrongArea;
      continue;
    }
    if (hint->GetUniqueId() == mLastCoverHintId) { // asm 0x000086DC, 0x1090
      ++lastUsed;
      continue;
    }
    const int type = static_cast< int >(hint->GetHintType()); // hint 0x158
    if (type == 14 || type == 15) { // asm 0x000086EC: (type - 14) <= 1 unsigned
      ++boostNav;
      continue;
    }
    if (type == 4) { // asm 0x000086F4: wall hints only
      // 0x00008700 reads the data byte's 0x20 (this+0xAE0), not a guest bitfield: a guardian
      // whose properties forbid the body projectile never takes a wall point either.
      if ((mData.mFlags210 & kFlag_NoBodyProjectile) != 0) {
        ++spotGated;
        continue;
      }
      if (targetInSafeZone) { // asm 0x00008708
        ++safeZone;
        continue;
      }
    }
    const CVector3f delta = mTargetPos - hint->GetTranslation(); // 0x10F8 - hint+0x54
    float score = delta.MagSquared();
    const float up = CVector3f::Dot(CVector3f::Up(), delta);
    if (type != 4 || up > 0.f) { // asm 0x00008760: wall hints at/below the target skip the term
      score += 4.f * up * up;    // rodata_9C
    }
    if (score < f31) { // asm 0x00008784
      score += f30;
    }
    // Store at the end (or over the worst entry when full), then bubble down to keep the array
    // ascending (asm 0x0000879C/0x0000882C).
    int slot;
    if (sCandidateCount < 4) {
      slot = sCandidateCount++;
    } else if (score < sCandidates[3].score) {
      slot = 3;
    } else {
      continue;
    }
    sCandidates[slot].id = hint->GetUniqueId();
    sCandidates[slot].score = score;
    for (; slot > 0 && sCandidates[slot].score < sCandidates[slot - 1].score; --slot) {
      const SCandidate swap = sCandidates[slot];
      sCandidates[slot] = sCandidates[slot - 1];
      sCandidates[slot - 1] = swap;
    }
  }
  TUniqueId picked = kInvalidUniqueId;
  if (sCandidateCount != 0) { // asm 0x000088A4
    const int index = mgr.Random()->Range(0, sCandidateCount - 1); // asm 0x000088B8, mgr+0x16e4
    picked = sCandidates[index].id;
    // The id came off the hint list, so retail takes the object as a hint with no type test.
    if (CScriptAIHint* hint = static_cast< CScriptAIHint* >(mgr.ObjectById(picked))) {
      // fn_30_6014 (asm 0x000088E4): in use, then the cover hint (0x108E) and the last one (0x1090).
      hint->SetInUse(true);
      mCoverHintId = hint->GetUniqueId();
      mLastCoverHintId = mCoverHintId;
    }
  }
  if (DebugBg()) {
    fprintf(stderr,
            "[ibbg] %04x cover scan: hints %d, candidates %d, pick %04x: inactive %d inuse %d "
            "area %d last %d boostnav %d wallspot %d safezone %d\n",
            GetUniqueId().Value(), scanned, sCandidateCount, picked.Value(), inactive, inUse,
            wrongArea, lastUsed, boostNav, spotGated, safeZone);
  }
}

// fn_30_5888 (the Ing's fn_29_3E1C): the segment test of one arm. Four boxes along the arm (shoulder-elbow,
// elbow-forearm, forearm-wrist, then the wrist extended six units along the forearm) are tested
// against the target's bounding box; retail's chained early-outs are one OR. A hit deals the arm
// swipe damage with the given knock-back and clears the contact-damage pause.
//
// The live flag (0xe5c bit 1, xE5C_1) guards the test: only a swing that is live can hit, and a
// hit clears the flag.
// @ fn_30_5888
void CIngBoostBallGuardian::ArmSwipeSegment(CStateManager& mgr, const CSegId& a, const CSegId& b,
                                             const CSegId& c, const CSegId& d,
                                             const CVector3f& knockback) {
  if (!xE5C_1) { // 0xe5c bit 1: only while the swing is live
    return;
  }
  CPhysicsActor* target = TCastToPtr< CPhysicsActor >(mgr.ObjectById(mTargetId));
  if (target == nullptr) {
    return;
  }
  const CAABox bounds = target->GetBoundingBox();
  const CVector3f p0 = GetLctrTransform(a).GetTranslation();
  const CVector3f p1 = GetLctrTransform(b).GetTranslation();
  const CVector3f p2 = GetLctrTransform(c).GetTranslation();
  const CVector3f p3 = GetLctrTransform(d).GetTranslation();
  bool hit = bounds.DoBoundsOverlap(BgSegmentBox(p0, p1)) ||
             bounds.DoBoundsOverlap(BgSegmentBox(p1, p2)) ||
             bounds.DoBoundsOverlap(BgSegmentBox(p2, p3));
  if (!hit) {
    const CVector3f delta = p3 - p2;
    if (!delta.IsMagnitudeSafe()) { // retail gives up here rather than normalising
      return;
    }
    hit = bounds.DoBoundsOverlap(BgSegmentBox(p3, p3 + delta.AsNormalized() * 6.f));
    if (!hit) {
      return;
    }
  }
  if (DebugBg()) {
    fprintf(stderr, "[ibbg] %04x arm swipe hit: arm %d (%02x..%02x), target %04x, damage %.1f\n",
            GetUniqueId().Value(), mSwipeArm, static_cast< int >(a.val()),
            static_cast< int >(d.val()), mTargetId.Value(),
            mData.mArmSwipeDamage.GetDamage());
  }
  // rodata A10 = 0x3B (kMT_Solid) and the filter's type word 3 = kFT_IncludeExclude.
  mgr.ApplyDamage(GetUniqueId(), mTargetId, GetUniqueId(), mData.mArmSwipeDamage,
                  CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList()),
                  knockback);
  // Unlike the Ing's copy, this module does not restart the contact-damage cooldown (0x440) here.
  xE5C_1 = false; // 0x00005C64: 0x11EC bit 2
}

// fn_30_8208 (this module).
// fn_29_6A88
// @ fn_30_8208
void CIngBoostBallGuardian::ArmSwipe(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("ArmSwipe");
    if (!TargetAimPosition(mgr, mTargetPos)) { // fn_29_4468, 0xdc8
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
      break;
    }
    // No team AI is linked to the guardian in its room, so this refusal never runs there.
    if (mTeamId != kInvalidUniqueId &&
        !CScriptTeamAiMgr::StartAttack(CScriptTeamAiMgr::kAT_Melee, mgr, mTeamId, GetUniqueId())) {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
      break;
    }
    mFormTime += mData.mUnknownC620;                       // 0xe3c += 0x7c8
    mAnimationState.SetState(CAnimationState::kAS_Ready);  // 0x6b4 = 1
    break;
  case kStateMsg_Update:
    // CTryCommand::ShouldRetry(this+0x6b4, *BodyController(), 7); 7 is pas::kAS_MeleeAttack.
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_MeleeAttack)) {
      mSwipeArm = BgPickSwipeArm(mgr) ? 1 : 0; // 0x000082F8: fn_30_5C80, 0x111C
      // rodata_38 = {1, 2}: the melee attack's severity, indexed by the arm.
      BodyController()->CommandMgr().DeliverCmd(
          CBCMeleeAttackCmd(mSwipeArm == 0 ? pas::kS_One : pas::kS_Two));
      // The live flag (0xe5c bit 1): the contact damage pauses during the swing, and the segment
      // test only runs while it is set.
      xE5C_1 = true; // 0xe5c bit 1
    } else if (BodyController()->GetCurrentStateId() == pas::kAS_MeleeAttack) {
      // The knock-back points sideways: the right arm pushes along -right, the left along +right
      // (retail reads m00/m10/m20, i.e. CTransform4f::GetRight()).
      const CVector3f right = GetTransform().GetRight();
      ArmSwipeSegment(mgr, mRightShoulderSegId, mRightElbowSegId, mRightForearmSegId,
                      mRightWristSegId, CVector3f::Zero() - right); // 0xd7c..0xd7f
      if (mSwipeArm == 1) {
        ArmSwipeSegment(mgr, mLeftShoulderSegId, mLeftElbowSegId, mLeftForearmSegId,
                        mLeftWristSegId, right); // 0xd80..0xd83
      }
    }
    break;
  case kStateMsg_Deactivate:
    mSwipeArm = -1;                                        // 0xdec
    xE5C_1 = false;                                        // 0xe5c bit 1
    mArmSwipeTime = 0.f;                                   // 0xe38
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    CScriptTeamAiMgr::EndAttack(CScriptTeamAiMgr::kAT_Melee, mgr, mTeamId, GetUniqueId(), false);
    break;
  default:
    break;
  }
}

// fn_30_8120 (this module).
// fn_29_69A0: the puddle is off the navmesh. Activate sets kAS_Ready, teleports the actor onto
// the closest physical point within 1000 (rodata_C4) and re-aligns to the surface with 1.0
// (rodata_64), then sets kAS_Over; Deactivate sets kAS_NotReady. Update does nothing (asm
// 0x00006A70 is the common tail).
// @ fn_30_8120
void CIngBoostBallGuardian::FailSafeMode(CStateManager& mgr, EStateMsg msg, float) {
  if (msg == kStateMsg_Activate) {
    LogState("FailSafeMode");
    mAnimationState.SetState(CAnimationState::kAS_Ready);
    int idx = -1;
    const CVector3f before = GetTranslation();
    CPathFindPointSearch::EClosestPointResult res = CPathFindPointSearch::kCPR_NoPoint;
    int numPoints = 0;
    // Port limit: an area with no path area leaves the host's CPFArea pointer null; the REL's search
    // always has one, so the guard only keeps the host's null area from being dereferenced.
    if (mPathArea != nullptr) {
      numPoints = mPathArea->GetNumPoints();
      res = mPointPathFindSearch.FindClosestPhysicalPoint(before, idx,
                                                          CPathFindPointSearchFilter(1000.f, 0, -1));
    }
    if (DebugBg()) {
      fprintf(stderr, "[ibbg] %04x failsafe: points %d res %d idx %d at (%.1f, %.1f, %.1f)\n",
              GetUniqueId().Value(), numPoints, static_cast< int >(res), idx, before.GetX(),
              before.GetY(), before.GetZ());
    }
    if (res == CPathFindPointSearch::kCPR_Success && idx != -1) {
      SetTranslation(mPathArea->GetPoint(idx).GetPosition());
      const CVector3f moved = GetTranslation();
      mSurfaceAlignment.AlignNearPosition(*this, mgr, GetTranslation(), 1.f);
      if (DebugBg()) {
        // Is the point the fail-safe picked on a walkable region at all? IsOffPath asks the ground
        // search, so this says why the Ing can stay off the path after the teleport.
        float height = 0.f;
        const bool onMesh =
            mPathFindSearch.GetHeightOfPointAboveMesh(moved, height, 0.f) == CPathFindSearch::kR_Success;
        const bool near1 = mPathFindSearch.NearlyOnPath(moved, 1.f) == CPathFindSearch::kR_Success;
        const bool near5 = mPathFindSearch.NearlyOnPath(moved, 5.f) == CPathFindSearch::kR_Success;
        fprintf(stderr,
                "[ibbg] %04x failsafe: moved %.2f to (%.1f, %.1f, %.1f), after align (%.1f, %.1f, %.1f)"
                " onmesh %d near1 %d near5 %d height %.2f\n",
                GetUniqueId().Value(), (moved - before).Magnitude(), moved.GetX(), moved.GetY(),
                moved.GetZ(), GetTranslation().GetX(), GetTranslation().GetY(), GetTranslation().GetZ(),
                onMesh ? 1 : 0, near1 ? 1 : 0, near5 ? 1 : 0, height);
      }
      mAnimationState.SetState(CAnimationState::kAS_Over);
    }
  } else if (msg == kStateMsg_Deactivate) {
    mAnimationState.SetState(CAnimationState::kAS_NotReady);
  }
}

// fn_30_7D5C (this module).
// fn_29_61B4: two rays a frame, out from the head locator to the
// target's aim point, until three hits are stored or the shared angle index runs out (the ring is
// eight angles, so a FindMiniPortals entry never searches more than four frames). The filter and
// the geometry are the Space Jump Guardian's (fn_29_1E6C's FindMiniPortals), which is verified.
// @ fn_30_7D5C
void CIngBoostBallGuardian::FindMiniPortals(CStateManager& mgr, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("FindMiniPortals");
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6b4 = 1
    mMiniPortalCount = 0;                                 // 0xe00 = 0
    sBgMiniPortalAngleIndex = 0;
    // fn_29_4468: no target, no portals - the state is over at once.
    if (!TargetAimPosition(mgr, mTargetPos)) {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
    }
    break;
  case kStateMsg_Update: {
    if (mMiniPortalCount >= 3 || sBgMiniPortalAngleIndex >= 8) {
      mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
      break;
    }
    // The ray runs from 3 model-scales in front of the head locator, plus the ring's offset.
    const CVector3f origin = GetLctrTransform(mEyeSegId).GetTranslation(); // 0xd7b
    const CVector3f start = origin + mModelScale.GetY() * (3.f * GetTransform().GetForward());
    const CMaterialFilter filter = CMaterialFilter::MakeIncludeExclude(
        CMaterialList(kMT_Solid),
        CMaterialList(kMT_Character, kMT_Player, kMT_CollisionActor, kMT_ProjectilePassthrough));
    for (int i = 0; i < 2; ++i) {
      // 0x00007FB4..0x00007FC0: the pair re-tests no bound (the bss guard is tested once, at
      // 0x00007E64), so with the index at 7 the second ray reads rodata_44[8], the float after the
      // table: rodata_64 = 1.0f. The host spells that read out instead of indexing past the array.
      const int index = sBgMiniPortalAngleIndex++;
      const float degrees = index < 8 ? skBgMiniPortalAngles[index] : 1.f;
      const float angle = degrees * 0.017453292f; // rodata_100
      const float c = CMath::FastCosR(angle);
      const float s = CMath::FastSinR(angle);
      const CVector3f offset =
          2.f * CVector3f(mModelScale.GetX() * c, mModelScale.GetY() * 0.f, mModelScale.GetZ() * s);
      const CVector3f end = start + GetTransform().Rotate(offset);
      if (mgr.RayCollideWorld(end, mTargetPos, filter, this)) { // 0xdc8
        mMiniPortalPositions[mMiniPortalCount++] = end; // 0xe04 + 12 * count
        if (mMiniPortalCount == 3) {
          mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6b4 = 3
          return;
        }
      }
    }
    break;
  }
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    break;
  default:
    break;
  }
}

// fn_30_7BB0 (this module).
// fn_29_6008: asks the body for one projectile attack; the animation's
// kUE_Projectile events then spawn the portals FindMiniPortals queued (fn_29_C228).
// @ fn_30_7BB0
void CIngBoostBallGuardian::MiniPortalAttack(CStateManager&, EStateMsg msg, float) {
  switch (msg) {
  case kStateMsg_Activate:
    LogState("MiniPortalAttack");
    mAnimationState.SetState(CAnimationState::kAS_Ready); // 0x6b4 = 1
    mMiniPortalIndex = 0;                                 // 0xe28 = 0
    break;
  case kStateMsg_Update:
    // ShouldRetry(0x6b4, body, 0x12): 0x12 is pas::kAS_ProjectileAttack.
    if (mAnimationState.CanIssueCommand(*BodyController(), pas::kAS_ProjectileAttack)) {
      BodyController()->CommandMgr().DeliverCmd(CBCProjectileAttackCmd(
          pas::kS_One, GetTranslation() + GetTransform().GetForward(), false));
    }
    break;
  case kStateMsg_Deactivate:
    mAnimationState.SetState(CAnimationState::kAS_NotReady); // 0x6b4 = 0
    mMiniPortalIndex = -1;                                   // 0xe28 = -1
    break;
  default:
    break;
  }
}

// fn_30_7390: the module's own Dead. Activate restores the speed the ctor saved (0x11A8). Forms
// 0/1 are deleted; the ing spot, the body projectile and the evaporating form leave the boost
// puddle mode, drop the target/orbit materials, explode with the death effect and play the death
// sound; the corporeal form and the form changes burn away through CPatterned::Dead (this module
// has no burning-flag skip and no hit-cooldown store, unlike the Ing REL's fn_29_5CB0).
// @ fn_30_7390
void CIngBoostBallGuardian::Dead(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    LogState("Dead");
    mSpeed = mInitialSpeed; // 0x000073C8: 0x3C8 = 0x11A8
  }
  if (mForm == kForm_Host || mForm == kForm_ControllingHost) {
    if (msg == kStateMsg_Activate) {
      DeathDelete(mgr); // 0x00007590
    }
    return;
  }
  if (mForm == kForm_IngSpot || mForm == kForm_BodyProjectile || mForm == kForm_Evaporate) {
    if (msg != kStateMsg_Activate) {
      return;
    }
    SetBoostPuddleMode(false);                   // 0x00007404: fn_30_3068
    RemoveMaterial(kMT_Target, kMT_Orbit, mgr); // 0x00007418: 0x28/0x29
    if (mDeathEffect) {                          // 0x0000741C: 0x104C
      mgr.AddObject(rs_new CExplosion(
          *mDeathEffect, mgr.AllocateUniqueId(),
          CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true, kInvalidEditorId),
          rstl::string_l("Ing death Fx"), GetTransform(), 0, GetModelData()->GetScale(),
          CColor::White(), -1)); // 0x00007504
    }
    CSfxManager::AddEmitter(mData.mIngSpot.GetDeathSound(), GetTranslation(), 0x7f, // 0x0000756C
                            GetCurrentAreaId().Value(), true, false, CSfxManager::kMedPriority);
    DeathDelete(mgr); // 0x00007578
    return;
  }
  CPatterned::Dead(mgr, msg, dt); // 0x000075A8
  if (msg == kStateMsg_Activate) {
    mDeadFade = 2.f;                             // 0x000075DC: 0x11C8, rodata_78
    RemoveMaterial(kMT_Target, kMT_Orbit, mgr); // 0x000075E0
  } else if (msg == kStateMsg_Update) {
    mAlphaDelta = 0.f;                                                     // 0x00007600: 0x40C
    mSurfaceAlignment.AlignNearPosition(*this, mgr, GetTranslation(), dt); // 0x00007608
    if (BodyController()->GetCurrentStateId() == pas::kAS_Death) {        // 0x00007614
      const CVector3f scale = GetModelData()->GetScale();
      const float z = scale.GetZ() - dt / 0.75f; // rodata_FC
      ModelData()->SetScale(CVector3f(scale.GetX(), scale.GetY(), z > 0.f ? z : 0.f)); // 0x0000766C
      if (z <= 0.f) {
        mDeadFade -= dt;             // 0x00007690
        mRenderModel = false;        // 0x0000769C: 0x11EC bit 0
        mIngSpotBlobVisible = false; // 0x000076A8: 0x11ED bit 6
      }
      if (mDeadFade <= 0.f) { // 0x000076BC
        DeathDelete(mgr);
      }
    }
  }
}
