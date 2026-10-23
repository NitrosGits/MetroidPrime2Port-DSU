# PirateRagDoll REL (module 50): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CPirateRagDoll.cpp` (the ragdoll that space
pirates, dark troopers and similar enemies drop into on death; 14 particles named in `skParts`, plus
constraints and optional AI-waypoint pins). `CRagDoll` is DOL-side. The port's fixes are `TARGET_PC`
blocks in that file. Retail asm: `build/G2ME01/PirateRagDoll/asm/MetroidPrime/Enemies/CPirateRagDoll.s`
(41 functions in the split). Per-function table: `PirateRagDoll_functions.tsv` (B = module body,
H = weak copy/template, T = REL runtime).

## Behaviour fixes (prd1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_50_E10 Update, impulse wake | 0x121C-0x1240 `fcmpu` of the acceleration (computed at 0x113C-0x1150) against `sZeroVector`; mTorsoImpulse zeroed at 0x11C4-0x11D8 | the wake test read the impulse after it had been zeroed |
| fn_50_E10 Update, old centre z | 0x1090 `fcmpo f3(p8),f2(p11); bge` p11 | `min(p8,p11)` with ties swapped |
| fn_50_E10 Update, new centre z | 0x14C4 `fcmpo f0(p8),f1(p11); bge` p11 | same |
| fn_50_1610 CheckStatic | 0x1754 `fcmpo f2(frame vel),f0(mImpactVel)`; 0x1758 `bge` | `max` with the operands swapped (NaN/tie arm) |
| fn_50_3B4 UpdateImpactSfx | 0x45C `fcmpo f0(scale*vel),f1(max)`; 0x460 `bge` max | `min` with the operands swapped |

Checked equivalent (no fix): Prime (objdiff 100%), AlignShoulderPad, PreRender, the ctor, IsValid,
~CAIWaypointPredicate (each ≥90% objdiff, the remaining differences are register allocation, labels
and inlined vector maths, all read by hand).

## Spawns

Any ragdolling enemy. The drivers used Dark Troopers in Sky Temple Grounds/Sanctuary-side areas of world
3bfa3eff: `0e_temple_hive_hall` (layer 2) and `01_temple_hive_04` (default layer; six AIWPs, all active,
so the ragdoll gets waypoint pins).

## Coverage (prd1)

Build `-B build-port-cov -DMP_PORT_COVERAGE=ON`. Natural runs:
`tools/cov/run.sh tools/cov/specs/pirateragdoll.py <out> 3bfa3eff,<area> --frames 15000 --driver
"python3 tools/drivers/pirateragdoll_drive.py {port} darktrooper <layer|-> 20"` (kills the troopers,
then shoots, drops into water and pushes the settled ragdoll; env PRD_WAIT, PRD_SHOTS, PRD_PINS=0,
PRD_ATTACH=1). Forced runs: `COV_GDB_SCRIPT=tools/cov/cov_force.py PRD_F=<set> tools/cov/force.sh
tools/cov/specs/pirateragdoll_force.py ...`, set one of attach/hit/water/impulse/pins/inactive.

| arm (CPirateRagDoll.cpp, pre-marker lines) | how |
|---|---|
| Update 217-231 actor-attached pose, PreRender 369-374, UpdateImpactSfx 400 delta arms | forced (PRD_F=attach) |
| Update 259 hit-by-projectile wake | forced (PRD_F=hit) |
| Update 261 water-top wake | forced (PRD_F=water) |
| Update 268 torso impulse | forced (PRD_F=impulse) |
| Update 279, 282-288, 304 null waypoint / inactive pin slot | forced (PRD_F=pins) |
| Update 280-281, 304 inactive actor | forced (PRD_F=inactive) |
| CheckStatic 174 branch 2 | pins/inactive runs |
| everything else | natural |

Dead arms (not run, proof):
- ctor cpp:73 invalid-locator break: all 14 `skParts` locators exist on every ragdolling model
  (data-dead).
- ctor cpp:142 TCast failure: `CAIWaypointPredicate::IsValid` already keeps only AIWPs.
- CheckStatic cpp:199 empty particle list: the ctor always builds 14.
- ~CAIWaypointPredicate deleting arm: the predicate only exists as a stack local in the ctor.
- CreatePirateRagDoll (fn_50_74): host callers construct with `rs_new` directly; left unmarked.

Marked by prd1 (9): AlignShoulderPad, UpdateImpactSfx, PreRender, Update, CheckStatic, Prime, ctor,
~CAIWaypointPredicate, IsValid. Unmarked: REL runtime, SetFuncPtrs/CreatePirateRagDoll, __sinit, and
the weak rstl/template copies.

## Space-pirate runtime check (prag2, 2026-10-24)

The ragdoll is no longer outside the object list's reach: since sp14 `CSpacePirate::Think` (0x1DD8)
builds, primes and steps it, and the corpse's transform is the ragdoll's, so `objs spacepirate`
shows the ragdoll's centre. `tools/drivers/pirateragdoll_sp_poll.py <port> 3 45` (07_sand_alamoportalobservation
L3, normal pace, `MP2_GODMODE=1`) kills the five pirates with `damage <id> 1000` and polls their
positions every second: all five corpses fall 1-3 units and stop within ~6 s, 0 NaN/inf/|c|>5000
readings in 45 s, 0 caught/reach-stub. The earlier "~9e6" corpse did not reproduce. Fast mode
(`MP2_FRAME_LIMIT=0`) outruns a wall-timed driver, so use normal pace for polls.
No new marks: the 9 bodies prd1 marked already have their arms proven (table above), and
SetFuncPtrs/CreatePirateRagDoll stay unmarked (host callers use `rs_new`).
