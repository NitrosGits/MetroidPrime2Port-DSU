// Host body for the SandBoss REL (module 55, Amorbis): the attack helpers and the charge-beam
// projectile. The stampede helpers and the charge-beam list (fn_55_26C0..fn_55_311C) follow retail
// (`// fn_55_X (ported, not exercised yet)`); every other function here is still a log-once stand-in
// (bool helpers return false, the rest do nothing).
// docs/module-analysis/SandBoss.md section 4 is the function map.
#include "MetroidPrime/Enemies/CSandBoss.hpp"

#include "Collision/CRayCastResult.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Collision/CMaterialList.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Math/CAABox.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptObjects/CScriptEffect.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/Weapons/CBeamInfo.hpp"
#include "MetroidPrime/Weapons/CBomb.hpp"
#include "MetroidPrime/Weapons/CWeaponAssetInfo.hpp"
#include "MetroidPrime/TCastTo.hpp"

void PortSandBossStandIn(const char* what);
CSandBoss* CastSandBoss(CEntity* entity);
const CSandBoss* CastSandBoss(const CEntity* entity);

// fn_55_107B0 (ported, not exercised yet)
// The charge beam: a light plasma projectile with a zeroed damage info (fn_55_2750 applies the
// double/triple-charge damage itself), no weapon assets, a 1-second damage duration and the plane
// its own ray check clips to (fn_55_1027C).
SandBossChargeBeam::SandBossChargeBeam(const TToken< CWeaponDescription >& description,
                                       const CBeamInfo& beamInfo, TUniqueId uid, TAreaId areaId,
                                       TUniqueId owner, const CPlane& plane)
: CPlasmaProjectile(description, rstl::string_l("SandBossChargeBeam"), kWT_Light, beamInfo,
                    CTransform4f::Identity(), kMT_ProjectilePassthrough, CDamageInfo(), uid, areaId,
                    owner, CWeaponAssetInfo(), true,
                    CWeapon::kPA_KeepInCinematic | CWeapon::kPA_BigStrike)
, mPlane(plane) {
  SetDamageDuration(1.f); // retail stores 1.0 in the weapon's duration field after construction
}

SandBossChargeBeam::~SandBossChargeBeam() {} // fn_55_108E4 / fn_55_10C18

// fn_55_1027C (ported, not exercised yet)
// The beam does not raycast the world: it clips its segment against the plane stored at 0x6A8 and
// then against the player (CGameProjectile::CanCollideWith), keeping whichever hit is nearer. A
// player hit touches the player and records the actor id in idOut and in the projectile's
// last-resolved field (retail 0x3F6). The near list and the static-geometry test are unused.
CRayCastResult SandBossChargeBeam::RayCollisionCheckWithWorld(
    TUniqueId& idOut, const CVector3f& start, const CVector3f& end, float magnitude,
    rstl::reserved_vector< TUniqueId, 1024 >&, CStateManager& mgr, EStaticGeometryTest) {
  idOut = kInvalidUniqueId;
  SetLastResolvedObject(kInvalidUniqueId);
  CRayCastResult result;
  const CVector3f delta = end - start;
  if (!delta.IsMagnitudeSafe()) {
    return result;
  }
  const CVector3f direction = delta.AsNormalized();
  const float denominator = CVector3f::Dot(direction, mPlane.GetNormal());
  if (denominator < 0.f) { // the segment runs into the plane
    const float time =
        -(CVector3f::Dot(start, mPlane.GetNormal()) - mPlane.GetConstant()) / denominator;
    if (time < magnitude) {
      result = CRayCastResult(time, start + time * direction, mPlane, CMaterialList(kMT_Solid));
      magnitude = time;
    }
  }
  CPlayer* player = mgr.GetPlayer(0);
  if (player != nullptr) {
    const CProjectileTouchResult touch = CanCollideWith(*player, mgr);
    if (touch.HasRayCastResult() && touch.GetRayCastResult().GetTime() < magnitude) {
      player->Touch(*this, mgr); // vtable +0x4C [inferred: CActor::Touch]
      result = touch.GetRayCastResult();
      idOut = touch.GetActorId();
      SetLastResolvedObject(idOut);
    }
  }
  return result;
}

// @ fn_55_1AEC
// How long to wait for the other worms before stampeding: three per-sibling delays in the
// stampede loader data, picked by how many siblings are already stampeding (0x165C b4).
float CSandBoss::GetStampedeDelay(CStateManager& mgr) const {
  int stampeding = 0;
  for (int i = 0; i < mSiblings.size(); ++i) { // 0xDD8 / 0xDDC
    const CSandBoss* sibling = CastSandBoss(mgr.GetObjectById(mSiblings[i]));
    if (sibling != nullptr && sibling->mStampedeActive) {
      ++stampeding;
    }
  }
  if (stampeding == 3) {
    return mData.stampedeProperties.unknown_0x5fb66017; // 0x834
  }
  if (stampeding == 2) {
    return mData.stampedeProperties.unknown_0xc2b98161; // 0x830
  }
  return mData.stampedeProperties.unknown_0xbed8a4ba; // 0x82C
}

// @ fn_55_1B94
// Counts the safe zones this worm is still wired to: every Attack/Attach connection whose target
// resolves to a CScriptSafeZone that is active (retail reads bit7 of the flag byte at +0x20, the
// big-endian first bitfield word, which CEntity names mActive).
int CSandBoss::ShouldRepeaterAttackCheck(CStateManager& mgr) const {
  int active = 0;
  const rstl::vector< SConnection >& connections = GetConnectionList(); // CEntity 0x10
  for (int i = 0; i < connections.size(); ++i) {
    const SConnection& connection = connections[i];
    if (connection.state != kSS_Attack || connection.msg != kSM_Attach) { // 0x4154544b / 0x41544348
      continue;
    }
    const CScriptSafeZone* safeZone = TCastToPtr< CScriptSafeZone >(
        mgr.ObjectById(mgr.GetIdForScript(connection.objId)));
    if (safeZone != nullptr && safeZone->GetActive()) {
      ++active;
    }
  }
  return active;
}

// fn_55_1C70 (ported, not exercised yet)
// Synchronises the group's next volley: every attached worm's beam cooldown (0xF44) is set to the
// smallest cooldown among this worm and its attached siblings plus delta, and each worm re-seeds
// its attack timers.
void CSandBoss::SyncAttacker(CStateManager& mgr, int delta) {
  int minimum = mChargeBeamCooldown; // 0xF44
  for (int i = 0; i < mSiblings.size(); ++i) {                          // 0xDD8 / 0xDDC
    const CSandBoss* sibling = CastSandBoss(mgr.GetObjectById(mSiblings[i]));
    if (sibling != nullptr && sibling != this && sibling->mAttachedToSphere && // 0x165D b7
        sibling->mChargeBeamCooldown < minimum) {
      minimum = sibling->mChargeBeamCooldown;
    }
  }
  for (int i = 0; i < mSiblings.size(); ++i) {
    CSandBoss* sibling = CastSandBoss(mgr.ObjectById(mSiblings[i]));
    if (sibling != nullptr && sibling->mAttachedToSphere) {
      sibling->mChargeBeamCooldown = minimum + delta;
      sibling->ResetAttackTimes(mgr, 0.f); // fn_55_8E60 (its argument is unused)
    }
  }
}

void CSandBoss::RotateToPlayerStep(CStateManager&, const CVector3f&, float) { // fn_55_22E4
  PortSandBossStandIn("CSandBoss::RotateToPlayerStep (fn_55_22E4)");
}

void CSandBoss::RotateHelper1D74(CStateManager&, float) { // fn_55_1D74
  PortSandBossStandIn("CSandBoss::RotateHelper1D74 (fn_55_1D74)");
}

// fn_55_1F84 (ported, not exercised yet)
// Turns the body toward a world point at the controller's turn rate (only for a positive dt and a
// point away from this worm).
void CSandBoss::FaceTowardPoint(float dt, const CVector3f& target) {
  if (dt <= 0.f) {
    return;
  }
  const CVector3f toTarget = target - GetTranslation(); // 0x54
  if (toTarget.IsMagnitudeSafe()) {
    BodyController()->FaceDirection(toTarget.AsNormalized(), dt);
  }
}

void CSandBoss::RotateHelper2030(CStateManager&, float) { // fn_55_2030
  PortSandBossStandIn("CSandBoss::RotateHelper2030 (fn_55_2030)");
}

void CSandBoss::RotateHelper2498(CStateManager&, float) { // fn_55_2498
  PortSandBossStandIn("CSandBoss::RotateHelper2498 (fn_55_2498)");
}

void CSandBoss::RotateHelper254C(CStateManager&, float) { // fn_55_254C
  PortSandBossStandIn("CSandBoss::RotateHelper254C (fn_55_254C)");
}

void CSandBoss::RotateHelper25D0(CStateManager&, float) { // fn_55_25D0
  PortSandBossStandIn("CSandBoss::RotateHelper25D0 (fn_55_25D0)");
}

// fn_55_26C0 (ported, not exercised yet)
// How many of the listed charge beams are still live (retail: the object still exists, its plasma
// projectile has not been reset and it is not releasing). DoubleCharge starts a second volley while
// fewer than two are live, TripleCharge a third while fewer than three are.
int CSandBoss::CountActiveChargeBeams(CStateManager& mgr) {
  int count = 0;
  for (int i = 0; i < mChargeBeams.size(); ++i) { // 0xE94
    const CPlasmaProjectile* beam =
        TCastToPtr< CPlasmaProjectile >(mgr.ObjectById(mChargeBeams[i].mBeam));
    if (beam != nullptr && beam->IsFiring()) {
      ++count;
    }
  }
  return count;
}

// @ fn_55_2750
// Per-frame charge-beam update: aims each beam along its recorded segment, moves its looping
// emitter there and damages the player when the beam tip is within ten units. Then runs the script
// effect's phases (mChargeBeamPhase): 0 grow over 0.6 s, 1 hold at the model scale, 2 shrink over
// chargeBeamInfo.shutdownTime; a finished shrink sends Deactivate to the effect and deletes the
// beams.
void CSandBoss::UpdateChargeBeams(CStateManager& mgr, float dt) {
  const CPlayer* player = mgr.GetPlayer(0);
  const CVector3f playerPos = player != nullptr ? player->GetTranslation() : CVector3f::Zero();
  const CDamageInfo damage = mDoubleChargeResult  // 0x165C b1: the probe chose the double charge
                                 ? CDamageInfo(mDoubleChargeDamage, dt)  // 0x14E0, per second
                                 : CDamageInfo(mTripleChargeDamage, dt); // 0x14FC
  const CMaterialFilter filter =
      CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList());
  for (int i = 0; i < mChargeBeams.size(); ++i) {
    SSandBossChargeBeamEntry& entry = mChargeBeams[i];
    CPlasmaProjectile* beam = TCastToPtr< CPlasmaProjectile >(mgr.ObjectById(entry.mBeam));
    if (beam == nullptr) {
      continue;
    }
    beam->UpdateFx(CTransform4f::LookAt(entry.mStart, entry.mEnd, CVector3f::Up()), dt, mgr);
    CSfxManager::UpdateEmitter(entry.mSfx, beam->GetCurrentPos(), CVector3f::Up(), 0x7f);
    if (player != nullptr && (beam->GetCurrentPos() - playerPos).MagSquared() < 100.f) {
      mIsMakingBigStrike = true;
      mDamageDuration = 1.f;
      mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(), damage, filter,
                      CVector3f::Zero());
    }
  }

  CScriptEffect* effect = TCastToPtr< CScriptEffect >(mgr.ObjectById(mScriptEffect)); // 0xEF8
  if (effect == nullptr) {
    return;
  }
  if (GetModelData() == nullptr) {
    return;
  }
  const CVector3f baseScale = GetModelData()->GetScale();
  if (mChargeBeamPhase == 1) { // hold
    if (!mChargeBeams.empty()) {
      effect->SetGlobalTranslation(mChargeBeams[0].mStart); // 0xE9C
      effect->SetGlobalScale(baseScale);
    }
  } else if (mChargeBeamPhase < 1) {
    if (mChargeBeamPhase >= 0) { // grow: 0.6 s from nothing to the model scale
      mChargeBeamPhaseTimer -= dt;
      if (mChargeBeamPhaseTimer <= 0.f) {
        mChargeBeamPhase = 1;
      } else {
        float grow = 1.f - mChargeBeamPhaseTimer / 0.6f;
        if (grow < 0.f) {
          grow = 0.f;
        }
        effect->SetGlobalScale(grow * baseScale);
      }
    }
  } else if (mChargeBeamPhase < 3) { // shrink over the beam's shutdown time
    mChargeBeamPhaseTimer -= dt;
    const float shutdownTime = mData.unknown_0x7619e561.chargeBeamInfo.shutdownTime; // 0x918
    if (mChargeBeamPhaseTimer <= 0.f || shutdownTime <= 0.f) {
      mgr.SendScriptMsg(effect, GetUniqueId(), kSM_Deactivate, kInvalidUniqueId);
      mChargeBeamPhase = -1;
      for (int i = 0; i < mChargeBeams.size(); ++i) {
        if (mgr.ObjectById(mChargeBeams[i].mBeam) != nullptr) {
          CSfxManager::RemoveEmitter(mChargeBeams[i].mSfx);
          mgr.DeleteObjectRequest(mChargeBeams[i].mBeam);
        }
      }
      mChargeBeams.clear();
    } else {
      float shrink = mChargeBeamPhaseTimer / shutdownTime;
      if (shrink > 1.f) {
        shrink = 1.f;
      }
      effect->SetGlobalScale(shrink * baseScale);
      // Retail also writes `shrink` over a CVector3f at beam+0x674; the host class does not identify
      // that member yet (its particles follow the beam's own expansion), so only the effect scales.
    }
  }
}

// fn_55_2CA4 (ported, not exercised yet)
// Releases every listed beam: each plasma projectile stops expanding and fades out (ResetBeam with
// fullReset false, so the actor stays until the effect finishes).
void CSandBoss::ResetChargeBeams(CStateManager& mgr) {
  for (int i = 0; i < mChargeBeams.size(); ++i) {
    if (CPlasmaProjectile* beam =
            TCastToPtr< CPlasmaProjectile >(mgr.ObjectById(mChargeBeams[i].mBeam))) {
      beam->ResetBeam(mgr, false);
    }
  }
}

// fn_55_2D40 (ported, not exercised yet)
// Fires one charge beam along start..end: beam info from the loader's chargeBeamInfo, identity then
// a LookAt transform, a looping emitter on the beam, and {beam, start, end, sfx} appended to the
// list (three at most).
void CSandBoss::CreateChargeBeam(CStateManager& mgr, const CVector3f& start, const CVector3f& end,
                                 ushort sfxId, bool charged) {
  const CBeamInfo beamInfo =
      TLdrToBeamInfo(mData.unknown_0x7619e561.chargeBeamInfo, charged ? 0x191 : 0x91); // 0x8EC
  const TUniqueId uid = mgr.AllocateUniqueId();
  const CPlane plane(CVector3f::Dot(GetTranslation(), CVector3f::Up()), CVector3f::Up());
  SandBossChargeBeam* beam = rs_new SandBossChargeBeam(
      mChargeBeamInfo.Token(), beamInfo, uid, GetCurrentAreaId(), GetUniqueId(), plane); // 0xE28
  if (beam == nullptr) {
    return;
  }
  beam->SetDamageDuration(1.f); // 0x158 |= kPA_BigStrike, 0x1C0 = 1.f
  beam->Fire(CTransform4f::LookAt(start, end, CVector3f::Up()), mgr, false);
  mgr.AddObject(*beam);
  CAudioSys::C3DEmitterParmData parms(150.f, 0.1f, 1, 0x7f, 0x14);
  parms.mPos = beam->GetCurrentPos();
  parms.mDir = CVector3f::Up();
  parms.mSfxId = sfxId;
  const CSfxHandle sfx =
      CSfxManager::AddEmitter(parms, GetCurrentAreaId().Value(), true, true, CSfxManager::kMedPriority);
  const SSandBossChargeBeamEntry entry = {uid, start, end, sfx};
  mChargeBeams.push_back(entry); // 0xE94
}

// fn_55_311C (ported, not exercised yet)
// Starts a charge-beam volley: the script effect moves to the volley's centre, receives Activate,
// and the beam grows from nothing over 0.6 s.
void CSandBoss::StartChargeBeamGrow(CStateManager& mgr, const CVector3f& position) {
  CScriptEffect* effect = TCastToPtr< CScriptEffect >(mgr.ObjectById(mScriptEffect)); // 0xEF8
  if (effect == nullptr) {
    return;
  }
  effect->SetTransform(CTransform4f::Translate(position));
  mgr.SendScriptMsg(effect, GetUniqueId(), kSM_Activate, kInvalidUniqueId);
  mChargeBeamPhase = 0;        // 0xDFC
  mChargeBeamPhaseTimer = 0.6f; // 0xF14
}

// fn_55_31E8 (ported, not exercised yet)
// Ends a charge volley: every attached sibling is told to leave its loop attack (ExitState, its
// animation state marked over), the beams are released, and a running grow/hold phase turns into the
// shrink phase over the beam's shutdown time.
void CSandBoss::EndChargeBeamVolley(CStateManager& mgr) {
  for (int i = 0; i < mSiblings.size(); ++i) { // 0xDD8 / 0xDDC
    CSandBoss* sibling = CastSandBoss(mgr.ObjectById(mSiblings[i]));
    if (sibling != nullptr && sibling->mAttachedToSphere) { // 0x165D b7
      sibling->BodyController()->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_ExitState));
      sibling->mAnimationState.SetState(CAnimationState::kAS_Over); // 0x6B4 = 3
    }
  }
  ResetChargeBeams(mgr); // fn_55_2CA4
  if (mChargeBeamPhase >= 0 && mChargeBeamPhase < 2) { // 0xDFC: grow or hold
    mChargeBeamPhase = 2;
    mChargeBeamPhaseTimer = mData.unknown_0x7619e561.chargeBeamInfo.shutdownTime; // 0x918
  }
}

// fn_55_3324 (ported, not exercised yet)
// Retargets a running volley: once the retarget timer runs out, either wait out the rest of the
// state (interval + variance left) or flip the sweep direction with the loader's chance and wait a
// new interval plus a random variance. The timer only runs while beams exist.
void CSandBoss::UpdateChargeBeamRetarget(CStateManager& mgr, float dt,
                                         const SLdrSandBossChargeBeamData& data) {
  if (!mChargeBeams.empty()) { // 0xE94
    mChargeBeamRetargetTimer -= dt; // 0xF40
  }
  if (mChargeBeamRetargetTimer > 0.f) {
    return;
  }
  const float elapsed = mStateMachine->GetTime();
  const float wait = data.changeDirectionInterval + data.changeDirectionVariance; // 0x14 + 0x18
  if (data.duration - elapsed <= wait) { // 0x10
    mChargeBeamRetargetTimer = wait;
    return;
  }
  if (mgr.Random()->Range(0.f, 100.f) <= data.changeDirectionChance) { // 0x1C
    mChargeBeamSweepDir = mChargeBeamSweepDir == 1 ? 0 : 1; // 0xF3C
  }
  mChargeBeamRetargetTimer =
      data.changeDirectionInterval + mgr.Random()->Range(0.f, data.changeDirectionVariance);
}

// fn_55_3434 (ported, not exercised yet)
// Moves the three triple-charge beam ends: the sweep angle advances, the radius oscillates between
// the triple charge's inner and outer radius, and each beam points along the bisector of two worms'
// facing axes (their transform's local Y column).
void CSandBoss::UpdateTripleChargeSweep(CStateManager& mgr, float dt) {
  if (mChargeBeams.size() != 3) {
    return;
  }
  rstl::vector< TUniqueId > ids;
  if (CollectAttachedSiblings(mgr, ids) != 3) { // fn_55_58B4
    return;
  }
  CVector3f facings[3];
  for (int i = 0; i < 3; ++i) {
    const CSandBoss* sibling = CastSandBoss(mgr.GetObjectById(ids[i]));
    if (sibling == nullptr) {
      return;
    }
    facings[i] = sibling->GetTransform().GetColumn(kDY); // sibling + 0x28/0x38/0x48
  }
  const CVector3f bossPos = GetTranslation();
  mChargeBeamSweepPhase +=
      dt * (mData.unknown_0x7619e561.tripleCharge.unknown_0x47cde539 * 6.2831855f); // 0x8D8
  const float innerRadius = mData.unknown_0x7619e561.tripleCharge.innerRadius; // 0x8C4
  const float outerRadius = mData.unknown_0x7619e561.tripleCharge.outerRadius; // 0x8C8
  const float radius =
      (CMath::FastSinR(mChargeBeamSweepPhase) * (outerRadius - innerRadius) +
       (outerRadius + innerRadius)) *
      0.5f;
  for (int i = 0; i < 3; ++i) {
    const CVector3f sum = facings[i] + facings[(i + 1) % 3];
    const CVector3f dir = sum.IsMagnitudeSafe() ? sum.AsNormalized() : facings[i];
    mChargeBeams[i].mEnd = bossPos + radius * dir; // 0xEA8 / 0xEC8 / 0xEE8
  }
}

// fn_55_37F4 (ported, not exercised yet)
// The double charge's version: the sweep angle uses the double charge's rate and radii, and the two
// beams point along the sum of the two worms' facing axes and its horizontal mirror.
void CSandBoss::UpdateDoubleChargeSweep(CStateManager& mgr, float dt) {
  if (mChargeBeams.size() != 2) {
    return;
  }
  rstl::vector< TUniqueId > ids;
  if (CollectAttachedSiblings(mgr, ids) != 2) { // fn_55_58B4
    return;
  }
  CVector3f facings[2];
  for (int i = 0; i < 2; ++i) {
    const CSandBoss* sibling = CastSandBoss(mgr.GetObjectById(ids[i]));
    if (sibling == nullptr) {
      return;
    }
    facings[i] = sibling->GetTransform().GetColumn(kDY); // sibling + 0x28/0x38/0x48
  }
  const CVector3f bossPos = GetTranslation();
  mChargeBeamSweepPhase +=
      dt * (mData.unknown_0x7619e561.doubleCharge.unknown_0x47cde539 * 6.2831855f); // 0x890
  const float innerRadius = mData.unknown_0x7619e561.doubleCharge.innerRadius; // 0x87C
  const float outerRadius = mData.unknown_0x7619e561.doubleCharge.outerRadius; // 0x880
  const float radius =
      (CMath::FastSinR(mChargeBeamSweepPhase) * (outerRadius - innerRadius) +
       (outerRadius + innerRadius)) *
      0.5f;
  // As fn_55_3F44: the summed facing and its horizontal mirror (x and y negated, z kept).
  const CVector3f sum = facings[0] + facings[1];
  const CVector3f mirrored(-sum.GetX(), -sum.GetY(), sum.GetZ());
  const CVector3f dir0 = sum.IsMagnitudeSafe() ? sum.AsNormalized() : facings[0];
  const CVector3f dir1 = mirrored.IsMagnitudeSafe() ? mirrored.AsNormalized() : facings[1];
  mChargeBeams[0].mEnd = bossPos + radius * dir0; // 0xEA8
  mChargeBeams[1].mEnd = bossPos + radius * dir1; // 0xEC8
}

// fn_55_58B4 (ported, not exercised yet)
// The siblings currently attached to the sphere, in list order; the charge volleys need exactly
// three (triple) or two (double) of them.
int CSandBoss::CollectAttachedSiblings(CStateManager& mgr, rstl::vector< TUniqueId >& ids) const {
  ids.clear();
  for (int i = 0; i < mSiblings.size(); ++i) { // 0xDD8 / 0xDDC
    const CSandBoss* sibling = CastSandBoss(mgr.GetObjectById(mSiblings[i]));
    if (sibling != nullptr && sibling->mAttachedToSphere) { // 0x165D b7
      ids.push_back(mSiblings[i]);
    }
  }
  return ids.size();
}

// fn_55_3ACC (ported, not exercised yet)
// Triple charge volley: every listed charge beam is released first; then, only with all three worms
// attached to the sphere, three beams start at the centroid of the worms' mouth locators and end
// innerRadius from this worm along the bisectors of each neighbouring pair of worms' facings (the
// beam list keeps the end points so fn_55_3434 can sweep them later), with the triple charge's beam
// sound.
void CSandBoss::StartTripleChargeBeam(CStateManager& mgr, const rstl::string& locator) {
  ResetChargeBeams(mgr); // fn_55_2CA4
  rstl::vector< TUniqueId > ids;
  if (CollectAttachedSiblings(mgr, ids) != 3) {
    return;
  }
  CVector3f points[3];
  CVector3f forward[3];
  for (int i = 0; i < 3; ++i) {
    const CSandBoss* sibling = CastSandBoss(mgr.GetObjectById(ids[i]));
    if (sibling == nullptr) {
      return;
    }
    points[i] = sibling->GetLctrTransform(locator).GetTranslation();
    forward[i] = sibling->GetTransform().GetColumn(kDY); // 0x28/0x38/0x48
  }
  const CVector3f centre = (points[0] + points[1] + points[2]) * 0.33333334f;
  const CVector3f bossPos = GetTranslation(); // 0x54
  const float radius = mData.unknown_0x7619e561.tripleCharge.innerRadius; // 0x8C4
  const ushort sfx =
      static_cast< ushort >(mData.unknown_0x7619e561.tripleCharge.sound_ChargeBeam); // 0x8DC
  // Each beam points along the bisector of two neighbouring worms' facings.
  for (int i = 0; i < 3; ++i) {
    const CVector3f sum = forward[i] + forward[(i + 1) % 3];
    const CVector3f dir = sum.IsMagnitudeSafe() ? sum.AsNormalized() : forward[i];
    CreateChargeBeam(mgr, centre, bossPos + radius * dir, sfx, false); // fn_55_2D40
  }
  if (!mChargeBeams.empty()) { // 0xE94
    StartChargeBeamGrow(mgr, centre); // fn_55_311C
  }
}

// fn_55_3F44 (ported, not exercised yet)
// Double charge volley: same shape with two worms -- two beams from the midpoint of the two mouths,
// ending innerRadius from this worm along the worms' summed facing and its horizontal mirror, with
// the double charge's beam sound.
void CSandBoss::StartDoubleChargeBeam(CStateManager& mgr, const rstl::string& locator) {
  ResetChargeBeams(mgr); // fn_55_2CA4
  rstl::vector< TUniqueId > ids;
  if (CollectAttachedSiblings(mgr, ids) != 2) {
    return;
  }
  CVector3f points[2];
  CVector3f forward[2];
  for (int i = 0; i < 2; ++i) {
    const CSandBoss* sibling = CastSandBoss(mgr.GetObjectById(ids[i]));
    if (sibling == nullptr) {
      return;
    }
    points[i] = sibling->GetLctrTransform(locator).GetTranslation();
    forward[i] = sibling->GetTransform().GetColumn(kDY); // 0x28/0x38/0x48
  }
  const CVector3f centre = (points[0] + points[1]) * 0.5f;
  const CVector3f bossPos = GetTranslation(); // 0x54
  const float radius = mData.unknown_0x7619e561.doubleCharge.innerRadius; // 0x87C
  const ushort sfx =
      static_cast< ushort >(mData.unknown_0x7619e561.doubleCharge.sound_ChargeBeam); // 0x894
  // The two beams take the summed facing and its horizontal mirror (x and y negated, z kept).
  const CVector3f sum = forward[0] + forward[1];
  const CVector3f mirrored(-sum.GetX(), -sum.GetY(), sum.GetZ());
  const CVector3f dir0 = sum.IsMagnitudeSafe() ? sum.AsNormalized() : forward[0];
  const CVector3f dir1 = mirrored.IsMagnitudeSafe() ? mirrored.AsNormalized() : forward[1];
  CreateChargeBeam(mgr, centre, bossPos + radius * dir0, sfx, false); // fn_55_2D40
  CreateChargeBeam(mgr, centre, bossPos + radius * dir1, sfx, false); // fn_55_2D40
  if (!mChargeBeams.empty()) { // 0xE94
    StartChargeBeamGrow(mgr, centre); // fn_55_311C
  }
}

// fn_55_42A8 (ported, not exercised yet)
// Picks the worm that should fire the dark beam and returns its uid (kInvalidUniqueId for none).
// With two attached siblings the pick only happens once this worm's doubleCharge cooldown field
// matches mChargeBeamCooldown (0x898 vs 0xF44), with three it is tripleCharge's (0x8E0); any other
// sibling count always picks. The walk covers the whole sibling list, so this worm is a candidate;
// it needs the worm attached to the sphere and in locomotion, stops at the first sibling whose
// dark-beam probe already succeeded (0x165C b3), and prefers the smallest angle between that worm's
// facing and the player (two siblings) or the largest 0xF34 (three).
TUniqueId CSandBoss::ShouldDarkBeamAttackCheck(CStateManager& mgr) const {
  const int attached = CountAttachedSiblings(mgr); // fn_55_595C
  bool pick;
  if (attached == 2) {
    pick = mData.unknown_0x7619e561.doubleCharge.unknown_0x8d4f3b88 == mChargeBeamCooldown; // 0x898
  } else if (attached < 2 || attached > 3) {
    pick = true;
  } else {
    pick = mData.unknown_0x7619e561.tripleCharge.unknown_0x8d4f3b88 == mChargeBeamCooldown; // 0x8E0
  }
  if (!pick) {
    return kInvalidUniqueId;
  }
  const CPlayer* player = mgr.GetPlayer(0); // mgr 0x14FC
  if (player == nullptr) {
    return kInvalidUniqueId;
  }
  const CVector3f toPlayer = player->GetTranslation() - GetTranslation(); // 0x54..0x5C
  TUniqueId best = kInvalidUniqueId;
  float bestValue = attached == 2 ? 3.4028234663852886e+38f : -1.f;
  for (int i = 0; i < mSiblings.size(); ++i) { // 0xDD8 / 0xDDC
    const CSandBoss* sibling = CastSandBoss(mgr.GetObjectById(mSiblings[i]));
    if (sibling == nullptr || !sibling->mAttachedToSphere) { // 0x165D b7
      continue;
    }
    if (sibling->mDarkBeamResult) { // 0x165C b3: someone already got the shot through
      break;
    }
    if (!sibling->CheckBodyStateForDarkBeam(mgr)) { // fn_55_8C88
      continue;
    }
    if (attached == 2) {
      const float angle =
          CVector3f::GetAngleDiff(sibling->GetTransform().GetColumn(kDY), toPlayer); // 0x28/38/48
      if (angle < bestValue) {
        bestValue = angle;
        best = mSiblings[i];
      }
    } else if (bestValue < sibling->mF34) { // 0xF34
      bestValue = sibling->mF34;
      best = mSiblings[i];
    }
  }
  return best;
}

void CSandBoss::FireDarkBeam(CStateManager&) { // fn_55_44CC
  PortSandBossStandIn("CSandBoss::FireDarkBeam (fn_55_44CC)");
}

void CSandBoss::HandleScreenShake(CStateManager&, const rstl::string&) { // fn_55_4C60
  PortSandBossStandIn("CSandBoss::HandleScreenShake (fn_55_4C60)");
}

// fn_55_51BC (ported, not exercised yet)
// The sphere is gone: take the DEAD/ATCH waypoint's transform, looking from this worm's origin at
// it, so the post-destroy state starts from the sphere's frame.
void CSandBoss::DestroySphereHelper(CStateManager& mgr) {
  const TUniqueId waypointId = FindConnectedObject(mgr, kSS_Dead, kSM_Attach);
  CScriptWaypoint* waypoint = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(waypointId));
  if (waypoint != nullptr) {
    // Retail overwrites the look point's Z with this worm's own Z before LookAt (the compiled
    // code stores the waypoint's Z and then this worm's Z over it), so the sight line is flat.
    const CVector3f waypointPos = waypoint->GetTranslation();
    SetTransform(CTransform4f::LookAt(GetTranslation(),
                                      CVector3f(waypointPos.GetX(), waypointPos.GetY(),
                                                GetTranslation().GetZ()),
                                      CVector3f::Up()));
  }
}

void CSandBoss::HandleGroundImpact(CStateManager&, const rstl::string&) { // fn_55_528C
  PortSandBossStandIn("CSandBoss::HandleGroundImpact (fn_55_528C)");
}

// fn_55_560C (ported, not exercised yet)
// Cracked sphere model: the sphere actor takes the round's cracked model (crackedSphere1..3 by
// mRound) with its own model scale, then the ZERO script message goes out.
void CSandBoss::CrackSphere(CStateManager& mgr) {
  CActor* sphere = TCastToPtr< CActor >(mgr.ObjectById(mSphere)); // 0xE8A
  if (sphere != nullptr && sphere->GetModelData() != nullptr) {
    CAssetId crackedModel = kInvalidAssetId;
    if (mRound == 1) { // 0xDE4
      crackedModel = mData.crackedSphere2; // 0x7CC
    } else if (mRound < 1) {
      if (mRound >= 0) {
        crackedModel = mData.crackedSphere1; // 0x7C8
      }
    } else if (mRound < 3) {
      crackedModel = mData.crackedSphere3; // 0x7D0
    }
    if (crackedModel != kInvalidAssetId) {
      // Retail builds CModelData(CStaticRes(cracked, sphere scale)) and assigns it to the sphere.
      const CModelData crackedModelData(
          CStaticRes(crackedModel, sphere->GetModelData()->GetScale()));
      sphere->SetModelData(crackedModelData, mgr);
    }
  }
  SendScriptMsgs(kSS_Zero, mgr, GetUniqueId(), kSM_None);
}

// fn_55_5B38 (ported, not exercised yet)
// The morph ball is in range when it is closer to the first armour locator than
// mData.suckMorphballRange and within fifteen degrees of the boss' forward axis, flattened.
bool CSandBoss::IsBallInRange(const CPlayer& player) const {
  const CTransform4f locatorXf = GetLctrTransform(mArmorLocators[0]); // 0xF00
  const CVector3f delta = player.GetTranslation() - locatorXf.GetTranslation(); // player 0x54..0x5C
  const float range = mData.suckMorphballRange; // 0x820
  if (range * range <= delta.MagSquared()) {
    return false;
  }
  const CVector3f flatDelta(CVector2f(delta.GetX(), delta.GetY()), 0.f);
  return CVector3f::GetAngleDiff(GetTransform().GetColumn(kDY), flatDelta) <= 0.2617994f;
}

// @ fn_55_5C30
// AttachToSphere's delay: half a second per command index, so the three worms attach in turn.
float CSandBoss::AttachToSphereHelper() { return static_cast< float >(mData.commandIndex) * 0.5f; }

// fn_55_5C6C (ported, not exercised yet)
// Same mouth box as fn_55_5E5C, but the bombs found in it are thrown along dir at 20 units a second
// (the ball's bombs leave the mouth with it).
void CSandBoss::Helper5C6C(CStateManager& mgr, const CVector3f& dir) {
  const CTransform4f locatorXf = GetLctrTransform(mArmorLocators[0]); // 0xF00
  const CVector3f mouth = locatorXf.GetTranslation();
  const float range = GetModelData()->GetScale().GetY() * 5.f;
  const CVector3f extent(range, range, range);
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, CAABox(mouth - extent, mouth + extent),
                    CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Bomb), CMaterialList()),
                    this);
  for (int i = 0; i < nearList.size(); ++i) {
    if (CBomb* bomb = TCastToPtr< CBomb >(mgr.ObjectById(nearList[i]))) {
      bomb->SetVelocityWR(dir * 20.f);
    }
  }
}

// fn_55_5E5C (ported, not exercised yet)
// Choke helper: every bomb in the box around the mouth locator is parked on the mouth (the ball's
// bombs follow it while the mouth works).
void CSandBoss::Helper5E5C(CStateManager& mgr) {
  const CTransform4f locatorXf = GetLctrTransform(mArmorLocators[0]); // 0xF00
  const CVector3f mouth = locatorXf.GetTranslation();
  const float range = GetModelData()->GetScale().GetY() * 5.f;
  const CVector3f extent(range, range, range);
  rstl::reserved_vector< TUniqueId, 1024 > nearList;
  mgr.BuildNearList(nearList, CAABox(mouth - extent, mouth + extent),
                    CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Bomb), CMaterialList()),
                    this);
  for (int i = 0; i < nearList.size(); ++i) {
    if (CBomb* bomb = TCastToPtr< CBomb >(mgr.ObjectById(nearList[i]))) {
      bomb->SetTranslation(mouth);
    }
  }
}

// fn_55_5FEC (ported, not exercised yet)
// Object drop (Dead, and after a spit): if the player still holds on to this worm, put a morphed
// player where the mouth is with the held ball's rotation, half a unit lower, launch him along the
// mouth's facing, apply the spit-out damage unless the head armour is already gone, throw the held
// bombs too, then let go of the player and send kSS_Exited for him. The player gets his solid
// material back either way.
void CSandBoss::HandleObjectDrop(CStateManager& mgr) {
  CPlayer* player = mgr.GetPlayer(0); // mgr 0x14FC
  // Retail also sets bit 7 of the player's byte at 0x1144. CPlayer.hpp does not name that byte (the
  // two morph-transition bits it does name are elsewhere), so the store is left out.
  if (player->GetAttachedActorId() == GetUniqueId()) { // player 0x2E4
    if (player->GetMorphballTransitionState() == CPlayer::kMS_Morphed) { // player 0x38C == 1
      player->Stop();
      CTransform4f xf = GetLctrTransform(mArmorLocators[0]) * m14A0; // 0xF00, 0x14A0
      xf.SetTranslation(xf.GetTranslation() + CVector3f(0.f, 0.f, -0.5f));
      player->Teleport(xf, mgr, false);
      // The impulse is scaled by the player's mass so the launch speed is mass independent
      // (retail reads 0x158, the first CPhysicsActor member).
      const CVector3f facing = GetTransform().GetColumn(kDY); // 0x28/0x38/0x48
      player->ApplyImpulseWR(facing * (player->GetMass() * 20.f), CAxisAngle::Identity());
      player->SetMoveState(NPlayer::kMS_ApplyJump, mgr);
      if (!mArmorDestroyed) { // 0x165D b6
        const CMaterialFilter filter =
            CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList());
        mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(), mSpitOutDamage,
                        filter, CVector3f::Zero()); // 0x1550
      }
      if (m165E_b3) { // 0x165E b3
        Helper5C6C(mgr, facing); // fn_55_5C6C
      }
    }
    player->DetachActorFromPlayer();
    SendScriptMsgs(kSS_Exited, mgr, player->GetUniqueId(), kSM_None); // 'EXIT' on the player
  }
  player->AddMaterial(kMT_Solid, mgr); // 0x3B
}

// fn_55_6250 (ported, not exercised yet)
// The ball was caught: put the player on the mouth locator, stop him, and keep the player's
// rotation in 0x14A0 (the held ball's orientation while BallInMouth re-parks it every frame).
void CSandBoss::SphereStateHelper6250(CPlayer& player) {
  const CTransform4f locatorXf = GetLctrTransform(mArmorLocators[0]); // 0xF00
  player.SetTranslation(locatorXf.GetTranslation());
  player.Stop();
  m14A0 = player.GetTransform().GetRotation(); // 0x14A0
}

// fn_55_62E4 (ported, not exercised yet)
// While the player is morphed, pulls the ball toward the mouth locator at 60 units a second; the
// return value says the ball arrived (and is parked in the mouth by fn_55_6250).
bool CSandBoss::SphereStateHelper62E4(float dt, CPlayer& player) {
  if (player.GetMorphballTransitionState() != CPlayer::kMS_Morphed) { // player 0x38C == 1
    return false;
  }
  const CTransform4f locatorXf = GetLctrTransform(mArmorLocators[0]); // 0xF00
  const CVector3f delta = locatorXf.GetTranslation() - player.GetTranslation();
  const float step = dt * 60.f;
  if (delta.MagSquared() < step * step || !delta.IsMagnitudeSafe()) {
    SphereStateHelper6250(player); // fn_55_6250
    return true;
  }
  player.Stop();
  player.SetTranslation(player.GetTranslation() + delta.AsNormalized() * step);
  return false;
}

// fn_55_6434 (ported, not exercised yet)
// Choke flash: the damage colour pulses between black and skDamageColor with |cos(pi t)| and the
// damage-hit timer is restarted through TakeDamage (fn_55_4C only stores skDamageHitTime).
void CSandBoss::SphereStateHelper6434(float dt) {
  const float blend = CMath::AbsF(CMath::FastCosR(dt * M_PIF));
  mDamageColor = CColor::Lerp(CColor::Black(), skDamageColor, blend); // 0x450
  TakeDamage(CVector3f::Zero(), 0.f);                                  // 0x448 = skDamageHitTime
}

// @ fn_55_64E4
// A stampede armour piece came off: explosion at the locator, the 0.2 s gate before the next piece
// can break (0xF30) and the IS01 waypoint taking the locator's transform.
void CSandBoss::Helper64E4(CStateManager& mgr, const CTransform4f& locatorXf) {
  if (mStampedeArmorExplosion.valid()) { // 0xE60
    CExplosion* explosion = rs_new CExplosion(
        *mStampedeArmorExplosion, mgr.AllocateUniqueId(),
        CEntityInfo(GetCurrentAreaId(), CEntity::NullConnectionList, true),
        rstl::string_l("StampedeArmorExplosionFx"), locatorXf, 0, GetModelData()->GetScale(),
        CColor::White(), -1);
    if (explosion != nullptr) {
      mgr.AddObject(explosion);
      mF30 = 0.2f; // 0xF30
    }
    if (CScriptWaypoint* waypoint = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(mIS01Target))) {
      waypoint->SetTransform(locatorXf); // 0xE82
      SendScriptMsgs(kSS_InternalState1, mgr, GetUniqueId(), kSM_None);
    }
  }
}

// @ fn_55_7C68
// While stampeding, sheds one armour piece at a time (tail first, one eighth of the stampede HP
// apart) as the stampede HP drops, with the armour explosion sound and the head pain sound while
// HP remains.
void CSandBoss::StampedeHelper(CStateManager& mgr, float dt) {
  mF30 -= dt; // 0xF30
  if (mF30 > 0.f) {
    return;
  }
  float threshold = mData.stampedeProperties.breakStampedeHP; // 0x838
  const float step = threshold * 0.125f;
  int shed = -1;
  for (int i = 7; i >= 0; --i) {
    threshold -= step;
    if ((mStampedeHP <= threshold || mStampedeHP <= 0.f) && mArmorState[i] != 0) { // 0x1458+4i
      const CTransform4f locatorXf = GetLctrTransform(mArmorLocators[i]); // 0xF00+i
      Helper64E4(mgr, locatorXf);                                        // fn_55_64E4
      mArmorState[i] = 0;
      shed = i;
      break;
    }
  }
  if (shed == -1) {
    return;
  }
  const CTransform4f shedXf = GetLctrTransform(mArmorLocators[shed]);
  CSfxManager::AddEmitter(static_cast< ushort >(mData.stampedeProperties.sound_StampedeArmorExplode),
                          shedXf.GetTranslation(), 0x7f, GetCurrentAreaId().Value(), true, false,
                          CSfxManager::kMedPriority); // 0x840
  if (mStampedeHP > 0.f) {
    const CTransform4f headXf = GetLctrTransform(mHeadLocator); // 0xF08
    CSfxManager::AddEmitter(
        static_cast< ushort >(mData.stampedeProperties.sound_StampedeArmorExplodePain),
        headXf.GetTranslation(), 0x7f, GetCurrentAreaId().Value(), true, false,
        CSfxManager::kMedPriority); // 0x844
  }
}

void CSandBoss::Helper7E60(CStateManager&) { // fn_55_7E60
  PortSandBossStandIn("CSandBoss::Helper7E60 (fn_55_7E60)");
}

void CSandBoss::Helper8008(CStateManager&) { // fn_55_8008
  PortSandBossStandIn("CSandBoss::Helper8008 (fn_55_8008)");
}

void CSandBoss::HandleDamage(CStateManager&, TUniqueId) { // fn_55_8258
  PortSandBossStandIn("CSandBoss::HandleDamage (fn_55_8258)");
}

// @ fn_55_83AC
// Picks the locomotion type from how far the player is off the facing axis: under 35 degrees or
// over 145 (0.6108652 / 2.5307274 rad) the body relaxes (pas 1); otherwise 0xB when the player is
// on one side of the local X axis and 0xA on the other. Retail's side test reads +0x24/+0x34/+0x44,
// which under the proven kDY mapping (0x28/0x38/0x48) is the kDX column [uncertain: row vs column].
void CSandBoss::RotateToPlayerHelper(const CVector3f& target) {
  const CVector3f toTarget = target - GetTranslation(); // 0x54..0x5C
  const float angle = CVector3f::GetAngleDiff(GetTransform().GetColumn(kDY), toTarget);
  if (angle < 0.6108652353286743f || angle > 2.5307273864746094f) {
    BodyController()->SetLocomotionType(pas::kLT_Relaxed); // 1
  } else if (CVector3f::Dot(GetTransform().GetColumn(kDX), toTarget) <= 0.f) {
    BodyController()->SetLocomotionType(pas::kLT_Internal11); // 0xB
  } else {
    BodyController()->SetLocomotionType(pas::kLT_Internal10); // 0xA
  }
}

// @ fn_55_10CB4
// Retail quicksorts the 0x1478 entries by their squared distance; at most sixteen entries, a host
// insertion sort is the same order.
void CSandBoss::SortStampedePoints() {
  for (int i = 1; i < mIdEntries.size(); ++i) {
    const SSandBossIdEntry entry = mIdEntries[i];
    int j = i - 1;
    while (j >= 0 && entry.mValue < mIdEntries[j].mValue) {
      mIdEntries[j + 1] = mIdEntries[j];
      --j;
    }
    mIdEntries[j + 1] = entry;
  }
}
