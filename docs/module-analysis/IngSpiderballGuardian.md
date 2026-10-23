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
