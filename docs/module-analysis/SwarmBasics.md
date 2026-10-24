# SwarmBasics REL (module 80): analysis notes

Base class of the boid swarms (CFlyerSwarm FSWM, CPlantScarabSwarm PSSM, CMetareeSwarm MSWM,
CIngBlobSwarm IBSM, the Emperor Ing light swarm). The module's code is upstream's
`src/MetroidPrime/Enemies/CSwarmBasics.cpp`; its `TARGET_PC` blocks are host guards (SwarmModelReady
in the ctor/PreRender/CachePose, the PreRenderBoid shift guard, RenderBoid's null display list) and
`PortDescribe` (what `obj <swarm>` prints: boids, generation, loop sfx, seekers). No SLdr loader lives
here: `CBasicSwarmData` is DOL code (`src/MetroidPrime/CBasicSwarmData.cpp`), the swarm SLdr headers
belong to the derived modules. Retail asm: `build/G2ME01/SwarmBasics/asm/MetroidPrime/Enemies/CSwarmBasics.s`.
Per-function table: `SwarmBasics_functions.tsv` (B upstream body, H weak/implicit copy, T REL runtime).

## Survey (swarm1, 2026-10-23)

Unit 95.7% fuzzy. `tools/upstream_hollow.py --min 0 SwarmBasics`: 7 trivial EMPTY bodies, all 100%.
Every sub-100% body was compared normalized (labels, relocations, register names) against upstream:

| function(s) | verdict |
|---|---|
| gravity uses (UpdateBoid, MoveBoid) | retail `kDefaultGravityAccel` = upstream `CPhysicsActor::kGravityAccel` (24.525001) |
| Touch fn_80_220C | `lbl_8041D558` = 0.3, the constant upstream passes to CreateRay; `__float_max` = FLT_MAX |
| UpdateEffects fn_80_7E4 | retail `divwu`, upstream `divw` on `CRandom16::Next() % activeCount`: both operands non-negative (Next is 0..0xFFFF, the list is non-empty whenever a boid updated) |
| UpdateLoopedSounds fn_80_E48 | retail `cmplw`, upstream `cmpw` on `xb1_ > x560_`: x560_ is 15 (60 in PortCEmperorIngLightSwarm), never negative |
| AssignSeekerBoids | retail has one redundant `beq` loop guard |
| DrawBoidSkinnedModel, SoftwareLight, CreateBoid, Render, BoxForPosition, GetListAt, rstl destroy/push_back_unsafe/uninitialized_copy, GetModelDataForAnimRes, fn_80_7440 (Cross copy) | scheduling, reloaded min/max constants, inlined rstl copies |

No behaviour difference found; no code change.

## Spawns that work (save mi_u)

| area | layer | ids | notes |
|---|---|---|---|
| Agon `42b935e4,01_sand_entrance` | 0, 2 | u041d, u04ff, u05c4, u05c6 (CFlyerSwarm) | active; `tp -338.4 -87.7 -6.7` stands in u041d |
| Torvus `3dfd2249,0g_swamp_hall` | 3 | u049e (CPlantScarabSwarm) | 50 boids |
| Sanctuary `1baa96c2,0a_cliff_hall` | 0 | u0457 (CMetareeSwarm) | 100 boids |
| Temple Grounds `3bfa3eff,0e_temple_hive_hall` | 0 | u044e, u0445 (CIngBlobSwarm) | |
| Torvus `3dfd2249,01_swamp_entrance` | 3, 4 | u04ec, u05a7 (CFlyerSwarm) | the only swarm seen with a loop sfx (loco 0x4a x4): the looped-sound chain |
| Torvus `04_swamp_temple_dark` L6, Great Temple `863fcd72,07_temple_mothlordsanctuary_dark` L4, Agon `04_sand_mining_dark` | | CFlyerSwarm, inactive | ids change with the layer order; `objs swarm` first |

## Coverage (swarm1)

Coverage build, spec `tools/cov/specs/swarmbasics.py`, drivers `tools/cov/specs/swarmbasics_{sand,swamp,cliff,hive,entrance}.con`
(god-mode player in the swarm; every beam, charge and missile; morph + boost; `send` Decrement/Increment,
InternalMessage0 x12, Deactivate/Activate, Delete):

    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/swarmbasics.py <out> 3dfd2249,01_swamp_entrance \
      --con tools/cov/specs/swarmbasics_entrance.con --fast --frames 3000
    python3 tools/cov/gcov_report.py build-port-cov <out dirs...> --module SwarmBasics

Marked (27, every line and arm taken, asm-checked): FinishConstruction E0, GetHealthInfo E4, HealthInfo EC,
ShouldBuildAreaCollisionCacheForPartition 790, GetAreaCollisionCacheForPartition 798, AddLoopedEmitter 9CC,
StartLoopedSound BB0, UpdateClosestPartitionLoopedSounds CB0, UpdateLoopedSounds E48, GetClosestPartitionList F6C,
GetOrbitPosition 17A8, IsBoidVisibleForLockOn 1F7C, ApplyAlignment 338C, ApplyCohesion(list) 3748,
CanRenderUnsorted 4278, SoftwareLight 4280, BoxForPosition 4778, GetListAt 4944, FindBestSurface 6CB0,
PointOnSurface 729C, ProjectVectorToPlane 7480, ProjectPointToPlane 74CC, GetTouchBounds 7544,
GetBoundingBox 7580, PreRenderAllViewports 7618, StopLocomotionSounds 814C, CBoid ctor A294.

| arm | how |
|---|---|
| StopLocomotionSounds: handle live / empty | forced (`send <id> Deactivate`, `send <id> Delete`) on the 01_swamp_entrance swarm |
| everything else marked | natural (the area plays; the player stands in the swarm and shoots) |

asmcheck findings on the marked bodies are tool limits: BIT on the CBoid ctor and StartLoopedSound (no
bitfield table: the cleared bits xb1_/xb2_04..80 are exactly mActive..mHasLoopedSound and xb1_), XCALL
GetBoundingBox (inlined in retail), CALL to unmarked helpers (fn_80_6AC0, 7440, 6EFC, 20DC, A9C, 3628).

## Unmarked behaviour bodies (why)

- Untaken by any swarm here: looped attack sounds (attack sfx is ffff everywhere), AcceptScriptMsg's attack-sound
  arms; HardwareLight, ApplyBoundsAvoidance, GetClosestBoxFacePlane, FindClosestCell (CBacteriaSwarm's paths);
  FreezeBoids (CHomingBlob), GetSeekerTargetLockedOn (seeker launcher lock), SetExplodeTimers, FreezeCollision,
  UpdateLightComboBeam and Touch's light-combo arm (Sunburst projectile); AddLoopedSoundToHandlesList, UpdateLoopedSoundPositions.
- Partly covered: Think (occluded-area arms: the player never left a swarm's area with it loaded), UpdateBoid,
  Touch, KillBoid, Render, PreRender, CachePose, GetLockOnIndex, UpdateLockOnBlend, QueueDeathMessage/FlushDeathMessages
  (the >10 death queue), the particle bodies (null-generator arm), ctor/dtor, CreateBoid, MoveToWayPoint.
- Next: a Sunburst/seeker driver, a door walk-out for the occlusion arms, a BacteriaSwarm area.

## lum3 baseline (2026-10-24): nothing marked, 21 bodies left

Five swarm runs (`tools/cov/specs/swarmbasics_{entrance,sand,swamp,cliff,hive}.con`, `swarmbasics.py`, coverage build, 3000 frames
each, caught/reach-stub 0) merged: 93 functions, 72 with branches, 11 with every arm taken. The 21 unmarked behaviour bodies all
have untaken arms (lines hit / branches taken): ctor 90/90 34/44, CreateBoid 33/36 7/10, UpdateBoid 46/85 17/44, Think 82/88 60/80,
UpdateAllBoidMovement 7/7 4/6, HardwareLight 0/8, PreRender 16/18 9/17, CachePose 7/12 2/6, PreRenderBoid 8/8 5/8, RenderBoid 5/6 3/6,
DrawBoidSkinnedModel 8/10 3/6, Render 48/53 39/56, Touch 48/72 49/93, FindBestLockOnIndex 21/21 13/14, AssignSeekerBoids 25/26 23/28,
FindClosestCell 0/12, UpdateEffects 25/26 24/34, UpdateSeekerTargets 41/46 40/47 (GetLockOnLocation[Valid] and
GetModelDataForAnimRes are not rows of the gcov report: header / free inline). What each family needs: frozen boids (`FreezeBoids`,
`mFreezeTimer` > 0: PreRenderBoid, RenderBoid, DrawBoidSkinnedModel, CachePose, Render), a non-animated swarm (`mAnimated` 0:
UpdateAllBoidMovement, Render; the BacteriaSwarm area), the model-not-ready TARGET_PC guards (CachePose, RenderBoid, PreRenderBoid),
the occluded-area arms (Think), the Sunburst light-combo arms (Touch, UpdateBoid), seeker lock (UpdateSeekerTargets, AssignSeekerBoids,
FindBestLockOnIndex 1630: a boid exactly at the camera). The gdb call/force approach of `docs/module-analysis/Lumite.md` (lum2/lum3:
`tools/cov/specs/lumite_lum2_calls.py`, the `CbnForce` caller-line force and `tools/cov/branch_lines.py` for per-edge counts) applies; the
natural drivers need a frozen-boid source (Ice/Plasma on a CHomingBlob swarm) and a door walk-out. This module needs its own
chunking: Touch (93 branches) and UpdateBoid (85 lines) alone are a slice each.
