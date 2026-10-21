#include "MetroidPrime/Enemies/CSandworm.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CBouncingBomb.hpp"

// WP6 of the retail Sandworm REL (module 56, G2ME01): the Bomb Guardian bombs. The three bomb
// attack states and their Picked* triggers, the CBouncingBomb spawns (patrol drop, toss, fountain,
// held bomb), the held-bomb handling and the bomb parameter block at x1944. The bombs are spawned
// from DoUserAnimEvent (WP4); the states only hold, aim and clean up.
//
// Held-bomb state: x1998_ is the bomb's id and x199A_ is read and written as one byte (1 = the bomb
// sits in the mouth and follows x1528_spit.x28_xf, the mouth transform).

namespace {

// One spine point of x99C, turned about the vertical axis through x1388. Retail x99C is a
// reserved_vector<CVector3f, 180> (count at 0x99C, 0xC-byte entries, capacity 0xB4).
void RotateSpinePoint(CVector3f& point, const CTransform4f& rot, const CVector3f& pivot) {
  const CVector3f turned =
      rot.Rotate(CVector3f(point.GetX() - pivot.GetX(), point.GetY() - pivot.GetY(), 0.f));
  point = CVector3f(turned.GetX() + pivot.GetX(), turned.GetY() + pivot.GetY(),
                    (point.GetZ() - pivot.GetZ()) + pivot.GetZ());
}

} // namespace

// Spawns at the mouth transform x1528_spit.x28_xf.
// @ fn_56_9030
CBouncingBomb* CSandworm::SpawnBouncingBomb(CStateManager& mgr, float fuseTime, float touchRadius,
                                            float gravityScale, float bounceRestitution) {
  CBouncingBomb* bomb = rs_new CBouncingBomb(
      TToken< CGenDescription >(*x1944_bombs.x0_bombFx),
      TToken< CGenDescription >(*x1944_bombs.x10_explosionFx), mgr.AllocateUniqueId(),
      GetCurrentAreaId(), GetUniqueId(), fuseTime, touchRadius, kWT_Bomb, 0x100,
      x1528_spit.x28_xf, x1944_bombs.x20_damage, 8.f, GetInvalidSfx(),
      GetBombBounceSfx(), GetBombExplodeSfx(), gravityScale, bounceRestitution);
  mgr.AddObject(bomb);
  return bomb;
}

// fn_56_F328 is CVector3f::Cross.
// Turns the whole worm about the vertical axis through x1388 so that the direction x1388 -> x1394
// swings toward `target`, by at most `rate` degrees per second (slowed inside 0.2 rad).
// @ fn_56_EF8C
// F328 has no body of its own: the REL's fn_56_F328 is the inlined CVector3f::Cross above.
void CSandworm::RotateBodyForBombAttack(CStateManager& mgr, const CVector3f& target,
                                        CVector3f& pivot, float rate, float dt) {
  const CVector3f& center = x1388_;
  CVector3f toTarget(target.GetX() - center.GetX(), target.GetY() - center.GetY(), 0.f);
  CVector3f facing(x1394_snatchStart.GetX() - center.GetX(),
                   x1394_snatchStart.GetY() - center.GetY(), 0.f);
  if (!toTarget.CanBeNormalized() || !facing.CanBeNormalized()) {
    return;
  }
  toTarget.Normalize();
  facing.Normalize();

  float angle = dt * (0.017453292f * rate);
  if (CVector3f::Cross(toTarget, facing).GetZ() > 0.f) {
    angle *= -1.f;
  }
  const float diff = CVector3f::GetAngleDiff(toTarget, facing);
  if (diff < 0.2f) {
    angle *= diff / 0.2f;
  }
  const CRelAngle relAngle = CRelAngle::FromRadians(angle);
  const CTransform4f rot = CTransform4f::RotateZ(relAngle);

  for (int i = 0; i < x99C_joints.size(); ++i) {
    RotateSpinePoint(x99C_joints[i], rot, center);
  }

  CTransform4f bodyRot = GetTransform().GetRotation();
  bodyRot.RotateLocalZ(relAngle);
  CVector3f pos = rot.Rotate(GetTranslation() - center) + center;
  pos.SetZ(SpineRenderHelperD690(mgr, pos));
  CTransform4f xf = GetTransform();
  xf.SetTranslation(pos);
  xf.SetRotation(bodyRot.GetRotation());
  SetTransform(xf);

  pivot = rot.Rotate(pivot - center) + center;
}


// SSandwormBombs (x1944)

// @ fn_56_1E9C
SSandwormBombs::SSandwormBombs() {}

// @ fn_56_1FD8
SSandwormBombs::SSandwormBombs(CAssetId bombFx, CAssetId explosionFx, const CDamageInfo& damage)
: x20_damage(damage) {
  // Retail does not test the ids for kInvalidAssetId.
  x0_bombFx = TLockedToken< CGenDescription >(
      TToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', bombFx))));
  x10_explosionFx = TLockedToken< CGenDescription >(
      TToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', explosionFx))));
}

// @ fn_56_1280C
SSandwormBombs::~SSandwormBombs() {}

// Triggers

// @ fn_56_3750
bool CSandworm::PickedBombFountain(CStateManager&, const CTriggerData&) const {
  return x1490_attacks.x0_picked == 7;
}

// @ fn_56_3764
bool CSandworm::PickedBombSpread(CStateManager&, const CTriggerData&) const {
  return x1490_attacks.x0_picked == 6;
}

// @ fn_56_3778
bool CSandworm::PickedBombToss(CStateManager&, const CTriggerData&) const {
  return x1490_attacks.x0_picked == 5;
}

// Patrol/Pursue bomb drop

// @ fn_56_3A8C
void CSandworm::UpdateBombDrop(CStateManager& mgr, float dt) {
  if (!CanDropBomb(mgr)) {
    x1984_bombDropTimer = 0.f;
    return;
  }
  x1984_bombDropTimer += dt;
  if (x1984_bombDropTimer > x1980_bombDropRate) {
    x1984_bombDropTimer -= x1980_bombDropRate;
    // Retail drops at x1614 (named for its residue use in fn_56_11FAC).
    DropBouncingBomb(mgr, x1614_residuePosHighHealth);
  }
}

// @ fn_56_3B24
bool CSandworm::CanDropBomb(CStateManager& mgr) const {
  return IsPossessedBombGuardian() && x1980_bombDropRate > 0.f && !AnyEyeDead(mgr);
}

// @ fn_56_3B9C
void CSandworm::ResetBombDropTimer() { x1984_bombDropTimer = 0.f; }

// @ fn_56_3BAC
CBouncingBomb* CSandworm::DropBouncingBomb(CStateManager& mgr, const CVector3f& pos) {
  CBouncingBomb* bomb = rs_new CBouncingBomb(
      TToken< CGenDescription >(*x1944_bombs.x0_bombFx),
      TToken< CGenDescription >(*x1944_bombs.x10_explosionFx), mgr.AllocateUniqueId(),
      GetCurrentAreaId(), GetUniqueId(), 1.f, 3.f, kWT_Bomb, 0x100, CTransform4f::Translate(pos),
      x1944_bombs.x20_damage, 8.f, GetInvalidSfx(), GetBombBounceSfx(), GetBombExplodeSfx(), 10.f,
      0.01f);
  mgr.AddObject(bomb);
  x1988_ = x8B0_activeTime;
  return bomb;
}

// Anim-event spawns (called from DoUserAnimEvent)

// @ fn_56_8E90
// Run 2026-10-21 (03_sand_temple L2, u048f forced into BombFountainAttack): the anim event's
// projectile case with the current attack 7 spawns the toss bomb - 27 calls. The else arm of the
// normalise test is unreachable: the vector's z is Range(1.0, 1.2), so it is never zero.
void CSandworm::SpawnBombToss(CStateManager& mgr) {
  CBouncingBomb* bomb = SpawnBouncingBomb(mgr, 12.f, 3.f, 1.f, 0.7f);
  // Retail draws z, then y, then x.
  const float z = mgr.Random()->Range(1.f, 1.2f);
  const float y = mgr.Random()->Range(-1.f, 1.f);
  const float x = mgr.Random()->Range(-1.f, 1.f);
  CVector3f dir(x, y, z);
  if (dir.CanBeNormalized()) {
    dir.Normalize();
  } else {
    dir = CVector3f::Up();
  }
  dir = dir * 25.f;
  dir = dir * mgr.Random()->Range(0.5f, 1.f);
  bomb->SetVelocity(dir);
  bomb->ApplyGravity();
}

// @ fn_56_91D4
void CSandworm::SpawnBombFountain(CStateManager& mgr) {
  CBouncingBomb* bomb = SpawnBouncingBomb(mgr, 10.f, 5.f, 1.f, 0.65f);
  const CVector3f fwd = x1528_spit.x28_xf.GetColumn(kDY);
  CVector3f dir(fwd.GetX(), fwd.GetY(), 0.f);
  if (dir.CanBeNormalized()) {
    dir.Normalize();
  }
  dir.SetZ(0.5f);
  dir.Normalize();

  const float dist = (GetTranslation() - mgr.Player(0)->GetTranslation()).Magnitude();
  float scale = 1.f;
  if (dist > 10.f) {
    if (dist > 40.f) {
      scale = 2.f;
    } else {
      scale = 1.f + (dist - 10.f) / 30.f;
    }
  }
  bomb->SetVelocity(dir * (15.f * scale));
  bomb->ApplyGravity();
}

// @ fn_56_9368
void CSandworm::AttachHeldBomb(CStateManager& mgr) {
  if (x199A_ == 0) {
    return;
  }
  CBouncingBomb* bomb = GetHeldBomb(mgr);
  if (bomb == nullptr) {
    return;
  }
  CVector3f fwd = x1528_spit.x28_xf.GetColumn(kDY);
  if (fwd.CanBeNormalized()) {
    fwd.Normalize();
  }
  bomb->SetTranslation(x1528_spit.x28_xf.GetTranslation() + fwd * 0.9f);
}

// @ fn_56_9440
void CSandworm::ReleaseHeldBomb(CStateManager& mgr) {
  CBouncingBomb* bomb = GetHeldBomb(mgr);
  if (bomb == nullptr) {
    return;
  }
  const CVector3f target = mgr.Player(0)->GetTranslation() + CVector3f(0.f, 0.f, -3.f);
  CVector3f dir = target - bomb->GetTranslation();
  if (dir.CanBeNormalized()) {
    dir.Normalize();
  }
  bomb->SetVelocity(dir * 35.f);
  x199A_ = 0;
}

// @ fn_56_9590
CBouncingBomb* CSandworm::GetHeldBomb(CStateManager& mgr) const {
  if (x1998_ == kInvalidUniqueId) {
    return nullptr;
  }
  return TCastToPtr< CBouncingBomb >(mgr.ObjectById(x1998_));
}

// fn_56_95E0 (ported, not exercised yet)
// Data-gated: the BG prefab never selects kind 4 (picked 7), so this cannot run; see Sandworm.md Port status.
void CSandworm::DeleteHeldBomb(CStateManager& mgr) {
  if (x1998_ != kInvalidUniqueId) {
    mgr.DeleteObjectRequest(x1998_);
    x1998_ = kInvalidUniqueId;
  }
}

// @ fn_56_9640
ushort CSandworm::GetBombExplodeSfx() const { return x1828_sfx.xa_bombExplode; }

// @ fn_56_9648
ushort CSandworm::GetBombBounceSfx() const { return x1828_sfx.x8_bombBounce; }

// @ fn_56_9650
ushort CSandworm::GetInvalidSfx() const { return CSfxManager::kInternalInvalidSfxId; }

// @ fn_56_9660
void CSandworm::SpawnHeldBomb(CStateManager& mgr) {
  CBouncingBomb* bomb = SpawnBouncingBomb(mgr, 10.f, 3.f, 1.f, 0.8f);
  x1998_ = bomb->GetUniqueId();
  x199A_ = 1;
  AttachHeldBomb(mgr);
}

// States

// @ fn_56_B8B8
// Run 2026-10-21 (03_sand_temple L2, u048f forced into BombFountainAttack): 481 calls, so the
// activate, the update and the deactivate arm all ran. The old "data-gated" note was wrong: the
// BG prefab's AFSM has the BombFountainAttack state, and forcing it (console `fsm`) runs the
// picked-7 chain that the prefab's own attack pick never chooses.
void CSandworm::BombFountainAttack(CStateManager& mgr, EStateMsg msg, float) {
  BeginAttack(7, msg);
  x1520_flags |= 0x80;
  if (msg == kStateMsg_Activate) {
    x19A8_bombPhase.x0_ = GetTranslation();
    x19A8_bombPhase.xc_flags &= ~0xC0;
    x19A8_bombPhase.x14_ = GetHealthInfo()->GetHP();
    SetBodyCmdRequest(5, 13);
    if (x19D0_glow.x1c_gen3 != nullptr) {
      x19D0_glow.x1c_gen3->SetParticleEmission(false);
    }
  } else if (msg == kStateMsg_Deactivate) {
    ResetAttackTimer(mgr);
    if (x199A_ == 1) {
      DeleteHeldBomb(mgr);
    }
    SetTranslation(x19A8_bombPhase.x0_);
    x19A8_bombPhase.xc_flags &= ~0xC0;
    x19A8_bombPhase.x20_ = 0.f;
    x19A8_bombPhase.x1c_ = 0.f;
    x19A8_bombPhase.x14_ = 0.f;
    x19A8_bombPhase.x0_ = CVector3f::Zero();
    x1490_attacks.x0_picked = -1;
  } else {
    AttachHeldBomb(mgr);
  }
  IssueMeleeAttackCmd(msg, pas::kAS_MeleeAttack, CBCMeleeAttackCmd(pas::kS_Four));
}

// @ fn_56_BAB8
void CSandworm::BombSpreadAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  BeginAttack(6, msg);
  x1520_flags |= 0x80;
  if (msg == kStateMsg_Activate) {
    x199C_ = GetTranslation();
    SetBodyCmdRequest(3, 13);
    x1A08_ = 1.3f;
  } else if (msg == kStateMsg_Deactivate) {
    ResetAttackTimer(mgr);
    SetTranslation(x199C_);
    x199C_ = CVector3f::Zero();
  } else if (x1A08_ > 0.f) {
    x1A08_ -= dt;
    const CVector3f target = mgr.Player(0)->GetTranslation();
    RotateBodyForBombAttack(mgr, target, x199C_, 100.f, dt);
  }
  IssueMeleeAttackCmd(msg, pas::kAS_MeleeAttack, CBCMeleeAttackCmd(pas::kS_Nine));
}

// @ fn_56_BC8C
void CSandworm::BombTossAttack(CStateManager& mgr, EStateMsg msg, float dt) {
  BeginAttack(5, msg);
  x1520_flags |= 0x80;
  if (msg == kStateMsg_Activate) {
    x198C_ = GetTranslation();
    SetBodyCmdRequest(3, 13);
    x1A08_ = 1.3f;
  } else if (msg == kStateMsg_Deactivate) {
    ResetAttackTimer(mgr);
    if (x199A_ == 1) {
      DeleteHeldBomb(mgr);
    }
    SetTranslation(x198C_);
    x198C_ = CVector3f::Zero();
    x1998_ = kInvalidUniqueId;
    x199A_ = 0;
  } else {
    AttachHeldBomb(mgr);
    if (x1A08_ > 0.f) {
      x1A08_ -= dt;
      const CVector3f target = mgr.Player(0)->GetTranslation();
      RotateBodyForBombAttack(mgr, target, x198C_, 100.f, dt);
    }
  }
  IssueMeleeAttackCmd(msg, pas::kAS_MeleeAttack, CBCMeleeAttackCmd(pas::kS_Eight));
}
