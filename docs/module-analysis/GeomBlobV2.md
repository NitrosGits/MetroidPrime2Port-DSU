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

## Coverage (gb4)

2026-10-24, base `4a6d20f9e`: **the four remaining SPE bodies are marked**. Each has
every source line and every non-exception gcov edge executed; no dead-arm exemptions.
The four bodies were rechecked instruction-by-instruction against retail. No behavioural
discrepancy was found, so the production diff is four marker comments only.

### Reproduction

```sh
T=$PWD/../MetroidPrimePort/build/review-tools/bin
$T/cmake -S . -B build-port-cov -DMP_PORT_COVERAGE=ON \
  -DCMAKE_PROJECT_metroid_prime2_port_INCLUDE=$PWD/tools/cov/specs/geomblobv2_gb4_keep_static.cmake
$T/ninja -C build-port-cov metroid_prime2_port
tools/games_running.sh --wait
BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/cov_force.py SECS=180 \
  tools/cov/force.sh tools/cov/specs/geomblobv2_gb4.py /tmp/opencode/gb4-verified \
  1baa96c2,2735236b --frames 2400 --fast
python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/gb4-verified/gcov \
  --module GeomBlobV2 --fn fn_25_4BEC,fn_25_5650,fn_25_5C34,fn_25_5D7C
```

The final run used the **post-marker** coverage binary, exited normally after 2400 updates,
and logged zero caught/reach-stub/Python errors plus `gb4 complete; private SPE destroyed`.
All assertions passed. `run.sh`'s rc alone is insufficient: a debugger stop can still return 0.
Retained evidence: `/tmp/opencode/reports/gb4-{gcov.json,coverage.txt,runtime.txt,asmcheck.txt,review.txt}`;
report `/tmp/opencode/reports/gb4.md`. The JSON preserves counters, CFG block ids and throw flags.

The x86-64 SysV spec reuses gb3's private factory and separate-stack scheduler. The gb3 driver
now has a library-only switch; its normal invocation still installs its original breakpoints.
The coverage-only CMake include retains static helper copies with GCC `-fkeep-static-functions`;
the normal link uses its ordinary flags. All hooks select symbols, so marker line shifts are safe.
No production probes or helper exports were added.

### Arms and assertions

All cases below are **forced** by `geomblobv2_gb4.py` using the command above. The private SPE
first runs 300 Think calls (one zero-rate early-out, 299 ordinary emission/update calls).
Scratch geometry is above the scheduled callee's stack and is never retained by game objects.

| retail id / function | lines; edges | cases |
|---|---|---|
| 4BEC Think | **113/113; 50/50** | zero-rate early return; ordinary spawning and bounds; capacity clamp; forced particle cases 0–12 below; custom light present/absent, active/inactive, non-light ID |
| 5650 GatherCollisionSurfaces | **54/54; 36/36** | real static cache traversal, duplicates, overlap rejection/acceptance, full capacity; dynamic cases 0–11 below |
| 5C34 GatherFromObbTree | **12/12; 11/11** | geometry cases 5–9: zero triangles, no overlap, excluded material, append at 255 then full, initially full; dynamic case 3 also gathers both triangles through a group |
| 5D7C GatherFromBox | **9/9; 6/6** | geometry cases 1–4: all 12 triangles, no overlap, append at 255 then full, initially full |

The helpers assert surface/ID counts and `0xA4000000 | triangleIndex`. The OBB fixture is a
nondegenerate triangle repeated twice in a controlled collision-array view, using the actual
`GetTriangle`/transform/filter/overlap implementations. Box geometry is the unit box.

| Think case | input and observed result |
|---|---|
| 0 | changed surface, tangent velocity, aligned previous normal: attach to ID 123, preserve lifetime |
| 1 | normal velocity: stop and detach; shorten lifetime by 11 |
| 2 | tangent velocity but perpendicular previous normal: stop and detach |
| 3 | previous-normal dot 0.8: project velocity and change target orientation |
| 4 | already on ID 123: skip migration |
| 5 | no gathered surfaces: detach |
| 6 | already detached, long lifetime: shorten by 11 |
| 7 | already detached with end frame -1: leave sentinel unchanged |
| 8 | already detached, short lifetime: new end precedes current frame, set -1 |
| 9 / 10 | phase mismatch with unequal/equal orientations: interpolate / skip |
| 11 | phase match with unequal orientations: commit target; resulting perpendicular normal detaches |
| 12 | spawn into an empty surface set: mark newborns expired; actual Update removes them, empty post-update path |

Cases 0–11 use one real allocated particle, with ADV/parent/particle counts kept consistent;
the spec writes surface-state inputs after the generator's real Update and asserts resulting
surface ids, fade end frames, and zero velocity on stop cases. The last five cases repeat
0–4 while exercising the light branches. An initial 128-particle forcing attempt tripped the
watchdog while Python edited all records; reducing this private fixture to one avoids that.

| Gather case | input and observed result |
|---|---|
| 0 / 11 | broad real static bounds with initially empty/full output: 256 surfaces |
| 1 | near-list id equals ignored id: skip |
| 2 | platform-only flag on ordinary ScriptActor: reject |
| 3 | platform predicate accepted; private OBB group with one tree: two surfaces |
| 4 | OBB group with zero trees: none |
| 5 / 6 | solid ScriptActor with AABox primitive, active/inactive: 12 surfaces / absent touch bounds |
| 7 | nonphysics tracker without Solid or Water: reject |
| 8 | Water predicate accepted, tracker touch bounds absent: none |
| 9 | invalid near id: null actor, skip |
| 10 | nonphysics tracker with Solid but absent touch bounds: none |

Dynamic fixtures replace the near-list result after the real builder; cases 1–10 then clear
the static output to isolate dynamic results. Cases 3/4 return a private OBB primitive with
the real vtable and a controlled group/tree view. Cases 5/6 return the actor's embedded AABox.
**Platform and Water success cases force only the cast return used as a boolean predicate**;
these are caller-arm tests, not evidence of a naturally spawned platform/water asset.
Real actor active/material mutations are restored at call return, including assertion failure.
Dynamic counts and `(uid << 22) | triangleIndex` are asserted for OBB and box outputs.

### Retail audit anchors

- **Think:** 0x4C90–0x4CB4 uses min(wanted, MAXP-existing), with the both-zero branch landing at
  the epilogue; 0x4D54 forces creation before bounds/gather; 0x4EC0/0x5038 handles no spawn surface.
  At 0x50EC/0x50F0 the start-frame phase is `&7`; 0x5100–0x5140 commits changed orientation.
  0x517C skips an unchanged surface. 0x5294/0x5298 is `abs(normalDot/speed)>0.95`;
  0x52A4/0x52A8 is `previousDot<0.3`; 0x5320/0x5324 is the `<0.995` orientation threshold.
  These ordered comparisons retain the PPC NaN branch shape. 0x53AC masks the phase delta by 7,
  0x53E0 computes `1-0.125*delta`; 0x5444 is signed fade division, with rodata 0x90
  `{11,11,11,5,0}` and 0x5474/0x5478's strict expiry test. 0x54D0 masks the full frame word
  by 7, and 0x54EC–0x554C performs the light-id/cast/active checks in source order.
- **GatherCollisionSurfaces:** center/half extents use rodata 0xD0=0.5; 0x5720 shifts area id
  by 16; 0x5750–0x5768 performs duplicate suppression. 0x57AC tests equality with 256 and
  0x57F8 skips the rest of the current leaf cache (not the dynamic tail). The five material
  exclusion shifts at 0x5834–0x58C0 use data words `{33,32,35,20,53}`. 0x591C ignores the
  selected id; 0x593C tests mFlag=0x40; 0x5958 shifts uid by 22. 0x5988/0x598C tests
  `'OBTG'`; tree-loop calls retain the actor's primitive transform. 0x5A1C masks Solid bit 59
  (`0x08000000` in the high word), with Water fallback and optional bounds at 0x5A44/0x5AAC.
- **GatherFromObbTree:** 0x5C64–0x5C74 builds `1ull << 20` (ProjectilePassthrough), not a mask
  of 20. 0x5CB8 transforms each reconstructed triangle; 0x5CE8/0x5D08 reject material/overlap.
  0x5D10 tests count==256; 0x5D30 ORs id bits with the signed-short index; 0x5D5C explicitly
  sign-extends that index before comparison, matching the source's `short` loop variable.
- **GatherFromBox:** 0x5DC8/0x5DCC select face `i/2` and vertex `(i&1)*2`, 0x5DF0/0x5DF4
  supply both -1 flag words, 0x5E18 rejects overlap, 0x5E20 tests count==256, 0x5E40 ORs the
  index, and 0x5E6C terminates at 12.

### All-tags checker review

The final SPE-only all-tags check has **57 findings + 3 NOPOS notes**; no
MASK/CROP/XBIT/XCMP/XSTORE/OFFS finding. All are resolved as follows:

- The 29 BIT entries at Think 0x54D0 are a full-word `mFrameCounter &= 7`, not 29 missing
  boolean stores. Its `floor` CALL is `CMath::FloorF`.
- Gather's x176_40 BIT is the named `mFlag`. CTri copy (5EDC), CCollisionSurface constructor
  (5E88), CCollisionSurface copy (B14), and bitwise_copy<6> (6918) are implicit/header code
  used by the real helper operations and POD pushes. Cache destructor 5B08 is automatic.
- Existing ctor findings: named mHasRenderBounds/mFlag initializers, highlighted-dark-visor
  setter, CStopwatch::GetGlobalMicros/CRandom16 construction, and implicit CEntityInfo destruction;
  the checker omits initializer-list calls. Destructor Free/BRANCH findings are compiler-emitted
  member/base destruction and deleting-destructor logic.
- BuildOrientation's 2034 is CVector3f::Cross; message-handler internal_dereference is string
  destruction. PreRender x150_01 is Get/SetPreRenderClipped (header mOutOfFrustum), x176_80 is
  mHasRenderBounds; that same flag explains AddToRenderer and PreRenderAllViewports BITs.
  AddToRenderer's XCALL Render is the virtual call through vtable offset 0x2c at 0x64A4/0x64AC.
  SetFlag's x176_40 is mFlag. The three NOPOS notes name those already-audited bitfields.

### Gates and scope

Normal host link and post-marker coverage link: **0 FAILED**. Generator `--check`: **rc 0**.
`configure.py && ninja` passed; explicit `dtk shasum -q -c config/G2ME01/build.sha1`:
**87 files OK**. Removing marker comments makes the entire source byte-identical to base
(both TARGET_PC and non-PC). Spec syntax and `git diff --check` passed. No additional B
functions were newly marked; existing bulk tags and shared/header bodies retain their status.
Only this SPE/caller-area run was needed; no unrelated smoke or module sweep.

## gb5 checkpoint: explicit inline helpers (2026-10-24)

Base `49261294b`. Inventory compared TSV classB rows with actual source/header markers,
not the stale TSV marker column:19 remaining. **Eight explicit inline bodies marked,
62 → 70/81**. No production logic or class/layout/classification changes; only eight
marker comments in five headers. Shared-header edits are marker-only.

### Matched-body verification and retail audit

All19 candidates are **100.0% matching in the freshly rebuilt upstream report**, with
exact module25 virtual offsets and original symbols. The source unit's `address` is
unit-relative: for SPE helpers use `metadata.virtual_address` (e.g.21908=5594), not
address3308. Saved binding records and report SHA256:
`/tmp/opencode/reports/gb5-evidence/matching-final.json`.

Per DECOMP_WORKFLOW's matched-body rule, 100% matches require no separate all-arm gcov
run. This exception is explicit: the ordinary caller-area run below is not claimed to
cover every arm of these header bodies. Each marked body also received a full instruction
audit using `asmcheck.py annotate GeomBlobV2 0x<off> --all-ins`.

| id | canonical explicit definition | instruction audit |
|---|---|---|
| A90 | `Enemies/CCollisionTracker.hpp`,STriangleSlot ctor | AA4 calls recordAD4; AA8–AB8 constructs plane at38 from vertices0/C/18 of the copied surface; returns this. Existing mTriangle initializer followed by CPlane(v0,v1,v2). |
| AD4 | same header,STriangleRecord ctor | AE4 preserves ushort index; AF0 invokes implicit surface copyB14; AF4 stores index at30. Existing mSurface/mIndex initialization, no branch. B14 already exists as a native implicit copy, not a stub. |
| 1830 | `WorldFormat/CCollisionCache.hpp`,**nonconst** NextTriangle overload | 184C–1854 trianglesRemaining guard; 1858–188C offset into ushort data,remaining−1,leafExhausted80=(remaining==0),offset+=24 halfwords; 1890–18F0 statusZero40 gates plane construction/copy and leafTriangleCount−1,last writes ushort status1; 18FC–192C empty triangles: leaves0→ReadGeometry,leaves−1,ReadLeaf,recurse. Host uses native sizeof(slot)/sizeof(ushort), matching its cache writer's layout. Const overload is a different body and is not marked1830. |
| 1948 | same header,GetLeafBounds | 1964–1974 if triangles remain,return leaf bounds; 1978–19A8 leaves0→ReadGeometry,leaves−1,ReadLeaf,retest; 19B4–1A24 repeats; 1A28–1A30 recurse. PPC compiler unrolls recursive iterations; source's single recursive step is byte-matching. |
| 23E8 | `MetroidPrime/CActor.hpp`,GetHealthInfo const | 23F4–2400 vtable slot38 invokes nonconst HealthInfo; 2404–2410 returns its pointer. Host const_cast then virtual HealthInfo is the exact forwarding body. |
| 3488 | `Kyoto/Particles/CElementGen.hpp`,GetParticleCount | 3488 lwz254;348C return. Existing mActiveParticleCount getter. |
| 461C | `MetroidPrime/CEffect.hpp`,explicit empty virtual dtor | 4634–4638 nullable MW wrapper;463C–4648 derived vtable;464C base CActor destruction;4650–465C signed-short deletion flag>0 frees. Host empty derived destructor implicitly runs base and native D0/D2 own the deletion ABI. No null-this call is legal native C++. |
| 5594 | `Kyoto/Particles/CElementGen.hpp`,GetEmitterTime const | 5594 lwz68;5598 return. Existing mCurFrame getter. |

asmcheck all-tags reports MISSING for all eight because its definition parser does not
recognize these unqualified inline class definitions. Literal markers, explicit source
bodies, full instruction audits and exact-symbol 100% report records establish identity;
no masked correctness discrepancy is being waived.

### Caller-area run / gates

```sh
tools/games_running.sh --wait
BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/cov_force.py SECS=300 \
  tools/cov/force.sh tools/cov/specs/geomblobv2_gb3_force.py <out> \
  1baa96c2,2735236b --frames 1600 --fast
```

Reuses gb3's real Ing/CT emission fixture; no new runtime probes or fixture modifications.
Post-marker binary exited normally at1600 frames,caught/reach-stub0,force/Python errors0.
Only affected module/caller area tested; no unchanged-family/whole-module sweep.
Host/coverage links rc0,0 FAILED;gen_files_cmake --check rc0;upstream configure+ninja rc0,
All77.98% fuzzy/54.39% matched,Game94.14%/63.29%;explicit dtk shasum87 files OK.
All five headers strip-diff CLEAN (marker-only); full evidence retained under
`/tmp/opencode/reports/gb5-evidence/`.
`tools/review_decomp_diff.sh 49261294b`:8 MARK, no other findings; each audited above.

### Eleven implicit bodies deferred: annotation/ownership decision required

These are already implemented by native compiler-generated special members, and all
eleven match100% in the upstream report. They have **no explicit function definition**
above which to place a truthful marker. Do not attach markers to class declarations or
add no-op implementations merely to change the progress count. No B/H reclassification
was made in this lane. The orchestrator should choose the project's representation for
implicit bodies; making shared engine definitions explicit exceeds a marker-only lane edit.

| ids | existing implicit definition site |
|---|---|
| B14 CCollisionSurface copy ctor | `include/WorldFormat/CCollisionSurface.hpp`,class |
| 3224/3490 SBlobStrand copy ctor/assignment;33A0 dtor | `include/MetroidPrime/Enemies/CCollisionTracker.hpp`,struct |
| 32A0/350C CSurfacePath copy ctor/assignment;33F8 dtor | same module header,class |
| 4094 CCollisionCache dtor | `include/WorldFormat/CCollisionCache.hpp`,class |
| 467C CEntityInfo dtor | `include/MetroidPrime/CEntityInfo.hpp`,class |
| 5B08 CAreaCollisionCache dtor | `include/WorldFormat/CMetroidAreaCollider.hpp`,class |
| 5EDC CTri copy ctor | `include/Kyoto/Math/CTri.hpp`,class |

Full instruction listings for all19 candidates are retained. No missing gameplay body,
engine defect or new accessor requirement was found; the residual work is source-marker
representation/ownership, not an undiscovered gameplay algorithm.
