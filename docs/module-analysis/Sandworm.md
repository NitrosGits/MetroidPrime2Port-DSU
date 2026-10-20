# Sandworm REL (module 56): `CSandworm` (Sandigger, Alpha Sandigger, Bomb Guardian) and `CSandwormEye`

This is an analysis to plan the host port, dated 2026-10-19. No port code exists yet.

**Sources:**
- the archive build's disassembly of `build/G2ME01/Sandworm` (`.text` 0x14FA8 bytes, 375 functions), its `.data`, `.rodata` and `.bss`, and the relocations;
- m2c pseudo-C of every function;
- the class and FSM strings in the MP2 Wii `RSO_Sandworm.rso`;
- `config/G2ME01/symbols.txt` for the names of DOL call targets.

The script type is `'WORM'`, from `MakeNamedScriptLoader('WORM', &REL_ENTRY(LoadSandworm))`. Every function is listed in
`Sandworm_functions.tsv`. A host-ready class sketch is kept outside the repo, in `/tmp/opencode/sw/CSandworm_layout.hpp`.

**Tags:**
- **[proven]**: read from an instruction, a relocation or a table entry.
- **[probable]**: a strong reading that no symbol pins down.
- **[?]**: a guess.

All `fn_56_X` names are offsets into the REL `.text`. All `data_X` names are offsets into `.data`.

**Shape in one line:** a 15-joint segmented burrowing worm (`front_spine_7` … `Skeleton_Root` … `back_spine_7`, collision spheres from a rodata table) with two separate eye actors (head and tail) that take the damage. One class serves all three forms. The constructor that runs is chosen by `canLinkTransfer` (0xb47dd18f), which really means "is Bomb Guardian". The Bomb Guardian form adds bouncing bombs, health-phase parameter blocks (5× `SLdrSandwormStruct`), Ing possession, and `SetBossParams`.

## a. Class inventory

| class | size | vtable | ctor / dtor | base | what it is |
|---|---|---|---|---|---|
| `CSandworm` | 0x1A28 [proven: `__nw__(0x1A28)`] | `data_884` (0x240 bytes) | `fn_56_12D68` (Sandigger and Alpha), `fn_56_14010` (Bomb Guardian) / `fn_56_1223C` | `CPatterned` (0x7C0; the first member write is at 0x7C0) | the enemy |
| `CSandwormEye` | 0x300 [proven: `__nw__(0x300)` ×2 in `fn_56_5060`] | `data_6F8` (0x98) | `fn_56_2434` / `fn_56_23C4` | **`CPhysicsActor`** [proven: the ctor calls `CPhysicsActor::CPhysicsActor`, and the vtable ends at `GetWeight`]. The repo header says `CActor`; that probably comes from the DOL TypesMatch chain, and the port must use `CPhysicsActor`. | eye target, named "Sandworm Eye Head" (x14C4) or "Sandworm Eye Tail" (x14C6) |
| `CBodyStateCmd` | 0x8 | `data_878` | dtor `fn_56_343C` | none | REL-local inline copy (DOL class) [proven: `{rtti,0,dtor}` vtable] |
| `CBCTauntCmd` | 0xC | `data_86C` | dtor `fn_56_3590` | `CBodyStateCmd` | inline copy. It is issued with cmd 0x15 (`kBSC_Taunt`) through `fn_56_14B58` [proven] |
| `CBCGenerateCmd` | 0x20 | `data_860` | dtor `fn_56_B4B0` | `CBodyStateCmd` | inline copy. Used by Descend and Rise through `fn_56_14C24` [proven: a 7-word copy into `mGenerate`] |
| `CBCMeleeAttackCmd` | 0x18 | `data_854` | dtor `fn_56_B800` | `CBodyStateCmd` | inline copy. Used by the Spit, Bomb and Grab states through `fn_56_14D40` [probable] |
| member at `CPatterned+0x620` | ? | `data_844` (0x10, all zero) | dtor `fn_56_12658` | ? | inline copy of a CAi member's dtor (knock-back controller) [?] |
| `CPatterned` dtor body | – | – | `fn_56_12474` | – | REL copy of `~CPatterned` [probable: it destroys the CAi members and the x620 member] |

The constructors spawn these DOL classes: `CElementGen` (0x338, four of them in `fn_56_1A38`), `CBouncingBomb` (0x218, in `fn_56_3BAC`, `9030` and `9660`), `CCollisionActorManager` (0x18, in `fn_56_CC8C`), `CProjectedShadow` (in `fn_56_9C10`) and `CSandwormEye` (in `fn_56_5060`).

Inline and template copies (no port body needed): 19 `inline-dol`, 6 `inline-hdr` (`SLdrSandworm.hpp` ctor/dtor/LoadTypedef and `CPASAnimParmData`'s ctor), 29 `template` (TCastToPtr wrappers, the `rstl::vector<CJointCollisionDescription>` chain, the token chains, and the `CAnimationState`-style command helpers `14B58`/`14C24`/`14D40`), and 36 weak destructors used only by exception handling.

## b. Layouts

### CSandworm (members from 0x7C0)

The full annotated version is in `CSandworm_layout.hpp`. Key members, with the property each one comes from:

| off | type | init / source | used by |
|---|---|---|---|
| 0x7C0 | int | 0 | – |
| 0x7C4 | `CPathFindSearch` | `(null, flags from patterned+0x120, patterned+0xEC, 1, 1)` | `GetSearchPath` [proven] |
| 0x8B0 | float | 0. Think adds dt to it | timers everywhere |
| 0x8C0 | bits | b7 = `unknown_0x06dee4c5 & 1`; b5 = 1 (cleared at the start of Think); **b4 = Bomb Guardian / b3 = Sandigger** [proven: the two ctors differ only here]; b0 is set by `ACTV` | `fn_56_3A20`, `30F8`, Think |
| 0x8C1 | bits | b7 = ForceUnderground. Set by msg `IM00` on the non-BG forms | `fn_56_3E6C` |
| 0x8C4..0x963 | 5× `optional<SLdrSandwormStruct>` (0x20 each, flag at +0x1C) | BG: `sandwormStruct` …`_0xb89dfe86`. Sandigger: empty | `fn_56_3990` picks the first struct whose float0 (`unknown_0x98106ee2`) is below the current health, so float0 is a health threshold [probable] |
| 0x970 | `reserved_vector<CSegId,13>` | filled in `fn_56_128B0` through `fn_56_E6BC` | spine helpers |
| 0x998 | `CCollisionActorManager*` | null. Created lazily by Think through `fn_56_CC8C`; destroyed by `XDEL` | collision |
| 0x99C | `reserved_vector<0x90-byte,15>` [?] | count 0 | spine segment state |
| 0x1210 | `reserved_vector<CVector3f,13>` [probable] | count 0 | radar position (`fn_56_5938`) |
| 0x12B0 | `reserved_vector<int,…>` | 13 entries of `data_6B4` (Sandigger) or `data_6B0` (BG) | [?] |
| 0x1354 | `CTransform4f` | identity | – |
| 0x1388/94/A0 | `CVector3f` ×3 | zero | Ing-snatch curve (`312C`, `3180`) [probable] |
| 0x13BC | `CProjectedShadow*` | null | `fn_56_9C10` |
| 0x13C0 | `reserved_vector<CTransform4f,4>` | 4 identities | [?] |
| 0x1484 | `optional<CToken>` | `'FSM2'` token from the patterned AI's state machine | FSM |
| 0x1490 | `SAttackHistory` | picked = -1, cur = -1, history empty | picked attack: 1 = Spit, 5 = Toss, 6 = Spread, 7 = Fountain [proven]; 4 = charge [probable] |
| 0x14B4 / 0x14B8 | float | `pursuitFrustrationRadius` / `pursuitFrustrationTimer` [proven] | Pursue |
| 0x14C4 / 0x14C6 | `TUniqueId` | head and tail eye ids | `E134`..`E1D0` |
| 0x14D8 | `SChargeParams` (0x34) | `chargeRangeMin`, `chargeRangeMax`, `chargeImpulseHorizontal`, `chargeImpulseVertical` (the impulses are clamped to ≤ 50) [proven] | Charge |
| 0x1518 / 0x151C | int | -1 / 0xD: the body-command request (`fn_56_A9F4`) | states |
| 0x1528 | 0x84-byte spit block | `fn_56_138F0`: `CProjectileInfo(projectile, projectileDamage)`, plus `spitAttackVisorEffect`, NaN, 0, `spitAttackMinRange`, `spitAttackMaxRange`, `unknown_0x61f75902` [proven arguments] | `ProjectileInfo()`, SpitAttack |
| 0x15AC | `STossParams` | `morphballTossDamage`, `morphballTossImpulseH`, `morphballTossImpulseV` [proven] | Grab |
| 0x1620 | `SMeleeParams` | `pincerSwipeDamage`, `meleeImpulseH`, `meleeImpulseV` [proven] | Melee |
| 0x164C | float | `unknown_0xe593f1c6` | – |
| 0x1650 / 0x1654 | float | `lurkUndergroundTimeMin` / `lurkUndergroundTimeMax` [proven] | Lurk |
| 0x1670 | 0x100-byte block | `fn_56_136E0`. `XALD` resets 13 quaternions | [?] |
| 0x1770 / 0x1774 / 0x1778 | `CAssetId` ×2, float | `pincerL`, `pincerR`, `pincerScale` [proven] | `fn_56_128B0` builds models at 0x177C and 0x17CC |
| 0x1828 | `SSfxIds` | `walkSound`, `walkVocalSound`, `meleeAttackSound`, `eyeKilledSound`, then (BG only) `bombBounceSound` (0x1830) and `bombExplodeSound` (0x1832) [proven] | sound helpers |
| 0x1858 | `CPathFindSearch` | second search | [?] |
| 0x1944 | `SBombParams` (0x3C) | BG: `ingBossBombFX`, `ingBossBombExplosionFX` (`PART` tokens, locked), `ingBossBombDamage` [proven] | bombs |
| 0x1980 / 0x1984 | float | `ingBossBombDropRate` (BG) / drop timer | `3A8C`..`3BAC` |
| 0x19A8 | `SBombPhase` (0x28) | BG: x19B8 = `unknown_0xefef7b45`, x19C0 = `unknown_0x547f9400` | damage accumulated in x19C4 above x19C0 forces BombFountain (x1490 = 7) [probable] |
| 0x19D0 | `SGlowParticles` | four `CElementGen`. Sandigger: `eyeGlow` ×2. BG: `eyeGlow` ×2, `pART`, `pART_0x8b2a15ee` [proven] | AddToRenderer draws gens 2 and 3 |
| 0x1A0C | `CAABox` | `mskNullBox` | bounds |

Members touched outside the CSandworm functions: the eyes are reached only through `TCastToPtr<CSandwormEye>` (`fn_56_E134`..`E1D0`). The DOL radar reaches the object only through the two func-pointer callbacks.

### CSandwormEye (members from 0x2D0)

| off | type | init | notes |
|---|---|---|---|
| 0x2D0 | `CAABox` | `mskNullBox` | touch bounds (`231C` set, `2350` get) [proven] |
| 0x2E8 | `CVector3f` | zero | orbit/aim point (`23A8`, `238C`, `2210`, `222C`) [proven] |
| 0x2F4 | bits | b7, b6 = 0 | b6 = dead/killed (`21F0` set, `2204` get) [probable] |
| 0x2F8 | `CRELFileToken` | name in `bss_C` | keeps the RSO loaded [probable] |

The ctor builds a null `CModelData` and a material list with bits `data_6D4`/`6D8`/`6DC`. It passes a default `CActorParameters`, `mskNullBox` and `skDefaultStepData` to `CPhysicsActor`.

## c. Loader, registration, radar

**Registration** [proven]: `RELMain` calls `fn_56_70`, which fills `bss_38` with `{LoadSandworm = fn_56_DC, PTMF data_6E0 → fn_56_5938, PTMF data_6EC → fn_56_5910}` and calls `SetSSandworm_FuncPtrs(&bss_38)`. `RELExit` calls `SetSSandworm_FuncPtrs(0)`.

**Radar** [proven]:
- `fn_56_5910` = **GetRadarPointCount**. It returns 13 when the object is active (x20 b7) and x420 b6 is set; otherwise it returns 0.
- `fn_56_5938` = **GetRadarPointPosition(index)**. It reads x1210 count ≤ index ? `fn_56_1096C` (locator position) : `fn_56_10090` (segment position).

**Loader `fn_56_DC`** [proven]:
1. It reads every `SLdrSandworm` property. The hash → stack-slot table matches `SLdrSandworm.inc` exactly; the 5 structs go through the inlined `LoadTypedefSandwormStruct` (`fn_56_1704`).
2. It builds `CModelData` (`LdrToModelData`). If no model is loaded, it returns null.
3. **`canLinkTransfer` (0xb47dd18f) == 1** gives the Bomb Guardian: `new(0x1A28)` then `fn_56_14010`. It passes 4 `CDamageInfo` (projectile, morphball toss, pincer, ingBossBomb) and everything listed below.
4. Otherwise it builds the Sandigger/Alpha form: `fn_56_12D68` with 3 `CDamageInfo` and none of the bomb, phase or glow-2/3 inputs.
5. Every sound id of -1 becomes `kInternalInvalidSfxId`.

| | Sandigger / Alpha (`12D68`) | Bomb Guardian (`14010`) |
|---|---|---|
| x8C0 b4 / b3 | 0 / 1 | 1 / 0 |
| x8C4 phase structs | absent | the 5 `SLdrSandwormStruct` |
| x12B4 fill | `data_6B4` | `data_6B0` |
| x1828 bomb sfx | invalid | `bombBounceSound` / `bombExplodeSound` |
| x1944 bomb params | empty | FX ×2 + damage |
| x1980 drop rate | 0 | `ingBossBombDropRate` |
| x19A8 phase | zeros | `unknown_0x547f9400`, `unknown_0xefef7b45` |
| glow gens | `eyeGlow`, `eyeGlow`, -, - | `eyeGlow`, `eyeGlow`, `pART`, `pART_0x8b2a15ee` |
| possession | `CanBeIngPossessed` = false | `CPatterned::CanBeIngPossessed` |
| Think extras | pincer fade (x3C8) while `pincerScale` < 1 | `SetBossParams` (`fn_56_10F90`) once health > 0.99 |

**Alpha versus normal Sandigger:** there is no code difference in the loader or the ctor. The difference is property-driven (patterned info, scale, damage) [probable].

**What it creates:**
- At construction: 2–4 `CElementGen` and the pincer models (`fn_56_128B0`).
- On `XCRT` (`fn_56_5060`): the 2 eyes. They are added to the state manager.
- Lazily in Think: the collision actors (`fn_56_CC8C`, 15 spheres from the rodata spine table).
- From attacks: the `CBouncingBomb` instances and spit projectiles (`LaunchProjectile` in `fn_56_84B8`).

**Script messages** (`fn_56_11374`) [proven]:
- `XCRT`: locomotion, body Activate, remove material 0x29, CreateEyes, x1A00.
- `ACTV`: sets x8C0 b0.
- `DCTV`: `colMgr->SetActive(false)`.
- `XDEL`: destroys the collision actor manager, deletes both eyes, stops the move sound.
- `XALD`: resets x1670.
- `XXDG` / `XRDG`: x420 b7 when the sender is a collision actor.
- `XDMG`: routes weapon damage. A hit on an eye (`566C` head / `56D4` tail) applies damage, kills an eye at half health, and runs BG health thresholds (rodata_E8 ×3, `fn_56_95E0`).
- `XHIT`: contact damage to the player. It also calls `fn_56_7EE4` when the attack history ends with 4.
- `IM00`: sets ForceUnderground on the non-BG forms.

Every message also goes on to `CPatterned::AcceptScriptMsg`.

## d. Vtables

### CSandworm `data_884`

Slot names follow `vtable-offsets.md`. Unlisted slots in 0x08..0x144 inherit from the DOL. **PreThink, SetupStateMachine, Death and KnockBack are not overridden.**

| slot | fn | | slot | fn |
|---|---|---|---|---|
| 0x08 dtor | `1223C` | | 0xC8 GetModelAlphau8 | `14ADC` (x44F) |
| 0x14 Think | `110C0` | | 0xDC ProjectileInfo | `6790` (&x1528) |
| 0x18 AcceptScriptMsg | `11374` | | 0xE0 GetSearchPath | `14A64` (&x7C4) |
| 0x24 PreRender | `A4F0` (→ CPatterned) | | 0xE4 GetPointSearchPath | `14AE4` (null) |
| 0x28 AddToRenderer | `4A14` | | 0xF4 GetRagDoll | `14AEC` (null) |
| 0x2C Render | `E3C0` | | 0xF8 CanBeIngPossessed | `30F8` |
| 0x34 PreRenderAllViewports | `9CD8` | | 0xFC CanBeUnPossessed | `30F0` (false) |
| 0x3C GetHealthInfo | `0` (inline copy) | | 0x10C GetAttackTarget | `14AF4` (invalid) |
| 0x48 GetTouchBounds | `14A74` | | 0x110 IsOnGround | `14B04` |
| 0x50 GetOrbitPosition | `2C30` | | 0x114 GetGravityConstant | `14B10` |
| 0x54 GetAimPosition | `321C` | | 0x124 GetDeathExplosionParticle | `14B1C` (&x754) |
| 0x70 DoUserAnimEvent | `646C` | | 0x138 GetIngSnatchingNormal | `3180` |
| 0xA4 TakeDamage | `11CE8` | | 0x13C GetIngSnatchingPoint | `312C` |
| 0xA8 CanBeShot | `14B24` (true) | | 0x140 GetIngSnatchingModelOverlapSize | `9860` (6.0) |
| 0xAC IsListening | `14B2C` (false) | | 0x144 RenderIngSnatchingTransition | `2994` |
| 0xB0 Listen | `14B34` (false) | | 0x148..0x1D8 | 37 triggers (section e) |
| 0xB4 GetOrigin | `14B3C` | | 0x1DC..0x234 | 23 states (section e) |
| | | | 0x238 | `33A4` **Interrupt** (from the `data_6A0` table) |
| | | | 0x23C | `14A6C` (returns 0) [?] |

**Where the MP2 order is uncertain:**
- Slots 0xA8..0xB4 and 0x10C..0x124 are named from `vtable-offsets.md`, which was checked against SpacePirate. The bodies (true/false/null, `x34C` bit, `x754`) match those names [probable].
- 0x23C has no name.
- Inside the trigger block the vtable order is not alphabetical (0x160 = CanMeleeAgain before 0x164 = CanSpitAgain; 0x1A4 = Spread before 0x1A8 = Toss; 0x1B4 = PathMesh before 0x1B8 = Morphball; 0x1D0 = SnatchStarted before 0x1D4 = SnatchEnded). The port must declare them in **vtable order**, which is the order in the layout header.

### CSandwormEye `data_6F8` (0x98)

| slot | fn |
|---|---|
| 0x08 dtor | `23C4` |
| 0x3C GetHealthInfo | `0` |
| 0x48 GetTouchBounds | `2350` |
| 0x50 GetOrbitPosition | `23A8` |
| 0x54 GetAimPosition | `238C` |

Every other slot is `CPhysicsActor` or `CActor`. Render is `CPhysicsActor::Render`.

## e. FSM

The graph (states, transitions, trigger arguments) lives in the **AFSM asset** referenced by the `'FSM2'` token at x1484. The REL only registers names. `fn_56_D548` calls `InitializeStateMachine`, then registers the triggers `data_1C0` (0x25), the states `data_524` (0x17) and the extras `data_6A0` (1: Interrupt). Think calls it while the FSM is not ready.

States follow the usual EStateMsg protocol (0 = enter, 1 = update, 2 = exit). The body-command plumbing is common to them:
- `A9F4`/`AA00` set or clear the request in x1518/x151C;
- `14B58`/`14C24`/`14D40` issue Taunt, Generate and MeleeAttack commands;
- `B85C` is the common attack start;
- `5CA4` resets the attack timer from `GetAverageAttackTime`.

**States** (all one-liners are [probable] unless marked otherwise; WP = work package):

| state | fn | WP | behaviour |
|---|---|---|---|
| BombFountainAttack | `B8B8` | 6 | MeleeAttack cmd, then spawns or launches the held bomb (`9368`, `95E0`) |
| BombSpreadAttack | `BAB8` | 6 | MeleeAttack cmd, rotates the body (`EF8C`), spreads bombs from anim events |
| BombTossAttack | `BC8C` | 6 | the same as Spread plus held-bomb handling |
| ChargePlayer | `8190` | 4 | locomotion and turn speed, charge impact damage (`7EE4`), stops looped sounds at exit |
| ChargeWindUp | `99A0` | 4 | Taunt cmd (`14B58`) |
| Deactivate | `5E5C` | 3 | trivial |
| Dead | `5D80` | 3 | kills eyes (`21F0`), `CPatterned::Dead`, DeathDelete, death FX (`11FAC`) |
| Descend | `B320` | 3 | Generate cmd and script messages; collision actors toggled |
| EyeKilledReaction | `3484` | 3 | Taunt cmd, re-orients (`AD14`) |
| GrabAttack | `BF44` | 4 | MeleeAttack cmd, grabs or releases the player (`6144`, `63B8`) |
| LurkUnderground | `C3F4` | 3 | locomotion and teleport to a script actor (`C468`) |
| MeleeAttack | `6FA0` | 4 | picks a PAS anim (`7138`), melee sound, attack timer |
| Null | `112E8` | 3 | sets underground collision (`AC24`) |
| Patrol | `AA74` | 3 | wobble steering, `CPatterned::Patrol`, move and vocal sounds, bomb drops (`3A8C`) |
| PickMediumAttack | `37A0` | 4 | picks the attack into x1490 (`3844`) |
| Prime | `F754` | 5 | spine/IK prime (`F77C`) |
| Pursue | `74B8` | 3 | path find, cover point, steering, sounds, bomb drop, frustration |
| Rise | `B50C` | 3 | Generate cmd, locomotion, script messages |
| Snatch | `986C` | 3 | Taunt cmd via `CanIssueCommand`; sets x1520 b7; x3C8 = 0.66 [proven] |
| SpitAttack | `B6DC` | 4 | MeleeAttack cmd. The projectile itself is fired from DoUserAnimEvent (`8AAC`) |
| StopGrabAttack | `BEA4` | 4 | Taunt-style cmd; clears the request |
| Straighten | `4200` | 3 | NextState cmd, LookAt / SetTransform to straighten the spine [proven calls] |
| Sulk | `5470` | 3 | locomotion and random timer (`55F8`), moves to a script actor |
| Interrupt | `33A4` | 3 | delivers `CBodyStateCmd` 0xC (NextState) then 0xD (AbortScripted) [proven] |

**Triggers:**

| trigger | fn | WP | reads |
|---|---|---|---|
| Activated | `C3D0` | 3 | x20 b7 (active) [proven] |
| BeginUnderground | `C3E8` | 3 | x8C0 b7 [proven] |
| CanAttack / CanCharge | `59A8` / `5AE0` | 4 | possession and snatch checks, AI hint / triggers 0x150, 0x190, 0x178 |
| CanDescend | `3E1C` | 3 | – |
| CanGrabMorphball | `C168` | 4 | AI hint, history |
| CanMeleeAgain / CanSpitAgain | `573C` / `3DE4` | 4 | history (`26C4`) |
| ChargeOver | `6798` | 4 | NearlyOnPath / distance |
| DoneLurking / DoneStraightening / DoneSulking | `58C0` / `4384` / `541C` | 3 | timers |
| FacingPlayer / FacingPlayerForCharge | `6A50` / `6904` | 3 / 4 | angle (`6AA8`) |
| ForceUnderground | `3E6C` | 3 | x8C1 b7 [proven] |
| GrabAttackFinished | `BF38` | 4 | x1604 b6 [proven] |
| HitDuringWindUp | `35EC` | 4 | x19B4 b7 [proven] |
| InAttackRange / InChargeRange / InSpitRange / MeleeAttackRange | `C350` / `6C04` / `C27C` / `73BC` | 4 | ranges |
| OneEyeKilled | `35F8` | 3 | eye dead flags |
| PickedBombFountain / Spread / Toss / SpitAttack | `3750` / `3764` / `3778` / `378C` | 6 / 6 / 6 / 4 | x1490 == 7 / 6 / 5 / 1 [proven] |
| PlayerHiding / PlayerIsOnPathMesh / PlayerReachable | `3E78` / `5978` / `3D44` | 3 | path and visibility |
| PlayerIsMorphball | `7390` | 4 | – |
| Primed | `F710` | 5 | always true [proven] |
| ReadyToCharge | `97F0` | 4 | x1508 b7 [proven] |
| SequenceAttack | `5530` | 4 | – |
| ShouldSulk | `557C` | 3 | – |
| SnatchStarted / SnatchEnded | `981C` / `97FC` | 3 | `IsPossessedBombGuardian` |
| SpitAngleOK | `69DC` | 4 | `bss_54` angle and `6B64` |

**Transitions visible in code:**
- `XDMG` sets x1490 = 7, which makes PickedBombFountain fire.
- A BG health threshold sets x19B4 b7 (HitDuringWindUp) and deletes the held bomb.
- `IM00` sets ForceUnderground.
- `XHIT` with history ending in 4 applies charge damage.

Everything else is in the AFSM.

## f. Function table

`Sandworm_functions.tsv` has 375 rows. Its columns are offset, size, owner, proposed name ([?] = guessed), kind, REL callers (or the data slot), DOL counterpart, and package.

| kind | count | kind | count |
|---|---|---|---|
| method | 157 | weak-dtor | 36 |
| fsm-trigger | 37 | virtual | 34 |
| template | 29 | fsm-state | 24 |
| inline-dol | 19 | member-ctor | 12 |
| inline-hdr | 6 | member-dtor | 5 |
| unreferenced | 5 | ctor/dtor | 3/2 |
| rel-entry / loader / radar | 3/1/2 | | |

## g. Work packages

`-` (75 functions, 0x20E8 bytes) means no port body: inline, template or weak copies that come from headers. Byte totals are REL `.text` sizes.

| WP | file (`src/MetroidPrime/Enemies/`) | fns | bytes | contents | depends on |
|---|---|---|---|---|---|
| WP1 | `PortCSandworm.cpp` | 67 | 0x4664 | `CSandworm.hpp` layout, both ctors and member structs, dtor, loader and `SSandworm_FuncPtrs` registration, radar, the simple virtual overrides, Think and its per-frame helpers, FSM init, the shared helpers (`3A20`, `3990`, `A9F4`/`AA00`, `26C4`, cmd helpers `14B58`/`14C24`/`14D40`) | none (do it first) |
| WP2 | `PortCSandwormEye.cpp` | 34 | 0x2504 | the `CSandwormEye` class; CreateEyes; eye lookup and predicates (`E134`..`E340`, `36A0`); UpdateEyes (`441C`, `4AD8`, `CAF0`); additive flinch; Orbit/Aim position; **AcceptScriptMsg** (damage routing is eye-driven) | WP1 |
| WP3 | `PortCSandwormStates.cpp` | 49 | 0x25D0 | locomotion and lifecycle states (Patrol, Pursue, Descend, Rise, Lurk, Sulk, Straighten, Snatch, Interrupt, EyeKilledReaction, Dead, Deactivate, Null) with their triggers, path/cover helpers and move/vocal sounds | WP1, WP5 (`AD14`, `AC24`) |
| WP4 | `PortCSandwormAttacks.cpp` | 63 | 0x356C | melee, spit (projectile and visor effect), charge, grab/toss; their triggers; **DoUserAnimEvent**; contact and charge damage | WP1, WP6 (bomb anim events) |
| WP5 | `PortCSandwormSpine.cpp` | 42 | 0x33FC | spine/segment chain and IK (`C600`..`C994`, `E764`..`10A20`), body orientation (`AD14`), collision actor manager setup and update (`CC8C`, `D09C`, `D350`), Prime/Primed | WP1 |
| WP6 | `PortCSandwormBombs.cpp` | 26 | 0x1874 | the three bomb states and picks, `CBouncingBomb` spawn, hold and release, patrol bomb drop, bomb sfx and params | WP1, WP4 (`B85C`, `5CA4`) |
| WP7 | `PortCSandwormRender.cpp` | 19 | 0x1BAC | PreRender/AddToRenderer/Render/PreRenderAllViewports, projected shadow, bounds, locator transforms, spine raycasts (`D690`, `E008`), snatch-transition render | WP1, WP5 |

These are balanced by effort, not by bytes. WP1 carries 0x2364 bytes of mechanical ctor and loader code.

Shared helpers belong to WP1 and are declared in `CSandworm.hpp`. WP3 and WP4 also call WP5's `AD14` and `E5F8`. Recommended order: WP1, then WP5, then WP2/WP3/WP7 in parallel, then WP4 and WP6.

## Open questions

1. The element types and capacities of x99C, x12B0 and x1670, and what `data_6B0`/`6B4` mean as 13 fill values.
2. The exact layout of the 0x84-byte spit block at x1528 (`CProjectileInfo` plus visor effect).
3. The meaning of `unknown_0x06dee4c5` (x8C0 b7, BeginUnderground) and of the SLdrSandwormStruct ints.
4. The names of vtable slot 0x23C and of the x620 member class (`data_844`).
5. The DoUserAnimEvent event-type → action mapping (`jumptable data_790`); it needs a per-case read in WP4.
6. The `CSandwormEye` base mismatch with the repo header (`CActor` versus `CPhysicsActor`).

## Port status (2026-10-19)

- All WP5 bodies are ported in `PortCSandwormSpine.cpp` (worker lanes), and the `TODO(WP5)` stubs are gone. 10110 and 10268 were skipped because nothing references them. Guessed flags: CAnimData+0x2AC|=2 is `SetPoseBuilt(true)`, and the bit0 test is `!Pose().HasScale()`. Host build `0 FAILED`, `gen_files_cmake --check` rc 0, and the CSandworm.hpp TARGET_PC strip-diff vs the baseline is clean (checked with a Python stripper; unifdef isn't installed).
- Runtime (`01_sand_entrance`, area 2): both worms work. u0596 is activated by trigger u0488 and u0517 by trigger u052c. Each cycles Pursue/SpitAttack, toggles its head/tail eyes, and dies at 0 HP: Dead state, then deleted along with its eyes and collision actors. 0 caught errors and 0 reach-stubs. When the player leaves the area, the worm runs Straighten → Descend → LurkUnderground → Deactivate and goes home.
  - Earlier kills used console `damage <worm uid>`, which skips the eye path. The eye path from real
    weapon fire is now exercised too — see the next bullet.
- WP2 has no missing body. Its 35th row, `fn_56_7138` (0x184), is the inlined `CPASAnimParmData`
  ctor, not a `CSandworm` method: both call sites (`6FA0` MeleeAttack, `11E54` AddAdditiveFlinch)
  build one `CPASAnimParmData` each. Read from the asm: `6FA0` passes state `0x1A` and eight
  `CPASAnimParm::NoParameter()`; `11E54` passes state `0x18` and `FromEnum(eye ? 10 : 9)` as parm 1
  plus seven NoParameter; the thresholds are `rodata_150` = 0, `rodata_128` = 1.0 and `rodata_124`
  = 0.5. Both port bodies match. The function row and the WP2 line in section g are corrected
  (WP2 = 34 functions, 0x2504 bytes).
- Eye weapon path (`01_sand_entrance`, area 2, 2026-10-19): with a temporary stderr probe on
  `AcceptScriptMsg`'s `kSM_Damage` arm and `CSandwormEye::SetDead` (not committed), the player
  killed both worms with the power beam only, no console `damage`. A hit arrives as
  `kSM_Damage` from the tail eye's collision actor with a `CWeapon` as `GetLastTouchedObject`; the
  worm's HP drops 2 per shot (120 → 0), and at `hp <= 0.5 * initialHP` the tail eye takes `SetDead`
  (0224 for u0117 and 0226 for u0196, both printed at hp 62 → 60), after which hits still land
  (`AnyEyeDead` true) until the Dead state kills both eyes and the worm is deleted. `566C`
  (head-eye hit test) is not reachable for the Sandigger forms, as expected.
  - Run method: `MP2_WORLD=01_sand_entrance MP2_CONSOLE=<port> MP2_GODMODE=1` under
    `gdb -x cov2.py` (breakpoints on every `// @ fn_56_` and `(ported, not exercised yet)` body),
    then console `objs sandworm`, `tp`/`face` 10 m from the worm every 4 shots, `press l 0` to hold
    lock-on and `press l+a 2` / `press l 6` to fire. Both worms ended at 0 HP; 0 caught errors, 0
    reach-stubs. That run promoted 13 markers: `14B58` `56D4` `F384` `C468` `2718` `33A4` `3484`
    `4200` `4384` `58C0` `5E5C` `B320` `C3F4`.
- Charge group (`01_sand_entrance`, 2026-10-20): the Sandigger charges only when the player stands
  inside `x14D8_charge` = 15..50 m (prefab properties; a probe on `CanCharge` printed the live range
  and each gate). Standing still 22 m in front, in the worm's arc, all six gates read true
  (`attack`, `range`, `onPath`, `ffc`, `reach`, `facing`) and the FSM ran ChargeWindUp → ChargePlayer →
  charge damage, which exercises the whole group: `8190`, `6CC0`, `7EE4`, `6798`, `97F0`, `99A0` and
  the two prerequisites `3D44` (PlayerReachable) and `6A50` (FacingPlayer). Closer than 15 m
  `InChargeRange` is false (spit only); at 18 m the run's `tp` landed off the path mesh
  (`onPath=0`), so placement matters, not distance alone.
- Markers: 212 functions carry `// @` — the previous 204 plus the 8 charge bodies. 71 remain
  `(ported, not exercised yet)`, including GetRagDoll, GetDeathExplosionParticle and
  RenderIngSnatchingTransition; `/tmp/opencode/sw2/unexercised79.md` (local) buckets them:
  A 24 (now 16 left), B 1, C 36 (Bomb Guardian/possession), D 12, E 6.
- Open:
  - BeginAttackHelper/2 still operate on x1490_attacks rather than SSandwormAttackHistory.
  - DoUserAnimEvent event numbers vs jumptable data_790; the CBodyController+0x4C write in ChargePlayer.
  - fn_56_6798 passes `arg0+0x54` and rodata_150 to A8E0; the port passes `pos, 0.f`.
  - CBCTauntCmd's second word is lost on the host; constants 0.4-1.2, 0.1-0.5, 0.2094 and rodata_194 are not pinned.
  - TypesMatch registers the eye under CActor, but on PC it derives CPhysicsActor.
  - The "Sandworm.rel" token string at lbl_56_bss_C is a guess.
  - Render line 25 skips a missing model.
  - x1614 has no name; x199A_ is a byte.
- Pseudo-C is in `/tmp/opencode/pseudo/Sandworm/fn_56_XXXX.c`. Runtime areas are Agon Wastes `01_sand_entrance` L0, `03_sand_temple` L2 and `04_sand_mining` L3 (docs/module_spawns.tsv). The bar is a fight the player can finish.
