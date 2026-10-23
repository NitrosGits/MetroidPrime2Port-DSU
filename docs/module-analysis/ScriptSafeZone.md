# ScriptSafeZone REL (module 66): analysis notes

The module's code is upstream's `src/MetroidPrime/ScriptObjects/CScriptSafeZone.cpp` (the light
safe zone, SAFE) and `CScriptSafeZoneCrystal.cpp` (its crystal, SFZC). The loaders are the
`SLdrSafeZone*.hpp` headers. The port's fixes are `TARGET_PC` blocks in those files. Retail asm:
`build/G2ME01/ScriptSafeZone/asm/MetroidPrime/ScriptObjects/*.s`. The DOL's CSafeZoneManager is
not part of this module.

## Behaviour fixes (sz1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_66_CC LoadSafeZone | `CScriptSafeZone` ctor arguments: flash brightness in f8 (`lfs f8,0xA2C`), flash time in the stack slot (0x9F0/0x9F4) | the two swapped (the header names of members 0x240/0x244 are swapped against their use, so the fix swaps the call arguments instead) |
| fn_66_14DC SLdrSafeZoneAttributes ctor | five sounds -1; colours (0.094118, 0.5, 0.5, 1); grey 0.75 | 0 / black |
| fn_66_1008 SLdrSafeZone ctor | flash sound -1; editor property 0x5d298a43 = 3; colour alpha 0.25; fogLeaving green 0.1 | 0 |
| fn_66_6478 SLdrSafeZoneCrystal ctor | editor property 3; ambient white; visor flags 0xf | 0 / black |
| fn_66_3A40 Think, activation clamp | 0x3B80 `fcmpo 1.0,new; bge` takes `new` only when 1 >= new, so a NaN keeps 1.0 | upstream `min_val(1.f, new)` already matches; sz1's swap was reverted in review |

## Survey (sz1)

Objdiff of every function under 100%. Apart from the fixes above, the differences are regalloc,
scheduling, stack slots and rodata label names. These were checked and are equivalent:
PreRenderAllViewports, ApplyRenderEffect (both clamp blocks), RayCylinderIntersection,
HandleProjectile, RenderDarkVisorSpot, ModifyObstruction, UpdatePlayerInside, the CDarkWorldInfo
ctor, LoadSafeZoneCrystal (no swapped property offsets), and the crystal `__sinit`. The `__sinit`
has the same override weapons 0, 3, 2, 1 and the same Dark match. `upstream_hollow.py` flags
nothing. fn_66_2CB8 is an 8-byte unpaired REL-local.

asmcheck and retail_diff findings on the marked bodies are tool limits: bitfields with no
stated bit position, callees with no marker yet, inlined CScriptTrigger base bodies
(InhabitantExited/Idle, UpdateEchoEmitter), and RemoveEmitter / SetLoader_SafeZone reached
through inlines. Every marked body is 100% in objdiff except PreRenderAllViewports (93%,
equivalent as above).

Per-function table: `ScriptSafeZone_functions.tsv`. B = upstream body, H = weak copy, template
or unnamed REL-local, T = REL runtime.

## Spawns

| area | ids (this save) | use |
|---|---|---|
| `42b935e4,02_sand_sandfalls_dark` area 25 | zone u0474 + crystal u0473 (active, (-82.3,-75,5.1)); u04d4+u04d3, u04be+u04bf, u04b0+u04b1, u044e+u044d, u043a+u0439, u0435+u0434 (mostly inactive) | player in/out, beams on crystals, messages |
| same, area 14 | u05b6, u058b, u057d + crystal u057c | unused |
| `3bfa3eff,02_temple_exploration_dark` area 46 | zone u0451 + crystal u0452 (inactive, (237.9,-194.7,-39.9)); Medium Ings u0454, u0417 | AI inside a zone (InhabitantIdle -> DamageActor) |

Other SFZC areas: Sand 09_sand_testtubepuzzle_dark, 08_sand_casualpirates_dark,
01_sand_entrance_dark; Temple 01_temple_landingsite_dark, 11_temple_grandpuzzle_dark; Swamp
01_swamp_entrance_dark.

## Coverage

Build `-B build-port-cov -DMP_PORT_COVERAGE=ON`. Merge the runs with
`python3 tools/cov/gcov_report.py build-port-cov <gcov dirs...> --module ScriptSafeZone`.

| driver | run | covers |
|---|---|---|
| `safezone_sand.con` | `run.sh specs/safezone.py`, sandfalls_dark, 2400 frames | enter/leave, each beam on the crystals, zone Increment/Decrement/Internal0-3/(De)Activate, crystal Internal0-6, damage |
| `safezone_sand2.con` | same area, 2000 frames | dark visor spots, scans, long-range shots (impact effect), power on expanded crystals |
| `safezone_temple.con` | exploration_dark, 2200 frames | Ings in the zone (normal and hurtful damage), dark/annihilator shots |

Merged: 74 bodies, 50 with branches, 14 with every arm taken. Marked: 35 (22 SAFE, 13 SFZC). No
forced arms. Covered but not marked (untaken arms): Think (zone and crystal state timer),
AcceptScriptMsg (both), SetZoneType, BuildLight, SetLowPassFilter, UpdateSafeZoneManager,
UpdateLoopSound, ApplyFog (fog off in these areas), InhabitantAdded, SpawnImpactEffect,
HandleProjectile, UpdatePlayerInside, AddToRenderer, RenderDarkVisorSpot, ApplyRenderEffect,
Render, ModifyObstruction, LoadSafeZone, the crystal ctor, Touch, SetState, UpdateModel,
UpdateStateTimer, UpdateRefreshEffect, UpdateEffectsVisibility, UpdateEffect,
GetScannableObjectInfo, LoadSafeZoneCrystal. Never reached: RayCylinderIntersection (no
cylinder zones in these areas), SpawnRefreshEffect.
