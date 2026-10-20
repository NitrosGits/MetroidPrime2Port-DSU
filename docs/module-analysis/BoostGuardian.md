# IngBoostBallGuardian REL (module 30): `CIngBoostBallGuardian` (Boost Guardian)

Analysis to plan the host port, dated 2026-10-20. No port code exists yet.

**Sources:** the archive build's disassembly of `build/G2ME01/IngBoostBallGuardian/asm`
(`.text` 0x0..0x110C4 plus a second section 0x11CD8.., 309 `.fn` symbols; `.data` 0xCDC,
`.rodata` 0x848, relocation-free), m2c pseudo-C for 293 of them in
`/tmp/opencode/pseudo/IngBoostBallGuardian/`, the generated loader struct
`include/MetroidPrime/ScriptLoader/SLdrIngBoostBallGuardian.hpp` (upstream already has it), and
`config/G2ME01/symbols.txt` for the DOL call targets.

Unlike Sandworm, this module's loader struct is already decompiled upstream, so the whole
parameter surface is known before any code is written: `SLdrIngBoostBallGuardian` =
`SLdrEditorProperties` + `SLdrPatternedAITypedef` + `SLdrActorParameters` +
`SLdrIngBoostBallGuardianData` (the Ing AI block, one field per Ing property) +
`SLdrBoostBallGuardianData` (the boost-ball block, including three phase structs).

**Shape in one line:** a `CPatterned` (flavor 0x19) that runs an Ing-shaped FSM and, in its
"boost" phases, becomes a 1.8-scale rolling ball (mass 150, spline speed) that damages what it
hits, retreats into a puddle form, and opens mini portals.

## a. Class inventory

| class | size | vtable | ctor | base | what it is |
|---|---|---|---|---|---|
| `CIngBoostBallGuardian` | 0x11F0 [proven: `li r3, 0x11f0` before `__nw__` at 0x00000488, and the ctor writes members up to 0x11EF] | `lbl_30_data_9C0`, 82 words, 34 local overrides | `fn_30_EC6C` (loader calls it at 0x0000051C); dtor `fn_30_E5EC` | `CPatterned` 0x7C0 [proven: the ctor's first call is `CPatterned::CPatterned` with `li r4, 0x19`, and the ctor installs the vptr at 0x0] | the enemy |
| second class, `CPhysicsActor`-based | 0x5E0 [proven: `li r3, 0x5e0` at 0x0000361C, `__nw__` at 0x00003624] | `lbl_30_data_C44`, 38 words, 14 local overrides | `fn_30_15924` (calls `CPhysicsActor::CPhysicsActor` at 0x00015A7C, installs the vptr at 0x00015A80) | `CPhysicsActor` [proven: the base ctor call] | spawned by `fn_30_3478`, see §f |

Loader: `fn_30_130` (0x130..0x6BC, 324 asm lines) = `LoadIngBoostBallGuardian`; `RELMain`
(`fn_30_0`..`fn_30_100`) installs `fn_30_130` into `lbl_30_bss_6C` and calls the DOL's
`fn_8022FFC4` = `SetSIngBoostBallGuardian_FuncPtrs`. The port replaces this pair with
`RELMain`/`RELExit` in the usual shape (`src/MetroidPrime/Enemies/PortCIng.cpp` is the model).

## b. FSM: 28 states, 49 triggers, 9 codes

`fn_30_110C4` is a `.ctors` static initialiser (it is listed in `.section .ctors`) that copies
three name tables into module `.bss`; the tables are in `.data` and every entry is
`{name pointer, 0, 0, 0}`. The three tables, in table order:

* states (`lbl_30_data_6B0`, 28): Start, WaitForFSMTransition, ExitHost, BecomeCorporeal,
  BecomeIngSpot, BecomeBodyProjectile, BecomeWallProjectile, Alert, Taunt, Lurk, PathFind,
  IngSpotPathFind, IngSpotPointPathFind, Patrol, FollowAttackPattern, BodyProjectileFlight,
  SuckEnergy, SeekWallPoint, SelectTarget, FaceTarget, FindCoverPoint, ArmSwipe, FailSafeMode,
  FindMiniPortals, MiniPortalAttack, Dead, BoostAttack, BoostModeRetreat
* triggers (`lbl_30_data_250`, 49): StateOver, IsAlert, IsControllingHost, IsFacingTarget,
  HasTarget, HasNewTarget, HasLineOfSight, UnderFire, HeardShot, AreaClear, CoverLeash,
  HasCoverPoint, HasWallCoverPoint, HasMiniPortals, IsCorporeal, IsIngSpot, IsBodyProjectile,
  IsFrustrated, IsAggressive, UseProjectileFSMEntry, TargetInSafeZone, TargetInLightSafeZone,
  CanChangeForm, ShouldFleeSafeZone, ShouldBecomeCorporeal, ShouldBecomeIngSpot, ShouldArmSwipe,
  ShouldTaunt, ShouldBodyProjectile, ShouldOpenMiniPortal, ProjectileSplat, ProjectileHitTarget,
  FoundMovementPos, PathShagged, PathOver, PointPathOver, FoundPointPath, IsOffPath,
  HasAttackPattern, AttackPatternOver, TargetIsBall, HasPathToTarget, CanGrappleTarget,
  IsBoostPuddleForm, IsCurrentlyInBoostPuddleForm, IsBoostOver, ShouldBoost,
  HasCollidedWithCrystal, BoostModeRetreatOver
* codes (`lbl_30_data_8DC`, 9): SetTargetDest, SetCoverDest, SetRecoverDest, SetPointCoverDest,
  SetWallPointCoverDest, SetExitHostDest, StartRangedAttack, EndRangedAttack, SplatOntoMesh

This is the Ing FSM (module 29 `CIng` carries the same first 42 triggers and 27 states) with seven
Boost additions: trigger `TargetIsBall`, `IsBoostPuddleForm`, `IsCurrentlyInBoostPuddleForm`,
`IsBoostOver`, `ShouldBoost`, `HasCollidedWithCrystal`, `BoostModeRetreatOver`, and states
`BoostAttack`, `BoostModeRetreat`. `IsAlert` in slot 1 and the rest line up one-for-one with
`CIng`'s, so the whole Ing half can be read off `docs/module-analysis/IngFamily.md` and the
`CIng` port rather than re-derived. What is *not* known yet: the transitions. They are not in
`.data` (no table big enough), so they come from the actor's `'FSM2'` asset — the same route
SandBoss used (`MP2_SB_FSM` probe dumping `CGenericFSM2`), reachable only once the class runs.

## c. Parameters (from `SLdrIngBoostBallGuardian.hpp`, upstream)

Ing block = `SLdrIng`: hearing 50, ingSpot speeds 15/7/25, turn 360, frustration 2, taunt 25,
aggressiveness 10, arm swipe 20/knock 10, body projectile 20/5, mini portal 40..100.
Boost block `SLdrBoostBallGuardianData`: scale 1.8, mass 150, spline speed, damage to morph ball
7/30/1.5/4 and to first person 7/30/1.5/6, min damage speed 10, search cone 80°, ingSpot damage
11/20/10 and 11/10/5, and three `SLdrIngBoostBallGuardianStruct` **phase** structs
(`boostPuddleModeTime` 15, `unknown_0xabe99de0` 0.5, `breakBoostPuddleModeDamage` 90,
`breakBoostModeDamage` 10, `nextBoostTimeMin/Max` 2..5, six ints 3/3/1/5/6/10,
`locomotionSpeedScale` 1, `ingSpotSpeedScale` 1). Sandworm's `SLdrSandwormStruct` was a health
threshold table; the same reading applies here: `fn_30_3990`-style "first struct whose threshold
is below current health" picks the phase.

## d. Spawn and the run harness

Area `Torvus Bog/06_swamp_boostballguardian_dark`, **layer 5 = "Boss Battle"** (already active on
load — `[layer] layer 5 of area 28 is already active`), area index 28 of world `3dfd2249`
(`04_Swamp_World_20041018`). Only the dark variant hosts it; `06_swamp_boostballguardian` (area
19) is the light room. `docs/module_spawns.tsv` also lists `Ing` (layers 4/5/6), `MinorIng` and
`DarkCommando` in the same area — expect company during a fight.

Harness: `/tmp/opencode/bg_run.sh <tag> <cmd-file> [secs]` builds
`build-port-link/metroid_prime2_port` in the `wt-mp2-boostguardian` worktree, boots the area,
forces `layer 5` at 34 s and drives the console on port 47131 with a `@<secs> <command>` file;
the console client is the shared `/tmp/opencode/sb2_console.py`. Baseline (entry stub, 2026-10-20):
area loads, no crash, the boss does not appear.

## e. Layout of `CIngBoostBallGuardian` (guest offsets, from the ctor `fn_30_EC6C`)

The ctor's member stores and DOL constructor calls, in offset order (full table with one asm
address per row: `/tmp/opencode/ibbg_layout.md`, 0x11F0 total, base 0x7C0):

| offset | what | evidence |
|---|---|---|
| 0x07C0 | 0x320-byte parameter block, copied in by `fn_30_1056C` from the loader's temp; the same shape as `CIng`'s `CIngData` at +0x7C0 (Ing AI properties), 0x110 bytes bigger | [0x0000ECEC] |
| 0x0AE4 / 0x0AE8 | int 2 / int -1 (the same pair `CIng` keeps at 0x9d4/0x9d8, which the ctor of the Ing search setup also sets) | [0x0000ED00], [0x0000ED10] |
| 0x0AEC | `CPathFindSearch` | [0x0000ED24] |
| 0x0BBC / 0x0BC0 | floats 5.0× and 3.0× two base-class floats at 0x60 | [0x00010190], [0x0001018C] |
| 0x0BD8 | `CPathFindPointSearch` | [0x0000ED30] |
| 0x0D60 / 0x0D64 | two `CCollisionActorManager*` (null here, freed in the dtor) | [0x0000ED3C], [0x0000ED40] |
| 0x0D68 | `CSurfaceAlignmentHelper` | [0x0000ED44] |
| 0x0DC0 | `CIngSpotPathFindNavigation` | [0x0000ED4C] |
| 0x0DD0 | `CAABox` = `CAABox::mskNullBox` (24 bytes) | [0x0000ED70].. |
| 0x0DE8 | byte 1 | [0x0000ED58] |
| 0x0DEC | 0x110-byte block, copied in by `fn_30_10250` (the boost/puddle parameters) | [0x0000ED98] |
| 0x0FD8 / 0x0FDA | two `TUniqueId` = kInvalid | [0x0000EDAC], [0x0000EDBC] |
| 0x0FDC..0x106C | **nine** `CToken`s, 0x10 apart, each followed by its valid byte at +0xC | [0x0000EE98] … [0x0000FA7C] |
| 0x1070..0x1098 | zeros, then fourteen `u16` = kInvalidUniqueId at 0x1074, 0x1076, 0x1084..0x1098 | [0x0000FB18]..[0x0000FBC8] |
| 0x10AC..0x10B5 | ten bytes, 0xFF then overwritten with `CAnimData::GetLocatorSegId` results (ten named locators) | [0x0000FBD8], [0x0000FFAC].. |
| 0x10B8 | `CLineOfSightTracker` | [0x0000FC10] |
| 0x10AC+? 0x10BA | byte copy of 0x10AD | [0x00010164] |
| 0x10F8..0x111B | three `CVector3f` = zero | [0x0000FC34].. |
| 0x111C | int -1 | [0x0000FC84] |
| 0x1120 | `CVector3f` = `Normalize(1.0, 0.1, 0.05)`, then 0x112C = its dot with the base's 0x54/0x58/0x5C | [0x0000FCA8]..[0x0000FCFC] |
| 0x1130 / 0x1158 / 0x115C | 0 / -1 / 0 | [0x0000FD00].. |
| 0x1160..0x1177 | two `CVector3f` = zero | [0x0000FD10].. |
| 0x1178 | `CDamageVulnerability` (0x30) | [0x0000FD48] |
| 0x11A8..0x11CF | ten floats: 3C8-derived, 1.5, 0.75, 1.0, 0, 0, 0, 1.0, 0, 0 | [0x0000FD64].. |
| 0x11CC / 0x11D0 | index 0 and a pointer to element 0 of a 0x38-stride array at 0xF2C | [0x0000FDAC], [0x0000FDC0] |
| 0x11D4..0x11EB | zeros and one float read back from that array element | [0x0000FDC4].. |
| 0x11EC..0x11EF | four bitfield bytes (bit31/27/26/25 set variously; 0x11ED bit25 = `(loader temp)+0x320`) | [0x0000FDEC]..[0x0000FF3C] |

Ctor argument order and the `CPatterned` base call are in §f once extracted; the dtor
`fn_30_E5EC` destroys, in order, the vulnerability at 0x1178, `fn_30_E4EC` at 0x109C (a pointer
the ctor never initialises — a retail bug worth remembering), the nine tokens, `fn_30_5E4` at
0x0DEC, `fn_30_EACC` at 0x0D68, the two collision managers, the vulnerability inside 0x7C0, then
`fn_30_2C44` (the 0x7C0 block) and `CPatterned`'s dtor.

## f. The second class (0x5E0, `CPhysicsActor`)

`fn_30_3478` builds a `CStaticRes` model, then `__nw__`s 0x5E0 bytes at 0x0000361C and calls
`fn_30_15924` (which calls `CPhysicsActor::CPhysicsActor` and installs `lbl_30_data_C44`), names
it with a rodata string at `lbl_30_rodata_12C + 0x53F`, adds it to the state manager
(`AddObject`) and strips material 0x23 from it; the parent keeps its id at +0x1094 and clears
bit 0x20 at +0x11EE. Its vtable's 14 local overrides are `fn_30_11CD8`, `1548C`, `1466C`,
`13E68`, `141B8`, `13ECC`, `13C5C`, `13C40`, `13B90`, `13B88`, `13BA4`, `1464C`, `11E44`,
`142BC`. What actor it is (`CIngBoostBallGuardianCrystal`?) is still open: the name string and the
class's own behaviour are the next thing to read (`fn_30_1466C`, `fn_30_13E68`).

## g. Function map

`fn_30_<hex>` names are offsets into the module's `.text`. Full inventory with sizes and DOL
callees: regenerate with the `.fn` walk used for `docs/module-analysis/Sandworm_functions.tsv`
(309 functions). Rough shape by address: 0x0..0x130 trivial accessors; 0x130 loader;
0xEC6C ctor; 0x110C4 the `.ctors` FSM name-table copy; 0x11CD8.. the second class and the big
state bodies. The vtable word lists for both classes: `/tmp/opencode/ibbg_vtable.md`.

## h. Open

- transitions of the FSM (needs a running class; `MP2_SB_FSM`-style probe).
- what the second class is (§f).
- the "PuddleCollisionActor" named child and the crystal behind `HasCollidedWithCrystal`.

