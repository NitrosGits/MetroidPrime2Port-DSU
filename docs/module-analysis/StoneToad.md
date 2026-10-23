# StoneToad REL (module 77): analysis notes

CStoneToad ("Mechanoid: Watchdrone") is a CPatterned that sleeps on its start position. A safe zone touching it
(or a hurting projectile/bomb, via AcceptScriptMsg) provokes it: WakeUp → ReactToSafeZone / ReactToWeapon →
MoveAway to its first active waypoint → Wait; when un-provoked (Decrement / safe zone off) it does MoveBack →
GoToSleep → Sleep. It turns its head at the player with four additive aim animations (UpdateLookAt). The module's
code is upstream's `src/MetroidPrime/Enemies/CStoneToad.cpp`, with the loader
`include/MetroidPrime/ScriptLoader/SLdrStoneToad.hpp`. Retail asm:
`build/G2ME01/StoneToad/asm/MetroidPrime/Enemies/CStoneToad.s` plus `REL/REL_Setup.s` (90 fns in total).
Per-function table: `StoneToad_functions.tsv` (B upstream body, H weak/template copy or `__sinit`, T REL runtime).

The only spawn area is Torvus Bog `09_swamp_catacombs_dark` (world 3dfd2249, area 65). It has two toads:
e00410048 (-46.1, -197.3, -45.0) and e00410066 (-22.3, -184.7, -45.1), both hp 5 and asleep, and the safe zone
e00410043 that provokes e00410048.

## Survey (st1, 2026-10-23)

| function | verdict |
|---|---|
| SLdrStoneToad ctor (inlined in LoadStoneToad, fn_77_4C8) | **fixed** (TARGET_PC, SLdrStoneToad.hpp): retail sets three defaults upstream left at the base values. Knockback resistance 2.0 (0x510 `lfs` rodata_0 / 0x528 `stfs 0x480(r1)`); ambient (1,1,1,1) (0x52C `bl __ct__6CColorFffff` / 0x548 `stw 0x68c(r1)`); visorFlags 0xf (0x534 `li r0,0xf` / 0x538 `stw 0x6dc(r1)`). No echo emitter or sound ids in this SLdr |
| every other body | equivalent to the asm (AcceptScriptMsg's `WeaponHits` import has upstream's `WeaponHurts(const CWeaponMode&)` signature) |

## Coverage (st1)

Spec `tools/cov/specs/stonetoad.py` (breakpoints on the states, Touch, AcceptScriptMsg, render hooks); driver
`tools/cov/specs/stonetoad.con`:

    BUILD=$PWD/build-port-cov SECS=1500 tools/cov/run.sh tools/cov/specs/stonetoad.py /tmp/opencode/st1_a \
      3dfd2249,09_swamp_catacombs_dark --con tools/cov/specs/stonetoad.con --frames 1500
    python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/st1_a/gcov --module StoneToad

The driver adds god mode and the light beam, activates safe zone e00410043 (e00410048: Touch → ReactToSafeZone →
WakeUp → MoveAway → Wait), damages and shoots e00410066, deactivates the safe zone and Decrements e00410048
(MoveBack → GoToSleep → Sleep), sends Increment/Decrement/AIUpdateDisabled/Deactivate/Activate to e00410066 and
kills it. 0 `[port] caught` / `reach-stub`, exit 0.

Marked (20, every live arm ran): GetCollisionResponseType, Touch, Sleep, MoveAway, Wait, MoveBack, the triggers
HitBySafeZone/MoveBackFinished/ShouldMoveAway/MoveAwayFinished/ShouldPatrol/AnimOver, PreThink, Render, PreRender,
AddToRenderer, SetupStateMachine, SetFuncPtrs, RELMain, RELExit.

Not reached: the weapon provoke. Beam, charged, missile, morph-ball bomb and power bomb hits on e00410066 never made
`WeaponHurts` true (only console `damage ... bomb` took hp 5→4, without a projectile), so AcceptScriptMsg's weapon
arm (cpp:215-226), ReactToWeapon and HitByWeapon never ran. Likely needs a weapon that the toad's vulnerability
accepts (a later beam/annihilator) or a forced spec.

Open arms (cpp line), so these bodies stay unmarked:

| function | open arms |
|---|---|
| AcceptScriptMsg 22F4 | weapon arm 215, 222-226; 129, 137, 143, 144, 166, 194, 241 3/4 (connection-state arms) |
| ReactToWeapon 1A48, HitByWeapon 1478 | never ran |
| WakeUp 1EC0 | 282 3/4 (default, dead), 291 0/2 (timing), 298/301 (no active waypoint, data) |
| ReactToSafeZone 1C84 | 312 3/4 (default, dead), 320 0/2 (timing) |
| GoToSleep 158C | 397 3/4 (default, dead), 405 0/2 (timing) |
| Think 21E4 | 254 1/2 (no collision actor manager) |
| ShouldMoveBack 14C8 | 432 1/2 |
| GetMoveDirection 10B0 | 479 (default, dead), 481 (zero away position), 490 1/2 |
| IsMovingAway/BackToStart 12D4/11DC | 461/471 1/2 (exactly at the target, data) |
| ctor 2F74 | 85/92/99/106 (missing look animation, data) |
| UpdateLookAt B48 | `look.GetX() >= 0` 1/2 (player never below the eye in the drive); ApproachWeight 497 1/2 is the inlined `0.f > 0.f` copy, constant false |
| LoadStoneToad 4C8 | 555 (no model data) |

Not driven arm by arm: the SLdr ctor and dtor, the implicit dtor, `__sinit` and the weak copies (H).

## Smoke family (proposal)

`stod`: area `3dfd2249,09_swamp_catacombs_dark`, driver `tools/cov/specs/stonetoad.con`, 1500 frames, not fast.
Expect `objs StoneToad` to end with only e00410048. The pass condition is 0 `[port] caught` / `reach-stub`.

## asmcheck

`tools/asmcheck.py check StoneToad src/MetroidPrime/Enemies/CStoneToad.cpp`: 0 findings over the 20 markers
(the bitfields carry their `0x860` masks in CStoneToad.hpp).
