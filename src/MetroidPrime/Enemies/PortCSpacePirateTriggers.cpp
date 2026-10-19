// The 39 triggers of the SpacePirate state machine (retail table fn_72_10E50 ->
// lbl_72_data_1D8). The trivial retail bodies (under 0x100 bytes, read from
// build/G2ME01/SpacePirate/m2c/fn_72_<off>.c) are translated; every other one returns false behind
// a once-per-name StandIn that names the retail function. Triggers that read a flag bit use the
// names in CSpacePirate.hpp (MP1's where the mask matches).
#include "MetroidPrime/Enemies/CSpacePirate.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/StateMachineCommon.hpp"

void PortSpacePirateStandIn(const char* what);

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

bool CSpacePirate::ShouldAttack(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::ShouldAttack trigger (fn_72_A71C)");
  return false;
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

bool CSpacePirate::ShouldDodge(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::ShouldDodge trigger (fn_72_82D0)");
  return false;
}

bool CSpacePirate::ShouldRetreat(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::ShouldRetreat trigger (fn_72_7C28)");
  return false;
}

bool CSpacePirate::InRange(CStateManager& mgr, const CTriggerData& data) const {
  return x8f9_27_inRange;
}

bool CSpacePirate::ShouldCrouch(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::ShouldCrouch trigger (fn_72_99A4)");
  return false;
}

bool CSpacePirate::ShouldMove(CStateManager& mgr, const CTriggerData& data) const {
  PortSpacePirateStandIn("CSpacePirate::ShouldMove trigger (fn_72_7BD4)");
  return false;
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
