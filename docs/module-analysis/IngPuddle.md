# IngPuddle REL (module 32): analysis notes

CIngPuddle (IPUD, "Ing Spot") is the dark-world Ing blob that oozes along the floor. It is a CPhysicsActor run by a
CGenericFSM2 (states Start, Patrol, Idle, Dead; triggers HasPatrolPath, StateOver, IsDead). It is **not** a
CPatterned, so the console's `fsm`/`statecall`/`trigcall`/`codecall`/`kill` refuse it ("not a patterned actor").
The module's code is upstream's `src/MetroidPrime/Enemies/CIngPuddle.cpp`, with the loader
`include/MetroidPrime/ScriptLoader/SLdrIngPuddle.hpp`. Retail asm:
`build/G2ME01/IngPuddle/asm/MetroidPrime/Enemies/CIngPuddle.s` (63 fns) plus `REL/REL_Setup.s` (5). Per-function
table: `IngPuddle_functions.tsv` (B upstream body, H weak/template copy, T REL runtime).

Spawns: only 7 IPUDs on the whole disc, all in Agon Wastes `06_sand_alamofront_dark` (world 42b935e4), layer 2:
e08260040, e08260044, e0826013f, e08260140, e08260142 (10000 HP, idle, immune to `damage` and shots) and
e082600c1, e082600c2 (5 HP, patrolling). Layer 2 is off at area load; `layer 2` turns it on.
All seven share blob 743b423a, normal hit 41e6fc41, heavy hit d9c67cb3 and death dad46b11, so every
"effect not valid" arm is dead in shipped data.

## Survey (ipd1, 2026-10-23)

| function | verdict |
|---|---|
| SLdrIngPuddleData ctor (fn_32_600) | **fixed** (TARGET_PC, SLdrIngPuddle.hpp): retail sets all five sounds to -1 (0x624 `li r0,-0x1`; `stw r0` at 0x63C-0x64C to +0x20..+0x30). Upstream left them at the base default |
| PreThink (fn_32_1BD8) | **fixed** (TARGET_PC, cpp PreThink): the blob scale rate is lbl_32_rodata_28 = 0x3F199999 = 0.59999996f (0x1C64 `lis` / 0x1C68 `lfs`, used by 0x1C78 `fmuls f11,f1,f31`). Upstream had 0.6f |
| every other body | equivalent to the asm (asmcheck) |

The 0x5ec bitfield `mIdling` now carries its position in the header (bit 0x80; ctor 0x2A4C-0x2A54
`lbz 0x5ec; rlwimi r0,r4,7,24,24; stb 0x5ec`; Idle writes it at 0x1660/0x166C and 0x16B8/0x16C4).

asmcheck (`python3 tools/asmcheck.py check IngPuddle src/MetroidPrime/Enemies/CIngPuddle.cpp`) has 0 NOPOS/BIT
findings. The rest is noise:

- CALL findings on unported template/helper copies (fn_32_3170 etc.), `CToken::Lock` and `internal_dereference`
  behind inline token/rc_ptr code.
- LoadIngPuddle: the `.inc` loop's calls to fn_32_600 / 2FC / 59C (SLdr ctor, LoadTypedef, dtor) and its
  6 conditions.
- HandleDamage XCALL GetDamageVulnerability: the asm calls it through vtable +0x40 (`bctrl` at 0x6D4); the C++
  makes the same virtual call.

## Coverage (ipd1)

Spec `tools/cov/specs/ingpuddle.py`, driver `tools/cov/specs/ingpuddle.con` (its header lists each step):

    tools/games_running.sh --wait
    BUILD=$PWD/build-port-cov timeout 900 tools/cov/run.sh tools/cov/specs/ingpuddle.py /tmp/opencode/ipd1/run \
      42b935e4,06_sand_alamofront_dark --con tools/cov/specs/ingpuddle.con --fast --frames 3200
    python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/ipd1/run/gcov --module IngPuddle

The driver turns on layer 2, activates and deactivates the puddles, deletes c2's first waypoint, shoots the
idle puddle (power, missile, charged), sends `damage` (null-weapon return), shoots c1 (normal-hit arm; c1 survives
and keeps patrolling) and kills c2 with a charged shot (heavy-hit arm; Patrol → Dead), and finally Deletes
e08260142 (`objs IngPuddle` 6 → 5).

25 markers: every live arm of each marked body ran. Open arms in marked bodies are dead or data-dead:

| function | arm (cpp line) | class |
|---|---|---|
| ctor 22C0 | optional-not-set and EH arms | data-dead / compiler |
| dtor 1D34 | EH and deleting-variant arms | compiler |
| AcceptScriptMsg 182C | 138 effect null on Deactivate | defensive (blob always spawned at Create) |
| GetTouchBounds 1790 | 167 optional not engaged | defensive (set every Think) |
| Dead 1490 | 209 non-Activate msg; 212 death effect invalid | dead (deleted same frame); data-dead |
| GetStateMachine 1098 / InitializeStateMachine FD0 | 284 / 294 FSM not loaded | defensive |
| UpdateBlobEffect D64 | 318 effect null | defensive |
| MoveAlongSurface B64 | 340 `IsMagnitudeSafe` false | dead: the only caller passes a unit vector, so \|projected\|² > 0.4 |
| UpdateTouchBounds A70 | 353 0/2 | gcov artifact (no source branch) |
| HandleDamage 688 | 364, 373, 382 effect invalid / alloc null | data-dead |

Unmarked (open live arms):

| function | open arms |
|---|---|
| Idle 1628 | 199 Deactivate with a live emitter (needs an idle puddle to leave Idle; idle puddles are immune); 186 switch default (dead) |
| Patrol 1154 | 232/233 re-entry with a current waypoint (FSM never re-enters Patrol); 255 zero `toNext`; 257 CScriptAIWaypoint speed (data: WAYP only); 274 Deactivate with no emitter; 230 switch default (dead) |

Forcing these needs console `fsm`/`kill` support for non-CPatterned CGenericFSM2 actors (platform/, out of scope).

## Smoke family (proposal)

`ingp`: area `42b935e4,06_sand_alamofront_dark`, driver `tools/cov/specs/ingpuddle.con`, 3200 frames fast (last
check @f2900), NC no. Expect `objs IngPuddle` to show 6 objects at @f2700 and 5 after the Delete at @f2900; pass
condition 0 `[port] caught` / `reach-stub`.

## ipd2: HealthInfo (fn_32_0) - verified and marked

`include/MetroidPrime/Enemies/CIngPuddle.hpp:34`, the inline `CHealthInfo* HealthInfo() override { return
&mHealthInfo; }`. Retail fn_32_0 is exactly that: `build/G2ME01/IngPuddle/asm/.../CIngPuddle.s`
`/* 00000000 */ addi r3, r3, 0x460` and `/* 00000004 */ blr` - one add of the member's retail offset
(0x460) and a return, no branches. This build's offset is 0x4C8 (1224): host objects outgrow retail, so
the offset is not a constant to copy, and the check below is "return == this + mHealthInfo", not
"return == this + 0x460".

**Runtime return evidence** (spec `tools/cov/specs/ipd2_healthinfo.py`, driver
`tools/cov/specs/ingpuddle.con`, coverage build, `42b935e4,06_sand_alamofront_dark`, `--fast`, 3200
frames, `/tmp/opencode/ipd2/a`): `game rc=0`, `caught/reach-stub: 0`, normal inferior exit. The oracle
sits on the out-of-line copy the vtable dispatches to (`_ZN10CIngPuddle10HealthInfoEv`), writes
nothing and forces nothing; the puddles are damaged naturally by the driver's shots and `damage`.
For every call it captures `this`, the caller frame, arms a finish breakpoint and compares the actual
`$rax` with `this + mHealthInfo`:

    5496 calls, 5496 MATCH, 0 MISMATCH, 0 unreadable return, 0 unarmed breakpoints, status=OK
    callers: CActor::GetHealthInfo (5490), CStateManager::ApplyLocalDamage (3),
             CStateManager::RecordDamageSource (3)

The gcov data of the same run attributes **5496 hits to that header line** - the same count, so the
line's executions and the oracle's out-of-line calls agree (no call went through an inlined copy that
the oracle would have missed). The only live arm is the single `return &mHealthInfo`, so "every arm
ran" is this one path, taken 5496 times with the pointer verified each time.

`asmcheck.py check IngPuddle src/…/CIngPuddle.cpp fn_32_0` reports `MISSING` - it maps definitions in
the .cpp it is given, and this body is header-inline - so the body's asm was read by hand against the
`.s` above (`addi`+`blr`, 8 bytes, no branches).

Gates: host build 0 FAILED; `configure.py && ninja` + forced `build/G2ME01/ok` re-check **87 files
OK**; `gen_files_cmake.py --check` rc 0. Evidence frozen before the marker:
`/tmp/opencode/reports/ipd2-evidence/` (gcno, gcda, raw gcda JSON, the module's asm, binary/source
hashes, the oracle log, the run and game logs).

Independent review reproduced 5496 executions, 1/1 blocks and no branches from the
archived gcno/gcda pair. The still-present coverage binary matched the recorded
`ba5b08db...` hash and was frozen as `/tmp/opencode/ipd2-independent/executed-binary`;
its getter is a coverage increment followed by `lea 0x4c8(%rdi),%rax; ret`.
The pre-marker header hash matches the base git blob. The oracle logs six individual
pointer pairs and aggregates all 5496 matches; it does not retain every pointer pair.
