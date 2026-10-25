# IngSpiderballGuardian REL (module 35): analysis notes

CIngSpiderballGuardian (ISBG, "Spider Guardian") is the Ing boss in the Sanctuary Fortress spider-ball
puzzle. It rolls along waypoints and does proximity damage. Bombs stun it once its stun health
(40 per phase) is used up; the stun only takes kSM_ResistedDamage. Relays then call
BeginNextPuzzleSection, which raises mPhase to select the next GetPhaseProperties row. The module's code is
upstream's `src/MetroidPrime/Enemies/CIngSpiderballGuardian.cpp` plus `SIngSpiderballGuardian_FuncPtrs.cpp`,
with the loader `include/MetroidPrime/ScriptLoader/SLdrIngSpiderballGuardian.hpp`. Retail asm:
`build/G2ME01/IngSpiderballGuardian/asm/MetroidPrime/Enemies/CIngSpiderballGuardian.s`. Per-function table:
`IngSpiderballGuardian_functions.tsv` (B upstream body, H weak/template copy, T REL runtime).

Its only placement is Sanctuary Fortress `06_cliff_spiderballguardian`, layer 13 (e0014007a). It loads
about 500 frames after `layer 13`, and the FSM is set up on its first active Think, so send
`Activate` only after `objs IngSpiderball` lists it.

## Survey (isbg1, 2026-10-23)

| function | verdict |
|---|---|
| SLdrIngSpiderballGuardian ctor (in fn_35_178) | **fixed** (TARGET_PC): retail defaults actorInformation ambient to (1,1,1,1) (0x208 `bl __ct__6CColorFffff`, 0x224 `stw r3,0x694(r1)`) and visorFlags to 0xf (0x210 `li r0,0xf`, 0x214 `stw r0,0x6e4(r1)`); upstream left them at the base defaults. The placement sets both, so nothing changes in game |
| every other body | equivalent to the asm (asmcheck, objdiff) |

The x994 bitfields now carry their positions (`// 0x994_24` .. `0x994_30`, MWCC MSB-first: mEffectsActive 0x80,
mProximityDamageEnabled 0x40, mRolling 0x20, mStunned 0x10, mCanBeDamaged 0x08, mPlayStunSound 0x04,
mReturnToPatrolRequested 0x02). Stunned `extrwi r3,r0,1,27` and Dead's clear of 0x20 confirm them.
asmcheck has one remaining finding, a CALL in EnableEnergyBar at 0x24FC: retail's `GetStringIndexByName`
is the host's `CStringTable::GetStringIndex`, a name alias (the same as PortCDigitalGuardian).
mHitByPlayerProjectile and mDisabledAnimationDeltas are CPatterned bits, so their NOPOS lines are left
to that header.

## Coverage (isbg1)

Coverage build: natural spec `tools/cov/specs/isbg.py` with driver `isbg.con`; forcing spec
`isbg_force.py` with `isbg_stun.con`:

    BUILD=$PWD/build-port-cov SECS=1800 timeout 1900 tools/cov/run.sh tools/cov/specs/isbg.py <c> \
      1baa96c2,06_cliff_spiderballguardian --con tools/cov/specs/isbg.con --fast --frames 6000
    BUILD=$PWD/build-port-cov SECS=1800 timeout 1900 tools/cov/force.sh tools/cov/specs/isbg_force.py <f> \
      1baa96c2,06_cliff_spiderballguardian --con tools/cov/specs/isbg_stun.con --fast --frames 8000
    python3 tools/cov/gcov_report.py build-port-cov <c>/gcov <f>/gcov --module IngSpiderballGuardian

`--frames` must be greater than the driver's last `@f`; otherwise con.py's batch hits EOF and nothing is sent.
Console `shoot` projectiles do real damage, so they arrive as kSM_Damage and ApplyStunDamage never runs.
The force spec therefore rewrites kSM_Damage to kSM_ResistedDamage in AcceptScriptMsg. It also drops
mStunHealth to 0.1 on hits 4 mod 10, which reaches the stun, its recovery and the next phase.

The merged run covers every arm of Think, Dead, ApplyStunDamage and GetPhaseProperties. The other marked
bodies are branchless and covered.

Open arms (cpp line:taken/total), so these bodies stay unmarked:

| function | open arms |
|---|---|
| ctor | 112:1/2 |
| AcceptScriptMsg | 141:0/2 (Deactivate with no roll sfx), 175:1/2 (IM4 while not stunned/charging) |
| DoUserAnimEvent | 197:0/2 |
| Death | 236:1/2 |
| Patrol | 267, 268 (target already set / previous waypoint set) |
| DamageReaction | 286:3/4, 308, 312 |
| StunnedReaction | 321:3/4, 343:1/2, 347:1/2, 351:1/2 |
| SelectNextWaypoint | most arms (waypoint graph branches) |
| MoveAlongWaypoints | 471, 489, 491, 499, 500 |
| ApplyProximityDamage | 511:3/6 |
| UpdateEffects | 526, 527, 534-547 |
| UpdateStateTimers | 557, 564 |
| UpdateRollSound | 579:0/3 (stunned roll sound) |
| SetGuardianState | 606:4/6, 610:4/5 (610's default case is unreachable) |
| LoadIngSpiderballGuardian | 700:1/2 |

Not driven: the loader typedef readers, the dtors and __sinit (they run, but nobody checked them
arm by arm), and IsListening.

## Smoke family (proposal)

`isbg`: area `1baa96c2,06_cliff_spiderballguardian`, driver `tools/cov/specs/isbg.con` (god, `layer 13`,
Activate at f1000, then damage, IM messages, puzzle sections, kill, Delete; about 5500 frames). Expect the
guardian in `objs IngSpiderball`, fsm Patrol, then Dead after the kill. The pass condition is 0
`[port] caught` / `reach-stub`.

## Coverage (isbg2: the 10 rows still unmarked)

At 69b3951df the module has 43 B rows and **33** of them carry a `// @ fn_35_*` marker, so 10 rows
were left, not 14 (the earlier count predates four markers). All ten are loader/getter/implicit
bodies, not boss behaviour: the boss state machine (Think, Patrol, DamageReaction, StunnedReaction,
SelectNextWaypoint, SetGuardianState, ...) is already marked, so the `fsm`/`damage`/`send` and
force-recipes above have nothing left to reach on this lane's rows - their arms are property-stream
cases and compiler-generated bodies.

One natural run at this commit (`isbg.py` + `isbg.con`, 6000 f, `caught/reach-stub: 0`) exercises all
ten (the loaders run when e0014007a loads).

Marked (8):

| fn | evidence |
|---|---|
| 0 IsListening | objdiff 100%; `li r3,0x1` / `blr`; gcov 1/1 |
| 7F0 ~SLdrIngSpiderballGuardian | objdiff 100%; 0x818 the Data dtor, then ActorParameters, PatternedAITypedef, EditorProperties, 0x844 delete flag. gcov 1/1 |
| E60 ~SLdrIngSpiderballGuardianStruct | objdiff 100%; empty dtor = only the 0xe74 null-this and 0xe7c delete-flag guards. gcov 1/1 |
| 86C LoadTypedefIngSpiderballGuardianData | objdiff 100%; the compare tree is the Data ids (0x8c0 0xf598739, 0x8d8 0xc463268c, 0x8ec 0xaed23abc, 0x900 0x8c2fbb19, 0x914 0xba78d281, 0x928 0xd5f3e9c4, 0x93c 0xcee38f10, 0x950 0xfc58adff, 0x964 0x3a5e2f52 ...). gcov 52/55 lines; the only untaken arm is the `default: ReadBytes` for an unknown id, which shipped data cannot send |
| CC4 LoadTypedefIngSpiderballGuardianStruct | objdiff 100%; the 8 field ids of the Struct. gcov 31/34; same unknown-id default untaken |
| B04 ~SLdrIngSpiderballGuardianData | objdiff 68.97% - **codegen only**: 0xb2c-0xb68 the six SLdrAudioPlaybackParms, 0xb74 SLdrDamageInfo, 0xb80-0xbbc the six Struct dtors = our reverse declaration order. gcov 1/1 |
| BEC Data ctor | objdiff None - **codegen only**: GCC inlines the Struct ctor MWCC kept out of line (our object has no `__ct__31SLdrIngSpiderballGuardianStructFv`). Retail: 6x struct ctor (0xc00-0xc28), SLdrDamageInfo 0xc30, 6x SLdrAudioPlaybackParms 0xc38-0xc60, then 0xc0 = 2.0f (0xc84), 0xc4 = 0xb (0xc94), 0xc8 = 40 (0xca0), 0xd0 = 10 (0xca8), 0xd4 = 20 (0xcac) = the header's order and defaults. gcov 20/20 |
| E9C Struct ctor | objdiff None, same inlining; retail stores 5/10/7/720/40/2/25/15 at 0x0..0x1c (0xeb4-0xf14, rodata_48/40/4c/50/3c/34/54/58) = the header's eight defaults in declaration order. gcov: the eight store lines each ran once (inlined into the Data ctor) |

Left unmarked (2, both 100% objdiff but with no source line to attach a marker to, exactly like the
other modules' __sinit rows):

| fn | why |
|---|---|
| 3D1C SLdrIngSpiderballGuardianData (copy) | implicit copy ctor, compiler-generated from the struct definition |
| 406C __sinit_CIngSpiderballGuardian_cpp | compiler-generated from the trigger/state/code tables |

## Coverage evidence (lane isg3, 2026-10-25)

Natural fight run `/tmp/opencode/isg3-nat/out` (spec `isbg.py`+`isbg.con`) plus forced run
`/tmp/opencode/isg3-force/out` (`tools/cov/specs/isg3_force.py`, `force_ret.py`; single-call writes restored
at return, FORCE_INNER guardian positions not restored); both on the coverage binary sha256
`abc9f7b0afe58f42d93a2596a60662e1878594c28fb121997e0e6e4de89b821d`, frozen in `/tmp/opencode/isg3-frozen/`.
`gcov_report.py` merged: fn_35_303C 14/14 lines 12/12 branches, fn_35_19D0 30/30 and 14/14, fn_35_1878 10/10
and 8/8, all ALL-ARMS. fn_35_178: the only open edge (752, `return nullptr`) is dead by data
(`tools/scan_isg_loader.py`: one ISBG placement, ancs 0x4e6c079a is ANCS). fn_35_3D1C: layout proof in
`SLdrIngSpiderballGuardian.hpp`.
