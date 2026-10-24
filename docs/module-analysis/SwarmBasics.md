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

## Coverage (swb1: the 21 rows left)

Counted with port_progress.py's marker rule at 3d8091b41: 21 B rows carry no `// @ fn_80_*` marker.
Runs: the sand_entrance and swamp_entrance drivers (3000 f each) natural, then the same two with
`tools/cov/specs/swarmbasics_force.py`, all four `caught/reach-stub: 0`, 0 failed writes.

Marked (6, all objdiff 100%; the three render bodies only differ on the host path):

| fn | note |
|---|---|
| 0 GetLockOnLocationValid, 3C GetLockOnLocation | header-inline lock-on queries |
| 8FE0 ctor | 34/44 arms taken, the rest are the host-model/optional arms |
| 4640 PreRender, 4210 CachePose, 4128 RenderBoid | TARGET_PC async-model guards differ on the host only |

Left unmarked (15):

| fn | untaken (merged gcov lines) |
|---|---|
| 6104 UpdateBoid | **0/85: never ran.** The boid update path needs active boids, and CreateBoid never fires (below) |
| 76F4 CreateBoid | **0/36: never ran.** Forcing the generation loop on (`x4f0_30_ = true`, mMaxCreatedBoids 24, mBoidGenRate 2, mBoidGenCooldownTimer -1 at Think) took Think's 768/771 arms but still created nothing: the areas' swarms have no inactive boid to activate, so `mBoids` is empty or all active |
| 50B8 Think | 61/80: 763/764/765 (the occlusion timer, the player never leaves the swarm's area) and the boid-loop arms 784-856 |
| 220C Touch | 48/93: the light-combo arm (Sunburst), the boid-box arms (no boids) and the death-message arms |
| 3B34 Render | 39/56: the light-combo/hardware-light arms (1159, 1165, 1173, 1184, 1200, 1212, 1217, 1232) and the boid draw arms |
| 4538 HardwareLight | 0/8: only called from Render's two light-combo arms (0x3DA8, 0x3F3C), which need that state |
| 07E4 UpdateEffects | 24/34: 1993-2004, the boid history/animation arms |
| 0168 UpdateSeekerTargets | 40/47: 2087/2088 (the seeker launcher lock), 2105-2125 |
| 1978 AssignSeekerBoids | 23/28: 1665:2/3, 1677/1678, 1683 |
| 1C78 FindBestLockOnIndex | 13/14: 1630:1/3, the `!delta.CanBeNormalized()` arm (a boid exactly at the camera) |
| 4038 DrawBoidSkinnedModel | 3/6: 1131/1132/1136, needs a live skinned model |
| 4194 PreRenderBoid | 5/8: 1065/1066/1069 |
| 5024 UpdateAllBoidMovement | 4/6: 879/880 (the boid loop's empty/all-active arms) |
| 10C0 FindClosestCell | 0/12: **unreachable** - no call site in the module (0 `bl`, no vtable slot); it is the CBacteriaSwarm/CIngBlobSwarm path |
| A1BC GetModelDataForAnimRes | inlined into the ctor in this build (objdiff None, no gcov entry): the `animRes.GetId() == kInvalidAssetId` arm needs the null-model case |

**Next attempt for the boid bodies**: a swarm area whose data actually generates boids (mNumBoids >
0 with generation on) - the note's Agon 08_sand_casualpirates_dark / Temple 0e_temple_hive_hall
(CIngBlobSwarm) rows are the candidates; the two entrance areas used here never spawn one.

## Coverage (swb2: the boid path, unblocked)

swb1 ran the two *entrance* areas, whose swarms have `mNumBoids == 0`: `kSM_Create` fills no boid
list, the generation loop has nothing to activate, and CreateBoid/UpdateBoid stayed at 0 lines. The
areas the spawn table gives counts for do work:

    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/swarmbasics.py <out> \
      3dfd2249,0g_swamp_hall --con tools/cov/specs/swarmbasics_swamp.con --fast --frames 3000
    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/swarmbasics.py <out> \
      1baa96c2,0a_cliff_hall --con tools/cov/specs/swarmbasics_cliff.con --fast --frames 3000
    # then the same two with tools/cov/specs/swarmbasics_force2.py (force.sh)

merged: CreateBoid **33/36** (7/10 arms), UpdateBoid **45/85** (15/44), Think 70/88 (52/80),
Render 37/53 (32/56), PreRenderBoid 8/8 (6/8), RenderBoid 6/6 (4/6), all four runs 0
caught/reach-stub and 0 failed writes.

Marked: **4038 DrawBoidSkinnedModel** (ALL-ARMS: `mFreezeTimer = 1` on the boid and
`mEnableLighting = false` forced at the two entry breakpoints).

Still open, with what is left:

| fn | untaken | next step |
|---|---|---|
| 76F4 CreateBoid | 437:1/2, 440:1/2, 446:0/2 | 437/440 are the waypoint-null arms: stub `GetWaypointForState` / `CScriptWaypoint::NextWaypoint` to kInvalidUniqueId with force_ret FORCE_INNER (`$rax`); 446 is the upside-down-surface (`dot(normal, z) == -1`) arm |
| 6104 UpdateBoid | 15/44 arms (589-641) | the freeze/launched/safe-zone/stuck arms: `mFreezeTimer`, `mLaunched`, the partition cache and `mBoidRadius` writes at the entry |
| 5024 UpdateAllBoidMovement | 0/7 in these areas, 4/6 in the entrance one | never entered here: `mBoids.size()` is 0 or `mAnimated` is false for the swamp/cliff swarms; force `mAnimated` or use the entrance swarm merged with this one |
| 4538 HardwareLight | 0/8 | only from Render's light-combo arms (0x3DA8, 0x3F3C): needs `mLightCombo` state, or a Sunburst/light-weapon driver |
| 3B34 Render | 1163, 1172-1177, 1190-1204, 1214-1236 | the light-combo arms above plus the outlier-boid and boid-draw arms |
| 220C Touch | 1463-1554 (45 arms) | the light-combo arm, the boid-box arms and the >10 death queue |
| 07E4 UpdateEffects | 1997-2008 | the boid history/animation arms |
| 0168 UpdateSeekerTargets, 1978 AssignSeekerBoids | 2091-2129, 1669-1687 | the seeker launcher: needs a seeker weapon fired at the swarm |
| 4194 PreRenderBoid | 1069:1/2, 1072:1/2 | 1072 is the TARGET_PC `idx < 32` guard (needs mModelDatas.size() > 32, which the shipped swarms do not have) |
| 1C78 FindBestLockOnIndex | 1634:1/3 | `!delta.CanBeNormalized()`, a boid exactly at the camera |
| 50B8 Think | 754, 764-772 (occlusion timer) and 785-857 | the occlusion arms need the player to leave the area with the swarm still loaded (a door walk-out); 785-857 are the boid loops |
| A1BC GetModelDataForAnimRes | inlined | the invalid-asset-id arm |

## swb3: the recipes applied - and why the forced run stalls

`tools/cov/specs/swarmbasics_force3.py` implements the swb2 recipes (FORCE_RET on
UpdateAllBoidMovement mAnimated, Render mEnableLighting/mUseSoftwareLight, UpdateBoid
mLaunched/mVulnerableToSafeZone/mFramesNotOnSurface/mLifeTime; FORCE_INNER stubs on
GetWaypointForState/NextWaypoint returning 0xffff, and a rewrite of FindBestCollisionInBox's returned
triangle so its normal is -Z for CreateBoid's upside-down arm). Two runs (swamp_hall, cliff_hall,
3000 f) **stalled instead of finishing**: the port watchdog reports `no frame for 15.3 s (frame 283);
main thread in 'game update'` and no `gcov` directory was written, so the lane produced no new
coverage. Two causes, both to fix before this recipe is retried:

1. **FORCE_RET restores through a dead register.** `((CSwarmBasics::CBoid*)$rcx)->mLaunched` is
   written at the body entry (fine) but the restore runs in the caller (`FinishBreakpoint`), where
   `$rcx` no longer points at the boid: `restore failed (...): Cannot access memory at address
   0x100b9`. The written value therefore sticks, the boid stays `mLaunched` forever and the launch
   loop (`while (distance >= 0 && !found)` plus the falling branch) never settles. Write swarm
   members (`$this->...`) or the vector element (`$this->mBoids[<index>]`) instead, or take the boid
   pointer from a convenience variable captured at the entry.
2. **Forcing Render's light flags is very expensive.** `mEnableLighting = 1` + `mUseSoftwareLight =
   0` sends every in-frustum boid through `HardwareLight`, which builds a `CActorLights` (area light
   list + dynamic lights) per boid per frame; with 50-100 boids the frame time explodes. Force it on a
   handful of frames (a high `skip`) or for one boid only.

Neither `CreateBoid`'s three arms nor `UpdateAllBoidMovement`/`UpdateBoid` were re-measured, so no
marker was added in swb3.

## swb4: the recipes retried with both fixes

`tools/cov/specs/swarmbasics_force4.py` (force_ret.py) fixes swb3's two defects: every FORCE_RET
lvalue is a `$this` member, so the restore at the call's return writes the same object (no
argument-register lvalue, which was what stuck `mLaunched`), and the hardware-light path is forced on
one Render call in 600. Runs: swamp_hall and cliff_hall, **1200 frames, timeout 700**, both exit
normally (`RC=0`, `caught/reach-stub: 0`), no failed or missing-symbol writes.

**Marked: 76F4 CreateBoid - 36/36 lines, 10/10 branches (ALL-ARMS).** The waypoint stubs
(`GetWaypointForState`/`NextWaypoint` -> 0xffff) take 437/440 and the rewritten triangle takes 446.

Measured now, still open:

| fn | now | why |
|---|---|---|
| 6104 UpdateBoid | 48/85, 17/44 | the `$this` writes took 589/597 but the launched path (604-641) and the no-surface/stuck arms (633-641, 655) need boid-field writes, which FORCE_RET cannot restore safely through a register; use force.py (persistent, one `skip`) or `$this->mBoids[<index>]` (the vector's element, resolved through $this) |
| 5024 UpdateAllBoidMovement | 0/7 | **the body never runs even with `mAnimated` forced**: it has an out-of-line symbol (objdiff 99.5%) but no caller in the module - retail calls it from Think's tail, and Think does not reach line 867 in these runs (Think 82/88: 851/853/856/857, the outlier loop, are untaken). Next: find what returns before Think's tail (the occlusion block and the outlier loop) |
| 4538 HardwareLight | 0/8 | Render 45/53 still leaves 1217/1218/1222/1223 untaken, so the forced `mEnableLighting`/`mUseSoftwareLight` did not reach the HardwareLight call; the locals are read at Render's entry, so a `loc:` write (with `$this` captured at the entry) is the next attempt |
| 3B34 Render | 45/53, 37/56 | as above, plus the light-combo arms |

## swb5: the three next steps measured

`tools/cov/specs/swarmbasics_force5.py` (force.sh) writes, all through `$this`: Think's outlier list
forged to `&mBoids.mItems[0]` (the vector's element array is `mItems`, not `mBegin`), boid 0's
`mLaunched` from the 1000th UpdateBoid on, and Render's light flags plus a forged outlier list on one
call in 590. Six runs (swamp_hall, cliff_hall, 1200 frames, timeout 700), every one exits normally
(`RC=0`, `caught/reach-stub: 0`, 0 failed writes).

| fn | before -> now | what the runs showed |
|---|---|---|
| 6104 UpdateBoid | 48/85, 17/44 -> **72/85, 27/44** | the `mItems[0].mLaunched` force works (the launched path 604-641 ran). Left: 590/592/598/599 (the lifetime and safe-zone arms: `mLifeTime` is a swarm member, so those need `$this->mLifeTime` on a frame where a boid is also launched), 617/618 (the `mRemainingLaunchNotOnSurfaceFrames` arm), 634-642 (the no-surface/stuck arms) |
| 5024 UpdateAllBoidMovement | 0/7, 0/6 | unchanged: **the body cannot be measured here - it has an out-of-line symbol (objdiff 99.5%) but gcov never attributes a hit to it, so GCC inlines it into Think and the standalone copy is dead.** Marking it needs the inlined-line evidence (the same route the SLdr struct ctor used) or a build where it is not inlined |
| 3B34 Render / 4538 HardwareLight | 36/53, 31/56 / 0/8 | unchanged: the outlier-loop light arms (1216-1226) still never run. The forced list at Think's entry is wiped by `UpdatePartition()` (which clears `mOutlierBoidList` later in the same body), and the write at Render's entry did not take either: `Render` reads `mOutlierBoidList` at 1215 but the loop stays untaken, so the write is being overwritten or the breakpoint's `$this` differs - check with a `forced.log` counter per hit before the next attempt |

## swb6: UpdateBoid to 35/44, and a Render anomaly

`tools/cov/specs/swarmbasics_force6.py` (force.sh) writes a six-phase cycle at UpdateBoid's entry
(through `$this->mBoids.mItems[0]`: mLaunched, mFramesNotOnSurface 30, the remaining-launch frames;
and through `$this`: mLifeTime 1/0, mVulnerableToSafeZone), captures `$this` at Think's entry and
forges the outlier list at `loc:CSwarmBasics.cpp:824` (the line after `UpdatePartition()`), plus a
Render probe (no writes) that logs the lighting flags and the outlier list per hit. Two runs
(swamp_hall, cliff_hall, 1200 f, timeout 700), both exit normally with 0 failed writes.

- **6104 UpdateBoid: 72/85, 27/44 -> 76/85, 35/44.** Left: 599 (the safe-zone hit itself:
  `PointIsInSafeZone` returns false in these areas - stub it with force_ret), 613:3/4, 617:1/2, 635,
  636, 642 (the launched no-surface arm and `mHasLoopedSound`), 653/656/657 (the non-launched tail).
- **3B34 Render: 0/53 in these runs** (it was 36/53 in swb5) and the Render probe never fired: the
  body was not entered at all, although PreRender ran (13/18). The forged outlier list persists
  between frames (force.py does not restore), so the first thing to check next time is whether the
  forged list - or the `mItems[0].mNext = 0` write, which also truncates whatever partition list that
  boid was in - changes the render path. Re-run the swb5 spec (no loc: write) to confirm Render comes
  back before blaming the light flags.
- 4538 HardwareLight: still 0/8, unreachable while Render does not run.
- 5024 UpdateAllBoidMovement: inlined into Think (no gcov hits), as swb5 recorded.

## swb7: UpdateBoid to 85/85 lines, 41/44 branches

`tools/cov/specs/swarmbasics_force7.py` (force_ret.py): a twelve-phase cycle at UpdateBoid's entry,
every lvalue `$this`-relative (`$this->mBoids.mItems[0]` for mLaunched, mVelocity, mLifeTime,
mFramesNotOnSurface, mRemainingLaunchNotOnSurfaceFrames, mHasLoopedSound; `$this` for mLifeTime,
mVulnerableToSafeZone, mBoidRadius), plus a FORCE_INNER stub of
`CSafeZoneManager::PointIsInSafeZone` returning 1 (the 599 hit, KillBoid and return). No loc:
breakpoints and no `mNext` writes - the two things that stopped Render in swb6. Eight runs over four
passes (swamp_hall, cliff_hall, 1200 f, timeout 700), every one `RC=0`, 0 caught/reach-stub, 0 failed
writes.

Progression: 76/85 35/44 -> 81/85 38/44 -> 85/85 40/44 -> **85/85 41/44**.

Still untaken, with what is known about each:

| arm | note |
|---|---|
| 613:3/4 | the launched `while (distance >= 0.f && !found)` fourth edge. Big and tiny boid radii both tried (mBoidRadius 1000 / 0.001 with velocities -20/-40/-400); the missing side is probably `found` true at the loop test, which needs a surface found while `distance` stays >= 0 - a boid placed just above a floor at a small radius |
| 656:1/2, 657:1/2 | the falling (`!launched && mFramesNotOnSurface < 30`) path: `plane.GetHeight` and `distance <= mBoidRadius * mSurfaceProbeScale`. mBoidRadius 1000 / 0.001 did not turn them; the next attempt should write the boid's translation onto the floor (`mItems[0].mTransform` + the player's position) instead of the radius |

The marker stays off (41/44) - the body is one arm from complete and the spec is ready for the last
step.

## swb8: UpdateBoid 85/85 lines, 43/44 branches (613 left)

`tools/cov/specs/swarmbasics_force8.py` (force.sh) writes boid 0 relative to where it already is:
each call adds +0.3 / -0.5 / +5 to `mItems[0].mTransform.m23` (the transform's translation Z) and
sets mLaunched, mVelocity (-60 / -4), mRemainingLaunchNotOnSurfaceFrames 0, mFramesNotOnSurface 0
and `$this->mBoidRadius` (0.5 / 1.0). **Corrections (swb9):** that is not a floor-relative,
non-drifting placement - the three offsets are cumulative and net +4.8 per three-call cycle, so the
boid climbs ~+4.8 per cycle and leaves the floor; the lvalue is `$this->mBoids.mItems[0]`, which is
only boid 0 of this swarm and not necessarily the object the call updates (Think's outlier loop and
the derived `UpdateBoid` overrides pass other boids); and phase 0's stated reason (`speed*dt -
boidRadius = 0.5 >= 0, so the loop leaves through !found with found true`) is wrong: at
`mSurfaceProbeScale` 1.5 the probe point sits a whole `dt*|v|` (1.0) below the plane with radius
1.0, so `FindBestSurface` never returns true in the first place. Runs: **two** with the swb7
force_ret spec (`a`, `a2`; the 599 safe-zone stub) and **six** with this one (`b`, `b2`-`b6`), on
`3dfd2249,0g_swamp_hall` and `1baa96c2,0a_cliff_hall` (not the `42b935e4,01_sand_entrance` the
spec header used to say), 1200 f, timeout 700. All eight exited normally - `[Inferior 1 (process N)
exited normally]` in `<out>/game.log` and `game rc=0`, `caught/reach-stub: 0` in
`/tmp/opencode/swb8/run_<dir>.log`, dirs `a a2 b b2 b3 b4 b5 b6` - 0 failed writes. Merged, the body
reaches **85/85 lines, 43/44 branches**.

The floor placement took 656/657 (the falling path's plane-height and radius tests). **613:3/4 was
the only arm left**: `while (distance >= 0.f && !found)` needs the loop to re-test with `found`
already true, i.e. a surface found on the first probe while `speed*dt - boidRadius >= 0` afterwards.
Radii of 0.1, 0.5, 1.0, 1000 and 0.001 were tried with velocities -4/-20/-40/-400/-60 and a boid 0.3
above and 0.5 below the floor; the surface probe (`FindBestSurface` with radius `2*mBoidRadius` at the
predicted point, `mSurfaceProbeScale` back along the velocity) did not report a hit on an iteration
that still had `distance >= 0`.

The proposal recorded here for the next attempt - "set `mSurfaceProbeScale` small ... or place the
boid over a known flat floor with `mItems[0].mVelocity` near zero and a huge radius so every probe
hits" - is **rejected for its second half** (swb9). A huge radius does not stop the probe from
hitting a surface (it only makes the probe sphere `2*mBoidRadius` wider); what it cannot do is
exercise the wanted short-circuit edge. `distance` starts at `speed*dt >= 0`, so the loop does enter
its first iteration for any finite nonnegative speed and dt, whatever the radius; one probe then
runs, and `distance -= boidRadius` makes `speed*dt - radius` negative whenever `speed*dt < radius`, so
the re-test leaves through `distance < 0` (613:1/4) on the first pass and never reaches the `!found`
re-test with `found` true (613:3/4). A near-zero velocity has the same first-order problem twice
over: the loop's distance is ~0, and the probe point (scale 1.5) stays on the boid, so a hit needs
the boid to already be within `2*mBoidRadius` of a triangle - which is the swb9 fixture, done with a
small radius and `speed*dt = 4*radius`.

## swb9: UpdateBoid 85/85 lines, 44/44 branches (ALL-ARMS) - marked

`tools/cov/specs/swarmbasics_force9.py` (force.sh, persistent writes) takes 613:3/4 - the launched
loop's re-test with `found` already true - from geometry, not from a stub. Per UpdateBoid call:

- **the boid is the call's own argument**: the entry register that holds an element of
  `$this->mBoids` (`$rcx` for this ABI; `$r8`/`$r9` are tried and would be reported), indexed off
  `mBoids.mItems`/`mCount`/`sizeof(CBoid)`. Never `mItems[0]`: Think's boid loop and its outlier loop
  pass different objects, and `CPlantScarabSwarm`/`CMetareeSwarm` override `UpdateBoid` and forward
  their own (the run's backtrace shows `CPlantScarabSwarm::UpdateBoid` -> `CSwarmBasics::UpdateBoid`
  -> `FindBestSurface` at `CSwarmBasics.cpp:617` with `radius = 0.600000024 = 2*mBoidRadius`).
- **"on a surface" is measured, not assumed**: from the boid's own `mSurface` triangle (a real
  `CCollisionSurface` a real `FindBestSurface` returned and 654 stored) - the translation is within
  `2*mBoidRadius` of that triangle's plane *and* its projection is inside the triangle (the three sign
  tests of `PointOnSurface`, recomputed in the spec) *and* the boid took the non-launched path last
  frame (`mFramesNotOnSurface == 0`, `mLaunched == 0`). Measured capture on `0g_swamp_hall`:
  `dt=0.016667 r=0.3000 h=0.31001` - exactly the `radius + 0.01` the snap at 663 leaves, on triangle
  `(-132.07,-56.25,40.85)(-135.43,-57.45,40.67)(-143.61,-57.25,41.78)`, normal `(-0.1099,0.4412,-0.8907)`.
- **saved absolute position, not a delta**: the position and triangle are cached per boid, and a later
  call re-writes that absolute position (only while the boid is still within 0.05 of it, so the
  `CAreaCollisionCache` Think built for this boid still covers the point).
- **the numbers that make the loop exit through `!found`**: `speed = 4*r/dt` (so `speed*dt = 4r`, and
  `4r - r = 3r >= 0` survives the `distance -= boidRadius`) and probe scale `s = h/(8r)`, so the probe
  point lands `h/2` over the same triangle - inside `2r` of the plane and inside the triangle, where
  `FindBestSurface` has to return true on the real octree by itself. Measured: `speed=72.000`,
  `scale=0.12917`, `speed_dt=1.2000`, `minus_radius=0.9000`, `predicted_h=0.15500`.

Each apply arms a one-shot breakpoint on `FindBestSurface`, so the loop's own call reports its real
result (`PROBE-RESULT swb9-<n> found=1 predicted=(...) radius=0.60000 surface_h=0.15500
surface_normal=(-0.1099,0.4412,-0.8907)` - the returned triangle is the captured one, at the predicted
height). Nothing forces `found`, the loop condition's distance, or a surface: no `force_ret` stub, no
`found` write.

Runs (all `3dfd2249,0g_swamp_hall`, `swarmbasics_swamp.con`, `--fast`): `d` 1200 f and `f` 300 f on
the coverage build, `g` 300 f on the shipped `build-port-link` after the marker - every one normal
inferior exit (`game rc=0`, `caught/reach-stub: 0`, 0 failed writes), `/tmp/opencode/swb9/{c,d,f,g}`. `c` and `d` are the same fixture with the probe logging still broken
(`PROBE-RESULT` never fired); `f` and `g` are the final spec, where it does. In this fixture's own
profile **613:3/4 is taken and 613:1/4 is not**: the loop's exit on `found` is the only one it reaches
(the falling exit needs the swb8 copy of the arms). Merged with the eight swb8 profiles:

    python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/swb9/d/gcov /tmp/opencode/swb9/f/gcov \
      /tmp/opencode/swb8/{a,a2,b,b2,b3,b4,b5,b6}/gcov --module SwarmBasics
    SwarmBasics - CSwarmBasics::UpdateBoid(...)  85/85  44/44  ALL-ARMS

**Deviation from the brief's cap (recorded, not hidden):** the slice was allowed "at most two targeted
1200-frame attempts". Seven game runs were spent: `a` (1200 f; the spec's module-level `class
gdb.FinishBreakpoint` raised `NameError`, so nothing was forced and the run proved nothing), `b`
(stopped at a breakpoint - a 5-tuple unpacked as 4 in the probe callback - no counters written), `c`
and `d` (1200 f, fixture working, probe logging broken), `e` (300 f, probe logging still broken), `f`
(300 f, final spec) and `g` (300 f, final spec on the shipped build). So the cap was exceeded: two
1200-frame fixture runs (`c`, `d`) plus three further fixture runs (`e`, `f`, `g`) and two runs lost
to spec bugs (`a`, `b`). Counting only the 1200-frame runs as "attempts" would understate it.

The merge is against the `build-port-cov` of `8816a08d9` (pre-marker), so the line numbers in this
table are that build's; the marker shifts the body by one line.

How each group of UpdateBoid's arms was taken (all forced except the tail):

| lines | arm | run |
|---|---|---|
| 590/592 | `mLifeTime > 0`, boid lifetime `<= 0` / not | force7 `ph7`/`ph8`/`ph10` |
| 598/599 | safe-zone test, and the hit (KillBoid + return) | force7 `ph6` + `FORCE_INNER` stub of `PointIsInSafeZone` |
| 613 (all 4) | loop body entered twice, exit on `distance < 0`, exit on `found` | force7 `ph0`/`ph9` (falling exit), swb9 (found exit) |
| 617/618 | `FindBestSurface` true, remaining-launch `== 0` / not | force7 `ph2`, `ph1` |
| 634/635/636 | `!found`, gravity, the counter | force7 `ph0`/`ph1` |
| 640-643 | `mFramesNotOnSurface >= 30` with and without a looped sound | force7 `ph3`/`ph4` |
| 653/656/657 | falling path: surface found, plane height, radius test | force7 `ph5`/`ph11`, swb8 floor placement |
| 664-685 | tail (near list, steering, turn) | natural |

**Marked** (`// @ fn_80_6104`, `src/MetroidPrime/Enemies/CSwarmBasics.cpp`). `asmcheck.py check
SwarmBasics ... fn_80_6104` (all tags) gives five findings, none a behaviour difference: `NOPOS
mVulnerableToSafeZone` (bitfield with no stated position in the header); `BIT 0x620C asm sets x4f0_01,
C++ never writes it` (that store *is* `mBoidUpdatedThisFrame = true` at 597; the mask matches the
header's 8th bit, the tool's anchored layout does not); `CALL 0x6370 fn_80_6AC0` (the `CCollisionSurface`
constructor of 614, a weak copy at 0x6AC0 - `SwarmBasics_functions.tsv` row 06AC0); three `XCALL`s
(`KillBoid`, `BoidCollidedCallback`, `ApplySteeringBehaviors`) against retail's four `bctrl` vtable
calls the tool cannot follow - slots `0x94` twice (0x61F4, 0x6290: the two early returns), `0xBC`
(0x6510), `0x90` (0x6938).

Gates: host build `metroid_prime2_port` 0 FAILED; `gen_files_cmake.py --check` rc 0;
`review_decomp_diff.sh 8816a08d9` rc 0 (one `MARK`, no MASK/DEL/NULL/STUB); matching build
`configure.py && ninja` `87 files OK`; no `TARGET_PC` block and no non-comment source change, so the
port/matching split is untouched by construction.
