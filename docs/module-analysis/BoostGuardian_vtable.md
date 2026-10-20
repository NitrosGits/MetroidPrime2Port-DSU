# IngBoostBallGuardian (REL module 30) — class vtables

Source: `build/G2ME01/IngBoostBallGuardian/asm/auto_04_00000000_data.s` (vtables), `auto_00_00000000_text.s` / `auto_00_00011CD8_text.s` / `auto_fn_30_110C4_text.s` (code). `kind` = `local` when the word is a `fn_30_*` symbol defined in this REL, else `dol` (named DOL symbol or null word). `idx` = slot index (idx 0 = obj word 2, the first word after the two leading `0x00000000` words); `obj#` = index of the word inside the object.
## 1. Vtables

### lbl_30_data_9C0 — 82 words (0x9C0..0xB07), main actor; installed by `fn_30_EC6C` at [0x0000ECE8]

| idx | obj# | word | kind |
|---|---|---|---|
| — | 0 | `0x00000000` | dol |
| — | 1 | `0x00000000` | dol |
| 0 | 2 | `fn_30_E5EC` | local |
| 1 | 3 | `fn_8009B2EC` | dol |
| 2 | 4 | `fn_30_DC10` | local |
| 3 | 5 | `fn_30_D94C` | local |
| 4 | 6 | `fn_30_DDA0` | local |
| 5 | 7 | `SetActive__6CActorFb` | dol |
| 6 | 8 | `ClearFluidList__6CActorFR13CStateManager` | dol |
| 7 | 9 | `fn_30_D2E0` | local |
| 8 | 10 | `fn_30_D250` | local |
| 9 | 11 | `fn_30_D384` | local |
| 10 | 12 | `CanRenderUnsorted__10CPatternedCFRC13CStateManager` | dol |
| 11 | 13 | `fn_30_D2C0` | local |
| 12 | 14 | `HealthInfo__3CAiFv` | dol |
| 13 | 15 | `fn_30_90` | local |
| 14 | 16 | `fn_30_D230` | local |
| 15 | 17 | `GetDamageVulnerability__10CPatternedCFRC9CVector3fRC9CVector3fRC11CDamageInfo` | dol |
| 16 | 18 | `fn_30_CC34` | local |
| 17 | 19 | `fn_30_CE50` | local |
| 18 | 20 | `GetOrbitPosition__10CPatternedCFRC13CStateManager` | dol |
| 19 | 21 | `fn_30_CFC4` | local |
| 20 | 22 | `GetHomingPosition__6CActorCFRC13CStateManagerf` | dol |
| 21 | 23 | `GetScanObjectIndicatorPosition__6CActorCFRC13CStateManager` | dol |
| 22 | 24 | `GetCollisionResponseType__10CPatternedCFRC9CVector3fRC9CVector3fRC11CWeaponModei` | dol |
| 23 | 25 | `FluidFXThink__3CAiFQ26CActor11EFluidStateR12CScriptWaterR13CStateManager` | dol |
| 24 | 26 | `OnScanStateChange__6CActorFQ26CActor10EScanStateR13CStateManager` | dol |
| 25 | 27 | `GetSortingBounds__6CActorCFRC13CStateManager` | dol |
| 26 | 28 | `fn_30_D4F8` | local |
| 27 | 29 | `GetScannableObjectInfo__10CPatternedCFv` | dol |
| 28 | 30 | `ProcessSoundEvent__6CActorFififfRC6CSegIdUsUsfUcUcfRC9CVector3fiR13CStateManagerb` | dol |
| 29 | 31 | `GetCollisionPrimitive__13CPhysicsActorCFv` | dol |
| 30 | 32 | `GetPrimitiveTransform__13CPhysicsActorCFv` | dol |
| 31 | 33 | `fn_30_C850` | local |
| 32 | 34 | `GetStepDownHeight__13CPhysicsActorCFv` | dol |
| 33 | 35 | `GetStepUpHeight__13CPhysicsActorCFv` | dol |
| 34 | 36 | `IsOnStaticGround__10CPatternedCFv` | dol |
| 35 | 37 | `GetWeight__13CPhysicsActorCFv` | dol |
| 36 | 38 | `fn_30_76F4` | local |
| 37 | 39 | `fn_30_CB84` | local |
| 38 | 40 | `DamageVulnerability__3CAiFv` | dol |
| 39 | 41 | `fn_30_CB54` | local |
| 40 | 42 | `fn_30_6C` | local |
| 41 | 43 | `fn_30_10` | local |
| 42 | 44 | `fn_30_C758` | local |
| 43 | 45 | `fn_30_74` | local |
| 44 | 46 | `RenderSystemsToBeDrawnFirst__10CPatternedCFRC13CStateManagerUiUi` | dol |
| 45 | 47 | `RenderSystemsToBeDrawnLast__10CPatternedCFRC13CStateManagerUiUi` | dol |
| 46 | 48 | `Freeze__10CPatternedFR13CStateManagerRC9CVector3f13CUnitVector3fff` | dol |
| 47 | 49 | `ThinkAboutMove__10CPatternedFf` | dol |
| 48 | 50 | `fn_30_24` | local |
| 49 | 51 | `Burn__10CPatternedFR13CStateManagerff` | dol |
| 50 | 52 | `Shock__10CPatternedFR13CStateManagerff` | dol |
| 51 | 53 | `MassiveDeath__10CPatternedFR13CStateManager` | dol |
| 52 | 54 | `MassiveFrozenDeath__10CPatternedFR13CStateManager` | dol |
| 53 | 55 | `fn_30_2C` | local |
| 54 | 56 | `fn_30_0` | local |
| 55 | 57 | `fn_30_8` | local |
| 56 | 58 | `GetContactDamage__10CPatternedCFv` | dol |
| 57 | 59 | `fn_30_CA18` | local |
| 58 | 60 | `fn_30_C6CC` | local |
| 59 | 61 | `fn_30_34` | local |
| 60 | 62 | `CanBeIngPossessed__10CPatternedCFR13CStateManager` | dol |
| 61 | 63 | `CanBeUnPossessed__10CPatternedCFR13CStateManager` | dol |
| 62 | 64 | `SetIngPossessed__10CPatternedFbR13CStateManager` | dol |
| 63 | 65 | `SetIngPossessed__10CPatternedFbfR13CStateManager` | dol |
| 64 | 66 | `SetAttackTarget__10CPatternedFR13CStateManager9TUniqueId` | dol |
| 65 | 67 | `fn_30_3C` | local |
| 66 | 68 | `fn_30_4C` | local |
| 67 | 69 | `fn_30_58` | local |
| 68 | 70 | `IsScanVisorSelfRender__10CPatternedCFv` | dol |
| 69 | 71 | `GetScanVisorRenderBounds__10CPatternedCFRC13CStateManager` | dol |
| 70 | 72 | `ScanVisorRender__10CPatternedCFRC13CStateManagerRC12CTransform4fRC11CModelFlags` | dol |
| 71 | 73 | `fn_30_64` | local |
| 72 | 74 | `fn_30_18` | local |
| 73 | 75 | `TryToBeCaptured__10CPatternedFR13CStateManager` | dol |
| 74 | 76 | `IssueDeathBodyCommand__10CPatternedFR13CStateManagerRC9CVector3f` | dol |
| 75 | 77 | `GetFadeOnDeathTime__10CPatternedCFv` | dol |
| 76 | 78 | `GetIngSnatchingNormal__10CPatternedCFf` | dol |
| 77 | 79 | `GetIngSnatchingPoint__10CPatternedCFf` | dol |
| 78 | 80 | `GetIngSnatchingModelOverlapSize__10CPatternedCFv` | dol |
| 79 | 81 | `RenderIngSnatchingTransition__10CPatternedCFRC13CStateManager` | dol |

### lbl_30_data_C44 — 38 words (0xC44..0xCDB), CPhysicsActor-based class; installed by `fn_30_15924` at [0x00015A90]

| idx | obj# | word | kind |
|---|---|---|---|
| — | 0 | `0x00000000` | dol |
| — | 1 | `0x00000000` | dol |
| 0 | 2 | `fn_30_11CD8` | local |
| 1 | 3 | `fn_8009AC24` | dol |
| 2 | 4 | `PreThink__7CEntityFfR13CStateManager` | dol |
| 3 | 5 | `fn_30_1548C` | local |
| 4 | 6 | `fn_30_1466C` | local |
| 5 | 7 | `SetActive__6CActorFb` | dol |
| 6 | 8 | `ClearFluidList__6CActorFR13CStateManager` | dol |
| 7 | 9 | `PreRender__6CActorFR13CStateManager` | dol |
| 8 | 10 | `fn_30_13E68` | local |
| 9 | 11 | `fn_30_141B8` | local |
| 10 | 12 | `CanRenderUnsorted__6CActorCFRC13CStateManager` | dol |
| 11 | 13 | `fn_30_13ECC` | local |
| 12 | 14 | `fn_30_13C5C` | local |
| 13 | 15 | `fn_30_13C40` | local |
| 14 | 16 | `fn_30_13B90` | local |
| 15 | 17 | `fn_30_13B88` | local |
| 16 | 18 | `fn_30_13BA4` | local |
| 17 | 19 | `fn_30_1464C` | local |
| 18 | 20 | `GetOrbitPosition__13CPhysicsActorCFRC13CStateManager` | dol |
| 19 | 21 | `GetAimPosition__13CPhysicsActorCFRC13CStateManagerf` | dol |
| 20 | 22 | `GetHomingPosition__6CActorCFRC13CStateManagerf` | dol |
| 21 | 23 | `GetScanObjectIndicatorPosition__6CActorCFRC13CStateManager` | dol |
| 22 | 24 | `GetCollisionResponseType__6CActorCFRC9CVector3fRC9CVector3fRC11CWeaponModei` | dol |
| 23 | 25 | `FluidFXThink__6CActorFQ26CActor11EFluidStateR12CScriptWaterR13CStateManager` | dol |
| 24 | 26 | `OnScanStateChange__6CActorFQ26CActor10EScanStateR13CStateManager` | dol |
| 25 | 27 | `GetSortingBounds__6CActorCFRC13CStateManager` | dol |
| 26 | 28 | `DoUserAnimEvent__6CActorFR13CStateManagerRC13CInt32POINode14EUserEventTypef` | dol |
| 27 | 29 | `GetScannableObjectInfo__6CActorCFv` | dol |
| 28 | 30 | `ProcessSoundEvent__6CActorFififfRC6CSegIdUsUsfUcUcfRC9CVector3fiR13CStateManagerb` | dol |
| 29 | 31 | `fn_30_11E44` | local |
| 30 | 32 | `GetPrimitiveTransform__13CPhysicsActorCFv` | dol |
| 31 | 33 | `fn_30_142BC` | local |
| 32 | 34 | `GetStepDownHeight__13CPhysicsActorCFv` | dol |
| 33 | 35 | `GetStepUpHeight__13CPhysicsActorCFv` | dol |
| 34 | 36 | `IsOnStaticGround__13CPhysicsActorCFv` | dol |
| 35 | 37 | `GetWeight__13CPhysicsActorCFv` | dol |

## 2. Local overrides (48 distinct `fn_30_*` in the two vtables)

- **`fn_30_E5EC`** — 9C0 slot 0 · `auto_00_00000000_text.s` L16520 · [0x0000E5EC] · 616 B (0x268) — Destructor: re-stores lbl_30_data_9C0 into 0(r30) [0x0000E61C], then tears down members — __dt__20CDamageVulnerabilityFv [0x0000E620], fn_30_E4EC [0x0000E62C], __dt__6CTokenFv [0x0000E658], fn_30_5E4 [0x0000E7C4], fn_30_EACC [0x0000E7D0], __dt__22CCollisionActorManagerFv [0x0000E7E4], fn_30_2C44 [0x0000E818], base fn_30_E854 [0x0000E824] — and Free__7CMemoryFPCv [0x0000E834] when the deleting flag is set.
- **`fn_30_DC10`** — 9C0 slot 2 · `auto_00_00000000_text.s` L15826 · [0x0000DC10] · 400 B (0x190) — PreThink override: GetPlane__17CCollisionSurfaceCFv [0x0000DC80], SetTranslation__6CActorFRC9CVector3f [0x0000DD50], PreThink__10CPatternedFfR13CStateManager [0x0000DD7C].
- **`fn_30_D94C`** — 9C0 slot 3 · `auto_00_00000000_text.s` L15636 · [0x0000D94C] · 708 B (0x2c4) — Think override: Think__6CActorFfR13CStateManager [0x0000D9B0], SetDrawShadow__6CActorFb [0x0000D9DC], Think__10CPatternedFfR13CStateManager [0x0000DA38], Update__22CCollisionActorManagerFfR13CStateManagerQ222CCollisionActorManager14EUpdateOptions [0x0000DA68], Update__19CLineOfSightTrackerFfR13CStateManager [0x0000DA8C], Update__23CSurfaceAlignmentHelperFR13CPhysicsActorR13CStateManagerf [0x0000DAB0], GetObjectById__13CStateManagerCF9TUniqueId [0x0000DB00].
- **`fn_30_DDA0`** — 9C0 slot 4 · `auto_00_00000000_text.s` L15932 · [0x0000DDA0] · 1680 B (0x690) — AcceptScriptMsg override: AcceptScriptMsg__10CPatternedFR13CStateManagerRC10CScriptMsg [0x0000DE0C], then Activate__15CBodyControllerFR13CStateManagerQ23pas15EAnimationState [0x0000DF0C], __ct__6CTokenFRC6CToken [0x0000DF6C]/GetObj__6CTokenFv [0x0000DF74], FindConnectedObjects__7CEntityCFRC13CStateManager18EScriptObjectState20EScriptObjectMessage [0x0000E068], DeleteObjectRequest__13CStateManagerF9TUniqueId [0x0000E098], Destroy__22CCollisionActorManagerCFR13CStateManager [0x0000E0C0], SetActive__22CCollisionActorManagerFR13CStateManagerb [0x0000E150], SetBossParams__13CStateManagerF9TUniqueIdfUi [0x0000E1F0], SetParticleEmissionRateScalar__17CCollisionTrackerFf [0x0000E288], SetPhysicsActive__22CCollisionActorManagerFR13CStateManagerb [0x0000E2D4], IsObjectInHurtfulSafeZone__16CSafeZoneManagerCFRC6CActorRC13CStateManager [0x0000E3CC].
- **`fn_30_D2E0`** — 9C0 slot 7 · `auto_00_00000000_text.s` L15187 · [0x0000D2E0] · 164 B (0xa4) — PreRender override: calls PreRender__10CPatternedFR13CStateManager [0x0000D32C] only for selected states (word 0xae4(r3) / float 0x11bc), then ORs 0x80 into the halfword at 0x102(r31) [0x0000D36C].
- **`fn_30_D250`** — 9C0 slot 8 · `auto_00_00000000_text.s` L15139 · [0x0000D250] · 112 B (0x70) — AddToRenderer override: AddToRenderer__10CPatternedCFRC13CStateManager [0x0000D290] under the same state/float gating.
- **`fn_30_D384`** — 9C0 slot 9 · `auto_00_00000000_text.s` L15237 · [0x0000D384] · 372 B (0x174) — Render override: SetupWorldSpacePortalPlane__10CModelDataCFRC12CTransform4fRC6CPlane [0x0000D3B8], Render__10CPatternedFR13CStateManager [0x0000D418], GetCharacterRenderMaskAndTarget__13CStateManagerCFRUiRUi [0x0000D444], RenderSystemsToBeDrawnFirst/Last__10CPatternedFR13CStateManagerUiUi [0x0000D458]/[0x0000D46C], RenderSystemsToBeDrawnFirst/LastPOICheck__17CParticleDatabaseCFUiUi [0x0000D4C4]/[0x0000D4DC].
- **`fn_30_D2C0`** — 9C0 slot 11 · `auto_00_00000000_text.s` L15175 · [0x0000D2C0] · 32 B (0x20) — PreRenderAllViewports override: single call PreRenderAllViewports__10CPatternedFR13CStateManager [0x0000D2CC].
- **`fn_30_90`** — 9C0 slot 13 · `auto_00_00000000_text.s` L97 · [0x00000090] · 44 B (0x2c) — No `bl`: loads the vtable from 0(r3) [0x0000009C] and calls vtable word at +0x38 (obj word 14 = HealthInfo__3CAiFv) with mtctr/bctrl [0x000000A0]-[0x000000A8].
- **`fn_30_D230`** — 9C0 slot 14 · `auto_00_00000000_text.s` L15127 · [0x0000D230] · 32 B (0x20) — ImmuneVulnerabilty override: single call ImmuneVulnerabilty__20CDamageVulnerabilityFv [0x0000D23C].
- **`fn_30_CC34`** — 9C0 slot 16 · `auto_00_00000000_text.s` L14693 · [0x0000CC34] · 220 B (0xdc) — No DOL calls: gates on the state word 0xae4(r4) [0x0000CC50] then calls local fn_30_CD78 [0x0000CC88] and fn_30_CD10 [0x0000CCA8].
- **`fn_30_CE50`** — 9C0 slot 17 · `auto_00_00000000_text.s` L14853 · [0x0000CE50] · 372 B (0x174) — Damage/touch handler: checks byte flag 0x11ee bit 26 [0x0000CE78], resolves the player via GetObjectById__13CStateManagerCF9TUniqueId [0x0000CECC] + TCastToPtr<7CPlayer> [0x0000CED0], then ApplyDamage__13CStateManagerF9TUniqueId9TUniqueId9TUniqueIdRC11CDamageInfoRC15CMaterialFilterRC9CVector3f [0x0000CF88] and Touch__10CPatternedFR6CActorR13CStateManager [0x0000CFA0].
- **`fn_30_CFC4`** — 9C0 slot 19 · `auto_00_00000000_text.s` L14957 · [0x0000CFC4] · 512 B (0x200) — Aim/orbit position: PredictMotion__13CPhysicsActorCFf [0x0000D060], local fn_30_D1C4 [0x0000D06C], GetLocatorTransform__9CAnimDataCF6CSegIdPC13CCharAnimTime [0x0000D0A0], __ml__12CTransform4fCFRC12CTransform4f [0x0000D0B0], __ct__12CTransform4fFRC12CTransform4f [0x0000D0BC], GetAimPosition__10CPatternedCFRC13CStateManagerf [0x0000D124].
- **`fn_30_D4F8`** — 9C0 slot 26 · `auto_00_00000000_text.s` L15341 · [0x0000D4F8] · 992 B (0x3e0) — Big switch on a selector (jumptable_30_data_B08, 0x26 entries) that toggles byte flags 0x11ef/0x11ec; arms an attack by building a CIngMiniPortalAttack (ctor [0x0000D834]) around a CIngMiniPortalInfo (ctor [0x0000D758]) allocated with __nw__FUlPCcPCc [0x0000D778] + AllocateUniqueId__13CStateManagerFv [0x0000D808], then AddObject__13CStateManagerFP7CEntity [0x0000D870] and DoUserAnimEvent__10CPatternedFR13CStateManagerRC13CInt32POINode14EUserEventTypef [0x0000D8B8].
- **`fn_30_C850`** — 9C0 slot 31 · `auto_00_00000000_text.s` L14412 · [0x0000C850] · 456 B (0x1c8) — CollidedWith override: CollidedWith__10CPatternedFRC9TUniqueIdRC18CCollisionInfoListR13CStateManager [0x0000C86C], then branches on the state word 0xae4(r30) [0x0000C870].
- **`fn_30_76F4`** — 9C0 slot 36 · `auto_00_00000000_text.s` L8558 · [0x000076F4] · 52 B (0x34) — Death override: forces the state to 2 when it is 7 [0x00007710], then Death__10CPatternedFR13CStateManagerRC9CVector3f18EScriptObjectState [0x00007714].
- **`fn_30_CB84`** — 9C0 slot 37 · `auto_00_00000000_text.s` L14641 · [0x0000CB84] · 176 B (0xb0) — KnockBack override: EnableAnimReaction__13CKnockBackMgrFQ213CKnockBackMgr13EAnimReactionb [0x0000CBD0], KnockBack__10CPatternedFR13CStateManagerRC14CKnockBackInfo [0x0000CC14].
- **`fn_30_CB54`** — 9C0 slot 39 · `auto_00_00000000_text.s` L14624 · [0x0000CB54] · 48 B (0x30) — No `bl`: reads the state word 0xae4(r3) [0x0000CB54]; for 0..1, 2..3 or >=5 stores skDamageHitTime__10CPatterned into 0x448(r3) [0x0000CB7C].
- **`fn_30_6C`** — 9C0 slot 40 · `auto_00_00000000_text.s` L80 · [0x0000006C] · 8 B (0x8) — Returns the constant 1 (`li r3, 1`) [0x0000006C].
- **`fn_30_10`** — 9C0 slot 41 · `auto_00_00000000_text.s` L21 · [0x00000010] · 8 B (0x8) — Returns the constant 1 (`li r3, 1`) [0x00000010].
- **`fn_30_C758`** — 9C0 slot 42 · `auto_00_00000000_text.s` L14342 · [0x0000C758] · 248 B (0xf8) — No `bl`: picks a float from 0x54/0x58(r3) against a rodata constant using the byte flag 0x420(r3) bit 25 [0x0000C760] and the r6 argument.
- **`fn_30_74`** — 9C0 slot 43 · `auto_00_00000000_text.s` L86 · [0x00000074] · 28 B (0x1c) — Copies three floats 0x54/0x58/0x5c(r4) into 0x0/0x4/0x8(r3) [0x00000078]-[0x00000088].
- **`fn_30_24`** — 9C0 slot 48 · `auto_00_00000000_text.s` L34 · [0x00000024] · 8 B (0x8) — Returns the byte at 0x44f(r3) [0x00000024].
- **`fn_30_2C`** — 9C0 slot 53 · `auto_00_00000000_text.s` L40 · [0x0000002C] · 8 B (0x8) — Returns 0 (`li r3, 0`) [0x0000002C].
- **`fn_30_0`** — 9C0 slot 54 · `auto_00_00000000_text.s` L9 · [0x00000000] · 8 B (0x8) — Returns this+0xAEC (`addi r3, r3, 0xaec`) [0x00000000].
- **`fn_30_8`** — 9C0 slot 55 · `auto_00_00000000_text.s` L15 · [0x00000008] · 8 B (0x8) — Returns this+0xBD8 (`addi r3, r3, 0xbd8`) [0x00000008].
- **`fn_30_CA18`** — 9C0 slot 57 · `auto_00_00000000_text.s` L14534 · [0x0000CA18] · 316 B (0x13c) — Fades the floats at 0x448 and 0x1080 of this against lbl_30_rodata_70; uses Black__6CColorFv [0x0000CAB4] and Lerp__6CColorFRC6CColorRC6CColorf [0x0000CACC].
- **`fn_30_C6CC`** — 9C0 slot 58 · `auto_00_00000000_text.s` L14303 · [0x0000C6CC] · 140 B (0x8c) — State-machine setup: SetupStateMachine__10CPatternedFR13CStateManager [0x0000C6E0], then three indirect calls through the state machine vtable words +0x18 [0x0000C6F4], +0x14 [0x0000C714] and +0x1c [0x0000C734] with lbl_30_data_250 (count 0x31), lbl_30_data_6B0 (0x1c) and lbl_30_data_8DC (0x9).
- **`fn_30_34`** — 9C0 slot 59 · `auto_00_00000000_text.s` L46 · [0x00000034] · 8 B (0x8) — Returns 0 (`li r3, 0`) [0x00000034].
- **`fn_30_3C`** — 9C0 slot 65 · `auto_00_00000000_text.s` L52 · [0x0000003C] · 16 B (0x10) — Stores the halfword kInvalidUniqueId into 0x0(r3) [0x00000044].
- **`fn_30_4C`** — 9C0 slot 66 · `auto_00_00000000_text.s` L60 · [0x0000004C] · 12 B (0xc) — Returns (byte 0x34c(r3) >> 1) & 0x0FFFFFFF (`extrwi r3, r0, 1, 28`) [0x00000050].
- **`fn_30_58`** — 9C0 slot 67 · `auto_00_00000000_text.s` L67 · [0x00000058] · 12 B (0xc) — Returns the float kDefaultGravityAccel [0x0000005C].
- **`fn_30_64`** — 9C0 slot 71 · `auto_00_00000000_text.s` L74 · [0x00000064] · 8 B (0x8) — Returns this+0x754 (`addi r3, r3, 0x754`) [0x00000064].
- **`fn_30_18`** — 9C0 slot 72 · `auto_00_00000000_text.s` L27 · [0x00000018] · 12 B (0xc) — Returns the float lbl_30_rodata_64 [0x0000001C].
- **`fn_30_11CD8`** — C44 slot 0 · `auto_00_00011CD8_text.s` L9 · [0x00011CD8] · 280 B (0x118) — Destructor: re-stores lbl_30_data_C44 into 0(r30) [0x00011D08], then fn_30_11DF0 [0x00011D0C], __dt__20CDamageVulnerabilityFv [0x00011D18], vtable slot-1 deletes of the objects at 0x564/0x568/0x56c [0x00011D40]/[0x00011D68]/[0x00011D90], fn_30_E4EC [0x00011D9C], __dt__17CCollidableSphereFv [0x00011DA8], fn_30_5E4 [0x00011DB4], __dt__13CPhysicsActorFv [0x00011DC0] and Free__7CMemoryFPCv [0x00011DD0].
- **`fn_30_1548C`** — C44 slot 3 · `auto_00_00011CD8_text.s` L3894 · [0x0001548C] · 1176 B (0x498) — Think override: fn_30_137C0 [0x000154F4], fn_30_13C78 [0x00015514], fn_30_152D4 [0x00015530], Magnitude__9CVector3fCFv [0x00015554], White__6CColorFv [0x000155C0], fn_30_12AB0 [0x0001561C], fn_30_13760 [0x00015624], GetObjectById__13CStateManagerCF9TUniqueId [0x000156B0], CanBeNormalized__9CVector3fCFv [0x00015718], Normalize__9CVector3fFv [0x00015728], GetAngleDiff__9CVector3fFRC9CVector3fRC9CVector3f [0x00015734], RotateZ__12CTransform4fRC9CRelAngle [0x000157C4], ApplyDamage__13CStateManagerF9TUniqueId9TUniqueId9TUniqueIdRC11CDamageInfoRC15CMaterialFilterRC9CVector3f [0x000158B8], fn_30_1282C [0x000158E8].
- **`fn_30_1466C`** — C44 slot 4 · `auto_00_00011CD8_text.s` L2936 · [0x0001466C] · 624 B (0x270) — AcceptScriptMsg override: AcceptScriptMsg__6CActorFR13CStateManagerRC10CScriptMsg [0x0001468C], then switches on the message id (0x5845454c [0x00014698], 0x58435254 [0x000147AC], 0x44435456 [0x000146C0]) using ObjectById__13CStateManagerCF9TUniqueId [0x000146EC], DeliverScriptMsg__13CStateManagerFRC10CScriptMsg [0x00014744], Range__9CRandom16Fii [0x00014788], Range__9CRandom16Fff [0x000147E0].
- **`fn_30_13E68`** — C44 slot 8 · `auto_00_00011CD8_text.s` L2379 · [0x00013E68] · 100 B (0x64) — AddToRenderer override: AddToRenderer__6CActorCFRC13CStateManager [0x00013E84]; if byte 0x5d8 bit 26 and IsActorVisible__13CStateManagerCFRC6CActor [0x00013E9C], EnsureRendered__6CActorCFRC13CStateManager [0x00013EB0].
- **`fn_30_141B8`** — C44 slot 9 · `auto_00_00011CD8_text.s` L2609 · [0x000141B8] · 260 B (0x104) — Render override: Render__13CPhysicsActorCFRC13CStateManager [0x000141D4], GetActiveVisor__12CPlayerStateCFRC13CStateManager [0x000141E0]; for visor 3 sets sEnableAlphaModulation__11CElementGen [0x00014204] and calls a virtual through gpRender's vtable word +0x128 [0x00014214].
- **`fn_30_13ECC`** — C44 slot 11 · `auto_00_00011CD8_text.s` L2409 · [0x00013ECC] · 748 B (0x2ec) — Bounds: pulls the bounds of the objects at 0x564/0x568/0x56c through their vtable word +0x70 [0x00013F0C], fn_30_CD78 [0x00013F20], AccumulateBounds__6CAABoxFRC9CVector3f [0x00013FAC], __ct__6CAABoxFRC9CVector3fRC9CVector3f [0x00014118], UpdatePortalSystemState__6CActorFR13CStateManager [0x00014194].
- **`fn_30_13C5C`** — C44 slot 12 · `auto_00_00011CD8_text.s` L2236 · [0x00013C5C] · 28 B (0x1c) — Returns this+0x5A0 when byte 0x5c0(r3) != 0, else 0 [0x00013C68]/[0x00013C70].
- **`fn_30_13C40`** — C44 slot 13 · `auto_00_00011CD8_text.s` L2224 · [0x00013C40] · 28 B (0x1c) — Byte-identical body to fn_30_13C5C: this+0x5A0 if byte 0x5c0(r3) != 0, else 0 [0x00013C4C]/[0x00013C54].
- **`fn_30_13B90`** — C44 slot 14 · `auto_00_00011CD8_text.s` L2168 · [0x00013B90] · 8 B (0x8) — Returns this+0x570 (the CDamageVulnerability member) [0x00013B90].
- **`fn_30_13B88`** — C44 slot 15 · `auto_00_00011CD8_text.s` L2162 · [0x00013B88] · 8 B (0x8) — Returns this+0x570 (the CDamageVulnerability member) [0x00013B88].
- **`fn_30_13BA4`** — C44 slot 16 · `auto_00_00011CD8_text.s` L2181 · [0x00013BA4] · 156 B (0x9c) — No `bl`: calls the state manager vtable word +0x7c [0x00013BC8] and the actor vtable word +0xc [0x00013BE4], sets byte 0x18(r30)=1 [0x00013BF4] and copies 7 words of the resulting transform into 0x0..0x14(r30) [0x00013C00]-[0x00013C24].
- **`fn_30_1464C`** — C44 slot 17 · `auto_00_00011CD8_text.s` L2924 · [0x0001464C] · 32 B (0x20) — Touch override: Touch__6CActorFR6CActorR13CStateManager [0x00014658].
- **`fn_30_11E44`** — C44 slot 29 · `auto_00_00011CD8_text.s` L113 · [0x00011E44] · 8 B (0x8) — Returns this+0x4D0 (the CCollisionPrimitive member) [0x00011E44].
- **`fn_30_142BC`** — C44 slot 31 · `auto_00_00011CD8_text.s` L2683 · [0x000142BC] · 912 B (0x390) — Physics/collision helper: initialises the static at lbl_30_bss_78 via __shl2i [0x00014310], GetObjectById__13CStateManagerCF9TUniqueId [0x00014414], fn_30_2F9C [0x000144E4], CanBeNormalized__9CVector3fCFv [0x00014524], Normalize__9CVector3fFv [0x00014534], Magnitude__9CVector3fCFv [0x0001454C], CollidedWith__13CPhysicsActorFRC9TUniqueIdRC18CCollisionInfoListR13CStateManager [0x00014634].

## 3. The class behind lbl_30_data_C44

- `lbl_30_data_C44` appears twice in the text: [0x00011CF8]/[0x00011D08] inside `fn_30_11CD8` (the destructor, which re-installs the vtable into 0(r30)) and [0x00015A80]/[0x00015A90] inside `fn_30_15924`, the constructor.
- Constructor: `fn_30_15924` — `auto_00_00011CD8_text.s` L4213, [0x00015924], size 1696 B (0x6A0), range 0x15924..0x15FC3. Vtable install: `lis r3, lbl_30_data_C44@ha` [0x00015A80], `addi r3, r3, lbl_30_data_C44@l` [0x00015A88], `stw r3, 0x0(r31)` [0x00015A90].
- Base constructor called: `__ct__13CPhysicsActorF9TUniqueIdRCQ24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>RC11CEntityInfoUiRC12CTransform4fRC10CModelDataRC13CMaterialListRC6CAABoxRC10SMoverDataRC16CActorParametersRC8StepData>` at [0x00015A7C] (the CPhysicsActor constructor; `this` = r31).
- Allocation: `fn_30_15924` contains **no** `__nw__FUlPCcPCc`; its caller `fn_30_3478` (`auto_00_00000000_text.s` L3871, [0x00003478]) does `li r3, 0x5e0` [0x0000361C] + `bl __nw__FUlPCcPCc` [0x00003624] => **0x5E0 = 1504 bytes**, then passes the pointer as `this` (`mr r3, r31` [0x000036C8]) into `bl fn_30_15924` [0x00003704].
- `fn_30_15924` also calls `fn_30_16238` [0x00015C70]/[0x00015C84]/[0x00015C98]; that helper allocates 0x338 [0x000162E8] and 0x220 [0x000163B0] itself, but those are spline/aux allocations, not the actor.
- For reference, the *main* actor is allocated 0x11F0 = 4592 bytes (`li r3, 0x11f0` [0x00000488], `bl __nw__FUlPCcPCc` [0x00000494]) and constructed by `fn_30_EC6C` [0x0000EC6C], which stores lbl_30_data_9C0 into 0x0(r31) [0x0000ECE8] after `__ct__10CPatternedF...` [0x0000ECD4].
