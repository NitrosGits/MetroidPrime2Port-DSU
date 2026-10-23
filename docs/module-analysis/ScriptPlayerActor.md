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
