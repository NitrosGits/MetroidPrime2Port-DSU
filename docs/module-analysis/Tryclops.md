# Tryclops REL (module 81): analysis notes

Current: **59/59 behaviour bodies** (gui1 completion below; counts in the
historical survey are not the current marker inventory).

## gui1 completion (base c4b67e741, 2026-10-24)

The actual remaining-marker inventory has three rows, all branchless and
**100% in fresh upstream objdiff**: GetSearchPath0, IsListening8, and static
initializer5028. No missing gameplay algorithm or new executable fix.

- `GetSearchPath`: actual native virtual call on a live Cyclops; return pointer
  equals the address of its mPathFindSearch member. Retail0..8 adds the member
  offset and returns. Marker in header above the inline definition.
- `IsListening`: actual native virtual call returns true; retail8..10 returns1.
- `__sinit_CTryclops_cpp`: compiler-generated initialization, not a fake callable
  function. Retail5028..5124 constructs Normal(1,effect0,ignoreRadius0) PowerBomb
  weapon5 override;5128..5130 sets bomb offset(0,0,-0.3); remaining stores bind
  the12 triggers and18 states. Native oracle verifies the override fields, bomb
  offset, mouth locator and **every name/resolved PMF target** in both tables.
  Root locator pointer is optimized out on host; source constant Skeleton_Root
  verified statically (not claimed runtime coverage). Native method-pointer
  representation differs from retail's three-word member pointers, as expected.

Fixture `tools/cov/specs/gui1_tryclops.py` with driver
`tools/drivers/gui1_tryclops.con`, force_ret, coverage build, Sanctuary
`1baa96c2,42c85ad9`,800 frames. Final **1/1 call**, all getter/static oracles
PASS, explicit normal inferior exit, zero Python/fixture/runtime/console errors,
caught or reach-stub. No getter retention flag is needed (virtual functions
already have addressable bodies). Static initializer evidence includes fresh
exact100% match rather than demanding a separate runtime function with its
retail spelling.

Gates: host/coverage links0 FAILED; manifest rc0; upstream configure+ninja and
checksum **87 files OK**; source minus markers identical to base PC/non-PC;
whitespace clean. Review against c4b67e741:3 further MARK,14 total for gui1.
No unchanged-family sweep. Evidence `/tmp/opencode/reports/gui1-evidence/`.

The module's code is upstream's `src/MetroidPrime/Enemies/CTryclops.cpp` (CTryclops) plus the
`SLdrTryclops` loader. Retail asm: `build/G2ME01/Tryclops/asm/MetroidPrime/Enemies/CTryclops.s`.
Per-function table: `Tryclops_functions.tsv`. In game: the Sanctuary Fortress Cyclops, which sucks
in a morph-ball player or a bomb and throws it back.

## Survey (try1, 2026-10-23)

No behaviour difference was found, and no port code was changed. `tools/upstream_hollow.py Tryclops`
finds nothing. In objdiff every CTryclops function is at 100% except:

- LoadTryclops (fn_81_178, 79.8%): only scheduling and argument-evaluation order differ.
- `SLdrTryclops::SLdrTryclops` (fn_81_83C, 0%): the host inlines it into LoadTryclops. The retail ctor
  also stores values that already equal the defaults (weaponType 0 at 0x8A8, knockBackResistance
  2.0 at 0x8D0, ambient white at 0x8F0, visorFlags 0xF at 0x900), so it behaves the same.

Bitfields at 0x930 (mShotTarget, mTargetingBomb, mVulnerable, mDizzy) are not declared in the
asmcheck tables. Its 61 findings are tool limits: bit positions, calls into inlined helpers
(TCastToPtr, string internal_dereference, CPathFindSearch ctor, cos) and calls to the unmarked
REL-local copies.

## Coverage (try1)

Area: Sanctuary Fortress 0f_cliff_hall `1baa96c2,42c85ad9`.

- Cyclops u047b (eid 0x1b0082) stands at (148, 214.9, -13.2); u0455 (eid 0x1b0080) at (133, 213.1, -13.2).
- The floor is at z -13.234.
- To get grabbed, go to `tp 148 213.3 -13.0` or `tp 133 211.5 -13.0`, then `press x 40` (morph ball).
- Both placements have Retreat→Follow waypoints (001b007e/001b007f).

Specs: `tools/cov/specs/tryclops.py` logs Think, Death and BallCloseToCollision.
`tools/cov/specs/tryclops_force.py` is for `tools/cov/force.sh` and does three things:

- At entry of TargetCover, HearShot, JumpBack, InRange, InPosition, SpotPlayer and Death, it sets a
  bogus mBombId (0x047b) when the field is 0xffff.
- It cuts the Think dizzy timer to 0.01 when it is above 1.
- In GetUp, it sets mShotTarget=1 when the message is Update.

| driver | covers |
|---|---|
| `tryclops_grab.con` | grab / throw / struggle loop on u047b |
| `tryclops_struggle.con` | the player struggling while held |
| `tryclops_bomb9.con` | a bomb sucked in and shot back |
| `tryclops_beam2.con` | beam shots (HearShot, Dizzy) |
| `tryclops_direct.con` | statecall/trigcall of every state and trigger |
| `tryclops_force*.con` (4) | runs with the force spec: bogus bomb, dizzy timer, GetUp, double kill, kill while held |

### Results

39 bodies are marked (`// @ fn_81_<off>`). Every arm of a marked body either ran or is dead.
Arms that only ran when forced are labelled `forced (<cmd>)` in the marker notes:

- AcceptScriptMsg Deactivate: `send`.
- TargetCover, InRange, InPosition, HearShot, Inside: statecall/trigcall plus force.sh.
- The null-target arms of Suck, Crouch, PathFindEx, Attack and Dizzy: `trigcall InAttackPosition`,
  then statecall.
- SelectTarget, Cover, AttractBomb, SetBombPosition: statecall.
- Death: a double kill (mAlive false), a kill while holding the player (u0455 at f705, Dizzy), and
  the bogus bomb.
- GetUp, and JumpBack line 461 (bogus bomb): force.sh.

These arms count as dead:

- The `default` arm of every 3-case `switch (EStateMsg)` (Suck, PathFindEx, Attack, GetUp, Dizzy,
  Crouch). EStateMsg has three enumerators, and the FSM and statecall send only those.
- Data-dead arms:
  - Crouch and JumpBack, "retreat waypoint does not resolve": both placements have the waypoint.
  - LoadTryclops, null model.
  - SetupCollisionManager, invalid mouth locator: both placements load the model and its locator.

Not marked (missing arms from the merged gcov of all runs):

| function | missing |
|---|---|
| Think | the 212 struggle arm (BallCloseToCollision always returned true) |
| BallCloseToCollision | 17 arms (the collision probes never missed) |
| InMaxRange, InDetectionRange, InAttackPosition, SpotPlayer | range and angle short-circuit arms |
| ObjectInVortexArea, AttractPlayer, ShootPlayer, ShootBomb | 2-4 arms each |
| PathFind | one switch arm; Shuffle (only a PathFind call) stays unmarked with it |
| TurnAround, TargetPlayer, DoUserAnimEvent | lines not reached |
| CoverBlown, ShouldRetreat, ApplySeparationBehavior | one arm each |
| SLdrTryclops ctor/dtor, REL-local template copies | no host body (H rows) |

Next steps: force BallCloseToCollision to return false (struggle arm), and make a run where a
second Cyclops is close by (ApplySeparationBehavior).

## Smoke family (proposal)

`tryclops`: `1baa96c2,42c85ad9`, ~2600 frames, `MP2_GODMODE=1`, script
`tools/drivers/tryclops_grab.con`. It teleports to 148 213.3 -13.0, then `press x 40` for the
grab and throw. Expect u047b's FSM state to change in the `obj u047b` lines, and caught 0.
