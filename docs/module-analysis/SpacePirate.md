# SpacePirate REL (module 72): analysis for a host body

Analysis only (2026-10-16). Sources: `config/G2ME01/rels/SpacePirate/symbols.txt` (sizes), the
archive build's disassembly `../MetroidPrime2Port/build/G2ME01/SpacePirate/asm/`
(`auto_00_00000000_text.s`, data/rodata `.s`), the DOL symbol names it calls, and MP1's
`../MetroidPrimePort/{include,src}/MetroidPrime/Enemies/CSpacePirate.*`.
Confidence tags: **[proven]** = read from an instruction/table entry; **[probable]** = strong
evidence (callee set + MP1 shape) but not proven; **[?]** = guess, verify before relying on it.
All `fn_72_X` below are REL `.text` offsets.

## 1. Function map

### Module head, vtable, statics

| fn | size | role | evidence |
|---|---|---|---|
| `_prolog`/RELMain | 0xB4 | calls `fn_72_D4` | [proven] |
| `fn_72_D4` | – | fills `lbl_72_bss_24` = `SSpacePirate_FuncPtrs` {loader `fn_72_140`, PMF `data_8C0` → `fn_72_E0D8` AttachActorToPirate(TUniqueId), PMF `data_8CC` → `fn_72_E0AC` DetachActorFromPirate}, then DOL `fn_80200E3C` (= `SetSSpacePirate_FuncPtrs`, struct 0x1C in `include/MetroidPrime/ScriptLoaderRel.hpp:246`) | [proven] |
| `_epilog`/RELExit | 0x90 | clears the registration | [proven] |
| `fn_72_140` | 0xACC | **loader** `LoadSpacePirate` (section 3) | [proven] |
| `fn_72_F49C` | 0xDBC | **CSpacePirate ctor** (section 2) | [proven]: called after `rs_new 0xc70` |
| `fn_72_F10C` | – | deleting dtor (vtable slot 2) → `fn_72_F1CC` | [proven] |
| `fn_72_F1CC` | 0x1E4 | dtor body: CToken×3, `fn_72_F3B0`, CBodyStateInfo/CmdMgr, CDamageVulnerability, CAi dtors | [proven] |
| `fn_72_10E50` | 0xB6C | static init: builds the trigger and state name→PMF tables below | [proven] (simulated) |
| `fn_72_F0A0` | – | `SetupStateMachine` (references both tables) | [probable]: slot position + refs |

Vtable `lbl_72_data_8D8` (82 words, 2 leading zeros; slots checked against
`include/MetroidPrime/Enemies/CPatterned.hpp` order). Overridden slots, all [proven] by slot index:

- CEntity: dtor `F10C`, PreThink `24C4`, **Think `1980` (0x6E0)**, **AcceptScriptMsg `E498` (0x944)**.
- CActor: PreRender `38E4` (0x128), Render `37C4` (0x120), PreRenderAllViewports `3A0C`,
  GetHealthInfo `64` (calls vtable slot 0x38 = `HealthInfo`; the archive's note that it returns a
  member directly is wrong), GetDamageVulnerability() `3AB4`, GetDamageVulnerability(pos,dir,dmg)
  `3A88`, GetTouchBounds `17C8`, Touch `E3D8`, GetSortingBounds `35CC` (0x100),
  DoUserAnimEvent `3B28` (0x254; jump table `data_A20`, falls back to `CPatterned::DoUserAnimEvent`).
- CAi: Death `C538`, KnockBack `C684` (0x358), TakeDamage `30` (stores skDamageHitTime into
  0x448 mDamageCooldownTimer), CanBeShot `5C` (true), IsListening `10` (true), Listen `E318`,
  GetOrigin `E090`.
- CPatterned: GetModelAlphau8 `3AE8`, ProjectileInfo `4F0C`, GetSearchPath `8` (→ this+0x920),
  GetPointSearchPath `40` (null), SetupStateMachine `F0A0`, GetRagDoll `17A0`, SetAttackTarget
  `E120`, GetAttackTarget `18` (u16 at 0xa94), IsOnGround `48` (bit in 0x34c), GetGravityConstant
  `24` (returns 50.0, same as MP1), IsScanVisorSelfRender `0` (true), GetScanVisorRenderBounds
  `343C` (0x190), ScanVisorRender `339C`, GetDeathExplosionParticle `54` (+0x754, base member),
  TryToBeCaptured `18BC`. Everything else is a base-class symbol.

### State machine (from `fn_72_10E50`; [proven] by table simulation)

Triggers (`lbl_72_data_1D8`, 39 × {name, 12-byte PMF}); functions ≥0x100 in bold:
Stuck C488, PatternShagged C468, HearShot BC78, HearPlayer BBC0, AggressionCheck ADC0,
CoverCheck ADB4, **CoverFind ABC0**, **CoverBlown AA64**, CoverNearlyBlown A988, CoveringFire A8D0,
**ShouldAttack A71C**, LineOfSight 9378, PatternOver BC94, SpotPlayer 938C, **ShouldDodge 82D0**,
**ShouldRetreat 7C28**, InRange 8A84, ShouldCrouch 99A4, ShouldMove 7BD4, ShotAt 7BA0, Attacked 7B6C,
**HasTargetingPoint 7970**, ShouldWallHang 767C, AnimOver 689C, **ShouldStrafe 960C**,
ShouldSpecialAttack 638C, StartAttack 6334, BreakAttack 6328, LostInterest 635C,
**BounceFind 602C**, OffLine 55B8, Landed 558C, ShouldJumpBack 686C, Leash 52F8,
HasAttackPattern 7810, IsAmbushing 7804, ShouldWarpIn 77D0, ShouldLaunchGrenade 779C,
InProjectileRange 76D0.

States (`lbl_72_data_604`, 37): Ambushing 7630, **WarpIn 7078**, **WarpOut 6E6C**, PostWarpOut 6DA4,
**Attack 9F1C**, Crouch 98D8, CoverAttack A310, Halt BBB0, Run BBA0,
**PathFind 8A90 (0x8E8)**, **Patrol BFCC**, **TargetPatrol BCB0**, **Shuffle 8670**,
**TurnAround 8464**, **Dodge 7DFC**, **Lurk AF70**, **Taunt B59C**, **Cover A3C0**, Dead 5C10,
TargetCover 78DC, TargetPlayer 7868, Approach 9478, **WallHang 69AC**, WallDetach 68E4,
**GetUp B3D4**, **Generate C1A4**, **Skid 94D0**, **DoubleSnap 99F0**, **JumpBack 63C8**,
**Bounce 5D0C**, **PathFindEx 5EA4**, Enraged A2B8, **Jump 5310**, Deactivate 5BF4,
**LaunchGrenade 58D8**, **Captured 5610**, RemoveFromWorld 5B88.
Names absent from MP1 (new behaviour): WarpIn/WarpOut/PostWarpOut, ShouldWarpIn, Bounce/BounceFind,
PathFindEx, Jump/Landed, LaunchGrenade/ShouldLaunchGrenade/InProjectileRange, Captured,
Deactivate, RemoveFromWorld, Enraged, Ambushing/IsAmbushing, HasAttackPattern, Stuck.

### Remaining functions ≥ 0x100

| fn | size | role | tag / evidence |
|---|---|---|---|
| `109C` | 0x2B0 | SLdr struct ctor (loader's stack object) | [proven]: first call of loader |
| `149C` | 0x304 | weapon-data sub-property reader (id 0xdc89cc3c) | [proven] |
| `D28` | 0x13C | copy ctor holding 3 CTokens (loader temp) | [probable] |
| `253C` | 0x310 | Think helper (attack timing) — MP1 `UpdateAttacks`? | [?] |
| `28F0` | 0x5B0 | Think helper (TeamAi / aim) — MP1 `UpdateAimBodyState`? | [?] |
| `2F4C` | 0x2D4 | cloak / electric visual update — MP1 `UpdateCloak` | [probable] |
| `3220` | 0x17C | squad add/remove/random — MP1 `Squad*` | [?] |
| `3D7C` | 0x2B4 | 2D vector + FastCos/Sin + sqrt; aim/arc geometry | [?] |
| `4030` | 0x144 | target pos from `CPlayer::GetEyeHeight` — MP1 `GetTargetPos` | [probable] |
| `4174` | 0x3B8 | spawns the "Bouncy Grenade" entity | [probable]: string ref |
| `45F8` | 0x4C0 | fires projectile (CBurstFire + LaunchProjectile) — MP1 `FireProjectile` | [probable] |
| `B9E0` | 0x184 | CPASAnimParmData builder | [proven] by callees |
| `C684` | 0x358 | KnockBack | [proven] vtable |
| `CB30` | 0x150 | sets `CBoneTracking` target to player — head look / `CheckTargetable` | [?] |
| `CC80` | 0x1B0 | Swoosh LCTR box vs player bounds — MP1 `CheckBlade` | [probable] |
| `CE30` | 0x470 | iterates CSpacePirates, OnPath, ray LOS, random — MP1 `GetStrafeDir` | [probable] |
| `D2A0` | 0x16C | OnPath + near-list ray test — MP1 `CantJumpBack` | [probable] |
| `D40C` | 0x1FC | MP1 `AvoidActors`: separation from other pirates -> locomotion cmd, steering delay | [proven] |
| `D608` | 0x118 | random normalized vector | [proven] by callees, user [?] |
| `D800` | 0x348 | guard position + `x8f7_28_noPlayerLos` (10 Hz LOS ray) — see §6 | [proven] |
| `DBD4` | 0x288 | near-list of CGameProjectile — MP1 `CheckForProjectiles` | [probable] |
| `E168` | 0x174 | sqrt + SetVelocityWR — MP1 `SetVelocityForJump` | [probable] |
| `EDEC` | 0x1E8 | `CScriptTeamAiMgr::IsTeamMemberInRange` user | [?] |
| `106D4` | 0x204 | CSpacePirateData copy into this+0x7c0 | [proven] |
| `10A10` | 0x254 | CSpacePirateData ctor (0x134) then `108D8` copies launcher data to +0xc8 | [proven] |
| `C0C`/`10C64` | – | bouncy-grenade data ctor (0x50) / grenade-launcher data ctor (0x6C, calls `10954`) | [proven] |
| `4F28` | 0x9C | ctor (0xFB30): weapon type 1 (data+0xc8) with a valid launcher CMDL (data+0xcc) -> `xc08_launcherModel = CModelData(CStaticRes(id, body model scale))` (`4FC4` = the optional's assign) | [proven] |

## 2. CSpacePirate members (`CHECK_SIZEOF 0xc70`)

Base: `CPatterned` (< 0x7c0) built with EPatternedAI 0x35, flavor 0, movement 0, collider 1,
body 1 [proven, ctor args]. Members up to 0xa94 sit at **MP1 offset + 0x258** (MP1 x568..x83c).
All offsets [proven] by a store in `fn_72_F49C` unless tagged.

| off | member | proof / note |
|---|---|---|
| 0x7c0 | CSpacePirateData (0x134) | `fn_72_106D4(this+0x7c0, data)` |
| 0x8f4.. | flag bits, unpacked from data+0x18 | rlwimi chain. 8f4_24 = `flags & 3` **(MP1: &1)** [proven mask, meaning ?]; 8f4_25..31 = 0x2,0x4,0x8,0x10,0x20,0x40,0x80; 8f5_24..31 = 0x200,0x400,0x1000,0x2000,0x4000,0x8000,0x10000,0x20000; 8f6_24 = 0x40000 (same masks/names as MP1 x634..x636). 8f6_25..8fa zeroed. mEnableAim = 8f7_25 |
| 0x8fc | frenzy frames = 0 | |
| 0x900/0x902 | cover point ids (TUniqueId) | = kInvalid |
| 0x904 | steering speed 1.0 | |
| 0x908 / 0x914 | forward vec / zero vec | |
| 0x920 | CPathFindSearch (0xF0) | GetSearchPath returns this+0x920 |
| 0xa10/0xa14 | 0 / 0 | |
| 0xa18 | = pInfo+0x58 | [?] initial HP or speed |
| 0xa1c | 0 | |
| 0xa20 | head CSegId ("Head_1") | |
| 0xa24/0xa28 | 0 / −1 | |
| 0xa2c | CBoneTracking (0x3C) on "Head_1", angles rodata C30/BC4 | |
| 0xa68 | −1 | |
| 0xa70 | intoJumpDist (anim distance) | |
| 0xa74 | eye height = 0.6 × bbox height | |
| 0xa78..0xa80 | zeros | |
| 0xa84 | attachedActor TUniqueId | Attach/Detach PMFs |
| 0xa86..0xa8c | seg ids: gun_LCTR, R_elbow, R_wrist, Swoosh_LCTR, L_hip, R_hip, Collar | |
| 0xa90 | attack remaining time 1.0 | |
| 0xa94 | attack target TUniqueId | GetAttackTarget |
| 0xa98 | CBurstFire (built from skBursts + firstBurstCount, DOL `fn_80138D3C`) | |
| 0xaf8 | jump height 3.0 | |
| 0xafc | patrol destination vec | |
| 0xb08..0xb18 | skidDir −1, 0, −1, TUniqueId, −1 | |
| 0xb1c/0xb20 | anim distances | |
| 0xb24/0xb28 | FLT_MAX ×2 | |
| 0xb2c | 0 | |
| 0xb30 | `rstl::single_ptr<CPirateRagDoll> xb30_ragDoll`, null (ctor 0xFA14) | built by Think's tail (0x1F08-0x2030) when the BecomeRagDoll delay 0xb2c runs out; reset `fn_72_2060`, deleting dtor `fn_72_20A8` (sp14, section 14) |
| 0xb34 | object via `fn_72_10568` (CIkChain?) | [?] |
| 0xb7c..0xbd0 | floats/vectors from data+0xa8..0xb0, +0xa0 (cloak, dodge-break, gun track delay) | |
| 0xbd4 | CProjectileInfo, built only if projectile asset valid | |
| 0xbfc/0xc54 | bytes; 0xc00/0xc58 = 0; 0xc04 = −1 | |
| 0xc5c | CUnitVector3f | |
| 0xc68 | 0 | |

Conditional ctor work [proven]: if flag bit 0x4 is clear, three anim-distance lookups; else a bit
in bodyController(0x48c)+0x56c is set. If pInfo+0x2a4 bit 0x80, builds a `CEchoEmitter` (0x70, DOL
`fn_802455CC`) with L_ankle/R_ankle/... segs and `SetEchoEmitter`. Flag 0x10000 clears bit 25 of
0x154. Sets a 0x682 bit; if `!HasBodyState(0x16)` sets 8f4 bit (0x10).

### CSpacePirateData (Echoes, 0x134) [proven by ctor `10A10` and loader stack offsets]

+0..0x14 six floats (aggressiveness, cowardice, …, hearing) · +0x18 flags · +0x1c bool (id 0xce67…) ·
+0x20 projectile asset · +0x24 projectile CDamageInfo (0x1C) · +0x40 u16 sfx · +0x44 blade CDamageInfo ·
+0x60 kneelChance · +0x64 kneel-shot asset · +0x68 kneel CDamageInfo · +0x84 dodgeCheck ·
+0x88 u16 impact sfx · +0x8c/0x90 intra-burst time/var · +0x94 (0x5080…) · +0x98 (0xc78b…) ·
+0x9c u16 alert sfx · +0xa0 gunTrackDelay · +0xa4 firstBurstCount · +0xa8 cloak · +0xac maxCloak ·
+0xb0/0xb4 breakDodge min/max · +0xb8/+0xba u16 hurled/death sfx · +0xbc (0x8708…) · +0xc0 avoidDistance ·
+0xc4 = 0.5 (constant B44) · +0xc8 launcher data (0x6C): {equippedWeapon, launcher asset,
+8 bouncy-grenade data 0x50 [mass, 0xed08…, CDamageInfo, numBounces, explosion×2, trail, effect,
u16 bounce/explode sfx, 0.1, 150, 0.1, 150 (constants B3C/B40), bit 0x4c = 1], +0x58 (0xa95a025b,
+the 58D8 volley count), +0x5c/0x60 launch speed min/max, +0x64/0x68 attack distance min/max}.
MP1's is 0xCC with an inline CProjectileInfo and no grenade block.

## 3. Loader `fn_72_140`

Format [proven]: u16 property count, then per property u32 id + u16 size; unknown ids are
skipped with `ReadBytes(nullptr, size)`. **37 top-level properties**, the same id set as
`include/MetroidPrime/ScriptLoader/SLdrSpacePirate.inc` (checked id by id); the weapon-data
property 0xdc89cc3c holds **16 sub-properties** (read by `149C`), matching
`LoadTypedefSpacePirateWeaponData`. The `.inc` already gives the types; stack offsets confirm the
field order: editorProperties 0x714, patterned 0x750, actorInfo 0x994, ingPossession 0xa0c, floats
0xb88… through avoidDistance 0xc28, weaponData 0xc2c. So the tree's SLdr structs are usable as-is.

Construction order [proven]: `LdrToModelData(editor scale, kInvalidAssetId,
patterned.animationInformation, true)` (`fn_8023A0B8`; returns null on an empty model →
loader returns null) · `LdrToDamageInfo` ×4 (`fn_8023B2F8`) · `LdrToPatternedInfo(patterned,
&ingPossession)` (`fn_8023A1D8`) · `rs_new(0xc70)` · `CSpacePirate(uid, name, LdrToEntityInfo,
LoadEditorTransform, modelData, LoadActorParameters, patternedInfo, CSpacePirateData&)`.

MP1 comparison: MP1 `LoadSpacePirate` (`ScriptLoader.cpp:1871`) reads positionally and builds the
data from the stream (`CSpacePirateData(in, propCount)`); Echoes builds it from the parsed SLdr
struct and adds the grenade launcher. Use the REL's sizes, not MP1's.

## 4. Reuse

- **Shape of the module**: `src/MetroidPrime/Enemies/CFlyingPirate.cpp:1961-2003` (loader on SLdr
  structs, `S*_FuncPtrs` static, `RELMain`/`RELExit`); SpacePirate additionally fills the two PMFs.
- **Port-only module registration and stand-ins**: `src/MetroidPrime/Enemies/PortCWallCrawler.cpp`
  (once-per-method `StandIn` log) + `tools/modules_port.txt` (switch the `SpacePirate` line from
  `PortModuleStubs.cpp` to the new source, rerun `tools/gen_files_cmake.py`).
- **MP1 `CSpacePirate`** (2985 lines): states/triggers shared by name (most of the list above),
  `CBurstFire` + skBursts (identical 12-table layout), cloak, squad, blade, dodge, cover, jump,
  `CPirateRagDoll`. Port offset-by-offset with the +0x258 rule.
- **Tree**: `CFlyingPirate.cpp` (pirate weapon/ragdoll idioms, Echoes API spellings),
  `CPirateRagDoll.cpp`, `CElitePirateGrenadeLauncher.cpp` (bouncy-grenade data types — compare its
  0x50 struct with ours before reusing).

## 5. Implementation order

1. **Milestone 1 (actor appears and stands)**: header with the member map above (opaque padding
   for unknowns, `CHECK_SIZEOF` 0xc70 only on the GC side; host uses `sizeof`), CSpacePirateData
   from the SLdr struct, loader + RELMain/RELExit + Attach/Detach PMFs, ctor (members, CBoneTracking,
   seg ids, CBurstFire), `SetupStateMachine` with only `Patrol`/`Halt` + `PatternOver`/`AnimOver`,
   every other vtable override forwarding to `CPatterned` behind a `StandIn` log. Verify: an area
   with a PIRT object logs the ctor + Think and the pirate draws.
2. Think/PreThink/AcceptScriptMsg (`1980`, `24C4`, `E498`) with MP1 bodies; Touch, Death, KnockBack.
3. Combat: FireProjectile/CBurstFire, Attack, TargetPlayer, blade, dodge.
4. Movement extras: PathFind(Ex), Cover*, Jump/JumpBack, WallHang, Taunt, Lurk.
5. Echoes-only: WarpIn/Out, LaunchGrenade + bouncy grenade (`4174`), Captured, Bounce, Ing possession.

## 6. Verified (2026-10-19)

Pseudo-C: `/tmp/opencode/pseudo/SpacePirate/fn_72_<off>.c` (m2c, 253/254; `3B28` DoUserAnimEvent
is a jump table, read its asm). Vtable slots: `docs/module-analysis/vtable-offsets.md`.

| Fact | Value / evidence |
|---|---|
| Test area | Agon Wastes `08_sand_casualpirates`, default layer: 6 CSpacePirate objects, start state `Patrol`. u0446 `SpacePirate-component`, u04be `SpacePirate-On Patrol`, u04c1 `SpacePirate-Already On`, u04d0 `SKIFF JUMPER` (= pirate id 208), u04d1 `Teleporting Pirate 1`, u04d8 `Teleporting Pirate 2`. `Already On` and `SKIFF JUMPER` are the passive pair (aggr/search/cover/hear all 0) - aggression tests want u0446/u04be/u04d1/u04d8 |
| Run | `tools/run_gameplay.sh` WORLD/TELEPORT=08_sand_casualpirates, GODMODE=1, `MP2_DEBUG_SPACEPIRATE=1` (state changes, KnockBack/Death/anim-event trace) |
| Damage test | console `damage <id> <n> [kb]`: hp 60 -> 55; kb 6 alive = reaction 1 (flinch); lethal = `Death`, `KnockBack dead reaction 4`, `Patrol -> Dead` |
| HP | 60 for all six at load. u0446 reads -1 because its flags are 0x10000 (`x8f5_30_floatingCorpse`): `kSM_AreaLoaded` sets `mAlive = false`, `HealthInfo()->SetHP(-1)` and drops Character/Solid/Target/Orbit (`PortCSpacePirate.cpp:921`). It is a scene corpse, not a loader fault |
| Slot 0x110 | `IsOnGround` (Landed/OffLine) |
| 0xb34 | `CIkChain` (`fn_72_10568` is its default ctor) |
| Body controller | retail activates it only in Think; the port also does in PreThink |
| Anim events handled | 19 BecomeRagDoll, 22 BreakLockOn (23/24 fall to CPatterned) |
| Aggro recipe | a console `damage` does **not** reset 0xb24 (`Attacked`): `CPatterned` sets `mHitByPlayerProjectile` only from a `CGameProjectile` sender (`kSM_Damage`/`kSM_ResistedDamage`, `CPatterned.cpp:300`/`310`) or a projectile `Touch`. Firing the power beam at a pirate (`tp`/`face` then `press a`) does: `208 state Patrol -> Taunt` |
| Taunt `B59C` | traced: msg 0 `hasTaunt 1`, `xa28` -1 -> 1 (`kTT_One`, right for this area: `mHearingRadius` 0 means no mate in range), `mEnableAim` 0 -> 1; exactly one `CBCTauntCmd(kTT_One)` delivered and body 5 -> **16 = `kAS_Taunt`**; msg 2 after the anim, `kTT_One` so no `InformListeners`, then `Taunt -> Lurk` and the Lurk/TargetPlayer/Run/PathFind cycle starts |
| Data floats | the MREA's own values, not a loader fault: every property is present with a 4-byte payload, so the SLdr defaults never apply; `hearingRadius` is 0 on all six actors, so `HearPlayer` cannot fire in this area (detail below) |
| Lurk `AF70` | traced: msg 0 rolls the three 0-100 percentages against `mData` (208 gives `aggr 0 cover 0 dodge 1`, right for its data: aggr/cover 0, dodge 80), `mEnableAim` 1, locomotion 3 = `kLT_Combat`, target kept from Taunt; msg 1 turns to the target through `CAnimationState` Ready -> body `kAS_Turn` -> Repeat -> Over and counts `xbac_holdPositionTime` (D720: 0 -> 7.5 s while the player stands still, reset when it moves); it exits `Lurk -> TargetPlayer -> Run -> PathFind -> Lurk`. and its `D800` (guard position / `x8f7_28_noPlayerLos`) is now ported |
| TargetPlayer `7868` | traced: Activate only (asm compares the msg with 0 and bails). `mDestObj` = `mgr.Player(0)`'s unique id (player + 0x8), `SetDestPos` = its translation (+0x54), `mReflectedDestPos` = its own translation, `mInPosition` false (byte 0x34C bit 0x80, cleared with `rlwimi ... 24,24`). One update long in the cycle: `TargetPlayer -> Run` |
| Run `BBA0` | 16 bytes with no message check: every call restores `x904_steeringSpeed` to rodata `B54` = 1.0, which Think feeds to `SetSteeringSpeedRange` when `xa10_steeringDelayTimer` is 0. `Run -> PathFind` after 1-3 updates |
| PathFind `8A90` | traced (state only; the jump-point branch was never taken here, no free off-path point in this area). Activate: cover point (`x900`) -> `mReflectedDestPos`/`mDestObj`/`mDestPos`; `Search(trans, destPos)` -> `mDestPos = GetPoint()`, `mInPosition` false, `CBCLocomotionCmd(dest - pos, zero, 1)`; with no path it scores the `kOL_AiWaypoint` list (object list 0x838) - active, `GetType() == 0`, `GetInUse(uid)` false, jump target invalid, same area, dist² > 25, forward·toJump > 0, height deltas agree - by `|toJump|² + 4z² + |dest-wp|² + 9z²` (+1000 when there is no path wp -> dest), and keeps the cheapest with `PathExists(pos, jp)`. Tail: steering blend 1 (FullSpeed), `x904` 1 when `mEnableAim`, `x8f9_27_inRange` false, `x8fa_24_normalDodge` true. Update: `CPatterned::PathFind`, `SetTargetVector(player - pos)`, `CBCJumpCmd(destPos, kJT_Normal, kJS_IntoJump, 0, kFF_AmbushJump)` and `x8fa_29_` once the jump point is inside `maxSpeed*((1.5dt+0.1)*scaleY) + xa70`, cover range `xa1c` (+`xa70` under `ShouldWallHang`) -> inRange. Deactivate: NotReady, jump point invalid, inRange false, blend 0 |
| `D800` UpdateGuardPosition | proven by trace: runs at 10 Hz off `xa80`, which is a **float** timer (the header had `int`; fixed) reset to 0 each cycle. Guard eye = `translation + z·xa74_eyeHeight`; with a cover point (`x900`) held the eye shifts ±2 along `GetTransform().GetRight()` per `xa68_coverDir` (0 = minus, 1 = plus, else none), in the open it is nudged `1.1 × Cross((aim - guard).AsNormalized(), up)`. The ray guard -> `player->GetAimPosition(mgr, 0)` excludes `kMT_Player` (0x20) and `kMT_ProjectilePassthrough` (0x14); a blocked ray resets `xa78_`, and `x8f7_28_noPlayerLos = xa78 < 0.5` (CSpacePirateData +0xc4) - so the bit means "sight lost within the last 0.5 s", not "cannot see the player". Target = the attack target when it casts to `CPlayer`, else `mgr.Player(0)` when `!IsMultiplayer` (`fn_80036F10`) |
| `D40C` AvoidActors | proven: walks the listening-AI list, same area, not itself; `CSteeringBehaviors::Separation` at this+0x61c with the other's translation and `mData.mAvoidDistance` (data+0xc0 = 10.0 here; asm passes `lfs f1, 0x880`); a safe separation becomes `CBCLocomotionCmd(normalized, zero, 0.5)`; then with both `xa10_steeringDelayTimer` 0, this one facing the other and the other moving along its own forward (`velocityWR`·forward > 0), `xa10 = 1`. Traced: 4 commands in a run, only when u04d8 comes inside the radius - it stands ~10 units up, so no visible swerve here |
| Retail symbol shapes | `fn_8014FFF0` = `CScriptAiJumpPoint::GetInUse(TUniqueId)`, `fn_800EC6B4` = `CScriptCoverPoint::ShouldWallHang`, `fn_801B73E4` = `CAnimationState::CanIssueCommand`; body controller +0x34 is cmdMgr `mTarget`, +0x4c its `mSteeringMode`; 0x6b4 is `mAnimationState`'s state word |
| Cycle movement | the cycle still repeats, but with PathFind real the pirate now walks to the player it targeted (over 12 s of watching after the Taunt, the player is pushed from 372.4,-284.1 to 377.9,-285.9 by the pirate's body); Lurk's zero steering and `D40C`/`D800` being unported keep the approach crude |
| 0xb28 | time since a player shot was near (ShotAt `7BA0`); header name `xb28_lowHealthFrenzyTimer` is wrong |
| Ported, unmarked | HearPlayer BBC0 (needs `mData.mHearingRadius > 0`, which is 0 in this area), SpotPlayer 938C, LineOfSight 9378 (0x8f7 bit 0x08), ShotAt 7BA0: no state these pirates reach (Patrol/Taunt) lists the last three, so nothing has exercised them. ShouldMove 7BD4 and ShouldAttack A71C (ported 2026-10-20): nothing in the live cycle lists them either |
| Triggers 7BD4 A71C 7C28 82D0 | ported 2026-10-20. `7BD4` = `GetCoverPoint(x900) && !ShouldStay()`. `A71C` casts the attack target to CPlayer (not an id compare) and returns false once 4+ same-area pirates that are alive and `x8f6_31_inAttackState` sit closer to that player. `7C28` gate `x8f6_29_enableRetreat && !x8f6_24_trooper`; waypoint from `GetConnectedObject(kSS_Patrol, kSM_Follow)`, else a connection-list scan resolved with `GetIdForScript`; `mDestObj` keeps the **GetConnectedObject** id even when the scan found the waypoint (0x330; then mReflectedDestPos, mInPosition, ReleaseCoverPoint, hearNoise, mEnableAim, SetHitByPlayerProjectile). `82D0` reads 0x8f4 bit 0x20 = **nonAggressive** (mFlags 0x4), as MP1 does — mFlags 0x20 is `x8f4_29_onlyAttackInRange`, bit 0x04 — then 0xb24/0xb28 < 0.33, 0xa7c < 0.5, else the CMetroid-`mIsAttacking` (0xa40 bit 0x04) front test |
| 2026-10-20 run (08_sand_casualpirates, 3200 frames, 65 state lines) | cycle repeats `Patrol -> Taunt -> Lurk -> TargetPlayer -> Run -> PathFind -> Lurk` 3×; `ShouldRetreat` and `ShouldDodge` evaluated 363× for 208 (1815× over all six ids), every one false — no xf6 bit or damage timer ever opens them here; `ShouldMove`/`ShouldAttack` 0× |
| ShouldStrafe 960C + `GetStrafeDir` CE30 | ported 2026-10-20. `960C`: gate `!x8f4_26_nonAggressive`, target ahead of `GetForward()`, `(xb28<0.66 \|\| xb24<0.66) && xb0c==0` then center-normalized dot > 0.707 -> `CE30(mgr, 10)` (0xB58); else `!result && xa7c>1` and a `CPlayer` attack target within 15 (0xB88) -> `CE30(mgr, 5)` (0xB78). `CE30` (retail's own, not MP1's CantJumpBack): other same-area `CSpacePirate`s drop a side (dot vs `GetRight()` > 0.866d² or >0 with d²<3, 0xC14/0xB5C), each survivor must pass `mPathFindSearch.OnPath(center ± dist*right) == kR_Success` then a `BuildNearList` (anchor `center` if left open else `center-dist*right`, length `2*dist` when both, `MakeInclude(kMT_Solid)` = data 0x8A0/0x3B) + `CGameCollision::RayDynamicLineOfSightTest(center, right, dist, …)` (returns clear); both open -> `CRandom16::Next() & 0x4000` drops left, else right. Returns 2 = `kSD_Left`, 3 = `kSD_Right`, -1 |
| Patrol `BFCC` | ported 2026-10-20, marked: Activate = `SetLocomotionType(kLT_Relaxed)` + `x904 = GetLocomotionSpeed(kLA_Walk)/GetLocomotionSpeed(kLA_Run)` (asm `fdivs` on the body's own speeds), Update = `UpdateGuardPosition(mgr, dt)` (D800 does read `f1` = dt), Deactivate = `kLT_Combat` + once-ever `kSS_Attack`/`kSM_None` send gated on `x8f9_31_sentAttackMsg` (`clrlwi. 31` / `rlwimi ... 31,31`). Traced: load state gives `state (none) -> Patrol`, and `Patrol -> Taunt` for 208, so Activate/Update/Deactivate all run. The `x8f7_24_enablePatrol` half (`CPatterned::Patrol`; steering 1 then 0 at body+0x4c; turn speed /1.25 and *1.25 from rodata `C10` = 1.25; `xafc_patrolDestPos = mWaypointNavigation.GetDestinationPosition()`) is asm-only: no pirate here has a `kSS_Patrol`/`kSM_Follow` connection, so the bit never sets (trace line `enablePatrol` in `AcceptScriptMsg`, 0 hits over 6 actors) |
| Dodge `7DFC` | ported 2026-10-20, unmarked (no state line in this area). MP1's `Dodge` with `TryCommand(TryDodge/TryRollingDodge/TryBreakDodge)` replaced by three direct `CBCStepCmd(dir, type)` deliveries behind `CanIssueCommand(kAS_Step)` (asm types 1/3/2 = `kStep_Dodge`/`kStep_RollDodge`/`kStep_BreakDodge`). Activate: clear `x8f9_29_enableBreakDodge`; while `!x8fa_24_normalDodge && !x8f5_29_noBreakDodge` and `xb94_dodgeDelayTimer <= 0` (asm `cror eq,lt,eq`, not `== 0` as the pseudo-C shows) roll `0.15*(1 + 4*(xa18_ - HealthInfo()->GetHP())/xa18_)` (BFC/B50/B54) and re-arm from `Range(mData.mBreakDodgeMinTime, mBreakDodgeMaxTime)` (0x870/0x874); then `xb18_dodgeDir = GetStrafeDir(mgr, break ? xb20 : xb1c)` and `kAS_Ready` only when it is not -1. Update: the 50/50 (B44) draw happens **before** the CanIssueCommand test; break-dodge also drops `kMT_Orbit` and calls `SetOrbitRequestForTarget(GetUniqueId(), kOR_ActivateOrbitSource, mgr)` once `mStateMachine->GetTime() > 0.5f`. Deactivate: `kAS_NotReady`, `x8f8_29_noPlayerDodge = true`, re-add the orbit material when missing. `kOB_ActivateOrbitSource` does not exist in this tree; the enum is `CPlayer::EPlayerOrbitRequest`, 8 = `kOR_ActivateOrbitSource` |
| Shuffle `8670` | ported 2026-10-20, unmarked (no state line in this area). MP1's `Shuffle` (MP1 CSpacePirate.cpp:2020) maps line for line except: target = `GetTargetPos` (CB30) not the player, no `behaviourOrient` write in Echoes, and the random vector is the module's `D608` (= MP1 `Random2f`, not in this tree: XY from two `Float()`-0.5 draws, first draw is y, x forced to 0.001 (BC8) under 0.001, z 0, `Normalize`, then a third draw lerps the length min..max). Activate clears the blend to normal, then either `TooClose(mgr, CTriggerData(0))` (gated on `x8f4_28_noShuffleCloseCheck`, `extrwi 1,28`) → dest = translation + `mMinAttackRange` (0x3dc) along `(trans-target).AsNormalized()` + `D608(mgr,0,5)` and `x8f6_30_shuffleClose` set, or dest = target + `mMaxAttackRange` (0x3e0) * `(rand+1)` along that vector + `2*mMax` * `(rand-0.5)` along `Cross(Up, trans-target)` (`fn_72_8A44` = `CVector3f::Cross`) and the bit cleared; both set `mDestObj` invalid and `x904` = 1. Then `CPatterned::PathFind` for **every** message, `CommandMgr().SetTargetVector(player - trans)` (cmdMgr+0x34, unguarded in retail), then Update `AvoidActors`, Deactivate clears the bit |
| TurnAround `8464` | ported 2026-10-20, **marked** 2026-10-21 (tools/cov; the no-safe-delta arm forced once, specs/spacepirate_force2.py). Activate: `xbc4_ = GetTargetPos(mgr)` (CB30), delta = that - translation with rodata `B9C` = 0 stored over Z, `Dot(GetForward(), delta.AsNormalized()) < rodata B64 = 0.8` → `kAS_Ready` (asm `bge` skips). Update: `mAnimationState.CanIssueCommand(body, kAS_Turn)`; the 3-D delta is **not** flattened, `IsMagnitudeSafe` → `CBCLocomotionCmd(Zero, dir, 1.0)` (zero `sZeroVector` stored first at 0x50, dir at 0x5c - same shape as Lurk's turn), else `kAS_Over`; both paths then `D800`. Deactivate: `kAS_NotReady`. Forward axis is `0x28/0x38/0x48` = `GetTransform().GetForward()`, translation `0x54/0x58/0x5c` |
| Halt `BBB0`, Approach `9478` | ported 2026-10-20, unmarked (neither gets a state line in this area). `BBB0` is 3 instructions with no message check: `x904 = 0` (rodata B9C) on every call. `9478`: Activate = blend `kSBM_Normal` (body+0x4c) + `x904 = 1` (rodata B54), Update = `AvoidActors`, no Deactivate case |
| TargetPatrol `BCB0` | ported 2026-10-20, unmarked (no state line in this area). Forwards to `CPatterned::Patrol` for **every** message (asm 0xBCE4, before the switch, unlike Patrol's gated enablePatrol half). Activate: `x904 = 1` (B54), cmdMgr `mSteeringMode = kSBM_FullSpeed` (body+0x4c), `GetConnectedObject(kSS_Attack 'ATTK', kSM_Follow 'FOLW')` -> `mWaypointNavigation.SetDestination` (**this+0x6c0 = nav+0x08**, not MP1's `mDestObj`), and `mInPosition` when `Dot(GetForward(), wp.trans - trans) == 0.f` exactly (0xBD90 `fcmpo/cror eq,lt,eq/bne`; the `|= 0x80` at 0xBDC4 is the nav `mInPosition` bitfield). Update (0xBDD8): `ObjectById(nav.GetDestination())` cast `CScriptAIWaypoint`; its `+0x164` flags bit1 (jump) or bit2 (drop) -> within `GetMaxSpeed()*((1.5dt+0.1)*scaleY) + xa70` sets `mInPosition` and `xaf8_jumpHeight = 3.f` (B5C) when bit1, else `0.f` (B9C); then when `BodyController()->GetCurrentStateId() == pas::kAS_Jump` (**`cmpwi 0xD` at body+0x37c = CBodyStateInfo `mState`**: the info base is 0x36c and `rstl::vector` is 0x10 there, so 0x37c is `mState`, not `mAdditiveState`) and the waypoint has no `kSS_Arrived`/`kSM_Next` connection, `CommandMgr().SetTargetVector(GetTargetPos(mgr) - trans)`; tail `xafc_patrolDestPos = mWaypointNavigation.GetDestinationPosition()` (0x6d4 = nav+0x1c). Deactivate: `mSteeringMode = kSBM_Normal` |
| Attack `9F1C` + `CheckBlade CC80` | ported 2026-10-20, unmarked (no state line in this area). `9F1C` is MP1's Attack with Echoes' members: Activate `kAS_Ready` (this+0x6b4 = 1), `kLT_Combat`, `xbc4_ = GetTargetPos`, `x908 = xbc4_ - GetBoundingBox().GetCenterPoint()`, `x904 = 0`, clear `x8f6` bit 0x20; `!x8f5_24_noMeleeAttack && TooClose(CTriggerData(0))` sets bit 0x20 + clears `x8f8` bit 0x40; the turn test (dot forward·`x908`.AsNormalized < 0.8 = rodata B64 -> `CBCLocomotionCmd(zero, x908, 1)`) runs **unconditionally**, not as MP1's else; then `x8f6` bit 0x01 and `xb90 = 0.75` (B84). Update's melee block (only while bit 0x20): `CanIssueCommand(body, kAS_MeleeAttack)` -> `DeliverCmd(CBCMeleeAttackCmd(pas::kS_One))` (asm delivers cmd 5 then fills the melee slot), `SetTargetVector(x908)`, `CC80`, and for `x8f5_27_shadowPirate` `mAlphaDelta` (CPatterned 0x40c) = -0.4 (rodata BB0) when this+0x6b4 == 3 else 1.0 + `xb90 = 0.75`; then always D800, D720. Deactivate clears bits 0x20/0x01 and this+0x6b4 = 0. `CC80` = MP1's CheckBlade (swoosh locator box = locator translation ± half the model scale, rodata B44 = 0.5, vs the attack target's bounds -> `ApplyDamage(this, target, this, data.mBladeDamage, MakeIncludeExclude(kMT_Solid, empty), zero)`; asm r7 = this+0x804 = data+0x44 = `mBladeDamage`); it **guards on `x8f8` byte bit 0x40** (0xCCA4 `rlwinm. r0,r0,26,31,31`, the header's `x8f8_25_appliedBladeDamage`, `extrwi. 1,25` = byte bit 0x40) and sets the same bit, as in MP1 (corrected 2026-10-20: an earlier reading here took the guard for `x8f8_26_alwaysAggressive`) |
| Crouch `98D8` | ported 2026-10-20, unmarked (no state line in this area). MP1's Crouch **minus** its `TargetPlayer` call: the asm's only two calls are `SetLocomotionType` and `GetCoverPoint`. Activate: `SetLocomotionType(kLT_Crouch)` (asm `li r4,0` - Echoes' 0 is `kLT_Crouch`, MP1's enum has Relaxed there), cover point -> `x908_targetDelta = cp->GetTransform().GetForward()` (cover +0x28/0x38/0x48), `x904 = 0` (rodata B9C), `xa68_coverDir = -1`; Update: `CommandMgr().SetTargetVector(x908)` (body +0x34/0x38/0x3c) then D800 with **r3 = this** (the body pointer goes to r5); no Deactivate (`cmpwi r5,1` plus `bge`/`b<0` fall to the epilogue) |
| CoverAttack `A310` | ported 2026-10-20, unmarked. Activate delivers a bare `CBodyStateCmd(kBSC_LeanFromCover)` (asm builds `{vtable, 0xB}` on the stack and calls `DeliverCmd` on body +0x4; 0xB is `kBSC_LeanFromCover`, and 0xD = `kBSC_AbortScripted` confirms the enum) and sets `x8f6_31_inAttackState` (`rlwimi ... 31,31` = 0x8f6 LSB). Update = D800 with r3 = this; Deactivate clears that bit; msg 3+ and negatives do nothing |
| TargetCover `78DC` | ported 2026-10-20, unmarked. Activate only (`cmpwi r5,0`, everything else bails before the work): cover point -> `mDestObj = x900` (`sth 0x330`) and `SetDestPos(cp +0x54)`; unconditionally `mReflectedDestPos = GetTranslation()` (`0x340..0x348`) and `mInPosition = false` (`rlwimi` clears 0x34C bit 0x80) |
| Cover `A3C0` | ported 2026-10-20, unmarked. MP1's `Cover` with Echoes inlining `TryCommand(mgr, kAS_Cover, &CPatterned::TryCover, x79c_coverDir)`: Activate bails while the body state (`lwz 0x37c`) is already 0x13 `kAS_Cover`, else takes the cover point, `xa68_coverDir = (cp->GetAttackDirection() >> 1) & 1` (asm `bl fn_800EC6D8` with the point in r3, `extrwi r0,r3,1,30`), `x6b4 = 1` (`kAS_Ready`), `SetDestPos(cp +0x54)`, and inside `CanIssueCommand(&x6b4, body, 0x13)` delivers `CBCCoverCmd(dir, cp translation, -cp forward)` (type 0x17, `{vtable, 0x17, dir, x/y/z, -fx/-fy/-fz}` on the stack, 0x24 bytes). Update repeats the guarded delivery, then inside the cover-point block `SetTargetVector(-cp forward)` (body +0x34), and calls D800 whether or not a point exists. Deactivate stores 0 into 0x6b4 (`kAS_NotReady`) |
| Skid `94D0` | ported 2026-10-20, unmarked. MP1's Skid: Activate `xb0c_strafeDelayTimer = 4.0` (rodata B50) + `x8f6_31_inAttackState`; Update delivers `CBCStepCmd(xb08_skidDir, kStep_Normal)` while the body state (0x37c) is not 3 `kAS_Step`; Deactivate clears the flag |
| GetUp `B3D4` | ported 2026-10-20, **marked** 2026-10-21 (tools/cov; lying forced once; the pending-death arm is dead: the DOL's Search(p, p) returns 0 or 1, 0x8013DD38). Activate: `x6b4 = 1`, `SquadReset` (`fn_72_DEDC`), `xbb0 = 0` (B9C) and `x8fa_25_` set (0x8fa bit 0x40 - MP1's GetUp has no such bit). Update: body state 2 `kAS_LieOnGround` **and** `mPathFindSearch.Search(trans, trans) == 2` `kR_NoSourcePoint` -> `SetPendingDeath(true)` (this+0x421 bit 0x10), else `CanIssueCommand(&0x6b4, body, 1)` -> `CBCGetupCmd(kGetup_Zero)` (bare type-0 command, zero payload); both paths then the leash timer. Deactivate: `x6b4 = 0` and clear `x8fa_25_` |
| Jump `5310` + leash `5280` | ported 2026-10-20, unmarked. `5310`: Activate `x6b4 = 1` + `xbb0 = 0` (B9C); Update inside `CanIssueCommand(&0x6b4, body, 0xD kAS_Jump)` sets `x8f7_30_jumpVelSet` (0x8f7 bit 0x02) and delivers the jump command that `5510` builds at `{vtable, type 0x11 kBSC_Jump, mType 0, mAnimationVariant 0, waypoint1 = mDestPos (this+0x334), waypoint2 = sZeroVector, mInitialState 2, mFacingFlags 2, mWallJump 0}` (2 is **both** `kJS_Loop` and `kFF_AmbushJump`, so the tree's ctor order is not in question), then the leash timer; Deactivate clears the bit and zeroes `x6b4`. `5280` = MP1's `UpdateLeashTimer`: `xbb0 += dt` while `GetPercentageFrozen() != 1.0` (B54, `fcmpu`+`beq`) and `!(body+0x5b8 > 0)` - 0x5b8 is `mElectrocutionDur`, i.e. `!IsElectrocuting()` |
| `5418`, `5510` | not states: `5418` is the out-of-line delivery of a 0x34-byte command (`DeliverCmd` by type, then the command copied to mgr+0x250..0x27C - the port's `CommandMgr().DeliverCmd(cmd)` covers it), `5510` the `CBCJumpCmd` builder above. `558C` (0x2C) is the `Landed` trigger (tail call through vtable slot 0x110 = `IsOnGround`), `52F8` (0x18) the `Leash` trigger (`xbb0_leashTimer > arg`, `fcmpo` + `extrwi` of CR bit 1 = cr0 GT) |
| WallHang `69AC` + WallDetach `68E4` | ported 2026-10-20, unmarked. `69AC`: Activate `x6b4 = 1`, `x8f7_29_inWallHang`, cover point -> the point's `kSS_Arrived`/`kSM_Next` connection resolved through `CheckConnectedObject_if(..., CValidEntityPredicate)` -> waypoint -> `mDestObj = waypoint->GetUniqueId()` (**waypoint +0x8**, where MP1 stores the `GetIdForScript` result), `SetDestPos`, `mReflectedDestPos`, `mInPosition = false`; then `x908 = cover forward` (still inside the cover branch), `x8f6_31_inAttackState`, `mBoneTracking.SetActive(false)`, `x8fa_30_`. Update: while `x6b4 == 1` and `GetAngleDiff(forward, x908) > 0xB F8 = 0.2618 rad (15 deg)` deliver `CBCLocomotionCmd(zero, x908, 1.0)` and skip the hang command; otherwise `CanIssueCommand(body, 0x14)` -> `CBCWallHangCmd(mDestObj)` (type 0x18); while body state `0x37c == 0x14` set `x8fa_30_ = !GetCurrentState()->UnkVtable2C()` (**CBodyState slot 0x28**, the tree's guessed name), and always `mBurstFire.SetBurstType(1)` (`stw 1, 0xa98` = CBurstFire's first field). Deactivate clears `x8f7_29`, `x6b4 = 0`, `x8f6_31`, `SetActive(true)`, `x8fa_30_`. `68E4` is 3 cases only: Activate sets `x8f7_29` + `x8fa_30_` + `x8fa_31_`, Update delivers a bare `CBodyStateCmd(kBSC_ExitState)` (type 0xA), Deactivate clears those three |
| JumpBack `63C8` + `CantJumpBack D2A0` | ported 2026-10-20, unmarked. `63C8` opens with `ShouldJumpBack(mgr, CTriggerData(0))` (686C) and returns on false. Activate bails when `x8f4_29_onlyAttackInRange` (0x8f4 bit 0x04) or `CantJumpBack(mgr, -forward, 5.0)` (B78) says no; then caches `GetSearchPath()->GetCharacterHeight()` (path +0xd0 = this+0x9f0), sets it to `5 + height`, builds `dest = translation + 10.0 (B58) * forward` and needs `Search(translation, dest) == kR_Success`, `|back() - dest|² < 3.0` (B5C) and `|RemainingPathDistance(translation) - 10.0| < 4.0` (B50) before `xafc_patrolDestPos = back()`, `xaf8_jumpHeight = 5.0`, `x8f9_25_useJumpBackJump`, `x6b4 = 1`; the height is restored on every exit. Update: bit clear -> `CBCStepCmd(kSD_Backward, kStep_Normal)` + `SetTargetVector(GetTargetPos - trans)`; bit set -> `CanIssueCommand(body, 0xD)` -> the `5510` jump command with `(mDestPos, kJT_Normal, kJS_IntoJump, 0, kFF_AmbushJump)`. Deactivate: `x6b4 = 0` + clear the bit when set, and always `xbac_holdPositionTime = 0`. `D2A0` is **not** MP1's three-LOS CantJumpBack: `OnPath(centre + dist*dir)` (centre = bbox centre) **returning success short-circuits to false**, and only off-path does it build a `CMaterialList(kMT_Solid)` filter (data 0x89C = 0x3B), `BuildNearList(centre, dir, dist, filter, this)` and return the **raw** `RayDynamicLineOfSightTest` byte (1 = clear) - retail's polarity, unproven here |
| DoubleSnap `99F0` | ported 2026-10-20, unmarked. MP1's DoubleSnap with Echoes' members. Activate: `kAS_Ready` only when `!x8f5_24_noMeleeAttack`, `xbc4_ = GetTargetPos` (CB30), `x908 = xbc4_ - translation`, `x904 = 0`, `x8f6_26_enableMeleeAttack`, `xb10_meleeSeverity = kS_One`, `x8f8_25_appliedBladeDamage` cleared, `x8f6_31_inAttackState`, `x8f9_30_closeMelee` cleared, then `fn_72_9E64(&bss_40, &uid)` = `mChargePlayerList.remove(GetUniqueId())` (**a module-level static `rstl::list<TUniqueId>`**, MP1's `static rstl::list<TUniqueId> mChargePlayerList`; its only other caller is ChooseTarget's EDEC path). Update: `CanIssueCommand(body, 7)` -> `CBCMeleeAttackCmd(xb10_meleeSeverity)`; then while `xb10 == 1 && x6b4 == 3` the second hit needs `|target - trans|² < mMinAttackRange²` (0x3dc) and `Dot(delta.AsNormalized(), forward) > -0.123` (rodata C08) -> `x6b4 = 1`, `xb10 = 2`, apply bit cleared, `x908 = delta`, `closeMelee`; the closeMelee branch re-reads the target delta, `SetTargetVector(x908)`, the shadow variant (0x8f5 bit 0x10) sets `mAlphaDelta` (0x40c) to -0.4 (BB0) at `x6b4 == 3` else 1.0 (B54) + `xb90 = 0.75` (B84); tail D800, D720, CC80. Deactivate clears 0x8f6 bits 0x20/0x01 and `x6b4 = 0` |
| Enraged `A2B8`, Deactivate `5BF4`, RemoveFromWorld `5B88`, Ambushing `7630`, PostWarpOut `6DA4` | ported 2026-10-20, unmarked (small Echoes-only states). `A2B8` (0x58): Activate delivers a bare `CBodyStateCmd(kBSC_ExitState)` (type 0xA). `5BF4` (0x1C, seven instructions, no switch): Activate only -> `SetPendingDeath(true)` (this+0x421 bit 0x10, the same bit `GetUp` sets). `5B88`: Activate -> `SetActive(false)` (vtable slot 0x1C) then `mgr.DeleteObjectRequest(uid)`. `7630` (0x4C): Activate -> `SetLocomotionType(x8f4_25_ceilingAmbush ? 6 : kLT_Crouch)` (`extrwi. 1,25` at 0x7648 = byte bit 0x40 = data flag 0x2; corrected 2026-10-20 from `x8f4_30_`; 6 has no name in the tree: `pas::kLT_Internal6`). `6DA4` (0xC8): Activate -> unless `x8fa_26_` (0x8fa bit 0x20) post `CScriptMsg(uid, uid, kSM_Deactivate)` (0x44435456) to itself via `mgr.DeliverScriptMsg`, always `xc04_warpPoint = -1`, and `DeleteObjectRequest` when `x8fa_27_` (0x8fa bit 0x10) |
| WarpIn `7078`, WarpOut `6E6C`, Generate `C1A4` | ported 2026-10-20, unmarked. `6E6C`: Activate `x6b4 = 1`, `xc04_warpPoint = 1`, clear 0x8fa bit 0x08; Update `CanIssueCommand(&x6b4, body, 0xC)` -> `CBCGenerateCmd(kGType_One, Zero, false, false)` (type 0xF, `{1, zero, -1, flags 0}`), else while the body state (0x37c) is 0xC it arms 0x8fa bit 0x08 + `xbd0` = `GetAnimTimeRemaining()`, then `mColor.SetAlpha(min(1, remaining/xbd0))` (rodata B54 = 1, the `fmuls` by BB8 = 255 is `CColor::SetAlpha`'s own ToUint8, whose GQR is `OS_FASTCAST_U8`); Deactivate `SetAlpha(0)` + `mAlphaDelta = 0`. `7078`: Update sets `x6b4 = 1`, alpha 0, `mAlphaDelta` 0, `xc04_warpPoint = 0`, clears 0x8fa bit 0x08 and `RemoveMaterial(Character, Solid, Target, Orbit)`; Activate switches on `xc04_warpPoint`: at 0 it reserves `GetConnectionList().size()`, pushes every `kSS_GRNT`/`kSM_Follow` connection's `GetIdForScript` -> `CScriptWaypoint`, teleports to `points[Random()->Range(0, n-1)]` via `SetTranslation` + `SetTransform(CQuaternion::FromMatrix(wp->GetTransform()).BuildTransform4f(GetTranslation()))`, then `BuildNearList` over a +/-10 box (B58) filtered `MakeInclude(Solid, Player, Character)` and `DetectDynamicCollisionBoolean(*GetCollisionPrimitive(), GetTransform(), ...)`: clear -> `xc04_warpPoint = 1` + `AddMaterial(Character, Solid, Target, Orbit)`; at 1 the same generate command + fade-in `SetAlpha(max(0, 1 - remaining/xbd0))`; Deactivate `kAS_NotReady`, those four materials back, alpha 1, `xc04_warpPoint = -1`, clear 0x8fa bit 0x20. `C1A4`: Activate delivers `CBCJumpCmd(xafc_patrolDestPos, x8f4_25 ? kJT_Ambush : kJT_Normal, kJS_IntoJump, 0, kFF_AmbushJump)` while `CanIssueCommand(&x6b4, body, 0xD)`; the ceiling variant (`x8f4` bit 0x40) then aims at `translation + sDownVector` with `xaf8_jumpHeight = 0` and skips the rest, else `GetConnectedObject(kSS_Attack, kSM_Follow)` -> `CActor` gives the destination with height 3 (B5C) and `kAS_Over` is the fallback, then `kLT_Combat`; tail `if (x6b4 == 2) SetLocomotionType(kLT_Combat)` (2 = `CAnimationState::kAS_Repeat`) and always `SetTargetVector(GetTargetPos - trans, z = 0)`. Update sets 0x8f7 bit 0x40 (`mEnableAim`), `AbortScriptedAnimations`, `SquadJoin`, the once-ever `kSS_Attack`/`kSM_None` send on `x8f9_31`, `x6b4 = 1`, then the same ceiling-or-connection destination choice. Deactivate `kAS_NotReady`, `kLT_Combat`, `xaf8_jumpHeight = 3`, clear 0x8f4 bit 0x40, `mBoneTracking.SetActive(true)` + `SetTarget(mAttackTarget)` |
| Bounce `5D0C`, PathFindEx `5EA4`, Captured `5610` | ported 2026-10-20, unmarked. `5D0C`: Activate -> `mStateMachine->GetTime() > 0.1` (rodata B3C) and body state not 0xD = `kAS_Jump` -> `StateMachineState().SetCodeTrigger()` (the asm sets bit 0x80 of state-machine +0x3c, i.e. the `mCodeTrigger` bit). Update: `xb14_jumpPoint` -> `CScriptAiJumpPoint`, its `GetJumpTarget()` (+0x182 = `mNextWaypoint`) -> `CScriptWaypoint`, then the two-waypoint `CBCJumpCmd(xafc_patrolDestPos, wp->GetTranslation())` built by `5E28` (type 0x11, `kJT_Normal`, variant 0, `kJS_IntoJump`, `kFF_AmbushJump`, **wall jump set** - the tree's 2-arg ctor exactly). `5EA4`: `CPatterned::PathFind` runs for **every** message before the switch; Activate -> `AvoidActors`, early-out when 0x8f9 bit 0x10 (`inRange`) is set, else `xa1c = maxSpeed*((1.5dt + 0.1)*scaleY) + xa70` (BEC/B3C as in `8A90`) and `inRange = |jumpPoint - trans|^2 < xa1c^2` (`mfcr`/`rlwimi 5,27,27` copies CR0[LT]); Update and Deactivate clear the bit. `5610`: Update -> `mDisabledAnimationDeltas = 3` (the 2-bit field in 0x422), `Stop()`, `x6b4 = 1`, `CBCGenerateCmd(kGType_Seven)` (type 0xF, `{7, zero, -1, flags 0}`), `RemoveMaterial(Orbit, Target, Solid)`, `SetDrawShadow(false)`; Activate delivers the same command behind `CanIssueCommand(&x6b4, body, 0xC)`; Deactivate `x6b4 = 0` |
| 0x6b4 | is a **`CTryCommand`**, not `CAnimationState`: the reloc at fn_72_9F1C 0xA130 is `ShouldRetry__11CTryCommandFRC15CBodyControlleri` (the tree's `CAnimationState::CanIssueCommand` is the same function under an MP1-derived name, so the ported calls stand). Its 4-byte state word is `CAnimationState::EState`: 0 NotReady, 1 Ready, 3 Over |
| 0x6d4 | not `mDestPos` (that is 0x334): `mAnimationState` is 4 bytes at 0x6b4, `CWaypointNavigation` (0x3c) starts 0x6b8, its `mDestinationPosition` is +0x1c; `TargetPatrol`'s post to 0x6f0 = nav+0x38 = `mInPosition` confirms the base |
| 2026-10-20 run #3 (same area, 3200 + 1200 frames) | after Patrol: cycle still `Patrol -> Taunt -> Lurk -> TargetPlayer -> Run -> PathFind -> Lurk` 4x, 0 reach-stubs, player pushed 372.4,-284.1 -> 377.9,-285.9 |
| 2026-10-20 run #2 (same area, 3200 frames) | `ShouldStrafe` evaluated 484× for 208, all false — the 0.66 s windows and the `xa7c>1` branch never open, so `CE30` is never reached; cycle now repeats 4×; 0 reach-stubs |
| The 12 trigger stand-ins (`ADC0` `ABC0` `AA64` `A988` `A8D0` `99A4` `7970` `767C` `602C` `55B8` `558C` `52F8`) | ported 2026-10-20, **no stand-in is left in the module**. Three of them are marked because the 208 cycle evaluates them (all always false there): `ADC0` 2851 calls, `7970` ~2850, `55B8` 2980; the other nine are never evaluated and stay unmarked. Retail facts: **`ADC0`** = `!x8f4_26_nonAggressive` (0x8f4 mask 0x20) and (`x8f8_26_alwaysAggressive` (0x8f8 mask 0x20) or (the module's charge list `bss_40` empty and `xa7c_timeNoPlayerLos > 10` (rodata B58))); when it is true the pirate's id is **pushed** into that list if it is not in it - the asm walk stops at the list's tail sentinel, so "reached the tail" means "not found" and `fn_72_AEB8`->`fn_72_AEE0` is a push_back (m2c prints the walk inverted). **`ABC0`** = the `kOL_AiWaypoint` list (`mgr+0x838`) scanned for cover points: active (0x20 bit 7), `!ShouldLandHere` (fn_800EC6A8), `!GetInUse(this uid)` (fn_800EC550), same area, not the cover point in 0x902, closer than `mData.mSearchRadius` (**0x7c8** = data+0x08) squared, and `!Blown(GetTargetPos())` - **fn_72_CB30, not the player** (MP1 uses the player); the winner is taken with `ReleaseCoverPoint(retainCooldown=1)` + `SetCoverPoint`, `x902 = x900` and `x914 = -cover forward`. **`AA64`** = the target is `GetTargetPos()`; inside `mMinAttackRange` (0x3dc) the trigger is true, outside it the cover point's `Blown(target)` decides, and a cover that is not blown but more than `3.0 * scaleY` away (the model's Y scale at 0x60, **not squared**) also counts as blown (the gate is `x904_steeringSpeed == 0` and the body state != `kAS_Step`). **`A988`** = the attack target cast to **CPlayer** (not `mgr.GetPlayer()`), the position + `1.0 * GetVelocityWR()`, then `Blown`. **`A8D0`** = the `kOL_ListeningAi` list (`mgr+0x830`) scanned for a pirate with `x8f6_31_inAttackState` (0x8f6 mask 0x01) in the same area, with no early break. **`99A4`/`767C`** = the cover point's `ShouldCrouch` (fn_800EC6CC, returned unmasked) / `ShouldWallHang` (fn_800EC6B4, tested on the top byte). **`7970`** = the attack target cast to CActor and then CPlayer: an active non-player target returns true right away; otherwise, with no player target, `mAttackTarget = ChooseTeamTarget(mgr)` (fn_72_EFD4 returns the id), `UpdateTeamTarget(mgr)` (fn_72_DE5C) and `mBoneTracking.SetTarget(mAttackTarget)`; then a CAABox around the translation with the margin `mData.mSearchRadius * 1.0` and the filter `MakeExclude(CMaterialList(kMT_Solid))` (rodata `data_8A8` = 0x3b - the port's `GetStrafeDir` uses the include form, a different word), and the first targeting point that is active, in this area and not locked (fn_8012CB70) becomes the target (the asm **breaks** there). **`602C`** = the `kOL_AiWaypoint` list scanned for jump points: active, `!GetInUse(this uid)`, type (0x15c) == 0, a next waypoint (0x182) that is not invalid, same area, closer than the best, and its forward pointing along the way to it; the score is `distSq + |mDestPos (0x334) - nextWaypoint|^2 + 9*dz^2` (the asm adds the 9*dz^2 on top of the dz^2 inside the square, so the height weighs 10), the next waypoint's forward must point along the way to `mDestPos`, `PathExists(this -> jumpPoint)` must succeed and `PathExists(nextWaypoint -> mDestPos)` costs +1000 when it fails; the scan **breaks** at the first candidate that reaches the destination and beats the best; the winner's *current* waypoint (0x180) must still resolve, then `SetDestPos(jumpPoint)`, `xb14` = the jump point's id, `xaf8` = its jump apex (0x158) and `xafc` = the current waypoint's translation. **`55B8`** = false while the body state (0x37c) is `kAS_Jump`, otherwise `!IsOnGround()` (the vtable slot 0x110 call, tested on the top byte). **`558C`** = the vtable slot 0x110 call (IsOnGround), returned unmasked. **`52F8`** = `xbb0_leashTimer > the trigger's float` - `extrwi r3, r0, 1, 1` after `fcmpo cr0` + `mfcr` takes CR bit 1, **cr0's GT bit** (LT 0, GT 1, EQ 2), the same rule as MP1 (an earlier reading here called it the equal bit; corrected 2026-10-20). Every one of the six actors' property blocks carries
`9579b1f2` aggressiveness, `f89ab419` coverCheck, `ed9bf5a3` searchRadius, `c3a27cf8` fallBackCheck,
`f0cf5dd7` fallBackRadius, `ed69488f` hearingRadius and `a64ab9b8` flags, each with a 4-byte payload,
so the `SLdrSpacePirate` ctor defaults (10/20/20/50 - confirmed printed before the read) never apply.
Loaded aggr/cover/search: `component` 50/50/20, `On Patrol` 20/50/20, `Already On` and `SKIFF JUMPER`
0/0/0, both `Teleporting Pirate` 50/50/20; `hearingRadius` is `0x00000000` on all six, so `HearPlayer`
cannot fire in this area (for retail as well). The decode is exact, not a mis-read: the ids match the
generated reader and `fn_72_140` id for id, `ReadFloat`/`ReadInt32`/`ReadUint16` all byte-swap under
TARGET_PC, and the properties after them (42-byte damage infos, 200-byte weapon data, 90-byte editor
properties, per-actor flags 0x10000/0x829c/0x40000) line up.
| `E0D8` `AttachActorToPirate` | ported 2026-10-20, unmarked. Refuses a second actor (0xa84 != invalid, 0xE0E4 -> false at 0xE118), sets the ragdoll's flag byte 0x118 bit 0x40 (PPC bit 25, 0xE100) and stores the id at 0xa84 (0xE108/0xE10C). sp14: the 0x118 0x40 write is `SetActorAttached(true)`; all arms forced (`pirateattach`, section 14) |
| `E0AC` `DetachActorFromPirate` | ported 2026-10-20, unmarked. 0xa84 = kInvalidUniqueId (0xE0B4) and the ragdoll's flag byte 0x118 bit 0x40 cleared (0xE0CC) ; sp14: `SetActorAttached(false)`, all arms forced (`pirateattach`, section 14) |
| `3AB4` `GetDamageVulnerability()` | ported 2026-10-20, unmarked. 0xc04 != -1 (0x3AC0/0x3AC4) -> the DOL's pass-through vulnerability (0x3ACC `PassThroughVulnerabilty`), else `CPatterned::GetDamageVulnerability()` (0x3AD4); 0xc04 is the warp point |
| `3A88` `GetDamageVulnerability(pos,dir,dmg)` | ported 2026-10-20, unmarked. A tail call through the vtable slot 0x40 (0x3A94-0x3AA0), which for CSpacePirate is the **no-argument** override `3AB4`; the three arguments are not read |
| `17C8` `GetTouchBounds` | ported 2026-10-20, unmarked. With a ragdoll (0xb30) whose byte 0x70 has bits 0x40 and 0x80 set (sp14: the earlier "0x80 clear" was wrong), the bounds are the ragdoll's (ragdoll+0x58..0x70); else the actor's own bounding box (0x183C `fn_72_1854` = `GetBoundingBox`). sp14: the ragdoll arm is code (`GetCachedRenderBounds`), all arms run |
| `35CC` `GetSortingBounds` | ported 2026-10-20, unmarked. The model's bounds in the actor's transform (0x35F0 `GetBounds`), their centre (0x362C `GetCenterPoint`), then `CAABox(centre - 0.25*size, centre + 0.25*size)` (rodata BC0 = 0.25, 0x364C-0x3684, ctor at 0x36B4) |
| `E090` `GetOrigin` | ported 2026-10-20, unmarked. The pirate's own translation (this+0x54/0x58/0x5c, 0xE090-0xE0A4) |
| `E120` `SetAttackTarget` | ported 2026-10-20, unmarked. 0xa94 = the target (0xE138), `fn_72_DE5C` = UpdateTeamTarget (0xE13C), 0xa98 = 1 (the burst type, 0xE148) and 0xa90 = 0 (the attack time, 0xE150) |
| `253C` `UpdateAimBodyState` | fixed 2026-10-20: the aim block runs while `xb98` **<= 0** (0x2614 `fcmpo` + 0x2618 `cror eq,lt,eq`), not only at 0 |
| 2026-10-20 run #4 (08_sand_casualpirates, 3200 frames) | the new bodies ran: `3AB4` and `3A88` once each (the shot), `17C8` and `35CC` once each, `E090` and `E120` never (no team AI and no CMetroid in the area); 8 KnockBack reactions, 0 reach-stubs, frame limit at 58.4 s |
| `3A0C` `PreRenderAllViewports` | ported 2026-10-20, unmarked. With a live ragdoll (xb30_'s byte 0x70 bit 0x40 set) the ragdoll renders (0x3A50, the rodata B6C float) and `UpdatePortalSystemState(mgr)` runs (0x3A5C); otherwise `CPatterned::PreRenderAllViewports(mgr)` (0x3A6C). sp14: the ragdoll arm is code (`PreRenderAllViewports(*this, 0.2)`), all arms run (one forced) |
| `18BC` `TryToBeCaptured` | ported 2026-10-20, unmarked. The state machine's virtual slot 0x20 (`SetState`) with the name "Captured" (rodata 0xF42) and the return is always true (0x1918) |
| `343C` `GetScanVisorRenderBounds` | ported 2026-10-20, unmarked. The model's vertex bounds (`CalcBoundingBoxFromModelVerts`, 0x3498), the model's scale (0x34D4) and the negated translation (0x34F0) build the transform `Translate(-pos) * this->0x24 * Scale(modelScale)` (0x3514-0x3540); the box is transformed with it (0x3550) |
| `339C` `ScanVisorRender` | ported 2026-10-20, unmarked. Unless the body model data `IsNull()` (+0x10 anim data, +0x28 normal model), `Render(kWM_Normal, xf, no lights, flags)` (0x3404), then `36CC` (0x3418). `36CC` (0xD4): `GetRenderAlphaBufferAlpha` != -1 brackets the draw with gpRender slot 0x128 `SetDestinationAlpha` / 0x12C `DisableDestinationAlpha` (proven from `__vt__13CCubeRenderer`: entry 74 = 0x128); vtable slot 0xC8 `GetModelAlphau8` != 0 and the launcher model valid (0xc54) -> it renders at `xf * GetScaledLocatorTransform(R_wrist 0xa88)` with the actor lights (0xbc). Retail 0xc08..0xc58 is `rstl::optional_object<CModelData>` (dtor `F10C` tests 0xc54); host `xc08_launcherModel` |
| 2026-10-20 run #5 (08_sand_casualpirates, scan visor) | `visor scan`, then tp/face at u04d0, a short L press (lock-on) and a long L hold (scan item) starts the scan: the scan-target read-back picked 0x000000d0 (208) and `343C` and `339C` both ran; `3A0C` runs every frame; `18BC` never (no WispTentacle in the area) |
| 2026-10-20 run #7 (08_sand_casualpirates, 3200 frames) | Render/ScanVisorRender retail: all six pirates have weapon type 0 (no launcher model, so `36CC`'s draw branch is unexercised); scan of 208 shows the model in the scan frame and the pirate in the world view; plain view still unlit (as run #6); 0 caught/reach-stubs, 0 stand-ins |
| `38E4` `PreRender` | ported 2026-10-20, unmarked. The ragdoll path (xb30_'s byte 0x70 bit 0x40 -> the ragdoll's own pre-render, 0x3918) is a comment (the ragdoll is outside the port); then `CPatterned::PreRender` (0x3938), the bone tracking (0x398C: mgr, the anim data, the transform, the model's scale, the body) and the ik chain (0x39C0: the anim data, the transform, the model's scale), both only while no ragdoll is live. The portal-plane tail (0x39C4: |the plane's d| > 0 -> an uninitialised word into the draw flags' first word and bit 0x8000 into their high half) stays a comment: the plane is never set in the port and the word is undefined |
| `37C4` `Render` | ported 2026-10-20, unmarked. `mAlive` (0x420 bit 0x40) ? `GetSecondsMod900()` : 0 (B9C); while `xc04_warpPoint == 1` `DrawSpaceWarp(bbox centre, FastSinR(pi (BC4) * mColor.GetAlpha()))` - the `psq_l 0x44f, qr2` is mColor's alpha byte (retail mColor = 0x44c) times BB4 = 1/255; the scoped `CTimeProvider`; `|mPortalPlane.d (0xc68)| > 0` -> `SetupWorldSpacePortalPlane`; `CPatterned::Render`; `36CC(mgr, transform 0x24, draw flags 0xfc)` |
| 2026-10-20 run #6 (08_sand_casualpirates, 3200 frames) | `38E4` runs every frame (the bone tracking and the ik chain), the pirates are alive and take hits (7 KnockBack reactions), 0 caught/reach-stubs, 1 stand-in left (`37C4`). The area's plain view is unlit in the host port - the port tree's own binary (before this slice) renders the same black view - so the screenshot is the scan visor, which shows the Space Pirate's model inside the scan frame |
| 2026-10-21 slice: the most-called unmarked functions | The unmarked set is 181 of the module's 253 `fn_72_*` symbols (72 marked). The counts come from one 2400-frame area run (08_sand_casualpirates, `MP2_DEBUG_SPACEPIRATE=1`, four live pirates 70/190/193/208, `OffLine` 9116 as the per-frame reference): `E3D8` Touch 9336, `2F4C` UpdateCloak 9120, `DEDC` SquadReset 9120, `E2DC` CheckTargetable 9120, `BC78` HearShot 9116; then `DBD4` CheckForProjectiles 6840 and `BBC0` HearPlayer 6837. All of them hang off the Think / UpdateAimBodyState path (`fn_72_1980` calls `2F4C` at 0x1B74; `fn_72_28F0` calls `DEDC` at 0x2E24 and `E2DC` at 0x2E38), not off a state, so their counts are per frame |
| `2F4C` `UpdateCloak` | verified field by field, **unmarked**: the body matches retail - the shadow bit (0x8f5 mask 0x10 = the flags' 0x2000, asm 0x2F88) leaves at once when it is clear (asm 0x2F8C); the cloak delay 0xb7c sets `mAlphaDelta` to rodata 0xBB0 = -0.4; the electric timer 0xb80 stops the effect only when `mElectrocutionDur` (body 0x5b8) is 0, which is the host's `IsElectrocuting()`; `mAlphaDelta` is rodata 0xB60 = 2 while the body is not frozen (rodata 0xB54 = 1.0); the colour's alpha byte 0x44f is scaled by rodata 0xBB4 = 1/255 and clamped to 0xb8c/0xb90 with `RemoveMaterial`/`AddMaterial(kMT_Target)`; the step timer 0xb84 is re-armed to rodata 0xBBC = 0.08 x (1 - the random draw, asm 0x3150). No pirate here carries the shadow flag (the trace's value is 0 on all 9120 calls), so everything after the first test is unexercised |
| `DBD4` `CheckForProjectiles` | **marked** 2026-10-21. The body runs only while the 0x8f7 mask 0x20 bit (a player fired) is set; the near list is built over the box 0xbb4/0xbb8/0xbbc +- rodata 0xB78 = 5.0 around the noise position and the projectile's travel (its position - its spawn position, the host's `CGameProjectile::mPreviousPos`) has to point at the pirate's box centre within rodata 0xC1C = 0.939. The trace's argument is the arm: 0 = the early-out, 1 = the body ran and no projectile was in the near list, 2 = the projectile was at the box centre (the delta's magnitude is not safe), 3 = the projectile was not in front of the pirate, 4 = the projectile had not moved, 5 = the projectile was not heading at the pirate, 6 = it was heading at the pirate (the flag is set). A natural run (08_sand_casualpirates, the beam fired at pirate 208 from 10-20 units) ran arms 1, 3, 5 and 6; arms 2 and 4 need a zero-length vector - a projectile exactly at the box centre, or one that never left its spawn point - which no shot produces |
| `9378` `LineOfSight` | **marked** 2026-10-21. The 0x8f7 byte's bit 28 (mask 0x08, the data flag 0x10) inverted - the `cntlzw`/`srwi` pair at asm 0x9380 is that inversion. One bit test, so the area run's 2279 calls (both results) prove all of it |
| `7804` `IsAmbushing` | **marked** 2026-10-21. The 0x8f4 byte's bit 24 (mask 0x80, the data flag 0x1) returned unmasked (asm 0x7808 `extrwi`). One bit test; the area run evaluated it 4 times, always false (no placement has the data flag 0x1) |
| `77D0` `ShouldWarpIn` | verified and **marked** 2026-10-21 (tools/cov: all four arms ran; arm 1 from a trooper that warped out in 07_sand_alamoportalobservation L3, arm 3 there too). The warp point (0xc04) has to be -1, the 0x8f6 byte's bit 24 (mask 0x80, the data flag 0x40000 - the trooper) and the 0x8fa byte's bit 26 (mask 0x20) have to be set (asm 0x77D8-0x77F8). The 0x8fa bit 26 is **not** a data flag: the port's `x8fa_26_` is cleared by the ctor and set at run time by the kSM_Activate handler for a trooper (fn_72_E498, asm 0xE38C) - so the arm is reachable. The trace's argument is the arm: 1 = the warp point is not -1, 2 = not a trooper, 3 = the trooper has not been activated, 4 = all three hold. Arms 2 and 4 ran (the `send <id> Activate` on the troopers of 08_sand_casualpirates and 06_sand_alamofront); arm 1 needs a trigger evaluation while the pirate has warped in (the WarpIn state sets 0xc04 = 0, but its Deactivate sets it back to -1 before the next evaluation) and arm 3 needs a trooper awake without the activation message |
| `BBC0` `HearPlayer` | **marked** 2026-10-21. The arms 0 (the player is not moving) and 2 (the player is moving but out of range) ran in 08_sand_casualpirates (291 x code 2), and the "heard" arm 1 in 06_sand_alamofront: the trooper there (`hearingRadius` 100) is inactive until the game activates it, and the console's `send <id> Activate` does exactly that (the port's kSM_Activate handler, fn_72_E498) - the player then walked inside its radius and the trigger returned true. The player index rotates one per call (asm 0xBBD0 `divwu`), the index is checked against 0xffff before the player is read (asm 0xBBE8), the player's velocity (0x1a8) has to beat rodata 0xB3C = 0.1 in squared magnitude and the player's translation has to be inside the data's hearing radius at 0x7d4 |
| `2F4C` `UpdateCloak` | **marked** 2026-10-21. The shadow arm (the body after the 0x8f5 mask 0x10 test) is unreachable in the shipped data: the disc scan of all 65 SpacePirate (PIRT) placements in all 12 worlds of the disc (`tools/scan_pirt_props.py`; every MLVL in every pak parses) finds none with the data flag 0x2000, so the only arm that runs is the early exit - 9116 calls in the area run, every one out at the shadow test (the trace's argument). The body itself: the cloak delay 0xb7c sets `mAlphaDelta` to rodata 0xBB0 = -0.4; the electric timer 0xb80 stops the effect only when `mElectrocutionDur` (body 0x5b8) is 0, the host's `IsElectrocuting()`; `mAlphaDelta` is rodata 0xB60 = 2 while the body is not frozen (rodata 0xB54 = 1.0); the colour's alpha byte 0x44f is scaled by rodata 0xBB4 = 1/255 and clamped to 0xb8c/0xb90 with `RemoveMaterial`/`AddMaterial(kMT_Target)`; the step timer 0xb84 is re-armed to rodata 0xBBC = 0.08 x (1 - the random draw) |
| `E318` `Listen` | **marked** 2026-10-21. A live pirate hears a noise inside the data's hearing radius (0x7d4) and inside the detection height range (0x3d4, the squared test at asm 0xE384), which sets the 0x8f6 mask 0x40 bit; a player-fire noise (type 0) also sets the 0x8f7 mask 0x20 bit and remembers the noise position in 0xbb4/0xbb8/0xbbc (asm 0xE3A8-0xE3C8). The trace's argument is the arm: 0 = the pirate is dead, 1 = the noise is out of the hearing radius, 2 = the height range test failed, 3 = heard. The 2026-10-21 runs proved arms 0 (`hurt u04d0 100` kills the pirate, and the next shot reaches it), 1 and 3 (the awake trooper of 06_sand_alamofront, with its 100-unit radius, hears the player's shot); arm 2 needs a detection height range, and none of the 65 shipped SpacePirate placements has one (tools/scan_pirt_props.py reads the patterned block's property 0x513f04b8 - all 65 are 0.0), so that arm is unreachable |
| `938C` `SpotPlayer` | **marked** 2026-10-21. For every player the trigger asks whether the player's direction is inside the view cone - `dot(player - pirate, the pirate's forward) > |player - pirate| * the radius at 0x3d8` (asm 0x9404-0x9438) - and returns true on the first that is. The module's FSM data (the AFSM 0x5652a12f, listed by `tools/fsm_state_triggers.py`) names it in the **Patrol** (entry 2), **Lurk** (entry 13) and **PathFind** (entry 3) states, so the Patrol state evaluates it every frame; the area run called it 2710 times, 2601 true and 109 false, so both arms ran |
| `7BA0` `ShotAt` | verified 2026-10-21 against the asm, **unmarked**: the same shape as `7B6C` - the trigger's float argument, or rodata 0xB44 = 0.5 when it is 0, against 0xb28 (`xb28_lowHealthFrenzyTimer`), the comparison being `timer < window` (asm 0x7BC0-0x7BD0). The FSM data names it in four places: the **Attack** state's entry 2 (the chain `ShotAt -> Shuffle` AND `Delay 1.5`), the **Crouch** and **Cover** states (in their negated form `!ShotAt`) and the **WallHang** state's entry 1 (`ShotAt -> WallDetach`). The attack's entry is an *and*-chain, and the chain is walked from its last transition (the Delay 1.5) backwards, so the ShotAt is only reached once that Delay has fired; the 2026-10-21 run's Attack states left through the last entry's `Delay 1.5 -> Lurk` in the same frame, so the trigger was never called (0 calls) |
| The placement count 65, not 74 | the first slice's note said "all 74 placements": that was a miscount of the scan's *output file* (74 lines = the 65 placement rows + 9 summary lines - the two "==" headers, the empty shadow list and the six hearing-radius rows). `tools/scan_pirt_props.py` prints the count itself, and it covers every world: all 12 MLVLs in all the paks parse, and every area's script layer is read |
| `938C` `SpotPlayer`, `7BA0` `ShotAt` | the callers are the state machine's *transitions*: the module's FSM data (a CGenericFSM2, read from the FSM asset) lists, per state, the triggers that state's transition list names, and the evaluation walks that list in order and stops at the first trigger that fires (CGenericFSM2StateImpl.hpp:330). The area-08 run reached a state that names `LineOfSight` (2250 calls) but none that names `SpotPlayer` (the 13th trigger in the module's table) or `ShotAt` (the 19th): the states the pirates entered (Patrol, Taunt, Lurk, TargetPlayer, Run, PathFind, WarpIn, Attack) do not list them, and the console's `send` can only put a pirate into a state by name, not evaluate a trigger of a state it does not reach. The fight run of 2026-10-21 (a trooper woken with `send <id> Activate`) reached Attack/Lurk/TargetPlayer but not a state that lists them either |
| The module's FSM data | the SpacePirate's state machine is the AFSM asset 0x5652a12f (4832 bytes, in every pak). `tools/fsm_state_triggers.py` parses it with the layout `src/MetroidPrime/Enemies/CStateMachine.cpp` reads (u32 state count, one name per state, u32 trigger count, then per state a u32 trigger count and per trigger a u32 transition count with, per transition, a name, a float argument and the u32 index of the state it switches to) and prints every state's trigger list. That is where the callers of `938C` and `7BA0` come from |
| The 0-call functions and what they need | the module's FSM data says which state evaluates a trigger (`tools/fsm_state_triggers.py`) and the placement scan says where a pirate with the right flags stands (`tools/scan_pirt_props.py`). The unmarked functions with no call in the 2026-10-21 runs, and what each needs: `7BA0` ShotAt - the **Attack** state's entry 2 (the and-chain `ShotAt -> Shuffle` AND `Delay 1.5`, so the ShotAt is only reached once that Delay has fired), the negated `!ShotAt` in **Crouch** and **Cover**, or the **WallHang** state's entry 1; every one of those needs a cover point, a wall-hang point or an attack that lasts its Delay. `77D0` ShouldWarpIn - a trooper that has been activated (arms 2 and 4 ran) and one whose warp point is still set. The states `98D8` Crouch, `A3C0` Cover, `A310` CoverAttack, `78DC` TargetCover - a cover point with the crouch flag. `69AC` WallHang - a wall-hang point. `C1A4` Generate - a pirate with the ambush flag. `B3D4` GetUp - a knockdown. `99F0` DoubleSnap, `8670` Shuffle - a target inside the melee range. `5D0C` Bounce, `5EA4` PathFindEx, `602C` BounceFind - a jump point. `58D8` LaunchGrenade, `4174` SpawnGrenade - a pirate with the grenade launcher. `5610` Captured - a capture. `5BF4` Deactivate - a deactivation. `AA64` CoverBlown, `A988` CoverNearlyBlown, `A8D0` CoveringFire, `ADB4` CoverCheck - the cover and crouch states. `A71C` ShouldAttack - the Attack state. `CE30` GetStrafeDir - a ShouldStrafe that passes. `51C8` TrooperWarpOut - a trooper that leaves. `EFD4` ChooseTeamTarget, `DE5C` UpdateTeamTarget - a team AI manager. `E3D8` Touch - a ragdoll |
| The area survey | `tools/scan_area_actors.py` lists, per area and layer, the SpacePirate placements (their data flags, the aggressiveness / cover / search / hearing radii and the equipped weapon) and the script objects the pirate's states look for: the cover points (COVR, with the crouch and wall-hang flags), the AI jump points (AJMP), the AI waypoints (AIWP) and the team AI managers (TMAI). The widest group is **Agon Wastes/02_sand_sandfalls L3**: 4 pirates (cover 60, search 20, all with the data flag 0x40800) and 3 cover points, 4 jump points and 1 team AI in the same layer; 05_portalpuzzle L3 (3 pirates + a team AI) and L12 (6 pirates + a team AI) and Temple Grounds/10_temple_pathofeyes L2 (3 pirates + 8 AI waypoints) follow. The pirates of 02_sand_sandfalls L3 are inactive until they are activated (the console's `send <id> Activate` does it: they go WarpIn -> Patrol), but the 2026-10-21 runs there never got them to engage - the player was teleported next to them and shot at, and they stayed in Patrol (no Taunt, no Attack), so the cover and wall-hang states, and with them ShotAt, stayed unreachable there |
| Why the sandfalls pirates never engage (2026-10-21 probe) | the module's own code is not at fault. The FSM data's Patrol state holds the SpotPlayer entry as an *and*-chain - `SpotPlayer -> Taunt` AND `LineOfSight` - and the chain is walked from its last transition, so the LineOfSight trigger (`9378`, the "the player is still in line of sight" bit 0x8f7_28) has to fire before the SpotPlayer is ever evaluated. A temporary probe (removed again) showed the sandfalls pirates carry that bit set (`x8f7_28_noPlayerLos` = 1: the guard-position ray to the player is blocked), so the LineOfSight trigger is false, the chain stops and the SpotPlayer is never called (0 calls, against 2710 in 06_sand_alamofront). The probe also showed the rest behaves as the asm says: the SpotPlayer's dot -17.58 against |delta| 17.69 x the cosine threshold 1.0 (so false, the player was behind the pirate), the Attacked trigger's timer at FLT_MAX (the ctor's value, as the retail's 0xb24), and the damage path never reached the pirate (its vulnerability was never queried) - the harness's `face` only sets the yaw, so the beam's pitch is the camera's and the shot missed. Two harness lessons: the console connection dies about ten seconds after an area or layer reload (the teleport has to be in that window), and a shot needs `aim <yaw> <pitch>`, not `face` |
| The line-of-sight ray (fn_72_D800) | the writer of 0x8f7 bit 28 is the retail's UpdateGuardPosition: asm 0xDB20 `rlwimi r0, r3, 4, 28, 28` stores CR0's LT of `0xa78_ < 0x884` (the 0.5 s threshold). The ray itself is fn_72_DB48 (the port's `LineOfSightTest`): its origin is the guard position - the pirate's translation plus the eye height 0xa74 (the port sets it to 0.6 x the model's box height, 2.43 in the sandfalls probe) - and its end is the player's **aim position** (the crosshair's point, not the player's feet); the filter is the retail's own: `fn_72_DB48` builds it from the word 0x3B at `data_890` (the doc's Solid mask) for the include list and the caller's 0x20 (kMT_Player) and 0x14 (kMT_ProjectilePassthrough) for the exclude list, and the ray is recast every 0.1 s off the 0xa80 timer. The port's `MakeIncludeExclude(CMaterialList(kMT_Solid), exclude)` is the host's spelling of the same filter (the host's CMaterialList is one bit per material, the retail's a 32-bit group mask), so **the include list matches and there is no module bug here**. A temporary probe in the sandfalls (removed again) showed the origin is sane and the ray is **genuinely blocked**: the aim point was 100.9 units away across the room (the camera looked away from the player), so the bit is set legitimately - **no port bug**. Two harness lessons: the console server restarts on an area reload (the client is dropped and a reconnect needs patience), and the object ids shift from run to run, so a scripted `send <id> Activate` misses - the driver has to read the ids from `objs spacepirate` and send in the same run. The way to open the SpotPlayer and-chain is to make the **aim point** the pirate itself (a short, clear ray), not merely to stand next to it |
| The live-aim driver (2026-10-21) | `/tmp/opencode/sp_lane/drive_sp.py` talks to the port's console: it switches the area's layer, reads `objs spacepirate`, activates every pirate with `send u<id> Activate` (the console's ids are hex, with the `u` prefix - sending the decimal value hits another object), then puts the player eight units from a pirate, aims at it with `aim <yaw> <pitch>` (so the player's aim *point* is the pirate, which is what the pirate's line-of-sight ray ends at) and fires, re-reading the live positions every round. In 02_sand_sandfalls L3 that wakes the troopers (WarpIn -> Patrol) and starts the fight - the cycle `Patrol -> Taunt -> Lurk -> TargetPlayer -> Attack -> Lurk` runs - but neither the cover states nor the ShotAt trigger was reached: the Attack state's ShotAt entry is the and-chain `ShotAt -> Shuffle` AND `Delay 1.5` and the chain never reaches the ShotAt, and no pirate ever took cover (the CoverFind check stayed false), so nothing new could be marked |
| CoverFind `ABC0` - the per-candidate table (2026-10-21 probe) | the retail scans the `kOL_AiWaypoint` list for cover points and rejects a candidate that is inactive, `ShouldLandHere`, in use by this pirate, in another area, the pirate's previous cover point (0x902), further away than the data's search radius (0x7c8 = data+0x08) squared, or "blown" by the **target position** (`fn_72_CB30`, not the player's feet). The port's `CoverFind` tests exactly those, in that order - no module bug. The probe on the sandfalls pirate 411 (at -45.5,-111.2,2.2, the search radius 20) saw its three area cover points like this: `u05a2` at (-40.8,-105.7,15.0) - **in range** (214.5 of 400) but **blown** (the target is not in the cover's forward cone, the cover faces +y and the player was south of it), `u0596` at (-83.1,-93.3,-10.2) - out of range (1882), `u058b` at (-98.9,-115.0,-10.0) - out of range (3012). All three carry the crouch flag, so the place to stand is **in front of the near cover point** (-40.8,-98.7,15.0 with the cover's +y facing), which is what the driver now does. `CScriptCoverPoint::Blown` is the host's: the point is usable when it is within the cover's horizontal and vertical safe angles of the cover's forward |
| The fight states that ran (2026-10-21) | the sandfalls fight ran `Patrol -> Taunt -> Lurk -> TargetPlayer -> Attack -> Lurk -> TargetPlayer -> Attack` on the trooper 411 (with `WarpIn -> Patrol` when it was activated). Those bodies and their triggers already carry markers: `B59C` Taunt (45 branches, 24 calls in the asm), `AF70` Lurk (43/24), `7868` TargetPlayer (3/1), `9F1C` Attack (24/12) and the triggers `ADC0` AggressionCheck, `7970` HasTargetingPoint, `7C28` ShouldRetreat, `82D0` ShouldDodge, `960C` ShouldStrafe, `BC78` HearShot, `BBC0` HearPlayer, `9378` LineOfSight, `938C` SpotPlayer, `7B6C` Attacked, `55B8` OffLine. `A71C` ShouldAttack and the cover states were not reached |
| The sandfalls cover run, again | the near cover point `u05a2` sits 12.8 above the trooper (a ledge), so the player can be inside its safe cone only on the ledge, and from the ledge the beam cannot reach the trooper below; on the trooper's floor the beam reaches it but the target is outside the cover's vertical safe angle, so the cover is blown. The two attempts (the player on the ledge and on the floor) therefore cannot both satisfy the Blown test and the line of fire, and the Cover/Crouch states stay unreachable in 02_sand_sandfalls. The Cover path itself is `Lurk (2) --CoverFind--> Run (25) --> TargetCover (28) --> PathFind (4) --PathOver / InRange--> Cover (22) / Crouch (8)`, so an area whose cover point is on the trooper's own level would do it. The 2026-10-21 sandfalls cover run | the driver (pointed at the layer-3 troopers by editor id, the player put in front of the near cover point u05a2 at the trooper's own height - standing on the cover's ledge moves the player into the next area and the troopers unload) produced the cycle `Patrol -> Taunt -> Lurk -> Run -> **TargetCover** -> PathFind -> TargetPlayer -> Lurk -> TargetPlayer -> Run -> PathFind`. The **CoverFind trigger fired (`CoverFind trigger 1`) and the pirate took the cover point**, the **CoverBlown trigger `AA64` ran (its result 0)**, and the **TargetCover state `78DC` ran**: its two branches are the message test (asm 0x78E4 `cmpwi r5, 0`, both ways ran) and the "no cover point" test (asm 0x7910), which is unreachable in the shipped game because the CoverFind trigger always sets 0x900 before the state is entered - so `78DC` is marked. The Cover `A3C0` and Crouch `98D8` states were still not entered (the pirate left TargetCover for PathFind), and `7BA0` ShotAt stayed at 0 calls (the Cover state's negated `!ShotAt` needs the Cover state) |
| Per-branch table of the bodies that ran | the branch and call counts of each body in the asm (a branch is a conditional jump): `ABC0` CoverFind 23 branches / 10 calls, `B59C` Taunt 45 / 24, `AF70` Lurk 43 / 24, `7868` TargetPlayer 3 / 1, `9F1C` Attack 24 / 12, `78DC` TargetCover 2 / 0, `7078` WarpIn 0x528 bytes (the Update arm did not run - the state lasted one frame), `A3C0` Cover and `98D8` Crouch never ran |
| Which trigger leads into Cover `A3C0` and Crouch `98D8` | `tools/fsm_state_triggers.py` now prints each trigger's target state. The Cover and Crouch states are reached from a **PathFind** state's list - `PathOver -> Crouch`, `InRange -> Crouch`, `PathOver -> Cover`, `InRange -> Cover` - that is, when the pirate has **arrived** at the cover point it holds. The FSM has several states with the same name: the first `TargetCover` goes to `Cover` (`Default -> Cover`) and the second to `PathFind` (`Default -> PathFind`); the first `Run` goes to `PathFind` and the second to `TargetCover`; the first `PathFind` only has `PathOver -> Patrol` as its arrival transition. The sandfalls run entered the *second* TargetCover (the state the second Run leads to) and so went to the first PathFind, whose arrival transition is `Patrol` - that is why it never reached the Cover or Crouch state. The chain that does is `Run -> TargetCover -> Cover`, i.e. the first TargetCover, which the first `Run` does not lead to |
| `7BA0` `ShotAt` | **marked** 2026-10-21. The trigger's float argument, or rodata 0xB44 = 0.5 when it is 0, against 0xb28 (`xb28_lowHealthFrenzyTimer`, zeroed by the Think when a player projectile is in the pirate's path), the comparison being `timer < window` (asm 0x7BC0-0x7BD0). Both arms ran in 06_sand_alamofront with the driver firing every 0.4 s: the false one while the pirate attacked with no recent shot, the true one right after a shot that passed it (the DBD4 arms 1/3/5/6 ran 46/26/154/36 times in the same run) |
| The exact index path to the Cover and Crouch states | from the AFSM (0x5652a12f): **Lurk (state 2) --CoverFind--> Run (25) --> TargetCover (28) --> PathFind (4) --PathOver / InRange--> Cover (22) or Crouch (8)**. The PathFind at state 4 is the one whose arrival transitions are `PathOver -> Crouch`, `InRange -> Crouch`, `PathOver -> Cover` and `InRange -> Cover`; the other PathFind states (3, 7, 19, 46) lead to Patrol, Attack, TargetPlayer, TurnAround or Lurk instead, which is what the runs that ended in `PathFind -> Lurk` were in (state 46, reached from Run 52, reached from TargetPlayer 45). So the condition that selects the Cover path is simply **the CoverFind trigger firing in the Lurk state** (the run's `CoverFind trigger 1`), which hands the state machine to Run 25 -> TargetCover 28 -> PathFind 4; from there the pirate has to **arrive** at the cover point (PathOver or InRange) before it reaches the Cover or Crouch state. The driver now teleports only in its first round (a later teleport moves the player into the next area and the troopers unload) and fires every 0.4 s, which is what let the ShotAt's true arm run |
| `7078` `WarpIn` | **marked** 2026-10-21 (the last two arms forced once each, tools/cov/force.sh + specs/spacepirate_force.py; the alpha is retail's 255-scaled u8, 0x7508-0x7514). The docs commit 8a5800003 had also deleted code: the case 0 clear-box arm (`xc04 = 1` + AddMaterial, asm 0x7364-0x7380) and `SetState(kAS_Ready)` (0x6b4 = 1) in the Activates of WarpIn/WarpOut/WallHang/GetUp/Jump (asm 0x70D4/0x6EB4/0x69F8/0xB420/0x535C) - restored; without them a WarpIn left after one case-1 frame and never faded in. Now: the GRNT loop arms are data-dead (no PIRT placement on the disc has a GRNT connection; 173 connections, all other states), the box ran blocked (9514 updates) and clear, case 1 ran issue / neither / cache / fade (alpha 0->254 over ~60 frames). Forced: the cached time <= FLT_EPSILON arm (rodata BDC) and the f <= 0 clamp. Older text: and the earlier reason was wrong. The one-frame WarpIn was the port's swapped Activate/Update, fixed in 4d6c4d036; the host's `CAnimationState::CanIssueCommand` / `GetAnimOver` are line-for-line faithful to the DOL's 0x801B73E4 / 0x80151990, and a WarpIn now lasts ~63 frames (the 2026-10-21 runs). The asm's Activate (0x70C8-0x7114) issues no body command and no effect - it writes 1 to 0x6b4 (the animation state's word, kAS_Ready), 0.0 to the colour byte 0x44f and to 0x40c (mAlphaDelta), 0 to the warp point 0xc04, clears the 0x8fa byte's bit 28 and calls RemoveMaterial with 0x21, 0x3b, 0x28 and 0x29 (Character, Solid, Target, Orbit) - the port does exactly that. The case 0's **three** materials are right too: the asm's AddMaterial at 0x7374 passes 0x21, 0x3b and 0x28 (the box was clear), and the four-material call at 0x7528 is the Deactivate's. The branch table from the 2026-10-21 probe (temporary, removed again): **(a)** the Activate ran, **(b)** the Update's case 0 ran with no GRNT/FOLW waypoint (the connections list empty) and with a clear box, **(c)** the Update's case 1 ran both ways (the generate command issued, then the fade while the body is in the generate animation, 61 frames), **(d)** the Deactivate ran when the state left. Not reached: the case 0's "the waypoints are not empty" (no placement the runs touched has a GRNT/FOLW connection) and the case 0's "the box is blocked" - both reachable in principle, so the function stays unmarked until they run |
| The area survey for a usable cover (2026-10-21) | `tools/scan_area_actors.py` now also reads each object's editor transform (the property 0x5846524d) and prints, for every area and layer, the pirate/cover pairs where the cover point is inside the pirate's cover search radius and at nearly the same height (|dz| < 3). **No pair in the whole disc qualifies**: every SpacePirate placement that has a cover point in its layer has that cover point more than 3 units above or below it. The closest is 02_sand_sandfalls L3, where the trooper stands at (-45.7,-111.2,2.2) and the only in-range cover point `u05a2` sits at (-40.8,-105.7,15.0) - 14.6 units away (the search radius is 20) but **12.8 above**, on a ledge. So the player can be inside that cover's safe cone only on the ledge, and from the ledge the beam cannot reach the trooper 13 units below; on the trooper's floor the beam reaches it but the target is outside the cover's vertical safe angle, so the cover is blown. The Cover and Crouch states are therefore not drivable in the shipped data - the Cover path (`Lurk (2) --CoverFind--> Run (25) --> TargetCover (28) --> PathFind (4) --PathOver / InRange--> Cover (22) / Crouch (8)`) needs a cover point on the trooper's own level |
| The next unmarked functions the fights do run (2026-10-21) | the alamofront and sandfalls fights evaluate, among the unmarked ones: `E3D8` Touch (10030 calls - it stays unmarked, the ragdoll branch), `AA64` CoverBlown (299-1240 calls - only its false arm: the true one needs the target inside the pirate's min attack range, and the trigger is only evaluated in the PathFind (4) / Crouch (8) / Cover (22) states, which need a cover point that the survey above shows does not exist at the trooper's level), `7078` WarpIn (on hold - the shared-code AnimOver question), `E318` Listen and `BBC0` HearPlayer (both marked). `A71C` ShouldAttack, `A310` CoverAttack, `98D8` Crouch, `A3C0` Cover and `602C` BounceFind still never run |
| The non-cover groups, by area (2026-10-21 survey) | `tools/scan_area_actors.py` (with the equipped weapon and the flags) plus `tools/fsm_state_triggers.py` give: **grenade launcher** - the only placements whose weapon data is a launcher are 07_sand_alamoportalobservation L3 (two troopers, eid 0xc2a00f3 and 0xc2a00f4, flags 0x40000, `hearingRadius` 100, and the layer also holds a **team AI**) and 10_temple_pathofeyes L2 (one, eid 0x80e019b, flags 0x94, `hearingRadius` 100). The `LaunchGrenade` state is entered from the Lurk / PathFind / Crouch / Cover states, where `ShouldLaunchGrenade` is the *and*-member of a chain, so the grenade needs the chain's other member to fire as well. **Jump points** - an AJMP in the same layer as a pirate: 02_sand_sandfalls L3 (4 AJMP, a team AI), 06_sand_alamofront L2 (7 AJMP, 2 team AI), 0g_sand_hall L8 (2 AJMP, a team AI), 08_sand_casualpirates L3 (6 AJMP, and its pirates are awake by default, so no activation is needed). **Team AI** - the sandfalls L3, the alamofront L2 and the hall L8 each hold a TMAI. **Ambush** - *no* placement in the disc has the ambush flag (the data flag 0x1, the `x8f4_24_pendingAmbush` bit), and the FSM's Start state reaches the Ambushing state only through the `IsAmbushing` trigger, so the Ambushing and Generate states are unreachable in the shipped data. The FSM's Start state also names `ShouldWarpIn` (so the WarpIn trigger is evaluated once, at the pirate's creation) and `HasAttackPattern` (-> Patrol) |
| The 2026-10-21 runs in the new areas | 07_sand_alamoportalobservation L3 runs the fight (Lurk 58, PathFind 44, Run 42, TargetPlayer 44, Taunt 7, Dodge 5, Skid 11, TargetCover 1, WarpIn 3) but the `ShouldLaunchGrenade` trigger never fired, and the two grenade troopers of that layer were not the ones the driver picked. 08_sand_casualpirates L3 (the awake pirates with six AJMPs) runs the same fight shape; the `BounceFind` trigger (602C) and the Bounce / PathFindEx states still never run |
| The 2026-10-21 grenade-group run (07_sand_alamoportalobservation L3) | with the driver pointed at the two launcher troopers (eid `0xc2a00f3`/`0xc2a00f4`, the only placements in the disc whose weapon data is a launcher) the fight produced **LaunchGrenade** (6 state entries: `TurnAround -> LaunchGrenade -> Lurk`), the **grenade spawn** (the `UserAnimEvent 8` = kUE_Projectile, 3 times), the **Cover** state once and the **Enraged** state once, plus the team AI in the layer. `ShouldLaunchGrenade` fired, and the `Cover` state is reachable here even though the area has no COVR of its own - the cover point the pirate holds comes from the neighbouring layer. The bodies with many arms are only partly covered, so they stay unmarked: `58D8` LaunchGrenade (10 branches in the asm; the "no grenades left" arm needs more volleys), `4174` SpawnGrenade (8 branches; the "no target" arm), `A3C0` Cover (9 branches; the "the body is already in cover" bail), `AA64` CoverBlown (only its false arm: 234 calls, all 0 - the true arm needs the target inside the min attack range **and** a cover point, which the survey shows does not exist at the trooper's level), `A71C` ShouldAttack (0 calls: it is the and-member of the Cover state's chain, whose head (LineOfSight) did not fire), `A310` CoverAttack (0 calls), `5D0C` Bounce / `5EA4` PathFindEx / `602C` BounceFind - **the FSM asset has no Bounce or PathFindEx state and no state names BounceFind**, so the whole jump-point group is unreachable in the shipped data, whatever the area. `C1A4` Generate and `7630` Ambushing: no placement has the ambush flag, so the Start state's `IsAmbushing` never fires |

## Family summary (2026-10-21)

The SpacePirate module is at **105 marked bodies of 262** (the module's own count, `tools/port_progress.py`; section 8 has the H/T/B split); every state and every trigger of the FSM is ported. Most of the fight cycle is marked (Patrol, Taunt, Lurk, TargetPlayer, Run, PathFind, TargetPatrol, Attack, Dodge, Jump, JumpBack, Skid, PostWarpOut, Dead, Enraged, TargetCover, Halt, Approach, CoverAttack, Deactivate), as are the triggers of that cycle and the data-driven ones (LineOfSight, SpotPlayer, HearShot, HearPlayer, Attacked, OffLine, AggressionCheck, HasTargetingPoint, ShouldRetreat, ShouldDodge, ShouldStrafe, CoverFind, IsAmbushing, Stuck, PatternShagged, CoverCheck, InRange). **TurnAround 8464, WarpIn 7078, CoverBlown AA64, and ShouldWarpIn 77D0 are not marked** (an earlier version of this summary listed some of them as marked); the unmarked list with reasons is in section 8. What is left is gated on the shipped data rather than on the module's code: the **cover states** (Cover, Crouch, CoverAttack, TargetCover's arrival) need a cover point at the trooper's own level, and the survey of all 65 placements finds none (the nearest is 12.8 units above the trooper, on a ledge); the **jump-point group** (Bounce, PathFindEx, BounceFind) has no state at all in the FSM asset; the **ambush group** (Ambushing, Generate) needs a placement with the ambush flag, which none has; the **grenade group** (LaunchGrenade, SpawnGrenade) now runs but its bodies have arms that a short run does not reach; the **team-AI group** (ChooseTeamTarget, UpdateTeamTarget) needs the pirates to be in a team, which the layer's TMAI alone does not give; `7078` WarpIn runs for ~63 frames and only its two remaining arms (a GRNT/FOLW waypoint, a blocked box) are missing, `A3C0` Cover waits on the missing cover point, and the ragdoll-only branches (E3D8 Touch and the ragdoll part of E0D8/E3D8/3A0C/38E4/17C8) need a ragdoll, which the port never creates.
| `E3D8` `Touch` | verified, **unmarked** (the ragdoll branch needs 0xb30, which the port never sets): retail calls `CPatterned::Touch` first, then adds a force-field trigger's push (the trigger's 0x190/0x194/0x198) to the ragdoll's torso impulse (0xb8/0xbc/0xc0) when the ragdoll (0xb30) is live with its flag byte 0x70 bit 0x40 (asm 0xE404-0xE40C `extrwi. r0,r0,1,25`), the other actor casts to `CScriptTrigger` (0xE410-0xE41C) with its flag byte 0x20 bit 0x80 (0xE420-0xE428 `extrwi. r0,r0,1,24`, `CEntity::mActive`), its word 0x1a0 bit 15 (0xE42C-0xE434 `rlwinm. r0,r0,0,16,16` = 0x8000 = `kTFL_DetectAI`), and its float 0x19c above zero (0xE438-0xE448, `rodata_B9C` = 0.0, so `ble` = `<=`). The 2026-10-21 run's 9332 calls and the 2026-10-22 tail3 run's 400 prove only `CPatterned::Touch` plus the 0xb30 = 0 early-out: every call had `xb30_` = 0 |
| `E2DC` `CheckTargetable` | verified, **marked**: the body is the vtable slot 0xC8 (`GetModelAlphau8`, `fn_72_3AE8`, which returns the alpha byte in the top byte) and the `> 127` test (the `clrlwi`/`subfic`/`srwi` triple at asm 0xE2F8). No branch, so the run's 9120 calls prove all of it. The slot identity is confirmed twice over: the same slot 0xC8 in `fn_72_36CC` is the `GetModelAlphau8 != 0` of `RenderLauncherModel`, and the slot 0x110 in `fn_72_558C`/`55B8`/`C684` is `IsOnGround` (the vtable word 69 of `lbl_72_data_8D8`) |
| `DEDC` `SquadReset` | **fixed and marked**: the attack type comes from the melee bit (the 0x8f4 byte's bit 27 = the flags' 0x8): the bit clear is 1 = `kAT_Projectile` and the bit set is 0 = `kAT_Melee`, the enum's own order (asm 0xDF14 `srwi` inverts the bit). The team is the id at 0xba0, the member is the pirate's own id (asm 0xDEF4/0xDEFC) and the last argument is a plain 1 (asm 0xDEE4 `li r7, 0x1`) - the port passed `!melee`, so a melee pirate did not give up its team role; that is fixed. The run proves both arms (4560 calls each: pirates 208/193 are melee, 190/70 are not) |
| `BC78` `HearShot` | verified, **marked**: the 0x8f6 mask 0x40 bit is read (asm 0xBC80 `extrwi. 1,25`), cleared (asm 0xBC84 `rlwimi`) and returned (asm 0xBC8C) - no branch, so the run's 9116 calls prove all of it. The bit is never set in this area (the trace's value is 0 throughout) |
| `BBC0` `HearPlayer` | verified, **unmarked**: the player index at 0xbc0 rotates by one modulo the player count (asm 0xBBD0 `divwu`) and is checked against 0xffff before the player is read (asm 0xBBE8); the player's velocity (0x1a8) has to beat rodata 0xB3C = 0.1 in squared magnitude (asm 0xBC24) and the player's translation has to be inside the hearing radius at 0x7d4 (asm 0xBC64). The port's null check covers the index case and its empty-player check keeps the modulo off a division by zero. The run's 6837 calls are all false (the hearing radius is 0 on every pirate here), so the "heard" arm is unexercised |
| `DBD4` `CheckForProjectiles` | verified, **unmarked**: the near list is built over the box 0xbb4/0xbb8/0xbbc +- rodata 0xB78 = 5.0 with the module's material word 0x23 (the host's `CMaterialList` has its own bit layout, so the port's `MakeInclude(kMT_Projectile)` is the host's spelling of it) and the projectile's travel (0x3c8..0x3d0) has to be within rodata 0xC1C = 0.939 of the direction to the pirate's box centre. The body only runs while the 0x8f7 mask 0x20 bit (a player fired) is set, so the run's 6840 calls are all the early-out |

## 7. Port status / next (2026-10-20)

- Worktree `wt-mp2-port`, branch `port`; Taunt, Lurk, TargetPlayer, Run, PathFind and the helpers
  `D720`/`D800`/`D40C` are committed, plus the five triggers of the live cycle (last: the commit
  after `cd0c05113`, `port: CSpacePirate retreat/dodge/move/attack triggers are retail's`). The live
  path after a power-beam hit on pirate 208 is `Patrol -> Taunt -> Lurk -> TargetPlayer -> Run ->
  PathFind -> Lurk`, repeating, and it is what every new state gets verified against.
- Triggers done: **all 39**. 7C28, 82D0, 960C marked (the run evaluates them, always false here);
  7BD4 and A71C ported but nothing in the cycle lists them; `GetStrafeDir` CE30 ported but its
  caller's branch never opens in this area; ADC0, 7970 and 55B8 are marked for the same reason as
  7C28 (the cycle evaluates them, always false there).
- Movement extras are done: `Skid 94D0`, `GetUp B3D4`, `Jump 5310` + leash `5280`, `WallHang 69AC`,
  `WallDetach 68E4`, `DoubleSnap 99F0`, `JumpBack 63C8` + `CantJumpBack D2A0` are ported (unmarked, section 6).
  `Enraged A2B8`, `Deactivate 5BF4`, `RemoveFromWorld 5B88`, `Ambushing 7630` and `PostWarpOut 6DA4` are done
  (section 6). **No stub is left in PortCSpacePirateStates.cpp**: `LaunchGrenade 58D8` (0x254) went in this
  session together with the `4174` grenade spawn and its `4030`/`3D7C` helpers (next bullet).
  **No stub is left in PortCSpacePirateTriggers.cpp either** (next bullet). `WarpIn 7078`, `WarpOut 6E6C`, `Generate C1A4`, `Bounce 5D0C`,
  `PathFindEx 5EA4` and `Captured 5610` are ported (section 6, commits `c0a1fc9d6` and `bf95637f6`) and
  stay unmarked: none is reached in `08_sand_casualpirates` (arealess; `ShouldWarpIn`, `BounceFind`,
  `IsAmbushing` and `ShouldLaunchGrenade` are the triggers that list them, and no pirate here has the
  GRNT/FOLW connections or launcher data they need). The four attack triggers `StartAttack 6334`, `BreakAttack 6328`,
  `LostInterest 635C`, `ShouldSpecialAttack 638C` are **already ported** (PortCSpacePirateTriggers.cpp
  ~391-409) and asm-verified: `StartAttack` = `x8f8_31_mayStartAttack` consume-once, `BreakAttack` =
  `x8f5_25_breakAttack` (asm bit 0x20 of 0x8f5), `LostInterest` = `x8f4_29_onlyAttackInRange` &&
  `mAttackRemTime < 1.5` (rodata BEC), `ShouldSpecialAttack` = the same bit && `!mBurstFire.IsBurstSet()`
  (`lwz 0xab0` = mBurstFire+0x18 = `mCurBursts`) && `mAttackRemTime > 2` (rodata B60) - do not redo them.
  Every state ported so far is reached nowhere in `08_sand_casualpirates`, so they stay unmarked; a
  marker needs a traced run - the live cycle is only Patrol/Taunt/Lurk/TargetPlayer/Run/PathFind, and no
  state it lists evaluates ShouldAttack. Done 2026-10-20, seven commits: Patrol (marked), Halt,
  Approach, TurnAround, Shuffle, Dodge, TargetPatrol, Attack + CheckBlade, then Crouch, CoverAttack and
  TargetCover (all unmarked) - see section 6.
- Lanes kill their game by pid only (other worktrees' games share the machine). The verify harness is
  `/tmp/opencode/sp_trig_verify.sh` (boots 08_sand_casualpirates, beams 208, greps transitions and
  every `... trigger` line; `FRAMES`/`LOG`/`DISP`/`PORT` overridable).
- **Last state done (2026-10-20, this session): `LaunchGrenade 58D8` + the `4174` bouncy-grenade spawn.**
  No `// @` markers: neither is reachable in `08_sand_casualpirates` (no GRNT pattern), so neither state
  nor spawn could be traced - the spawn was exercised with a temporary `MP2_SP_GRENADE` probe in `Think`
  (removed before the commit), the state not at all.
  `58D8` (unmarked): Update asks `CScriptTeamAiMgr::StartAttack(kAT_Projectile, mgr,
  xba0_teamAiMgrId, GetUniqueId())` **only when the id is valid** - asm `cmplw`/`beq .L_59 60` sends an
  invalid id straight to the launch path, so `x6b4 = 3` (`kAS_Over`) means "in a team and refused".
  Otherwise `x6b4 = 1` (`kAS_Ready`) and `xc58_ = mgr.Random()->Range(1, mData.mGrenadeCount)`. Activate:
  `CanIssueCommand(&x6b4, body, 9 = kAS_LoopAttack)` -> `CBCLoopAttackCmd(kLAT_Zero,
  waitForAnimOver = true)` (type 7, `{0, 1, skipInto 0}`), `else if (xc58_ <= 0)` -> bare
  `CBodyStateCmd(kBSC_ExitState)`. Deactivate: `x6b4 = 0`, `mAttackRemTime = GetAverageAttackTime()` +
  `Random()->Float() * mAttackTimeVariation` (CPatterned 0x3e8) **only when `mgr.IsMultiplayer()`**
  (`mgr+0x16e8` bit 0x80), then a bare `kBSC_ExitState` and `CScriptTeamAiMgr::EndAttack(kAT_Projectile,
  mgr, xba0_teamAiMgrId, GetUniqueId(), false)`.
  `4174` -> `CSpacePirate::SpawnGrenade`, called from `DoUserAnimEvent 3B28`'s `kUE_Projectile` case (it
  owns the `--xc58_`): `GetLctrTransform(xa86_gunSeg)` -> origin, `mData.mGrenadeMinLaunchSpeed` seeds the
  speed, `GetObjectById(mAttackTarget)` (no target -> no grenade), aim point from `4030`, ballistic solve
  `3D7C`, `delta.z = 0`, `gunXf.GetForward()` fallback, one `Slerp` capped at rodata BD8 = 0.785 rad, a
  second `Slerp` toward `sUpVector` by the solved angle, `CTransform4f::LookAt(origin, origin + dir,
  sUpVector)`, then `CBouncyGrenade` (`rs_new 0x390`, name `"Bouncy Grenade"` = rodata F62, allocation
  tag `"??(??)"` = F5B) with `(AllocateUniqueId, name, CEntityInfo(areaId, NullConnectionList, true,
  kInvalidEditorId), LookAtTransform, CModelData() /*452C = default ctor*/, CActorParameters(),
  GetUniqueId(), speed, mData.BuildGrenadeData() /*this+0x890*/, 5.0 /*B78*/, mskInvertedBox,
  kInvalidUniqueId, 0.0 /*B9C*/, 0, nullptr, nullptr)` and `mgr.AddObject`.
  `4030` -> `GetGrenadeAimPoint`: `target->GetAimPosition(mgr, 0.5f)` (vtable slot 0x54), minus
  `0.5 * CPlayer::GetEyeHeight()` while the player is `kMS_Unmorphed`, and - when the launcher data has
  bounces - pulled 70% back toward the pirate (BE0) for `aim.z <= pirate.z + 2.0` (B60): the asm is
  `cror eq,lt,eq`, a `<=` test, **not** the equality one m2c prints.
  `3D7C` -> `SolveGrenadeLaunch`: ten candidates from 0 upwards in pi/40 steps (from pi/4 downwards when
  the target is above the gun), `speed^2 = (dist * 0.5*g*dist) / (dist*cos*sin - dz*cos^2)` - the same
  denominator as `CDarkCommando::SolveGrenadeLaunch`, not its negation - accepting the first value inside
  `[minLaunchSpeed^2, maxLaunchSpeed^2]` and otherwise keeping the closest miss, with
  `mData.mGrenadeMinLaunchSpeed` as the initial speed.
  Probe evidence (1080p, `MP2_GODMODE=1`, area 08, 1400 frames): 53 spawns on pirate 208, every one
  `added` with a solved speed (3.8-26.6 m/s) and `bounces 2` (the prefab's own launcher data - so the
  weapon-data property is present even though no connection lists the state); the boot stayed clean
  (0 `[port] caught`/reach-stub, cycle unchanged) and the plain harness run without the probe was clean too.
- **Launcher block, resolved (was the blocker for `58D8`/`4174`):** `CSpacePirateData` +0xc8..0x134, built
  by `fn_72_10C64` from the SLdr weapon data (`fn_72_149C` reader; struct `SLdrSpacePirateWeaponData`) and
  copied in by `fn_72_108D8`: `+0x00` `equippedWeapon`, `+0x04` `grenadeLauncher`, `+0x08` a 0x50
  `CBouncyGrenadeData` built by `fn_72_C0C` = (mass `grenadeMass`, speed `unknown_0xed086ce0`, damage
  `grenadeDamage` via `LdrToDamageInfo`, numBounces, gen ids **explosion, explosion, trail, effect** -
  the asm loads `+0x24` into both of the first two slots - `sound_GrenadeBounce`/`sound_GrenadeExplode`,
  then `grenadeMinAttackDist`/`grenadeMaxAttackDist` in the explode-sfx falloff/max-dist slots and the
  constants 0.1 (B3C) / 150 (B40) / bit set), `+0x58` **`unknown_0xa95a025b`** (default 3) = grenades per
  volley, the `58D8` Range cap and `xc58_`, `+0x5c`/`+0x60` `grenadeMinLaunchSpeed`/`MaxLaunchSpeed`
  (`+0x5c` is the `4174`/`3D7C` speed seed - 20/50, not the mass 25 the first reading guessed),
  `+0x64`/`+0x68` `grenadeMinAttackDist`/`MaxAttackDist` - **the old naming was right**, `fn_72_10C64`
  takes them from `weaponData.grenadeMin/MaxAttackDist`. The host struct now names all of it and
  `BuildGrenadeData()` reassembles the retail object; `mData+0x120` is no longer uninitialised.
- **The grenade-launcher triggers went in right after it** (unmarked, same reason - no pirate in area 08
  has the ATTK/FOLW connection that `HasAttackPattern` asks for): `ShouldLaunchGrenade 779C` is
  `mData.mEquippedWeapon == 1 && mAttackRemTime <= 0` (asm reads the launcher block's +0x00, not a
  distance); `InProjectileRange 76D0` switches on the same word - 0 (unarmed) is always in range, 1
  (grenade launcher) wants a live `mAttackTarget` with its squared distance inside
  `[mGrenadeMinAttackDist, mGrenadeMaxAttackDist]` squared (independent confirmation of that naming:
  they are the block's +0x64/+0x68), anything else is out of range; `HasAttackPattern 7810` is
  `GetConnectedObject(mgr, kSS_Attack, kSM_Follow) != kInvalidUniqueId`.
- **The 12 trigger stand-ins are gone (2026-10-20, this session):** `PortCSpacePirateTriggers.cpp`
  has no stand-in left - every one of the 39 triggers is now retail's (section 6 has the facts).
  (`PortCSpacePirateStates.cpp` had none either; the 14 vtable overrides in
  `PortCSpacePirate.cpp` were ported the same day - the next bullet.)
  Three of them carry a `// @` marker because the 208 cycle evaluates them (always false there):
  `ADC0` (AggressionCheck), `7970` (HasTargetingPoint), `55B8` (OffLine). The other nine are never
  evaluated, so they stay unmarked. `PortSpacePirateStandIn` is now called from nowhere but
  `PortCSpacePirate.cpp`'s own `StandIn` wrapper (the module's other stand-ins); the dead
  declarations in the states and triggers files are gone. Commits `6c8e91bcb`, `0af474a2f`,
  `33a4f2ae1`, `e2194e6b5`. The module is at 27/262 marked functions
  (`tools/port_progress.py`): every state and every trigger is ported, but only the states the
  208 cycle reaches are marked.
- **The 14 vtable overrides are all ported and now marked (2026-07-20, this session, branch
  `decomp/spacepirate-vt` off `port`).** The doc's old line that `PortCSpacePirate.cpp` "still has
  14 vtable-override stand-ins" was stale: the bodies went in with the states, but six of them were
  left unmarked. Each was re-read against the REL's asm and marked now:
  `3A88` (0x2C, the vtable tail call through the slot 0x40), `3A0C` (0x7C, PreRenderAllViewports),
  `17C8` (0x8C, GetTouchBounds), `339C` (0xA0, ScanVisorRender), `37C4` (0x120, Render) and
  `38E4` (0x128, PreRender). The other eight were already marked: `E0D8` (0x48), `E0AC` (0x2C),
  `3AB4` (0x34), `35CC` (0x100), `343C` (0x190), `E120` (0x48), `E090` (0x1C) and `18BC` (0x74) - the
  last two are never exercised by the area run, so they keep their "Retail fn_72_..." comment.
  asmcheck's findings on the six are the known false positives: the skipped ragdoll branches
  (`xb30_` is never set), the vtable tail call in `3A88`, the string's destructor in `18BC` and the
  uninitialised draw-flags word in `38E4`'s portal-plane tail (the plane is never set).
  The area run (08_sand_casualpirates) is clean: 0 `[port] caught` / `[reach-stub]`, the 208 cycle
  `Patrol -> Taunt -> Lurk -> TargetPlayer -> Run -> PathFind -> Lurk` repeats, and the pirate draws
  (screenshot `/tmp/opencode/sp_vt_shot.png`).
- **The marking pass over the ported-but-unmarked functions (2026-07-20, this session, branch
  `decomp/spacepirate-vt`):** every ported function in the three `PortCSpacePirate*.cpp` files was run
  through `tools/asmcheck.py check --all`. The marked count went from 61 to 67: `ADC0` (0xF8,
  AggressionCheck - the port's comment cited only its helper `AEE0`), `7970` (0x1FC, HasTargetingPoint -
  its helper `DE5C`), `4F28` (0x9C, InitLauncherModel), `36CC` (0xF8, RenderLauncherModel), `F0A0`
  (0x6C, SetupStateMachine) and `BCB0` (0x31C, TargetPatrol). All six are exercised by the area run:
  the log shows `AggressionCheck trigger 0` and `HasTargetingPoint trigger 0` 2858 times each,
  `InitLauncherModel (fn_72_4F28) weapon 0` for every pirate, the state machine's setup in every
  pirate's first frame and `state Taunt -> TargetPatrol` once. **No real mismatch was found** - the
  remaining asmcheck findings on them are the known false positives (the `TCastToPtr` calls, the
  navigation's `mInPosition` bit written through `SetInPosition`, the vtable tail call in `3A88` and
  the `fn_72_CB30` helper call, which is the port's `GetTargetPos`).
  **32 ported functions stay unmarked** because the area run never reaches them: the states WarpIn,
  Attack, Crouch, CoverAttack, Halt, Shuffle, TurnAround, Dodge, SquadJoin, Cover, TargetCover,
  Approach, WallHang, GetUp, Generate, DoubleSnap, Bounce, PathFindEx, Deactivate, LaunchGrenade and
  Captured (no state line for them in the run), the triggers CoverFind, CoverBlown, CoverNearlyBlown,
  CoveringFire, ShouldAttack, GetStrafeDir and BounceFind (never evaluated: the live cycle
  `Patrol -> Taunt -> Lurk -> TargetPlayer -> Run -> PathFind -> Lurk` does not list them, and
  `ShouldMove`/`ShouldAttack` are evaluated 0 times), and the main file's EnsureBodyActive,
  SpawnGrenade, Touch, Listen (no grenade launcher data, no team AI and no CMetroid in the area, and
  the pirate never touches the player). The loader `140` stays unmarked as well (asmcheck's 15
  findings on it are the property reader's own calls).
- **The pirates fight (2026-07-20, this session): 8 more of the 32 marked.** A console script
  (`/tmp/opencode/sp_fight_con.py`) arms the player (the beam, the missile and the bombs), activates
  every pirate, teleports next to each one and shoots it; the pirates then engage. The run's state
  lines show `TargetPlayer -> Attack` (38), `Attack -> Lurk` (37), `PathFind -> Dodge` (28),
  `Dodge -> Lurk` (28), `Attack -> JumpBack` (7), `JumpBack -> Lurk` (7), `PathFind -> Jump` and
  `Jump -> Lurk` (1 each) and `state (none) -> WarpIn` (2), and its trigger lines show
  `CoverFind trigger 0` (1822). Marked: `9F1C` (0x39C, Attack), `7DFC` (0x4D4, Dodge), `63C8`
  (0x448, JumpBack - the state had no retail id in its comment at all), `5310` (0x108, Jump),
  `7078` (0x528, WarpIn), `D720` (0xE0, UpdateHeldPosition - Lurk's update runs it every
  message), `ABC0` (0x1F4, CoverFind) and `CB30` (0x150, GetTargetPos - the cover search calls it
  for every candidate). The marked count is now 75. **No real mismatch was found**: the remaining
  asmcheck findings on the eight are the known false positives (the retail's own helpers called
  from C++ - `CB30`, `5510`, `5418`, `10D38`, `75DC` - `GetObjectById`, the 0x8f9 break-dodge
  bitfield and the member-hash comparisons on x902/x914).
  **24 stay unmarked** because the run still does not reach them: the states Crouch, Cover,
  CoverAttack, Halt, Shuffle, TurnAround, SquadJoin, TargetCover, Approach, WallHang, GetUp,
  Generate, DoubleSnap, Bounce, PathFindEx, Deactivate, LaunchGrenade and Captured (the pirates
  take cover only when a cover point is near, the wall hang needs a wall-hang point, the grenade
  launcher needs a launcher, the ambush needs an ambush connection), the triggers CoverBlown,
  CoverNearlyBlown, CoveringFire, ShouldAttack, GetStrafeDir and BounceFind (the live cycle does
  not list them and `ShouldMove`/`ShouldAttack` are evaluated 0 times), and the main file's
  EnsureBodyActive, SpawnGrenade, Touch and Listen (no launcher data, no team AI and no CMetroid in
  the area, and the pirate never touches the player - the player is teleported, not walked into).
- Open with the user: whether this work moves to a `decomp/spacepirate` branch as DECOMP_WORKFLOW
  expects.

## 8. Classification and gdb coverage (2026-10-21)

`SpacePirate_functions.tsv` (next to this note) classifies all 262 `.text` functions: **B 145** (module
behaviour, 0xD9EC bytes; 104 marked, 41 unmarked), **H 107** (0x3B94: host copies of shared code - the REL
runtime 11A30-11BFC, weak/EH dtors, the CBC*Cmd deleting dtors (vtables data_A8C-B20), rstl list/vector
templates, TToken copy chains, the SLdr ctor/dtor/reader 109C/FE8/13A4/149C (`LoadTypedefSpacePirateWeaponData`
on the host), B9E0, 5418/5510, 8A44, 10568, the copy ctors 106D4/108D8/10954, the dtor F10C/F1CC/F3B0/F444,
the static init 10E50, and the vtable thunks 30/40/48/54/5C/64 that equal a base class; 64 is marked), **T 10**
(0x6C8). T evidence: every one of the 65 PIRT placements on the disc uses the AFSM `0x5652a12f` (63) or
`0x7b49526b` (2), and neither names StartAttack 6334, LostInterest 635C, ShouldSpecialAttack 638C,
BounceFind 602C, Bounce 5D0C or PathFindEx 5EA4 (those names only appear in other modules' AFSMs
`0x7a71739a`/`0x7d9a41a2`/`0x214e18eb`/`0x68a7f573`); 5E28 is only called from 5D0C; 2EA0, EDDC and EDE4 have no
reference at all. 686C ShouldJumpBack stays B: JumpBack 63C8 calls it directly.

**gcov pass (2026-10-21)**: `build-port-cov`, seven runs (three area drivers, an
07_sand_alamoportalobservation L3 default run, and force specs `spacepirate_force.py`, `_force2.py`,
`_force3.py`), merged with `tools/cov/gcov_report.py ... --module SpacePirate`. The earlier markers hold
(their untaken edges are the cited dead ones: switch defaults, WarpIn's GRNT arms, GetUp's never-2
Search, SquadJoin's melee). Newly marked: Shuffle 8670 (all natural), Cover A3C0 and Generate C1A4 (one
forced call each). The pass found Generate's Activate and Update bodies swapped against retail
(asm 0xC1E8 / 0xC32C): a generating pirate never jumped; fixed.

**gcov pass 2 (2026-10-21)**: rerun on the build with five Activate/Update fixes (Generate C1A4 and,
found by checking every state's retail message dispatch, Bounce 5D0C, PathFindEx 5EA4, LaunchGrenade
58D8 and Captured 5610 had their Activate and Update cases swapped). `spacepirate_force4.py` (run
line in the spec) forces one call each: Crouch 98D8 and CoverNearlyBlown A988/CoveringFire A8D0 reach
ALL-ARMS, and Captured 5610 every arm but the switch default; all four are marked. Not closable by
one-call forces: WallHang 69AC (needs a wall-hang cover point with its waypoint connections: the
body never enters kAS_WallHang), LaunchGrenade 58D8 (a team that grants the projectile slot; the
multiplayer arm is dead), CoverBlown AA64 (a covering pirate with zero steering speed through the
step/distance tests), ShouldAttack A71C (only in Cover's and-chain after its Delays). Not in this
AFSM (0x5652a12f) at all, so they need another pirate FSM: DoubleSnap 99F0, Bounce 5D0C,
PathFindEx 5EA4, BounceFind 602C, StartAttack 6334, LostInterest 635C, ShouldSpecialAttack 638C.

**gcov pass 3 (2026-10-21)**: Ambushing 7630 did run: the old `fsm Ambushing` run (`gc_g`) was killed
by its timeout before exit, so no .gcda was written. Under gdb the forced state runs its Updates and
evaluates BreakAttack 6328 every frame. `spacepirate_force4.py` now sets x8f4_25_ceilingAmbush on one
Activate (the ceiling arm, asm 0x7650), which makes Ambushing ALL-ARMS. BreakAttack is branchless and
is marked. Disc scan (every AFSM/FSM2 asset, every PIRT placement's property bytes): PIRT objects use
only two FSMs, 0x5652a12f (63 placements) and 0x7b49526b (2). Neither lists DoubleSnap, Bounce,
PathFindEx, BounceFind, StartAttack, LostInterest or ShouldSpecialAttack, so those are dead in the
shipped pirate data; other FSMs list Bounce/ShouldSpecialAttack (0x7a71739a, 0x7d9a41a2, 0xcdb2a199),
PathFindEx (0x214e18eb) and LostInterest (0x1bbdb38f, 0xcf340923). Wall-hang cover points (COVR
property 0x969de5ff bit 0x10) ship in 02_sand_sandfalls L3: 000a0186 and 000a0180. With the player at
(-83.1, -72, -9.5) and pirate 000a010e placed at (-78, -96, -10), then `fsm Lurk`, the chain
CoverFind -> Run -> TargetCover -> PathFind -> WallHang runs naturally. That gives WallHang 69AC every
arm except the switch default and the no-cover-point arm (the older force run took that one), and
runs CoverBlown AA64 with 12 of its 20 edges still untaken. Pathofeyes check (cube PLAT 000e0147 blocked by
pirate 000e01b4): PathFind 8A90 and Lurk AF70 match the asm call for call. That pirate's Search
returns kR_Success on 1339 of its 1429 PathFind updates; it is chasing the player across the track, not
standing on a failed search.
BreakAttack 6328 is only in Ambushing's list, and Ambushing 7630 (marked before gcov) gets 0 calls
even after `fsm <pirate> Ambushing`: the trace shows the transition, then no trigger is evaluated
for that pirate (open).

**gcov pass 4 (2026-10-21, rebuilt cov build)**: the forces now take the forced arm on the second
Activate so the first keeps the natural arm (`spacepirate_force4.py`; Captured pinned to
`SP_FORCE_EID=001d0051`). Crouch 98D8 and Ambushing 7630 ALL-ARMS; Captured 5610 and WallHang 69AC every
arm but the switch default (WallHang marked). CoverBlown AA64 ALL-ARMS with `spacepirate_force5.py`
(sandfalls L3, four one-call forces) and marked. ShouldAttack A71C (`spacepirate_force6.py`) ran once,
8/20 edges untaken; unmarked. The seven dead states carry a 'dead in shipped data' comment and TSV note.
Only LaunchGrenade and ShouldAttack remain unmarked among live states/triggers.

**Helpers-lane gaps (2026-10-21)**: ChooseTarget EDEC now has retail's team-target flag (the loop
breaks at an inactive target, asm 0xEE74) and the squad branch (asm 0xEF50-0xEFA4). The squad radius
there is 50 (rodata B70), not the 20 of ChooseTeamTarget (B74). The port-only null check on the
player is gone. Every arm runs except the >30-unit swap, which needs a second player; EDEC is marked.
KnockBack C684 stores both AddEmitter handles in 0xc00 (asm 0xC944, 0xC9B8). ShouldFrenzy 3220 now
also rolls a burst while the bss_40 charge-player list is non-empty (asm 0x3290-0x32A4). ShouldAttack
A71C stays unmarked: console staging of its crowd fails (CoverBlown ends Cover first), see its comment.

**Coverage without probes**: the shared harness `tools/cov/run.sh` (docs/LANE_BRIEF.md, "Coverage without
probes") with the spec `tools/cov/specs/spacepirate.py` and the driver `tools/drivers/spacepirate_drive.py`:

    tools/cov/run.sh tools/cov/specs/spacepirate.py <out> 42b935e4,08_sand_casualpirates \
      --driver "python3 tools/drivers/spacepirate_drive.py {port} 3 8" --frames 14000 --env MP2_DEBUG_SPACEPIRATE=1

The driver wakes and fights the pirates over the console, then forces the states the FSM does not reach
(`fsm <id> <state>`, env `SP_FORCE`, on the pirate `SP_FORCE_EID`) and kills two; `SP_DELETE_TEAM=1` deletes
the squad managers before two last rounds. Results are labelled *forced* below when only the forced state
ran them. Runs: 08_sand_casualpirates (no layer) and 07_sand_alamoportalobservation L3, 14000 frames each,
0 `[port] caught` / reach-stub.

Marked from these runs (11): `10` IsListening, `8` GetSearchPath (400+ calls, branchless), `C488` Stuck (all
four arms: 400 entries vs 181 CPatterned::Stuck calls = the `<= 0.5 s` arm; CPatterned::Stuck true once;
Stuck true 7 times = PathShagged true, which is a tail call, so the caller-filtered breakpoint never sees it),
`C468` PatternShagged (both results), `ADB4` CoverCheck (both results in alamo), `8A84` InRange (59 calls),
`E090` GetOrigin (400+, the alamo team AI), `BBB0` Halt, `9478` Approach, `A310` CoverAttack and `5BF4`
Deactivate (*forced*: every message ran).

Marked from the 2026-10-21 tools/cov runs (3): `4F0C` ProjectileInfo (null arm data-dead: every one of the
65 PIRT placements carries the projectile 0xef485db9 = 0x681ed507, and the ctor builds the optional only for a
valid id, 0xFB34-0xFB40), `689C` AnimOver (wall-hang arm once, from a *forced* WallHang), `DE5C`
UpdateTeamTarget (`SP_DELETE_TEAM=2` in alamo L3: cast 20000, SetMemberTargetId 319, so the stale-id arm ran).
Marked from the second batch (4, console-driven via `SP_STEPS`): `686C` (bit set and held > 6 s, casual L3),
`77D0` (arm 1 from a warped-out trooper), `DFD4` (entry-valid 368, cast-null 226; melee role data-dead: the
three melee placements have no team), `EFD4` (cast-null 150 after `send <team> Delete` + the player deactivated).

Unmarked B (41) and what each still lacks:
- Never called in the runs: `0`/`18`/`24`/`17A0` (vtable thunks; the host inlines the direct calls),
  `C9DC`/`CA94` (only on a cinematic-camera edge in Think), `18BC` TryToBeCaptured (a capture), `51C8`
  TrooperWarpOut, `6328` BreakAttack, `99F0` DoubleSnap (no such state name in the AFSM: `fsm` refuses it),
  `A71C` ShouldAttack, `D608` (from Shuffle's close branch).
- Partly covered: `4F0C` ProjectileInfo
  `98D8` Crouch (*forced*, no cover point: only the null-cover arm),
  `A8D0`/`A988`/`AA64`
  (cover triggers, a few calls each), `3220` ShouldFrenzy (true in alamo, false in casual; its inner arms not
  traced), `CE30`, `EDEC`, `E3D8` (ragdoll branch), and the *forced*-only states `5610`, `58D8`,
  `69AC`, `8670`, `A3C0`, `C1A4` (messages ran, inner arms not traced).
- Not traced: the module head `90`/`B4`/`D4` (the host's REL runtime registers the module), the loader `140`,
  `C0C`, the ctor `F49C`, `10A10`, `10C64`, `10CE4`, `1854`, `2EA8`, the grenade bodies `3D7C`/`4030`/`4174`,
  and `E314` (an empty function the port does not need as a symbol).

## 9. sp20 slice (2026-10-22)

New marks, each ALL-ARMS in `tools/cov/gcov_report.py` over the natural run (08_sand_casualpirates L3), two
staged alamo runs (07_sand_alamoportalobservation L3: troopers 002a00f3/002a00f4 - the L3 ids print without the
layer byte) and the one-call forced runs of `tools/cov/specs/spacepirate_force7.py` (`force_ret.py`, every write
restored when the call returns). Forced arms are *labelled*.

| fn | body | how its arms ran |
|---|---|---|
| `90`/`B4`/`D4` | RELExit / RELMain / `PortRegisterSpacePirateFuncPtrs` | no branches; every boot |
| `140` | loader | all but `!modelData` (retail 0x7A4; data-dead: every placement has a valid ANCS). Retail's `new`-null arm (0xAE0) has no host arm |
| `10A10`/`10C64` | `PortInitSpacePirateData` / `PortInitLauncherBlock` | no branches; data+0xbc is never read (not stored), data+0xc4 is always 0.5 (0xDB14 uses the literal) |
| `E314` | `EmptyE314` (bare `blr`), called from Dead Activate (0x5C6C) and AcceptScriptMsg 0xE91C | no branches |
| `A71C` | ShouldAttack | `trigcall` with five pirates placed 4 units off the player in Attack: closer>3 and the non-player target; *forced*: the own list entry (target = the player), another area (this pirate's area 77), a dead attacker (the last attacking pirate's mAlive 0) |
| `3220` | ShouldFrenzy | natural low-HP roll; *forced*: no target + empty charge list, morphed player target, low HP with timer 1.0 / 0.0, the frenzy-chance bit. That bit (0x8f8 0x80) is written only by the ctor, as false (F6E8), so its two rolls are dead in retail |
| `3D7C` | SolveGrenadeLaunch (inlined into SpawnGrenade in the cov build) | `uevent <trooper> 0` (kUE_Projectile) with the troopers placed above / below the player; *forced*: launch speed range 0.1-0.2 and 200-201 |
| `4030` | GetGrenadeAimPoint | unmorphed player (`press x` first: the harness spawn in this area is morphed), *forced*: morphed player with 0 bounces, the pirate targeting itself (no player) |

sp20 also fixed: ctor xa18 = starting HP (it was 0, so Dodge 7DFC's low-HP test now sees a real value), the grenade
SFX falloff 0.1 / max distance 150, and 3D7C's NaN senses (0x3EC4/0x3F2C/0x3F60 `ble`, 0x3F74 `bge`).
Reclassified: `99F0` DoubleSnap B -> T (no such state in the shipped AFSM), `C0C` B -> H (inline
`CBouncyGrenadeData` ctor, CBouncyGrenade.hpp:43).

Still unmarked from the slice:
- `4174` SpawnGrenade: 11/12; the arm where the flat aim direction cannot be normalised (aim straight over the gun
  locator) needs an invalid gun segment or an exact player position; not forced.
- `58D8` LaunchGrenade: the team arms (1681/1684) need a trooper that is a member of a CScriptTeamAiMgr; the alamo
  troopers' team never takes them in the runs.
- `CE30` GetStrafeDir, `10CE4` (the WallHang waypoint search; 02_sand_sandfalls L3 staging not run): not reached.
- `F49C` ctor: needs shared/DOL header accessors: CBodyStateInfo `mChangeLocoAtEndOfAnimOnly` setter (body+0x56c
  0x80, 0xFF44), CPatterned x682 0x80 (0xFFA0), CPatternedInfo echo parameters (pInfo+0x2a4 0x80, 0xFFCC-0x101B0),
  CActor x154 0x40 clear for a floating corpse (0x10230).
- `17A0` GetRagDoll, `E3D8` Touch: the ragdoll (xb30) is not ported, so their arms cannot run (tail3, 2026-10-22: 17A0 entered 3 times and E3D8 400+, `xb30_` = 0 on every one - see section 12).

## 10. sp21 slice (2026-10-22)

New marks (gcov over `tools/cov/specs/spacepirate_force8.py`, `force_ret.py`, 07_sand_alamoportalobservation L3,
grenades by `uevent <trooper> 0`; forced arms *labelled*):

| fn | body | how its arms ran |
|---|---|---|
| `F49C` | ctor | natural: echo emitter (every placement: the PatternedAI loader default is `isEchoEmitter = true`), lights, projectile, AdditiveAim present; *forced* (ctor args at entry, not restored: per-load temporaries): data flags onlyAttackInRange 0x20 + floatingCorpse 0x10000 and no projectile, pInfo echo bit 0, actParms `mMakeLights` 0, `HasBodyState(0x16)` asked as 99 (melee arm). Left: 6 arms at 351/393 that are exception landing pads (host-only, no retail arm) |
| `4174` | SpawnGrenade | ALL-ARMS; *forced*: no target (65535), player `mPosition` x/y NaN for one call (flat aim not normalisable, gun forward used) |

sp21 also: header accessors (TARGET_PC) `CBodyStateInfo::SetLocoAnimChangeAtEndOfAnimOnly` (body+0x56c 0x80) and
`CPatternedInfo::GetEchoParameters` (+0x2a4); the ctor builds `CPirateEchoEmitter` and locks the projectile token.
NaN senses undone to plain compares (LANE_BRIEF rule: plain `ble`/`bge` are taken on NaN): 3D7C 0x3EC4/0x3F2C/0x3F60/0x3F74,
4174 0x42F4; CE30 0xCEDC `bge` now `!(magSq < distSq)`. `tools/retail_diff.py --kinds nan` is clean for 3220, A71C, 3D7C,
4174, F49C; it still lists leads in 1980, 253C, 28F0, 38E4, 5280, 8A90, ABC0, BBC0, C684, E498 (not audited).

Still unmarked:
- `58D8` LaunchGrenade: body matches the asm (enum values, cmd fields, 0x5A80 multiplayer bit). Team arms need a trooper that
  ran SquadJoin (Taunt) and is in an active FSM state; `fsm <trooper> LaunchGrenade` failed ("no state machine state") right
  after the uevent steps. Not run.
- `CE30` GetStrafeDir: body matches the asm (list = mgr+0x830 = kOL_ListeningAi: rstl::auto_ptr is 8 bytes; kMT_Solid 59;
  kSD_Left 2 / Right 3). ShouldStrafe is not a `trigcall` name, and a gdb breakpoint on it (~5000 hits) overran the 1100 s run.
- `10CE4`: not started (02_sand_sandfalls L3 WallHang staging).

## 11. sp22 slice (2026-10-22)

NaN audit of the `tools/retail_diff.py --kinds nan` leads (rule: `ble`/`bge`/`bne` after `fcmpo` are taken on NaN):

| fn | verdict |
|---|---|
| `8A90` PathFind | **fixed** 0x8D40, 0x8D74, 0x8DC4, 0x8E1C (cf18e35f4); 0x8E94, 0x917C, 0x92E8 match |
| `ABC0` CoverFind | **fixed** 0xACBC |
| `BBC0` HearPlayer | **fixed** 0xBC28 |
| `1980` Think | false positive: CMath::Max matches the `bge`, 0x1B90 `<` matches; 0x1F14/0x1F28 are in the unported ragdoll tail (below) |
| `253C`, `28F0`, `5280` | false positives: IsElectrocuting (`body+0x5b8 > 0`), Max, `>`/`<`, SetTimeToNextShot's inline `> 0` |
| `38E4` | lead is in the portal-plane code (xc68), not ported |
| `C684`, `E498` | false positives: `>=` matches the cror, `< -1.f` matches, CMath::Clamp |

New marks (gcov over build-port-cov, 0 caught/reach-stub; forced arms *labelled*):

| fn | body | how its arms ran |
|---|---|---|
| `CE30` | GetStrafeDir | ALL-ARMS 50/50: `trigcall <trooper> ShouldStrafe` (it is in skTriggers) with the other trooper ringed at 0.9/1.4/2.0/5.0 and put right/left (`tools/drivers/spacepirate_strafe.py`, alamoportalobservation L3); *forced*: the other-area skip (`spacepirate_force9.py`, area 77 for one call in ten) |
| `58D8` | LaunchGrenade | ALL-ARMS 14/14: `statecall` steps (Taunt joins the squad, then Activate/Update/Deactivate), claims granted and denied, layer-0 pirates with no squad (team 65535); *forced*: `mpforce 1` (multiplayer variation), msg 3 (switch default) |
| `10CE4` | waypoint predicate IsValid | no branches (2/2 lines); true natural (id 1429, sandfalls L3 wall hang, `spacepirate_force10.py`); false *forced* (id asked as 65535). Body moved out of the class so asmcheck/retail_diff parse it |

asmcheck CALL notes: CE30 0xCE84 and 10CE4 0x10D04 call `TCastToPtr` where the port uses `TCastToConstPtr` (same cast).

Gap found: Think `1980` (marked) lacks retail's xb2c ragdoll tail (0x1F08-0x2030: decrement xb2c, create the ragdoll
via fn_50_1938). Not fixed in sp22; the mark needs revisiting with the ragdoll work.

## 12. tail3 slice (2026-10-22): the two ragdoll rows, 17A0 and E3D8

Both bodies checked line by line against `build/G2ME01/SpacePirate/asm/auto_00_00000000_text.s`
(the port tree's dump). They match; what keeps them unmarked is the ragdoll, which the port never
creates.

| fn | body vs asm | what ran (run 2026-10-22, `42b935e4,02_sand_sandfalls` L3, 12000 frames, fast) |
|---|---|---|
| `17A0` GetRagDoll | 0x17A0 `lwz r3,0xb30`; 0x17A4/0x17A8 null -> 0; 0x17AC/0x17B0 the 0x70 byte's 0x40 (`extrwi. r0,r4,1,25`) -> 0; 0x17B8/0x17BC its 0x80 (`1,24`) -> `bnelr` with r3 = the ragdoll. The port's three `return nullptr`s and the final return are exactly that; `CRagDoll::mPrimed`/`mRenderBoundsValid` sit in the byte at 0x70 (port layout: mRenderBounds 0x58..0x70, bitfields 0x70) | **3 hits, all `xb30_` = 0, return null** (`retp=0`). Its only caller is `CGameProjectile::CanCollideWithGameObject` (`src/MetroidPrime/Weapons/CGameProjectile.cpp:146`); console `shoot u<id>` on each live trooper produced 400 `CanCollideWithGameObject` hits and those 3 `GetRagDoll` calls. The other two arms need a live ragdoll: dead in the port |
| `E3D8` Touch | 0xE3F4 `bl Touch__10CPatterned...`; 0xE3F8..0xE434 the six skips (0xb30, its 0x40 bit, `TCastToPtr<CScriptTrigger>`, the 0x20 byte's 0x80 = `GetActive`, 0x1a0's 0x8000 = `kTFL_DetectAI`, `rodata_B9C` = 0.0 with `ble` = `<=`); 0xE44C..0xE47C the three `fadds`/`stfs` push. The port has all six tests in asm order; the push is a comment (0xb8/0xbc/0xc0 are the module's pirate ragdoll, past the DOL CRagDoll's 0xa0) | **400 hits (the spec's cap), every one `xb30_` = 0**: the base call and the first early-out only. The trigger's own arms need a force-field `CScriptTrigger` as the touching actor, and the set below it needs the push |

Why the remaining arms cannot be forced the way ds42's were: a non-null `xb30_` at `17A0` makes
`CGameProjectile` dereference it (`GetRagDoll()->ProjectileCollision(...)`), so a fabricated pointer
crashes instead of covering an arm; only a real ragdoll object works, and the port's creation path
is retail's `1980` tail (0x1F08-0x2030), which is unported (its callee `fn_50_1938` is the upstream
CPirateRagDoll ctor, built - section 13). `E3D8` additionally needs
a `CScriptTrigger` with `kTFL_DetectAI` and a force field as the touching actor.

## 13. sp13 slice (2026-10-23): the grenade group re-check, the ragdoll blocker

**Grenade bodies** (`3D7C`, `4030`, `4174`; marked since sp20/sp21): re-read against the asm, equal.
`4174`: `--xc58` first (0x419C), gun locator 0xa86, angle seed B9C = 0.0, speed seed 0x8e4
(`mGrenadeMinLaunchSpeed`), `GetObjectById(0xa94)` null -> epilogue (0x41FC), `4030`/`3D7C`, flat
delta z = B9C, `CanBeNormalized` (0x42A4) else the gun forward (column 1: 0x18c/0x19c/0x1ac),
`GetAngleDiff > BD8` (0.7853982, 0x42F4 `ble`) -> `Slerp` to BD8, `Slerp(horizontal, sUpVector,
angle)`, `LookAt(origin, origin + dir, sUpVector)`, `new 0x390` null-tested (0x43C4) and
`AddObject` only when non-null (0x4508); ctor args: name rodata F62, `CEntityInfo(area 0x4,
NullConnectionList, true, kInvalidEditorId)`, owner 0x8, data `this+0x890`, B78 = 5.0,
`mskInvertedBox`, kInvalidUniqueId, B9C = 0.0, 0, null, null. One host-only difference: the port
casts the target to `CActor` (`TCastToConstPtr`) where retail passes the entity straight to `4030`
(its vtable slot 0x54 call); the attack target is always an actor. `3D7C`: BD0 FLT_MAX, BD4 pi/40,
BD8 pi/4, BDC 1.1920929e-7, B54/B58 the 1..10 counter, B44 0.5; senses as in section 10. `4030`:
`TCastToPtr<CPlayer>`, 0x38c == 0 (unmorphed) -> z -= B44 * eye height, 0x8b4 (numBounces) != 0 and
z <= pos.z + B60 (2.0, `cror lt,eq`) -> pos + BE0 (0.7) * (aim - pos).
Coverage (build-port-cov, 07_sand_alamoportalobservation L3, `uevent <trooper> 0` x21,
`specs/spacepirate_force11.py`): `3D7C` 14/14, `4030` 8/8, `4174` 12/12 ALL-ARMS, 0 caught. Forced
(one call each, restored): self-target, morphed + 0 bounces, speed 0.1-0.2 / 200-201, player x/y NaN
(flat aim not normalisable), no target (65535), and the **unmorphed player**: this area's harness
player is morphed on every natural call (`press x` does not change it; the spec's SPEC reads 1).
H rows confirmed: `452C` is only `bl __ct__10CModelDataFv` (0x4538, the `CModelData()` temporary);
`454C` is the 0xdc local's `CEntityInfo` dtor (null test 0x4564, `45A4` on +4, `Free` when r4 > 0);
`45A4` is the `rstl::vector<SConnection>` dtor (`Free(+0xc)`, then itself when r4 > 0). The host's
temporaries destruct the same objects at scope end.

**Ragdoll** (`17A0` GetRagDoll, `E3D8` Touch stay unmarked):
- 0xb30 is an `rstl::auto_ptr<CPirateRagDoll>`: the ctor stores 0 (0xFA14), the dtor frees it (0xF188),
  and the only other writer is Think's tail: `xb2c > 0` (0x1F18 `ble`) -> `xb2c -= dt`; `xb2c <= 0`
  (0x1F2C `cror lt,eq`) and 0xb30 null (0x1F3C) -> `new 0x11c` `CPirateRagDoll(mgr, this, 0x848
  sound, flags (0x8f5 bit 0x1 -> 4, bit 0x2 -> 3, 0x1FB4-0x1FFC), B70, BA8 = -3.0, a 14-float
  reserved_vector from rodata B04)`, stored by `fn_72_2060` (auto_ptr reset, 0x2010),
  `RemoveMaterial(0x29, 0x28)` (0x2024), `xb2c = 0` (0x2030).
- **0xb30 is non-null in shipped play**: `xb2c` is armed by `kUE_BecomeRagDoll` (3B28; gcov: the
  case ran 4 times for 4 kills by `damage <id> 1000` / `kill <id>` in the run above) and by a floating
  corpse (E498). So every ordinary pirate death builds the ragdoll in retail; the port arms the
  timer and never builds it (0xb30 null on all 71 `17A0` and 2000 `E3D8` calls,
  `specs/spacepirate_ragdoll.py`).
- `fn_50` is module 50 **PirateRagDoll** (13516 B text, 48 functions); `fn_50_1938` (0xA74) is
  the `CPirateRagDoll` ctor. It is upstream code (`src/MetroidPrime/Enemies/CPirateRagDoll.cpp`,
  in `files.cmake`), already built and used by CommandoPirate (`fn_9_3E1C` ->
  `PortCCommandoPirate.cpp` `rs_new CPirateRagDoll`) and CDarkTrooper. Callers of the ctor in the
  REL asm: SpacePirate `fn_72_1980` (0x2004) and CommandoPirate `fn_9_3E1C`. Not a blocker.
- **The blocker is SpacePirate's own**: Think `1980`'s ragdoll arms (0x1CA0-0x1EA8: skip CPatterned
  Think / bone tracking while primed, the transform from the ragdoll, its water, AdvanceParticles)
  and its tail (0x1F08-0x2030), plus `xb30_` typed `void*` in `CSpacePirate.hpp`. Once a ragdoll
  exists every consumer's ragdoll arm goes live, and these **marked** bodies carry that arm as a
  comment or only in part (they were marked on "the arm is dead", which is false): `17C8`
  GetTouchBounds, `24C4` PreThink (tests null only, not the 0x70 0x40 primed bit at 0x24F0),
  `38E4` PreRender, `3A0C` PreRenderAllViewports, `E0AC`/`E0D8` Detach/AttachActorToPirate (the
  0x118 0x40 write), `E498` AcceptScriptMsg (0xE924/0xE940, the TODO in the port). `253C`
  UpdateAttacks (0x25B4) and `C684` KnockBack (0xC6C4) have the null test in code. A ragdoll slice
  (follow CommandoPirate's `xbfc_ragDoll`) has to port all of them together; porting only the tail
  would build a ragdoll that nothing updates or renders.


## 14. sp14 slice (2026-10-23): the ragdoll, Think 1980 and its consumers

`xb30_` is `rstl::single_ptr<CPirateRagDoll> xb30_ragDoll` (one pointer at 0xb30). `fn_72_2060` is
its reset (the old one is null at 0x2010) and `fn_72_20A8` the CPirateRagDoll deleting dtor (tsv rows
fixed). Think `1980` is marked: head 0x19A8-0x1C9C (echo bounds, `!IsMultiplayer()` 0x1A20, the
cinematic collision 0x1A44-0x1A70, the timers, the attack helpers), the ragdoll choice 0x1CA0-0x1D58
(none, or unprimed on a sinking corpse: CPatterned Think + bone tracking unless 100% frozen 0x1CEC;
else CActor Think, UpdateAlphaDelta, UpdateHitDamageTime, `UpdateIngPossession` 0x1D44, UnFreeze),
Prime 0x1D74-0x1DD0 (identity rotation, translation kept, playback rate B9C = 0) or Update
0x1DD8-0x1E94 (water top: BA0 = -2^127 unless an active CScriptWater fluid, its trigger max z;
dt * death time scale; AdvanceParticles), over and still 0x1E98-0x1F04 (momentum 0, Stop, then once
mFadeToDeath, alpha delta BA4 = -1/3, 0x8f8 0x02), and the tail 0x1F08-0x2030 (NaN skips, 0x1F2C
`cror`; flags `(0x8f5 0x01 ? 4 : 0) | (0x8f5 0x02 ? 3 : 0)`, gravity B70 = 50, floating BA8 = -3,
radii B04 = 0.45 0.52 0.35 0.1 0.15 0.35 0.1 0.15 0.25 x6, `RemoveMaterial(Orbit, Target)`).
One host guard: the echo emitter (this+0xc8) is null-tested before its bounds store (0x19C0-0x1A00);
retail stores without a test, the emitter exists only when the pInfo's echo flag is set (ctor
0xFFCC). Its null arm is the host's, not a retail arm.

Consumers re-read against the asm: `17A0` (0x40 and 0x80, both needed), `17C8` (primed and bounds
valid -> `GetCachedRenderBounds`), `24C4` (the 0x24F0 primed bit), `38E4` (ragdoll PreRender 0x392C
when primed; bone tracking/ik chain only without a primed one), `3A0C`, `E0AC`/`E0D8` (0x118 0x40,
`CPirateRagDoll::SetActorAttached`, a TARGET_PC setter in the shared header), `E3D8` (the force field
added to the torso impulse 0xb8), `E498` kSM_Decrement 0xE924-0xE950 (clears 0x04 NoOverTimer and
0x20 ContinueSmallMovements; the old TODO).

Coverage (build-port-cov, gcov_report merged): natural runs `specs/spacepirate_ragdoll.py` +
`tools/drivers/spacepirate_ragdoll_drive.py` in 07_sand_alamoportalobservation L3, 08_sand_casualpirates
(floating corpses on water) and 06_sand_alamofront L2; forced runs `specs/spacepirate_ragdoll_force.py`
(`COV_GDB_SCRIPT=tools/cov/force_ret.py`, header has the commands). Result: `1980` 92/92 lines, 76/77
branches (the one left is the host echo guard's null arm), `17A0` 8/8, `17C8` 8/8, `24C4` 4/4, `38E4`
8/8, `3A0C` 4/4, `E3D8` 14/14, `E0AC` 2/2, `E0D8` 4/4, `E498`'s Decrement arms both; 0 caught/reach-stub.

| arm | how |
| --- | --- |
| no ragdoll / unprimed / primed, sinking and floating, Prime, Update in and out of water, over and still, fade once, the tail with flags 0 and 3 | natural |
| Think: multiplayer (0x1A28) | forced (FORCE_INNER `IsMultiplayer` -> 1); pirates are single-player only |
| Think: cinematic on/off and its exit with 0x8f5 0x01 (0x1A40-0x1A70) | forced (`IsInCinematicCamera` -> 1; 0x8f5 0x01 written for one call) |
| Think: in a projectile's path (0x1B0C) | forced (0x8f7 0x10 written, the body clears it) |
| Think: 100% frozen (0x1C78, 0x1CEC); frozen body with a ragdoll (0x1D4C) | forced (`GetPercentageFrozen` -> 1.0; `mFrozen` for one call) |
| Think: fluid id that is no water, an inactive water (0x1E24, 0x1E38) | forced (`InFluidId` -> the pirate; -> water 0x0443 after the driver's `send Deactivate`) |
| Think: delay ends with a ragdoll there (0x1F34); creation flag 4 (0x8f5 0x01) | forced (xb2c = 0.0001 for one call; the flag for one call). No shipped placement sets 0x20000 (scan of every SpacePirate SCLY) |
| `17A0` unprimed (0x17B4), bounds invalid (0x17BC) | forced (after Prime: unprimed for even ids; after every second Update: bounds invalid), run in alamoportalobservation with only these two |
| `38E4`/`3A0C` unprimed ragdoll | forced (3A0C entry leaves it unprimed until the next Think) |
| `E3D8` inactive trigger (0xE428) | forced (a water touching a primed ragdoll deactivated, not restored) |
| `E0AC`/`E0D8` all | forced (`pirateattach` console command: alive and ragdolled, twice, then `-`); the port's CMetroid only detaches |


1. `x8f4_24` mask is `flags & 3` (MP1 `& 1`): a 2-bit field or a different flag.
2. 0xa18 (pInfo+0x58) is the starting HP (`pInfo.GetHealthInfo().GetHP()`, ctor 0xF89C/0xF8B8; sp20). 0xb30 is the ragdoll (section 14).
3. The [?] rows of section 1 (`253C`, `28F0`, `3220`, `3D7C`, `CB30`, `EDEC`).

## ht2 class audit (2026-10-23, base 2275ab065)

A marked H/T row whose port body is this class's own override, method or FSM trigger/state is B; `tools/ht_audit.py` flagged them and lane ht2 reclassified them. Each tsv note starts with `ht2:` and gives the first asm reference.
- fn_72_64 is now B.
- Kept T: 5D0C/5EA4/602C/99F0, which are not in the shipped AFSMs.
