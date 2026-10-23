# WallWalker REL (module 85): analysis notes

CWallWalker (WLWK, "Mekenobite") is a CWallCrawler that patrols walls on waypoints, generates and fires
volleys of targetable homing projectiles, flinches when a leg (two collision actors, 5 HP each, always
refilled) is hit, and dies when both legs are hit by missiles or the body is killed. The module's code is
upstream's `src/MetroidPrime/Enemies/CWallWalker.cpp` with the loader
`include/MetroidPrime/ScriptLoader/SLdrWallWalker.hpp`. Retail asm:
`build/G2ME01/WallWalker/asm/MetroidPrime/Enemies/CWallWalker.s`. Per-function table:
`WallWalker_functions.tsv` (B upstream body, H weak/template copy, T REL runtime).

## Survey (ww1, 2026-10-23)

The matching build pairs every CWallWalker body at 100% except one:

| function | verdict |
|---|---|
| SLdrWallWalker ctor fn_85_1790 (73.8%) | **fixed** (TARGET_PC): retail defaults the grenade sounds to -1 (asm 0x1790-0x18D8); upstream used 0 |

## Shipped data (disc scan, `tools/scan_wallwalker_props.py`)

There are 6 WLWK placements, and every one sets its grenade sounds explicitly, so the loader fix changes
nothing in game.

- **0n_cliff_hall:** the walkers follow WAYP, not AIWP.
- **09_cliff_temple, layer 3:** the walkers follow AIWP, every one with flags 0x200 (the Generate arm).
  - Walkers: e002500f9 (legs u05fc/u05fd), e0025014e, e00250156, and e0025009e (inactive).

## Coverage (ww1)

Coverage build, natural spec `tools/cov/specs/wallwalker.py`, drivers `wallwalker_temple.con` and
`wallwalker_hall.con`, forcing spec `wallwalker_force.py` with `wallwalker_force.con`:

    BUILD=$PWD/build-port-cov SECS=1800 tools/cov/run.sh tools/cov/specs/wallwalker.py <c1> \
      1baa96c2,09_cliff_temple --con tools/cov/specs/wallwalker_temple.con --fast
    BUILD=$PWD/build-port-cov SECS=1800 tools/cov/run.sh tools/cov/specs/wallwalker.py <c2> \
      1baa96c2,0n_cliff_hall --con tools/cov/specs/wallwalker_hall.con --fast
    BUILD=$PWD/build-port-cov SECS=1800 tools/cov/force.sh tools/cov/specs/wallwalker_force.py <f2> \
      1baa96c2,09_cliff_temple --con tools/cov/specs/wallwalker_force.con --fast --frames 3400
    python3 tools/cov/gcov_report.py build-port-cov <c1>/gcov <c2>/gcov <f2>/gcov ... --module WallWalker

Use `SECS=1800`: the default 900 s run timeout kills the game before gcov flushes. Don't make inferior
calls in the force spec (for example GetHealthInfo()): they hang under ptrace, so write the fields
(`mHealthInfo`) directly.

Merged result: 27 functions, 15 with branches, 6 with every arm taken (Think needs f2).

**Marked (21, every live line and arm taken, asm-checked):**

| fn_85_ | function |
|---|---|
| 10C | SetRelLoaderFunctionToLoader |
| 1790 | SLdrWallWalker ctor |
| 18DC | LaunchProjectiles |
| 221C | DoUserAnimEvent |
| 2340 | Dead |
| 2748 | RandomVectorInCone |
| 2894 | Death |
| 28B4 | Think |
| 2BFC | DestroyCollisionManager |
| 2C48 | SetupCollisionManager |
| 313C | SetNumberShots |
| 3178 | ShouldShootProjectile |
| 3280 | AreBothLegsHit |
| 32A0 | IsRightLegHit |
| 32AC | IsLeftLegHit |
| 32B8 | ShootProjectile |
| 3484 | RightLegHitReaction |
| 3664 | LeftLegHitReaction |
| 3C84 | SetupStateMachine |
| 3D10 | ~CWallWalker |
| 3DD8 | CWallWalker ctor |

| arm | how |
|---|---|
| LaunchProjectiles: more shots than the AI projectile pool (4) | forced (`wallwalker_force.py`: 8 shots) |
| ShouldShootProjectile: early true while shots are queued | forced (`wallwalker_force.py`: mNumShots > 0) |
| DoUserAnimEvent: event type 2 | forced (`wallwalker_force.py`: from the 5th event) |
| Dead burning (e0025009e) | forced (`wallwalker_force.py`: mBurning) |
| Dead and Think: Ing-possessed (e00250156) | forced (`wallwalker_force.py`: Ing target and blend set, uncharged dark death weapon) |
| Dead and Think: dark charged death (e0025014e) | forced (`wallwalker_force.py`: `mHealthInfo.mDeathWeaponMode` set to dark and charged) |
| Leg hits and both-legs death | natural, through the console: `hurt u05fd/u05fc 10 6` (missile) |
| Body death | natural, through the console: `hurt <id> 10 9` (screw attack) |
| Left/RightLegHitReaction 151/169 and ShootProjectile 187, switch default | code-dead: EStateMsg has only Activate, Update and Deactivate, and TStateMachineState.hpp:215-256 passes only those |
| SetupCollisionManager 262, TCastToPtr null | code-dead: CCollisionActorManager's ctor has just created and added every actor id it returns |
| LaunchProjectiles 418, empty optional | code-dead: the optional_object is built from a held token, so it is always engaged |
| everything else marked | natural |

**asmcheck findings on the marked bodies are tool limits:**

- BIT/NOPOS: upstream's bitfields (x8f0, x153 and x168 flags) have no stated bit positions.
- CALL: calls to rstl/weak template copies and to the unmarked UpdateCollisionManager (2ABC).
- XCALL: the self-name matches on the state-function table.
- The BRANCH counts in LaunchProjectiles and ~CWallWalker come from inlined token and optional code.

These all pair at 100% in the matching build, so the logic is the retail logic.

## Unmarked behaviour bodies (why)

| function | untaken arms (all live) |
|---|---|
| AcceptScriptMsg | 113 (the last-touched object is not a weapon), 114 (a missile carrying attrib 22) |
| Patrol | 124:3/4 (msg / IsInPosition), 129:1/2 (an AIWP without 0x200; none ships, but the hall WAYP path is not proven) |
| UpdateCollisionManager | 283 (null manager), 286/287 (a leg hit while the missile/leg flags are already set; a third collision index) |
| LoadWallWalker | 439 (no model data: data guard), 444 (GetObj token arms) |

**Smoke family proposal:** `wallwalker`, area `1baa96c2,09_cliff_temple`, driver
`tools/cov/specs/wallwalker_temple.con` (needs `layer 3`).
