// Host body for the SandBoss REL (module 55, Amorbis): the attack helpers and the charge-beam
// projectile. The stampede helpers follow retail (`// fn_55_X (ported, not exercised yet)`); every
// other function here is still a log-once stand-in (bool helpers return false, the rest do
// nothing; the charge beam forwards to CPlasmaProjectile).
// docs/module-analysis/SandBoss.md section 4 is the function map.
#include "MetroidPrime/Enemies/CSandBoss.hpp"

#include "Collision/CRayCastResult.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"

void PortSandBossStandIn(const char* what);
const CSandBoss* CastSandBoss(const CEntity* entity);

SandBossChargeBeam::SandBossChargeBeam(const TToken< CWeaponDescription >& description,
                                       const rstl::string& name, EWeaponType type,
                                       const CBeamInfo& beamInfo, const CTransform4f& xf,
                                       EMaterialTypes material, const CDamageInfo& damage,
                                       TUniqueId uid, TAreaId areaId, TUniqueId owner,
                                       const CWeaponAssetInfo& resources, bool drawOwnerFirst,
                                       uint attribs) // fn_55_107B0
: CPlasmaProjectile(description, name, type, beamInfo, xf, material, damage, uid, areaId, owner,
                    resources, drawOwnerFirst, attribs) {
  PortSandBossStandIn("SandBossChargeBeam::SandBossChargeBeam (fn_55_107B0)");
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

void CSandBoss::BuildPointList(CStateManager&) { // fn_55_26C0
  PortSandBossStandIn("CSandBoss::BuildPointList (fn_55_26C0)");
}

void CSandBoss::UpdateAttacks(CStateManager&, float) { // fn_55_2750
  PortSandBossStandIn("CSandBoss::UpdateAttacks (fn_55_2750)");
}

void CSandBoss::UpdatePointList(CStateManager&) { // fn_55_2CA4
  PortSandBossStandIn("CSandBoss::UpdatePointList (fn_55_2CA4)");
}

void CSandBoss::CreateChargeBeam(CStateManager&) { // fn_55_2D40
  PortSandBossStandIn("CSandBoss::CreateChargeBeam (fn_55_2D40)");
}

void CSandBoss::ChargeHelper311C(CStateManager&) { // fn_55_311C
  PortSandBossStandIn("CSandBoss::ChargeHelper311C (fn_55_311C)");
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
