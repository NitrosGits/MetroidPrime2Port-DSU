# SpankWeed REL (module 73): analysis notes

Current: **11/11 behaviour bodies** (spw1 continuation, 2026-10-24; evidence below).

CSpankWeed (SPNK): a patterned plant enemy that hides and lunges. Think pins its HP at 1000000, moves the
collision primitive to the "Eye" locator while it is not hiding, and updates a CCollisionActorManager
built from its joints. KnockBack only applies while mCanKnockBack is set. Source: upstream
`src/MetroidPrime/Enemies/CSpankWeed.cpp`, loader `SLdrSpankWeed.hpp/.inc`. Retail asm:
`build/G2ME01/SpankWeed/asm/MetroidPrime/Enemies/CSpankWeed.s`. Per-function table:
`SpankWeed_functions.tsv` (B upstream body, H weak/template copy, T dead or REL runtime).

## Survey (spw1, 2026-10-23)

Every B body was compared with the retail asm.

| function(s) | verdict |
|---|---|
| Think fn_73_1D48 | **fixed** (TARGET_PC): `0x1DD0 bne .L_00001E70` skips only the eye block. The `Update` at 0x1E80 runs on every active frame, with `0x1E7C li r5,0x1` (kUO_WorldSpace). Upstream called it inside the eye block with kUO_ObjectSpace |
| ctor, AcceptScriptMsg, KnockBack, GetAimPosition, GetOrbitPosition, Render, LoadSpankWeed, dtor, SLdr ctor/dtor | equivalent. The loader property ids match the retail compares (0x255a4580, b3774750, 9b5a4744, 835adca1, ed9bf5a3, 7c11b995, 5cdc877d, 7e397fed) |
| asmcheck findings on marked bodies | calls to the inline string dtor, ~CPatterned, Free, the SLdr ctor, the optional_object copy, the ctor and the loader typedef readers. The 19-vs-4 branch count comes from the `.inc` property switch |

## Retail-dead code

- **No states or triggers are registered.** The vtable keeps `SetupStateMachine__10CPatterned`, and upstream has no override, so the port registers none either. Patrol, FadeIn, FadeOut, Lurk, TargetPatrol, Attack, Flinch, InDetectionRange, InRange, HearPlayer, Delay, AnimOver, Attacked and IsPlayerNear (called only by the triggers) are unreachable (T).
- **CSpankWeedCollisionActor is never constructed.** There is no `bl __ct__24CSpankWeedCollisionActor`; the only internal call is its own Think → UpdateBounds (0x3554). All its functions are T.
- **mState (0x80c) and mPreviousState (0x810)** are written only by the ctor (-1, 0x2BCC/0x2BD0) and by the unregistered states. So GetOrbitPosition's 3/2 and 2/3 lerp arms are dead. The `min(time, 1)` at 0xE10 feeds only those arms (f7 is unused on the fallthrough at 0xED8).
- **mCanKnockBack (0x7d4)** is set to 1 only in FadeIn (0x1AA4 `li r3,1` / 0x1AB0 `stb`). It is cleared at ctor 0x2B78, FadeOut 0x18D0 and KnockBack 0x1D30. So KnockBack's apply arm is dead.
- **mIsHiding (0x7f0)** is cleared only in Patrol's Deactivate (0x1CDC, unregistered). So Think's eye block is retail-dead; the coverage run forces it.

## Shipped data

`python3 tools/scan_spnk_props.py`: `== 0 SPNK placements in 291 areas, 71819 objects, 168 types`. The
module is data-dead in retail, so all coverage is forced through `spawn SPNK`.

## Coverage (spw1)

The coverage build uses spec `tools/cov/specs/spankweed.py` and the forced driver `tools/drivers/spankweed_forced.con`:

- u0535 / e0036ff00 has a borrowed SpacePirate ANCS u04d0, with wake/search/attack radii 30/20/10 and hurtSleepDelay 2.
- A second `spawn SPNK` with no ANCS takes the loader's null-model arm.
- The run uses god mode.

Commands:

    COV_GDB_SCRIPT=tools/cov/cov_force.py BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/spankweed.py <out> \
      42b935e4,08_sand_casualpirates --con tools/drivers/spankweed_forced.con --fast --frames 900
    python3 tools/cov/gcov_report.py build-port-cov <out>/gcov --module SpankWeed

Result: rc 0, 0 caught/reach-stub.

| marked body | arms |
|---|---|
| Think fn_73_1D48 | ALL-ARMS. Eye block `forced (gdb: mIsHiding = 0 at Think entry)`; inactive return `forced (send Deactivate)` |
| LoadSpankWeed fn_73_494 | ALL-ARMS. Null model `forced (spawn SPNK, no ancs)` |
| KnockBack fn_73_1D08 | the false arm ran (`damage ... kb missile`); the true arm is dead (see above) |
| GetOrbitPosition fn_73_DAC | the fallthrough ran; the lerp arms and the min arm are dead (see above) |
| dtor fn_73_0 | ran (`send Delete`) |

Unmarked bodies, with the arms that never ran:

- ctor: non-uniform scale (177), lockon locator found (195).
- AcceptScriptMsg: the Create spatial-primitive loop, the HitObject collision-actor arm and other message arms.
- GetAimPosition: lockon locator found (533).
- Render: the spatial-primitive arms (568-574).

The borrowed body has no `lockon_target_LCTR` locator and no spatial primitive. No placement names a SpankWeed
ANCS, so these arms need a donor model that has both.

## spw1 continuation completion (2026-10-24)

Base `84d40c38b`, branch `decomp/spw1-84d40c38b` (the old `decomp/spw1`
already names the original survey; preserved). **5 → 11/11**. Six new marker
comments only; no native executable code, layout or matching path changed.

### Donors and shipped-data proof

The earlier "one donor needs both" assumption was too strong. We use:

- Pirate borrowed from `u04d0`, ANCS8169653a/character1/animation34.
- Wisp ANCS41e770c2/character0/animation0, whose CINF contains
  `lockon_target_LCTR` and whose CSPP2b3f06e8 is present. Two actors receive
  scale inputs(1,2,3) and(1,1,2), testing both non-uniform comparison arms.
- Lumite ANCS2f490dd9/character0/animation0 has no lockon locator; constructor
  fallback fields remain zero and GetAimPosition takes the missing-locator arm.

All **9 CSPP archive entries /3 unique resource ids** were decoded independently:
0d8cdf67 has1 sphere/0 boxes,839ecf93 has2/0,2b3f06e8 has8/0. Every archive copy
has zero boxes; offsets and SHA256 hashes are retained in `all-cspp-proof.json`.
Serialization is two vectors: spheres32 bytes each (two serialized segment words,
64-bit material, sphere16), then the box count. This follows SSphere's stream
constructor and `CSpatialPrimitive.cpp:5`. The native Wisp token's `mBoxes.mCount`
is also asserted zero. Therefore both the Create box body and Render OBB loop
are **dead for every shipped spatial primitive**, not merely absent from a donor.
No geometry/vtable/type tags were fabricated to manufacture box coverage.

The prior note's missing-lockon assertion for the borrowed pirate does not hold
on this build; the explicit Lumite donor supplies that case. A Blogg ANCS trial
was rejected by the Agon resource set and replaced before the final run.

### New marker / arm table

| fn | verification |
|---|---|
| B20 SLdr dtor | Fresh exact objdiff100% binding; real standalone constructor/dtor/free lifetime also executed. |
| B90 SLdr ctor | Retail B90..C8C audited store-by-store; standalone default oracle checks weapon9/damage20/knockback1/wait1, HP1000000/resistance2, unknown=true, radii60/30/15/hide5, white ambient and visor15. Every executable header line hit; no objdiff percentage claimed for this absent-report symbol. |
| 2A84 actor ctor | **40/40 lines,30/30 edges ALL-ARMS**; four scale/default oracles, both non-uniform tests and locator present/missing. Wisp lockon target equals world position plus offset; Lumite fallback target/offset remain zero. |
| F14 GetAimPosition | **14/14 lines,5/5 edges ALL-ARMS**; Wisp and Lumite, dt0 and0.1. |
| C8C Render | **10/15 lines,7/8 edges**; real GPU frames take primitive present/absent and empty-box paths. Five missing lines/one edge are the shipped-zero-box body above. |
| 1EE4 AcceptScriptMsg | **61/73 lines,69/85 edges**; real initial Create plus repeated Create on already-active controller, no-animation/no-spatial inputs, HitObject with real collision actor and player/non-player/stale last-touch ids, damage cooldown/state guards, Decrement state tests, Activate/Deactivate, AIUpdateDisabled, Delete on a non-current object and default. Timer/projectile-hit oracles PASS. Residual paths below. |

Remaining AcceptScriptMsg counters:

- The box/name-map construction/copy/traversal/destruction body cannot run with
  shipped CSPPs (zero-box proof above); no H reclassification or caller stub.
- Null model's inline HasAnimation guard is invalid for a constructed SpankWeed:
  its constructor unconditionally dereferences GetModelData for scale and layout,
  and Create later dereferences it for the swoosh locator. The valid no-animation
  ModelData case **was** run with its real wrapper object retained.
- Create's HasActorLights-false guard has no defined successful continuation:
  the same Create unconditionally calls `ActorLights()->SetLightingPositionOffset`
  below it, whose inline setter writes through this. All default loader fixtures
  construct lights; there are no shipped SPNK placements overriding that input.
  This is a retail invalid-configuration path, not a missing safe constructor case.
  Activate's null-light guard was safely tested separately. No null-check "fix"
  or contradictory HasActorLights/ActorLights return override was introduced.
- Untaken zero-count EH-only cleanup blocks inside the never-entered box body
  are likewise covered by that shipped-data exclusion, not claimed ALL-ARMS.

### Reproduction / gates

Coverage-only `spankweed_spw1_keep_inline.cmake` retains callable SLdr bodies;
normal host and matching flags are unchanged. Run:

`BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py SECS=420
tools/cov/force.sh tools/cov/specs/spankweed_spw1.py <out>
42b935e4,08_sand_casualpirates --con tools/drivers/spankweed_spw1.con
--fast --frames 1450`.

Final post-marker run: **31/31 calls**, all four actor constructor/default/lockon
oracles and standalone SLdr oracle PASS; explicit normal inferior exit; zero
failed calls/writes/restores/Python errors/caught/reach-stub/console errors.
Rendering is natural: an early diagnostic with a second later CALLS breakpoint
stopped the batch before exit, so the final spec uses one call queue only.

All26 asmcheck findings resolved: ctor/aim locator-name string temporaries;
3ABC reserve,2820 map copy,2750 string concatenation,25B4 vector push,27AC map
destruction,2968 vector destruction,111C CMotionState copy and RB-tree iterator
traversal are named H/template operations, not missing game bodies. Header XSTORE
findings mistake property hashes for offsets: actual stores are70 weapon9,
74 damage20,7c knockback1,80 wait1,84 HP1000000,88 resistance2,294 white ambient,
2e4 visor15,2f8 bool1,2fc/300/304/308 radii/timer. Base ctor initialization and
white CColor come through nested loader-data constructors; B20 Free is the
compiler's deleting-destructor path. Five retail-diff call aliases are the same
helpers; none unresolved.

Host/coverage links0 FAILED; manifest rc0; upstream configure+ninja and shasum
**87 files OK**. Production source stripped of marker comments is identical to
base for PC/non-PC. Whitespace clean. Review against84d40c38b:6 MARK only.
No unchanged-family sweep. Evidence `/tmp/opencode/reports/spw1-evidence/`:
raw module gcno/gcda/gcov JSON, native/header coverage, donor scans/resource
hashes, retail audit, build/strip/review logs and final run logs.
