# FishCloud REL (module 20): analysis notes

CFishCloud (FISH) is a boid school in a box that swims, scatters from weapons and the player, and (when
killable) dies to shots; CFishCloudModifier (FSHM) is an attractor or repulsor (optionally swirling) that
follows a cloud. The module's code is upstream's `src/MetroidPrime/ScriptObjects/CFishCloud.cpp` with the
loader `include/MetroidPrime/ScriptLoader/SLdrFishCloud.hpp`. Retail asm:
`build/G2ME01/FishCloud/asm/MetroidPrime/ScriptObjects/CFishCloud.s`. Per-function table:
`FishCloud_functions.tsv` (B upstream body, H weak/implicit copy, T REL runtime).

## Survey (fish1, 2026-10-23)

`tools/upstream_hollow.py FishCloud`: nothing hollow; all 106 retail functions pair with upstream
(101 in CFishCloud.s, 5 in REL_Setup).

| function(s) | verdict |
|---|---|
| SLdrFishCloud ctor fn_20_DFC (64%) | **fixed**: retail `deathSound = -1` (0xe48 `li r0,-1`; 0xe94 `stw r0,0xc0(r31)`); upstream used 0. The extra store of 3 at +0x38 re-stores the SLdrEditorProperties default |
| Render 2BD8 (97%) | CModelFlags x0_ word uninitialized stack in retail and upstream alike: kept |
| AddSelf 68D4 (94%), InitAnimBoids, Touch, LoadFishCloud, destroy_elements | register allocation / scheduling only |
| FindFarthestPlane 46A0, OldBuildBoidNearList 1C40 | unreferenced in retail (no `bl`); GetListAt 2054 only from OldBuildBoidNearList |

## Shipped data (disc scan)

69 FISH placements, every one with an explicit death sfx -1 (so the loader fix has no shipped effect).
33 FSHM, each with exactly one connection, MDFY/FOLW to a FISH in the same area; the variants are
repulsor+swirl or attractor without swirl; at most 4 modifiers per cloud (reserve 10).

Dead arms, with the proof:

- `mWorldSpace` is always true: its only writer, the ctor (line 188), is `!(close_enough(right.x,1) &&
  close_enough(right.x,0) && ...)`, never false. So the `!mWorldSpace` arms of PointInBox, FindClosestPlane,
  FindFarthestPlane and PlaceBoid are dead.
- `mUpdateWithoutPartitions` is only written false (ctor): Think's OldBuildBoidNearList arm is dead.
- `mRandomMovement` / `mRandomMovementTimer` are only written by the ctor and Think's reset: Think's random
  arms (490/495/507/545) never run naturally.
- Data: ApplyRotation's clockwise=false (swirl modifiers are all repulsors), Think's non-swirl repulsor arm
  (514), AddSelf/RemoveSelf mismatched-connection arms, the Add* capacity-full arms.

## Coverage (fish1)

Coverage build, spec `tools/cov/specs/fishcloud.py` (counts Touch, KillBoid, ApplyRotation, Add*), drivers
`fishcloud_sacredpath.con` (06_temple_sacredpath: three animated clouds, the attractor e00340143),
`fishcloud_landingsite.con` (01_Temple_LandingSite: e0000000f with four swirl repulsors),
`fishcloud_swampentrance.con` (Torvus 01_swamp_entrance: static-model killable e00020042), and the forcing
spec `fishcloud_force.py` (run on the first two drivers):

    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/fishcloud.py <c1> \
      3bfa3eff,06_temple_sacredpath --con tools/cov/specs/fishcloud_sacredpath.con --fast --frames 2600
    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/fishcloud.py <c2> \
      3bfa3eff,01_Temple_LandingSite --con tools/cov/specs/fishcloud_landingsite.con --fast --frames 2000
    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/fishcloud.py <c3> \
      3dfd2249,01_swamp_entrance --con tools/cov/specs/fishcloud_swampentrance.con --fast --frames 1800
    BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/fishcloud_force.py <f1> (c1's args, --frames 2000)
    BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/fishcloud_force.py <f2> (c2's args, --frames 1800)
    python3 tools/cov/gcov_report.py build-port-cov <f1>/gcov <f2>/gcov <c1>/gcov <c2>/gcov <c3>/gcov --module FishCloud

Merged: 56 functions, 38 with branches, 23 with every arm taken.

Marked (39, every line and arm taken or the missing arm proven dead, asm-checked): LoadFishCloud 340,
LoadFishCloudModifier AC, SetRelLoaderFunctionToLoader 70, RenderParticles FA0, UpdateParticles 1020,
AddParticles 109C, ApplyContainment 1144, ApplyRepulsion 1244, ApplySeparation(point) 1388,
ApplySeparation(list) 14AC, ApplyCohesion(point) 1570, ApplyCohesion(list) 168C, ApplyWander 1780,
ApplyAlignment 18A0, ApplyRotation 1A1C, FishCloudCross 1C00, CreatePartitionList 22AC, KillBoid 2B04,
Render 2BD8, RenderBoid 2EC4, PreRender 2FD8, GetTouchBounds 3730, RemoveRepulsor 3798,
RemoveAttractor 3990, AddRepulsor 3A88, AddAttractor 3F2C, AcceptScriptMsg 4088, PointInBox 4D04,
GetUntransformedBoundingBox 4DBC, GetBoundingBox 4E28, PreRenderAllViewports 4EA4, InitAnimBoids 4F74,
~CFishCloud 53B0, CBoid ctor 673C, CModifierSource::operator< 6784, CModifierSource ctor 67B4,
CFishCloudModifier AcceptScriptMsg 69F0, ctor 6AAC, dtor 6BEC.

| arm | how |
|---|---|
| ApplyContainment 980 velocity zero (and Think 538 zero speed) | forced (`fishcloud_force.py`: a boid's velocity zeroed every 32nd think) |
| ApplyCohesion(point) 923, ApplySeparation(point) 950: point on the boid | forced (`fishcloud_force.py`: the boid moved onto the point every 32nd think) |
| AddAttractor 400 / AddRepulsor 418 vector full | forced (`fishcloud_force.py`: capacity = count from the 3rd / 5th call) |
| Render 598 inactive cloud, 614 inactive boid | forced (`fishcloud_force.py`: boid 0 inactive after 400 thinks, the cloud after 900) |
| RemoveAttractor 431 / RemoveRepulsor 440 not found | natural (a modifier deactivated twice) |
| ApplyRotation 877 clockwise=false | data-dead (every swirl FSHM is a repulsor; clockwise = mIsRepulsor) |
| PointInBox 260 `!mWorldSpace` | code-dead (only writer: ctor line 188, always true) |
| everything else marked | natural |

asmcheck findings on the marked bodies are tool limits: CALL to rstl templates (lower_bound 6C4C, erase
3890, push_back_unsafe 434C, reserve 6E00/6EA4), member dtors in ~CFishCloud, the unmarked ApplyAttraction
(1264) called from ApplyContainment, the SLdr typedef loads in the loaders, BIT/NOPOS on the 0x334 flags
(0x80 mRandomMovement, 0x40 mWorldSpace, 0x20 mEnableWeaponRepelDamping, 0x10 mValidModel) and CActor's
x150 pre-render-clipped bit. retail_diff: only M_PIF and the same template calls.

## Unmarked behaviour bodies (why)

- Dead: FindFarthestPlane, OldBuildBoidNearList, GetListAt (no live callers; 0% covered).
- Partly covered: Think (occluded area 451/452, missing modifier source 510/522), Touch (no touch bounds 669,
  non-player non-weapon 686, spawned morph 688, morph-ball arms 690/711/721), PlaceBoid and FindClosestPlane
  (re-place 347, NaN 292), BuildBoidNearList / UpdatePartitionList (partition edges), ApplyAttraction (963:
  the forced out-of-line copy never got the every-32nd-think hit), the CFishCloud ctor (particle 2-4, sfx),
  AddSelf/RemoveSelf (connection arms; RemoveSelf on area unload unproven).
- Smoke family proposal: `fishcloud`, area `3bfa3eff,06_temple_sacredpath`, driver
  `tools/cov/specs/fishcloud_sacredpath.con`.
