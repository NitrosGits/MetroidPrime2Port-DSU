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

Near-complete, not marked (gcov line:arm untaken):

| function | untaken |
|---|---|
| WallHang | 520:3/4 msg switch default, 528:3/4 mState default (not provably dead) |
| DetachFromTarget | 724:0/2 |
| AttachToTarget | 598:1/2 |
| SetPatrolDest | 746:0/2 |
| GetGrowthStage | 584:1/2 |
| SelectTarget | 830:1/2, 831:1/2 |
| GetOrigin | 910:1/2 |
| SwarmRemove | 348:1/2 |
| GetDamageVulnerability() | 170:3/4, 173:0/2 |
| DoUserAnimEvent | 198:0/2 |
| PreDamageSpacePirate | 612:0/2, 616:0/2 (622:0/2 dead) |

Never ran: Leash, CanBeIngPossessed, all of CBabyMetroid (no spawn known).
