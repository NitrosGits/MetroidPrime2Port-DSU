# Ing REL (module 29) and MinorIng REL (module 44): analysis for host bodies

Analysis only (2026-10-16). Sources: `config/G2ME01/rels/{Ing,MinorIng}/symbols.txt` (sizes),
the archive build's disassembly `../MetroidPrime2Port/build/G2ME01/{Ing,MinorIng}/asm/`
(Ing: `auto_00_00000000_text.s` 0x0-0xF2C8, `auto_fn_29_F2C8_text.s`, `auto_00_0000FF88_text.s`;
MinorIng: `auto_00_00000110_text.s`, `auto_00_00009D58_text.s`, `auto_fn_44_ABCC_text.s`; plus
each module's data/rodata `.s`), the DOL symbol names they call, and the Wii build's RSO exports
(`../MetroidPrime3Port/orig/R3ME01/files/MP2/RSO/Production/selfile.sel`, `RSO_Ing.rso`,
`RSO_MinorIng.rso`, read with `strings`).
Confidence tags: **[proven]** = read from an instruction, a table entry or an exported name;
**[probable]** = strong evidence but not proven; **[?]** = guess, verify before relying on it.
`fn_29_X` / `fn_44_X` are REL `.text` offsets.

Areas needing **Ing** (16): 1baa96c2 15/31/39/57; 3bfa3eff 34/35/61; 3dfd2249 28;
42b935e4 7/25/28/36/38/50/56; 863fcd72 11.
Areas needing **MinorIng** (14): 1baa96c2 18/31; 3bfa3eff 13; 3dfd2249 28/60;
42b935e4 12/13/20/25/30/31/53/56; 863fcd72 11.

---

## A. Ing.rel (module 29)

### A.1 Classes

| class | GC size | vtable | ctor / dtor | evidence |
|---|---|---|---|---|
| `CIng : CPatterned` | 0xe60 | `lbl_29_data_A24` (82 words) | `fn_29_D540` (0x1110) / `fn_29_CF64` | **[proven]** name: Wii export `TypesMatch__4CIngCFi`; size: loader `rs_new(0xe60)`; vtable stored by the ctor |
| `CIngData` (plain struct) | 0x214 | - | `fn_29_EE68` (0x158) / `fn_29_10F8`; copy `fn_29_E7C8` | **[proven]** ctor body + loader stack layout. Name **[?]** (by analogy with `CMediumIngData`) |
| `CIngMiniPortalAttack : CActor` | 0x238 | `lbl_29_data_CE4` | `fn_29_11E8C` (0x190, named in symbols.txt) / `fn_29_114B0` | **[proven]** symbol name; size from `rs_new(0x238)` in `fn_29_C228` |
| `CIngMiniPortalInfo` | 0x88 | - | `fn_29_12100` (named) / copy `fn_29_1201C` | **[proven]** symbol name, Wii ctor signature `(TUniqueId, f, f, TLockedToken<CGenDescription>, us, f, f, CDamageInfo, SLdrPlasmaBeamInfo)`; size and members from the two ctors (A.3) |
| `CIngExitHostEffect : CActor` | 0x5e8 | `lbl_29_data_C60` | `fn_29_1105C` (0x3DC) / `fn_29_FF88` | class at `data_C60` holds a `CPathFindSearch` + 2 tokens and is `rs_new(0x5e8)`'d in the ExitHost state `9D68` **[proven]**; that it *is* `CIngExitHostEffect` (Wii export ctor `(TUniqueId, CEntityInfo, CTransform4f, CVector3f, CVector3f, uint, uint, f, f, TUniqueId, f, f, us)`) is **[probable]** |
| sub-structs of CIngData (body projectile 0x40, ing spot = `CIngSpotData` 0x60, grapple, mini-portal, swarm 0x2A) | see A.3 | - | builders `fn_29_F12C`, DOL `CIngSpotData` ctor 0x80218920, `fn_29_F090`, `fn_29_F000`, `fn_29_EFC0` | **[proven]** layouts; names **[?]** |

`lbl_29_data_A20` (= 0x20): meaning unknown **[?]**. `lbl_29_data_CE0` (= 0x23) is the
`CMaterialList(kMT_Projectile)` the mini-portal ctor passes [proven from `fn_29_11E8C`].

### A.2 Function map (functions >= 0x100, plus the module head)

**Module head [proven]**: `RELMain` (0xE0) calls `fn_29_100`, which stores loader `fn_29_130`
into `lbl_29_bss_6C` (the 4-byte `SIng_FuncPtrs`) and calls `SetSIng_FuncPtrs` (DOL 0x80218918);
`RELExit` (0xBC) calls the setter with null. Static init `fn_29_F2C8` (0xCC0) fills the FSM tables.

| fn | size | role | tag |
|---|---|---|---|
| `130` | 0xFC8 | loader (`LoadIng`), see A.4 | proven |
| `1294` | - | optional<CModelData> move used by the loader | proven |
| `10F8` | - | CIngData dtor | proven |
| `D540` | 0x1110 | CIng ctor | proven |
| `CF64` | 0x1C4 | CIng dtor (vtable slot) | proven |
| `C228` | 0x3B8 | `DoUserAnimEvent`: jump table `data_B6C`, spawns `CIngMiniPortalAttack` | proven |
| `C654` / `C814` / `C9A4` | 0x1C0 / 0x190 / 0x514 | CIng vtable overrides; `C814` looks like Think, `C9A4` (largest) like AcceptScriptMsg or Think | **[?]** - slots not mapped to CPatterned's vtable yet |
| `B208` | - | SetupStateMachine (registers the tables below) | proven |
| `E7C8` | 0x110 | CIngData copy ctor | proven |
| `E8D8`/`E944`/`E9E8`/`EA7C`/`EB38`/`EBEC`/`EC88` | <0x100 | copy ctors of swarm / mini-portal / beam info / grapple / ing-spot / body-proj / vulnerability | proven |
| `ED38` | 0x130 | **[?]** CIngData helper (sits between copies and ctor) | ? |
| `EE68` | 0x158 | CIngData ctor | proven |
| `EFC0`/`F000`/`F090`/`F12C` | <0x100 | swarm / mini-portal / grapple / body-proj builders | proven |
| `F2C8` | 0xCC0 | static init of the FSM tables | proven |
| `1F1C`, `2414`, `2B70`, `4F14`, `5CB0`, `11A7C` | 0x17C..0x358 | each allocates a 0x180 `CExplosion` (DOL `fn_800532D4`) | proven |
| `3190` | 0x23C | builds the 0x18 `CCollisionActorManager` | proven |
| `4830` | 0x130 | builds a 0x1e0 `CCollisionTracker` via GeomBlobV2's `fn_25_4290` | proven |
| `4B4C` | 0x13C | builds a 0x1b8 `CGameLight` | proven |
| `76B8` | 0x658 | state SuckEnergy; also allocates a 0x180 `CHUDBillboardEffect` | proven |
| `FF88` | - | CIngExitHostEffect? dtor | proven (class identity probable) |
| `1105C` | 0x3DC | CIngExitHostEffect? ctor | as above |
| `10154`, `1046C`, `1065C`, `1077C`, `10944`, `10F58` | 0x104..0x40C | CIngExitHostEffect? methods (lie between its dtor and ctor) | **[probable]** |
| `114B0` | - | CIngMiniPortalAttack dtor | proven |
| `11570` | 0x2D0 | CIngMiniPortalAttack aim helper: steers the aim point (0x214) at the target actor's touch bounds, speed = distance / `beamInfo.travelSpeed`, clamped at 1.2217 rad; calls the target's vtable 0x54 twice | proven |
| `11CA4` | 0x1E8 | CIngMiniPortalAttack Think (vtable 0x14): ages 0x220, deletes the fired projectile and itself at the lifetime (info+0x08), fires (`fn_29_11840`) between info+0x04 and the lifetime, then `CActor::Think` | proven |
| `11C14` | - | CIngMiniPortalAttack AcceptScriptMsg (vtable 0x18): `XDEL` deletes the projectile, `XCRT` calls `fn_29_11A7C` | proven |
| `11840` | 0x1A8 | CIngMiniPortalAttack fire: aims (`fn_29_11570`), `TLdrToBeamInfo` (DOL 0x80239B64, attributes 0x291), `rs_new(0x6a8)` `CPlasmaProjectile` at the portal's transform, then `AddObject` and the projectile's 0x12a = the portal's uid | proven |
| `11E8C` | 0x190 | CIngMiniPortalAttack ctor | proven |
| `12100` | - | CIngMiniPortalInfo ctor | proven |
| `121F4` | - | module static init: `lbl_29_bss_7C` = `string_l("Grenchler.rel")`, registered with dtor `fn_29_2144` | proven |
| `131C`..`3E1C`, `432C`, `4604`, `4960`, `B294`..`C0B4`, `D128` | 0x108..0x45C | CIng private helpers (unassigned) | **[?]** |

**State machine [proven by simulating `fn_29_F2C8`; 16-byte entries]**

- 49 triggers (`data_250`): StateOver B1F4, IsAlert B1E8, IsControllingHost B1C8, IsFacingTarget
  B0E0, HasTarget B0A4, HasNewTarget B04C, HasLineOfSight B01C, UnderFire B000, HeardShot AFE4,
  AreaClear AE68, CoverLeash ADE8, HasCoverPoint ADBC, HasWallCoverPoint AD7C, HasMiniPortals AD68,
  IsCorporeal AD54, IsIngSpot AD40, IsBodyProjectile AD2C, IsFrustrated AD14, IsAggressive ACEC,
  IsLuredBySafeZone AC98, StillLuredBySafeZone AC78, UseProjectileFSMEntry ACE0, TargetInSafeZone
  AC20, TargetInLightSafeZone ABC8, EmergePtInSafeZone AB7C, CanChangeForm AAF8, ShouldEvaporate
  AAA8, ShouldFleeSafeZone AA94, ShouldBecomeCorporeal AA4C, ShouldBecomeIngSpot AA04,
  ShouldArmSwipe A8CC, ShouldTaunt A8C0, ShouldBodyProjectile A780, ShouldOpenMiniPortal A664,
  ProjectileSplat A658, ProjectileHitTarget A4EC, FoundMovementPos A4E0, PathShagged A498,
  PathOver A450, PointPathOver A428, FoundPointPath A400, IsOffPath A3C8, HasAttackPattern A370,
  AttackPatternOver A340, TargetIsBall A2D8, InBallPursuitRange A224, HasPathToTarget A1C0,
  InGrappleRange A100, CanGrappleTarget A098.
- 29 states (`data_6BC`): Start A094, WaitForFSMTransition A090, ExitHost 9D68, BecomeCorporeal
  9A74, BecomeIngSpot 9750, BecomeBodyProjectile 94E8, BecomeWallProjectile 8FE8, Alert 8EF0,
  SafeZoneReaction 8CF4, Taunt 8BAC, Lurk 8A2C, PathFind 8808, IngSpotPathFind 8684,
  IngSpotPointPathFind 8570, Patrol 84A4, FollowAttackPattern 82D4, BodyProjectileFlight 7E30,
  SuckEnergy 76B8, SeekWallPoint 75C0, SelectTarget 7518, FaceTarget 7244, FindCoverPoint 6DA0,
  ArmSwipe 6A88, FailSafeMode 69A0, GrappleBall 6578, FindMiniPortals 61B4, MiniPortalAttack
  6008, Dead 5CB0, Evaporate 5C28.
- 12 code functions (`data_91C`, same `SCodeFunction` idiom as CMediumIng's `skCodeFuncs`):
  SetTargetDest 5B60, SetCoverDest 5AC4, SetRecoverDest 59D0, SetPointCoverDest 5820,
  SetWallPointCoverDest 5714, SetExitHostDest 54E8, SetLuredDest 53E0, SetLuredPointDest 51D0,
  StartRangedAttack 5190, EndRangedAttack 514C, SplatOntoMesh 4F14, ExpelGrappledBall 4D18.

### A.3 Member maps

**CIng (0xe60) [proven from ctor `D540` unless tagged]**

| off | member | init |
|---|---|---|
| 0x000 | CPatterned base | `EPatternedAI 0x18`, flavor 0, movement 0, collider 1, body 1 |
| 0x620 | (CPatterned's CAiKnockBackMgr) | `EnableKnockBackPhysics(false)` (DOL 0x801BD560) |
| 0x7c0 | CIngData (0x214) | copy of the loader's data |
| 0x9d4 / 0x9d8 | int / int | 2 / -1 |
| 0x9dc | CPathFindSearch | `(null, 0x301, pInfo+0xec, 1.0, 1.0)`; its +0xd0/+0xd4 (0xaac/0xab0) = model scale x rodata_98 / rodata_D0 (radius / height) |
| 0xac8 | CPathFindPointSearch | `(null)` |
| 0xc50 | float | 0 |
| 0xc58 | CSurfaceAlignmentHelper | default |
| 0xcb0 | CIngSpotPathFindNavigation (0x10) | default |
| 0xcc0 | CAABox | `mskNullBox`; 0xcd8 = 1 |
| 0xcdc / 0xcde | TUniqueId x2 | invalid |
| 0xce0, 0xcf0 | optional<TLockedToken<CGenDescription>> (0x10 each) | from data+0x1e4, +0x1f4 (swarm effects) |
| 0xd00 / 0xd04 | 0 / 0 | |
| 0xd08, 0xd18, 0xd28 | optional PART tokens | data+0xb0, +0xb4, +0xb8 (ing-spot hit/heavy/death effects) |
| 0xd38 | optional PART token | data+0x178 (mini-portal effect) |
| 0xd48 | optional PART token | data+0x8c (body-proj splat effect) |
| 0xd58 / 0xd5c / 0xd60 / 0xd64 | 0 / invalid uid / 0 / 0 | |
| 0xd68..0xd78 | TUniqueId x9 | invalid |
| 0xd7a..0xd83 | CSegId x10 | `GetLocatorSegId(name)`; names at `lbl_29_rodata_120`+{0x52e "head" [proven], 0x554 "L_elbow" [proven], 0x55c "R_elbow", 0x627/0x632/0x63d/0x647/0x653/0x65e/0x668 = "...collar_SDK", shoulder/forearm/wrist_end per side} **[probable]** (raw-byte decode; the L_/R_ prefixes of the arm strings were not separated cleanly) |
| 0xd88 | CLineOfSightTracker (DOL 0x801F5888) | |
| 0xd8a | CSegId | copy of 0xd7b |
| 0xdc8..0xde8 | CVector3f x3 | zero |
| 0xdec | int | -1 |
| 0xdf0 | CVector3f | normalised vector |
| 0xdfc | float | dot product with the transform |
| 0xe00 | | 0 |
| 0xe04..0xe24 | CVector3f[3] mini-portal points | `fn_29_61B4` stores up to three ray hits here (`0xe04 + 12*i`); not initialised in the ctor body [proven] |
| 0xe28 | int mini-portal event index | `fn_29_C228` uses it as `0xe04 + 12*i` and post-increments; `fn_29_6008` sets 0 on entry, -1 on exit [proven] |
| 0xe2c | int | 0 |
| 0xe30..0xe58 | floats | rodata constants |
| 0xe5c..0xe5e | bitfields | one bit from CIngData+0x210 |

Also in the ctor: `SetDrawShadow(false)`.

**CIngData (0x214) [proven from `fn_29_EE68`]**

| off | field |
|---|---|
| 0x00..0x14 | floats: hearingRadius, prop 0x5d0d.., prop 0xc620.., frustration, taunt, aggressiveness (arg order) |
| 0x18 / 0x1c | lightColor / lightAttenuation |
| 0x20 | armSwipe CDamageInfo (inline, to 0x3b) |
| 0x3c | triggerVuln CDamageVulnerability |
| 0x6c | body-projectile data (0x40): +0 CDamageInfo, +0x1c f, +0x20 splat asset, +0x24/+0x26 sounds (u16), +0x28..0x3c floats |
| 0xac | CIngSpotData (0x60) |
| 0x10c | grapple data: +0 f, +4 CDamageInfo, +0x20 f, +0x24/+0x26 sounds, +0x28..0x30 floats, +0x34 vuln |
| 0x170 | mini-portal data: +0 min, +4 max, +8 effect, +0xc sound, +0x10 CDamageInfo, +0x2c SLdrPlasmaBeamInfo |
| 0x1e4 | swarm / exit-host data: +0/+4/+8 assets, +0xc f, +0x10 asset, +0x14..0x1c floats, +0x20..0x28 five u16 sounds |
| 0x210 | 8 flag bits from `ingFlags` |

**CIngMiniPortalInfo (0x88) [proven from the ctor `fn_29_12100` and the copy ctor `fn_29_1201C`]**

| off | member | note |
|---|---|---|
| 0x00 | TUniqueId target | the player the portal aims at |
| 0x04 | float | read as a time at 0x15c by `fn_29_11CA4` (aim time) **[? name]** |
| 0x08 | float | read as a time at 0x160 by `fn_29_11CA4` (lifetime) **[? name]** |
| 0x0c | TLockedToken<CGenDescription> (0x0c) | CToken copy, the object word at token+0x08 copied from the source, then `Lock()` |
| 0x18 | ushort sound | |
| 0x1c / 0x20 | float x2 | **[? names]** |
| 0x24 | CDamageInfo (0x1c) | field by field: 5 words, 3 u16 (0x38/0x3a/0x3c), then the byte at 0x3e |
| 0x40 | SLdrPlasmaBeamInfo (0x48) | copied as 0x12 words by `fn_29_E9E8` |

**CIngMiniPortalAttack (0x238; CActor 0x158; vtable `lbl_29_data_CE4`) [proven from the ctor `fn_29_11E8C` and the dtor `fn_29_114B0`]**

| off | member | init |
|---|---|---|
| 0x000 | CActor | `(uid, name, info, 0, xf, CModelData(), CMaterialList(lbl_29_data_CE0 = 0x23 = kMT_Projectile), CActorParameters(), kInvalidUniqueId)`; the model temp is destroyed after |
| 0x158 | CIngMiniPortalInfo (0x88) | copy of the ctor's `portalInfo` |
| 0x1e0 | CProjectileInfo (0x28) | `(portalInfo+0x44 = beamInfo.weaponSystem, portalInfo+0x24 = the damage)` |
| 0x208 | CVector3f | the ctor's `scale` |
| 0x214 | CVector3f | `sZeroVector`; `fn_29_11CA4` looks at it from the actor's translation (0x54) |
| 0x220 | float | `lbl_29_rodata_7E0` = 0.0; `fn_29_11CA4` adds dt |
| 0x224 | TUniqueId | invalid; `fn_29_11CA4` looks it up and deletes it, `fn_29_11840` stores the fired projectile's id here |
| 0x228 | CRELFileToken | `CRELFileToken("Grenchler.rel", 0)`: the name is the static `rstl::string` at `lbl_29_bss_7C`, which `fn_29_121F4` (module static init, registered with `__register_global_object` and dtor `fn_29_2144`) fills with `string_l("Grenchler.rel")` [proven]; loadMode 0 queues the load |
| 0x230 | uchar | bit 7 cleared, the rest kept = read from uninitialised memory; `fn_29_11CA4` sets bit 7 once the portal has aimed |
| 0x12a | CActor's `mNextDrawNode`, not a derived member | the ctor's `owner` id, stored last [proven by CActor's member order: 0x13c mNonLoopingSounds, 0x134 mPvsIndex, 0x130 mAddedToken, 0x12c mDrawnToken, 0x12a mNextDrawNode] |

**vtable `lbl_29_data_CE4` [proven, `auto_04_00000000_data.s`]**: only four slots leave `CActor`'s
chain - 0x08 `fn_29_114B0` (dtor), 0x14 `fn_29_11CA4` (Think), 0x18 `fn_29_11C14`
(AcceptScriptMsg), and 0x3C `fn_29_90`, which just calls slot 0x38 (`HealthInfo`) and is exactly
the host's `CActor::GetHealthInfo`, so it needs no override. Everything else (PreThink, PreRender,
AddToRenderer, Render, Touch, DoUserAnimEvent, ...) is `CActor`'s. 0x00/0x04 are null.

Slots the portal calls on its *beam* (`CPlasmaProjectile`, DOL vtable `__vt__17CPlasmaProjectile`
0x803b2a98) [proven]: 0x8c `UpdateFx(xf, dt, mgr)`, 0x90 `ResetBeam(mgr, fullReset)`, 0x94
`Fire(xf, mgr, flag)`.

`CIngExitHostEffect` members: not mapped (out of scope for a first slice).

### A.4 Loader `fn_29_130` (0xFC8) [proven by simulation; `/tmp/ingan/ldr.py`]

- Reads **all 69** property ids of `include/MetroidPrime/ScriptLoader/SLdrIng.hpp`; stack layout
  follows the header field order (SLdrIng at sp+0x8c0: patterned 0x8fc, actor 0xb40, ingFlags
  0xbb8, facePlate 0xbbc, ..., lightColor 0xd20, lightAttenuation 0xd24, vulnerabilities
  0xd28/0xe84/0xfe0). Unknown ids are skipped with `ReadBytes(nullptr, size)`.
- `facePlateModel` (sp+0xbbc) is parsed but **never read afterwards** [proven for the loader; the
  ctor gets no copy].

Construction order:
1. `LdrToModelData(scale, kInvalidAssetId, patterned.animationInformation, true)` -> `fn_29_1294`
   into optional<CModelData> (sp+0x3a8); **returns null if empty** (valid byte +0x4c).
2. `LdrToDamageInfo(bodyProjectileContactDamage)` -> `fn_29_F12C` body-proj data (dmg, 0xa0d6,
   suckTime, splatEffect, speed, dropTime, min/max distance, sound_BodyProjectile,
   sound_SplatWall, odds).
3. `LdrToDamageVulnerability(ingSpotVuln)` -> `CIngSpotData` ctor (blobEffect, hitNormal,
   hitHeavy, death, maxSpeed, maxWallSpeed, ballPursuit, 0x5039, turnSpeed, vuln, 5 sounds).
4. `LdrToDamageVulnerability(grappleBallVuln)` + `LdrToDamageInfo(exitGrappleDamage)` ->
   `fn_29_F090` (0x67f6, spitForce, sound_ExitGrapple, sound_Grapple, maxHold, postWait,
   pursuitRange, vuln).
5. `LdrToDamageInfo(miniPortalProjectileDamage)` -> `fn_29_F000` (effect, min/max distance,
   sound, dmg, SLdrPlasmaBeamInfo).
6. `fn_29_EFC0` swarm data (pART, sRSC, pART_3da2, 0x2327, pART_081e, 0xcb39, 0x587c, 0x0bd7,
   sounds swarmMove, exitHost, exitHostSafeZone, insideHost, 0x148b).
7. `LdrToDamageVulnerability(triggerVuln)` + `LdrToDamageInfo(armSwipe)` -> `fn_29_EE68` CIngData
   (ingFlags, hearing, armSwipe, triggerVuln, lightColor, bodyProj, ingSpot, grapple, miniPortal,
   swarm, 0x5d0d, 0xc620, frustration, taunt, aggressiveness, lightAttenuation).
8. `rs_new(0xe60)`; `LdrToPatternedInfo(patterned, nullptr)`.
9. `CIng(uid, name=&editorProperties, LdrToEntityInfo, LoadEditorTransform, modelData,
   LoadActorParameters, patternedInfo, data)` = `fn_29_D540`.
10. `fn_29_10F8` destroys the CIngData temp.

### A.5 CIng behaviour port (`src/MetroidPrime/Enemies/PortCIng.cpp`, 2026-10-19)

Behaviour port, not a matching one. Retail functions are named in comments; `// @ fn_29_<off>`
marks only the 45 functions a run actually called (an instrumented build, run below). The other
44 that were read against retail carry `// fn_29_<off> (not yet exercised in a run)`.

- **Tables** (registered in `SetupStateMachine`): 49 triggers, 29 states and 12 code functions.
  The code functions take `(CStateManager&, float)` and run as FSM2 code nodes (`SetTargetDest`,
  `SetCoverDest`, `SetRecoverDest`, `SetPointCoverDest`, `SetWallPointCoverDest`,
  `SetExitHostDest`, `SetLuredDest`, `SetLuredPointDest`, `StartRangedAttack`,
  `EndRangedAttack`, `SplatOntoMesh`, `ExpelGrappledBall`).
- **Forms** (`mForm`): 2 corporeal, 3 ing spot, 4 body projectile, 5 evaporating. 0/1 (host) and
  6..9 (wall/portal) are [?].
- **FSM graph** (the disc's FSM2, dumped by `MP2_DEBUG_ING=1`):
  - `Start` -> HasAttackPattern / HasPatrolPath / IsCorporeal / IsIngSpot / IsLuredBySafeZone.
  - `BecomeCorporeal` -StateOver-> `Lurk` or `Patrol`.
  - `Lurk` -InDetectionRange|HeardShot|UnderFire-> `SelectTarget` -> sub-machine IngCorporeal
    (or IngPuddle in ing-spot form); ShouldEvaporate leads to `SafeZoneReaction` / `Evaporate`.
  - AttackPatternOver -> IsCorporeal -> `Lurk` -> IngPuddle; IsBodyProjectile ->
    `BodyProjectileFlight` -> IngCorporeal.
- **Minimal `[?]` bodies** (the state ends or the function returns at once):
  - States: Become* (BecomeCorporeal, BecomeIngSpot, BecomeBodyProjectile, BecomeWallProjectile:
    form switch only), ArmSwipe, GrappleBall, MiniPortalAttack, FindMiniPortals, SuckEnergy,
    BodyProjectileFlight and ExitHost.
  - Code functions: SetExitHostDest, SplatOntoMesh and ExpelGrappledBall.
- **Other `[?]` values:**
  - Values borrowed from MinorIng (`fn_44_9D58`): the line-of-sight eye segment and times
    (0.1/0.05), the surface-alignment mode (1), the team roles and the touch box.
  - Retail details not read: the loop-sfx choice and parameters, steering speed ranges, the
    script messages `SelectTarget` sends, the burn effects in `Dead`, the contact damage in
    `Touch`, `Listen`'s noise filter, the source of the state timer (`mStateTime`, host) and
    `SetRecoverDest` (taken as the closest path point within 20).
- **Mini-portal classes** (`src/MetroidPrime/Enemies/PortCIngMiniPortalAttack.cpp`, 2026-10-20):
  `CIngMiniPortalInfo`'s ctor `fn_29_12100` and copy ctor `fn_29_1201C`, and
  `CIngMiniPortalAttack`'s ctor `fn_29_11E8C` (member maps in A.1/A.3). The dtor `fn_29_114B0` is
  the implicit destructor, and `fn_29_11490` is inlined as `CModelData()`. Nothing exercises them
  yet, so no `// @` markers. The token at 0x228 is retail's `CRELFileToken("Grenchler.rel", 0)`.
  Behaviour is ported too (2026-10-20): Think `fn_29_11CA4`, AcceptScriptMsg `fn_29_11C14`
  (`kSM_Create` -> open, `kSM_Delete` -> drop the beam), open `fn_29_11A7C` (`CExplosion`
  "Ing Mini Portal Fx" + 3D emitter + fire), fire `fn_29_11840` (one `CPlasmaProjectile`
  "Ing Mini Portal Beam" from `TLdrToBeamInfo(beamInfo, 0x291)`, material `kMT_ProjectilePassthrough`,
  `EWeaponType` from the damage's weapon mode) and aim `fn_29_11570` (blend of two `GetAimPosition`
  samples at t = 0 and t = distance / `travelSpeed`, clamped to 70 deg off `GetForward()`).
  `fn_29_90` is not needed (slot 0x3C is `CActor::GetHealthInfo`). Still no `// @`: nothing has run
  them. Exercise them by spawning a portal - `CStateManager::AddObject` delivers `kSM_Create`, so
  any spawn runs open + fire; the Jump Guardian (`Agon 02_sand_sandfalls_dark[L2]`) is the easiest
  spawner, since `CIngSpaceJumpGuardian`'s `kUE_Projectile` case is already upstream code.
- **Mini-portal states and spawn** (`PortCIng.cpp`, 2026-10-20): `FindMiniPortals` `fn_29_61B4`
  (two rays per update, from 3 model-scales in front of the head locator, rotated by
  `{180, 0, 135, 45, -135, -45, 90, -90}` around the model scale, to `mTargetPos`; a file-static
  angle index is shared by every Ing, as retail's REL bss is; the out-of-bounds read of the last
  pair is guarded), `MiniPortalAttack` `fn_29_6008` (`CBCProjectileAttackCmd(kS_One, translation +
  forward, false)` once `CanIssueCommand(kAS_ProjectileAttack)`), and `DoUserAnimEvent`'s
  `kUE_Projectile` case `fn_29_C228` (one portal per event while `mMiniPortalEffect` is engaged and
  `mMiniPortalIndex < mMiniPortalCount`, damage = the block's copy scaled by dt with
  `SetNoImmunity(true)`, floats 1 / 2 / 150 / 1). The helper `fn_29_4468` is `TargetAimPosition`
  (already there). The spawn name (`lbl_29_rodata_120 + 0x573`) does not resolve in the rodata dump;
  the guardian's "Ing Mini Portal Attack" stands in **[?]**. Still no `// @`.
- **Grapple** (`PortCIng.cpp`, 2026-10-20): `GrappleBall fn_29_6578` and `ExpelGrappledBall
  fn_29_4D18` are ported (state time = `StateMachineState().GetTime()`, the morph-ball test is
  `GetSpawnedMorphballState() == kMS_Unmorphed && GetMorphballTransitionState() == kMS_Morphed`,
  damage = `dt * mUnknown67F6` with `kWT_AI` and no immunity, spit = `mass * mSpitForce` up the
  `Up()->forward` slerp of 20 deg, `mCurDamageRemTime = mDamageWaitTime`). `CSfxHandle mGrappleSfx`
  is 0xd04. **[?]** in it: the morph half of the 0x1144 flag pair has no CPlayer setter, and the
  damage word's two flag bits come from an uninitialised stack slot in the pseudo-C. No `// @`.
- **Verified at runtime** (Sanctuary 0i_cliff_hall_dark, MLVL `1baa96c2` MREA `2735236b`, layer 0):
  - The area places two INGS objects (`00390081`, `00390089`). They load **inactive** with
    detection range 0, so a run must `send <uid> Activate` over `MP2_CONSOLE`, and the beam
    (HeardShot) is what wakes them.
  - Observed: Start -> BecomeCorporeal -> Lurk -> SelectTarget -> FaceTarget -> Alert ->
    SetTargetDest -> PathFind -> BecomeIngSpot -> (form 3) Start -> Lurk -> FindCoverPoint.
  - No `[port] caught`, no `[reach-stub]`, exit 0. Driver: `/tmp/opencode/ing_run2.sh` with
    `ing_con.py`: activate, `tp 58.4 356 9`, then `press a 4` every 2 s.
  - Portal exercise attempt (2026-10-20): `Agon Wastes/02_sand_sandfalls_dark` (MLVL `42b935e4`,
    area 25) does place the Jump Guardian (`u051f`, editor `001900e9`, layer 2, inactive,
    hp 1100/1100). `send Activate` works (`obj` then reports it active) but two runs that teleported
    the player next to it and held fire for 120 s did **not** damage it (hp unchanged, screenshots
    black), so no portal ever spawned.
  - Damage, resolved (2026-10-20, `ing_light6.sh`): a light-beam shot **does** damage a corporeal
    Ing - hp 300 -> 180 over six shots at 6 units - and the relay is visible in the log (XHIT from
    each collision actor, XDMG = `kSM_Damage` when one is damaged). The three earlier runs failed
    for two independent reasons, neither of them aiming alone:
    1. `fn_29_3014` calls `GetDamageVulnerability__10CPatternedCFv` **directly**, not through the
       vtable, so retail's collision actors get CAi's own table. The port copied the virtual
       override, which in corporeal form is `mTriggerVulnerability` - pass-through for every beam -
       so `CGameProjectile::CanCollideWith` dropped the Ing before any geometry test. Fixed in
       `PortCIng.cpp` (`SetupCollisionActors`); this was the whole "nothing ever lands" bug.
    2. The **power beam cannot hurt an Ing at all** (retail): the Ing reports
       `CWeaponTypeVulnerability::kE_PassThrough`, and the shot flies through to the wall behind.
       Use the light beam, and `give lightBeam` alone is not enough - the light beam consumes light
       ammo, so `give lightAmmo 50` is required or the gun fires nothing (0 projectiles vs 3/3 on
       the power beam in the same run).
    3. Aiming is still unsolved for low targets: the C-stick does not turn the view in a console
       run (retail's free look needs the look button held, and `PortInput::Hold` replaces the whole
       held state, so a `press` cancels a `cstick`), so every shot so far is level
       (`dir (0.0, 1.0, -0.008)` in the new `[proj]` log). Level shots pass over a puddle, so the
       ing-spot form is still unhit; the `damage <id> <n>` console command does work on it
       (300 -> 295), which proves the `kSM_Damage` branch itself.
  - `MP2_DEBUG_BEAM=1` (2026-10-20) logs each projectile's ray test and every near-list verdict
    (`CGameProjectile::RayCollisionCheckWithWorld`), plus `MP2_DEBUG_ING=1` logs the damage
    messages that reach the Ing. The player's beams are `CEnergyProjectile` (`CGunWeapon.cpp:403`),
    not `CBeamProjectile`.

### A.6 Next slices (handoff, 2026-10-20)

Branch `decomp/ing`, no merge into `port`. Gates per slice: `ninja -C build-port-link
metroid_prime2_port` -> `FAILED: 0`; `python3 tools/gen_files_cmake.py --check` rc 0; a run in area
57 (`bash /tmp/opencode/ing_run2.sh`, 0 `[port] caught` / `[reach-stub]`). One `worker` lane at a
time; it may only touch `src/MetroidPrime/Enemies/PortCIng.cpp`, `include/MetroidPrime/Enemies/
CIng.hpp` (+ its own new `Port*.cpp`), the orchestrator updates this doc. No `// @` before a run
exercises the code.

Remaining `[?]` stubs in `PortCIng.cpp` (retail size, pseudo-C in `/tmp/opencode/pseudo/Ing/`,
disassembly `.../build/G2ME01/Ing/asm/auto_00_00000000_text.s`):

| slice | functions | note |
|---|---|---|
| ~~grapple~~ done | `fn_29_6578` GrappleBall + `fn_29_4D18` ExpelGrappledBall | ported 2026-10-20 (`d56f9f94b`), builds and runs but no run enters the state: it needs the player in morph ball inside `mPursuitRange`. Next run for it: `press b` to morph, then walk into the Ing |
| arm swipe | `fn_29_6A88` (0x318) + `fn_29_3E1C` (0x400, the two arm segment pairs `0xd7c..0xd7f` / `0xd80..0xd83`) + `fn_29_421C` (0x50, picks 0/1 and indexes `rodata_38`) | needs `CScriptTeamAiMgr::StartAttack/EndAttack`, `CBCMeleeAttackCmd`, `pas::kAS_MeleeAttack`; owns `0xe5c` bit 1 |
| suck | `fn_29_76B8` SuckEnergy (0x778) | allocates a 0x180 `CHUDBillboardEffect` |
| splat | `fn_29_4F14` SplatOntoMesh (0x238) | one of the twelve code functions |
| exit host | `fn_29_9D68` ExitHost (0x32C) + `fn_29_54E8` SetExitHostDest (0x22C) + the `CIngExitHostEffect` class (`fn_29_1105C` ctor 0x3DC, `fn_29_FF88` dtor, `10154`/`1046C`/`1065C`/`1077C`/`10944`/`10F58`; `lbl_29_data_C60`) | biggest; the class identity is still only **[probable]** |
| MinorIng | collision-actor damage path `fn_44_29D4` (never ran), inglet spawn in MassiveDeath `fn_44_7058`, post-`Dead` blob/delete | see §B.2 |

Marked by the 2026-10-20 damage run (`ing_light6.sh`): `@ fn_29_2E8C` (the `kSM_Damage` branch),
`@ fn_29_2684` (`CollisionActorDamage`) and `@ fn_29_2ED8` (`CollisionActorHitPlayer` - XHIT arrives
from every collider the shot touches). `fn_29_2B70` (ing-spot hit) and `fn_29_2414` (its effect)
still wait for a hit on a puddle.

**Blocker for the puddle:** nothing aims at a low target. Retail free look needs the look button
held and `PortInput::Hold` replaces the whole held state, so a console `press` cancels a `cstick`.
Fix it in the platform layer, which is not this session's file: add `PortMouse::SetAimAngles(yaw,
pitch)` (the aim state already carries both, `Synchronize` sets them from a direction) plus a
console command `aim <yaw_deg> <pitch_deg>`, enabled with twin-stick (`PortMouse::SetStickEnabled`)
so `CPlayer::UpdateMouseAim` is the one that turns the body. Then a run can aim down at a puddle and
earn the last two markers.

Handles/scripts that work today: `/tmp/opencode/ing_run2.sh` + `ing_con.py` (area 57, activates
both Ings, `tp 58.4 356 9`, holds fire), `ing_light6.sh`/`ing_light6_con.py` (the damage run: gives
`lightBeam` + `lightAmmo`, activates both, six light-beam shots at 6 units, then a `damage` control;
prints hp after each shot and the projectile count), `sjg_run.sh`/`sjg_con.py` (Jump Guardian in
Agon `02_sand_sandfalls_dark`), `sjg_diag.sh` (the same with screenshots). Each uses its own Xvfb
display and `MP2_CONSOLE` port; see the file header. **Always kill a run by pid** - a
`pkill -f metroid_prime2_port` also matches your own shell, and other sessions run the same binary
from their own worktree.

---

## B. MinorIng.rel (module 44)

- **Module head [proven]**: `RELMain` (0xC0) -> `fn_44_E0` stores loader `fn_44_110` into
  `lbl_44_bss_84` and calls `SetSMinorIng_FuncPtrs` (DOL 0x80218AA0).
- **Class**: `CMinorIng : CPatterned`, **not** derived from CIng [proven: its ctor `fn_44_9D58`
  (0x878) calls the `CPatterned` ctor directly with `EPatternedAI 0x4b`, flavor 0, movement 0,
  collider 1, body 1, then stores vtable `lbl_44_data_620` (0x148 = 82 words); name from Wii export
  `TypesMatch__9CMinorIngCFi`]. **GC size 0xd40** [proven: loader `rs_new(0xd40)`].
- Ctor members (headline only) [proven]: 0x7c0 CMinorIngData (0x158, copy `fn_44_A9DC`), 0x918
  `CProjectileInfo(data.projectile, data.projectileDamage)`, 0x960 `CPathFindSearch(null, 0x301,
  ...)`, 0xa4c `CPathFindPointSearch`, 0xbd8 `CSurfaceAlignmentHelper`, 0xc30
  `CIngSpotPathFindNavigation`, 0xc88 `CVisualKnowledge`, two `GetLocatorSegId` lookups, PAS
  anim-parm queries, `EnableKnockBackPhysics` + `SetAnimReactionRange(2,2)`, 0xce0 a 2-entry
  container (`fn_44_A5D0`) **[?]**, 0xd30 a pointer from `fn_44_AB78`.
- Other classes: none found; module has 217 functions, one vtable [probable: only `data_620` is
  vtable-sized].
- **Loader `fn_44_110` (0x844) [proven]**: reads the 20 top-level ids of `SLdrMinorIng.hpp`
  (editorProperties, patterned, actorInformation, projectileDamage, projectile, 0xa03e450c,
  attackAngleLimit, lineOfSightHeightOffset, 0xd6c8eac2, 0x2a5449ba, hearingRadius, 7 bools incl.
  0x09207f51 (via r22) and allowPuddleLockOn, ingSpot via `fn_44_194C`, unknown_0x3da35851 via
  `fn_44_1CE0`; the sub-structs' ids are parsed in those helpers). Order: `LdrToModelData(scale,
  kInvalidAssetId, anim, true)` -> optional, **null if empty**; `CIngSpotData` ctor from ingSpot;
  `fn_44_FE8` builds the bouncing-projectile struct (SLdrUnknownStruct34) at sp+0x1f8;
  `LdrToHealthInfo`/`LdrToDamageInfo` x2; `fn_44_9B4` (0x290) = CMinorIngData ctor at sp+0x2f8;
  `rs_new(0xd40)`; `LdrToPatternedInfo(patterned, nullptr)`; ctor `fn_44_9D58(uid, name, info, xf,
  modelData, patternedInfo, actorParams, data)`.
- **FSM [proven names/functions by simulating `fn_44_ABCC`]**: 34 triggers (`data_1A8`), e.g.
  PathShagged 6184, ShouldFire 5F60, PuddleHitByWeapon 5D64, StunOver 3E00, IsInCorporealForm
  5D08, StateOver 2038; 20 more entries in `data_47C` (15) + `data_5A8` (5): PathFind 8128,
  PointPathFind 7F98, Stunned 85DC, ProjectileAttack 777C, Patrol 7740, Dead 6EB8,
  SetWallPointCoverDest 5264, SetCoverDest 4C9C, IntoPuddleForm 8460, IntoCorporealForm 82A0, Idle
  7960, FollowAttackPattern 6A40, MoveAwayFromSafeZone 6BD4, AlwaysPointPathFind 7D20,
  WaitForFSMTransition 6A3C, SelectTarget 8C1C, FindCoverPoint 4E84, SetPointCoverDest 4AC4,
  SetLuredDest 21A8, and one entry `22B0` whose name pointer is the non-string `rodata_450` **[?]**.
  Which entries are states vs code functions is **[?]** (the 15/5 split mixes Set*Dest names into
  the first table).

### B.2 MinorIng behaviour port (`src/MetroidPrime/Enemies/PortCMinorIng.cpp`, 2026-10-19)

- **State/code split [proven by the FSM2 resource]**: `data_47C` (15) are *states*, `data_5A8` (5) are
  *code functions*: SelectTarget 8C1C, FindCoverPoint 4E84, SetPointCoverDest 4AC4, SetLuredDest 21A8,
  SetLuredPointDest 22B0. SetCoverDest/SetWallPointCoverDest are activate-only states.
- **22B0 [proven]**: `rodata_450` holds the string `"SetLuredPointDest"` (the name pointer is an
  offset into .rodata, not a non-string).
- **FSM2, not AFSM [proven at runtime]**: MinorIng's patterned `stateMachine2` is a `CGenericFSM2`,
  so `CGenericFSM2State::SetCodeFunctions` installs and *runs* the code functions (the AFSM
  `TStateMachineState` no-op does not apply). `MP2_DEBUG_MINORING=1` dumps the graph once
  (`[minoring] fsm ...`) and prints each state change. Main loop: Start -> SelectTarget ->
  FoundMovementPos -> PathFind; PathFind: Delay(0.2) -> ShouldFire -> IntoCorporealForm ->
  AnimOver -> ProjectileAttack; Idle: Delay(1) -> ShouldPathFind -> PathFind; SelectTarget ->
  !FoundMovementPos -> Idle; PuddleHitByWeapon -> Stunned; StunOver -> CancelStun/StateOver ->
  IntoPuddleForm.
- **vtable `data_620` slots overridden by the module** (offset -> fn -> CPatterned-chain name,
  `vtable-offsets.md`): 0x8 97E4 dtor; 0x10 8FCC PreThink; 0x14 8E5C Think; 0x18 9150
  AcceptScriptMsg; 0x24 8E10 PreRender; 0x28 8DF0 AddToRenderer; 0x2C 8E30 Render; 0x3C 70
  GetHealthInfo; 0x40 8DBC GetDamageVulnerability; 0x48 41AC GetTouchBounds; 0x4C 424C Touch; 0x54
  6FA0 GetAimPosition; 0x70 6384 DoUserAnimEvent; 0x80 4050 GetPrimitiveTransform; 0x98 76CC Death;
  0x9C 8D2C KnockBack; 0xA4 18 TakeDamage; 0xA8 68 CanBeShot; 0xAC 10 IsListening; 0xB0 38A8 Listen;
  0xB4 59E8 GetOrigin; 0xC8 28; 0xD4 7058 MassiveDeath; 0xDC 5AB8 ProjectileInfo; 0xE0 0
  GetSearchPath; 0xE4 8 GetPointSearchPath; 0xF0 9758 SetupStateMachine; 0xF4 30 GetRagDoll; 0x10C
  38 GetAttackTarget; 0x110 48 IsOnGround; 0x114 54 GetGravityConstant; 0x124 60
  GetDeathExplosionParticle.
- **States -> retail function** (`// @` = behaviour checked): PathFind 8128, PointPathFind 7F98,
  Stunned 85DC, ProjectileAttack 777C, Patrol 7740, Dead 6EB8, IntoPuddleForm 8460,
  IntoCorporealForm 82A0, Idle 7960, WaitForFSMTransition 6A3C, SetWallPointCoverDest 5264,
  SetCoverDest 4C9C, FollowAttackPattern 6A40, MoveAwayFromSafeZone 6BD4, AlwaysPointPathFind 7D20:
  all `@`. All 34 triggers and the five code functions (8C1C, 4E84, 4AC4, 21A8, 22B0) carry `@`.
- **Collision actor, touch bounds, blob effect, damage (all `@`)**: 4080/4050 primitive transform
  (lock-on locator, or +1.8 up for the move check); 319C creates the `CCollisionActor`
  (0.675/0.675/2.8 x scale, health copied, filter Solid minus CollisionActor/AIPassthrough/Player);
  305C/30F4 activate it for every form but the puddle and `MoveToWR` it to `lockon_target_SDK`;
  48F0/3004/458C the `CCollisionTracker` blob (size/density from the Into Corporeal/Into Stunned anim
  progress, rate scaled while evaporating); 3E8C/41AC puddle/body touch AABBs; 25E4 puddle hit
  (normal/heavy PART at damage >= 10 against the ing-spot vulnerability, sfx 4/5, 0.111 s timer);
  2CD8/29D4 the collision-actor relay (weapon / safe zone / radial-damage special function source,
  health difference applied, Death(DeathRattle), knockback). PreThink's +0x68 flag is
  `kMT_GroundCollider`. Dead (6EB8) waits for the blob's `GetParticleCount()` (CElementGen vtable
  0x74) to reach 0. The ctor lock-on check is against `CSegId::Invalid()` (255), not `Null()`.
- **[?] still**: the inglet spawn in MassiveDeath (7058: needs the inglet CHealthInfo at data+0xb0,
  CBouncyGrenadeData at +0xd0 and 0x8e0..0x8f8, none modelled by the host loader); fn_25_2554(0.5)
  taken to be `SetMinimumPathLength`; the blob colour taken to be `CPatterned::mColor` (0x44c);
  `UpdateHealthCause` mapped to `CStateManager::RecordDamageSource`. Visor checks read
  `mgr.GetPlayerState(0)` (retail `mgr+0x150c`); `mgr.GetPlayerState()` is null on the host, and
  `CBouncyGrenade.cpp:166/188` has the same latent bug.
- **Runtime (area 13, 2026-10-19, `MP2_DEBUG_MINORING=1`, GODMODE)**: 12 IntoCorporealForm ->
  ProjectileAttack cycles, 11 `projectile launched`, 5 `puddle hit, fx 1`, one `state Dead`;
  0 `[port] caught`, 0 `[reach-stub]`, frame limit reached at 133.1 s. No `body hit` line yet: the
  Dead came from a hit while corporeal that did not go through the collision actor.

---

## C. Reuse of in-tree code, and gaps

**Reusable now**
- `CIngSpotData` (`include/MetroidPrime/Enemies/CIngSpotData.hpp`, 0x60, DOL ctor) and
  `CIngSpotPathFindNavigation` (0x10): both modules embed them [proven].
- `CMediumIng` (`src/MetroidPrime/Enemies/CMediumIng.cpp:18-60,275-326`): template for the loader
  shape, the `skTriggers`/`skStates`/`skCodeFuncs` tables and `RELMain` [proven same idiom].
- `CIngSpaceJumpGuardian` (`CHECK_SIZEOF 0xa38`) and `CIngSpiderballGuardian` (0x998) are
  separate CPatterned classes, **not** CIng bases or subclasses [proven by their headers]; useful
  only as references for mini-portal / jump code. CIngSpaceJumpGuardian *calls* Ing.rel's
  `CIngMiniPortalAttack`/`CIngMiniPortalInfo` ctors, now real bodies in
  `src/MetroidPrime/Enemies/PortCIngMiniPortalAttack.cpp`.
- `CCollisionTracker` (`include/MetroidPrime/Enemies/CCollisionTracker.hpp`, 0x1e0, GeomBlobV2
  host body): Ing's `fn_29_4830` constructs one through GeomBlobV2 [proven]; the host CIng can
  call its ctor (`tools/scope_module.py` keeps symbols referenced outside a module global
  [probable, by its docstring]).
- `SLdrIng.hpp/.inc`, `SLdrMinorIng.hpp/.inc`; loader proxies `RelProxy_LoadIngs`
  (`src/MetroidPrime/Enemies/SIng_FuncPtrs.cpp`, 'INGS') and `RelProxy_LoadMinorIng`
  (`SMinorIng_FuncPtrs.cpp`, 'MNNG'), both null-guarded under TARGET_PC; `SIng_FuncPtrs` /
  `SMinorIng_FuncPtrs` in `include/MetroidPrime/ScriptLoaderRel.hpp:315,625`.

**Gaps (file : symbol)**
- `tools/modules_port.txt:13,16` - Ing / MinorIng map to `src/MetroidPrime/PortModuleStubs.cpp`.
- `include/MetroidPrime/CEntityInfo.hpp` - no `kET_Ing` (116) / `kET_MinorIng` (147).
- `src/MetroidPrime/TypesMatch.cpp:324,355,589,642` - placeholder comments for both.
- No `include/MetroidPrime/Enemies/CIng.hpp` / `CMinorIng.hpp`; no CIngData / CMinorIngData.
- No `CIngExitHostEffect` declaration anywhere.

---

## D. Implementation order

1. **MinorIng first, as a passive actor** (smallest milestone, 14 areas): `CMinorIng.hpp`
   (0xd40 GC, `sizeof` on host), `CMinorIngData`, loader `fn_44_110`, ctor members of B, empty FSM
   (Idle only), `kET_MinorIng` + `TypesMatch`, a `PortCMinorIng.cpp` line in
   `tools/modules_port.txt`. Done when an area such as 3bfa3eff 13 loads with the actor present
   and no fault.
2. **MinorIng FSM**: the 34 triggers + 20 states/code functions, starting with Idle / Patrol /
   PathFind / ProjectileAttack / Dead.
3. **CIng passive**: `CIngData` + sub-structs, loader `fn_29_130`, ctor `fn_29_D540`, Start +
   WaitForFSMTransition + Dead only; `kET_Ing`; registration. Done when 3bfa3eff 34 loads.
4. **CIngMiniPortalInfo / CIngMiniPortalAttack**: the ctors `11E8C`, `12100`, `1201C` are ported
   (2026-10-20) and the reach stubs are gone; left are the actor's own methods (`11CA4` Think,
   `11570`, `11840` fire), which also completes the Space Jump Guardian's mini portals.
5. **CIng FSM** in groups: movement (PathFind, Patrol, IngSpotPathFind...), form changes
   (BecomeCorporeal / IngSpot / BodyProjectile, `CCollisionTracker`), attacks (ArmSwipe,
   GrappleBall, MiniPortalAttack, SuckEnergy), then ExitHost with `CIngExitHostEffect`.

## Open questions (verify first)

1. `data_C60` (0x5e8, ctor `1105C`) is `CIngExitHostEffect` - compare its ctor's parameter use
   with the Wii export signature.
2. `facePlateModel` is unused by Ing's loader and ctor - confirm no later read via `SLdrIng`.
3. Ing's dependency on GeomBlobV2's `CCollisionTracker` (`fn_29_4830` -> `fn_25_4290`) and that
   the host module link keeps that symbol global.
4. CIng vtable slot roles (`C654`, `C814`, `C9A4`) and the arm segment-name strings.
5. MinorIng's state / code-function split and the `22B0` entry.
