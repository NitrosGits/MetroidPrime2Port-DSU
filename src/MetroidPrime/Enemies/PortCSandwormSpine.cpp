// Retail Sandworm REL (module 56): spine segments and collision actors (WP5). Bodies land here per
// docs/module-analysis/Sandworm_functions.tsv.
#include "MetroidPrime/Enemies/CSandworm.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Collision/CMaterialList.hpp"
#include "Kyoto/Animation/CCharLayoutInfo.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CDamageVulnerability.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Collision/CJointCollisionDescription.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include "rstl/string.hpp"
#include "rstl/vector.hpp"

namespace {

// lbl_56_rodata_34: one entry per spine collision sphere, front to back.
struct SSandwormSpineJointDesc {
  const char* name;
  float radius;      // +4: scaled by fn_56_5884 in fn_56_D09C; raw in fn_56_CBB4 (not morphed)
  float morphRadius; // +8: raw in fn_56_CBB4 while the player is morphed
};

const SSandwormSpineJointDesc skSpineJoints[15] = {
    {"front_spine_7", 2.4f, 1.0f}, {"front_spine_6", 1.2f, 0.8f}, {"front_spine_5", 1.1f, 0.6f},
    {"front_spine_4", 1.1f, 0.6f}, {"front_spine_3", 1.1f, 0.6f}, {"front_spine_2", 1.1f, 0.6f},
    {"front_spine_1", 1.1f, 0.6f}, {"Skeleton_Root", 1.1f, 0.6f}, {"back_spine_1", 1.1f, 0.6f},
    {"back_spine_2", 1.1f, 0.6f},  {"back_spine_3", 1.1f, 0.6f},  {"back_spine_4", 1.1f, 0.6f},
    {"back_spine_5", 1.1f, 0.6f},  {"back_spine_6", 1.2f, 0.8f},  {"back_spine_7", 2.4f, 1.0f},
};

// WORKAROUND (header): retail fn_56_1080C returns float; CSandworm.hpp declares it void.
// @ fn_56_1080C
float SumSegmentLengths(const CSandworm& self, int count) {
  float sum = 0.f;
  for (int i = 0; i < count; ++i) {
    sum += self.x131C_[i];
  }
  return sum;
}

// WORKAROUND (header): retail fn_56_10038 returns CVector3f; CSandworm.hpp declares it void.
// @ fn_56_10038
CVector3f PincerOffset(const CSandworm& self) {
  return CVector3f(0.f, 0.f, self.x1770_pincers.x8_scale * self.SegmentHelper5884());
}

CSegId CachedSegId(const CSandworm& self, int idx) {
  return CSegId(static_cast< int >(self.x988_[idx]));
}

// WORKAROUND (header): retail fn_56_108BC returns CVector3f; CSandworm.hpp declares it void.
// Same as fn_56_1096C but indexes the joint table without the +1.
// @ fn_56_108BC
CVector3f JointLocatorPosition(const CSandworm& self, int idx) {
  CSegId segId;
  if (self.x984_ > 0) {
    segId = CachedSegId(self, idx);
  } else {
    segId = self.GetAnimationData()->GetLocatorSegId(rstl::string_l(skSpineJoints[idx].name));
  }
  return self.GetLctrTransform(segId).GetTranslation();
}

} // namespace

// @ fn_56_F710
bool CSandworm::Primed(CStateManager& mgr, const CTriggerData& data) const { return true; }

// @ fn_56_5884
float CSandworm::SegmentHelper5884() const {
  const CVector3f scale = GetModelData()->GetScale();
  return (scale.GetX() + scale.GetY() + scale.GetZ()) / 3.f;
}

// @ fn_56_107E8
float CSandworm::SpineHelper107E8() const { return SumSegmentLengths(*this, 12); }

// Caches the reference-stance length of each spine segment once.
// fn_56_C704/fn_56_C7F4 are the inlined CCharLayoutInfo::GetFromParentUnrotated and its
// parent-valid test (converter byte != 0x63).
// @ fn_56_C600
// @ fn_56_C704
// @ fn_56_C7F4
void CSandworm::SpineHelperC600() {
  if (SpineHelper107E8() > 0.f) {
    return;
  }
  const float scale = GetModelData()->GetScale().GetX();
  const CCharLayoutInfo* layout = GetAnimationData()->GetCharLayoutInfo();
  for (int i = 0; i < 6; ++i) {
    x131C_[i] = scale * layout->GetFromParentUnrotated(x970_segIds[i]).Magnitude();
  }
  // Retail never writes x131C_[6]: the second loop starts at 7 and reads segIds[i + 1], so
  // segment 6 -> 7 is skipped. Uncertain whether x131C_[6] is set elsewhere (zeroed in the ctor).
  for (int i = 7; i < 12; ++i) {
    x131C_[i] = scale * layout->GetFromParentUnrotated(x970_segIds[i + 1]).Magnitude();
  }
}

// @ fn_56_E728
CSegId CSandworm::GetSegId(int idx) const { return x970_segIds[idx]; }

// @ fn_56_E6BC
CSegId CSandworm::GetSegLocatorId(int idx) const {
  return GetAnimationData()->GetLocatorSegId(rstl::string_l(skSpineJoints[idx + 1].name));
}

// @ fn_56_1096C
CVector3f CSandworm::GetLocatorPosition(int idx) const {
  CSegId segId;
  if (x984_ > 0) {
    segId = CachedSegId(*this, idx + 1);
  } else {
    segId = GetAnimationData()->GetLocatorSegId(rstl::string_l(skSpineJoints[idx + 1].name));
  }
  return GetLctrTransform(segId).GetTranslation();
}

// Retail returns only the translation (a CVector3f); the header's CTransform4f is kept and callers
// take GetTranslation().
// @ fn_56_10A20
CTransform4f CSandworm::GetLctrTransformWrapper(const CSegId& id) const {
  return GetLctrTransform(id);
}

// @ fn_56_10090
CVector3f CSandworm::GetSegmentPosition(int idx) const { return x1210_[idx] + PincerOffset(*this); }

// @ fn_56_F368
void CSandworm::SpineHelperF368() { x1670_.xf0_ = GetTranslation(); }

// @ fn_56_CC8C
// @ fn_56_D09C
void CSandworm::CreateCollisionActorManager(CStateManager& mgr) {
  rstl::vector< CJointCollisionDescription > descs;
  descs.reserve(15);
  // fn_56_D09C (inlined): one sphere per joint whose locator exists.
  const CAnimData* animData = GetAnimationData();
  for (int i = 0; i < 15; ++i) {
    const CSegId segId = animData->GetLocatorSegId(rstl::string_l(skSpineJoints[i].name));
    if (segId != CSegId::Invalid()) {
      descs.push_back(CJointCollisionDescription::SphereCollision(
          segId, CVector3f::Zero(), skSpineJoints[i].radius * SegmentHelper5884(),
          rstl::string_l(skSpineJoints[i].name), 10000.f));
    }
  }

  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
      CMaterialList(kMT_Solid), CMaterialList(kMT_CollisionActor, kMT_AIPassthrough, kMT_Player)));
  AddMaterial(kMT_ProjectilePassthrough, mgr);
  AddMaterial(kMT_RadarObject, mgr);
  x998_collisionActors = new CCollisionActorManager(mgr, GetUniqueId(), GetCurrentAreaId(), descs,
                                                    GetActive());

  // Retail assumes all 15 locators exist: no null check on the cast and no bound on the
  // manager's actor count.
  for (int i = 0; i < 15; ++i) {
    CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(
        x998_collisionActors->GetCollisionDescFromIndex(i).GetCollisionActorId()));
    CHealthInfo* health = actor->HealthInfo();
    health->SetKnockbackResistance(100000.f);
    health->SetHP(10000.f);
    if (i == 14) {
      actor->SetDamageVulnerability(GetDamageVulnerability()->MakeIgnoreRadius());
    } else {
      actor->SetDamageVulnerability(CDamageVulnerability::ReflectVulnerabilty());
    }
  }
  SetMass(20000.f);
  RemoveMaterial(kMT_Target, kMT_Orbit, kMT_SeekerTarget, mgr);

  // Retail appends without resetting x984_; a second call would overrun x988_.
  for (int i = 0; i < 15; ++i) {
    const CSegId segId = animData->GetLocatorSegId(rstl::string_l(skSpineJoints[i].name));
    x988_[x984_] = static_cast< u8 >(segId.val());
    ++x984_;
  }
}

// Shrinks the spheres while the player is morphed (state 1) and restores them otherwise.
// @ fn_56_CBB4
void CSandworm::SpineHelperCBB4(CStateManager& mgr, int morphed) {
  if (x998_collisionActors.get() == nullptr || ((x8C0_flags >> 4) & 1) == 1) {
    return;
  }
  // Retail fn_56_3048 is (this, mgr) -> bool with no out parameter; the header (WP4) declares an
  // out vector, which is ignored here.
  CVector3f unused = CVector3f::Zero();
  if (AIHintHelper(mgr, unused) == true) {
    return;
  }
  if (morphed == x7C0_) {
    return;
  }
  for (int i = 0; i < 15; ++i) {
    if (CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(
            x998_collisionActors->GetCollisionDescFromIndex(i).GetCollisionActorId()))) {
      // Retail passes the table radii unscaled here, unlike fn_56_D09C.
      actor->SetSphereRadius(morphed == 1 ? skSpineJoints[i].morphRadius : skSpineJoints[i].radius);
    }
  }
  x7C0_ = morphed;
}

// @ fn_56_D350
void CSandworm::UpdateCollisionActors(CStateManager& mgr, float dt) {
  if (x998_collisionActors.get() == nullptr || !GetActive()) {
    return;
  }
  SpineHelperCBB4(mgr, mgr.GetPlayer(0)->GetMorphballTransitionState() == CPlayer::kMS_Morphed);

  CVector3f offset;
  if (x1210_.size() > 6) {
    offset = GetSegmentPosition(6) - GetLocatorPosition(6);
  } else {
    // Retail quirk: without the radar trail the offset is the raw locator position, not zero.
    // Only used when x1520 bit7 is set, so possibly unreachable.
    offset = GetLocatorPosition(6);
  }

  for (int i = 0; i < 15; ++i) {
    if (CCollisionActor* actor = TCastToPtr< CCollisionActor >(mgr.ObjectById(
            x998_collisionActors->GetCollisionDescFromIndex(i).GetCollisionActorId()))) {
      actor->SetActive(true);
      if (((x1520_flags >> 7) & 1) == 1) {
        actor->MoveToWR(JointLocatorPosition(*this, i) + offset, dt);
      } else {
        actor->MoveToWR(JointLocatorPosition(*this, i), dt);
      }
    }
  }
}
