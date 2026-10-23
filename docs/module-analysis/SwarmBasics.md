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
