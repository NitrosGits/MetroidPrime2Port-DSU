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

## Stage2 (module 79) — verified

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
