# GeomBlobV2 REL (module 25): analysis notes

The Ing's surface blob: a CCollisionTracker (an effect actor with its own CElementGen) whose
particles trace strands along the collision surfaces around it (CSurfacePath/SBlobStrand). Code is
upstream's `src/MetroidPrime/Enemies/CCollisionTracker.cpp` and `CSurfaceParticleEffect.cpp`.
Retail asm: `build/G2ME01/GeomBlobV2/asm/MetroidPrime/Enemies/{CCollisionTracker,CSurfaceParticleEffect}.s`.
Per-function table: `GeomBlobV2_functions.tsv` (B upstream body, H weak/inline copy, T REL runtime).

## Who uses it

- CCollisionTracker: every Ing (REL 29) makes one in its create branch (`CIng::CreateBlob`,
  fn_29_4830, via `LoadGeomBlobV2`), with emission 0; `CIng::AlignBlobToSurface` (fn_29_4604) sets
  the emission to 1 while the Ing is an ing spot (or a dead corporeal/forming one that started as a
  spot). So any area with ing spots runs Think/Trace; the smoke family `ing` (1baa96c2,2735236b) is
  the closest.
- CSurfaceParticleEffect is dead in retail: `SetSurfaceParticleEffectFuncPtrs` stores the factory in
  `gFactory_SurfaceParticleEffect` (.sbss 0x80419558), and no DOL or REL code reads it. Its bodies
  can only be covered by forcing (no console command creates one).

## Survey and fixes (gb1, 2026-10-23)

Every sub-100% body was diffed normalized (labels, relocations, registers, rodata constants
resolved) against upstream. One fix (`TARGET_PC`):

| where | retail | port before |
|---|---|---|
| CSurfacePath::Trace (0x3E0-0x3F4 `fcmpo f1,0.8; bge` break) | `!(abs(cos) < 0.8)` | `abs(cos) >= 0.8` (cror; beq) |

The value already passed `abs(cos) > 0.2` (0x3BC `ble`), so a NaN cannot reach the test; the fix
only keeps the asm shape.

No behaviour difference (scheduling, register or layout only) in: the CT ctor (`__dt__6CToken`
flag -1 vs 0), Simplify (retail resizes instead of popping), SlideOffPlane, CastRay,
FindNearestTriangle, IsFinished (`bge` = `<`), IsSameTriangle, Cross, IsMovingAlongEdge, CT/SPE
ProjectOntoPlane, FindExitEdge, GetBarycentric, the CCollisionCacheIterator/optional_object
assignments (retail skips the padding words 0x24/0x34/0x3c), CT/SPE AcceptScriptMsg, CT Think,
SPE Think, GatherCollisionSurfaces, GatherFromBox/ObbTree, FindNearestSurface*,
BuildOrientationFromNormal, PreRenderAllViewports and the SPE ctor.

Accepted retail quirks, not changed:
- GetInverseArea computes the negated determinant in all three axis cases. The sign cancels in
  FindExitEdge (t = -b/delta) and IsMovingAlongEdge takes the abs, so the host's exact sign is
  equivalent.
- AddSegment reserves capacity before the 0.95 dot test (upstream after); only the allocation
  differs.

Checked facts: CParticle is mEndFrame 0x0, mPos 0x4, mVel 0x1c, mStartFrame 0x28, mLineLength
0x2c (0x38 bytes); SPE flag byte 0x176 is MSB-first (0x80 mHasRenderBounds, 0x40 mFlag); CT
Think's cache update is policy 0 (`kCUP_KeepNearListIds`).

## Open

- Coverage and markers not done: the gb1 coverage build failed on a full disk. Spec:
  `tools/cov/specs/geomblobv2.py` (ing family area). Expected reach: CT Think, Trace and its
  helpers, AddSegment, Simplify, IsFinished, GetPathPosition, Render/RenderStrands; SPE none.
- SPE Think's fade index divides by `mEndFrame - mStartFrame` (0x5444 `divw`): zero would trap on
  the host, but the class is never created, so no guard was added.
