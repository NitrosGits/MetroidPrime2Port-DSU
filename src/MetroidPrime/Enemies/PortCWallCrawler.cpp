#include "MetroidPrime/Enemies/CWallCrawler.hpp"

#include "Collision/CCollidableSphere.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "Kyoto/Math/CPlane.hpp"
#include "Kyoto/Math/CQuaternion.hpp"
#include "Kyoto/Math/CRelAngle.hpp"
#include "Kyoto/Math/CSphere.hpp"
#include "Kyoto/Math/CloseEnough.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/ScriptObjects/CScriptWaypoint.hpp"
#include "MetroidPrime/TGameTypes.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "WorldFormat/CAreaOctTree.hpp"
#include "WorldFormat/CMetroidAreaCollider.hpp"

#include <cstdio>

namespace {

// Reports a stand-in the first time it runs, so a log lists what is still a shell.
void StandIn(const char* what) {
  static const char* sSeen[32] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == what) {
      return;
    }
  }
  if (sCount < 32) {
    sSeen[sCount++] = what;
  }
  std::fprintf(stderr, "[wallcrawler] %s is a port stand-in (see PortCWallCrawler.cpp)\n", what);
}

} // namespace

// The behaviour below is adapted from the Prime port's `CWallWalker` (MetroidPrimePort,
// src/MetroidPrime/Enemies/CWallWalker.cpp), which is the same class one game earlier: same
// members, same body-controller calls, same waypoint loop. Echoes' REL function sizes line up
// with it (Render 0x20 = a bare CPatterned::Render, AlignToFloor 0x6B8, Think 0x828), so this is
// the port's behaviour for those functions rather than a fresh reading of the disassembly.

namespace {

// Shared by the floor search below.
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

// Turns an actor's up vector towards a surface normal, clamped to an angular rate.
void OrientToSurfaceNormal(CWallCrawler& actor, const CVector3f& normal, float clampAngle) {
  const CVector3f& up = actor.GetTransform().GetUp();
  const float dot = CVector3f::Dot(up, normal);
  if (close_enough(dot, 1.f) || dot < -0.999f) {
    return;
  }
  const CQuaternion rotation = CQuaternion::ShortestRotationArcClamped(
      actor.GetTransform().GetUp(), normal, CRelAngle::FromRadians(clampAngle));
  const CQuaternion local(rotation.GetScalar(),
                          actor.GetTransform().TransposeRotate(rotation.GetVector()));
  actor.SetRotation((CQuaternion::FromMatrix(actor.GetTransform()) * local).BuildNormalized());
}

} // namespace

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
, mAlignSurface(CVector3f::Zero(), CVector3f::Right(), CVector3f::Forward(), 0xffffffff)
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
, x838_(CVector3f::Zero())
, x844_(f1)
, x848_(f2)
, x84c_(f3)
, x850_(f4)
, x854_(f5)
, x858_(f6)
, x85c_(0.f)
, mAlignToFloor(false)
, mHasAlignSurface(false)
, mPlayerObstructed(false)
, mDisableMove(disableMove)
, x860_27_(false)
, x860_28_(false)
, x860_29_(false)
, x860_30_(false)
, x861_24_(false) {}

CWallCrawler::~CWallCrawler() {}

void CWallCrawler::PreThink(float dt, CStateManager& mgr) {
  CPatterned::PreThink(dt, mgr);
  ++mThinkCounter;
  if (GetActive() && !mPlayerObstructed && mPatrolPauseRemTime <= 0.f && !mDisableMove &&
      BodyController() != nullptr && close_enough(BodyController()->GetPercentageFrozen(), 0.f) &&
      mAlignToFloor) {
    AddMotionState(PredictMotion(dt));
    ClearForcesAndTorques();
    if (mHasAlignSurface) {
      // Hold the enemy at one collision radius above the surface it is stuck to.
      const CPlane plane = mAlignSurface.GetPlane();
      const CVector3f position = GetTranslation();
      const float height = plane.GetHeight(position) - mColSphere.GetSphere().GetRadius() - 0.01f;
      SetTranslation(CVector3f::Lerp(position, position - height * plane.GetNormal(), 10.f * dt));
    }
    MoveCollisionPrimitive(CVector3f::Zero());
  }
}

void CWallCrawler::Think(float dt, CStateManager& mgr) {
  CPatterned::Think(dt, mgr);
  // The bending hack eases a weight towards 1 while the enemy is tumbling and back to 0 once it
  // is not; the additive animation it drives is not hooked up (its animation index lives in a
  // member the header does not name yet).
  const float speed = GetVelocityWR().Magnitude();
  if (mTumbleAngle > 0.f) {
    StandIn("CWallCrawler::Think bending animation");
    mBendingHackWeight = CMath::Clamp(0.f, mBendingHackWeight + dt * speed / 0.6f, 1.f);
  } else if (mBendingHackWeight > 0.f) {
    mBendingHackWeight = CMath::Clamp(0.f, mBendingHackWeight - dt * speed / 1.5f, 1.f);
  }
}

void CWallCrawler::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  CPatterned::AcceptScriptMsg(mgr, msg);
}

void CWallCrawler::Render(const CStateManager& mgr) const { CPatterned::Render(mgr); }

rstl::optional_object< CAABox > CWallCrawler::GetTouchBounds() const {
  return CPatterned::GetTouchBounds();
}

const CCollisionPrimitive* CWallCrawler::GetCollisionPrimitive() const { return &mColSphere; }

TUniqueId CWallCrawler::GetNextWaypoint(CStateManager& mgr, const CScriptWaypoint* waypoint,
                                        bool reverse) {
  // `reverse` only matters for the patrol order, which the waypoint itself owns; ask it either
  // way, as Prime's wall walker does.
  return waypoint != nullptr ? waypoint->NextWaypoint(mgr) : kInvalidUniqueId;
}

void CWallCrawler::SetConstraint(CStateManager& mgr, int constraint) {
  x834_ = constraint;
  if (constraint != 0) {
    UpdateConstraintPlane(mgr);
  }
}

void CWallCrawler::UpdateConstraintPlane(CStateManager& mgr) {
  // Echoes-only (no Prime counterpart): re-establish the surface the enemy is crawling on by
  // looking for one under it, which is what the constraint plane is built from. The retail body
  // (fn_83_148, 0x344) also folds in the patrol/wall logic; this is the align-surface half.
  StandIn("CWallCrawler::UpdateConstraintPlane");
  AlignToFloor(mgr, mColSphere.GetSphere().GetRadius(), GetTranslation(), 0.f);
}

bool CWallCrawler::UpdateWPDestination(CStateManager& mgr) {
  CScriptWaypoint* waypoint = TCastToPtr< CScriptWaypoint >(mgr.ObjectById(GetDestObj()));
  if (waypoint == nullptr) {
    return false;
  }
  const CVector3f position = waypoint->GetTranslation();
  const CVector3f delta = position - GetTranslation();
  if (delta.MagSquared() < mAdvanceWpRadius * mAdvanceWpRadius) {
    mDestObj = GetNextWaypoint(mgr, waypoint, false);
    // Echoes' CScriptWaypoint has no pause accessor (its fieldless base is what upstream has);
    // Prime's wall walker also rests at a waypoint that asks for it, so that is missing here.
    mgr.DeliverScriptMsg(CScriptMsg(GetUniqueId(), waypoint->GetUniqueId(), kSM_Arrived));
    SetDestPos(position);
    return true;
  }
  SetDestPos(position);
  return false;
}

void CWallCrawler::AlignToFloor(CStateManager& mgr, float radius, const CVector3f& newPos, float dt) {
  const float margin = radius + mCollisionCloseMargin;
  const CVector3f extent(margin, margin, margin);
  CAreaCollisionCache cache(CAABox(newPos - extent, newPos + extent));
  CGameCollision::BuildAreaCollisionCache(mgr, cache);

  if (mHasAlignSurface) {
    mHasAlignSurface = PointOnSurface(mAlignSurface, newPos);
  }
  if (!mHasAlignSurface || (mThinkCounter & 3) == 0) {
    float best = margin;
    bool found = false;
    for (int i = 0; i < static_cast< int >(cache.GetNumCaches()); ++i) {
      const CMetroidAreaCollider::COctreeLeafCache& leaf = cache.GetOctreeLeafCache(i);
      for (int j = 0; j < leaf.GetNumLeaves(); ++j) {
        const CAreaOctTree::Node& node = leaf.GetLeaf(j);
        const CAreaOctTree::TriListReference triangles = node.GetTriangleArray();
        const CAreaOctTree& tree = node.GetOwner();
        for (int k = 0; k < triangles.GetSize(); ++k) {
          const CCollisionSurface surface = tree.GetTriangle(triangles.GetAt(k));
          const float dist =
              CMath::AbsF(CVector3f::Dot(surface.GetNormal(), newPos) - surface.GetPlane().GetConstant());
          if (dist < best && PointOnSurface(surface, newPos)) {
            best = dist;
            mAlignSurface = surface;
            found = true;
          }
        }
      }
    }
    mHasAlignSurface = found;
  }

  if (mHasAlignSurface) {
    OrientToSurfaceNormal(*this, CVector3f(mAlignSurface.GetNormal()), mAlignAngVel * dt);
    mTumbleAngle = 0.f;
  } else {
    const float angularVelocity =
        GetVelocityWR().Magnitude() / mColSphere.GetSphere().GetRadius();
    const float angle = angularVelocity * dt;
    OrientToSurfaceNormal(*this, GetTransform().GetForward(), angle);
    mTumbleAngle += angle;
  }
}

void CWallCrawler::AlignToPlane(const CUnitVector3f& normal, float dt) {
  OrientToSurfaceNormal(*this, CVector3f(normal), mAlignAngVel * dt);
}

const CPlane& CWallCrawler::GetConstraintPlane() const { return mAlignSurface.GetPlane(); }

CVector3f CWallCrawler::ProjectVectorToPlane(const CVector3f& vec, const CVector3f& planeDir) {
  return vec - CVector3f::Dot(vec, planeDir) * planeDir;
}

CVector3f CWallCrawler::GetClosestPointOnPlane(const CVector3f& point, const CPlane& plane) {
  return plane.GetClosestPoint(point);
}

// The module's entry points. Retail's are four bytes each (a bare `blr`), because the module's
// constructors do any registration; `module prolog` in platform/compiled_modules.cpp is what the
// port calls instead.
void RELMain() {}
void RELExit() {}

// The module also holds the WallWalker loader (the EType = kT_WallWalker variant). Nothing in
// the port's compiled units calls it, so it stays a safe null rather than a reach stub that
// would return whatever happened to be in r3.
CEntity* LoadWallWalker(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  std::fprintf(stderr, "[wallcrawler] LoadWallWalker is not implemented; actor skipped\n");
  return nullptr;
}
