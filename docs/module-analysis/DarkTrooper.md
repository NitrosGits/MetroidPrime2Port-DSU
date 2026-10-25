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

## dtr11

- Marked, asm checked by hand and every branch taken (gcov of the natural `darktrooper_unknown.con` run plus two forced runs, `COV_GDB_SCRIPT=tools/cov/specs/darktrooper_dtr11_calls.py tools/cov/force.sh`, merged with `gcov_report.py`):
  - FireSmallShot fn_12_46B4, 17/17: all arms `forced (tools/cov/specs/darktrooper_dtr11_fss.gdb)`: gate (346/348), jitter, the three CanBeNormalized arms by geometry (gun on the target, forward straight up, forward opposite) and the angle test (366/370/384); LaunchProjectile is skipped by `$skiplp`. Range draw order (z, y, x) and LookAt up vector were already right; asmcheck CALL findings are the weak CTransform4f helpers (4260, 4378), the XCALL FacingPlayer is inlined.
  - PreRender fn_12_4B80, 8/8: 308:3/4 (ragdoll primed) and 308 not-primed `forced (darktrooper_dtr11_rend.gdb)` at a dead trooper's ThinkRagDoll; 319:1/2 (CanBeNormalized false) `forced (darktrooper_dtr11_rend.gdb)` by setting mHeadSegId = mRootSegId, so the head - root vector is zero.
  - PreRenderAllViewports fn_12_2F80, 4/4: primed and not primed, both `forced (darktrooper_dtr11_rend.gdb)`.
  - GetScannableObjectInfo fn_12_549C, 10/10: dead, inactive, transitioning (state != 3), transitioning in state 3, plain, no scan info: all `forced (darktrooper_dtr11_rend.gdb)`. Bit positions: mAlive x420_40, mActive x20_80, mTransitioning xa9c_40 (asmcheck NOPOS/BIT note: the field comment gives no position).
- Not marked: GetSearchPath fn_12_0 (`addi r3, r3, 0x7c0` getter; the retail offset of mPathFindSearch is not checked against the header), LoadDarkTrooper 12C (1161:1/2, the null-model return, not forced), ctor 5A48 (untaken 154:7-8/9, 155:2/3, 161:1/2, 166:0/2, 167: sleeper-animation, no-scan-info and flotsam arms; 178 are exception-cleanup edges), dtor 5520 (180 cleanup edges), DeliverCommand instances (1199:0/2, 1202:1/2). These need their own slice.

## dtr12

- Ragdoll crash (dtr11 render/scan force run): force-script artifact, not a port bug. gdb convenience vars `$al/$si/$dl/...` write real x86 registers; scripts renamed to `$v...` (726587f29). Natural run and a no-breakpoint control never crash.
- fn_12_0 GetSearchPath marked: asm `addi r3,r3,0x7c0; blr`; 0x7c0 = CHECK_SIZEOF(CPatterned) = offset of first CDarkTrooper member mPathFindSearch.
- Not done (still unmarked): LoadDarkTrooper null-model arm, ctor 5A48 sleeper/no-scan/flotsam arms, dtor 5520, DeliverCommand instances 1199/1202.

## dct1 (offline closure of three rows; one run plan)

Base `234139ed2`, branch `decomp/dct1`. **No game launched.** Three of the four open B rows are
closed from the retail asm plus the shipped data; the fourth (the main ctor) has live arms and
keeps a run plan. Retail source: `auto_00_00000000_text.s` / `CDarkTrooper.s` of a matching
build; the loader-struct offsets below come from `build/G2ME01/asm/MetroidPrime/ScriptLoader.s`.

| fn | row | what settled it |
|---|---|---|
| fn_12_B28 | SLdrDarkTrooper ctor | MARKED. Branch-free body 0xB28-0xD38: seven sub-struct ctors at +0x0/+0x3C/+0x280/+0x2F8/+0x480/+0x49C/+0x4BC then 21 stores, each matched one for one by the host ctor (same field, same value). Field identity derived, not assumed: LoadDarkTrooper's frame fixes the struct base at +0x478 (patterned +0x3C and actorInformation +0x280 agree with its r26/r27 bases), `patterned+n` is LoadTypedefPatternedAITypedef's own store offset (0x3FAE47EB leashRadius -> +0x28, 0x8A6AB139 collisionRadius -> +0x1AC, 0x3011B5DF collisionHeight -> +0x1B0, 0xD9355674 stepUpHeight -> +0x1C0, 0x4BC4C4D9 creatureSize -> +0x22C, 0x4476BED8 echo set -> +0x230, 0xCF90D15E health -> +0x48 so +0x4 is hI_KnockBackResistance), and `actorInformation+n` from LoadTypedefActorParameters (lighting at +0, ambient sixth = +0x14; 0x05AD250E visor set at +0x60, so visorFlags +0x64). Values: rodata 0x4C/50/54/58/5C/60/64/68 = 100/2/0.5/1.6/1/0/5/18. dt1's four PC defaults (0x4B0 -1, 0x88 2.0, 0x26C 1, 0x294 CColor, 0x2E4 0xF) are five of those stores. No branch, so no arm. |
| fn_12_5520 | `~CDarkTrooper` | MARKED. The module's only reference to `__dt__12CDarkTrooperFv` is the deleting-destructor vtable slot (`.data` 0x2BC + 0x8; the single `.4byte` in the asm) - no call site, no other address-taken use - so the 0x553C/0x5540 null-this guard and the 0x5630-0x5638 `extsh. r0,r30; ble` non-deleting arm are dead for every lifetime. Body = teardown only: 0x5554 vtable, 0x5558/0x5564/0x5570 the +0xA50/+0xA10/+0x9D0 member dtors, 0x5584-0x5598 the +0x9C4 token + engaged byte, 0x55AC-0x55F4 the +0x9A8 element-destructor loop, 0x5600-0x5620 the +0x904 scan-info token + Free, 0x562C base `fn_12_5660`, Free when r4 > 0. Same shape as the accepted dc4 destructor proofs. Taken lines came from the dtr6/dtr8 kill drivers; the untaken gcov lines are host EH cleanup edges. |
| fn_12_12C | LoadDarkTrooper | MARKED. The only arm dtr11 left untaken is the null-model return (0x4FC-0x504 `lbz r0,0x1C0(r1); cmplwi 0; bne 0x574`, `li r3,0` at 0x56C), and it is dead by the shipped data: LdrToModelData gets kInvalidAssetId as the model, so an empty optional also needs the ancs id to resolve to no resource type. New scanner `tools/scan_darktrooper_loader.py` walks every MLVL of every pak (all areas, all layers): **35 DKTR placements, 35 with an ancs that exists with FourCC ANCS** (0xA2CC5B52 in Metroid1.pak for 33, 0x6735E3CA for the two 05_temple_federationsite L9 ones), so the CAnimRes path at 0x4D8/0x4DC always runs. `__nw`'s null return (0x590-0x594) stays the throwing-new OOM ABI limitation (dc4). Caveat: the arm would be reachable on an install whose paks lack that ANCS (missing-pak reading), which is the port's documented "missing paks read as empty" case, not shipped data. |
| fn_12_5A48 | `CDarkTrooper` ctor | **OPEN, needs a run.** Arms still unexercised are live in the shipped data, so no offline proof can close them: (a) the sleeper test at :149/:155 (`FindBestAnimation(kAS_Locomotion, 0, 7/9).second == initialAnim`) - shipped `initial_anim` values are 1/9/11/12/13 and the PAS best-match index needs the ANCS's PAS database, which is not parsed yet; (b) `scannableInfoWhenAttacking != kInvalidAssetId` (:161) - 33 of 35 placements have a SCAN id, the two federationsite L9 ones have 0xFFFFFFFF; (c) `if (flotsam)` (:166) - 11 of 35 have flotsam = 1. |

### Run plan for fn_12_5A48 (not spent in this slice)

One coverage/force run covers (b) and (c); (a) needs a force or the hanging troopers to be sleepers.

    tools/games_running.sh --wait
    MP2_FAST_LOAD=1 BUILD=build-port-link tools/cov/force.sh <spec> <out> \
      3bfa3eff,05_temple_federationsite --driver 'python3 tools/drivers/con.py {port} @f600 layer 9' \
      --frames 2400 --fast

- Area `3bfa3eff,05_temple_federationsite`, layer 9: u<b>0242 and u<b>0259, both flotsam = 1 **and**
  `scannableInfoWhenAttacking = 0xFFFFFFFF` - one spawn takes the :166 true arm and the :161 false arm.
  Sites: `loc:CDarkTrooper.cpp:161` and `loc:CDarkTrooper.cpp:166` (the ctor is unchanged by dct1's
  markers, which were added after it), plus `ret`-less key checks on `$this->x928_40` and
  `$this->x9c4_...`-free assertions; the spec should assert `SetDrawShadow`/
  `SetHighlightedInDarkVisor(false)` ran and no `mAttackScanInfo` allocation happened.
- Sleeper arm (:149/:155): the shipped "hanging" troopers are the candidates -
  `3bfa3eff,01_temple_hive02` layer 0 (u001300e8 "hanging by b", flotsam 1) and layer 3
  (u0c1301d9 "Hanging Trooper 001", flotsam 1); run the same spec there and read the cov key on the
  `FindBestAnimation` line. If neither takes it, force the dependency output in the spec
  (`gdb` write of the returned anim index at the `FindBestAnimation` return) rather than faking
  `mIsSleeper`, so the real comparison executes.
- Needs a `tools/cov/specs/darktrooper_dct1.py` + driver (a `layer 9` console step is enough; no new
  driver command).

### Not touched

`fn_12_FC` (SetFuncPtrs, marked) and the five H rows are outside this slice. No source probes, no
TARGET_PC changes: every dct1 change is a marker comment (code lines identical to base after
stripping `//`).
