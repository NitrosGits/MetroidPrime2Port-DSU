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
| UpdateGrabber | 237/245 lines; Idle-with-projectile arms, max-range clamp in Extend, tendril-gen absent |
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
