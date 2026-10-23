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
another pirate.
