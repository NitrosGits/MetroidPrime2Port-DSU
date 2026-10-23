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

Marked (25, every line and arm taken, asm-checked): LoadBacteriaSwarm A0, IsBoidVisibleForLockOn E28,
GetOrbitPosition FE4, PlayBoidSound 10F0, RenderParticles 11BC, UpdateParticles 1200, KillBoid 12E0,
ShouldBuildAreaCollisionCacheForPartition 13F4, GetAreaCollisionCacheForPartition 13FC,
ApplyBoundsAvoidance 1E08, ApplyCohesion(list) 2240, RenderBoid 2A34, SoftwareLight 2A40, PreRender 2DBC,
BoxForPosition 2EA4, GetListAt 3064, MoveBoid 340C, UpdateSwarmAnimations 34C8, UpdateAllBoidMovement 34CC,
PointOnSurface 5290, ProjectPointToPlane 54C0, GetTouchBounds 5538, GetBoundingBox 5574,
PreRenderAllViewports 560C, CBoid ctor 6DB4.

| arm | how |
|---|---|
| KillBoid: no DeathRattle / no Dead message | forced (`bacteriaswarm_force.py`: deathRattleChance and deadChance 0; Touch passes 1.0, and `CRandom16::Float` reaches 1.0 only for Next() = 0xFFFF) |
| RenderParticles / UpdateParticles: no death generator | forced (`bacteriaswarm_force.py`: both generators nulled from the 2601st Think) |
| everything else marked | natural |

asmcheck findings on the marked bodies are tool limits: NOPOS/BIT on the CBoid ctor (no bitfield table:
`rlwimi r0,r30,22,0,9` is the 10-bit mIndex, the x81 clears are mActive..mPursuingPlayer), CALL to the
CRayCastResult copy (F88), Cross (5434), CCollisionSurface ctor (4C20) and unmarked AddParticle 1238,
XCALL ApplyCohesion (the list overload calls the point overload 2120), and the loader's SLdr typedef calls.

## Unmarked behaviour bodies (why)

- Data-dead or forced-only, not run: AcceptScriptMsg AreaLoaded with live boids, GetWaypointForState
  (no waypoints), ApplySafeZoneAvoidance hits (no SAFZ), FindClosestCell, UpdateEffects, HardwareLight,
  FindBestCollisionInBox, ProjectVectorToPlane, CanRenderUnsorted (never called here).
- Partly covered: ctor/dtor, Think (occlusion-timer and sound-timer arms), UpdateBoid, ApplySteering
  (door arms), ApplySeparation/ApplyCohesion(point), ApplyPlayerAttraction, Touch (non-zero player touch
  radius: shipped value 0), CreateBoid (waypoint arm), AddParticle (null arm), GetAimPosition, GetLockOnIndex,
  FindBestLockOnIndex, UpdateLockOnBlend, Render, AddToRenderer, UpdatePartition, BuildBoidNearList,
  FindBestSurface, AddDoorRepulsors, ShortestRotationArcWrapped.
- Smoke family proposal: `bacteriaswarm`, area `3bfa3eff,04_temple_unknownenemies_dark`, driver
  `tools/cov/specs/bacteriaswarm_temple.con` (god 1; tp 181.9 147.5 -28.9; shoot/scan e003d0023; Delete).
