// Port-only: the IngBoostBallGuardian REL's second class (module 30), the boost attack's rolling
// ball (include/MetroidPrime/Enemies/PortCBoostBallGuardianBall.hpp). The behaviour is ported
// from the module's own bodies at 0x11CD8..0x164D0; the addresses are the module's own.
//
// Not here yet (BoostGuardian.md section f, slices b and c): Think (fn_30_1548C, a named
// stand-in below), the roll fn_30_12AB0, CollidedWith fn_30_142BC, AcceptScriptMsg fn_30_1466C
// and the three render overrides (fn_30_13E68, fn_30_141B8, fn_30_13ECC), so the ball neither
// moves nor draws its effects yet.
#include "MetroidPrime/Enemies/PortCBoostBallGuardianBall.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Collision/CMaterialList.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CAABox.hpp"
#include "Kyoto/Math/CMayaSpline.hpp"
#include "Kyoto/Math/CSphere.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "Kyoto/Particles/CGenDescription.hpp"
#include "Kyoto/Particles/CParticleSpawnSystem.hpp"
#include "Kyoto/Particles/CSpawnSystemDescription.hpp"
#include "Kyoto/SObjectTag.hpp"
#include "MetroidPrime/CAxisAngle.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/Enemies/PortBoostGuardianShared.hpp"

#include <stdio.h>

namespace {

// 0x000159B4..0x00015A04: the caller's list (empty when null) with kMT_Solid (lbl_30_data_C04)
// and kMT_Projectile (lbl_30_data_C08) added.
CMaterialList BallMaterials(const CMaterialList* matList) {
  CMaterialList list(kMT_Solid, kMT_Projectile);
  if (matList != nullptr) {
    list.Add(*matList);
  }
  return list;
}

// 0x00015ABC..0x00015B14: radius 1.0 when the model data has neither an animation nor a static
// model, else half the model bounds' height.
float BallRadius(const CModelData& mData, const CModelData* own) {
  if (mData.GetAnimationData() == nullptr && !mData.HasNormalModel()) {
    return 1.f;
  }
  const CAABox bounds = own->GetBounds();
  return (bounds.GetMaxPoint().GetZ() - bounds.GetMinPoint().GetZ()) * 0.5f;
}

// fn_30_16238: a particle generator for an asset, by type: 'PART' makes a CElementGen
// (kMOT_Normal, kOSF_One; 0x00016318), 'SPSC' a CParticleSpawnSystem (kOSF_One, no model
// lights; 0x000163E0); an invalid id or any other type gives none.
// @ fn_30_16238
CParticleGen* MakeBallEffect(CAssetId id, FourCC type) {
  if (id == kInvalidAssetId) {
    return nullptr;
  }
  switch (type) {
  case 'PART': {
    TToken< CGenDescription > desc(gpSimplePool->GetObj(SObjectTag('PART', id)));
    return rs_new CElementGen(desc, CElementGen::kMOT_Normal, CElementGen::kOSF_One);
  }
  case 'SPSC': {
    TToken< CSpawnSystemDescription > desc(gpSimplePool->GetObj(SObjectTag('SPSC', id)));
    return rs_new CParticleSpawnSystem(desc, CElementGen::kOSF_One, false);
  }
  default:
    return nullptr;
  }
}

} // namespace

// @ fn_30_15924
CBoostBallGuardianBall::CBoostBallGuardianBall(
    TUniqueId uid, const rstl::string& name, const CEntityInfo& info, const CTransform4f& xf,
    const CVector3f& pos, const CModelData& modelData, const CActorParameters& actParams,
    TUniqueId ownerId, const CBoostBallGuardianData& data, int phaseIndex,
    const CDamageVulnerability& vulnerability, const rstl::vector< TUniqueId >& modifyTargets,
    const CMaterialList* matList, const CHealthInfo* health)
: CPhysicsActor(uid, name, info, 0, xf, modelData, BallMaterials(matList), CAABox::MakeNullBox(),
                SMoverData(data.mMass, CVector3f::Zero(), CAxisAngle::Identity(),
                           CVector3f::Zero(), CAxisAngle::Identity()),
                actParams, skDefaultStepData) // 0x00015A7C
, mPhaseIndex(phaseIndex)                     // 0x00015A98
, mState(0)                                   // 0x00015A9C
, mHomePos(GetTranslation())                  // 0x00015AA0
, mData(data)                                 // 0x00015AB8: fn_30_10250
, mSphere(CSphere(CVector3f::Zero(), BallRadius(modelData, GetModelData())),
          GetMaterialList())                  // 0x00015B38
, mOwnerId(ownerId)                           // 0x00015B6C
, mId4F2(kInvalidUniqueId)
, mId4F4(kInvalidUniqueId)
, mId4F6(kInvalidUniqueId)
, mTouchedId(kInvalidUniqueId)                // 0x00015B8C
, mModifyTargets(modifyTargets)               // 0x00015B90: fn_30_160D0
, mDamageTimer(1.f)                           // 0x00015BAC
, m510(0.f)
, mSplineDuration(mData.mSpeed.GetDuration()) // 0x00015BC8
, mHalfHeight(0.f)
, mSplineTime(0.f)
, m520(0.4f)                                  // 0x00015C00
, m524(0.f)
, mSplineEndTime(0.f)
, mStateTime(0.f)
, m530(1.f)                                   // 0x00015C14
, mScale(CVector3f::One())                    // 0x00015C1C
, m540(CVector3f::Zero())
, m54C(CVector3f::Zero())
, m558(0)                                     // 0x00015C60
, m55C(0)
, m560(0)
, mTrailGen(MakeBallEffect(mData.mTrailEffect, 'SPSC'))        // 0x00015C70: data+0xA8
, mShieldGen(MakeBallEffect(mData.mShieldEffect, 'PART'))      // 0x00015C84: data+0xAC
, mChargeUpGen(MakeBallEffect(mData.mChargeUpEffect, 'PART'))  // 0x00015C98: data+0xB0
, mVulnerability(vulnerability)               // 0x00015CA8: fn_30_AD4
, mRollingSfx()                               // 0x00015CBC
, m5D8_80(false)                              // 0x00015CCC..0x00015D28: 0x5D8 all clear
, m5D8_40(false)
, m5D8_20(false)
, m5D8_10(false)
, m5D8_08(false)
, mDone(false)
, m5D8_02(false)
, mHalted(false)
, m5D9_80(false)                              // 0x00015D2C
, m5D9_40(false)
, m5D9_20(true)                               // 0x00015D48
, m5D9_10(true) {                             // 0x00015D54
  if (health != nullptr) {
    mHealthInfo = *health; // 0x00015D68: fn_30_15FC4
  }
  SetMomentumWR(CVector3f::Zero()); // 0x00015D74: 0x1C0
  if (mTrailGen.get() != nullptr) {
    mTrailGen->SetParticleEmission(true); // 0x00015DA4
  }
  if (mShieldGen.get() != nullptr) {
    mShieldGen->SetParticleEmission(true);
  }
  if (mChargeUpGen.get() != nullptr) {
    mChargeUpGen->SetParticleEmission(true);
  }
  SetCoefficientOfRestitutionModifier(1.f); // 0x00015DF4
  if (pos.GetX() != 0.f || pos.GetY() != 0.f || pos.GetZ() != 0.f) {
    // 0x00015E5C: the sphere's local box; the ball sits half its height (plus 0.02) above pos.
    const CAABox local = mSphere.CalculateLocalAABox();
    mHalfHeight = (local.GetMaxPoint().GetZ() - local.GetMinPoint().GetZ()) * 0.5f;
    const CVector3f start(pos.GetX(), pos.GetY(), pos.GetZ() + (0.02f + mHalfHeight));
    SetTranslation(start); // 0x00015EA4
    mHomePos = start;
  }
  // 0x00015EC0: with six knots or more the roll ends at the third-from-last knot's time, but no
  // later than the duration less 0.9.
  const int knots = static_cast< int >(mData.mSpeed.GetKnotCount());
  if (knots >= 6) {
    mSplineEndTime = mData.mSpeed.GetKnots()[knots - 3].GetTime();
    const float latest = mData.mSpeed.GetDuration() - 0.4f - 0.5f;
    if (mSplineEndTime >= latest) {
      mSplineEndTime = latest;
    }
  }
  // 0x00015F2C: the filter keeps its include list and excludes kMT_Character (lbl_30_data_C0C).
  CMaterialList exclude = GetMaterialFilter().GetExcludeList();
  exclude.Add(kMT_Character);
  SetMaterialFilter(
      CMaterialFilter::MakeIncludeExclude(GetMaterialFilter().GetIncludeList(), exclude));
}

// @ fn_30_11CD8
CBoostBallGuardianBall::~CBoostBallGuardianBall() {}

// The DOL's TypesMatch for type 148 (TypesMatch.cpp's TYPES_MATCH_IMPL shape).
CEntity* CBoostBallGuardianBall::TypesMatch(int typeId) const {
  if (typeId == kTypeId) {
    return const_cast< CBoostBallGuardianBall* >(this);
  }
  if (typeId > kTypeId) {
    return nullptr;
  }
  return CPhysicsActor::TypesMatch(typeId);
}

// fn_800979F0: TryCast(entity, 0x94).
template <>
CBoostBallGuardianBall* TCastToPtr< CBoostBallGuardianBall >(CEntity* entity) {
  return static_cast< CBoostBallGuardianBall* >(TryCast(entity, CBoostBallGuardianBall::kTypeId));
}

// fn_30_1548C: Think (0x498 bytes) switches on mState - 0 grows in (fn_30_137C0), 1 rolls along
// the speed spline (fn_30_12AB0), 2 shrinks (fn_30_13760, which sets mDone after 0.3 s) - then
// applies the contact damage every 0.2 s and runs the rolling sfx (fn_30_1282C). Not written yet
// (slice b): the ball stays in state 0 and never sets mDone, so the guardian's BoostAttack waits.
void CBoostBallGuardianBall::Think(float dt, CStateManager& mgr) {
  static bool sLogged = false;
  if (!sLogged && DebugBg()) {
    sLogged = true;
    fprintf(stderr, "[ibbg] %04x boost ball: Think fn_30_1548C (0x498) not ported yet\n",
            GetUniqueId().Value());
  }
}

// @ fn_30_13C5C
CHealthInfo* CBoostBallGuardianBall::HealthInfo() {
  return mHealthInfo ? &mHealthInfo.data() : nullptr;
}

// @ fn_30_13C40
const CHealthInfo* CBoostBallGuardianBall::GetHealthInfo() const {
  return mHealthInfo ? &mHealthInfo.data() : nullptr;
}

// @ fn_30_13B90
const CDamageVulnerability* CBoostBallGuardianBall::GetDamageVulnerability() const {
  return &mVulnerability;
}

// @ fn_30_13B88
const CDamageVulnerability* CBoostBallGuardianBall::GetDamageVulnerability(
    const CVector3f&, const CVector3f&, const CDamageInfo&) const {
  return &mVulnerability;
}

// fn_30_13BA4: the collision primitive's box at the actor's transform.
// @ fn_30_13BA4
rstl::optional_object< CAABox > CBoostBallGuardianBall::GetTouchBounds() const {
  return GetCollisionPrimitive()->CalculateAABox(GetTransform());
}

// @ fn_30_1464C
void CBoostBallGuardianBall::Touch(CActor& actor, CStateManager& mgr) {
  CPhysicsActor::Touch(actor, mgr);
}

// @ fn_30_11E44
const CCollisionPrimitive* CBoostBallGuardianBall::GetCollisionPrimitive() const {
  return &mSphere;
}
