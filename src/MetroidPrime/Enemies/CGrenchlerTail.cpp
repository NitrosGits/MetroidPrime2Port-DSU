#include "MetroidPrime/Enemies/CGrenchlerTail.hpp"

#include "Collision/CCollisionInfoList.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CStateManager.hpp"

// Retail `__sinit_CGrenchlerTail_cpp` (0x156DC-0x15738) has no tables and no stores at all: it only
// builds the `skRelName` string - rstl::string_l(.rodata 0x8AF "Grenchler.rel") at 0x156F8, the copy
// ctor into .bss 0x54 at 0x15708, __register_global_object (dtor 0x15790, chain .bss 0x48) at
// 0x1571C, internal_dereference on the temporary at 0x15724. Frame stwu 0x156DC (-0x20) / lmw
// 0x15728; no branch. Host: the same static, `skRelName = rstl::string_l("Grenchler.rel")` below.
// @ fn_27_156DC
static const rstl::string skRelName = rstl::string_l("Grenchler.rel");

// @ fn_27_1536C
CGrenchlerTail::CGrenchlerTail(TUniqueId uid, TAreaId areaId, const CModelData& modelData,
                               int animId, const CTransform4f& xf, bool isSmall)
: CPhysicsActor(uid, rstl::string_l("Grenchler Tail"),
                CEntityInfo(areaId, rstl::vector< SConnection >(), true), 0, xf, modelData,
                CMaterialList(),
                CAABox(CVector3f(-0.4f, -0.4f, -0.4f), CVector3f(0.4f, 0.4f, 0.4f)),
                SMoverData(isSmall ? 0.3f : 0.75f), CActorParameters::None(),
                CPhysicsActor::skDefaultStepData)
, mElapsedTime(0.f)
, mBounceCount(0.f)
, mRelToken(skRelName, 0)
, mIsSmall(isSmall) {
  CAnimPlaybackParms parms(animId, -1, 1.f, true);
  ModelData()->AnimationData()->SetAnimation(parms, false);
  ModelData()->EnableLooping(false);

  float scale = 1.f;
  if (isSmall == true) {
    scale = 0.1f;
  }
  CVector3f velocity = xf.GetForward() * -1.f;
  velocity.SetZ(3.f * scale);
  SetVelocityWR(velocity);
  SetMomentumWR(CVector3f(0.f, 0.f, -40.f * scale));
  SetAngularVelocityOR(
      CAxisAngle::FromQuaternion(CQuaternion::ZRotation(CRelAngle::FromDegrees(250.f * scale))));
}

// @ fn_27_152B8
void CGrenchlerTail::Think(float dt, CStateManager& mgr) {
  AddMaterial(kMT_Solid, mgr);
  CActor::Think(dt, mgr);
  UpdateAnimation(dt, mgr, true);
  mElapsedTime += dt;
  if (mElapsedTime > 6.f) {
    mgr.DeleteObjectRequest(GetUniqueId());
  }
}

// @ fn_27_151E0
rstl::optional_object< CAABox > CGrenchlerTail::GetTouchBounds() const {
  const CVector3f& pos = GetTranslation();
  rstl::optional_object< CAABox > bounds(
      CAABox(pos - CVector3f(0.4f, 0.4f, 0.4f), pos + CVector3f(0.4f, 0.4f, 0.4f)));
  return bounds;
}

// @ fn_27_1513C
void CGrenchlerTail::PreRender(CStateManager& mgr) {
  CActor::PreRender(mgr);
  SetModelFlags(CModelFlags(CModelFlags::kT_Blend, 1.f - mElapsedTime / 6.f));
}

// @ fn_27_14FE0
void CGrenchlerTail::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                                  CStateManager& mgr) {
  static CMaterialList floor(kMT_Floor);
  for (int i = 0; i < list.GetCount(); ++i) {
    if (list[i].GetMaterialLeft().SharesMaterials(floor)) {
      mBounceCount += 1.f;
      if (mBounceCount >= 3.f || mIsSmall == true) {
        StopMoving();
      } else {
        SetVelocityWR(CVector3f(0.f, 0.f, 16.f / mBounceCount));
      }
      break;
    }
  }
}

// @ fn_27_14F70
void CGrenchlerTail::StopMoving() {
  SetVelocityWR(CVector3f::Zero());
  SetMomentumWR(CVector3f::Zero());
  SetAngularVelocityOR(CAxisAngle::FromQuaternion(CQuaternion::NoRotation()));
}
