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
- Death and KnockBack: empty and never called.

## Smoke family

Proposed `amt`: area `42b935e4,06_sand_alamofront` with the driver above. Pass when the console log
shows the Start/Attack/Patrol cycle (status lines) with no caught or reach-stub hits.

## Coverage (iss2: the 8 rows left)

Counted with port_progress.py's marker rule at 04852528f: 8 B rows have no `// @ fn_1_*` marker. Runs:
amt1's driver (`tools/drivers/aimannedturret_alamofront.con`, 14000 f) natural, then two forcing runs
(`aimannedturret_force.py` with force.sh, `aimannedturret_force2.py` with force_ret.py), all
`caught/reach-stub: 0`.

| fn | state |
|---|---|
| 6D8 LoadTypedefAIMannedTurretData | marked: objdiff 100% (the compare tree is the Data property ids) |
| A18 ~SLdrAIMannedTurretData | marked: objdiff 100% |
| A88 SLdrAIMannedTurretData ctor | marked: objdiff 100% (0xA88 stfs/stw defaults) |
| 5B0 ~SLdrAIMannedTurret | marked (amt3): member teardown only; not byte-identical (objdiff 74.2%): GCC inlines sub-object dtors MWCC called; same order as retail (0x5B0 chain) |
| 4964 SLdrAIMannedTurretData (copy) | marked (amt3) above the struct: implicit memberwise copy, word map + static_assert layout |
| 1F28 UpdateAim | 45/56. Forced and taken: the `snap` arm (`$rdx = 1` at the body entry; both callers pass 0) closed 410/411/412/444. Still open: 402/403/404 and 417/418/419 (`|dot| >= 1e-4` false), 422/426 (the horizontal rotation limits, 179 deg) and 451/453/458 (the elevation animations). The `loc:` writes of `mAimDirection` failed - force.py binds `$this` to `$rdi`, which no longer holds the object mid-function; the fix is to capture `$rdi` at the body entry into a convenience variable and cast it back in the `loc:` set |
| 2C6C Attack | 21/22: only 278:0/2 (the inlined `CMath::Limit(dot, 1)` clamp, needs a non-unit `mAimDirection`) |
| 0CF0 Fire | 8/10: 585:1/2 (`mWeaponToken.IsLoaded()` false - the token streams in before the driver's first shot; `CObjectReference::IsLoaded` is inlined so FORCE_INNER found no symbol) and 596:1/2 (`projectile != nullptr` false, needs a failed allocation) |

## amt3 (2026-10-25): Fire, UpdateAim, Attack marked

Bodies match retail (NaN compares included); no TARGET_PC change. Fixture `tools/cov/specs/amt3_force.py`
(19dca9583): single-object filter (first UpdateAim `this`), writes owned by the non-inline frame
(Attack+896 sits in inlined `CMath::Limit`; comparing `newest_frame()` there never matched), originals
recorded, readback, restore at return. Run `build-port-cov/amt3-run_force2` (06_sand_alamofront, 14000 f,
rc 0, caught 0); frozen copy `/tmp/opencode/amt3-frozen/force2` (binary 269d86ba...). In that run every
non-throw arm of the three bodies is taken; unrun arcs are landing pads. Fire's null-projectile arm
(retail 0xE48) is folded on the host (throwing new). Snap stays code-dead in retail (li r5,0 at 0x2C34/0x2EFC).
