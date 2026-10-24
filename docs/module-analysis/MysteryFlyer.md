# MysteryFlyer REL (module 45): analysis notes

CMysteryFlyer (MYSF, "Morphology: War Wasp") is a small flying enemy. It patrols, follows an attack path once
alerted, then hovers, approaches, dodges and shoots its projectile. Wasps can join a TeamAiMgr and keep
apart from each other (CalculateSeparation). The module's code is upstream's
`src/MetroidPrime/Enemies/CMysteryFlyer.cpp`, with the loader `include/MetroidPrime/ScriptLoader/SLdrMysteryFlyer.hpp`.
Retail asm: `build/G2ME01/MysteryFlyer/asm/MetroidPrime/Enemies/CMysteryFlyer.s`. Per-function table:
`MysteryFlyer_functions.tsv` (B upstream body, H weak/template copy, T REL runtime).

The driver area is Temple Grounds `0e_temple_hall` (world 3bfa3eff), layer 0. It has three wasps,
e003100fe, e003100ff and e00310101. All of them have an attack path, and the area has no TeamAiMgr.

## Survey (mf1, 2026-10-23)

| function | verdict |
|---|---|
| SLdrMysteryFlyer ctor (in fn_45_170) | **fixed** (TARGET_PC): retail sets four defaults that upstream left at the base values. isEchoEmitter = true (0x1BC `li r0,1`, 0x1CC `stb r0,0x66c(r1)`); KnockBackResistance = 2.0 (0x1C0 `lfs`, 0x1D8 `stfs f0,0x488(r1)`); ambient (1,1,1,1) (0x1DC `bl __ct__6CColorFffff`, 0x1F8 `stw r3,0x694(r1)`); visorFlags 0xf (0x1E4 `li r0,0xf`, 0x1E8 `stw r0,0x6e4(r1)`) |
| Approach (fn_45_1850) | **fixed** (TARGET_PC): retail sums the destination as `(translation + range * forward) + hoverHeight * up` (0x18E4/0x18EC/0x18F4 `fadds`, then 0x1908-0x1910 `fadds`). Upstream summed forward and up first, which differs only by float rounding |
| every other body | equivalent to the asm (asmcheck) |

The x814 bitfields now carry their positions in the header (MWCC MSB-first). The four bits are
mMoving 0x80, mHasLineOfSight 0x40, mAlerted 0x20 and mHasAttackPath 0x10. They are confirmed by Patrol 0x1B70
`rlwimi r0,r3,7,24,24` and UpdateLineOfSight 0xB40 `rlwimi r0,r3,6,25,25`.

asmcheck has 8 findings left, all on marked bodies and all noise:

- ctor (0x2A18/0x2A74/0x2A7C): CALLs to the CProjectileInfo, CPathFindSearch and CSteeringBehaviors ctors. These are member initialisers.
- dtor fn_45_2618: CALL/CALLS/BRANCH. This is the compiler-generated member and base dtor chain (fn_45_9B0, fn_45_26AC) plus `delete`.

ResetAttack's retail `fmadds` can differ from the host in the last bit.

## Coverage (mf1)

Coverage build with spec `tools/cov/specs/mysteryflyer.py` and driver `mysteryflyer.con`:

    BUILD=$PWD/build-port-cov SECS=1800 timeout 1900 tools/cov/run.sh tools/cov/specs/mysteryflyer.py <c> \
      3bfa3eff,0e_temple_hall --con tools/cov/specs/mysteryflyer.con --fast --frames 3100
    python3 tools/cov/gcov_report.py build-port-cov <c>/gcov --module MysteryFlyer

The driver does these steps in order:

1. god mode and missiles.
2. `Activate`, place the player, then `Alert` all three wasps.
3. Damage one wasp.
4. Force Approach, Dodge, Attack, Hover and GoToPatrol with `fsm`.
5. Run `codecall` for CalcLineOfSight and ResetAttack.
6. Kill one wasp, then `Deactivate` and `Delete` another.

Every FSM state runs. Once alerted, the wasps stay in FollowAttackPath; their attack-path waypoints loop.
Every arm of the ctor, Think, Patrol, Hover, Dead, UpdateLineOfSight, UpdateMovement and UpdateSteering
is taken. Approach, Attack and Dodge miss only the switch default (269/300/323 3/4). EStateMsg has three
values, so that default is dead.

Open arms (cpp line:taken/total), so these bodies stay unmarked:

| function | open arms |
|---|---|
| AcceptScriptMsg | 125, 126 (the War Wasp body has no KnockBack state) |
| DoUserAnimEvent | 141:1/2 (no FadeIn event) |
| Generate | 154, 155 (needsToGenerate is always true here) |
| GoToPatrol | 178-195 (no Patrol connection in this area) |
| FollowAttackPath | 233, 241 (every wasp has an attack path) |
| NearPatrol | 367:0/2 |
| InDetectionRange | 375:2/4 (the unalerted detection arm; patrolling wasps never saw the player) |
| AttackPathOver | 380 |
| ShouldAttack | 388, 389 (team and distance arms) |
| Leash, LineOfSight | never called (no FSM transition uses them here) |
| JoinTeam, QuitTeam | 426-441 (no TeamAiMgr in the area) |
| CalculateSeparation | 474:1/6, 5/6 (another wasp in a different area) |
| PathOver | 411:2/4 |
| LoadMysteryFlyer | 561:1/2 (null model data) |

Not driven: the loader typedef readers, the SLdr dtors and __sinit (they run, but nobody checked them arm by arm),
and IsListening/GetSearchPath.

## Smoke family (proposal)

`mysf`: area `3bfa3eff,0e_temple_hall`, driver `tools/cov/specs/mysteryflyer.con`, about 3100 frames. The driver
runs god mode, Activate and Alert, then damage, forced states, kill, Deactivate and Delete. Expect three wasps in
`objs MysteryFlyer`, then the killed one Dead. The pass condition is 0 `[port] caught` / `reach-stub`.

## Coverage (mfly1: the 11 rows still unmarked)

Same spec and driver as mf1 (`mysteryflyer.py` + `mysteryflyer.con`, 3100 frames, `--fast`,
`caught/reach-stub: 0`). The TSV's marker column is stale; the list below is the TSV's B rows minus the
`// @ fn_45_*` markers in src/include. No body differed from the asm (`asmcheck.py check`: no
CROP/XCMP/XSTORE/MASK/OFFS finding on any of the 11), so nothing was changed but two header comments.

Marked (4):

| fn | evidence |
|---|---|
| 0 IsListening | objdiff 100%; `li r3,0x1; blr` at 0x0/0x4 = `return true` |
| 8 GetSearchPath | objdiff 100%; `addi r3,r3,0x818; blr` at 0x8/0xC = `&mPathFindSearch`, which is therefore at 0x818 (now in the header) |
| 840 LoadTypedefMysteryFlyerData | objdiff 100%; the compare tree re-read (0x894 0x1559f27, 0x8ac 0xc75998aa, 0x8c0 0x845ef489, 0x8d4 0xcea30138, 0x8e8 0x5f3fffd6, 0x8fc 0x51253ba3, default `input.ReadBytes` at 0x910) is exactly the six C++ cases. gcov 25/28 lines; the only untaken arm is the unknown-property default, data-gated (shipped lists carry only those six ids) |
| 9B0 ~SLdrMysteryFlyerData | objdiff 100%; the empty inline dtor is the SLdrDamageInfo chain (0x9d8) plus the compiler's null-this (0x9cc) and delete-flag (0x9e0) guards |

Left unmarked (6):

| fn | why |
|---|---|
| 170 LoadMysteryFlyer | gcov 573:1/2: the `if (!modelData) return nullptr` arm needs LdrToModelData to fail; objdiff 81.4% (the SLdr ctor is inlined here in retail) |
| 7C4 / A08 | **the REL names and the bodies disagree**: the code at 0x7C4 is an `SLdrMysteryFlyer` deleting dtor (0x7e0 null-this, 0x7ec `__dt__20SLdrMysteryFlyerDataFv`, 0x7f8/0x804/0x810 the three sub-dtors, 0x820 Free) while the symbol there is `__ct__16SLdrMysteryFlyerFv`, and the code at 0xA08 is the `SLdrMysteryFlyerData` ctor (0xa20 `stw -1,0x0`, 0xa24 `__ct__14SLdrDamageInfoFv`, 0xa64 2.0f, 0xa6c 1) while the symbol there is `__dt__16SLdrMysteryFlyerFv`. Our object emits `__ct__16SLdrMysteryFlyerFv` at 0x7C4 (objdiff None) and `__dt__16SLdrMysteryFlyerFv` at 0xA08 (27.8%), so the two names are attached to the wrong bodies in the symbol table. Marking by code would put fn_45_7C4 on `~SLdrMysteryFlyer()` and fn_45_A08 on `SLdrMysteryFlyerData()`; the lane left both alone - orchestrator's call |
| C10 JoinTeam | 436:1/2, 439:0/2, 440:0/2, 440:1/2: needs a TeamAiMgr in the area (the hall has none), so `GetAssociatedTeamId` never returns an id and the `TCastToPtr<CScriptTeamAiMgr>` arm never runs |
| 23B8 AcceptScriptMsg | 125:1/2, 126:1/2: `HasBodyState(pas::kAS_KnockBack)` is false for the War Wasp body and `GetCurrentStateId() != kAS_KnockBack` never sees any other state; the other states (Create/AreaLoaded/Damage/Alert) all ran |
| 2BF8 __sinit_CMysteryFlyer_cpp | objdiff 100%, but it is compiler-generated from `skTriggers`/`skStates`/`skCodeFuncs`: there is no source line to put a marker on, which is why bulk_mark_matched skips it (no marker for __sinit exists anywhere in src) |

## mfly2 (2026-10-24, base 1219ecb59): fn_45_237C DoUserAnimEvent marked

Source (cpp:138-145): `if (!(type == kUE_FadeIn ? true : false)) CPatterned::DoUserAnimEvent(...)`.
Retail fn_45_237C (`cmpwi r6,0xe`, the ?: flag idiom, `clrlwi.`/`bne` skip, else
`bl DoUserAnimEvent__10CPatterned...`) matches it, and the host build has `cmp $0xe,%ecx` at the entry
block (`jne` to the base call, `ret` on the FadeIn side), so the by-value EUserEventType lives in ecx
(SysV's fourth integer argument) and is never spilled.

Shipped MYSF animations have no FadeIn event (mf1), and the run showed exactly **one**
DoUserAnimEvent call in 3100 frames, so the two arms were measured in two launches, with the fixture
changing ONLY the register that holds the by-value argument on a real invocation:

    MFORCE_CALL=1 (forced): call 1 type 0x0 -> 14 (kUE_FadeIn); ecx snapshot 0x0; returned with
      baseCalls=0; ecx restored to 0x0 (read back 0x0)
    MFORCE_CALL=0 (natural-only): call 1 type 0x0; "base DoUserAnimEvent hit from
      CMysteryFlyer::DoUserAnimEvent (call 1)"; returned with base_hits=1

so the skip arm has a real return (not just an entry) with zero caller-attributed base hits, and the
natural arm reached the CPatterned base from this function's own call site. No stack write, no fabricated
call or object, no source probe; the only mutation is `set $ecx`, restored at the return.

Merged gcov (both runs, same build): **fn_45_237C 4/4 lines, 2/2 branches, ALL-ARMS**. Marker added
(`// @ fn_45_237C`). Mocked-gdb regression `tools/cov/specs/mfly2_douseranimevent_selftest.py` -> 0
scenario failures (both arms, forced call reaching the base, forced call missing, unreturned call, ecx
restore mismatch, no natural call, implausible type value, install failure). Evidence
`/tmp/opencode/mfly2_evidence` (both `.gcda` 52311cdf.../049c158f..., matching `.gcno` 03028ec0..., source
5eee6f7e..., binary a054cbf4..., `doUserAnimEvent.disasm.txt`, both diag/fixture transcripts).
