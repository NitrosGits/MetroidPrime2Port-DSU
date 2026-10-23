# MediumIng (module 41)

The Hunter Ing REL. The upstream `src/MetroidPrime/Enemies/CMediumIng.cpp` has the class, the
loader, the ctors and empty stubs; the port's bodies are in
`src/MetroidPrime/Enemies/PortCMediumIng.cpp` (the upstream stubs they replace sit under
`#ifndef TARGET_PC`). Function table: `MediumIng_functions.tsv` (class B/H/T, slice, marker).

## Status (2026-10-23, slice 1 of 2, lane mi1)

- Slice 1 (the non-state bodies) is in: CMediumIng Think, AcceptScriptMsg, PreRender, Render,
  Touch, Death, UpdateCurrentSafeZone, UpdateMistEffect, CreateMistEffect, Listen, SetFlagE1,
  CalculateSeparation and retail's unnamed helpers UpdateTimers 3274, ScanProjectileThreats 4644,
  SetMistCollision 49CC, IsMistDecayed 5DC8, IsBlocked 5E0C, UpdateMistState 6078; the tentacle's
  Think, PreRender, Touch, GetTouchBounds, GetProgress, Extend, Retract, UpdateShake.
- 24 of 156 B functions marked: every slice-1 body that ran in play. Not marked: CalculateSeparation
  2024 (only PathFind calls it, at 0x7374: slice 2). SetFlagE1 1B04 is T: nothing in the REL
  references it.
- Slice 2 (open): the FSM states SubStart..Dead, SelectTargetState, SelectTarget*, Set*Destination,
  FindSafeZoneRetreatPoint, FindClosest*, and the unidentified fn_41_* helpers of the table.
- Without slice 2, `mC1` (0xcfc 0x40, cleared by SubStart) stays set: the Ing is pre-render clipped,
  CanBeShot is false, and the projectile scan and mist arms do not run. The console clears it:
  `var <ing> ming.c1 0`. ArmAttack is what extends a tentacle; the console does its calls:
  `var <tentacle> ming.extend 1` (Extend towards the player, one unit up) and
  `var <tentacle> ming.retract 1` (Retract, then mRetracting as ArmAttack sets it at 0x41CC).

## Where it spawns

Sky Temple Grounds area 46 (`3bfa3eff,02_temple_exploration_dark`): CMediumIng u0417 (208.5,
-185.8, -47.2) and u0454, eight inactive tentacles u0491..u0494, u0498..u049b at (0,0,0). Agon
Wastes `04_sand_mining_dark` (layers 4..6) has none on the default card.

## Run

    tools/cov/run.sh tools/cov/specs/mediuming.py <out> 3bfa3eff,02_temple_exploration_dark \
      --con tools/cov/specs/mediuming.con --fast --frames 1000

1000 updates, exit 0, `[port] caught` / `reach-stub` 0. The driver clears mC1 (the Ing shows), takes
the scan visor, makes noises 6 and 4, shoots it (hp 150 -> 98), places it on the player (Touch),
extends and retracts two tentacles (they end inactive after the shake) and kills it (fsm Dead).
Screenshots: before (mC1 set, nothing drawn) / after (the Ing drawn) / scan visor / tentacles /
death, kept by the lane in `/tmp/opencode/mi1-shots/`.

## Arms (the run above; gdb spec keys)

| fn | function | arms taken (hits) | not taken |
|----|----------|-------------------|-----------|
| 7804 | Think | mC1 0 (1124) / 1 (514); mMistState 0..4 | mC0 1 (no mist attack without slice 2) |
| 3274 | UpdateTimers | mC2 0 (1200) / 1 (562) | - |
| 4644 | ScanProjectileThreats | mMistState 0 (1124) | other mist states skip the scan by design |
| 6078 | UpdateMistState | states 0, 1, 2, 3, 4; mC5 0/1 | - |
| 5DC8 | IsMistDecayed | ret 1 (2572) / 0 (184) | - |
| 49CC | SetMistCollision | solid 0 / 1 | - |
| 5E0C | IsBlocked | ret 0 | ret 1 (nothing in the way at the spot) |
| 1680 | UpdateMistEffect | mC0 0 (1762) | mC0 1 |
| 198C | CreateMistEffect | 2 (one per Ing) | - |
| 155C | UpdateCurrentSafeZone | 2 | - |
| 7478 | Listen | noise 2, 4, 6 with mAlive; ret 0 and 1 (noise 4) | dead Ing |
| 79C8 | AcceptScriptMsg | 8 message ids (XCRT, XALD, ... 400 cap) | - |
| 75A4 | PreRender | mC1 0 (156) / 1 (64), mC3 1 | mC3 0 |
| 74F8 | Render | 156, combat and scan visor | - |
| 761C | Touch | mC5 0 (3647) / 1 (353) | mD2 / mD3 1 (slice-2 attack flags) |
| 1B18 | Death | 2 | - |
| A4 | CanBeShot | ret 1 | - |
| 9F84 | T Think | extending, retracting (UpdateShake ran) | - |
| 94C8 | T PreRender | mExtending 0 (20) / 1 (20) | - |
| 93FC / 94A4 | T GetTouchBounds / GetProgress | 608 / 636 | - |
| 91B0 | T Touch | mCanDamage 1 (400 cap) | mCanDamage 0 (early return) |
| 9A34 / 9CE0 / 9ED0 | T Extend / Retract / UpdateShake | 2 / 2 / 2 | - |
| 2024 | CalculateSeparation | not run | slice 2 (PathFind) |

## Port notes

- Host guards (each commented in the source): no blob effect when the ing-spot blob token is
  invalid; a null path area reads as no region; tentacle PreRender returns on a null anim data or
  fewer than three pose elements; alpha bytes are clamped like psq_st's u8 saturation.
- The tentacle ctor sets mCanDamage under TARGET_PC (retail leaves it to the zeroed heap).
- `CHealthInfo::GetLastDamageWeaponMode()` is a TARGET_PC getter for the Touch arm.
- Touch's static interference reads `mgr.PlayerState(0)` (retail: mgr+0x150C).
- CalculateSeparation uses CPatterned's steering (0x61C), not CMediumIng's own `mSteering`.
