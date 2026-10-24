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
- **ApplySafeZoneAvoidance 1C78** - the 0x1D04 (no zone) arm is the only one that runs: the five shipped
  BSWMs have no SAFZ in their area, so `SphereTouchingWhichSafeZone` never returns an id and 911/913/915-917
  stay at 0. Gated by shipped data, not forceable (the query is not a member write).
- **Render 262C** - untaken 742:1/2, 743:1/2 (`lights == 0`, the light mask is never 0 in a lit room),
  757:1/2, 763:3/4, 775:3/4, 781:1/2. The mActive-false arms are dead by construction (UpdatePartition
  links only active boids; KillBoid reactivates the same index in the same call), but 742/743 are not.
- **Think 3570** - untaken 470:1/2, 471:1/4 (the area is never occluded while the player stands in it;
  the driver's walk-out does not leave the area), 524:1/2 (`ShouldBuildAreaCollisionCacheForPartition`
  is `li r3,1; blr` at 13F4, so the false arm is dead), 529:1/2, 532:3/4, 542:1/2, 545:3/4, 577:1/2,
  578:1/2 (same mActive argument as Render) - the occlusion arms alone block the marker.
- **FindBestSurface 4CB0** - 317:1/2: the `kMT_AIPassthrough` skip never happens (no such surface in the
  probed boxes); not forceable.
- **CreateBoid 56DC** - 226:0/4, 226:2/4, 226:3/4: the waypoint arms need a Follow connection to a live
  CScriptWaypoint and the five shipped BSWMs have no waypoint connections (shipped-data dead, but no run
  took them); 232:1/4, 232:3/4 need `forceParticle == false` from KillBoid (which only runs on a boid box
  overlap). Not marked.
- **CBacteriaSwarm ctor 5F10** - 161:1/2 and 168:1/2: both `!= kInvalidAssetId` skips stay untaken because
  every shipped BSWM has valid particle ids; the two ids are stack arguments, so they cannot be forced
  from a member write. Not marked.
- **FindBestLockOnIndex B44** - 1158:1/3: the `!delta.CanBeNormalized()` arm needs a boid exactly at the
  camera position. Not marked.
