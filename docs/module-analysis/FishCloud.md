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
- Partly covered (fish2 later marked Touch, AddSelf and the SLdr ctor/dtor rows, see below): Think (occluded area 451/452, missing modifier source 510/522), Touch (no touch bounds 669,
  non-player non-weapon 686, spawned morph 688, morph-ball arms 690/711/721), PlaceBoid and FindClosestPlane
  (re-place 347, NaN 292), BuildBoidNearList / UpdatePartitionList (partition edges), ApplyAttraction (963:
  the forced out-of-line copy never got the every-32nd-think hit), the CFishCloud ctor (particle 2-4, sfx),
  AddSelf/RemoveSelf (connection arms; RemoveSelf on area unload unproven).
- Smoke family proposal: `fishcloud`, area `3bfa3eff,06_temple_sacredpath`, driver
  `tools/cov/specs/fishcloud_sacredpath.con`.

## Second pass (fish1b, 2026-10-24, base 30d7b344b)

`python3 tools/bulk_mark_matched.py --module FishCloud` marks 0 (44 template/operator/unnamed, 7
definition-not-found), so there is no objdiff-100% row left to take before the arms.

Three fresh runs (spec `fishcloud.py`; sacredpath 2600 frames natural, LandingSite 2000 natural,
sacredpath 2200 with `fishcloud_force.py`), all `[port] caught/reach-stub: 0` and all exited normally
so the gcov was written. Merged profile, unmarked bodies only:

| body | arms | open |
|---|---|---|
| 1264 ApplyAttraction | 9/9, 5/6 | 1006:1/2 - the `CanBeNormalized` false arm (the out-of-line copy is forced by fishcloud_force.py; the inline copies in the other Apply* bodies are not, so the same test has to be forced at each `loc:`) |
| 1D24 BuildBoidNearList | 36/38, 40/48 | 842:1/2, 850:1/2, 853:0/2, 854:0/2, 856:0/2, 859:1/2, 867:0/2, 868:0/2 - the partition edges (a boid outside the queried partitions) |
| 2148 UpdatePartitionList | 14/14, 6/8 | 797:1/2, 799:1/2 - the out-of-partition index arms |
| 22EC Touch | 63/74, 62/81 | 686:1/4, 696/698/701/702:0-1/2, 703-706, 715/717:1/2, 719:2/4, 740:2/6, 750:2/6 - the no-touch-bounds, spawned-morph and morph-ball arms |
| 3054 Think | 74/79, 62/71 | 476:1/2, 477:1/4, 503:0/2, 504:0/2, 535:1/2, 539:0/2, 547:0-1/2, 584:1/2 - the occluded arms (451/452 in the old numbering: an occluded area is needed) and the random-movement arms |
| 43F8 PlaceBoid | 8/20, 3/8 | 357/358/359:0-1/2, 365:0/2 - the `!mWorldSpace` arms (dead: the ctor always sets it) and the re-place arm |
| 4A04 FindClosestPlane | 14/23, 8/18 | 284:0/2, 287:0-1/2, 290:0-1/2, 291:0-3/4, 308:2/4 - the `!mWorldSpace` arms and the NaN arm |
| 573C ctor | 62/68, 25/57 | 179/181/182, 192-224, 230-260 (the particle 2-4 and sfx arms: no driven cloud carries those) |
| 1C40 OldBuildBoidNearList, 2054 GetListAt, 46A0 FindFarthestPlane | 0 lines | no caller in retail (no `bl`), so they cannot run and stay unmarked |

No source change in this pass and no marker: no unmarked body reached all its live arms.

## Third pass (fish2, 2026-10-25, base 92bcb4775): Touch, AddSelf, SLdr ctor/dtor

Evidence under /tmp/opencode/fish2 (gcov dirs `<run>/gcov`). Natural: c1-c5 (sacredpath/LandingSite/swamp), d1-d2
(0k_temple_hall, `fishcloud_morph.con`). Forced: t1 (swamp `3dfd2249,01_swamp_entrance`, `fishcloud_touch_force.py` weapon case),
t3 (hall `3bfa3eff,0k_temple_hall`, ball case), a2 (LandingSite, `fishcloud_addself_force.py`). All `caught/reach-stub: 0`, rc 0.

| row | result | how |
|---|---|---|
| 22EC Touch | 74/74 lines, 81/81 branches, marked (see rework below) | natural: all but 698:1, 702:1, 705:1/3, 706; **forced (force.sh fishcloud_touch_force.py, t1)**: `CGameProjectile::mActive=0` (698:1), boid `mActive=0` (702:1), boids moved onto the toucher (705/706 KillBoid); 715/717/719 morph arms natural (d2) |
| 68D4 AddSelf | 14/14, 17/17, marked | natural 13/14, 13/17 (every shipped FSHM has one MDFY/FOLW to a FISH); **forced (force.sh fishcloud_addself_force.py, a2)** by call: state!=Modify (89:1), msg!=Follow (89:2), unknown objId (93:0), non-cloud objId (96:3) |
| D98 / DFC SLdrFishCloud dtor / ctor | marked | gcov of SLdrFishCloud.hpp: ctor and dtor lines all run (d2 x4, c1 x6, c3 x1), 0 unrun lines. Asm DFC: deathSound -1 (0xe48/0xe94, fish1's TARGET_PC fix), counts 0 at +0xa4/+0xac/+0xb4/+0xbc (0xf68..0xf74), bools 1 at +0xc4/+0xc5, 3 at +0x38 (0xea4). D98 is `~SLdrEditorProperties` + Free |
| 2EC SLdrFishCloudModifier dtor | marked | gcov of SLdrFishCloudModifier.hpp: dtor ran (d2 x2, c1 x1); asm 0x2EC = `~SLdrEditorProperties` + Free |

Rework (fish2b, 2026-10-25): the first version called 740:2/6, 740:5/6, 750:2/6, 750:5/6 "throw landing pads". That was wrong:
gcov_report.py already excludes the throw edges, and these are the real false arms of `flat.CanBeNormalized() && vel.CanBeNormalized()`
and `delta.CanBeNormalized() && vel.CanBeNormalized()` (:2 = first test false, :5 = velocity false). The counters showed flat/delta-false
never ran in any natural run (ball contact boids are never on the ball's axis) and velocity-false only in f1, which the first merge left out.
Line numbers in the gcov report are those of the cov build (740/750); in the source after the two fn_20_22EC/68D4 markers they are 742/752.

| arm | how |
|---|---|
| 740:2 flat false, 750:2 delta false | **forced (force.sh fishcloud_touch_norm_force.py, n4, sacredpath `3bfa3eff,06_temple_sacredpath`)**: at the entry of the out-of-line `CVector3f::CanBeNormalized` called from Touch the vector is zeroed on every third contact; hits logged in n4/forced.log |
| 740:5 vel false, 750:5 vel false | forced by the same spec (every second second-test) and f1 (`fishcloud_force.py` zeroes boid velocities) |
| everything else in Touch | natural, plus the weapon forces of the first pass (t1) |

Merged report (c1-c5, d1, d2, f1, t1, t3, n4; fish2b/merged_touch.txt): `CFishCloud::Touch` 74/74 lines, 81/81 branches, ALL-ARMS. Every
run `caught/reach-stub: 0`, rc 0. No arm of Touch is declared dead.

The earlier "no standalone body to mark" notes for the SLdr rows were wrong: the markers sit above the header-inline
definitions (SLdrDarkCommando.hpp:14,43,71 precedent). The deathSound -1 default stays under TARGET_PC.
