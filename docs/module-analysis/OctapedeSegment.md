# OctapedeSegment REL (module 46): analysis notes

COctapedeSegment (OCTS, in-game "Octapede"): a CWallCrawler segment. Chains of segments follow a head
along walls (Patrol); killing a segment breaks the chain apart (Death: bounce away with spin, then
Dead -> run around -> Explode), and the head is found again with FindHeadSegment. The module's code is
upstream's `src/MetroidPrime/Enemies/COctapedeSegment.cpp` with the loader `SLdrOctapedeSegment.hpp/.inc`.
Retail asm: `build/G2ME01/OctapedeSegment/asm/MetroidPrime/Enemies/COctapedeSegment.s`. Per-function
table: `OctapedeSegment_functions.tsv` (B upstream body, H weak/implicit copy, T REL runtime).

## Survey (oct1, 2026-10-23)

`tools/upstream_hollow.py OctapedeSegment`: nothing hollow. Every sub-100% body was diffed against the
retail asm (registers and stack offsets normalized; rodata constants compared by value).

| function(s) | verdict |
|---|---|
| SLdrOctapedeSegment ctor fn_46_DA4 (44%) | **fixed**: knock-back resistance 2.0 at +0x8C (0x108/0x124, rodata 0x28), ambient colour (1,1,1,1) at +0x298 (0x134/0x148), visor flags 0xF at +0x2E8 (0x14C/0x158), the five sound ids -1 (0x38 `li r0,-0x1`; 0x40..0x80); upstream had 0. Same pattern as WispTentacle (a1648c13e) |
| Patrol 96.4, AcceptScriptMsg 96.0, Think 99.95, Death 96.65, Bounce 99.8, CollidedWith 99.6, Load 99.8, Cross 92 | scheduling, extra temporaries, `fmadds+fneg` vs `fnmadds`, one extra `clrlwi`; equivalent |
| `retail_diff.py` call/const findings | false positives: inlined SLdr ctor/reader (DA4, 9E0), Cross (1D64), `DeliverScriptMsg` wrapper, `CRelAngle::FromDegrees` (0.017453292) |

## Shipped data (disc scan)

16 placements each in Sanctuary 0b_cliff_hall L2 and 0p_cliff_hall L0. 0p_cliff_hall: four chains of
four, heads u0483 (followers u0467 <- u047e <- u0474), u0480 (u046b <- u046f <- u0487), u0450, u0442.
A follower links to the segment it follows by an APRC -> Activate connection; heads hold APRC -> Action
connections to their children.

## Coverage (oct1)

Coverage build, spec `tools/cov/specs/octapedesegment.py`, drivers `tools/drivers/octapede_cliffhall.con`
(natural) and `tools/drivers/octapede_forced.con` (forced), god mode:

    BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/octapedesegment.py <out> 1baa96c2,0p_cliff_hall \
      --con tools/drivers/octapede_cliffhall.con --fast --frames 3000     # forced: octapede_forced.con, 2600
    python3 tools/cov/gcov_report.py build-port-cov <out>/gcov --module OctapedeSegment

| arm | how |
|---|---|
| no between-segments effect (ctor), no waypoint, no children, zero break-apart velocity | forced (`spawn OCTS ancs u0487 prop 0x1b58c5f1 0xffffffff prop 0x96b863c5 3` -> u048e) |
| Load null-model arm | forced (`spawn OCTS` without ancs; loader returns null) |
| Patrol Activate/Update/Deactivate on head and spawned segment | forced (`statecall <id> Patrol 0/1/2`) |
| Dead not-broken-apart arm, Explode deleting a chain member (null child/front) | forced (`fsm u0467 Dead`) |
| Death, Bounce, Freeze, Dead natural arms | `shoot`/`damage`/`kill` on chain members, `beam dark` |
| Create/Activate/Deactivate/Landed messages | forced (`send <id> <msg>`) |

Dead or data-only arms, not forced: Patrol's switch default (EStateMsg has three values) and its
null `nearest` (same id looked up again in the tick); Explode's cast fail (every DeGenerate -> Activate
target in 0p is a CScriptEffect; 0b not checked); FindHeadSegment/AcceptScriptMsg cast fails; the
AcceptScriptMsg head==self arm (no follower has Action children); DoUserAnimEvent's default (only
SoundPlay events ship); Burn's burn-disabled arm. The ctor's untaken arms are throw/EH only.

Marked (17): ctor, dtor, Patrol, SetupStateMachine, PreThink, Render, PreRender, AddToRenderer, Death,
Dead, Bounce, ThinkAboutMove, Freeze, the loader, SetFuncPtrs, RELMain, RELExit. Unmarked: Think
(occlusion arms 303/304/308; `setarea` to force them crashes in CActor::UpdatePortalSystemState on an
unloaded area, console misuse), CollidedWith (list-count-0, live-segment, non-mask arms),
AcceptScriptMsg, FindHeadSegment, Explode, DoUserAnimEvent, Burn, the SLdr ctor/dtor (header).
