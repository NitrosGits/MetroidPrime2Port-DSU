# AIMannedTurret REL (module 1): analysis notes

The pirate-manned turret AI (AIMT): it controls an attached animated turret actor
(`kSS_AttachedAnimatedObject`) and a rider actor. It aims at the player, telegraphs (four `canNN_LCTR`
particle generators), fires an energy projectile from `muzzleend_LCTR`, and sweeps a patrol spline
once the player stays out of range. Code is upstream's `src/MetroidPrime/Enemies/CAIMannedTurret.cpp`.
The loader is `SLdrAIMannedTurret.inc`. Retail asm:
`build/G2ME01/AIMannedTurret/asm/MetroidPrime/Enemies/CAIMannedTurret.s`.
Per-function table: `AIMannedTurret_functions.tsv` (B upstream body, H weak/implicit copy, T REL runtime).

## Survey and fixes (amt1, 2026-10-23)

Every sub-100% body was diffed normalized (labels, relocations, registers, constants resolved) against
upstream. No behaviour difference in: CheckPatrol, CheckAttack, PreRender, Fire, the ctor, the dtor,
the SLdr dtor, LoadAIMannedTurret (editorProperties.active=false like retail), AttachToActors,
AcceptScriptMsg, Start, Attack (an extra `frsp` only), Patrol and `__sinit` (flash color 0.5,0,0,1).
GetMuzzleTransform calls GetObjectById where retail calls ObjectById. Both use kOL_All, so the result
is the same.
The ctor's telegraph token (retail 0x4250 `bl fn_1_4284`: optional<TLockedToken> copy, Lock at 0x42CC)
is locked in the port as well: the CToken copy ctor locks when its source holds a lock.
fn_1_B74, C88, CC4, 4360 and 4648 are unreferenced. fn_1_1330 and fn_1_4284 are compiler helpers.
One fix (`TARGET_PC`):

| where | retail | port before |
|---|---|---|
| UpdateAim (0x23EC `fcmpo |diff|,0.1`, 0x23F0 `blt 0x24DC`) | step unless `|diff| < 0.1` | `|diff| >= 0.1` (NaN-only difference) |

Header: the 0x614 flag byte is MSB-first (0x80 mReady, 0x40 mDead, 0x20 mTookDamage). The `Stop` message
sets 0x40 and clears 0x80.

## Spawns (whole-disc scan)

There are only two AIMT objects in the game, both in Agon `06_sand_alamofront` layer 2, with identical
properties: hRot 179/179, vUp/vDown 89, hSpeed 65, vSpeed 60, fireRate 3 (random 0),
maxAttackRange 85, startAttackRange 80, leash 5 s, telegraph 70415ad0, fsm ddd2c218,
weapon 03314306, and the same splines (horizontal max time ~11.95 s, vertical 12 s).
AIMT e001d0055 sits at (186.4,-67.8,28.1), with turret 001d005f and rider platform 001d0054.
AIMT e001d0238 sits at (187.0,-112.7,27.2), with turret 001d024b and platform 001d023a.

## Driver

`tools/drivers/aimannedturret_alamofront.con`, area `42b935e4,06_sand_alamofront`, save mi_u,
`MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8 MP2_GODMODE=1`, 14000 frames through `tools/smoke/fam.sh`. The
driver runs `layer 2`, activates the turret actors and sends `Start`. It moves the player around the
turret (aim arms), uses `mpforce 1` for the multiplayer sfx arm of SpawnTelegraph, then `tp 90 -68 10`
(>85 units out) for the leash and Patrol. It damages the AIMT in Patrol (TookDamage leads back to
Attack), then sends Stop/Deactivate/Activate/Start and AreaLoaded, and deletes the turret and then the
rider for the null-actor arms. Merged with the tail21 driver run, `caught/reach-stub` is 0.

## Coverage and marks

24 bodies are marked. Every one of their live arms ran. The dead arms, with the proof:

| arm | proof |
|---|---|
| ctor 102 vertical spline shorter | data: both AIMTs' vertical spline (12 s) is longer than the horizontal one |
| ctor 105 / SpawnTelegraph 514 no telegraph | data: both have telegraph 70415ad0 |
| SetupStateMachine 115 no FSM, Think 497 no state | data: both have fsm ddd2c218, and Setup always sets "Start" |
| AttachToActors 129, Start 228 turret without animation | data: both attached turrets are animated actors |
| dtor delete-flag / ctor 109 cleanup | compiler-generated (D0/D2 split, EH cleanup) |

Unmarked:
- UpdateAim (1F28): the snap arm is dead (both callers pass `li r5,0`: Patrol 0x2C34, Attack 0x2EFC).
  The horizontal-limit arms (403/407; the limits are 179°), the |dot|<1e-4 arms and the elevation
  clamp arms never ran. Forcing them needs gdb (`tools/cov/force.sh`).
- Attack (2C6C): the clamp arm of the inline `Limit(dot,1)` never ran.
- Fire (CF0): the weapon-not-loaded arm never ran (load timing).
- Death and KnockBack: empty and never called.

## Smoke family

Proposed `amt`: area `42b935e4,06_sand_alamofront` with the driver above. Pass when the console log
shows the Start/Attack/Patrol cycle (status lines) with no caught or reach-stub hits.
