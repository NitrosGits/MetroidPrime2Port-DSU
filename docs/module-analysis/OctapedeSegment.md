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

## Second pass (oct1, 2026-10-24)

`tools/cov/specs/oct1_force.py` on the oct1 natural driver (`octapede_cliffhall.con`), merged with the
oct1 runs (`gcov_report.py build-port-cov /tmp/opencode/oct_{a,b,c}/gcov --module OctapedeSegment`).
Every write is one call, restored at the next entry.

| body | arms | forced | still open |
|---|---|---|---|
| 30BC AcceptScriptMsg | 42/46 | 0x281's bouncing early return (`mSegmentState = kSegS_Bouncing`, `mBouncesRemaining = 2`) | 236:3/4 and 239:3/4 (the AreaLoaded connection casts: the `connMsg == kSM_Action` and cast-null arms, decided by the shipped APRC connections), 248:1/4 and 248:2/4 (`mHeadSegmentId != GetUniqueId() && mChildSegmentIds.size() > 0` - the head-self write did not move them) |
| 14E0 CollidedWith | 22/26 | 0x572's `if (mBrokenApart)` false arm and 0x585's `... && mBouncesRemaining > 0` false arm | 571:3/4 (the function-static bounce mask's guard), 582:1/2 (`list.GetCount() > 0` false: a collision with an empty list), 584:1/2, 585:1/4 |
| 2924 Think | 62/72 | nothing landed: `mBouncesRemaining = 0` with `mPlayerObstructed` set/clear did not reach 0x312/0x313 (they read the tick's own values) | 308:1/2, 309:1/4, 309:2/4, 313:0/2 (the occlusion-state arms: `area.GetOcclusionState() != kOS_Visible`), 324:5/6, 348:1/4, 348:2/4, 356:3/4, 371:3/6, 371:5/6 |
| 1D64 Cross | - | - | the inlined `CVector3f::Cross` helper: no standalone instrumented body, so no coverage evidence and no marker |
| D28/DA4 SLdrOctapedeSegment dtor and ctor | - | - | inline in the header; the ctor's retail defaults are oct1's fix |

## Third pass (oct2, 2026-10-24): FORCE_INNER does not apply here

`tools/cov/specs/oct2_force.py` (run with `COV_GDB_SCRIPT=tools/cov/force_ret.py`) targets the oct1
table's open arms through the inlined callees' answers. The run reported `no symbol` for every
FORCE_INNER entry, and the binary confirms why: the coverage build has **no** standalone symbol for
`CGameArea::GetOcclusionState`, `CCollisionInfoList::GetCount` or `TCastToConstPtr<COctapedeSegment>`
(`nm` on build-port-cov has zero `TCastToConstPtr` symbols and no `GetOcclusionState`), so there is no
return to hook - all three are inlined into their callers in this build.

The routes that remain for the oct1 arms:

- Think 308/309/313 (occlusion): write the *area's* occlusion member from the Think entry (the area
  comes from `mgr.GetWorld()->GetAreaAlways(GetCurrentAreaId())`, an inferior call or a hand-walk of
  the world's area array), or force `mPlayerObstructed` after line 311 - the oct1 attempt set it at the
  entry and line 307 cleared it again, so it must be written after that line (`loc:` on 310 or 313).
- Think 324/348/356/371: member writes (`mSegmentState`, `mBrokenApart`, `mAlignToFloor`,
  `mFollowedSegmentId`, `mElectric`) at a `loc:` after 307, plus the health value behind
  `GetHealthInfo()->GetHP()` (inline) written through the health member.
- CollidedWith 582/584/585: a real collision with an empty `CCollisionInfoList` (the list is built by
  the collision system, not by a member), and 571:3/4 is the function-static bounce mask's guard.
- AcceptScriptMsg 236/239/248: the cast is inlined (no symbol); 248 needs a follower with Action
  children (the shipped heads have them, the followers not) or a `loc:` write of `mChildSegmentIds`
  after the `reserve(16)`/loop so `clear()` is safe.

The forced run also hit the render watchdog ("no frame for 5.0 s (frame 200)"), so it produced no gcov
data; the FORCE_RET writes it did make are the oct1 ones.
