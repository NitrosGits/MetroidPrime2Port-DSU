# StoneToad REL (module 77): analysis notes

CStoneToad ("Mechanoid: Watchdrone") is a CPatterned that sleeps on its start position. A safe zone touching it
(or a hurting projectile/bomb, via AcceptScriptMsg) provokes it: WakeUp → ReactToSafeZone / ReactToWeapon →
MoveAway to its first active waypoint → Wait; when un-provoked (Decrement / safe zone off) it does MoveBack →
GoToSleep → Sleep. It turns its head at the player with four additive aim animations (UpdateLookAt). The module's
code is upstream's `src/MetroidPrime/Enemies/CStoneToad.cpp`, with the loader
`include/MetroidPrime/ScriptLoader/SLdrStoneToad.hpp`. Retail asm:
`build/G2ME01/StoneToad/asm/MetroidPrime/Enemies/CStoneToad.s` plus `REL/REL_Setup.s` (90 fns in total).
Per-function table: `StoneToad_functions.tsv` (B upstream body, H weak/template copy or `__sinit`, T REL runtime).

The only spawn area is Torvus Bog `09_swamp_catacombs_dark` (world 3dfd2249, area 65). It has two toads:
e00410048 (-46.1, -197.3, -45.0) and e00410066 (-22.3, -184.7, -45.1), both hp 5 and asleep, and the safe zone
e00410043 that provokes e00410048.

## Survey (st1, 2026-10-23)

| function | verdict |
|---|---|
| SLdrStoneToad ctor (inlined in LoadStoneToad, fn_77_4C8) | **fixed** (TARGET_PC, SLdrStoneToad.hpp): retail sets three defaults upstream left at the base values. Knockback resistance 2.0 (0x510 `lfs` rodata_0 / 0x528 `stfs 0x480(r1)`); ambient (1,1,1,1) (0x52C `bl __ct__6CColorFffff` / 0x548 `stw 0x68c(r1)`); visorFlags 0xf (0x534 `li r0,0xf` / 0x538 `stw 0x6dc(r1)`). No echo emitter or sound ids in this SLdr |
| every other body | equivalent to the asm (AcceptScriptMsg's `WeaponHits` import has upstream's `WeaponHurts(const CWeaponMode&)` signature) |

## Coverage (st1)

Spec `tools/cov/specs/stonetoad.py` (breakpoints on the states, Touch, AcceptScriptMsg, render hooks); driver
`tools/cov/specs/stonetoad.con`:

    BUILD=$PWD/build-port-cov SECS=1500 tools/cov/run.sh tools/cov/specs/stonetoad.py /tmp/opencode/st1_a \
      3dfd2249,09_swamp_catacombs_dark --con tools/cov/specs/stonetoad.con --frames 1500
    python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/st1_a/gcov --module StoneToad

The driver adds god mode and the light beam, activates safe zone e00410043 (e00410048: Touch → ReactToSafeZone →
WakeUp → MoveAway → Wait), damages and shoots e00410066, deactivates the safe zone and Decrements e00410048
(MoveBack → GoToSleep → Sleep), sends Increment/Decrement/AIUpdateDisabled/Deactivate/Activate to e00410066 and
kills it. 0 `[port] caught` / `reach-stub`, exit 0.

Marked (20, every live arm ran): GetCollisionResponseType, Touch, Sleep, MoveAway, Wait, MoveBack, the triggers
HitBySafeZone/MoveBackFinished/ShouldMoveAway/MoveAwayFinished/ShouldPatrol/AnimOver, PreThink, Render, PreRender,
AddToRenderer, SetupStateMachine, SetFuncPtrs, RELMain, RELExit.

Not reached: the weapon provoke. Beam, charged, missile, morph-ball bomb and power bomb hits on e00410066 never made
`WeaponHurts` true (only console `damage ... bomb` took hp 5→4, without a projectile), so AcceptScriptMsg's weapon
arm (cpp:215-226), ReactToWeapon and HitByWeapon never ran. Likely needs a weapon that the toad's vulnerability
accepts (a later beam/annihilator) or a forced spec.

Open arms (cpp line), so these bodies stay unmarked:

| function | open arms |
|---|---|
| AcceptScriptMsg 22F4 | weapon arm 215, 222-226; 129, 137, 143, 144, 166, 194, 241 3/4 (connection-state arms) |
| ReactToWeapon 1A48, HitByWeapon 1478 | never ran |
| WakeUp 1EC0 | 282 3/4 (default, dead), 291 0/2 (timing), 298/301 (no active waypoint, data) |
| ReactToSafeZone 1C84 | 312 3/4 (default, dead), 320 0/2 (timing) |
| GoToSleep 158C | 397 3/4 (default, dead), 405 0/2 (timing) |
| Think 21E4 | 254 1/2 (no collision actor manager) |
| ShouldMoveBack 14C8 | 432 1/2 |
| GetMoveDirection 10B0 | 479 (default, dead), 481 (zero away position), 490 1/2 |
| IsMovingAway/BackToStart 12D4/11DC | 461/471 1/2 (exactly at the target, data) |
| ctor 2F74 | 85/92/99/106 (missing look animation, data) |
| UpdateLookAt B48 | `look.GetX() >= 0` 1/2 (player never below the eye in the drive); ApproachWeight 497 1/2 is the inlined `0.f > 0.f` copy, constant false |
| LoadStoneToad 4C8 | 555 (no model data) |

Not driven arm by arm: the SLdr ctor and dtor, the implicit dtor, `__sinit` and the weak copies (H).

## Smoke family (proposal)

`stod`: area `3dfd2249,09_swamp_catacombs_dark`, driver `tools/cov/specs/stonetoad.con`, 1500 frames, not fast.
Expect `objs StoneToad` to end with only e00410048. The pass condition is 0 `[port] caught` / `reach-stub`.

## asmcheck

`tools/asmcheck.py check StoneToad src/MetroidPrime/Enemies/CStoneToad.cpp`: 0 findings over the 20 markers
(the bitfields carry their `0x860` masks in CStoneToad.hpp).

## st2 closure (2026-10-24)

Base `d513df406c4fef0bcb119da2b42c04254c8bbaf5`, not stale `port`. The st1 open-arm
list above is historical: other batches had already marked those bodies. Actual `// @`
grep before st2 left exactly `fn_77_0`, `4C8`, `B48`, `22F4`; all four are now marked.
All **33 B bodies** have actual markers (35 total including RELMain/RELExit). No body,
constant, branch direction, mask or call target changed in st2: comments/markers and
one indentation-only line (full-file C++ token identity verified).

### Arm evidence

Final runs use the final comment-only sources and their matching `.gcno` files:

| body / retail range | arms and outcome |
|---|---|
| destructor `0..84` | Both native deleting and non-deleting destructor variants executed. Header gcov: both variants have every block executed and no body branches. GDB records both entries with the same live collision manager. Retail `0x34..54` destroys the manager, waypoint vector, then base; native member destruction has the same order. Retail null-this (`0x18..1C`) and member-address (`0x24..30`) checks are compiler scaffolding: a valid destructor invocation has non-null this and nonzero `this+0x85c`. Retail positive delete-flag path is the native deleting destructor; the non-deleting base/member form is its actual callee. |
| loader `4C8..788` | **10/10 lines, 10/10 branches.** Shipped property parsing/normal construction plus **forced** (`stonetoad_st2_loader.py`) absent-model return. The second actual loader call clears `modelData.m_valid` immediately before the native comparison. The first actual instance remains usable and is destroyed normally. |
| look `B48..1060` | **24/24 lines, 5/5 branches**, plus inlined `ApproachWeight` **9/9 lines, 4/4 branches**. Ten near/far/front/back player-position fixtures, six weights, each repeated across all four exact normalized axes. Axes and periodic zero-angle answers are **forced** (`stonetoad_st2.py`); transforms/weights are restored after each scenario. Both ordered sign decisions, positive/zero target, rising/falling and both clamp outcomes executed. |
| look branchless selects | gcov omits SSE min/max/select conditions. The spec discovers all **17 native `minss`/`maxss`/`cmpltss` sites** in UpdateLookAt and records their input comparisons. **Both outcomes at every site**, including angle saturation, each rising/falling clamp, all inactive-axis decays and the negative-side zero-target copies: `/tmp/opencode/st2-complete2/st2-selects.log`. This is operand evidence before the actual instruction, not merely a claimed source arm. |
| messages `22F4..2BA0` | **101/101 lines, 104/104 branches**. Natural driver plus **forced** native queue covers all subconditions below. Thirteen independent field oracles passed (12 weapon outcomes plus actual base Activate). |
| `2404..2498` AreaLoaded | Shipped Approach/other connections and normal waypoint resolution; **forced** null waypoint cast and empty connection list. |
| `24A0..24C4` Create body | Natural inactive-body activation and **forced** Create on the already active controller. |
| `24D8..2508` model guards | Natural valid model/animation/spatial primitive; **forced** null model, null animation and invalid optional spatial primitive, one at a time. |
| `2544..2938` Create loops | Natural nonempty sphere/actor loops and material/filter/health setup; **forced** `ObjectById` null results exercise actor-skip. This scenario saves the original manager, creates a separate real manager, destroys its new actor ids, destructs/frees it and restores the original manager. No orphan fixture manager or production probe remains. |
| `2960..298C` messages | Natural AIUpdateDisabled/Deactivate/Delete (including intentional Delete fallthrough); **forced** missing manager for AIUpdateDisabled. |
| `2994..29E4` Increment/Decrement | Natural driver, both flag stores and Attack send. |
| `29EC..2A20` sender | Invalid sender and real collision-actor sender; latter resolves its actual last-touched id. |
| `2A24..2AF8` weapons | **Forced** each projectile/bomb/power-bomb typed-cast answer and both WeaponHurts results; null casts/short-circuit paths also run. Fixtures are calloc-backed typed storage used only by the inline current-damage-info read, never virtual calls/destruction. Cast/WeaponHurts return stubs are temporary gdb breakpoints, not committed game changes; no claim that the shipped vulnerability accepts these weapons. |
| `2AFC..2B48` weapon reaction | All 12 weapon fixtures check native `provoked`, `returnRequested`, `provoker` stores against the expected true/false result. |
| `2B4C..2B88` tail | Natural unconditional base call and changed/unchanged active synchronization with a manager; **forced** actual base Activate with a temporarily null manager covers the remaining conjunction. Base is stubbed only in isolated artificial message fixtures, not in this tail test or natural driver. Alert/HitObject/default also run. |

### Retail structural check

- Destructor: implicit member/base destructors and native D0/D2 split account for retail
  vector/manager/base cleanup and delete flag; no explicit port body is missing.
- Loader: `0x4EC..548` SLdr construction/defaults (st1 fix), `0x54C..600` included
  property switch, `0x604..648` optional model validity, `0x688..718` allocation and all
  constructor conversions, then ordinary RAII cleanup. Host `sizeof` replaces retail 0x868.
- Look: inverse/local Eye/player aim, normalized direction (`0xB7C..C70`);
  57.295776 degrees/radian and 60-degree clamp (`0xC74..CC0`); Up/Left normalized
  projection (`0xCC4..D0C`); ordered >= tests (`0xD24..2C`, `0xEB0..B8`). Four active
  smoothing expansions plus inactive-axis decay account for retail's 27 comparisons;
  the helper and branchless native selects explain the smaller source/gcov branch count.
- Messages: all retail cases, collision-manager ownership, loop tails and unconditional
  base/tail order are preserved. `0x2980` Delete fallthrough is intentional. Virtual
  collision response at vtable byte offset `0x60` (PPC slot 24) explains asmcheck's
  XCALL; `0x2888/0x28A8` are health/vulnerability virtual slots. `0x2680..26EC` are
  shifted material enum masks; `0x27EC` excludes Character (bit 33). Flag masks are
  0x80/0x40 at 0x860, provoker values 0 (safe zone) and 1 (weapon).

Full asmcheck RC **1**, 29 heuristic findings across 32 already-marked functions;
correctness-only MASK/XBIT/CROP/XCMP/XSTORE/OFFS RC **0**, zero findings. Relevant
full-check findings are compiler/template expansion, not missing bodies: message
`3608/36AC` reserves, `2D90` string concatenation, `2C74` joint copy, `2BF4` push,
`2BA0` health copy, `2DEC` vector destruction; string dereference is RAII. Its
`WeaponHits` import spelling corresponds to upstream `WeaponHurts(CWeaponMode const&)`
(also recorded by st1). Loader constructors/default color and included property loaders,
ReadBytes and optional copy (`788`) are present via headers/includes. Look's inline helper
explains BRANCH and string RAII explains CALL. Remaining ctor/CTryCommand findings are
pre-existing outside these four bodies. No heuristic finding was silently treated as RC 0.

### Reproduce / retained evidence

```
COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py BUILD=$PWD/build-port-cov SECS=600 \
  tools/cov/force.sh tools/cov/specs/stonetoad_st2.py <new-main-dir> \
  3dfd2249,09_swamp_catacombs_dark --con tools/cov/specs/stonetoad.con --fast --frames 1400
COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py BUILD=$PWD/build-port-cov SECS=600 \
  tools/cov/force.sh tools/cov/specs/stonetoad_st2_loader.py <new-loader-dir> \
  3dfd2249,09_swamp_catacombs_dark --fast --frames 600
python3 tools/cov/gcov_report.py build-port-cov <new-main-dir>/gcov <new-loader-dir>/gcov --module StoneToad
```

Run `tools/games_running.sh --wait` before each launch. The loader spec asserts this
coverage build's validity-test instruction at loader+484 before installing its breakpoint;
re-derive this offset if a different compiler changes codegen. Its forced absent model
intentionally suppresses one temporary model's cleanup (test-process-only leak). The main
spec's shared force_ret emits expected `no exit hook ... dummy frame` messages during
inferior calls: the enclosing real UpdateLookAt frame still owns the inner hooks; their
actual execution is evidenced by select counts. No set/call/restore failures in final runs.

Final runtime: `/tmp/opencode/st2-complete2` (1400 frames, 85 native scenarios, 13 store
oracles) and `/tmp/opencode/st2-final-loader` (600 frames, both destructor variants), both
normal inferior exits and zero caught/reach-stub. Summary `/tmp/opencode/st2-final-coverage.txt`,
audit `/tmp/opencode/st2-runtime-audit.log`; raw JSON/source/gcno snapshots in
`/tmp/opencode/reports/st2-evidence`. All exploratory run directories are retained as requested;
earlier harness iterations are not closure evidence (member-name mistakes, source-line
breakpoints after optimized tests, and phase-correlated clamp fixtures were corrected).

Gates: final link build RC0; coverage build RC0; manifest RC0; configure + matching build
RC0; all **87 build.sha1 entries OK**; full-file C++ token identity after comment stripping
RC0 (stronger than non-PC strip-diff). Review has four new MARK lines, each justified above;
its one DEL is the indentation-only power-bomb continuation (no C++ token deletion).
No ASM correctness findings. Exact logs `/tmp/opencode/st2-{final-link-build,
final-cov-build,final-manifest,matching-configure,matching-build,matching-sha1,strip-diff,
asmcheck,asmcheck-correctness,review}.log`. No unrelated family sweep or shared tooling edits.
