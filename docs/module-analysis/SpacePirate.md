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
| `4F28` | – | ctor-time init from data+0xc8 (launcher) | [probable] |

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
| 0xb30 | pointer = null (ragdoll?) | [?]; GetRagDoll `17A0` should confirm |
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
u16 bounce/explode sfx, 0.1, 150, 0.1, 150 (constants B3C/B40), bit 0x4c = 1], +0x58 (0xa95a025b),
+0x5c/0x60 launch speed min/max, +0x64/0x68 attack distance min/max}.
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
| Ported, unmarked | HearPlayer BBC0 (needs `mData.mHearingRadius > 0`, which is 0 in this area), SpotPlayer 938C, LineOfSight 9378 (0x8f7 bit 0x08), ShotAt 7BA0: no state these pirates reach (Patrol/Taunt) lists the last three, so nothing has exercised them |
| Done (`// @`) | 24C4 1980 DB48 45F8 28F0 253C E498 C538 C684 3B28 64 5C10 7B6C B59C (with DFD4 `SquadJoin`) AF70 (with D720 `UpdateHeldPosition`) 7868 BBA0 8A90 D800 (`UpdateGuardPosition`) D40C (`AvoidActors`) |

**CSpacePirateData floats are the MREA's, not a loader fault** (2026-10-19; loader instrumented behind
`MP2_DEBUG_SPACEPIRATE=1`, then reverted). Every one of the six actors' property blocks carries
`9579b1f2` aggressiveness, `f89ab419` coverCheck, `ed9bf5a3` searchRadius, `c3a27cf8` fallBackCheck,
`f0cf5dd7` fallBackRadius, `ed69488f` hearingRadius and `a64ab9b8` flags, each with a 4-byte payload,
so the `SLdrSpacePirate` ctor defaults (10/20/20/50 - confirmed printed before the read) never apply.
Loaded aggr/cover/search: `component` 50/50/20, `On Patrol` 20/50/20, `Already On` and `SKIFF JUMPER`
0/0/0, both `Teleporting Pirate` 50/50/20; `hearingRadius` is `0x00000000` on all six, so `HearPlayer`
cannot fire in this area (for retail as well). The decode is exact, not a mis-read: the ids match the
generated reader and `fn_72_140` id for id, `ReadFloat`/`ReadInt32`/`ReadUint16` all byte-swap under
TARGET_PC, and the properties after them (42-byte damage infos, 200-byte weapon data, 90-byte editor
properties, per-actor flags 0x10000/0x829c/0x40000) line up.

## 7. Port status / next (2026-10-19)

- Worktree `wt-mp2-port`, branch `port`; Taunt, Lurk, TargetPlayer, Run and PathFind are committed on top of `00e7e7de4`. The live path after a power-beam hit on pirate 208 is `Patrol -> Taunt -> Lurk -> TargetPlayer -> Run -> PathFind -> Lurk`, repeating, and it is what every new state gets verified against.
- Next slices, one `worker` lane each: the avoidance helper `D40C` (`CSteeringBehaviors::Separation` + its locomotion command, the last TODO in PathFind); then the remaining triggers (ShouldAttack A71C, ShouldMove 7BD4, ShouldRetreat 7C28, ...).
- Lanes kill their game by pid only (other worktrees' games share the machine).
- Open with the user: whether this work moves to a `decomp/spacepirate` branch as DECOMP_WORKFLOW expects.

## Open questions (verify first)

1. `x8f4_24` mask is `flags & 3` (MP1 `& 1`): a 2-bit field or a different flag.
2. Identity of 0xa18 (pInfo+0x58) and 0xb30 (null pointer; ragdoll slot?).
3. The [?] rows of section 1 (`253C`, `28F0`, `3220`, `3D7C`, `CB30`, `EDEC`).
