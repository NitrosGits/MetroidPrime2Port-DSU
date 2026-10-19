// The 39 triggers of the SpacePirate state machine (retail table fn_72_10E50 ->
// lbl_72_data_1D8). The trivial retail bodies (under 0x100 bytes, read from
// build/G2ME01/SpacePirate/m2c/fn_72_<off>.c) are translated; every other one returns false behind
// a once-per-name StandIn that names the retail function. Triggers that read a flag bit use the
// names in CSpacePirate.hpp (MP1's where the mask matches).
#include "MetroidPrime/Enemies/CSpacePirate.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CMetroid.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptObjects/CScriptCoverPoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/StateMachineCommon.hpp"
#include "MetroidPrime/TCastTo.hpp"

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

bool CSpacePirate::AggressionCheck(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::AggressionCheck trigger (fn_72_ADC0)");
  return false;
}

bool CSpacePirate::CoverCheck(CStateManager& mgr, const CTriggerData& data) const {
  return x8f8_27_coverCheck;
}

bool CSpacePirate::CoverFind(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::CoverFind trigger (fn_72_ABC0)");
  return false;
}

bool CSpacePirate::CoverBlown(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::CoverBlown trigger (fn_72_AA64)");
  return false;
}

bool CSpacePirate::CoverNearlyBlown(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::CoverNearlyBlown trigger (fn_72_A988)");
  return false;
}

bool CSpacePirate::CoveringFire(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::CoveringFire trigger (fn_72_A8D0)");
  return false;
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

bool CSpacePirate::ShouldCrouch(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::ShouldCrouch trigger (fn_72_99A4)");
  return false;
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

bool CSpacePirate::HasTargetingPoint(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::HasTargetingPoint trigger (fn_72_7970)");
  return false;
}

bool CSpacePirate::ShouldWallHang(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::ShouldWallHang trigger (fn_72_767C)");
  return false;
}

bool CSpacePirate::AnimOver(CStateManager& mgr, const CTriggerData& data) const {
  if (x8f7_29_inWallHang) {
    return GetBodyController()->GetCurrentStateId() != pas::kAS_WallHang;
  }
  return CPatterned::AnimOver(mgr, data);
}

bool CSpacePirate::ShouldStrafe(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::ShouldStrafe trigger (fn_72_960C)");
  return false;
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

bool CSpacePirate::HasAttackPattern(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::HasAttackPattern trigger (fn_72_7810)");
  return false;
}

bool CSpacePirate::IsAmbushing(CStateManager& mgr, const CTriggerData& data) const {
  return x8f4_24_pendingAmbush;
}

bool CSpacePirate::ShouldWarpIn(CStateManager& mgr, const CTriggerData& data) const {
  return xc04_warpPoint == -1 && x8f6_24_trooper && x8fa_26_;
}

bool CSpacePirate::ShouldLaunchGrenade(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::ShouldLaunchGrenade trigger (fn_72_779C)");
  return false;
}

bool CSpacePirate::InProjectileRange(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::InProjectileRange trigger (fn_72_76D0)");
  return false;
}
