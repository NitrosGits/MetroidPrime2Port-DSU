// Host body for the SandBoss REL (module 55, Amorbis): the attack helpers and the charge-beam
// projectile. The stampede helpers and the charge-beam list (fn_55_26C0..fn_55_311C) follow retail
// (`// fn_55_X (ported, not exercised yet)`); every other function here is still a log-once stand-in
// (bool helpers return false, the rest do nothing).
// docs/module-analysis/SandBoss.md section 4 is the function map.
#include "MetroidPrime/Enemies/CSandBoss.hpp"

#include "Collision/CRayCastResult.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptObjects/CScriptEffect.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/Weapons/CBeamInfo.hpp"
#include "MetroidPrime/Weapons/CWeaponAssetInfo.hpp"
#include "MetroidPrime/TCastTo.hpp"

void PortSandBossStandIn(const char* what);
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

CRayCastResult SandBossChargeBeam::RayCollisionCheckWithWorld(
    TUniqueId& idOut, const CVector3f& start, const CVector3f& end, float magnitude,
    rstl::reserved_vector< TUniqueId, 1024 >& nearList, CStateManager& mgr,
    EStaticGeometryTest staticTest) { // fn_55_1027C
  PortSandBossStandIn("SandBossChargeBeam::RayCollisionCheckWithWorld (fn_55_1027C)");
  return CPlasmaProjectile::RayCollisionCheckWithWorld(idOut, start, end, magnitude, nearList, mgr,
                                                       staticTest);
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

bool CSandBoss::ShouldRepeaterAttackCheck(CStateManager&) const { // fn_55_1B94
  PortSandBossStandIn("CSandBoss::ShouldRepeaterAttackCheck (fn_55_1B94)");
  return false;
}

void CSandBoss::SyncAttacker(CStateManager&) { // fn_55_1C70
  PortSandBossStandIn("CSandBoss::SyncAttacker (fn_55_1C70)");
}

void CSandBoss::RotateToPlayerStep(CStateManager&, const CVector3f&, float) { // fn_55_22E4
  PortSandBossStandIn("CSandBoss::RotateToPlayerStep (fn_55_22E4)");
}

void CSandBoss::RotateHelper1D74(CStateManager&, float) { // fn_55_1D74
  PortSandBossStandIn("CSandBoss::RotateHelper1D74 (fn_55_1D74)");
}

void CSandBoss::RotateHelper1F84(CStateManager&, float) { // fn_55_1F84
  PortSandBossStandIn("CSandBoss::RotateHelper1F84 (fn_55_1F84)");
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

// fn_55_2750 (ported, not exercised yet)
// Per-frame charge-beam update: aims each beam along its recorded segment, moves its looping
// emitter there and damages the player when the beam tip is within ten units. Then runs the script
// effect's phases (mChargeBeamPhase): 0 grow over 0.6 s, 1 hold at the model scale, 2 shrink over
// chargeBeamInfo.shutdownTime; a finished shrink sends Deactivate to the effect and deletes the
// beams.
void CSandBoss::UpdateChargeBeams(CStateManager& mgr, float dt) {
  const CPlayer* player = mgr.GetPlayer(0);
  const CVector3f playerPos = player != nullptr ? player->GetTranslation() : CVector3f::Zero();
  const CDamageInfo damage = mDoubleChargeResult  // 0x165C b1: the probe chose the double charge
                                 ? CDamageInfo(mDoubleChargeDamage, 1.f)  // 0x14E0
                                 : CDamageInfo(mTripleChargeDamage, 1.f); // 0x14FC
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
  beam->Fire(CTransform4f::LookAt(start, end, CVector3f::Up()), mgr, false);
  mgr.AddObject(*beam);
  const CSfxHandle sfx =
      CSfxManager::AddEmitter(sfxId, beam->GetCurrentPos(), 0x7f, GetCurrentAreaId().Value(), true,
                              true, CSfxManager::kMedPriority);
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

void CSandBoss::ChargeHelper31E8(CStateManager&) { // fn_55_31E8
  PortSandBossStandIn("CSandBoss::ChargeHelper31E8 (fn_55_31E8)");
}

void CSandBoss::ChargeHelper3324(CStateManager&) { // fn_55_3324
  PortSandBossStandIn("CSandBoss::ChargeHelper3324 (fn_55_3324)");
}

void CSandBoss::ChargeHelper3434(CStateManager&) { // fn_55_3434
  PortSandBossStandIn("CSandBoss::ChargeHelper3434 (fn_55_3434)");
}

void CSandBoss::ChargeHelper37F4(CStateManager&) { // fn_55_37F4
  PortSandBossStandIn("CSandBoss::ChargeHelper37F4 (fn_55_37F4)");
}

void CSandBoss::StartTripleChargeBeam(CStateManager&, const rstl::string&) { // fn_55_3ACC
  PortSandBossStandIn("CSandBoss::StartTripleChargeBeam (fn_55_3ACC)");
}

void CSandBoss::StartDoubleChargeBeam(CStateManager&, const rstl::string&) { // fn_55_3F44
  PortSandBossStandIn("CSandBoss::StartDoubleChargeBeam (fn_55_3F44)");
}

bool CSandBoss::ShouldDarkBeamAttackCheck(CStateManager&) const { // fn_55_42A8
  PortSandBossStandIn("CSandBoss::ShouldDarkBeamAttackCheck (fn_55_42A8)");
  return false;
}

void CSandBoss::FireDarkBeam(CStateManager&) { // fn_55_44CC
  PortSandBossStandIn("CSandBoss::FireDarkBeam (fn_55_44CC)");
}

void CSandBoss::HandleScreenShake(CStateManager&, const rstl::string&) { // fn_55_4C60
  PortSandBossStandIn("CSandBoss::HandleScreenShake (fn_55_4C60)");
}

void CSandBoss::DestroySphereHelper(CStateManager&) { // fn_55_51BC
  PortSandBossStandIn("CSandBoss::DestroySphereHelper (fn_55_51BC)");
}

void CSandBoss::HandleGroundImpact(CStateManager&, const rstl::string&) { // fn_55_528C
  PortSandBossStandIn("CSandBoss::HandleGroundImpact (fn_55_528C)");
}

void CSandBoss::CrackSphere(CStateManager&) { // fn_55_560C
  PortSandBossStandIn("CSandBoss::CrackSphere (fn_55_560C)");
}

bool CSandBoss::IsBallInRange(CStateManager&) const { // fn_55_5B38
  PortSandBossStandIn("CSandBoss::IsBallInRange (fn_55_5B38)");
  return false;
}

// fn_55_5C30 (ported, not exercised yet)
// AttachToSphere's delay: half a second per command index, so the three worms attach in turn.
float CSandBoss::AttachToSphereHelper() { return static_cast< float >(mData.commandIndex) * 0.5f; }

void CSandBoss::Helper5C6C(CStateManager&) { // fn_55_5C6C
  PortSandBossStandIn("CSandBoss::Helper5C6C (fn_55_5C6C)");
}

void CSandBoss::Helper5E5C(CStateManager&) { // fn_55_5E5C
  PortSandBossStandIn("CSandBoss::Helper5E5C (fn_55_5E5C)");
}

void CSandBoss::HandleObjectDrop(CStateManager&) { // fn_55_5FEC
  PortSandBossStandIn("CSandBoss::HandleObjectDrop (fn_55_5FEC)");
}

void CSandBoss::SphereStateHelper6250(CStateManager&) { // fn_55_6250
  PortSandBossStandIn("CSandBoss::SphereStateHelper6250 (fn_55_6250)");
}

void CSandBoss::SphereStateHelper62E4(CStateManager&) { // fn_55_62E4
  PortSandBossStandIn("CSandBoss::SphereStateHelper62E4 (fn_55_62E4)");
}

void CSandBoss::SphereStateHelper6434(CStateManager&) { // fn_55_6434
  PortSandBossStandIn("CSandBoss::SphereStateHelper6434 (fn_55_6434)");
}

// fn_55_64E4 (ported, not exercised yet)
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

void CSandBoss::RotateToPlayerHelper(const CVector3f&) { // fn_55_83AC
  PortSandBossStandIn("CSandBoss::RotateToPlayerHelper (fn_55_83AC)");
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
