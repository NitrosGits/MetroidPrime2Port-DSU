#ifndef _PORTINGSHARED
#define _PORTINGSHARED

// Port-only: behaviour helpers Ing.rel (module 29) and MinorIng.rel (module 44) both carry, as
// free functions so CIng can reuse what CMinorIng ported. Each names its retail pair
// (`docs/module-analysis/IngFamily.md` B.2); the bodies follow MinorIng's copy.

#include "Kyoto/Math/CVector3f.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/PathFinding/CPathFindPointSearch.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/ScriptObjects/CScriptSafeZone.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include <float.h>

// Search flags the Ings run with, and the ones the lure helpers switch to for one query.
static const uint kPortIngSearchFlags = 0x301;

// Point search between two points of the area's point graph (retail indexes CPFArea's point
// array, +0x190, with stride 0x1c).
inline bool PortIngPointSearch(CPathFindPointSearch& search, CPFArea& area, int from, int to) {
  return search.Search(area.GetPoint(from), area.GetPoint(to)) == CPathFindPointSearch::kR_Success;
}

// @ fn_44_204C
// The MinorIng's copy of the lure point query (the Ing module's is fn_29_1C98): the retail's
// fn_44_204C (asm 0x204C) reads the safe zone id at 0xcda, gets the zone, asks the search for the
// height of the point five units above it with the flags at 3, and writes the point below it; the
// host's body is the same query. The Ing module's fn_29_1C98 (0x15C) is the same body with the
// zone id at 0xd78: the Agon Wastes coverage run of 2026-10-22 (lane ing4, `var <ing>
// ing.lurezone <id>; codecall <ing> SetLuredDest|SetLuredPointDest` in one tick, forced, over
// an invalid id, a non-zone object and every safe zone) ran every arm (no zone, height found,
// height not found). TCastToConstPtr is TCastToPtr (asm 0x1CD0) on a const pointer.
// @ fn_29_1C98
inline bool PortIngLuredMeshPoint(CStateManager& mgr, CPathFindSearch& search, TUniqueId safeZone,
                                  CVector3f& out) {
  const CScriptSafeZone* zone = TCastToConstPtr< CScriptSafeZone >(mgr.GetObjectById(safeZone));
  if (zone == nullptr) {
    return false;
  }
  const CVector3f above = zone->GetTranslation() + CVector3f::Up() * 5.f;
  float height = 0.f;
  search.SetFlags(3);
  const bool found =
      search.GetHeightOfPointAboveMesh(above, height, 0.f) == CPathFindSearch::kR_Success;
  if (found) {
    out = above + CVector3f::Down() * height;
  }
  search.SetFlags(kPortIngSearchFlags);
  return found;
}

// fn_44_21A8 = fn_29_53E0 (code function SetLuredDest): `dest` becomes the lure point when a mesh
// path to it exists (flags 1 for that query), else stays `pos`. Returns whether it was found.
inline bool PortIngLuredDest(CStateManager& mgr, CPathFindSearch& search, TUniqueId safeZone,
                             const CVector3f& pos, CVector3f& dest) {
  dest = pos;
  CVector3f lure = pos;
  if (!PortIngLuredMeshPoint(mgr, search, safeZone, lure)) {
    return false;
  }
  search.SetFlags(1);
  const bool found = search.PathExists(pos, lure) == CPathFindSearch::kR_Success;
  if (found) {
    dest = lure;
  }
  search.SetFlags(kPortIngSearchFlags);
  return found;
}

// fn_44_22B0 = fn_29_51D0 (code function SetLuredPointDest): point path from the point closest to
// `pos` (within 20) to the flag-1 point nearest `pos` that connects to it and has a mesh path to the
// lure point.
inline bool PortIngLuredPointDest(CStateManager& mgr, CPathFindSearch& search,
                                  CPathFindPointSearch& pointSearch, CPFArea* area,
                                  TUniqueId safeZone, const CVector3f& pos) {
  pointSearch.ClearWaypoints();
  CVector3f lure = pos;
  if (area == nullptr || !PortIngLuredMeshPoint(mgr, search, safeZone, lure)) {
    return false;
  }
  bool found = false;
  search.SetFlags(1);
  int from = -1;
  pointSearch.FindClosestPhysicalPoint(pos, from, CPathFindPointSearchFilter(20.f, 0, -1));
  if (from != -1) {
    int best = -1;
    float bestDistSq = FLT_MAX;
    for (int i = 0; i < area->GetNumPoints(); ++i) {
      const CPFPoint& point = area->GetPoint(i);
      if ((point.GetFlags() & 1) == 0 || !area->PointPathExists(from, i)) {
        continue;
      }
      const float distSq = (pos - point.GetPosition()).MagSquared();
      if (distSq < bestDistSq &&
          search.PathExists(point.GetPosition(), lure) == CPathFindSearch::kR_Success) {
        best = i;
        bestDistSq = distSq;
      }
    }
    found = best != -1 && PortIngPointSearch(pointSearch, *area, from, best);
  }
  search.SetFlags(kPortIngSearchFlags);
  return found;
}

#endif // _PORTINGSHARED
