// The 39 triggers of the SpacePirate state machine (retail table fn_72_10E50 ->
// lbl_72_data_1D8). The trivial retail bodies (under 0x100 bytes, read from
// build/G2ME01/SpacePirate/m2c/fn_72_<off>.c) are translated; every other one returns false behind
// a once-per-name StandIn that names the retail function. Triggers that read a flag bit use the
// names in CSpacePirate.hpp (MP1's where the mask matches).
#include "MetroidPrime/Enemies/CSpacePirate.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CMetroid.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTargetingPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/StateMachineCommon.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "rstl/algorithm.hpp"

#include <stdio.h>
#include <stdlib.h>

void PortSpacePirateStandIn(const char* what);

// PortCSpacePirate.cpp's TraceCall sits in that file's anonymous namespace, so it has no external
// linkage to declare here; this is the same MP2_DEBUG_SPACEPIRATE gate and line format, so a run
// that exercises a trigger below proves it exactly like the entry points there do.
namespace {
void TraceCall(const CSpacePirate& self, const char* what, int arg) {
  static const bool sOn = getenv("MP2_DEBUG_SPACEPIRATE") != nullptr;
  if (sOn) {
    fprintf(stderr, "[spacepirate] %u %s %d\n", self.GetUniqueId().Value(), what, arg);
  }
}
} // namespace

bool CSpacePirate::Stuck(CStateManager& mgr, const CTriggerData& data) const {
  if (mStateMachine->GetTime() > 0.5f) {
    return CPatterned::Stuck(mgr, data) || CPatterned::PathShagged(mgr, data);
  }
  return false;
}

bool CSpacePirate::PatternShagged(CStateManager& mgr, const CTriggerData& data) const {
  return CPatterned::Stuck(mgr, data);
}

bool CSpacePirate::HearShot(CStateManager& mgr, const CTriggerData& data) const {
  const bool heard = x8f6_25_hearNoise;
  const_cast< CSpacePirate* >(this)->x8f6_25_hearNoise = false;
  return heard;
}

// Retail polls one player per call (0xbc0 rotates), so a second player does not add hearing range
// in the same frame: a moving player inside the data's hearing radius is heard.
bool CSpacePirate::HearPlayer(CStateManager& mgr, const CTriggerData& data) const {
  CSpacePirate* self = const_cast< CSpacePirate* >(this);
  const int count = static_cast< int >(mgr.GetNumPlayers());
  if (count <= 0) {
    return false;
  }
  self->xbc0_playerIndex = (self->xbc0_playerIndex + 1) % count;
  const CPlayer* player = mgr.GetPlayer(self->xbc0_playerIndex);
  if (player == nullptr || player->GetVelocityWR().MagSquared() <= 0.1f) {
    return false;
  }
  const CVector3f delta = player->GetTranslation() - GetTranslation();
  return delta.MagSquared() < mData.mHearingRadius * mData.mHearingRadius;
}

// @ fn_72_ADC0
// The 208 cycle does evaluate this trigger (2851 calls in the 2026-10-20 run), but it is always
// false there, so only the first branch is proven by the run.
// Retail: a pirate with the non-aggressive bit (0x8f4 mask 0x20) never charges. Otherwise it
// charges when it is always aggressive (0x8f8 mask 0x20), or when no pirate is charging yet (the
// module's static charge list at bss_40 is empty) and the player has been out of sight for more
// than 10 s (rodata B58). A charging pirate that is not in that list yet adds its id to it: the
// asm walk ends at the list's tail sentinel, so "reached the tail" means "not in the list" and
// pushes (fn_72_AEB8 -> fn_72_AEE0 is a push_back), while a match stops the walk and does not.
bool CSpacePirate::AggressionCheck(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  if (!x8f4_26_nonAggressive) {
    if (x8f8_26_alwaysAggressive) {
      result = true;
    } else if (mChargePlayerList.empty() && xa7c_timeNoPlayerLos > 10.f) {
      result = true;
    }
    if (result) {
      if (rstl::find(mChargePlayerList.begin(), mChargePlayerList.end(), GetUniqueId()) ==
          mChargePlayerList.end()) {
        mChargePlayerList.push_back(GetUniqueId());
      }
    }
  }
  TraceCall(*this, "AggressionCheck trigger", result ? 1 : 0);
  return result;
}

bool CSpacePirate::CoverCheck(CStateManager& mgr, const CTriggerData& data) const {
  return x8f8_27_coverCheck;
}

// fn_72_ABC0. Unmarked: no state the 208 cycle reaches lists this trigger (2026-10-20 run).
// Retail: the AiWaypoint object list (mgr+0x838) is scanned for the cover points this pirate
// could take - active (0x20 bit 7), not a land-hang point (fn_800EC6A8), not reserved by this
// pirate (fn_800EC550 with this pirate's id), in this area, not the cover point it used last
// time (0x902) and closer than mData.mSearchRadius (0x7c8) squared. The closest one that is
// not blown by the target position (fn_72_CB30, not the player) wins. Then the old cover point
// is released (retainCooldown 1), the new one taken, 0x902 remembers it and 0x914 becomes
// -cover forward. The target is fetched again through the object list, so a point that vanished
// leaves the trigger false.
bool CSpacePirate::CoverFind(CStateManager& mgr, const CTriggerData& data) const {
  CSpacePirate* self = const_cast< CSpacePirate* >(this);
  bool result = false;
  float minDistSq = mData.mSearchRadius * mData.mSearchRadius;
  const CScriptCoverPoint* closest = nullptr;
  const CObjectList& list = mgr.GetObjectListById(kOL_AiWaypoint);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CScriptCoverPoint* point = TCastToConstPtr< CScriptCoverPoint >(list[i]);
    if (point == nullptr) {
      continue;
    }
    if (!point->GetActive() || point->ShouldLandHere() || point->GetInUse(GetUniqueId()) ||
        point->GetCurrentAreaId() != GetCurrentAreaId() ||
        point->GetUniqueId() == self->x902_previousCoverPoint) {
      continue;
    }
    const float distSq = (GetTranslation() - point->GetTranslation()).MagSquared();
    if (distSq >= minDistSq) {
      continue;
    }
    if (point->Blown(self->GetTargetPos(mgr))) {
      continue;
    }
    minDistSq = distSq;
    closest = point;
  }
  if (closest != nullptr) {
    self->ReleaseCoverPoint(mgr, self->x900_coverPoint, true);
    CScriptCoverPoint* point = TCastToPtr< CScriptCoverPoint >(mgr.ObjectById(closest->GetUniqueId()));
    if (point != nullptr) {
      self->SetCoverPoint(point, self->x900_coverPoint);
      result = true;
      self->x902_previousCoverPoint = self->x900_coverPoint;
      self->x914_coverPointRearDir = -closest->GetTransform().GetForward();
    }
  }
  TraceCall(*this, "CoverFind trigger", result ? 1 : 0);
  return result;
}

// fn_72_AA64. Unmarked: no state the 208 cycle reaches lists this trigger (2026-10-20 run).
// Retail: the point that is tested is the pirate's own target position (fn_72_CB30), not the
// player. Inside the minimum attack range (0x3dc) the cover is not blown yet, so the trigger is
// true. Outside it the cover point reports whether that target is inside its safe angle
// (fn_800EC5A0). A cover that is not blown but more than 3 * scaleY away from the pirate (the
// model's Y scale, 0x60) also counts as blown - the asm compares the squared distance with
// 3.0 * scaleY, it does not square the radius.
bool CSpacePirate::CoverBlown(CStateManager& mgr, const CTriggerData& data) const {
  CSpacePirate* self = const_cast< CSpacePirate* >(this);
  bool result = true;
  const CVector3f target = self->GetTargetPos(mgr);
  const CVector3f toTarget = target - GetTranslation();
  if (toTarget.MagSquared() > mMinAttackRange * mMinAttackRange) {
    const CScriptCoverPoint* point = GetCoverPoint(mgr, x900_coverPoint);
    if (point != nullptr) {
      result = point->Blown(target);
      if (!result && x904_steeringSpeed == 0.f &&
          GetBodyController()->GetCurrentStateId() != pas::kAS_Step) {
        const CVector3f toCover = point->GetTranslation() - GetTranslation();
        if (toCover.MagSquared() > 3.f * GetModelData()->GetScale().GetY()) {
          result = true;
        }
      }
    }
  }
  TraceCall(*this, "CoverBlown trigger", result ? 1 : 0);
  return result;
}

// fn_72_A988. Unmarked: no state the 208 cycle reaches lists this trigger (2026-10-20 run).
// Retail: with no cover point the trigger is true. With one it asks whether the cover is blown
// by the attack target's position plus its velocity (1.0 * velocity, so the raw velocity is
// added). The target is the pirate's attack target cast to CPlayer - not mgr.GetPlayer() - so a
// missing or non-player target leaves the trigger false.
bool CSpacePirate::CoverNearlyBlown(CStateManager& mgr, const CTriggerData& data) const {
  bool result = true;
  const CScriptCoverPoint* point = GetCoverPoint(mgr, x900_coverPoint);
  if (point != nullptr) {
    result = false;
    const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mAttackTarget));
    if (player != nullptr) {
      const CVector3f pos = player->GetTranslation() + player->GetVelocityWR();
      result = point->Blown(pos);
    }
  }
  TraceCall(*this, "CoverNearlyBlown trigger", result ? 1 : 0);
  return result;
}

// fn_72_A8D0. Unmarked: no state the 208 cycle reaches lists this trigger (2026-10-20 run).
// Retail: the listening-AI object list (mgr+0x830) is scanned for other pirates; one of them
// that has its in-attack-state bit (0x8f6 mask 0x01) set and is in this area means this pirate
// is covering that one's fire. The scan does not stop at the first hit.
bool CSpacePirate::CoveringFire(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CSpacePirate* pirate = TCastToConstPtr< CSpacePirate >(list[i]);
    if (pirate == nullptr || pirate == this) {
      continue;
    }
    if (!pirate->x8f6_31_inAttackState || pirate->GetCurrentAreaId() != GetCurrentAreaId()) {
      continue;
    }
    result = true;
  }
  TraceCall(*this, "CoveringFire trigger", result ? 1 : 0);
  return result;
}

// fn_72_A71C. Unmarked: no state the 208 cycle reaches lists this trigger (2026-10-20 run).
// The asm casts the attack target to CPlayer (it does not compare ids as MP1 does) and then refuses
// the attack while more than three other pirates, alive and in the attack state in this area, are
// closer to that player than this one.
bool CSpacePirate::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  bool result = true;
  if (TCastToConstPtr< CPlayer >(mgr.GetObjectById(mAttackTarget)) != nullptr) {
    const CVector3f target = const_cast< CSpacePirate* >(this)->GetTargetPos(mgr);
    const float distSq = (GetTranslation() - target).MagSquared();
    int closer = 0;
    const CObjectList& list = mgr.GetObjectListById(kOL_ListeningAi);
    for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
      const CSpacePirate* pirate = TCastToConstPtr< CSpacePirate >(list[i]);
      if (pirate == nullptr || pirate == this || !pirate->x8f6_31_inAttackState ||
          !pirate->GetAlive() || pirate->GetCurrentAreaId() != GetCurrentAreaId()) {
        continue;
      }
      if ((pirate->GetTranslation() - target).MagSquared() < distSq) {
        ++closer;
        if (closer > 3) {
          result = false;
        }
      }
    }
  }
  TraceCall(*this, "ShouldAttack trigger", result ? 1 : 0);
  return result;
}

// Retail reads 0x8f7 bit 4: true while the Think's line-of-sight ray has not lost the player.
bool CSpacePirate::LineOfSight(CStateManager& mgr, const CTriggerData& data) const {
  return !x8f7_28_noPlayerLos;
}

bool CSpacePirate::PatternOver(CStateManager& mgr, const CTriggerData& data) const {
  return GetDestObj() == kInvalidUniqueId;
}

// The detection cone test, over every player: the first one whose offset lies inside
// mDetectionAngle (cosine) of the transform's forward is spotted.
bool CSpacePirate::SpotPlayer(CStateManager& mgr, const CTriggerData& data) const {
  const uint count = mgr.GetNumPlayers();
  for (uint i = 0; i < count; ++i) {
    const CPlayer* player = mgr.GetPlayer(i);
    if (player == nullptr) {
      continue;
    }
    const CVector3f toPlayer = player->GetTranslation() - GetTranslation();
    if (CVector3f::Dot(toPlayer, GetTransform().GetForward()) >
        toPlayer.Magnitude() * mDetectionAngle) {
      return true;
    }
  }
  return false;
}

// @ fn_72_82D0
// Retail reads 0x8f4 bit 0x20, the nonAggressive bit, as MP1 does (the ctor unpacks mFlags 0x4
// into it; mFlags 0x20 is x8f4_29_onlyAttackInRange, bit 0x04).
bool CSpacePirate::ShouldDodge(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  if (x8f8_28_enableDodge) {
    if (!x8f4_26_nonAggressive && !x8f8_29_noPlayerDodge) {
      const CVector3f toTarget =
          const_cast< CSpacePirate* >(this)->GetTargetPos(mgr) - GetTranslation();
      result = CVector3f::Dot(toTarget, GetTransform().GetForward()) > 0.f &&
               (xb24_timeSinceHitByPlayer < 0.33f || xb28_lowHealthFrenzyTimer < 0.33f) &&
               xa7c_timeNoPlayerLos < 0.5f;
    }
    // A dodge also answers a metroid that is attacking this pirate from the front.
    if (!result) {
      const CMetroid* metroid = TCastToConstPtr< CMetroid >(mgr.GetObjectById(mAttackTarget));
      if (metroid != nullptr && metroid->IsAttacking()) {
        const CVector3f delta = GetTranslation() - metroid->GetTranslation();
        if (CVector3f::Dot(delta, metroid->GetTransform().GetForward()) > 0.f) {
          result = true;
        }
      }
    }
  }
  TraceCall(*this, "ShouldDodge trigger", result ? 1 : 0);
  return result;
}

// @ fn_72_7C28
// This trigger writes: the retreat waypoint becomes the destination and the pirate drops its cover
// point, aim and noise state. With no waypoint it retreats to where it already stands, and mDestObj
// keeps whatever GetConnectedObject returned (kInvalidUniqueId), as retail stores that same value.
bool CSpacePirate::ShouldRetreat(CStateManager& mgr, const CTriggerData& data) const {
  if (!x8f6_29_enableRetreat || x8f6_24_trooper) {
    TraceCall(*this, "ShouldRetreat trigger", 0);
    return false;
  }
  CSpacePirate* self = const_cast< CSpacePirate* >(this);
  const TUniqueId wpId = GetConnectedObject(mgr, kSS_Patrol, kSM_Follow);
  const CScriptWaypoint* wp = TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(wpId));
  if (wp == nullptr) {
    const rstl::vector< SConnection >& connections = GetConnectionList();
    for (AUTO(it, connections.begin()); it != connections.end(); ++it) {
      if (it->state != kSS_Patrol || it->msg != kSM_Follow) {
        continue;
      }
      wp = TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(mgr.GetIdForScript(it->objId)));
      if (wp != nullptr) {
        break;
      }
    }
  }
  if (wp != nullptr) {
    self->mDestObj = wpId;
    self->SetDestPos(wp->GetTranslation());
  } else {
    self->mDestObj = kInvalidUniqueId;
    self->SetDestPos(GetTranslation());
  }
  self->x8f6_29_enableRetreat = false;
  self->mReflectedDestPos = GetTranslation();
  self->mInPosition = false;
  self->ReleaseCoverPoint(mgr, self->x900_coverPoint, true);
  self->x8f6_25_hearNoise = false;
  self->mEnableAim = false;
  self->SetHitByPlayerProjectile(false);
  TraceCall(*this, "ShouldRetreat trigger", 1);
  return true;
}

bool CSpacePirate::InRange(CStateManager& mgr, const CTriggerData& data) const {
  return x8f9_27_inRange;
}

// fn_72_99A4. Unmarked: no state the 208 cycle reaches lists this trigger (2026-10-20 run).
// Retail: the pirate's own cover point (0x900) is fetched; with no cover point the trigger is
// false, with one it is whatever the cover point's crouch flag says (fn_800EC6CC, returned
// unmasked).
bool CSpacePirate::ShouldCrouch(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  const CScriptCoverPoint* point = GetCoverPoint(mgr, x900_coverPoint);
  if (point != nullptr) {
    result = point->ShouldCrouch();
  }
  TraceCall(*this, "ShouldCrouch trigger", result ? 1 : 0);
  return result;
}

// fn_72_7BD4. Unmarked: no state the 208 cycle reaches lists this trigger (2026-10-20 run).
bool CSpacePirate::ShouldMove(CStateManager& mgr, const CTriggerData& data) const {
  const CScriptCoverPoint* cover = GetCoverPoint(mgr, x900_coverPoint);
  const bool result = cover != nullptr && !cover->ShouldStay();
  TraceCall(*this, "ShouldMove trigger", result ? 1 : 0);
  return result;
}

// Retail 0xb24/0xb28 are time-since counters (Think zeroes them when a player projectile hits the
// pirate, or is in its path); the trigger's argument is the window, 0.5 s when it is 0.
bool CSpacePirate::ShotAt(CStateManager& mgr, const CTriggerData& data) const {
  const float window = data.GetFloat() != 0.f ? data.GetFloat() : 0.5f;
  return xb28_lowHealthFrenzyTimer < window;
}

// @ fn_72_7B6C
bool CSpacePirate::Attacked(CStateManager& mgr, const CTriggerData& data) const {
  const float window = data.GetFloat() != 0.f ? data.GetFloat() : 0.5f;
  return xb24_timeSinceHitByPlayer < window;
}

// @ fn_72_7970
// The 208 cycle does evaluate this trigger (about 2850 calls in the 2026-10-20 run), but it is
// always false there and the attack target is the player every time, so only the box/near-list
// branch is proven by the run: the team-target branch and the active-non-player early return are
// not.
// Retail: the attack target (0xa94) is cast to CActor and then to CPlayer. An active non-player
// target ends the trigger right there (true, nothing else runs). Otherwise the body runs: with
// no player target the pirate picks a team target (fn_72_EFD4, which returns the id), tells the
// squad (fn_72_DE5C) and points its bone tracking at that target. Then a CAABox around the
// translation - the margin is mData.mSearchRadius (0x7c8) times the rodata constant 1.0 - is
// searched for targeting points, with the filter built from the rodata word lbl_72_data_8A8
// (0x3b = kMT_Solid) as an exclude list (include 0xffffffff, exclude 1 << 59, type 2). The first
// targeting point that is active, in this area and not locked (fn_8012CB70) becomes the attack
// target; the squad is told and the bone tracking follows. The asm breaks out of the loop on
// that first hit (the m2c loop only looks like it does not advance).
bool CSpacePirate::HasTargetingPoint(CStateManager& mgr, const CTriggerData& data) const {
  CSpacePirate* self = const_cast< CSpacePirate* >(this);
  bool result = false;
  const CActor* actor = TCastToConstPtr< CActor >(mgr.GetObjectById(mAttackTarget));
  const CPlayer* player = TCastToConstPtr< CPlayer >(actor);
  if (player == nullptr && actor != nullptr && actor->GetActive()) {
    TraceCall(*this, "HasTargetingPoint trigger", 1);
    return true;
  }
  if (player == nullptr) {
    self->mAttackTarget = self->ChooseTeamTarget(mgr);
    self->UpdateTeamTarget(mgr);
    self->mBoneTracking.SetTarget(self->mAttackTarget);
  }
  const float margin = mData.mSearchRadius * 1.f;
  const CVector3f center = GetTranslation();
  const CAABox bounds(center - CVector3f(margin, margin, margin),
                      center + CVector3f(margin, margin, margin));
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, bounds, CMaterialFilter::MakeExclude(CMaterialList(kMT_Solid)),
                     nullptr);
  for (uint i = 0; i < nearList.size(); ++i) {
    const CScriptTargetingPoint* point =
        TCastToConstPtr< CScriptTargetingPoint >(mgr.GetObjectById(nearList[i]));
    if (point == nullptr || !point->GetActive() ||
        point->GetCurrentAreaId() != GetCurrentAreaId() || point->GetLocked()) {
      continue;
    }
    result = true;
    self->mAttackTarget = point->GetUniqueId();
    self->UpdateTeamTarget(mgr);
    self->mBoneTracking.SetTarget(self->mAttackTarget);
    break;
  }
  TraceCall(*this, "HasTargetingPoint trigger", result ? 1 : 0);
  return result;
}

// fn_72_767C. Unmarked: no state the 208 cycle reaches lists this trigger (2026-10-20 run).
// Retail: the pirate's own cover point (0x900) is fetched; with no cover point the trigger is
// false, with one it is the cover point's wall-hang flag (fn_800EC6B4, tested on the top byte
// of the return value).
bool CSpacePirate::ShouldWallHang(CStateManager& mgr, const CTriggerData& data) const {
  bool result = false;
  const CScriptCoverPoint* point = GetCoverPoint(mgr, x900_coverPoint);
  if (point != nullptr) {
    result = point->ShouldWallHang();
  }
  TraceCall(*this, "ShouldWallHang trigger", result ? 1 : 0);
  return result;
}

bool CSpacePirate::AnimOver(CStateManager& mgr, const CTriggerData& data) const {
  if (x8f7_29_inWallHang) {
    return GetBodyController()->GetCurrentStateId() != pas::kAS_WallHang;
  }
  return CPatterned::AnimOver(mgr, data);
}

// fn_72_CE30. Unmarked: ShouldStrafe never enters the branch that calls it in the 208 cycle
// (the 0.66 s shot windows and the lost-LOS branch never open there).
// Retail's step-direction picker for the strafe trigger: a side is dropped when another pirate of
// the same area is that way (dot against the transform's right axis, in the squared-distance frame
// MP1 uses), then the survivors must be on the path find mesh and have a clear dynamic ray. Both
// sides clear is a coin flip on CRandom16 bit 0x4000. Returns pas::EStepDirection.
int CSpacePirate::GetStrafeDir(CStateManager& mgr, float dist) {
  const float distSquared = dist * dist;
  bool canStrafeLeft = true;
  bool canStrafeRight = true;
  CObjectList& list = mgr.ObjectListById(kOL_ListeningAi);
  for (int i = list.GetFirstObjectIndex(); i != -1; i = list.GetNextObjectIndex(i)) {
    const CSpacePirate* other = TCastToConstPtr< CSpacePirate >(list[i]);
    if (other == nullptr || other == this || other->GetCurrentAreaId() != GetCurrentAreaId()) {
      continue;
    }
    const CVector3f delta = other->GetTranslation() - GetTranslation();
    const float magSquared = delta.MagSquared();
    if (magSquared >= distSquared) {
      continue;
    }
    const float rightDot = CVector3f::Dot(delta, GetTransform().GetRight());
    if (rightDot > 0.866f * magSquared || (rightDot > 0.f && magSquared < 3.f)) {
      canStrafeRight = false;
    } else if (rightDot < 0.866f * -magSquared || (rightDot < 0.f && magSquared < 3.f)) {
      canStrafeLeft = false;
    }
  }

  const CVector3f center = GetBoundingBox().GetCenterPoint();
  const CVector3f right = GetTransform().GetRight();
  if (canStrafeRight) {
    canStrafeRight =
        mPathFindSearch.OnPath(center + dist * right) == CPathFindSearch::kR_Success;
  }
  if (canStrafeLeft) {
    canStrafeLeft = mPathFindSearch.OnPath(center - dist * right) == CPathFindSearch::kR_Success;
  }
  if (!canStrafeLeft && !canStrafeRight) {
    TraceCall(*this, "GetStrafeDir", pas::kSD_Invalid);
    return pas::kSD_Invalid;
  }

  // The near list is anchored on the centre when the left side is open and on the left end point
  // otherwise; its length doubles when both sides are still in play (0xB60 = 2).
  const CVector3f startPoint = canStrafeLeft ? center : center - dist * right;
  const float length = canStrafeLeft && canStrafeRight ? 2.f * dist : dist;
  const CMaterialFilter filter = CMaterialFilter::MakeInclude(CMaterialList(kMT_Solid));
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, startPoint, right, length, filter, this);
  if (canStrafeLeft) {
    canStrafeLeft = CGameCollision::RayDynamicLineOfSightTest(mgr, startPoint, right, dist, filter,
                                                             nearList, nullptr);
  }
  if (canStrafeRight) {
    canStrafeRight = CGameCollision::RayDynamicLineOfSightTest(mgr, center, right, dist, filter,
                                                               nearList, nullptr);
  }
  if (canStrafeLeft && canStrafeRight) {
    if ((mgr.Random()->Next() & 0x4000) != 0) {
      canStrafeLeft = false;
    } else {
      canStrafeRight = false;
    }
  }

  int straightDir = pas::kSD_Invalid;
  if (canStrafeLeft) {
    straightDir = pas::kSD_Left;
  } else if (canStrafeRight) {
    straightDir = pas::kSD_Right;
  }
  TraceCall(*this, "GetStrafeDir", straightDir);
  return straightDir;
}

// @ fn_72_960C
// Two ways to skid: a hurt or frenzied pirate whose target is ahead and close to its own axis
// strafes at 10 m (0xB58) around a 0.66 s (0xC04) window and a zero delay timer; failing that, a
// player the pirate has lost line of sight to for over a second and that stands within 15 m
// (0xB88) makes it strafe at 5 m (0xB78). Either way -1 leaves it facing the target.
bool CSpacePirate::ShouldStrafe(CStateManager& mgr, const CTriggerData& data) const {
  CSpacePirate* self = const_cast< CSpacePirate* >(this);
  self->xb08_skidDir = -1;
  bool result = false;
  bool noPlayerStrafe = false;
  if (!x8f4_26_nonAggressive) {
    const CVector3f toTarget = self->GetTargetPos(mgr) - GetTranslation();
    if (CVector3f::Dot(toTarget, GetTransform().GetForward()) > 0.f) {
      if ((xb28_lowHealthFrenzyTimer < 0.66f || xb24_timeSinceHitByPlayer < 0.66f) &&
          xb0c_strafeDelayTimer == 0.f) {
        const CVector3f center = GetBoundingBox().GetCenterPoint();
        const CVector3f toTargetFromCenter = (self->GetTargetPos(mgr) - center).AsNormalized();
        if (CVector3f::Dot(toTargetFromCenter, GetTransform().GetForward()) > 0.707f) {
          self->xb08_skidDir = self->GetStrafeDir(mgr, 10.f);
          if (self->xb08_skidDir != -1) {
            result = true;
          } else {
            noPlayerStrafe = true;
          }
        }
      }
      if (!noPlayerStrafe && !result && xa7c_timeNoPlayerLos > 1.f) {
        const CPlayer* player = TCastToConstPtr< CPlayer >(mgr.GetObjectById(mAttackTarget));
        if (player != nullptr) {
          const CVector3f delta = player->GetTranslation() - GetTranslation();
          if (delta.Magnitude() < 15.f && self->xb08_skidDir == -1) {
            self->xb08_skidDir = self->GetStrafeDir(mgr, 5.f);
            if (self->xb08_skidDir != -1) {
              result = true;
            }
          }
        }
      }
    }
  }
  TraceCall(*this, "ShouldStrafe trigger", result ? 1 : 0);
  return result;
}

bool CSpacePirate::ShouldSpecialAttack(CStateManager& mgr, const CTriggerData& data) const {
  return x8f4_29_onlyAttackInRange && !mBurstFire.IsBurstSet() && mAttackRemTime > 2.f;
}

bool CSpacePirate::StartAttack(CStateManager& mgr, const CTriggerData& data) const {
  if (x8f8_31_mayStartAttack) {
    const_cast< CSpacePirate* >(this)->x8f8_31_mayStartAttack = false;
    return true;
  }
  return false;
}

bool CSpacePirate::BreakAttack(CStateManager& mgr, const CTriggerData& data) const {
  return x8f5_25_breakAttack;
}

bool CSpacePirate::LostInterest(CStateManager& mgr, const CTriggerData& data) const {
  return x8f4_29_onlyAttackInRange && mAttackRemTime < 1.5f;
}

bool CSpacePirate::BounceFind(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::BounceFind trigger (fn_72_602C)");
  return false;
}

bool CSpacePirate::OffLine(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::OffLine trigger (fn_72_55B8)");
  return false;
}

bool CSpacePirate::Landed(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::Landed trigger (fn_72_558C)");
  return false;
}

bool CSpacePirate::ShouldJumpBack(CStateManager& mgr, const CTriggerData& data) const {
  return !x8f4_28_noShuffleCloseCheck || xbac_holdPositionTime > 6.f;
}

bool CSpacePirate::Leash(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::Leash trigger (fn_72_52F8)");
  return false;
}

bool CSpacePirate::IsAmbushing(CStateManager& mgr, const CTriggerData& data) const {
  return x8f4_24_pendingAmbush;
}

bool CSpacePirate::ShouldWarpIn(CStateManager& mgr, const CTriggerData& data) const {
  return xc04_warpPoint == -1 && x8f6_24_trooper && x8fa_26_;
}

// Retail fn_72_779C (0x2C): the launcher block's +0x00 is 1 - the loadout is the grenade launcher -
// and the attack timer has run out (rodata B9C = 0, compared with the `cror lt/eq` idiom, so `<= 0`).
bool CSpacePirate::ShouldLaunchGrenade(CStateManager& mgr, const CTriggerData& data) const {
  return mData.mEquippedWeapon == 1 && mAttackRemTime <= 0.f;
}

// Retail fn_72_76D0 (0xD4): a switch on the same launcher word. Unarmed (0) is always in range,
// anything but 0/1 never is, and the grenade launcher (1) wants a live attack target whose
// squared 3-D distance to the pirate falls inside [minAttackDist, maxAttackDist] squared
// (data +0x12c/+0x130, rodata-free: both bounds are inclusive `cror ge`/`cror le` tests).
bool CSpacePirate::InProjectileRange(CStateManager& mgr, const CTriggerData& data) const {
  switch (mData.mEquippedWeapon) {
  case 0:
    return true;
  case 1: {
    const CActor* target = TCastToConstPtr< CActor >(mgr.GetObjectById(mAttackTarget));
    if (target == nullptr) {
      return false;
    }
    const float distSq = (target->GetTranslation() - GetTranslation()).MagSquared();
    return distSq >= mData.mGrenadeMinAttackDist * mData.mGrenadeMinAttackDist &&
           distSq <= mData.mGrenadeMaxAttackDist * mData.mGrenadeMaxAttackDist;
  }
  default:
    return false;
  }
}

// Retail fn_72_7810 (0x54): the pirate has an attack pattern when its ATTK/FOLW connection
// resolves to a live object (asm: GetConnectedObject, then the `id != kInvalidUniqueId` idiom).
bool CSpacePirate::HasAttackPattern(CStateManager& mgr, const CTriggerData& data) const {
  return GetConnectedObject(mgr, kSS_Attack, kSM_Follow) != kInvalidUniqueId;
}
