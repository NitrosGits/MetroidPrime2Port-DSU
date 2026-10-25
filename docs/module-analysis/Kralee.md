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
| ctor 244C | 81 1/3, 87, 90 — **resolved and marked in the third pass** (kral2): the shipped ids are valid, the open edge is the skip arm |
| PreRender 1080 | 267 (!mAlive: a killed Kralee is deleted before its next PreRender) |
| AddToRenderer 1018 | 282 1/6 (!mAlive) |
| WarpIn 117C | 301 1/3, 304 1/2 (invalid locator, data), 306 1/2 |
| Patrol 20DC | 313 3/4 (switch default, dead: EStateMsg has three values), 326 1/2 (no waypoint, data) |
| WarpOut 1DB8 | 352 3/4 (default, dead), 361 0/2 (CanIssueCommand true, timing) |
| LoadKralee 13C | 408 (no model data) — **marked in the third pass** (kral2): dead by the whole-disc scan |

Not driven arm by arm: the SLdr dtor, `__sinit`, the weak copies (H) and fn_37_1398 (unreferenced). The SLdr
ctor is `inline` in SLdrKralee.hpp and is marked (third pass).

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
| 244C ctor | 13/16 | - | 81:1/3, 87:1/2, 90:1/2. The FORCE_RET writes at the ctor entry did not move them because the init list overwrites the members from the stack arguments; see the third pass: both ids are VALID in all ten disc placements, so 87/90's open edge is the skip arm, dead by data |
| 13C LoadKralee | 9/10 | - | 411:1/2 (the null modelData return): `modelData.m_valid = 0` in a FORCE_RET write is accepted at the entry but the restore cannot see the local ("No symbol modelData in current context"), and the arm stays untaken - the optional is not addressable at -O2. Third pass: dead by data (every placement's ancs is a real ANCS) |
| BF8 SLdrKralee ctor | - | - | its retail defaults are kr1's fix; it is `inline` in SLdrKralee.hpp and IS markable — marked in the third pass (kral2), which also maps every store of the body |

Marked this pass: Patrol and WarpOut. The PreRender/AddToRenderer/WarpIn rows the earlier table listed
were marked before this pass.

## Third pass (kral2, 2026-10-25)

Marks three rows: fn_37_13C LoadKralee, fn_37_BF8 SLdrKralee ctor (a header-inline body can carry a marker,
precedent SLdrDarkCommando.hpp:14) and fn_37_244C the ctor. The deciding evidence is a whole-disc scan, not the
two 0B Kralees: `tools/scan_kralee_props.py` (read-only over `tools/echoes_disc.py`).

**Scan** (10 KRAL placements: 0b_temple_hall 2, 02_temple_exploration L3 4, 0k_temple_hall 1, 04_sand_mining L2 3):
ancs 0x34c0fb32 (ANCS, present) and **warpIn 0x884ab091 / warpOut 0x533a9efa (both PART, present) on all ten**,
visibleTime/invisibleTime 0.0 on all ten, no unknown property id. Therefore:

- kr1's and kral1's "both shipped Kralees carry invalid warp particle ids" is **backwards**. The `!= kInvalidAssetId`
  arms (cpp 88/91) are the ones every load takes; the open edge is the *skip* arm, which needs a placement that omits
  the property (the loader default is kInvalidAssetId, SLdrKralee.hpp:61-62; SLdrKralee.inc:100-107 reads it) — no
  placement on the disc does.
- LoadKralee's `if (!modelData) return nullptr` (cpp 415): LdrToModelData (ScriptLoader.cpp:446-459) returns an empty
  optional only when both `GetResourceTypeById` answers are 0. The model argument is kInvalidAssetId
  (`CResLoader.cpp:208-214` answers 0 for an id in no pak) and every placement's ancs is a real ANCS, so the arm is
  dead by data — the same proof, for the same arm, as CFlyingPirate's (CFlyingPirate.cpp:2103).
- The SLdr ctor does have a body to mark: `inline SLdrKralee::SLdrKralee()` at SLdrKralee.hpp:59.

**SLdrKralee ctor (BF8) store map** — retail r31 = the sldr base; rodata labels are the module's:

| retail offset | member | value | asm |
|---|---|---|---|
| +0x0 / +0x40 / +0x284 | editorProperties / patterned / actorInformation | the nested ctors | 0xC0C/0xC14/0xC1C |
| +0x3C | flavor | 0 | 0xC68 |
| +0x40..+0x5C | patterned mass, speed, turnSpeed, detectionRange, detectionHeightRange, detectionAngle, minAttackRange, maxAttackRange | 25, 3, 720, 5, 5, 90, 4, 20 | rodata_0/4/8/C/C/10/14/18 |
| +0x74/+0x78/+0x80 | contactDamage dI_WeaponType / dI_Damage / dI_KnockBackPower | 9 / 10 / 5 | 0xCC0 `li r0,0x9`, rodata_1C, rodata_C |
| +0x84 / +0x8C | damageWaitTime / health.hI_KnockBackResistance | 3 / 2 | rodata_4 / 0xD0C rodata_20 (kr1 fix) |
| +0x1EC/+0x1F0/+0x204 | collisionRadius / collisionHeight / unknown_0xe287d8dd | 0.2 / 5 / 0 | rodata_24 / rodata_C / rodata_28 |
| +0x298 | actorInformation.lighting.ambientColor | CColor(1,1,1,1) | 0xD1C `bl __ct__6CColorFffff` with rodata_2C x4, 0xD30 stw (kr1 fix) |
| +0x2E8 | actorInformation.visor.visorFlags | 0xf | 0xD34/0xD40 (kr1 fix) |
| +0x2FC..+0x344 | the 19 Kralee floats | 2.5 2.5 360 180 120 0.4 1 0.02 1 1 0 0 0 0 2.5 10 10 1 50 | rodata_30,30,34,38,3C,40,2C,44,2C,2C,28x4,30,1C,1C,2C,48 |
| +0x348/+0x34C/+0x350/+0x354 | warpInParticleEffect / warpOutParticleEffect / warpInSound / warpOutSound | -1, -1, -1, -1 | 0xC20 `li r0,-0x1`, stw at 0xC28/0xC38/0xC48/0xC58 (the sounds are the TARGET_PC block) |
| +0x358/+0x359 | initiallyPaused / initiallyInvisible | 0 / 0 | 0xDFC/0xE00 stb |

Every declared member is stored once, in declaration order (the TARGET_PC block's four stores are the retail
0xD0C-0xD40 group), so the host header equals the retail body. The ctor is not a standalone symbol in the port: the
load ran it inline — SLdrKralee.hpp:59-103 all count 2 in the coverage build, from the two placements 0B loads.

**Runs** (both 0 `[port] caught` / `reach-stub`, exit 0; coverage binary sha256 9611da74...):

    BUILD=$PWD/build-port-cov SECS=2400 tools/cov/run.sh tools/cov/specs/kralee.py /tmp/opencode/kral2-ev/nat \
      3bfa3eff,0b_temple_hall --con tools/cov/specs/kralee.con --frames 1300
    BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh \
      tools/cov/specs/kral2_force.py /tmp/opencode/kral2-ev/force 3bfa3eff,0b_temple_hall \
      --con tools/cov/specs/kralee.con --fast --frames 1300
    python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/kral2-ev/nat/gcov \
      /tmp/opencode/kral2-ev/force/gcov --module Kralee

| body | lines/branches (merged) | untaken | verdict |
|---|---|---|---|
| 244C ctor | 40/40, 13/16 | 82:1/3, 88:1/2, 91:1/2 | **marked** (all three proved, below) |
| 13C LoadKralee | 13/13, 9/10 | 415:1/2 | **marked** (dead by data) |
| BF8 SLdr ctor | SLdrKralee.hpp:59-103 all count 2 | - | **marked** |

The ctor's three open edges:

- **88:1/2, 91:1/2** are the skip arms: gcov's fall-through edge is the `push_back_unsafe` body and it ran twice
  (`branch 1 taken 2 (fallthrough)`, `branch 2 taken 0`), the untaken edge being the `cmp $0xffffffff` / `je`
  (host 0xf28020 and 0xf28040), i.e. the id-invalid case no placement has.
- **82:1/3** is a real source arm, corrected in **kral2b (2026-10-25)**: gcov's untaken normal edge is
  **block 3 -> 5, count 0**, not the EH edge 6 -> 37. Block 3 maps to lines 82/58; the init-list attribution
  at line 82 hides `mWarpState(initiallyInvisible ? kWS_Invisible : kWS_Visible)` at line 58. In the frozen
  kral2 host binary (`../evidence-archive/kral2/bin/metroid_prime2_port`), `0xf27db1 test %r15b,%r15b`,
  `0xf27db7 mov $1,%eax`, and **`0xf27dbc jne 0xf27dc9`** select Invisible (1) when the argument is true;
  the false path increments its coverage counter and clears eax to Visible (0), then stores to `0x964(%rbx)`.
  Thus the untaken arm is **initiallyInvisible=true**, dead by shipped data: property **0x738d1c80 is explicitly
  byte 00 on all ten placements**, and none of the generated objects is KRAL (scan below).
  `SLdrKralee.inc:120-122` reads the bool, `CKralee.cpp:429` forwards it unchanged; omission would also yield
  false (`SLdrKralee.hpp:103`), but no shipped KRAL relies on that default. The EH arc exists separately and
  cannot discharge this ternary. The marker at `CKralee.cpp:40` is just `// @ fn_37_244C`, with no proof label.

**Reproducible kral2b scan** (read-only, `MP2_DISC` optionally selects the user's G2ME01 ISO):

```sh
python3 tools/scan_kralee_props.py
```

Exit 0; each of the ten placement lines reports `initiallyInvisible=false(explicit:00)`. Exact summary:

```text
== 10 KRAL placements, 0 generated KRAL objects: ancs NOWHERE=0, ancs present=10, warpIn valid=10, warpOut valid=10, warpIn unset=0, warpOut unset=0, unknown prop id=0
== initiallyInvisible: false=10, true=0, explicit=10, default=0
== coverage: worlds=12/12, areas=291/291, SCLY layers=1620/1620, SCGN sections=291/291, SCGN objects=7764, parse_errors=0
```

The scanner visits every MLVL resource (including unnamed worlds), every SCLY layer and the SCGN section
after each area's layers, extracting script sections even from multi-section MREA blocks. Generated KRAL
records, if present, receive their own `KRAL SCGN` lines and the same property decoding. Invalid/truncated
records or missing sections produce diagnostics and a nonzero exit, so partial scans cannot establish absence.

**415:1/2** is LoadKralee's null return (gcno block 63 -> 73, the direction the two spawns rule out). kral2's force
attempt failed for a reason worth recording: `LdrToModelData` returns `rstl::optional_object<CModelData>` by value,
so the hidden return pointer takes rdi and the animation-set reference is in **rcx, not rdx** — the log's
`set failed (*(unsigned int*)((char*)4294967295))` is rdx holding the CAssetId argument. Dead by data instead.

Earlier bodies (AcceptScriptMsg, Think, PreRender, AddToRenderer, WarpIn, Patrol, WarpOut, ~CKralee,
SetupStateMachine, WarpOutExit, the three triggers, SetFuncPtrs, RELMain/RELExit) are marked from kr1/kral1; their
arms were not re-driven this pass.
