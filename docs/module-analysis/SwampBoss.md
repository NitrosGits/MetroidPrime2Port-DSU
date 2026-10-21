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
  - **ExposeBelly reached** (a later run of the same recipe, 2026-10-20): 17 shots, 22 joint hits,
    6 of them charged (`orig` 40-50, `dmg` 20-25, e.g. `[sb1bdmg] state=5 weak=0 canHit=1
    dmg=23.17 orig=46.34 ... joint=230`). The belly accumulator went 0 -> 1.0 -> 20.2 -> 36.9 ->
    **53.1 -> 57.8**, the state became 3 and the `ExposeBelly` function ran; the `HeavyHit`
    (`fn_78_65E8`) and `LightHit` (`fn_78_6590`) triggers were evaluated in that state, but
    `LightHitReact` and `HeavyHitReact` did not run - the two shots that followed aimed at the belly
    joint (`L_middle2_boob_LCTR`, radius 2.5) and missed by 18.7 and 56 units, and state 3 ended.
    The three functions that ran are marked.
  - **More of the firing mechanism, measured:** the beam leaves along the **player body's**
    forward, which lags the mouse aim by 13-44 degrees with a 0.1 s gap, so the aim has to be set
    at least 0.5 s before the shot. The charged shot is released by hand (`press a 0` holds A,
    `stick 0 0 0` releases) so it leaves with a fresh aim; a `tp` *during* the charge cancels the
    shot, 0.5 s before the release is fine. `MP2_RAPID_CHARGE=1` makes every press land 3 hits.
    The beam flies ~60 units/s (0.9 s to a joint 55 units away) and the arena's water surface
    (z ~ 45 from the floor) eats shots fired from the floor.
- **`damage` is capped per call:** 1160 applied as 580, so the harness steps it in a loop.
- **Not reached:** ExposeBelly, HeavyHitReact, LightHitReact, SlideIntoWater, and the HeavyHit /
  LightHit triggers — all of them need a real hit to land. `ShouldExposeBelly` is evaluated but the
  state never runs because `xd10_bellyAccum` never moves without damage.
- **Counted markers:** 105 `// @ fn_78_<off>`, every one from a trace list (52 added in this pass).
  `fn_78_A5A8` was dropped: `tools/port_progress.py` reports it as not a function in any module's
  `symbols.txt`, and the min-health test is `fn_78_6548` (the FSM trigger). `BodyTaunt` is
  `fn_78_B9D0`; `SetLocomotion` and `JointActor` are host helpers with no retail function of their
  own.

### Stage1: the unmarked functions (2026-10-21, decomp lane)

`symbols.txt` lists 243 Stage1 functions; 107 carried a `// @ fn_78_<off>` marker after the
2026-10-20 traced fight, and **136 are unmarked**. The counts below are the `bl` sites in the
retail asm (`build/G2ME01/SwampBossStage1/asm`), so they are the module's own call sites; an FSM
state or trigger is called from the FSM table, not by a `bl`, so it shows 0.

**The most-called unmarked functions, and this lane's verdict on each:**

| fn | retail call sites | port body | verdict |
|---|---|---|---|
| `fn_78_A5B4` | 8 (4D3C, 5CDC, 5D6C, 5E84) | `CSwampBossStage1::SplashLocomotion` | **marked** |
| `fn_78_CA4` | 8 (160, 7BC, A5E4) | none - the REL's copies of the `SLdrDamageInfo` / `SLdrDamageVulnerability` / `SLdrShockWaveInfo` destructors | **cited** (DOL copy) |
| `fn_78_A5CC` | 6 (587C, 5984, 6198) | `CSwampBossStage1::RiseLocomotion` | **marked** |
| `fn_78_9A1C` | 6 (48A8, 9744, 9AD8) | `JointMatches` (file-local) | **marked** |
| `fn_78_E84` | 6 (838) | none - the REL's copy of `CInputStream::ReadFloat` / `ReadBytes` | **cited** (DOL copy) |
| `fn_78_10B0`, `fn_78_8424` | 6 each | none - `SLdr` copy ctor / `CToken` destructor | **cited** (DOL copies) |
| `fn_78_35DC` | 4 (3320, 348C) | `CSwampBossStage1::HitReactBody` | compared, **not marked** (see below) |
| `fn_78_2C78` | 4 (2B20, 4D3C) | none - the REL's copy of the `CPASAnimParmData` constructor | **cited** (DOL copy) |

**`fn_78_A5B4` / `fn_78_A5CC` (the splash and rise locomotion ids).** Retail, field by field:
`lwz r0, 0xb6c(r3)` (the alt-animation flag, an `int`, so `!= 0` is the test), then a compare
against 0, then one of two `li` returns - `0xb` / `0xa` for the splash, `0xe` / `0x1` for the
rise. The port read the same member (`xb6c_altAnims`, `int`) and returned the same two values, so
no behaviour change was needed; both getters were moved out of the header's class body into
`PortCSwampBossStage1.cpp` (an inline class member cannot be read by `tools/asmcheck.py`, which
only parses definitions at file scope) and marked there. `asmcheck.py check` on the three
functions is clean (0 findings).

**`fn_78_9A1C` (does a joint match the requested type).** Retail: `cmpwi r4, 0x3` on the joint's
own type (from the rodata table), `cmpwi r5, 0x3` on the requested type - either one being 3
(`kAllJoints`) matches everything - and otherwise `subf r0, r4, r5` + `cntlzw` + `srwi 3, 5`, the
equal-when-zero idiom, is the equality test. The port's `JointMatches` is the same test with the
same argument order (`JointMatches(kJoints[i].mType, type)` in `SetJointsActive`), so it is
marked as it stands. The `a1 == 3` arm is unreachable: no entry of the joint table has type 3.

**`fn_78_35DC` (the shared hit-react body).** Retail: `RecordState(3, msg)`,
`SetLocomotionType(5)`, and under `msg == 1` (`kStateMsg_Update`) `RetractTongue`,
`UpdateTongueTip(mgr, 1.0f, false)`, `HeadWeightDown(dt)` and `UpdateHeadTurn(mgr, dt)`. The
port's `HitReactBody` (called by `LightHitReact` 3320 and `HeavyHitReact` 348C) is the same
body, and `msg == kStateMsg_Update` is the same test (the host's `EStateMsg` starts at
`kStateMsg_Activate = 0`). **Not marked**: no run reached it - both callers are FSM states that
need state 3 (`ExposeBelly`), and state 3 needs the belly accumulator over 50, which needs ~50
landed joint hits; the harness lands about one shot in six and the charged shot never comes out
of `press a 0` / `stick 0 0 0` as a charged one (the trace shows `orig=2.00` every time).

**The rest of the inventory** (136 unmarked functions; the ones with a retail body first, then
the REL's copies of host/DOL classes, which are cited here and are neither reimplemented nor
marked - the host already has the class, so a port body would be a second copy):

| fn | call sites | callers | note |
|---|---|---|---|
| `fn_78_1104` | 2 | 838 | the `SLdrAudioPlaybackParms` reader (a DOL reader) |
| `fn_78_130` | 1 | RELMain | the REL entry that installs the loader |
| `fn_78_21C8` | 0 | - | `OnScanStateChange`, forwards to `CActor` |
| `fn_78_25F4` | 0 | - | `IncrementMisses` (a code function, the FSM calls it) |
| `fn_78_2840` | 2 | 260C | a code function of the FSM |
| `fn_78_28A4` | 2 | 2728 | the `EScriptObjectState` 4CC helper ('I30'..'I33') - a DOL copy |
| `fn_78_2C78` | 4 | 2B20, 4D3C | the `CPASAnimParmData` constructor - a DOL copy |
| `fn_78_3088` | 2 | 2E38 | `CVector3f::Cross` - a DOL copy |
| `fn_78_3320` | 0 | - | `LightHitReact` (an FSM state) |
| `fn_78_348C` | 0 | - | `HeavyHitReact` (an FSM state) |
| `fn_78_35DC` | 4 | 3320, 348C | the shared hit-react body (see above) |
| `fn_78_3964` | 2 | 44CC | the tongue-pull helper |
| `fn_78_39C0` | 2 | 44CC | `PullMorphedPlayer` |
| `fn_78_5618` | 2 | 55AC | the `rstl` vector erase - an rstl copy |
| `fn_78_5784` | 0 | - | `SlideIntoWater` (an FSM state) |
| `fn_78_69AC`, `fn_78_69D8` | 0 | - | `GetAimPosition` / `GetDamageVulnerability`, forward to `CPatterned` |
| `fn_78_772C` | 2 | 7440 | the tongue-tip helper |
| `fn_78_7F38` | 2 | 7F14 | `LdrToDamageInfo` - a DOL copy |
| `fn_78_8004` | 0 | - | `ProjectileInfo` (a vtable slot) |
| `fn_78_838` | 2 | 160 | the `SLdrSwampBossStage1` property reader (the port uses the generated `.inc`) |
| `fn_78_94`, `fn_78_9C`, `fn_78_A4`, `fn_78_C0` | 0 | - | the trivial vtable slots (see "Non-overridden / dropped virtuals") |
| `fn_78_9A1C` | 6 | 48A8, 9744, 9AD8 | the joint-type match - **marked** here |
| `fn_78_9A44` | 0 | - | `CollidedWith`, forwards to `CPatterned` |
| `fn_78_A5B4` | 8 | 4D3C, 5CDC, 5D6C, 5E84 | the splash locomotion - **marked** here |
| `fn_78_A5CC` | 6 | 587C, 5984, 6198 | the rise locomotion - **marked** here |
| `fn_78_A5E4` | 0 | - | the destructor (a vtable slot) |
| `fn_78_B474`, `fn_78_B630` | 2 each | B270 | `SLdr` copy constructors |
| `fn_78_BCD0` | 1 | 8 | `CPhysicsActor::GetBoundingBox` - a DOL copy |
| `fn_78_BD0C` | 0 | - | a weak `__dt` the REL carries - a DOL copy |
| `fn_78_E84` | 6 | 838 | `CInputStream::ReadFloat` / `ReadBytes` - a DOL copy |

The remaining 101 unmarked functions are the REL's copies of host/DOL classes and are cited
rather than reimplemented: the `SLdr` / `SLdrActorParameters` / `SLdrPatternedAITypedef` /
`SLdrEditorProperties` constructors, destructors and copies (`fn_78_1074` `fn_78_10B0`
`fn_78_12E8` `fn_78_13A0` `fn_78_141C` `fn_78_14EC` `fn_78_1570` `fn_78_15C8` `fn_78_161C`
`fn_78_1688` `fn_78_16E4` `fn_78_1750` `fn_78_18B0` `fn_78_1918` `fn_78_19A4` `fn_78_19FC`
`fn_78_1B54` `fn_78_1B8C` `fn_78_1C18` `fn_78_1CB4` `fn_78_1D2C` `fn_78_1D90` `fn_78_1E0C`
`fn_78_2100` `fn_78_2178` `fn_78_2C3C` `fn_78_2DFC` `fn_78_33E8` `fn_78_3444` `fn_78_3580`
`fn_78_3848` `fn_78_4218` `fn_78_474` `fn_78_4B4` `fn_78_4D4` `fn_78_4FC` `fn_78_55AC`
`fn_78_638` `fn_78_68F0` `fn_78_6958` `fn_78_6A4` `fn_78_6F8` `fn_78_75C` `fn_78_7788`
`fn_78_7AA8` `fn_78_7BC` `fn_78_7E68` `fn_78_7EC0` `fn_78_7F14` `fn_78_81A8` `fn_78_81F0`
`fn_78_82A4` `fn_78_82E4` `fn_78_8304` `fn_78_832C` `fn_78_83AC` `fn_78_83E8` `fn_78_8424`
`fn_78_8480` `fn_78_84A0` `fn_78_84E8` `fn_78_8574` `fn_78_85AC` `fn_78_861C` `fn_78_8694`
`fn_78_8BBC` `fn_78_8C40` `fn_78_8C78` `fn_78_8DE0` `fn_78_8E18` `fn_78_8E38` `fn_78_8F20`
`fn_78_A744` `fn_78_A928` `fn_78_A9BC` `fn_78_ADAC` `fn_78_AE04` `fn_78_AE5C` `fn_78_AEB8`
`fn_78_AF0C` `fn_78_B03C` `fn_78_B098` `fn_78_B0F0` `fn_78_B154` `fn_78_B1E4` `fn_78_B270`
`fn_78_B8EC` `fn_78_B970` `fn_78_BBA8` `fn_78_BC68` `fn_78_CA4` `fn_78_D50`), plus the trivial
vtable slots that only forward to the base (`fn_78_0` `fn_78_44` `fn_78_4C` `fn_78_54`
`fn_78_5C` `fn_78_64` `fn_78_74` `fn_78_80` `fn_78_8C`).

**Run evidence for the three new markers** (2026-10-21, world `3dfd2249` area
`04_swamp_temple_dark`, layer 3 forced, `MP2_GODMODE=1`, 6000 frames, the boss woken with the
console `send u05bb Activate` and dropped to 600 HP with `damage u05bb 1199`, the player parked
in the arena's AI hint so the tongue chain fires). The probe was a temporary trace in the port
code, never committed; the run walked the states -1, 0, 1, 2, 4, 5, 7, 8, 9 and 10.

| function | arms seen in the traced run |
|---|---|
| `fn_78_A5B4` | `xb6c == 0` 154x, `xb6c == 1` 5x - both returns |
| `fn_78_A5CC` | `xb6c == 0` 7x, `xb6c == 1` 44x - both returns |
| `fn_78_9A1C` | `type == 3` -> true (448+32+32x), `type != 3` and equal -> true (280+7+30x), `type != 3` and different -> false (100+100+30+30+20+20+7x) - every reachable arm |
| `fn_78_35DC` | never entered - no run reached state 3 |

### Stage1: how the boss is activated, and why the harness had to wake it by hand (2026-10-21)

Nothing in the SwampBossStage1 REL activates the boss: the module never calls
`CEntity::SetActive` (its one vtable +0x1c call is a body-controller method), and the retail
`Think` (fn_78_95D8) returns early while the entity's active bit (0x20 bit 7) is clear, exactly
like the port's `if (!GetActive()) return;`.

The activator is in the **area's script data** (read from the running game's own connection
lists, `obj <uid>` in the console):

| step | object (editor id) | connection |
|---|---|---|
| the activator | CScriptRelay 002000a2 (`u05ba`) | `on Zero send Activate -> 002001c1` (the boss) |
| its trigger | CScriptRelay 002001d0 (`u044f`) | `on Zero send SetToZero -> 002000a2` |
| the switch | CScriptSwitch 002001e0 (`u044d`) | `on Open send SetToZero -> 002001d0` |
| the opener | CScriptLayerController 002001d7 (`u0454`) | `on Arrived send Open -> 002001e0` |

So the activation is **layer driven**: the layer controller's "Arrived" (the port's
`CScriptLayerController::Think` sends it when the layer it watches reaches `kLP_Ready`) opens the
switch, the switch's "on Open" starts the relay chain, and the last relay activates the boss.
(The recipe that drives this chain from the console is in "the marked functions' arms, the
activation recipe, the charged shot" below.)
Above it sit CScriptLayerController 002001c5 ("Arrived" -> "Increment" -> 002001d7) and the
switch cascade the four CScriptDamageableTrigger objects 002003e1/e9/ea/ec feed (they die ->
the counter 002003eb -> relays -> switches 00200226/002001e4).

**The port is not at fault; the harness was.** The chain is intact in the port - driving it by
hand works (`send u05ba SetToZero` activates the boss and `send u044f SetToZero` does too,
both verified in a run) - and the relays, switches and layer controllers are the host's own
code. The harness never reached the chain because it switched layers with the port's debug
`LAYER3` warp, which loads the boss's layer directly and never gives the layer controller its
`kSM_Load`/`kSM_Increment`, so the controller's "Arrived" (the thing that opens the switch)
never fires. In the retail game the player walks into the arena, a trigger starts the sequence
timers, the layer controllers increment, the layers load, "Arrived" opens the switch and the
relay chain activates the boss; `send u05bb Activate` is the equivalent shortcut.

### Stage1: the charged shot (2026-10-21, first pass - corrected below)

The charge is driven by `CPlayerGunBase::ProcessInput`
(src/MetroidPrime/Player/CPlayerGunBase.cpp:76-83), which sets the gun's input bit 4 from
`CPlayer::ChargeBeamHeld` (the `CControlMapper::kC_ChargeBeam` control); only that bit starts
the charge (`CPlayerGun.cpp:631`) and releases it (`CPlayerGun::ButtonRelease`,
CPlayerGun.cpp:557). The charge state machine is the upstream retail code
(`kCP_ChargeRequested` -> `kCP_Charging` -> `kCP_ChargeFx` -> `kCP_Charged`, the factor growing
at 1/1.8 per second, `CPlayerState::GetMissileComboChargeFactor`) and the port only adds the
`TARGET_PC` rapid-charge option on top.

This first pass concluded from the `MP2_DEBUG_BEAM=1` log that the charged shot "never fires
charged"; that was a misreading - the line prints the beam's ray magnitude, not the damage. See
"the marked functions' arms, the activation recipe, the charged shot" below for the corrected
measurement.

### Stage1: the five most-called unmarked functions of the first pass (2026-10-21)

| fn | retail call sites | port body | verdict |
|---|---|---|---|
| `fn_78_35DC` | 4 (3320, 348C) | `HitReactBody` | compared, **not marked** (state 3 unreachable) |
| `fn_78_39C0` | 2 (44CC) | `PullMorphedPlayer` | **marked** |
| `fn_78_3964` | 2 (44CC) | `SendOrbitMessage` | **marked** |
| `fn_78_25F4` | 0 (a code function) | `IncrementMisses` | **marked** |
| `fn_78_3320` | 0 (an FSM state) | `LightHitReact` | compared, **not marked** (state 3 unreachable) |

- **`fn_78_39C0` (`PullMorphedPlayer`)** - the retail reads the target's x/y (the z is read and
  then overwritten with 0), takes the difference to the player's x/y, `CanBeNormalized` +
  `Normalize`, scales it by 30 (`lbl_78_rodata_184`), `SetVelocityWR`, then writes the constant
  force (0, 0, mass * gravity) and `SetMoveState(4)` (`kMS_FallingMorphed`). The port does the
  same in the same order; only the constant force goes through `SetConstantForceWR` instead of
  the three stores.
- **`fn_78_3964` (`SendOrbitMessage`)** - the retail tests bit 7 of the flag byte at 0xce4 and,
  if it is clear, sets it and sends the "IS10" state to its own id; the port's
  `xce4_orbitMsg` is that bit and `SendOrbitMessage` does the same.
- **`fn_78_25F4` (`IncrementMisses`)** - the retail bumps the miss counter at 0xbfc and stores 3
  in the tongue state at 0xc34; the port does exactly that (branch-free).
- **`fn_78_35DC` / `fn_78_3320`** - compared field by field with the retail (the hit-react
  body and `LightHitReact`), no difference found, but **not marked**: both need state 3
  (`ExposeBelly`), and state 3 needs the belly accumulator over 50 - about 50 landed joint
  hits, which the harness does not manage (it lands about one shot in six and the charged shot
  never fires charged, see above).

Run evidence (2026-10-21, `04_swamp_temple_dark`, layer 3, `MP2_GODMODE`, 12000 frames, the
boss woken with `send u05bb Activate` and dropped to 600 HP with `damage u05bb 1199`, the player
parked in the arena's AI hint and morphed with the X button):

| function | seen | arms |
|---|---|---|
| `fn_78_25F4` | 4x (`miss=0..3`) | branch-free, all of it |
| `fn_78_39C0` | 35x | the CanBeNormalized arm (the other needs a zero-length direction) |
| `fn_78_3964` | 2x (`msg=0`) | the "not sent yet" arm (the flag is cleared by the matching IS11 message when the pull ends) |
| `fn_78_35DC`, `fn_78_3320` | 0x | no run reached state 3 |

### Stage1: the marked functions' arms, the activation recipe, the charged shot (2026-10-21)

**The arms of the marked functions** (one temporary probe per function, never committed):

| fn | calls | arm that ran | the other arm |
|---|---|---|---|
| `fn_78_39C0` | 12 | `CanBeNormalized` true, with a difference of 0.084..0.106 units | **unreachable in shipped data**: `fn_78_44CC` passes the player's position plus a normalized direction times `dt * 7.5` (`dt > 0`), so the difference is never zero. Commented at the marker; the marker stays. |
| `fn_78_3964` | 1 | the flag was clear, so the "IS10" message went out | **unreachable in shipped data**: the flag is cleared again by the matching IS11 message (`fn_78_390C`, the port's `ReleaseOrbitMessage`, which ran once with the flag set) when the pull ends, so every activation sees it clear. Commented at the marker; the marker stays. |
| `fn_78_35DC` | 0 | - | **not marked**: state 3 is unreachable in the harness runs, so the function was never entered. |
| `fn_78_3320` | 0 | - | **not marked**: never entered either. |

**Why state 3 (`ExposeBelly`) is unreachable.** The `ShouldExposeBelly` trigger needs `xd10` (the
belly accumulator) over 50 (over 5 at min health), and `xd10` only grows in the port's
`ApplyDamageMsg` (fn_78_9B74, ported) when a joint takes a hit - the harness's shots do not land,
so the accumulator stayed at 0.0 in every run. This is neither shipped data (the disc's FSM2 has
the edge `ShouldExposeBelly -> ExposeBelly`, and the writer is ported) nor an unported writer, so
`fn_78_35DC` and `fn_78_3320` stay unmarked. The arm of `fn_78_35DC` that did not run is its
`msg == 1` branch (`RetractTongue`, `UpdateTongueTip`, `HeadWeightDown`, `UpdateHeadTurn`); the
function was never entered at all, so its other arm did not run either. `fn_78_3320` was never
entered either, so neither of its arms ran (the `DeliverCmd(CBCAdditiveReactionCmd)` on Activate
and the no-op otherwise).

**Activating the boss through the area's own chain.** The area's script objects (read with
`obj <uid>` in the console) are the only thing that activates it; the chain is

| step | object (editor id) | connection |
|---|---|---|
| the activator | CScriptRelay 002000a2 (`u05ba`) | `on Zero send Activate -> 002001c1` (the boss) |
| its trigger | CScriptRelay 002001d0 (`u044f`) | `on Zero send SetToZero -> 002000a2` |
| the switch | CScriptSwitch 002001e0 (`u044d`) | `on Open send SetToZero -> 002001d0` |
| ... | CScriptSwitch 00200226 (`u043e`) | `on Open send SetToZero -> 002001e0` |
| ... | CScriptRelay 00200228 (`u0476`) | `on Zero send Open -> 00200226` + `on Zero send SetToZero -> 00200226` |
| ... | CScriptSwitch 002001e4 (`u04b6`) | `on Open send SetToZero -> 0020022a` + `on Open send SetToZero -> 00200227` + `on Open send Play -> 002001c5` |
| ... | CScriptRelay 002001fc (`u0443`) | `on Zero send SetToZero -> 002001e4` |
| the root | CScriptCounter 002003eb | `on MaxReached send SetToZero -> 002001fc` - the four CScriptDamageableTrigger objects 002003e1/e9/ea/ec dying |
| the openers | CScriptLayerController 002001c5 / 002001d7 | `on Arrived send Open -> 002001e4` / `-> 002001e0` |

**The recipe the harness uses** (the area's own script messages; only the layer controllers'
"Arrived" and the counter's "MaxReached" are stood in for):

    send u044d Open        # the switch 002001e0 open - the message the layer controller 002001d7
                           # sends on "Arrived"
    send u0476 SetToZero   # the relay 00200228 - where the switch 002001e4's "on Open" lands

Those two commands make the area's own relays and switches do the rest and the boss activates
(`fsm Bob`): 00200228 "on Zero" opens the switch 00200226 and sets it to zero, 00200226 "on Open"
sets 002001e0 to zero, 002001e0 "on Open" sets the relay 002001d0 to zero, 002001d0 "on Zero" sets
the relay 002000a2 to zero, and 002000a2 "on Zero" sends the Activate.

The two links the harness cannot drive:

- the layer controllers 002001c5/002001d7 never send "Arrived": the port's
  `CScriptLayerController::Think` only sends it for a **dynamic** layer it was told to load, and
  `send u04ce Increment` / `send u0454 Increment` (the controllers' own Load/Increment messages)
  load no layer (the log shows no `[layer] area 32 layer N activated` after them).
- the counter 002003eb never reaches its maximum: the four CScriptDamageableTrigger objects are
  inactive, so killing them (`hurt u05a0 500` and the rest, which drives their HP to -500) does
  not send their "Dead" message and the counter does not increment.

**The charged shot.** The loaded tweak data binds the charge to the same button as the fire
(probed from `CTweakPlayerControls::GetMapping`: `fire=15 auto=0 charge=15 charge2=15 morph=17
morph2=17`, so the charge is the A button and there is no auto-fire), so the harness input is
`press a 0` ... `stick 0 0 0`. The charge does build: with A held the factor grows 0.009, 0.102,
... 1.000 (the rate is 1/1.8 per second) and the shot leaves when the charge completes. My
earlier note ("the charged shot never fires charged") was wrong - the `MP2_DEBUG_BEAM=1` line
prints the beam's ray magnitude, not the damage, and the shots that landed were the press shots.
What is still true is that the charged shots do not reach the boss's joints: the aim has to be
set ~1.8 s before the shot leaves and the player falls in the water, so the belly accumulator
stayed at 0.0 and state 3 was never reached.

### Stage1: five more of the most-called unmarked functions (2026-10-21)

| fn | retail call sites | port body | verdict |
|---|---|---|---|
| `fn_78_35DC` | 4 (3320, 348C) | `HitReactBody` | compared, **not marked** (state 3 unreachable) |
| `fn_78_3320` | 0 (an FSM state) | `LightHitReact` | compared, **not marked** (state 3 unreachable) |
| `fn_78_348C` | 0 (an FSM state) | `HeavyHitReact` | compared, **not marked** (state 3 unreachable) |
| `fn_78_5784` | 0 (an FSM state) | `SlideIntoWater` | compared, **not marked** (the state is never reached) |
| `fn_78_9A44` | 0 (a vtable slot) | `CollidedWith` | compared, **not marked** (its own work needs a joint touch during a tongue pull) |

- **`fn_78_348C` (`HeavyHitReact`)** - the retail sets the joints to `kAllJoints` with
  `RemoveTarget|Reflect` on Activate, sends `CBodyStateCmd(kBSC_NextState)`, calls the shared
  hit-react body and then the taunt with type 4; the port does the same in the same order.
- **`fn_78_5784` (`SlideIntoWater`)** - the retail records state 6 and, on Activate, sets the
  joints to `kAllJoints` with `RemoveTarget|Reflect` (0xa), rumbles and stops the tongue; the
  port does the same.
- **`fn_78_9A44` (`CollidedWith`)** - the retail only does its own work while the state is 5
  (TonguePull): it walks the collision list for `kMT_SeekerTarget` (bit 0 of the word at
  collision-info+0x30) and sets the "player touched" flag, and then calls the base. The port
  does the same.

### Stage1: the arena entry, the charged shots, the hit-react functions (2026-10-21)

**The area's chain, in full** (from the area's script data, `obj <uid>` in the console):

- the four **CScriptDamageableTrigger** objects 002003e1/e9/ea/ec (the arena's acid pools) are
  activated by the **CScriptRelay 002001e3 (`u04a5`)** - they are inactive until that relay
  fires, so without it they cannot be killed and the counter 002003eb never reaches its maximum;
- the **CScriptSwitch 002001d2 (`u0547`)** is the root: its "on Open" sets the relay 002001e3;
- the **CScriptCounter 002003eb** ("on MaxReached" -> the relay 002001fc) counts the four
  triggers' deaths;
- the relay 002001fc -> the switch 002001e4 -> "on Open" -> "Play" -> the **CScriptLayerController
  002001c5** (its layer load ends in "Arrived" -> "Increment" -> the layer controller 002001d7 ->
  its layer load ends in "Arrived" -> "Open" -> the switch 002001e0) and -> the relays
  0020022a/00200227 -> ... -> the relay 00200228 -> the switch 00200226 -> the switch 002001e0 ->
  the relay 002001d0 -> the relay 002000a2 -> "Activate" -> the boss.

**Why the harness cannot drive the whole chain.** The `LAYER3` warp loads layer 3 itself, so
when the layer controllers are given their Load/Increment their layer is already Ready,
`mWaitingForLoad` is never set, and the port's `CScriptLayerController::Think` (upstream code, no
`TARGET_PC` change) only sends "Arrived" after such a load: `send u04ce Increment` and
`send u0454 Increment` load no layer (the log shows no `[layer] area 32 layer N activated` after
them). Without the warp the area's own entry choreography does not run either: the harness's
teleport through the arena logs no layer activation at all, and the area's loaded triggers
(`u046b` 00200011 and the rest) carry no message connections - the trigger that starts the
sequence is in a layer that only loads with the fight.

**What the harness uses** (the area's own switches and relays, verified - the boss comes up with
`fsm Bob`):

    send u044d Open        # the switch 002001e0 - the message the layer controller 002001d7 sends
                           # on "Arrived"
    send u0476 SetToZero   # the relay 00200228 - where the switch 002001e4's "on Open" lands

**The charged shot works.** The tweak binds the charge to the A button (the same one as the
fire: `fire=15 charge=15 auto=0 morph=17`), so the harness input is `press a 0` ... `stick 0 0 0`;
the charge builds over ~1.8 s and the shot leaves when it completes. The run's damage trace shows
`dmg=19.91 orig=39.81` for a charged hit and `dmg=1.00 orig=2.00` for a normal one. The aim has
to be refreshed while the charge runs (the shot leaves ~1.8 s later and the player falls in the
water).

**The live-aim driver** (`/tmp/opencode/sb1_lane/drive.py`, adapted from the stage-2 lane's):
it reads the joint spheres (`objs collision` - the boss's 16 are the run of 16 consecutive
collision-actor uids), puts the player 4 units to the side of the target joint at the camera's
height, aims from the gun (2.15 above the feet) at the joint's live position and fires. 37 hits
in 330 charged shots, and the belly accumulator reached **21.58** - short of the 50 that
`ShouldExposeBelly` wants, so state 3 still did not run.

**The five functions** (compared with the retail, all of them still unmarked):

| fn | port body | retail | verdict |
|---|---|---|---|
| `fn_78_35DC` | `HitReactBody` | records state 3, locomotion 5, and on `msg == 1` retracts the tongue, updates the tip, drops the head weight and turns the head | matches; **not marked** - state 3 was never reached (0 entries) |
| `fn_78_3320` | `LightHitReact` | the shared body plus, on Activate, `DeliverCmd(CBCAdditiveReactionCmd)` | matches; **not marked** - never entered |
| `fn_78_348C` | `HeavyHitReact` | on Activate `SetJointsActive(kAllJoints, RemoveTarget|Reflect)` and `DeliverCmd(CBodyStateCmd(kBSC_NextState))`, then the shared body, then the taunt (type 4) | matches; **not marked** - state 3 was never reached |
| `fn_78_5784` | `SlideIntoWater` | records state 6 and, on Activate, `SetJointsActive(kAllJoints, 0xa)`, rumbles and stops the tongue | matches; **not marked** - the state is never reached |
| `fn_78_9A44` | `CollidedWith` | its own work only while the state is 5 (TonguePull): it walks the collision list for `kMT_SeekerTarget` and sets the "player touched" flag, then calls the base | matches; **not marked** - no run had a joint touch during a tongue pull (0 entries) |

The blocker for all five is the same: state 3 needs the belly accumulator over 50 and the
harness's hit rate (37 hits in 330 charged shots, the accumulator reset by `BeachWait` between
the beach cycles) does not get there.

### Stage1: the belly accumulator, the hit rate and fn_78_5784 (2026-10-21)

**The belly accumulator (`xd10`) never decays.** The retail writes it in three places only:

| asm | function | what |
|---|---|---|
| 0x52C0 | fn_78_5268 (`BeachWait`) | `xd10 = 0` - the reset |
| 0x53C0 | fn_78_5374 (`Dive`) | `xd10 = 0` - the reset |
| 0xA3AC / 0xA3D8 | fn_78_9B74 (`AcceptScriptMsg`, the port's `ApplyDamageMsg`) | the cap (`xd10 = threshold * 0.5` while no tongue has been launched) and the increment |

The port matches all three (`BeachWait` and `Dive` zero it, `ApplyDamageMsg` grows it), and the
trigger `fn_78_6458` reads it against 5.0 at min health and 50.0 otherwise. So the accumulator
only climbs between a `BeachWait`/`Dive` and the next one - the hits have to land in that window,
in a state where the port adds to it (0 Beach, 4 PlatformReady, 5 TonguePull, 9 TongueLoop,
10 Vomit).

**The hit rate is the blocker, not the decay.** With the live-aim driver (the charge input is
the A button, so the shot leaves ~1.8 s after the press and the aim has to be refreshed while
the charge runs) a long run landed 37 hits in 330 charged shots - 11% - and the accumulator
reached 21.58 before the next `Dive` reset it. The 21.58 is 43% of the 50 the trigger wants
(and 4x the 5 it wants at min health), so the remaining gap is the hit rate and the resets,
not a decay.

**`fn_78_5784` (`SlideIntoWater`) is marked.** The retail records state 6 and, on Activate only,
sets the joints to `kAllJoints` with `RemoveTarget|Reflect`, rumbles and stops the tongue; every
message falls through to the body part. The port does the same, and a run exercised every arm:

| arm | retail branch | probe count |
|---|---|---|
| the joints and the rumble (Activate only) | `cmpwi r31, 0x0` at 0x57B0 -> 0x57B8 (`li r5, 0x3` at 0x57C0, `li r6, 0xa` at 0x57C4, `bl fn_78_9744` at 0x57C8, `bl fn_78_511C` at 0x57D4) | 9 x Activate |
| the tongue stop (Activate only) | `cmpwi r31, 0x0` at 0x57D8 -> 0x57E0 (`bl fn_78_6BEC` at 0x57E4) | 9 x Activate |
| the body part (every message) | the fall-through at 0x57E8 | 909 x Update, 9 x Deactivate |

**The other four stay unmarked**, each with the arm that did not run:

| fn | arm that did not run |
|---|---|
| `fn_78_35DC` | the `msg == 1` branch (`cmpwi r31, 0x1` at 0x3620 -> the four calls); the function was never entered (state 3 needs the accumulator over 50) |
| `fn_78_3320` | the `DeliverCmd(CBCAdditiveReactionCmd)` on Activate (`cmpwi r31, 0x0` at 0x3340); the function was never entered |
| `fn_78_348C` | the Activate branch (`cmpwi r31, 0x0` at 0x34B8 -> `bl fn_78_9744` at 0x34C4 and `bl DeliverCmd` at 0x34E8); the function was never entered |
| `fn_78_9A44` | the state-5 branch (`cmpwi r0, 0x5` at 0x9A54 -> the `kMT_SeekerTarget` walk); no run had a joint touch during a tongue pull |

A run that reaches state 3 (the belly accumulator over 50, or over 5 at min health) would open
all of them: `fn_78_35DC` and `fn_78_3320` from the `LightHit`/`HeavyHit` triggers, and
`fn_78_9A44` from the player touching a joint while the tongue pulls.

### Stage1: forcing state 3, the branch tables and the next three (2026-10-21)

`git merge port` brought the fast harness (`MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8` runs the
game ~3.5x faster) and the console's `fsm`/`place`/`frame`. The live-aim driver is now
`tools/drivers/swampboss1_aim.py`.

**Forcing state 3.** `fsm <boss> ExposeBelly` does enter the state (the port's
`CGenericFSM2State::SetState` takes the name), but the console's reply says "not entered" for
every state, because the machine leaves the state again inside the same update: the reply
compares the name after `SetState` with the one asked for, and the FSM2 evaluates the state's
own transitions in that update. So the reply is not evidence either way - the heartbeat
(`st=` in the probe) and the function probes are.

The state does not stay long enough for the hit-react triggers: `fsm <boss> ExposeBelly` followed
by `damage <boss> 40` (the LightHit trigger needs the HP under the snapshot taken at the
activation) leaves the state before the damage lands, and the damage before the forcing takes
the snapshot after it, so neither order fires the trigger. Pushing the accumulator with
`damage <joint> <n>` does not work either: the port's `ApplyDamageMsg` only counts a hit that
came from a projectile (`proj` is the joint's last toucher), and a console `damage` has the
player as the toucher.

**The branch tables** (the arms, with the retail branch addresses; a run of the traced fight and
of the forced-state runs):

| fn | arm | retail branch | probe count | verdict |
|---|---|---|---|---|
| `fn_78_35DC` | the `msg == 1` branch (RetractTongue, UpdateTongueTip, HeadWeightDown, UpdateHeadTurn) | `cmpwi r31, 0x1` at 0x3620 | 0 | **not marked** - the function was never entered (state 3 unreached) |
| `fn_78_35DC` | the other messages | `bne .L_00003670` at 0x3624 | 0 | not marked |
| `fn_78_3320` | the Activate `DeliverCmd(CBCAdditiveReactionCmd)` | `cmpwi r31, 0x0` at 0x3340 | 0 | **not marked** - never entered |
| `fn_78_3320` | the shared body | `bl fn_78_35DC` at 0x333C | 0 | not marked |
| `fn_78_348C` | the Activate branch (SetJointsActive(kAllJoints, 0xa) and `DeliverCmd(CBodyStateCmd)`) | `cmpwi r31, 0x0` at 0x34B8 | 0 | **not marked** - never entered |
| `fn_78_348C` | the shared body and the taunt | `bl fn_78_35DC` at 0x3508, `bl fn_78_B9D0` | 0 | not marked |
| `fn_78_9A44` | the state-5 branch (the `kMT_SeekerTarget` walk) | `cmpwi r0, 0x5` at 0x9A54 | 0 | **not marked** - no joint touch during a tongue pull |
| `fn_78_9A44` | the base call | the tail of the function | 0 | not marked |

**The next three unmarked functions by static call count** (the ones after the five above; all
three are the REL's copies of base behaviour, so they are cited, not ported):

| fn | call sites | retail | note |
|---|---|---|---|
| `fn_78_21C8` | 0 (a vtable slot) | `bl OnScanStateChange__6CActorF...` | `CActor::OnScanStateChange` - the port does not override it (the "Non-overridden / dropped virtuals" list) |
| `fn_78_69AC` | 0 (a vtable slot) | `bl GetAimPosition__10CPatternedCFRC13CStateManagerf` | `CPatterned::GetAimPosition` - the base's |
| `fn_78_69D8` | 0 (a vtable slot) | `bl GetDamageVulnerability__10CPatternedCFv` | `CPatterned::GetDamageVulnerability` - the base's |

### Stage1: the console's `shoot`, the weak spot and the accumulator cap (2026-10-21)

`git merge port` brought the console's `shoot <id> [charged] [d]` (a real beam projectile at an
actor, homing on it) and the fixed `fsm` reply.

**`shoot` works and the damage is real.** `shoot <belly> charged 4` puts a charged projectile into
the larva: the trace shows `dmg=25.00 orig=50.00` (the charged beam, halved by the boss's 0.5
vulnerability) and the boss's HP goes 600 -> 550.

**But the shots do not reach the weak spot.** The trace shows `weak=0` for every hit: the beam is
fired from the player's side and the spine joints are in the way, so the hits land on a body
joint (`weak=0`) and never on `L_middle2_boob_LCTR` (the type-1 joint, the only one with the weak
spot's vulnerability and the x2 damage multiplier). With `d = 1` the projectile starts inside the
target's sphere and registers no touch at all (0 hits in 5 shots); with `d = 4` it hits something
(2 hits in 24 shots, 8%).

**And the accumulator cannot climb past the cap.** The port's `ApplyDamageMsg` grows `xd10` only
in the states 0, 4, 5, 9 and 10, and caps it at half the threshold (25) while no tongue has been
launched (`xc00_tongues < 1`); `BeachWait` and `Dive` reset it to 0. A charged hit in state 0
therefore leaves `xd10 = 25` for a few frames and the next `BeachWait` wipes it before
`fn_78_6458` is evaluated - the trace's heartbeat never sees a value above 0.0.

**So state 3 is still not reached** and the branch tables for `fn_78_35DC`, `fn_78_3320`,
`fn_78_348C` and `fn_78_9A44` are unchanged (0 entries each, the arms as listed above). What
would be needed: a hit that lands on the **weak** joint while the boss is in a tongue state
(4, 5, 9 or 10) - the tongue has to have run once, so `xc00_tongues` lifts the cap - and the
shots have to arrive before the next `BeachWait`/`Dive` reset.

The `fsm` reply is now honest ("entered; now B"), which confirms the forced entry works; the
remaining problem is only that the state does not survive long enough for the `LightHit` and
`HeavyHit` triggers to fire on the next frame.

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
