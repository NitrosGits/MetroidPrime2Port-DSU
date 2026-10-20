# SwampBossStage1 / SwampBossStage2 (Chykka Larva / Chykka, Torvus Bog)

Modules 78 (`SwampBossStage1`, script type `SBS1`) and 79 (`SwampBossStage2`, `SBS2`). Arena:
Torvus Bog `04_swamp_temple_dark` — Stage1 on layer 3, Stage2 on layer 6 (`docs/module_spawns.tsv`).

Pseudo-C, one file per function: `/tmp/opencode/pseudo/SwampBossStage{1,2}/fn_<id>_<off>.c`
(Ghidra via REA over the `rel_plf2exec` image; text at 0x81000000). `_callmap.txt` there lists each
function's callees. Read ranges; don't re-decompile.

## Stage1 (module 78) — verified

| What | Value |
|---|---|
| Entry | `fn_78_130` sets `SSwampBossStage1_FuncPtrs` (`lbl_78_bss_20`) to loader `fn_78_160` |
| Object | `__nw(0xdb8)`, ctor `fn_78_AA50`, dtor `fn_78_A5E4` |
| Property reader | `fn_78_838` (SLdr layout matches generated `SLdrSwampBossStage1.inc` field order) |
| Vtable | `lbl_78_data_510` |
| DoUserAnimEvent jump table | data+0x658 |

Vtable overrides: dtor A5E4, Think 95D8, AcceptScriptMsg 9B74, PreRender 69F8, AddToRenderer 2204,
Render 7AE4, PreRenderAllViewports 7890, GetHealthInfo C0, GetDamageVulnerability 69D8,
GetTouchBounds 8, GetOrbitPosition 2100, GetAimPosition 69AC, OnScanStateChange 21C8,
GetSortingBounds 785C, DoUserAnimEvent 86E8, GetScannableObjectInfo A570, CollidedWith 9A44,
TakeDamage 2078, CanBeShot 94, IsListening 0, Listen 9C, GetOrigin A4, GetModelAlpha 44,
ProjectileInfo 8004, GetSearchPath 4C, GetPointSearchPath 54, SetupStateMachine 5C,
GetAttackTarget 64, IsOnGround 74, GetGravityConstant 80, GetDeathExplosionParticle 8C.
Death and KnockBack are inherited. (Function names are `fn_78_<off>`.)

FSM2 tables (name → function, `fn_78_`):

| # | Trigger | fn | # | State | fn |
|---|---|---|---|---|---|
| 0 | BeachAttackOver | 653C | 0 | Beach | 4AA8 |
| 1 | EnoughSplashing | 5E00 | 1 | BeachBubbles | 5188 |
| 2 | GivingUp | 65F0 | 2 | BeachRise | 587C |
| 3 | HeavyHit | 65E8 | 3 | BeachWait | 5268 |
| 4 | InVomitRange | 624C | 4 | Bob | 6034 |
| 5 | LightHit | 6590 | 5 | Dead | 22C8 |
| 6 | MinHealth | 6548 | 6 | Dive | 5374 |
| 7 | ReadyToTongue | 67C8 | 7 | ExposeBelly | 3694 |
| 8 | ShouldBeach | 6358 | 8 | HeavyHitReact | 348C |
| 9 | ShouldBob | 64D0 | 9 | LightHitReact | 3320 |
| 10 | ShouldExposeBelly | 6458 | 10 | Null | 5304 |
| 11 | ShouldSplash | 63D8 | 11 | PlatformReady | 4984 |
| 12 | SplashAttackOver | 5CD4 | 12 | ResetPlatform | 38A4 |
| 13 | TongueHit | 66D4 | 13 | SetSplashOver | 5D6C |
| 14 | TongueMissed | 6638 | 14 | SlideIntoWater | 5784 |
| 15 | TooMuchPulling | 44B0 | 15 | SplashDescend | 5E84 |
| 16 | Wait | 4D1C | 16 | SplashJump | 5A88 |
| | | | 17 | SplashRise | 5984 |
| | | | 18 | SplashTelegraph | 5088 |
| | | | 19 | SplashWait | 4D3C |
| | | | 20 | StopBeaching | 4C90 |
| | | | 21 | Swim | 6198 |
| | | | 22 | TongueLoop | 31E8 |
| | | | 23 | TonguePull | 44CC |
| | | | 24 | Vomit | 4280 |

Code functions: IncrementMisses 25F4, SetSplashRotation 5C30, ShreddersOn 2590, ShreddersOff 252C.

### Stage1 host body (`src/MetroidPrime/Enemies/PortCSwampBossStage1.cpp`, 2026-10-20)

Behaviour port, not matching. `// @ fn_78_<off>` marks the 105 functions the traced LAYER3 runs
exercised (see the runtime status below); ported-but-unexercised bodies carry no marker. Unclear
reads are `// TODO(uncertain)`. `tools/modules_port.txt` points `SwampBossStage1` at this file.

**Members** (ctor `fn_78_AA50`; names in `CSwampBossStage1.hpp`): 0x7c0 `SLdrSwampBossStage1Data`,
0xafc three-deep state history (`fn_78_5560`/`55AC`), 0xb14 state id (−1 between states), 0xb18
render time, 0xb1c clock, 0xb20/0xb2c root/head locators, 0xb3c pain accumulator, 0xb40 sort bounds,
0xb58 scanned / radar-present bits, 0xb5a last hitter, 0xb80 `CCollisionActorManager`, 0xb84 phase,
0xb88 attack index, 0xb8c next-attack time, 0xb90 shock-wave info, 0xbc4 telegraph gen, 0xc0c..0xc40
tongue (mouth, target, tip, extend, state, pull time/speed, sway), 0xc44/0xc94 tongue/tip models,
0xd10 belly accumulator, 0xd14 heavy-hit latch, 0xd18 weak-spot vulnerability, 0xd58 saved player
step-up, 0xd60 ring gen, 0xd64 water id (`IS08` connection), 0xd7c `CProjectileInfo` (returned by
`ProjectileInfo` 8004), 0xda4 spit visor token.

**State ids (`xb14`, set by `RecordState` 5560)**: −1 SplashWait, 0 Beach, 1 Bob, 2 Dive, 3
ExposeBelly / hit reacts, 4 PlatformReady, 5 TonguePull, 6 SlideIntoWater, 7 BeachRise / Splash
Rise/Jump/Descend, 8 SplashTelegraph, 9 TongueLoop, 10 Vomit. `CanTakeHits` (A528) is true for 0,3,4,5,6,9,10.

**Constants**: the 16-joint sphere table is rodata+0x8 (`kJoints`: name, water radius, beach radius,
type 0 body / 1 belly / 2 tongue, flags). Joint mass and collision-actor HP are 10000 (8CD8/8954;
HP is reset after every hit in 9B74). Min health is 1.01 (A5A8). Other literals are inline in the
cpp with their function: tongue reach 85° / 100 units, vomit range 6 (morphed, XY) / 8.5, vomit
knockback ±12, give-up 8 s or 3 misses, pull limit 8 s, step-up 0.5, sort-bounds shrink 0.85.

**The joint table, re-read from the rodata** (`build/G2ME01/SwampBossStage1/asm/auto_03_*_rodata.s`,
`lbl_78_rodata_8`, 16 entries of 0x14 bytes: name, water radius, beach radius, type, flags):
head 5.0/5.5, tongue_LCTR_SDK 3.2/3.2 (type 2), L_upper_1 and R_upper_1 2.0/2.0, Spine_2 5.0/5.0,
Spine_3 5.0/5.0, Spine_4 4.1/4.1, Spine_5 3.7/15.0, Spine_6 3.0/15.0, Spine_7 2.5/2.5,
Spine_8 2.0/2.0, Spine_9 1.8/1.8, Spine_11 1.6/1.6, Spine_13 1.5/1.5, Spine_15 1.5/1.5 and
L_middle2_boob_LCTR 2.5/0.1 (type 1, the belly - its *beach* radius is 0.1, so it is a
0.1-unit sphere while the larva is beached). The flags are 0, 2, 4 or 5; `fn_78_8868` only tests
bit 0, and no entry has it set, so no joint ever gets `kMT_ProjectilePassthrough` from the
beach state. The port's table matches the rodata entry for entry.

**Attacks**
- *Tongue* (TongueLoop 31E8 → TonguePull 44CC): event 0x1b launches (6C2C), the tip ray-tests the
  player (716C/772C); a hit pulls the player (3AD0, morph ball 39C0) towards the mouth and locks
  orbit on the tongue (48A8). TongueHit / TongueMissed / TooMuchPulling / GivingUp end it.
- *Splash / shockwave* (SplashTelegraph → SplashJump → SplashDescend …): telegraph gen + loop at
  the water height (4E58), event 0xc spawns a `CShockWave` at the root, 4 below the water (7C2C).
  Locomotion is `SplashLocomotion`/`RiseLocomotion` (A5B4/A5CC: 0xb/10 and 0xe/1 by `xb6c`).
- *Spit* (event 0 in state 7, 800C): energy projectile from `tongue_LCTR_SDK` at the player's aim
  point, extent `spitProjectileRadius` (needs `CGameProjectile::SetProjExtent`).
- *Vomit* (4280, event 8 → 3EB8): once per state, within 10.5 XY and 100°, applies `damageInfo`,
  adds the visor-goo billboard and knocks the player back.
- *Beach* (4AA8 … StopBeaching 4C90): beach radii/materials (8868), tongue joint targetable, rumble,
  advances the attack index.
- *Belly* (ExposeBelly 3694): after enough belly damage (`ShouldExposeBelly`), only joint type 1 is
  targetable; weak-spot hits use `xd18` × `weakSpotDamageMultiplier`, a hit ≥ `0x1f4e7c2c` in state
  3 latches HeavyHit and adds `0xee6b6f47` damage.

**Non-overridden / dropped virtuals**: CanBeShot 94 (=1) and Listen 9C (=0) match the base; GetOrigin
A4 matches the base; IsListening 0 has no port virtual; trivial 0/44/4C/54/5C/64/74/80/8C are
skipped. 68F0 (FSM id from `b70`) is dropped with `b70`, the ctor's FSM parameter and B8EC, because
`CPatterned::SetupStateMachine` (called from `CPatterned::Think`) loads the FSM. GetHealthInfo C0,
GetAimPosition 69AC and GetDamageVulnerability 69D8 only forward to the base and are not overridden.

**Uncertain** (`TODO(uncertain)` in the cpp): health-info flags cleared in the ctor; the IRenderer
+0x44 particle overload (host uses the 1-argument one); the tongue alpha byte; the body state 7 test
in GetScannableObjectInfo.

**Resolved from the pseudocode while reviewing** (kept here so they are not re-opened): the joint
filter excludes `kMT_Character`, not material 1 (8954 ORs 2 into the word at filter+8, the high half
of the big-endian exclude u64 — same bit order as the radar test in 993C); `CollidedWith` tests
`kMT_SeekerTarget` (bit 0 of the word at collision-info+0x30, the high half of mMaterialLeft); the
tongue-out one-shot takes an XY-only direction (6C2C zeroes z before normalising); VomitHit zeroes
the radius-damage amount and the knockback of the Ldr damage info (+8 and +0x10 of CDamageInfo);
EaseInOut's mangled signature fixes the argument order the host already used (2E38); the Beach
descriptor's taunt type is 6 (4AA8 → B9D0); projectile+0x19c is `CWeapon::mCurDamageInfo`; the ctor
stores `mskNullBox` and −1000 in 0xb40/0xd0c (AA50/AE04); `SetBossParams` gets the initial HP
(*GetHealthInfo() = +0).

### Stage1 runtime status (2026-10-20, second pass)

Traced runs in `04_swamp_temple_dark` with `LAYER3@200+4` (forced layer), MP2_GODMODE, a temporary
per-function trace/state probe (never committed), the player driven through the debug console:
`/tmp/opencode/sb1_kill.sh` (aimed shots), `sb1_lowhp.sh` (low-HP phase run), `sb1_dead.sh` (kill);
`sb1_markers.py` writes the markers from a run's trace list.

- **Runs:** 0 `[port] caught`, 0 `[reach-stub]`. The splash-cycle run and the low-HP run end
  `exit=0`; the kill run ends on the harness wall clock (`exit=124`) with no signal.
- **Spawn:** u05bb / editor 002001c1 at (−166.1, −123.7, 86.0), inactive until `Activate`; then all
  16 joint collision actors exist (joint HP 10000, initial 0).
- **States reached:** SplashWait (−1) ↔ Bob (1) → SplashRise/Jump/Descend (7) → SplashTelegraph (8)
  at full health; and, at low health, Beach (0) → BeachBubbles/BeachWait/BeachRise → Dive (2),
  PlatformReady (4), TongueLoop (9) → TonguePull (5) (the whole tongue: launch, tip ray, pull,
  orbit lock, retract, render), Vomit (10), StopBeaching, Null, ResetPlatform and Dead.
- **Phase gate (this is what unlocks the second half):** `ScheduleNextAttack` advances the phase
  while `HP < CurrentPhase().unknown_0x98106ee2`; phase 1 was entered at 540 HP and phase 2 at 1 HP.
  Phase 0's attack slots are `0,0,0,2`, so `ShouldBeach` (`CurrentAttack() == 1`) never fires at
  1200 HP in retail either. Below 600 HP the beach/tongue/vomit attacks are what the disc asks for.
- **The tongue needs the player inside the arena hint:** `ReadyToTongue` is behind
  `xbf8_platformTime >= 0.65`, which only accrues in PlatformReady, and PlatformReady's trigger
  needs the player inside the arena `CScriptAIHint` (type 21). The only hint (u05b4) sits 1.7 units
  from the boss centre; parking the player there with `tp` every iteration (`PlayerInsideHint` →
  true) is what made the whole chain fire — the first pass never stayed inside it.
- **Shooting (corrected 2026-10-20, late):** the beam is not broken. A hit does full damage:
  `[sb1dmg] state=7 weak=0 dmg=25.00` is a charged power beam (50) against the boss's 0.5
  multiplier. What the first pass read as "the beam cannot reach the joints" was two harness
  errors plus one arena fact:
  - `aim <yaw> <pitch>` sets the **camera's** pitch, while `status`'s `position` is the player's
    origin at its **feet**, ~1.5 units lower. Aiming from the feet threw every shot 2-4 units low;
    the sampler showed the projectile at z 82 against a joint at z 86. Aim from `status`'s
    `camera ... at (x, y, z)` instead: the camera pitch then reads back exactly (`camera pitch`
    matches the request to 0.1 deg) and the projectile's launch direction matches it (ray probe:
    `dir` z 0.99 for an 83.5 deg request).
  - `CBeamProjectile`'s ray path is fine: `CSortedListManager` holds 56-141 actors, the
    projectile's 2-unit box returns candidates whenever it overlaps one, and the world ray hits
    geometry when the segment crosses it.
  - The arena is water: a shot that crosses the water surface is consumed before it reaches a
    joint, so the stand-off has to be inside the water beside the boss, not on the ring.
  - **Still open:** even at 4.6 units inside the water, only about one shot in a hundred lands
    (1 hit in ~250 s of firing). The joints are 1.5-5.5 unit spheres and the aim is 2-3 frames
    stale against a moving target, which does not account for it; the next measurement is the
    ray probe on a shot that starts inside a joint sphere, to see what rejects it.
- **Shooting, measured (2026-10-20, third pass - the ray probe answered it):** the game path is
  fine; every miss was the harness. 18 charged shots were fired at the joints from five stand-off
  distances (0, 1, 2, 4, 6 units) with the projectile's position sampled every 30 ms
  (`/tmp/opencode/sb1_inside_summary.json`):
  - **The projectile follows the camera's aim exactly.** In all 18 shots the sampled flight
    direction matched the requested yaw/pitch to about a degree (e.g. requested yaw -88.3,
    pitch -18.9; sampled -88.5, -22.5 with the first sample already 3.7 units along the ray). So
    `aim` -> `CPlayer::UpdateAssistedAiming` -> `mAssistAimXf` -> `CGunWeapon::Fire` -> the
    projectile's transform is correct, and `CGameProjectile::RayCollisionCheckWithWorld` sees the
    joint when the ray crosses it.
  - **The shot's line passed 1.8-4.4 units from the joint's centre** in every shot - more than the
    radius of 8 of the 16 joints (1.5-2.5 units), which is why only about one shot in a hundred
    landed (the big 5.0-5.5 unit joints were hit by luck).
  - **Why the line missed:** the driver teleported the player to the joint and then aimed from the
    camera position it read at that instant, but the player is then in mid-air over the deep water
    and keeps falling (the camera sits 2.5 above the feet and drops every frame), and the charged
    shot only leaves the muzzle after the ~1 s charge - so the muzzle was metres away from the
    point the driver aimed from. The joint itself also leaps (its z moves several units between
    two reads). Aiming from the player's **feet** (as the first driver did) is worse still: a
    2.5-unit vertical error, 32 degrees at a 4-unit stand-off.
  - **The water surface did not eat the shots in the sampled runs:** every sampled projectile flew
    40+ units and lived for the whole 2.6 s sampling window. All of them started under the surface
    (the camera sits 2.5 above the feet and the arena's water surface is at z ~ 81.5) and stayed
    under it, so a shot that *crosses* the surface upwards is still untested.
  - **The recipe that lands hits** (six driver passes, `/tmp/opencode/sb1_inside6_summary.json`):
    teleport the player so the **camera** ends up ~1 unit from the joint (the camera is 2.5 above
    the feet, so teleport to the joint's z - 2.5), read the joint's position twice 0.12 s apart to
    get its velocity, aim at the **predicted** position `p1 + v * T` with `T = 0.60 + 0.12 + D/120`,
    wait 0.60 s (the gun's follow transform settles after a teleport), then `beam power` and
    `press a 60`, and correct the aim by the measured line error before the next shot. Two hits
    landed that way: `[sb1dmg] state=1 ... joint=231` (Spine_3) and `[sb1dmg] state=7 ... joint=239`
    (Spine_13), both `dmg=1.00 orig=2.00` - an *uncharged* shot (the `press a 60` fires its normal
    shot at once; the charged one only leaves on the release 60 updates later). A charged hit is
    50 damage (25 after the boss's 0.5 multiplier) and the belly accumulator needs 50.
  - **With the lead, 82 shots over four runs landed 6 hits** - and every shot that reached a
    sphere landed: 6/6 hits produced a `[sb1dmg]` line and the boss's HP fell 1200 -> 1199 -> 1198.
    **A shot that touches a joint is never rejected; the misses are pure geometry.**
  - **Why the lead is needed:** the 16 joint spheres swim with the larva at **22-28 units/s** (all
    16 translate rigidly), and the muzzle is still 2-11 units/s off the camera's aim line right
    after a teleport (0.46 units with a stationary player) because the gun's follow transform
    settles. Between the placement and the muzzle the sphere swims 8-20 units away, so no shot
    ever *started* inside a sphere.
  - **The arena has no ground near the boss**: the pool floor is 30-40 units below the larva and
    the player sinks 12-50 units/s, so a short stand-off only exists by teleporting next to the
    joint in mid-air. Below 600 HP the larva beaches and stops leaping - that is the state where
    a stand-off and a fresh aim can both be had.
  - **A phase-1 run with the recipe** (2026-10-20, `/tmp/opencode/log_sb1b.log`): the boss was
    dropped to 785 HP with two console `damage` calls, and it then ran Beach (state 0), Dive (2),
    PlatformReady (4), TonguePull (5), TongueLoop (9) and Vomit (10). Five joint hits landed:
    `[sb1dmg] state=4 ... dmg=14.12 orig=28.24 joint=231`, `state=5 ... dmg=1.00 orig=2.00
    joint=230`, `state=9 ... dmg=1.00 orig=2.00 joint=22d`, `state=4 ... dmg=14.58 orig=29.17
    joint=22c`, `state=5 ... dmg=1.00 orig=2.00 joint=230` - a charged shot does 28-29 damage
    (14-15 after the 0.5 multiplier) and an uncharged one 2 (1 after it). The belly accumulator
    reached **16.6** of the 50 it needs, so `ShouldExposeBelly` was evaluated (it is in the run's
    `[sb1trace]` list) but ExposeBelly did not run yet. The run's trace also covers the beach
    path (`Beach`, `BeachBubbles`, `BeachWait`, `BeachRise`, `BeachAttackOver`, `SetBeachMaterials`,
    `ShouldBeach`, `DecrementAttached`, `AlertShredders`, `AnyJointOnRadar`, `IsJointOfType`,
    `ApplyDamageMsg`, `Vomit`, `VomitHit`, `Spit`, `Wait`, `TongueHit`, `TongueMissed`,
    `LockOrbitOnTongue`, `PullPlayer`, `ReleaseOrbitLock`, `KnockPlayer`, `LaunchTongue`,
    `ExtendTongue`, `FacePlatform`, `PickPlatform`, `PlayerInsideHint`) - the next session can
    turn that list into markers with `python3 /tmp/opencode/sb1_markers.py --apply <log>`.
- **`damage` is capped per call:** 1160 applied as 580, so the harness steps it in a loop.
- **Not reached:** ExposeBelly, HeavyHitReact, LightHitReact, SlideIntoWater, and the HeavyHit /
  LightHit triggers — all of them need a real hit to land. `ShouldExposeBelly` is evaluated but the
  state never runs because `xd10_bellyAccum` never moves without damage.
- **Counted markers:** 105 `// @ fn_78_<off>`, every one from a trace list (52 added in this pass).
  `fn_78_A5A8` was dropped: `tools/port_progress.py` reports it as not a function in any module's
  `symbols.txt`, and the min-health test is `fn_78_6548` (the FSM trigger). `BodyTaunt` is
  `fn_78_B9D0`; `SetLocomotion` and `JointActor` are host helpers with no retail function of their
  own.

## Stage2 (module 79) — verified

Function map, member layout (ctor `fn_79_BE3C`), decoded-but-unported behaviour and rodata
constants: [SwampBoss-stage2.md](SwampBoss-stage2.md). Host body: `PortCSwampBossStage2.cpp`
(complete; the traced fight is in SwampBoss-stage2.md §6).

| What | Value |
|---|---|
| Entry | `fn_79_140` sets `SSwampBossStage2_FuncPtrs` (`lbl_79_bss_8`) to loader `fn_79_170` |
| Object | `__nw(0xf20)` in the loader |
| Vtable | data+0x424 (0x8100e60c in the linked image) |
| DoUserAnimEvent jump table | data+0x56c |

Vtable overrides (slot: `fn_79_`): 0x08 dtor B8D4; 0x14 Think AFB4; 0x18 AcceptScriptMsg B3CC;
0x24 PreRender A76C; 0x28 AddToRenderer A59C; 0x2C Render A78C; 0x3C GetHealthInfo D0;
0x48 GetTouchBounds 8; 0x54 GetAimPosition A540; 0x68 OnScanStateChange B8B4;
0x70 DoUserAnimEvent A838; 0x74 GetScannableObjectInfo 2690; 0xA4 TakeDamage 44; 0xA8 CanBeShot A4;
0xAC IsListening 0; 0xB0 Listen AC; 0xB4 GetOrigin B4; 0xC8 GetModelAlpha 54; 0xDC ProjectileInfo A4AC;
0xE0 GetSearchPath 5C; 0xE4 GetPointSearchPath 64; 0xF0 SetupStateMachine A4B4; 0xF4 GetRagDoll 6C;
0x10C GetAttackTarget 74; 0x110 IsOnGround 84; 0x114 GetGravityConstant 90;
0x124 GetDeathExplosionParticle 9C; 0x138 GetIngSnatchingNormal 5E3C; 0x13C GetIngSnatchingPoint 5D40.
CollidedWith, Death, KnockBack and Touch are inherited.

FSM2 tables (`fn_79_`):

| # | Trigger | fn | # | State | fn |
|---|---|---|---|---|---|
| 0 | StateOver | A498 | 0 | Start | 9F4C |
| 1 | AllGrowthsDead | A438 | 1 | BecomeLightFlyer | 9534 |
| 2 | OvipositorDead | A418 | 2 | BecomeDarkFlyer | 91A4 |
| 3 | ZeroHealth | A3D8 | 3 | DeathSequence | 8FAC |
| 4 | PlayerInWater | A3F8 | 4 | Lurk | 9A3C |
| 5 | ShouldSpit | A3A0 | 5 | DarkLurk | 9754 |
| 6 | ShouldBarrage | A378 | 6 | SpitWater | 8D90 |
| 7 | ShouldTaunt | A350 | 7 | SpitBarrage | 8BB4 |
| 8 | ShouldSwoop | A328 | 8 | Taunt | 8A3C |
| 9 | SwoopOver | A31C | 9 | Level | 8958 |
| 10 | CircleDamage | A310 | 10 | Swoop | 82B8 |
| 11 | CircleOver | A08C | 11 | Blow | 7E24 |
| 12 | ShouldBlow | A064 | 12 | Circle | 7BE0 |
| 13 | IsLevel | A13C | 13 | Reel | 7870 |
| 14 | SwitchedPlatform | A0C8 | 14 | Recover | 76EC |
| 15 | Stunned | A044 | 15 | Flinch | 755C |
| 16 | StunTimeOut | 9FD4 | 16 | DropFlyer | 7308 |
| 17 | OneGrowthDead | 9FA4 | | | |
| 18 | ShouldDropFlyer | 9F7C | | | |

Code function: ResetAttackPhase 72FC.
