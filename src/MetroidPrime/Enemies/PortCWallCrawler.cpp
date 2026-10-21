#include "MetroidPrime/Enemies/CWallCrawler.hpp"

#include "Collision/CCollidableSphere.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CPlane.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CSphere.hpp"
#include "Kyoto/Math/CUnitVector3f.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIWaypoint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/TGameTypes.hpp"
#include "MetroidPrime/Tweaks/CTweakBall.hpp"
#include "WorldFormat/CAreaOctTree.hpp"
#include "WorldFormat/CMetroidAreaCollider.hpp"

// Read from the retail WallCrawler REL (G2ME01, asm/auto_00_00000000_text.s) for behaviour, not
// for code shape; docs/module-analysis/WallCrawler.md has the function map and the evidence.
//
// The six trailing ctor floats (f1..f6) land in x848_..x85c_; x838_/x844_ are the constraint
// plane's normal and constant (the header's names are offsets only):
//   x848_ (f1)  touch-bounds scale
//   x84c_ (f2)  floor-snap rate, per 1/60 s
//   x850_ (f3)  bend weight ramp-up divisor while tumbling     (kART_Six)
//   x854_ (f4)  bend weight ramp-down divisor while tumbling
//   x858_ (f5)  bend weight ramp-up divisor while re-orienting (kART_Five)
//   x85c_ (f6)  bend weight ramp-down divisor while re-orienting
// The 0x860/0x861 bits past mDisableMove (the masks are the retail's, asm 0x258C-0x25F0):
//   x860_28_  (0x08) tumbling: no align surface, rolling about the forward axis (set in AlignToFloor)
//   x860_29_  (0x04) on a surface but up is still > ~30 degrees off its normal (set in AlignToFloor)
//   x860_30_  (0x02) the kART_Six additive reaction is playing
//   x860_31_  (0x01) the kART_Five additive reaction is playing
//   x861_24_  (0x80) bending enabled (ctor sets it; nothing in this module clears it)

namespace {

// @ fn_83_1CB4
// @ fn_83_1F04
// @ fn_83_1E78
bool PointOnSurface(const CCollisionSurface& surface, const CVector3f& point) {
  const CVector3f normal = surface.GetNormal();
  const CVector3f projected = point - CVector3f::Dot(point - surface.GetVert(0), normal) * normal;
  for (int i = 0; i < 3; ++i) {
    const CVector3f edge = surface.GetVert((i + 2) % 3) - surface.GetVert(i);
    const CVector3f delta = projected - surface.GetVert(i);
    if (CVector3f::Dot(normal, CVector3f::Cross(delta, edge)) < 0.f) {
      return false;
    }
  }
  return true;
}

// Plays or retunes the bend reaction while the weight is up, stops it once it reaches zero.
void DeliverBend(CBodyController* bc, float weight, bool& playing, pas::EAdditiveReactionType type) {
  if (weight > 0.0001f) {
    if (playing) {
      bc->CommandMgr().DeliverCmd(CBCAdditiveWeightCmd(weight));
    } else {
      bc->CommandMgr().DeliverCmd(CBCAdditiveReactionCmd(type, weight, true));
    }
    playing = true;
  } else {
    bc->CommandMgr().DeliverCmd(CBodyStateCmd(kBSC_StopReaction));
    playing = false;
  }
}

} // namespace

// @ fn_83_2374
CWallCrawler::CWallCrawler(EPatternedAI character, TUniqueId uid, const rstl::string& name,
                           EFlavorType flavor, CEntityInfo& info, const CTransform4f& xf,
                           const CModelData& mData, const CPatternedInfo& pInfo,
                           EMovementType moveType, EColliderType colType, EBodyType bodyType,
                           const CActorParameters& actParms, float sphereRadius,
                           float collisionCloseMargin, float alignAngVel, float advanceWpRadius,
                           float playerObstructionMinDist, EType type, bool disableMove, float f1,
                           float f2, float f3, float f4, float f5, float f6)
: CPatterned(character, uid, name, flavor, info, xf, mData, pInfo, moveType, colType, bodyType,
             actParms)
, mAlignSurface(CVector3f::Zero(), CVector3f::Right(), CVector3f::Forward(), 0xffffffff) // @ fn_83_26CC (0x54): the CCollisionSurface ctor (asm 0x24A0)
, mColSphere(CSphere(CVector3f::Zero(), sphereRadius), GetMaterialList())
, mCollisionCloseMargin(collisionCloseMargin)
, mAlignAngVel(alignAngVel)
, mTumbleAngle(0.f)
, mPatrolPauseRemTime(0.f)
, mAdvanceWpRadius(advanceWpRadius)
, mPlayerObstructionMinDist(playerObstructionMinDist)
, mBendingHackWeight(0.f)
, mType(type)
, mThinkCounter(0)
, x834_(0)
, x838_(CVector3f(0.f, 1.f, 0.f))
, x844_(0.f)
, x848_(f1)
, x84c_(f2)
, x850_(f3)
, x854_(f4)
, x858_(f5)
, x85c_(f6)
, mAlignToFloor(false)
, mHasAlignSurface(false)
, mPlayerObstructed(false)
, mDisableMove(disableMove)
, x860_28_(false)
, x860_29_(false)
, x860_30_(false)
, x860_31_(false)
, x861_24_(true)
, mConstraintPlane(0.f, CUnitVector3f(0.f, 1.f, 0.f, CUnitVector3f::kN_No)) {}

// @ fn_83_208C
CWallCrawler::~CWallCrawler() {}

// @ fn_83_1098
void CWallCrawler::PreThink(float dt, CStateManager& mgr) {
  CPatterned::PreThink(dt, mgr);
  if (!GetActive() || mPlayerObstructed) {
    return;
  }
  if (mPatrolPauseRemTime <= 0.f && !mDisableMove &&
      CMath::AbsF(BodyController()->GetPercentageFrozen()) < 0.00001f) {
    if (!mAlignToFloor) {
      return;
    }
    // @ fn_83_129C (0x6C): the CMotionState copy the retail makes before AddMotionState
    // (asm 0x1148-0x115C) - the C++ copy of the predicted state is the same value.
    AddMotionState(PredictMotion(dt));
    ClearForcesAndTorques();
    if (mHasAlignSurface) {
      // Ease towards one collision radius (+0.01) above the surface, at x84c_ per 1/60 s.
      const CPlane plane = mAlignSurface.GetPlane();
      const CVector3f position = GetTranslation();
      const float height = plane.GetHeight(position) - mColSphere.GetSphere().GetRadius() - 0.01f;
      const float t = 60.f * x84c_ * dt;
      SetTranslation(position * (1.f - t) + (position - height * plane.GetNormal()) * t);
    }
    MoveCollisionPrimitive(CVector3f::Zero());
  } else if (mPatrolPauseRemTime > 0.f) {
    Stop();
  }
}

// @ fn_83_580
void CWallCrawler::Think(float dt, CStateManager& mgr) {
  if (GetMovable()) {
    AddMaterial(kMT_Solid, mgr);
  } else {
    RemoveMaterial(kMT_Solid, mgr);
  }
  CPatterned::Think(dt, mgr);

  if (mPatrolPauseRemTime > 0.f) {
    mPatrolPauseRemTime = CMath::Max(0.f, mPatrolPauseRemTime - dt);
  }

  CBodyController* bc = BodyController();
  if (bc->HasBodyState(pas::kAS_AdditiveReaction)) {
    // The bend: an additive reaction whose weight follows how fast the enemy moves while it
    // tumbles (kART_Six) or turns onto a new surface (kART_Five).
    x860_28_ = x860_28_ && x861_24_;
    x860_29_ = x860_29_ && x861_24_;
    const bool tumbleBend = x860_28_ || x860_30_;
    const float speed = GetVelocityWR().Magnitude();
    if ((x860_28_ && !x860_31_) || (x860_29_ && !x860_30_)) {
      if (mBendingHackWeight < 1.f) {
        mBendingHackWeight += dt * speed / (tumbleBend ? x850_ : x858_);
        if (mBendingHackWeight >= 1.f) {
          mBendingHackWeight = 1.f;
        }
      }
    } else if (mBendingHackWeight > 0.f) {
      mBendingHackWeight -= dt * speed / (tumbleBend ? x854_ : x85c_);
      if (mBendingHackWeight <= 0.f) {
        mBendingHackWeight = 0.f;
      }
    }

    if (mBendingHackWeight > 0.f || x860_30_ || x860_31_) {
      bool playing;
      if (tumbleBend) {
        playing = x860_30_;
        DeliverBend(bc, mBendingHackWeight, playing, pas::kART_Six);
        x860_30_ = playing;
      } else {
        playing = x860_31_;
        DeliverBend(bc, mBendingHackWeight, playing, pas::kART_Five);
        x860_31_ = playing;
      }
    }
  }

  if (!mAlive) {
    return;
  }
  CPlayer* player = mgr.Player(0);
  const rstl::optional_object< CAABox > playerBox = player->GetTouchBounds();
  const rstl::optional_object< CAABox > box = GetTouchBounds();
  if (!playerBox.valid() || !box.valid() || !playerBox->DoBoundsOverlap(*box)) {
    return;
  }
  const CMaterialFilter filter =
      CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList());
  if (player->GetMorphBall()->IsBoosting()) {
    // Boosting into it hurts the crawler instead; the contact timer is left alone.
    mgr.ApplyDamage(player->GetUniqueId(), GetUniqueId(), player->GetUniqueId(),
                    gpTweakBall->GetBoostBallDamage(), filter, CVector3f::Zero());
  } else if (mCurDamageRemTime <= 0.f) {
    CDamageInfo damage = GetContactDamage();
    damage.SetNoImmunity(true);
    mgr.ApplyDamage(GetUniqueId(), player->GetUniqueId(), GetUniqueId(), damage, filter,
                    CVector3f::Zero());
    mCurDamageRemTime = mDamageWaitTime;
  }
}

// @ fn_83_1FD0
void CWallCrawler::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  if (msg.GetMessage() == kSM_Create) {
    AddMaterial(kMT_Immovable, mgr);
    RemoveMaterial(kMT_Solid, mgr);
    AddMaterial(kMT_NonSolidDamageable, mgr);
  }
  CPatterned::AcceptScriptMsg(mgr, msg);
}

// @ fn_83_EE4
void CWallCrawler::Render(const CStateManager& mgr) const { CPatterned::Render(mgr); }

// @ fn_83_4C8
rstl::optional_object< CAABox > CWallCrawler::GetTouchBounds() const {
  // Retail calls the CPhysicsActor vtable slot at 0x80 for the transform; GetPrimitiveTransform is
  // the virtual there (inferred from the slot, not from a symbol).
  return GetBaseBoundingBox().GetTransformedAABox(GetPrimitiveTransform() *
                                                  CTransform4f::Scale(x848_));
}

// @ fn_83_1308
const CCollisionPrimitive* CWallCrawler::GetCollisionPrimitive() const { return &mColSphere; }

// @ fn_83_D8
TUniqueId CWallCrawler::GetNextWaypoint(CStateManager& mgr, const CScriptWaypoint* waypoint,
                                        bool reverse) {
  // Retail ignores `reverse` here; subclasses override the slot.
  return waypoint->NextWaypoint(mgr);
}

// @ fn_83_114
void CWallCrawler::SetConstraint(CStateManager& mgr, int constraint) {
  x834_ = constraint;
  if (constraint != 0) {
    UpdateConstraintPlane(mgr);
  }
}

// @ fn_83_148
void CWallCrawler::UpdateConstraintPlane(CStateManager& mgr) {
  // The constraint plane runs through the next three non-collinear patrol waypoints (up to 30 are
  // walked). With only two, it is the vertical-ish plane through them and a point one unit above
  // the second; with fewer, the old plane stays.
  TUniqueId dest = GetDestObj();
  if (dest == kInvalidUniqueId) {
    dest = GetConnectedObject(mgr, kSS_Patrol, kSM_Follow);
  }
  const CScriptWaypoint* waypoint = TCastToConstPtr< CScriptWaypoint >(mgr.GetObjectById(dest));
  CVector3f points[3];
  int count = 0;
  for (int i = 0; count < 3 && i < 30 && waypoint != nullptr; ++i) {
    const CVector3f pos = waypoint->GetTranslation();
    bool collinear = false;
    if (count == 2) {
      const CVector3f a = (points[0] - points[1]).AsNormalized();
      const CVector3f b = (pos - points[1]).AsNormalized();
      collinear = CMath::AbsF(CVector3f::Dot(a, b)) > 0.984f;
    }
    if (!collinear) {
      points[count++] = pos;
    }
    waypoint = TCastToConstPtr< CScriptWaypoint >(
        mgr.GetObjectById(GetNextWaypoint(mgr, waypoint, false)));
  }

  CPlane plane(0.f, CUnitVector3f(x838_, CUnitVector3f::kN_No));
  if (count == 3) {
    plane = CPlane(points[0], points[1], points[2]);
  } else if (count == 2 && CMath::AbsF(points[0].GetX() - points[1].GetX()) >= 0.00001f &&
             CMath::AbsF(points[0].GetY() - points[1].GetY()) >= 0.00001f) {
    plane = CPlane(points[0], points[1], points[1] + CVector3f(0.f, 0.f, 1.f));
  } else {
    return;
  }
  x838_ = plane.GetNormal();
  x844_ = plane.GetConstant();
}

// @ fn_83_F04
bool CWallCrawler::UpdateWPDestination(CStateManager& mgr) {
  CScriptWaypoint* waypoint = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(GetDestObj()));
  if (waypoint == nullptr) {
    return false;
  }
  bool advanced = false;
  const CVector3f position = waypoint->GetTranslation();
  if ((position - GetTranslation()).MagSquared() < mAdvanceWpRadius * mAdvanceWpRadius) {
    mDestObj = GetNextWaypoint(mgr, waypoint, true);
    advanced = true;
    if (const CScriptAIWaypoint* aiWaypoint = TCastToConstPtr< CScriptAIWaypoint >(waypoint)) {
      if (CMath::AbsF(aiWaypoint->GetPause()) >= 0.00001f) {
        mPatrolPauseRemTime = aiWaypoint->GetPause();
        // Retail tests the EType word for zero here (no named type is 0).
        if (mType == 0) {
          BodyController()->SetLocomotionType(pas::kLT_Relaxed);
        }
      }
    }
    mgr.SendScriptMsg(waypoint, GetUniqueId(), kSM_Arrived);
  }
  SetDestPos(position);
  return advanced;
}

// @ fn_83_1310
void CWallCrawler::AlignToFloor(CStateManager& mgr, float radius, const CVector3f& newPos, float dt) {
  // The retail's search box is radius+margin wide (asm 0x1390-0x13D4) while its best distance
  // starts at the radius itself (f24 = the radius argument, asm 0x1390).
  const float extent = radius + mCollisionCloseMargin;
  const CVector3f half(extent, extent, extent);
  CAreaCollisionCache cache(CAABox(newPos - half, newPos + half));
  CGameCollision::BuildAreaCollisionCache(mgr, cache);
  float best = radius;

  // The surface the crawler stands on is re-tested every frame (asm 0x1418-0x1438): the flag is
  // the byte 0x860's 0x40 mask (mHasAlignSurface). The retail never sets the 0x20 mask
  // (mPlayerObstructed) in this module - only PreThink reads it (asm 0x10CC).
  if (mHasAlignSurface) {
    mHasAlignSurface = PointOnSurface(mAlignSurface, newPos);
  }
  // The search runs when there is no surface, or on every fourth think (asm 0x1440-0x1450).
  if (!mHasAlignSurface || (mThinkCounter & 3) == 0) {
    bool found = false;
    for (int i = 0; i < static_cast< int >(cache.GetNumCaches()); ++i) {
      const CMetroidAreaCollider::COctreeLeafCache& leaf = cache.GetOctreeLeafCache(i);
      for (int j = 0; j < leaf.GetNumLeaves(); ++j) {
        const CAreaOctTree::Node& node = leaf.GetLeaf(j);
        const CAreaOctTree::TriListReference triangles = node.GetTriangleArray();
        const CAreaOctTree& tree = node.GetOwner();
        for (int k = 0; k < triangles.GetSize(); ++k) {
          const CCollisionSurface surface = tree.GetTriangle(triangles.GetAt(k));
          float dist;
          if (x834_ == 0) {
            dist = CMath::AbsF(surface.GetPlane().GetHeight(newPos));
          } else {
            // Constrained: AI-passthrough triangles are skipped, and the distance of a triangle
            // whose normal agrees with the constraint plane (> 0.5) is scaled up by 1 + that
            // agreement. Constraint 1 compares with the normal, 2 with its reverse, 3 with either.
            if ((surface.GetSurfaceFlags() & (static_cast< u64 >(1) << kMT_AIPassthrough)) != 0) {
              continue;
            }
            float agree = CVector3f::Dot(surface.GetNormal(), x838_);
            if (x834_ == 2) {
              agree = -agree;
            } else if (x834_ == 3) {
              agree = CMath::AbsF(agree);
            }
            float scale = 1.f;
            if (agree > 0.5f) {
              scale += agree;
            }
            dist = scale * CMath::AbsF(surface.GetPlane().GetHeight(newPos));
          }
          if (dist < best && PointOnSurface(surface, newPos)) {
            best = dist;
            // @ fn_83_19C8 (0x5C): the CCollisionSurface copy (asm 0x14BC and 0x160C).
            mAlignSurface = surface;
            found = true;
          }
        }
      }
    }
    mHasAlignSurface = found;
  }

  if (mHasAlignSurface) {
    const CVector3f normal = mAlignSurface.GetNormal();
    AlignToPlane(CUnitVector3f(normal, CUnitVector3f::kN_No), mAlignAngVel * dt);
    mTumbleAngle = 0.f;
    x860_28_ = false;
    x860_29_ = CVector3f::Dot(GetTransform().GetUp(), normal) < 0.86f;
  } else {
    // Nothing to stand on: roll forward about the sphere at its rolling speed (degrees/s).
    const CVector3f forward = GetTransform().GetForward();
    const CVector3f& velocity = GetVelocityWR();
    const float angVel = 57.295776f * velocity.Magnitude() / mColSphere.GetSphere().GetRadius();
    const CVector3f target = CVector3f::Dot(velocity, forward) > 0.f ? forward : -forward;
    AlignToPlane(CUnitVector3f(target, CUnitVector3f::kN_No), angVel * dt);
    x860_28_ = true;
    x860_29_ = false;
    mTumbleAngle += angVel * dt;
  }
}

// `degrees` is the most the up vector may turn this call.
// @ fn_83_1B50
void CWallCrawler::AlignToPlane(const CUnitVector3f& normal, float degrees) {
  const CVector3f up = GetTransform().GetUp();
  const float dot = CVector3f::Dot(up, normal);
  if (CMath::AbsF(dot - 1.f) < 0.00001f || dot < -0.999f) {
    return;
  }
  const CQuaternion rotation = CQuaternion::ShortestRotationArcClamped(
      up, normal, CRelAngle::FromRadians(degrees * 0.017453292f));
  const CQuaternion local(rotation.GetScalar(), GetTransform().TransposeRotate(rotation.GetVector()));
  SetRotation((CQuaternion::FromMatrix(GetTransform()) * local).BuildNormalized());
}

// GetPlane() builds the plane by value; returning it directly bound the reference to a temporary,
// which GCC compiles to a null reference (PillBug's patrol then faulted in CPlane::GetClosestPoint).
// Retail returns the constraint plane stored at 0x838 (x838_ normal, x844_ constant).
// @ fn_83_D0
const CPlane& CWallCrawler::GetConstraintPlane() const {
  mConstraintPlane = CPlane(x844_, CUnitVector3f(x838_, CUnitVector3f::kN_No));
  return mConstraintPlane;
}

// @ fn_83_1EB8
CVector3f CWallCrawler::ProjectVectorToPlane(const CVector3f& vec, const CVector3f& planeDir) {
  return vec - CVector3f::Dot(vec, planeDir) * planeDir;
}

// @ fn_83_1F7C
CVector3f CWallCrawler::GetClosestPointOnPlane(const CVector3f& point, const CPlane& plane) {
  return plane.GetClosestPoint(point);
}

// The module's entry points. Retail's are four bytes each (a bare `blr`), because the module's
// constructors do any registration; `module prolog` in platform/compiled_modules.cpp is what the
// port calls instead.
// @ fn_83_C8
void RELMain() {}
// @ fn_83_CC
void RELExit() {}
