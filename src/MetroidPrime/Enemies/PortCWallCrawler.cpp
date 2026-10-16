// Port: CWallCrawler, the base class of the wall-crawling enemies (Parasite, PillBug, Kralee,
// OctapedeSegment, WallWalker). Upstream has the header but no implementation, and the class's
// code lives in the WallCrawler REL module, which is PowerPC only, so this file is the port's
// host body for that module.
//
// Everything here is written for *behaviour*: the class's real shape is only known from the
// header (the members the derived classes' constructors need) and from the REL's disassembly,
// which has no names for anything but nine tiny accessors. So the file is split in two:
//
//   * what the header and the disassembly pin down - the constructor's member mapping, the
//     plane helpers (plain vector maths), the module's entry points - is implemented properly;
//   * the AI (Think, the aligning and waypoint machinery, the constraint state machine) is a
//     stand-in that forwards to CPatterned where that is meaningful and otherwise does nothing,
//     and says so once per method through `StandIn()`.
//
// The stand-ins keep the game running in areas whose module list needs WallCrawler.rel - which
// used to stop the port outright (`module prolog: RELProd/WallCrawler.rel is not compiled into
// the port`) - with those enemies inert instead of absent. Replacing a stand-in with the real
// body is a matter of reading its disassembly in
// ../MetroidPrime2Port/build/G2ME01/WallCrawler/asm and the function sizes in
// ../MetroidPrime2Port/config/G2ME01/rels/WallCrawler/symbols.txt:
//
//   fn_83_2374  the constructor (0x310)      - from the 22-argument call in the derived ctors
//   fn_83_208C  the destructor (0x70)
//   fn_83_1098  PreThink (0x204)             - vtable slot 4
//   fn_83_580   Think (0x828)                - vtable slot 5
//   fn_83_1FD0  AcceptScriptMsg (0xBC)       - vtable slot 6
//   fn_83_EE4   Render (0x20)                - vtable slot 11
//   fn_83_1310  (0x6B8) and fn_83_1B50 (0x164) - called from the derived classes' AI
//   fn_83_1EB8  (0x4C, called 13 times by CPillBug) - a small hot helper

#include "MetroidPrime/Enemies/CWallCrawler.hpp"

#include "Collision/CCollidableSphere.hpp"
#include "Kyoto/Math/CSphere.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/TGameTypes.hpp"

#include <cstdio>

namespace {

// One line per stand-in, so a run's log lists exactly which parts of the class are still a
// shell rather than pretending the enemy is complete.
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
, mColSphere(CSphere(xf.GetTranslation(), sphereRadius), CMaterialList(kMT_Character))
, mCollisionCloseMargin(collisionCloseMargin)
, mAlignAngVel(alignAngVel)
, mTumbleAngle(f1)
, mPatrolPauseRemTime(0.f)
, mAdvanceWpRadius(advanceWpRadius)
, mPlayerObstructionMinDist(playerObstructionMinDist)
, mBendingHackWeight(f2)
, mType(type)
, mThinkCounter(0)
, x834_(0)
, x838_(CVector3f::Zero())
, x844_(f3)
, x848_(f4)
, x84c_(f5)
, x850_(f6)
, x854_(0.f)
, x858_(0.f)
, x85c_(0.f)
, mAlignToFloor(false)
, mHasAlignSurface(false)
, mPlayerObstructed(false)
, mDisableMove(disableMove)
, x860_27_(false)
, x860_28_(false)
, x860_29_(false)
, x860_30_(false)
, x861_24_(false) {
  StandIn("CWallCrawler::CWallCrawler");
}

CWallCrawler::~CWallCrawler() {}

// Everything below is the stand-in half: the enemies exist, keep their collision and draw
// through CPatterned, and do not crawl.

void CWallCrawler::PreThink(float dt, CStateManager& mgr) {
  StandIn("CWallCrawler::PreThink");
  CPatterned::PreThink(dt, mgr);
}

void CWallCrawler::Think(float dt, CStateManager& mgr) {
  StandIn("CWallCrawler::Think");
  CPatterned::Think(dt, mgr);
}

void CWallCrawler::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("CWallCrawler::AcceptScriptMsg");
  CPatterned::AcceptScriptMsg(mgr, msg);
}

void CWallCrawler::Render(const CStateManager& mgr) const {
  StandIn("CWallCrawler::Render");
  CPatterned::Render(mgr);
}

rstl::optional_object< CAABox > CWallCrawler::GetTouchBounds() const {
  return CPatterned::GetTouchBounds();
}

const CCollisionPrimitive* CWallCrawler::GetCollisionPrimitive() const { return &mColSphere; }

TUniqueId CWallCrawler::GetNextWaypoint(CStateManager& mgr, const CScriptWaypoint* waypoint,
                                        bool reverse) {
  StandIn("CWallCrawler::GetNextWaypoint");
  return kInvalidUniqueId;
}

void CWallCrawler::SetConstraint(CStateManager& mgr, int constraint) {
  StandIn("CWallCrawler::SetConstraint");
}

void CWallCrawler::UpdateConstraintPlane(CStateManager& mgr) {
  StandIn("CWallCrawler::UpdateConstraintPlane");
}

bool CWallCrawler::UpdateWPDestination(CStateManager& mgr) {
  StandIn("CWallCrawler::UpdateWPDestination");
  return false;
}

void CWallCrawler::AlignToFloor(CStateManager& mgr, float radius, const CVector3f& newPos, float dt) {
  StandIn("CWallCrawler::AlignToFloor");
}

void CWallCrawler::AlignToPlane(const CUnitVector3f& normal, float dt) {
  StandIn("CWallCrawler::AlignToPlane");
}

const CPlane& CWallCrawler::GetConstraintPlane() const {
  StandIn("CWallCrawler::GetConstraintPlane");
  return mAlignSurface.GetPlane();
}

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
