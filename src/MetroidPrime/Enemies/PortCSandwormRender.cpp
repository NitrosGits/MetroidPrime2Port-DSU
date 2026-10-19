// Retail Sandworm REL (module 56): rendering, shadow, bounds and pose (WP7). Bodies land here per
// docs/module-analysis/Sandworm_functions.tsv.
#include "MetroidPrime/Enemies/CSandworm.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Collision/CRayCastResult.hpp"
#include "Kyoto/Graphics/CColor.hpp"
#include "Kyoto/Graphics/CModelFlags.hpp"
#include "Kyoto/Math/CPlane.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CUnitVector3f.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CProjectedShadow.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"

#include <math.h>

// The pincer models x1770_pincers.xc_leftModel / x5c_rightModel are optional_objects that
// FinishConstruction (fn_56_128B0) only fills when the CMDL id is valid. Retail always reads the
// storage (no check); the port skips a missing model instead of reading an empty optional.
//
// Several WP7 helpers have a different retail signature from the declaration in CSandworm.hpp
// (fn_56_9C88, D690, D898, DBA4, DC28, E008). Until the header is fixed they are file-local
// functions here; fn_56_D83C is the inline CRayCastResult copy and has no body.

namespace {

// rodata_1B8: the degree-to-radian factor every angle here goes through.
const float kDegToRad = 0.017453292f;

const CModelData* PincerModel(const rstl::optional_object< CModelData >& model) {
  return model.valid() ? &model.data() : nullptr;
}

// data_6C0 / data_6BC are both 0x3B (kMT_Solid).
CMaterialFilter SolidFilter() {
  return CMaterialFilter::MakeInclude(CMaterialList(kMT_Solid));
}

// @ fn_56_9C88
void AccumulateModelBounds(CAABox& box, const CModelData* model, const CTransform4f& xf) {
  if (model == nullptr) {
    return;
  }
  const CAABox bounds = model->GetBounds(xf);
  box.AccumulateBounds(bounds.GetMinPoint());
  box.AccumulateBounds(bounds.GetMaxPoint());
}

// @ fn_56_DBA4
// A decaying wobble: sin(t * freq) fading out linearly over 0.7 s, in degrees times `amp`.
CRelAngle Wobble(float t, float freq, float amp) {
  const float s = static_cast< float >(sin(t * freq));
  return CRelAngle::FromRadians(kDegToRad * (amp * (s * ((0.7f - t) / 0.7f))));
}

// @ fn_56_D898
// One claw transform (L_front_claw, R_front_claw, R_back_claw, L_back_claw). `lift` is the
// claw raise in degrees (x181C front, x1820 back); `back` turns the claw round; `mirror` flips
// the wobble for the right/left partner.
CTransform4f ClawTransform(const CSandworm& self, CStateManager& mgr, const char* locator,
                           float lift, bool back, bool mirror) {
  CTransform4f xf = self.GetLctrTransform(rstl::string_l(locator));
  if (back == true) {
    xf.RotateLocalZ(CRelAngle::FromRadians(3.1415927f));
  }
  xf.RotateLocalX(CRelAngle::FromRadians(kDegToRad * lift));

  if (!self.GetAlive()) {
    // Dead: shrink the claws to 40% over 3 s.
    const float t = self.x14BC_deadTime > 3.f ? 1.f : self.x14BC_deadTime / 3.f;
    xf.ScaleBy(0.6f * (1.f - t) + 0.4f);
    return xf;
  }

  const float sinceAttack = self.x8B0_activeTime - self.x1824_;
  if (sinceAttack < 0.7f) {
    const float sign = mirror == true ? -1.f : 1.f;
    xf.RotateLocalZ(Wobble(sinceAttack, 40.f, 3.f * sign));
    xf.RotateLocalX(Wobble(sinceAttack, 60.f, 5.f * sign));
  } else if (back == true && self.CanDropBomb(mgr) == true) {
    // Back claws open (up to 90 degrees) around a bomb drop: x1988 is the last drop time and
    // x1984 the drop timer counting up to the drop rate x1980.
    const float rate = self.x1980_bombDropRate;
    float window = 0.2f;
    if (0.5 * rate < 0.2f) {
      window = 0.5f * rate;
    }
    float open = 0.f;
    const float sinceDrop = self.x8B0_activeTime - self.x1988_;
    if (sinceDrop < window) {
      open = 1.f - sinceDrop / window;
    } else {
      const float toDrop = rate - self.x1984_bombDropTimer;
      if (toDrop < window) {
        open = 1.f - toDrop / window;
      }
    }
    if (open > 0.f) {
      open *= 90.f;
      if (mirror == true) {
        open *= -1.f;
      }
      xf.RotateLocalZ(CRelAngle::FromRadians(kDegToRad * open));
    }
  }
  return xf;
}

// @ fn_56_DC28
// How many degrees (0..29) a front claw has to be raised so its tip, 2.4 along the locator's
// Y axis, clears the floor by 0.6. Back claws always return 0.
float ClawLiftToClearFloor(const CSandworm& self, CStateManager& mgr, const char* locator,
                           bool back) {
  if ((self.x14C0_flags & 0x80) != 0) {
    return 0.f;
  }
  CTransform4f xf = self.GetLctrTransform(rstl::string_l(locator));
  if (back == true) {
    // Retail rotates the copy and drops it.
    xf.RotateLocalZ(CRelAngle::FromRadians(3.1415927f));
    return 0.f;
  }

  CVector3f tip = xf.GetTranslation() + 2.4f * xf.GetColumn(kDY);
  const float floorZ = self.SpineRenderHelperE008(mgr, tip);
  float lift = 0.f;
  // Retail also builds translation + 0.5 * (2.4 * Y) each step and never reads it.
  if (-0.6f + tip.GetZ() >= floorZ) {
    return lift;
  }
  for (float i = 1.f; i < 30.f; i += 1.f) {
    xf.RotateLocalX(CRelAngle::FromRadians(kDegToRad));
    lift += 1.f;
    tip = xf.GetTranslation() + 2.4f * xf.GetColumn(kDY);
    if (-0.6f + tip.GetZ() > floorZ) {
      break;
    }
  }
  return lift;
}

} // namespace

// @ fn_56_E008
// @ fn_56_D83C
// Floor height under `pos`: a 20-unit ray down from 5 above. Retail keeps the filter in a
// function-local static (bss_20).
float CSandworm::SpineRenderHelperE008(const CStateManager& mgr, const CVector3f& pos) const {
  const CRayCastResult res = CGameCollision::RayStaticIntersection(
      mgr, pos + CVector3f(0.f, 0.f, 5.f), CVector3f(0.f, 0.f, -1.f), 20.f, SolidFilter());
  if (!res.IsValid()) {
    return pos.GetZ();
  }
  return res.GetPoint().GetZ();
}

// @ fn_56_D690
// Floor height under `pos`: 20 units down from 2 above, then 40 units down from 6 above.
// Called by fn_56_C994 (WP5) and fn_56_EF8C (WP6); `this` is unused.
float CSandworm::SpineRenderHelperD690(const CStateManager& mgr, const CVector3f& pos) const {
  const CMaterialFilter filter = SolidFilter();
  const CVector3f down(0.f, 0.f, -1.f);
  CRayCastResult res = CGameCollision::RayStaticIntersection(
      mgr, CVector3f(pos.GetX(), pos.GetY(), 2.f + pos.GetZ()), down, 20.f, filter);
  if (res.IsValid()) {
    return res.GetPoint().GetZ();
  }
  res = CGameCollision::RayStaticIntersection(
      mgr, CVector3f(pos.GetX(), pos.GetY(), 6.f + pos.GetZ()), down, 40.f, filter);
  if (res.IsValid()) {
    return res.GetPoint().GetZ();
  }
  return pos.GetZ();
}


// @ fn_56_A4F0
void CSandworm::PreRender(CStateManager& mgr) { CPatterned::PreRender(mgr); }

// @ fn_56_4A14
void CSandworm::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
  if (x19D0_glow.x14_gen2 != nullptr && x19D0_glow.x14_gen2->GetParticleEmission() == true) {
    gpRender->AddParticleGen(*x19D0_glow.x14_gen2);
  }
  if (x19D0_glow.x1c_gen3 != nullptr && x19D0_glow.x1c_gen3->GetParticleEmission() == true) {
    gpRender->AddParticleGen(*x19D0_glow.x1c_gen3);
  }
}

// @ fn_56_E3C0
void CSandworm::Render(const CStateManager& mgr) const {
  if ((x14C0_flags & 0x40) != 0) {
    return;
  }
  CPatterned::Render(mgr);

  // The eye glows draw while the first one emits; otherwise the Bomb Guardian glows do.
  if (x19D0_glow.x4_gen0 != nullptr && x19D0_glow.x4_gen0->GetParticleEmission() == true) {
    x19D0_glow.x4_gen0->Render();
    if (x19D0_glow.xc_gen1 != nullptr && x19D0_glow.xc_gen1->GetParticleEmission() == true) {
      x19D0_glow.xc_gen1->Render();
    }
  } else {
    if (x19D0_glow.x14_gen2 != nullptr && x19D0_glow.x14_gen2->GetParticleEmission() == true) {
      x19D0_glow.x14_gen2->Render();
    }
    if (x19D0_glow.x1c_gen3 != nullptr && x19D0_glow.x1c_gen3->GetParticleEmission() == true) {
      x19D0_glow.x1c_gen3->Render();
    }
  }

  // Clip plane 1000 below the origin, facing up: keeps the whole model.
  const CVector3f up = CVector3f::Up();
  const CVector3f pos = GetTranslation();
  const CVector3f planePoint(pos.GetX() + 0.f, pos.GetY() + 0.f, pos.GetZ() + -1000.f);
  GetModelData()->SetupWorldSpacePortalPlane(
      GetTransform(), CPlane(planePoint, CUnitVector3f(up, CUnitVector3f::kN_No)));

  const CModelData* left = PincerModel(x1770_pincers.xc_leftModel);
  const CModelData* right = PincerModel(x1770_pincers.x5c_rightModel);
  if (ShouldRenderTailEye(mgr) == true) {
    if (left != nullptr) {
      RenderModelHelper(mgr, *left, x13C0_[0]);
    }
    if (right != nullptr) {
      RenderModelHelper(mgr, *right, x13C0_[1]);
    }
  }
  if (ShouldRenderHeadEye(mgr) == true) {
    if (left != nullptr) {
      RenderModelHelper(mgr, *left, x13C0_[2]);
    }
    if (right != nullptr) {
      RenderModelHelper(mgr, *right, x13C0_[3]);
    }
  }
}

// @ fn_56_DB6C
void CSandworm::RenderModelHelper(const CStateManager& mgr, const CModelData& model,
                                  const CTransform4f& xf) const {
  model.Render(mgr, xf, GetActorLights(), GetModelFlags());
}

// @ fn_56_9CD8
void CSandworm::PreRenderAllViewports(CStateManager& mgr) {
  CPatterned::PreRenderAllViewports(mgr);

  if ((x14C0_flags & 0x80) != 0) {
    // Fade to black over 7 s, except after a Dark or Annihilator combo kill.
    const float brightness = 1.f - x14BC_deadTime / 7.f;
    const CWeaponMode mode = GetHealthInfo()->GetCauseOfDeathWeapon();
    if (!mode.IsComboed() ||
        (mode.GetRawType() != kWT_Dark && mode.GetRawType() != kWT_Annihilator)) {
      SetModelFlags(
          CModelFlags::AlphaBlended(CColor(brightness, brightness, brightness, 1.f)));
    }
  }

  // Header declares an extra int; retail passes only mgr (see report).
  SpineHelperF5B0(mgr, 0);
  x13C0_[0] = ClawTransform(*this, mgr, "L_front_claw", x181C_, false, false);
  x13C0_[1] = ClawTransform(*this, mgr, "R_front_claw", x181C_, false, true);
  x13C0_[2] = ClawTransform(*this, mgr, "R_back_claw", x1820_, true, false);
  x13C0_[3] = ClawTransform(*this, mgr, "L_back_claw", x1820_, true, true);

  if ((x15AC_toss.x58_flags & 0x80) != 0) {
    TeleportGrabbedPlayer(mgr);
  }

  float a = ClawLiftToClearFloor(*this, mgr, "L_front_claw", false);
  float b = ClawLiftToClearFloor(*this, mgr, "R_front_claw", false);
  if (a > b) {
    b = a;
  }
  if (b > x181C_) {
    x181C_ = b;
  }
  a = ClawLiftToClearFloor(*this, mgr, "R_back_claw", true);
  b = ClawLiftToClearFloor(*this, mgr, "L_back_claw", true);
  if (a > b) {
    b = a;
  }
  if (b > x1820_) {
    x1820_ = b;
  }

  UpdateLocatorTransforms();
  UpdateEyes(mgr, 0.f);
  if (mgr.GetPlayerState()->GetActiveVisor(mgr) == CPlayerState::kPV_Scan) {
    x8C1_flags |= 0x40;
  } else {
    x8C1_flags &= ~0x40;
  }
  ModelData()->AdvanceParticles(GetTransform(), 0.f, mgr);

  const CAABox bounds = ComputeBounds();
  SetOtherBounds(bounds);
  SetRenderBounds(bounds);
  UpdateShadow(mgr);
  UpdatePortalSystemState(mgr);
}

// @ fn_56_9A58
void CSandworm::UpdateShadow(CStateManager& mgr) {
  int count = 1;
  if (ShouldRenderTailEye(mgr) == true) {
    count = 3;
  }
  if (ShouldRenderHeadEye(mgr) == true) {
    count += 2;
  }

  const CModelData* left = PincerModel(x1770_pincers.xc_leftModel);
  const CModelData* right = PincerModel(x1770_pincers.x5c_rightModel);
  const CModelData* models[5] = {GetModelData(), left, right, left, right};
  const CTransform4f* xfs[5] = {&GetTransform(), &x13C0_[0], &x13C0_[1], &x13C0_[2],
                                &x13C0_[3]};
  if (ShouldRenderHeadEye(mgr) == true && ShouldRenderTailEye(mgr) == false) {
    xfs[1] = xfs[3];
    xfs[2] = xfs[4];
  }
  if (left == nullptr || right == nullptr) {
    count = 1; // port guard, see the file comment
  }

  x13BC_shadow->SetBounds(GetOtherBounds());
  x13BC_shadow->RenderShadowBuffer(mgr, count, models, xfs, 0, CVector3f::Zero(), 1.1f, 10.f);
  x13BC_shadow->SetOpacity((x8C0_flags & 0x01) != 0 ? 0.f : 0.5f);
}

// @ fn_56_9C10
void CSandworm::CreateProjectedShadow() {
  x13BC_shadow = rs_new CProjectedShadow(0x80, 0x80, 0, 1);
}

// @ fn_56_43A4
void CSandworm::UpdateBoundsHelper() {
  if (x8B0_activeTime > x1A24_) {
    x1A24_ = x8B0_activeTime;
    x1A0C_bounds = ComputeBounds();
  }
}

// @ fn_56_A04C
CAABox CSandworm::ComputeBounds() const {
  CAABox box = CAABox::MakeMaxInvertedBox();
  if ((x1520_flags & 0x80) == 0) {
    if (HasModelData()) {
      box = GetModelData()->GetBounds(GetTransform());
    } else {
      box = CAABox(GetTranslation(), GetTranslation());
    }
  } else {
    const CVector3f extent(3.f, 3.f, 1.5f);
    for (int i = 0; i < x1210_.size(); ++i) {
      const CVector3f locator = GetLocatorPosition(i);
      box.AccumulateBounds(x1210_[i] - extent);
      box.AccumulateBounds(x1210_[i] + extent);
      box.AccumulateBounds(locator - extent);
      box.AccumulateBounds(locator + extent);
    }
  }

  const CModelData* left = PincerModel(x1770_pincers.xc_leftModel);
  const CModelData* right = PincerModel(x1770_pincers.x5c_rightModel);
  AccumulateModelBounds(box, left, x13C0_[0]);
  AccumulateModelBounds(box, right, x13C0_[1]);
  AccumulateModelBounds(box, left, x13C0_[2]);
  AccumulateModelBounds(box, right, x13C0_[3]);
  return box;
}

// @ fn_56_A348
void CSandworm::UpdateLocatorTransforms() {
  x1528_spit.x28_xf = GetLctrTransform(rstl::string_l("acid_1_LCTR"));
  x15AC_toss.x0_xf = GetLctrTransform(rstl::string_l("attach_LCTR"));
  x1608_residuePos = GetLctrTransform(rstl::string_l("eye_front_LCTR")).GetTranslation();
  x1614_residuePosHighHealth = GetLctrTransform(rstl::string_l("eye_back_LCTR")).GetTranslation();
  x1388_ = GetLocatorPosition(6);
  x1394_snatchStart = GetLocatorPosition(0);
  x13A0_snatchEnd = GetLocatorPosition(12);
  x8C0_flags |= 0x02;
}

// @ fn_56_11078
void CSandworm::UpdatePose() {
  if ((x8C0_flags & 0x02) == 0) {
    GetAnimationData()->BuildPoseIfNecessary();
    UpdateLocatorTransforms();
  }
}

// @ fn_56_2994
// CPatterned::RenderIngSnatchingTransition, except the normal-model half is drawn directly at
// 0.99 scale instead of through CPhysicsActor::Render.
void CSandworm::RenderIngSnatchingTransition(const CStateManager& mgr) const {
  const CVector3f normal = GetIngSnatchingNormal(mIngPossessionBlend);
  const CVector3f point = GetIngSnatchingPoint(mIngPossessionBlend);
  CAnimData* animData = const_cast< CAnimData* >(GetAnimationData());
  const CVector3f overlap = (0.5f * GetIngSnatchingModelOverlapSize()) * normal;

  const CPlane ingPlane(point - overlap, CUnitVector3f(normal, CUnitVector3f::kN_No));
  GetModelData()->SetupWorldSpacePortalPlane(GetTransform(), ingPlane);
  animData->SetSkinnedModel(*mIngModel);
  CPhysicsActor::Render(mgr);

  const CPlane normalPlane(point + overlap, CUnitVector3f(-1.f * normal, CUnitVector3f::kN_No));
  GetModelData()->SetupWorldSpacePortalPlane(GetTransform(), normalPlane);
  animData->SetSkinnedModel(mNormalModel);

  const CModelData::EWhichModel which = CModelData::GetRenderingModel(mgr);
  CTransform4f xf = GetTransform();
  xf.ScaleBy(0.99f);
  GetModelData()->Render(which, xf, GetActorLights(), GetModelFlags());
}
