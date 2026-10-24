# Sporb REL (module 76): analysis notes

The module's code is upstream's `src/MetroidPrime/Enemies/CSporbBase.cpp`, `CSporbNeedle.cpp`
(also CSporbPowerBomb), `CSporbProjectile.cpp` (the grabber) and `CSporbTop.cpp`, for the Sporb and
the Power Bomb Guardian. The port's fixes are `TARGET_PC` blocks in `CSporbBase.cpp` and the
`SLdrSporb*.hpp` loaders. Retail asm: `build/G2ME01/Sporb/asm/MetroidPrime/Enemies/*.s`.

## Behaviour fixes (sporb1, 2026-10-23)

| function | retail | upstream had |
|---|---|---|
| fn_76_71AC UpdateGrabber, Hold: the ball boosts free | 0x84EC-0x84F8 `lbz r0,0x1144(r7); rlwimi r0,r8,7,24,24; stb`: `EnableLeaveMorphBall(true)` | `GetMorphBall()->SetAsProjectile(true)` |
| fn_76_71AC UpdateGrabber, Extend with the ball inside | 0x795C `cror; beq` hold, 0x797C `blt` hold, 0x799C `ble` extend: a NaN extends | `dot > 0 && abs >= 1e-5 && next <= aim` (NaN holds) |
| fn_76_C130 ctor, flinch animation | 0xCC48-0xCCB4 eight NoParameter parms, `li r4,0x17`, no FromEnum | `FromEnum` lookup |
| fn_76_39D8 SLdrSporbBase ctor | 0x38 = 3, knock-back resistance 2.0, echo emitter 1, ambient white, visor flags 0xf, power-bomb and the 11 grabber/ball sounds -1 | 0 / false / black |
| fn_76_102CC SLdrSporbNeedle ctor | ambient white, visor flags 0xf, 5 sounds -1 | 0 / black |
| fn_76_3B8 / fn_76_DE60 LoadSporbTop / LoadSporbProjectile | knock-back resistance 2.0, ambient white, visor flags 0xf | 0 / black |

## Survey (sporb1)

Objdiff of every CSporb* function: apart from the fixes above, the differences are regalloc,
block layout, string/rodata addressing, and inlined vs REL-local template copies
(fn_76_4F88 = CImpactVisorEffect::None, fn_76_5334 = its dtor). These were checked and are
equivalent: the CSporbProjectile ctor's material lists (same sets in another order) and bbox,
PlayBallSpit/EscapeEffect (GetLocatorSegId vs GetSegIdFromString), CSporbNeedle::Render,
ShootPowerBomb statics, UpdateAim, CSporbPowerBomb::Think, the ctor's `x880_ = 0` and the
FakeDeath colour. CSporbProjectile::Think's morph test matches retail
(`player+0x390 ? 0 : player+0x38c`, == 1). Think: retail gates the power-bomb emitter loop on
`lbz 0x9ac` (the guardian flag) and the port does not. The result is the same, because only the
guardian fills those emitters.

Per-function table: `Sporb_functions.tsv`. B = upstream body, H = weak copy, template or
unnamed REL-local, T = REL runtime.

## Spawns

| area | layer | ids (this save) | use |
|---|---|---|---|
| `3dfd2249,0e_swamp_hall` | 0 | u0470 (top u046e, grabber u0471) at (-4,-51,55); u048e (top u048f, grabber u0491) at (15.6,-52.3,55.1) | grab chain, needles, fake death |
| `3dfd2249,06_swamp_boostballguardian` | 0 | u041b (top u041c, grabber u041d) | an ordinary Sporb: its guardian flag (0xb628855a) is 0 on disc |
| `3dfd2249,11_swamp_bloggtank_dark` | 2 (`layer 2` first) | u05f8 (top u05f7, no grabber), target waypoints u069b/u069a/u0696 (inactive until sent Activate) | the Power Bomb Guardian (flag 1) |
| `3dfd2249,0g_swamp_hall` | 0 | u0454/u0453/u0452, u0436/u0435/u0433 | unused |

The sporb sleeps until `send <id> Alert`. `fsm <id> <State>` accepts Start, Dead, Patrol, Sleep,
Attack, WakeUp, GoToSleep, Fire, AttackExit, ContinueFire, FakeDeath, FakeDead, Spit, SpitExit,
ContinueSpit, Flail, ContinueFlail and Flinch. `damage <top> N` hurts the top. The base takes no
damage, and the guardian's top only takes power bombs. In the 0e hall the floor under u0470 is a
pit (the ball falls out of the world), and a wall stands between u0470 and the flat spot at
(2,-51,42). The usable resting spot is (10.2,-55.2,42.5), under u048e.

## Coverage

Build `-B build-port-cov -DMP_PORT_COVERAGE=ON`. Merge the runs with
`python3 tools/cov/gcov_report.py build-port-cov <gcov dirs...> --module Sporb`.

| driver | run | covers |
|---|---|---|
| `sporb_hall.con`, `sporb_hall2.con` | `run.sh specs/sporb.py`, 0e hall, 3000 frames | wake, attack, needles, fake death, GoToSleep/Flail/Spit by fsm |
| `sporb_grab.con` | `COV_GDB_SCRIPT=$PWD/tools/cov/cov_force.py force.sh specs/sporb_force.py`, 0e hall, 2000 frames | grab, hold, retract with the ball, attach, spit, boost escape |
| `sporb_undertemple.con` | `run.sh`, 06 area, 2800 frames | the ordinary sporb there |
| `sporb_pbg.con` | `run.sh`, 11 dark layer 2, 3200 frames | guardian: power bombs, waypoint targets, stages, flinch, sleep |
| `sporb_pbomb.con` | `run.sh specs/sporb_pbomb.py`, 11 dark layer 2, 2000 frames | CSporbPowerBomb: bombs at the picked floor targets, then (after `send u05f8 SetToZero`) at the player: a direct hit, the blast's player and actor arms, the wait timer |
| `sporb_pbomb.con` | `COV_GDB_SCRIPT=$PWD/tools/cov/cov_force.py force.sh specs/sporb_pbomb_force.py`, same | the blast's TestRayDamage-false arms |

Merged: 171 bodies (sporb1). sporb2 adds the two pbomb runs to sporb1's five profiles.

Forced arms:

| arm | how |
|---|---|
| CSporbProjectile::Think ball-inside (fn_76_F084) | `forced (force.sh: mBallEscapeRadius = 100 while launched)`. The aimed grabber lands about 2 units from a resting ball, outside the 2x sphere radius. Everything after this arm runs unforced. |
| ContinueFlail, ContinueSpit, SpitExit, GoToSleep | `forced (fsm)` |
| CSporbPowerBomb::Think (fn_76_11E24) TestRayDamage false: 0x12038 `beq` (player, wait timer out), 0x12088 `beq` (player, inside the wait time), 0x1218C `beq` (other actor) | `forced (force.sh: specs/sporb_pbomb_force.py)`. After the first ray test of the run, every CStateManager::TestRayDamage starts 500 units lower (the write lands in Think's `center`, unread after a failed test). Nothing in the arena stands between a blast and the player. |

**Marked by sporb1 (60).** Each has every line run and every arm taken, or is branch-free.
asmcheck findings are explained below.

- **CSporbBase:** GetTouchBounds, SetupStateMachine, Render, Sleep, ContinueFlail, ContinueSpit,
  SpitExit, ContinueFire, FakeDead, AnimOver, ShouldPatrol, ShouldSpit, ShouldFlail,
  AttackExitOver, SpitOver, IsWithinRange, GetAimAngle, GetLocatorPosition,
  BuildLookAtTransform, StopTendril, PredictFlightTime, PreRender.
- **CSporbNeedle:** AcceptScriptMsg, Think, UpdateFuse, GetClosestCameraDistanceSq,
  GetTouchBounds.
- **CSporbProjectile:** SetupStateMachine, AddToRenderer, Sleep, Patrol, Attack, WakeUp,
  GoToSleep, Launch, Close, Open, Spit, ShouldPatrol, ShouldAttack, ShouldFire, ShouldReload,
  ShouldLaunch, ShouldClose, ShouldOpen, ShouldSpit, GetTouchBounds.
- **CSporbTop:** ctor, SetupStateMachine, Think, AddToRenderer, Sleep, Patrol, Attack, Spit,
  Close, GetOrbitPosition, GetCollisionPrimitive, GetTouchBounds, StartFlinch.

asmcheck/retail_diff findings, all explained:
- `internal_dereference` comes from rstl::string temporaries.
- StopTendril -> ClearTendrilParticles: the callee is ported but unmarked.
- Needle Think/UpdateFuse:
  - x330 bits 0x80/0x40/0x20 are mExploded/mHitWorld/mLaunchSoundPlayed in MSB-first order.
  - Their 0.1/50 constants and the GetClosestCameraDistanceSq calls sit in the header's
    PlaySfx/PlayLoopedSfx.
  - UpdateEffects and Explode are ported but unmarked.
- Top ctor: x800 0x80 is mImmune. The CCollisionPrimitive ctor is inlined from
  CCollidableSphere.
- Top GetTouchBounds: GetCollisionPrimitive is a virtual call (vtable 0x7c).
- BuildLookAtTransform: 6.2831855 is `2.f * M_PIF`.

**Marked by sporb2 (24).** Same rule; each body was read against the retail asm.

- **CSporbPowerBomb:** ctor (fn_76_12230), Think (11E24), ResolveCollisionWithWorld (11DB8),
  ResolveCollisionWithActor (11D44), RayCollisionCheckWithWorld (11D14), Render (11CF4), dtor
  (11A50), and the header's GetExplosionNormal (11CCC) and GetImpactParticle (11CE8).
- **CSporbTop:** ShouldPatrol, ShouldAttack, AnimOver, ShouldFire, ShouldReload, ShouldClose,
  ShouldSpit, Render, PreRender, dtor (fn_76_0).
- **CSporbProjectile:** Render, PreRender, dtor (fn_76_DDE0).
- **CSporbNeedle:** PreThink, Touch.

asm checks (sporb2), all equivalent to the port:
- ctor 0x122B4-0x12310: CEnergyProjectile gets `false, 1.0, 4.0, 4.0` (stack 0x2c = 0; rodata
  3BC = 1.0, 3C0 = 4.0). 0x12318-0x1235C: growth rate = `damage+0xc` (radius) / (end - start).
  0x12348-0x12388: three timers, the radius and the wait timer start at rodata 3B8 = 0. 0x12380:
  state -1. 0x1236C: normal = up.
- Think: 0x11E58 `fcmpo 0, t; bge` -> 0 is `rstl::max_val(0, t)` (`a < b ? b : a`, keeps 0 on
  NaN). 0x11E74/0x11E7C state 0 or 1. 0x11E8C `cror gt,eq` fuse >=. 0x11EA0 `cror gt,eq` and
  0x11EB0 `cror lt,eq`: start <= elapsed <= end. 0x11EC0 `rlwimi 4,27,27` (0x10 at 0x562) is
  SetExplodePending. 0x11EE4 radius -> 0x18c (mOrigDamageInfo + 0xc). 0x11ED0/0x11EF4 `li -3;
  and` on the exclude list's high word clears bit 33, kMT_Character. 0x12004/0x12170 `bge` skip:
  `distSq < r*r`. 0x12018 `cror lt,eq; bne`: wait timer <= 0. 0x120F8/0x120FC: the copy's damage
  (+4) and radius damage (+8) = 0. 0x121D8 `fmadds` radius += rate * dt, then 0x121E4 elapsed
  += dt. 0x121F0-0x12200: otherwise state 1 adds dt to the fuse timer.
- ResolveCollisionWithActor 0x11D64 state 0, 0x11D70/0x11D78 fuse timer = fuse time; World
  0x11DD8 state 1; both copy the plane normal (result +0x10) and zero velocity and gravity.
- RayCollisionCheckWithWorld 0x11D1C `li r10,1`: kSGT_CollisionGeometry.
- GetImpactParticle 0x11CEC `stb 0, 0xc(r3)`: an empty optional.
- Top Should*: `subfic 1; cntlzw` is `== 1` (Spit), `cntlzw` alone `== 0` (Close); the
  `li c; srawi; subfc; adde` idiom is signed `state >= c` with c = 3/4/5/6 (Patrol, Attack,
  Fire, Reload; kS_* values in CSporbTop.hpp).
- dtors: Top 0x24 CCollidableSphere at 0x7d8, Projectile 0xDE04 at 0x7c8, then the base dtor;
  PowerBomb calls fn_76_11AB0, the REL-local CEnergyProjectile dtor.

Dead arm: CSporbPowerBomb::Think 0x12130 `beq` (`TCastToPtr<CActor>` null). The near list comes
from CSortedListManager::BuildNearList (CSortedLists.cpp:278), which pushes only the ids of its
nodes' CActor. Deletions are deferred, so an id in the list is always a live actor.

**Covered but not marked.** These were held back by the cap or not proven:
- DecayAim: its 4 compares are in `rstl::max_val`, and the template's arms are not attributed
  to the body.
- CSporbNeedle and CSporbBase dtors: their owned-pointer deletes carry null-test arms not
  checked here.
- CSporbTop::ResetAttack: an empty port body with no retail function.

Near-complete, not marked (gcov line:arm untaken):

| function | untaken |
|---|---|
| AttachPlayer, ResetAfterGrab | projectile missing (1416, 1418, 1603, 1605) |
| SpitPlayer | 7/14 arms (morph-state variants) |
| UpdateShooting, ResetShotTimers, AttackExit, Fire, Attack | 1-4 arms each |
| CSporbProjectile::Think | 157/158 (no locator), 169 (ball leaves while launched) |
| CreatePowerBomb | the emitter loop ending with no free slot (1844, 1846) |
| ShootPowerBomb | TryCache / CanCreateProjectile false, a null bomb (1914-1926) |

Never ran:
- ScanVisorRender and GetScanVisorRenderBounds (Base, Top, Projectile): `visor scan` without a
  scan target did not reach them.
- FindTargetPlayer, GetContactDamage, GetTopAttachPosition, GetLeanDirection, CSporbTop::Freeze.

Guardian note (sporb2): the guardian loads with its timer scale maxed, so it fires at
PickTargetPosition floor points (the impacts land 5-20 units from the player). `send u05f8
SetToZero` makes it aim at the player (mPowerBombAimPosition), and then a bomb can hit the player.

## sporb3: UpdateGrabber fn_76_71AC (2026-10-24)

**Marked.** Read all retail blocks 0x71AC–0x8CDC (0x1B34 bytes), including unordered compares.
The earlier survey missed seven clamp operand-order differences. `rstl::min_val(a,b)` is
`b < a ? b : a`, and `max_val(a,b)` is `a < b ? b : a`; operand order matters for NaNs and signed
zero. The PC path now matches the retail selections below. Also move locator/bounds construction
inside the successful projectile lookup: retail 0x7230/0x7250 exits before that work.
No other behaviour change; the two sporb1 corrections remain valid.

### Block audit

| retail block | verified behaviour |
|---|---|
| 0x71AC–0x730C | Prologue; invalid id / failed projectile cast returns; inverted bounds and world connect locator only after those guards; state dispatch 0–6, default goes to the common tail. |
| 0x7310–0x7318 | Idle sets projectile solid phase to 0. |
| 0x731C–0x73D8 | Attach: locator rotation quaternion and position, attached=true; `mCanSpit` changes state 5 to 6. |
| 0x73DC–0x74F8 | Spit: same transform; 0x7490 `ble` skips release unless timer **>** rodata 114 = 0.6 (NaN waits). Release clears passable/carrying, adds material 0x3b, sets solid=0, state=4 and resets; otherwise attached=true. Timer always adds dt. |
| 0x74FC–0x76A8 | Launch: state=1, projectile state=0; X rotation rodata 118 = -1.5707964 (`-M_PIF/2`); quaternion from look-at times rotation. Flight sound `oris 0xa000` = `0xa0000000`; fire sound unflagged. |
| 0x76AC–0x7788 | Extend: landed phase=1 **or** any grabbing id enters Hold, stops loop/physics, plays world-hit sound and closes projectile. |
| 0x778C–0x795C | BallInside=2: pick target without lead, world ball-attach locator, look-at velocity, dot and next/aim squared distances. 0x7934 `fcmpo dot,0`; 0x7958 `cror eq,lt,eq`; 0x795C `beq` holds iff dot<=0 (unordered proceeds). |
| 0x7960–0x7B4C | 0x7978/0x797C `fcmpo abs(dot),1e-5; blt` holds for small magnitude; 0x7998/0x799C `fcmpo next,aim; ble` extends unless next>aim (unordered extends). Hold moves the projectile by target minus locator offset, closes/stops, sounds, claims grabbing id, stops wakes and applies damage. 0x7AC4–0x7AD4 constructs `1ull << 59`: data_358 = 0x3b, Solid. |
| 0x7B50–0x7BBC | Optional valid top; non-guardian disables telegraph. Invalid id and failed cast converge without effect. |
| 0x7BBC–0x7E5C | Extend tendril: look-at from origin; 0x7C2C/0x7C30 chooses `length < range ? length : range` (unordered selects range). Count = uint(distance / rodata 120 = 0.15)+2. Sinusoidal x offset times i/count, y=0.15*i; particle pos/prev-pos stores, maximum size, bounds expansion. |
| 0x7E60–0x7F80 | 0x7E74/0x7E78 chooses `next < 0 ? 0 : next` (unordered keeps next), then sets outgoing velocity. Range test uses **incoming** speed in the unrotated y step, as upstream does. 0x7F10 `ble` skips clamp unless squared distance>range². 0x7F5C computes an unused transformed clamp vector: there is intentionally **no** SetTranslation. Hold/stop/close follows. |
| 0x7F84–0x83AC | Hold timer: 0x7F90/0x7F94 chooses `duration < next ? duration : next` (unordered keeps next). 0x8008/0x800C length clamp as Extend. Sine amplitude `1-hold/duration`, phase ends at count-1; live particle end-frame=0; remaining particles rest at base position. |
| 0x83AC–0x8488 | Attach timer: 0x83B8/0x83BC chooses next on unordered. 0x83D8 `cror eq,gt,eq` requires ordered timer>=duration; retract, optionally carry ball/disable boost/send Entered. Otherwise solid or a different grabbing Sporb retracts only at exact hold duration (0x8478 `fcmpu`; NaN unequal). |
| 0x848C–0x869C | While still holding: boosting **and** passable escapes, sets Retract, clears x8f8/passable/carrying, opens projectile, sets player leave-morph bit 0x80 at 0x1144, marks escaped, clears grabbing id, computes unused locator, effect+sound. Retract sound chooses carrying/missed sound and ORs 0x20000000 (0x8660). Always stops projectile. |
| 0x86A0–0x8A38 | Retract length clamp 0x86F8/0x86FC identical; straight tendril, end-frame=0, particle bounds and leftover resting loop. |
| 0x8A38–0x8B78 | 0x8A4C/0x8A50 chooses `0 < next ? 0 : next` (unordered keeps next). 0x8AB0 `bge` skips arrival unless direction²<step² (NaN keeps retracting). Arrival stops/positions; no ball resets, carrying enters Attach and optionally closes top, sets locomotion Lurk=2 and state Flailing=8. Stops tendril on both arrival paths. Otherwise sets rotated velocity. |
| 0x8B78–0x8BBC | Carrying sets attached. Escaped and releaseTimer>rodata C8=0.1 closes; otherwise timer adds dt (NaN adds). |
| 0x8BC0–0x8CDC | Save grabber position except states 5/6, always set quaternion+position transform, attach player iff attached, copy bounds iff generator, epilogue. |

Sound calls throughout use vtable 0x78, constants C0=1, C8=0.1, F8=1000, C4=0, channel 0,
segment 0, priority 20, volume 127, current distance/position/next-area/mgr and true.
Particle size maxima use strict `size > maxSize`, retaining maxSize for unordered, as upstream.
Retail's null particle-address branches (0x7D54/0x8158/0x87E4) cannot occur with i<active count:
the live vector allocation supplies those indexed elements (CElementGen.cpp:666–700, 854).
The upstream Hold/Retract generator-null guards are stronger than retail's unconditional
leftover-loop dereference (0x824C/0x88D8); retained and explicitly exercised, not newly added.

### Reproducible coverage and branch table

Final source: **245/245 lines, 134/134 non-exception gcov edges, ALL-ARMS**. The compiler
attributes the initial dot test to several adjacent expression lines; all copies are covered.
Two final runs, both in `3dfd2249,0e_swamp_hall`, normal exits and caught/reach-stub=0:

```sh
tools/games_running.sh --wait
BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py tools/cov/force.sh \
  tools/cov/specs/sporb_grabber_arms.py /tmp/opencode/sporb3-final-arms \
  3dfd2249,0e_swamp_hall --con tools/cov/specs/sporb_hall2.con --fast --frames 3000
tools/games_running.sh --wait
BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/cov_force.py tools/cov/force.sh \
  tools/cov/specs/sporb_force.py /tmp/opencode/sporb3-final-grab \
  3dfd2249,0e_swamp_hall --con tools/cov/specs/sporb_grab.con --fast --frames 2600
python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/sporb3-final-arms/gcov \
  /tmp/opencode/sporb3-final-grab/gcov --module Sporb --fn fn_76_71AC
```

The first spec runs 37 one-call scenarios and restores the inputs. Scenario indexes below are
zero-based; every call succeeded. Assertions #10 and #27–36 all PASS. The second run forces only
the projectile's capture radius (sporb1 spec); the complete grab/attach/spit and boost chains then
execute. Natural below means normal execution after the stated driver/capture setup.

| branch family | evidence (both outcomes unless specified) |
|---|---|
| invalid projectile / cast | forced (`force_ret.py` #0/#1), natural valid lookup |
| all seven states / default | natural state dispatch plus forced #2 Idle, #3 default |
| Extend landed / another grabbing Sporb / neither | natural landed/flight; forced #5 grabbing id |
| BallInside / ordinary flight | capture-radius run / hall flight |
| dot<=0 / abs(dot)<1e-5 / overshoot / continue | forced #27 zero forward, #28 tiny forward; capture run supplies ordered overshoot and continuation; forced #29 NaN forward asserts Extend |
| top absent/present, guardian/non-guardian | forced #6/#7; natural top/non-guardian |
| all three particle loops, particles present/absent | natural iterations/exits/writes; forced #9/#13/#24 active count=0 skips writes |
| generator present/absent (each state and common tail) | natural present; forced #4/#8/#12/#23 null generator |
| maximum range / ordinary flight | forced #10 range=0.15 and projectile displaced +10 x (assert Hold); natural flight |
| outgoing/incoming speed clamps | natural and forced #11 negative outgoing, #25 positive incoming; NaN #30/#33 assert propagation |
| hold/attach timer clamps | natural below/at durations; forced #31/#32 NaN assert propagation and no early attach |
| length clamps in Extend/Hold/Retract | natural lengths; forced #10 finite clamp; #34/#35/#36 unordered length asserts count=22 for range=3 |
| Hold expired/not, attach timeout with/without ball | forced #14/#15, #21/#22; natural hold/retract chain |
| solid, no grabbing id, own id, other id | forced #14–18 |
| boosting false/true, passable false/true | natural non-boost; forced #19 boost+not-passable, #20 boost escape; capture run also boosts free |
| Retract arrival/non-arrival, carrying/not | natural complete/missed grab chains; forced #26 arrives carrying with missing top |
| release timer / escaped flag | natural capture/boost run (both timer arms and escaped/not) |
| Attach can-spit false/true; Spit timer wait/release | natural capture/attach/spit chain |
| tail Attach/Spit exclusion, attached flag, bounds | natural all states; forced #4 supplies bounds-null |

### Gates and audit findings

- Host and coverage links: 0 FAILED. `gen_files_cmake.py --check`: rc 0. TARGET_PC strip-diff
  against bf635ac9f: CLEAN (ignoring marker comments). Matching build plus dtk hash check:
  **87 files OK**, All 77.98% fuzzy / 54.39% matched; Game Code 94.14% / 63.29%.
  CSporbBase unit 74.15462% fuzzy / 23.537437% matched; unchanged non-PC code.
- `asmcheck` all tags: four CALL findings. `internal_dereference` is the string destructor;
  fn_76_59BC = the real unmarked PickTargetPosition body (CSporbBase.cpp:1766), fn_76_E358 =
  real PlayBallEscapeEffect (CSporbProjectile.cpp:423), fn_76_7000 = real AttachPlayer
  (CSporbBase.cpp:1494). All executed; no stub. Four OFFS findings are the older compound
  explanatory comments pairing multiple addresses/operand names; exact instructions and
  senses are audited in the table above. No mask/store/compare finding.
- `retail_diff`: those same three unmarked-but-real callees; -1.5707964 is `-M_PIF / 2.f`.
  `review_decomp_diff`: one MARK, fn_76_71AC, justified by the block audit and ALL-ARMS run;
  no ASM lines (upstream file, so the explicit all-tag check above is the applicable one).
- Optional UpdateShooting / UpdateAim remain unmarked in this slice. No whole-module sweep
  or unrelated-family runs; no shared-code changes or temporary production probes.

## sporb4: UpdateShooting fn_76_6ABC / UpdateAim fn_76_6390 (2026-10-24)

**Both marked.** Complete retail block audits (0x6ABC–0x6F6C and 0x6390–0x6820).
Two TARGET_PC corrections: saturate a negative burst-size spread before the host u8 cast;
preserve an unordered angle in UpdateAim's scale clamp. The older UpdateAim survey above
missed the latter operand-order difference. Non-PC source remains identical, except markers.

### Shooting block audit

| retail block | verified behaviour |
|---|---|
| 0x6ABC–0x6B0C | Prologue; shooting byte at 0x887 must be set. 0x6B04 `fcmpo timer,interval`, 0x6B08 `cror eq,gt,eq`, 0x6B0C `bne`: ordered **>=**, so NaN in either timer skips firing. Every skip reaches 0x6F38, not the epilogue. |
| 0x6B10–0x6B5C | Both child lookups and checked casts run; top must exist, and projectile must exist unless guardian byte 0x9ac is set. |
| 0x6B60–0x6B8C | Shooting=true; top state=5 (Fire), optional projectile state=8 (Fire); shot timer=rodata C4=0. |
| 0x6B90–0x6BC8 | Interval = timerScale*(min+(max-min)*Random.Float); byte shot count increment at 0x6BB8 wraps before unsigned comparison to burst count. Below count skips only the burst-reset block. |
| 0x6BCC–0x6C58 | Same interpolation for attack interval, shot count=0. 0x6C14 `subf r0,r6,r4` is signed maxShots-minShots; 0x6C1C `xoris 0x8000` plus rodata 108=4503601774854144 converts it to float. 0x6C30 `psq_st f31,0x18(r1),1,qr2` saturates/truncates the random spread to u8, then adds minShots and stores a byte. Shooting=false, x88c=true, x85c=0. The PC lower clamp restores that saturation for reversed bounds. |
| 0x6C5C–0x6CCC | Compute top head and world locator even for guardian/no-spawner. Guardian skips needle generation. 0x6CC0/0x6CC4 `clrlwi ...,6` masks both editor ids to 26 bits; `TEditorId::operator!=` does the same. |
| 0x6CD0–0x6D38 | GenerateScriptObject; null entity skips only needle setup/sound. Otherwise look-at transform, locator translation, retain initial velocity magnitude. |
| 0x6D3C–0x6DFC | Two Next()%2 tests: even selects +1, odd -1. Each variance uses a separate Float; multiply by rodata 110=0.017453292 for radians. |
| 0x6E00–0x6E74 | Build RotateZ(yaw), RotateX(pitch), multiply **X*Z**, then set velocity to spread*(speed*look-forward). |
| 0x6E78–0x6F08 | ProcessSoundEvent via vtable 0x78; sound at 0x892, C0=1, C8=0.1, F8=1000, C4=0, zero channel/segment/flags, priority=20, volume=127, camera distance, top-head location, next area, mgr, true. |
| 0x6F0C–0x6F6C | Send Attack (`lis 0x4154; addi 0x544b` = 0x4154544b), invalid unique id, None=-1; common timer += dt at 0x6F38–0x6F40; epilogue. |

The spread is always finite and in [-255,255]: both bounds are bytes, and CRandom16::Float
is a 16-bit integer times 1.52590222E-5, in [0,1]. No upper clamp or NaN policy is needed here.
For normal ordered bounds the sum is <=maxShots; for reversed bounds it is minShots.

### Aim block audit and all 17 clamp selections

| retail block | verified behaviour |
|---|---|
| 0x6390–0x6438 | Prologue, quick inverse of actor transform, transform copied aim target, normalize 3D direction, GetAimAngle. |
| 0x643C–0x6484 | rodata FC=57.295776 converts angle to degrees; fabs; rodata 100=120. 0x6458 `fcmpo f3,f0` compares **120,absDegrees**, 0x645C `bge` selects absDegrees, including unordered. Correct PC form is `min_val(absDegrees,120)/120`, not reversed arguments. |
| 0x6488–0x64F0 | Flatten normalized direction to z=0; dot Forward and Left, normalize 2D result. 0x64E8/0x64EC/0x64F0 requires ordered forward component>=0; unordered enters back half. |
| 0x64F4–0x6654 | Forward half (through 0x65AC) / back half: scale*abs(component); target>0 gates smoothing, else decay; always decay opposite lean. Stores at 0x7ec/0x7f0. Exact clamps below. |
| 0x6658–0x667C | Finish forward decay; 0x6674/0x6678/0x667C requires ordered left component>=0; unordered enters right half. |
| 0x6680–0x67F0 | Left half (through 0x6738) / right half as above; stores at 0x7f4/0x7f8. |
| 0x67F4–0x6820 | Epilogue, no extra updates. |

Let `t` be the desired positive lean, `o` the old lean, and `d`=rodata 104=0.025.
Each table entry is the fcmpo offset (the following instruction is the branch).

| direction | t>0 / t>o gates (`ble` skips) | rising clamp | falling clamp | zero-target decay | opposite decay |
|---|---|---|---|---|---|
| forward | 0x6500 / 0x650C | 0x6520 | 0x6540 | 0x656C | 0x6598 |
| back | 0x65BC / 0x65C8 | 0x65DC | 0x65FC | 0x6628 | 0x6654 |
| left | 0x668C / 0x6698 | 0x66AC | 0x66CC | 0x66F8 | 0x6724 |
| right | 0x6748 / 0x6754 | 0x6768 | 0x6788 | 0x67B4 | 0x67E0 |

- Rising: compare `o+d,t`; `bge` selects t, else o+d. Upstream `min_val(o+d,t)`
  reverses the comparison but is equivalent on every reachable input: t>0 and t>o have
  already rejected unordered t/o; t is bounded by 1, so o+d cannot become NaN.
- Falling: compare `t,o-d`; `bge` selects t (also unordered), else o-d. The existing
  `max_val(t,o-d)` is exact, including a NaN previous lean snapping to finite t.
- Both decays: compare `0,o-d`; `bge` selects zero (also unordered), else o-d.
  Existing `max_val(0,o-d)` is exact. An unordered angle makes every t unordered and
  all four leans decay; an unordered direction likewise takes the nonpositive-target paths.
- The scale clamp plus these 16 clamps account for asmcheck's 17 extra conditions.
  Signed-zero selection cannot differ in a rising clamp, where t>0.

### Reproducible coverage and branch evidence

Final source: UpdateShooting **51/51 lines, 26/26 gcov edges**; UpdateAim **38/38 lines,
28/28 edges**, both **ALL-ARMS**. Template clamp edges are additionally checked by the
explicit below/above-target, decay, and NaN output assertions, rather than inferred from
the enclosing function's gcov total. **68/68 scenarios and assertions passed**, no forcing
or restore errors. One 1800-frame run, normal exit, caught/reach-stub=0; all driver commands OK.

```sh
tools/games_running.sh --wait
BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py SECS=300 tools/cov/force.sh \
  tools/cov/specs/sporb_shoot_aim_arms.py /tmp/opencode/sporb4-final-arms \
  3dfd2249,0e_swamp_hall --con tools/drivers/sporb4_hall.con --fast --frames 1800
python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/sporb4-final-arms/gcov \
  --module Sporb --fn fn_76_6ABC --fn fn_76_6390
```

All numbered cases below are **forced (`force_ret.py`, sporb_shoot_aim_arms.py)**. Each call
restores its inputs and both children's state; emitted needles remain real game objects.
Aim cases override only GetAimAngle's return while executing the real inverse, vector
normalizations, sign gates, clamps and stores. Natural hall updates run between driver steps.

| branch family | evidence |
|---|---|
| Shooting disabled / timer below / above / equal; unordered timer or interval | #0/#1, natural fire, #58; #2/#3 assert no fire and common timer tail |
| Top absent/present; projectile absent/present; guardian exception | #4/#5/#6 and natural hall children; #6 asserts guardian-without-projectile fires |
| Burst complete/incomplete and byte wrap | #9 and ordinary shots; #10 count=255 wraps before comparison |
| Spread negative/zero/positive | #11 reversed 4/2 bounds yields 4; #9 equal bounds yields 2; #59 bounds 2/10, seed=1 (third RNG=10113) yields 3 |
| Guardian / masked invalid spawner / generation null / needle | #6/#7/#8/#12–19 plus natural hall fire |
| Pitch and yaw parity | Eight controlled RNG seeds #12–19 plus natural needles; gcov both outcomes for each |
| Aim quadrants, both signs and both smoothing directions | #20–47: all quadrants, old lean 0/.7/.71/1/NaN; rising unclamped/clamped, falling clamped/unclamped/NaN |
| Zero/unordered angle and active/opposite decay | #25–26/#32–33/#39–40/#46–47 decay from .5; #60–67 repeat all quadrants at zero angle with old .01/NaN, assert zero |
| Axes/zero components, angle below/over clamp | #48–55: all four axes, 60/240 degrees, old .5/0 |
| Negative angle; unordered direction | #56 uses -60 degrees; #57 NaN target x, all leans decay |

### Gates and follow-up

- Host and coverage links: **0 FAILED**. Manifest `gen_files_cmake.py --check`: **rc 0**.
  TARGET_PC strip-diff vs e62dbcda8: **CLEAN**, ignoring markers. Configure+ninja plus
  explicit dtk hash check: **87 files OK**; All 77.98% fuzzy / 54.39% matched,
  Game Code 94.14% / 63.29%.
- Explicit all-tag asmcheck: one BRANCH finding for UpdateAim (27 asm conditions vs 10
  syntactic C++ conditions); the 17 template clamps audited above explain the difference.
  UpdateShooting: no findings. `retail_diff` (both ids): **no findings**.
- `review_decomp_diff.sh e62dbcda8`: two MARK lines, justified by the full audits and final
  ALL-ARMS run; no ASM lines (upstream file, checked explicitly above).
- Shared follow-up: `include/Kyoto/Basics/CCast.hpp:72` does not emulate the retail u8
  saturation. The already-marked ResetShotTimers (`CSporbBase.cpp:1596`) also calls it on
  a signed bound difference; retail 0x69F4 is the same `psq_st ...,1,qr2`. That caller and
  shared conversion remain outside this two-function slice; neither is needed for the
  local UpdateShooting correction. No other functions marked or unrelated runs performed.

## sporb5: core update, messages and targeting (2026-10-24)

Base **31baf7af4**. Compared the module-76 retail inventory with actual source/header `// @`
markers: 35 unmarked B-class bodies, 138 marked. This checkpoint marks **four**:
Think (AF8C), AcceptScriptMsg (B9E4), PredictAimOffset (481C), PickTargetPosition (59BC).
At this checkpoint the module reached **142/173**; remaining inventory is below. Loader/constructor byte size
is dominated by generated properties/initializers; the core runtime branching was taken first.

### Corrections

All seven changes preserve the original non-PC text under TARGET_PC:

| function / retail | correction |
|---|---|
| Think 0xB2B0 `lwz r3,0x60(r28)`; 0xB2B8 `lwz r3,0x10(r3)` | Flinch additive animation belongs to **this base's** CAnimData, not the top's. r28 is this throughout. |
| Think 0xB3A0 `fcmpo f0,f1`; 0xB3A4 `bge`; 0xB3AC `fmr f0,f1` | f0=1, f1=blend+dt. Select next on unordered: `min_val(next,1)`. |
| Think 0xB6C4 `subf`; 0xB6C8 `clrlwi. r31,r0,24`; 0xB6DC `divwu` | Double-shot bound difference wraps to **uchar** before nonzero test and modulo. Reversed 4/3 bounds give range=255, divisor=256. |
| Think 0xB880–0xB888 | Gate the emitter loop on guardian byte 0x9ac. The older survey accepted the ordinary actor's invalid-handle loop as equivalent; now the original skip is explicit. |
| AcceptScriptMsg 0xBDB0 `subf`; 0xBDB4 `clrlwi. r31,r0,24`; 0xBDC8 `divwu` | Same byte-range correction when Create chooses the initial double-shot count. |
| AcceptScriptMsg 0xBF0C `subf`; 0xBF18 `stw`; 0xBF24 `fsubs` with rodata 128=2^52 | Remaining stages convert as **unsigned** (no xoris). Since host vector::size returns int, explicitly convert the subtraction to uint before float. Stage=5, count=4 produces a large positive health factor, not a negative one. |
| PickTargetPosition 0x5A0C `lbz r4,0x9ac(r4)`; 0x5A14 `cmplwi r4,0`; 0x5A18 `beq`; 0x5C2C `cmplwi r4,0`; 0x5C3C `beq` | The ordinary actor enters with r4=0 and nothing writes r4 or calls another function before the second comparison. The prediction call at 0x5C60 is therefore **dead**, regardless of incoming boolean r6 (never read). Use zero predicted offset; preserve the public signature. |

### Full block audit: Think, 0xAF8C–0xB9E0

| block | result |
|---|---|
| 0xAF8C–0xAFD4 | Prologue; CEntity +0x20 bit 0x80 is active, false goes directly to epilogue; otherwise CPatterned::Think. |
| 0xAFD8–0xB06C | Guardian and not timerScaleMaxed (+0xa74 bit0x20): predict from locator and add player's virtual aim position (vtable 0x54, time=0), store power-bomb aim xyz +0xa50. |
| 0xB070–0xB0C4 | Detection range resets timer to C4=0; outside max attack range and timer<leash increments by dt. 0xB0B8 `fcmpo`/`bge` means strict <, unordered skips. |
| 0xB0C8–0xB0F4 | Guardian, nonempty stages and unsigned byte attacks>=threshold set shotsInBurst=2. |
| 0xB0F8–0xB174 | Actor transform times scaled connect locator; invalid top id / failed top cast converge. Source ObjectById handles invalid id itself. |
| 0xB178–0xB1FC | Alive top gets connect transform; dead top enters FakeDeath unless state 6/7. Guardian forces immune (top +0x800 bit0x80). |
| 0xB200–0xB27C | Transform top lockon locator through connect; require hit-by-player bit (+0x420 bit0x80) and not immune. |
| 0xB280–0xB370 | Repeated hit test at 0xB284 cannot be false after 0xB270; mark topHit, clear top hit, add base flinch at weight1, false/true. Ordered blend>=1 snapshots base lockon position and four lean weights; NaN skips snapshot. Always blend=0. |
| 0xB374–0xB3F0 | Immune clears hit; advance blend with corrected clamp. Orbit xyz = saved*(1-blend)+head*blend. |
| 0xB3F4–0xB500 | Alive top, frozen top, unfrozen base: Freeze via vtable 0xc0, zero/forward vectors, top duration, rodata138=-1; optionally freeze projectile, then ResetShotTimers. |
| 0xB504–0xB5B4 | Valid projectile and passable: retail `(state!=6 || state!=7)` is deliberately tautological; set x8f8, stop non-boosting player/angular velocity/velocity, clear player's +0x1144 bit0x80 (leave-morph). |
| 0xB5B8–0xB5EC | Grabber states4/0 (Idle/Launch) copy connect transform; every valid projectile copies its position. |
| 0xB5F0–0xB65C | State0/1 decays and resets. States3/4/8/9/10 use firing windup; threshold .8 ordinary, 2.433333 guardian burst>1, otherwise1.3 (rodata144/13C/140). Strict x860>threshold; unordered waits. Negative state values have no writer (EState and assignments only 0–11). |
| 0xB660–0xB6F4 | Guardian, stages, valid index, sufficient attacks and burst>1 reset counter then choose inclusive double-shot interval using corrected byte range. |
| 0xB6F8–0xB720 | Guardian updates aim; both types UpdateAttack(dt,mgr). |
| 0xB724–0xB7B4 | Before windup: ordinary or unlocked guardian picks target; boolean is state3/4; lock aim, UpdateAim, x860+=dt. |
| 0xB7B8–0xB83C | Other states: same ordinary/unlocked test; the state3/4 expression is necessarily false here; pick, lock, UpdateAim. |
| 0xB840–0xB87C | States8/6/7/11 additionally decay; sleeping/patrolling branch calls decay and ResetShotTimers. |
| 0xB880–0xB93C | Guardian-only loop over four emitter slots. Invalid/dead projectile: RemoveEmitter, zero handle, invalid uid; live energy projectile: UpdateEmitter at translation, zero velocity, volume0x80. |
| 0xB940–0xB9E0 | Four base additive animations forward/back/left/right with corresponding weights, false/false; epilogue. |

**Dead predicate proof:** 0xB698 / source `mShotsInBurst > 1` false cannot occur after its
preceding guardian/nonempty/counter tests pass: 0xB0C8–0xB0F4 already set the burst to2.
The only intervening reset is frozen-top ResetShotTimers, which also sets x860=0; then
the positive windup threshold prevents reaching this predicate. This is the sole uncovered
Think gcov edge. The passable-state OR and repeated hit-bit branch above are other retail
redundancies optimized away by the host, not missing port logic.

### Full block audit: AcceptScriptMsg, 0xB9E4–0xC034

| block | result |
|---|---|
| 0xB9E4–0xBAE4 | Message dispatch: STOP, INCR, DECR, ALRT, SMAX, RSET, XALD, STRT, ZERO, XCRT; XAUD/default go to common base call. FourCC lis/addi pairs decoded directly. |
| 0xBAE8–0xBB38 | Iterate 12-byte connections and resolve each editor id; Approach=0x41505243. Try top first. |
| 0xBB3C–0xBBA4 | Set top uid; save vulnerability via vtable0x40; assign Immune via vtable0xa0; set immune0x80 and base uid. |
| 0xBBA8–0xBC10 | Otherwise try projectile (store uid), then waypoint (append uid); failed casts do nothing. |
| 0xBC14–0xBCD0 | GRNT=0x47524e54 stores needle spawner. Loop exit resets shared grabbing uid. Optional projectile gets top uid; optional top gets projectile uid. |
| 0xBCD4–0xBD6C | Create activates inactive body, Crouch=0, animation state=-1. Remove Target=40, Orbit=41, SeekerTarget=63; assign guardian Reflect vs ordinary Immune vulnerability; remove Target again, add material54. |
| 0xBD70–0xBDE0 | Guardian and valid nonempty stage choose initial double-shot interval; byte wrapping described above. |
| 0xBDE4–0xBE28 | Optional tendril gets actor position and creates uint(maxRange/rodata120=.15)+2 particles. |
| 0xBE2C–0xBE5C | Guardian Increment: max(stage-1,0), byte store, SetStage. |
| 0xBE60–0xBF70 | Guardian Decrement: increment byte before SetStage, deliver ExitState=10, FSM Flinch; unsigned (count-stage)/count times initial HP. Stack-command destructor null arm at0xBEA8 is impossible. |
| 0xBF74–0xBFA0 | Alert resets detection timer/sets alerted; Reset clears alerted/topHit and sets timer=leash. |
| 0xBFA4–0xC000 | Start/Stop modify +0xa74 bit0x80; SetToMax copies max scale and sets bit0x20; SetToZero uses rodataC0=1 and clears bit0x20. |
| 0xC004–0xC034 | Every message reaches CPatterned::AcceptScriptMsg, then epilogue. |

### Full block audit: targeting

| function / block | result |
|---|---|
| PredictAimOffset 0x481C–0x48CC | Prologue; effective morphed iff spawned state==0 and transition state==1. Morphed uses ball surface-to-world, otherwise player's transform. |
| 0x48D0–0x4968 | Radius=mAimSpreadRadius*Float; angle=rodataF4=6.2831855*Float. Rotate radius*player forward about normalized player up via quaternion and matrix. |
| 0x496C–0x49A4 | Repeat effective morph test; only non-boosting morphed player predicts motion; others return spread. |
| 0x49A8–0x4A54 | Virtual player aim at time0, GetGravity, PredictFlightTime(height,gravity,from,aim), PredictMotion(time*predictionScale), copy CMotionState and add its displacement to aim. |
| 0x4A58–0x4AD8 | One-time filter init: include 1ull<<59 (data618=Solid), exclude (1ull<<20)|(1ull<<33) (data61C=ProjectilePassthrough, data620=Character), filter type3. |
| 0x4ADC–0x4BA4 | Distance/normalized direction to predicted point; invalid hit uid, empty near list, BuildNearList excludes this actor; RayWorldIntersection, copy result, check valid byte. |
| 0x4BA8–0x4C68 | Valid hit returns hitPoint-aim+spread; invalid returns displacement+spread; other morph/boost states return spread; epilogue. No floating compare/clamp to alter. |
| PickTargetPosition 0x59BC–0x5A5C | Prologue; guardian branch copies player translation as fallback and reserves waypoint-count vector storage. |
| 0x5A60–0x5AD0 | Each uid must cast to waypoint and be active. Radius=0 unless stage count nonzero and index valid, then stage+0x18 spread radius. |
| 0x5AD4–0x5B84 | Two Float draws per active waypoint: random radius and angle2*pi. Rotate forward-radius about Z, add waypoint xyz, append. |
| 0x5B88–0x5BF4 | Zero candidates retains player translation; one uses index0 without RNG; multiple use unsigned Next()%count. Return xyz and destroy positions vector. |
| 0x5BF8–0x5C74 | Ordinary effective morph test chooses zero base offset vs mAttackAimOffset. Prediction arm is dead as proven above; predicted offset is zero. |
| 0x5C74–0x5D20 | Virtual player aim(time0)+predicted+base, then epilogue. |

PredictAimOffset's only uncovered host edge is the `__cxa_guard_acquire` false return after
the initial guard-byte test found uninitialized. The filter's only initializer is this
function; its sole live production caller is Sporb Think on the game-update thread (the
old ordinary targeting call is now non-PC only). No concurrent or recursive initialization
occurs, and the initializer consists of material-list bit operations. Both first-use and
already-initialized paths ran; the compiler's contention path is not a retail arm.

### Coverage and assertions

Spec: `tools/cov/specs/sporb_think_msg_arms.py`. Its labels include the retail offsets and
inputs, so assertion evidence is readable without fragile scenario-number references.
Forced Think calls temporarily bypass CPatterned::Think to keep the selected state stable;
forced message calls bypass only the common CPatterned::AcceptScriptMsg tail. Natural
updates/messages in both runs execute those base calls. Child state/inputs are restored;
animation/FSM/particle side effects are exercised, not mocked. Flinch traces the real
AddAdditiveAnimation recipient. Ray scenarios execute the real query, then override only
its returned validity/point. Ordinary targeting asserts zero calls to PredictAimOffset.

| body | lines / nonexception edges | evidence |
|---|---|---|
| Think | 113/113, 107/108 | All live arms; sole dead burst predicate above. Inactive; detection inside/outside/NaN; missing/wrong-type children; all12 states; dead top; flinch blends1/.5/NaN; blend clamp .5/1.5/NaN; freeze relay with base frozen/unfrozen and projectile present/absent; passable/boost combinations; guardian stage count/index/counters and equal/ordered/reversed ranges; natural live/dead emitter updates. |
| AcceptScriptMsg | 80/80, 48/48 | ALL-ARMS. Natural load/create plus every message in the switch; empty/invalid Approach connection; missing children; guardian stage guards; equal/ordered/reversed double-shot bounds; Decrement stage0/3/4/255 asserts health .75/0/1073741824/1 with initialHP=1. |
| PickTargetPosition | 36/36, 39/39 | ALL-ARMS. Both actor kinds; all effective-morph selectors and both track arguments; no/one/three active waypoints; missing waypoint cast; empty/out-of-range stage; natural inactive waypoint arm. |
| PredictAimOffset | 35/35, 23/24 | All live arms; only host static-guard contention edge above. Three morph-selector combinations, boosting, forced valid/invalid ray. Zero spread/prediction-scale yields zero miss offset or asserted hit offset(1,2,3). |

Hall: **65 calls, 63 assertions**. Guardian: **72 calls, 70 assertions**. Four other calls
allocate/free plain message scratch; **133/133 assertions PASS**, no forcing errors.
Both exit normally, caught/reach-stub=0, 1800/3200 frame limits:

```sh
tools/games_running.sh --wait
BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py SECS=300 tools/cov/force.sh \
  tools/cov/specs/sporb_think_msg_arms.py /tmp/opencode/sporb5-final-hall \
  3dfd2249,0e_swamp_hall --con tools/drivers/sporb4_hall.con --fast --frames 1800
tools/games_running.sh --wait
SPORB5_GUARDIAN=1 BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py \
  SECS=300 tools/cov/force.sh tools/cov/specs/sporb_think_msg_arms.py \
  /tmp/opencode/sporb5-final-pbg 3dfd2249,11_swamp_bloggtank_dark \
  --con tools/cov/specs/sporb_pbg.con --fast --frames 3200
python3 tools/cov/gcov_report.py build-port-cov /tmp/opencode/sporb5-final-hall/gcov \
  /tmp/opencode/sporb5-final-pbg/gcov --module Sporb \
  --fn fn_76_AF8C --fn fn_76_B9E4 --fn fn_76_481C --fn fn_76_59BC
```

### Gates / static findings

- Host/coverage links: 0 FAILED; manifest check rc0; TARGET_PC strip-diff vs31baf7af4 clean,
  ignoring markers; configure+ninja and dtk hash check: **87 files OK**. All77.98% fuzzy /
  54.39% matched, Game Code94.14% /63.29%.
- asmcheck BIT/NOPOS findings: stage byte +0xa04 is shifted by3 to index 8-byte retail
  ownership_transfer entries, not eight independent bit tests. +0xa74 bits0x80/0x40/0x20
  map to mActive/mAimLocked/mTimerScaleMaxed. CEntity +0x20 bit0x80 is GetActive; the
  actor-specific mActive is a different field. Each mapping was checked in the blocks above.
- CALL findings are real implementations/implicit C++ operations: fn_76_9914 copies
  CDamageVulnerability (21 byte entries, words+0x18/+0x1c, nested remainder); the Immune/
  ImmuneRicochet names are tool aliases for ImmuneVulnerabilty/ReflectVulnerabilty;
  internal_dereference is string destruction. fn_76_D854 reserves vector<CVector3f>,
  fn_76_5D24 destroys it; waypoint TCastToConstPtr uses the same retail cast. fn_76_4D04
  copies13 floats (CMotionState), fn_76_4C6C copies CRayCastResult including validity.
- retail_diff: three implicit vector/copy calls above and two 6.2831855 constant findings,
  both `2.f * M_PIF`. No unexplained finding. review_decomp_diff: four MARK lines, no ASM
  lines for this upstream file; explicit per-function asmcheck supplies the audit instead.

### Remaining 31 bodies after the first four-function checkpoint

These are **not certified by this checkpoint**: no completed full audit/all-live-arms proof.
Keep prioritizing the runtime bodies and then the property/initialization branches. The
inventory's historical marker column was not used or updated.

| id | body |
|---|---|
| 2858 | LoadSporbBase |
| C130 | CSporbBase constructor |
| FE64 | LoadSporbNeedle |
| 10680 | CSporbNeedle::Explode |
| F494 | CSporbProjectile constructor |
| DE60 | LoadSporbProjectile |
| 9304 | CSporbBase::FakeDeath |
| 55A4 | CSporbBase::DoUserAnimEvent |
| 3B8 | LoadSporbTop |
| 44BC | CSporbBase::ShootPowerBomb |
| F084 | CSporbProjectile::Think |
| 4D70 | CSporbBase::CreatePowerBomb |
| E48 | CSporbTop::ScanVisorRender |
| A898 | CSporbBase::Attack |
| 5388 | CSporbBase::GetTopAttachPosition |
| 7000 | CSporbBase::AttachPlayer |
| 403C | CSporbBase::ScanVisorRender |
| E358 / E4D4 | CSporbProjectile::PlayBallEscapeEffect / PlayBallSpitEffect |
| 915C | CSporbBase::ShouldAttack |
| 102CC | SLdrSporbNeedle constructor |
| AA88 | CSporbBase::Patrol |
| 10D24 | CSporbNeedle::Render |
| 3C50 / 26A8 | CTendrilGen constructor / destructor |
| 2804 | SetFuncPtrs |
| 5558 | CSporbBase::GetLeanDirection |
| 2E8 / DE58 | Top / Projectile IsScanVisorSelfRender |
| DE50 / 10468 | Projectile / Needle GetCollisionPrimitive |

## sporb5 continuation: state transitions, animation events and attachment (2026-10-24)

Nine further bodies audited/marked, bringing this slice to **13 new markers**, module
**151/173**. The first checkpoint is f9310cbf7. Source base remains31baf7af4.

### Three further corrections

| function | defining retail instructions | correction |
|---|---|---|
| Attack A898 | 0xAA48 `lbz r3,0xa10(r31)`; 0xAA4C `lbz r0,0xa11(r31)`; 0xAA50 `addi r3,r3,1`; 0xAA54 `cmplw r0,r3`; 0xAA58 `bge 0xAA60`; 0xAA5C `mr r3,r0`; 0xAA60 `stb r3,0xa10(r31)` | Clamp the full incremented counter before storing a byte. The old uchar conversion wrapped255+1 to0 before the clamp. Now255/3 yields3 and255/255 yields255. |
| DoUserAnimEvent 55A4 | 0x57CC `lbz r0,0x9ac(r28)`; 0x57D0 `cmplwi r0,0`; 0x57D4 `beq 0x5884` | The **whole** projectile-event action is guardian-only, including locator lookup, ShootPowerBomb and increment. Ordinary actors skip directly to the common CPatterned tail; they must not dereference their null ProjectileInfo. |
| GetLeanDirection 5558 | 0x5564 `fsubs f3,f4,f3`; 0x556C `fsubs f1,f1,f2`; 0x5580/5584/5588 `stfs` xyz; 0x558C `bl AsNormalized` | Input f1/f2/f3/f4 are forward/back/left/right. Normalize `(right-left,forward-back,0)`, not `(back-left,forward-right,0)`. Distinct-axis input(1,2,3,7) now produces(4,-1,0)/sqrt17. |

### Complete block audits

| body / block | audit |
|---|---|
| Patrol AA88–AABC | Prologue; only Activate=0 enters local state changes; other messages go to common tail. |
| AAC0–AAF8 | Set base state1, locomotion1(Relaxed); resolve top+0x7c4, successful cast sets top state3. |
| AAFC–AB20 | Resolve projectile+0x7c6; successful cast sets projectile state6. |
| AB24–AB54 | Every message calls CPatterned::Patrol with original msg/dt, then epilogue. |
| Attack A898–A8C4 | Only Activate=0 acts; all other messages return. |
| A8C8–A928 | Base state2, locomotion2(Lurk); optional top state4, optional projectile state7. |
| A92C–A984 | Ready byte+0x888=1, windup+0x860=0, clear +0xa74 bit0x40(aimLocked). Range requires inside maxGrab and outside minGrab; calls preserve short circuit. |
| A988–A9F0 | Effective morphed iff spawned0 and transition1. Grab only when in range, shared grabbing uid invalid and non-guardian: attackType1, grabberState4; else attackType0. |
| A9F4–AA44 | Guardian, nonzero stage count, unsigned index<count, burst!=2, stage doubleShotChance>0. `fcmpo; ble` skips zero/negative/NaN. |
| AA48–AA84 | Full-width counter increment/clamp then byte store as corrected; reset powerBombsFired byte+0xa18=0; epilogue. |
| FakeDeath 9304–9344 | Dispatch Activate0, Update1, Deactivate2; all other values do nothing. |
| 9348–9434 | State6, animation Ready1; CBCGenerateCmd type15, generate6, targetzero, anim-1, flags all false. DeliverCmd then copy complete derived command; locomotion7. |
| 9438–9474 | Optional projectile: clear passable+0x7ec, DeleteObjectRequest(projectile uid). |
| 9478–94A8 | StopTendril; SendScriptMsgs EXIT=0x45584954, player uid, message-1(None). |
| 94AC–94F0 | Only if shared grabbing uid==this uid: set player+0x1144 bit0x80 (leave-morph), SetBallBoostState0(Available), clear shared uid. |
| 94F4–9518 | UnFreeze; copy Black RGB only to actor+0x44c/44d/44e, preserving alpha. |
| 951C–9604 | Update: CanIssueCommand(controller,Generate12), if true deliver/copy same Generate6 command. |
| 9608–9628 | Deactivate animation NotReady0; epilogue. |
| ShouldAttack 915C–91C8 | Get area's loaded phase16; unloaded treated occluded0, loaded consults postconst+0x13c; occluded returnsfalse. |
| 91CC–91F8 | Returnfalse unless alerted+0x909 and shared grabbing uid invalid. |
| 91FC–9234 | Range inside maxAttack+0x7e0 and outside minAttack+0x7e4, same short circuit as source. |
| 9238–92A0 | Optional top: return inRange OR topHit+0x94f; missing top returns just inRange; epilogue. |
| DoUserAnimEvent 55A4–55F0 | Dispatch event8(DamageOn),0(Projectile),31(SoundPlay), all others common tail. |
| 55F4–560C | SoundPlay dispatch attackType1(Grab),0(Shoot), others skip. |
| 5610–569C | Grab sound+0x94c; ProcessSoundEvent virtual slot0x78 with volume1, .1, maxdistance1000, zero segment/flags/time, priority20, volume127, camera distance, translation, next-area id, mgr, true. |
| 56A0–5738 | Shoot sound+0x94a with identical args unless state11(Flinching). |
| 573C–57C8 | DamageOn only in state8(Flailing): ApplyDamage(this,player,this,damage+0x83c,includeSolid/excludeEmpty,type3,zero vector); data358=59 means1ull<<59. |
| 57CC–57FC | Projectile ordinary actor skips to common tail as corrected; guardian with maxed bit0x20 clear copies +0xa50 aim xyz into +0x914. |
| 5800–5834 | Maxed guardian with fired byte+0xa18!=0 picks new target(false); first shot retains old target. |
| 5838–5880 | Guardian GetLocatorPosition; ShootPowerBomb(height+0x9d8,locator,mgr,4,aim), increment fired byte. |
| 5884–58B4 | All event types, including skipped ordinary projectile event, call CPatterned::DoUserAnimEvent with unchanged node/type/dt, then epilogue. |
| AttachPlayer 7000–707C | Prologue; invalid projectile uid or failed cast skips whole action. |
| 7080–7110 | Save player translation; virtual aim slot0x54(time0); copy projectile transform; lookup scaled `ball_attach_LCTR` and get its translation; destroy temporary string. |
| 7114–71A8 | World=projectileXf*locator; SetTranslation(world-(aim-playerPosition)); epilogue. |
| GetLeanDirection 5558–55A0 | Prologue, corrected axis differences, rodataC4 zero Z, AsNormalized; epilogue. |
| GetTopAttachPosition 5388–53E8 | Prologue; require valid top uid, successful cast, alive bit0x40 at top+0x420; otherwise return zero. |
| 53EC–5478 | GetLeanDirection(forward/back/left/right); unit vectors from saved topAttachXY/zeroZ and lean; LookAt with angle rodataF4=6.2831855 (2*pi). |
| 547C–5518 | Build rotation matrix; rotate top orbit(+0x7c4)-base translation, then add base translation; store xyz. |
| 551C–5554 | Failed guard returns zero xyz; common epilogue. |
| Projectile Think F084–F14C | Prologue; actorXf*scaled `ball_attach_LCTR`, extract locator position; MoveCollisionPrimitive(locatorPos-actorPos). |
| F150–F1C0 | Always CPatterned::Think; player aim(time0); effective morph selector spawned0/transition1; require projectile state0(Launch). |
| F1C4–F21C | Solid phase0: squared distance<=radius² via `fcmpo; cror eq,lt,eq; bne skip`; NaN skips. Remove Solid59, set phase2(BallInside), passable1. |
| F220–F260 | Otherwise only phase2: squared distance>radius² via `fcmpo; ble skip`; NaN skips. Set phase0, add Solid59, passable0. Phase1(Landed) unchanged. |
| F264–F29C | Epilogue. |

### Runtime arm evidence

Spec `tools/cov/specs/sporb_runtime_arms.py`, same hall/guardian driver and run environment
as the earlier checkpoint; substitute `sporb_runtime_arms.py` for `sporb_think_msg_arms.py`
and use `/tmp/opencode/sporb5-runtime-final-{hall,pbg}`. Hall1800 frames, guardian3200.
**67/65 calls, 65/63 assertions respectively; 128 PASS, zero FAIL/forcing errors.**
All nine bodies have every nonexception arm taken; no dead-edge waivers:

| body | lines | edges | forced cases plus natural caller coverage |
|---|---|---|---|
| Patrol AA88 | 11/11 | 6/6 | msgs0/1/2/3; children present/missing; common base call runs. |
| Attack A898 | 30/30 | 32/32 | msgs0/1/2/3, child guards, each effective morph arm, outside/inside-min range, shared grab owner, both actor types, all stage guards, chancezero/NaN, counter0/3/255 versus limit3/255. |
| FakeDeath 9304 | 28/28 | 10/10 | all message arms; update ready/not-ready; absent/live projectile; shared owner self/notself; alpha123 retained with RGBzero; real deferred deletion last in batch. |
| ShouldAttack 915C | 9/9 | 16/16 | unloaded/loaded, occluded/visible, alerted false, grab busy, outside/too-close/inrange, top present/absent, topHit outside. |
| DoUserAnimEvent 55A4 | 33/33 | 17/17 | each event/default; sound attack-1/0/1, flinching/nonflinching; damage flail/nonflail; ordinary projectile asserts **zero ShootPowerBomb calls and unchanged fired count**, guardian maxed/unmaxed and first/later shots assert one call and countincrement. |
| AttachPlayer 7000 | 14/14 | 5/5 | invalid id, wrong type, actual projectile locator and player translation. |
| GetLeanDirection 5558 | 2/2 | 0/0 | asymmetric independent vector oracle(4,-1,0)/sqrt17. |
| GetTopAttachPosition 5388 | 12/12 | 10/10 | invalid/wrong-type/dead top, live aligned vectors assert returned top orbit. |
| Projectile Think F084 | 24/24 | 20/20 | each morph selector, launch/nonlaunch, phases0/1/2, radiuslarge/zero/NaN; phase assertions verify entry/exit and unordered skips. |

Forced event calls use nonnull scratch for the **unused** node reference and bypass only the
common CPatterned event tail; natural events execute the real tail. Forced projectile Think
bypasses only CPatterned::Think to keep selected state stable; natural updates execute it.
Other functions run their real callees. Scalar state/inputs restore between scenarios;
command, audio, damage, material and delete effects are real. Forced cases are identified
by offset labels in `forced.log`. Both runs exit0, caught/reach-stub0, console plans complete.

### Gates and static findings

- Host and coverage links0 FAILED; manifest rc0; non-PC strip-diff versus31baf7af4 clean
  for both upstream files; configure+ninja and dtk hash check **87 files OK**.
- asmcheck: Attack stage-byte shift at0xAA24 indexes 8-byte ownership_transfer entries,
  not eight independent flag tests. +0xa74 bit0x40 is aimLocked; bit0x20 is timerScaleMaxed.
  FakeDeath `ShouldRetry__11CTryCommand` is the reconstructed `CAnimationState::CanIssueCommand`.
  AttachPlayer/Projectile Think `internal_dereference` is string destruction;
  GetTopAttachPosition's const top cast uses the retail TCastToPtr implementation.
  ShootPowerBomb44BC has a real body at CSporbBase.cpp; it remains unmarked pending its
  own all-arms evidence. The tool's “unported” means no marker, not a missing call.
- retail_diff: only that unmarked ShootPowerBomb call and 6.2831855=`2.f*M_PIF` in
  GetTopAttachPosition. No unexplained finding. Diff has no production probes.
- review_decomp_diff versus31baf7af4:13 MARK lines and one NULL heuristic hit. The latter
  is the boolean `!mIsPowerBombGuardian` at0x57D4, not a pointer guard; its retail branch
  is quoted above. No ASM lines (upstream files); explicit asmcheck covers all13 bodies.
- Tests limited to these changed/marked bodies and their hall/guardian callers; no other
  family or whole-module sweep was run.

### Remaining 22 unmarked bodies at the continuation checkpoint

Inventory is from actual `// @` markers, not the historical TSV marker column:
2858 LoadSporbBase; C130 Base ctor; FE64 LoadSporbNeedle; 10680 Needle::Explode;
F494 Projectile ctor; DE60 LoadSporbProjectile; 3B8 LoadSporbTop;
44BC Base::ShootPowerBomb; 4D70 Base::CreatePowerBomb; E48 Top::ScanVisorRender;
403C Base::ScanVisorRender; E358/E4D4 Projectile::PlayBallEscapeEffect/PlayBallSpitEffect;
102CC SLdrSporbNeedle ctor; 10D24 Needle::Render; 3C50/26A8 CTendrilGen ctor/dtor;
2804 SetFuncPtrs; 2E8/DE58 Top/Projectile IsScanVisorSelfRender;
DE50/10468 Projectile/Needle GetCollisionPrimitive.

Needle::Explode10680–10A9C was read against retail with no mismatch found, but remains
**unmarked**: the normal hall run did not spawn any needles, so no runtime proof yet.
The previous sporb4 `sporb_shoot_aim_arms.py` spec's forced UpdateShooting calls generate
real needles and are the starting point for that continuation. Its owner guards include
collision-actor owner remapping; test those as well as direct/radius damage, trail-null,
already-exploded, zero/NaN radius and distance boundaries. Do not count the static read
alone as certification.

## sporb6: needles, bombs, rendering and effects (2026-10-24)

Base494a2ba23. Fifteen further bodies audited instruction-by-instruction with
`tools/asmcheck.py annotate Sporb 0x<off> --all-ins`. No production correction in this
checkpoint. Module **151 → 166/173**. Full annotated listings and compact runtime/static
evidence: `/tmp/opencode/reports/sporb6-evidence/`.

### Full block audit

| body / blocks | retail correspondence |
|---|---|
| Explode10680–1071C | +330 bit80 already-exploded exits; remove Solid59, set bit, explosion emitter (area,true,false,medium), explosion emission=true, optional trail emission=false. |
| 10720–107F4 | Compare hit uid to owner; successful CCollisionActor cast **replaces** ownership comparison with collision-owner uid. Noninvalid nonowner takes ApplyDamage(needle,hit,needle), original damage, include1ull<<59/exclude0/type3/zero direction. |
| 107F8–108E0 | `fcmpo radius,0; ble` means strict positive, skipping NaN. Only invalid hit uid reaches radius path. Bounds=position±radius; include Character33/Player32, filter type1, no ignored actor; BuildNearList. |
| 108E4–10958 | Iterate every uid; direct/collision-owner remap again, skip owner, ObjectById null skips. |
| 1095C–10A9C | Euclidean distance; `fcmpo distance,radius; bge` means strict less, skipping equal/NaN. Scale=(radius-distance)/radius; damage and knockback scaled, same radius, both damage flags cleared, sound idsFFFF; ApplyDamage as above, loop/epilogue. |
| ShootPowerBomb44BC–4590 | One-time tick-time initialization; virtual ProjectileInfo slotDC; cached token accepted, otherwise lock and loaded-object guards, GetObj/cache; unsuccessful cache exits. |
| 4594–4694 | CanCreateProjectile(owner,AI11,count); GetGravity/PredictFlightTime; velocity=(dx/time,dy/time,-dz/time+.5*g*time), Translate(from), real CreatePowerBomb, nullable result guard. |
| 4698–4734 | SetVelocity(tick*velocity), SetGravity(0,0,-g*tick), AddObject, epilogue. |
| CreatePowerBomb4D70–4E88 | Allocate uid; allocate native-size bomb; active=true, AI11, excludeCharacter33, this owner, invalid homing, attrs0, underwaterfalse, scaleOne, no visor effect, unusedfalse/playImpacttrue; fuse/start/end/wait fields. Retail nullable allocation path is host throwing-new ABI (CMemory.hpp:62–63); no successful null return from allocation. |
| 4E8C–4F08 | Emitter parms(maxDistance,distanceComp,1,128,20), transform translation, zero direction, configured sound/area,true,false,medium. |
| 4F0C–4F84 | Iterate emitter slots, first handlezero **and** invalid projectile uid receives new handle/uid; otherwise exhaust loop. Return bomb. |
| Base ScanVisorRender403C–410C | Save full transform/flags, install supplied values, capture scanning uid, CPatterned::Render, restore. |
| 4110–41C4 | Only scanning self and state!=7(FakeDead),!=0(Sleeping): supplied transform times scaled connect locator; optional top renders with same flags. |
| Top ScanVisorRenderE48–F50 | Save/install transform/flags, base render; optional projectile model renders with supplied xf/flags and its actor lights; restore top. |
| F54–1044 | Scanning self or any successfully-cast Sporb projectile: optional base, scaled connect locator inverse, base scan render(xf*inverse,flags). Retail eagerly casts the scan uid; source short-circuits the side-effect-free lookup for self. |
| Needle Render10D24–10DE0 | Require not exploded, model pointer and nonnull model contents; model Render(mgr,xf,null lights,opaque alpha1). |
| EscapeE358–E4D0 / SpitE4D4–E64C | Format SPORB_TOP_EFFECT%d-%d with asset id and **old** byte counter; increment wraps. Lookup ball_attach_LCTR; invalidFF maps to0. Particle data time0, PART asset, scale1; escape mode1(continuous emitter), spit mode2(continuous system). AddParticleEffect(hash,64,data,model scale,mgr,area,false,0). |
| CTendrilGen3C50–3CE0 | CElementGen(token,normal0,one1), derived vtable, copy all six max-inverted bounds floats. |
| SetFuncPtrs2804–2854 | Static table base/ projectile/needle/top at offsets0/4/8/C, call SetSSporb_FuncPtrs; no conditions. |
| SLdrSporbNeedle102CC–103B4 | Editor/actor/damage default ctors; model/trail/explosion invalidFFFFFFFF, sounds all-1; ambient white1, visor flags15, speed60, mass1, weapon9, damage5, knockback2, fuse1.5. Rodata380/384/388/38C/390=1/60/5/2/1.5. Existing TARGET_PC corrections already supply white/visor/sound defaults. |
| Getters2E8/DE58/DE50/10468 | Top/Projectile self-render returntrue; Projectile/Needle collision primitive return member+7C8/+310 (host uses field address). |

### Forced coverage / oracles

`tools/cov/specs/sporb_needle_arms.py`: capture an ordinary shooting base without stopping;
at a live CActor::Render entry, real UpdateShooting creates the needle. Allocate, construct
and register a real CCollisionActor. Real ApplyDamage/emitter/model/particle callees execute.
The radius broadphase executes before its result is replaced with owner, collision actor,
player and stale uid (or empty), making ownership/null/boundary cases reproducible.
GetLocatorSegId executes before overriding its returned byte toFF. State overrides restore;
damage, particle effects and deferred fixture deletion are real. Scan tests compare every
byte of both actors' transforms/flags before and after each call.

| body | lines / edges | exercised cases |
|---|---|---|
| Explode10680 | 43/43,24/24 | already exploded, present/null trail, owner/direct player/collision owner/other collision owner, invalid hit, radius0/-1/NaN, empty/candidate near list, stale uid, distance0/1/2/3/NaN with radius2. Damage/knockback oracle8/4 at center,4/2 at distance1, none at boundary/outside/NaN. |
| Needle Render10D24 | 4/4,6/6 | exploded, absent model pointer, present empty model, normal model. |
| Base/Top ScanVisorRender403C/E48 | 14/14,12/12;20/20,13/13 | scan none/self/projectile; base states0/7/2; child/base present/missing; nine exact state-restoration assertions. |
| EffectsE358/E4D4 | each10/10,10/10 | natural and forced-invalid locator; old index255 wraps to0, real particle effects. |
| CTendrilGen3C50 / SetFuncPtrs2804 | 2/2,1/1;8/8,0/0 | natural hall loading. |
| SLdrSporbNeedle102CC | 26/26,3/3 (header gcov) | natural needle loading/spawning, complete initializer execution. |
| Getters2E8/DE58/DE50/10468 | each1/1,0/0 (header gcov) | explicit calls assert true/member-pointer results. |
| ShootPowerBomb44BC | 19/19,10/11 | cached/uncached, unlocked/locked-unloaded/loaded, quota0, success, forced null CreatePowerBomb return; ballistic velocity/gravity assertions at height2,g9.81,dx3,dy4,dz0. |
| CreatePowerBomb4D70 | 17/18,10/10 | zero slots, free first, busy handles, busy ids, mixed/free last. Unexecuted source line is exception cleanup; nullable retail allocation is replaced by standard throwing new. |

ShootPowerBomb's sole untaken edge is host `__cxa_guard_acquire` returningfalse after the
guard byte read uninitialized. Only this initializer writes the static; all callers are on
the game thread, GetTickTime is nonrecursive. First-use and initialized paths ran. This
contention edge is not a live retail arm. The forced null-result case bypasses only
CreatePowerBomb; all ordinary construction cases execute its actual body.

Reproduce with `BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py SECS=300`
and `tools/games_running.sh --wait` before each run:

```
tools/cov/force.sh tools/cov/specs/sporb_needle_arms.py <hall-out> \
  3dfd2249,0e_swamp_hall --con tools/drivers/sporb4_hall.con --fast --frames 1800
tools/cov/force.sh tools/cov/specs/sporb_bomb_arms.py <guardian-out> \
  3dfd2249,11_swamp_bloggtank_dark --con tools/cov/specs/sporb_pbg.con --fast --frames 3200
```

Both final candidate runs exit normally, caught/reach-stub0, no force failures. Do not trust
force.sh rc0 alone: force_ret's default batch hook services only one stopping CALLS entry;
a second stops the inferior without a gcov exit flush. This spec deliberately uses one
stopping entry for its complete suite. No shared-tool changes.

Checkpoint gates: both direct ninja host/coverage links0 FAILED; manifest rc0; configure+
ninja All77.98% fuzzy/54.39% matched, Game94.14%/63.29%; dtk hash check87 files OK.
All eight touched upstream C++/header files are **marker-only**, strip-diff CLEAN.
Final runs `sporb6-checkpoint-hall`1800 / `sporb6-checkpoint-pbg`3200:41/10 calls,
33/16 PASS assertions, exitnormal, caught/reach-stub0. Raw header coverage proves the
five inline bodies in addition to the ten rows in coverage-checkpoint.txt.

Static findings: asmcheck cannot parse in-class CTendrilGen/getter bodies (MISSING); their
complete short listings were checked manually above. Needle +330 bit80 is mExploded;
CModelFlags constructs the white CColor. TCastToConstPtr uses retail's same cast; implicit
string destruction accounts for internal_dereference. CAnimData::GetLocatorSegId delegates
the character-layout lookup; TryCache calls CToken::GetObj; ProjectileInfo is virtual slotDC.
CreatePowerBomb's4F88/5334 are CImpactVisorEffect::None and its destructor, with three empty
optionals (4F88–4FB0) and the corresponding optional cleanup (5334–5358); real host code is
in Weapons/CImpactVisorEffect.hpp. SLdrSporbNeedle's XSTORE findings misread property hashes
as member offsets; every actual store is mapped in the102CC audit. Its three constructor
CALL findings are ordinary member initializers. retail_diff reports only the two visor
effect aliases. No unexplained finding and no production probes.

### sporb6 projectile constructor checkpoint

`F494` is now audited and certified: **166 → 167/173** (six remain). This checkpoint
corrects the host collision box's Y/Z half-extents under TARGET_PC. All other constructor
behavior remains as upstream; in particular `mCarryingBall` is a separate byte, not part
of retail's `stb r0,0x7ec(r29)` passable initialization.

| blocks | full retail correspondence |
|---|---|
| F494–F51C | CPatterned(AI0x49,uid,name,flavor0,info,xf,model,patterned,movement1,collider1,body5,actorParams). |
| F520–F5D8 | Derived vtable; state-1, solid phase0. Radius chooses halfExtent unless it is strictly less than height*.5 (F550 `fcmpo`, F558 `bge`; unordered selects halfExtent). Sphere origin=patterned+AC/B0/B4; material=actor+68. Escape radius=2*radius; passable0, two effect ids, byte index0, invalid top uid; shadowfalse. |
| F5DC–F684 | Include clears Character33/Player32, adds ProjectilePassthrough20/CameraPassthrough21 (`lis r4,0x30` gives0x300000); exclude=Character33/Player32/AIPassthrough28, filter type3; sphere material gets resulting include mask. |
| F688–F6F4 | Base box deltas X=max+C-min+0, Z=max+14-min+8, Y=max+10-min+4, each*.5. `F6CC stfs f4,0x90(r1)`, `F6D8 stfs f5,0x94(r1)`, `F6EC stfs f2,0x98(r1)` store **X/Z/Y**, with negatives at84/88/8C; SetBoundingBox. Upstream used X/Y/Z: corrected only for TARGET_PC. |
| F6F8–F77C | DisabledAnimationDeltas=3 via +422 mask30. Reserve2 asset vector. Independently append each effect id unlessFFFFFFFF (`addis1; cmplwiFFFF`). |
| F77C–F878 | ParticleResData(particles,five empty vectors); AnimationData's ParticleDB::CacheParticleDesc, destroy resource data and all six vectors, return this. |

Forced evidence: `tools/cov/specs/sporb_projectile_ctor_arms.py` observes **natural** loader
constructors. After the real CPatterned ctor it temporarily supplies halfExtent/height;
GetBaseBoundingBox temporarily supplies asymmetric half-extents(1,2,3). The finish
breakpoint checks radius and all six bound coordinates, then restores original finite
gameplay inputs and the corrected original box. No duplicate model ownership or mocked
constructor. Every actual particle cache call executes.

| case | halfExtent,height | spit,escape valid | expected radius | outcome |
|---|---|---|---|---|
| 0 | 2,6 | yes,yes | 3 | 2/2 PASS |
| 1 | 4,6 | no,yes | 4 | 2/2 PASS |
| 2 | 3,6 | yes,no | 3 | 2/2 PASS |
| 3 | 2,6 | no,no | 3 | 2/2 PASS |
| 4 | NaN,6 | no,no | NaN | 2/2 PASS |
| 5 | 2,NaN | no,no | 2 | 2/2 PASS |

All cases require box min(-1,-3,-2), max(1,3,2). The clean pre-fix case0 run returned
(-1,-2,-3)/(1,2,3) and failed both assertions. Fixed runs0–5 each exit normally after1200
frames in `3dfd2249,0e_swamp_hall`, caught/reach-stub0. Combined gcov **34/34 lines,
25/25 edges ALL-ARMS**, including both radius arms and each independent asset guard.

Reproduce each case (wait for a game slot first):

```sh
SPORB6_CTOR_CASE=<0..5> BUILD=$PWD/build-port-cov \
  COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py SECS=300 \
  tools/cov/force.sh tools/cov/specs/sporb_projectile_ctor_arms.py <out> \
  3dfd2249,0e_swamp_hall --fast --frames 1200
```

Evidence has `F494.asm`,
`coverage-ctor.txt`, six `ctor-*-summary.log`, and `profiles-and-run-logs.tar.gz` containing
the Sporb gcno/gcda files and complete final run logs, including the failing pre-fix run.

Remaining uncertified bodies (no markers added): `2858 LoadSporbBase`, `C130 CSporbBase`
constructor, `FE64 LoadSporbNeedle`, `DE60 LoadSporbProjectile`, `3B8 LoadSporbTop`,
`26A8 CTendrilGen` destructor. All six full assembly listings are saved; Base constructor
and Base loader still need complete audit. Loader early returns/optional paths, Base
constructor branches and implicit destructor still need complete runtime certification.

Final gates: direct host/coverage links rc0, 0 FAILED; manifest rc0; matching configure+
ninja unchanged percentages, explicit dtk shasum **87 files OK**. All eight touched
upstream files pass non-PC byte comparison after removing markers and selecting the new
TARGET_PC block's original else path. Final builds differ from the six passing constructor
runs only by the marker and corrected assembly-offset comment; no repeat smoke needed.

F494 static findings are accounted for: `mDisabledAnimationDeltas` NOPOS is the explicit
+422 mask30 in F700; CCollisionPrimitive is CCollidableSphere's base initializer
(`include/Collision/CCollidableSphere.hpp:9`). FAE4 reserves a four-byte-element vector;
F910 constructs six consecutive vectors, F87C destroys those six, and FA90 frees a vector.
They are the existing `rstl::vector<CAssetId>` and `CCECharacterInfo::CParticleResData`
implementations, not missing callees (all four full listings saved). Final review has
16 MARK lines, all justified in the two audit/coverage tables, with no other findings.
