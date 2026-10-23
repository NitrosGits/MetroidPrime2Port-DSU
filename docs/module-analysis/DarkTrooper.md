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
`include/MetroidPrime/StateMachineCommon.hpp:4`). Not marked:

| function | missing arm |
|---|---|
| FireMissile | dtr3 forced run (`tools/cov/specs/darktrooper_missile_force.py`: mFiresMissiles=1, missile optional m_valid=1, ProjectileInfo redirected to the small shot; forced, not natural): 25/26 lines, launch arm ran 191x; untaken 388:1/2 (gate false), 390, 408, 412, 427 arms (LOS/range/facing) — not marked |
| CanMissileAttack, ShouldPause | forced mFiresMissiles=1: CanMissileAttack 5/6 (untaken 1029:1/4), ShouldPause 1/4 (InRange / mShouldPause arms unseen) — not marked. InMissileRange 2/2, no branches, asm-verified: marked |
| CanSmallShotAttack, InRange, ApplyMeleeDamage | one short-circuit / out-of-range arm |
| Think, Sleep, Pursue, Dead, CreateRagDoll, ThinkRagDoll, Touch, KnockBack, PreRender, DoUserAnimEvent, AcceptScriptMsg, GetAimPosition, FireSmallShot | 1-10 arms each (gcov_report rows) |
| UnmarkPathRegion, MarkPathRegion, GetContactDamage, GetRagDoll, Null, ctor, dtor | not or partly reached |

Next: a Dark Missile Trooper area (01_temple_hive02 / 01_temple_hive_04) for the missile arms;
forced arms with `tools/cov/force.sh` for the rest.

## Smoke family (proposal)

`darktrooper`: `3bfa3eff,04_temple_unknownenemies`, 2400 frames, cmd: `@f300 layer 2; layer 3`,
`@f340 send u049d Alert`, `@f600 objs DarkTrooper`, `@f1500 damage u049d 999`, `@f2000 objs DarkTrooper`.
