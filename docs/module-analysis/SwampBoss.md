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

Behaviour port, not matching. `// @ fn_78_<off>` marks the 54 functions a traced LAYER3 run
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

### Stage1 runtime status (2026-10-20)

Traced runs in `04_swamp_temple_dark` with `LAYER3@200+4` (forced layer), MP2_GODMODE, a temporary
per-function trace/state probe (never committed), the player driven through the debug console.

- **Runs:** 0 `[port] caught`, 0 `[reach-stub]`, `exit=0`.
- **Spawn:** u05bb / editor 002001c1 at (−166.1, −123.7, 86.0), inactive until `Activate`; then all
  16 joint collision actors exist (joint HP 10000, initial 0).
- **Attack cycle:** the FSM loops SplashWait (−1) ↔ Bob (1) → SplashRise/Jump/Descend (7) →
  SplashTelegraph (8) and back; `Spit`, `LaunchShockWave`, `StartTelegraph`/`UpdateRing`,
  `ShreddersOn/Off`, `Rumble`, `Swim`, `SetBeachMaterials`/`SetJointsActive` all run.
- **Damage path:** close-range volleys (≤15 units from a joint) reach `ApplyDamageMsg` and move the
  boss 1200 → 1199; the body joints are reflect outside the vulnerable window, so those hits land
  at the reflect's 1 point. `SplashJump`'s Activate sets `kAllJoints` to `kJF_Vulnerable` (and its
  Deactivate back to `kJF_Reflect`), so retail damage comes from shooting the leap. 260 s of
  continuous fire from the arena ring (46 units out, state 7 three times) did not reach a joint at
  all, so the harness only lands hits from close range — the kill is scripted-aim work, not a port
  gap.
- **Not reached:** Beach, ExposeBelly, PlatformReady, TongueLoop/TonguePull, Vomit,
  HeavyHitReact, Dead. Two harness limits and two config facts:
  - Phase 0's attack slots are `0,0,0,2` (first..fourth), so `ShouldBeach` (`CurrentAttack() == 1`)
    never fires in phase 0 in retail either; the splash cycle is what the disc asks for at 1200 HP,
    and the phase only advances below 50 HP.
  - `ReadyToTongue` is behind `xbf8_platformTime >= 0.65`, and that clock only accrues in
    PlatformReady (state 4), i.e. the tongue belongs to the Beach → PlatformReady → TongueLoop
    chain. It also needs the player inside the arena `CScriptAIHint` (type 21) within its radius;
    the only hint (u05b4) sits 1.7 units from the boss centre, and the player teleported around it
    never stayed inside (`PlayerInsideHint` logged 0 at 24–36 units out).
  - Console `damage` cannot stand in for a shot: `ApplyDamageMsg` needs a `CGameProjectile` as the
    joint's last toucher, so a damaged joint with no projectile does nothing to the boss.
- **Counted markers:** the runs traced 54 functions (48 markers kept, 6 added for triggers the run
  evaluated); every ported-but-unexercised body carries none.

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
