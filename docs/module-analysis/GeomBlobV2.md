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

## Coverage and markers (gb1b, 2026-10-23)

Compiler coverage (build-port-cov) of the smoke family `ing` (1baa96c2,2735236b, ing_con.py, 16000
updates), `gcov_report.py build-port-cov <gcov_ing> --module GeomBlobV2`. 21 marks, each ALL-ARMS or
branchless with every line hit, and either 100% match or audited above: SSurfacePathSegment ctor
1F74, CSurfacePath ctor 21E0, GetCurrentLength 3E78, IsFinished 36A0, ProjectOntoPlane 2124,
SlideOffPlane D14, GetInverseArea 14E0, GetBarycentric 1334, FindExitEdge EAC, GetOppositeEdge
1288, IsMovingAlongEdge 10F8, CT dtor 3F78, SetParticleEmissionRateScalar 256C, GetTouchBounds
2578, BuildStrandTransform 3C98, RenderStrands 37D8, AddToRenderer 3F20, PreRenderAllViewports
3F58, LoadGeomBlobV2 2490, Set/ClearSurfaceParticleEffectFuncPtrs 48CC/48A8.

Not marked (arms the run did not take):
- Trace (118, 194, 195, 208, 222), Think (inactive/occluded early-outs 643/646, VEL1-less
  descriptions 656, strand-swap 672, 710), Simplify, IsSameTriangle, FindSharedEdge 430, CastRay
  462, FindNearestTriangle 505, AddSegment 528 (the `startNew` arm).
- AcceptScriptMsg: the light arms; the Ing blob's description has no light.
- Render: the dark-visor alpha arms. GetPathPosition 782/784. The CT ctor misses one line.
- The never-called accessors (ParticleSystem, Get/SetMinimumPathLength, GetParticleEmissionRateScalar, Touch).
- All CSurfaceParticleEffect bodies but the func-ptr pair: the class is never created.

Spec for call counts: `tools/cov/specs/geomblobv2.py`.

## Open

- SPE Think's fade index divides by `mEndFrame - mStartFrame` (0x5444 `divw`): zero would trap on
  the host, but the class is never created, so no guard was added.

## Coverage (gb3)

2026-10-24, base `a7cf4439b`: **17 new marks; four SPE bodies remain unmarked**.
Rechecked the remaining bodies against the retail asm and gb1's normalized audit. No additional
behaviour divergence was found. C++ changes are marker comments only; both paths remain identical.

### Reproduction and evidence

All runs used this worktree's `build-port-cov`, world/area `1baa96c2,2735236b`, and
`tools/games_running.sh --wait`. Successful runs exited normally, with zero `[port] caught` /
`reach-stub` and no debugger exceptions. The coverage source line numbers below are those of
**base a7cf4439b**, before inserting the gb3 marker comments. Retained full per-line counters:
`/tmp/opencode/reports/gb3-gcov.json`; report: `/tmp/opencode/reports/gb3.md`.

| run | invocation after `BUILD=$PWD/build-port-cov` | updates |
|---|---|---:|
| `gb3-ing` | `tools/cov/run.sh tools/cov/specs/geomblobv2_gb3.py <out> 1baa96c2,2735236b --frames 16000 --env MP2_DEBUG_ING=1 --driver 'MP2_CONSOLE={port} python3 tools/smoke/drivers/ing_con.py <out>/game.log'` | 16000 |
| `gb3-emission` | `COV_GDB_SCRIPT=tools/cov/cov_force.py tools/cov/force.sh tools/cov/specs/geomblobv2_gb3_force.py <out> 1baa96c2,2735236b --frames 8000 --fast` | 8000 |
| `gb3-helpers3` | same force command, `geomblobv2_gb3_helpers.py`, `--frames 2000 --fast` | 2000 |
| `gb3-actor` | same force command, `geomblobv2_gb3_actor.py`, `--frames 2400 --fast` | 2400 |
| `gb3-surface3` | same force command, `geomblobv2_gb3_surface.py`, `--frames 2400 --fast` | 2400 |

Merge these prefixes with `tools/cov/gcov_report.py build-port-cov <prefixes> --module GeomBlobV2`
(forced profiles first). The Ing driver observed an IngSpotPointPathFind -> BecomeCorporeal
transition and the killed Ing count dropping 2 -> 1 after six seconds.

The helper spec uses disposable copies 16 KiB below SP; it never changes live segment storage.
The actor spec restores active, area phase/occlusion, and VEL1 at function return. It also expires
the first/last strands, sets one post-creation count to zero, and enables one tracker's custom
light. The SPE spec is x86-64 SysV-specific: it redirects selected tracker Think calls to the
real SPE factory/methods on a separate aligned stack, restores the caller's SP on return, and
finally invokes the deleting destructor. The private SPE is never added to the world. Its own
particle generator enables ADV storage; `CreateNewParticles` allocates it normally. No source
probes, runtime stubs, or shared-code changes are involved.

### Marked functions and arms

`L` = executed/source lines; `A` = taken/non-exception gcov edges (inlined duplicates included).

| id | function | L; A | additional arm evidence |
|---|---|---|---|
| 0 | Simplify | 13/13; 12/12 | forced (`helpers3`): 0/1/2 segments, equal segment merge, independent direction/orientation mismatches |
| 1FC | Trace | 87/91; 57/60 | forced initial cache miss and self-neighbor; D1/D2 below cover remaining edges |
| 1220 | FindSharedEdge | 8/8; 8/8 | forced (`helpers3`) `sharedEdge=65535`, reaching -1; normal run returns 0/1/2 |
| 1608 | CastRay | 29/29; 17/18 | D3: accepted ray distance cannot exceed ray length |
| 1A4C | FindNearestTriangle | 35/35; 23/24 | D4: output pointer is always supplied |
| 1CC8 | AddSegment | 24/24; 12/12 | Ing/emission: negligible step, last/non-last, normal-aligned empty/nonempty path, reserve, append |
| 2074 | IsSameTriangle | 7/7; 10/10 | forced (`helpers3`): null A/B, index mismatch, each vertex mismatch, identical triangles |
| 2584 | CT AcceptScriptMsg | 18/18; 21/23 | forced custom light: create/forward/delete; ordinary no-light/default paths; D5 |
| 27DC | CT Think | 94/94; 53/54 | forced inactive, unloaded/occluded area, no VEL1, expired first/last, zero particle count; D6 |
| 4290 | CT ctor | 20/21; 19/19 | two real Ing trackers; missing base-initializer line is EH cleanup (D5) |
| 49A4, 49AC | SPE GetParticleGen, const/nonconst | each 1/1; 0/0 | forced (`surface3`), distinct exact symbols |
| 49E4 | SPE AcceptScriptMsg | 18/18; 21/23 | forced create with/without light, default Deactivate, delete twice; D5 |
| 5F48 | SPE ProjectOntoPlane | 5/5; 0/0 | called by the private SPE's emitted particles |
| 61C4 | BuildOrientationFromNormal | 11/11; 5/6 | natural normals plus forced +X fallback and +2X first clamp; D7 |
| 6518 | SPE PreRenderAllViewports | 12/12; 2/2 | forced private SPE before emission and after 294 Think calls |
| 6750 | SPE ctor | 14/15; 8/8 | real factory invocation; missing base-initializer line is EH cleanup (D5) |

### Dead-arm proofs

- **D1 — Trace crossed overrun:** base lines 198/199 are two gcov annotations of the same
  block-41 edge to block 42, not separate tests. `crossed` becomes true only at 187–191 after
  `0.0011920929 < hitDistance < remaining` (retail 0x6E4–0x724). `stepDistance=hitDistance`, then
  subtracting it cannot make remaining negative (0x728–0x734). Therefore the crossed branch
  inside `remaining<0` (0x780–0x7C0) is dead. The non-crossed truncation and crossed continuation ran.
- **D2 — Trace callback:** the sole retail call is CT Think at 0x3004; it constructs the optional
  invalid at 0x2F80 and passes it through unchanged. The only host call likewise passes a default
  optional. No callback writer/export exists. Consequently lines 213–215 / asm 0x854–0x884 are dead.
- **D3 — CastRay bound:** `CollisionUtil::RayTriangleIntersection` receives `distance=length`;
  its only successful store is `d=finalT` after rejecting `finalT<0 || finalT>=d`
  (`src/Collision/CollisionUtil.cpp:724–730`). An accepted finite distance is strictly shorter.
  A NaN fails the preceding `distance<nearestDistance` comparison. The final `distance<=length`
  rejection (gcov 473:5/6; retail 0x17B4–0x17BC) is therefore redundant for valid geometry/rays.
- **D4 — FindNearestTriangle out-pointer:** both call sites are in Trace and pass `&triangle`
  (retail 0x33C/0x348 and 0x930/0x948). There is no other caller. The null `triangle` arm of the
  tie-break condition (gcov 516:1/4; asm 0x1B60) is dead; `*triangle` may be null and both cases ran.
- **D5 — EH bookkeeping:** both message handlers' two unexecuted edges are block 22 -> 23/25
  (base CT line 642, SPE 407). Block 22 is entered only by edges labelled `throw` from the light
  construction, e.g. block 8 -> 22; these are compiler exception-cleanup guards, not retail
  arms. Both constructors' sole zero-count source line is the exceptional destruction of the
  constructed CEffect base. All non-exception constructor edges ran. Raw block IDs and throw
  flags are retained in `gb3-gcov.json`.
- **D6 — CT swap iterator:** the swap branch excludes `it+1==end`. For old count N, index
  `i<=N-2`; pop_back makes new end N-1, so `it!=end` remains true. Its false edge (692:1/4;
  retail 0x29A4/0x29A8) is dead. The subsequent swapped-strand finished/not-finished branches ran.
- **D7 — fallback normal clamp:** let `d=Limit(normal.x,1)`. Entering the fallback requires
  `|(1,0,0)-normal*d|<=FLT_EPSILON` (0x627C–0x6284). Thus `normal.x*d` is near 1, `|d|` near 1,
  and `|normal.z*d|<=epsilon`, implying `|normal.z|` near zero, far below 1 even allowing float
  rounding. The fallback's `Limit(Dot(Up,normal),1)` clamp (gcov 121:0/2; 0x62C8–0x62DC) cannot
  trigger. Non-finite input cannot satisfy the tangent-magnitude test.

### Asm check review

The all-tags checker reports 27 CT and 10 SPE findings, plus two SPE NOPOS notes; no MASK,
CROP, XBIT, XCMP, XSTORE or OFFS finding. Every reported call has an existing host implementation:

| retail ids / report | host equivalent / reason |
|---|---|
| 2350, 4728 | `rstl::vector::reserve` for segments/strands; Simplify's reserve is unreachable because it only shrinks, and upstream pops instead |
| C18, B70, A90 | optional triangle assignment, iterator assignment, STriangleSlot ctor in the existing headers |
| 1948, 1830 | inline CCollisionCache GetLeafBounds / NextTriangle |
| 2034, 22FC | CVector3f::Cross and inlined bitwise segment construction in push_back_unsafe |
| 467C, 3610, 3490, 31A4, 33A0 | implicit CEntityInfo dtor, strand pop_back/assignment/push_back/dtor |
| 21E0 from Think | SBlobStrand's existing default constructor constructs its CSurfacePath |
| OSGetTime, CRandom16 ctor; ctor CALLS | `mRandom(CStopwatch::GetGlobalMicros())`; initializer-list calls are omitted by the checker |
| floor; internal_dereference | CMath::FloorF wrapper; automatic rstl::string destruction |
| x154_40 in both ctors | `SetHighlightedInDarkVisor(true)` (retail 0x4530 / 0x68FC) |
| x176_80 / x176_40; NOPOS | SPE mHasRenderBounds / mFlag; explicit false ctor initialization and true/false PreRenderAllViewports stores (0x68E4, 0x68F0, 0x65A0, 0x6620) |

### Remaining slice

Unmarked: **4BEC SPE Think**, **5650 GatherCollisionSurfaces**, **5C34 GatherFromObbTree**,
**5D7C GatherFromBox**. The private SPE run covers Think 103/113 lines and 36/50 edges, gather
41/54 and 21/36; the two dynamic-geometry helpers were not called in this setup. Keep these
unmarked until particle transition/light edge cases and solid-box/OBB targets are forced.
`geomblobv2_gb3_surface.py` is a working factory driver for that continuation; no shared accessor
or console change is required. Existing bulk marks are outside gb3's new-marker claim.

Discarded setup attempts: `gb3-helpers` (inferior allocator call from a breakpoint stalled),
`gb3-helpers2` (reserved gdb.Breakpoint property name), `gb3-surface` (return breakpoint had to
be explicitly disabled). None contributed counters. Successful replacement specs use memory
writes / scheduled calls and complete normally. Large run directories are removed after retaining
the merged module counters and this evidence.
