# Sandworm REL (module 56): `CSandworm` (Sandigger, Alpha Sandigger, Bomb Guardian) and `CSandwormEye`

The module's analysis note. Written 2026-10-19 to plan the host port, extended 2026-10-21 with
the full function classification (section g), the upstream-class check (section h) and the
canonical port order (section i).

**The module is ported.** The bodies live in `src/MetroidPrime/Enemies/PortCSandworm*.cpp`;
253 of the 262 module-behaviour functions carry a `// @ fn_56_` marker (268 markers in
total; the other 15 sit on the thunks and the inline command helpers). The
`docs/modules.tsv` row for Sandworm still says `status=entry-stub`, which is stale bookkeeping:
the row was written before the port, and `tools/rel_manifest.py` derives the status from
`files.cmake`, which has listed the seven `PortCSandworm*.cpp` files since 2026-10-19.
Regenerate the manifest (`python3 tools/rel_manifest.py`, needs the disc) to fix the row.

**Sources:**
- the archive build's disassembly of `build/G2ME01/Sandworm` (`.text` **0x15AA8 = 88,744 bytes**,
  **383 function symbols**: 375 module functions `fn_56_0` … `fn_56_14FA8`, plus `RELMain`,
  `RELExit` and 7 crtexe runtime symbols), its `.data`, `.rodata` and `.bss`, and the relocations;
- m2c pseudo-C of every function (the asm it was made from is
  `<archive>/build/G2ME01/Sandworm/asm`; `tools/rel_pseudo/m2c_module.sh Sandworm` rebuilds the
  pseudo-C into `/tmp/opencode/pseudo/Sandworm/` when m2c is installed - it is not installed in
  this session, so that directory only holds a README);
- the class and FSM strings in the MP2 Wii `RSO_Sandworm.rso`;
- `config/G2ME01/rels/Sandworm/symbols.txt` for the names of DOL call targets.

The script type is `'WORM'`, from `MakeNamedScriptLoader('WORM', &REL_ENTRY(LoadSandworm))`.
Every one of the 383 function symbols is classified in section g; `Sandworm_functions.tsv` is the
machine-readable copy (383 rows, with the added `class` column).

**Tags:**
- **[proven]**: read from an instruction, a relocation or a table entry.
- **[probable]**: a strong reading that no symbol pins down.
- **[?]**: a guess.

All `fn_56_X` names are offsets into the REL `.text`. All `data_X` names are offsets into `.data`.

**Shape in one line:** a 15-joint segmented burrowing worm (`front_spine_7` … `Skeleton_Root` … `back_spine_7`, collision spheres from a rodata table) with two separate eye actors (head and tail) that take the damage. One class serves all three forms. The constructor that runs is chosen by `canLinkTransfer` (0xb47dd18f), which really means "is Bomb Guardian". The Bomb Guardian form adds bouncing bombs, health-phase parameter blocks (5× `SLdrSandwormStruct`), Ing possession, and `SetBossParams`.



## a. Module shape

| | |
|---|---|
| REL | `config/G2ME01/rels/Sandworm`, module id **56**, `.text` **0x15AA8 = 88,744 bytes** |
| function symbols | **383** in `config/G2ME01/rels/Sandworm/symbols.txt`: 375 module functions (`fn_56_0` … `fn_56_14FA8`), `RELMain`, `RELExit` and 7 crtexe runtime symbols (`_prolog`, `_epilog`, `_unresolved`, `ModuleConstructors`, `ModuleDestructors`, `__register_global_object`, `__destroy_global_chain`) |
| script object type | **`'WORM'`** — `MakeNamedScriptLoader('WORM', &REL_ENTRY(LoadSandworm))`, `src/MetroidPrime/ScriptLoader.cpp:196` |
| in-game names | Morphology: **Sandigger**, Morphology: **Bomb Guardian**, Morphology: **Alpha Sandigger** (`docs/modules.tsv`) |
| spawn areas | Agon Wastes `01_sand_entrance` (L0), `03_sand_temple` (L2), `04_sand_mining` (L3); 3 spawns |
| REL entry point | `_prolog` (0x159EC, crtexe) → `ModuleConstructors` (0x15A5C) → **`RELMain` (0x50)** → `fn_56_70` fills `SSandworm_FuncPtrs` (loader + the two radar callbacks) and calls `SetSSandworm_FuncPtrs` |
| REL exit point | `_epilog` (0x159C8) → `ModuleDestructors` (0x15A10) → **`RELExit` (0x2C)** clears the table |
| script loader (factory) | **`LoadSandworm` (`fn_56_DC`, 0xF6C)** reads every `SLdrSandworm` property, builds the `CModelData` (null ⇒ no actor) and news the enemy |
| actor constructor | **`fn_56_12D68`** (0x834) Sandigger/Alpha form, **`fn_56_14010`** (0x9C4) Bomb Guardian form; the choice is `canLinkTransfer == 1` (property 0xb47dd18f) |
| eye constructor | **`fn_56_2434`** (`CSandwormEye`, 0x300 bytes); the two eyes are created by `fn_56_5060` (CreateEyes, on `XCRT`) |
| host entry point | `src/MetroidPrime/Enemies/SSandworm_FuncPtrs.cpp` (`RelProxy_LoadSandworm` → `gLoader_Sandworm->mLoadSandworm`) and `src/MetroidPrime/Enemies/PortCSandworm.cpp:896` (`PortLoadSandworm`), registered in the port's `RELMain` (`fn_56_50`, `PortCSandworm.cpp:963`) |

**Which form runs where.** Only one prefab in the whole game sets `canLinkTransfer`: `03_sand_temple`
layer 2 (mrea `75fc9aae`, editor id `001a009c`, hp 950) is the Bomb Guardian (`u048f`); the other
worm of that layer (`001a00e4`, hp 1000) and the `04_sand_mining` L3 worm (`00080189`) are Sandigger
forms. `docs/module_spawns.tsv` lists the *layer* the REL appears in, not the form.

**The port does not run the REL.** The host re-implements the module: `RELMain`/`RELExit` in
`PortCSandworm.cpp` fill the same `SSandworm_FuncPtrs` proxy table the REL fills, and the DOL's
`'WORM'` loader calls `PortLoadSandworm` instead of the REL's `fn_56_DC`. The REL's own runtime
(`fn_56_14FA8`, the 7 crtexe symbols) therefore has no host body; section g says so.


## b. Class inventory

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


## c. Layouts

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


## d. Loader, registration, radar

**Registration** [proven]: `RELMain` calls `fn_56_70`, which fills `bss_38` with `{LoadSandworm = fn_56_DC, PTMF data_6E0 → fn_56_5938, PTMF data_6EC → fn_56_5910}` and calls `SetSSandworm_FuncPtrs(&bss_38)`. `RELExit` calls `SetSSandworm_FuncPtrs(0)`.

**Radar** [proven]:
- `fn_56_5910` = **GetRadarPointCount**. It returns 13 when the object is active (x20 b7) and x420 b6 is set; otherwise it returns 0.
- `fn_56_5938` = **GetRadarPointPosition(index)**. It reads x1210 count ≤ index ? `fn_56_1096C` (locator position) : `fn_56_10090` (segment position).

**Loader `fn_56_DC`** [proven]:
1. It reads every `SLdrSandworm` property. The hash → stack-slot table matches `SLdrSandworm.inc` exactly; the 5 structs go through the inlined `LoadTypedefSandwormStruct` (`fn_56_1704`).
2. It builds `CModelData` (`LdrToModelData`). If no model is loaded, it returns null.
3. **`canLinkTransfer` (0xb47dd18f) == 1** gives the Bomb Guardian: `new(0x1A28)` then `fn_56_14010`. It passes 4 `CDamageInfo` (projectile, morphball toss, pincer, ingBossBomb) and everything listed below.
   - **Which prefab**: across Agon Wastes (every `Metroid3.pak` MLVL/MREA) only one worm sets it —
     `03_sand_temple` layer 2 (mrea `75fc9aae`, editor id `001a009c`, hp 950 on the disc). The other
     worm in that layer (`001a00e4`, hp 1000) and the `04_sand_mining` L3 one (`00080189`) are 0, i.e.
     Sandigger forms; `docs/module_spawns.tsv` lists the layer the REL appears in, not the form. At
     runtime the BG is `u048f` (flags `0x10`, spawned inactive) and the other is `u04e9`.
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


## e. Vtables

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


## f. FSM

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


## g. Function classification (all 383 symbols)

Three classes, applied by rule so the table can be audited:

- **H - host copy** (101 functions, 0x2BF8 bytes): the body is a copy of something the host
  already has, so the port writes no body for it. The last column names the DOL/upstream
  function it copies. Five sources: the crtexe REL runtime (7 symbols), compiler-emitted
  destructors (36 `weak-dtor` for the exception tables + 3 `compiler-dtor` for the module's own
  structs), rstl / inline template code (30 `template`, 6 `inline-hdr`), DOL class members copied
  inline (19 `inline-dol`).
- **T - data / vtable thunk** (20 functions, 0x110 bytes): the body only returns a constant, a
  member address or a flag. The port writes it as one line in the vtable.
- **B - module behaviour** (262 functions, 0x12DA0 bytes): real behaviour - these are the port
  bodies. The last column says whether the body carries a `// @ fn_56_` marker.

| class | functions | .text bytes |
|---|---|---|
| H host copy | 101 | 0x2BF8 (11,256) |
| T data / vtable thunk | 20 | 0x110 (272) |
| B module behaviour | 262 | 0x12DA0 (77,216) |
| **total** | **383** | **0x15AA8 (88,744)** |

Breakdown by the note's own `kind` column: 156 `method`, 37 `fsm-trigger`, 36 `weak-dtor`, 34 `virtual`, 30 `template`, 24 `fsm-state`, 19 `inline-dol`, 12 `member-ctor`, 7 `runtime`, 6 `inline-hdr`, 5 `unreferenced`, 3 `compiler-dtor`, 3 `ctor`, 3 `rel-entry`, 2 `dtor`, 2 `member-dtor`, 2 `radar`, 1 `loader`, 1 `static-init` (= 383).

### g.1 H - host copy (101)

| id | size | what it is | callers | DOL / upstream counterpart |
|---|---|---|---|---|
| `fn_56_0` | 0x2C | GetHealthInfo() const -> HealthInfo() [slot 0x3C, shared by both vtables] | data_734 data_8C0 | CActor::GetHealthInfo (header inline) |
| `fn_56_1048` | 0x40 | [?] CModelData move/copy chain used by the loader and x177C | DC | [?] CModelData/TToken copy (header inline) |
| `fn_56_1088` | 0x20 | [?] CModelData move/copy chain used by the loader and x177C | 1048 12B94 | [?] CModelData/TToken copy (header inline) |
| `fn_56_10A8` | 0x28 | [?] CModelData move/copy chain used by the loader and x177C | 1088 | [?] CModelData/TToken copy (header inline) |
| `fn_56_10D0` | 0x13C | [?] CModelData move/copy chain used by the loader and x177C | 10A8 | [?] CModelData/TToken copy (header inline) |
| `fn_56_120C` | 0x6C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_1278` | 0x54 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_12CC` | 0x64 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_1330` | 0x60 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_1390` | 0xE8 | SLdrSandworm::~SLdrSandworm | - | SLdrSandworm.hpp |
| `fn_56_1478` | 0x28C | SLdrSandworm::SLdrSandworm | DC | SLdrSandworm.hpp |
| `fn_56_1704` | 0x190 | LoadTypedefSandwormStruct | DC | SLdrSandworm.hpp |
| `fn_56_1894` | 0x3C | SLdrSandwormStruct::~SLdrSandwormStruct | 1223C 1390 13F74 DC | SLdrSandworm.hpp |
| `fn_56_18D0` | 0x4C | SLdrSandwormStruct::SLdrSandwormStruct | 1478 | SLdrSandworm.hpp |
| `fn_56_1DD0` | 0x78 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_1E48` | 0x54 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_1EDC` | 0x90 | [?] CDamageInfo default ctor (inline) | 1E9C | [?] CDamageInfo::CDamageInfo() |
| `fn_56_1F6C` | 0x6C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_2198` | 0x58 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_2268` | 0x38 | [?] material test (x68 & 0x200) inline HasMaterial | 321C | CMaterialList::HasMaterial (inline) |
| `fn_56_263C` | 0x68 | [?] SMoverData ctor (inline) | 2434 | [?] SMoverData::SMoverData |
| `fn_56_26A4` | 0x20 | CModelData::CModelDataNull (inline) | 2434 | CModelData::CModelDataNull |
| `fn_56_285C` | 0x94 | SAttackHistory::~SAttackHistory (x1490) | 1223C | - |
| `fn_56_2908` | 0x8C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_343C` | 0x48 | ~CBodyStateCmd (data_878, REL copy) | data_880 | CBodyStateCmd::~CBodyStateCmd |
| `fn_56_3590` | 0x5C | ~CBCTauntCmd (data_86C, REL copy) | data_874 | CBCTauntCmd::~CBCTauntCmd |
| `fn_56_4AD0` | 0x8 | [?] inline getter (u8 at +0x80) | - | [?] |
| `fn_56_5250` | 0x50 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_52A0` | 0x58 | ~CEntityInfo (inline) | 5060 | CEntityInfo::~CEntityInfo |
| `fn_56_52F8` | 0x54 | ~vector<SConnection> (inline) | 52A0 | rstl::vector dtor |
| `fn_56_70FC` | 0x3C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_7138` | 0x184 | CPASAnimParmData::CPASAnimParmData | 11E54 6FA0 | CPASAnimParmData.hpp |
| `fn_56_72BC` | 0x3C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_85B4` | 0x54 | [?] CImpactVisorEffect / token helper chain (template) | 84B8 | - |
| `fn_56_8608` | 0x5C | [?] CImpactVisorEffect / token helper chain (template) | 85B4 86CC | - |
| `fn_56_8664` | 0x20 | [?] CImpactVisorEffect / token helper chain (template) | 8608 | - |
| `fn_56_8684` | 0x48 | [?] CImpactVisorEffect / token helper chain (template) | 8664 | - |
| `fn_56_86CC` | 0x8C | [?] CImpactVisorEffect / token helper chain (template) | 84B8 | - |
| `fn_56_8758` | 0xB4 | [?] CImpactVisorEffect / token helper chain (template) | 86CC | - |
| `fn_56_880C` | 0x40 | [?] CImpactVisorEffect / token helper chain (template) | 8758 | - |
| `fn_56_884C` | 0x20 | [?] CImpactVisorEffect / token helper chain (template) | 880C 898C | - |
| `fn_56_886C` | 0x28 | [?] CImpactVisorEffect / token helper chain (template) | 884C | - |
| `fn_56_8894` | 0x80 | [?] CImpactVisorEffect / token helper chain (template) | 886C | - |
| `fn_56_8914` | 0x3C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_8950` | 0x3C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_898C` | 0x38 | [?] CImpactVisorEffect / token helper chain (template) | 86CC | - |
| `fn_56_89C4` | 0x70 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_8A34` | 0x78 | [?] CImpactVisorEffect / token helper chain (template) | 86CC | - |
| `fn_56_9544` | 0x4C | [?] const GetObjectById cast (weak) | - | TCastToPtr<CBouncingBomb> |
| `fn_56_B4B0` | 0x5C | ~CBCGenerateCmd (data_860, REL copy) | data_868 | CBCGenerateCmd::~CBCGenerateCmd |
| `fn_56_B800` | 0x5C | ~CBCMeleeAttackCmd (data_854, REL copy) | data_85C | CBCMeleeAttackCmd::~CBCMeleeAttackCmd |
| `fn_56_CF80` | 0x84 | [?] vector dtor/push_back/reserve (template) | CC8C | rstl::vector<CJointCollisionDescription> |
| `fn_56_D004` | 0x38 | [?] vector dtor/push_back/reserve (template) | CF80 | rstl::vector<CJointCollisionDescription> |
| `fn_56_D03C` | 0x60 | [?] vector dtor/push_back/reserve (template) | D004 | rstl::vector<CJointCollisionDescription> |
| `fn_56_D1B4` | 0x38 | [?] vector dtor/push_back/reserve (template) | D09C | rstl::vector<CJointCollisionDescription> |
| `fn_56_D1EC` | 0x20 | [?] vector dtor/push_back/reserve (template) | 14F40 D1B4 | rstl::vector<CJointCollisionDescription> |
| `fn_56_D20C` | 0x28 | [?] vector dtor/push_back/reserve (template) | D1EC | rstl::vector<CJointCollisionDescription> |
| `fn_56_D234` | 0xC0 | [?] vector dtor/push_back/reserve (template) | D09C D20C | rstl::vector<CJointCollisionDescription> |
| `fn_56_D2F4` | 0x5C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_D5D4` | 0x68 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_D63C` | 0x54 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_EF50` | 0x3C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_F328` | 0x40 | [?] EF8C helper | EF8C | - |
| `fn_56_10F54` | 0x3C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_12474` | 0x1E4 | ~CPatterned body (REL copy) | 1223C | CPatterned::~CPatterned |
| `fn_56_12658` | 0x94 | [?] dtor of x620 member (vtable data_844) | 12474 | [?] CAiKnockBack-type member dtor |
| `fn_56_126EC` | 0x94 | ~x1528 (CProjectileInfo+extras) | 1223C | - |
| `fn_56_12780` | 0x8C | ~SPincers (x1770) | 1223C | - |
| `fn_56_12B64` | 0x30 | [?] model holder assign chain (template) | 128B0 | [?] |
| `fn_56_12B94` | 0x48 | [?] model holder assign chain (template) | 12B64 | [?] |
| `fn_56_12BDC` | 0xC4 | [?] model holder assign chain (template) | 12B94 | [?] |
| `fn_56_12CA0` | 0xC8 | [?] model holder assign chain (template) | 12BDC | [?] |
| `fn_56_1362C` | 0x3C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_136A4` | 0x3C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_13754` | 0x3C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_13B24` | 0x5C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_13BF8` | 0x64 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_13C5C` | 0x60 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_13CBC` | 0x3C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_13CF8` | 0x58 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_13D50` | 0x3C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_13D8C` | 0x8C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_13E18` | 0x8C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_13EA4` | 0x58 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_13EFC` | 0x3C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_13F38` | 0x3C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_13F74` | 0x60 | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_13FD4` | 0x3C | [?] weak/EH-only destructor (no REL caller; Free/~CToken body) | - | [?] compiler-emitted dtor |
| `fn_56_14B58` | 0xCC | [?] IssueTauntCmd (CAnimationState-style, kBSC_Taunt) | 3484 99A0 | - |
| `fn_56_14C24` | 0x11C | [?] IssueGenerateCmd (kBSC_Generate) | B320 B50C | - |
| `fn_56_14D40` | 0x10C | [?] IssueMeleeAttackCmd (kBSC_MeleeAttack) | B6DC B8B8 BAB8 BC8C BF44 | - |
| `fn_56_14E4C` | 0x34 | CTransform4f copy (inline) | 12D68 14010 | CTransform4f::CTransform4f(const&) |
| `fn_56_14E80` | 0xC0 | [?] vector dtor/push_back/reserve (template) | CC8C | rstl::vector<CJointCollisionDescription> |
| `fn_56_14F40` | 0x68 | [?] vector dtor/push_back/reserve (template) | 14E80 | rstl::vector<CJointCollisionDescription> |
| `__destroy_global_chain` | 0x58 | __destroy_global_chain: walks __global_destructor_chain, calling each registered dtor | .dtors$10 (REL loader) | crtexe global destructor chain |
| `__register_global_object` | 0x1C | __register_global_object: pushes (object, dtor, cookie) on __global_destructor_chain | 14FA8 | crtexe global object registry |
| `_unresolved` | 0xC4 | _unresolved: unresolved-symbol trap; OSReport + stack walk, never returns | REL loader (PLC) | crtexe unresolved-symbol handler |
| `_epilog` | 0x24 | _epilog: module exit; ModuleDestructors then RELExit | REL loader | crtexe module epilog |
| `_prolog` | 0x24 | _prolog: module entry; ModuleConstructors then RELMain | REL loader | crtexe module prolog |
| `ModuleDestructors` | 0x4C | ModuleDestructors: runs the module's static destructor list | _epilog | crtexe module destructor table |
| `ModuleConstructors` | 0x4C | ModuleConstructors: runs the module's static constructor list | _prolog | crtexe module constructor table |

### g.2 T - data / vtable thunks (20)

| id | size | what it is | callers | port |
|---|---|---|---|---|
| `fn_56_238C` | 0x1C | GetAimPosition [slot 0x54] (x2E8) | data_74C | marked |
| `fn_56_23A8` | 0x1C | GetOrbitPosition [slot 0x50] (x2E8) | data_748 | marked |
| `fn_56_30F0` | 0x8 | CanBeUnPossessed [slot 0xFC] -> false | data_980 | marked |
| `fn_56_6790` | 0x8 | ProjectileInfo [slot 0xDC] -> &x1528 | data_960 | marked |
| `fn_56_9860` | 0xC | GetIngSnatchingModelOverlapSize [slot 0x140] -> 6.0 | data_9C4 | marked |
| `fn_56_A4F0` | 0x20 | PreRender [slot 0x24] -> CPatterned::PreRender | data_8A8 | marked |
| `fn_56_11CE8` | 0x10 | TakeDamage [slot 0xA4] (x448 = skDamageHitTime) | data_928 | marked |
| `fn_56_14A64` | 0x8 | GetSearchPath [slot 0xE0] -> &x7C4 | data_964 | marked |
| `fn_56_14A6C` | 0x8 | [?] slot 0x23C -> 0 | data_AC0 | no marker |
| `fn_56_14ADC` | 0x8 | GetModelAlphau8 [slot 0xC8] (x44F) | data_94C | marked |
| `fn_56_14AE4` | 0x8 | GetPointSearchPath [slot 0xE4] -> null | data_968 | no marker |
| `fn_56_14AEC` | 0x8 | GetRagDoll [slot 0xF4] -> null | data_978 | no marker |
| `fn_56_14AF4` | 0x10 | GetAttackTarget [slot 0x10C] -> invalid | data_990 | marked |
| `fn_56_14B04` | 0xC | IsOnGround [slot 0x110] (x34C) | data_994 | marked |
| `fn_56_14B10` | 0xC | GetGravityConstant [slot 0x114] -> kDefaultGravityAccel | data_998 | no marker |
| `fn_56_14B1C` | 0x8 | GetDeathExplosionParticle [slot 0x124] -> &x754 | data_9A8 | no marker |
| `fn_56_14B24` | 0x8 | CanBeShot [slot 0xA8] -> true | data_92C | no marker |
| `fn_56_14B2C` | 0x8 | IsListening [slot 0xAC] -> false | data_930 | marked |
| `fn_56_14B34` | 0x8 | Listen [slot 0xB0] -> false | data_934 | no marker |
| `fn_56_14B3C` | 0x1C | GetOrigin [slot 0xB4] | data_938 | no marker |

### g.3 B - module behaviour (262)

`port` column: **marked** = the body carries a `// @ fn_56_` marker; **no marker** = the body is
in the tree but carries none (a data-gated body, a dead slot, or a body the port folded into a
neighbour); **no body** = the port has none (the two unreferenced math helpers the port skipped and
the REL's static init, which the host does not need).

The 17 **no marker** rows are the 9 B functions section j lists as not
exercised (three of them without a body at all) and the 8 dead virtual slots of T. Section l
has the per-arm table for every one of them.

| id | size | what it is | callers | port |
|---|---|---|---|---|
| `RELExit` | 0x24 | RELExit | - | marked |
| `RELMain` | 0x20 | RELMain | - | marked |
| `fn_56_70` | 0x6C | RegisterFuncPtrs (fills bss_38, SetSSandworm_FuncPtrs) | RELMain | marked |
| `fn_56_DC` | 0xF6C | LoadSandworm(CStateManager&,CInputStream&,const CEntityInfo&) | - | marked |
| `fn_56_191C` | 0x11C | SGlowParticles::~SGlowParticles (x19D0, 4 owned CElementGen) | 1223C | marked |
| `fn_56_1A38` | 0x398 | SGlowParticles::SGlowParticles(id0,id1,id2) -> 4x new CElementGen(0x338) | 12D68 14010 | marked |
| `fn_56_1E9C` | 0x40 | SBombParams::SBombParams() (x1944, Sandigger: empty) | 12D68 | marked |
| `fn_56_1FD8` | 0x1C0 | SBombParams::SBombParams(bombFX,explosionFX,damage) (x1944, Bomb Guardian) | 14010 | marked |
| `fn_56_21F0` | 0x14 | [?] SetDead (x2F4 bit6) | 11374 5D80 | marked |
| `fn_56_2204` | 0xC | [?] IsDead (x2F4 bit6) | 321C 35F8 36A0 441C E204 E2C0 | marked |
| `fn_56_2210` | 0x1C | [?] GetX2E8 (CVector3f) | - | no marker |
| `fn_56_222C` | 0x1C | [?] SetX2E8 (CVector3f) | 441C | marked |
| `fn_56_2248` | 0x14 | [?] SetX2F4bit7 | 441C | marked |
| `fn_56_225C` | 0xC | [?] GetX2F4bit7 | 441C | marked |
| `fn_56_22A0` | 0x7C | [?] SetTargetable (Add/RemoveMaterial) | 441C | marked |
| `fn_56_231C` | 0x34 | SetTouchBounds (x2D0 CAABox) | 441C | marked |
| `fn_56_2350` | 0x3C | GetTouchBounds [slot 0x48] | data_740 | marked |
| `fn_56_23C4` | 0x70 | ~CSandwormEye [slot 0x08] | data_700 | marked |
| `fn_56_2434` | 0x208 | CSandwormEye::CSandwormEye(uid,name,info,xf) | 5060 | marked |
| `fn_56_26C4` | 0x54 | SAttackHistory::EndsWith(type,count) (x1490) | 11374 3844 3DE4 573C 74B8 C168 | marked |
| `fn_56_2718` | 0x58 | [?] Descend helper | B320 | marked |
| `fn_56_2770` | 0x78 | [?] BeginAttack helper | B85C | marked |
| `fn_56_27E8` | 0x74 | [?] BeginAttack helper | 2770 | marked |
| `fn_56_28F0` | 0x18 | SAttackHistory::SAttackHistory (x1490) | 12D68 14010 | marked |
| `fn_56_2994` | 0x29C | RenderIngSnatchingTransition [slot 0x144] | data_9C8 | marked |
| `fn_56_2C30` | 0xBC | GetOrbitPosition [slot 0x50] (eye-based) | data_8D4 | marked |
| `fn_56_2CEC` | 0x35C | [?] FindGrabAIHint (TCastToPtr<CScriptAIHint>) | 59A8 C168 | marked |
| `fn_56_3048` | 0xA8 | [?] AIHint helper | C168 CBB4 | marked |
| `fn_56_30F8` | 0x34 | CanBeIngPossessed [slot 0xF8] (Bomb Guardian only) | data_97C | marked |
| `fn_56_312C` | 0x54 | GetIngSnatchingPoint [slot 0x13C] (cubic of x1388..x13A8) | data_9C0 | marked |
| `fn_56_3180` | 0x9C | GetIngSnatchingNormal [slot 0x138] | data_9BC | marked |
| `fn_56_321C` | 0x188 | GetAimPosition [slot 0x54] (eye-based) | data_8D8 | marked |
| `fn_56_33A4` | 0x98 | Interrupt [slot 0x238, FSM extra state] | data_ABC | marked |
| `fn_56_3484` | 0x10C | EyeKilledReaction [state slot 0x1fc] | data_A80 | marked |
| `fn_56_35EC` | 0xC | HitDuringWindUp [trigger slot 0x188] | data_A0C | marked |
| `fn_56_35F8` | 0xA8 | OneEyeKilled [trigger slot 0x19c] | data_A20 | marked |
| `fn_56_36A0` | 0xB0 | [?] AnyEyeDead / eye-state query | 11374 3484 3B24 441C 4AD8 74B8 AC80 E340 | marked |
| `fn_56_3750` | 0x14 | PickedBombFountain [trigger slot 0x1a0] | data_A24 | marked |
| `fn_56_3764` | 0x14 | PickedBombSpread [trigger slot 0x1a4] | data_A28 | marked |
| `fn_56_3778` | 0x14 | PickedBombToss [trigger slot 0x1a8] | data_A2C | marked |
| `fn_56_378C` | 0x14 | PickedSpitAttack [trigger slot 0x1ac] | data_A30 | marked |
| `fn_56_37A0` | 0xA4 | PickMediumAttack [state slot 0x214] | data_A98 | marked |
| `fn_56_3844` | 0x14C | [?] PickAttackType (random, history) | 37A0 | marked |
| `fn_56_3990` | 0x90 | GetPhaseParams() (first SLdrSandwormStruct whose threshold < health) | 10F90 3844 5CA4 74B8 | marked |
| `fn_56_3A20` | 0x6C | IsPossessedBombGuardian (x8C0 bit4 && IsIngPossessed && !IsBeingSnatched) | 11374 321C 36A0 37A0 3B24 441C 4AD8 5530 59A8 5AE0 5CA4 6C04 74B8 7A2C 97FC 981C E204 E340 | marked |
| `fn_56_3A8C` | 0x98 | [?] UpdateBombDrop(dt) | 74B8 AA74 | marked |
| `fn_56_3B24` | 0x78 | [?] CanDropBomb | 3A8C D898 | marked |
| `fn_56_3B9C` | 0x10 | ResetBombDropTimer (x1984=0) | 74B8 AA74 | marked |
| `fn_56_3BAC` | 0x198 | [?] DropBouncingBomb (new CBouncingBomb 0x218) | 3A8C | marked |
| `fn_56_3D44` | 0x20 | PlayerReachable [trigger slot 0x1bc] | data_A40 | marked |
| `fn_56_3D64` | 0x80 | [?] PathReachable(pos) | 3D44 C27C | marked |
| `fn_56_3DE4` | 0x38 | CanSpitAgain [trigger slot 0x164] | data_9E8 | marked |
| `fn_56_3E1C` | 0x50 | CanDescend [trigger slot 0x158] | data_9DC | marked |
| `fn_56_3E6C` | 0xC | ForceUnderground [trigger slot 0x180] | data_A04 | marked |
| `fn_56_3E78` | 0x34 | PlayerHiding [trigger slot 0x1b0] | data_A34 | marked |
| `fn_56_3EAC` | 0x48 | [?] UpdateLoopedSfx (3 emitters) | 110C0 | marked |
| `fn_56_3EF4` | 0xC0 | [?] UpdateEmitter helper | 3EAC | marked |
| `fn_56_3FB4` | 0x88 | [?] StartMeleeSfx | 6FA0 | marked |
| `fn_56_403C` | 0x4C | [?] StopMoveSfx | 11374 4088 74B8 AA74 | marked |
| `fn_56_4088` | 0x64 | [?] StartMoveSfx | 74B8 AA74 | marked |
| `fn_56_40EC` | 0xD0 | [?] StartVocalSfx (random) | 74B8 AA74 | marked |
| `fn_56_41BC` | 0x44 | [?] StopVocalSfx | 74B8 AA74 | marked |
| `fn_56_4200` | 0x184 | Straighten [state slot 0x230] | data_AB4 | marked |
| `fn_56_4384` | 0x20 | DoneStraightening [trigger slot 0x170] | data_9F4 | marked |
| `fn_56_43A4` | 0x78 | [?] UpdateBounds helper | 441C | marked |
| `fn_56_441C` | 0x5F8 | [?] UpdateEyes(mgr,dt) (eye pose/targetable/damage state) | 110C0 9CD8 AD14 | marked |
| `fn_56_4A14` | 0xBC | AddToRenderer [slot 0x28] (+glow gens 2,3) | data_8AC | marked |
| `fn_56_4AD8` | 0x588 | [?] UpdateEyeVulnerability/additive flinch | 441C | marked |
| `fn_56_5060` | 0x1F0 | CreateEyes (2x new CSandwormEye 0x300: "Sandworm Eye Head"/"Tail") | 11374 | marked |
| `fn_56_534C` | 0xD0 | [?] unreferenced script-actor lookup | - | no marker |
| `fn_56_541C` | 0x54 | DoneSulking [trigger slot 0x174] | data_9F8 | marked |
| `fn_56_5470` | 0xC0 | Sulk [state slot 0x234] | data_AB8 | marked |
| `fn_56_5530` | 0x4C | SequenceAttack [trigger slot 0x1c8] | data_A4C | marked |
| `fn_56_557C` | 0x7C | ShouldSulk [trigger slot 0x1cc] | data_A50 | marked |
| `fn_56_55F8` | 0x74 | [?] sulk helper | 5470 557C | marked |
| `fn_56_566C` | 0x68 | [?] HitIsOnHeadEye(collisionActor) | 11374 | marked |
| `fn_56_56D4` | 0x68 | [?] HitIsOnTailEye(collisionActor) | 11374 | marked |
| `fn_56_573C` | 0x74 | CanMeleeAgain [trigger slot 0x160] | data_9E4 | marked |
| `fn_56_57B0` | 0xD4 | [?] KnockPlayer(impulse,dmg) (ApplyImpulseWR, Rumble) | 5F5C 6E40 7EE4 | marked |
| `fn_56_5884` | 0x3C | [?] segment helper | 10038 D09C | marked |
| `fn_56_58C0` | 0x50 | DoneLurking [trigger slot 0x16c] | data_9F0 | marked |
| `fn_56_5910` | 0x28 | Radar: GetRadarPointCount -> 13 when active && !x420bit6 | data_6F4 | marked |
| `fn_56_5938` | 0x40 | Radar: GetRadarPointPosition(index) | data_6E8 | marked |
| `fn_56_5978` | 0x30 | PlayerIsOnPathMesh [trigger slot 0x1b4] | data_A38 | marked |
| `fn_56_59A8` | 0x138 | CanAttack [trigger slot 0x150] | data_9D4 | marked |
| `fn_56_5AE0` | 0x19C | CanCharge [trigger slot 0x154] | data_9D8 | marked |
| `fn_56_5C7C` | 0x28 | [?] clamp x1350 | 110C0 | marked |
| `fn_56_5CA4` | 0xDC | [?] ResetAttackTimer (Range, GetAverageAttackTime) | 6FA0 8190 B6DC B8B8 BAB8 BC8C BEA4 | marked |
| `fn_56_5D80` | 0xDC | Dead [state slot 0x1f4] | data_A78 | marked |
| `fn_56_5E5C` | 0x38 | Deactivate [state slot 0x1f0] | data_A74 | marked |
| `fn_56_5E94` | 0xC8 | [?] TeleportIfRequested (Think, x1604) | 110C0 9CD8 | marked |
| `fn_56_5F5C` | 0x1E8 | [?] ApplyMeleeDamage | 646C | marked |
| `fn_56_6144` | 0xA8 | [?] ReleaseGrabbedPlayer (DetachActorFromPlayer) | 646C 74B8 BF44 | marked |
| `fn_56_61EC` | 0x138 | [?] GrabPlayer (AttachActorToPlayer) | 646C | marked |
| `fn_56_6324` | 0x94 | [?] unreferenced (GetIdForScript) | - | no marker |
| `fn_56_63B8` | 0x58 | [?] SendGrabMsgs | 646C BF44 | marked |
| `fn_56_6410` | 0x5C | [?] SendAttackMsgs | 646C | marked |
| `fn_56_646C` | 0x324 | DoUserAnimEvent [slot 0x70] (jumptable data_790) | data_8F4 | marked |
| `fn_56_6798` | 0x16C | ChargeOver [trigger slot 0x168] | data_9EC | marked |
| `fn_56_6904` | 0xD8 | FacingPlayerForCharge [trigger slot 0x17c] | data_A00 | marked |
| `fn_56_69DC` | 0x74 | SpitAngleOK [trigger slot 0x1d8] | data_A5C | marked |
| `fn_56_6A50` | 0x58 | FacingPlayer [trigger slot 0x178] | data_9FC | marked |
| `fn_56_6AA8` | 0xBC | [?] AngleToTarget | 6904 6A50 6B64 8AAC | marked |
| `fn_56_6B64` | 0xA0 | [?] SpitAimAngle | 69DC 8AAC | marked |
| `fn_56_6C04` | 0xBC | InChargeRange [trigger slot 0x190] | data_A14 | marked |
| `fn_56_6CC0` | 0xB4 | [?] charge helper | 8190 | marked |
| `fn_56_6D74` | 0xCC | [?] pursue helper | 74B8 | marked |
| `fn_56_6E40` | 0x160 | [?] UpdateContactDamage (Think) | 110C0 | marked |
| `fn_56_6FA0` | 0x15C | MeleeAttack [state slot 0x208] | data_A8C | marked |
| `fn_56_72F8` | 0x98 | [?] InMeleeRange helper | 6E40 73BC | marked |
| `fn_56_7390` | 0x2C | PlayerIsMorphball [trigger slot 0x1b8] | data_A3C | marked |
| `fn_56_73BC` | 0x38 | MeleeAttackRange [trigger slot 0x198] | data_A1C | marked |
| `fn_56_73F4` | 0xC4 | [?] pursue path speed helper | 74B8 | marked |
| `fn_56_74B8` | 0x574 | Pursue [state slot 0x21c] | data_AA0 | marked |
| `fn_56_7A2C` | 0x234 | [?] FindCoverPoint | 74B8 | marked |
| `fn_56_7C60` | 0x1A8 | [?] cover-point scan | 7A2C | marked |
| `fn_56_7E08` | 0x58 | [?] cover-point test | 7C60 | marked |
| `fn_56_7E60` | 0x84 | [?] SetPathDestination | 74B8 | marked |
| `fn_56_7EE4` | 0x2AC | [?] ApplyChargeDamage | 11374 8190 | marked |
| `fn_56_8190` | 0x328 | ChargePlayer [state slot 0x1e8] | data_A6C | marked |
| `fn_56_84B8` | 0xFC | [?] LaunchSpitProjectile | 8AAC | marked |
| `fn_56_8AAC` | 0x3E4 | [?] FireSpit (anim event) | 646C | marked |
| `fn_56_8E90` | 0x1A0 | [?] BombToss spawn | 646C | marked |
| `fn_56_9030` | 0x1A4 | [?] SpawnBouncingBomb (new 0x218) | 8E90 91D4 | marked |
| `fn_56_91D4` | 0x194 | [?] BombFountain spawn | 646C | marked |
| `fn_56_9368` | 0xD8 | [?] AttachHeldBomb | 9660 B8B8 BC8C | marked |
| `fn_56_9440` | 0x104 | [?] ReleaseHeldBomb | 646C | marked |
| `fn_56_9590` | 0x50 | [?] GetHeldBomb (TCastToPtr<CBouncingBomb>) | 9368 9440 | marked |
| `fn_56_95E0` | 0x60 | [?] DeleteHeldBomb | 11374 646C B8B8 BC8C | marked |
| `fn_56_9640` | 0x8 | GetBombExplodeSfx (x1832) | 3BAC 9030 9660 | marked |
| `fn_56_9648` | 0x8 | GetBombBounceSfx (x1830) | 3BAC 9030 9660 | marked |
| `fn_56_9650` | 0x10 | GetInvalidSfx | 3BAC 9030 9660 | marked |
| `fn_56_9660` | 0x190 | [?] SpawnHeldBomb (new 0x218) | 646C | marked |
| `fn_56_97F0` | 0xC | ReadyToCharge [trigger slot 0x1c4] | data_A48 | marked |
| `fn_56_97FC` | 0x20 | SnatchEnded [trigger slot 0x1d4] | data_A58 | marked |
| `fn_56_981C` | 0x44 | SnatchStarted [trigger slot 0x1d0] | data_A54 | marked |
| `fn_56_986C` | 0x134 | Snatch [state slot 0x224] | data_AA8 | marked |
| `fn_56_99A0` | 0xB8 | ChargeWindUp [state slot 0x1ec] | data_A70 | marked |
| `fn_56_9A58` | 0x1B8 | [?] UpdateShadow | 9CD8 | marked |
| `fn_56_9C10` | 0x78 | CreateProjectedShadow (x13BC) | 128B0 | marked |
| `fn_56_9C88` | 0x50 | [?] bounds accumulate | A04C | marked |
| `fn_56_9CD8` | 0x374 | PreRenderAllViewports [slot 0x34] | data_8B8 | marked |
| `fn_56_A04C` | 0x2FC | [?] ComputeBounds | 43A4 9CD8 | marked |
| `fn_56_A348` | 0x1A8 | [?] UpdateLocatorTransforms | 11078 9CD8 | marked |
| `fn_56_A510` | 0x1F8 | [?] AddAnimatedScale per segment | E764 | marked |
| `fn_56_A708` | 0x1D8 | [?] FollowPath | 74B8 8190 | marked |
| `fn_56_A8E0` | 0x114 | [?] NearlyOnPath test | 5978 6798 A708 | marked |
| `fn_56_A9F4` | 0xC | SetBodyCmdRequest(a,b) (x1518/x151C) | 3484 5D80 986C 99A0 AA00 B6DC B8B8 BAB8 BC8C BF44 | marked |
| `fn_56_AA00` | 0x28 | ClearBodyCmdRequest (-1,0xD) | 74B8 8190 BEA4 | marked |
| `fn_56_AA28` | 0x4C | [?] Think helper | 10AD8 | marked |
| `fn_56_AA74` | 0x1B0 | Patrol [state slot 0x210] | data_A94 | marked |
| `fn_56_AC24` | 0x5C | [?] SetUnderground (collision) | 112E8 AA74 | marked |
| `fn_56_AC80` | 0x94 | [?] path helper | 74B8 A708 | marked |
| `fn_56_AD14` | 0x39C | [?] OrientBodyToPath (LookAt, SetTransform) | 3484 74B8 A708 | marked |
| `fn_56_B0B0` | 0x88 | [?] body helper | AD14 FC94 | marked |
| `fn_56_B138` | 0x1E8 | [?] spine update helper | F5B0 | marked |
| `fn_56_B320` | 0x190 | Descend [state slot 0x1f8] | data_A7C | marked |
| `fn_56_B50C` | 0x1D0 | Rise [state slot 0x220] | data_AA4 | marked |
| `fn_56_B6DC` | 0x124 | SpitAttack [state slot 0x228] | data_AAC | marked |
| `fn_56_B85C` | 0x5C | [?] BeginAttack (common) | 6FA0 74B8 8190 B6DC B8B8 BAB8 BC8C BF44 | marked |
| `fn_56_B8B8` | 0x200 | BombFountainAttack [state slot 0x1dc] | data_A60 | marked |
| `fn_56_BAB8` | 0x1D4 | BombSpreadAttack [state slot 0x1e0] | data_A64 | marked |
| `fn_56_BC8C` | 0x218 | BombTossAttack [state slot 0x1e4] | data_A68 | marked |
| `fn_56_BEA4` | 0x94 | StopGrabAttack [state slot 0x22c] | data_AB0 | marked |
| `fn_56_BF38` | 0xC | GrabAttackFinished [trigger slot 0x184] | data_A08 | marked |
| `fn_56_BF44` | 0x160 | GrabAttack [state slot 0x200] | data_A84 | marked |
| `fn_56_C0A4` | 0xC4 | [?] grab helper | C168 | marked |
| `fn_56_C168` | 0x114 | CanGrabMorphball [trigger slot 0x15c] | data_9E0 | marked |
| `fn_56_C27C` | 0xD4 | InSpitRange [trigger slot 0x194] | data_A18 | marked |
| `fn_56_C350` | 0x80 | InAttackRange [trigger slot 0x18c] | data_A10 | no marker |
| `fn_56_C3D0` | 0x18 | Activated [trigger slot 0x148] | data_9CC | no marker |
| `fn_56_C3E8` | 0xC | BeginUnderground [trigger slot 0x14c] | data_9D0 | marked |
| `fn_56_C3F4` | 0x74 | LurkUnderground [state slot 0x204] | data_A88 | marked |
| `fn_56_C468` | 0x198 | [?] MoveToScriptActor (GetIdForScript) | 5470 C3F4 | marked |
| `fn_56_C600` | 0x104 | [?] spine/segment chain helper | F5B0 F77C | marked |
| `fn_56_C704` | 0xF0 | [?] spine/segment chain helper | C600 E764 | marked |
| `fn_56_C7F4` | 0x34 | [?] spine/segment chain helper | C704 | marked |
| `fn_56_C828` | 0x16C | [?] spine/segment chain helper | AC24 | marked |
| `fn_56_C994` | 0x15C | [?] spine/segment chain helper | B138 C828 F77C FC94 | marked |
| `fn_56_CAF0` | 0xC4 | [?] SetEyeCollisionVulnerability | 4AD8 | marked |
| `fn_56_CBB4` | 0xD8 | [?] spine/segment chain helper | D350 | marked |
| `fn_56_CC8C` | 0x2F4 | CreateCollisionActorManager (new 0x18) | 110C0 AC24 | marked |
| `fn_56_D09C` | 0x118 | [?] BuildSphereDescriptions | CC8C | marked |
| `fn_56_D350` | 0x1F8 | [?] UpdateCollisionActors | 110C0 | marked |
| `fn_56_D548` | 0x8C | [?] InitStateMachine (InitializeStateMachine + 3 tables) | 110C0 | marked |
| `fn_56_D690` | 0x1AC | [?] spine/segment chain helper | C994 EF8C | marked |
| `fn_56_D83C` | 0x5C | [?] spine/segment chain helper | D690 E008 | marked |
| `fn_56_D898` | 0x2D4 | [?] spine/segment chain helper | 9CD8 | marked |
| `fn_56_DB6C` | 0x38 | [?] render helper | E3C0 | marked |
| `fn_56_DBA4` | 0x84 | [?] spine/segment chain helper | D898 | marked |
| `fn_56_DC28` | 0x3E0 | [?] spine/segment chain helper | 9CD8 | marked |
| `fn_56_E008` | 0x12C | [?] spine/segment chain helper | DC28 | marked |
| `fn_56_E134` | 0x34 | GetHeadEye(mgr) (TCastToPtr<CSandwormEye>) | 11374 441C 5D80 | marked |
| `fn_56_E168` | 0x34 | GetTailEye(mgr) | 441C 5D80 | marked |
| `fn_56_E19C` | 0x34 | GetTailEye(mgr) const [?] | 321C 35F8 36A0 E204 E2C0 | marked |
| `fn_56_E1D0` | 0x34 | GetHeadEye(mgr) const [?] | 2C30 321C 35F8 36A0 441C E204 E2C0 | marked |
| `fn_56_E204` | 0xBC | [?] ShouldRenderHeadEye | 9A58 E3C0 | marked |
| `fn_56_E2C0` | 0x80 | [?] ShouldRenderTailEye | 9A58 E3C0 | marked |
| `fn_56_E340` | 0x80 | [?] eye-visible predicate | E204 E2C0 | marked |
| `fn_56_E3C0` | 0x238 | Render [slot 0x2C] | data_8B0 | marked |
| `fn_56_E5F8` | 0xC4 | [?] player-direction helper | 61EC 72F8 C0A4 | marked |
| `fn_56_E6BC` | 0x6C | GetSegLocatorId(i) (x974) | 128B0 | marked |
| `fn_56_E728` | 0x10 | GetSegId(i) (x974) | C600 E764 F384 | marked |
| `fn_56_E738` | 0x2C | [?] spine/segment chain helper | E764 | marked |
| `fn_56_E764` | 0x7BC | [?] spine/segment chain helper | F5B0 | marked |
| `fn_56_EF20` | 0x30 | [?] spine/segment chain helper | E764 | marked |
| `fn_56_EF8C` | 0x39C | [?] RotateBodyForBombAttack (GetRotation/SetTransform) | BAB8 BC8C | marked |
| `fn_56_F368` | 0x1C | [?] spine/segment chain helper | F5B0 | marked |
| `fn_56_F384` | 0x22C | [?] spine/segment chain helper | F5B0 | marked |
| `fn_56_F5B0` | 0x160 | [?] spine/segment chain helper | 9CD8 F77C | marked |
| `fn_56_F710` | 0x8 | Primed [trigger slot 0x1c0] -> true | data_A44 | marked |
| `fn_56_F718` | 0x3C | [?] spine/segment chain helper | A510 | marked |
| `fn_56_F754` | 0x28 | Prime [state slot 0x218] (spine prime/IK) | data_A9C | marked |
| `fn_56_F77C` | 0x2B4 | [?] spine/segment chain helper | F754 | marked |
| `fn_56_FA30` | 0xF4 | [?] spine/segment chain helper | F5B0 | marked |
| `fn_56_FB24` | 0x170 | [?] spine/segment chain helper | FC94 | marked |
| `fn_56_FC94` | 0x328 | [?] spine/segment chain helper | F5B0 | marked |
| `fn_56_FFBC` | 0x7C | [?] spine/segment chain helper | E764 | marked |
| `fn_56_10038` | 0x58 | [?] spine/segment chain helper | 10090 4200 B138 F384 F5B0 F77C | marked |
| `fn_56_10090` | 0x80 | [?] GetSegmentPosition(i) | 11CF8 5938 A708 D350 FB24 FFBC | marked |
| `fn_56_10110` | 0x158 | [?] unreferenced math helper | - | no body |
| `fn_56_10268` | 0x2A8 | [?] unreferenced math helper | - | no body |
| `fn_56_10510` | 0x1C4 | [?] spine/segment chain helper | FA30 | marked |
| `fn_56_106D4` | 0x114 | [?] spine/segment chain helper | AD14 FC94 | marked |
| `fn_56_107E8` | 0x24 | [?] spine/segment chain helper | AD14 C600 F5B0 F77C FC94 | marked |
| `fn_56_1080C` | 0xB0 | [?] spine/segment chain helper | 107E8 FA30 | marked |
| `fn_56_108BC` | 0xB0 | [?] spine/segment chain helper | D350 | marked |
| `fn_56_1096C` | 0xB4 | [?] GetLocatorPosition(i) | 5938 A04C A348 B138 D350 F384 F5B0 | marked |
| `fn_56_10A20` | 0x48 | [?] GetLctrTransform wrapper | 108BC 1096C | marked |
| `fn_56_10A68` | 0x70 | [?] unreferenced (SetSteeringSpeedRange) | - | no marker |
| `fn_56_10AD8` | 0x110 | [?] UpdateTimers (Think) | 110C0 | marked |
| `fn_56_10BE8` | 0xBC | [?] UpdateFade (Think) | 110C0 | marked |
| `fn_56_10CA4` | 0x30 | [?] fade helper | 10BE8 | marked |
| `fn_56_10CD4` | 0x58 | [?] Think helper | 110C0 | marked |
| `fn_56_10D2C` | 0xD8 | [?] UpdateColorLerp (Yellow/Black) | 110C0 | marked |
| `fn_56_10E04` | 0x150 | [?] DetectIncomingProjectiles (BuildNearList) | 110C0 | marked |
| `fn_56_10F90` | 0xE8 | UpdateBossParams (SetBossParams, Bomb Guardian) | 110C0 | marked |
| `fn_56_11078` | 0x48 | [?] UpdatePose (BuildPoseIfNecessary) | 110C0 F77C | marked |
| `fn_56_110C0` | 0x228 | Think [slot 0x14] | data_898 | marked |
| `fn_56_112E8` | 0x48 | Null [state slot 0x20c] | data_A90 | marked |
| `fn_56_11330` | 0x44 | [?] AcceptScriptMsg helper | 11374 | marked |
| `fn_56_11374` | 0x974 | AcceptScriptMsg [slot 0x18] | data_89C | marked |
| `fn_56_11CF8` | 0x100 | [?] AngleBetween | 11374 | marked |
| `fn_56_11DF8` | 0x5C | [?] RemoveAdditiveFlinch | 4AD8 | marked |
| `fn_56_11E54` | 0x158 | [?] AddAdditiveFlinch | 4AD8 | marked |
| `fn_56_11FAC` | 0x22C | [?] SpawnScriptedFx/SendScriptMsgs | 10F90 441C 5D80 | marked |
| `fn_56_121D8` | 0x64 | [?] post-ctor helper | 128B0 | marked |
| `fn_56_1223C` | 0x238 | ~CSandworm [slot 0x08] | data_88C | marked |
| `fn_56_1280C` | 0xA4 | ~SBombParams (x1944) | 1223C | marked |
| `fn_56_128B0` | 0x2B4 | [?] FinishConstruction (pincer models, seg ids, clamp impulses) | 12D68 14010 | marked |
| `fn_56_12D68` | 0x834 | CSandworm::CSandworm (Sandigger/Alpha form) | DC | marked |
| `fn_56_1359C` | 0x90 | SGrab x19A8 ctor (Sandigger) | 12D68 | marked |
| `fn_56_13668` | 0x3C | SSfx x1828 ctor | 12D68 14010 | marked |
| `fn_56_136E0` | 0x74 | x1670 ctor | 12D68 14010 | marked |
| `fn_56_13790` | 0x68 | SMelee x1620 ctor | 12D68 14010 | marked |
| `fn_56_137F8` | 0xF8 | SToss x15AC ctor | 12D68 14010 | marked |
| `fn_56_138F0` | 0x234 | x1528 projectile ctor | 12D68 14010 | marked |
| `fn_56_13B80` | 0x78 | SCharge x14D8 ctor | 12D68 14010 | marked |
| `fn_56_14010` | 0x9C4 | CSandworm::CSandworm (Bomb Guardian form) | DC | marked |
| `fn_56_149D4` | 0x90 | SGrab x19A8 ctor (Bomb Guardian) | 14010 | marked |
| `fn_56_14A74` | 0x68 | GetTouchBounds [slot 0x48] (GetBoundingBox) | data_8CC | marked |
| `fn_56_14FA8` | 0x8E8 | Module static init (.ctors): builds the global rstl::string (the REL file name in bss_C), registers fn_56_5250 as its global destructor, copies the static FSM trigger/state tables into .data | .ctors table (REL loader) | no body |

### g.4 Count check

Every function id of the module's symbol list must appear exactly once in the tables above. The
check reads the symbol list of record and the table rows of this section:

```
$ python3 - <<'PY'
import re, pathlib, collections
sym = pathlib.Path('config/G2ME01/rels/Sandworm/symbols.txt').read_text()
ids = re.findall(r'^(\S+) = \.text:0x[0-9A-Fa-f]+; // type:function', sym, re.M)
doc = pathlib.Path('docs/module-analysis/Sandworm.md').read_text()
sec = doc.split('## g. Function classification')[1].split('\n## h.')[0]
pat = r'^\| `(fn_56_[0-9A-F]+|RELMain|RELExit|_[A-Za-z_]+|Module[A-Za-z]+|__[A-Za-z_]+)` \|'
rows = re.findall(pat, sec, re.M)
c = collections.Counter(rows)
print('symbols:', len(ids))
print('classified rows:', len(rows))
print('distinct ids:', len(c))
print('missing:', [i for i in ids if i not in c])
print('duplicated:', [i for i, n in c.items() if n > 1])
print('extra:', [r for r in c if r not in ids])
PY
symbols: 383
classified rows: 383
distinct ids: 383
missing: []
duplicated: []
extra: []
```

The same check on the machine-readable copy (`Sandworm_functions.tsv`, one row per function, the
`class` column naming the class above):

```
$ awk -F'\t' 'NR>1{print $1}' docs/module-analysis/Sandworm_functions.tsv | sort -u | wc -l
383
```

## h. Upstream classes, and what the port still lacks

### h.1 What upstream already has

`grep -rn "Sandworm\|CSandworm\|CWorm" include/ src/` — 16 files, and **the class exists upstream**:
`include/MetroidPrime/Enemies/CSandworm.hpp` is a PrimeDecomp/echoes file. Before the port it was
29 lines and held only

```cpp
class CSandworm : public CPatterned { ~CSandworm() override; CEntity* TypesMatch(int) const override; };
class CSandwormEye : public CActor { ~CSandwormEye() override; CEntity* TypesMatch(int) const override; };
int GetSandwormRadarPointCount(const CSandworm*);
CVector3f GetSandwormRadarPointPosition(const CSandworm*, int);
```

The port added one `#ifdef TARGET_PC` block to that header (the member structs `SSandwormAttackHistory`,
`SSandwormCharge`, `SSandwormSpit`, `SSandwormToss`, `SSandwormMelee`, `SSandworm1670`,
`SSandwormPincers`, `SSandwormSfx`, `SSandwormBombs`, `SSandwormBombPhase`, `SSandwormGlow`, the
`CSandworm`/`CSandwormEye` members and the port-only `TARGET_PC` constructor set). Everything outside
the block is upstream's and stays byte-identical, so an upstream sync stays a plain merge.

Other upstream files the module's behaviour needs (all already in the tree, none of them port code):

| upstream file | what the module uses it for |
|---|---|
| `include/MetroidPrime/ScriptLoader/SLdrSandworm.hpp` | `SLdrSandworm`, `SLdrSandwormStruct`, `LoadTypedefSandwormStruct` — the generated script typedef; header-only |
| `include/MetroidPrime/Weapons/CBouncingBomb.hpp` | the Bomb Guardian's bomb (`fn_56_3BAC`, `9030`, `9660`) |
| `include/MetroidPrime/Weapons/CImpactVisorEffect.hpp` | the spit's visor effect (`fn_56_85B4`…`fn_56_8A34` are the template instantiations of its `ParticleEffect`) |
| `include/MetroidPrime/BodyState/CBodyStateCmdMgr.hpp` | `CBodyStateCmd` and the DOL's `CBCGenerateCmd` / `CBCTauntCmd` / `CBCMeleeAttackCmd` — the REL carries its own copies of exactly these three (`data_854`, `data_860`, `data_86C`, `data_878`) |
| `include/MetroidPrime/Enemies/CPatterned.hpp`, `CAi.hpp` | the base classes |
| `include/MetroidPrime/CPhysicsActor.hpp` | the retail base of `CSandwormEye` |

There is **no** upstream `CWorm*` class: the worm's class is `CSandworm`, the eye's is `CSandwormEye`.

### h.2 The DOL engine classes the behaviour needs, and their state in the port

| DOL class | used for | port state |
|---|---|---|
| `CPatterned` / `CAi` | base, the FSM2 host | ported (`src/MetroidPrime/Enemies/CPatterned.cpp`) |
| `CPhysicsActor` | `CSandwormEye`'s retail base | upstream class; the port derives the eye from it |
| `CGenericFSM2` | the `'FSM2'` asset, states and triggers | ported (`src/MetroidPrime/Enemies/CGenericFSM2.cpp`) |
| `CBodyController`, `CBodyStateCmdMgr` | the body commands (Taunt/Generate/MeleeAttack) | ported |
| `CElementGen` | eye glow, bomb particles | ported (`src/Kyoto/Particles/CElementGen.cpp`) |
| `CBouncingBomb` | Bomb Guardian bombs | upstream decomp + `src/MetroidPrime/Weapons/CBouncingBomb.cpp` |
| `CCollisionActorManager` | the 15 spine spheres | ported |
| `CProjectedShadow` | the worm's shadow | ported |
| `CPathFindSearch` / `CPathFindPointSearch` | navigation | ported |
| `CWaypointNavigation` | the Pursue path | ported |
| `CIngSnatchingSwarm` + the `CAi` possession API | the Bomb Guardian's Ing possession | ported |
| `CScriptTeamAiMgr`, `CTeamAiRole` | `GetOrigin` (slot 0xB4) | ported (`CTeamAiRole` is header-only) |
| `CImpactVisorEffect` | the spit visor effect | **header-only upstream**, no `.cpp`; the port calls the inline `ParticleEffect` |

### h.3 What is still missing

1. **The REL's own runtime.** `fn_56_14FA8` (0x8E8, the `.ctors` entry) builds the module's global
   `rstl::string` (the REL file name in `bss_C`), registers `fn_56_5250` as its global destructor
   and copies the static FSM trigger/state tables into `.data`; the 7 crtexe symbols walk the
   global destructor chain and the module's static ctor/dtor lists. The host needs none of it: the
   proxy table in `src/MetroidPrime/Enemies/SSandworm_FuncPtrs.cpp` replaces `fn_56_70`, and the
   FSM tables are built per actor by `fn_56_D548` (`InitStateMachine`). The one thing that is lost
   is the REL's file-name string, so `CSandwormEye::x2F8_relToken` (`CRELFileToken`) has no host
   equivalent.
2. **`CSandwormEye`'s base.** Retail derives the eye from `CPhysicsActor`; the DOL's TypesMatch
   chain registers it under `CActor` (`src/MetroidPrime/TypesMatch.cpp:373`,
   `TYPES_MATCH_IMPL(CSandwormEye, CActor, kET_SandwormEye)`). The port derives the eye from
   `CPhysicsActor` and leaves the DOL registration alone, so a DOL-side cast that expects `CActor`
   for the eye is the one mismatch left.
3. **The REL-local body-command copies.** The REL's `CBCTauntCmd` is 0xC bytes and its second word
   is lost on the host (the DOL's `CBCTauntCmd` does not carry it). The port uses the DOL's classes
   instead of porting the copies, so the taunt's second parameter is not reproduced.
4. **The Bomb Guardian's own bodies.** Four bodies the prefab's own attack pick never reaches (its
   `SLdrSandwormStruct` kinds never pick attack 7): `fn_56_B8B8` (BombFountainAttack),
   `fn_56_8E90` (SpawnBombToss), `fn_56_35EC` (HitDuringWindUp), `fn_56_95E0` (DeleteHeldBomb's
   picked-7 branch). **Corrected 2026-10-21:** they are not unreachable - the prefab's AFSM has the
   BombFountainAttack state, and forcing it with the console runs all four (section l).
5. **Thirteen dead slots** and `fn_56_C828` are unreachable for the same reason as in section j
   (prefab data or no caller in the tree); they are ported and annotated, not missing.


## i. Port order

The order to take: **the ctor + vtable + FSM skeleton first, then the behaviour in Think's call
order.**

- **P0** is the skeleton: the loader (`fn_56_DC`) and the two actor constructors
  (`fn_56_12D68`, `fn_56_14010`) write the whole member layout, so nothing else can be written
  before them; the destructors, the eye's constructor, the three REL entry points and the module's
  static init come with them.
- **P1** adds the vtable and the FSM skeleton: `fn_56_D548` (`InitStateMachine`) registers the
  triggers and the states, so no state body can be written before it, and `fn_56_110C0` (`Think`)
  is the order the rest follows.
- **P2** onwards follow **Think's call order**: the eyes, the collision actors, the per-frame
  helpers, then the states in the order the FSM reaches them (Pursue/Patrol → the attacks → the
  bombs → the lifecycle states → the snatching path).
- The **20 thunks** (section g.2) need no slice: they are the vtable skeleton and are written with
  the vtable in P0/P1.
- The **97 host copies** (section g.1) need no body at all.
- The **266 module-behaviour** functions are the 27 slices below, ten per slice (the last has
  seven). The `port file` column names where each slice ended up in the tree.

| slice | functions | port file | what it is |
|---|---|---|---|
| P0 | `fn_56_DC`, `fn_56_12D68`, `fn_56_14010`, `fn_56_1223C`, `fn_56_2434`, `fn_56_23C4`, `fn_56_2C`, `fn_56_50`, `fn_56_70`, `fn_56_14FA8` | PW ×7, PWEye ×2, no body ×1 | skeleton — the loader, both actor ctors, both dtors, the eye ctor/dtor, the three REL entry points, the module static init |
| P1 | `fn_56_D548`, `fn_56_110C0`, `fn_56_128B0`, `fn_56_121D8`, `fn_56_10AD8`, `fn_56_10BE8`, `fn_56_10CA4`, `fn_56_10CD4`, `fn_56_10D2C`, `fn_56_5C7C` | PW ×10 | the FSM skeleton (`InitStateMachine`), `Think` and the per-frame timers (pose, fade, colour lerp, timers) |
| P2 | `fn_56_6E40`, `fn_56_10E04`, `fn_56_3EAC`, `fn_56_10F90`, `fn_56_5E94`, `fn_56_3990`, `fn_56_3A20`, `fn_56_A9F4`, `fn_56_AA00`, `fn_56_AA28` | PW ×8, PWAttacks ×1, PWStates ×1 | the Think tail: contact damage, projectile detection, looped sounds, boss params, the grabbed player, the phase params, the body-command request |
| P3 | `fn_56_441C`, `fn_56_4AD8`, `fn_56_5060`, `fn_56_36A0`, `fn_56_566C`, `fn_56_56D4`, `fn_56_CAF0`, `fn_56_E134`, `fn_56_E168`, `fn_56_E19C` | PWEye ×10 | the eyes: update, vulnerability, create, the eye predicates, the eye lookup |
| P4 | `fn_56_E1D0`, `fn_56_E204`, `fn_56_E2C0`, `fn_56_E340`, `fn_56_11330`, `fn_56_11CF8`, `fn_56_11DF8`, `fn_56_11E54`, `fn_56_11FAC`, `fn_56_CC8C` | PWEye ×8, PW ×1, PWSpine ×1 | the eye lookup (const), the eye render and visibility tests, the message helper, the flinch, the PAS anim, the residues, then the collision-actor manager |
| P5 | `fn_56_D09C`, `fn_56_D350`, `fn_56_AC24`, `fn_56_AD14`, `fn_56_E6BC`, `fn_56_E728`, `fn_56_E738`, `fn_56_E764`, `fn_56_EF20`, `fn_56_EF8C` | PWSpine ×9, PWBombs ×1 | the spine: collision actors, the underground flag, the body orientation, the segment ids |
| P6 | `fn_56_F384`, `fn_56_F5B0`, `fn_56_F718`, `fn_56_F77C`, `fn_56_FA30`, `fn_56_FB24`, `fn_56_FC94`, `fn_56_FFBC`, `fn_56_10038`, `fn_56_10090` | PWSpine ×10 | the spine: the segment state and the IK helpers |
| P7 | `fn_56_10510`, `fn_56_106D4`, `fn_56_107E8`, `fn_56_1080C`, `fn_56_108BC`, `fn_56_1096C`, `fn_56_C600`, `fn_56_C704`, `fn_56_C7F4`, `fn_56_C828` | PWSpine ×9, no body ×1 | the spine: the locators, the segment transforms and the segment math |
| P8 | `fn_56_C994`, `fn_56_CBB4`, `fn_56_A510`, `fn_56_B0B0`, `fn_56_B138`, `fn_56_F368`, `fn_56_5884`, `fn_56_AC80`, `fn_56_A8E0`, `fn_56_74B8` | PWSpine ×7, PWStates ×3 | the spine render helpers and the Pursue state |
| P9 | `fn_56_AA74`, `fn_56_3D64`, `fn_56_3E78`, `fn_56_3D44`, `fn_56_5978`, `fn_56_59A8`, `fn_56_5AE0`, `fn_56_6A50`, `fn_56_6904`, `fn_56_6D74` | PWStates ×7, PWAttacks ×3 | the Patrol state and the pursuit triggers |
| P10 | `fn_56_73F4`, `fn_56_7A2C`, `fn_56_7C60`, `fn_56_7E08`, `fn_56_7E60`, `fn_56_A708`, `fn_56_4088`, `fn_56_40EC`, `fn_56_403C`, `fn_56_37A0` | PWStates ×9, PWAttacks ×1 | the path and cover helpers, the move and vocal sounds, then the attack pick |
| P11 | `fn_56_3844`, `fn_56_2770`, `fn_56_27E8`, `fn_56_5CA4`, `fn_56_3FB4`, `fn_56_6FA0`, `fn_56_B85C`, `fn_56_573C`, `fn_56_3DE4`, `fn_56_B6DC` | PWAttacks ×10 | the attack-history helpers, the melee state, the attack timer and the spit state |
| P12 | `fn_56_8AAC`, `fn_56_84B8`, `fn_56_69DC`, `fn_56_6B64`, `fn_56_C27C`, `fn_56_73BC`, `fn_56_72F8`, `fn_56_5F5C`, `fn_56_57B0`, `fn_56_8190` | PWAttacks ×10 | the spit projectile, the ranges, the melee damage, then the charge state |
| P13 | `fn_56_99A0`, `fn_56_6CC0`, `fn_56_6798`, `fn_56_6C04`, `fn_56_97F0`, `fn_56_7EE4`, `fn_56_BF44`, `fn_56_C0A4`, `fn_56_61EC`, `fn_56_6144` | PWAttacks ×10 | the charge wind-up, the charge triggers and the grab state |
| P14 | `fn_56_63B8`, `fn_56_6410`, `fn_56_C168`, `fn_56_BEA4`, `fn_56_BF38`, `fn_56_2CEC`, `fn_56_3048`, `fn_56_6AA8`, `fn_56_7390`, `fn_56_3A8C` | PWAttacks ×9, PWBombs ×1 | the grab helpers, the stop-grab state and the morphball trigger |
| P15 | `fn_56_3B24`, `fn_56_3B9C`, `fn_56_3BAC`, `fn_56_8E90`, `fn_56_9030`, `fn_56_91D4`, `fn_56_9368`, `fn_56_9440`, `fn_56_9590`, `fn_56_95E0` | PWBombs ×8, no body ×2 | the bomb drop, the bomb spawn and the hold/release helpers |
| P16 | `fn_56_9640`, `fn_56_9648`, `fn_56_9650`, `fn_56_9660`, `fn_56_B8B8`, `fn_56_BAB8`, `fn_56_BC8C`, `fn_56_3750`, `fn_56_3764`, `fn_56_3778` | PWBombs ×9, no body ×1 | the bomb sfx accessors, the three bomb states and the bomb-attack picks |
| P17 | `fn_56_378C`, `fn_56_35EC`, `fn_56_35F8`, `fn_56_5530`, `fn_56_557C`, `fn_56_33A4`, `fn_56_3484`, `fn_56_5D80`, `fn_56_5E5C`, `fn_56_B320` | PWStates ×7, PWAttacks ×2, no body ×1 | the spit pick, the wind-up hit trigger, the eye-killed trigger, the interrupt code, the dead/deactivate states, the descend state |
| P18 | `fn_56_B50C`, `fn_56_C3F4`, `fn_56_5470`, `fn_56_55F8`, `fn_56_541C`, `fn_56_4200`, `fn_56_4384`, `fn_56_58C0`, `fn_56_112E8`, `fn_56_2718` | PWStates ×10 | the rise, lurk, sulk, straighten and null states with their timers |
| P19 | `fn_56_C468`, `fn_56_986C`, `fn_56_981C`, `fn_56_97FC`, `fn_56_F710`, `fn_56_F754`, `fn_56_2994`, `fn_56_312C`, `fn_56_3180` | PWStates ×4, PWSpine ×2, PW ×2, PWRender ×1 | the Ing-snatching path (the Bomb Guardian) |
| P20 | `fn_56_191C`, `fn_56_1A38`, `fn_56_1E9C`, `fn_56_1FD8`, `fn_56_21F0`, `fn_56_2204`, `fn_56_2210`, `fn_56_222C`, `fn_56_2248`, `fn_56_225C` | PWEye ×5, PW ×2, PWBombs ×2, no body ×1 | the glow and bomb member ctors/dtors and the eye flag helpers |
| P21 | `fn_56_22A0`, `fn_56_231C`, `fn_56_2350`, `fn_56_26C4`, `fn_56_285C`, `fn_56_28F0`, `fn_56_2C30`, `fn_56_30F8`, `fn_56_321C`, `fn_56_3E1C` | PWEye ×5, PW ×3, no body ×1, PWStates ×1 | the eye accessors, the attack history, the orbit/aim positions and the possession check |
| P22 | `fn_56_3E6C`, `fn_56_3EF4`, `fn_56_41BC`, `fn_56_43A4`, `fn_56_4A14`, `fn_56_534C`, `fn_56_5910`, `fn_56_5938`, `fn_56_6324`, `fn_56_646C` | PWStates ×3, PWRender ×2, no body ×2, PW ×2, PWAttacks ×1 | the movement triggers, the renderer hooks, the radar callbacks and the anim-event handler |
| P23 | `fn_56_9A58`, `fn_56_9C10`, `fn_56_9C88`, `fn_56_9CD8`, `fn_56_A04C`, `fn_56_A348`, `fn_56_C350`, `fn_56_C3D0`, `fn_56_C3E8`, `fn_56_D690` | PWRender ×7, no body ×2, PWStates ×1 | the shadow, the pre-render pass, the attack-range trigger and the underground triggers |
| P24 | `fn_56_D83C`, `fn_56_D898`, `fn_56_DB6C`, `fn_56_DBA4`, `fn_56_DC28`, `fn_56_E008`, `fn_56_E3C0`, `fn_56_E5F8`, `fn_56_F328`, `fn_56_10110` | PWRender ×7, no body ×2, PWAttacks ×1 | the render helpers, the bounds and the spine raycasts |
| P25 | `fn_56_10268`, `fn_56_10A20`, `fn_56_10A68`, `fn_56_11078`, `fn_56_11374`, `fn_56_126EC`, `fn_56_12780`, `fn_56_1280C`, `fn_56_1359C`, `fn_56_13668` | no body ×5, PW ×2, PWRender ×1, PWEye ×1, PWBombs ×1 | the script-message handler, the pose update and the remaining member destructors |
| P26 | `fn_56_136E0`, `fn_56_13790`, `fn_56_137F8`, `fn_56_138F0`, `fn_56_13B80`, `fn_56_149D4`, `fn_56_14A74` | PW ×7 | the last member constructors and the touch-bounds accessor |

### i.1 The slices in one paragraph

P0 is the skeleton (the loader and the two actor constructors write the member layout). P1 is
the FSM skeleton and the Think core. P2 is Think's tail. P3-P4 the eyes. P5-P8 the spine.
P9-P10 the pursuit. P11-P14 the attacks (melee, spit, charge, grab). P15-P16 the bombs.
P17-P18 the lifecycle states (dead, deactivate, descend, rise, lurk, sulk, straighten, null).
P19 the Ing-snatching path. P20-P26 the member constructors and destructors, the script-msg
handler, the anim-event handler and the render helpers.

### i.2 The files, as built


`-` (83 functions, 0x2BE8 bytes) means no port body: 82 host copies (section g.1) and the REL's
static init (`fn_56_14FA8`), which the host does not need. Byte totals are REL `.text` sizes.

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

## j. Port status (2026-10-19, updated 2026-10-21)

- 2026-10-21 (third pass): the two bodies that still had a retail caller are marked now -
  `fn_56_95E0` (both arms: the breakpoint printed the held bomb's id 1523/1547 for the delete
  arm and -1 for the early-out) and `fn_56_10A20` (68174 calls; the port's two spine helpers
  now call the wrapper, as retail does). The seven that are left are cited with proof in
  section l: `fn_56_C350` and `fn_56_C3D0` are vtable entries no FSM2 asset asks for (the
  disc scan is in l.2), and `fn_56_2210`, `534C`, `6324`, `10A68` (plus `10110`, `10268`,
  `14FA8`) have no caller in the retail module at all.
- 2026-10-21 (second pass, code + note): the four bodies that the old note called
  "data-gated" ran after all - the Bomb Guardian's AFSM *has* the BombFountainAttack state,
  and forcing it with the console (`fsm u048f BombFountainAttack`, plus `send u048f Activate`)
  runs the picked-7 chain that the prefab's own attack pick never chooses. `fn_56_35EC`
  (478 trigger calls), `fn_56_8E90` (27), `fn_56_B8B8` (481) and `fn_56_C828` (1, forced
  `fsm u048f Null`) are marked now; `fn_56_95E0` ran 3 times but only its delete arm, so it
  stays unmarked. Section l has the per-arm table for all 15 unmarked B functions, the three
  bodies without one and the answers to the three `tools/asmcheck.py` findings.
- 2026-10-21 (this pass, docs only): the note gained the classification of all **383** function
  symbols (section g: 97 host copies, 20 thunks, 266 module behaviour), the upstream-class check
  (section h) and the canonical port order in 27 slices of ~10 (section i). `Sandworm_functions.tsv`
  grew from 375 to 383 rows - the 8 symbols it never listed were `fn_56_14FA8` (the module's static
  init) and the 7 crtexe runtime symbols - and it gained a `class` column. No port code changed:
  host build `0 FAILED`, `gen_files_cmake --check` rc 0. The `docs/modules.tsv` Sandworm row is
  stale (`status=entry-stub`); regenerate the manifest to fix it.
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
- Grab group (`01_sand_entrance`, 2026-10-20): morphing (`press x`, `status` shows `morph 1`) and
  standing ~2 m from the worm makes the FSM take the grab branch — `CanGrabMorphball` →
  `GrabHelper` → `GrabAttack` → `GrabPlayer` → `SendGrabMsgs`/`SendAttackMsgs` →
  `TeleportGrabbedPlayer` per frame → `ReleaseGrabbedPlayer`/`ApplyMeleeDamage` →
  `StopGrabAttack` → `GrabAttackFinished`, so all 11 bodies are marked. `2C30`
  (CSandworm::GetOrbitPosition, eye-based) was hit in the same run. The worm has to be
  re-`tp`ed onto every ~2.5 s because it keeps moving.
- Sulk group (`01_sand_entrance`, 2026-10-20): worm u0517 went `Pursue` → `PlayerHiding=1` +
  `CanDescend=1` → `Straighten` → `Descend` → `Sulk` → `DoneSulking=1` → `Rise`, so `3E1C`
  (`CanDescend`), `5470` (`Sulk`) and `541C` (`DoneSulking`) are marked. Sequence: `objs sandworm`,
  `tp` 20 m in front of the worm + `face` it, 8 s still; `press l 0` (hold lock-on) then
  `press l+a 3` / `press l 10` bursts to hp 120 → 90 (power beam, ~2 hp per hit), console
  `damage u0517 18` to hp 60 for the sulk band; `tp` back 14 m from it every few bursts. Then stand
  still on the path (`tp` worm + (0,-14)) for the sulk/DoneSulking, and step off it (worm + (10,-8),
  8 s) to flip `PlayerHiding`/`CanDescend`. Proof: `[trig] CanDescend=1 neg=0 in=Pursue`,
  `[trig] DoneSulking=0/1 neg=0 in=Sulk`, cov2 hits `PortCSandwormStates.cpp:240:fn_56_3E1C`,
  `:336:fn_56_5470`, `:331:fn_56_541C`. `ShouldSulk` stayed 0 even at hp 60 (50%), so this prefab
  sulks through the hide/descend gate, not the HP gate.
- Still unexercised after that run: `C350` (`InAttackRange`) and `C3D0` (`Activated`). A temporary
  dump of every `CGenericFSM2` the worm loads (three machines; the worm's is the 23-state one, all
  23 states' transitions printed) shows no state referencing either trigger, and `Deactivate`'s
  only transition is `DoneLurking` → `Rise`, so console `send Deactivate`/`Activate` never evaluates
  `Activated`; `InAttackRange` is likewise never asked (`Pursue` uses `CanAttack`, `InSpitRange`,
  `MeleeAttackRange`). Both must belong to another prefab's AFSM, so they stayed open for the Bomb
  Guardian run — which never evaluated them either (bucket C below), so they remain open. Probe
  reverted.
- Bomb Guardian form (`03_sand_temple` L2, area 26, mrea `75fc9aae`, 2026-10-20): the layer spawns
  two worms — `u048f` (canLinkTransfer 1, hp 950, inactive until the player walks into the arena) and
  `u04e9` (Sandigger form, hp 1000). Entering the arena wakes the BG; on the first boot it was then
  possessed in place: hp reset 950 → 160/160 (`ingPossessedHealth`), `mIngPossessionTarget` 1.00 and
  the blend ramping 0.15 → 0.98 in ~1 s with `IsBeingSnatched()` true, which ran 27 of the 36
  bucket-C bodies in one go — the ctor trio `14010`/`149D4`/`1FD8`, `10F90`, `3990`, `3844`, `30F8`,
  `14AF4`, the snatch-render four (`2994` `312C` `3180` `9860`), `566C`, `11CF8`, `986C`/`97FC` and the
  spread/bomb-spawn bodies. That possession was not reproducible on later boots and area 26 contains
  no `CIngSnatchingSwarm` (`objs` finds none; `30F8`/`14AF4` ran on that first boot only), so the
  fight runs used a temporary `SetIngPossessed(true, mgr)` hook, reverted with the stderr probe.
- Phases and the bomb attacks: the prefab's five `SLdrSandwormStruct`s have thresholds 120/80/40/0/0
  and kinds 3/5/5, so `PickAttackType` only ever returns 5 (toss) or 6 (spread) — kind 3 alternates
  them, the kinds at `x968_` 1/2 are 5 and fall into `default` → -1, which resets `x968_`. Stepping hp
  down the bands (console `damage u048f`, 20-25 s dwell each, beam fire in between) ran
  `BombSpreadAttack` (`BAB8`), `BombTossAttack` (`BC8C`) and the whole held-bomb chain (`9660` `9368`
  `9590` `9440`), plus `EF8C`/`F328` and the spawn/sfx trio. Kind 4 never appears in the prefab's
  own pick, so picked 7 is not chosen by play: `B8B8` (`BombFountainAttack`), `8E90`
  (`SpawnBombToss`), `35EC` (`HitDuringWindUp`, the picked-7 wind-up flag branch) and `95E0`
  (`DeleteHeldBomb`'s picked-7 branch) are **data-gated**, not port gaps — annotated as such in the
  source, and they count as unreachable (bucket E) for this build. **Corrected 2026-10-21:** the
  prefab's AFSM *does* have the BombFountainAttack state, and a forced state (`fsm u048f
  BombFountainAttack`) runs all four (section l).
- That area also ran four bucket-D bodies: `321C` (`GetAimPosition`), `F710` (`Primed`), `F754`
  (`Prime`) and `F77C` (`SpineHelperF77C`) — `Prime` runs there because this prefab's `Null`
  path has the underground bit clear. `14AEC`/`14B24` still need a projectile to reach the worm
  actor itself; `14B3C` turned out to be a dead slot of its own (see below).
- Dead slots annotated (2026-10-20): nine bodies with a static reason cannot run in this build and
  the source now says why instead of leaving them open. `14AE4` (`GetPointSearchPath`: only
  `CIngSpotPathFindNavigation` reads it, and the worm navigates with `mWaypointNavigation`), `14B34`
  (`Listen`: `CStateManager::InformListeners` only walks `kOL_ListeningAi`, which admits a patterned
  only while `IsListening()` is true, and `14B2C` returns false), `14B3C` (`GetOrigin`: the only
  caller is `CScriptTeamAiMgr::PositionTeam` mode 0, and an actor joins a team only over an
  ACTV/PLAY link to an MRLY; the WORM and MRLY connection lists of `01_sand_entrance`,
  `03_sand_temple` and `04_sand_mining` hold no such link into a worm), `14A6C` (`Unknown23C`),
  `2210` (`GetX2E8`), `10A20` (`GetLctrTransformWrapper`) — no caller anywhere in the tree — and
  `534C`, `10A68`, `6324`, which already carried "unreferenced in retail" notes. `F328` is not a
  body at all: it is the inlined `CVector3f::Cross` inside `EF8C`, so its stale tag is gone.
- Bomb and possession runs (2026-10-20, seven boots of 03_sand_temple L2 plus two of 01_sand_entrance):
  - `C828` still has no route. `SetUnderground` (`AC24`) never ran in any area-26 boot — no cov hit and
    the uid probe stayed silent — because u048f's FSM only ever goes `Start` → `Prime` → `Snatch` and
    stays in `Snatch` (n≈3000-4200). `send Activate`/`Deactivate`, console `damage`, walking out to 40 m
    and back and a 60 s dwell all left it there; `Null`/`Patrol` never activated, and the Ing possession
    did not reproduce in any boot (hp stayed 950). The prefab precondition does hold live:
    `x8C0_flags` reads `0x10` inert and `0x12` awake, bit 7 clear throughout. The `AC24`/`112E8` hits of
    the earlier BG runs therefore belonged to the adjacent u04e9, not to the Bomb Guardian.
  - The morph ball's bomb is a `CWeapon`, not a `CGameProjectile`: `CBomb` (`CPlayerGun::DropBomb`) and
    `CBouncingBomb` (`SpawnBouncingBomb`) both derive from `CWeapon`, whose `Think` does its own
    box/sphere test and `Explode`, so neither ever reaches `CGameProjectile::CanCollideWith` and
    `GetRagDoll`/`CanBeShot` cannot be reached through a bomb. The only `CGameProjectile`s in the tree
    are `CBeamProjectile` (the player's beam) and `CEnergyProjectile`.
  - A bomb kill works and is clean: worm u0596 at 12/120 hp, morphed (`press x`), 3 m away, ~9 x
    `press a` → hp -9 and the actor deleted, 0 caught errors. It exercises neither `14AEC`/`14B24` nor
    `14B1C`: the worm's death takes no `kFU_ExplodeDeath` follow-up, so `CreateXDamageParticles` never
    runs.
  - `14B10` `GetGravityConstant` is a dead slot: probed in 03_sand_temple, `HasBodyState(kAS_Jump)` and
    `HasBodyState(kAS_Hurled)` are 0 on both worms at every sample, and those two body states are its
    only callers.
  - The projectile bodies are dead slots. A probe at `CGameProjectile::CanCollideWith` with beam
    bursts sweeping both area-2 worms logged 29 distinct uids, every one `patterned=0` — the triggers
    (`0488`, `052c`) and the worms' own collision actors — so the worm pattern never enters a
    projectile's near list and `CanCollideWithGameObject` (the only caller of `GetRagDoll` and
    `CanBeShot`) never runs for it. The worm takes beam damage through its collision actors.
  - `GetDeathExplosionParticle` (`14B1C`) is data-gated. Probing `CKnockBackMgr::KnockBack` while
    killing worms with six weapon modes (power, dark, light, missile, bomb, power bomb) recorded
    followups None (0), BurnPhase (5), BurnDeath (9) and LaggedBurnDeath (10) — never
    `kFU_ExplodeDeath` (7), so `CPatterned::CreateXDamageParticles`, the only reader, never runs.
- No body is left open. `C828` is blocked by its own prefab: it runs only from `SetUnderground`,
  which only `Patrol`/`Null` activate call, and u048f never leaves `Start`/`Prime`/`Snatch`
  (seven boots, above); the `AC24` hits of earlier runs belong to the adjacent u04e9, whose prefab
  sets the bit. The rest are the four data-gated bomb bodies (`B8B8` `8E90` `35EC` `95E0`),
  `C350`/`C3D0` (asked by neither prefab) and the dead slots below.
- Markers: 262 functions carry `// @`; 20 remain `(ported, not exercised yet)` and every one of them
  now carries the reason it cannot run: the four data-gated bomb bodies, `C350`/`C3D0` (asked by
  neither prefab), `C828` (its prefab never leaves `Snatch`) and thirteen dead slots (`14AE4`,
  `14B34`, `14B3C`, `14A6C`, `2210`, `10A20`, `534C`, `10A68`, `6324`, `14B10`, `14AEC`, `14B24`,
  `14B1C`). `/tmp/opencode/sw2/unexercised79.md` (local) buckets them: A 24 (21 done;
  `C350`/`C3D0` are never asked by either prefab), B 1 (`C828`), C 36 (32 done, 4 data-gated),
  D 12 (4 done), E 6. Hit lists: `/tmp/opencode/sw3/hits_union.txt` (219 bodies),
  `cov3_hits_bg1.txt`/`cov3_hits_bg_fight3.txt` (Bomb Guardian), `cov3_hits_sw6a.txt` (projectile
  trace); driver `/tmp/opencode/sw3/bg.py`.
- Open:
  - BeginAttackHelper/2 still operate on x1490_attacks rather than SSandwormAttackHistory.
  - DoUserAnimEvent event numbers vs jumptable data_790; the CBodyController+0x4C write in ChargePlayer.
  - fn_56_6798 passes `arg0+0x54` and rodata_150 to A8E0; the port passes `pos, 0.f`.
  - CBCTauntCmd's second word is lost on the host; constants 0.4-1.2, 0.1-0.5, 0.2094 and rodata_194 are not pinned.
  - TypesMatch registers the eye under CActor, but on PC it derives CPhysicsActor.
  - The "Sandworm.rel" token string at lbl_56_bss_C is a guess.
  - Render line 25 skips a missing model.
  - x1614 has no name; x199A_ is a byte.
- Pseudo-C is in `/tmp/opencode/pseudo/Sandworm/fn_56_XXXX.c`. Runtime areas are Agon Wastes
  `01_sand_entrance` L0 (Sandigger/Alpha), `03_sand_temple` L2 (both forms; the Bomb Guardian is
  `u048f` there) and `04_sand_mining` L3 (Sandigger form only, `canLinkTransfer` 0). The bar is a
  fight the player can finish.

## k. Open questions

1. The element types and capacities of x99C, x12B0 and x1670, and what `data_6B0`/`6B4` mean as 13 fill values.
2. The exact layout of the 0x84-byte spit block at x1528 (`CProjectileInfo` plus visor effect).
3. The meaning of `unknown_0x06dee4c5` (x8C0 b7, BeginUnderground) and of the SLdrSandwormStruct ints.
4. The names of vtable slot 0x23C and of the x620 member class (`data_844`).
5. The DoUserAnimEvent event-type → action mapping (`jumptable data_790`); the projectile case (0) is read (section l), the others still need a per-case read.
6. The `CSandwormEye` base mismatch with the repo header (`CActor` versus `CPhysicsActor`).
7. The `bss_54` triple (0.0, 0.0, 1.5) the REL's static init writes (section l.2): the host uses the same
   constants inline, but nothing in the tree reads them yet.

## l. The unmarked functions: who calls them in retail, and which arms ran

### l.0 How "who calls it" was established

Three independent scans over the module (the REL's own asm in the archive tree,
`<archive>/build/G2ME01/Sandworm/asm`):

1. **Direct calls.** `grep 'bl fn_56_XXXX'` over the module's `.text` gives the call sites.
2. **Table references.** The module's `.data` (the archive's `auto_04_00000000_data.s`) holds the
   vtables and the FSM registration tables; a function named there (as a symbolic `.4byte
   fn_56_XXXX` or as its raw `.text` offset) is called *through* that table. The CSandworm's vtable
   is `lbl_56_data_884`, the eye's is `lbl_56_data_6F8`.
3. **The FSM assets.** A trigger or a state runs only when the FSM2 asset asks for it by name. The
   146 `FSM2` assets of the 20 paks were read with `tools/echoes_disc.py` and searched for the
   names - they are stored as plain strings, so the scan is exact (l.2).

### l.1 The callers and the arms

| id | retail caller | arms | evidence | decision |
|---|---|---|---|---|
| `fn_56_95E0` DeleteHeldBomb | four call sites: the anim event `fn_56_646C` (case 26), the fountain state's exit `fn_56_B8B8` (0xB9A8), the toss state's exit `fn_56_BC8C` (0xBD3C) and the worm's XDMG handler `fn_56_11374` (0x11B64) | the bomb id test: delete a live bomb / early-out | run 2026-10-21: the breakpoint printed the held id - 1523 and 1547 (a live bomb, the delete arm) and -1 (no bomb, the early-out). The early-out comes from the XDMG path: the fountain attack's exit had already deleted the bomb, so the id was invalid when the eyes were shot | **marked** |
| `fn_56_10A20` GetLctrTransformWrapper | `fn_56_108BC` (0x1094C) and `fn_56_1096C` (0x10A00) | none (a straight copy of the translation, 0x10A48..0x10A50) | run 2026-10-21: 68174 calls, one per spine update. The port now calls the wrapper from those two helpers, as retail does (the port used to inline `GetLctrTransform(segId).GetTranslation()`) | **marked** |
| `fn_56_C350` InAttackRange | the CSandworm vtable (`data_884`): the FSM asks for the trigger by name | two compares (0xC3AC, 0xC3B8) | **never asked** - of the 146 FSM2 assets only the Metroid's (`1870596557`) names "InAttackRange"; the worm's own two assets do not (l.2) | cited, not marked |
| `fn_56_C3D0` Activated | the CSandworm vtable (`data_884`) | one test (the actor's active flag, 0xC3D0) | **never asked** - no FSM2 asset in the game names "Activated" (0 of 146) | cited, not marked |
| `fn_56_2210` CSandwormEye::GetX2E8 | **none**: no `bl`, and not in any table (not in the eye's vtable `data_6F8` either) | none (a straight copy) | dead code in the retail module | cited, not marked |
| `fn_56_534C` CountScriptActors | **none** (no `bl`, not in any table) | the GRNT/NEXT test, the CActor cast test | dead code in the retail module | cited, not marked |
| `fn_56_6324` Unreferenced6324 | **none** (no `bl`, not in any table) | the CONN/FOLW test | dead code in the retail module | cited, not marked |
| `fn_56_10A68` SetSteeringSpeedRange | **none** (no `bl`, not in any table) | the 1.2 s test (rodata_1C8) | dead code in the retail module | cited, not marked |

The four dead ones are the same class as `fn_56_10110` and `fn_56_10268` (l.3): the retail module
carries them, nothing in the module calls them, and the DOL can only reach the module through
`RELMain`, `RELExit`, the loader and the two radar callbacks - none of which reaches them.

### l.2 The FSM2 assets: which trigger is asked where

The scan (`tools/echoes_disc.py`, 20 paks, 146 `FSM2` assets) lists the assets that name a worm
trigger or state:

| FSM2 asset | size | the worm names it uses |
|---|---|---|
| `1922004252` (Metroid3.pak) | 2048 | triggers: BeginUnderground, CanAttack, CanCharge, CanDescend, CanGrabMorphball, CanSpitAgain, CanMeleeAgain, ChargeOver, DoneLurking, DoneStraightening, DoneSulking, ForceUnderground, GrabAttackFinished, **HitDuringWindUp**, InSpitRange, MeleeAttackRange, OneEyeKilled, PickedBombFountain, PlayerHiding, PlayerIsMorphball, PlayerIsOnPathMesh, Primed, ReadyToCharge, SequenceAttack, ShouldSulk, SnatchStarted, SnatchEnded, SpitAngleOK. states: BombFountainAttack, ChargePlayer, ChargeWindUp, Deactivate, Dead, Descend, EyeKilledReaction, GrabAttack, LurkUnderground, MeleeAttack, Null, Patrol, PickMediumAttack, Prime, Pursue, Rise, Snatch, StopGrabAttack, Straighten, Sulk |
| `3519817736` (Metroid3.pak) | 288 | triggers: PickedBombFountain, PickedBombToss, PickedBombSpread, PickedSpitAttack. states: BombFountainAttack, BombSpreadAttack, BombTossAttack, PickMediumAttack, SpitAttack (the Bomb Guardian's own machine) |
| `1870596557` (Metroid1/2/3.pak) | 3264 | the Metroid's machine: it names **InAttackRange** (and FacingPlayer) - the only asset in the game that asks for the worm's `InAttackRange` trigger |

So the two triggers that no AFSM asks are gated by the shipped data, not by the port:

- **`fn_56_C350` InAttackRange** is the *Metroid* boss's trigger (the same name in the Metroid's
  FSM2). The worm's own two machines never ask it, so the worm's copy never runs.
- **`fn_56_C3D0` Activated** is asked by no FSM2 asset in the game (0 of 146), so the worm's copy
  never runs.

The eight dead virtual slots of T stay unmarked for the reasons in section j.

### l.3 The three bodies without one

- **`fn_56_10110`** (0x158 bytes) walks the joint list at `this + 0x9A0` (12-byte entries) from
  `count - 1` down to 0, and for the first entry whose squared distance from `entry[count]` exceeds
  `radius²` writes the index and returns `entry[count] + radius * normalise(entry[i] -
  entry[count])` (the compares at 0x101B4, the normalise at 0x101C4, the stores at 0x101F8..0x10204).
  When no entry is far enough it writes 0 and returns `entry[0]` (0x10214..0x10230).
- **`fn_56_10268`** (0x2A8 bytes) takes the same list and sweeps the segment `i → i + 1`: it lerps
  from `entry[i]` to `entry[i+1]` with `t` from rodata_150 in steps of rodata_244 (0.02) up to
  rodata_128 (1.0) and returns the first point whose distance to the given point exceeds the radius
  (the magnitude test at 0x10418), with the index pair and the point in the out struct (0x10430).
  When no segment crosses the radius it returns 0, 0 and `entry[0]` (0x10458..0x10478).
- Both are **dead code in the retail module**: `grep 'bl fn_56_10110'` and `grep 'bl
  fn_56_10268'` over the module's asm return nothing, and the module's only external entries are
  the script loader, the two radar callbacks and the REL entry points - none of them reaches them.
  The port therefore keeps the declarations (`UnreferencedMath10110`, `UnreferencedMath10268`)
  without a body, and this note records what they did.

### l.4 `fn_56_14FA8`: the module's static init

The REL's `.ctors` entry (0x14FA8, 0x8E8 bytes) does three things, all at module load:

1. **0x14FB4..0x14FDC** builds a global `rstl::basic_string` in `bss_C` from the literal at
   rodata_6D9, which is **"Sandworm.rel"** (the same rodata object holds "Interrupt",
   "Sandworm Eye Head" and "Sandworm Eye Tail"). The eye's `x2F8` (`CRELFileToken`) keeps the REL
   loaded with that name.
2. **0x14FDC..0x14FEC** calls `__register_global_object(fn_56_5250, bss_0)`, so the string's
   destructor (`fn_56_5250`, which dereferences the string and frees it) goes on the global
   destructor chain.
3. **0x14FF8..0x15890** copies the *relocated* 3-word groups of the small `.data` tables
   (`data_4` … `data_1B4`, `data_410` … `data_518`, `data_694`) into the matching 0x10-byte entry of
   the three FSM registration tables: the trigger table `data_1C0` (37 entries), the state table
   `data_524` (23) and the extras table `data_6A0` (1). The REL's linker puts the trigger and state
   function pointers in their own tables, and the static init copies them into the tables
   `fn_56_D548` hands to the FSM. It also writes `bss_54` = {rodata_150, rodata_150, rodata_1AC} =
   {0.0, 0.0, 1.5} (0x15060, 0x150D0, 0x150D4).

**Where the host does the same:** the host never runs the REL, so there is no static init and no
global destructor chain. (1) has no host equivalent - `CSandwormEye::x2F8_relToken` keeps a
`CRELFileToken` whose name the host cannot resolve, and the port leaves it empty. (3) is replaced
by the port's own static tables: `skTriggers`, `skStates` and `skCodeFuncs` in
`src/MetroidPrime/Enemies/PortCSandworm.cpp` (the same 37/23/1 entries, with the names the REL
registers - `Activated` … `SpitAngleOK`, `BombFountainAttack` … `Sulk`), handed to the FSM by
`CSandworm::InitStateMachine` (`fn_56_D548`, port body). Because the host's tables are C++ arrays
of function pointers, no copy step and no relocation is needed. (2) `bss_54` has no host
counterpart either: the port uses the same constants inline (rodata_150 = 0.0, rodata_1AC = 1.5).

### l.5 The asmcheck findings

`tools/asmcheck.py check Sandworm <file> --only MASK,XBIT,CROP,XCMP,XSTORE,OFFS` over all seven
`PortCSandworm*.cpp` files: 279 functions checked, **3 findings**, all answered here.

| finding | answer |
|---|---|
| `XCMP fn_56_128B0 0x12924`: the C++ compares `x1770_pincers` (0x1770), the asm never compares it | **false positive of the tool, and a real difference in the test.** The asm compares the *value* loaded from 0x1770: `addis r0, r6, 1; cmplwi r0, 0xffff` (0x12928, 0x1292C) - the module's "asset id == -1" idiom (the same one as `fn_56_1A38` at 0x1A4C and `fn_56_138F0` at 0x13944). The tool cannot model the `addis` (it keeps the value opaque), so it does not see the compare. The port's `x1770_pincers.x0_left != kInvalidAssetId` is the project's reading of that idiom (see `include/MetroidPrime/Enemies/CDarkSamus.hpp:819` and `src/MetroidPrime/Enemies/PortCDarkSamus.cpp:174`). The residual difference: the retail's test only looks at the low 16 bits, so an asset id like 0x0000FFFF would be "invalid" for retail and valid for the port; no such id is reachable from the loader (the ids are 0xFFFFFFFF or small numbers), so the port keeps the clearer C++. |
| `XCMP fn_56_128B0 0x129A0`: the same for `x1774_right` | the same, the second pincer model (0x129A4, 0x129A8). |
| `XCMP fn_56_3D64 0x3DD0`: the C++ compares `x1850_` (0x1850), the asm never compares it | **false positive.** The asm *loads* the byte (`lbz r3, 0x1850(r31)`, 0x3DD0) and returns it; the byte only ever holds 0 or 1 (`li r0, 1; stb` at 0x3DB0, `li r0, 0; stb` at 0x3DBC), so the compiler can skip the normalisation and the port's `x1850_ != 0` is the same value. The tool has no rule for "the C++ compares, the asm returns the byte". |
