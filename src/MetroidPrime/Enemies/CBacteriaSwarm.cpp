#include "MetroidPrime/Enemies/CBacteriaSwarm.hpp"

#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CStateManager.hpp"

#include "Collision/CCollidableSphere.hpp"
#include "Collision/CRayCastResult.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Graphics/CGX.hpp"
#include "Kyoto/Graphics/CGraphics.hpp"
#include "Kyoto/Graphics/CLight.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CSphere.hpp"
#include "Kyoto/Math/CUnitVector3f.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "Kyoto/Particles/CGenDescription.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/CActorLights.hpp"
#include "MetroidPrime/CBasicSwarmData.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CObjectList.hpp"
#include "MetroidPrime/CSafeZoneManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Cameras/CFirstPersonCamera.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrBacteriaSwarm.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptDoor.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "REL/REL_Setup.h"
#include "WorldFormat/CAreaOctTree.hpp"
#include "WorldFormat/CMetroidAreaCollider.hpp"
#include "rstl/math.hpp"

#include <float.h>

// @ fn_6_6EDC
CTransform4f CBacteriaSwarm::ShortestRotationArcWrapped(const CVector3f& a, const CVector3f& b,
                                                        const CRelAngle& angle) {
  const float dot = CVector3f::Dot(a, b);
  if (close_enough(dot, 1.f)) {
    return CTransform4f::Identity();
  }
  if (dot > -0.99981f) {
    return CQuaternion::ShortestRotationArcClamped(a, b, angle).BuildTransform4f();
  }
  if (!(a == CVector3f::Right()) && !(b == CVector3f::Right())) {
    return CQuaternion::AxisAngle(CUnitVector3f(CVector3f::Cross(a, CVector3f::Right())), angle)
        .BuildTransform4f();
  }
  return CQuaternion::AxisAngle(CUnitVector3f(CVector3f::Cross(a, CVector3f::Up())), angle)
      .BuildTransform4f();
}

// @ fn_6_6DB4
CBacteriaSwarm::CBoid::CBoid(const CTransform4f& xf, uint index)
: mTransform(xf)
, mVelocity(0.f, 0.f, 0.f)
, mAmbientLighting(0.3f, 0.3f, 0.3f, 1.f)
, mNext(nullptr)
, mSurface(CVector3f(1.f, 0.f, 0.f), CVector3f(0.f, 1.f, 0.f), CVector3f(0.f, 0.f, 1.f), ~0)
//, mSpeed(0.f)
, mColorBlend(0.f)
, mIndex(index)
, mActive(false)
, mInFrustum(false)
, mInSafeZone(false)
, mPursuingPlayer(false) {}

// @ fn_6_5F10
// 0x5FB0-0x6198: actor args and shifted Scannable/Trigger/NonSolidDamageable/RadarObject bits.
// 0x6214-0x63BC: empty vectors, 125 null partitions, data-derived radii/priorities/counts,
// timers 0/5, probe scale 1.5 and degree-to-radian turn-speed conversion (rodata_68).
// 0x63C4-0x6450: sounds/timing/volume, lock index -1, zero positions and blend.
CBacteriaSwarm::CBacteriaSwarm(
    const TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
    const CVector3f& boundingBoxExtent, const CTransform4f& xf, CActorParameters actorParameters,
    const CBasicSwarmData& data, const float surfaceStickPriority, const float containmentPriority,
    const float patrolTurnSpeed, const float avoidSafeZoneTurnSpeed, const float patrolSpeed,
    const float safeZoneEscapeSpeed, const float playerPursuitSpeed, const float acceleration,
    const float deceleration, const CAssetId particleEffect, const CColor& patrolColor,
    const CColor& pursuitColor, const float colorChangeTime, const float minPatrolSoundTime,
    const float maxPatrolSoundTime, const float patrolSoundWeight, const float minPursuitSoundTime,
    const float maxPursuitSoundTime, const float pursuitSoundWeight, const ushort patrolSound,
    const ushort pursuitSound, const float soundFallOff, const float maxAudibleDistance,
    const uchar minVolume, const uchar maxVolume, const CStaticRes& scanModel,
    const bool spawnInstantly, const bool unknownFlag)
: CActor(uid, name, info, 0, xf,
         scanModel.GetId() == kInvalidAssetId ? CModelData(CModelData::None())
                                              : CModelData(scanModel),
         CMaterialList(kMT_Scannable, kMT_Trigger, kMT_NonSolidDamageable, kMT_RadarObject),
         actorParameters, kInvalidUniqueId)
, mAabox(CVector3f(0.f, 0.f, 0.f), CVector3f(0.f, 0.f, 0.f))
, mThinkCounter(0)
, mOccludedTimer(5.f)
, mBoundingBoxExtent(boundingBoxExtent)
, mSeparationRadius(data.mInfluenceRadius)
, mCohesionMagnitude(data.mCohesionPriority)
, mSeparationMagnitude(data.mSeparationPriority)
, mAttractionMagnitude(data.mPlayerAttractPriority)
, mAttractionRadius(data.mPlayerAttractDistance)
, mPartitionedBoidLists(125, nullptr)
, mOutlierBoidList(nullptr)
, mBoidGenRate(data.mSpawnSpeed)
, mBoidGenCooldownTimer(0.f)
, mDamageCooldownTimer(0.f)
, mDamageCooldown(data.mDamageWaitTime)
, mBoidRadius(data.mCollisionRadius)
, mTouchRadius(data.mTouchRadius)
, mPlayerTouchRadius(data.mDamageRadius)
, mDamage(data.mContactDamage)
, mNumBoids(data.mCount)
, mMaxCreatedBoids(data.mMaxCount)
, mCreatedBoids(0)
, mSurfaceProbeScale(1.5f)
, mSafeZoneAvoidance(data.mSafeZoneAvoidancePriority)
, mSurfaceStickPriority(surfaceStickPriority)
, mContainmentPriority(containmentPriority)
, mPatrolTurnSpeed((M_PIF / 180.f) * patrolTurnSpeed)
, mAvoidSafeZoneTurnSpeed((M_PIF / 180.f) * avoidSafeZoneTurnSpeed)
, mPatrolSpeed(patrolSpeed)
, mSafeZoneEscapeSpeed(safeZoneEscapeSpeed)
, mPlayerPursuitSpeed(playerPursuitSpeed)
, mAcceleration(acceleration)
, mDeceleration(deceleration)
, mColorChangeTime(colorChangeTime)
, mPatrolColor(patrolColor)
, mPursuitColor(pursuitColor)
, mNumDeathParticles(data.mNumDeathParticles)
, mPatrolSound(patrolSound)
, mMinPatrolSoundTime(minPatrolSoundTime)
, mMaxPatrolSoundTime(maxPatrolSoundTime)
, mNextPatrolSoundTime(minPatrolSoundTime)
, mPatrolSoundWeight(patrolSoundWeight)
, mPatrolSoundTimer(0.f)
, mPursuitSound(pursuitSound)
, mMinPursuitSoundTime(minPursuitSoundTime)
, mMaxPursuitSoundTime(maxPursuitSoundTime)
, mNextPursuitSoundTime(minPursuitSoundTime)
, mPursuitSoundWeight(pursuitSoundWeight)
, mPursuitSoundTimer(0.f)
, mSoundFallOff(soundFallOff)
, mMaxAudibleDistance(maxAudibleDistance)
, mMinVolume(minVolume)
, mMaxVolume(maxVolume)
, mLockOnIndex(-1)
, mLastOrbitPosition(CVector3f::Zero())
, mLockOnBlendStart(CVector3f::Zero())
, mLockOnBlend(0.f)
// 0x6454-0x64DC: second scan-model validity choice and temporary cleanup.
, mScanModelData(scanModel.GetId() == kInvalidAssetId ? CModelData(CModelData::None())
                                                      : CModelData(scanModel))
, mBlendingLockOn(false) // 0x64E0-0x64F0: clear 0x504 mask 0x80.
, mSpawnInstantly(spawnInstantly) // 0x64F4-0x650C: copy mask 0x40.
, x504_26_(unknownFlag) // 0x6508-0x6518: copy mask 0x20.
, mAnyPatrolling(false) // 0x651C-0x6524: clear mask 0x10.
, mAnyPursuing(false) { // 0x6528-0x6530: clear mask 0x08.
  // 0x64EC-0x6534: skip invalid death PART; 0x6538-0x6694: construct, emission=false.
  if (data.mDeathParticleEffect != kInvalidAssetId) {
    mDeathParticleDescription =
        rstl::optional_object< TLockedToken< CGenDescription > >(TLockedToken< CGenDescription >(
            gpSimplePool->GetObj(SObjectTag('PART', data.mDeathParticleEffect))));
    mDeathParticleGenerator = rs_new CElementGen(*mDeathParticleDescription);
    mDeathParticleGenerator->SetParticleEmission(false);
  }
  // 0x6698-0x66A0: skip invalid boid PART; 0x66A4-0x6800: construct, emission=false.
  if (particleEffect != kInvalidAssetId) {
    mBoidParticleDescription = rstl::optional_object< TLockedToken< CGenDescription > >(
        TLockedToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', particleEffect))));
    mBoidParticleGenerator = rs_new CElementGen(*mBoidParticleDescription);
    mBoidParticleGenerator->SetParticleEmission(false);
  }
}

// @ fn_6_5CD8
CBacteriaSwarm::~CBacteriaSwarm() {}

// @ fn_6_5A8C
void CBacteriaSwarm::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const EScriptObjectMessage message = msg.GetMessage();
  CActor::AcceptScriptMsg(mgr, msg);
  switch (message) {
  case kSM_Create:
    mBoids.reserve(mNumBoids);
    for (int i = 0; i < mBoids.capacity(); ++i) {
      mBoids.push_back_unsafe(CBoid(CTransform4f::Identity(), i));
    }
    AddDoorRepulsors(mgr);
    SetDrawShadow(false);
    AddMaterial(kMT_Scannable, mgr);
    break;
  case kSM_AreaLoaded:
    if (mSpawnInstantly) {
      uint count = mBoids.size();
      for (uint i = 0; i < count; ++i) {
        const CBoid& boid = mBoids[i];
        if (!boid.mActive) {
          CreateBoid(mgr, i, true);
          ++mCreatedBoids;
        }
      }
    }
    break;
  default:
    break;
  }
}

// @ fn_6_56DC
void CBacteriaSwarm::CreateBoid(CStateManager& mgr, int index, bool forceParticle) {
  // 0x5758-0x57C8: world bounds, center and extent; 0x57D8/0x5804/0x5830: strict > 0.5.
  const CAABox bounds = GetBoundingBox();
  const CVector3f center = bounds.GetCenterPoint();
  const CVector3f size = bounds.GetMaxPoint() - bounds.GetMinPoint();
  const float signX = mgr.Random()->Float() > 0.5f ? 1.f : -1.f;
  const float signY = mgr.Random()->Float() > 0.5f ? 1.f : -1.f;
  const float signZ = mgr.Random()->Float() > 0.5f ? 1.f : -1.f;
  // 0x5850-0x58B0: independent z,y,x draws, half-extent * sign * draw + center.
  const float z = 0.5f * size.GetZ() * signZ * mgr.Random()->Float() + center.GetZ();
  const float y = 0.5f * size.GetY() * signY * mgr.Random()->Float() + center.GetY();
  const float x = 0.5f * size.GetX() * signX * mgr.Random()->Float() + center.GetX();
  const CVector3f position(x, y, z);
  // 0x58B4-0x58E0: 360 degrees * draw, degrees-to-radians, RotateZ.
  const CRelAngle angle = CRelAngle::FromDegrees(360.f * mgr.Random()->Float());
  const CMatrix3f rotation = CMatrix3f::RotateZ(angle);
  const TUniqueId waypointId = GetWaypointForState(kSS_Patrol, mgr);
  // 0x58F0/0x5900: Patrol=0x5054524C; 0x5924-0x595C: non-null AND active (mask 0x80).
  const CScriptWaypoint* waypoint =
      TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(waypointId));
  const CTransform4f xf = (waypoint && waypoint->GetActive()) ? waypoint->GetTransform()
                                                              : CTransform4f(rotation, position);
  // 0x597C-0x59CC: whole transform, active byte 0x81 |= 0x20, zero velocity, patrol speed.
  mBoids[index].mTransform = xf;
  mBoids[index].mActive = true;
  mBoids[index].mVelocity = CVector3f::Zero();
  mBoids[index].mSpeed = mPatrolSpeed;
  // 0x59D4/0x59DC: generator then forceParticle; 0x5A14/0x5A20: translation, exactly one particle.
  if (mBoidParticleGenerator.get() && forceParticle) {
    mBoidParticleGenerator->SetTranslation(mBoids[index].GetTranslation());
    mBoidParticleGenerator->ForceParticleCreation(1);
  }
}

// @ fn_6_560C
void CBacteriaSwarm::PreRenderAllViewports(CStateManager& mgr) {
  const CAABox bounds = GetBoundingBox();
  SetOtherBounds(bounds);
  SetRenderBounds(bounds);
  UpdatePortalSystemState(mgr);
}

// @ fn_6_5574
CAABox CBacteriaSwarm::GetBoundingBox() const {
  const CVector3f extent(0.5f * mBoundingBoxExtent.GetX(), 0.5f * mBoundingBoxExtent.GetY(),
                         0.5f * mBoundingBoxExtent.GetZ());
  const CAABox bounds(-extent, extent);
  return bounds.GetTransformedAABox(GetTransform());
}

// @ fn_6_5538
rstl::optional_object< CAABox > CBacteriaSwarm::GetTouchBounds() const { return mAabox; }

// @ fn_6_54C0
CVector3f CBacteriaSwarm::ProjectPointToPlane(const CVector3f& point, const CVector3f& planePoint,
                                              const CVector3f& normal) {
  return point - CVector3f::Dot(point - planePoint, normal) * normal;
}

// @ fn_6_5474
CVector3f CBacteriaSwarm::ProjectVectorToPlane(const CVector3f& point, const CVector3f& normal) {
  return point - CVector3f::Dot(point, normal) * normal;
}

// @ fn_6_5290
bool CBacteriaSwarm::PointOnSurface(const CCollisionSurface& surface, const CVector3f& pos,
                                    const CPlane& plane) {
  const CVector3f projected = ProjectPointToPlane(pos, surface.GetVert(0), plane.GetNormal());
  for (int i = 0; i < 3; ++i) {
    const int next = i + 2;
    const int previous = next == 2 ? next : next - 3;
    const CVector3f edge2 = surface.GetVert(previous) - surface.GetVert(i);
    const CVector3f edge1 = projected - surface.GetVert(i);
    const CVector3f cross = CVector3f::Cross(edge1, edge2);
    if (CVector3f::Dot(plane.GetNormal(), cross) < 0.f) {
      return false;
    }
  }
  return true;
}

// @ fn_6_4F58
CCollisionSurface CBacteriaSwarm::FindBestCollisionInBox(CStateManager& mgr, const CVector3f& pos) {
  CCollisionSurface result(CVector3f(1.f, 0.f, 0.f), CVector3f(0.f, 1.f, 0.f),
                           CVector3f(0.f, 0.f, 1.f), ~0);
  const CAABox& bounds = GetBoundingBox();
  const CVector3f extent = 0.5f * (bounds.GetMaxPoint() - bounds.GetMinPoint());
  for (float scale = 0.1f; scale < 1.f; scale += 0.1f) {
    const CAABox searchBounds(pos - extent * scale, pos + extent * scale);
    CAreaCollisionCache cache(searchBounds);
    CGameCollision::BuildAreaCollisionCache(mgr, cache);
    if (FindBestSurface(cache, pos, 2.f * (extent * scale).Magnitude(), result)) {
      return result;
    }
  }
  return result;
}

// @ fn_6_4CB0
bool CBacteriaSwarm::FindBestSurface(const CAreaCollisionCache& cache, CVector3f pos, float radius,
                                     CCollisionSurface& out) {
  // 0x4CD0-0x4D0C: initial sphere/range, found=false; 0x4EB8-0x4ED0: cache/leaf loops.
  bool found = false;
  float minDistance = radius;
  CSphere sphere(pos, radius);
  for (int i = 0; i < int(cache.GetNumCaches()); ++i) {
    const CMetroidAreaCollider::COctreeLeafCache& leafCache = cache.GetOctreeLeafCache(i);
    for (int j = 0; j < leafCache.GetNumLeaves(); ++j) {
      const CAreaOctTree::Node& node = leafCache.GetLeaf(j);
      // 0x4D24-0x4D2C: only intersecting leaves; 0x4D30-0x4D60: triangle loop.
      if (CCollidableSphere::Sphere_AABox_Bool(sphere, node.GetBoundingBox())) {
        const CAreaOctTree::TriListReference triangles = node.GetTriangleArray();
        const CAreaOctTree& tree = node.GetOwner();
        const int triangleCount = triangles.GetSize();
        for (int k = 0; k < triangleCount; ++k) {
          const CCollisionSurface surface(tree.GetTriangle(triangles.GetAt(k)));
          // 0x4D48 lis r31,0x1000; 0x4D70-0x4D90: skip low-word bit 28 (0x10000000).
          if (!CMaterialList(surface.GetSurfaceFlags()).HasMaterial(kMT_AIPassthrough)) {
            const CPlane plane = surface.GetPlane();
            const float distance = CMath::AbsF(plane.GetHeight(pos));
            // 0x4DD4-0x4E04: absolute plane height < best, then PointOnSurface.
            if (distance < minDistance && PointOnSurface(surface, pos, plane)) {
              // 0x4E08-0x4E9C: shrink sphere, copy surface, set found and new best distance.
              sphere = CSphere(pos, distance);
              out = surface;
              found = true;
              minDistance = distance;
            }
          }
        }
      }
    }
  }
  return found;
}

// @ fn_6_4734
void CBacteriaSwarm::UpdateBoid(CAreaCollisionCache& cache, CStateManager& mgr, float dt,
                                CBoid& boid, int partitionIndex) {
  rstl::reserved_vector< CBoid*, 50 > nearList;
  BuildBoidNearList(boid, mSeparationRadius, nearList);
  CCollisionSurface surface(CVector3f(1.f, 0.f, 0.f), CVector3f(0.f, 1.f, 0.f),
                            CVector3f(0.f, 0.f, 1.f), ~0);
  bool found = false;
  const float radius = 2.f * mBoidRadius;
  const float boidRadius = mBoidRadius;
  const float speed = boid.mVelocity.Magnitude();
  float distance = speed * dt;
  CVector3f pos = boid.GetTranslation();
  const CVector3f step = boidRadius * ((1.f / speed) * -boid.mVelocity);
  while (distance >= 0.f && !found) {
    const CVector3f predicted = pos + mSurfaceProbeScale * (dt * boid.mVelocity);
    if (FindBestSurface(cache, predicted, radius, surface)) {
      boid.mTransform = ShortestRotationArcWrapped(boid.GetTransform().GetUp(), surface.GetNormal(),
                                                   CRelAngle::FromRadians(M_PIF))
                            .MultiplyIgnoreTranslation(boid.GetTransform());
      found = true;
    }
    distance -= boidRadius;
    pos += step;
  }
  CVector3f ahead = boid.GetTransform().GetForward();
  if (found) {
    ahead += mSurfaceStickPriority * surface.GetPlane().GetNormal();
  }
  ApplySteeringBehaviors(mgr, boid, ahead, nearList);
  float newSpeed = boid.mSpeed;
  if (boid.mInSafeZone) {
    newSpeed = rstl::min_val(mSafeZoneEscapeSpeed, newSpeed + mAcceleration);
  } else if (boid.mPursuingPlayer) {
    newSpeed = rstl::min_val(mPlayerPursuitSpeed, newSpeed + mAcceleration);
  } else {
    newSpeed = rstl::max_val(mPatrolSpeed, newSpeed - mDeceleration);
  }
  boid.mSpeed = newSpeed;
  float turnSpeed = mPatrolTurnSpeed;
  if (boid.mInSafeZone || boid.mTouchingSafeZone) {
    turnSpeed = mAvoidSafeZoneTurnSpeed;
  }
  if (boid.mPursuingPlayer) {
    boid.mColorBlend = rstl::min_val(mColorChangeTime, boid.mColorBlend + dt);
    mAnyPursuing = true;
  } else {
    boid.mColorBlend = rstl::max_val(0.f, boid.mColorBlend - dt);
    mAnyPatrolling = true;
  }
  boid.mTransform =
      ShortestRotationArcWrapped(boid.GetTransform().GetForward(), ahead.AsNormalized(),
                                 CRelAngle::FromRadians(turnSpeed * dt))
          .MultiplyIgnoreTranslation(boid.GetTransform());
}

// @ fn_6_4500
void CBacteriaSwarm::ApplySteeringBehaviors(CStateManager& mgr, CBoid& boid, CVector3f& ahead,
                                            const rstl::reserved_vector< CBoid*, 50 >& nearList) {
  boid.mInSafeZone = false;
  boid.mTouchingSafeZone = false;
  boid.mPursuingPlayer = false;
  for (int i = 0; i < 7; ++i) {
    if (boid.mInSafeZone || boid.mTouchingSafeZone) {
      break;
    }
    switch (i) {
    case 0:
      for (rstl::vector< CRepulsor >::iterator it = mDoorRepulsors.begin();
           it != mDoorRepulsors.end(); ++it) {
        if ((it->mCenter - boid.GetTranslation()).MagSquared() < it->mMagnitude * it->mMagnitude) {
          ApplySeparation(boid, it->mCenter, it->mMagnitude, 4.5f, ahead);
        }
      }
      break;
    case 1:
      ApplySafeZoneAvoidance(mgr, boid, nearList, ahead);
      break;
    case 2:
      ApplyPlayerAttraction(boid, mgr.GetPlayer(0)->GetAimPosition(mgr, 0.f), *mgr.GetPlayer(0),
                            ahead, mAttractionRadius, mAttractionMagnitude);
      break;
    case 3:
      ApplyBoundsAvoidance(boid, nearList, ahead);
      break;
    case 5:
      ApplySeparation(boid, nearList, ahead);
      break;
    case 6:
      ApplyCohesion(boid, nearList, ahead);
      break;
    default:
      break;
    }
    if (ahead.MagSquared() >= 9.f) {
      break;
    }
  }
}

// @ fn_6_42E8
void CBacteriaSwarm::AddDoorRepulsors(CStateManager& mgr) {
  CObjectList& objects = mgr.ObjectListById(kOL_PhysicsActor);
  int count = 0;
  for (int i = objects.GetFirstObjectIndex(); i != -1; i = objects.GetNextObjectIndex(i)) {
    if (CScriptDoor* door = TCastToPtr< CScriptDoor >(objects[i])) {
      if (door->GetCurrentAreaId() == GetCurrentAreaId()) {
        ++count;
      }
    }
  }
  mDoorRepulsors.reserve(count);
  for (int i = objects.GetFirstObjectIndex(); i != -1; i = objects.GetNextObjectIndex(i)) {
    if (CScriptDoor* door = TCastToPtr< CScriptDoor >(objects[i])) {
      if (door->GetCurrentAreaId() == GetCurrentAreaId()) {
        rstl::optional_object< CAABox > bounds = door->GetTouchBounds();
        if (bounds.valid()) {
          float diagonal = (bounds->GetMinPoint() - bounds->GetMaxPoint()).Magnitude();
          mDoorRepulsors.push_back_unsafe(CRepulsor(bounds->GetCenterPoint(), 0.75f * diagonal));
        }
      }
    }
  }
}

// @ fn_6_3570
void CBacteriaSwarm::Think(float dt, CStateManager& mgr) {
  // 0x35B4/0x35DC: actor active mask 0x80; 0x35BC-0x35D4: clear patrol/pursuit 0x10/0x08.
  if (!GetActive()) {
    return;
  }
  mAnyPatrolling = false;
  mAnyPursuing = false;
  if (!GetActive()) {
    return;
  }
  SetTransformDirty();
  // 0x35E8-0x3608: subtract dt from both cooldowns and increment think counter.
  mBoidGenCooldownTimer -= dt;
  mDamageCooldownTimer -= dt;
  ++mThinkCounter;
  const CGameArea& area = mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId());
  // 0x3630-0x364C: phase Loaded=16 then Visible=1; non-loaded means Occluded.
  if (area.GetOcclusionState() != CGameArea::kOS_Visible) {
    if (mOccludedTimer > 0.f) {
      // 0x365C-0x3668: strict > zero before subtract; 0x3678-0x3680: <= zero exits.
      mOccludedTimer -= dt;
    }
    if (mOccludedTimer <= 0.f) {
      return;
    }
    if (mThinkCounter & 2) {
      // 0x3688/0x368C: PPC bit 30 = value mask 0x02, return when set.
      return;
    }
  } else {
    mOccludedTimer = 7.f; // 0x3694-0x369C, rodata 0x48.
  }
  UpdateParticles(dt);
  int prevLockOnIndex = mLockOnIndex;
  mLockOnIndex = GetLockOnIndex(mgr);
  UpdateLockOnBlend(prevLockOnIndex, mLockOnIndex, dt);
  const CPlayerState* playerState = mgr.GetPlayer(0)->GetPlayerState();
  // 0x36E0/0x36EC/0x36F8: both visor enums 2 and index != -1; material ids 40,41.
  if (playerState->GetCurrentVisor() == CPlayerState::kPV_Scan &&
      playerState->GetTransitioningVisor() == CPlayerState::kPV_Scan && mLockOnIndex != -1) {
    AddMaterial(kMT_Target, kMT_Orbit, mgr);
    // 0x3714-0x3738: last orbit position copied from the selected boid.
    mLastOrbitPosition = mBoids[mLockOnIndex].GetTranslation();
  } else {
    RemoveMaterial(kMT_Target, kMT_Orbit, mgr);
  }
  // 0x3814-0x3834: unlimited==0 OR created<max, then cooldown<=0 (cror eq,lt,eq).
  while ((mMaxCreatedBoids == 0 || mCreatedBoids < mMaxCreatedBoids) &&
         mBoidGenCooldownTimer <= 0.f) {
    bool created = false;
    for (int i = 0; i < mBoids.size(); ++i) {
      // 0x3770-0x37DC: first inactive slot; CreateBoid(...,true), ++created, cooldown+=1/rate.
      if (!mBoids[i].mActive) {
        CreateBoid(mgr, i, true);
        ++mCreatedBoids;
        mBoidGenCooldownTimer += 1.f / mBoidGenRate;
        created = true;
        break;
      }
    }
    if (!created) {
      // 0x37E0-0x3804: failed search still adds 1/rate and leaves creation loop only.
      mBoidGenCooldownTimer += 1.f / mBoidGenRate;
      break;
    }
  }
  UpdatePartition();
  // 0x383C-0x3894: partition rebuilt, count=0, bounds copied; 0x39B4/0x39C8/0x39D4: 5^3.
  const CAABox bounds = GetBoundingBox();
  int count = 0;
  mAabox = GetBoundingBox();
  for (int x = 0; x < 5; ++x) {
    int rowIndex = x;
    for (int y = 0; y < 5; ++y, rowIndex += 5) {
      for (int z = 0; z < 5; ++z) {
        CBoid* boid = mPartitionedBoidLists[rowIndex + z * 25];
        if (boid != nullptr) {
          CAreaCollisionCache cache = GetAreaCollisionCacheForPartition(x, y, z);
          if (ShouldBuildAreaCollisionCacheForPartition(rowIndex + z * 25)) {
            // 0x38F8-0x390C: vtable slot 0x94, build iff true (retail helper always returns 1).
            CGameCollision::BuildAreaCollisionCache(mgr, cache);
          }
          for (; boid != nullptr; boid = boid->mNext) {
            // 0x3918-0x3948: ++count, active mask 0x20, accumulate translation.
            ++count;
            if (boid->mActive) {
              mAabox.AccumulateBounds(boid->GetTranslation());
            }
            if ((mThinkCounter & 1) == (count & 1) && boid->mActive) {
              // 0x394C-0x3990: matching low bits AND active; UpdateBoid slot 0x80.
              UpdateBoid(cache, mgr, dt, *boid, rowIndex + z * 25);
            }
          }
        }
      }
    }
  }
  for (CBoid* boid = mOutlierBoidList; boid != nullptr; boid = boid->mNext) {
    // 0x39E4-0x3A34: same count/active/parity checks for outliers.
    ++count;
    if (boid->mActive) {
      mAabox.AccumulateBounds(boid->GetTranslation());
    }
    if ((mThinkCounter & 1) == (count & 1) && boid->mActive) {
      // 0x3A38-0x3ADC: margin 1.5+(0.5+radius); bounds/cache; UpdateBoid partition=-1.
      const float margin = 1.5f + (0.5f + mBoidRadius);
      const CVector3f extent(margin, margin, margin);
      const CAABox boidBounds(boid->GetTranslation() - extent, boid->GetTranslation() + extent);
      CAreaCollisionCache cache(boidBounds);
      CGameCollision::BuildAreaCollisionCache(mgr, cache);
      UpdateBoid(cache, mgr, dt, *boid, -1);
    }
  }
  UpdateSwarmAnimations(mgr, dt);
  UpdateAllBoidMovement(dt);
  // 0x3B2C-0x3B7C: patrol 0x10; timer+=dt, >= threshold resets 0 and draws min+(max-min)*R.
  if (mAnyPatrolling) {
    mPatrolSoundTimer += dt;
    if (mPatrolSoundTimer >= mNextPatrolSoundTime) {
      mPatrolSoundTimer = 0.f;
      mNextPatrolSoundTime =
          mMinPatrolSoundTime + (mMaxPatrolSoundTime - mMinPatrolSoundTime) * mgr.Random()->Float();
    }
  }
  if (mAnyPursuing) {
    // 0x3B80-0x3BD0: pursuit 0x08; identical >= threshold timer/random update.
    mPursuitSoundTimer += dt;
    if (mPursuitSoundTimer >= mNextPursuitSoundTime) {
      mPursuitSoundTimer = 0.f;
      mNextPursuitSoundTime = mMinPursuitSoundTime +
                              (mMaxPursuitSoundTime - mMinPursuitSoundTime) * mgr.Random()->Float();
    }
  }
  if (mBoidParticleGenerator.get()) {
    // 0x3BD4-0x3C48: generator guard, aim position, min(root active count, boid count), i=0.
    const CVector3f playerPos = mgr.GetPlayer(0)->GetAimPosition(mgr, 0.f);
    const int particleCount =
        rstl::min_val(mBoidParticleGenerator->GetParticleCount(), int(mBoids.size()));
    for (int i = 0; i < particleCount; ++i) {
      CBoid& boid = mBoids[i];
      // 0x3C54-0x3CC4: active 0x20; pos/prev=boid, end=0, patrol/pursuit color blend.
      if (boid.mActive) {
        CElementGen::CParticle& particle = mBoidParticleGenerator->mParticles[i];
        particle.mPos = boid.GetTranslation();
        particle.mPrevPos = boid.GetTranslation();
        particle.mEndFrame = 0;
        particle.mColor =
            CColor::Lerp(mPatrolColor, mPursuitColor, boid.mColorBlend / mColorChangeTime);
        const float distanceSquared = (playerPos - boid.GetTranslation()).MagSquared();
        // 0x3CC8-0x3D48: patrol 0x10, timer==0, !pursuing 0x02, random<=weight, sound.
        if (mAnyPatrolling && mPatrolSoundTimer == 0.f && !boid.mPursuingPlayer &&
            mgr.Random()->Float() <= mPatrolSoundWeight) {
          PlayBoidSound(mPatrolSound, boid.GetTranslation(), distanceSquared);
        }
        // 0x3D4C-0x3DCC: pursuit 0x08, timer==0, pursuing 0x02, random<=weight, sound.
        if (mAnyPursuing && mPursuitSoundTimer == 0.f && boid.mPursuingPlayer &&
            mgr.Random()->Float() <= mPursuitSoundWeight) {
          PlayBoidSound(mPursuitSound, boid.GetTranslation(), distanceSquared);
        }
      }
    }
  }
}

// @ fn_6_34CC
void CBacteriaSwarm::UpdateAllBoidMovement(float dt) {
  uint count = mBoids.size();
  for (uint i = 0; i < count; ++i) {
    MoveBoid(mBoids[i], mBoids[i].mSpeed * CVector3f::Forward(), dt);
  }
}

// @ fn_6_34C8
void CBacteriaSwarm::UpdateSwarmAnimations(CStateManager& mgr, float dt) {}

// @ fn_6_340C
void CBacteriaSwarm::MoveBoid(CBoid& boid, const CVector3f& offsetDelta, float dt) {
  if (boid.mActive) {
    boid.mVelocity = (1.f / dt) * boid.GetTransform().Rotate(offsetDelta);
    boid.mTransform.AddTranslation(dt * boid.mVelocity);
  }
}

// @ fn_6_316C
void CBacteriaSwarm::UpdatePartition() {
  mPartitionedBoidLists.clear();
  for (int i = 0; i < 125; ++i) {
    mPartitionedBoidLists.push_back(nullptr);
  }
  mOutlierBoidList = nullptr;
  const CAABox bounds = GetBoundingBox();
  const CVector3f extent = bounds.GetMaxPoint() - bounds.GetMinPoint();
  const CVector3f size = extent / 5.f;
  for (rstl::vector< CBoid >::iterator it = mBoids.begin(); it != mBoids.end(); ++it) {
    if (it->mActive) {
      const CVector3f& pos = it->GetTranslation();
      const CVector3f delta = pos - bounds.GetMinPoint();
      const int x = CCast::ToInt32(delta.GetX() / size.GetX());
      const int y = CCast::ToInt32(delta.GetY() / size.GetY());
      const int z = CCast::ToInt32(delta.GetZ() / size.GetZ());
      const int index = x + 5 * y + 25 * z;
      if (index < 0 || index >= 125 || x < 0 || x >= 5 || y < 0 || y >= 5 || z < 0 || z >= 5) {
        it->mNext = mOutlierBoidList;
        mOutlierBoidList = it.get_pointer();
      } else {
        it->mNext = mPartitionedBoidLists[index];
        mPartitionedBoidLists[index] = it.get_pointer();
      }
    }
  }
}

// @ fn_6_3064
CBacteriaSwarm::CBoid* CBacteriaSwarm::GetListAt(const CVector3f& pos) {
  const CAABox bounds = GetBoundingBox();
  const CVector3f delta = pos - bounds.GetMinPoint();
  const int index = CCast::ToInt32(delta.GetX() / (bounds.GetWidth() / 5.f)) +
                    CCast::ToInt32(delta.GetY() / (bounds.GetHeight() / 5.f)) * 5 +
                    CCast::ToInt32(delta.GetZ() / (bounds.GetDepth() / 5.f)) * 25;
  if (index < 0 || index >= 125) {
    return mOutlierBoidList;
  }
  return mPartitionedBoidLists[index];
}

// @ fn_6_2EA4
CAABox CBacteriaSwarm::BoxForPosition(int x, int y, int z, float margin) const {
  const CAABox bounds = GetBoundingBox();
  const CVector3f extent = bounds.GetMaxPoint() - bounds.GetMinPoint();
  const CVector3f size(extent.GetX() / 5.f, extent.GetY() / 5.f, extent.GetZ() / 5.f);
  return CAABox(CVector3f(x * size.GetX() + bounds.GetMinPoint().GetX() - margin,
                          y * size.GetY() + bounds.GetMinPoint().GetY() - margin,
                          z * size.GetZ() + bounds.GetMinPoint().GetZ() - margin),
                CVector3f((x + 1) * size.GetX() + bounds.GetMinPoint().GetX() + margin,
                          (y + 1) * size.GetY() + bounds.GetMinPoint().GetY() + margin,
                          (z + 1) * size.GetZ() + bounds.GetMinPoint().GetZ() + margin));
}

// @ fn_6_2DBC
void CBacteriaSwarm::PreRender(CStateManager& mgr) {
  bool active = false;
  for (rstl::vector< CBoid >::iterator it = mBoids.begin(); it != mBoids.end(); ++it) {
    if (it->mActive) {
      it->mInFrustum = mgr.GetFrustumPlanes().SphereInFrustumPlanes(
          CSphere(it->GetTranslation(), 2.f * mBoidRadius));
      active = true;
    } else {
      it->mInFrustum = false;
    }
  }
  SetPreRenderClipped(!active);
}

// @ fn_6_2CB4
void CBacteriaSwarm::HardwareLight(const CStateManager& mgr, const CAABox& bounds) const {
  // 0x2CC8-0x2D28: period 8, zero offset, max dynamic/area 4/4, remaining flags false.
  CActorLights lights(8, CVector3f::Zero(), 4, 4);
  // 0x2D2C-0x2D4C: needs-relight mask 0x80 set, cast-shadows mask 0x40 cleared.
  lights.SetNeedsRelight(true);
  lights.SetCastShadows(false);
  // 0x2D58-0x2D78: this actor's actual area; both builds use the caller's same bounds.
  lights.BuildAreaLightList(mgr, mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()), bounds);
  lights.BuildDynamicLightList(mgr, bounds);
  // 0x2D88/0x2D90/0x2D9C: dynamic build, ActivateLights, then local destructor.
  lights.ActivateLights();
}

// @ fn_6_2A40
CColor CBacteriaSwarm::SoftwareLight(const CStateManager& mgr, const CAABox& bounds) const {
  CActorLights lights(8, CVector3f::Zero(), 4, 4);
  lights.SetNeedsRelight(true);
  lights.SetCastShadows(false);
  lights.BuildAreaLightList(mgr, mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId()), bounds);
  lights.BuildDynamicLightList(mgr, bounds);
  CColor result = lights.GetAmbientColor();
  const CVector3f center = bounds.GetCenterPoint();
  for (uint i = 0; i < lights.GetActiveLightCount(); ++i) {
    const CLight& light = lights.GetLight(i);
    const float distance = (light.GetPosition() - center).Magnitude();
    const float attenuation = rstl::min_val(
        1.f, 1.f / (distance * (distance * light.GetAttenuationQuadratic()) +
                    (distance * light.GetAttenuationLinear() + light.GetAttenuationConstant())));
    result =
        CColor::Add(result, CColor::Lerp(CColor::Black(), light.GetColor(), 0.8f * attenuation));
  }
  return result;
}

// @ fn_6_2A38
bool CBacteriaSwarm::CanRenderUnsorted(const CStateManager& mgr) const { return true; }

// @ fn_6_2A34
void CBacteriaSwarm::RenderBoid(const CBoid* boid, uint& drawMask, const CModelFlags& flags) const {
}

// @ fn_6_29D8
void CBacteriaSwarm::AddToRenderer(const CStateManager& mgr) const {
  if (GetActive()) {
    RenderParticles();
    if (!GetPreRenderClipped()) {
      EnsureRendered(mgr);
    }
  }
}

// @ fn_6_262C
void CBacteriaSwarm::Render(const CStateManager& mgr) const {
  // 0x2644-0x2670: destination alpha enabled only for alpha != -1.
  const int alpha = GetRenderAlphaBufferAlpha(mgr);
  if (alpha != -1) {
    gpRender->SetDestinationAlpha(alpha);
  }
  uint drawMask = -1; // 0x2680/0x268C; 0x2690-0x26AC: opaque flags, alpha=1.
  const CModelFlags flags(CModelFlags::kT_Opaque, 1.f);
  const uint lights = CGraphics::GetLightMask();
  // 0x26C0-0x26EC: mask zero selects NONE/NONE, otherwise CLAMP/SPOT.
  CGX::SetChanCtrl(CGX::Channel0, GX_FALSE, GX_SRC_REG, GX_SRC_REG,
                   static_cast< GXLightID >(lights), lights ? GX_DF_CLAMP : GX_DF_NONE,
                   lights ? GX_AF_SPOT : GX_AF_NONE);
  for (int x = 0; x < 5; ++x) {
    // 0x2700-0x284C: 5^3 partition loops, stride y=5,z=25; null heads skipped.
    int rowIndex = x;
    for (int y = 0; y < 5; ++y, rowIndex += 5) {
      for (int z = 0; z < 5; ++z) {
        const int index = rowIndex + z * 25;
        CBoid* boid = mPartitionedBoidLists[index];
        if (boid != nullptr) {
          const CAABox bounds = BoxForPosition(x, y, z, 0.f);
          if ((index & 3) == (mThinkCounter & 3)) {
            // 0x273C/0x2770-0x2778: compare low 2 bits for software-light refresh.
            const CColor color = SoftwareLight(mgr, bounds);
            for (CBoid* it = boid; it != nullptr; it = it->mNext) {
              if (it->mActive) {
                // 0x27AC/0x27B0: active mask 0x20; 0x27BC-0x27CC: Lerp by rodata 0.3.
                it->mAmbientLighting = CColor::Lerp(it->mAmbientLighting, color, 0.3f);
              }
            }
          }
          for (; boid != nullptr; boid = boid->mNext) {
            if (boid->mInFrustum && boid->mActive) {
              // 0x27E4-0x27F0: frustum mask 0x10 then active 0x20; RenderBoid slot 0x88.
              RenderBoid(boid, drawMask, flags);
            }
          }
        }
      }
    }
  }
  CBoid* boid = mOutlierBoidList;
  int index = 0;
  for (; boid != nullptr; boid = boid->mNext) {
    // 0x2858-0x2870: ++index, frustum 0x10 then active 0x20.
    ++index;
    if (boid->mInFrustum && boid->mActive) {
      const CVector3f pos = boid->GetTranslation();
      const CVector3f extent(mBoidRadius, mBoidRadius, mBoidRadius);
      const CAABox bounds = CAABox(pos - extent, pos + extent);
      // 0x2874-0x28C0: boid-centered radius bounds; 0x28C8-0x2904: low-2-bit refresh.
      if ((index & 3) == (mThinkCounter & 3)) {
        const CColor color = SoftwareLight(mgr, bounds);
        if (boid->mActive) {
          // 0x2928-0x2950: recheck active 0x20, same 0.3 Lerp.
          boid->mAmbientLighting = CColor::Lerp(boid->mAmbientLighting, color, 0.3f);
        }
      }
      RenderBoid(boid, drawMask, flags);
    }
  }
#ifdef TARGET_PC
  // 0x2980 bl DisableAllLights__9CGraphicsFv: retail turns the lights off after the outlier
  // loop, before the particle generator renders (0x2984 lwz r3,0x458(r30)).
  CGraphics::DisableAllLights();
#endif
  if (mBoidParticleGenerator.get()) {
    // 0x2984-0x299C: non-null generator Render, after DisableAllLights.
    mBoidParticleGenerator->Render();
  }
  if (alpha != -1) {
    // 0x29A0-0x29C0: disable destination alpha under the same != -1 guard.
    gpRender->DisableDestinationAlpha();
  }
}

// @ fn_6_2524
void CBacteriaSwarm::BuildBoidNearList(const CBoid& boid, float radius,
                                       rstl::reserved_vector< CBoid*, 50 >& nearList) {
  CBoid* other = GetListAt(boid.GetTranslation());
  const CVector3f pos = boid.GetTranslation();
  while (other != nullptr && nearList.size() < 50) {
    const float distance = (other->GetTranslation() - pos).MagSquared();
    if (distance != 0.f && distance < radius) {
      nearList.push_back(other);
    }
    other = other->mNext;
  }
}

// @ fn_6_2458
void CBacteriaSwarm::ApplySeparation(CBoid& boid,
                                     const rstl::reserved_vector< CBoid*, 50 >& nearList,
                                     CVector3f& ahead) {
  if (nearList.size() > 0) {
    CVector3f closest(0.f, 0.f, 0.f);
    float minDistance = FLT_MAX;
    for (rstl::reserved_vector< CBoid*, 50 >::const_iterator it = nearList.begin();
         it != nearList.end(); ++it) {
      const CVector3f delta = boid.GetTranslation() - (*it)->GetTranslation();
      const float distance = delta.MagSquared();
      if (distance != 0.f && distance < minDistance) {
        minDistance = distance;
        closest = (*it)->GetTranslation();
      }
    }
    ApplySeparation(boid, closest, mSeparationRadius, mSeparationMagnitude, ahead);
  }
}

// @ fn_6_2334
void CBacteriaSwarm::ApplySeparation(CBoid& boid, const CVector3f& pos, float radius,
                                     float magnitude, CVector3f& ahead) {
  const CVector3f delta = boid.GetTranslation() - pos;
  if (delta.CanBeNormalized()) {
    const float distance = delta.MagSquared();
    const float radiusSquared = radius * radius;
    if (distance < radiusSquared) {
      const float factor = 1.f - distance / radiusSquared;
      ahead += factor * delta.AsNormalized() * magnitude;
    }
  }
}

// @ fn_6_2240
void CBacteriaSwarm::ApplyCohesion(CBoid& boid, const rstl::reserved_vector< CBoid*, 50 >& nearList,
                                   CVector3f& ahead) {
  if (nearList.size() > 0) {
    CVector3f center(0.f, 0.f, 0.f);
    for (rstl::reserved_vector< CBoid*, 50 >::const_iterator it = nearList.begin();
         it != nearList.end(); ++it) {
      center += (*it)->GetTranslation();
    }
    center = (1.f / nearList.size()) * center;
    ApplyCohesion(boid, center, mSeparationRadius, mCohesionMagnitude, ahead);
  }
}

// @ fn_6_2120
void CBacteriaSwarm::ApplyCohesion(CBoid& boid, const CVector3f& pos, float radius, float magnitude,
                                   CVector3f& ahead) {
  const CVector3f delta = pos - boid.GetTranslation();
  if (delta.CanBeNormalized()) {
    const float distance = delta.MagSquared();
    const float radiusSquared = radius * radius;
    const float factor = distance > radiusSquared ? 1.f : distance / radiusSquared;
    ahead += factor * delta.AsNormalized() * magnitude;
  }
}

// @ fn_6_1F78
void CBacteriaSwarm::ApplyPlayerAttraction(CBoid& boid, const CVector3f& pos, const CPlayer& player,
                                           CVector3f& ahead, float radius, float magnitude) {
  CVector3f delta = pos - boid.GetTranslation();
  if (player.GetPlayerState()->HasPowerUp(CPlayerState::kIT_LightSuit)) {
    delta = boid.GetTranslation() - pos;
  }
  if (delta.CanBeNormalized()) {
    const float distance = delta.MagSquared();
    const float radiusSquared = radius * radius;
    const float factor = distance > radiusSquared ? 0.f : 1.f - distance / radiusSquared;
    ahead += magnitude * (factor * delta.AsNormalized());
    if (factor > 0.f) {
      boid.mPursuingPlayer = true;
    }
  }
}

// @ fn_6_1E08
void CBacteriaSwarm::ApplyBoundsAvoidance(CBoid& boid,
                                          const rstl::reserved_vector< CBoid*, 50 >& nearList,
                                          CVector3f& ahead) {
  const CAABox bounds = GetBoundingBox();
  const CVector3f future = boid.GetTranslation() + 1.f * boid.mVelocity;
  if (!bounds.PointInside(future)) {
    const float priority = mContainmentPriority;
    ahead += priority * (bounds.GetCenterPoint() - future).AsNormalized();
  }
}

// @ fn_6_1C78
void CBacteriaSwarm::ApplySafeZoneAvoidance(CStateManager& mgr, CBoid& boid,
                                            const rstl::reserved_vector< CBoid*, 50 >& nearList,
                                            CVector3f& ahead) {
  // 0x1CB4-0x1D04: boid position + touch radius query; skip invalid UID.
  const CVector3f pos = boid.GetTranslation();
  const TUniqueId zoneId =
      mgr.GetSafeZoneManager()->SphereTouchingWhichSafeZone(mgr, CSphere(pos, mTouchRadius));
  if (zoneId != kInvalidUniqueId) {
    // 0x1D14-0x1D1C: skip a missing object (retail uses a static cast).
    if (const CScriptSafeZone* zone =
            static_cast< const CScriptSafeZone* >(mgr.GetObjectById(zoneId))) {
      boid.mTouchingSafeZone = true; // 0x1D20-0x1D2C: boid byte 0x81 mask 0x04.
      const float radius = zone->GetScale().GetX();
      const CVector3f delta = pos - zone->GetTranslation();
      if (delta.MagSquared() < radius * radius) { // 0x1D30-0x1D74: strict radius-X test.
        boid.mInSafeZone = true; // 0x1D78-0x1D80: byte 0x81 mask 0x08, no else clear.
      }
      ahead += mSafeZoneAvoidance * delta.AsNormalized(); // 0x1D84-0x1DCC: additive force.
    }
  }
}

// @ fn_6_1AB0
TUniqueId CBacteriaSwarm::GetWaypointForState(EScriptObjectState state, CStateManager& mgr) {
  rstl::vector< TUniqueId > waypoints;
  waypoints.reserve(8);
  for (rstl::vector< SConnection >::const_iterator it = GetConnectionList().begin();
       it != GetConnectionList().end(); ++it) {
    if (it->state == state && it->msg == kSM_Follow) {
      TUniqueId uid = mgr.GetIdForScript(it->objId);
      const CScriptWaypoint* waypoint = TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(uid));
      if (waypoint && waypoint->GetActive()) {
        waypoints.push_back_unsafe(uid);
      }
    }
  }
  int count = waypoints.size();
  if (count > 1) {
    return waypoints[mgr.Random()->Next() % count];
  }
  if (count > 0) {
    return waypoints[0];
  }
  return kInvalidUniqueId;
}

// @ fn_6_17FC
void CBacteriaSwarm::Touch(CActor& actor, CStateManager& mgr) {
  CActor::Touch(actor, mgr);
  if (CPlayer* player = TCastToPtr< CPlayer >(actor)) {
    float radius = mPlayerTouchRadius;
    if (close_enough(radius, 0.f)) {
      radius = mTouchRadius;
    }
    const CAABox playerBounds = *player->GetTouchBounds();
    for (rstl::vector< CBoid >::iterator it = mBoids.begin(); it != mBoids.end(); ++it) {
      if (it->mActive) {
        const CVector3f extent(radius, radius, radius);
        const CAABox bounds = CAABox(it->GetTranslation() - extent, it->GetTranslation() + extent);
        if (playerBounds.DoBoundsOverlap(bounds) && mDamageCooldownTimer <= 0.f) {
          mgr.ApplyDamage(
              GetUniqueId(), player->GetUniqueId(), GetUniqueId(), mDamage,
              CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList()),
              CVector3f::Zero());
          mDamageCooldownTimer = mDamageCooldown;
          KillBoid(*it, mgr, 1.f, 1.f);
          break;
        }
      }
    }
  }
}

// @ fn_6_163C
CVector3f CBacteriaSwarm::FindClosestCell(const CVector3f& pos) const {
  // 0x1674-0x1694: result=zero, best=__float_max; 0x16A0-0x16B0: x/y/z start at zero.
  float minDistance = FLT_MAX;
  CVector3f result = CVector3f::Zero();
  for (int x = 0; x < 5; ++x) {
    int rowIndex = x;
    for (int y = 0; y < 5; ++y, rowIndex += 5) {
      for (int z = 0; z < 5; ++z) {
        // 0x16B4-0x16DC: non-null head at x+5*y+25*z; BoxForPosition margin=rodata 0.1.
        if (mPartitionedBoidLists[rowIndex + z * 25] != nullptr) {
          const CAABox bounds = BoxForPosition(x, y, z, 0.1f);
          const float distance = (bounds.GetCenterPoint() - pos).MagSquared();
          // 0x171C-0x1764: squared distance, fcmpo/bge skip => strict < (unordered skips).
          if (distance < minDistance) {
            // 0x1768-0x1780: update center and best together; equal candidates keep first.
            result = bounds.GetCenterPoint();
            minDistance = distance;
          }
        }
      }
    }
  }
  // 0x1784-0x17B0: all three loop limits 5, strides 25/5; 0x17B4-0x17BC: return xyz.
  return result;
}

// @ fn_6_1448
void CBacteriaSwarm::UpdateEffects(CStateManager& mgr, CAnimData& animData, int volume) {
  // 0x1478-0x149C: passed count > 0 and static POI data pointer non-null.
  int count;
  const CSoundPOINode* nodes = animData.GetSoundPOIList(count);
  if (count > 0 && nodes != nullptr) {
    for (int i = 0; i < count; ++i) {
      const CSoundPOINode& node = nodes[i];
      const int character = node.GetCharacterIndex();
      // 0x14B0-0x14D0: type 8 AND (character -1 OR anim character match).
      if (node.GetPoiType() == kPT_Sound &&
          (character == -1 || character == animData.GetCharacterIndex())) {
        const uint soundId = node.GetSoundId();
        const ushort sfx = soundId; // 0x14E0: clrlwi r31,r3,16, keep low 16 bits.
        const int area = GetCurrentAreaId().Value();
        // 0x14DC/0x14E4: bit 31 (0x80000000) clear; set means skip this node only.
        if ((soundId & 0x80000000) == 0) {
          const CVector3f playerPos = mgr.GetPlayer(0)->GetTranslation();
          const CVector3f pos = FindClosestCell(playerPos);
          // 0x1524-0x1544/0x1550-0x156C: initialize each static once, retain first values.
          static float maxDistance = node.GetMaxDistance();
          static float falloff = node.GetFallOff();
          // 0x157C-0x15B0: clamp volume to [0,127], flags=1, min volume=20.
          CAudioSys::C3DEmitterParmData params(maxDistance, falloff, 1,
                                               volume < 0 ? 0 : (volume > 127 ? 127 : volume), 20);
          params.mPos = pos;
          params.mDir = CVector3f::Zero();
          params.mSfxId = sfx;
          // 0x15D4-0x15FC: nearest cell, zero direction, sfx; area, acoustics=true, loop=false.
          CSfxManager::AddEmitter(params, area, true, false);
        }
      }
    }
  }
  // 0x1600-0x160C: advance one complete sound node, signed i<count loop.
}

// @ fn_6_13FC
CAreaCollisionCache CBacteriaSwarm::GetAreaCollisionCacheForPartition(int x, int y, int z) const {
  return CAreaCollisionCache(BoxForPosition(x, y, z, mBoidRadius + 0.5f));
}

// @ fn_6_13F4
bool CBacteriaSwarm::ShouldBuildAreaCollisionCacheForPartition(int partitionIndex) const {
  return true;
}

// @ fn_6_12E0
void CBacteriaSwarm::KillBoid(CBoid& boid, CStateManager& mgr, float deathRattleChance,
                              float deadChance) {
  AddParticle(boid.GetTransform());
  boid.mActive = false;
  const float deadRoll = mgr.Random()->Float();
  const float deathRattleRoll = mgr.Random()->Float();
  if (deathRattleRoll < deathRattleChance) {
    SendScriptMsgs(kSS_DeathRattle, mgr, kInvalidUniqueId, kSM_None);
  }
  if (deadRoll < deadChance) {
    SendScriptMsgs(kSS_Dead, mgr, kInvalidUniqueId, kSM_None);
  }
  CreateBoid(mgr, boid.mIndex, false);
}

// @ fn_6_1238
void CBacteriaSwarm::AddParticle(const CTransform4f& xf) {
  if (mDeathParticleGenerator.get()) {
    mDeathParticleGenerator->SetParticleEmission(true);
    mDeathParticleGenerator->SetTranslation(xf.GetTranslation());
    mDeathParticleGenerator->ForceParticleCreation(mNumDeathParticles);
    mDeathParticleGenerator->SetParticleEmission(false);
  }
}

// @ fn_6_1200
void CBacteriaSwarm::UpdateParticles(float dt) {
  if (mDeathParticleGenerator.get()) {
    mDeathParticleGenerator->Update(dt);
  }
}

// @ fn_6_11BC
void CBacteriaSwarm::RenderParticles() const {
  if (mDeathParticleGenerator.get()) {
    gpRender->AddParticleGen(*mDeathParticleGenerator);
  }
}

// @ fn_6_10F0
void CBacteriaSwarm::PlayBoidSound(ushort sfx, const CVector3f& pos, float distanceSquared) {
  if (distanceSquared < mMaxAudibleDistance * mMaxAudibleDistance) {
    CAudioSys::C3DEmitterParmData parms(mMaxAudibleDistance, mSoundFallOff, 1, mMaxVolume,
                                        mMinVolume);
    parms.mPos = pos;
    parms.mDir = CVector3f::Zero();
    parms.mSfxId = sfx;
    CSfxManager::AddEmitter(parms, GetCurrentAreaId().Value(), true, false);
  }
}

// @ fn_6_1000
CVector3f CBacteriaSwarm::GetAimPosition(const CStateManager& mgr, float dt) const {
  if (mLockOnIndex == -1) {
    return mLastOrbitPosition;
  }
  if (mBlendingLockOn && dt == 0.f) {
    return (1.f - mLockOnBlend) * mLockOnBlendStart + mLockOnBlend * mLastOrbitPosition;
  }
  return mLastOrbitPosition + dt * mBoids[mLockOnIndex].mVelocity;
}

// @ fn_6_FE4
CVector3f CBacteriaSwarm::GetOrbitPosition(const CStateManager& mgr) const {
  return mLastOrbitPosition;
}

// @ fn_6_E28
bool CBacteriaSwarm::IsBoidVisibleForLockOn(const CStateManager& mgr, const CBoid& boid,
                                            const CVector3f& cameraPos,
                                            const CVector3f& cameraForward) const {
  const CVector3f delta = boid.GetTranslation() - cameraPos;
  const float distance = delta.Magnitude();
  const float inv = 1.f / distance;
  const CVector3f dir = inv * delta;
  if (CVector3f::Dot(cameraForward, dir) > 0.9238795f) {
    const CMaterialFilter filter = CMaterialFilter::MakeInclude(CMaterialList(kMT_Solid));
    const CRayCastResult result = mgr.RayStaticIntersection(cameraPos, dir, distance, filter);
    if (!result.IsValid()) {
      return true;
    }
  }
  return false;
}

// @ fn_6_D0C
int CBacteriaSwarm::GetLockOnIndex(CStateManager& mgr) const {
  const CTransform4f cameraXf = mgr.GetCameraManager(0)->GetFirstPersonCamera()->GetTransform();
  const CVector3f cameraPos = cameraXf.GetTranslation();
  const CVector3f cameraForward = cameraXf.GetForward();
  const bool playerOrbiting = mgr.GetPlayer(0)->GetOrbitTargetId() == GetUniqueId();
  if (mLockOnIndex != -1) {
    int result = -1;
    if (mBoids[mLockOnIndex].mActive &&
        IsBoidVisibleForLockOn(mgr, mBoids[mLockOnIndex], cameraPos, cameraForward)) {
      result = mLockOnIndex;
    }
    if (result != -1 && !playerOrbiting) {
      result = FindBestLockOnIndex(mgr);
    }
    return result;
  }
  return FindBestLockOnIndex(mgr);
}

// @ fn_6_B44
int CBacteriaSwarm::FindBestLockOnIndex(CStateManager& mgr) const {
  // 0xBA0-0xBD4: dot threshold 0.5, result=-1, squared orbit range and actual FP camera.
  float maxDot = 0.5f;
  int index = 0;
  int result = -1;
  float maxDistanceSq = mgr.GetPlayer(0)->GetOrbitMaxTargetDistance();
  maxDistanceSq *= maxDistanceSq;
  const CTransform4f cameraXf = mgr.GetCameraManager(0)->GetFirstPersonCamera()->GetTransform();
  const CVector3f cameraPos = cameraXf.GetTranslation();
  const CVector3f cameraForward = cameraXf.GetForward();
  for (rstl::vector< CBoid >::const_iterator it = mBoids.begin(); it != mBoids.end();
       ++it, ++index) {
    if (it->mActive) { // 0xBF8-0xC00: byte 0x81 mask 0x20.
      const CVector3f delta = it->GetTranslation() - cameraPos;
      if (delta.MagSquared() > maxDistanceSq) { // 0xC04-0xC40: out-of-range continue.
        continue;
      }
      if (delta.CanBeNormalized()) { // 0xC44-0xC50: reject camera-coincident boid.
        const float dot = CVector3f::Dot(cameraForward, delta.AsNormalized());
        if (dot > maxDot) { // 0xC60-0xC7C: strict best-dot test; ties keep earlier index.
          result = index;
          maxDot = dot; // 0xC80-0xC84: update winner and threshold.
        }
      }
    }
  }
  return result;
}

// @ fn_6_A8C
void CBacteriaSwarm::UpdateLockOnBlend(int prevIndex, int newIndex, float dt) {
  if (newIndex >= 0) {
    if (prevIndex >= 0 && prevIndex != newIndex) {
      if (!mBlendingLockOn) {
        mLockOnBlendStart = mLastOrbitPosition;
      }
      mBlendingLockOn = true;
      mLockOnBlend = 0.f;
    }
    if (mBlendingLockOn) {
      mLockOnBlend += 3.f * dt;
      if (mLockOnBlend >= 1.f) {
        mBlendingLockOn = false;
      }
    }
  } else {
    mBlendingLockOn = false;
  }
}

// @ fn_6_A0
CEntity* LoadBacteriaSwarm(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrBacteriaSwarm sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrBacteriaSwarm.inc"

#ifdef TARGET_PC
  // fn_6_A0 never copies the loaded active (+0xc0, stored at 0x3E4 stb r0,0x2a8(r1)) into
  // editorProperties.active: after the property loop (0x614 blt) it goes straight to
  // 0x62C bl __nw__FUlPCcPCc, and LdrToEntityInfo (0x69C) reads editorProperties (r1+0x1e8).
#else
  sldrThis.editorProperties.active = sldrThis.active;
#endif
  return rs_new CBacteriaSwarm(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties), sldrThis.editorProperties.transform.scale,
      LdrToTransform4f(sldrThis.editorProperties), LdrToActorParameters(sldrThis.actorInformation),
      LdrToBasicSwarmData(sldrThis.basicSwarmProperties), sldrThis.unknown_0x4a85a2da,
      sldrThis.containmentPriority, sldrThis.patrolTurnSpeed, sldrThis.avoidSafeZoneTurnSpeed,
      sldrThis.bacteriaPatrolSpeed, sldrThis.bacteriaSafeZoneEscapeSpeed,
      sldrThis.bacteriaPlayerPursuitSpeed, sldrThis.bacteriaAcceleration,
      sldrThis.bacteriaDeceleration, sldrThis.bacteriaParticleEffect, sldrThis.bacteriaPatrolColor,
      sldrThis.bacteriaPlayerPursuitColor, sldrThis.colorChangeTime, sldrThis.minPatrolSoundTime,
      sldrThis.maxPursuitSoundTime, sldrThis.patrolSoundWeight, sldrThis.minPursuitSoundTime,
      sldrThis.maxPursuitSoundTime, sldrThis.pursuitSoundWeight, sldrThis.patrolSound,
      sldrThis.pursuitSound, sldrThis.soundFallOff, sldrThis.maxAudibleDistance, sldrThis.minVolume,
      sldrThis.maxVolume, CStaticRes(sldrThis.bacteriaScanModel, CVector3f(0.f, 0.f, 0.f)),
      sldrThis.spawnInstantly, false);
}

#ifndef MONOLITHIC
// @ fn_6_70
static void SetFuncPtrs() {
  static SBacteriaSwarm_FuncPtrs funcPtrs;
  funcPtrs.mLoader = &LoadBacteriaSwarm;
  SetSBacteriaSwarm_FuncPtrs(&funcPtrs);
}

extern "C" void RELMain() { SetFuncPtrs(); }

extern "C" void RELExit() { SetSBacteriaSwarm_FuncPtrs(nullptr); }
#endif
