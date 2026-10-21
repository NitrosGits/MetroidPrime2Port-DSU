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

### b.1 The transitions: the actor's 'FSM2' asset

The transitions are not in the REL; they are in the actor's `'FSM2'` asset, and the probe
(`MP2_BG_FSM=1`, `PortCIngBoostBallGuardian.cpp`) dumps it: every state, trigger, code function and
sub-machine with each transition's *target* (the transition's own `mName` is empty in this asset, so
the target node is the information). The dump is 442 lines; the raw one is
`/tmp/opencode/bg_fsm_dump.txt` (the probe prints it again on every run).

The asset is `b5d55287` (the actor's `stateMachine` is invalid, so the `FSM2` branch is the one
that is used). It is a wrapper around three sub-machines:

| machine | states | triggers | codes | what it is |
|---|---|---|---|---|
| top level | 8 | 16 | 0 | the wrapper: it decides which sub-machine to enter (`IngCorporeal`, `IngPuddle`, `IngAttackPattern`) |
| `IngAttackPattern` | 6 | 5 | 0 | `SelectTarget`, `BecomeCorporeal`, `BecomeIngSpot`, `FollowAttackPattern`, `Start` |
| `IngCorporeal` | 20 | 52 | 9 | the Ing's own FSM: `SelectTarget`, `Alert`, `FaceTarget`, `Lurk`, `PathFind`, `Taunt`, `ArmSwipe`, `FindMiniPortals`, `MiniPortalAttack`, `BecomeIngSpot`, `BecomeBodyProjectile`, `BodyProjectileFlight`, `SuckEnergy`, `ExitHost`, `WaitForFSMTransition` |
| `IngPuddle` | 20 | 46 | 5 | **the Boost Guardian's own FSM**: the same Ing states plus `BoostAttack` and `BoostModeRetreat`, and the boost triggers `IsBoostPuddleForm`, `IsCurrentlyInBoostPuddleForm`, `IsBoostOver`, `ShouldBoost`, `HasCollidedWithCrystal`, `BoostModeRetreatOver` |

54 states, 119 triggers, 14 codes and 3 sub-machines in all; 242 transitions. The boost half's
graph, from the `IngPuddle` sub-machine:

- `Start` -> `UseProjectileFSMEntry` (code `SetRecoverDest`), `IsCurrentlyInBoostPuddleForm`
- `Lurk` -> `IsOffPath`, `HasTarget`
- `FindCoverPoint` -> `IsBoostPuddleForm`, `HasCoverPoint` (code `SetCoverDest`)
- `IngSpotPathFind` -> `PathOver`, `PathShagged`, `Delay`, `IsCurrentlyInBoostPuddleForm`,
  `IsBoostPuddleForm`, `ShouldBoost` (code `SetPointCoverDest` on `PathShagged`)
- `IngSpotPointPathFind` -> `PointPathOver`
- `BecomeCorporeal` -> `StateOver`
- `BecomeWallProjectile` -> `StateOver`
- `FailSafeMode` -> `StateOver`
- `WaitForFSMTransition` -> `Delay`
- `SeekWallPoint` -> `HasLineOfSight`, `Delay`
- `BoostAttack` -> `HasCollidedWithCrystal`, `IsBoostOver`
- `BoostModeRetreat` -> `BoostModeRetreatOver`, `PathOver`
- `FaceTarget` -> `StateOver`

The host's machine finds a node's handler **by name** (`CGenericFSM2State::SetStateFunctions` and
its two siblings walk the machine's node lists and match the name, and the call is propagated into
the sub-machines), so the port's tables - the REL's 49 triggers, 28 states and 9 code functions -
bind to the nodes of all three sub-machines whose names they carry. A name the asset does not have
(`BoostAttack` outside `IngPuddle`, `HasPatrolPath`, `InDetectionRange`, `Delay`, ...) simply has
no node, and the port's body for it never runs: the asset's graph, not the REL's tables, decides
what runs.

### b.2 Which function implements which node


`docs/module-analysis/BoostGuardian_fsm.tsv` (86 rows: 49 triggers, 28 states, 9 code functions)
gives every node's name and the `fn_30_<hex>` that implements it, with the evidence (the .data
label of the 12-byte record the `.ctors` initialiser `fn_30_110C4` copies into the table entry,
and the store offsets). The mechanism: the three destination tables are .data objects
`lbl_30_data_250` (triggers), `lbl_30_data_6B0` (states) and `lbl_30_data_8DC` (codes), each entry
16 bytes with the name pointer in word 0; the 86 source records are .data objects at a 0xC stride
from `lbl_30_data_4` and hold `{0, 0xFFFFFFFF, fn_30_XXXX}`, and the initialiser copies them in
order - the first 49 into the trigger table, the next 28 into the state table, the last 9 into the
code table. `fn_30_C6CC` (SetupStateMachine) then hands the three tables to the machine with the
counts 0x31, 0x1C and 9.

The boost half's own bodies (the only ones this module does not share with the Ing REL):

| node | function | what it does |
|---|---|---|
| `HasCollidedWithCrystal` | `fn_30_B788` | bit 0 of the byte at guest 0x11EE |
| `ShouldBoost` | `fn_30_B794` | bit 4 of that byte |
| `IsCurrentlyInBoostPuddleForm` | `fn_30_B7A0` | bit 7 of the byte at guest 0x11EF |
| `IsBoostPuddleForm` | `fn_30_B7AC` | bit 3 of 0x11EE and not bit 1 of 0x11ED |
| `IsBoostOver` | `fn_30_B7D4` | bit 5 of 0x11EE |
| `BoostModeRetreatOver` | `fn_30_B76C` | the u16 at guest 0x1096 is kInvalidUniqueId |
| state `BoostAttack` | `fn_30_79C0` | Activate spawns the second class (`fn_30_3478`) and deactivates the collision manager at 0xD64; Update copies the child's translation (+0x54/+0x58/+0x5C, z from 0x1174) onto the actor and sets bit 5 of 0x11EE when `fn_30_13B98()` is not zero; Deactivate stops the child (its byte 0x5D8 bit 0), sends the script state `EXIT` (0x45584954), randomises the float at 0x11E8 from the array element at 0x11D0 (+0x10, +0x14), and, only when bit 0 of 0x11EE (HasCollidedWithCrystal) is set, clears it and zeroes the float at 0x11DC (0x7B40); activates the collision manager at 0xD64 and deletes the child (the id at 0x1094) |
| state `BoostModeRetreat` | `fn_30_7728` | Activate clears bit 4 of 0x11EC, sets bit 7 of 0x11EE and finds two nav points (`fn_30_2F9C` with 0xE and 0xF), storing their ids at 0x1096 and 0x1098, sets the navigation's destination and the navigation's face target to kInvalidUniqueId; Update clears 0x1096 when the actor is within 1.0 of the first point; Deactivate looks at the second point, clears bit 7 of 0x11EE, sends the script state `CTIM` (0x4354494D) and tail-calls this module's IngSpotPathFind (`fn_30_9DA0`) |

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
address per row: `docs/module-analysis/BoostGuardian_layout.md`, 0x11F0 total, base 0x7C0):

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
| 0x1120 | `mPortalPlaneNormal` = `Normalize(0, 0, 1)`, then 0x112C = its dot with the translation (0x54) | [0x0000FCA8]..[0x0000FCFC] |
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
`142BC`. It is the boost ball: named "BoostBallGuardian", DOL type tag 148 (parent
`CPhysicsActor`; BoostAttack casts the child with `fn_800979F0` = `TryCast(entity, 0x94)`). Host:
`CBoostBallGuardianBall`, `src/MetroidPrime/Enemies/PortCBoostBallGuardianBall.cpp`.

Its asm is in `build/G2ME01/IngBoostBallGuardian/asm/auto_00_00011CD8_text.s` (46 functions,
0x11CD8..0x164D0, about 17.5 KB; the older single-file dump misses the small ones). Read so far:

- *Spawn (`fn_30_3478`, 0x318).* `fn_30_340C` stores the player's aim position (vtable +0x54, z
  replaced by the actor's) at 0x1160; `LookAt` from the actor to it, both z = 0 (0x34D4); sends
  'ENTR' (0x350C); clears 0x11EE 0x10; `CStaticRes(model data+0xA4, scale data+0x00)` (0x3548);
  `CHealthInfo` from the actor's `GetHealthInfo()` (vtable +0x3C, `fn_30_388C`, then hp copied to
  +4); spawn position from `fn_30_324C` -> 0x116C; material list bits `lbl_30_data_9B4`/`9B8`;
  name rodata_12C+0x546; `CEntityInfo(area, NullConnectionList, kInvalidEditorId)`, active.
- *Ctor (`fn_30_15924`, 0x6A0)* arguments: uid, name, info, xf, pos (0x116C), model, actor params,
  then on the stack the owner id, the boost data (0xDEC), the phase index (0x11CC -> 0x2D0), the
  boost vulnerability (0xEFC -> 0x570 via `fn_30_AD4`), a block at 0x109C (-> 0x4FC via
  `fn_30_160D0`; 0x109C is the guardian's `FindConnectedObjects(kSS_Modify, kSM_Activate)` from
  kSM_Create, 0xE06C - platform `001c0078` in area 28), the material list, the health info (-> optional at 0x5A0, flag 0x5C0).
  `CPhysicsActor(..., SMoverData(mass data+0x0C), skDefaultStepData)`; boost data copied to
  0x2E4 (`fn_30_10250`); `CCollidableSphere` at 0x4D0 (radius 1.0 without a model, else half the
  bounds height); three particle generators 0x564/0x568/0x56C from data +0xA8/+0xAC/+0xB0
  (`fn_30_16238`: the trail at +0xA8 is 'SPSC', a `CParticleSpawnSystem`; shield +0xAC and
  charge-up +0xB0 are 'PART'); spline end time from the speed spline's knots (0x528); coefficient of
  restitution 1.0; material filter adds `lbl_30_data_C0C`.
- *Vtable:* dtor `11CD8`, Think `1548C` (0x498), AcceptScriptMsg `1466C` (0x270), AddToRenderer
  `13E68`, Render `141B8` (0x104), PreRenderAllViewports `13ECC` (0x2EC), HealthInfo/GetHealthInfo
  `13C5C`/`13C40` (0x5A0 if 0x5C0), GetDamageVulnerability x2 `13B90`/`13B88` (0x570),
  GetTouchBounds `13BA4` (the sphere's AABB), Touch `1464C` (base call), GetCollisionPrimitive
  `11E44` (0x4D0), CollidedWith `142BC` (0x390).
- *Think* switches on 0x2D4: 0 = grow in (`137C0` 0x3BC, `13C78` effects mask 4, `152D4` scale),
  1 = roll (spline time 0x51C, the speed vs data+0x90, trail fade, `13C78` mask 7, `12AB0` 0xC00
  the roll itself), 2 = shrink (`13760`: 0.3 s, sets 0x5D8 0x04, which `fn_30_13B98` returns and
  BoostAttack's Update turns into 0x11EE 0x20); then every 0.2 s it applies the data+0x14
  (morph ball) or +0x30 (first person) damage to the touched id at 0x4F8, knocked 60 degrees off
  the velocity, and runs the rolling sfx (`1282C`).
- *Data:* `CBoostBallGuardianData::mSpeed` is the `boostBallSpeed` spline (guest data +0x4C); the
  pair at +0xC0/+0xC4 are the loader's constants 1000.0/1.0 (`fn_30_748` 0x8E8/0x8EC), the
  rolling sfx's `C3DEmitterParmData(maxDist, distComp)` (0x129C8): `mSoundMaxDistance`/
  `mSoundDistanceComp`.
- *Ported (slices a, b):* ctor, dtor, the small overrides, `fn_30_16238`, spawn `3478` with `340C`/
  `324C`, BoostAttack `79C0`; Think `1548C` with grow-in `137C0`, effects `13C78`, scale/fade
  `152D4`, shrink `13760`, rolling sfx `1282C`, PlaySound `12904`, the roll `12AB0` (impulse
  `11E70`, path check `148DC`, steering `12124`/`123CC`, aim at player `11F2C`, cover point
  `12510`), CollidedWith `142BC` and AcceptScriptMsg `1466C` (kSM_Create fills the per-roll
  bounce counts at 0x5C8; the roll pops one per spline run and shrinks when none are left, which
  is what IsBoostOver sees). `FindBoostNavPoint` (`2F9C`) is a static taking the actor, so the
  ball shares it. Area 28: the ball rolls, bounces, ends BoostAttack and respawns on the next.
- *Contact sweep (slice c):* `14CCC` sweeps a first-person-radius sphere along the frame's motion
  above `mMinDamageSpeed`: a player hit becomes Think's `mTouchedId`, any other hit is damaged, and
  the first non-actor hit ends the speed spline (`m5D9_80`); damageable triggers nearer than the
  hit are damaged too. Area 28: hits the player and world actors.
- *Retail quirks kept:* `12510` takes its pick modulo the exhausted list index (-1), so it always
  heads for the first candidate; `13260` normalises a nav-point line it never reads.
- *Render (slice c):* AddToRenderer `13E68`, Render `141B8` (dark visor: alpha modulation),
  PreRenderAllViewports `13ECC` (render bounds = the effects' union). The ball draws (area 28).
- *0xD64 manager:* `CreateIngSpotManager` (`4A38`) builds one `PuddleCollisionActor` box (5,5,1.5,
  mass 1000) at `Root` in kSM_Create (0xDF24, after the body manager); `SetupManagerActors` (`4924`)
  gives it the health info and CPatterned's vulnerability. `10F9C`/`4BD8`/`4C58`/`4D74` are rstl
  vector inlines (reserve, push_back_unsafe, copy, dtor). Its retail lifecycle is in: Delete
  (0xE0CC), Activate (`mForm == kForm_IngSpot`, 0xE16C), Deactivate (0xE224), AIUpdateDisabled
  (0xE2EC), BecomeIngSpot on (0xAF70), BecomeCorporeal off (0xB144), the dtor; BoostAttack's
  calls are unguarded as in retail. Area 28: the box is built (8th collision actor).
  `DDA0`'s ACTV/DCTV gate (`f31` vs rodata_70) is the active bit taken before the base handles the
  message: ACTV acts only when inactive, DCTV only when active (ported). kSM_Create also builds the two
  "IngBlobEffect" trackers (`648C`, 0x1074 ingSpot / 0x1076 boost blob effect) and the "Ing Light"
  (`67B4`, 0x1084); ACTV turns the light on and sets the boss params ("BossBoostBallGuardian"), DCTV
  runs `510C` (LeaveTeam), turns the light off and zeroes both trackers' emission; Delete deletes
  all three. Their per-frame drivers are ported: `65C8` fades the light
  (0x11C4) on the head locator; `6108` drives one tracker on the surface under the guardian. The host's ACTV `JoinTeam`
  is the Ing's; Delete runs `6064` (the cover hint at 0x108E). Damage is retail's: XDMG runs `479C` (ing-spot form: `44A4`, the
  puddle collider or a hit the boost ball relays; else `4078`, a body collider) and sets the
  player-projectile bit; XRDG runs `4324` in the ing-spot form only. Both count only a CWeapon as
  the collider's last-touched object, so the console's `damage <collider>` (the player as attacker)
  moves no guardian HP, in retail too; real shots do. In the boost puddle form its colliders take 0x1178, a
  resist-everything vulnerability (ctor: 100.0, effect 3), so bombs arrive as XRDG at `4324` and
  feed 0x11D8. The puddle never kills (1 HP, puddle mode off
  for good); `4030` turns puddle mode on with damage. `D4F8`'s events 37/38 clear/set 0x11EF 0x10
  (`mBodyHitsLethal`) in the host's (Ing-derived) DoUserAnimEvent. `CA18` (UpdateHitDamageTime) tints
  with 0x1080 and the XENZ/XEXZ count 0x115C; its other readers are ported too: `C1B0`
  IsAggressive, `C05C` ShouldFleeSafeZone and PathFind's update (`A07C`).
- *0xD60 manager:* `CreateCollisionActorManager` is now `4E90` (kSM_Create 0xDF18): the seven
  `lbl_30_rodata_0` spheres (same table as the Ing's), `SetupManagerActors` (`4924`), `AddMaterial`
  Unknown54 + ProjectilePassthrough, filter Solid / CollisionActor, AIPassthrough, Player,
  Projectile, Bomb. It replaces the Ing-half `single_ptr` manager and `SetupCollisionActors`; both
  managers are raw pointers deleted in the dtor (0xD64 then 0xD60). Retail calls them unchecked
  everywhere but AIUpdateDisabled, and so does the host now. Area 28: a sphere hit relays to the
  guardian; the puddle box takes the IngSpot form's turns.
- *Fight check (2026-10-21, after the ctor timers 9f041d425):* a hook-free area-28 run
  (`MP2_BG_FSM=1 MP2_DEBUG_BG=1`) drops HP: a beam shot in the corporeal form 1000 -> 994, then
  bombs on the ing-spot form outside boost puddle mode (`44A4`, weapon type 4, x1.0) 10 each to 954;
  bombs in boost puddle mode only feed 0x11D8 (`4324`, resisted), as retail. Puddle-only bombing
  (the earlier driver) never moves HP; that is not a stuck form.

## g. Function map

`fn_30_<hex>` names are offsets into the module's `.text`. Full inventory with sizes and DOL
callees: regenerate with the `.fn` walk used for `docs/module-analysis/Sandworm_functions.tsv`
(309 functions). Rough shape by address: 0x0..0x130 trivial accessors; 0x130 loader;
0xEC6C ctor; 0x110C4 the `.ctors` FSM name-table copy; 0x11CD8.. the second class and the big
state bodies. The vtable word lists for both classes: `docs/module-analysis/BoostGuardian_vtable.md`.

### b.3 Where this module's copy of the Ing code differs from the Ing's

The port's Ing half is the Ing host body's code (branch `decomp/ing`), because this module is the
Ing REL with the boost half added. That is the right starting point, but it is not the same code
everywhere: comparing the two RELs' function sizes for the 78 nodes the two modules share, 61 are
the same size and **17 differ**, so for those the module's own body is the authority and the Ing
port's body is (at best) a near neighbour. These are the ones to read against this module's asm:

| node | Ing (`fn_29_`) | size | this module (`fn_30_`) | size |
|---|---|---|---|---|
| `CanGrappleTarget` | `fn_29_A098` | 0x68 | `fn_30_B7E0` | 0x5C |
| `ExitHost` | `fn_29_9D68` | 0x328 | `fn_30_B478` | 0x2EC |
| `BecomeCorporeal` | `fn_29_9A74` | 0x2F4 | `fn_30_B03C` | 0x43C |
| `BecomeIngSpot` | `fn_29_9750` | 0x2C8 | `fn_30_ABC4` | 0x41C |
| `Lurk` | `fn_29_8A2C` | 0x180 | `fn_30_A0D8` | 0x144 |
| `PathFind` | `fn_29_8808` | 0x224 | `fn_30_9F50` | 0x188 |
| `IngSpotPathFind` | `fn_29_8684` | 0x184 | `fn_30_9DA0` | 0x1B0 |
| `IngSpotPointPathFind` | `fn_29_8570` | 0x114 | `fn_30_9C7C` | 0x124 |
| `FollowAttackPattern` | `fn_29_82D4` | 0x1D0 | `fn_30_99D4` | 0x1DC |
| `BodyProjectileFlight` | `fn_29_7E30` | 0x4A4 | `fn_30_953C` | 0x498 |
| `SuckEnergy` | `fn_29_76B8` | 0x658 | `fn_30_8DD4` | 0x648 |
| `SeekWallPoint` | `fn_29_75C0` | 0xF8 | `fn_30_8CC0` | 0x114 |
| `FaceTarget` | `fn_29_7244` | 0x2D4 | `fn_30_8998` | 0x280 |
| `FindCoverPoint` | `fn_29_6DA0` | 0x418 | `fn_30_8520` | 0x3EC |
| `Dead` | `fn_29_5CB0` | 0x358 | `fn_30_7390` | 0x364 |
| `SetExitHostDest` | `fn_29_54E8` | 0x22C | `fn_30_6C40` | 0x23C |
| `SplatOntoMesh` | `fn_29_4F14` | 0x1F0 | `fn_30_6980` | 0x1F4 |

`BecomeCorporeal` and `BecomeIngSpot` are the ones the boost half changed most (the module's are
0x148 and 0x154 bytes bigger): they are where the form change meets the boost ball.

**Which Ing commit this module's Ing half matches.** The Ing half's bodies are the Ing host
body's (branch `decomp/ing`, `src/MetroidPrime/Enemies/PortCIng.cpp`), and they match that branch
at **`1776bb60b`** ("CIng SetRecoverDest's second point query is connected to the first": the second
point query's filter uses the first query's index `from` as its connected point, the asm keeping it
in r5 from 0x00005A2C). Of the 112 bodies the two share, 96 are token-identical after the renames
(`CIng::` -> `CIngBoostBallGuardian::`, `DebugIng` -> `DebugBg`, `OptionalPart` ->
`BgOptionalPart`, `PortIngPointSearch` -> `BgPointSearch`, `SolidFilter` -> the shared header's,
`[ing]` -> `[ibbg]`) and the 16 that differ are this module's adaptations, all of them the lure
safe zone the module does not have (`mLureSafeZoneId`, `mSafeZoneCount`, `mSafeZoneIngSpot`), the
grapple block and the trigger vulnerability it does not have, the exit host and the mini-portal
attack (the classes are not in this branch), and the probe's own activation in `Think`. A fix the
Ing session commits afterwards has to be carried over: the files are `PortCIng.cpp` against this
module's `PortCIngBoostBallGuardian{Triggers,States1,States2,Overrides}.cpp`. Note the two RELs'
*bodies* also differ for the 17 nodes in the table above, so a fix there is the Ing's fix and this
module's body is still to be read.

**Differences from `decomp/ing` introduced by the bit audit (2026-10-20).** The data flag byte
(this+0xAE0) is not the raw property here: the loader `fn_30_10C24` packs it
(`PackBgDataFlags` in `PortBoostGuardianShared.hpp`, with the asm offset of every rlwimi). The bit
values therefore differ from the Ing's 0x9d0 byte: `kFlag_AlwaysTargetable` 0x02,
`kFlag_NoPathFind` 0x04, `kFlag_NoFormChange` 0x08 (there is no no-grapple bit), and
`kFlag_StartAsIngSpot` (0x80) is always 0. Bodies that now differ from the Ing's copy:

- `CanGrappleTarget` (`fn_30_B7E0`): no data-flag guard.
- `Lurk` (`fn_30_A0D8`): sets `mAlert` from the target id alone, with no lookup. Its body-projectile
  roll is not gated by any data flag.
- `PathFind` (`fn_30_9F50`): the speed range is 1.0 or the walk/run ratio. It sets moved only when
  it actually paths, and has no `ApplySeparation`. Deactivate only stops.
- `BecomeCorporeal` (`fn_30_B03C`) clears `mCurrentlyInBoostPuddleForm` in Activate. Generate type 2
  needs `mBoostPuddleGenerate` and no `mProjectileFSMEntry`. `mProjectileFSMEntry` is cleared in
  Deactivate (not Activate), together with 0x11EE 0x04 and 0x1098.
- `BecomeIngSpot` (`fn_30_ABC4`) sets `mCurrentlyInBoostPuddleForm` and clears
  `mBoostModeRetreatActive` in Activate. Generate type is 5 when in the puddle form. Deactivate makes
  the form 3 only from form 7.
- `kSM_Create` (`fn_30_68F0`) reads the data byte's 0x80, not the guest bit.
- In the constructor, `mFoundMovementPos`, `mRenderModel` and `mBoostOver` start true (0xFE0C,
  0xFE3C, 0xFECC).

**The audit's three follow-ups, resolved against the asm (2026-10-20).**

- *Locomotion types.* This module passes only 0 and 1 to `SetLocomotionType` (ten calls), and the
  Ing REL does the same at the same places (`fn_29` 0x4CCC/0x4CF8, 0x9944/0x9954, 0x9C98/0x9CE8).
  The port had copied `decomp/ing`'s Relaxed(1)/Combat(3) for the ing-spot/corporeal pair; it is
  Crouch(0)/Relaxed(1): `kSM_Create` 0x6934/0x6960, `BecomeIngSpot` Update 0xAEF0 and Deactivate
  0xAF00, `BecomeCorporeal` Update 0xB36C and Deactivate 0xB3BC. The body/wall projectile calls
  (0xA6FC, 0xA838, 0xAA98, 0xAB6C) were already 0. **`decomp/ing`'s `PortCIng.cpp` has the same
  slip** (lines ~1333/1343/1381/1386/2945/2948) - not fixed there.
- *`IngSpotPathFind` (`fn_30_9DA0`).* No separation and no ball-pursuit speed (the Ing's shape).
  The speed is `ingSpotMaxSpeed` (data +0x08, 0x9EEC) or, when "fast", data +0x14
  (`unknown_0x50398a06`, `CIngSpotData::GetSpeedModifier`, 0x9EE4), times the current phase's
  `ingSpotSpeedScale` (`*(0x11D0)+0x34`, 0x9EF8). Fast = retreating (0x11EE 0x80) or (puddle form
  0x11EF 0x80 and boost pending 0x11EE 0x10 and a cover hint, `fn_30_60C0` = `GetCoverHint`).
  `FaceDirection` only while 0x1098 (the retreat's second point) is empty (0x9F10).
- *`fn_30_30E4` / `fn_30_3190` / `fn_30_3068`* are ported (`PortCIngBoostBallGuardianTriggers.cpp`,
  `SetIngSpotManagerVulnerability` / `SetBodyManagerReflect` / `SetBoostPuddleMode`):
  `30E4(b)` gives every collision actor of the 0xD64 manager the boost-ball vulnerability (0x1178)
  or the ing spot's (0xA5C) - a no-op on the host, which never builds that manager; `3190(b)` sets
  0x11EE 0x02 and gives the 0xD60 manager (the host's `mCollisionActorManager`) the reflect
  vulnerability or `CPatterned::GetDamageVulnerability()`; `3068(b)` with 0x11EF 0x20 (ctor 1)
  sets 0x11EE 0x10/0x08/0x04 and zeroes 0x11D4, else clears 0x11EE 0x08, zeroes 0x11D8/0x11DC and
  reloads 0x11E4 from the phase's `boostPuddleModeTime`. Callers: `BecomeIngSpot` 0xACC8-0xACE8
  (all true), `BecomeCorporeal` 0xB094 (`30E4(false)`) and 0xB17C (`3190(false)`); `3068` is also
  called from `fn_30_4030`/`4078`/`4324`/`44A4`/`7390` (unported boost half) and from
  `fn_30_5D90` (ported). The current phase (0x11D0) is `mPhaseIndex` (0x11CC); `fn_30_D94C`'s
  phase advance (0xDB18..0xDBE8) moves it with the damage taken and sets `mSpeed` (0x3C8).
- *Settled: the `IngSpotPathFind` speed fields.* The asm reads `this+0x7C8` / `this+0x7D4` (data
  +0x08 / +0x14). Unlike the Ing REL (whose block has `unknown_0xc620183a` / `aggressiveness`
  there), this module's guardian block starts with the SLdr struct verbatim (`fn_30_1056C` ->
  `fn_30_10694` [0x00010588] copies +0x00..+0xEF word by word), and the loader `fn_30_2344` stores
  `ingSpotMaxSpeed` at SLdr+0x08 [0x00002808] and `unknown_0x50398a06` at +0x14 [0x00002838]
  (BoostGuardian_datablocks.md section 1). The host passes those two as `CIngSpotData`'s
  `maxSpeed` / `unknown1c` (`PortCIngBoostBallGuardian.cpp` ~403), so `GetMaxSpeed()` /
  `GetSpeedModifier()` are the right fields; no change. Guest 0x3C8 (`fn_30_D94C`'s phase
  advance) is `CPatterned::mSpeed` (counted from `mStateMachine` 0x350 and from the 0x420 flag word).
- *`fn_30_2F9C`* (`FindBoostNavPoint`, Boost.cpp) is ported, and with it `BoostModeRetreat`'s
  Activate: hint type 0xE is the destination (0x1096), 0xF the facing point (0x1098).

`tools/asmcheck.py check IngBoostBallGuardian PortCIngBoostBallGuardian.cpp --only
MASK,XBIT,CROP,XCMP,XSTORE,OFFS` leaves three XBIT hits on the ctor (`mFoundMovementPos`,
`mIngSpotBlobVisible`, `mRenderModel`), and the same kind on `fn_30_9DA0` in States2: false
positives. Those Ing-half members state the Ing's offsets (`CIng: 0xE5C bit 3`) because most bodies
cite `fn_29_*` and `review_decomp_diff.sh` checks them against the Ing REL; the guardian's ctor
stores the guest bits (0x11EC 0x10 at 0xFE0C, 0x11ED 0x40 at 0xFE60, 0x11EC 0x01 at 0xFE3C).
Restating them as guest bits clears these and adds 24 XBITs on the Ing-module check, so they stay.
The MASK hit (`mBoostPuddleGenerate` placed by the host bitfield layout at 0x11EF 0x04) is fixed
with a position comment (`guest 0x11EE bit 2`).

## h. The unmarked functions: what each one is

`tools/port_progress.py` counted 204 of the module's 318 functions as marked (206 after this
slice), so 114 were unmarked. Every one of them is either the REL's own copy of code the host's
C++ already does or compiler output; this section says which is which, so the number stops
being a mystery.

A **REL copy of host/DOL code** never gets a marker: the host's own C++ (or the compiler) does
the work, and a marker would claim a function the port does not implement. Where the same
function also exists in the GameCube executable, the table gives its DOL address (from
`config/G2ME01/symbols.txt`), so the copy can be checked against the original.

The classes: 10 the REL runtime and the module's own entry glue, 47 the compiler's weak
deleting dtors and copies of the module's own types, 31 the template inlines the compiler
emitted out of line, 15 the REL's own copies of a DOL / host class member, 2 inline accessors
and thunks, 5 the module's own memberwise copies of its two parameter blocks and 4 with
retail behaviour (section h.1).

The "called from" column is the REL's own call graph, read from the disassembly; a "-" means
nothing in the module calls it and no table names it.

##### rel - The REL runtime and the module's own entry glue

| retail function | size | what it is | DOL counterpart | called from |
|---|---|---|---|---|
| `RELMain` | 0x20 | the REL's entry: it calls fn_30_100 | - | _prolog |
| `RELExit` | 0x24 | the REL's exit: the DOL's SetSIngBoostBallGuardian_FuncPtrs (0x8022FFC4) with null | - | _epilog |
| `fn_30_100` | 0x30 | the installer: fn_30_130 into lbl_30_bss_6C, then the DOL's SetSIngBoostBallGuardian_FuncPtrs (0x8022FFC4) | - | RELMain |
| `_prolog` | 0x24 | the runtime's prolog: ModuleConstructors, then RELMain | - | - |
| `_epilog` | 0x24 | the runtime's epilog: ModuleDestructors, then RELExit | - | - |
| `ModuleConstructors` | 0x4C | the module's .ctors list | - | _prolog |
| `ModuleDestructors` | 0x4C | the module's .dtors list | - | _epilog |
| `__destroy_global_chain` | 0x58 | the runtime's global destructor chain | - | - |
| `__register_global_object` | 0x1C | the runtime's global-object registration | - | fn_30_12510, fn_30_8520 |
| `_unresolved` | 0xC4 | the runtime's unresolved-relocation stub | - | - |

##### gen - The compiler's weak deleting dtors and copies of the module's own types

| retail function | size | what it is | DOL counterpart | called from |
|---|---|---|---|---|
| `fn_30_5E4` | 0xB8 | the module's own data block: the vulnerability's dtor, then the block's | - | fn_30_11CD8, fn_30_130, fn_30_E5EC |
| `fn_30_69C` | 0x58 | the SLdr damage-info block's dtor chain | - | fn_30_14AC, fn_30_1988, fn_30_5E4 |
| `fn_30_6F4` | 0x54 | the SLdr damage-info block's inner dtor | - | fn_30_69C |
| `fn_30_B30` | 0x54 | a member's weak deleting dtor | - | - |
| `fn_30_FEC` | 0x8C | a member's weak deleting dtor | - | - |
| `fn_30_12A0` | 0x6C | a CToken holder's weak deleting dtor | - | - |
| `fn_30_130C` | 0x54 | a CToken holder's weak deleting dtor | - | - |
| `fn_30_1360` | 0x64 | the CAnimData member's weak deleting dtor | - | - |
| `fn_30_13C4` | 0x60 | the CModelData member's weak deleting dtor | - | - |
| `fn_30_18B4` | 0x3C | a member's weak deleting dtor | - | - |
| `fn_30_1078` | 0x64 | the vulnerability + SLdr block's weak deleting dtor (unreferenced) | - | - |
| `fn_30_1424` | 0x88 | the loader's SLdr block's dtor (actor parameters, patterned typedef, editor properties) | - | fn_30_130 |
| `fn_30_14AC` | 0x94 | the loader's SLdr damage-info block's dtor | - | fn_30_1424 |
| `fn_30_2C44` | 0x88 | the SLdr damage-vulnerability / plasma-beam block's dtor | - | fn_30_1078, fn_30_130, fn_30_1424, fn_30_E5EC |
| `fn_30_6B74` | 0x48 | a state functor's deleting dtor (an FSM2 node vtable) | - | a .data table |
| `fn_30_7D00` | 0x5C | a state functor's deleting dtor (an FSM2 node vtable) | - | a .data table |
| `fn_30_84C4` | 0x5C | a state functor's deleting dtor (an FSM2 node vtable) | - | a .data table |
| `fn_30_A308` | 0x5C | a state functor's deleting dtor (an FSM2 node vtable) | - | a .data table |
| `fn_30_A8C0` | 0x5C | a state functor's deleting dtor (an FSM2 node vtable) | - | a .data table |
| `fn_30_AFE0` | 0x5C | a state functor's deleting dtor (an FSM2 node vtable) | - | a .data table |
| `fn_30_890C` | 0x8C | a member's weak deleting dtor | - | - |
| `fn_30_941C` | 0x50 | a wide rstl string member's weak deleting dtor | - | - |
| `fn_30_946C` | 0x68 | a CToken holder's weak deleting dtor | - | - |
| `fn_30_94D4` | 0x68 | a CToken holder's weak deleting dtor | - | - |
| `fn_30_C46C` | 0x3C | a member's weak deleting dtor | - | - |
| `fn_30_CE14` | 0x3C | a member's weak deleting dtor | - | - |
| `fn_30_D8D8` | 0x74 | the SLdr plasma-beam block's weak deleting dtor (unreferenced) | - | - |
| `fn_30_E4EC` | 0x54 | the connected-ids vector member's weak deleting dtor | - | fn_30_11CD8, fn_30_DDA0, fn_30_E5EC |
| `fn_30_E540` | 0x58 | a CToken holder's weak deleting dtor | - | - |
| `fn_30_E598` | 0x54 | a CToken holder's weak deleting dtor | - | - |
| `fn_30_EA38` | 0x94 | CPatterned's token member's weak deleting dtor | - | fn_30_E854 |
| `fn_30_EACC` | 0x58 | the CSurfaceAlignmentHelper chain's weak deleting dtor | - | fn_30_E5EC |
| `fn_30_EB24` | 0x58 | the CSurfaceAlignmentHelper chain's weak deleting dtor | - | fn_30_EACC |
| `fn_30_EB7C` | 0x58 | the CSurfaceAlignmentHelper chain's weak deleting dtor | - | fn_30_EB24 |
| `fn_30_EBD4` | 0x98 | the CSurfaceAlignmentHelper chain's weak deleting dtor | - | fn_30_EB7C |
| `fn_30_101A8` | 0x3C | a member's weak deleting dtor (the ball's section) | - | - |
| `fn_30_101E4` | 0x6C | a CToken holder's weak deleting dtor (the ball's section) | - | - |
| `fn_30_1049C` | 0x58 | the CCollisionActorManager member's weak deleting dtor (unreferenced) | - | - |
| `fn_30_104F4` | 0x3C | a member's weak deleting dtor (unreferenced) | - | - |
| `fn_30_10530` | 0x3C | a member's weak deleting dtor (unreferenced) | - | - |
| `fn_30_11DF0` | 0x54 | a member's weak deleting dtor (the ball's section) | - | fn_30_11CD8 |
| `fn_30_127C0` | 0x3C | a member's weak deleting dtor (the ball's section) | - | - |
| `fn_30_3790` | 0x50 | an rstl string member's weak deleting dtor | - | - |
| `fn_30_37E0` | 0x58 | a member's weak deleting dtor | - | fn_30_3478, fn_30_38E0, fn_30_3CB0, fn_30_6980, fn_30_7390, fn_30_B478, fn_30_D4F8 |
| `fn_30_3838` | 0x54 | a member's weak deleting dtor | - | fn_30_37E0 |
| `fn_30_4D18` | 0x5C | an rstl string member's weak deleting dtor | - | - |
| `fn_30_E854` | 0x1E4 | CPatterned's own teardown (its token member, body state, vulnerability) | - | fn_30_E5EC |

##### rstl - The template inlines the compiler emitted out of line

| retail function | size | what it is | DOL counterpart | called from |
|---|---|---|---|---|
| `fn_30_9E0` | 0x44 | the phase vector's copy constructor | - | fn_30_10250, fn_30_748 |
| `fn_30_A24` | 0x68 | the phase vector's copy assignment (the element loop) | - | fn_30_9E0 |
| `fn_30_A8C` | 0x20 | the phase struct's copy assignment | - | fn_30_A24, fn_30_F30 |
| `fn_30_AAC` | 0x28 | the phase struct's self-assignment guard | - | fn_30_A8C |
| `fn_30_F30` | 0x48 | the phase vector's element destructor | - | fn_30_130 |
| `fn_30_AD4` | 0x5C | the CDamageVulnerability's copy constructor | - | fn_30_10250, fn_30_1056C, fn_30_10C24, fn_30_15924, fn_30_748 |
| `fn_30_B84` | 0x130 | rstl's vector allocate (the damage-vulnerability array) | - | fn_30_AD4 |
| `fn_30_CB4` | 0xAC | the boost block's copy constructor | - | fn_30_10250, fn_30_748 |
| `fn_30_D60` | 0x1D0 | rstl's vector allocate (the damage-vulnerability array) | - | fn_30_CB4 |
| `fn_30_4BD8` | 0x38 | the joint-sphere vector's push_back_unsafe | - | fn_30_4A38, fn_30_4E90 |
| `fn_30_4C10` | 0x20 | the joint-sphere vector's element constructor | - | fn_30_1105C, fn_30_4BD8 |
| `fn_30_4C30` | 0x28 | the joint-sphere vector's element copy | - | fn_30_4C10 |
| `fn_30_4D74` | 0x84 | the joint-sphere vector's destructor | - | fn_30_4A38, fn_30_4E90 |
| `fn_30_4DF8` | 0x38 | the material-filter vector's element constructor | - | fn_30_4D74 |
| `fn_30_4E30` | 0x60 | the material-filter vector's element copy | - | fn_30_4DF8 |
| `fn_30_10E04` | 0xA4 | rstl's vector allocate (the connected-ids array) | - | fn_30_E430 |
| `fn_30_10EA8` | 0x98 | rstl's vector copy (the connected-ids array) | - | fn_30_1FB4 |
| `fn_30_10F40` | 0x5C | the connected-ids vector's element copy | - | fn_30_10EA8 |
| `fn_30_10F9C` | 0xC0 | rstl's vector reserve (the joint-sphere array) | - | fn_30_4A38, fn_30_4E90 |
| `fn_30_1105C` | 0x68 | the joint-sphere vector's element constructor | - | fn_30_10F9C |
| `fn_30_136B0` | 0x4C | rstl's vector insert (the ball's near list) | - | fn_30_12AB0 |
| `fn_30_136FC` | 0x64 | rstl's vector insert (the ball's near list) | - | fn_30_136B0 |
| `fn_30_14C54` | 0x78 | rstl's vector erase (the ball's near list) | - | fn_30_148DC, fn_30_14CCC |
| `fn_30_1642C` | 0xA4 | rstl's vector reserve (the ball's bounce counts) | - | fn_30_1466C |
| `fn_30_CD78` | 0x9C | rstl's optional_object<CAABox> assignment | - | fn_30_13ECC, fn_30_CC34 |
| `fn_30_E430` | 0xB0 | rstl's vector assignment (the connected-ids array) | - | fn_30_DDA0 |
| `fn_30_E4E0` | 0xC | rstl's vector's element destructor helper | - | fn_30_E430 |
| `fn_30_10DC` | 0x40 | the CModelData copy's optional-model branch | - | fn_30_130 |
| `fn_30_111C` | 0x20 | the optional model data's copy | - | fn_30_10DC |
| `fn_30_113C` | 0x28 | the optional model data's null guard | - | fn_30_111C |
| `fn_30_160D0` | 0x100 | the connected-ids vector's copy constructor (the ball's) | - | fn_30_15924 |

##### dol - The REL's own copies of a DOL / host class member

| retail function | size | what it is | DOL counterpart | called from |
|---|---|---|---|---|
| `fn_30_388C` | 0x54 | the CHealthInfo copy constructor | 0x80070D60 `__ct__11CHealthInfoFRC11CHealthInfo` | fn_30_160A8, fn_30_3478, fn_30_4924 |
| `fn_30_33B0` | 0x5C | the CRayCastResult copy constructor | 0x80015CAC `__ct__14CRayCastResultFRC14CRayCastResult` | fn_30_324C |
| `fn_30_A91C` | 0x40 | CVector3f::Cross | 0x8001994C `Cross__9CVector3fFRC9CVector3fRC9CVector3f` | fn_30_11F2C, fn_30_12124, fn_30_123CC, fn_30_12AB0, fn_30_A45C |
| `fn_30_D1C4` | 0x6C | the CMotionState copy constructor | 0x8000F200 `__ct__12CMotionStateFRC12CMotionState` | fn_30_CFC4 |
| `fn_30_161D0` | 0x68 | the CMotionState copy constructor (the ball's) | 0x8000F200 `__ct__12CMotionStateFRC12CMotionState` | fn_30_15924 |
| `fn_30_6710` | 0xA4 | the CLight copy constructor | 0x80038C9C `__ct__6CLightFRC6CLight` | fn_30_65C8 |
| `fn_30_1164` | 0x13C | the CModelData copy constructor (three optional tokens, each locked) | 0x80018FBC `__ct__10CModelDataFRC10CModelData` | fn_30_113C |
| `fn_30_4C58` | 0xC0 | the CJointCollisionDescription copy constructor | 0x80137214 `__ct__26CJointCollisionDescription...` | fn_30_4A38, fn_30_4C30, fn_30_4E90 |
| `fn_30_160A8` | 0x28 | the CHealthInfo copy constructor (the ball's optional) | 0x80070D60 `__ct__11CHealthInfoFRC11CHealthInfo` | fn_30_16088 |
| `fn_30_16088` | 0x20 | rstl's optional_object<CHealthInfo> assignment (the ball's) | 0x8016FCB8 `assign__optional_object<CHealthInfo>` | fn_30_15FF4 |
| `fn_30_15FF4` | 0x94 | rstl's optional_object<CHealthInfo> assignment (the ball's) | 0x8016FCB8 `assign__optional_object<CHealthInfo>` | fn_30_15FC4 |
| `fn_30_15FC4` | 0x30 | the ball's optional health-info assignment | 0x8016FC88 `__as__optional_object<CHealthInfo>` | fn_30_15924 |
| `fn_30_1F08` | 0xAC | the CMayaSpline copy constructor (the loader's) | 0x80080B3C `__ct__11CMayaSplineFRC11CMayaSpline` | fn_30_1988 |
| `fn_30_1FB4` | 0xE0 | the CMayaSpline assignment (the loader's) | 0x80080DB8 `__as__11CMayaSplineFRC11CMayaSpline` | fn_30_1F08 |
| `fn_30_2094` | 0xC | the CMayaSpline's knot-array release | 0x800327FC `__dt__11CMayaSplineFv` | fn_30_1FB4 |

##### acc - Inline accessors and thunks

| retail function | size | what it is | DOL counterpart | called from |
|---|---|---|---|---|
| `fn_30_90` | 0x2C | the vtable's HealthInfo() thunk: it calls the base's slot 9 (vslot 0x38) | - | a .data table |
| `fn_30_4028` | 0x8 | an unreferenced accessor (the word at 0x254) | - | - |

##### host - The module's own memberwise copies of its two parameter blocks

| retail function | size | what it is | DOL counterpart | called from |
|---|---|---|---|---|
| `fn_30_10250` | 0x24C | the boost block's memberwise copy into the object (guest 0xDEC) | - | fn_30_15924, fn_30_EC6C |
| `fn_30_1056C` | 0x128 | the guardian block's memberwise copy into the object (guest 0x7C0) | - | fn_30_EC6C |
| `fn_30_10694` | 0x240 | the guardian block's memberwise copy, part two | - | fn_30_1056C, fn_30_10C24 |
| `fn_30_108D4` | 0x2BC | the guardian block's memberwise copy, part three | - | fn_30_10694 |
| `fn_30_10B90` | 0x94 | the guardian block's memberwise copy, part four | - | fn_30_10694 |

##### beh - Retail behaviour

| retail function | size | what it is | DOL counterpart | called from |
|---|---|---|---|---|
| `fn_30_68F0` | 0x90 | the kSM_Create body: the form, the locomotion type and the surface alignment | - | fn_30_DDA0 |
| `fn_30_6014` | 0x50 | the cover hint claim: SetInUse(true), then the hint's id into 0x108E and 0x1090 | - | fn_30_8520 |
| `fn_30_5260` | 0x170 | the separation steering (the module never calls it) | - | - |
| `fn_30_110C4` | 0xC14 | the .ctors initialiser: the 86 FSM name/function records into the three tables | - | - |

### h.1 The behaviour functions, branch by branch

Four functions are behaviour rather than a copy. The two the fight reaches are marked; the
other two are dead in retail (nothing calls them and no table names them), so they stay
unmarked. Every branch of each is listed below with what the run proved.

The evidence is one run of the area (world `3dfd2249`, area
`06_swamp_boostballguardian_dark`, layer 5, `MP2_BG_FSM=1 MP2_DEBUG_BG=1 MP2_GODMODE=1`):
the player's start point is 122 units from the guardian, so the console's `tp 14.9 290 58`
puts it inside the guardian's 100-unit detection range. The probe lines below were in the tree
for that run only (they are not committed).

| function | asm branch | what the branch selects | verdict |
|---|---|---|---|
| `fn_30_68F0` | 0x00006918 `beq .L_00006920` | the data byte's 0x80 clear: form 2 (corporeal) | (b) ran: `[ibbg] PROBE create 012b form 2 startAsIngSpot 0` |
| `fn_30_68F0` | 0x0000691C `li r3, 0x3` | the data byte's 0x80 set: form 3 (ing spot) | (c) unreachable in the shipped data: the loader's `fn_30_10C24` never sets 0x80 (0x00010D9C `rlwimi 7,24,24` from r11 = 0), so the flag byte's 0x80 is always 0 |
| `fn_30_68F0` | 0x0000692C `beq .L_0000695C` | the same bit, read a second time | (b) ran (the same arm as 0x00006918) |
| `fn_30_68F0` | 0x00006938 `bl SetLocomotionType` (0x00006934 `li r4, 0`) | crouch, with the surface alignment | (c) unreachable in the shipped data (the same 0x80) |
| `fn_30_68F0` | 0x00006954 `bl fn_801F7714` | `AlignNearPosition` (rodata_64 = 1.0) | (c) unreachable in the shipped data (the same 0x80) |
| `fn_30_68F0` | 0x00006958 `b .L_00006968` | the join after the alignment | (b) ran (the crouch arm is the only one that reaches it) |
| `fn_30_68F0` | 0x00006964 `bl SetLocomotionType` (0x00006960 `li r4, 1`) | relaxed | (b) ran: the probe line above shows form 2, so this is the arm that ran |
| `fn_30_6014` | none | the body is branch-free: `SetInUse`, then the id into 0x108E and 0x1090 | (b) ran: `[ibbg] PROBE claim 012b cover hint 0158` (six times in the run); a branch-free body needs only an entry line |
| `fn_30_5260` | 0x000052A8, 0x000052B0, 0x000052C0, 0x00005328, 0x00005390, 0x000053AC | the separation steering's cast, self, team, magnitude, index and loop tests | (c) unreachable: the module's own text never calls `fn_30_5260` and no table names it, so the whole body is dead in retail (the host's `ApplySeparation` is the live one) |
| `fn_30_4028` | none | `lwz r3, 0x254(r3)`, an unreferenced accessor | (c) unreachable: nothing calls it and no table names it |
| `fn_30_110C4` | none | the `.ctors` initialiser: 773 straight-line instructions that copy the 86 FSM records (49 triggers, 28 states, 9 codes) into the module's three tables | (b) ran at module load (the machine's states and triggers fire), but no marker: it is the compiler's static initialiser, and the host's tables are static arrays in `PortCIngBoostBallGuardianFsm.cpp` |

So `fn_30_68F0` (the `kSM_Create` body) and `fn_30_6014` (the cover hint claim) carry a marker
each: every reachable arm of the first ran or is unreachable in the shipped data, and the
second is branch-free and ran. `fn_30_5260` and `fn_30_4028` stay unmarked - retail never
calls them, so the port has no behaviour to reproduce and no host counterpart.

`asmcheck.py` reports one `BIT` finding on `fn_30_68F0` (0x6914: "asm tests xae0_80; C++
never reads it"). It is the same false positive the module already has on the other flag-bit
bodies (`fn_30_A0D8` 0xA174, `fn_30_9F50` 0x9FFC, `fn_30_9DA0` 0x9DFC, `fn_30_9C7C` 0x9CC8,
`fn_30_8520` 0x8700): the host reads the packed flag byte through `mData.mFlags210`, and the
annotator cannot resolve a member of a member, so it does not see the read.


## i. Port plan

- **WP1 (done, `1e753b70e`):** class header and layout, the loader (`PortLoadIngBoostBallGuardian`)
  and the `RELMain`/`RELExit` registration, so the actor loads and stands. Model of the shape:
  `PortCIng.cpp`'s milestone 1 (StandIn log per override, forward to `CPatterned`).
- **WP2 (this session):** the FSM. The probe (`MP2_BG_FSM`) dumps the `CGenericFSM2` machine; the
  three tables (49 triggers, 28 states, 9 codes) are registered under the REL's own names in
  `PortCIngBoostBallGuardianFsm.cpp`; the Ing half's bodies come from the Ing host body (branch
  `decomp/ing`) and the boost half from this module's own bodies (section b.2).
- **WP3:** the boost ball — the rolling state, the crystal collision, the puddle form, the three
  phase structs and their damage thresholds.
- **WP4:** mini portals (`fn_30_D4F8` builds `CIngMiniPortalAttack`), the swarm/exit-host half,
  the second class and the render paths.

### i.1 What the port's files are

| file | what it holds |
|---|---|
| `PortCIngBoostBallGuardian.cpp` | the loader, the constructor, the module's own entry points and the `MP2_BG_FSM` probe |
| `PortCIngBoostBallGuardianFsm.cpp` | the three tables (49 triggers, 28 states, 9 codes) and their registration |
| `PortCIngBoostBallGuardianTriggers.cpp` | the trigger and code-function bodies (the Ing half) |
| `PortCIngBoostBallGuardianStates1.cpp`, `...States2.cpp` | the state bodies (the Ing half) |
| `PortCIngBoostBallGuardianOverrides.cpp` | the base-class overrides the Ing half needs (Think, AcceptScriptMsg, DoUserAnimEvent, ...) |
| `PortCIngBoostBallGuardianBoost.cpp` | the boost half: the six boost triggers and the two boost states, from this module's own bodies |
| `include/MetroidPrime/Enemies/PortBoostGuardianShared.hpp` | the form constants, the property-flag bits, the joint-sphere table and the small shared helpers |

The host layout is its own: the guest's object is 0x11F0 (the non-host build keeps that size, and
the `CHECK_SIZEOF` is the guest's), but the host's members are the Ing's (the Ing half, under the
Ing REL's names) plus this module's boost members, so the host's own size differs. The guest
offsets are in section e; the module's own addresses for the state machine's nodes are in
`BoostGuardian_fsm.tsv`.

## j. Open

- what the second class is (section f) and the boost ball's own states.
- the exit host: this module's own copy of the Ing's exit-host body and the exit-host class (the
  Ing's `CIngExitHostEffect` is not in this branch).
- the mini portal attack: this module's own copy of the Ing's `CIngMiniPortalAttack`.
- the nav-point finder `fn_30_2F9C`, which `BoostModeRetreat`'s Activate needs.
- the "PuddleCollisionActor" named child and the crystal behind `HasCollidedWithCrystal`.
- the module's own copies of the base-class overrides (`fn_30_DC10` PreThink,
  `fn_30_DDA0` AcceptScriptMsg, `fn_30_D2E0` PreRender, `fn_30_D384` Render, `fn_30_CE50` Touch,
  `fn_30_D4F8` DoUserAnimEvent, `fn_30_76F4` Death, `fn_30_CB84` KnockBack): the port uses the Ing
  half's copies, and the module's own ones may carry boost-specific code. Compare their sizes
  before trusting the Ing half's behaviour. `fn_30_D94C` Think is ported (2026-10-20) with
  `fn_30_5D90` timers, `fn_30_55D8` ing-spot think and `fn_30_56E0` touch box; its callees
  `fn_30_6108` (tracker drivers) and `fn_30_65C8` (hit light) are ported, and `fn_30_5CD0`/`fn_30_3AF8` run the Ing half's `UpdateValidTarget`/`UpdateLoopSfx`.
- live fight (2026-10-20): the boost puddle countdown now reaches `BecomeIngSpot` ->
  `BoostAttack`, which spawns the boost ball (`CBoostBallGuardianBall` in `objs`, at the ground
  under the guardian); with the ball's Think still a stand-in the state waits there (section f).
- 0x10AC is the `Collar_SDK` locator (ctor 0x0000FFAC), live here (the Ing's 0xd7a stays 0xff):
  `GetAimPosition` `fn_30_CFC4` takes the collar path. `fn_30_5888` (arm segment test) does not
  restart the 0x440 contact-damage cooldown, unlike the Ing's `fn_29_3E1C`; `fn_30_5C80` picks the arm.

## k. What the loader and the constructor actually pass

`fn_30_EC6C` takes nine arguments, all read (full derivation: `docs/module-analysis/BoostGuardian_ctor.md`):

| # | argument | evidence |
|---|---|---|
| 1 | `TUniqueId` (arrives as a pointer in r4; the body copies the u16 and hands the *copy* to `CPatterned`) | [0x0000EC84], [0x0000ECB0] |
| 2 | `const rstl::string&` name — the loader passes `&editorProperties`, whose first member is the name | [0x0000EC80] |
| 3 | `const CEntityInfo&` | [0x0000EC74] |
| 4 | `const CTransform4f&` (the loader builds it at sp+0xC0, which is why reading the loader naively looks like a model) | [0x0000EC78], [0x000004D4] |
| 5 | `const CModelData&` — **one** model, built at sp+0x2D0 and copied to sp+0x320 | [0x0000EC94], [0x00000488] |
| 6 | `const CActorParameters&` | [0x0000ECCC] |
| 7 | `const CPatternedInfo&` (its +0xEC is the pathfinding index) | [0x0000EC90], [0x0000ED0C] |
| 8 | the 0x320-byte guardian block, by reference | [0x0000EC98] |
| 9 | the 0x110-byte boost block, by reference | [0x0000EC9C] |

The base call is `CPatterned(` `character 0x19`, uid, name, `kFT_Zero`, info, xf, model, pInfo,
`kMT_Ground`, `kCT_One`, `kBT_BiPedal`, actParms `)` - [0x0000ECD4] with the constants at
0x0000ECAC/0x0000ECBC/0x0000ECC4/0x0000ECC8. 0x19 is the one free value in the DOL type list, which
is why `kET_IngBoostBallGuardian = 117` is the tag to use.

**The two blocks** (`docs/module-analysis/BoostGuardian_datablocks.md`, 69 + 31 rows, every field traced):

- guardian, `obj+0x07C0`: the first **0x29C bytes are a verbatim copy of
  `SLdrIngBoostBallGuardianData`** itself (three copiers walk it: `fn_30_10694`, `fn_30_10B90`,
  `fn_30_108D4`), and the last 0x84 are the converted tail - `ingSpotVulnerability` plus three
  `CDamageInfo`s and a flag byte. `SLdrIngBoostBallGuardianData` is 0x29C bytes and
  `SLdrBoostBallGuardianData` 0x2E8, and the loader's two temps sit 0x29C apart.
- boost, `obj+0x0DEC`: 0x110 bytes inline at +0x000..+0x04C and +0x090..+0x10F, with the spline at
  +0x04C..+0x090 converted by `fn_30_CB4`; `boostBallVulnerability` sits just past it and the three
  phase structs are a separate 3-entry vector at **0xF2C** (count) / 0xF30, 0xF68, 0xFA0 (0x38
  stride) - which is what the constructor's 0x11CC/0x11D0 pair points into.

The port does not reproduce those two conversions: its host structs hold the same properties in its
own layout (`CIngBoostBallGuardianData`, `CBoostBallGuardianData` in the header) and the loader fills
them straight from the `SLdr` fields, which is what later slices will read.

