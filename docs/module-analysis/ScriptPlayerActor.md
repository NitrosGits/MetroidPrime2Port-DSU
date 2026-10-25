# ScriptPlayerActor REL (module 61): audit of the upstream body

Audit by lane spa1 (2026-10-23). The body is upstream's `src/MetroidPrime/ScriptObjects/CScriptPlayerActor.cpp`
(`CScriptPlayerActor`, script type `PLAC`); the port builds it unchanged except for one `TARGET_PC`
fix. Sources: the retail asm `build/G2ME01/ScriptPlayerActor/asm/MetroidPrime/ScriptObjects/CScriptPlayerActor.s`,
objdiff of upstream's matching build against it, the shipped PLAC data (all worlds, 310 placements).
Per-function table: `ScriptPlayerActor_functions.tsv`. All `fn_61_X` are REL `.text` offsets.

## 1. Fix

| fn | asm | upstream | port |
|---|---|---|---|
| `Think` fn_61_2630 | 0x27D4 `lwz r4,0xebc(r31)` (mPlayerGun), 0x27DC `lwz r4,0x778(r4)` | `GetPrimaryWeaponId()` (CPlayerGun +0x774, the current beam) | `GetPrimaryDestWeaponId()` (+0x778, mNextBeamId): the actor loads the beam the gun is switching **to** |

## 2. Equivalences (objdiff below 100%, no behaviour difference)

- `~single_ptr<CProjectedShadow>` / the dtor fn_61_2E3C: MWCC instantiates it with the type incomplete;
  the host has it complete. Both free the shadow.
- `GetNextSuitCharIdx`, `GetSuitCharIdx`: branch layout and scheduling only.
- `SetupEnvFx`: `fcmpu` operands swapped (an equality test).
- `LoadSuit`, `LoadPlayerActor`: `TLockedToken` against `TToken` + `GetObj`; the same lock.
- `AcceptScriptMsg`: register allocation and a duplicated base-call tail at 0x2C10.
- `SetIntoStateManager`: stack temporaries. `PreRender`: `3 - (grapple == 0)` is `2 + (grapple != 0)`.
- `SLdrPlayerActor` ctor fn_61_874: its extra stores (+0x38 = 3, ambient `CColor(1,1,1,1)`, visor 0xF)
  repeat the defaults the base property structs already set.
- `LoadPlayerActor`'s beam: retail `beam = (3 < gun-1) ? -1 : gun-1` differs from upstream's switch
  only for a negative render-gun override; the shipped data has 0 (301), 1 (8) and 5 (1). Kept.
- `Think` calls `IsMultiplayer` at 0x2660 and drops the result.
- fn_61_740: a `CToken`-holder dtor with no caller (dead, class T).

## 3. Shipped data (PLAC flags at +0x400)

Flags used: 0, 0x4, 0x5, 0x7, 0xC, 0x44, 0x84, 0x104, 0x204, 0x304, 0x404. Bits: 0x1 suit swap,
0x2 alternate next suit, 0x4 Deactivate resets, 0x8 count the area's loading actors, 0x10 keep in the
player-actor list on Deactivate (never set), 0x20 no shadow (never set), 0x40 no gun render, 0x80 wait
for Increment, 0x100 gun before suit, 0x200 no grapple draw, 0x400 no suit re-check.

## 4. Coverage (build-port-cov, `tools/cov/specs/scriptplayeractor.py`)

Drivers `tools/drivers/spa_*.con` (editor ids without the layer bits: the console resolves `e00LLAAAA`
ids with the layer field cleared): sand_temple_dark (flags 5/7, layer 10), sand_hall (gun 5, layer 2),
pickup_swamp_c (0x404), temple_hive03 (0xC, layer 2), temple_sungenerator (gun 1, layer 8),
temple_hive03_dark (0x104 + grapple beam, layer 2), temple_finalgate (0x44 + grapple, layer 3),
0n_temple_hall (0x84, `send AreaLoaded`), Temple_LandingSite (flags 0). Forcing:
`tools/cov/specs/scriptplayeractor_force.py` with the landing-site driver.

| body | arms | marked |
|---|---|---|
| ctor 2FC8 | all but the closing brace's 2 unwind edges | yes |
| AcceptScriptMsg 29F0 | all; 0x10 arm forced (`send Deactivate`, mFlags 0x10 via force spec); AreaLoaded-while-active via `send AreaLoaded` | yes |
| LoadSuit 1A20, LoadBeam 1914, UsePlayerBeamModel 13A4, Build{Beam,Grapple}ModelData 1FEC/1F58, Render 2490, Get{Gun,Grapple}Transform 25C4/2508, SetActive BF4, SetFuncPtrs 70 | all | yes |
| Think 2630 | model-not-loaded short-circuit arms (219/224/225), one 198 arm | no |
| SetupEnvFx AB8 | rain arm (no rainy placement reached), 0x20 (data-dead) | no |
| GetSuitCharIdx FDC, GetNextSuitCharIdx 103C | count-overflow arms; 0x2 with Light suit | no |
| Pump*/Cancel*/UpdateGrappleModel, Touch*, AddToRenderer, RenderGun/RenderGrapple, PreRender, Setup{On,Off}lineModelData, SetIntoStateManager, LoadPlayerActor | defensive null/inactive/missing-ACS arms | no |

`tools/asmcheck.py check` on the marked bodies: only `NOPOS`/`BIT` (the header states no bitfield
positions; objdiff matched the bit stores) and `CALL` to unmarked callees.

## 5. Smoke family (proposed)

`spa`: Agon Wastes 42b935e4, 03_sand_temple_dark, `tools/drivers/spa_sandtempledark.con`,
~2000 frames fast: Activate/Increment/Deactivate/Reset/Delete on two suit-swap actors.

## 6. Forced arms (spa2, 2026-10-24)

`tools/cov/specs/spa2_force.py` + `tools/drivers/spa2_templedark.con` (the spa1 temple driver with the
two-actor Activate/Deactivate chain SetIntoStateManager needs), merged with the spa1 drivers
(`gcov_report.py build-port-cov /tmp/opencode/spa_{a,b,c,d,e}/gcov --module ScriptPlayerActor`).
Every write is one call, restored at the next entry.

| body | arms | forced | still open |
|---|---|---|---|
| LoadPlayerActor C0 | 27/28 | the unknown-ACS arm (`loc:599`: sldrThis.animationInformation.ancs = 0xffffffff), isSolid (`loc:613`) | 621:0/2: the renderGunOverride switch's default arm; writes of sldrThis.renderGunOverride at 620/621 do not reach the switch's read |
| SetupEnvFx AB8 | 7/17 | mFlags 0x20 (0x553's no-shadow arm) | 548:0/2 (`set` false on a reachable call), the rain block 549/550: writing the world's mNeededEnvFx = kEFX_Rain and mEnvFxManager->mFxDensity = 1 at the body entry leaves 0x549's first test false, so the code reads another copy |
| PreRender C30 | 14/21 | shadow null (0x530 edge 1), beam model data null (the HasGunModelData arm) | 0x530's inlined HasGunModelData sub-arms (b4/b5, b9), 0x535's `HasGrappleModelData() ? 3 : 2` (no run loads a grapple model; UpdateGrappleModel/PumpGrappleModel never run), 534:0/2 |
| Think 2630 | 51/67 | use-player-beam with no beam data (194/195), invalid beam, grapple-loading, the loading blocks | 185:3/4, 198:0/2, 200:3/6 (the cinematic-camera arm), 214:0/2, 221/226/227's || edges, mAreaTrackingLoad |
| SetIntoStateManager 970 | 16/18 | 0x573's `!set && previous` (the two-actor chain in spa2_templedark.con) | 580:1/4 and 580:2/4: `actor`/`actor->mIsPlayerActor` are in registers at 0x580, so `loc:` writes of them (and of `id.value`) do not reach the test |
| GetSuitCharIdx FDC | 3/4 | mCharacterCount 0 (both count arms) | 514:1/2 (the inner count arm): needs renderSuit(suit) >= mCharacterCount with suit < mCharacterCount; `$rdx` writes at entry did not produce it |
| GetNextSuitCharIdx 103C | 4/6 | - | 489:0/2 and 495:1/2: mFlags/mSuit writes at entry do not change the branch (the two suit assignments compile to one select; the branch gcov reports on that line is not the one the C++ reads) |
| dtor 2E3C | - | - | inline in the header; see section 7 |

No retail divergence and no source change in this pass: the bodies still match the asm as the survey
records.

## 7. spa3 (2026-10-25): forced arms, `tools/cov/specs/spa3_force.py` + `tools/drivers/spa3_templedark.con`

Per-call cycling forcers (this/mgr captured at entry; no register restore). Runs: f2 (`42b935e4,03_sand_temple_dark`, `--fast --frames 2200`), f3 (`SPA3_ACS=1`, unknown-ACS), plus natural a..j.

| fn | result | note |
|---|---|---|
| C0 LoadPlayerActor | ALL-ARMS, marked | unknown-ACS arm forced (`SPA3_ACS=1`, ancs=0xffffffff at loc:599) |
| FDC GetSuitCharIdx | ALL-ARMS, marked | (suit,count) pairs; inner arm needs `give gravityBoost` (Dark renders 5, `CPlayerState.cpp:597`) |
| 103C GetNextSuitCharIdx | ALL-ARMS, marked | all (mFlags&2, mSuit) pairs; spa2 missed it with call-number one-shots |
| 970 SetIntoStateManager | 17/18, unmarked | 580:2 (chain actor that is not a player actor) not reached |
| AB8 SetupEnvFx | 15/17, unmarked | 549:4/5 (rain with no model data) not reached |
| C30 PreRender | 18/21, unmarked | 530:4/9, 535:3 not reached; `loc:530` writes land after the test (crash at 535 with a null shadow) |
| 2630 Think | 63/67, unmarked | 200:3, 221:8/10, 227:4 not reached |
| 2E3C dtor | 29 deletes, unmarked | all but the `mBackupModelData` live arm ran; that arm needs a delete within 2 PreRenders of `SetupOfflineModelData` (`.cpp:369`, countdown at 522-526); 368:1 is itself untaken |
