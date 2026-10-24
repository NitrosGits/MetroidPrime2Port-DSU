# ChozoGhost REL (module 8): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CChozoGhost.cpp`; the loader defaults are
`include/MetroidPrime/ScriptLoader/SLdrChozoGhost.hpp` (the port's fix is a `TARGET_PC` block there).
Retail asm: `build/G2ME01/ChozoGhost/asm/MetroidPrime/Enemies/CChozoGhost.s`. Per-function table:
`ChozoGhost_functions.tsv` (B = module body, H = weak copy/template or inline ctor, T = REL runtime).

## Shipped data: the module is dead

No shipped area places a CHOG object, no area's module list names ChozoGhost.rel, and no AFSM holds
the ChozoGhost states (Growth, Hurled, WallDetach) or the AIStage trigger. Echoes never loads this
REL (`docs/modules.tsv`: 0 areas). All the coverage below is forced with console `spawn CHOG`.

## Behaviour fixes (cg1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_8_FC0 SLdrChozoGhost ctor | 0x1098 `li r12,-1`; 0x110C/0x1110/0x1114/0x111C `stw r12` 0x384/0x38c/0x390/0x3a8: the Impact, PhazeIn, PhazeOut and ProjectileVisor sounds -1 | 0 (a real sfx id) |
| same | 0x1150 CColor(1,1,1,1), 0x1164 `stw 0x294` ambient; 0x1168/0x1174 `stw 0xf,0x2e4` visorFlags | 0 / 0 |
| same, inlined SLdrGhostBehave ctor | 25,25,25,25,2,50,1 per range (0x1024-0x109C), then 0.0 for far heckle/chargeAttack and near lurk/heckle/move (0x11FC-0x1258) | the final values (unchanged) |

Checked equivalent (no fix): objdiff is 100% for every CChozoGhost body except AIStage (99%: the
`subf` operands swapped, same value), LoadChozoGhost (99.4%, regalloc and rodata labels) and
PreRender (91%: regalloc; both builds store an uninitialised CModelFlags word, no reader).
`upstream_hollow.py ChozoGhost`: none.

## Coverage (cg1)

`spawn CHOG` needs three stand-ins: the borrowed SpacePirate body `ancs u04d0 1 34` (character 1:
the donor's `GetCharacterIndex()` comes back 0, whose PAS database is empty, and the body controller
then starts state -1 and crashes), the pirate AFSM `fsm 5652a12f`, and the pirate WPSC 681f3907 for
both projectile props (`prop 54151870`, `prop 3a58089c`: the -1 defaults crash
CProjectileWeapon on the first kUE_Projectile). The PatternedAITypedef's knockbackRules stays -1;
the missing RULE reads as zeros and its parent rule 0 recurses until the heap runs out, so the run
writes RULE 393ca543 into an unset knockbackRules at `LdrToPatternedInfo` (gdb,
`tools/cov/specs/chozoghost_force.py`). Build `-B build-port-cov -DMP_PORT_COVERAGE=ON`, then:

    COV_GDB_SCRIPT=tools/cov/cov_force.py BUILD=build-port-cov tools/cov/force.sh \
      tools/cov/specs/chozoghost_force.py <out> 42b935e4,08_sand_casualpirates \
      --con tools/drivers/chozoghost_forced.con --frames 1000
    python3 tools/cov/gcov_report.py build-port-cov <out>/gcov --module ChozoGhost

Not `--fast`: the driver needs `frame` breaks (the console queues at most 256 commands), and an
unpaced game outruns each break by hundreds of updates. The driver runs every state body
(Activate, an Update per frame for 14 frames, Deactivate), every trigger, the script messages, user
anim events 0/3/6/14/15, the echo/scan/dark visors, contact with the player, beam/missile/charged
hits and kills, and a `spawn CHOG` without a body (the loader's null arm).

Marked by cg1 (29; every arm taken or branch-free with every line run; all forced (`spawn CHOG`),
objdiff as above): CBehaveChance ctor, AcceptScriptMsg, CanBeShot, GetCollisionResponseType,
GetDamageVulnerability, GetModelAlphau8, Render, IsVisibleEnough, FloatToLevel, IsOnGround,
ChooseBehaveChanceRange, SetWarpPosition, AIStage, Growth, InRange, SelectTarget, ShouldAttack,
ShouldTaunt, Lurk, ShouldMove, ShouldFlinch, AggressionCheck, Dead, PreThink, Think,
SetupStateMachine, LoadChozoGhost, RELMain, RELExit.

| function | arms | how |
|---|---|---|
| AcceptScriptMsg | OffGround with `!mVerticalMovement` | forced (`statecall Hurled 0`, then `send OffGround`) |
| ChooseBehaveChanceRange | mid range | forced (`spawn ... prop 61e511b3 0.5 prop 2369607a 1.0`, `place ... player 0.75`) |
| LoadChozoGhost | `!modelData` | forced (`spawn CHOG` without `ancs`) |
| GetDamageVulnerability | `x665_24_` false | forced (`statecall Growth 2` before the hits) |
| Dead | 0/1/2 | forced (`statecall <id> Dead 0/1/2`) |

Unmarked (live arms not run): the CChozoGhost ctor (the projectileVisor PART arm: needs a PART id
in the area), GetBehave (one `switch (type)` arm), Touch (one arm), PreRender (the red-tint arms),
InActive (body controller already on: `spawn` activates it), Generate, WallDetach, Run, Attack,
Land, Taunt, Hurled, Deactivate (CanIssueCommand / animation-progress arms the borrowed pirate body
does not reach), Leash (`mPlayerInLeashRange`, set by FindBestAnchor), ProjectileInfo and
DoUserAnimEvent (the attack-type-2 arm, the `speed > 0` arm), KnockBack (the charged Hurled range:
the borrowed rules keep the KnockBack reaction on), FindBestAnchor, AddToTeam, RemoveFromTeam,
GetOrigin and Shuffle's role arm (they need script anchors and a TMAI team connection, which a
spawned ghost has none of).

Host note: Shuffle's `Next() % numBolts` would raise SIGFPE on the host when numBolts is 0 (PPC
`divwu` does not trap). No guard was added, because no shipped data reaches it.

## cg2: PreRender fn_8_4468 (2026-10-24) — marked

Base **cab73977149d500ad8eec66d82bab5b2f79ea6c8**, new exclusive worktree
`../wt-mp2-cg2`, branch `decomp/cg2`. Only this production body gains comments and
its marker: full-file C++ tokens equal the base. No behaviour divergence/fix, no
loader/ctor/shared-file changes, no other slice or worker work duplicated.

### Measured arms and native outputs

**23/23 lines, 8/8 nonthrow branch rows**, final launch1 raw profile. The old cg1
red-tint blocker is closed. All CHOG activity remains **forced console spawn**;
there is still no shipped CHOG placement. Within that real actor, seven natural
PreRender calls get explicitly forced transient input members; no function return
or branch instruction is patched.

| measured source arm | counts / evidence |
|---|---|
| 301: second GetActiveVisor is Echo / not Echo | **2 / 201**. Every fixture independently captures both actual native visor returns, exactly [0,0] or [1,1]. Echo sets particles=false/calculateLighting=false and its white ambient is checked before base and at return; combat sets both flags true. |
| 309: alpha < 255 / alpha == 255 | **4 / 199**. Dead ghost fixtures use its actual alpha byte; live fixtures return255 despite input alpha19/29, through the actual virtual getter/inlined equivalent. |
| 309: red != 0 / red == 0 when alpha ==255 | **2 / 197**. Opaque red still selects blending; opaque zero-red selects Normal. |
| 310: tint / white branch after selecting blending | **5 / 1**. Nonzero red uses the saturation calculation; zero red + alpha64 produces white RGB with alpha64. |
| 311: max(255-2*red,0) computation | Retail is branchless sign-mask arithmetic, not a missing gcov arm. Actual byte oracles cover red1→253, red64→127, red127→1, red128→0 and red255→0, including both sides of the127/128 boundary. |
| Unconditional tails | Each of the seven calls verifies actual base PreRender(this,mgr), then CBoneTracking::PreRender with the real member/manager/animation/actor-transform/body-controller pointers and all three actual model-scale values. Native owner return verifies both calls completed exactly once. |

The 203 body calls include seven forced-input calls and196 unforced calls. The
complete per-fixture output oracles (before base **and** after full native return):

| case | alive / input red,alpha / visor | actual mode; RGBA; particles/calculate |
|---|---|---|
| normal | 1 / 0,19 / combat | **0; (255,255,255,255); 1/1** |
| white-alpha | 0 / 0,64 / combat | **5; (255,255,255,64); 1/1** |
| red64-live | 1 / 64,29 / combat | **5; (255,127,127,255); 1/1** |
| red127-echo | 0 / 127,128 / echo | **5; (255,1,1,128); 0/0** |
| red128-echo | 0 / 128,255 / echo | **5; (255,0,0,255); 0/0** |
| red255-zero-alpha | 0 / 255,0 / combat | **5; (255,0,0,0); 1/1** |
| red1 | 0 / 1,254 / combat | **5; (255,253,253,254); 1/1** |

All also verify material0 / flags3. No retail source arm needs a dead-arm exclusion.
The native compiler's guarded devirtualization fallback at PreRender+168 is not a
retail conditional: actual target dynamic type is CChozoGhost, its real constructor
installs its getter, no subclasses exist in this source tree, and no fixture writes
its vtable. The ordinary getter's alive/dead outcomes are both exercised. No fake
vtable is introduced merely to take a GCC implementation alternative.

### Retail audit and unchanged semantics

- `0x448C..0x44AC`: Echo enum1; particle flag is byte0x421 mask **0x01**, set to
  !echo via cntlzw/rlwimi. Host bit positions differ and come from DWARF, never
  from these retail offsets. The second actual visor query at44B4 independently
  selects false/white ambient or true lighting (44C8/44DC/44EC).
- `0x44F4..0x4528`: local actor-color copy, virtual alpha getter slot0xC8,
  alpha low byte <255 OR red nonzero. `0x452C..0x4554`: red guard, signed
  255-2*red, arithmetic sign mask clamps negatives to0, red255 and equal G/B.
- `0x455C..0x4564`: zero-red blend path takes whole White. `0x4568..0x45A8`:
  blend5/material0/flags3, calculated RGB with separately returned alpha.
  `0x45B0..0x4608`: Normal blend0/material0/flags3 and white RGBA1.
- `0x4614`: direct CActor base PreRender, not CPatterned. `0x4618..0x4650`:
  native model animation, actor transform, model scale, actual body controller
  passed to the body-controller overload of CBoneTracking::PreRender.
- The pre-existing uninitialized CModelFlags x0 word (retail4578/45E4 loads)
  is unchanged, not "fixed" as part of this slice. Semantic oracles deliberately
  check blend/material/flags/RGBA, not this unused word; raw snapshot/restore still
  preserves every byte of the pre-call flags object.

Full asmcheck RC1: one **XCALL** completeness heuristic (the alpha call is virtual
at4504–450C) plus **NOPOS** advisory for particles (retail byte421 bit01 decoded
above). Correctness MASK/XBIT/CROP/XCMP/XSTORE/OFFS: **RC0, no findings**.

### Fixture safety and preflight

New files only: `tools/cov/specs/cg2_prerender.py`, `cg2_selftest.py`, and
`tools/drivers/cg2_prerender.con`. The existing console `rules 393ca543` option
supplies the safe CHOG RULE directly; borrowed `ancs u04d0 1 34`, pirate AFSM
5652a12f and both WPSCs681f3907 follow cg1's safe recipe. No loader breakpoint,
loader edit or forged actor is needed. First target is verified dynamically as
`CChozoGhost *`, editor0036ff00, uid0535.

Each call reads all ORIGINAL bytes, arms its own FinishBreakpoint, then attempts
member writes/readbacks. Seven regions per call: actual alive/particle/calculate
flag storage bytes, mColor, complete mDrawFlags, existing CActorLights and the
actual player-state current visor. Flag storage is derived recursively from DWARF
bit positions (host1192/1193/373), not guessed uint units or retail offsets.
Existing model/animation/lights/body controller are required; no owning pointer
is replaced or resource lifetime raw-restored. Only those fixture/input/output
regions are restored; the ordinary base/animation/bone-render work actually runs.

Ownership uses real gdb.Frame equality and inferior/thread identity, not frame
levels. Recursive fixture entry is refused before new writes; callee observations
are attributed only to the immediate owning PreRender. Attempted-write exceptions
restore even after a write lands; all restore regions and readbacks are attempted
independently. Oracle failure/unwind also restore; stale/wrong-context callbacks
cannot overwrite another owner's state. Pending/no-case/failed runs fail the exit
summary. **18 offline tests pass**, including six post-mutation failure points,
partial/silent rollback and readback errors, unwind, arm failure, stale/duplicate/
recursive/context ownership, dynamically changing levels and boundary RGBA tables.
These mocks test safety, not native arm coverage. Offline GDB installation caught
the two CBoneTracking overloads before a launch; final hook selects the exact
body-controller signature. Exact executed fixture == committed fixture.

### Launch, frozen evidence, independent reproduction and gates

**1/2 launches used**, no second run needed. `/tmp/opencode/cg2-a`, paced800 frames,
normal exit, **zero caught/reach-stub/Python/fixture errors**, seven complete native
result oracles and **seven restores / 49 actual byte-exact original/readback pairs**,
no pending owner. Exact launch:

```
tools/games_running.sh --wait
BUILD=$PWD/build-port-cov SECS=600 tools/cov/force.sh tools/cov/specs/cg2_prerender.py \
  /tmp/opencode/cg2-a 42b935e4,08_sand_casualpirates \
  --con tools/drivers/cg2_prerender.con --frames 800
```

Default force.py, not force_ret/cov_force. No fast flag is needed for this short,
paced driver. All evidence/logs preserved, overriding old cleanup instructions.
`/tmp/opencode/reports/cg2-evidence/` freezes actual executable bytes, binary/source/
gcno hashes, pre-comment measured source, exact fixture/selftest/driver and native/
retail asm; subdir **a/** freezes matching gcno/gcda, raw JSON and actual game/fixture
logs before the marker rebuild. The measured source is exactly the base. The
runtime BINARY hash matches the preserved executable, not just a later rebuilt
binary. gcno/gcda and archive SHA256SUMS support independent reproduction after
the live build directory has been rebuilt:

```
python3 /tmp/opencode/reports/cg2-evidence/reproduce.py
# PASS frozen profile: PreRender 23/23 lines, 8/8 nonthrow branch rows
```

This was independently run using only archived gcno/gcda; new raw outputs remain
under `/tmp/opencode/cg2-gcov-repro-aqib4smo` and log
`/tmp/opencode/cg2-independent-coverage.log`. Do not feed old profiles to rebuilt
gcno. The initial matching asm-target invocation generated the asm in SPLIT then
reported an unknown target after Ninja regeneration; final matching build and all
artifact hashes pass. No game launch was spent on that static build/preflight work.

Gates `/tmp/opencode/cg2-{link-build,final-cov-build,manifest,matching-build,
matching-sha1,strip-diff,asmcheck-all,asmcheck-correctness,selftest-final,runtime-audit,
independent-coverage,review}.log`: actual mp_mod_ChozoGhost + executable host and
coverage builds pass, manifest RC0, matching configure/build RC0, **87/87** SHA1s,
whole-file token identity/non-PC strip-diff PASS,18 tests PASS, raw replay PASS.
Review's one MARK is fn_8_4468. No behavioral token deletion, other-function edit,
other-family run, broad smoke or second/post-marker game launch.
