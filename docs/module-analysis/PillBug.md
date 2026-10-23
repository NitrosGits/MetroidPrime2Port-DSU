# PillBug REL (module 48): analysis notes

CPillBug (PILB, "Morphology: Pillbug") is a wall/floor crawler that patrols a waypoint path, speeds up
(FastPatrol) when the player is in detection range, pauses after touching the player, and falls off
when bombed (Bombed -> Fall -> Landed -> Injured), then walks back to its path (ReturnToPath via IS00
return waypoints) or wanders until it is close to the path again. The module's code is upstream's
`src/MetroidPrime/Enemies/CPillBug.cpp` with the loader `include/MetroidPrime/ScriptLoader/SLdrPillBug.hpp`.
Retail asm: `build/G2ME01/PillBug/asm/MetroidPrime/Enemies/CPillBug.s`. Per-function table:
`PillBug_functions.tsv` (B upstream body, H weak/implicit copy, T REL runtime).

## Survey (pb1, 2026-10-23)

All 82 retail functions pair with upstream (77 in CPillBug.s, 5 in REL_Setup).

| function(s) | verdict |
|---|---|
| SLdrPillBug ctor fn_48_AA4 | **fixed**: retail sets knockback resistance 2.0 (rodata 0x0; 0xAE8 `lfs`, 0xAF4 `stfs f0,0x88`), ambient color CColor(1,1,1,1) (rodata 0x4; 0xB04 `bl CColor`, 0xB18 `stw r0,0x294`) and visorFlags 0xF (0xB1C `li r0,0xf`, 0xB28 `stw r0,0x2e4`); upstream left the SLdr defaults |
| asmcheck BIT findings (xa34_*/xa35_80) in the ctor, HitPlayer, Landed, AnimOver, PathOver, Bombed, Attacked, IsOnGround, FastPatrol | bitfield-name mapping only: the members `mAttacked..mReturningToPath` (CPillBug.hpp:97-105) are the 0xA34/0xA35 bits; the accessors are one-line returns |
| asmcheck CALL fn_48_3DA8 (ctor), fn_48_2A5C (GetContactDamage) | weak copies: CDamageVulnerability copy ctor and CDamageInfo ctor |
| asmcheck CALL FindReturnWaypoint 2B2C, PatrolImpl 2210, TCastToPtr<CPlayer>, CCollisionPrimitive ctor | defined upstream bodies / inlined casts; not divergences |
| asmcheck BRANCH counts in HasReturnPath, Patrol, FastPatrol | inlined-callee conditions (PatrolImpl, FindReturnWaypoint) |
| FindReturnWaypoint 2B2C | retail discards one chain step result (kept: retail quirk) |
| GetCurrentConstraint 2AEC | no callers in retail (0%) |

## Shipped data (disc scan)

Spawns: Agon 05_portalpuzzle L2, 0b_sand_hall, 0d_sand_hall, Sanctuary 06_cliff_spiderballguardian L8.
All 15 paths loop, so GetNextWaypoint's non-loop arms are data-dead, and the empty-mPath arms
(ReturnToPath 551, Wander 693) never run (the host would index an empty vector there; data-dead).

Dead arms, with the proof:

- `mPlayerObstructed` is never set in PillBug; its only reader is PreThink (`PortCWallCrawler.cpp:375`),
  so Think 140:0 is dead.
- planar is never 0 in shipped data (PatrolImpl 490:1, ReturnToPath 568:1).
- x:3 arms of the state bodies are EStateMsg defaults (no other message reaches them).

## Coverage (pb1)

Coverage build (`-B build-port-cov -DMP_PORT_COVERAGE=ON`), spec `tools/cov/specs/pillbug.py`, drivers:

    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/pillbug.py <c1> \
      42b935e4,0b_sand_hall --con tools/cov/specs/pillbug_sandhall.con --fast --frames 3000
    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/pillbug.py <c2> \
      42b935e4,0d_sand_hall --con tools/cov/specs/pillbug_sandhall2.con --fast --frames 3000
    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/pillbug.py <c4> \
      1baa96c2,06_cliff_spiderballguardian --con tools/cov/specs/pillbug_cliff.con --fast --frames 4500
    python3 tools/cov/gcov_report.py build-port-cov <c1>/gcov <c2>/gcov <c4>/gcov --module PillBug

`pillbug_cliff.con` bombs three bugs with real morph-ball bombs (f500, f1700, f2700; `frame` lines split
the `at` batches, the console queues at most 256) and has one forced line,
`send e0014042b ResistedDamage` at f1702. Do not use `damage` with power bombs: it removes the object.

Marked (16, every line and arm taken or the missing arm proven dead, asm-checked): ctor 3B2C,
CollidedWith 3458, GetContactDamage 2A1C, Patrol 21D4, FastPatrol 2014, HasReturnPath 2968, HitPlayer
2930, Landed 293C, AnimOver 2948, PathOver 295C, Bombed 2A04, Attacked 2A10, GetAimPosition 3564,
IsOnGround 358C, GetTouchBounds 3598, SetupStateMachine 3694.

Open arms (unmarked): AcceptScriptMsg 114:1, 116:1; Think 147:1/3; GetDamageVulnerability 161:0;
ThinkAboutMove 186:4; BuildPath 254:1, 261:1, 264:0, 277:1, 280:1; HasInnerPathLoop 296/301/308/314;
GetClosestPathIndex 330:0; FindReturnWaypoint 351:1, 353:1, 367:1; CloseToPath 431/439/446/453:0
(RaySphere hit); PatrolImpl 508:1; ReturnToPath 543/549/559/578; Fall 622:1, 623:1; Injured 642:0,
643:0; Turn 665:2; Wander 690:1, 692:1; Pause 746:2; LoadPillBug 777:1. The 549/622/642/690/692 arms
sit on lines with no source condition (compiler/inline guards), unresolved.
