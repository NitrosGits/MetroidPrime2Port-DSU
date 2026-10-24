# Kralee REL (module 37): analysis notes

CKralee (KRAL, "Morphology: Kralee") is a CWallCrawler that crawls along walls and floors (Patrol) between waypoints.
It warps out (WarpOut → WarpOutExit, invisible and intangible) and back in (WarpIn) on visible/invisible timers or
on script messages. The module's code is upstream's `src/MetroidPrime/Enemies/CKralee.cpp`, with the loader
`include/MetroidPrime/ScriptLoader/SLdrKralee.hpp`. Retail asm:
`build/G2ME01/Kralee/asm/MetroidPrime/Enemies/CKralee.s` (62 fns) plus `REL/REL_Setup.s` (5). Per-function
table: `Kralee_functions.tsv` (B upstream body, H weak/template copy or `__sinit`, T REL runtime or unreferenced).

The driver area is Temple Grounds `0b_temple_hall` (world 3bfa3eff). It has two Kralees, e0010008f and e00100090,
both with visibleTime = invisibleTime = 0. Kralees also spawn in 02_temple_exploration (layer 3), 0k_temple_hall
and Agon Wastes 04_sand_mining (layer 2).

## Survey (kr1, 2026-10-23)

| function | verdict |
|---|---|
| SLdrKralee ctor (fn_37_BF8) | **fixed** (TARGET_PC, SLdrKralee.hpp:91-102): retail sets four defaults that upstream left at the base values. warpInSound = -1 (0xC20 `li r0,-0x1` / 0xC48 `stw 0x350`); knockback resistance 2.0 (0xD0C `stfs f6,0x8c`, rodata_20); ambient (1,1,1,1) (0xD1C/0xD30 `stw 0x298`); visorFlags 0xf (0xD34 `li r0,0xf` / 0xD40 `stw 0x2e8`) |
| AcceptScriptMsg (fn_37_1BA0) | **fixed** (TARGET_PC, cpp:117-122): on Create, retail sets locomotion type 7 (0x1C40 `li r4,0x7` / 0x1C44 `bl SetLocomotionType`); upstream had kLT_Relaxed. Patrol's own call (0x2120 `li r4,0x1`) is unchanged |
| every other body | equivalent to the asm |

`tools/retail_diff.py --diff ec2ac11ba` lists only noise: Think and AcceptScriptMsg call WarpIn (fn_37_117C,
unmarked, so "unported"), Think calls the weak `CDamageVulnerability::operator=` (fn_37_1A78, cpp:246), and Think's
constant 255 is the inlined `CColor::SetAlpha` at cpp:252 (0x1958 `lfs` rodata_50 = 255.0 / 0x196C
`stb 0x44f`). `tools/asmcheck.py check Kralee` crashes in `crbit`
(a None CR field, TypeError at asmcheck.py:699) on this module; it is not ours to fix.

## Coverage (kr1)

The specs are `tools/cov/specs/kralee.py` (breakpoints) and `kralee_force.py` (forced values). Run both, then merge:

    BUILD=$PWD/build-port-cov SECS=1500 tools/cov/run.sh tools/cov/specs/kralee.py /tmp/opencode/kr1_a \
      3bfa3eff,0b_temple_hall --con tools/cov/specs/kralee.con --frames 1300
    BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/kralee_force.py /tmp/opencode/kr1_f \
      3bfa3eff,0b_temple_hall --con tools/cov/specs/kralee_force.con --fast --frames 1300
    python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/kr1_a/gcov /tmp/opencode/kr1_f/gcov --module Kralee

The base driver must not use `--fast`: at 8 ticks a frame the killed Kralee is deleted before its PreRender.

What each driver does:

- `kralee.con` adds god mode and the Dark and Echo visors. It places e00100090 two units in front of the player,
  sends Increment/Decrement (warp out and in), cycles the dark, echo and combat visors (Render/AddToRenderer arms),
  sends Increment again and kills e0010008f.
- `kralee_force.con` with `kralee_force.py` forces visibleTime 1.5 / invisibleTime 1.0 in AcceptScriptMsg (the Create
  duration arms and the Think timers). At Think counters 300, 400, 500 and 600 it toggles mDisableMove,
  mAlignToFloor, the body controller's frozen duration and the area occlusion state. The driver hurts and
  Decrements the Kralee, sends Create three times, then Deactivate/Activate, and ends with damage and visor cycles.

Both runs: 0 `[port] caught` / `reach-stub`, exit 0.

Marked (every live arm ran): AcceptScriptMsg, WarpOutExit, Think, ~CKralee, SetupStateMachine, Render, ShouldPatrol,
ShouldWarpOut, AnimOver, SetFuncPtrs, RELMain, RELExit. Think's only open arms are cpp:155 1/2 and 156 1/4, the
inlined `CGameArea::GetOcclusionState` arm for an area whose post-construct data is not loaded. That arm is dead:
`CGameArea` unloading calls `mgr->PrepareAreaUnload(GetId())` (CGameArea.cpp:1066). This frees the area's script
objects (`FreeScriptObjects`, CStateManager.cpp:618) before `mPhase = kP_Allocate` (1069), so no Kralee thinks in
an unloaded area.

Open arms (cpp line), so these bodies stay unmarked:

| function | open arms |
|---|---|
| ctor 244C | 81 1/3, 87, 90 (invalid warp particle ids; both shipped Kralees have them) |
| PreRender 1080 | 267 (!mAlive: a killed Kralee is deleted before its next PreRender) |
| AddToRenderer 1018 | 282 1/6 (!mAlive) |
| WarpIn 117C | 301 1/3, 304 1/2 (invalid locator, data), 306 1/2 |
| Patrol 20DC | 313 3/4 (switch default, dead: EStateMsg has three values), 326 1/2 (no waypoint, data) |
| WarpOut 1DB8 | 352 3/4 (default, dead), 361 0/2 (CanIssueCommand true, timing) |
| LoadKralee 13C | 408 (no model data) |

Not driven arm by arm: the SLdr ctor and dtor, `__sinit`, the weak copies (H) and fn_37_1398 (unreferenced).

## Smoke family (proposal)

`kral`: area `3bfa3eff,0b_temple_hall`, driver `tools/cov/specs/kralee.con`, 1300 frames, not fast. Expect
`objs Kralee` to list e0010008f and e00100090, then only e00100090 after the kill. The pass condition is 0
`[port] caught` / `reach-stub`.

## Second pass (kral1, 2026-10-24)

`tools/cov/specs/kral1_force.py` run with `COV_GDB_SCRIPT=tools/cov/force_ret.py` (writes live for one
call, restored at its return; `FORCE_INNER` forces a callee's answer while the body runs), merged with
the kr1 base run (`gcov_report.py build-port-cov /tmp/opencode/kral_{a,f,g}/gcov --module Kralee`).

| body | arms | forced | still open |
|---|---|---|---|
| 20DC Patrol | 11/12 -> **marked** | `FORCE_INNER` on `CPatterned::GetConnectedObject`: it answers kInvalidUniqueId while Patrol runs, so `*waypoint == kInvalidUniqueId` (line 329's no-waypoint arm) | 316:3/4 dead: the msg switch covers 0x2190/0x21AC/0x222C and EStateMsg has three values |
| 1DB8 WarpOut | 8/9 -> **marked** | `FORCE_INNER` on `CAnimationState::CanIssueCommand`: it answers true while WarpOut runs (the generate command arm at line 364) | 355:3/4 dead: the same three-value msg switch |
| 244C ctor | 13/16 | - | 81:1/3 (an inlined guard on the init list's last line), 87:1/2 and 90:1/2: the shipped Kralees carry invalid warp particle ids, so the `!= kInvalidAssetId` arms need a valid id; the `mWarpInParticleEffect`/`mWarpOutParticleEffect` writes at the ctor entry (FORCE_RET phases 1/0/3, valid and invalid) did not move either arm |
| 13C LoadKralee | 9/10 | - | 411:1/2 (the null modelData return): `modelData.m_valid = 0` in a FORCE_RET write is accepted at the entry but the restore cannot see the local ("No symbol modelData in current context"), and the arm stays untaken - the optional is not addressable at -O2 |
| BF8 SLdrKralee ctor | - | - | `inline` in SLdrKralee.hpp:46, no standalone body to mark (its retail defaults are kr1's fix) |

Marked this pass: Patrol and WarpOut. The PreRender/AddToRenderer/WarpIn rows the earlier table listed
were marked before this pass.
