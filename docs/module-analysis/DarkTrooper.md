# DarkTrooper REL (module 12): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CDarkTrooper.cpp` (CDarkTrooper) plus the
`SLdrDarkTrooper.hpp` loader. The port's fixes are `TARGET_PC` blocks in those two files. Retail asm:
`build/G2ME01/DarkTrooper/asm/MetroidPrime/Enemies/CDarkTrooper.s`. Per-function table:
`DarkTrooper_functions.tsv`. In game: Morphology Dark Trooper / Dark Missile Trooper.

Bitfields (verified against the asm): byte 0x928 = 0x80 mAlerted, 0x40 mFiresMissiles,
0x20 mUpdatingAnimation, 0x10 mAvoidDownFrames; 0x9a0 0x80 = LOS tracker has line of sight;
0x9d4 0x80 ragdoll flotsam; attack flags 0xa4c / 0xa8c = 0x80 mFinished, 0x40 mBreakAttack;
0xa94 0x80 mShouldPause; 0xa9c = 0x80 mIsSleeper, 0x40 mTransitioning. Ragdoll byte 0x70:
0x40 primed, 0x20 continue small movements, 0x04 no over-timer.

## Behaviour fixes (dt1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_12_B28 SLdrDarkTrooper ctor | 0xB74 -1 -> +0x4b0 (ragdollImpactSound); 0xBB8 `stfs` rodata_50 (2.0) -> +0x88 (knockback resistance); 0xBF4 `stb 1,0x26c` (echo emitter); 0xBF8/0xC0C CColor(1 x4) -> +0x294 ambient; 0xC10 `li r0,0xf` / 0xC1C +0x2e4 visor flags | sound 0, none of the others |
| fn_12_3F8C FireMissile, fn_12_46B4 FireSmallShot | Range(-0.3,0.3) x3 at 0x403C/0x4058/0x4074 (0x4764/0x4780/0x479C) go to z, y, x; LookAt's up = gun xf Z column (0x8c4/0x8d4/0x8e4, 0x414C-0x418C / 0x4868-0x48A8) | unspecified draw order; `CVector3f::Up()` |
| fn_12_23B4 UnmarkPathRegion | 0x23E4 GetPathArea / `beq` epilogue: no reset when the area is null | reset to -1 regardless |
| fn_12_17C0 CanMissileAttack, fn_12_1800 CanSmallShotAttack | 0x17D0-0x17DC / 0x1814-0x1820 `fcmpo cooldown,0; ble`: `!(cooldown > 0)` (NaN passes) | `cooldown <= 0` |
| fn_12_19C0 MissileAttack, fn_12_1D68 SmallShotAttack | 0x1B04 / 0x1EAC End*Attack then `b` epilogue (0x1C40 / 0x1FE4): the finishing frame skips RotateToPoint/DeliverCommand | fell through to both |

Retail's FireMissile also ends with EndSmallShotAttack (0x4234); upstream matches, kept. The retail
ctor does not store 3 at SLdr+0x38 (unlike Shrieker).

## Survey (dt1)

Objdiff of every function below 100% plus the retail asm. Apart from the fixes above the
differences are regalloc, scheduling, stack layout and REL-local copies (the unnamed fn_12_* rows).
Checked and equivalent: ctor (layout, bitfields, material filter), GetScannableObjectInfo,
GetDamageVulnerability, SetPathArea/GetPathArea, SetAttackState + history helpers fn_12_29B0/2A1C,
FacingPlayer, ShouldTrack, GetContactDamage, GetIngSnatchingNormal/Point, AcceptScriptMsg ('XXDG'
ReflectedDamage sets 0x420 0x80), KnockBack, PreRender, GetAimPosition (`dt > 0` ble), DoUserAnimEvent
(EndAction/Activate return without the base call; upstream's `handled` is equivalent), Pursue
(`> 16` ble), Sleep (locomotion == 1 -> random 7/9, `< 0.5` bge), Touch (`force > 0` ble), Dead,
CreateRagDoll (14 radii, flags 3/0, sound 0x9dc), MarkPathRegion, PreRenderBoneTracking,
ShouldPause, LoadDarkTrooper. Not re-read: Think (99.2), ThinkRagDoll (95), ~CDarkTrooper (62).

## Coverage (dt1)

Driver `tools/drivers/darktrooper_unknown.con`, spec `tools/cov/specs/darktrooper.py`, Temple
Grounds 04_temple_unknownenemies layers 2 3 (u049d u0488 u0487), 5200 frames `--fast`. 34 bodies
marked (every line and arm ran, or no branches). SmallShotAttack and MissileAttack are marked with one
dead arm: msg outside Activate/Update/Deactivate (EStateMsg has three enumerators,
`include/MetroidPrime/StateMachineCommon.hpp:4`). Not marked (dtr5 marked CanSmallShotAttack fn_12_1800, InRange fn_12_200C, ApplyMeleeDamage fn_12_3AFC; ALL-ARMS on natural + missile driver + `darktrooper_missile_force.py`, whose forced mFiresMissiles=1 run covers the `:1021` short-circuit; asm read against `asmcheck.py annotate`, its BIT/NOPOS finding on x928_40 is the missing bit-position table, same as the accepted fn_12_17C0):

| function | missing arm |
|---|---|
| Think, Sleep, Pursue, Dead, CreateRagDoll, ThinkRagDoll, Touch, KnockBack, PreRender, DoUserAnimEvent, AcceptScriptMsg, GetAimPosition, FireSmallShot | 1-10 arms each (gcov_report rows) |
| UnmarkPathRegion, MarkPathRegion, GetContactDamage, GetRagDoll, Null, ctor, dtor | not or partly reached |

Missile family (dtr4, FireMissile / CanMissileAttack / ShouldPause marked, ALL-ARMS on merged gcov): driver
`tools/drivers/darktrooper_missile.con` (`--frames 4300`; places the player in/out of range, clear/blocked line, facing/not, morphed
(`press X`)), specs `darktrooper_missile_force.py` (mFiresMissiles=1, forced), `_los.py` (LOS clear, forced), `_deg408.py` /
`_deg412.py` (forced: zero `toTarget` / `forward` XY locals at :408 / :412; dead in practice, the jittered target and gun column
never give exactly zero), plus a natural run with mFiresMissiles=0 (gate-false arm). Arms 408/412 are forced only.

Still open after dtr5 (flag-gated, none reachable by console in the shipped areas; each needs a gdb force at entry): AcceptScriptMsg 202:2/8,5/8 217 227 233 247:3/4 252; KnockBack 280 285 (flotsam/ragdoll); GetAimPosition 329 (lockOn 0xff); FireSmallShot 342 362 366 380; DoUserAnimEvent 459 473 474 502; Think 569 579 592; Sleep 624 627; Touch 687 688; Dead 734 740; Pursue 810 817 825.

Next (superseded for missiles): a Dark Missile Trooper area (01_temple_hive02 / 01_temple_hive_04) for the missile arms;
forced arms with `tools/cov/force.sh` for the rest.

## dtr6 (marks and branch tables)

Base 56e185886. Gcov line numbers are source lines of the pre-mark file. Drivers: `tools/drivers/darktrooper_{hive01,hive02,flotsam,msgs,smallshot,pursue}.con`;
placements: `tools/scan_darktrooper_props.py` (firesMissiles=1 only hive01 L6 e00060251; flotsam=1 hive_04 L0, hive_hall L0/L2, hive02 L0/L2/L3, federationsite L9; avoidDownFrames=1 hive02 L2/L3).

| Function | Offset | Result | Notes |
|---|---|---|---|
| Think | 364C | MARKED | asm read (4081-4229); all arms natural |
| Pursue | 24BC | MARKED | asm read: constants 3/6/9/1.5/16/0.87266463 and flags match. Natural: Activate, Update (turn x3, x1.5, none; InRange 3 close arm; PathShagged arm), Deactivate. IsOver arm: **forced (tools/cov/specs/darktrooper_pursue_force.py)**, `mPathFindSearch.mCurWaypoint = 100000` after 50 hits. Remaining gcov branches are a call edge after SetTurnSpeed and the msg-not-in-3-enumerators arm (EStateMsg has three enumerators: dead). Edge: asm `IsOver` is a signed compare (empty waypoint list = over), upstream header's is unsigned; not changed (shared header). |
| Sleep | 31BC | unmarked | not asm-read this slice; only call edges at 627 untaken |
| Dead | 2C78 | unmarked | not asm-read; 734/740 are call edges |
| Touch | 3000 | marked (dtr7) | forced run, see dtr7 section |
| AcceptScriptMsg | fn_12_5024 | marked (dtr8) | all arms evidenced, see dtr8 |
| KnockBack | fn_12_4CE4 | marked (dtr8) | all arms evidenced, see dtr8 |
| GetAimPosition | - | unmarked | 329 `mLockOnTarget != 0xff` false arm: set from `lockon_target_LCTR` at construction (CPatterned.cpp:194), probably dead by data, unproven |
| FireSmallShot | - | unmarked | 342:1/2, 362:1/2, 366:1/4 degenerate geometry arms untaken (forces not written) |
| DoUserAnimEvent | - | unmarked | 459, 480-481, 488, 502 untaken; needs the hive01 missile trooper kill (`damage u058a 999 0 missile` leaves hp 596) |

## Smoke family (proposal)

`darktrooper`: `3bfa3eff,04_temple_unknownenemies`, 2400 frames, cmd: `@f300 layer 2; layer 3`,
`@f340 send u049d Alert`, `@f600 objs DarkTrooper`, `@f1500 damage u049d 999`, `@f2000 objs DarkTrooper`.

## dtr7

- Touch (fn_12_3000) marked: asm checked line by line (stwu/mflr prologue, ragdoll get + IsPrimed, TCastToPtr<CScriptTrigger>, GetActive, flags & 0x8000, magnitude `ble`, TorsoImpulse += force field). Arms: natural run (inactive triggers, no ragdoll) plus `tools/cov/specs/darktrooper_touch_force.py` with `tools/drivers/darktrooper_touch.con` (`touchact` calls Touch with a given trigger; the spec writes flags / force magnitude by trigger uid at Touch entry). Line 691 and 692 both arms taken.
- Not marked (arms not yet evidenced): AcceptScriptMsg 248, KnockBack 281, FireSmallShot 343/363/367, DoUserAnimEvent 460/481/482/503, Sleep 630, Dead 738/744.
- `tools/cov/force.py` now accepts a callable in a spec's sets (conditional writes).

## dtr8

- AcceptScriptMsg (fn_12_5024) and KnockBack (fn_12_4CE4) marked: asm checked line by line (switch on the message id, 'XDMG' 0x5350 / 'XXDG' 0x533C cases, rodata 0.5 / 0 / -1 / 1.5 / 0.2 / 20 / -1000, flotsam bits at 0x9d4, ragdoll 0x9d0, impulse 0x6ac / ragdoll+0xb8). No C++/asm difference.
- Evidence: natural runs (`darktrooper_msgs.con`, `darktrooper_flotsam.con`, `darktrooper_unknown.con`) plus `tools/cov/specs/darktrooper_msg_force.py` (conditional writes by message / call count: mFiresMissiles on the first Activate and Alert, mAlive+mTransitioning on the first Damage (the early return) then mAlive only, mFlotsam=0 with mIsSleeper 1 / 0 on the first two AreaLoaded, the ragdoll pointer nulled on the second Decrement and on KnockBack 3 / 4 with HP -1 / 10, restored at the next Think). Run it with the msgs driver (AcceptScriptMsg) and the flotsam driver (KnockBack); every line-205..248 and 281..287 arm is taken in the merged gcov.
- FireSmallShot (fn_12_46B4) stays unmarked: it is never called in any driver (`mSmallShotAttack.mProjectile` is null, 0/25 lines), so every arm needs the projectile built first.

## dtr10

- ThinkRagDoll (fn_12_2D98) marked: asm checked by hand (x928 0x10 AvoidDownFrames, 0x20 UpdatingAnimation, x420 0x10 FadeToDeath, order matches). Branch table: natural run plus `forced (tools/cov/specs/darktrooper_dtr10_ragdoll.gdb)`: mFluidIds holding the trooper's own id (non-water, null-cast arm), invalid and valid fluid id at 718, ragdoll nulled, IsOver / WillContinue / AvoidDown / FadeToDeath / UpdatingAnimation combinations. 18/19 lines, 15/18 branches.
- Dead-arm proof (water): 719:1-3 (CScriptWater non-null) need a WATR object in a DarkTrooper area. A scan of shipped data finds none (hive_hall has 5 troopers and no water; pathofeyes has water and no troopers; `tools/drivers/darktrooper_water_probe.con`). Dead by data.
- MarkPathRegion (fn_12_21A0) marked: 10/10 branches with `forced (tools/cov/specs/darktrooper_dtr10_mark.gdb)`: waypoints built from real region centroids (region 97 marked, already-flagged arm, then mCount = 0 empty arm). asmcheck clean.
- Not marked: FireSmallShot 46B4, PreRender 4B80, GetScannableObjectInfo 549C, PreRenderAllViewports 2F80 (arms still need forces; FireSmallShot untaken 346, 348, 366, 370, 384).
