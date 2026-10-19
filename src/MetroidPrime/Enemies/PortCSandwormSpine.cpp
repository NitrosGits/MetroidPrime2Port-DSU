// Retail Sandworm REL (module 56): spine segments and collision actors (WP5). Bodies land here per
// docs/module-analysis/Sandworm_functions.tsv.
#include "MetroidPrime/Enemies/CSandworm.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Collision/CMaterialList.hpp"
#include "Kyoto/Animation/CCharLayoutInfo.hpp"
#include "Kyoto/Animation/CJointData_LinearStorage.hpp"
#include "Kyoto/Math/CRelAngle.hpp"

#include <math.h>
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

} // namespace

// @ fn_56_1080C
float CSandworm::SpineHelper1080C(int count) const {
  float sum = 0.f;
  for (int i = 0; i < count; ++i) {
    sum += x131C_[i];
  }
  return sum;
}

// @ fn_56_10038
CVector3f CSandworm::SpineHelper10038() const {
  return CVector3f(0.f, 0.f, x1770_pincers.x8_scale * SegmentHelper5884());
}

// Same as fn_56_1096C but indexes the joint table without the +1.
// @ fn_56_108BC
CVector3f CSandworm::SpineHelper108BC(int idx) const {
  CSegId segId;
  if (x984_segCount > 0) {
    segId = CSegId(static_cast< int >(x988_segIds[idx]));
  } else {
    segId = GetAnimationData()->GetLocatorSegId(rstl::string_l(skSpineJoints[idx].name));
  }
  return GetLctrTransform(segId).GetTranslation();
}

// fn_56_F710 (ported, not exercised yet)
bool CSandworm::Primed(CStateManager& mgr, const CTriggerData& data) const { return true; }

// @ fn_56_5884
float CSandworm::SegmentHelper5884() const {
  const CVector3f scale = GetModelData()->GetScale();
  return (scale.GetX() + scale.GetY() + scale.GetZ()) / 3.f;
}

// @ fn_56_107E8
float CSandworm::SpineHelper107E8() const { return SpineHelper1080C(12); }

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
  if (x984_segCount > 0) {
    segId = CSegId(static_cast< int >(x988_segIds[idx + 1]));
  } else {
    segId = GetAnimationData()->GetLocatorSegId(rstl::string_l(skSpineJoints[idx + 1].name));
  }
  return GetLctrTransform(segId).GetTranslation();
}

// Retail returns only the translation (a CVector3f); the header's CTransform4f is kept and callers
// take GetTranslation().
// fn_56_10A20 (ported, not exercised yet)
CTransform4f CSandworm::GetLctrTransformWrapper(const CSegId& id) const {
  return GetLctrTransform(id);
}

// @ fn_56_10090
CVector3f CSandworm::GetSegmentPosition(int idx) const { return x1210_[idx] + SpineHelper10038(); }

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

  // Retail appends without resetting x984_segCount; a second call would overrun x988_segIds.
  for (int i = 0; i < 15; ++i) {
    const CSegId segId = animData->GetLocatorSegId(rstl::string_l(skSpineJoints[i].name));
    x988_segIds[x984_segCount] = static_cast< u8 >(segId.val());
    ++x984_segCount;
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
  if (AIHintHelper(mgr) == true) {
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
        actor->MoveToWR(SpineHelper108BC(i) + offset, dt);
      } else {
        actor->MoveToWR(SpineHelper108BC(i), dt);
      }
    }
  }
}

// Projects one spine path point onto the floor: two probes one unit to either side of the
// actor's forward axis (Z dropped, normalised), averaged. The point keeps its X/Y.
// @ fn_56_C994
CVector3f CSandworm::SpineHelperC994(CStateManager& mgr, const CVector3f& pt) const {
  CVector3f side = GetTransform().GetForward();
  side.SetZ(0.f); // rodata_150
  if (side.CanBeNormalized()) {
    side.Normalize();
  }
  // The probes are the side vector rotated 90 degrees in XY.
  const float hA = SpineRenderHelperD690(
      mgr, CVector3f(pt.GetX() - side.GetY(), pt.GetY() + side.GetX(), pt.GetZ()));
  const float hB = SpineRenderHelperD690(
      mgr, CVector3f(pt.GetX() + side.GetY(), pt.GetY() - side.GetX(), pt.GetZ()));
  return CVector3f(pt.GetX(), pt.GetY(), 0.5f * (hA + hB)); // rodata_124
}

// Rebuilds the spine path from the actor: the last joint is the actor's position, each earlier
// joint steps 0.5 down (rodata_200; X/Z step rodata_150, i.e. 0) and is re-projected onto the
// floor by fn_56_C994.
// fn_56_C828 (ported, not exercised yet)
void CSandworm::SpineHelperC828(CStateManager& mgr) {
  while (x99C_joints.size() < 180) { // 0xB4 capacity
    x99C_joints.push_back(CVector3f::Zero());
  }
  CVector3f pt = GetTranslation();
  x99C_joints[x99C_joints.size() - 1] = pt;
  for (int i = x99C_joints.size() - 2; i >= 0; --i) {
    pt.SetY(pt.GetY() - 0.5f);
    x99C_joints[i] = SpineHelperC994(mgr, pt);
  }
}

// Index of the joint reached by walking `t` units from the front of the spine path.
// @ fn_56_106D4
int CSandworm::SpineHelper106D4(float t) const {
  int idx = x99C_joints.size() - 1;
  if (idx < 0) {
    return 0; // retail reads the joint before the array here
  }
  float dist = 0.f; // rodata_150
  CVector3f prev = x99C_joints[idx];
  while (dist < t) {
    if (idx == 0) {
      return 0;
    }
    const CVector3f cur = x99C_joints[idx - 1];
    dist += (cur - prev).Magnitude();
    prev = cur;
    --idx;
  }
  return idx;
}

// Retail's erase helper on the raw reserved_vector layout (count at +0, elements at +4), which
// shifts the tail down and returns the slot or end(). fn_56_AD14 erases the front joint, n times.
// @ fn_56_B0B0
void CSandworm::BodyHelperB0B0(rstl::reserved_vector< CVector3f, 180 >& vec, int index) {
  if (index < 0 || index >= vec.size()) {
    return;
  }
  vec.erase(vec.begin() + index);
}

// One joint's animated scale (the pose scales fn_56_A510 grows/shrinks during the death sink).
// @ fn_56_F718
void CSandworm::SpineHelperF718(const CSegId& segId, const CVector3f& scale) const {
  GetAnimationData()->JointData().Scale(static_cast< int >(segId.val())) = scale;
}

// Death squash: X goes 0.9 -> 1.1 over 3 s and Y shrinks to 0 over 5 s, every spine joint.
// @ fn_56_A510
void CSandworm::AddAnimatedScale() {
  const float dead = x14BC_deadTime;
  const float grow = dead > 3.f ? 1.f : dead / 3.f; // rodata_15C, rodata_128
  CAnimData* animData = AnimationData();
  // Retail tests bit 0 of CAnimData+0x2AC, the flag AddAnimatedScale sets; the pose scale array
  // is what it allocates, so the pose's own flag is the equivalent test on the host.
  if (!animData->Pose().HasScale()) {
    animData->AddAnimatedScale();
  }
  // rodata_1A8 + (rodata_180 - dead / rodata_13C), clamped to rodata_128.
  const float shrink = 0.2f + (0.8f - dead / 5.f);
  float y = 1.f;
  if (shrink < 1.f) {
    y = shrink;
  }
  // rodata_1EC is 0.20000005 (0.2f rounded up by the retail builder).
  const CVector3f scale(0.20000005f * grow + 0.9f, y, 1.f); // rodata_1D4
  for (int i = 0; i < 15; ++i) {
    SpineHelperF718(animData->GetLocatorSegId(rstl::string_l(skSpineJoints[i].name)), scale);
  }
}

// @ fn_56_AC24
void CSandworm::SetUnderground(CStateManager& mgr) {
  if (((x8C0_flags >> 7) & 1) == 0) {
    SpineHelperC828(mgr);
  }
  if (x998_collisionActors.get() == nullptr) {
    CreateCollisionActorManager(mgr);
  }
}

// Points the body at the last leg of the radar path (X/Y only), rebuilds the spine path from that
// pose, throws away the joints the body no longer covers, reverses it and mirrors it into the
// array tail so the segments behind the body trail off.
// @ fn_56_AD14
void CSandworm::OrientBodyToPath(CStateManager& mgr) {
  const int radarCount = x1210_.size();
  if (radarCount < 0xD) {
    return;
  }
  const CVector3f last = x1210_[radarCount - 1];
  SetTranslation(last);

  CVector3f dir = last - x1210_[radarCount - 2];
  if (!dir.CanBeNormalized()) {
    dir = last - x1210_[radarCount - 3];
  }
  dir.SetZ(0.f); // rodata_150
  if (!dir.CanBeNormalized()) {
    return;
  }
  dir.Normalize();
  const CQuaternion rot =
      CQuaternion::FromMatrix(CTransform4f::LookAt(CVector3f::Zero(), dir, CVector3f::Up()));
  SetTransform(rot.BuildTransform4f(GetTranslation()));

  // fn_56_107E8's return (the reference-stance length) is fn_56_106D4's argument.
  const int bodyJoints = SpineHelper106D4(SpineHelper107E8());
  for (int i = 0; i < bodyJoints; ++i) {
    BodyHelperB0B0(x99C_joints, 0);
  }

  const int pathCount = x99C_joints.size();
  for (int i = 0; i < pathCount / 2; ++i) {
    const CVector3f tmp = x99C_joints[i];
    x99C_joints[i] = x99C_joints[pathCount - 1 - i];
    x99C_joints[pathCount - 1 - i] = tmp;
  }
  while (x99C_joints.size() < 180) {
    x99C_joints.push_back(CVector3f::Zero());
  }
  // 90 pairs written back to front from the path, clamped to the front joint once it runs out.
  const CVector3f front = x99C_joints[0];
  int k = 0;
  for (int ctr = 0x5A; ctr != 0; --ctr) {
    for (int j = 1; j <= 2; ++j) {
      const int src = pathCount - (k + j);
      x99C_joints[180 - (k + j)] = src < 0 ? front : x99C_joints[src];
    }
    k += 2;
  }

  x134C_ = x8B0_activeTime;
  x14B0_ = 0.f;
  x1824_ = x8B0_activeTime;
  UpdateEyes(mgr, 0.f); // retail also passes a zero vector and 0.f
}

// One joint of the body's joint-space rotation cache (x1670_.x0_).
// @ fn_56_E738
void CSandworm::SpineHelperE738(int index, const CQuaternion& q) {
  if (index < 0 || index >= 13) {
    return; // retail indexes the raw array without a bound
  }
  x1670_.x0_[index] = q;
}

// Quaternion conjugate, i.e. the inverse of a unit quaternion.
// @ fn_56_EF20
CQuaternion CSandworm::SpineHelperEF20(const CQuaternion& q) const {
  return CQuaternion(q.GetScalar(), -q.GetVector());
}

// Segment `index`'s position relative to the actor, rotated into the actor's frame.
// @ fn_56_FFBC
CVector3f CSandworm::SpineHelperFFBC(int index) const {
  if (index < 0 || index >= x1210_.size()) {
    return CVector3f::Zero(); // retail indexes x1210_ unchecked
  }
  return GetTransform().TransposeRotate(GetSegmentPosition(index) - GetTranslation());
}

// Point `t` units down the spine path from the front joint, interpolating between joints.
// @ fn_56_10510
CVector3f CSandworm::SpineHelper10510(float t) const {
  int idx = x99C_joints.size() - 1;
  if (idx < 0) {
    return CVector3f::Zero(); // retail reads the joint before the array here
  }
  float dist = 0.f; // rodata_150
  CVector3f prev = x99C_joints[idx];
  for (;;) {
    if (!(dist < t)) {
      return prev;
    }
    if (idx == 0) {
      return x99C_joints[0];
    }
    const CVector3f cur = x99C_joints[idx - 1];
    const CVector3f delta = cur - prev;
    const float mag = delta.Magnitude();
    if (dist + mag > t) {
      return prev + (t - dist) * delta.AsNormalized();
    }
    prev = cur;
    --idx;
    dist += mag;
  }
}

// Rebuilds the radar trail: point 0 is the last spine joint, point i sits at the reference stance
// length of segments 0..i-1 down the path.
// @ fn_56_FA30
void CSandworm::SpineHelperFA30() {
  while (x1210_.size() < 0xD) { // 13 radar points
    x1210_.push_back(CVector3f::Zero());
  }
  x1210_[0] = x99C_joints.empty() ? CVector3f::Zero() : x99C_joints[x99C_joints.size() - 1];
  for (int i = 1; i < 0xD; ++i) {
    x1210_[i] = SpineHelper10510(SpineHelper1080C(i));
  }
}

// Rebuilds the whole spine path from the twelve spine locator pairs: each leg is sampled 15 times
// (rodata_1F0 = 1/15) and pushed onto the array from the back, re-projected by fn_56_C994.
// @ fn_56_B138
void CSandworm::SpineUpdateHelper(CStateManager& mgr) {
  while (x99C_joints.size() < 180) { // 0xB4 capacity
    x99C_joints.push_back(CVector3f::Zero());
  }
  const float kInvSamples = 0.06666667f; // rodata_1F0
  const CVector3f lift = SpineHelper10038();
  int slot = 179; // retail writes from joints[179] downwards
  for (int leg = 0; leg < 12; ++leg) {
    CVector3f a = GetLocatorPosition(leg);
    const CVector3f step = (GetLocatorPosition(leg + 1) - a) * kInvSamples;
    for (int k = 0; k < 15; ++k) {
      if (slot < 0) {
        break; // retail stops at the front of the array
      }
      x99C_joints[slot] = SpineHelperC994(mgr, a - lift);
      a += step;
      --slot;
    }
  }
}

// Body pose from the cached joint rotations: segment 6 straight from x1670_, the rest of the chain
// blended from the reference stance by each segment's fade, then the actor is moved onto radar
// point 6.
// @ fn_56_F384
void CSandworm::SpineHelperF384() {
  CAnimData* animData = ModelData()->AnimationData();
  CJointData_LinearStorage& jointData = animData->JointData();

  const int seg6 = static_cast< int >(GetSegId(6).val());
  jointData.Translation(seg6) = x1670_.xd4_;
  jointData.Rotation(seg6) = x1670_.xe0_;

  // Segments 5..7 keep the pose they already have.
  for (int i = 0; i < 13; ++i) {
    if (i <= 4 || i >= 8) {
      const CQuaternion q =
          CQuaternion::Slerp(CQuaternion::NoRotation(), x1670_.x0_[i], x1670_.xfc_);
      jointData.Rotation(static_cast< int >(GetSegId(i).val())) = q;
    }
  }
  animData->BuildPose(jointData);
  animData->SetPoseBuilt(true); // retail sets bit 1 of CAnimData+0x2AC

  if (x1210_.size() <= 6) {
    return; // retail reads x1210_[6] unchecked
  }
  const CVector3f pos =
      GetTranslation() + (x1210_[6] - GetLocatorPosition(6)) + SpineHelper10038();
  SetTranslation(pos);
}

// Drops the front joint, and if the body has fallen behind the path (the actor is closer to the
// path pivot than the last joint is) pulls the whole path back onto the actor, then appends the
// actor's floor point with its height clamped to the old last joint (rodata_1BC = 0.05).
// @ fn_56_FC94
void CSandworm::SpineHelperFC94(CStateManager& mgr) {
  if (x1518_bodyCmdFirst > 0) {
    return;
  }
  if (x151C_bodyCmdLast < 0xC) {
    return;
  }
  if (x99C_joints.empty()) {
    return; // retail reads joints[count - 1] unchecked
  }
  const CVector3f last = x99C_joints[x99C_joints.size() - 1];
  CVector3f gap = GetTranslation() - last;
  gap.SetZ(0.f); // rodata_150
  if (gap.Magnitude() < 0.001f) { // rodata_174
    return;
  }

  bool pulled = false;
  BodyHelperB0B0(x99C_joints, 0);
  const CVector3f pivot = x99C_joints[SpineHelper106D4(0.5f * SpineHelper107E8())]; // rodata_124
  CVector3f toPivot = GetTranslation() - pivot;
  toPivot.SetZ(0.f);
  CVector3f toLast = last - pivot;
  toLast.SetZ(0.f);
  if (toPivot.Magnitude() < toLast.Magnitude()) {
    const float dx = GetTranslation().GetX() - last.GetX();
    const float dy = GetTranslation().GetY() - last.GetY();
    for (int i = 0; i < x99C_joints.size(); ++i) {
      CVector3f& p = x99C_joints[i];
      p = CVector3f(p.GetX() + dx, p.GetY() + dy, p.GetZ());
    }
    pulled = true;
  }

  const CVector3f c = SpineHelperC994(mgr, GetTranslation());
  float z = c.GetZ();
  const float lastZ = last.GetZ();
  if (fabsf(z - lastZ) > 0.05f) { // rodata_1BC
    z = z < lastZ ? lastZ - 0.05f : lastZ + 0.05f;
  }
  if (x99C_joints.size() < 180) { // retail writes joints[count] unchecked
    x99C_joints.push_back(CVector3f(c.GetX(), c.GetY(), z));
  }
  if (pulled) {
    SpineHelperFB24(mgr);
  }
}

// Re-aims the body at the line from segment 2 to segment 0 once it is at least 0.7853982 rad
// (rodata_240, 45 degrees) off it; a smaller error keeps the current facing.
// @ fn_56_FB24
void CSandworm::SpineHelperFB24(CStateManager& mgr) {
  if (x1210_.size() < 3) {
    return;
  }
  CVector3f fwd = GetTransform().GetForward();
  fwd.SetZ(0.f); // rodata_150
  CVector3f dir = GetSegmentPosition(0) - GetSegmentPosition(2);
  dir.SetZ(0.f);
  if (!dir.CanBeNormalized()) {
    return;
  }
  if (!fwd.CanBeNormalized()) {
    return;
  }
  dir.Normalize();
  fwd.Normalize();
  if (CVector3f::GetAngleDiff(dir, fwd) < 0.7853982f) { // rodata_240
    return;
  }
  const CQuaternion rot =
      CQuaternion::FromMatrix(CTransform4f::LookAt(CVector3f::Zero(), dir, CVector3f::Up()));
  SetTransform(rot.BuildTransform4f(GetTranslation()));
}

// Lays the spine path straight out along the body's facing, scaled by 2.0 (rodata_120, underground)
// or 1.1 (rodata_1E4) reference stances, then switches the frame's "path rebuilt" bits and
// re-runs fn_56_F5B0.
// fn_56_F77C (ported, not exercised yet)
void CSandworm::SpineHelperF77C(CStateManager& mgr) {
  if (((x8C0_flags >> 7) & 1) == 1) {
    return;
  }
  SpineHelperC600();
  while (x99C_joints.size() < 180) { // 0xB4 capacity
    x99C_joints.push_back(CVector3f::Zero());
  }
  CVector3f fwd = GetTransform().GetForward();
  fwd.SetZ(0.f); // rodata_150
  if (fwd.CanBeNormalized()) {
    fwd.Normalize();
  } else {
    fwd = CVector3f::Forward();
  }
  const float scale = ((x8C0_flags >> 4) & 1) == 1 ? 2.f : 1.1f;
  const float length = scale * SpineHelper107E8();
  const float step = length / 180.f; // rodata_234
  const CVector3f base = GetTranslation() - (0.5f * length) * fwd; // rodata_124
  const CVector3f lift = SpineHelper10038();
  for (int i = 0; i < 180; ++i) {
    const CVector3f pt = base + (step * static_cast< float >(i)) * fwd - lift;
    x99C_joints[i] = SpineHelperC994(mgr, pt);
  }
  x1520_flags |= 0x40;
  // bit7 = bit6 (latched at the top of the next call).
  x1520_flags = static_cast< u8 >((x1520_flags & ~0x80) | ((x1520_flags << 1) & 0x80));
  x8C0_flags &= ~1; // bit0, unknown_0x06dee4c5's low bit
  SpineHelperF5B0(mgr, 0);
  UpdatePose();
}

// Per-frame spine driver: rebuilds the joint table, handles entering/leaving the underground path
// (bit7 of x1520), then either the pose blend (over ground) or the path/body update (underground).
// `index` is unused: retail takes only (this, mgr) and fn_56_F77C leaves two stale args behind.
// @ fn_56_F5B0
void CSandworm::SpineHelperF5B0(CStateManager& mgr, int index) {
  SpineHelperC600();
  if (SpineHelper107E8() == 0.f) {
    return;
  }
  const bool underground = ((x1520_flags >> 7) & 1) == 1;
  const bool wasUnderground = ((x1520_flags >> 6) & 1) == 1;
  if (underground != wasUnderground) {
    if (underground) {
      SpineUpdateHelper(mgr);
      SetTranslation(GetLocatorPosition(0) - SpineHelper10038());
    } else if (x1670_.xfc_ == 0.f) {
      if (x1210_.size() > 6) {
        SetTranslation(x1210_[6]); // retail reads x1210_[6] unchecked
      }
    }
  }
  // bit6 latches bit7.
  x1520_flags = static_cast< u8 >((x1520_flags & ~0x40) | ((x1520_flags >> 1) & 0x40));
  if (((x1520_flags >> 7) & 1) == 0) {
    if (x1670_.xfc_ > 0.f) {
      SpineHelperF384();
    }
  } else {
    SpineHelperFC94(mgr);
    SpineHelperFA30();
    SpineHelperE764();
    SpineHelperF368();
  }
}

// Builds the whole skeleton pose for the underground path: segment 6 from the local segment
// offsets, then the tail (8..12) and head (4..0) segments aimed from their neighbours in the
// parent frame, blended in by each segment's fade.
// @ fn_56_E764
void CSandworm::SpineHelperE764() {
  CVector3f local[13];
  for (int i = 0; i < 13; ++i) {
    local[i] = SpineHelperFFBC(i);
  }
  CAnimData* animData = ModelData()->AnimationData();
  const CCharLayoutInfo* layout = animData->GetCharLayoutInfo();
  animData->BuildPoseIfNecessary();
  CJointData_LinearStorage& jointData = animData->JointData();
  const float invScale = 1.f / GetModelData()->GetScale().GetX(); // rodata_128

  const int seg6 = static_cast< int >(GetSegId(6).val());
  jointData.Translation(seg6) = local[6] * invScale;
  const CRelAngle fullTurn = CRelAngle::FromRadians(6.2831855f); // rodata_230
  const CQuaternion q6 =
      CQuaternion::LookAt(CVector3f::Forward(), CUnitVector3f(local[4] - local[8]), fullTurn);
  jointData.Rotation(seg6) = q6;
  SpineHelperE738(6, q6);
  x1670_.xd4_ = local[6];
  x1670_.xe0_ = q6;

  CQuaternion running = q6;
  for (int i = 8; i < 13; ++i) {
    const CQuaternion inv = SpineHelperEF20(running);
    const int j = i < 12 ? i + 1 : i;
    const CVector3f dir = inv.Transform(local[j] - local[j - 1]);
    const CVector3f fromParent = layout->GetFromParentUnrotated(GetSegId(j));
    const CQuaternion q =
        CQuaternion::LookAt(CUnitVector3f(fromParent), CUnitVector3f(dir), fullTurn);
    const int seg = static_cast< int >(GetSegId(i).val());
    const float fade = x12E8_fade[i];
    if (fade == 1.f) {
      jointData.Rotation(seg) = q;
    } else if (fade > 0.f) {
      jointData.Rotation(seg) = CQuaternion::Slerp(jointData.Rotation(seg), q, fade);
    }
    SpineHelperE738(i, jointData.Rotation(seg));
    running = running * q;
  }
  running = q6;
  for (int i = 4; i >= 0; --i) {
    const CQuaternion inv = SpineHelperEF20(running);
    const int j = i > 0 ? i - 1 : i;
    const CVector3f dir = inv.Transform(local[j] - local[j + 1]);
    const CVector3f fromParent = layout->GetFromParentUnrotated(GetSegId(j));
    const CQuaternion q =
        CQuaternion::LookAt(CUnitVector3f(fromParent), CUnitVector3f(dir), fullTurn);
    const int seg = static_cast< int >(GetSegId(i).val());
    const float fade = x12E8_fade[i];
    if (fade == 1.f) {
      jointData.Rotation(seg) = q;
    } else if (fade > 0.f) {
      jointData.Rotation(seg) = CQuaternion::Slerp(jointData.Rotation(seg), q, fade);
    }
    SpineHelperE738(i, jointData.Rotation(seg));
    running = running * q;
  }
  if (((x14C0_flags >> 7) & 1) == 1) {
    AddAnimatedScale();
  }
  animData->BuildPose(jointData);
  animData->SetPoseBuilt(true); // retail sets bit 1 of CAnimData+0x2AC
}

// FSM state slot 0x218: activating the state rebuilds and straightens the spine path.
// fn_56_F754 (ported, not exercised yet)
void CSandworm::Prime(CStateManager& mgr, EStateMsg msg, float dt) {
  if (msg == kStateMsg_Activate) {
    SpineHelperF77C(mgr);
  }
}
