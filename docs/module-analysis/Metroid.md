# Metroid REL (module 40): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CMetroid.cpp` (plus `CBabyMetroid.cpp`);
the port adds bodies upstream lacks in that file's `TARGET_PC` blocks. Retail asm:
`build/G2ME01/Metroid/asm/MetroidPrime/Enemies/CMetroid.s` (after `python3 configure.py && $T/ninja`).

## Verified facts

| fact | value | evidence |
|---|---|---|
| CPatterned +0x3fc | `mXDamageThreshold` (protected) | CPatterned ctor 0x80079F98 stores it from CPatternedInfo +0xbc; fn_40_4300 0x43AC reads the pirate's |
| CMetroid vtable 0x40 | slot 16 `GetDamageVulnerability()` | fn_40_3FC0 0x4014 |
| CMetroid vtable 0xd4 | slot 53 `MassiveDeath` (CPatterned's; CMetroid has no override) | fn_40_3FC0 0x40C8 |
| CScriptSafeZone +0x3f0 | zone type (0 = kZT_Normal) | fn_40_3FC0 0x40B0 |
| pirate attach | `CSpacePirate::AttachActorToPirate` -> fn_72_E0D8 (`PortAttachActor`) | fn_40_4300 0x4380 |
| rodata_84 / rodata_A8 | 1.0f / 0.0f | fn_40_4300 0x43A4 / 0x43C0 |

## Spawns that work

| area | layers | ids (this save) | use |
|---|---|---|---|
| Agon 08_sand_casualpirates (`42b935e4,08_sand_casualpirates`) | 6, 7 after the warp (@f300+) | metroids u0547 u053d u0539 u0538 u0535 (`send <id> Activate`); u0539 placed at 366 -279 3 latches pirate u04d8 | latch, shots, bombs |
| Agon 09_sand_testtubepuzzle_dark | 4 | metroids u05f8 u05ec u05e6; safe zones u054a u0449 u04e0 (Normal) | safe-zone arm |

## Coverage (metroid1, 2026-10-23, spec `tools/cov/specs/metroid_growth.py`)

fn_40_3FC0 `ApplyDamageGrowth` (marked):

| arm | how |
|---|---|
| sender gone -> return | `hurt u0547 1` (no damager) |
| projectile that hurts -> GetVulnerableDamage, ApplyGrowth | dark/light/annihilator `shoot` (`metroid_casualpirates.con`) |
| projectile that doesn't hurt -> bomb/zone casts | power beam on a sucking metroid |
| CBomb -> GetVulnerableDamage, ApplyGrowth | real morph-ball bomb under u0547 (`metroid_bomb.con`) |
| safe zone Normal -> nothing | forced (`send u05f8 Damage from u054a`) |
| safe zone not Normal -> MassiveDeath | forced (`send u054a Increment; send u05f8 Damage from u054a`): MassiveDeath 1 hit, u05f8 deleted |

fn_40_4300 `PreDamageSpacePirate` (not marked): the true path ran (AttachToTarget ret=1,
pirate threshold 1000000 -> ApplyDamage 999999, hp 60 -> -999939). Its four early false returns
(no target, target not a pirate, IsPirateValidTarget false, AttachActorToPirate false) were not
reached, also with a second metroid forced (`fsm u053d Attack`, `metroid_latch2.con`): it latched
another pirate. (up2, 2026-10-23: four natural runs, `damage`/`send Delete` on the target pirates at f700-f1200,
still none of the first three (met1's `metroid_forced2.con` did take the IsPirateValidTarget-false
arm, gcov 619:0/2; no target and not-a-pirate stay untaken); the fourth is dead: IsPirateValidTarget already requires
`GetAttachedActor()==kInvalidUniqueId` and PortAttachActor only fails when it is set.)

fn_40_2EC8 `ShouldReleaseFromTarget`: the pirate branch now checks AllEnergyDrained (0x8f8 bit 0x02,
0x2FD4-0x2FE0) before IsDead, as retail does; the check ran in every up2 run, never true.

## Survey (met1, 2026-10-23)

Upstream CMetroid/CBabyMetroid are 96-100% fuzzy against retail. Objdiff of every function under 100%
(FindNearestTarget, ComputeSuckPiratePosRot, SuckEnergyFromTarget, ComputeSuckPlayerPosRot,
SelectTarget, Generate, InAttackPosition, ctor, sinit, ApplySeparationBehavior; CBabyMetroid Generate,
Dodge, Think) shows only regalloc, scheduling, fmadds and stack offsets. The only behaviour differences
were fn_40_3FC0, fn_40_4300 and fn_40_2EC8, already fixed in TARGET_PC blocks. `tools/upstream_hollow.py
Metroid`: no hollow bodies (with `--min 0`, 12 eight-byte empty getters that match retail). Not
deep-checked: the SLdrMetroidAlpha ctor (56%); the loader overwrites its defaults.

Per-function table: `Metroid_functions.tsv` (B upstream body, H weak copy, T REL runtime).

## Coverage (met1)

Build `-B build-port-cov -DMP_PORT_COVERAGE=ON`; each run:
`BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/metroid_growth.py <out> 42b935e4,08_sand_casualpirates --con <con> --fast --frames N`,
then `python3 tools/cov/gcov_report.py build-port-cov <gcov dirs...> --module Metroid`.
Drivers: `metroid_casualpirates.con`, `metroid_bomb.con`, `metroid_testtube_dark.con` (testtube area),
`metroid_player.con` (latch on the god-mode player, morph/bombs/power bomb), `metroid_forced.con`
(WallHang, Dodge, missile/charged shots, TurnAround, TelegraphAttack), `metroid_forced2.con`
(AreaLoaded twice, double latch, dark charged shot on a latched metroid, latch on the morphed player
then kill). Latch on the player: `place uX player 2; fsm uX SelectTarget`, then `fsm uX Attack`.

Marked by met1 (all arms or branch-free, asmcheck clean apart from inlined helpers): ctor, dtor, Think,
AcceptScriptMsg, Render, GetDamageVulnerability(3-arg), Death, UpdateAILogicTimers, Patrol, PathFind,
GetDamageMultiplier, SelectNewTarget, DisableSolidCollision, InterpolateToPosRot,
ComputeSuckTargetPosRot, GetCollisionPrimitive, StateOver, ShotAt, SpotPlayer, ShouldWallHang,
ShouldDodge, SetupStateMachine, FaceHugVulnerability.

## Coverage (met2, 2026-10-23): the arms met1 left

Build and report as in met1 ("Coverage (met1)"); the merged profile is runs `met2_a` (arms +
`metroid_arms.con`), `met2_b` (`metroid_arms2.con`), `met2_c` (testtube, `metroid_patrol.con`),
`met2_d` (`metroid_target.con`), `met2_e` (`metroid_attach.con`), and the forced ones
(`tools/cov/force.sh`, `COV_GDB_SCRIPT=tools/cov/cov_force.py`, spec `tools/cov/specs/metroid_force*.py`,
drivers `metroid_force.con`, `metroid_force2.con`, `metroid_suck.con`). Every row below ends
`ALL-ARMS` in that profile.

| function | arms (forced in bold where the fight cannot take it) |
|---|---|
| `fn_40_4E44` WallHang | msg switch default (`statecall u0547 WallHang 3`), then Zero -> One -> Two; the `mState` switch default is `mState == kAiState_Over`, which the state body never sets itself: `fsm u0547 Dodge` writes Over, then `statecall u0547 WallHang 1` (**forced (statecall)**) |
| `fn_40_3B18` DetachFromTarget | player arm (unmorphed, `statecall u0547 Attack 2`), **morphed arm (`press x` then `statecall u0547 Attack 2`: the quaternion/`GetAimPosition` branch)**, and the **pirate arm**: `place u0539 366 -279 3; fsm u0539 Attack` latches u04d8, `statecall u0539 Attack 2` detaches it (`pirate->GetAttachedActor() == GetUniqueId()`) |
| `fn_40_3DCC` AttachToTarget | true arm (latch on the player) and **false arm (`statecall u0547 Attack 0` while the player already holds u0547: `AttachActorToPlayer` refuses**, `metroid_attach.con`) |
| `fn_40_3E88` SwarmRemove | true path twice over (team member deleted; `kill`/`send Deactivate`), then **the three early-outs forced**: `mTeamAiManagerId` = kInvalidUniqueId, = an id no object has, = u0554 (the area's other team, which does not hold the metroid) — `tools/cov/specs/metroid_force.py` |
| `fn_40_4300` PreDamageSpacePirate | true path (met1), IsPirateValidTarget false (u053d's Attack on the pirate u0539 holds), **no target and not-a-pirate forced** (`mAttackTarget` = kInvalidUniqueId, = 0x0fff; `metroid_force.py`). The fourth early return (`AttachActorToPirate` false) is **dead**: fn_72_E0D8 returns false only when the pirate's 0xa84 is not kInvalidUniqueId (0xE0DC-0xE0E8, 0xE118), and IsPirateValidTarget has just required 0xa84 == kInvalidUniqueId (fn_40_471C-0x4728, 0x4360-0x4368), with no write in between |
| `fn_40_44EC` GetGrowthStage | 1.f/2.f arms naturally; **the 3.f arm forced**: every MTDA placement on the disc has 1 hp, so the growth energy never reaches the explosion threshold — `mGrowthEnergy` written at `GetDamageMultiplier`'s entry while a metroid drains the player (`metroid_force5.py`, driver `metroid_suck.con`; the drain inlines GetGrowthStage, so the body is reached through the multiplier) |
| `fn_40_47AC` SetPatrolDest | false arm (no patrol path, `codecall u0547 SetPatrolDest` in 08_sand_casualpirates) and **true arm in 09_sand_testtubepuzzle_dark L4 (`codecall u05f8 SetPatrolDest`; its MTDA placements have a kSS_Patrol/kSM_Follow connection, the casual pirates' have none — a full disc scan of every MREA: 26 MTDA placements, 18 with that connection)** |
| `fn_40_5AC0` SelectTarget | msg default and `mState` default (`statecall u0547 SelectTarget 3` / `1` with Over), One -> Two with a target (`place u0547 player 2; fsm u0547 SelectTarget`), **Two with no target forced (`mAttackTarget` = kInvalidUniqueId while `mState` == Two, `metroid_force2.py`) and Two with a target id that is no object (`0x0fff`, `metroid_force3.py`)** |
| `fn_40_1698` GetOrigin | the team manager's PositionTeam asks for it (mode 0); player target unmorphed and morphed, the **zero-direction arm (`place u0547 player 0`, exactly on the player, so `(this - player).ToVec2f()` cannot be normalized**, `metroid_arms2.con`), the pirate target arm, and **the same zero direction on the pirate arm: player teleported away, `place u04d8` and `place u0547` on the same point (`metroid_force.con`)** |

Still not marked:

| function | untaken |
|---|---|
| GetDamageVulnerability() | 170:3/4, 173:0/2 |
| DoUserAnimEvent | 198:0/2 |

Never ran: Leash, CanBeIngPossessed, all of CBabyMetroid (no spawn known).

Marked by met2: SwarmRemove, WallHang, GetGrowthStage, AttachToTarget, PreDamageSpacePirate,
DetachFromTarget, SetPatrolDest, SelectTarget, GetOrigin (Metroid 33/167 marked).
