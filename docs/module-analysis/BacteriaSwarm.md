# BacteriaSwarm REL (module 6): analysis notes

CBacteriaSwarm (BSWM, in-game "Ingstorm"): a boid swarm in a box that patrols, chases the player and
dies on touch; it is not a CSwarmBasics. The module's code is upstream's
`src/MetroidPrime/Enemies/CBacteriaSwarm.cpp` with the loader `SLdrBacteriaSwarm.inc`. Retail asm:
`build/G2ME01/BacteriaSwarm/asm/MetroidPrime/Enemies/CBacteriaSwarm.s`. Per-function table:
`BacteriaSwarm_functions.tsv` (B upstream body, H weak/implicit copy, T REL runtime).

## Survey (bsw1, 2026-10-23)

`tools/upstream_hollow.py BacteriaSwarm`: nothing hollow. The whole retail object was diffed against a
dtk disasm of the matching build's object with the registers and stack offsets normalized.

| function(s) | verdict |
|---|---|
| Render fn_6_262C | **fixed**: retail calls `CGraphics::DisableAllLights()` after the boid loop (0x2980); upstream skipped it |
| LoadBacteriaSwarm fn_6_A0 | **fixed**: retail never copies the `active` property into `editorProperties.active` |
| SLdrBacteriaSwarm ctor | swsldr1's sound -1 defaults already in; the other defaults come from the out-of-line ctors |
| loader maxPursuitSoundTime -> maxPatrolSoundTime | retail does the same (f31 = 0x4e8): kept |
| ctor mThinkCounter = 0 (0x170) | retail leaves it uninitialized; kept (host heap is not zeroed) |
| ApplySteeringBehaviors jump table | matches: 0 door, 1 safe zone, 2 player, 3 bounds, 4 none, 5 separation, 6 cohesion |
| Think particle loop | null guard and unsigned compares equivalent |
| KillBoid, UpdateAllBoidMovement, UpdateBoid, ApplyBounds/SafeZoneAvoidance, Soft/HardwareLight, GetListAt, BoxForPosition, FindClosestCell, UpdateEffects | scheduling only |
| `destroy_elements` (rstl) | `lwz 0x0` vs `0x8`: rstl template, not module code |

## Shipped data (disc scan)

Five BSWMs (modules.tsv); every one has edActive 1, active 1, spawnInstantly 0, one connection
(DeathRattle -> SetToZero on a GENR), no waypoint connections, and no SAFZ in its area. So the
AreaLoaded spawn loop and GetWaypointForState's waypoint arms are data-dead, ApplySafeZoneAvoidance's
hit arms never run, and the loader fix has no shipped effect.

## Coverage (bsw1)

Coverage build, spec `tools/cov/specs/bacteriaswarm.py`, driver `tools/cov/specs/bacteriaswarm_temple.con`
(Temple Grounds dark 04_temple_unknownenemies_dark, BSWM e003d0023 L0: god-mode player in the swarm, every
beam, scan-visor orbit lock, morph + boost, a walk-out and back, Deactivate/Activate, Delete), and the
forcing spec `tools/cov/specs/bacteriaswarm_force.py` (same driver):

    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/bacteriaswarm.py <out> \
      3bfa3eff,04_temple_unknownenemies_dark --con tools/cov/specs/bacteriaswarm_temple.con --fast --frames 3200
    BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/bacteriaswarm_force.py <out2> (same args)
    python3 tools/cov/gcov_report.py build-port-cov <out>/gcov <out2>/gcov --module BacteriaSwarm

Marked (29, every line and arm taken, asm-checked): LoadBacteriaSwarm A0, IsBoidVisibleForLockOn E28,
GetOrbitPosition FE4, PlayBoidSound 10F0, RenderParticles 11BC, UpdateParticles 1200, KillBoid 12E0,
ShouldBuildAreaCollisionCacheForPartition 13F4, GetAreaCollisionCacheForPartition 13FC,
ApplyBoundsAvoidance 1E08, ApplyCohesion(list) 2240, RenderBoid 2A34, SoftwareLight 2A40, PreRender 2DBC,
BoxForPosition 2EA4, GetListAt 3064, MoveBoid 340C, UpdateSwarmAnimations 34C8, UpdateAllBoidMovement 34CC,
PointOnSurface 5290, ProjectPointToPlane 54C0, GetTouchBounds 5538, GetBoundingBox 5574,
PreRenderAllViewports 560C, CBoid ctor 6DB4; bac1: SLdrBacteriaSwarm ctor 8C0 and dtor 844,
UpdateBoid 4734, ApplySteeringBehaviors 4500.

| arm | how |
|---|---|
| ApplySteeringBehaviors: `if (mInSafeZone \|\| mTouchingSafeZone) break` (0x4568, 0x4570) | forced (`bacteriaswarm_force.py` `1E08 steering break bits`: the boid flags written at ApplyBoundsAvoidance, i.e. inside steering case 3, alternating mInSafeZone/mTouchingSafeZone; ApplySteeringBehaviors clears them on entry, so an entry write is lost) |
| UpdateBoid: mInSafeZone arms (364, 373) | forced (same entry: the flags are read after ApplySteeringBehaviors returns) |
| Think: creation loop 496/499/508 | forced (`3570 Think create all`: mMaxCreatedBoids 0 + mBoidGenCooldownTimer -1000 from the 601st Think) |
| KillBoid: no DeathRattle / no Dead message | forced (`bacteriaswarm_force.py`: deathRattleChance and deadChance 0; Touch passes 1.0, and `CRandom16::Float` reaches 1.0 only for Next() = 0xFFFF) |
| RenderParticles / UpdateParticles: no death generator | forced (`bacteriaswarm_force.py`: both generators nulled from the 2601st Think) |
| everything else marked | natural |

asmcheck findings on the marked bodies are tool limits: NOPOS/BIT on the CBoid ctor (no bitfield table:
`rlwimi r0,r30,22,0,9` is the 10-bit mIndex, the x81 clears are mActive..mPursuingPlayer), CALL to the
CRayCastResult copy (F88), Cross (5434), CCollisionSurface ctor (4C20) and unmarked AddParticle 1238,
XCALL ApplyCohesion (the list overload calls the point overload 2120), and the loader's SLdr typedef calls.

## Coverage (bac1)

Same build and driver as bsw1 (`bacteriaswarm.py` + `bacteriaswarm_temple.con`), one natural run and
one forcing run, merged (`gcov_report.py build-port-cov <nat>/gcov <force>/gcov --module BacteriaSwarm`):

    BUILD=$PWD/build-port-cov tools/cov/run.sh   tools/cov/specs/bacteriaswarm.py       /tmp/.../nat   3bfa3eff,04_temple_unknownenemies_dark --con tools/cov/specs/bacteriaswarm_temple.con --fast --frames 3200
    BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/bacteriaswarm_force.py /tmp/.../force (same args)

Both runs: `caught/reach-stub: 0`. The forcing run's `forced.log` shows 0 failed sets. Note the host
CBoid layout is not retail's (host mSpeed 0x80, the flag unit 0x89 vs retail 0x78/0x81), so the spec
forces through the member names, never retail's offsets.

## Unmarked behaviour bodies (why)

bac1 (14 unmarked rows, all asm-checked with `asmcheck.py check`; no CROP/XCMP/XSTORE/MASK/OFFS
finding on any of them, so no body changed):

- **Unreachable in retail** - no `bl` to them anywhere in the module asm: UpdateEffects 1448 (its only
  caller in the family would be UpdateSwarmAnimations 34C8, which is `blr` only), FindClosestCell 163C
  (called only from UpdateEffects, 0x1510), HardwareLight 2CB4. Not markable: no run can execute them.
- **ApplySafeZoneAvoidance 1C78** - bac1 reached only the no-zone arm. bsw2 closes the remaining arms
  with explicitly labelled callee-answer fixtures; see below (the old "not forceable" claim was wrong).
- **Render 262C** - untaken 742:1/2, 743:1/2 (`lights == 0`, the light mask is never 0 in a lit room),
  757:1/2, 763:3/4, 775:3/4, 781:1/2. The mActive-false arms are dead by construction (UpdatePartition
  links only active boids; KillBoid reactivates the same index in the same call), but 742/743 are not.
- **Think 3570** - untaken 470:1/2, 471:1/4 (the area is never occluded while the player stands in it;
  the driver's walk-out does not leave the area), 524:1/2 (`ShouldBuildAreaCollisionCacheForPartition`
  is `li r3,1; blr` at 13F4, so the false arm is dead), 529:1/2, 532:3/4, 542:1/2, 545:3/4, 577:1/2,
  578:1/2 (same mActive argument as Render) - the occlusion arms alone block the marker.
- **FindBestSurface 4CB0** - bac1 missed the AIPassthrough skip. bsw2 closes it with one modified
  returned triangle temporary, without changing the tree or disc data.
- **CreateBoid 56DC** - 226:0/4, 226:2/4, 226:3/4: the waypoint arms need a Follow connection to a live
  CScriptWaypoint and the five shipped BSWMs have no waypoint connections (shipped-data dead, but no run
  took them); 232:1/4, 232:3/4 need `forceParticle == false` from KillBoid (which only runs on a boid box
  overlap). Not marked.
- **CBacteriaSwarm ctor 5F10** - bac1 missed the invalid particle-id paths. bsw2 forces the actual
  by-value stack argument and referenced data member, plus invalid scan model; see below.
- **FindBestLockOnIndex B44** - bac1 missed camera-coincident boid geometry. bsw2 closes it using a
  real boid and the real first-person camera transform.

## bsw2: four remaining assigned bodies (2026-10-24)

Base `3b3cd4799cee99fdeac09bf58e8bc68f178b7a5f`, branch `decomp/bsw2`.
Only ctor 5F10, surface search 4CB0, safe-zone avoidance 1C78 and lock-on B44 were
annotated/marked. Full-file C++ token comparison against the base is identical after
stripping comments: no behaviour fix was needed. Excluded bodies and SwarmBasics were untouched.

### Per-function arm table

| function | complete evidence / outcome |
|---|---|
| **fn_6_B44 FindBestLockOnIndex — marked** | **21/21 lines, 14/14 branches**. Natural calls plus forced (`bacteriaswarm_bsw2.py` native queue): empty range, inactive boid, exact camera coincidence, far out of orbit range, camera-right perpendicular, behind camera, camera-forward winner. The real first-person camera matrix supplies all geometry; one real boid's transform/active flag and temporary vector count are restored after each call. Seven return-value oracles verify -1 or index 0. |
| **fn_6_1C78 ApplySafeZoneAvoidance — marked** | **14/14 lines, 10/10 branches**. Natural no-zone calls; forced callee-answer fixtures for invalid id, valid id/missing object, inside sphere, outside sphere and exact radius boundary. The typed safe-zone storage is used only by inline position/scale reads (no virtual call or destructor). Five independent oracles check touching/in-zone flags and all three components of the accumulated ahead vector. No fixture is claimed to be a shipped SAFZ. |
| **fn_6_4CB0 FindBestSurface — marked** | **23/23 lines, 16/16 branches**. Natural real collision-cache traversal covers cache/leaf/triangle loops, hit/miss, closer/not-closer, PointOnSurface true/false, output update and return. Forced one actual `GetTriangle(unsigned int)` return: only its stack temporary gets AIPassthrough bit 28. Logged flags **0x80000003 → 0x90000003**, then the one-shot hook disables itself. No collision-tree/disc mutation. |
| **fn_6_5F10 ctor — marked** | **79/79 lines, 40/46 branches**; all remaining successful-retail paths accounted for below. Natural valid scan/particle ids (four constructor calls); forced actual ctor entry (`BSW2_CTOR_SKIP=1`) invalidates death-particle reference member, boid-particle stack argument and referenced scan-model id (three calls). Finish hooks verify both generator pointers null and absent scan data, restore both reference arguments. The by-value stack argument expires normally; no production accessor/probe. |
| ctor two old-pointer-delete arms (measured source 172,179) | **Retail-dead by first-write proof.** Retail `0x62A0 li r6,0`, `0x63B4 stw r6,0x440(r15)`, `0x63C0 stw r6,0x458(r15)` clear both generator pointers. The first subsequent member writes are `0x666C stw r14,0x440` and `0x67D8 stw r14,0x458`; CElementGen construction receives its token/own storage, not this swarm. Thus tests at `0x6648..6650` and `0x67B4..67BC` cannot see an old generator. Host disassembly agrees: ctor+1309/+1374 zero host offsets 0x6c0/0x6f0; +2000/+2506 test them before the first assignment. Do not fabricate an old pointer merely to cover an impossible arm. |
| ctor four arms attributed to closing brace (measured line 182) | **Host EH cleanup, not retail-body arms.** These are the two optional-description validity tests at host ctor+10764/+10824, both sides counted by gcov in the cold unwind landing pad. The path destructs partially constructed members/base and ends at `_Unwind_Resume` (+10944). Retail `5F10..68AC` has ordinary initialization/RAII temporary cleanup and normal return, not this GCC landing pad. These four host-only exception arms are not falsely labelled runtime-covered or shipped gameplay paths. |

### Retail structural check

- **B44:** initial dot=0.5/result=-1, squared orbit limit (`0xBA0..BC4`), real FP camera
  (`0xBC8..BEC`), active byte 0x81 mask **0x20** (`0xBF8 extrwi ...,1,26`), strict
  distance `>` (`0xC3C/40`), normalization guard (`0xC48/50`), strict best-dot `>`
  (`0xC78/7C`) and winner updates (`0xC80/84`). No visibility ray was added; ties retain
  the earlier index. The vector-loop tail matches `0xC88..CA4`.
- **1C78:** sphere uses boid translation and `mTouchRadius` (`0x1CB4..1CEC`); invalid
  id and null object guards (`0x1D00/04`, `0x1D18/1C`); static cast as in retail.
  Touching mask **0x04** (`0x1D28 rlwimi ...,2,29,29`), in-zone mask **0x08**
  (`0x1D7C rlwimi ...,3,28,28`), strict squared-distance `< radius-X²` (`0x1D70/74`),
  then unconditional additive normalized escape force (`0x1D84..1DCC`). No false-arm flag clear.
- **4CB0:** radius initializes sphere and best distance (`0x4CD0..4D08`), intersecting
  leaves only (`0x4D24/2C`), actual triangles (`0x4D60`), mask from `0x4D48 lis r31,0x1000`
  = **0x10000000**, low u64 word (`0x4D70..4D90`), not 0x1000. Absolute plane height,
  strict closest-distance comparison and PointOnSurface gate (`0x4D94..4E04`); sphere,
  output, best distance and found updated together (`0x4E08..4E9C`). Three nested loop tails match.
- **5F10:** checked initial actor/material and scan-model selection (`0x5FB0..61D4`),
  data-derived members/defaults and partition loop (`0x61D8..6450`), second scan-model
  choice (`0x6454..64DC`), flag byte (`0x64E0..6530`) and both particle paths
  (`0x64EC..6800`). Flag masks are 0x80 blending=false, 0x40 spawn argument, 0x20 unknown
  argument, 0x10 patrolling=false, 0x08 pursuing=false. The already documented host
  `mThinkCounter=0` initialization is retained, not reclassified as a new divergence.

Full asmcheck for **each of these four**: RC1, seven completeness heuristics plus four
NOPOS advisories. The latter's bit positions are derived above. CALLs: `6D94` is
CModelData::None, `6B34` its copy constructor, CAABox is the member initializer,
CToken::Lock is in TLockedToken construction, `68AC` is optional assignment, `4EFC`
is the implicit surface copy. The ctor BRANCH finding (retail 24 conditions vs source 2)
is initializer choices, partition/temporary/ownership expansion; the measured host branches
and two retail-dead old-pointer tests above account for it. Correctness tags
MASK/XBIT/CROP/XCMP/XSTORE/OFFS: RC0, no findings. No actual divergence found.

### Reproduction and evidence

```
tools/games_running.sh --wait
COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py BUILD=$PWD/build-port-cov SECS=600 \
  tools/cov/force.sh tools/cov/specs/bacteriaswarm_bsw2.py <new-main-dir> \
  3bfa3eff,04_temple_unknownenemies_dark --con tools/cov/specs/bacteriaswarm_bsw2.con --fast --frames 1000
tools/games_running.sh --wait
BSW2_CTOR_SKIP=1 COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py BUILD=$PWD/build-port-cov SECS=600 \
  tools/cov/force.sh tools/cov/specs/bacteriaswarm_bsw2.py <new-ctor-dir> \
  3bfa3eff,04_temple_unknownenemies_dark --fast --frames 600
python3 tools/cov/gcov_report.py build-port-cov <new-main-dir>/gcov <new-ctor-dir>/gcov \
  --module BacteriaSwarm --fn fn_6_B44 --fn fn_6_1C78 --fn fn_6_4CB0 --fn fn_6_5F10
```

Final runs `/tmp/opencode/bsw2-main4` (1000 frames, 14 queue scenarios / 12 return-store
oracles / one triangle mutation) and `/tmp/opencode/bsw2-ctor3` (600 frames, three actual
argument fixtures/restores) both have normal inferior exits, zero caught/reach-stub,
no Python/fixture errors. `/tmp/opencode/reports/bsw2-evidence` preserves **matching gcno,
measured source, raw JSON, binary/source hashes and ctor disassembly** before marker rebuild;
the marker-only replacement changes no line numbers, but old profiles must use that gcno.
Coverage summary `/tmp/opencode/bsw2-coverage2.txt`.

All exploratory directories retained. `main1/ctor1` had an early harness assertion on a
duplicate cold ctor symbol; `main2` timed out from an over-broad per-triangle breakpoint
(debugger overhead, no production fix), and `main3`'s one-shot caller-name filter missed
the qualified name without parameters. They are **not** final closure evidence. `ctor2`
successfully checked particle skips, but final ctor3 also covers both scan-model choices.

Final gates/logs: `/tmp/opencode/bsw2-{link-build,cov-build,manifest,strip-diff,matching-configure,
matching-build,matching-sha1,asmcheck-all,asmcheck-correctness,review,runtime-audit}.log`.
Host/coverage builds, manifest, strip-diff token identity, matching build and all 87 SHA1
entries, review and runtime audit RC0; full asmcheck RC1 explained above. No whole-family
smoke or excluded-body changes. Pre-existing configure log and all prior evidence preserved.
Review's four MARK lines are the four bodies justified here; its DEL entries are existing
lines with appended asm-offset comments, not deleted C++ tokens (strip-diff verifies that).

## bsw3: Render, Think and CreateBoid (2026-10-24)

Merged base **8135223007b0f9edab12181bac9ebc7115295d0b** = bsw2
`e697972461e82ef8fdf1617401805c7204f34874` plus integration `b6390eb78`.
bsw2 is preserved unchanged. Only these three bodies gain comments/markers; full-file
C++ token comparison with this base is identical. No gameplay change was justified.
UpdateEffects, FindClosestCell and HardwareLight remain unmarked.

### Per-body arms (line numbers are the archived measured.cpp, before comments)

| body | measured run f | accounting / outcome |
|---|---|---|
| **fn_6_56DC CreateBoid — marked** | **26/26 lines, 16/16 branches** | Natural random sign choices and creation; forced actual waypoint-result fixtures cover null, real inactive waypoint and real active waypoint. Four full-transform equality oracles pass for the latter. A separate native call with a real boid and `forceParticle=false` covers the final short-circuit edge and checks active, zero velocity and patrol speed. Actual boid bytes and manager RNG are restored exactly after that direct call. |
| **fn_6_262C Render — marked** | **42/42 lines, 38/42 branches** | Forced (`tools/cov/specs/bacteriaswarm_bsw3.py`): light mask 0/1; actual alpha callee result 64/-1; absent generator; one-real-boid consistent outlier list, frustum false/true and refresh counter alternatives. Natural populated partition loops and both parity alternatives. Four missing active-false edges (773,779,791,797) are structurally dead below. |
| **fn_6_3570 Think — marked** | **105/105 lines, 93/100 branch rows** | Forced same spec: inactive; occluded timeout zero, crossing zero, positive/throttle and positive/proceed; actual area phase not Loaded with expired timeout; empty creation search; first inactive creation; absent particle generator; true outlier geometry and alternating parity; scan/combat/transitioning visor; finite creation cap. All sound-condition alternatives ran without a sound-answer hook. Seven missing gcov rows describe six distinct edges: cache false (540), linked inactive (545,548,558,561), prefix inactive (593 and 594 duplicate the same block 82→98). Proofs below, not observations-as-deadness. |

### Structural dead-arm proofs and boundaries

1. **Linked boids remain active.** The only false writers are CBoid construction and
   KillBoid; the only true writer is CreateBoid. `AcceptScriptMsg(Create)` constructs
   the fixed vector, all initially inactive. `UpdatePartition` clears every head,
   traverses the actual vector and links **only active** elements (`0x3304` test;
   source UpdatePartition). Neither SoftwareLight/RenderBoid nor the bounds/cache
   operations, steering, animation or movement callees deactivate a boid. The only
   KillBoid caller is Touch's already-active boid path. KillBoid sets false, draws,
   sends messages, then unconditionally calls CreateBoid on the **same index** with
   false (`0x12E0..0x13F4`); CreateBoid sets true before returning, regardless of
   waypoint/generator status. Thus at Think/Render scheduling boundaries no linked
   inactive element exists. Think rebuilds lists before its loops; Render starts
   from those lists (or initially null heads). Render's 797 false edge additionally
   contradicts the active test dominating it at 791, with only geometry/light reads
   between them. This proof concerns a valid object's ordinary lifecycle, not an
   arbitrary debugger write, repeated synthetic Create or a dangling/deleted object.
2. **No deferred-message shortcut.** `CStateManager::SendScriptMsg` CAN dispatch
   synchronously when its queue exceeds 128 (CStateManager.cpp:3632–3645). The proof
   does not claim otherwise: message dispatch invokes AcceptScriptMsg, not the
   frame's Think/Render scheduler. CBacteriaSwarm's script handler has no Think or
   Render call and never sets an existing boid false. Its AreaLoaded path also only
   creates inactive entries. The false interval inside Touch/KillBoid is not a frame
   scheduling boundary. Think's own update callees do not invoke Touch/KillBoid;
   rendering's light/graphics calls do not invoke gameplay collision callbacks.
3. **Particle-prefix proof uses the actual counter, not vector size.** Let K be the
   length of the active boid prefix at a scheduling boundary. Both creation callers
   (`Think` first-inactive search and `AreaLoaded` ascending loop) extend that prefix;
   completed kills preserve it. Each ordinary first activation via CreateBoid(...,true)
   adds one to K; kills use false and do not themselves add a root particle. The root generator
   ctor initializes `mActiveParticleCount=0`, only reserves storage, and swarm ctor
   disables emission. This swarm only updates its **death** generator; its root boid
   generator is accessed by ctor/CreateBoid/Think/Render and is not globally registered
   for independent Update. Root creation is only ForceParticleCreation(1).
   `CreateNewParticles` clamps downward and appends at most one; a particle with
   endFrame -1 is popped without increment, otherwise active count increments once
   (CElementGen.cpp:750–857). Thus while K<N, activeParticleCount≤K. An AreaLoaded
   handler (including any synchronous dispatch during a kill) either does nothing
   when !mSpawnInstantly, or scans all slots and leaves **K=N**; in the latter case
   duplicate activation of a transiently inactive slot could add another particle,
   but min(activeParticleCount,N)≤K still holds. Once K=N, completed kills preserve
   K=N, so this weaker, sufficient invariant remains true even if the root count
   exceeds N. This avoids assuming every force=true call is a first activation.
   For completeness,
   all other counter writers are ctor zero, DestroyParticles zero and
   UpdateExistingParticles zero plus one increment per retained root particle
   (155,666,700,854,1115); no writer counts children. GetParticleCount returns this
   counter (header:125; retail `0x3E1C lwz r3,0x254(r3)`), not mParticles.size().
   Render never creates root particles. Thus every i<min(activeParticleCount,N) is
   within the active prefix; false at retail `0x3C5C` is impossible. gcov lines
   593/594 repeat one source/destination edge, not two independent checks; native
   Think+1831/+1838 has the single active test/skip.
4. **Other retail-only checks.** The repeated actor-active test (`0x35DC`) cannot
   become false after the first succeeds: only two local flag clears intervene;
   GCC folds it. `ShouldBuildAreaCollisionCacheForPartition` is `li r3,1; blr`
   (`0x13F4`), so its false edge at `0x3900` is dead for this concrete class.
   Retail's particle-address null check `0x3C68/0x3C6C` is also dead: positive root
   active count requires successful append and real storage; i is in the bounded
   prefix. The host reference has no separate nullable particle check. No fabricated
   null vector, inactive linked element or branch instruction forcing was used.

### Retail audit

- **56DC:** GetBoundingBox/center/extent; three strict >0.5 comparisons with ±1
  (`0x57D8/0x5804/0x5830`), z,y,x random draws and independent half-extent products;
  rotation draw×360 then degrees conversion; Patrol 0x5054524C; cast/null/active
  checks and whole transform selection (`0x5920..0x597C`); active mask 0x20,
  zero velocity and patrol speed stores (`0x5980..0x59CC`); generator/force AND and
  one particle (`0x59D0..0x5A20`). No change to random order or equality semantics.
- **262C:** alpha !=-1 paired enable/disable; channel mask selects NONE vs CLAMP/SPOT;
  fixed 5³/strides 5 and 25; low-two-bit software-light refresh, active 0x20 and
  frustum 0x10, Lerp 0.3; outlier increment precedes refresh; RenderBoid vtable 0x88.
  Existing TARGET_PC DisableAllLights matches `0x2980` and remains unchanged.
- **3570:** both active guards, patrol/pursuit clear masks 0x10/0x08, cooldown and
  counter writes, area phase 16/visible 1; strict >0 decrement, <=0 exit, mask 2
  throttle, visible reset 7; both visor enums 2, index -1 and material ids 40/41;
  creation limit/cooldown predicates and failed-search break; both partition/outlier
  count/parity/active paths; margin 1.5+(0.5+radius); timer >= threshold and
  min+(max-min)*R; root count minimum, color/position writes; patrol/pursuit sound
  conditions use timer ==0, player-pursuing mask 0x02 and random <= weight. Retail
  `cror eq,lt,eq`/`cror eq,gt,eq` shapes retain unordered handling; no comparison fix.

Full asmcheck: **13 completeness findings + two NOPOS advisories**, no correctness
findings. Create's TCastToPtr name is represented by TCastToConstPtr. Think's 3E24/
4290 CALLs are cache copy/destruction; CreateBoid/ShouldBuild/UpdateBoid/animation/
movement and RenderBoid XCALLs are retail vtable calls (slots 0x7C,0x94,0x80,0x8C,
0x90,0x88 respectively). `x173_01/02` BITs are integer counter masks 1/2/3, not
missing boolean members. Patrol/pursuit NOPOS positions are decoded above.

### Reproduction, rollback checks and gates

```
tools/games_running.sh --wait
BUILD=$PWD/build-port-cov SECS=600 tools/cov/force.sh \
  tools/cov/specs/bacteriaswarm_bsw3.py <new-dir> \
  3bfa3eff,04_temple_unknownenemies_dark --con tools/drivers/bsw3.con --fast --frames 1200
```

Use default force.py, **not force_ret.py**. All custom fixtures use actual native
entries and real live actor storage. Every original is read before mutation, the
FinishBreakpoint is armed before writes, and restores are read back. The direct
Create call runs outside stop() through force.sh's hook-backtrace, protected by
try/finally; only boid/RNG are touched, force=false leaves generator unchanged.
Native entry symbols, not stale line/retail offsets, locate the bodies. The primary
swarm is guarded by editor id **003d0023**. The waypoint is verified dynamically as
a real CScriptWaypoint (uid 0410); no heap actor is fabricated. Callee return-result
fixtures change only the real return register, which is overwritten by normal calls.

Final run `/tmp/opencode/bsw3-f`: normal exit, 1200 frames, zero caught/reach-stub,
no Python/fixture errors; 15 Think phases, 32 Render fixtures, 12 waypoint fixtures,
four transform oracles and one direct-output oracle; all 60 transaction restores
verified (59 FinishBreakpoint plus direct). **317 original/readback field pairs**
are logged as actual bytes/logical bit values and independently equal. No pending
rollback at exit. The per-body table above is run f (run e had identical totals).
`/tmp/opencode/bsw3-fixture-test.py` additionally passes mocked normal return,
oracle failure, out-of-scope/unwind, mutation error and inferior-call error restore
tests. These mocks are safety checks, not runtime-arm evidence.

`/tmp/opencode/reports/bsw3-evidence/{a,b,d,e,f}` preserves matching gcno/gcda,
raw gcov JSON and measured source; d/e additionally preserve binary/source hashes,
fixtures and driver (e/f include particle writer sources). All profiles refer to
the pre-comment coverage executable; e/f were collected without rebuilding that
executable after comments. The old measured.cpp, not current line numbers, is
authoritative. Run a targeted the adjacent area-62 swarm and had no Render fixtures;
c stopped at the debugger queue because internal-breakpoint command lists did not
run (no native call/no closure claim); b and d exited normally. All exploratory
evidence and the pre-existing configure log remain intact.

Gates `/tmp/opencode/bsw3-{link-build,manifest,matching-configure,matching-build,
matching-sha1,strip-diff,asmcheck-all,asmcheck-correctness,fixture-test,runtime-audit,review}.log`:
host link and manifest pass; matching configure/build pass; config/G2ME01/build.sha1
verifies **87/87** artifacts; whole-file C++ token identity/strip-diff passes;
correctness asmcheck RC0, full asmcheck RC1 explained above. Review's three MARK
lines are precisely these bodies; DEL entries are two inline-comment additions,
not removed tokens. No broad smoke or other-family rebuild/run was requested.
