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
## swb10: FindBestLockOnIndex 21/21 lines, 14/14 branches (ALL-ARMS) - marked

`tools/cov/specs/swarmbasics_lockon.py` (force.py). One existing element of `$this->mBoids` is moved
for the duration of each `FindBestLockOnIndex` call and its **absolute** translation (and `mActive`)
is restored by a finish breakpoint at the call's return. Three phases, cycled per call:

| phase | the boid's translation | arm it reaches |
|---|---|---|
| 0 | the camera's own (`m03,m13,m23` read from `mCameraManagers[0]->mFpCamera->mTransform`) | `delta == 0` -> `CanBeNormalized()` false (the documented blocker: a boid exactly at the camera) |
| 1 | camera + 1e6 on X | `delta.MagSquared() > maxDistanceSq` true -> `continue` |
| 2 | camera + the transform's own right column | normalizable, `dot == 0` -> the `dot > maxDot` false side |

The camera is read through the source's own chain (`CStateManager::GetCameraManager(0)`,
`CCameraManager::GetFirstPersonCamera()`, `GetTransform()` = `CActor::mTransform`, then
`GetTranslation/GetForward/GetRight` of `CTransform4f`), every member name checked against the build's
DWARF with `ptype`/`p &((T*)0)->m` before the run - no retail offset is used (the host layout differs:
`mActive` 0xBA host vs 0xB2 retail, `sizeof(CBoid)` 192 host vs 0xB8 retail, camera transform 0x34 host
vs 0x24 retail). The candidate is the first already-active element, else element 0 with `mActive` set
for the call and restored after it; the other 49 elements are untouched, so the loop still takes both
sides of `if (it->mActive)`. No array/count/model write, no condition stub, no `found`/return write.

**The "13/14, one arm" recollection does not match this build.** On the pre-merge coverage build (same
body source), before the fixture, the body was **20/21 lines, 10/14 branches** (four arms untaken),
because the swamp swarm's boids are almost never `mActive` while the function scans them - only 4 of
52800 element visits in the swb9 profiles were active. With the fixture the single run gives **21/21
lines, 14/14 branches, ALL-ARMS**.

Run: `3dfd2249,0g_swamp_hall`, `swarmbasics_swamp.con`, `--fast`, 1200 frames, one launch:
`game rc=0`, `caught/reach-stub: 0`, normal inferior exit, 864 applies and 864 restores, 0 failed
writes (`/tmp/opencode/swb10/a`, geometry in `swb10_geom.log`). Note the coverage build was rebuilt on
the merged tree (6bec08d36) for this run, so its gcno differs from the swb8/swb9 runs' - their
profiles are stamp-incompatible and were **not** merged; this row is the one run alone.

`asmcheck.py check SwarmBasics ... fn_80_1C78` (all tags): **0 findings**. The body matches the asm
shape: `lwz r3,0x14fc(r4)` player + `bl GetOrbitMaxTargetDistance` (0x1CF0), `lwz r4,0x151c(r31);
lwz r4,0x18(r4); addi r4,r4,0x24; bl __ct__12CTransform4f` (0x1CF4-0x1D08), the loop over
`0x184/0x17c` with stride 0xB8 (0x1DC4-0x1DD8), `extrwi. 1,24` on 0xb2(r31) for `mActive` (0x1D30),
`fcmpo/bgt` against the squared orbit distance (0x1D70), `bl CanBeNormalized` + `clrlwi./beq` (0x1D7C),
`bl AsNormalized` and the `fmadds` dot compared with `lbl_80_rodata_0` = 0.5 (`fcmpo/ble` 0x1DAC).

Marked `// @ fn_80_1C78` (`src/MetroidPrime/Enemies/CSwarmBasics.cpp`). Line numbers above are the
measured build's (before this marker, which shifts the body by one line).
## swb11: AssignSeekerBoids 25/26 lines, 24/28 branches - the taken-scan arms stay blocked

Base `014df5305`. `tools/cov/specs/swarmbasics_seeker.py` (force.py) and
`tools/drivers/swb11_seeker.con`. The caller is the real one
(`UpdateSeekerTargets` -> `AssignSeekerBoids(mgr, keptBoids, numLost, newBoids)` every frame), so
`taken`, `numNeeded` and `out` are the caller's own vectors: the spec never writes, resizes or
replaces them, and moves only one existing `mBoids` element per call (restored by a finish
breakpoint), cycling: phase 0 = the boid beyond the orbit distance (camera + 1e6 on X), phase 1 = the
boid exactly at the camera, phase 2 = no move.

**Before (swb10's profile, same body source):** 24/26 lines, 21/28 branches; untaken 1669:0/2,
1670:0/2, 1673:2/3 (the distance and normalisation arms, the swamp swarm's boids are almost never
`mActive` while the function scans them) plus the whole taken-scan, 1685/1686/1691.

**After one 1200-frame run (`/tmp/opencode/swb11/b`, normal exit, `caught/reach-stub: 0`,
651 applies / 651 restores, 0 failed writes):** **25/26 lines, 24/28 branches**. The geometry arms
turned (the 1670/1673 entries are gone). Still untaken, and all four for one reason:

| arm | needs |
|---|---|
| 1685:0/2 | `taken` non-empty (the `for (j < taken.size())` body) |
| 1686:0/2, 1686:1/2 | both sides of `idx == taken[j]` |
| 1691:1/2 | `!found` false, i.e. a candidate whose index *is* in `taken` |

**Measured blocker:** `taken` was empty on every one of the 655 calls (`taken=0 needed=5`), because
the gun's seeker targets never intersected the swarm's five `kMT_SeekerTarget` actors. The driver
gave the Seeker Launcher (`gave seekerLauncher: 1/1`), 255 missiles, combat visor, stood 22 units
from the boid cluster at (-135.5,-57.3,40.4) (measured in the first run) and held the use-item button
five times (frames 380-1140) with a yaw/pitch sweep (`useItem = 18 = kFL_YButton` per `SLdr/Tweaks.inc`), and
`GetSeekerTargetLockedOn` stayed **0/8 (never called)**, `UpdateSeekerTargets` 2113:1/2 (the
`keptBoids.push_back_unsafe`) untaken. So `CPlayerGun::UpdateSeeker`'s lock block never produced a
target. Which of its prerequisites failed is **not** measured (two launches, no more allowed):
`mSeekerChargeState` reaching `kSCS_FullyCharged`, `GetTargetId(mgr)` being the swarm (the path that
would call `GetSeekerTargetLockedOn`), or the generic 50-px path's
`GetTargetableVisorFlags` visibility on the seeker actors. The gun side needs its own slice - a spec
on `CPlayerGun::UpdateSeeker` with the seeker state and target id logged per call. No marker: the
module note's marker rule needs every live arm.

Run 1 (`/tmp/opencode/swb11/a`) is *not* evidence: it pressed Z at frame 820, the game paused
(`[harness] no game update for 30.0 s`), the timeout ended it and no counters were written. Launch 2
(`b`) is the only evidence, and it is the profile above (`build-port-cov` of `014df5305` + the swb10
marker, so its line numbers are the committed tree's: `AssignSeekerBoids` at 1658).

**Correction (log label only):** the swb10 spec's APPLY line labelled a field `boid=` but passed
`this` to the printf, so it printed the swarm actor pointer. `swarmbasics_lockon.py` labels it
`swarm=` now and `swarmbasics_seeker.py` logs `swarm=` and `boid=` (the address of `$this->mBoids[idx]`)
separately; the archived `/tmp/opencode/swb10` logs are untouched and still say `boid=` for the swarm.
## swb12: AssignSeekerBoids 26/26 lines, 27/28 branches - one output-loop edge left

Base `e99a3837f`, branch `decomp/swb12`. No source change. `tools/cov/specs/swarmbasics_seeker.py`
(force.py) now has two fixtures and `tools/drivers/swb12.con` (no input at all):

- **`gun_fixture` at `UpdateSeekerTargets`' entry** supplies ONE real seeker uid: the swarm's own
  five `kMT_SeekerTarget` actors (created in `kSM_Create`, uids in `mSeekerTargets`, boid indices in
  `mSeekerBoidIndices` from the previous frame) give a slot whose uid resolves through
  `CObjectList::GetObjectById`'s own two tests (`mObjects[uid.Value()].mEntity` and the uid equality,
  read from `mObjectLists[0]` - no inline call from gdb) *and* whose boid index is in range. That
  uid goes into the gun's `mSeekerTargets` (`reserved_vector<pair<TUniqueId,float>,5>`) as its only
  element, written element-first then `mCount`, only when the gun's vector is empty; `mCount` and the
  element's exact bits are restored at `UpdateSeekerTargets`' return. The caller then builds a
  non-empty `keptBoids` by itself - `taken`/`out` are never touched.
- **`assign_fixture` at `AssignSeekerBoids`' entry**, one phase per call: ph0 beyond the orbit
  distance, ph1 exactly at the camera, ph2 natural, ph3 the taken boid plus more boids in front of
  the camera. Every restore breakpoint is armed **before** the first write and a fixture aborts
  without writing if arming fails (swb11 wrote first - corrected here). ph2 is deliberate: it is
  what leaves the run a natural swarm, **not** an observation that the gun locks by itself.

**Measured: 26/26 lines, 27/28 branches** (`/tmp/opencode/swb12/b`, normal exit,
`caught/reach-stub: 0`, 725 applies, 0 failed writes). The four taken-scan arms (1685:0/2, 1686:0/2,
1686:1/2, 1691:1/2) all turned: the gun element gave `taken=[idx]`, and the ph3 cone put that boid and
others among the candidates, so the scan's equality and inequality sides both ran.

**That run is a flawed-persistent-placement fixture, not accepted transient verification.** Its
`plan` stored the *target* position in the field the restore wrote, so the 962 `RESTORE-BOID` lines
rewrote the target: the boids were never put back, the mutations accumulated across the run, and the
phase-2 calls ("natural swarm") ran on a swarm whose boids had already been moved. The coverage counts
above stand as counts (the arms did execute), but nothing in that run shows that the transient writes
are undone, and its log lines must not be read as successful restores. Corrected afterwards, with no
launches left: `plan` now keeps `target` and the ORIGINAL translation read before any write (separate
fields), the restore writes the original through the cached `&mBoids[idx]`; the gun element is
snapshotted and restored as two raw 32-bit words at the cached element address (`*(int*)((char*)elem+0)`
/ `+4`, written back with `set {int}`), not through a float `%r`; phase 3 takes the index from *this
call's* `taken` vector (guarded, in range) instead of a global left by an earlier call or swarm.
Verified statically without a launch (emulated gdb over a model of the boid fields, the caller's
vectors, the camera and the gun element): every phase leaves all boid transforms and `mActive` equal to
their originals after the call returns, the writes carry the targets, `taken=[]` and an out-of-range
taken entry move nothing, and the gun's `_RestoreGun` puts the element's exact words and `mCount`
back.

**The one arm left, 1682:1/4** - the `out.size() < numNeeded` **false** exit of
`for (uint i = 0; out.size() < numNeeded && i < candidates.size(); ++i)`. It needs `out` to actually
reach `numNeeded`: with one uid in the gun the caller passes `numNeeded = numLost = 4`, and ph3 moved
only two boids, so `out` never filled. `newBoids` has `reserve(numLost)` capacity, so exactly
`numNeeded` non-taken candidates is the safe maximum. ph3 now moves
`min(count, numNeeded + 1)` boids into the cone (the taken one first, then distinct non-taken
indices); that widening is **derived from this arm, not yet measured** - it is the one change the next
run needs, and no other arm is open.

Run 1 (`/tmp/opencode/swb12/a`) found no slot: the spec's object-list expression indexed
`mData[0][0]` instead of `((cast)[0])`, so the actor lookup threw and every slot was rejected; that is
a spec bug (fixed), not a game-side finding. Launches spent: **2 of 2**.

Evidence (`/tmp/opencode/reports/swb12-evidence/`): `CSwarmBasics.cpp.gcno`, the run's
`CSwarmBasics.cpp.gcda`, the raw `CSwarmBasics.cpp.gcov.json.gz`, `binary.sha256`, `source.sha256` -
archived before any marker change, and since this slice changes no source the gcno stays the
committed tree's.
## swb13: AssignSeekerBoids 26/26 lines, 28/28 branches (ALL-ARMS) - marked

Base `3a921d5dc`, branch `decomp/swb13`. One launch of the two allowed.

The fixture (`tools/cov/specs/swarmbasics_seeker.py`) now does what the swb12 rework should have:
every write and every restore goes through the **cached absolute boid pointer** (`&mBoids[idx]`,
`((CSwarmBasics::CBoid*)addr)->…`), never re-resolving the vector; the original translation is
snapshotted as **raw 32-bit words** (offsets of `mTransform.m03/m13/m23` taken from the debug info,
logged once) together with `mActive`; and each restore **reads back** what it wrote and compares with
the snapshot, logging `RESTORE-BOID … OK` or `… MISMATCH` and counting mismatches. The gun element is
restored as its two raw words plus `mCount`, also read back (`RESTORE-GUN … OK/MISMATCH`). Phase 3
takes the index from **this call's** `taken` vector (guarded in range) and moves
`min(count, numNeeded + 1)` boids into the camera cone (the taken one first, then distinct non-taken
indices), so the output loop fills `out` to `numNeeded` and exits on its first condition; it also
refuses to widen when the caller's `out` capacity is below `numNeeded`. `taken`/`out` are never
written.

Run: `3dfd2249,0g_swamp_hall`, `swarmbasics_swamp.con`… `tools/drivers/swb12.con`, `--fast`,
1200 frames, `/tmp/opencode/swb13/a`: `game rc=0`, `caught/reach-stub: 0`, normal inferior exit,
639 applies.

| evidence | count |
|---|---|
| `RESTORE-BOID … OK` (words + `mActive` identical to the snapshot) | **1243** |
| `RESTORE-BOID … MISMATCH` / `FAILED` | **0 / 0** |
| `RESTORE-GUN … OK` (element words + `mCount` identical) | **603** |
| `RESTORE-GUN … MISMATCH` / `FAILED` | **0 / 0** |

Coverage: **26/26 lines, 28/28 branches, ALL-ARMS** (the last arm, 1682:1/4 - the
`out.size() < numNeeded` false exit - turned with the widened cone). Marked `// @ fn_80_1978`
(`src/MetroidPrime/Enemies/CSwarmBasics.cpp:1658`; the run's numbers are the pre-marker ones, the
marker shifts the body by one line).

`asmcheck.py check SwarmBasics … fn_80_1978` (all tags) gives three CALL findings, all template
instances of `rstl::vector<pair<uint,float>>` on the local candidate list: `0x1A24` `bl reserve`
(`candidates.reserve(mBoids.size())`, line 1666), `0x1B38` `bl rstl::sort<…>` (line 1681), `0x1BD4`
`bl __dt__…vector<pair<uint,float>>` (the local's destructor). No MASK/BIT/CR/STORE findings.

Gates: host build `metroid_prime2_port` 0 FAILED; `gen_files_cmake.py --check` rc 0;
`review_decomp_diff.sh 3a921d5dc` (one `MARK`, nothing else); matching `configure.py && ninja` and a
forced `build/G2ME01/ok` re-check **87 files OK**. Evidence archived before the marker
(`/tmp/opencode/reports/swb13-evidence/`: gcno, the run's gcda, raw gcov JSON, `swb13_geom.log`,
`binary.sha256`, `source.sha256`).

Static pre-run check of the same fixture (emulated gdb over a model that stores boid memory as raw
words): all phases leave every boid's words and `mActive` bit-identical, `taken=[]` and an
out-of-range entry move nothing, phase 3 moves 5 (taken + 4 non-taken), and the gun restore is exact -
`/tmp/opencode/swb13-static/check.py` → PASS.
## swb14: UpdateSeekerTargets 46/46 lines, 45/47 branches - marked, two arms proven dead

Base `b6390eb78`, branch `decomp/swb14`. One launch of the two allowed.

The four arms the swb13 profile left open (its numbering; +1 after this slice's marker) were:

| arm (swb13) | line | why it was open | swb14 |
|---|---|---|---|
| 2113:1 | `for (uint j = 0; j < numTargets; ++j) {` in the `numNew` loop | the loop's guard exit (no match) | **dead**, proof below |
| 2122:1 | the same guard in the `numNew..numLost` loop | the same | **dead**, proof below |
| 2124:5 | `if (CActor* act = TCastToPtr<CActor>(mgr.ObjectById(mSeekerTargets[j])))` | the null-actor side | **taken** |
| 2133:5 | the same `if` in the final loop | the null-actor side | **taken** |

**How the two null-actor sides were reached.** `tools/cov/specs/swarmbasics_seeker.py`'s
`caller_fixture` now has three phases per `UpdateSeekerTargets` call: ph0 supplies one real seeker uid
in the gun (the caller then builds a non-empty `keptBoids`), ph1 arms the **guarded callee-answer**
below, ph2 does neither; every call gets a window that closes at its own return. For ph1 the spec
picks one real `kMT_SeekerTarget` uid (live actor, valid `mSeekerBoidIndices` entry) and, only inside
that window and only for that uid, substitutes the `uid` argument of
`CObjectList::GetObjectById` with `kInvalidUniqueId` (`0x1CA4` `cmp %si, kInvalidUniqueId` /
`0x1CAE je` → its own null return). `TCastToPtr<CActor>` then evaluates on a **real null pointer**, so
the `if (act)` predicate is the game's own code. Nothing is deleted or dangled, no vector, count or
descriptor is written, and only a register argument of that one call changes - no persistent state, so
there is nothing to restore (the next call gets the natural answer). The fixture logs
`FORCE-NULL uid=… -> 0x…` (capped at 4 lines), and every *memory* write it makes (the gun element in
ph0) is restored with readback as before. Two fail-safe/tooling additions from this slice: the window's
closer has an `out_of_scope` (gdb calls it if the frame is discarded abnormally), which closes the
window and disables the null-answer breakpoint so a fixture can never leak armed across an unwind and
logs `WINDOW-UNWOUND`; and an `exited` handler writes one `SUMMARY exit:` line with the exact counters
(`callerCalls`, `gunWrites`, `nullArmed`, `nullHits`, `windowOpens`, `windowUnwound`, `assignCalls`,
`applies`, `restoreFail`, `err`) - the per-hit lines stay capped, and no frozen run's log is re-read as
counts.

Run: `3dfd2249,0g_swamp_hall`, `tools/drivers/swb12.con`, `--fast`, 1200 frames,
`/tmp/opencode/swb14/a`: `game rc=0`, `caught/reach-stub: 0`, normal inferior exit. **46/46 lines,
45/47 branches**; the only two left are the inner searches above. The per-hit log lines are capped
(`NULL-PHASE` at 3, `FORCE-NULL` at 4), so the run proves **at least 3 armed/phased windows and at
least 4 substituted lookups** - not exact counts; exact counters exist only from this slice's new
`SUMMARY exit:` line, for future runs (the frozen run's log cannot be re-read as counts). Expectation met for AssignSeekerBoids as well (unchanged, still 26/26 28/28 - not re-counted).
Readbacks: **629** `RESTORE-BOID OK` and **117** `RESTORE-GUN OK`, all word-for-word identical to
their snapshots, **0** mismatches/failures.

**The two dead arms** (2114:1 and 2123:1 in the run's numbering; 2115:1 and 2124:1 after this
slice's marker) are the two exits of the two inner `for (uint j = 0; j < numTargets; ++j)` searches.
Each search has two no-match exit paths; neither is reachable (the successful-match break is):

- **zero-trip exit** - `0x2CC cmplwi r29,0` with `0x2D0 ble .L_00000308` (the `numNew` loop) and
  `0x33C cmplwi r29,0` with `0x340 ble .L_000003A8` (the `numNew..numLost` loop). It needs
  `numTargets == 0`, which makes the initial fill produce `numLost == 0`. The bounded
  `AssignSeekerBoids` output then gives `numNew == 0`, so neither outer loop enters an inner
  search. This does not require assuming creation has already populated exactly five targets.
- **exhausted-search fallthrough** - after `0x304 bdnz .L_000002D4` (the `numNew` loop) and
  `0x3A4 bdnz .L_00000344` (the `numNew..numLost` loop), i.e. the "no match found" way out. It needs a
  `lostTargets` entry that is in no slot of `mSeekerTargets`. `lostTargets` has one writer - the
  `push_back_unsafe(uid)` at `0x250 stw` (count) / `0x254 sthx r8,r7,r0` (element), whose value is
  `uid = mSeekerTargets[i]` (`0x1E8 lhzx r8,r6,r4`) - so every element of `lostTargets` is a copy of an
  element of the same `TUniqueId` vector the searches compare against (`0x570(r27)` in both). Between
  the fill and the searches the only call is `AssignSeekerBoids` (`0x2A0 bl`), whose body references
  `mSeekerTargets` nowhere (audited: it reads `mBoids`, the camera and its own parameters, and writes
  only `out`), and it bounds `numNew <= numLost` (its `out` fill is guarded by `out.size() < numNeeded`
  with `numNeeded == numLost`), so every index either loop visits stays inside `lostTargets`. With
  `mSeekerTargets` unchanged and every entry present, the compare (`0x2DC cmplw` / `0x34C cmplw`)
  matches at some `j` and the `break` (`0x2F8` / `0x398`) leaves the loop.

For contrast the *first* search (the gun's targets) has a reachable zero-trip exit - `0x1F0 cmplwi
r30,0` with `0x1F4 ble .L_00000238`, taken whenever the gun has no seeker targets, i.e. the normal
case - and a reachable `0x234 bdnz .L_000001F8` exhaustion; both are covered.

`asmcheck.py check SwarmBasics … fn_80_168` (all tags): four CALL findings, all library instances -
`0x1BC`/`0x1C8` `reserve` of the `TUniqueId`/`uint` vectors, `0x45C`/`0x474` their destructors.
Marked `// @ fn_80_168` (`src/MetroidPrime/Enemies/CSwarmBasics.cpp:2084`).

Gates: host build 0 FAILED; `gen_files_cmake.py --check` rc 0; `review_decomp_diff.sh b6390eb78` (one
MARK); matching `configure.py && ninja` + forced `build/G2ME01/ok` re-check **87 files OK**. Evidence
archived before the marker: `/tmp/opencode/reports/swb14-evidence/` (gcno, gcda, raw gcov JSON,
`swb13_geom.log`, `binary.sha256`, `source.sha256`).
## swb15: UpdateAllBoidMovement 7/7 lines, 6/6 branches measured - NOT marked

Base `9794622f0`, branch `decomp/swb15`, two launches (the cap). The coverage numbers stand - the
compiler counters do not depend on the fixture - but the run's **oracle accounting does not close**,
so the marker was removed again (added in `a5eedafae`, removed in `0304657f7`); subsequent tooling-only corrections do not re-add it, and this
body stays unmarked until the anomaly is explained.

**Why this body needs its own area.** `UpdateAllBoidMovement` is the base-class default of a virtual
and every shipped swarm class overrides it (CBacteriaSwarm, CMetareeSwarm, CIngBlobSwarm,
CPlantScarabSwarm, CEmperorIngLightSwarm each have their own out-of-line body); CFlyerSwarm does not,
so only a CFlyerSwarm reaches the base body - through Think's virtual call. Agon
`42b935e4,01_sand_entrance` has four, naturally `mAnimated = 1` with 5 or 7 boids and 4
model/advancement entries (`mask = 3`).

**Run `b` (raw, as logged)**: 4325 body calls, `game rc=0`, `caught/reach-stub: 0`, normal exit,
coverage 7/7 lines, 6/6 branches. Restores: 2880 `OK` + 1445 `NATURAL` = 4325.

**Reconciliation of the oracle log** (`/tmp/opencode/swb15-reconcile/`, mechanical, from the raw
`swb15_geom.log`): 4325 window opens, 4324 `MOVE-LOOP` closes (4323 `OK` + 1 `MISMATCH`) and 4325
restores. Exactly one tag differs: **`swb15-2861-ph2` opened and restored but never closed**. The
cause is visible in the raw order: two calls of the **same** swarm entered before the first one's
finish fired -

    swb15-2861-ph2 FORCED swarm=0x555558777ba0 ... mBoids.mCount 7->0   (open)
    swb15-2862-ph0 FORCED swarm=0x555558777ba0 ... mBoids.mCount 0->0   (second open, same swarm)
    RESTORE swb15-2861-ph2 OK ...            (closes the *global* window, i.e. 2862-ph0's)
    MOVE-LOOP swb15-2862-ph0 OK calls=0 expected=0
    RESTORE swb15-2862-ph0 OK ...            (window already consumed: no close)

The earlier claim of **33** "open while open" events is unverified: the independent raw-order
recount found one in run b (the pair above), not 33. The bad window `swb15-2819-ph1`
logs `i=7 MISMATCH`, 8 calls for 7 boids; indices 3–6 were not printed, so a repeated
`i=6` and attribution to a second window cannot be established from this log.
The fixture had the structural defects the review names - a
singleton `_st['window']`, a `FinishBreakpoint` that closes whatever window is global, no caller/frame
ownership, no `out_of_scope`, no pending-window check at exit.

**What the raw evidence does *not* resolve**: *why* two entries of the same body overlap. The nm table
has exactly one symbol for the body (and one for `MoveBoid`), which rules out a duplicate from the
symbol *match* but NOT several breakpoint locations at that one symbol (or a late/duplicated gdb
finish); the log records no frame identity, so a late/duplicated gdb finish and a
genuinely re-entrant call cannot be told apart offline. Per the review rule ("if raw evidence cannot
uniquely resolve the anomaly, remove the marker") the marker is out.

**Second offline correction (Sol, from `5ff64187a`, ZERO launches):** the previous
"level and name" ownership rule was itself wrong. At UpdateAllBoidMovement entry
the owner is level **0**; inside MoveBoid the very same caller is level **1**.
`/tmp/opencode/swb15-independent-frame/` reproduces old accepted0/unattributed1.
The previous mocks concealed this by giving the body level1 even at entry and
creating a separate same-level caller rather than modelling a changing stack.

The corrected `tools/cov/specs/swb15_updateall.py` retains each actual `gdb.Frame`
and uses **GDB frame equality** against MoveBoid's **immediate** older frame, with
validity/name checks and matching selected inferior number / thread global number.
It does not substitute Python object identity, a raw stack-pointer/PC heuristic,
`this`, name or depth for activation identity. Levels are diagnostic only. Fresh
Python wrappers for the same activation may compare equal; recursive same-this
activations remain distinct even when they have the same function name and each
entered at level0. Missing/unreadable/invalid frames or equality errors, wrong
context, zero owners and multiple owners reject attribution without incrementing
the window's MoveBoid counters. An unreadable possible owner does not fall back to
another candidate. Duplicate entry for the same real activation is refused before
any write; later entries on another activation are not incorrectly rejected.

Each record still owns its FinishBreakpoint and original pre-write values. Restore
is armed before **attempting** mutation, including a write that changes memory and
then raises. Write/readback failure rolls back immediately; both fields are always
attempted and read independently even if the first operation fails. Failures remain
failures even when an operation raised after its value landed. Unwind now restores
the window's forced fields, not just its bookkeeping. Out-of-order finish fails and
cancels/restores younger windows in LIFO order before the older snapshot, disables
their finish breakpoints, and records them as cancelled/unwound. Later or duplicate
callbacks do **not** replay stale snapshots or double-count closes. If bookkeeping
order is ambiguous or the current restore context differs, the fixture does not
guess or write through an uncertain owner: it records failure and poisons further
forcing. A failed restore likewise poisons new writes; abnormal normal-return
callbacks request a debugger stop. No restoration is claimed without both exact
field readbacks. Absolute stored target expressions, not current argument registers,
locate the original fields at return. Unrelated bitfield/storage bytes are preserved.

Summary fails on ownership/argument/count errors, unwind, cancelled/out-of-order/
late callbacks, refused writes, rollback/restore/readback failures, missing restore
breakpoints, pending records, unbalanced window counters, or a no-window run. Pending
summary accounting is idempotent (repeated summaries cannot multiply pending records).

The new offline suite (`tools/cov/specs/swb15_updateall_selftest.py`) passes **33
unittest cases**, including parameterized subcases. Its stack computes levels from
the **current activation stack** on every read and yields new Frame wrappers from
newest_frame/older. Memory is byte arrays at addresses **above 32 bits**; field
expressions resolve their encoded target addresses, not `$rdi`. Tests assert accepted
and rejected attribution, exact whole-memory bytes (including unrelated flag bits),
and counters for all phases, recursive same-this and distinct owners, duplicate/
ambiguous entries, invalid/unreadable frames/context/equality, thunk/wrong caller,
unwind, out-of-order cancellation and late callback suppression, mutations raising
after landing, silent write/readback failures, independent rollback attempts,
poisoned-write refusal, missing return arm, pending/idempotent/empty/unbalanced
summaries, and argument mismatch. Failed restoration tests assert the exact remaining
non-original memory and FAIL counters, not a fabricated successful rollback.

This is **offline tooling correction only, NOT runtime validation**. The two-game-
launch cap remains exhausted; no game, build, smoke or new coverage run was made.
The historical overlap-count claim, unmatched open and argument mismatch remain **unresolved**; the
raw counters/readbacks are not rewritten, and fn_80_5024 stays **UNMARKED**. Offline
gates: both Python files compile, the 33-case suite passes, manifest check and
review_decomp_diff pass; final source/header diff versus `9794622f0` remains empty.
Report/tests/diff: `/tmp/opencode/reports/swb15-sol-fix.md` and
`/tmp/opencode/swb15-sol-*`. Prior evidence and configure log are preserved.
Independent review: `/tmp/opencode/swb15-review-astra/` preserves the rerun of all 33
tests, git-blob dynamic regression and recount scripts. The 15 archived hashes matched;
the changing-stack regression rejects the baseline and accepts the correction with
exact memory restoration. This models frame equality, not real GDB unwinder lifetime.

The body's asm shape was checked independently of the fixture: `0x5044 lbz r0,0x4f0(r3)` +
`0x504C extrwi. r0,r0,1,27` (mAnimated, mask 0x10), `0x5050 beq` to the return, `0x5054` the model
count, `0x5060 subi` the mask, `0x5068-0x509C` the loop (`and`/`mulli 0x1c`/`lwz` items/`bl MoveBoid`/
`cmplw`+`blt`). `asmcheck.py check SwarmBasics … fn_80_5024` needs a marker to map the body, so it was
run against the marker commit `a5eedafae`: no findings, one `NOPOS` note (the header states no bit for
`mAnimated`).

Evidence frozen before the marker went in, and untouched by this correction:
`/tmp/opencode/reports/swb15-evidence/` (gcno, gcda, raw gcov JSON, the module's asm, binary/source
hashes, the raw fixture log, the run log, mock tests); the mechanical reconciliation and the hardened
fixture's mock suite are in `/tmp/opencode/swb15-reconcile/`.

## swb18: FindClosestCell 12/12 15/15 (ALL-ARMS) and UpdateEffects 26/26 32/34 - both marked

Base `0d8d3aa9f` (298119d8b + a `port` merge), branch `decomp/swb18`. Source change: two `// @`
marker lines only. Six launches (recorded, not hidden): run 1 died in an object-list walk
(`gdb.MemoryError`, nothing logged), run 2 passed a `CTransform4f` to the CVector3f helper (`no
member mX`), run 3 raised `NameError: lo` in the cell-centre helper - three fixture bugs, each
fixed in its own commit; run 4 was a hard-check launch that stopped on an oracle error of mine (the
cell-centre margin, below); run 5 was the `SWB18_SOFT=1` diagnostic, where every arm ran, the whole
fixture completed and the measured centres named that bug; run 6 is the accepted one: `game rc=0`, `caught/reach-stub: 0`, normal inferior exit, `closest=6 effects=8
sound_calls=3 applies=3 restores=3 restore_fail=0 release_fail=0 soft_fail=0 done=True`
(`/var/tmp/swb18/run6`, copy in `/tmp/opencode/reports/swb18-evidence/`).

`tools/cov/specs/swarmbasics_swb18.py` (force.py, not force_ret.py) is a **forced native call**
fixture, the approach swb4/bsw4 used for bodies no run reaches: a one-shot breakpoint at
`CSwarmBasics::UpdatePartition` (base-class, so every derived swarm reaches it) waits for
`mUpdateFrameIdx >= 500`, >= 2 boids, a non-empty `mActiveBoidIndices` and a live
`mModelDatas[0].mAnimData.mItem`; the work then runs in one batch command outside `stop()`
(`hook-backtrace` -> `swb18-run`) with frame ownership (same thread, same pc, same level), and
every write sits inside a `Transaction` that snapshots the original bytes **before** the first
write and restores them reversed with a byte readback.

| body | result | how |
|---|---|---|
| **fn_80_10C0 FindClosestCell - marked** | **12/12 lines, 15/15 branches, ALL-ARMS** | six native calls: empty list (result stays zero), a later cell closer (update), the first cell already closest (strict `<` keeps it), an exact midpoint tie (first wins), a query 1e6 away, and an unordered (NaN) query (no update, zero returned). The partition list and the box geometry are forced (`mTransform` identity, `mBoundingBoxExtent` 50, two heads at slots 0 and 4) and the expected centres are recomputed here, never by calling `BoxForPosition`/`FindClosestCell` |
| **fn_80_7E4 UpdateEffects - marked** | **26/26 lines, 32/34 branches** | eight native calls on the **swarm's own** CAnimData (`mModelDatas[0]` - the object the natural caller passes): zero count, wrong node type, `roll > weight`, character mismatch, the `0x80000000` sound-id bit, character -1, character == the anim's own index, and two nodes. Every emitted emitter is read at `CSfxManager::AddEmitter` and released through the native `RemoveEmitter` |

The two untaken edges are **GCC host-only static-guard contention**, not retail arms: they are
`branch 4 taken 0` on lines 2012/2013 (`static float maxDistance = node.GetMaxDistance();`,
`static float falloff = node.GetFallOff();`), i.e. the acquire-returns-0 side of
`__cxa_guard_acquire` (blocks 11 -> end and 15 -> end; `call 2 returned 2`, `branch 3 taken 2`,
`branch 4 taken 0`). Retail has only the guard byte load/test/store and both of its sides ran -
init 2 / skip 56 at `0x8E0 bne .L_000008F8` with `0x8E4 stfs lbl_80_bss_0` + `0x8F4 stb 1` and the
same at `0x908 bne .L_00000920` with `0x90C stfs lbl_80_bss_8` + `0x91C stb 1`; there is no
lock/acquire-return branch. This is the same proof bsw4 recorded for `fn_6_1448`, and the payload
oracles show the statics hold: the first emitting call takes `maxDistance 17 / falloff 0.25` from
the node and the two later ones emit 17/0.25 again although their node says 31/0.75 and 45/0.5.

**Verified layout facts (host, from the build's DWARF, not retail offsets):** `mBoids` 0x1d8,
`mPartitionedBoidLists` 0x240 with `mData` at +8 (so `reserved_vector::operator[]` is
`mData + idx*8`), `sizeof(CBoid)` 192, `CBoid`: mTransform 0, mVelocity 0x30, mNext 0x58, mActive
0xba; `CAnimData::mPassedSoundCount` 0x418, `mCharIdx` 0x404, `CSoundPOINode`: mType 0x10,
mWeight 0x24, mCharIdx 0x28, mSfxId 0x30, mFalloff 0x34, mMaxDist 0x38 (retail 0xc/0x20/0x24/0x2c/
0x30/0x34: same order, +4); swarm `mMaxVolume` 0x7f1, `mMinVolume` 0x7f0 (retail 0x525/0x524,
same order). **`BoxForPosition`'s 0.1 margin cancels out of a cell's centre** (retail `0x1150 lfs
f1, lbl_80_rodata_4` is the margin): the box is `min - margin` .. `max + margin`, so the centre is
`min + (idx + .5) * size` - run 5 measured (20.0,-20.0,-20.0) where an uncancelled margin predicts
19.9. `tools/cov/specs/swarmbasics_swb18_selftest.py` (committed) covers that arithmetic, the
strict-`<` first-wins rule, the NaN query and the `x + 5y + 25z` slot order without gdb or a game.

Still open in this slice, with what is measured and what each needs:

| fn | state | blocker |
|---|---|---|
| 220C Touch | 48/72 lines, 49/93 branches, unmarked | 93 branch rows over three independent paths: the `CLightComboProjectile` arm (`CanCreateRay`, `CreateRay`, the `xa8_`/`xaa_` pair) needs a real Sunburst projectile in the air; the `CEnergyProjectile` arm needs a charge/beam projectile hit plus its explode-reflect `kWCR_EnemyNormal` branch; the player path needs the cannon-ball (`kIT_CannonBall`) and screw-attack states and the `mDamageCooldownTimer` arm. A forced-call fixture is not enough here: it has to drive real projectiles through the real collision callbacks, so this body needs its own slice (a Sunburst/seeker driver plus a `Touch` spec), not a row in this one |
| A1BC GetModelDataForAnimRes | inlined into the ctor (objdiff None, no gcov entry), unmarked | the body is `animRes.GetId() != kInvalidAssetId ? CModelData(animRes) : CModelDataNull()`, and a `static` free function inlined into its only caller cannot be called natively. The invalid-id arm needs a **constructor call with an invalid asset id** - i.e. a real actor whose SLdr resolves no model - which no shipped swarm provides, and the ctor's own marker (fn_80_8FE0) does not distinguish the two sides. Needs either a `-fno-inline`-style build of just this unit with a documented reason, or a fixture that calls the ctor with a fabricated `CAnimRes` (a real caller-visible path, so it needs a decision by the orchestrator, not this lane) |

## swb17: Render 53/53 lines, 56/56 branches; HardwareLight 8/8, 5/5; PreRenderBoid 8/8, 8/8 - all three marked

Base `298119d8b`, branch `decomp/swb17`. Fixture `tools/cov/specs/swb17_render.py` (selftest
`swb17_render_selftest.py`), drivers `tools/drivers/swb17.con` (Torvus `3dfd2249,0g_swamp_hall` layer 3,
CPlantScarabSwarm) and `swb17_sand.con` (Agon `42b935e4,01_sand_entrance` layer 2, CFlyerSwarm). Merged
gcda of runs 3 + 6 + 9 (`gcov_report.py build-port-cov <run3> <run6> <run9> --module SwarmBasics`).

- **Render windows** (every 2nd Render call, first active in-frustum boid = outlier head, retained frame,
  `_Finish` armed before the writes, LIFO restore with readback): phases A-G force
  `mEnableLighting`/`mUseSoftwareLight`/`mThinkCounter`/head `mInFrustum`/`mActive`, F clears the head's
  `mActive` when SoftwareLight returns (the 1227 arm); H forces GetRenderAlphaBufferAlpha to 0; LA/LD/LB set
  `CGraphics::mLightActive=1` after DisableAllLights returns (the 1178-1180 light-combo arm; Render reads
  `GetLightMask` after it and it is otherwise always 0); N nulls `mDisplayList.mPtr` (1175 false arm).
  The oracle counts Hardware/SoftwareLight hits per window by caller-frame ownership and compares with
  the closed form from the partition cells and the outlier chain: 0 count failures, 0 restore failures.
- **N only on a CFlyerSwarm** (`SWB17_DL0=1`): only the base `RenderBoid` has the TARGET_PC null guard;
  CPlantScarab/Metaree/Bacteria/IngBlob/SnakeWeed/LightSwarm overrides draw through the list (run4 crashed
  in `CSwarmDisplayList::Draw this=0`). Run 6: all 12 phases OK, 6 N windows.
- **PreRenderBoid 1072 (idx<32)**: `tools/scan_swarm_anims.py` - all 42 shipped swarm placements (34 FSWM,
  4 IBSM, 1 MSWM, 3 PSSM) have a valid ANCS, so the model list is non-empty and `idx <= 3`: the false side is
  a host-only guard for PPC `slw >= 32 -> 0`. Not reachable by shipped content; forced anyway: the `empty`
  window writes `mModelDatas.mCount=0` and `mIndex=40` on the argument boid (read back, restored at return):
  279 windows in run 9, no CachePose call, no fault. The `freeze` arm (68+ natural hits) is the other edge.
- asmcheck: HardwareLight and PreRenderBoid clean. Render reports 3 benign findings: two `BIT 0x3D2C` are
  `clrlwi r0,r0,30` of `x170` = `mThinkCounter & 3` (misread as x173 bits), `XCALL RenderBoid` is the
  `bctrl vcall(this)+0xa8`; the two NOPOS lines are the bitfield `mEnableLighting`/`mUseSoftwareLight`.

## swb16: Think 98/98 lines, 76/76 branches; UpdateAllBoidMovement 8/8 lines, 4/4 branches - both marked

`fn_80_50B8` `CSwarmBasics::Think` and `fn_80_5024` `CSwarmBasics::UpdateAllBoidMovement` are both
measured in one binary on real arms. Both are now marked.

**Binary.** `build-port-cov` (`-DMP_PORT_COVERAGE=ON`) is frozen as `/var/tmp/swb16/frozen/metroid_prime2_port`,
sha256 `9df975473111b1adc6c648d8ab9ff33cc95018cb63d70933d3da85dd7f607e03`. Its gcno is
`/var/tmp/swb16/gcn/CSwarmBasics.cpp.gcno`, sha256 `f813d456b80495e490b80bf47eb67bba685cf7ceaab66398a1b72b8c85f71161`.

In this build UpdateAllBoidMovement is out of line (`e31410`). Think reaches it only through the
virtual call at Think+0x595, which returns to +0x59b. Retail reaches it the same way, a `bctrl` through
the vtable. The evidence therefore comes from a non-inlined build, and no `noinline` or source change
was needed.

**Fixture** `tools/cov/specs/swb16_think.py`, loaded by `tools/cov/force.py`:

- **What it writes.** It only writes `$eflags`, and only at 20 conditional jumps whose flags are dead at both
  successors: the next instruction executed writes flags, or is a call or ret. Each original eflags value
  is restored at the successor breakpoint, and every write is read back.
- **Members written.** The forced occlusion paths write `mOccludedTimer` (0x1d4, line 769) and the S8 path
  writes `mBoidGenCooldownTimer` (0x63c, line 808). Both are saved raw at plan start and written back,
  then read back, before the activation leaves the path. S18 only records `x52c_` (0x7f8) and
  `mCreatedBoids` (0x7a4) and checks them at T+0x2bf, where both exits of the 814 create loop join.
  It does not write them.
- **Guarded ownership.** The Think body rsp is fixed (6 pushes + 0x2528), so CFA = rsp+0x2558.
  - Entry at T+0x83 (after GetActive) opens an activation: thread, CFA, `this`.
  - Every site requires the same thread, the same CFA and the same `this` register.
  - Exit at T+0x5a (`e3729a`) closes it, and fails the run if a restore is still pending.
  - UpdateAll is owned only when its return address is T+0x59b, rsp+8+0x2558 == CFA, and rdi == this.
    Both U sites sit before its first call, so no nested frame is involved. This is the swb15
    MoveBoid-level confusion, avoided.
- **No inferior calls in stop().**
- **Static check at load.** Each site's mnemonic and target is checked by `x/2i` against the table, and any
  mismatch disables all forcing. A dry run on the frozen binary (`/var/tmp/swb16/dry{,2,3}`) verified
  all 21 sites.
- **Stale comment.** The header comment still says "epilogue Think+0x51". The code uses `EXIT=0x5a`.

| site | off | jcc | line | forced |
|------|-----|-----|------|--------|
| S1/S2 | T+0xe1 / T+0x8d3 | je / jne | 767 IsLoaded false / not Visible -> occluded | 8 each |
| S3/S4 | T+0xf6 / T+0x116 | jbe / jae | 768 timer > 0, 771 timer <= 0 return (inverted) | 8 each |
| S5 | T+0x128 | jne | 774 mThinkCounter & 2 (adaptive) | 1 + 3 taken / 4 fall natural |
| S6 | T+0x1e9 | jne | 787 !x4f1_25_ -> RemoveMaterial | 3 |
| S7/S7b | T+0x918 / T+0x93a | je / jb | 795 mMaxCreatedBoids == 0, 796 loop exit | 3 each |
| S8 | T+0x9ab | jne | 799 boid active -> 807/808 cooldown | 3 |
| S9-S13 | T+0x86a..0x7ba | je/jne/je/ja/je | 835 ShouldBuild, 840/843/844/845 partition loop | 3 each |
| S14-S17 | T+0x419..0x469 | jne/je/ja/je | 855/858/858/859 outlier loop | 3 each |
| S18 | T+0x29b | jle | 818 x52c_ > 0 -> break | 3 (force3) |
| U1/U2 | U+0x14 / U+0x3b | je / je | 883 !mAnimated -> ret, 885 count == 0 | 3 each |

**Runs.** All on GPU (Xvfb), never headless, with `42b935e4,01_sand_entrance`,
`--con tools/cov/specs/swarmbasics_sand.con --fast --frames 3000`.

Each run dir keeps the fixture copy, `fixture.sha256`, `fixture.commit`, `binary.sha256`, `dry-verify.log`,
`swb16.log`, gcda and game logs. The dawn/pipeline caches were deleted.

| run | fixture commit | fixture sha256 | result |
|-----|----------------|----------------|--------|
| `/var/tmp/swb16/force1` | `21b7cf147` | `ad0f1bc4…` | FAIL (own criterion) |
| `/var/tmp/swb16/force2` | `fdb7daac5` | `c20cc49e60c3f54f0099535fd423029fbb50df3aedd0f2200f59a870924ab116` | PASS |
| `/var/tmp/swb16/force3` | `83126976f` | `d894d553aaad0d83a2bb28ece85294284a2f2ee9f2e0b388f95fd5a2dbedf7b2` | PASS |

- **force1** put its exit hook at T+0x51, which only the GetActive-false path reaches. It saw 11285
  opens against 1693 closes, so its gcda is not used. Its forcing was still correctly owned.
- **force2** first forced S18 to fall through, but the natural outcome is always fall-through, so it
  never fired. Results: opens = closes = 11285, unattributed 0, fail 0, uOwned 11274 / uForeign 0,
  flagRestored 78, memRestored 18.
- **force3** forced the break instead. Results: the same counts, with flagRestored 81. It left
  Think at 75/76 by itself.

**Coverage.** The force2 and force3 gcda were merged with `gcov-tool merge` into
`/var/tmp/swb16/m23`. gcda sha256s:

- force2: `c746c3ae…3728`
- force3: `28a40bbe…8581`
- merged: `fdb25bf306c9e29f492a67929c841e70a3a90792a7e80b8ee53c38ab9853a3e2`

gcov output is in `/var/tmp/swb16/gm23`:

- Think: lines 98/98, branches executed 76/76, taken 76/76, calls 29/29.
- UpdateAllBoidMovement: lines 8/8, branches 4/4, calls 1/1.

**UpdateAll asm vs source** (lines 883-890):

- `0x5044 lbz 0x4f0` + `0x504C extrwi. 1,27` is mask 0x10. mAnimated is the 4th bitfield of 0x4f0.
- `0x5050 beq` goes to the return.
- `0x5048 lwz 0x17c` is the boid count. `0x5054 lwz 0x490` / `0x5060 subi` is the model-count mask.
- The loop is `0x5068-0x509C`: `and` with the mask, `mulli 0x1c` into deltas at 0x4a8, boid stride
  0xb8 from 0x184, `bl fn_80_4D8C` (MoveBoid), then `cmplw` + `blt`.
- This is one-to-one with the source.

**asmcheck triage** (`check SwarmBasics`, both markers). Every finding is an auto-name or call-kind
false positive:

- `BIT 0x5100 clr x4f0_01` is `mBoidUpdatedThisFrame = false` (line 760). It is the last, LSB
  bitfield at 0x4f0.
- `BIT 0x51AC x173_02` and `0x554C x173_01` are int masks on `mThinkCounter` (0x170), not byte bits:
  `& 2` at line 774, and `(mThinkCounter & 1) == (i & 1)` in the partition and outlier loops.
- `CALL 0x54CC fn_80_5820` and `0x54D8 fn_80_5C8C` are the weak `CAreaCollisionCache` copy ctor/dtor
  (TSV rows 05820/05C8C). They come from the partition and outlier cache locals.
- The XCALLs CreateBoid, GetBoundingBox, ShouldBuild..., UpdateAllBoidMovement, UpdateBoid,
  UpdateClosestPartitionLoopedSounds and UpdateSwarmAnimations are all virtual (header 125-154), and
  retail calls them with `bctrl`.
- `NOPOS mAnimated` means the header states no bit position for mAnimated. The 0x504C extract above
  fixes it at mask 0x10.

Neither function has a dead arm. Source and header are unchanged apart from the two markers.
Report: `/tmp/opencode/reports/swb16.md`.

## swb19: GetModelDataForAnimRes marked (both arms), Touch 64/93 branches - not marked

Branch `decomp/swb19` from `255e5d22d`. Source change: one marker line. Not touched: Think,
UpdateAllBoidMovement, Render, HardwareLight, PreRenderBoid. Runs live in `/var/tmp/swb19/run{1,3,4}`
(evidence copy `/tmp/opencode/reports/swb19-evidence/`), all `42b935e4,01_sand_entrance`,
`tools/drivers/swb19_touch.con`, `--fast`, `game rc=0`, `caught/reach-stub: 0`.

### fn_80_A1BC GetModelDataForAnimRes - MARKED, both arms, forced

The helper is a `static` inlined into its only caller, so it has no symbol to call and no gcov row
of its own; its code is attributed to **source line 188 inside the constructor's initialiser list**.
Evidence (`build-port-cov`, gcno frozen before the marker,
sha256 `16e527be2b9c10a0d4c30215889e8d65936e261a31b67ad4af9e965b83b5e728`):

| arm | asm (host, inlined) | gcov line 188 |
|---|---|---|
| valid `CModelData(animRes)` | `0xe39734 cmpl $0xffffffff,(%rsi)` then `0xe39783 call CModelData::CModelData(CAnimRes const&)` | `branch 0 taken 5`, `call 2 returned 5` |
| invalid `CModelDataNull()` | `0xe39763 je` past the ctor call to the null copy | `branch 1 taken 1` (forced) |
| host EH unwind edge | - | `branch 4 taken 0` (a throw edge; nothing in the arm throws) |

Retail has the same body **out of line** at 0xA1BC (the ctor calls it: `0x9020 bl fn_80_A1BC`):
`0xA1C4 lwz r5,0x0(r4)` (the CAnimRes's first word, `mAncsId`), `0xA1CC addis r0,r5,0x1` /
`0xA1D4 cmplwi r0,0xffff` = `id == kInvalidAssetId` (0xFFFFFFFF, the `addis` idiom), `0xA1DC beq
.L_0000A1E8`; valid -> `0xA1E0 bl __ct__10CModelDataFRC8CAnimRes`, invalid -> `0xA1E8 bl fn_80_87B8`,
which is the local 5-instruction `CModelData::CModelDataNull()` (`bl __ct__10CModelDataFv`; TSV row
087B8, class H). Same two arms, same order, same target functions.

Fixture (`tools/cov/specs/swarmbasics_swb19.py`): a **file:line breakpoint on line 188 does not bind**
("No compiled code for line 188", run 1: `ctor_sites=0`), so the fixture finds the single
`cmpl $0xffffffff,(%rsi)` in the ctor's own disassembly (logged as `SITE`, ctor+68) and sets it as
`symbol+offset`, which relocates with the PIE. At that stop it writes `mAncsId = 0xFFFFFFFF` through
the `animRes` reference, reads the write back, and restores the original word with a readback at the
constructor's return. Run 4: `FORCE anim_id_before=0xBFABAEEE written=0xFFFFFFFF` then
`RESTORE readback=eeabaabf (0xBFABAEEE) model_datas=0` - the invalid arm's *effect* (a null
CModelData) read on the actor. `ctor_sites=6`: the other five constructors took the valid arm
naturally, so both sides of the ternary ran in one run.

**Port finding (asked for, not fixed here).** With the null model the actor crashes the game:
`CSwarmBasics::AllocateSkinnedModels` (CSwarmBasics.cpp:422) runs `mModelData->PickAnimatedModel(which)`
with no null test, and the port's `CActor::mModelData` is a `rstl::single_ptr`, so a null model is a
null pointer there (retail's is a value; a default CModelData survives). Run 2 died exactly there
(`SIGSEGV` in `CModelData::PickAnimatedModel this=0x0`, backtrace in the evidence). The module's other
null-model paths already guard with `SwarmModelReady` (lines 278, 1018, 1053); `AllocateSkinnedModels`
is the hole. The fixture keeps the run alive *without* a source change by clearing `mAnimated` on that
one actor (a real non-animated swarm skips the body, `if (mAnimated)`) and restoring it 40 ticks
later - both logged. Suggested fix on `port`: `if (mModelData.null()) return;` at the top of the
`if (mAnimated)` block, quoting this evidence.

`asmcheck.py check SwarmBasics ... fn_80_A1BC`: one CALL finding, a tool limit - `0xA1E8 fn_80_87B8`
is `CModelData::CModelDataNull()` (`port`: `return CModelData()`), a host copy in another unit, not a
module-80 body.

### fn_80_220C Touch - 55/72 lines, 64/93 branches, NOT marked

Merged profile of runs 1 (natural, 3000 f), 3 (ctor fixture + stand-in boid, 3200 f) and 4 (aligned
windows + freeze/screw/spawn forces, 3200 f):
`gcov_report.py build-port-cov /var/tmp/swb19/run{1,3,4}/gcov --module SwarmBasics`. Not marked:
29 edges are left and none of them is proven dead except the two at 1493 (below).

What the runs did establish (all real objects, all state changes measured):

* `tools/cov/specs/swarmbasics_swb19.py` logs every `Touch` entry read-only: 240771 calls in run 4.
  The swarm's touch box is large, so everything inside it touches: the actor types seen are
  `CEnergyProjectile` (weapon modes 0/1/2/6 and charged 0/1/2/3), `CPlayer`, plus the area's static
  actors. **No `CLightComboProjectile` ever reached Touch** - every light charge shot was a plain
  charged beam (`w=65538`), so the light-combo block is untouched.
* A **stand-in boid**: the player's box overlap needs the player *inside* a boid (run 1: 3215
  player touches of the swarm box, 0 inside a boid). The fixture pins one live boid of every swarm
  in the area onto the player's `mPosition` for a window (boid translation snapshotted first,
  restored with a readback: 4 swarms, `glue_fail=0`). That is what took `1542:0`, `1543-1550`,
  the cooldown sides of `1541`, and the cannon-ball overlap arm.
* Real-state forces, each written to the field the body reads and restored with a readback:
  a frozen boid (`mFreezeTimer = 1`, FREEZE readback `0000803f`), the player in screw-attack ball
  mode (`mBallState = kBS_ScrewAttack`, SCREW readback `04000000`), and the player spawned morphed
  (`mSpawnedMorphBallState = kMS_Morphed`, SPAWN readback `01000000`). Each took its arm
  (`1537:3`, `1533:0`, `1527:1`/`1528:1`).

The 29 edges left, and what each needs:

| edges | arm | status |
|---|---|---|
| 1469-1477 (12) | `CLightComboProjectile` block (`CanCreateRay`, the ray loop, `CreateRay`, `xa8_`/`xaa_`) | needs a real Sunburst. `give sunburst 1; give chargeCombo 1; beam light; press a 0` held 200 frames then released (`stick 0 0`) never produced a comboed weapon mode; `shoot ... charged` bypasses `CPlayerGun`'s state machine. Next: hold the pad and verify `mChargePhase == kCP_Charged` in a `CPlayerGun` probe before release, or call `CAuxWeapon::FireLightCombo` with the aux weapon's own cached `CWeaponDescription` token (`mCombos[kBI_Light]`) |
| 1493:1/5, 1493:3/5 (2) | `if (energy && !TCastToPtr<CLightComboProjectile>(energy))` | **dead by construction**: this `else` branch is entered only when the same pointer failed the identical cast at 1469 |
| 1495:1/3, 1498-1500 (4) | `Explode` returned false -> the reflect block | the swarm's vulnerability table (logged per swarm: entries `1.0/kE_Normal`, `2.0/kE_Normal`, `0.5/kE_Normal`, `0.0/kE_Normal`; `mWeaponIndices` 0-10 -> entry 0, 11-20 -> entry 3) has **no `kE_Reflect` entry**, and Touch passes the constant `kWCR_EnemyNormal`, so `deflected` is false for every weapon. The other route, `Explode`'s first test, needs a *piercing* projectile that already has the swarm in `mCollisionCooldowns` |
| 1507-1509 (4) | the non-energy projectile path (`mHealth -=`, `KillBoid`) | needs a `CGameProjectile` that is not a `CEnergyProjectile`: the `CBeamProjectile` family (Emperor Ing's beam, Splitter's laser). Nothing in 01_sand_entrance fires one |
| 1464:1 (1) | `WeaponHits` false | weapon types 11-20 are `0.0/kE_Normal` for these swarms (logged); no player weapon has such a type. Needs an enemy projectile carrying `kWT_AI` or similar |
| 1521:1 (1) | `mPlayerTouchRadius != 0` | all four sand-entrance swarms have 0 (`touch_radius` 0.7, `mPlayerTouchRadius` 0, logged); needs a swarm whose `CBasicSwarmData` sets it |

Verified data facts worth keeping: `mTouchRadius` 0.7 for u041d's swarm (5 boids, positions logged),
editor id 131736's swarm has 7 boids created at Identity and never moved (it is the forced null-model
one); `CActor::mPosition` is the actor position the collision system uses (not `mTransform`);
`CMorphBall::mBallState` at `mMorphBall.mPtr`, `CPlayer::mSpawnedMorphBallState` and
`mMorphBallState` are the fields these arms read.

Gates: host build `metroid_prime2_port` 0 FAILED; `gen_files_cmake.py --check` rc 0; matching
`configure.py && ninja` `87 files OK`; `review_decomp_diff.sh 255e5d22d` one MARK; `asmcheck` one
explained CALL finding; `retail_diff --diff 255e5d22d` (see report).
