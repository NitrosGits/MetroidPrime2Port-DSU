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
| fn_66_3A40 Think, activation clamp | `fcmpo new,1.0; ble` keeps the new value, so a NaN stays | `min(1, new)` returns 1 on NaN; now `rstl::min_val(new, 1.f)` |

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

## sz1 lane, second pass (base 2c825a4f5, 2026-10-24)

New TARGET_PC fixtures (`CScriptSafeZone::PortConsoleVar`): `var <zone> sz.cyl <1-11>` runs RayCylinderIntersection (static, inlined into HandleProjectile; no shipped zone is a cylinder) on eleven hand-made rays
(answer 1000 * hit + 10 * t), `var <zone> sz.obst 1` runs ModifyObstruction (delta 0) with the area's path area removed for the call.
Specs (all `42b935e4,02_sand_sandfalls_dark`; rebuild and rerun the three natural drivers first, older gcda do not match):
`safezone_force1.py` (force.py `loc:`: valid dark-visor spot texture on every zone, one model of the first four crystals invalid), `safezone_force2.py` + `safezone_force2.con`
(force_ret FORCE_RET on natural calls + a gdb hook for the spot texture, `TCastToPtr<CCinematicCamera>` and the `$sz_c` flag; `COV_ROOT=$PWD COV_GDB_SCRIPT=tools/cov/force_ret.py`).

| function | arms | forced (cmd) |
|---|---|---|
| ApplyRenderEffect 2008 | 34/34 | forced (flash timer / inside alpha +-1e6: the alpha clamps; mMobile 0/1) |
| RenderDarkVisorSpot 2474 | 6/6 | forced (spot texture written at load; the view matrix moved onto the zone: `toSpot` zero) |
| HandleProjectile 2D14 | 16/16 | forced (mShape := 1 cylinder / 7 unknown on natural shots) |
| RayCylinderIntersection 2F58 | 22/22 | forced (`sz.cyl 1-11`, and HandleProjectile's cylinder arm) |
| UpdatePlayerInside 28A0 | 10/10 | forced (mIgnoreCinematicCamera 0/1; `TCastToPtr<CCinematicCamera>` answering its argument for the call) |
| ModifyObstruction 1C3C | 12/12 | forced (`sz.obst 1`: no path area) |
| LoadSafeZoneCrystal 5730 | 18/18 | forced (each of the four crystal models invalid on successive crystals) |

Header / loader rows marked without a run (objdiff 100%, user rule): ~SLdrSafeZoneAttributes 14A0, LoadTypedefSafeZoneAttributes 181C, ~SLdrSafeZoneFog 143C, LoadTypedefSafeZoneFog 16A4, ~SLdrSafeZone F40,
~SLdrSafeZoneCrystal 6408, CDarkWorldInfo copy ctor B00 / dtor D80 / LoadOptionalTexture 5400, and the CDarkWorldInfo ctor 51A8 (98.6%, regalloc). Not marked: the `__sinit` row 8B18 (82%).

## sz2: Think (fn_66_3A40) — NOT marked

Fixture `tools/cov/specs/sz2_think.py` (+ `sz2_think.con`, `sz2_selftest.py`, 17 offline tests): 37 forced cases (state-timer arms), owned by FinishBreakpoint window (frame equality + inferior/thread), snapshot/restore with readbacks, callee mocks. Model written from the retail asm and the source.
Launch 1 failed (absolute-address line breakpoints). Launch 2 ran the whole game, but the entry guard `mEchoEmitter.mPtr != 0 -> skip` excluded all 26955 natural Think calls: **0/37 cases ran**, no forced result. Cap 2/2 consumed.
Natural line hits (26955 calls): only the dark-grow arm (217/221, 6x), the full-shell no-op (222, 13263x), the inactive no-op (250, 13686x) and `UpdateSafeZoneManager` (261, 6x) ran. Never executed: non-dark clear (226), lifetime timer (230), moved block, Echo InformListeners, inactive decay and its zero arm (251-255), emitter off (266), lifetime expiry (273-279), camera-inside change, fog-dirty. Executed binary sha256 2ff8ee13…f12fb, fixture sha256 2dc6c107…fab44e, evidence in `/tmp/opencode/sz2-evidence/`.
A corrected fixture (do not skip natural emitters; mock only) would be offline-only until run. No behaviour change; no `// @` marker.
