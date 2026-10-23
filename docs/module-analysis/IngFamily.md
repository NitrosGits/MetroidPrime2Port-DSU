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
| `CIngExitHostEffect : CActor` | 0x5e8 | `lbl_29_data_C60` | `fn_29_1105C` (0x3DC) / `fn_29_FF88` | **[proven]**: class at `data_C60` holds a `CPathFindSearch` + 2 tokens and is `rs_new(0x5e8)`'d in the ExitHost state `9D68`; the DOL's own mangled ctor `__ct__18CIngExitHostEffect...` (`config/R3ME01/symbols.txt:3184`) takes the same 13 arguments as `fn_29_1105C` |
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
| `FF88` | - | CIngExitHostEffect dtor | proven |
| `1105C` | 0x3DC | CIngExitHostEffect ctor | proven |
| `1006C` | 0x4C | stop the swarm-move emitter (`CSfxManager::RemoveEmitter` on 0x5dc) | proven |
| `100B8` | 0x9C | the swarm-move emitter: `AddEmitter` (0x5e0, volume 0x7f, looped), else `UpdateEmitter` along `GetForward()` | proven |
| `10154` | 0x318 | build the next bezier leg: path search at 0x188, the four control points at 0x2cc/0x2d8/0x2e4/0x2f0, then the length and the 1-Range(0,0.5) speed factor | proven |
| `1046C` | 0x1F0 | bezier length as the four quarter chords | proven |
| `1065C` | 0x120 | the random offset Range(0,4)*Up + Range(-2,2)*Right | proven |
| `1077C` | 0x1C8 | the flight step: `t = mX28c / (mX2fc/mX300*mX290)`, rebuild at `t >= 1`, arrival `±dt*mX290` sets 0x5e2 bit 7 | proven |
| `10944` | 0x40C | the two particle systems: aim blend while 0x29c runs down, the 60-slot trail ring at 0x308 (write index 0x5d8), effect A at the swarm and B `mX2c4`*60 frames behind, both gens' bounds accumulated into CActor's 0xcc/0xe4 | proven |
| `10DEC` | 0x78 | AddToRenderer (`gpRender->AddParticleGen` B then A) | proven |
| `10E64` | 0x50 | Render (B then A) | proven |
| `10EB4` | 0xA4 | AcceptScriptMsg (`CEntity`'s handler): `XDEL` stops the emitter, `XALD` sets the path search's area | proven |
| `10F58` | 0x104 | Think (vtable 0x14): returns unless the actor is active; Fly + particles + emitter; once 0x5e2 bit 7 is set both generators go to rate 0 and the actor deletes itself when both have no particles left; does not chain to `CActor::Think` | proven |
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
| 0x9d0 | uint (CIngData+0x210) | the flag word: its most significant byte is the retail's flag byte, which fn_29_EE68 fills with the ingFlags' low byte bit-reversed (the eight rlwimi's at asm 0x0000EF4C-0x0000EFA8 put the ingFlags' bits 0..7 into the byte's bits 7..0), so a byte mask m is the word mask (m << 24) |
| 0x9d4 / 0x9d8 | int / int | 2 / -1 |
| 0x9dc | CPathFindSearch | `(null, 0x301, pInfo+0xec, 1.0, 1.0)`; its +0xd0/+0xd4 (0xaac/0xab0) = model scale x rodata_98 / rodata_D0 (radius / height) |
| 0xac8 | CPathFindPointSearch | `(null)` |
| 0xc50 | float | 0 |
| 0xc58 | CSurfaceAlignmentHelper | default |
| 0xcb0 | CIngSpotPathFindNavigation (0x10) | default |
| 0xcc0 | CAABox | `mskNullBox`; 0xcd8 = 1 |
| 0xcdc / 0xcde | TUniqueId x2 | invalid |
| 0xce0, 0xcf0 | optional<TLockedToken<CGenDescription>> (0x10 each) | from data+0x1e4, +0x1f4 (swarm effects); held as `mSwarmEffect` / `mSwarmEffect2` |
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

- 0xcf0 (the second swarm GEN token, `CIngData` spark +0x10 mPART081E, engaged byte 0xcfc) is held by
  the host as `mSwarmEffect2` - it was in the "not held" list before the exit host.
- `IsControllingHost` (`fn_29_B1C8`) is `mHostId (0xcdc) != kInvalidUniqueId` **[proven]**. The port
  had it as a form test until 2026-10-20, which is what kept ExitHost unreachable.
- `fn_29_1F1C` is the landing smoke: a `CExplosion` "IngExitHostSmokeFx" on the 0xcf0 token, sound =
  `CIngData` swarm +0x28 mSound.

**CIngData (0x214) [proven from `fn_29_EE68`]**

| off | field |
|---|---|
| 0x00..0x14 | floats: hearingRadius, prop 0x5d0d.., prop 0xc620.., frustration, taunt, aggressiveness (arg order) |
| 0x18 / 0x1c | lightColor / lightAttenuation |
| 0x20 | armSwipe CDamageInfo (inline, to 0x3b) |
| 0x3c | triggerVuln CDamageVulnerability |
| 0x6c | body-projectile data (0x40): +0 CDamageInfo, +0x1c f (0xa0d63374, the suck drain a second), +0x20 splat asset, +0x24/+0x26 sounds (u16), then +0x28 **speed**, +0x2c **dropTime**, +0x30 **minAttackDist**, +0x34 **maxAttackDist**, +0x38 **suckTime**, +0x3c **odds** [proven 2026-10-20: the loader's parse switch sends 0xce980820->sp+0xc70, 0xb752c177->0xc74, 0xe6ddb662->0xc78, 0xb57bae86->0xc7c, 0xdfea60a2->0xc68, 0xdc741fbd->0xc88, and `fn_29_F12C` stores f3->+0x28, f4->+0x2c, f5->+0x30, f6->+0x34, f2->+0x38, f7->+0x3c] |
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

**`CIngExitHostEffect` members [proven]**: ported header
`include/MetroidPrime/Enemies/CIngExitHostEffect.hpp` (retail 0x5e8; the host class is larger, nothing
outside the REL reads it). Offsets that matter: 0x158 `CTransform4f` (the ctor's copy), 0x188
`CPathFindSearch` (null area, flags 3, index 0, 1.0/1.0, `kRP_Center`), 0x274 destination, 0x280 model
scale, 0x28c bezier time, 0x290 speed, 0x294 target id, 0x298/0x29c/0x2a0 the ctor's f3/f3/f4, 0x2a4
and 0x2b4 the two `CToken`s (asset A = `CIngData` swarm +0x04 mSRSC, B = +0x08 mPART3DA2), 0x2b0/0x2c0
the two `CParticleGen*` (deleted by the dtor), 0x2c4 the ctor's f1, 0x2c8 bezier count and 0x2cc the
four control points, 0x2fc length, 0x300 speed factor, 0x304 the 60-slot trail container (the host's
`CVector3f[60]` sits at 0x308), 0x5d8 write index, 0x5dc `CSfxHandle`, 0x5e0 sound, 0x5e2 bit 7
arrival.

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
marks only the 62 functions a run actually called. The 49 that were read against retail without a
run carry `// fn_29_<off> (not yet exercised in a run)`. (The exit host class is its own file,
`PortCIngExitHostEffect.cpp`: 12 more `// @`.)

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
- **Minimal/unexercised bodies** (the source still says `not yet exercised in a run`; the state ends
  or the function returns at once):
  - States (9): WaitForFSMTransition, BecomeBodyProjectile, BecomeWallProjectile, SafeZoneReaction,
    Taunt, Patrol, FollowAttackPattern, SeekWallPoint, Dead, Evaporate. `BodyProjectileFlight` left
    the list on 2026-10-20 (A.10), `GrappleBall` the same day (A.12), and `IngSpotPathFind`,
    `IngSpotPointPathFind` and `FailSafeMode` in A.13.
  - Code functions (6): SetWallPointCoverDest, SetLuredDest, SetLuredPointDest, StartRangedAttack,
    EndRangedAttack (`ExpelGrappledBall` A.12; `SetCoverDest`, `SetPointCoverDest` and
    `SetRecoverDest` A.13).
  - Not on either list: the exit host (state `9D68`, code `54E8` and the `CIngExitHostEffect` class) is
    ported and marked (A.9).
- **Other `[?]` values:**
  - Values borrowed from MinorIng (`fn_44_9D58`): the line-of-sight eye segment and times
    (0.1/0.05), the surface-alignment mode (1), the team roles and the touch box.
  - Retail details not read: steering speed ranges, the script messages `SelectTarget` sends, the
    burn effects in `Dead`, the contact damage in `Touch`, the source of the state timer
    (`mStateTime`, host) and `SetRecoverDest` (taken as the closest path point within 20). The
    loop-sfx choice and parameters and `Listen`'s noise filter were read and ported on 2026-10-20
    (A.19).
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
  the guardian's "Ing Mini Portal Attack" stands in **[?]**. `@ fn_29_61B4` (FindMiniPortals) and
  `@ fn_29_6008` (MiniPortalAttack) are marked as of the 2026-10-20 gate fix in A.7, which is what
  let the two states run at all; the portal spawn itself (`fn_29_C228`) still has no run.
- **Grapple** (`PortCIng.cpp`, 2026-10-20): `GrappleBall fn_29_6578` and `ExpelGrappledBall
  fn_29_4D18` are ported (state time = `StateMachineState().GetTime()`, the morph-ball test is
  `GetSpawnedMorphballState() == kMS_Unmorphed && GetMorphballTransitionState() == kMS_Morphed`,
  damage = `dt * mUnknown67F6` with `kWT_AI` and no immunity, spit = `mass * mSpitForce` up the
  `Up()->forward` slerp of 20 deg, `mCurDamageRemTime = mDamageWaitTime`). `CSfxHandle mGrappleSfx`
  is 0xd04. The 0x1144 flag pair is now both halves: `CPlayer::EnableEnterMorphBall` (TARGET_PC, bit
  6) joined `EnableLeaveMorphBall`, and retail sets the pair in `SuckEnergy` too (clears at
  0x00007A98/0x00007AB0, restores at 0x00007D74/0x00007D88). **[?]** left: the damage word's two
  flags. No `// @` yet - see A.10 for the gate that keeps the state unreachable.
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
- **Exit host reachable** (2026-10-20, area 57): SuckEnergy clears `mHostId` once the body
  projectile's suck time (5.0 s) runs out, and the FSM takes `!IsControllingHost` -> the
  `SetExitHostDest` code node -> `ExitHost`. The FSM2 dump now recurses into sub-machines
  (`DumpFsm2`, `MP2_DEBUG_ING`), which is what shows `fsm trigger !IsControllingHost(0):
  ->SetExitHostDest` and `fsm code SetExitHostDest: ->ExitHost`.

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
| ~~grapple~~ ported | `fn_29_6578` GrappleBall + `fn_29_4D18` ExpelGrappledBall | ported 2026-10-20 (`d56f9f94b`); the run that reaches it is the row below (A.12). Morphing is `press x` (`morphIntoBall` is function-list 17) and the player must stay inside the puddle's touch box |
| ~~arm swipe~~ done | `fn_29_6A88` (0x318) + `fn_29_3E1C` (0x400, the two arm segment pairs `0xd7c..0xd7f` / `0xd80..0xd83`) + `fn_29_421C` (0x50, picks 0/1 and indexes `rodata_38`) | ported + `// @` 2026-10-20 (`1b058f9a4`); see A.7 |
| ~~suck~~ done | `fn_29_76B8` SuckEnergy (0x778) | ported 2026-10-20, `// @`; see A.8. It also exposed a field-name shift in `CIngBodyProjectileData` (A.3) |
| ~~splat~~ done | `fn_29_4F14` SplatOntoMesh (0x238) | ported 2026-10-20, `// @` (probe-driven); see A.8 |
| ~~exit host~~ done | `fn_29_9D68` ExitHost (0x32C) + `fn_29_54E8` SetExitHostDest (0x22C) + the `CIngExitHostEffect` class (`fn_29_1105C` ctor 0x3DC, `fn_29_FF88` dtor, `1006C`/`100B8`/`10154`/`1046C`/`1065C`/`1077C`/`10944`/`10DEC`/`10EB4`/`10F58`; `lbl_29_data_C60`) | ported 2026-10-20 (`2382c8e15`), `// @` except `10E64` (Render, never queued - no model); class identity [proven]; see A.9 |
| ~~body projectile~~ done | `fn_29_7E30` BodyProjectileFlight (0x2E8) | ported + `// @` 2026-10-20 (`ing_bp3.log`); the audit also fixed the aim prediction (0.5f), the unconditional emitter calls and the 0x1144 bit-6 pair; see A.10 |
| ~~grapple~~ done | `fn_29_6578` GrappleBall + `fn_29_4D18` ExpelGrappledBall + the ball triggers `fn_29_A2D8`/`A224`/`A100` | ported + `// @` 2026-10-20; the run needs `layer 2`/`layer 3`, `press x` to morph and `tp` onto the puddle - see A.12 |
| ~~fail-safe~~ measured | `fn_29_69A0` FailSafeMode + `fn_29_A3C8` IsOffPath | the teleport works (5.38 units, once) and the `0233` Ing then sits on a point-graph node the ground search does not accept; the loop is retail's, not a port bug - see A.14 |
| ~~module fns~~ ported | `fn_29_3B50` (the 0xe40 body-state-5 timer), `fn_29_B784` (`UpdateHitDamageTime`), `fn_29_B294` + `fn_29_1DF4` (`Listen` and its safe-zone helper), `fn_29_2270` (the ing spot's looped sounds), `fn_29_C0B4` (`Render`) | ported 2026-10-20; the five that stay stand-ins and their sizes are in A.19 |
| ~~module fns 2~~ ported | `fn_29_37D8` (the separation steering), `fn_29_4960` (the light's blend and update), `fn_29_4B4C` (the actor's own light) | ported 2026-10-20, one commit each; only `fn_29_4830` (the blob) and `fn_29_16AC` (the loader's data block) stay stand-ins - A.19 |
| ~~minimal states~~ done | `fn_29_94E8` BecomeBodyProjectile, `fn_29_8FE8` BecomeWallProjectile, `fn_29_8CF4` SafeZoneReaction, `fn_29_8BAC` Taunt, `fn_29_84A4` Patrol, `fn_29_82D4` FollowAttackPattern, `fn_29_75C0` SeekWallPoint, `fn_29_5CB0` Dead, `fn_29_5C28` Evaporate | read against the asm 2026-10-20: the locomotion type is 0 (`kLT_Crouch`), `SafeZoneReaction`'s emitter was missing its volume and priority, `Patrol`'s speed range is walk/run, `FollowAttackPattern`'s ing-spot block runs on every message, `SeekWallPoint` no longer clears `mMoving` without a hint, `Dead` calls `CPatterned::Dead` and honours the two burn flags |
| MinorIng | collision-actor damage path `fn_44_29D4` (never ran), inglet spawn in MassiveDeath `fn_44_7058`, post-`Dead` blob/delete | see §B.2 |

Marked by the 2026-10-20 damage run (`ing_light6.sh`): `@ fn_29_2E8C` (the `kSM_Damage` branch),
`@ fn_29_2684` (`CollisionActorDamage`) and `@ fn_29_2ED8` (`CollisionActorHitPlayer` - XHIT arrives
from every collider the shot touches). `@ fn_29_2B70` (ing-spot hit) and `@ fn_29_2414` (its effect)
came from `ing_aim5.sh` the same day (below).

**Aiming at a low target (resolved 2026-10-20):** nothing could aim a shot down at a puddle - retail
free look wants the look button held and `PortInput::Hold` replaces the whole held state, so a
console `press` cancels a `cstick`, and every shot was level (`dir (0.0, 1.0, -0.008)`). The platform
layer now has `PortMouse::SetAimAngles(yawDeg, pitchDeg)`, reached by the console verb
`aim <yaw> <pitch>`: yaw reads as `face` does (0 = +y, 90 = +x) and a negative pitch looks down. It
places the aim outright (`sAim.yaw/pitch`, `initialized`) and arms mouse aim
(`PortMouse::SetEnabled(true)`), because that is the source `CFirstPersonCamera` takes the pitch
from - twin-stick alone only turns the body - and `CPlayer::UpdateMouseAim` then turns the body to
the same yaw. It sticks until something else owns the view (a lock-on, the morph ball, a cinematic),
which resynchronises the aim from the camera; re-issue it after such a gap.

**The puddle recipe** (`/tmp/opencode/ing_aim5.sh` + `ing_aim5_con.py`, area 57, display `:92`,
console 47306): `give lightBeam 1`, `give lightAmmo 50`, `beam light`, activate both Ings (they load
with detection range 0, so the AI only wakes on a heard shot: teleport 5 units off, `aim 0 -5`, fire
three), then wait - the ing-spot form is the end of the attack-pattern cycle and took 3-6 minutes in
every run so far. When `[ing] <tag> state … form 3` appears, map the log tag to the console id
(`0x400 | tag`, e.g. `0022` -> `u0422`), `tp` onto it, check the teleport with `status` (a `tp` into
a moving puddle silently leaves the player behind: re-`tp` until the `status` position is within
~2 units), `aim <yaw> -80` and fire. The muzzle starts inside the puddle's box, so the shot lands on
the first frame. Result: `[ing] 0022 spot hit heavy 1` + `[ing] 0022 hit fx: heavy explosion`, the
projectile log showing `actor 0022 at t 0.21`.

Handles/scripts that work today: `/tmp/opencode/ing_run2.sh` + `ing_con.py` (area 57, activates
both Ings, `tp 58.4 356 9`, holds fire), `ing_light6.sh`/`ing_light6_con.py` (the damage run: gives
`lightBeam` + `lightAmmo`, activates both, six light-beam shots at 6 units, then a `damage` control;
prints hp after each shot and the projectile count), `ing_aim5.sh`/`ing_aim5_con.py` (the puddle
run above), `sjg_run.sh`/`sjg_con.py` (Jump Guardian in Agon `02_sand_sandfalls_dark`), `sjg_diag.sh`
(the same with screenshots). Each uses its own Xvfb display and `MP2_CONSOLE` port; see the file
header. **Always kill a run by pid** - a `pkill -f metroid_prime2_port` also matches your own shell,
and other sessions run the same binary from their own worktree.

### A.7 The arm swipe (ported 2026-10-20)

`fn_29_6A88` `ArmSwipe`, `fn_29_3E1C` `ArmSwipeSegment` (one arm), `fn_29_421C` (the arm pick) and
`fn_29_3D74` (the box helper) are in `PortCIng.cpp` and marked; the first run that entered the state
(`ing_arm.sh`, area 57, `:98`, 18000 frames / 305 s) logged `[ing] 0022 state ArmSwipe form 2` and
`[ing] 0022 arm swipe hit: arm 1 (05..1a), target 0000, damage 20.0`, 0 `[port] caught`.

Facts read off the REL (the pseudo-C was wrong on two of them):

| item | value |
|---|---|
| anim locators, ctor `fn_29_D540` | `0xd7b` head, `0xd7a` `Collar_SDK` (nothing reads it), `0xd7c..0xd7f` `R_shoulder`/`R_elbow`/`R_forearm`/`R_wrist_end`, `0xd80..0xd83` the `L_` set. The pseudo stores the shoulders at CPatterned `0x35f`/`0x360`; the asm does not |
| `rodata_38` | `{1, 2}` = `pas::kS_One`/`kS_Two`, the `CBCMeleeAttackCmd` severity indexed by the arm |
| the `ApplyDamage` filter | include `kMT_Solid`, exclude empty, type 3 (`kFT_IncludeExclude`); `data_A10` = 0x3B is the material's bit number, not a mask |
| knock-back | `-GetRight()` for the right arm, `+GetRight()` for the left (the pseudo reads the transform's `m00/m10/m20`) |
| the four boxes | shoulder-elbow, elbow-forearm, forearm-wrist, then wrist extended 6 units along (wrist-forearm) normalised; `fn_29_3D74` pads z by ±3 and nothing else; ANY overlap deals damage |
| the arm pick | `mgr.Random()->Range(0, 100) >= 50` - the state manager's RNG at `+0x16e4`, not the actor's |

**A gate bug this found, in three triggers:** `fn_29_A8CC` (`ShouldArmSwipe`), `fn_29_A780`
(`ShouldBodyProjectile`) and `fn_29_A664` (`ShouldOpenMiniPortal`) all ask
`CScriptTeamAiMgr::CanStartAttack` only when `mTeamId` is not `kInvalidUniqueId` - the asm compares
the two and branches past the call (A8CC `0x0000A920`, A780 `0x0000A8A4`, A664 `0x0000A788`). The
port had all three gates unconditional, so an Ing the area places without a team could never enter
any of the states. Fixed for all three (the A8CC one came with the arm swipe).

That is what unblocked the mini portal: after the gate fix a run entered `FindMiniPortals` and then
`MiniPortalAttack` (form 2, 12 units off) - `ing_bp.log`, where `ing_light6.log`, `ing_aim1.log`,
`ing_aim5.log` and `ing_arm.log` (all before the fix) have neither state at all. `fn_29_61B4` and
`fn_29_6008` are marked on that run. No portal projectile appeared in it, so `fn_29_C228`'s
`kUE_Projectile` spawn still waits for a run that sees the animation event.

Still open in the slice: the run was `GODMODE=1`, so the damage reaching the player was not
observed (only the segment test and the `ApplyDamage` call); and `mSwipeArm` is exercised as 1
(both arms' tests run) but a `0` swing has not been logged.

### A.8 SuckEnergy (ported 2026-10-20)

`fn_29_76B8` (0x778), asm `auto_00_00000000_text.s:8447-8870`. The state possesses the *player*: the
Ing sets form 0 and rides the host, so it is the only state whose `mHostId` is a `CPlayer`.

| branch | retail behaviour |
|---|---|
| Activate (msg 0) | `mForm = 0`; `AnimationData()->GetParticleDB()`: `DestroyAllActiveParticles` + `ClearAllNonPersistentEffects`; `DeliverCmd(CBodyStateCmd(13))` = `kBSC_AbortScripted`; `RemoveMaterial` 0x21/0x25/0x3b/0x28/0x29; `mHostId = mTargetId` (0xcdc <- 0xd6c); if the swarm GEN token at 0xce0 is engaged (0xcec) allocate 0xcde and add a `CHUDBillboardEffect(*token, null, uid, true, "Ing Possessed Mold", GetNearClipDistance, GetScaleForPOV, 0, White, One, Zero, true)`; `AddEmitter(0x852, translation, 0x7f, area, true, false, med)` stored to 0xd00; `ApplyDamage(self, host, self, mData+0x6c contact damage, CMaterialFilter() default, GetForward())`; if the host is a `CPlayer`: `EnableLeaveMorphBall(false)`, `AttachActorToPlayer(self, false)`, and (when `IngAttachedWarningCount` is below its maximum) increment it and `DisplayHudMemo("IngAttachedWarning", CHUDMemoParms(5, 1, 0, 0, 1<<player, 1))`; finally `SfxStart(0x9ca, kMaxVolume, 0x40, area, true, false, med)` to 0xd00, overwriting the emitter handle (retail's dead store) |
| Update (msg 1) | if `StateMachineState().GetTime() > mData+0xa4` (body-projectile +0x38 = `suckTime`) drop `mHostId`; else look the host up, `SetTransform(host->GetTransform())`, `SetTranslation(host->GetAimPosition(mgr, 0))`, then `ApplyDamage(self, host, self, CDamageInfo(kWT_AI, dt * mData+0x88, 0, 0, noImmunity), MakeIncludeExclude(kMT_Solid, {}), GetForward())` — the include comes from `1 << lbl_29_data_0`, and `lbl_29_data_0` = 0x3B |
| Deactivate (msg 2) | `EnableLeaveMorphBall(true)`; detach when the attached actor is this Ing; `RemoveEmitter(0xd00)`, 0xd00 = 0, `mHostId` invalid. `mAnimationState` is never touched (§A.7's note about the FSM is why) |

Facts the asm settled, against the pseudo-C: the knock-back is the transform's **forward** column
(+0x04/+0x14/+0x24 of the matrix at 0x24 reads `m01/m11/m21`), not up; Activate *clears* CPlayer
0x1144 bits 7/6 (i.e. `EnableLeaveMorphBall(false)`, same as GrappleBall's Activate) and Deactivate
sets them; the HUD memo sits inside the `host is a player` branch; retail's damage weapon-mode word is
uninitialised stack with bits 0..18 cleared (m2c prints `& 0x1fff | 0xb0000`), so the port uses
`kWT_AI` like GrappleBall and says so.

Trace (`ing_suck.log`, area 57, `/tmp/opencode/ing_run2.sh`, 4500 frames / 80 s): the Ing enters the
state after the body projectile (`state SuckEnergy form 4`), and logs
`suck: host 0000 attached (mold 0216), 5.0s, 3.0 dmg/s` + `suck tick: riding host 0000 (mold 0216),
0.05 dmg/frame, host hp 99`; 0 `[port] caught`, 0 `[reach-stub]`, exit 0. **Not traced**: Deactivate
(the 5 s suck outlived the run) and the billboard/memo visuals.

The field-name shift this slice found: `CIngBodyProjectileData`'s six floats were named one property
early, so `ShouldBodyProjectile` compared against (maxAttackDist, suckTime) and `BodyProjectileFlight`
read `minAttackDist` as the drop time and `speed` as the suck time. Renaming them to the retail slot
order (A.3) fixed all three call sites with no edit to them.

`fn_29_4F14` SplatOntoMesh (0x238) is ported in the same file: form 3 (ing spot), `kBSC_AbortScripted`,
`CSurfaceAlignmentHelper::OrientToSurfaceNormal(*this, normal, 1.f)` (the normal is `{0xdd4,0xdd8,0xddc}`,
which nothing in the module ever writes — the ctor zeroes 0xdc8..0xde8 — so the call is a no-op), the
splat-wall sound (`CIngData+0x92`) with its handle discarded, and, when the body projectile's splat
token at 0xd48 is engaged (`0xd54`, from `CIngData+0x8c`), a `CExplosion` "Ing Splat Wall Fx" on the
Ing's transform at `GetModelData()->GetScale()` (retail reads the model data's first three words, and
`CModelData`'s first member is its `CVector3f mScale`), flags 0, `CColor::White()`, player -1.

The FSM in area 57 does **not** ask for it: three runs went `BodyProjectileFlight` -> `SuckEnergy`
without it, so the trace came from a local probe (`getenv("MP2_ING_PROBE_SPLAT")` calling
`SplatOntoMesh` once from the flight state's Activate; the patch is not committed). Trace
(`ing_splat.log`): `state BodyProjectileFlight form 8`, `state SplatOntoMesh form 8`,
`splat: splat fx explosion`, then `state Start form 3` — the explosion spawned and the form change
reached the machine; 0 `[port] caught`, 0 `[reach-stub]`, exit 0. The `MP2_ING_PROBE_SPLAT` hook is
not in the tree; re-add it locally to re-trace.

### A.9 Exit host (ported 2026-10-20)

`fn_29_9D68` ExitHost (0x32C), `fn_29_54E8` SetExitHostDest (0x22C), and the `CIngExitHostEffect`
class the state spawns. Pseudo-C: `/tmp/opencode/pseudo/Ing/{fn_29_9D68,fn_29_54E8,fn_29_1105C,
fn_29_FF88,fn_29_10154,fn_29_1046C,fn_29_1065C,fn_29_1077C,fn_29_10944,fn_29_10F58}.c`.

**ExitHost `fn_29_9D68`** (three branches on the msg, from the pseudo + asm):

| branch | retail |
|---|---|
| Activate | `AddMaterial(kMT_Target, mgr)`; `mForm = 1` (the other host form); delete `mPossessedEffectId` (0xcde) and `mD76Id` (0xd76) if valid; `mD76Id = mgr.AllocateUniqueId()`; `rs_new(0x5e8)` + `fn_29_1105C`; on a null return `mAnimationState = kAS_Over` else `AddObject` + `mAnimationState = kAS_Ready`; then `AddEmitter(fn_29_AC20(mgr, &data) ? mSoundExitHostSafeZone(swarm+0x24) : mSoundExitHost(swarm+0x22), translation, 0x7f, area, true, false, med)` |
| Update | look up 0xd76; when it is gone **or its byte 0x5e2 has bit 7 set** (the effect's own "done" flag, cleared in its ctor) `mAnimationState = kAS_Over`, else `SetTranslation(effect+0x54)` |
| Deactivate | `SetTranslation(mTargetPos 0xdc8)`; `mSurfaceAlignment.AlignToSurface(*this, mgr, mTargetPos, 1.f)`; `AddMaterial(kMT_Character, kMT_Solid, kMT_Orbit, mgr)`; `mBallCooldown(0xe48) = mData.mGrapple.mPostWaitTime(0x8f8)`; `fn_29_1F1C(this, mgr)` (an explosion spawner, same family as `fn_29_2414`); `mForm = 3` |

**Ctor call site** (`fn_29_9D68` @0x00009F3C..: the only `rs_new(0x5e8)`), which fixes the argument
order the Wii export only hinted at: `(uint2b, uid, entityInfo, xf, vecA=0xdc8, vecB=model scale,
assetA=swarm+0x04 mSRSC, assetB=swarm+0x08 mPART3DA2, u16 sound=swarm+0x20 mSoundSwarmMove)` plus four
floats taken from the swarm block: `f1 = +0x0c mUnknown2327, f2 = +0x14 mUnknownCB39, f3 = +0x18
mUnknown587C (stored twice), f4 = +0x1c mUnknown0BD7`, and a `TUniqueId*` at 0xd6c. In the ctor body
those floats land at 0x2c4, 0x290, 0x298/0x29c, 0x2a0.

**`CIngExitHostEffect` vtable `lbl_29_data_C60` [proven from the data dump]** — 32 words, 0x80; the
module's own slots are 0x08 `fn_29_FF88` (dtor), 0x14 `fn_29_10F58` (Think), 0x18 `fn_29_10EB4`
(AcceptScriptMsg), 0x28 `fn_29_10DEC` (AddToRenderer), 0x2C `fn_29_10E64` (Render) and the shared
0x3C `fn_29_90` thunk (it calls slot 0x38, exactly like the mini portal's); every other slot is the
base's own symbol, including 0x0C `CActor::TypesMatch` and 0x10 `CEntity::PreThink`.

**Ctor member map [proven from `fn_29_1105C`]** (the table with the asm offsets is A.18): CActor 0x158; 0x158 `CTransform4f` (copy of the
ctor's transform); 0x188 `CPathFindSearch` (0xec); 0x274 `CVector3f` (the state's 0xdc8), 0x280
`CVector3f` (the model scale), 0x28c int 0, 0x290 f2, 0x294 target `TUniqueId`, 0x298/0x29c f3, 0x2a0 f4,
0x2a4 `CToken` (asset A) + 0x2ac byte "the effect was built" (`ptr != nullptr`) + 0x2b0
`CParticleGen*` (the `fn_29_10D50` result), 0x2b4 `CToken` (asset B) + 0x2bc the same byte + 0x2c0
`CParticleGen*`, 0x2c4 f1, 0x2c8 0, 0x2fc 0, 0x300 1.0f, 0x304 the trail container's size (60, written
only by the copy `fn_29_11438` and never read: 0x308 holds 60 copies of the actor's translation), 0x5d8/0x5dc 0,
0x5e0 u16 sound, 0x5e2 bit 7 cleared. `fn_29_10D50(this, vecB, translation, token)` returns the
`CParticleGen*` stored at 0x2b0/0x2c0 and its sign is the byte at 0x2ac/0x2bc.

**Port status [proven]**: all three slices are ported - `PortCIngExitHostEffect.cpp` and
`CIng::ExitHost` / `SetExitHostDest` / `SpawnExitHostSmoke` in `PortCIng.cpp`. Every function carries
a `// @` except `fn_29_10E64` (Render), which never runs because the actor has no model and so is
never queued for RenderActorQueue - the generators draw through AddToRenderer. Verified log chain,
`/tmp/opencode/ing_run5.log`: `state SetExitHostDest` -> `state ExitHost` ->
`exit host: swarm 021d, sound 20d0` -> `exit host: smoke explosion` ->
`exit host: left at (78.0, 368.0, 4.9)`, with 0 `[port] caught` / `[reach-stub]` and exit 0.

### A.10 Body projectile flight (ported + `// @` 2026-10-20)

`fn_29_7E30` runs for the first time in `/tmp/opencode/ing_bp3.log` (area 57, the usual
`ing_bp3.sh`, 14000 frames): `0018 state BecomeBodyProjectile form 2` ->
`state BodyProjectileFlight form 8` -> `flight: form 8 ... at (58.5, 358.8, 5.0)` (two frames of the
drift branch) -> `flight: form 4 ... at (70.5, 357.9, 7.9)` (the anim's `kUE_BeginAction` sets form
4; 11.4 units in 0.33 s = the property's 35 units a second) -> `state SuckEnergy form 4` on the hit,
so Activate, both Update branches and Deactivate all ran. `ShouldBodyProjectile` needs the target
between `bodyProjectileMinAttackDist`/`MaxAttackDist` (10..45 on this prefab) with dz in
[-15, 3] and `mBodyProjectileAllowed` (set in `Lurk`'s Activate from `bodyProjectileOdds`, 70 %), so
standing ~6 units off and waiting is enough.

Three corrections the audit against the asm produced in the same slice:

| item | retail | port before |
|---|---|---|
| aim prediction | `fn_29_4468`'s f1 is a real argument: the flight passes `rodata_B4` = 0.5f (0x00008054), the other seven callers pass `rodata_6C` = 0.0f | helper hardcoded 0.f |
| flight `UpdateEmitter` / `RemoveEmitter` | unconditional (0x00008090, 0x0000828C..0x000082A0) | guarded by `if (mBodyProjectileSfx)` |
| 0x1144 pair | bit 7 *and* bit 6 (`mCanStartMorphTransition` too) are cleared in `SuckEnergy`/`GrappleBall`'s Activate and set in their exits | only bit 7 had a `CPlayer` setter; `EnableEnterMorphBall` added (TARGET_PC) |

`MP2_DEBUG_ING` now also prints, at load, the property block the probes need (flags, body-projectile
distances/odds/speed/drop/suck, grapple pursuit/hold/post-wait/spit/drain, mini-portal distances),
one line a second for the `ShouldBodyProjectile` / `TargetIsBall` / `InBallPursuitRange` /
`InGrappleRange` triggers, and the flight's form/time/direction.

### A.11 The cover scan was looking for the wrong hint types (`fn_29_6DA0`, fixed 2026-10-20)

The puddle (`mForm` 3) reaches `IngSpotPathFind` only through the `SetCoverDest` code node, i.e.
only after `HasCoverPoint`, which is `GetCoverHint(mgr, mCoverHintId) != nullptr`: the id is set by
`FindCoverPoint` `fn_29_6DA0` and by nothing else. With cover never resolving, every puddle sat in
`FindCoverPoint` (area 57) or looped `Lurk -> SetRecoverDest -> FailSafeMode` (Agon), and the grapple
behind `IngSpotPathFind` could not be reached at all.

`MP2_DEBUG_ING`'s scan counters named the bug on the first run (`ing_gr4.log`, Agon
`01_sand_entrance_dark`, 85 candidates, `types 3:64 5:21`, `type 64` rejected): retail filters
**types 3 and 4** (asm 0x00006F2C: `cmpwi 0x3` then `cmpwi 0x4`), and the port had guessed 4 and 5
from MinorIng's copy. The rewritten routine, from the asm:

| item | retail |
|---|---|
| activate-only, then `fn_29_4560` (release) and `fn_29_4468` (aim, f1 = 0.0) | same as the port had |
| candidates | a **file-static** array in the REL's bss (count + four `{TUniqueId, float}` entries), shared by every Ing, cleared per call |
| near/far score bounds | `f31 = max(bodyProj.minAttackDist, mMinAttackRange)²`, `f30 = max(bodyProj.maxAttackDist, miniPortal.maxAttackDist)²` (asm 0x00006E98/0x00006EB0, offsets 0x85c/0x3e0, 0x860/0x934) |
| filter | active bit, type 3 or 4, `!GetInUse(uid)`, same area, not `mLastCoverHintId` (0xd74), not in a safe zone |
| type-4 (wall) only | skipped when `mData.mFlags210` bit 5 (0x9d0 bit 5, "no body projectile") is set, or when `fn_29_ABC8` - `TargetInLightSafeZone`, the target sitting in a hurtful safe zone - holds; both evaluated once per call |
| score | `|mTargetPos - hint|² + 4·dz²`, the second term only when the type is not 4 or `dz > 0`; `+ f30` when the score is below `f31` |
| pick | the four lowest, insertion-sorted, then **one at random** through the state manager's RNG (`Range(0, count-1)`, mgr+0x16e4), then `fn_29_4510` (in use, `mCoverHintId` 0xd72, `mLastCoverHintId` 0xd74) |

Result (`ing_gr5.log`, same area): `023e cover scan: candidates 85, pick 0058` and the puddle walks
`FindCoverPoint -> SetCoverDest -> IngSpotPathFind -> SetPointCoverDest -> IngSpotPointPathFind`,
three states that had never run. The second Agon Ing (`0233`) still loops `Lurk -> SetRecoverDest ->
FailSafeMode`, i.e. it is off the navmesh (`IsOffPath`), which is a separate question (A.6 row).

**[?]** `0x3e0` is taken to be CPatterned's `mMinAttackRange` (the field `CPatterned` stores the
patterned-info minimum attack range in, and the one whose meaning matches a "too close" floor);
nothing in the module reads it elsewhere to confirm.

---

### A.12 The grapple (`fn_29_6578` + `fn_29_4D18`, marked 2026-10-20)

The chain that finally reaches it, traced in Agon Wastes `01_sand_entrance_dark` (`ing_gr7.log`,
`ing_gr8.log`; the script is `/tmp/opencode/ing_gr7.sh` + `ing_gr7_con.py`):

1. `0018`-style arena scripts do not transfer: the Ings here are `u063e`/`u0633`/`u04fd`; layers 2
   and 3 must be switched on with the console `layer 2` / `layer 3` before `objs Ing` lists them.
2. Wake them with a heard shot, then **morph into the ball with `press x`** - the `morphIntoBall`
   tweak is function-list 17, which is the X button (`Tweaks.inc`); `press b` never morphs, and the
   earlier recipe that suggested it silently left the player standing.
3. Sit **on** the puddle (`tp` to its own translation each few seconds). 1.2 units behind it is
   still outside the touch box: `grapple range: touchbox 1 morph 1 normalZ 1.00 overlap 0`.
4. Then `IngSpotPathFind` evaluates `CanGrappleTarget -> TargetIsBall -> InBallPursuitRange ->
   HasPathToTarget -> !TargetInSafeZone -> SetTargetDest -> IngSpotPathFind` (the ball one) and
   `InGrappleRange` flips to 1: `overlap 1 -> 1` -> `state GrappleBall form 3`.

**`GrappleBall` `fn_29_6578`** in the run: Activate attaches the player, clears both 0x1144 bits and
opens the `mSoundInsideHost` voice (0xd04); Update drags the Ing from 0.5 to 0.0 units off the ball
(`MoveOnSurface`, `mIngSpot.maxWallSpeed`), deals `dt * 0x8cc` (5.0 a second here) with `kWT_AI`, no
immunity, solid-only filter, and ramps `mHitReactionTime` by 0.30 a second to 1.00; at the property's
5 s hold time the animation state goes Over; Deactivate restores both bits, detaches, removes the
voice and sets `mBallCooldown` (0xe48) to `mPostWaitTime` (10 s).

**`ExpelGrappledBall` `fn_29_4D18`** then runs as the machine's code node: with the reaction timer at
1.00 it applies `mExitGrappleDamage` (0x8d0) once and resets `mCurDamageRemTime`, stops the ball,
launches it up the surface normal slerped 20 degrees toward the Ing's forward with
`mass * mSpitForce` = 6000.0 (30 x 200), `SetMoveState(kMS_ApplyJump)`, the exit voice, and clears
the reaction timer and `mMoving`.

Two trigger corrections came out of the same asm pass (`fn_29_A2D8` 0x0000A2E0, `fn_29_A100`
0x0000A144): both the ball tests read the player's **spawned** morph state (0x390) first and only
then the transition state (0x38c), so `TargetIsBall` and `InGrappleRange` now demand
`kMS_Unmorphed` + `kMS_Morphed`; the port had only the transition half.

```text
023e grapple range: touchbox 1 player 0000 morph 1 normalZ 1.00 overlap 1 flags 01 -> 1
023e state GrappleBall form 3
023e grapple: held 1.02/5.00 s, reaction 0.30, 5.0 dmg/s, ball 0.0 away
...
023e grapple: held 4.02/5.00 s, reaction 1.00, 5.0 dmg/s, ball 0.0 away
023e state ExpelGrappledBall form 3
023e expel: hit reaction 1.00, impulse 6000.0, dir (0.11, -0.32, 0.94)
```

Still open around this slice: `IngSpotPathFind` `fn_29_8684`, `IngSpotPointPathFind` `fn_29_8570`,
`FailSafeMode` `fn_29_69A0`, `SetCoverDest` `fn_29_5AC4` and `SetPointCoverDest` `fn_29_5820` all ran
in these runs but none has been read against its asm yet, so they keep `not yet exercised` until
that pass happens. The second Agon Ing (`0233`) still loops `Lurk -> SetRecoverDest -> FailSafeMode`
- it is off the navmesh (`IsOffPath`), and `SetRecoverDest`'s "closest point within 20" is itself a
`[?]`.

### A.13 The puddle's path states, read against the asm (2026-10-20)

A.12 left five functions that ran but had never been read against retail, plus `SetRecoverDest`'s
guessed body. All six are now asm-faithful and marked (`Ing` 81 -> 87/274):

| function | what was wrong |
|---|---|
| `IngSpotPathFind` `fn_29_8684` | the body ran unconditionally; retail returns before pathing unless the properties allow it (`0x9d0` bit 1, `kFlag_NoPathFind`) **and** a destination was found (`0xe5c` bit 3, `mFoundMovementPos`) - asm 0x000086DC. `mRecalcPath`/`mMoving` are stored on Activate only (0x000086BC). The speed was an else-if chain: retail takes the safe-zone modifier (0x888) or the maximum (0x87c) and then **overrides** with the ball pursuit speed (0x884) when `TargetIsBall` (0x00008778-0x00008894) |
| `IngSpotPointPathFind` `fn_29_8570` | same two guards and the same Activate stores (0x0000858C); its blend starts from the safe-zone modifier or the maximum, not always from the maximum (0x00008628) |
| `SetRecoverDest` `fn_29_59D0` | not the "closest point within 20 as the destination" it was guessed to be: two closest-point queries from the Ing's own position - filter `(20, 0, -1)` then `(20, 1, -1)` - and a point-graph search between the two results; it sets **no destination**, only `mFoundMovementPos` (0x000059E0-0x00005AA8) |
| `SetPointCoverDest` `fn_29_5820` | only points whose flags word has **bit 0** set count (`clrlwi r0, r0, 31`, 0x000058E0 - not bit 31); the "when the best is also the closest point take the previous one" fallback was invented and is gone (0x00005974 keeps only the last best) |
| `SetCoverDest` `fn_29_5AC4` | already correct - hint translation or the Ing's own position, `mFoundMovementPos` only with a hint, then `SetDestination` and face target `kInvalidUniqueId` at 0x718 |
| `FailSafeMode` `fn_29_69A0` | already correct - kAS_Ready, teleport to the closest physical point within 1000 (rodata_C4), `AlignNearPosition(..., 1.0)`, kAS_Over; Update does nothing |

The audit lane that proposed these had two wrong specifics, both rejected against the asm: it put
the `SetCoverDest` waypoint clear at 0xacc (that store is in `SetRecoverDest`, 0x00005A08) and read
the `SetPointCoverDest` point test as bit 31 (`clrlwi r0, r0, 31` is the low bit).

Verified after the change (`ing_gr11.log`, same area): the puddle still walks
`SetCoverDest -> IngSpotPathFind -> ... -> GrappleBall -> ExpelGrappledBall` twice, with 0
`[port] caught` / `[reach-stub]` and a clean exit. The second Agon Ing (`0233`) still spends the run
in `Lurk -> SetRecoverDest` without finding a connected point pair: it sits off the navmesh, and
`FailSafeMode`'s teleport either does not move it or lands it off-mesh again - that is the next
thing to measure (a position poll, not an asm question).

### A.14 The fail-safe teleport does move the Ing (2026-10-20)

A.13's open question - "does `FailSafeMode`'s teleport move it?" - is now measured with a position
poll (the probe in `CIng::FailSafeMode` and `CIng::Think`, `MP2_DEBUG_ING=1`). The answer: **yes,
once, and then the Ing stays where it was put**.

From `/tmp/opencode/ing_fs1.log` (Agon `01_sand_entrance_dark`, 0 `[port] caught` /
`[reach-stub]`, exit 0):

| what | measurement |
|---|---|
| the first fail-safe | `failsafe: points 470 res 0 idx 329 at (-358.1, -51.3, -14.4)` -> `moved 5.38 to (-361.9, -47.5, -13.7)` |
| every later cycle | `moved 0.00` - the Ing already stands on the point, and the search returns the same node 329 |
| the once-a-second poll | `0233 pos (-361.9, -47.5, -13.7) form 3 state Lurk` for the rest of the run |
| the other Ing | `023e` (u063e) never loops: it walks, becomes an ing spot and reaches `BodyProjectileFlight` |

So the fail-safe is not broken: it puts the Ing on the closest point-graph node of the area (470
nodes) and the Ing stays exactly there. The loop `Lurk -> SetRecoverDest -> FailSafeMode` is stable
because the two navigations disagree about that spot: `IsOffPath` is the *ground* search
(`CPathFindSearch::OnPath`, fn_29_A3C8, the search at 0x9dc) while `SetRecoverDest`'s two closest
point queries are the *point* graph (the ing spot's navigation, 0xac8). The node the fail-safe
picks is not on a walkable ground region, so `OnPath` keeps failing and the FSM goes round again.

Both ends of that are retail's: fn_29_69A0 and fn_29_A3C8 were read against the REL (A.13), and
`FindClosestPhysicalPoint` is the host's own (upstream decomp) implementation, unmodified. The
`0233` Ing in this area therefore sits off the ground mesh in retail too - the loop is what the
level asks for, not a port bug. What is left open is only why the Ing ends up off the mesh at all:
it walked there as a corporeal Ing and became an ing spot on that spot.

**The `SetRecoverDest` correction (2026-10-20, after the measurement).** The recover code's second
point query builds its filter with the *first* query's index as the connected point (the asm keeps
that index in `r5` from 0x00005A2C and passes it to the filter's constructor at 0x00005A50), so the
two points are always graph-related. The port passed -1, so the pair could be unrelated and the
graph search between them failed - the reason the off-mesh Ing never found a movement position.
Fixed (the filter is now `(20, 1, from)`), and the run confirms the loop is *not* caused by it: the
`0233` Ing's nearest point node (329) has no graph links at all, so the second query finds nothing
even with the corrected filter, and the fail-safe still loops. The point graph of that corner of
Agon has an isolated node, which is the level's data, not the port's.

### A.15 Which triggers a run exercises (2026-10-20)

The FSM dump (`[ing] fsm state X: ->T1 ->T2`, `[ing] fsm trigger T: ->targets`) in the log is
enough to say which of the 51 trigger entries a run evaluated: a state evaluates its own outgoing
triggers, and a trigger that fires leads to the next ones in its target list. `/tmp/opencode/
ing_triggers.py` walks that closure from the states the log shows as entered
(`[ing] <uid> state <Name> form <n>`).

From `ing_fs1.log` (25 states and code nodes entered): **43 of the 51 triggers were evaluated**.
The eight that were not, and so are still unexercised:

| trigger | retail | why it never fired |
|---|---|---|
| `FoundMovementPos` | fn_29_A4E0 | no state's outgoing list asks for it (the ing-spot path states read the flag themselves) |
| `AttackPatternOver` | fn_29_A340 | needs an attack pattern (kSS_Attack) on the Ing |
| `IsControllingHost` | fn_29_B1C8 | the Ing was never inside a host |
| `IsIngSpot` | fn_29_AD40 | the sub-machine's `Start` asks it, but the Ing left the sub-machine first |
| `IsBodyProjectile` | fn_29_AD2C | the body projectile's own sub-machine entry |
| `ShouldBecomeCorporeal` | fn_29_AA4C | the attack pattern never ran |
| `ShouldBecomeIngSpot` | fn_29_AA04 | the attack pattern never ran |
| `StillLuredBySafeZone` | fn_29_AC78 | no safe zone lured the Ing |

The 43 that did run are the ones the 2026-10-20 marking pass covers (the other 43 include the 32
already marked).

### A.16 The remaining minimal bodies, read against the asm (2026-10-20)

The last of the Ing's guessed bodies are gone. Three audit passes over the REL, each one reading
the asm and fixing what the port had wrong (all in `src/MetroidPrime/Enemies/PortCIng.cpp`):

| body | what the port had wrong |
|---|---|
| `BecomeBodyProjectile` `fn_29_94E8`, `BecomeWallProjectile` `fn_29_8FE8` | the locomotion type is **0** (`kLT_Crouch`, asm 0x9624/0x96F4/0x928C/0x93C4), not `kLT_Relaxed` |
| `SafeZoneReaction` `fn_29_8CF4` | the emitter was missing the volume (0x7f) and the priority, and the retail has no invalid-sfx test (asm 0x8D58); the knock-back's direction is the actor's forward and its severity 1 (asm 0x8D90) |
| `Taunt` `fn_29_8BAC` | already faithful (the taunt command 0x15, the flag bit 6 of 0xe5c) |
| `Patrol` `fn_29_84A4` | the speed range is not 1.0/1.0: the retail divides the body's walk speed by its run speed (asm 0x84F4-0x8508, `GetLocomotionSpeed(1)/GetLocomotionSpeed(2)`) and sets the blend mode 1 |
| `FollowAttackPattern` `fn_29_82D4` | the ing-spot move block runs on **every** message, not only on Update, and looks the waypoint up itself (asm 0x8310-0x8410) |
| `SeekWallPoint` `fn_29_75C0` | a missing hint leaves `mMoving` alone (asm 0x75F8) and the direction is normalised (asm 0x7674) |
| `Dead` `fn_29_5CB0` | the base class's death message was missing, the burn is skipped when either of the two burn flags is set (the byte at 0x421 bits 1 and 2 = the host's `mBurning`/`mLaggedBurnDeath`, asm 0x5EA0-0x5EAC), the damage cooldown (0x448) is set, the scale's z is clamped at zero and always written, and `mStartAsIngSpot` is cleared with it |
| `Evaporate` `fn_29_5C28` | already faithful (the form 5 and the two seconds) |
| `AreaClear` `fn_29_AE68` | the near box is the model scale times 10 **on each axis**, not the x scale three times (asm 0xAEB4-0xAEF4) |
| `HasMiniPortals` `fn_29_AD68` | the retail tests the count for non-zero (the neg/or/srwi-31 idiom), not `> 0` |
| `ProjectileHitTarget` `fn_29_A4EC` | the hit box is the model scale times 2.5 on each axis (asm 0xA5A0-0xA5F4) |
| `IsFrustrated` `fn_29_AD14`, `TargetInSafeZone` `fn_29_AC20`, `EmergePtInSafeZone` `fn_29_AB7C`, `ProjectileSplat` `fn_29_A658`, `PointPathOver` `fn_29_A428`, `FoundPointPath` `fn_29_A400`, `HasPathToTarget` `fn_29_A1C0`, `WaitForFSMTransition` `fn_29_A090` | already faithful (the REL's `fn_29_45BC` is the port's `GetCoverHint`; the ing-spot navigation's `IsPathOver`/`HasPath` are the DOL's `fn_801F81C8`/`fn_801F8178`) |
| `SetWallPointCoverDest` `fn_29_5714` | the second point query's filter takes the *first* query's index as its connected point (asm 0x5778) |
| `EndRangedAttack` `fn_29_514C` | retail's last argument to `EndAttack` is 0, so the Ing keeps its team role (asm 0x5154) |
| `StartRangedAttack` `fn_29_5190`, `SetLuredDest` `fn_29_53E0`, `SetLuredPointDest` `fn_29_51D0` | already faithful (the lure helper already does the search's flags 3 -> 1 -> 0x301 and keeps the Ing's own position when there is no mesh path) |
| `SetRecoverDest` `fn_29_59D0` | the second point query's filter takes the first query's index as its connected point (asm 0x5A2C) - see A.14 |
| `Listen` `fn_29_B294` + `fn_29_1DF4` | the host's body counted every noise but the path obstruction as a heard shot; retail answers only the type 0 (`kLNT_PlayerFire`), and the type 6 (`kLNT_SafeZone`) sets the lure zone (0xd78) unless the Ing already has a nearer one - see A.19 |
| `UpdateLoopSfx` `fn_29_2270` | the host's body added the two emitters but never moved them; retail updates each with the actor's position (0x54) and the transform's first column |
| `UpdateHitDamageTime` `fn_29_B784` | the host ran the base class's body; retail's clears the damage colour and the echo's damage when the timer reaches zero |
| `Render` `fn_29_C0B4` | the host's body lacked the portal plane, the form 9 flag and the particle-only paths - see A.19 |

The eleven trigger/state bodies above are marked (`// @ fn_29_XXXX`) on the strength of the
`ing_fs1.log` run, which evaluated them; the nine state bodies are marked only once a run has
exercised them (see A.17).

### A.17 The runs that marked the last states (2026-10-20)

Three runs in Agon Wastes `01_sand_entrance_dark` (the Ing recipe of A.12) with the audited
bodies, all with 0 `[port] caught` / `[reach-stub]` and a clean exit:

| run | what it added |
|---|---|
| `ing_fs1.log` | the fail-safe measurement (A.14); 43 of the machine's 51 triggers evaluated (A.15) |
| `ing_fs3.log` | `Taunt` (6) and `BecomeBodyProjectile` (1) -> `BodyProjectileFlight` (1) - both marked; `ArmSwipe` (12) |
| `ing_fs5.log` | `Dead` (3) -> marked: the console's `damage <uid> 100` three times takes the Ing's 300 HP to zero |

Two things the runs settled:

- **The fail-safe's node is nowhere near the ground mesh.** The probe's mesh diagnostics on the
  point the fail-safe picks read `onmesh 0 near1 0 near5 0 height 0.00`: `OnPath` finds no region
  there, `NearlyOnPath` finds none within 5 units either, and there is no mesh height to speak
  of. The point-graph node 329 is not a spot on the floor - it is an isolated node of the ing
  spot's own graph, which is also why the corrected `SetRecoverDest` (A.14) still finds no second
  point.
- **Killing an Ing takes the normal death path only under the x-damage threshold.** A `damage` of
  9999 (the first attempt) takes the HP to -9699 and `CPatterned::Death` reads that as a massive
  death, so the `Dead` state never runs. Three hits of 100 (the Ing has 300 HP) kill it the
  normal way, and the state runs.

The `Dead` state's **corporeal** branch (the burn, form 2) is still unexercised: every Ing in that
area is an ing spot by the time it can be killed. The same is true of `BecomeWallProjectile`,
`SafeZoneReaction`, `Patrol`, `FollowAttackPattern` and `SeekWallPoint` - they need scenarios
(the notes in A.15 and the "Next slices" below), not audits.

**Next slices** (the biggest unported pieces left in the module, by size):

1. ~~`CIngMiniPortalAttack`~~ - ported and marked (A.29).
2. The eight triggers no run has fired (A.15) - those need a scenario (a lure safe zone, an attack
   pattern, a host), not an audit.

### A.18 The exit host effect's member map, from the ctor's asm (2026-10-20)

`fn_29_1105C` (0x3DC, asm 0x1105C-0x11434) read against the REL: every store below is the REL's own
instruction, so the retail offset and the asm offset sit side by side. The class is
`include/MetroidPrime/Enemies/CIngExitHostEffect.hpp` + `src/MetroidPrime/Enemies/
PortCIngExitHostEffect.cpp`; retail's size is 0x5e8 (the CActor base is 0x158), the host's is larger
(a CPathFindSearch and two CTokens carry pointers), so the host allocates `sizeof` and the retail
offsets are comments only.

| retail | member | value | asm |
|---|---|---|---|
| 0x000 | the vtable `lbl_29_data_C60` | - | 0x11160 |
| 0x158 | `CTransform4f` | a copy of the ctor's transform (r25) | 0x11164 |
| 0x188 | `CPathFindSearch` (0xec) | area null, flags 3, index 0, radius 1.0f, height 1.0f (`rodata_7A8`), filter 0, region kRP_Center | 0x11168-0x11190 |
| 0x274 | `CVector3f` | the dest (r26) - CIng+0xdc8 | 0x111A4-0x111BC |
| 0x280 | `CVector3f` | the scale (r27) - the model's scale | 0x111C4-0x111DC |
| 0x28c | float | 0.0f (`rodata_798`) | 0x111DC |
| 0x290 | float | f2 - the ctor's x290 (CIngData swarm +0x14) | 0x111E0 |
| 0x294 | `TUniqueId` | the target id (the stack arg at 0x448, `lhz`/`sth`) | 0x111E4-0x111E8 |
| 0x298 | float | f3 - the ctor's x298 (swarm +0x18) | 0x111EC |
| 0x29c | float | f3 again | 0x111F0 |
| 0x2a0 | float | f4 - the ctor's x2a0 (swarm +0x1c) | 0x111F4 |
| 0x2a4 | `CToken` | assetA (r28) through the resource factory + the pool | 0x111F8-0x11238 |
| 0x2ac | byte | the effect A "created" flag: `(neg r3 \| r3) >> 31` | 0x1125C-0x11270 |
| 0x2b0 | `CParticleGen*` | fn_29_10D50(this, the scale, 0x54, token A) | 0x11248-0x11274 |
| 0x2b4 | `CToken` | assetB (r29) | 0x11278-0x11394 |
| 0x2bc | byte | the effect B "created" flag | 0x112DC-0x112F0 |
| 0x2c0 | `CParticleGen*` | fn_29_10D50(this, the scale, 0x54, token B) | 0x112C8-0x11300 |
| 0x2c4 | float | f1 - the ctor's x2c4 (swarm +0x0c) | 0x11308 |
| 0x2c8 | int | 0 - the bezier point count | 0x1130C |
| 0x2cc-0x2f8 | `CVector3f[4]` | **never written by the ctor** (no store between 0x2c8 and 0x2fc), so retail leaves them uninitialised; the host zeroes them | - |
| 0x2fc | float | 0.0f (`rodata_798`) | 0x11310 |
| 0x300 | float | 1.0f (`rodata_7A8`) | 0x11314 |
| 0x304 | int | the trail container's size: 0, then 60 (0x3c) by fn_29_11438 | 0x11318, 0x11424-0x114FC |
| 0x308 | `CVector3f[60]` | 60 copies of the actor's translation (0x54/0x58/0x5c), built in a temporary and copied in | 0x11348-0x113F0, 0x113F8 |
| 0x5d8 | int | 0 - the trail write index | 0x1131C |
| 0x5dc | `CSfxHandle` | 0 - null | 0x11320 |
| 0x5e0 | `ushort` | the ctor's sound (the stack arg at 0x44e) - CIngData swarm +0x20 | 0x11324 |
| 0x5e2 | byte | bit 7 cleared: `rlwimi r0, r6, 7, 24, 24` with r6 = 0 clears bit 24 of the word = 1u<<(31-24) = 0x80 of the byte; the other bits keep retail's uninitialised memory, the host zeroes them | 0x11328-0x11330 |

The ctor's arguments (asm 0x11090-0x110C8): r3 = this, r4 = the uid pointer, r5 = the CEntityInfo, r6 =
the transform, r7 = the dest vector, r8 = the scale vector, r9 = assetA, r10 = assetB, the stack at
0x448 = the target id pointer and 0x44e = the sound; f1 = x2c4, f2 = x290, f3 = x298, f4 = x2a0. The
caller (fn_29_9D68 at 0x9F48-0x9F54) is the module's only `rs_new(0x5e8)` and fills them in that
order: the swarm block at CIng+0x9a8 (assetA), +0x9ac (assetB), +0x9b0 (f1), +0x9b8 (f2), +0x9bc
(f3), +0x9c0 (f4), +0x9c4 (the sound), the dest CIng+0xdc8, the target CIng+0xd6c and the model's
scale (CIng+0x60). The port's caller (`CIng::ExitHost`, `src/MetroidPrime/Enemies/PortCIng.cpp`)
passes exactly those.

**The dtor `fn_29_FF88`** (0xDC, asm 0xFF88-0x10140): the vtable is reset to `lbl_29_data_C60`
(0xFFA8-0xFFB4), then the effect B is deleted through its vtable slot 0x08 with the deleting flag 1
(0xFFBC-0xFFE4), the token B destroyed (0xFFE8), the effect A deleted the same way (0xFFF4-0x10024),
the token A destroyed (0x10028), the CActor base destroyed (0x10034-0x1003C) and the block freed when
the deleting flag is set (0x10040-0x1004C). The two "created" bytes only gate those two deletes
(0xFFBC, 0xFFF4), so the host's `delete mEffectB; delete mEffectA;` is the same test, and the
host's member order destroys them in the same order (mEffectB, mTokenB, mEffectA, mTokenA).

**Runtime (2026-10-20, this lane)**: Temple Grounds area 57 (world 1baa96c2, area 2735236b), 12000
frames, and Agon 01_sand_entrance_dark (world 42b935e4), 14000 frames - both exit 0 with 0
`[port] caught` / `[reach-stub]`, and the Ing's FSM ran (Start -> BecomeCorporeal -> Lurk, ArmSwipe,
BecomeIngSpot, IngSpotPathFind). The exit host effect's ctor did **not** run in either: only the
ExitHost state builds it, and that needs the Ing to leave a host - the run that did reach it
(`/tmp/opencode/ing_run5.log`) shows `exit host: swarm 021d, sound 20d0`.

**Open question 1, answered: `data_C60` is `CIngExitHostEffect`.** The Wii export's own mangled ctor
(`config/R3ME01/symbols.txt:3184`) is
`__ct__18CIngExitHostEffectF9TUniqueIdRC11CEntityInfoRC12CTransform4fRC9CVector3fRC9CVector3fUiUiff9TUniqueIdffUs`,
i.e. `CIngExitHostEffect(TUniqueId, const CEntityInfo&, const CTransform4f&, const CVector3f&,
const CVector3f&, uint, uint, float, float, TUniqueId, float, float, ushort)` - 13 arguments - and
they line up one for one with the REL ctor's use of them (uid, info, transform, dest, scale, the two
asset ids, the two floats x2c4/x290, the target id, the two floats x298/x2a0, the sound). The name
matches as well: the REL's rodata_7C8 is "IngExitHostSwarmFx" (asm 0x1108C), the vtable's slot 0x08
is `fn_29_FF88` (a dtor), and the class is the only `rs_new(0x5e8)` in the module. No other class in
the module has that ctor shape, so the identification is closed.

### A.19 The module's own functions: the stand-ins, the 0xe40 timer and the damage per form (2026-10-20)

The 16 stand-ins the 2026-10-20 run logged (the module functions the host substitutes) were taken
one by one. Six are ported now, five are the host's own C++ and stay cited, five stay stand-ins:

| retail fn | size | what it is | the host |
|---|---|---|---|
| `fn_29_3B50` | 0x108 | the body-state-5 timer (0xe40), then the body/animation update and the state machine | the timer is ported (`CIng::Think`); the body/animation update is the base class's and is not gated |
| `fn_29_B784` | 0x10C | `CPatterned::UpdateHitDamageTime` | ported (`CIng::UpdateHitDamageTime`) |
| `fn_29_B294` + `fn_29_1DF4` | 0xDC + 0x128 | `Listen` and its safe-zone helper | ported (`CIng::Listen`) |
| `fn_29_2270` | 0x1A4 | the ing spot's idle and move looped sounds | ported (`CIng::UpdateLoopSfx`) |
| `fn_29_C0B4` | 0x174 | `Render` | ported (`CIng::Render`) |
| `fn_29_344C`, `fn_29_3568`, `fn_29_F1A0` | 0xC0, 0x84, 0xC0 | the joint description's copy ctor and the joint vector's grow and dtor | the host's `rstl::vector< CJointCollisionDescription >` is the same structure |
| `fn_29_432C` | 0x13C | the module's timers (0xe30, 0xe38, 0xe3c, 0xe44, 0xe48, 0xe58, 0x440) | the host's `UpdateTimers` is that step |
| `fn_29_CF64` | 0x1C4 | the destructor (it frees the data block, `fn_29_10F8`) | the host's member destructors |
| `fn_29_E7C8` | 0x110 | the data block's build in the constructor | the loader's finished block |
| `fn_29_37D8` | 0x170 | the separation steering | ported (`CIng::ApplySeparation`) |
| `fn_29_4960` | 0x148 | the light's blend (0xe50) and the light object | ported (`CIng::UpdateLight`) |
| `fn_29_4B4C` | 0x13C | the actor's own light (a `CGameLight`, 0x1b8 bytes) | ported (`CIng::CreateLight`) |
| `fn_29_16AC` | 0x3D4 | the loader's data block (the SLdrIng default constructor) | ported (`PortLoadIng`) |
| `fn_29_4830` | 0x130 | the ing spot's blob object (0xd5c) | ported (`CIng::CreateBlob`): module 25's GeomBlobV2, the host's `CCollisionTracker` (0x1e0) |

**No module function the run reaches is a stand-in any more.** The last one was the blob
(`fn_29_4830`): module 25 is **GeomBlobV2**, and the host has it - `CCollisionTracker` (0x1e0 bytes,
the retail's GeomBlobV2 size) in `src/MetroidPrime/Enemies/CCollisionTracker.cpp`, with the factory
`LoadGeomBlobV2` and the module's function-pointer table in `SGeomBlobV2_FuncPtrs.cpp`. The blob is
made in the create branch (asm 0xCB84) from the ing spot's blob effect (a 'PART' token, asm
0xCB24-0xCB44), with the Ing's area id, the name "IngBlobEffect" (rodata_120+0x59b), a transform at
the Ing's position, the Ing as the owner, the flags 0 and the track radius 10 (`lbl_25_rodata_40`).
Its particle emission rate scalar (the member at 0x190 - the retail reads it at 0x2B60 and hands it
to `CElementGen::SetGeneratorRate`) is then set to 0 (`fn_25_256C`), so the blob emits no
particles.

**The loader's data block** (`fn_29_16AC`, 0x3D4) is the retail's default constructor for the
`SLdrIng` block: it fills it with the game's defaults before the loader parses the properties over
it (the sub-struct constructors at asm 0x16C0-0x1790, the -1 words at 0x16D4-0x1724 and
0x1734-0x174C, the floats at 0x300-0x314, 0x348-0x358, 0x370-0x380 and 0x3a0-0x464). The host's
generated `SLdrIng` carries the export's defaults, and they agree field for field - except for the
fifteen sound ids, which the retail leaves invalid (-1, so the block's ushorts are 0xffff, the
invalid sfx id) while the generated header leaves them 0. The loader now sets them to -1 before
the parse, so an Ing whose level sets no sound id stays silent instead of playing sound 0.

**The separation steering** (`fn_29_37D8`, asm 0x37D8-0x3924) walks the state's listening-AI list
(the CStateManager's list at 0x830 - Prime 1's `kOL_ListeningAi` is the sixth list, so 0x808+5*8) and,
for every other `CPatterned` in the same area, asks the steering behaviours for the **3D**
separation with the radius `rodata_7C` (15) times the model's x scale. A safe separation vector is
delivered as a locomotion command in its **normalized** direction with the speed `rodata_B4` (0.5) -
once per neighbour, so the last one wins. The host's body was a guess (the 2D separation, the
vectors summed, one command); it is now the retail's own step.

**The light** (`fn_29_4B4C` creates it, `fn_29_4960` updates it): the module makes a `CGameLight`
(0x1b8 bytes, the same class the Jump Guardian makes) with the Ing's area id for persistence, the
actor's active flag, the name "Ing Light" (`rodata_120`+0x5a9), the identity transform, the Ing's
own id as the parent and a black point light at the origin; its id goes to 0xd68. `fn_29_4960` then
runs the light's blend (0xe50): it rises by dt while the Ing is alive (the bit 6 of the byte at
0x420) and corporeal, capped at `rodata_64` (1.0), and falls by dt otherwise, stopped at zero. The
light takes the blend of the black and the data's light colour (0x7d8) at the eye segment's
transform (0xd7b) with the attenuation 1/0x7dc (the data's 1.170, so 0.855).

**What advances 0xe40** (the 2.5 s draw timer of the forms 3 and 5): `fn_29_3B50`, asm
0x3B7C-0x3BA8. It reads the body controller's state id (0x37c) and, when that is 5
(`pas::kAS_Locomotion`), adds dt to 0xe40; in any other state 0xe40 is cleared. The constructor
(asm 0xE2C4) and `BecomeIngSpot` (fn_29_9750, asm 0x9834) clear it too. The retail then only
updates the body controller and the animation while 0xe40 is below `rodata_C8` (2.5f) - the host's
`CPatterned::Think` does both unconditionally, so only the timer is ported. The ing spot's body
asks for the generate state (12, fn_29_9750's `ShouldRetry` at asm 0x9854), so its timer stays 0
and the draw gate never trips for it.

**The damage per form** (the question the 2026-10-20 smoke raised): the tables say why - the
loader prints them (`[ing] load: light beam spot x2.00 trigger x0.00 base x1.00`):
- the joint collision actors carry `CPatterned`'s own table (`fn_29_3014` calls
  `GetDamageVulnerability__10CPatternedCFv` directly, not through the vtable), so a beam that hits
  a collider does the base damage (the light beam x1.00);
- `CIng::GetDamageVulnerability` (fn_29_BF20) gives the Ing its own table: the trigger table (the
  light beam x0.00, so the body is immune to it) in the forms that are not 3 or 4, and the ing
  spot's table (x2.00) in the form 3;
- in the ing spot form a hit that reaches the Ing itself does not move the HP at all: fn_29_2B70
  only resets the reaction timer, flags the spot and spawns the hit effect, because the spot has
  no health of its own (measured: the `XDMG` messages arrive and the HP stays).
So the 300->260 and the 300->180 of the smoke are the same base table with two and six beam ticks
that landed, not two per-form vulnerabilities.

### A.20 The unmarked functions: the REL's copies of host code, and the six hot vtable bodies (2026-10-21)

The module has 274 functions; 128 carry a `// @ fn_29_<off>` marker (the 127 `tools/port_progress.py`
reports plus `fn_29_3424`, whose marker sits inside a shared comment line). The other **146** split
in two: **76 are the REL's own copies of host/DOL code** (never markable - the host's C++ already
does them) and **70 are the module's own behaviour**.

**Group 1 - the REL's copies of host/DOL code (76).** Every one of them was read in the asm; the
host does the same work in its own C++:

| what | n | functions | the evidence |
|---|---|---|---|
| the vtable thunks | 14 | 0x0, 0x8, 0x10, 0x18, 0x24, 0x2C, 0x34, 0x3C, 0x4C, 0x58, 0x64, 0x6C, 0x74, 0x90 | 2..11 instructions and **no call at all**: `addi r3, r3, 0x9dc; blr` (the search path, slot 56), `addi r3, r3, 0xac8; blr` (the point search, slot 57), `li r3, 0x1; blr` (CanBeShot slot 42, IsListening slot 43), `li r3, 0x0; blr` (ProjectileInfo slot 55, GetRagDoll slot 61), the two float constants (rodata_64 = 1.0, the DOL's lbl_8041B758), the byte at 0x44f (slot 50), the byte at 0x34c bit 3 (slot 68), `this+0x754` (slot 73), the actor's translation (GetOrigin, slot 45) and the slot-0x38 call (GetHealthInfo, slot 15) |
| the loader's DOL-type temporaries | 12 | 0x118C, 0x11E4, 0x123C, 0x1294, 0x12D4, 0x12F4, 0x131C, 0x1458, 0x14C4, 0x1518, 0x157C, 0x15DC | the optional<CModelData> move (0x1294) and the ctors/dtors of CToken (0x131C), CAnimData (0x1518), CModelData (0x157C), CDamageVulnerability (0x11E4, 0x123C) and the SLdr blocks (0x118C, 0x15DC); the host's loader (`PortLoadIng`) and the generated `SLdrIng` do the same |
| the compiler-generated destructors and the new/free wrappers | 33 | 0x2098, 0x20F0, 0x10F8, 0x114B0, 0x5104, 0x6158, 0x6D44, 0x71B8, 0x7D10, 0x7D60, 0x7DC8, 0x8C98, 0x8E94, 0x944C, 0x9A18, 0xAFA8, 0xBB50, 0xC5E0, 0xCEB8, 0xCF10, 0xCF64, 0xD128, 0xD30C, 0xD3A0, 0xD3F8, 0xD450, 0xD4A8, 0xE650, 0xE68C, 0xE6F8, 0xE750, 0xE78C, 0xECE4 | every one of them is the deleting-destructor shape `mr. r31, r3 / beq / extsh. r0, r4 / ble / bl Free__7CMemoryFPCv`, with the vtable reset and the DOL's own `__dt__` calls; the host's member destructors and `rs_new` do the same |
| the rstl internals | 4 | 0x2144, 0x350C, 0x35EC, 0x3624 | the module's static string's destructor (0x2144, the "Grenchler.rel" string) and the joint vector's destroy/copy helpers |
| the plain struct copies | 3 | 0x94A8, 0xBA4C, 0xBAB4 | the DOL's cross product (0x94A8), the bounding-box copy (0xBA4C) and the optional<CAABox> copy (0xBAB4) |
| the REL runtime | 10 | RELMain, RELExit, 0x121F4, __destroy_global_chain, __register_global_object, _unresolved, _epilog, _prolog, ModuleConstructors, ModuleDestructors | the module's entry points and the linker's support; 0x121F4 is the static initializer that fills the BSS string |

**Group 2 - the module's own behaviour (70).** `0x100` (the loader's head), `0x130` (the loader),
`0x1C98`, `0x1DF4`, `0x3190`, `0x3684`, `0x3948`, `0x426C`, `0x45BC`, `0x4604`, `0x4AA8`, `0x4C88`,
`0x514C`, `0x5190`, `0x51D0`, `0x53E0`, `0x5714`, `0x5C28`, `0x75C0`, `0x82D4`, `0x84A4`, `0x8CF4`,
`0x8FE8`, `0xA340`, `0xA4E0`, `0xAA04`, `0xAA4C`, `0xAA94`, `0xAC78`, `0xB1C8`, `0xB1E8`, `0xB3C4`,
`0xB534`, `0xB890`, `0xB8C0`, `0xBB8C`, `0xBCB4`, `0xBEB4`, `0xBF20`, `0xBFF0`, `0xD540` (the ctor),
`0xE7C8`, `0xE8D8`, `0xE944`, `0xE9E8`, `0xEA7C`, `0xEB38`, `0xEBEC`, `0xEC88`, `0xED38`, `0xEE68`,
`0xEFC0`, `0xF000`, `0xF090`, `0xF12C`, `0xF260`, `0xF2C8` (the FSM tables), `0x10D50`, `0x10E64`,
`0x11438`, `0x11490`, `0x11570`, `0x11840`, `0x119E8`, `0x11A7C`, `0x11C14`, `0x11CA4`, `0x11E8C`,
`0x1201C`, `0x12100` (the mini-portal class).

**The six hot vtable bodies, read against the asm.** They are the module's own overrides of the
engine's per-frame and per-event slots, so they are the most-called of the unmarked group. Five
matched the port; one did not and is fixed:

| fn | slot | what the asm does | the port |
|---|---|---|---|
| `fn_29_BF20` GetDamageVulnerability | 0x40 | **all four arms**: the base table (the form 4) in the Agon run of 2026-10-21 (`fsm <ing> BecomeBodyProjectile` and a hit on the Ing), the grapple's table in the Agon morph-ball run of 2026-10-21 (2 calls), the ing spot's table and the trigger's table (the forms 0, 1, 2, 3, 6 and 7) in the Agon run | **marked**
| `fn_29_B890` TakeDamage | the forms 0, 1 and 5 return (0xB894-0xB8AC) and the other forms store (0xB8B0) | the forms 3 and 4 ran (the Agon run of 2026-10-21; the coverage run: the key 4 with 6 hits); the early return (the forms 0, 1 and 5) did not - the hit in the host and the evaporating forms reached the vulnerability lookup but not TakeDamage: the trigger table takes nothing from the console's weapons (light, missile, dark, annihilator, power, cannonball and powerbomb were all tried). It needs a forced call: under gdb, make the vulnerability lookup return the ing spot's table (the form 3) so the damage gets through, and the hit then arrives in the form 0 | not marked |
| `fn_29_BCB4` GetAimPosition | 0x54 | the forms 2 and 6..8 predict their motion (only with dt > 0), the collar locator (0xd7a, invalid in retail) would add the model's scale times the locator's world position, and the forms 6/7 fall back to the base when the aim is below the actor's own height | matched (all four arms, including the dead locator path) |
| `fn_29_BB8C` Touch | 0x4c | the ing spot (3) damages the target it touches when the ball is not held, the toucher is the target (0xd6c) and the cooldown (0x440) has run out; the damage is the actor's own `GetContactDamage`, the filter is Solid alone (type 3) and the cooldown is reloaded from 0x444 | matched (the filter, the ids and the reload) |
| `fn_29_B534` CollidedWith | the form (0xB558), the flight time (0xB56C), the empty list (0xB710), the contact that is not a splat surface (0xB720) and the one that is (0xB738) | the form is not the body projectile (the forms 2, 3, 5, 6, 7 and 8) and the form 4 (the body projectile, 7 hits in the Agon run of 2026-10-21) ran; the flight time under half a second and a contact that is a splat surface ran in the Agon run; the contact that is **not** a splat surface and the empty list did not - they need one forced call each (the collision list's count and the material list's value changed for one call under gdb) | not marked |
| `fn_29_B8C0` KnockBack | 0x9c | **the port was missing two steps**: the anim reaction (`EnableAnimReaction(2, true)` only in the corporeal form outside the knock-back body state 6) and the flinch type at 0x688 (2 for the light beam, 1 for everything else) | **fixed** - the body is now the Boost Guardian's copy of the same code (fn_30_CB84) |

`fn_29_B3C4` (Death, slot 0x98) is the seventh hot body; it is **not** ported faithfully yet - see
the finding (2) below.

**The run** (`/tmp/opencode/ing_lane/ing_run.sh` + `ing_con.py`, rebuilt from A.5/A.6 after the
old scripts were lost): Temple Grounds world 1baa96c2, area 57 (`0i_cliff_hall_dark`, MREA
2735236b), the two Ings activated over the console, the light beam given, the player teleported
to 58.4 356 9 and firing every 2 s. Three runs: the plain one (`ing_run.log`, 20000 frames), the
light-beam one (`ing_light.log`, 9000) and the long one (`ing_long.log`, 30000); all three exit 0
with 0 `[port] caught` and 0 `[reach-stub]`.

The arms below were told apart with a **local probe** - one `if (DebugIng()) printf(...)` per arm
in the six bodies - which is **not committed** (a probe is not port code). It is kept as
`/tmp/opencode/ing_lane/ing_arm_probe.patch` (apply it on top of the tree to get the arm lines
back); the arm counts in the table are its output.

What the runs prove, per body (an arm that did not run is *not* unreachable - it is simply not
reached by this recipe):

| body | arms the run covered | arms still open |
|---|---|---|
| `fn_29_BF20` GetDamageVulnerability | the trigger table (46 calls, the corporeal form) and the ing spot's table (45) | the base table (the body projectile, form 4) and the grapple's table (the ball held) |
| `fn_29_B890` TakeDamage | the forms 0, 1 and 5 return (0xB894-0xB8AC) and the other forms store (0xB8B0) | the forms 3 and 4 ran (the Agon run of 2026-10-21; the coverage run: the key 4 with 6 hits); the early return (the forms 0, 1 and 5) did not - the hit in the host and the evaporating forms reached the vulnerability lookup but not TakeDamage: the trigger table takes nothing from the console's weapons (light, missile, dark, annihilator, power, cannonball and powerbomb were all tried). It needs a forced call: under gdb, make the vulnerability lookup return the ing spot's table (the form 3) so the damage gets through, and the hit then arrives in the form 0 | not marked |
| `fn_29_BCB4` GetAimPosition | the base (the forms 0, 1, 3, 4) and the predict path (the forms 2, 6, 7, 8), with dt = 0 | the predict path with dt > 0 (the lock-on asks with a prediction time, but only while the Ing is an ing spot) |
| `fn_29_BB8C` Touch | the hit (47), the form (35643), the id (2181) and the cooldown (1024) | the "the ball is held" case |
| `fn_29_B534` CollidedWith | the form (0xB558), the flight time (0xB56C), the empty list (0xB710), the contact that is not a splat surface (0xB720) and the one that is (0xB738) | the form is not the body projectile (the forms 2, 3, 5, 6, 7 and 8) and the form 4 (the body projectile, 7 hits in the Agon run of 2026-10-21) ran; the flight time under half a second and a contact that is a splat surface ran in the Agon run; the contact that is **not** a splat surface and the empty list did not - they need one forced call each (the collision list's count and the material list's value changed for one call under gdb) | not marked |
| `fn_29_B8C0` KnockBack | the no-reaction case with the light beam's flinch type 2 | the corporeal form's reaction and the flinch type 1 |

So none of the six gets a `// @` marker yet: every one of them still has an arm this recipe does
not reach. Two **branch-free** bodies do get one, because the run evaluated them (an entry-only
log proves a body with no arm): `fn_29_B1E8` (IsAlert - the run entered the Alert state, whose
`!HasTarget` branch evaluates the trigger) and `fn_29_B1C8` (IsControllingHost - the run entered
the SuckEnergy state, whose outgoing trigger list is "->IsControllingHost ->TargetInSafeZone").

**Two findings for the next slice.** (1) The module's flag bytes are counted **from the top of the
byte**: the asm's `extrwi`/`rlwimi` index I is the byte's natural bit (I - 24) and the port's flag
names use the reverse order (the port's bit k = the natural bit 7 - k). The proof is the Lurk
state: the retail sets the byte at 0xe5d's natural bit 5 (asm 0x00008A8C, the index 29) and the
IsAlert trigger reads exactly that bit (fn_29_B1E8, the index 29) - the same flag, and the port's
`mAlert` is it. The GrappleBall sets the natural bit 7 (asm 0x000065E8, the index 31) and the
Touch and GetDamageVulnerability read it with `clrlwi r0, r0, 31` (the byte's bit 7) - again the
same flag, the port's `mGrappled`. Every flag the port names for 0xe5c/0xe5d/0xe5e is consistent
with that reading. (2) `fn_29_B3C4` (Death, slot 0x98) is **not** ported faithfully: the retail's
body does not call `CPatterned::Death` at all - it sets the body controller's time scale (0x5c8 =
1.0), sets the *animation* effect state "Dead" (rodata_120+0x45c) instead of the FSM's "Dead"
state, removes the material 0x25, clears the alive bit (0x420 bit 6) and the byte at 0x34c
(unless the burn flags at 0x421 are set), issues the death body command only in the forms 2 and
6..9, and sends the script messages itself. The host's base class does the shared part but also
the parts the Ing's body replaces, so porting it needs the anim data's vtable slot 0x28 (the test
before the effect state) mapped first - left open, and `fn_29_B3C4` stays unmarked.

### A.21 The death body and the next five hot behaviour bodies (2026-10-21)

**`fn_29_B3C4` (Death, vtable slot 0x98) is now the retail's body.** The retail's Ing *replaces*
`CPatterned::Death` - the base class's body is never called (asm 0x0000B3C4-0x0000B530):

| step | the asm | the port |
|---|---|---|
| the gate | the byte at 0x420's bit 1 (the alive bit) | `GetAlive()` |
| the body controller | 0x5c8 = 1.0f | `BodyController()->SetTimeScale(1.f)` |
| the FSM | the state machine's vtable slot 0x28 (the same slot `fn_29_3B50` asks before it calls `InitializeStateMachine`: "the machine has a state") and then its slot 0x20 with (mgr, owner, "Dead") | `mStateMachine->HasState()` and `mStateMachine->SetState(mgr, *this, rstl::string_l("Dead"))` |
| the material | 0x25 | `RemoveMaterial(kMT_GroundCollider, mgr)` (the host's enum value is 37 = 0x25) |
| the burn flags | the byte at 0x421's bits 1 and 2 (the host's `mBurning` and `mLaggedBurnDeath`) guard the clearing of the byte at 0x34c's bit 6 | `if (!mBurning && !mLaggedBurnDeath) mVerticalMovement = false;` |
| the alive bit | the byte at 0x420's bit 1 | `mAlive = false` |
| the death body command | only in the form 2 and the form changes 6..9 | the same forms |
| the script messages | outside the alive gate, and only when the state argument is not -1; the source is the invalid id | `if (state != kSS_InvalidState) SendScriptMsgs(state, mgr, kSM_None);` (-1 is `kSM_None`) |

What the Ing's body does **not** do (the base class's body does, and the retail's Ing does not):
the massive-death logic, `SetHighlightedInDarkVisor(false)`, the unpossess step, and the death
sound (`CSfxManager::AddEmitter(mDeathSfx, ...)`) - the Ing's own `Dead` state (fn_29_5CB0) plays
the ing spot's death sound instead. The sounds also stop on the delete, not in the death body
(`fn_29_2194` is called from the Think, fn_29_C9A4), so the port's `StopSfx()` call left the
death body.

**The wake-up, diagnosed (2026-10-21).** The nine runs that kept the Ings as ing spots were a
harness problem, not the module's. With a probe in `CIng::Listen` and in
`CStateManager::InformListeners` the run says exactly what happens:

1. **The Ing has to be active before it can hear anything.** `CStateManager::InformListeners`
   (src/MetroidPrime/CStateManager.cpp:3512-3518) walks `kOL_ListeningAi` and calls
   `patterned->Listen(...)` **only for an active actor**; the probe printed
   "inform: listener 04f2 **active 0**" for every noise while the Ings slept. The active flag
   comes from the `Activate` script message: `CEntity::AcceptScriptMsg` (src/MetroidPrime/CEntity.cpp:44-49)
   sets it, and the chain `CIng::AcceptScriptMsg` -> `CPatterned::AcceptScriptMsg` ->
   `CAi::AcceptScriptMsg` (src/MetroidPrime/Enemies/CAi.cpp:59) -> `CActor::AcceptScriptMsg` ->
   `CEntity::AcceptScriptMsg` is complete, so the message does activate the Ing - **but only if it
   reaches it**. The driver's `send <uid> Activate` was sent in the first two rounds, before the
   area's actors were up, so it went to nobody. Sending it **once, when the Ing first shows up in
   `objs Ing`**, wakes it (the probe then prints "listener 04f2 active 1"). It must not be repeated:
   the message restarts the Ing's state machine.
2. **The player has to stand on the puddle.** The noise is registered at the gun
   (`CPlayerGun.cpp:247` informs the listeners with the gun's world position), and the retail's
   Listen (fn_29_B294, asm 0x0000B32C-0x0000B3A4) rejects the noise when the squared distance is
   over the hearing radius squared (CIngData+0 = 40) or the squared height difference over the
   leash value (0x3d4) squared. The port's body matches the asm field for field (the radius, the
   leash, the "0 disables the test" case) - the probe printed "dist 438.36, dz 7.83, radius 40.0"
   for a player that had fallen off the ledge, and "dist 43.46, dz 1.89" for one standing on the
   puddle. **No module bug here**: the port's `CIng::Listen` is the retail's body.
3. **The beam has to be the light one.** The power beam is charged, so a short console press does
   not fire (the probe saw no type-0 noise at all); the light beam fires while it is held (the
   probe: "inform: type 0 at (97.0, 366.3, 6.4)" and 57 "listen: ... heard").

With all three, the Ings wake: the probe logged "heard" 57 times and the log shows the whole
cycle again - "state PathFind form 2" (the corporeal form), "state FindCoverPoint form 3",
"state SetCoverDest form 3", "state IngSpotPathFind form 3" - and the console's
`damage u<uid> 100` hits the corporeal Ing. Two things are still open: the hit has to land while
the Ing is corporeal (the ing-spot form takes no damage, A.19), and the driver's id mapping (the
log prints the unique id without its version bits, the console with them: `0x400 | tag`).

**`fn_29_B3C4` is not marked yet** (the run of 2026-10-21, the probe in the death body): the
run killed **both** Ings of area 57 and the death body ran with these arms: alive, not alive
(the second death), the machine has a state, not burning, **the form 2 - the death body command**
(the death in the corporeal form, through a real beam at the joint collision actors), the form 3
(no command) and the script messages. Two arms are open:
- **the burning arm** (the byte at 0x421's bits 1 and 2: the Ing has to be on fire when it dies).
  The host has no fire that burns a patterned actor - only `COctapedeSegment` calls
  `CPatterned::Burn` (src/MetroidPrime/Enemies/COctapedeSegment.cpp:267,650,657) and the fluid
  plane never does, so the arm is **blocked by unported shared code**, not unreachable. Request
  for the orchestrator: the fire plane should call `Burn` on the patterned actors it sets on fire
  (the Ing's area list in docs/modules.tsv is the Agon Wastes, which has lava);
- **"no messages"** (the state argument -1): the Ing's death always comes with a real state (the
  damage path's kSS_DeathRattle), so this arm cannot run in the shipped data.

**The ing spot does die** - the earlier note that it cannot was wrong: the run of 2026-10-21 shows
the death body with the form 3 ("form 3, no command"), and the ing spot's HP does go down.

**The corporeal death needs a real beam at the joint collision actors.** The console's `damage`
goes to the Ing itself, and the Ing's own table (the trigger table) takes nothing from the power
beam, so the corporeal Ing never loses HP that way. The joints carry the Ing's health and the
base table (fn_29_3014), so the driver's `shoot <joint uid> charged 1` (a real beam projectile
from one unit away, at the joint's aim position) is what kills it - the run of 2026-10-21 killed
both Ings that way and the death body ran with the form 2.

**The four hot bodies' branches** (the count is from the asm; the "run" column is what the area
57 run of 2026-10-21 reached, with a probe in each body):

| fn | branches | arms | the run |
|---|---|---|---|
| `fn_29_45BC` GetCoverHint | 1 | the id is invalid -> null; the id is valid -> the lookup | **both ran** (2 and 4145 calls) -> **marked** |
| `fn_29_4604` the blob align | 3 | the blob is gone -> nothing; the align flag is 0 -> nothing; the align flag is 1 -> the look along the normal, with the up vector (the normal's z < 0.95) or the right one (z >= 0.95) | the align flag 0 (the forms 0, 1, 2, 4, 6, 7, 8) and 1 (the forms 3, 6, 7), the up vector and the right vector; the "the blob is gone" arm cannot run (the blob is made in the create branch and deleted in the delete branch) -> **marked** (the two steps the retail's body has and the port had not - GeomBlobV2's fn_25_256C and the blob's flag byte at 0x154 - are ported now: the host's `CCollisionTracker::SetParticleEmissionRateScalar` and its new `SetOwnerDamaged`) |
| `fn_29_426C` the valid target | 3 | the forms 0/1 -> false; the forms 3/5 with the Dark visor -> the byte at 0x9d0's bit 0; the forms 3/5 with another visor -> true; the other forms -> true | **all four ran** ("always a target" 7166, "another visor" 21, "never a target" 2277 - the host form - and "the Dark visor" 9292, with the console's `give darkVisor` and `visor dark`) -> **marked** |
| `fn_29_3948` MoveOnSurface | 2 | the parallel part is below 0.6 of the move -> the perpendicular part; the move is not magnitude-safe -> the zero vector; else the normalised move | **all three ran** (3369, 37 and 311 calls) -> **marked** |

**The Agon Wastes recipe** (A.12) is the other way in: there the Ings are ing spots that the
player morphs into a ball next to, and the Ing grabs it. The driver's `grapple` mode
(`tools/drivers/ing_drive.py`) does it: the layers on, the Ings woken, `press x` to morph and the
player teleported onto the puddle. The run of 2026-10-21 (Agon Wastes `01_sand_entrance_dark`, the
world 42b935e4) reached the state: "state GrappleBall form 3" and the probe's arms -
`fn_29_BB8C`'s "the ball is held" (4484 calls, the Ing with the ball in it) and
`fn_29_BF20`'s "the grapple table" (2 calls) - so **`fn_29_BB8C` is marked** (all five of its
arms ran: the hit, the form, the id, the cooldown and the ball). `fn_29_BF20` keeps one open
arm: the base table, which only the body projectile (the form 4) asks for.

**The next five hot behaviour bodies** (by the module's own call sites and by how often a run
calls them), each read field by field against the REL - all five match the port:

| fn | what it is | verdict |
|---|---|---|
| `fn_29_45BC` (0x48) | the cover hint lookup: the retail reads the Ing's own hint id (0xd72) and looks it up, casting to `CScriptAIHint` | the port's helper takes the id instead; every call site passes 0xd72 except `FindCoverPoint`, which looks up the *picked* hint - the retail does that in its own helper (fn_29_4510), so the object is the same either way |
| `fn_29_4604` (0x22C) | the blob's align: the blend factor is 1.0 in the ing-spot form, in the form changes 6..9 when the Ing started as an ing spot, and in the corporeal form when the Ing is dead and started as an ing spot; the blob then looks along the surface normal with the up vector (the normal's z >= 0.95) or the right one | the port matches, including the 0.6/0.95 constants and the up/right choice (the retail's double trick resolves to "z < 0.95 -> up, else right") |
| `fn_29_426C` (0xC0) | the valid target: the forms 0/1 are never targets, the forms 3 and 5 follow the Dark visor (3) and the byte at 0x9d0's bit 0, every other form is always a target | the port matches (`CIng::UpdateValidTarget`) |
| `fn_29_3948` (0x208) | the move on the surface: the parallel part is cut when it is below 0.6 of the move's length, the result is normalised, the actor moves by speed*dt and the move direction is stored; a move that is not magnitude-safe stores the zero vector | the port matches (`CIng::MoveOnSurface`) |
| `fn_29_4AA8` (0xA4) | the light struct's copy assignment (0x50 bytes, field by field) | **not module behaviour** - it is one of the plain struct copies, so it joins the host-code group of A.20 |

### A.22 The five hot vtable bodies, arm by arm (2026-10-21)

The next five unmarked Ing bodies (the module's own hot vtable slots), each read field by field
against the REL and each run in area 57 (Temple Grounds `0i_cliff_hall_dark`, the world `1baa96c2`
and the area `2735236b`, the fast harness `MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8`) with a
temporary probe in every arm. The probes are not committed.

**The fix.** `fn_29_BCB4` predicted the motion in the form 9 as well: the port's range ran to
`kForm_BecomingWallProjectile`, but the retail's `cmpwi r0, 0x9 / beq` (asm 0x0000BD28-0x0000BD30)
sends the form 9 and everything above it to the base class. The range now ends at
`kForm_BecomingBodyProjectile` (8) - the predict forms are 2, 6, 7, 8, as the doc said.

**The bodies.** `fn_29_B534` (CollidedWith), `fn_29_B890` (TakeDamage), `fn_29_B8C0` (KnockBack),
`fn_29_BCB4` (GetAimPosition) and `fn_29_BF20` (GetDamageVulnerability) are otherwise the retail's
bodies - the mask of the six splat surfaces is the retail's own six bit indices (0x1d, 0x1e, 0x1f,
0x3b, 0x2b, 0x2a = the host's `kMT_Ceiling`, `kMT_Wall`, `kMT_Floor`, `kMT_Solid`, `kMT_Immovable`,
`kMT_Occluder`), the collision list's stride is 0x60 and its material list and normal are the
`CCollisionInfo`'s 0x30 and 0x40, the flinch type is the weapon type (2 = the light beam) and the
cover hint's id is the Ing's own.

| fn | the arms (the asm) | the run (2026-10-21, area 57) | verdict |
|---|---|---|---|
| `fn_29_B534` CollidedWith | the form (0xB558), the flight time (0xB56C), the empty list (0xB710), the contact that is not a splat surface (0xB720) and the one that is (0xB738) | the form is not the body projectile (the forms 2, 3, 5, 6, 7 and 8) and the form 4 (the body projectile, 7 hits in the Agon run of 2026-10-21) ran; the flight time under half a second and a contact that is a splat surface ran in the Agon run; the contact that is **not** a splat surface and the empty list did not - they need one forced call each (the collision list's count and the material list's value changed for one call under gdb) | not marked |
| `fn_29_B890` TakeDamage | the forms 0, 1 and 5 return (0xB894-0xB8AC) and the other forms store (0xB8B0) | the forms 3 and 4 ran (the Agon run of 2026-10-21; the coverage run: the key 4 with 6 hits); the early return (the forms 0, 1 and 5) did not - the hit in the host and the evaporating forms reached the vulnerability lookup but not TakeDamage: the trigger table takes nothing from the console's weapons (light, missile, dark, annihilator, power, cannonball and powerbomb were all tried). It needs a forced call: under gdb, make the vulnerability lookup return the ing spot's table (the form 3) so the damage gets through, and the hit then arrives in the form 0 | not marked |
| `fn_29_B8C0` KnockBack | the form is not 2 (0xB8E8); the form 2 and the body state 6 (0xB8F8); the form 2 and any other body state (0xB900); the weapon type 2 (0xB928); any other weapon (0xB93C) | all five: the reaction off in the form 3 (26 calls), the reaction off in the form 2 (1 call), the reaction on in the form 2 (12 calls), the flinch 2 and the flinch 1 (the power beam, 6 calls) | the port matches; **marked** |
| `fn_29_BCB4` GetAimPosition | the base class (0xBE4C); the prediction (0xBD34) with dt above zero (0xBD3C) and with dt of zero (the aim stays the zero vector); the collar locator (0xBD6C) and the forms 6/7's fall back (0xBDF8) | the base in the form 3 (dt 0.00), the prediction in the form 6 (dt 0.22, 0.23) and the form 2 (dt 0.07, 0.22) - the lock-on asks the target with a prediction time - and the form 6 with dt 0.00; the collar locator and the forms 6/7's fall back are inside the locator's path, and the locator is invalid in retail (0xd7a = 0xff) | the port matches (after the form 9 fix); **marked** |
| `fn_29_BF20` GetDamageVulnerability | the form 4 takes the base class's table (0xBF34); the form 3 takes the grapple's table when the ball is held (0xBF50) or the ing spot's table (0xBF64); every other form takes the trigger's table (0xBF6C) | the trigger's table in the forms 2 and 6 (71 calls), the ing spot's table in the form 3 (37 calls), the grapple's table in the Agon Wastes run of 2026-10-21 (2 calls); **the base table never ran** - only the body projectile (the form 4) asks for it | the port matches; **not marked** (one arm open) |

**The two arms that are still open** are the same two the earlier runs left open (A.20): the
contact that is not a splat surface and the base damage table both need the body projectile (the
form 4), and the area 57 runs of 2026-10-21 never reached that form - the Ings there stay in the
ing spot (3) or the form changes (6). The Agon Wastes run reaches the body projectile.

**The clrlwi question (for the orchestrator).** Three places in the module read the flags byte at
0xe5d with `lbz r0, 0xe5d(r3); clrlwi. r0, r0, 31` and branch on the result: `fn_29_BB8C` (the
touch, 0xBBB0), `fn_29_BF20` (0xBF50) and the Think's damage branch (0xCDC0). The port reads that
as the byte's bit 0 (`mGrappled`), which is what `tools/asmcheck.py` models (`clrlwi` as the
`rlwinm` family, the mask 0x1). The PowerPC definition of `clrlwi rA, rS, 31` is "keep the low 31
bits", so on a byte the branch would ask "is the whole flags byte zero" - every flag of 0xe5d
(`mRenderModel`, `mPortalPlane`, `mStartAsIngSpot`, `mForm9Render`, `mFollowingAttackPath`,
`mAlert`, `mMoving`, `mGrappled`) instead of the one bit. The module's other bit reads of the same
byte use `extrwi` (0x22A0, 0x4684, 0xB1E8), so the compiler does have a bit-test idiom. Two of
the three places are marked functions (`fn_29_BB8C`, `fn_29_2E8C`), so this is a decision for the
orchestrator, not for a lane: if the strict reading is the right one, those two bodies and
`fn_29_BF20` have to test the whole byte.

### A.23 The death body's burning arm and the next five triggers (2026-10-21)

**`fn_29_B3C4` (the death body) is now marked.** The one arm that was open - the Ing burning when
it dies (asm 0x0000B474: the byte at 0x421's bits 1 and 2) - runs with the power bomb: the
console's `damage <ing> 100 powerbomb` applies the knock-back whose follow-up is
`kFU_LaggedBurnDeath`, the follow-up burns the Ing, and the death then runs with the lagged bit
set. The recipe (from the shared lane): `send <ing> Activate; place <ing> player 8; damage <ing>
100 powerbomb`. The run of 2026-10-21 (area 57) shows the death body three times: `alive 1
burning 0 lagged 1 form 3` (the burn arm), `alive 1 burning 0 lagged 0 form 6` and `alive 0
burning 0 lagged 0 form 2` (the not-alive arm and the form 2's death body command). The two
arms that cannot run are cited: the machine always has a state (the constructor sets it up) and
the Ing's death always carries a real state (the damage path's `kSS_DeathRattle`), so the "no
message" arm is unreachable.

**The next five unmarked behaviour functions** are the triggers the FSM2 graph puts in the state
lists; each was read against the REL and probed in the area 57 run of 2026-10-21 (the probes are
not committed). None of them is marked: the arms that need the attack pattern's waypoint or a
safe zone did not run.

| fn | the arms (the asm) | the run | verdict |
|---|---|---|---|
| `fn_29_A4E0` FoundMovementPos | the byte at 0xe5c's bit 3 (0xA4E0) | the trigger is only in the `SetLuredDest` code function's list, and the lure never ran | the port matches; not marked |
| `fn_29_A340` AttackPatternOver | the attack path flag (0xA348) and the waypoint destination against `kInvalidUniqueId` (0xA35C) | the true arm (the path is followed and the destination is gone) ran with the state forced (`fsm <ing> FollowAttackPattern`); the two false arms did not | the port matches; not marked |
| `fn_29_AA04` ShouldBecomeIngSpot | the desired form is 3 (0xAA1C) and `!fn_29_AD40` (0xAA24) | the "the desired form is not 3" arm and both of the `IsIngSpot` variants ran (forced); the arms with the desired form set did not - the desired form is only set by the attack pattern's waypoint (0x00008310) | the port matches; not marked |
| `fn_29_AA4C` ShouldBecomeCorporeal | the desired form is 2 (0xAA64) and `!fn_29_AD54` (0xAA6C) | the "the desired form is not 2" arm only | the port matches; not marked |
| `fn_29_AA94` ShouldFleeSafeZone | the word at 0xe2c is not zero (0xAA94, the neg/andc/srwi idiom) | the trigger never ran - the area 57 run has no safe zone | the port matches; not marked |

The port's `mSafeZoneCount > 0` is the retail's "the count is not zero" for every count the
module can hold (the XENZ/XEXZ messages only add and remove one).

### A.24 The Ing attack patterns and the safe zones in the shipped data (2026-10-21)

`tools/scan_ing_pattern.py` (new, on the CommandoPirate lane's `tools/scan_commando_pattern.py`
model) reads the user's own disc - every world's MLVL, every area's MREA and every SCLY layer -
and lists the script objects that the module's open triggers need: the Ings (the `INGS` object)
with their script connections, the AI waypoints (`AIWP`) with their flag word and the safe zones
(`SAFE`).

**The attack pattern.** The module's FollowAttackPattern state (fn_29_82D4) takes the attack
pattern from the connection in the `ATTK` state (0x4154544B) with the `FOLW` message
(0x464f4c57) - the same fourCCs the CommandoPirate uses. Nine of the 95 areas have an Ing with
such a connection:

| world | area (MREA) | the Ings with the pattern | the safe zones |
|---|---|---|---|
| 0x3bfa3eff (Temple Grounds) | `01_temple_landingsite_dark` (0x16e116b4) | 4 | 3 |
| 0x3bfa3eff | `04_temple_unknownenemies_dark` (0xfc8fc0a4) | 2 | 0 |
| 0x42b935e4 (Agon Wastes) | `01_sand_entrance_dark` (0x3f5cdf39) | 3 | 7 |
| 0x42b935e4 | `02_sand_sandfalls_dark` (0x6b77f9bd) | 2 | 9 |
| 0x42b935e4 | `05_portalpuzzle_dark` (0xbf7f572c) | 2 | 7 |
| 0x42b935e4 | `06_sand_alamofront_dark` (0xea79522c) | 4 | 6 |
| 0x42b935e4 | `07_sand_alamoportalobservation_dark` (0xa49f8a6d) | 2 | 9 |
| 0x42b935e4 | `0m_sand_hall_dark` (0xf2ccc41e) | 1 | 3 |
| 0x3dfd2249 (Great Temple) | `06_swamp_boostballguardian_dark` (0xa49a572f) | 2 | 2 |

**The waypoint the Ing follows is a `WAYP`, never an `AIWP`.** The scan prints the target of
every `ATTK`/`FOLW` connection with its type: in every area it is a `CScriptWaypoint` (the base
class, 0x158 bytes). The retail's fn_29_82D4 casts the navigation's destination to
`CScriptAIWaypoint` before it reads the flags at +0x164 (asm 0x00008428) - and the port does the
same - so the cast fails there too, and the desired form (0x9d8) is never taken from the attack
pattern. The form change comes from the other writers of 0x9d8 (the states that want a form
change), which is why the desired form does change in a run.

**The run.** The Agon Wastes `01_sand_entrance_dark` (the world 42b935e4, the area 0x3f5cdf39, the
layers 1..8 loaded with the console's `layer`), the Ings woken with `send <id> Activate` and the
player next to them: the FollowAttackPattern state runs and the triggers are evaluated.

| fn | the arms | the run (a probe in the body) | marker |
|---|---|---|---|
| `fn_29_A340` AttackPatternOver | the attack path flag (0xA348) and the destination against `kInvalidUniqueId` (0xA35C) | all three: the flag is not followed (the destination invalid and not), it is followed and the destination is there, it is followed and the destination is gone | **marked** |
| `fn_29_AA4C` ShouldBecomeCorporeal | the desired form is 2 (0xAA64) and `!fn_29_AD54` (0xAA6C) | the "the desired form is not 2" arm and the "the desired form is 2 and the Ing is not corporeal" arm; the "already corporeal" arm did not run | not marked |
| `fn_29_AA04` ShouldBecomeIngSpot | the desired form is 3 (0xAA1C) and `!fn_29_AD40` (0xAA24) | the "the desired form is not 3" arm; the arms with the desired form 3 did not run | not marked |
| `fn_29_A4E0` FoundMovementPos | the byte at 0xe5c's bit 3 (0xA4E0) | the trigger is in the `SetLuredDest` code function's list only, and no Ing was lured | not marked |
| `fn_29_AA94` ShouldFleeSafeZone | the word at 0xe2c is not zero (0xAA94) | the count never changed (the player teleporting into a safe zone does not raise the XENZ message) | not marked |

### A.25 The classification of the 274 Ing functions (2026-10-21)

`docs/module-analysis/Ing_functions.tsv` lists every function of the Ing REL with its class, on
the EmperorIngStage1_functions.tsv model: **H** = the host's own copy (the DOL's code the module
carries a copy of - never marked), **T** = a vtable thunk (the module's copy of a base class's
virtual body), **B** = the module's own behaviour (marked once a run exercises it). The columns
are the offset, the size, the function, the class, the kind, the callers (the static `bl` in the
module's own text), the data tables the body touches, a one-line description and the marker
state.

**The count check**: 274 functions = **61 H + 14 T + 199 B**; of the 199 B, **140 are marked**
and **59 are the unmarked B work list**. The 61 H are the module's own deleting destructors and
free-only thunks, the loader's DOL-type temporaries, the rstl internals, the plain struct copies
and the REL runtime; the 14 T are the vtable thunks (IngFamily.md A.20 lists both groups
function by function).

**The unmarked B list** (59), grouped by what they need:

| the group | n | the functions |
|---|---|---|
| the module head and the loader | 2 | `fn_29_100` `fn_29_130` (the host's `PortLoadIng` does the loader) |
| the Ing's own behaviour (the port has the body) | 24 | `fn_29_1C98` `fn_29_1DF4` `fn_29_3190` `fn_29_3684` `fn_29_4C88` `fn_29_514C` `fn_29_5190` `fn_29_51D0` `fn_29_53E0` `fn_29_5714` `fn_29_5C28` `fn_29_75C0` `fn_29_84A4` `fn_29_8CF4` `fn_29_8FE8` `fn_29_A4E0` `fn_29_AA04` `fn_29_AA4C` `fn_29_AA94` `fn_29_AC78` `fn_29_B534` `fn_29_B890` `fn_29_BF20` `fn_29_D540` |
| the host's own copies (the data block, the copies and the tables) | 20 | `fn_29_4AA8` `fn_29_BEB4` `fn_29_BFF0` `fn_29_E68C` `fn_29_E7C8` `fn_29_E8D8` `fn_29_E944` `fn_29_E9E8` `fn_29_EA7C` `fn_29_EB38` `fn_29_EBEC` `fn_29_EC88` `fn_29_ED38` `fn_29_EE68` `fn_29_EFC0` `fn_29_F000` `fn_29_F090` `fn_29_F12C` `fn_29_F260` `fn_29_F2C8` |
| the exit host effect and the mini portal classes | 13 | `fn_29_10D50` `fn_29_10E64` `fn_29_11438` `fn_29_11490` `fn_29_11570` `fn_29_11840` `fn_29_119E8` `fn_29_11A7C` `fn_29_11C14` `fn_29_11CA4` `fn_29_1201C` and the two mini portal constructors |

The Agon Wastes boot (`01_sand_entrance_dark`) reaches almost every one of them: its log shows
the states Alert, ArmSwipe, BecomeBodyProjectile, BecomeCorporeal, BecomeIngSpot,
BecomeWallProjectile, BodyProjectileFlight, Dead, Evaporate, ExitHost, FaceTarget, FailSafeMode,
FindCoverPoint, FindMiniPortals, FollowAttackPattern, GrappleBall, IngSpotPathFind,
IngSpotPointPathFind, Lurk, MiniPortalAttack, PathFind, Patrol, SafeZoneReaction, SeekWallPoint,
SelectTarget, SetCoverDest, SetPointCoverDest, SetRecoverDest, SetTargetDest, SuckEnergy, Taunt
and WaitForFSMTransition - so one boot covers the state bodies, the code functions and both
classes.

**`fn_29_82D4` (FollowAttackPattern) is marked**: the Agon Wastes run (the area
`01_sand_entrance_dark`, the world 42b935e4, the area 0x3f5cdf39) with a probe in the body ran
the three messages, the ing-spot form and the other forms, and the "the waypoint is not an AI
waypoint" arm; the waypoint's flag arms are dead (every attack pattern's target is a WAYP -
`tools/scan_ing_pattern.py`).

**The lure path needs a console command that makes a noise.** The Ing is lured by the
`kLNT_SafeZone` noise, which the host only sends from a safe zone of the "echo" type
(`CScriptSafeZone::Think`, src/MetroidPrime/ScriptObjects/CScriptSafeZone.cpp:229,
`mgr.InformListeners(GetTranslation(), (EListenNoiseType)6)`) - the Agon Wastes' safe zones are
normal ones, so the run never lured an Ing and `fn_29_1DF4`, `fn_29_51D0`, `fn_29_53E0`,
`fn_29_8CF4`, `fn_29_5C28`, `fn_29_A4E0`, `fn_29_AA94` and `fn_29_AC78` keep their arms open.
**The ask**: a console command `noise <type>` that calls
`CStateManager::InformListeners(playerPos, type)` at the player's position - the call
CScriptSafeZone.cpp:229 makes for the type 6 (kLNT_SafeZone) - so a run can lure an Ing and
reach the eight bodies above.

### A.26 The lure path with the console's `noise` (2026-10-21)

The orchestrator's console command `noise <type> [x y z]` (port 196d15fee) calls
`CStateManager::InformListeners` at the player's position (or at x y z). **It works: an Ing
reacts to it.** The Agon Wastes `01_sand_entrance_dark` (the world 42b935e4, the area
0x3f5cdf39) with `noise 6` (the kLNT_SafeZone) at the safe zone nearest the Ing:

* the noise's position has to be **inside** a safe zone - the manager finds the zone by the
  noise's position (`CScriptSafeZoneManager::PointIsInWhichSafeZone`) - and the Ing has to be
  within its hearing radius (40) of it, so the noise is placed at the safe zone's own position;
* the Ing's lure zone (0xd78) is set by the Listen's safe zone branch (fn_29_1DF4), and the FSM
  runs the lure code functions: `SetLuredDest` (fn_29_53E0) 64 times and `SetLuredPointDest`
  (fn_29_51D0) 44 times in that run.

The arms that ran, and the ones that did not:

| fn | the arms | the run |
|---|---|---|
| `fn_29_1DF4` | the safe zone the noise is in becomes the lure zone | ran (the lure zone was set) |
| `fn_29_51D0` | the point-graph route to the lure point | ran 44 times with the lure zone set; the arms inside the shared helper are not shown |
| `fn_29_53E0` | the lure point is the nav-mesh point below the lure zone | ran 64 times with the lure zone set |
| `fn_29_A4E0` | the flag at 0xe5c's bit 3 | the trigger was evaluated with the flag clear; the "the flag is set" arm did not run |
| `fn_29_AC78` | the lure zone id against kInvalidUniqueId | the trigger was evaluated with the id invalid; the "the id is set" arm did not run |
| `fn_29_AA94` | the word at 0xe2c is not zero | the trigger never ran: the count only changes with the XENZ message (the player entering a safe zone), and `noise 6` informs the listeners without changing it |
| `fn_29_8CF4` | the SafeZoneReaction state | the state did not run (the Ing took the lure code function, not that state) |
| `fn_29_5C28` | the Evaporate state | the state did not run |

So the eight bodies are reachable now, but every one of them keeps one arm open; the next lane
needs the "the lure zone is set" moments: the trigger lists that hold `fn_29_A4E0` and
`fn_29_AC78` are the lure code function's and the SafeZoneReaction state's, so the arms run when
the Ing is lured **and** the FSM evaluates that list in the same tick.

### A.27 The lure path, closed with the console's `noise` (2026-10-21)

The console's `noise 6` (the kLNT_SafeZone noise), repeated at the safe zone nearest the Ing
while the Ing stands next to it, **lures the Ing** and the trigger that reads the lure zone now
runs in both states. The Agon Wastes `01_sand_entrance_dark` (the world 42b935e4, the area
0x3f5cdf39) with the noise repeated every tick:

| fn | the arms | the run (a probe in the body) | marker |
|---|---|---|---|
| `fn_29_AC78` StillLuredBySafeZone | the lure zone id (0xd78) against kInvalidUniqueId | **both**: the id invalid (404 calls) and the id set (404 calls) - the lure zone is set and cleared again as the noise repeats | **marked** |
| `fn_29_1DF4` | the safe zone the noise is in becomes the lure zone | ran (the lure zone was set, and the lure code functions ran 64 and 44 times) | not marked (the body is the shared `PortIngLuredDest`, whose arms are the helper's) |
| `fn_29_51D0` | the point-graph route to the lure point | the code function ran 44 times with the lure zone set and 6 times without it | not marked (the arms are inside the shared `PortIngLuredPointDest`) |
| `fn_29_53E0` | the lure point is the nav-mesh point below the lure zone | the code function ran with the lure zone set and without it | not marked (the arms are inside the shared `PortIngLuredDest`) |
| `fn_29_A4E0` | the flag at 0xe5c's bit 3 | the trigger was evaluated with the flag clear; the "the flag is set" arm did not run | not marked |
| `fn_29_AA04` | the desired form is 3 and `!fn_29_AD40` | only the "the desired form is not 3" arm ran | not marked |
| `fn_29_AA4C` | the desired form is 2 and `!fn_29_AD54` | the "the desired form is not 2" arm and the "the desired form is 2 and the Ing is not corporeal" arm ran; the "already corporeal" arm did not | not marked |
| `fn_29_AA94` | the word at 0xe2c is not zero | the trigger never ran - the count only changes with the XENZ message (the player entering a safe zone) | not marked |

**What is still missing** for the rest: the XENZ message (the player entering a safe zone -
the console's `tp` into the zone does it) and the "the lure zone is set" moments for the two
triggers whose lists are the lure code function's and the Lurk state's.

### A.28 The arm coverage of the ported-but-unmarked bodies (2026-10-21)

The Agon Wastes `01_sand_entrance_dark` boot (the world 42b935e4, the area 0x3f5cdf39) with a
probe in every body and the states that the boot does not reach on its own forced with
`fsm <ing> <state>` (the forced arms are labelled as such).

| fn | the arms | the run | marker |
|---|---|---|---|
| `fn_29_84A4` Patrol | the message (asm 0x000084E0): the Activate and the other messages | both: the probe's keys 0, 1 and 2, four calls each (the state forced) | **marked** |
| `fn_29_514C` EndRangedAttack | no branch (one call: the attack type 1, the team id 0xd6a, the Ing's id and the last argument 0) | evaluated once | **marked** |
| `fn_29_5C28` Evaporate | the message (the Activate 0x5C48, the Update 0x5C54, the Deactivate 0x5C4C) and the state time against two seconds (0x5C84) | the three messages ran (the keys 0, 4, 8 - four calls each, forced); the two-second arm and the "the message is below zero" arm did not (the latter is dead: the state machine never sends it) | not marked |
| `fn_29_75C0` SeekWallPoint | the message (0x75DC, 0x75E8), the cover hint (0x75F8), the magnitude (0x7650) and the length (0x7640) | the three messages ran (the keys 0, 16, 32 - four calls each, forced); the hint, the magnitude and the length arms did not (the forced state leaves before the Ing moves) | not marked |
| `fn_29_8CF4` SafeZoneReaction | the message (0x8CFC, 0x8D10, 0x8D14, 0x8D20), the zone flags (0x8D64, 0x8D70) and the time (0x8D88) | the three messages ran (the keys 0, 8, 16 - four calls each, forced); the zone and the time arms did not | not marked |
| `fn_29_8FE8` BecomeWallProjectile | the message (0x9014, 0x9024, 0x9028, 0x9034, 0x9038), the cross product (0x90CC) and the body command (0x91BC, 0x91E4) | the three messages ran (the keys 0, 5, 9 - four calls each, forced); the cross product and the body command arms did not | not marked |
| `fn_29_B534` CollidedWith | the form (0xB558), the flight time (0xB56C), the empty list (0xB710), the contact that is not a splat surface (0xB720) and the one that is (0xB738) | the form is not the body projectile (the forms 2, 3, 5, 6, 7 and 8) and the form 4 (the body projectile, 7 hits in the Agon run of 2026-10-21) ran; the flight time under half a second and a contact that is a splat surface ran in the Agon run; the contact that is **not** a splat surface and the empty list did not - they need one forced call each (the collision list's count and the material list's value changed for one call under gdb) | not marked |
| `fn_29_B890` TakeDamage | the forms 0, 1 and 5 return (0xB894-0xB8AC) and the other forms store (0xB8B0) | the forms 3 and 4 ran (the Agon run of 2026-10-21; the coverage run: the key 4 with 6 hits); the early return (the forms 0, 1 and 5) did not - the hit in the host and the evaporating forms reached the vulnerability lookup but not TakeDamage: the trigger table takes nothing from the console's weapons (light, missile, dark, annihilator, power, cannonball and powerbomb were all tried). It needs a forced call: under gdb, make the vulnerability lookup return the ing spot's table (the form 3) so the damage gets through, and the hit then arrives in the form 0 | not marked |
| `fn_29_BF20` GetDamageVulnerability | the form 4 (0xBF34), the form 3 with the ball (0xBF50), the form 3 (0xBF64) and the other forms (0xBF6C) | the forms 0, 3 (with and without the ball) and 9 ran; the form 4 (the base table) needs a hit on the body projectile | not marked |
| `fn_29_5714` SetWallPointCoverDest | the hint (0x5748), the closest point (0x5784), the connected point (0x57C4) and the point path (0x57F0) | the probe has one label per arm (no hint or no area, no closest point, and the found/not found pair); the Agon run of 2026-10-21 did not reach the code function | not marked |
| `fn_29_4C88` the create handler | the data flag byte 0x9d0's bit 7 (0x4CAC and 0x4CC0): the flag set gives the ing spot (the form 3), the flag clear the corporeal Ing (the form 2) | **both arms**: the coverage run of 2026-10-21 (tools/cov, the key is the flag word) shows the flag set (the top bit, 180 hits) and the flag clear (0, 2820 hits) - the Ing 0x101c01c3 of the Great Temple's `06_swamp_boostballguardian_dark` is the one placement the disc scan found without the flag (the scan reads the raw flag word, the port's IngFlagsWord reverses the bits) | **marked** |
| `fn_29_5190` StartRangedAttack | no branch (one call) | never ran - the Ing never starts a team projectile attack | not marked |
| `fn_29_3684` the team leave | the id at 0xd6a (0x36AC), the TCastToPtr (0x36D0), the IsPartOfTeam (0x36EC) and the QuitTeam + id reset (0x3704/0x3710) | all three tests both ways in one run (A.33): Sand World `02_sand_sandfalls_dark` layer 5 | **marked** |

### A.29 The mini portal, read against the asm and marked (2026-10-22)

The eight `CIngMiniPortalAttack`/`CIngMiniPortalInfo` bodies in `PortCIngMiniPortalAttack.cpp`
(`fn_29_11570` aim 0x2D0, `11840` fire 0x1A8, `11A7C` open 0x198, `11C14` AcceptScriptMsg 0x90,
`11CA4` Think 0x1E8, `11E8C` ctor 0xF4, `12100`/`1201C` the info ctor/copy) were read against the
asm: no behaviour change was needed. Facts checked: the aim blend constants (`rodata_7E4` 0.5,
`7E8` 0.25, `7EC` 0.75, `7F0` 70 degrees), the translation at 0x54/0x58/0x5c and the forward at
0x28/0x38/0x48, `CPlasmaProjectile` slots 0x90 ResetBeam / 0x94 Fire / 0x8c UpdateFx, and bit 7 of
0x230 set from a constant 1 (`0x11DD8 li r3, 1` before the `rlwimi`), not from Fire's result.

Runs (coverage build, Agon `01_sand_entrance_dark`, `tools/drivers/ing_miniportal.py`: wake the
Ing, `place` it 10 units from the player, `fsm FindMiniPortals` then `fsm MiniPortalAttack`, with a
`fsm FaceTarget` first on odd rounds): the portals open, fire their beam at the aim time, delete
the beam at the lifetime and delete themselves, which delivers XDEL. The forced arms
(`tools/cov/specs/ing_miniportal_force.py`, `tools/cov/force.sh`):

| arm | how |
|---|---|
| `11840` @0x11874 no target | forced: `mInfo.mTarget` = invalid on every 3rd Fire; the next Think then finds no beam (@0x11CF4) and the portal deletes itself (@0x11E3C) |
| `11CA4` @0x11D80 target gone at the aim time | forced: `mInfo.mTarget` = invalid at the aim time, portals whose beam id is 2 mod 4 |
| `11570` @0x1168C travel speed not above 0 | forced: `travelSpeed` = 0 on every 4th aim made by Think |
| `11570` @0x11734 delta too short to normalise | forced: the delta handed to `IsMagnitudeSafe` from AimAtTarget zeroed on every 2nd call (aiming the portal at itself, so the blend is its own translation, did not take the arm in a forced run: the blend presumably misses the translation by a rounding step, above the 1e-28 floor) |
| `11A7C` @0x11B84 no explosion | dead on the host: `rs_new` does not return null |

Every other arm is natural, including both sides of the 70 degree test (@0x11750). Merged gcov:
all eight bodies ALL-ARMS, 0 `caught`/reach-stub.

### A.30 The unmarked behaviour rows, reclassified and marked (2026-10-22)

Of the 45 unmarked `B` rows, 15 are host copies the asm shows have no behaviour of their own;
`Ing_functions.tsv` now has them as `H host-copy`: `3424` (rstl placement construct: null check +
copy ctor), `4AA8` (a plain 0x4D-byte field copy), `BEB4` (a float copy of the PredictMotion
result), `BFF0` (only `CPatterned::PreRenderAllViewports`), `E68C` (deleting dtor, A.20), `E7C8`
(CIngData copy ctor), `E8D8`/`E944`/`E9E8`/`EA7C`/`EB38`/`EBEC` (sub-struct copy ctors), `EC88`/`ED38`
(vulnerability/vector copy, `__copy` + `allocate`), `F260` (rstl construct loop over `3404`).

Marked against the asm (13): the branch-free data ctors `EE68` CIngData (the flag byte's bit
reversal is IngFlagsWord), `EFC0` swarm, `F000` mini portal, `F090` grapple (retail passes the
vulnerability last, r7; each value lands in retail's field), `F12C` body projectile, `100` the
RELMain helper; and, with every arm run in the Agon coverage run (`tools/drivers/ing_triggers.py`,
gcov ALL-ARMS or branch-free; console `var <ing> ing.form|ing.desired|ing.found|ing.safezones`):

| fn | forced with |
|---|---|
| `AA94` ShouldFleeSafeZone | `send XENZ`/`XEXZ` + `trigcall` (false, true, false) |
| `A4E0` FoundMovementPos | `var ing.found 0/1` + `trigcall` |
| `AA04`/`AA4C` ShouldBecomeIngSpot/Corporeal | `var ing.desired -1/2/3` x `var ing.form 2/3` + `trigcall` (4/4 branches each) |
| `5190` StartRangedAttack | `codecall` |
| `B890` TakeDamage | `var ing.form 0/1/5/2/3`, then `hurt <joint> 1` (the joint relays kSM_Damage -> CollisionActorDamage) |
| `5C28` Evaporate | `fsm Evaporate`; the Ing deleted itself after two seconds |

Evaporate was also fixed: the two-second test reads the state machine's time (asm 0x5C68-0x5C78,
`lwz r3, 0x350` + vtable 0x30 GetTime), not the host's `mStateTime`, which LogState clears.

### A.31 The last behaviour rows: fixes, marks and blockers (2026-10-22, lane ing4)

Driver `tools/drivers/ing_rows.py` (Agon `01_sand_entrance_dark`, one Ing). Forced values go
through `var <ing> ing.hurtful|ing.evap|ing.takeoff|ing.form|ing.coverhint|ing.lurezone|ing.team`
chained with the call (`a; b` runs in one tick, so Think's ValidateLureZone and the cover-hint
release cannot undo the forced id in between). Every arm below is forced unless noted.

Fixed against the asm (unmarked rows too): `3190` no anim null test (0x31C4-0x3208); `8FE8`
Deactivate no anim null test (0x93F4-0x9410); `8CF4` the one-second test reads the state
machine's GetTime (0x8E48-0x8E68, `lwz r3,0x350(r31)` + vtable 0x30), not mStateTime; `75C0` length
test before `!IsMagnitudeSafe` (0x7640, 0x7650); `1DF4` hearing test `distSq > r^2` exits
(0x1E58-0x1E60 `cror eq,lt,eq; bne`); `3684` is now `LeaveTeam`: invalid id, TCast,
`IsPartOfTeam` (0x36EC), QuitTeam (0x3704), id reset (0x3710), from Delete (0xCC28) and
Deactivate (0xCCB8).

| fn | result | forced with / why not |
|---|---|---|
| `3190` CreateCollisionActorManager | marked, 14/14 | natural (kSM_Create) |
| `F2C8` FSM table init | marked, branch-free | its copy loop simulated: 49 triggers -> data_250, 29 states -> data_6BC, 12 code fns -> data_91C; names and order match `skTriggers`/`skStates`/`skCodeFuncs` |
| `1C98` lure mesh point | marked (on `PortIngLuredMeshPoint`), all arms | `ing.lurezone` invalid / hint / each zone + `codecall SetLuredDest|SetLuredPointDest` |
| `75C0` SeekWallPoint | marked, 9/10 | `ing.coverhint` + `statecall`, Ing placed on / far from the hint; the missing arm (`!IsMagnitudeSafe` after the length test) needs 0 < |delta|^2 < 1e-28 above dt*speed: dead |
| `53E0` SetLuredDest | open | `PathExists` to the lure point never succeeded (7 tries) |
| `51D0` SetLuredPointDest | open | Ing's own closest point never -1; the final point search's failure arm not run |
| `5714` SetWallPointCoverDest | open | the `to` closest point / the point search arms (3177, 3183, 3191) not run |
| `8CF4` SafeZoneReaction | open, 12/14 | the knock-back command never issued (CanIssueCommand false) and an unknown message not sent |
| `8FE8` BecomeWallProjectile | open, 20/25 | needs a floor surface (cross not safe), no aim target, an in-cone aim, and an unknown message |
| `3684` LeaveTeam | marked | the area does load a team: A.33, Sand World `02_sand_sandfalls_dark` layer 5 (no gdb force needed) |
| `1DF4` (in Listen `B294`) | open | the kept-nearer-zone arms did not run even with the zone set in the same tick; Agon's zones are likely not type 2 (Echo), which ValidateLureZone requires |
| `B534` CollidedWith | open, 1/10 | `fsm BecomeBodyProjectile` never produced a contact at >= 0.5 s |
| `10D50` ConstructEffect | open, 1/2 | the token-not-loaded arm not run |
| `10E64` CIngExitHostEffect::Render | skipped | the effect has no model, so it is never queued for rendering on the host |
| `130` loader / `D540` ctor | skipped | the generated reader's property switch (14 arms) and the DebugIng / no-model arms need other layers' data |

Reclassified `H host-copy`: `11490` (a plain `CModelData()` wrapper), `11438` (the trail
container copy-assign), `119E8` (CBeamInfo copy ctor, 0x48 bytes, after TLdrToBeamInfo at
0x1189C in `11840`).

The rows of this table that are still open were taken up by lane tail8 (A.34): `1DF4`, `53E0`,
`8CF4` and `10D50` are marked, `10E64` is proved dead on the host, and `5714` keeps four arms
(`from == -1`, `to == -1`, the search's failure, the host's `mPathArea` guard) that this area's
point graph cannot produce.

### A.32 FaceTarget fn_29_7244 rewritten from the asm (2026-10-22, lane leads1)

The port's body called IsFacingTarget (`<=`, NaN-true) and steered at the live target. Retail
(asm 0x7244-0x7518) reads the aim point once, on Activate (fn_29_4468 into 0xdc8, its z set to the
Ing's own at 0x72AC), and both angle tests are `fcmpo; ble Over` (0x7304/0x7308, 0x74A4/0x74A8):
a NaN angle gives Over. Update steers at the stored point; forms 2 / 3 and 6..9 / the rest; msg 2
is NotReady. Driver `tools/drivers/ing_facetarget.py` (Agon `01_sand_entrance_dark`, `tp` round
the Ing, `var ing.target|ing.targetpos|ing.form` + `statecall FaceTarget`, all forced); gcov 23/24.

| arm | result |
|---|---|
| Activate: no target / angle > 20 deg (Ready) / within (Over) | forced (`var ing.target 65535`; `tp` bearings) |
| Update form 2: CanIssueCommand false / command / unsafe delta | forced (`statecall 2` first; `ing.form 2`; `ing.targetpos 1`) |
| Update forms 3, 7: unsafe / FaceDirection / Over | forced (`ing.targetpos 1`; `tp` bearings) |
| Update forms 0, 4, 10: Over; Deactivate: NotReady | forced (`ing.form`) |
| msg >= 3 (the last `else if` edge) | no FSM sends it; not a retail arm (retail's `bge 0x74FC` does nothing) |

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

---

### A.33 LeaveTeam fn_29_3684 covered on an area that has a team (2026-10-22, lane tail7)

The row was open for "no `CScriptTeamAiMgr` in the area". A disc scan says otherwise: 80
area-layers place a TMAI and five of them also place an ing spot, so a team is reachable without
gdb forcing. Sand World `02_sand_sandfalls_dark` (area 25), layer 5 `AFTER`, is the one used:
`INGS` editor ids `0x14190138`/`0x14190139` each carry an `ACTV`/`PLAY` connection to the
`CScriptTeamAiMgr` `0x14190161`, so `JoinTeam` (fn_29_372C, through `GetAssociatedTeamId` ->
`CheckConnectedObject_if`) resolves a real team and the Ing joins it by itself at spawn.

The body is the asm 0x3684-0x372C: 0x36AC `cmplw` the id at 0xd6a against `kInvalidUniqueId`
(0x36B0 `beq` end), 0x36C4 `ObjectById`, 0x36C8 `TCastToPtr<16CScriptTeamAiMgr>`, 0x36D0 `beq`
end, 0x36E4 `IsPartOfTeam(0x8)`, 0x36EC `beq` end, 0x3704 `QuitTeam(id)`, 0x3710 `sth` the id
back to invalid. There is no message or remove call in it - the brief's "remove call" is the
QuitTeam. The port had the 0x36D0 and 0x36EC tests merged into one `||`; they are now two
returns (`src/MetroidPrime/Enemies/PortCIng.cpp:3516,3519`), as retail's two `beq`s are.

| arm | forced with | evidence (the spec's counts) |
|---|---|---|
| 0x36B0 the id is `kInvalidUniqueId` | `var <ing> ing.team 65535; send <ing> Deactivate` | `je` taken 1, the arm's own `ret` 1 |
| 0x36D0 `ObjectById` gives null | `var ing.team 1` (an id the area does not load) + Deactivate | `je` taken 1 |
| 0x36D0 the object is no team | `var ing.team` = the other Ing (u061b) + Deactivate | `je` taken 1 |
| 0x36EC a real team without the Ing | after the member leave, `var ing.team 1590` + Deactivate in the same tick | `je` taken 1, and the id stays 1590 (0x3710 does not run) |
| 0x3704/0x3710 the body | `var ing.team 65535; codecall <ing> SetTargetDest; send <ing> Deactivate` | QuitTeam arm 4, `ing.team = 65535` after the leave, and the arm ran once more on the Ing's own leave (`team at start 1590` -> 65535) |
| the `kSM_Delete` call site (0xCC28) | the second Ing, joined, `send <ing> Delete` | the same body ran, then "no object" |

8 entries: the invalid `je` 1 taken / 7 not, the no-team `je` 2 / 5, the not-member `je` 1 / 4,
so each test's both directions ran.

Forcing notes: the Ing re-joins the team in its own Think (its state runs SetTargetDest), so the
member leave and the not-a-member leave must be one console line - `a; b; c` runs in one tick -
with an `Activate` between them (the Deactivate handler needs `wasActive`, asm 0xCC9C). A leave
step across ticks finds the role restored and takes the member arm again (the first three runs of
this lane did exactly that).

Run: `tools/cov/run.sh tools/cov/specs/ing_team.py <out> "42b935e4,02_sand_sandfalls_dark"
--driver "python3 tools/drivers/ing_team.py {port} <out>/game.log" --fast --frames 12000 --env
MP2_DEBUG_ING=1`; the spec counts the arms at the emitted `je`s and the QuitTeam block
(symbol-relative offsets, `$eflags & 0x40` is ZF), because an optimised `return;` has no line of
its own. game exit 0, 0 `[port] caught`/reach-stub.

### A.34 The small unmarked class-B rows, arm by arm (2026-10-22/23, lane tail8)

Six rows of `Ing_functions.tsv`: `1DF4` (0x128), `53E0` (0x108), `5714` (0x10C), `8CF4` (0x1A0),
`10D50` (0x9C) and `10E64` (0x50). Driver `tools/drivers/ing_tail8.py`, spec
`tools/cov/specs/ing_tail8.py`, the Agon Wastes `01_sand_entrance_dark` (the world 42b935e4).
Three coverage runs (`/tmp/opencode/tail8_run1..3`, each ~35 s, `--fast`, frames 9000-14000); the
last is the one quoted.

Read against the asm, the four behaviour bodies needed **no change**: `53E0`'s flag-1 mesh query,
its kR_Success test (`cmpwi r3,0` / `bne`, 0x5470) and the 0x301 restore, `5714`'s two filters
(rodata_94 = 20 for both, the second's connected point = the first's index) and its point search,
`8CF4`'s message switch, forward knock-back and 0x350/GetTime second, and `10D50`'s single
ConstructChildParticleSystem call all agree with the port. What changed:

| where | what |
|---|---|
| `include/MetroidPrime/Enemies/PortIngShared.hpp:55-89` | `fn_29_1DF4` lifted out of `CIng::Listen` into `PortIngSafeZoneNoise` (behaviour identical; the asm turned into one call at 0xB3A8) so the body has a name to mark and to break on |
| `src/MetroidPrime/Enemies/PortCIng.cpp:3273-3290` (`53E0`), `:3217-3236` (`5714`), `:2005-2050` (`8CF4`) | arm addresses, the polarity of every test and the two notes that make the arms reachable (the knock-back needs an Activate first; the point search reads the area from the point search itself) |
| `src/MetroidPrime/Enemies/PortCIngExitHostEffect.cpp:42-56`, `:376-390` | `10D50`'s host `HasReference` guard called out as the host's (retail 0x10D50-0x10DC8 has no branch at all), and `10E64`'s dead-arm proof |

| fn | arms (asm) | the run | marker |
|---|---|---|---|
| `1DF4` | hearing exit 0x1E58-0x1E60 (cror eq,lt,eq; bne, a NaN exits too); unset 0xd78 takes the zone 0x1E84; held id no zone 0x1EAC-0x1EF4; held zone kept when not farther 0x1EE4/0x1EE8 (bge skips) | all: 38 out of radius / 57 in; 5 unset takes / 52 held; 23 not-a-zone replaces / 29 zone compares; 13 keeps / 16 nearer replaces | **marked** |
| `53E0` | no CScriptSafeZone at 0xd78 (0x1CD4); mesh point not found (1C98's height query); PathExists 0x5470 (5 success / 9 failure) | all: 46 no-zone / 28 zone, 14 mesh point found / 14 not, 5 found path / 9 not; the found arm needs the Ing *placed on* the lure zone (the same tick, `place` + `var ing.lurezone` + `codecall SetLuredDest`) | **marked** |
| `5714` | no hint 0x5748 (1); no closest point 0x5788; point for the hint 0x57C8; point search 0x57F4 | the no-hint arm ran (1 of 90); `from == -1`, `to == -1` and the search's failure arm did **not** (89/89 succeeded); the host's `mPathArea` guard arm did not either | not marked |
| `8CF4` | messages 0/1/2 and the unknown one (0x8CFC-0x8D24); hurtful 0x8D64; evaporating 0x8D70; CanIssueCommand 0x8D84 (both answers); the state time 0x8E48-0x8E70 | all: msg keys 4/202/4 and 3 unknown; hurtful 4, evaporating 136; CanIssueCommand 6 yes / 134 no; time over a second 1, under 61 | **marked** |
| `10D50` | branch-free (0x10D50-0x10DC8) | the ctor built both effects in the ExitHost run (2 entries) and one ConstructChildParticleSystem call each; the host guard's true side never ran (it is the host's, not retail's) | **marked** |
| `10E64` | no branch either; two CParticleGen slot 0x10 calls (0x10E80, 0x10E94) | dead on the host: 0 Render entries while the effect existed and ran its AddToRenderer 22 times. Nothing queues an actor's Render but `mgr.RenderLast*`, and this class's AddToRenderer (0x10DEC, no CActor call) submits only the two particle gens; CActor::AddToRenderer reaches Render only when the actor has a model, and the ctor's model is `CModelData()` (0x11200) | not marked (dead) |

Why `5714` stays open: `FindClosestPhysicalPoint` (upstream `CPathFindPointSearch.cpp`, the DOL's
own body) stores the index of the nearest candidate **whatever the distance**, so `from == -1` needs
an area with no points and `to == -1` needs one where even the Ing's own point is not connected to
itself; and the second query's filter tests `point -> from` while the search tests `from -> point`,
so its failure arm needs a one-way point link, which the 82 hints (each pair, cross-hint too) never
produced. Both remaining arms want a point graph this area does not have; the guard arm wants a
forced null `mPathArea` (`tools/cov/force.sh`).

Forcing notes: `var <ing> ing.lurezone` + `noise 6 x y z` in one line covers the four `1DF4` holds;
the "kept" arm needs the Ing standing *on* the held zone and the noise 8 units off it (equal
distances go the other way and store). `statecall <ing> SafeZoneReaction 0` immediately before the
Update is what makes the knock-back possible (0x6b4 must be kAS_Ready), and a second Update in the
same line takes CanIssueCommand's false answer. The evaporating arm deletes the Ing, so it runs
last, after `fsm ExitHost` has built the effect.

Run: `tools/cov/run.sh tools/cov/specs/ing_tail8.py <out> "42b935e4,01_sand_entrance_dark"
--driver "python3 tools/drivers/ing_tail8.py {port} <out>/game.log" --fast --frames 14000`; the
spec counts the arms at the emitted branch (symbol-relative offsets; `$eflags & 0x40` is ZF at a
`je`, `& 0x1` CF at the hearing test's `jb`, `& 0x41` at the `jbe`). game exit 0, 0
`[port] caught`/reach-stub.

### A.35 The last unmarked class-B rows (2026-10-23, lane ing30)

`51D0`, `5714`, `8FE8`, `B534` and the ctor `D540` marked; `10E64` reclassified T. Driver
`tools/drivers/ing30.py`, spec `tools/cov/specs/ing30_force.py` (`tools/cov/force.sh` on the gcov
build), the Agon Wastes `01_sand_entrance_dark`, one run of 16000 frames `--fast` after the last
edit (exit 0, 0 caught/reach-stub). Forced arms are labelled `forced`.

| fn | fix | arms (asm, real addresses) | run |
|---|---|---|---|
| `51D0` | host `area == nullptr` guard removed from `PortIngLuredPointDest` (also MinorIng `22B0`): with no area the first query finds no point and the body returns at 0x52A0 | lure mesh point 0x5244; `from` 0x52A0 (forced: filter flag 0x80000000); point flag 0x52CC, graph link 0x52E0, distance 0x530C (bge, NaN skips), PathExists 0x5324; `best` 0x534C; Search 0x5374 (forced: null source) | all both ways |
| `5714` | host `mPathArea == nullptr` guard removed (retail reads the area from the point search, 0x57D4/0x57DC) | hint 0x5748 (`ing.coverhint 0xffff`); `from` 0x5788 (forced); `to` 0x57C8 (forced); Search 0x57F4 (forced) | all both ways |
| `8FE8` | `kWallLaunchAngle` 0x3F490FDA -> 0x3F490FDB (rodata_104, 0x934C) | message switch; cross product 0x90CC (forced wall/floor triangle at Activate); CanIssueCommand / loop attack / neither (`bodystate 9`/`5`); take-off or form 4; aim target; cone 0x9358 (ble, NaN no clamp) | all |
| `B534` | `time < 0.5` -> `!(time >= 0.5)` (0xB56C fcmpo, cror eq,gt,eq; bne: NaN returns) | form 0xB558; time 0xB574; empty list; not a splat surface 0xB720 (forced: first contact's material zeroed); splat 0xB738 | all |
| `D540` | touch box engaged null box (0xD61C-0xD678); 0xdf0 = (0,0,1), 0xdfc = translation.z (0xE1D4-0xE2A4); `mLightBlend` 1.0 (0xE2D4); `mFoundMovementPos` true (0xE314); `xE5C_1`/`mTakeOff` initialised 0 (were uninitialised); search 0xd0 height = 5 * scale.z, 0xd4 radius = 3 * scale.y (0xE618-0xE630, were swapped); dropped the `[?]` SetMode(1) (0xca4 stored only at 0x9140/0x9830/0x9B6C/0x9D18) and the ray-filter override (none in retail) | the seven effect-token id tests 0xD68C etc. (forced: every second id invalid) | both |

`10E64` is T: vtable `data_C60` slot 0x2C. Retail `10DEC` AddToRenderer only calls gpRender
vtable 0x44 (AddParticleGen) twice, never CActor::AddToRenderer, AddDrawableActor or RenderLast;
the Ing asm has no RenderLast/AddDrawableActor reference. CStateManager reaches an actor's Render
only through RenderActorQueue (mRenderLast), RecursiveDrawTree (AddedToken set only by
AddDrawableActor/AddDrawableActorPlane, CStateManager.cpp:324/332) and RendererDrawCallback
(drawables from AddDrawableActor), so the slot is never dispatched (tail8: 0 Render vs 22
AddToRenderer).

Follow-up: MinorIng's ctor (`PortCMinorIng.cpp:386-410`, fn_44_9D58) carries the same SetMode(1),
height/radius order and ray-filter override the Ing ctor had; check it against its own asm.

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

1. `facePlateModel` is unused by Ing's loader and ctor - confirm no later read via `SLdrIng`.
2. Ing's dependency on GeomBlobV2's `CCollisionTracker` (`fn_29_4830` -> `fn_25_4290`) and that
   the host module link keeps that symbol global.
3. CIng vtable slot roles (`C654`, `C814`, `C9A4`) and the arm segment-name strings.
4. MinorIng's state / code-function split and the `22B0` entry.
