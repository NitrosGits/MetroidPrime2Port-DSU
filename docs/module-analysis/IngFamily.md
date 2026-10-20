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

**Ctor member map [proven from `fn_29_1105C`]**: CActor 0x158; 0x158 `CTransform4f` (copy of the
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

1. `CIngMiniPortalAttack` - the mini-portal attack actor. Its ctor (`fn_29_11E8C`), dtor
   (`fn_29_114B0`) and the info class (`fn_29_12100`/`121F4`) are ported; the actor's own methods
   are not: `fn_29_11570` (the aim helper, 0x2D0), `fn_29_11840` (fire, 0x1A8), `fn_29_11CA4`
   (Think, 0x1E8) and `fn_29_11A7C` (the effect, 0x198). The state is reachable: the Ing opens a
   mini portal when `ShouldOpenMiniPortal` fires (aggressive and the target in a safe zone), and
   `MiniPortalAttack` then asks the body for a projectile attack.
2. `CIngExitHostEffect` - the exit host's effect actor. `fn_29_1105C` (the ctor, 0x3DC), the
   bezier leg (`fn_29_10154`, `1046C`, `1065C`, `1077C`), the two particle systems (`fn_29_10944`,
   0x40C, plus `10DEC`/`10E64`/`10EB4`), the Think (`fn_29_10F58`) and the swarm emitter
   (`fn_29_1006C`/`100B8`). The state `ExitHost` is ported and marked; the effect actor is what it
   spawns.
3. The eight triggers no run has fired (A.15) - those need a scenario (a lure safe zone, an attack
   pattern, a host), not an audit.

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
