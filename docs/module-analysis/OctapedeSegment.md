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
| 1D64 Cross | - | - | deferred then; header-inline gcov evidence and marker established by oct3 below |
| D28/DA4 SLdrOctapedeSegment dtor and ctor | - | - | deferred then; header-inline bodies are markable (oct3 below); defaults are oct1's fix |

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

## Fourth pass (oct3, 2026-10-25): all six remaining behaviour rows marked

Base `92bcb4775`. Markers only in C++; the bodies retain oct1's retail-equivalent implementation.
The earlier forcing spec's `kSegS_Bouncing = 2` was wrong: the enum is **1** (Landed is 2),
`include/MetroidPrime/Enemies/COctapedeSegment.hpp:17-21`. No behaviour fix was necessary.
The TSV also synchronizes the four markers already present at base (1014, 11FC, 1318, 278C).

Evidence: `/tmp/opencode/oct3-evidence/` contains the executed binary, all gcno files, three
isolated gcda trees, exact executed source/specs/drivers, logs, retail/host disassembly and
`SHA256SUMS`. All line numbers in the following arm table refer to **executed-source**, before
the new marker blocks. PIE-relative sites were dry-run on that exact binary (`oct3-sites.txt`).
Writes restore on the **same call's return**, including list and area storage. No return values,
branch instructions or coverage counters are patched.

Counts below exclude gcov's `throw: true` exception-unwind edges.

| row | evidence / remaining compiler edge | result |
|---|---|---|
| 2924 Think | 58/58 lines, **72/72 edges**; natural + force1 + force2 | marked |
| 30BC AcceptScriptMsg | 43/43 lines, **46/46 edges**; natural + force1 + force2 | marked |
| 14E0 CollidedWith | 26/26 lines, **25/26 edges**; remaining static-guard contention edge is compiler-only (proof below) | marked |
| 1D64 Cross | header gcov: **43 executions**, 1/1 block, zero branches; Death:454 = 13, Bounce:539 = 30 in natural | marked at actual inline body |
| DA4 SLdr ctor | header gcov: **16 executions** in natural; four untaken edges explicitly `throw: true`, no untaken normal branch | marked at actual inline body |
| D28 SLdr dtor | header gcov: **16 executions**, 5/5 blocks, zero branches | marked at actual inline body |

| open arm from oct1/oct2 | how oct3 exercised it |
|---|---|
| Think inactive, 308/309 area accessor / hidden area | forced (`oct3_force.py`, `Think entry`): clear active, or write the loaded area's `mPostConst->mOcclusionState = 0`; separately `mPhase = 0` with zero bounces. The latter returns before any base/world update and restores the phase immediately. No invalid area id. |
| Think 313 obstruction after the tick's clear | forced (`Think late obstruction`, Think+1687): set `mPlayerObstructed` immediately before its load on the visible, zero-bounce path. |
| Think 324 align/bouncing, 348 missing-head suppression, 356 already-broken dead head | forced (`Think after base`, Think+208): state 1 vs 0, align false; missing head 0x7ffe with state 1 or broken=true; invalid head + HP=0 + state 0 + broken=true. |
| Think 340 and 364 frozen-death alternatives | forced (`Think after base`): self-head + HP=0 + frozen; invalid head + state 1 + frozen. |
| Think 371 null electric / not alive | forced (`Think electric tail`, Think+635): valid followed id with electric pointer null, or non-null electric with `mAlive = false`. |
| AcceptScriptMsg 236/239 cast failures | forced (`AreaLoaded invalid target`, Accept+525): change one APRC/Action and one APRC/Activate connection's target editor id to 0xffffffff **before its load**; restore after AreaLoaded. |
| AcceptScriptMsg 248 head=self / follower with children | forced (`AreaLoaded head/children`, Accept+764): self head id; separately populate one already-reserved child slot and count=1 on a follower, reaching `clear()`. |
| AcceptScriptMsg 255, 281 | `oct3_messages.con`: `send u0483 Create` on active controller; Landed messages with forced states 1/1/0 and bounce budgets 2/0/0. |
| CollidedWith whole segment, empty list, non-mask, exhausted bounces | forced (`Collision inputs`): real collision callback with broken=false; then invalid counterpart + list count=0; then zero material masks; then floor mask + zero bounces. List storage and ids restore before caller resumes. |
| CollidedWith segment/player/other actor | forced (`Collision inputs`): real callback counterpart changed to self segment, player u0400, then a real **CGameLight u0401** selected from the actor-qualified list; actor-only arm reaches Bounce with counts=false. |

**Static guard proof.** The sole untaken `571:3/4` is host CollidedWith+176, the zero return
from `__cxa_guard_acquire` **after** its first guard-byte test already saw zero (host disassembly
`oct3-collision-dis.txt:20-60`). It is GCC's concurrent-initialization double check, not a retail
arm: retail 0x1504/0x1508/0x150C tests one byte and branches directly over initialization;
0x165C/0x166C writes that byte once. Both first-use and already-initialized retail arms ran.
The only writer is this function-local static initializer (`COctapedeSegment.cpp:579-580`);
its `CMaterialList` constructor (`include/Collision/CMaterialList.hpp:107-115`) only ORs six
bits, with no callback/re-entry. Gameplay invokes collision callbacks synchronously via
`src/MetroidPrime/CGameCollision.cpp:717-727`; no second game-update thread initializes this
static between its two host checks. This compiler-generated contention edge is excluded,
like the ctor's EH-only edges, rather than claimed executed.

**Cross identity.** `include/Kyoto/Math/CVector3f.hpp:63-68` is the actual host body inlined in
the two corresponding callers (retail Bounce call 0x1ADC; Death call 0x24F0). Retail 0x1D74/0x1D84/0x1D90
computes/stores `ay*bz - by*az`, 0x1D80/0x1D8C/0x1D98 `az*bx - bz*ax`, and
0x1D88/0x1D94/0x1D9C `ax*by - bx*ay`. Header lines 64-66 are those expressions; the helper
has no conditional arm. The marker belongs on that header definition, not a duplicate wrapper.

**Loader identity.** Header ctor `SLdrOctapedeSegment.hpp:58` invokes editor/patterned/actor/damage
members in retail order (0xDB8, 0xDC0, 0xDC8, 0xDD8). Retail 0xDD4 gives the invalid effect id;
0xDE4/0xDF4/0xE04/0xE14/0xE24 give the five -1 sounds. Oct1's corrected resistance/ambient/visor
stores correspond to 0xEC8 (+0x8C, 2), 0xEEC (+0x298, white), 0xEFC (+0x2E8, 0xF).
The remaining patterned overrides (0xE4C..0xED4) and segment defaults (0xF14..0xFFC) agree with
the header: movement defaults, zero break-apart/run ranges, turn intervals 1/2, bounces 1/1,
bounciness .75, explosion weapon 9/damage 5/knockback 2. Oct1 already audited these constants.
The empty explicit dtor at header:133 still performs implicit reverse member teardown:
damage +0x364 (0xD50), actor +0x284 (0xD5C), patterned +0x40 (0xD68), editor +0 (0xD74).
Retail's null-this check (0xD44) and deleting-destructor flag (0xD78..0xD84) are ABI scaffolding:
the loader's only instance is its non-null **stack local**, `COctapedeSegment.cpp:691`, destroyed
automatically on exit. No heap/delete caller exists for this loader struct.

Reproduction (use `E=/tmp/opencode/oct3-evidence`, `BUILD=$E/frozen-build`, `MP2_HEADLESS=1`;
run `tools/games_running.sh --wait` before each launch):

```sh
tools/cov/run.sh "$E/natural/executed-spec.py" <new-natural> 1baa96c2,0p_cliff_hall \
  --con "$E/natural/executed-driver.con" --fast --frames 3000
tools/cov/force.sh "$E/force1/executed-spec.py" <new-force1> 1baa96c2,0p_cliff_hall \
  --con "$E/force1/executed-driver.con" --fast --frames 2600
tools/cov/force.sh "$E/force2/executed-spec.py" <new-force2> 1baa96c2,0p_cliff_hall \
  --con "$E/force2/executed-driver.con" --fast --frames 1200
```

All three exited normally, frame targets 3000/2600/1200, caught/reach-stub **0**. Link build
`metroid_prime2_port`: 0 FAILED; gen-files check rc 0; TARGET_PC marker-block strip gives exact
base text in all three C++ files; matching build **87 files OK**, game-code matched **63.29%**.
Review: six MARK hits, each accounted for above, no behavior-code edits or probes. No wider
module/area sweep: only this slice and its callers' 0p_cliff_hall area were exercised.
