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
