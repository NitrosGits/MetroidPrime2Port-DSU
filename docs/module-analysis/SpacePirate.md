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
| Ported, unmarked | HearPlayer BBC0 (needs `mData.mHearingRadius > 0`, which is 0 in this area), SpotPlayer 938C, LineOfSight 9378 (0x8f7 bit 0x08), ShotAt 7BA0: no state these pirates reach (Patrol/Taunt) lists the last three, so nothing has exercised them. ShouldMove 7BD4 and ShouldAttack A71C (ported 2026-10-20): nothing in the live cycle lists them either |
| Triggers 7BD4 A71C 7C28 82D0 | ported 2026-10-20. `7BD4` = `GetCoverPoint(x900) && !ShouldStay()`. `A71C` casts the attack target to CPlayer (not an id compare) and returns false once 4+ same-area pirates that are alive and `x8f6_31_inAttackState` sit closer to that player. `7C28` gate `x8f6_29_enableRetreat && !x8f6_24_trooper`; waypoint from `GetConnectedObject(kSS_Patrol, kSM_Follow)`, else a connection-list scan resolved with `GetIdForScript`; `mDestObj` keeps the **GetConnectedObject** id even when the scan found the waypoint (0x330; then mReflectedDestPos, mInPosition, ReleaseCoverPoint, hearNoise, mEnableAim, SetHitByPlayerProjectile). `82D0` reads 0x8f4 bit 0x20 = **nonAggressive** (mFlags 0x4), as MP1 does — mFlags 0x20 is `x8f4_29_onlyAttackInRange`, bit 0x04 — then 0xb24/0xb28 < 0.33, 0xa7c < 0.5, else the CMetroid-`mIsAttacking` (0xa40 bit 0x04) front test |
| 2026-10-20 run (08_sand_casualpirates, 3200 frames, 65 state lines) | cycle repeats `Patrol -> Taunt -> Lurk -> TargetPlayer -> Run -> PathFind -> Lurk` 3×; `ShouldRetreat` and `ShouldDodge` evaluated 363× for 208 (1815× over all six ids), every one false — no xf6 bit or damage timer ever opens them here; `ShouldMove`/`ShouldAttack` 0× |
| ShouldStrafe 960C + `GetStrafeDir` CE30 | ported 2026-10-20. `960C`: gate `!x8f4_26_nonAggressive`, target ahead of `GetForward()`, `(xb28<0.66 \|\| xb24<0.66) && xb0c==0` then center-normalized dot > 0.707 -> `CE30(mgr, 10)` (0xB58); else `!result && xa7c>1` and a `CPlayer` attack target within 15 (0xB88) -> `CE30(mgr, 5)` (0xB78). `CE30` (retail's own, not MP1's CantJumpBack): other same-area `CSpacePirate`s drop a side (dot vs `GetRight()` > 0.866d² or >0 with d²<3, 0xC14/0xB5C), each survivor must pass `mPathFindSearch.OnPath(center ± dist*right) == kR_Success` then a `BuildNearList` (anchor `center` if left open else `center-dist*right`, length `2*dist` when both, `MakeInclude(kMT_Solid)` = data 0x8A0/0x3B) + `CGameCollision::RayDynamicLineOfSightTest(center, right, dist, …)` (returns clear); both open -> `CRandom16::Next() & 0x4000` drops left, else right. Returns 2 = `kSD_Left`, 3 = `kSD_Right`, -1 |
| Patrol `BFCC` | ported 2026-10-20, marked: Activate = `SetLocomotionType(kLT_Relaxed)` + `x904 = GetLocomotionSpeed(kLA_Walk)/GetLocomotionSpeed(kLA_Run)` (asm `fdivs` on the body's own speeds), Update = `UpdateGuardPosition(mgr, dt)` (D800 does read `f1` = dt), Deactivate = `kLT_Combat` + once-ever `kSS_Attack`/`kSM_None` send gated on `x8f9_31_sentAttackMsg` (`clrlwi. 31` / `rlwimi ... 31,31`). Traced: load state gives `state (none) -> Patrol`, and `Patrol -> Taunt` for 208, so Activate/Update/Deactivate all run. The `x8f7_24_enablePatrol` half (`CPatterned::Patrol`; steering 1 then 0 at body+0x4c; turn speed /1.25 and *1.25 from rodata `C10` = 1.25; `xafc_patrolDestPos = mWaypointNavigation.GetDestinationPosition()`) is asm-only: no pirate here has a `kSS_Patrol`/`kSM_Follow` connection, so the bit never sets (trace line `enablePatrol` in `AcceptScriptMsg`, 0 hits over 6 actors) |
| Dodge `7DFC` | ported 2026-10-20, unmarked (no state line in this area). MP1's `Dodge` with `TryCommand(TryDodge/TryRollingDodge/TryBreakDodge)` replaced by three direct `CBCStepCmd(dir, type)` deliveries behind `CanIssueCommand(kAS_Step)` (asm types 1/3/2 = `kStep_Dodge`/`kStep_RollDodge`/`kStep_BreakDodge`). Activate: clear `x8f9_29_enableBreakDodge`; while `!x8fa_24_normalDodge && !x8f5_29_noBreakDodge` and `xb94_dodgeDelayTimer <= 0` (asm `cror eq,lt,eq`, not `== 0` as the pseudo-C shows) roll `0.15*(1 + 4*(xa18_ - HealthInfo()->GetHP())/xa18_)` (BFC/B50/B54) and re-arm from `Range(mData.mBreakDodgeMinTime, mBreakDodgeMaxTime)` (0x870/0x874); then `xb18_dodgeDir = GetStrafeDir(mgr, break ? xb20 : xb1c)` and `kAS_Ready` only when it is not -1. Update: the 50/50 (B44) draw happens **before** the CanIssueCommand test; break-dodge also drops `kMT_Orbit` and calls `SetOrbitRequestForTarget(GetUniqueId(), kOR_ActivateOrbitSource, mgr)` once `mStateMachine->GetTime() > 0.5f`. Deactivate: `kAS_NotReady`, `x8f8_29_noPlayerDodge = true`, re-add the orbit material when missing. `kOB_ActivateOrbitSource` does not exist in this tree; the enum is `CPlayer::EPlayerOrbitRequest`, 8 = `kOR_ActivateOrbitSource` |
| Shuffle `8670` | ported 2026-10-20, unmarked (no state line in this area). MP1's `Shuffle` (MP1 CSpacePirate.cpp:2020) maps line for line except: target = `GetTargetPos` (CB30) not the player, no `behaviourOrient` write in Echoes, and the random vector is the module's `D608` (= MP1 `Random2f`, not in this tree: XY from two `Float()`-0.5 draws, first draw is y, x forced to 0.001 (BC8) under 0.001, z 0, `Normalize`, then a third draw lerps the length min..max). Activate clears the blend to normal, then either `TooClose(mgr, CTriggerData(0))` (gated on `x8f4_28_noShuffleCloseCheck`, `extrwi 1,28`) → dest = translation + `mMinAttackRange` (0x3dc) along `(trans-target).AsNormalized()` + `D608(mgr,0,5)` and `x8f6_30_shuffleClose` set, or dest = target + `mMaxAttackRange` (0x3e0) * `(rand+1)` along that vector + `2*mMax` * `(rand-0.5)` along `Cross(Up, trans-target)` (`fn_72_8A44` = `CVector3f::Cross`) and the bit cleared; both set `mDestObj` invalid and `x904` = 1. Then `CPatterned::PathFind` for **every** message, `CommandMgr().SetTargetVector(player - trans)` (cmdMgr+0x34, unguarded in retail), then Update `AvoidActors`, Deactivate clears the bit |
| TurnAround `8464` | ported 2026-10-20, unmarked (no state line in this area). Activate: `xbc4_ = GetTargetPos(mgr)` (CB30), delta = that - translation with rodata `B9C` = 0 stored over Z, `Dot(GetForward(), delta.AsNormalized()) < rodata B64 = 0.8` → `kAS_Ready` (asm `bge` skips). Update: `mAnimationState.CanIssueCommand(body, kAS_Turn)`; the 3-D delta is **not** flattened, `IsMagnitudeSafe` → `CBCLocomotionCmd(Zero, dir, 1.0)` (zero `sZeroVector` stored first at 0x50, dir at 0x5c - same shape as Lurk's turn), else `kAS_Over`; both paths then `D800`. Deactivate: `kAS_NotReady`. Forward axis is `0x28/0x38/0x48` = `GetTransform().GetForward()`, translation `0x54/0x58/0x5c` |
| Halt `BBB0`, Approach `9478` | ported 2026-10-20, unmarked (neither gets a state line in this area). `BBB0` is 3 instructions with no message check: `x904 = 0` (rodata B9C) on every call. `9478`: Activate = blend `kSBM_Normal` (body+0x4c) + `x904 = 1` (rodata B54), Update = `AvoidActors`, no Deactivate case |
| TargetPatrol `BCB0` | ported 2026-10-20, unmarked (no state line in this area). Forwards to `CPatterned::Patrol` for **every** message (asm 0xBCE4, before the switch, unlike Patrol's gated enablePatrol half). Activate: `x904 = 1` (B54), cmdMgr `mSteeringMode = kSBM_FullSpeed` (body+0x4c), `GetConnectedObject(kSS_Attack 'ATTK', kSM_Follow 'FOLW')` -> `mWaypointNavigation.SetDestination` (**this+0x6c0 = nav+0x08**, not MP1's `mDestObj`), and `mInPosition` when `Dot(GetForward(), wp.trans - trans) == 0.f` exactly (0xBD90 `fcmpo/cror eq,lt,eq/bne`; the `|= 0x80` at 0xBDC4 is the nav `mInPosition` bitfield). Update (0xBDD8): `ObjectById(nav.GetDestination())` cast `CScriptAIWaypoint`; its `+0x164` flags bit1 (jump) or bit2 (drop) -> within `GetMaxSpeed()*((1.5dt+0.1)*scaleY) + xa70` sets `mInPosition` and `xaf8_jumpHeight = 3.f` (B5C) when bit1, else `0.f` (B9C); then when `BodyController()->GetCurrentStateId() == pas::kAS_Jump` (**`cmpwi 0xD` at body+0x37c = CBodyStateInfo `mState`**: the info base is 0x36c and `rstl::vector` is 0x10 there, so 0x37c is `mState`, not `mAdditiveState`) and the waypoint has no `kSS_Arrived`/`kSM_Next` connection, `CommandMgr().SetTargetVector(GetTargetPos(mgr) - trans)`; tail `xafc_patrolDestPos = mWaypointNavigation.GetDestinationPosition()` (0x6d4 = nav+0x1c). Deactivate: `mSteeringMode = kSBM_Normal` |
| Attack `9F1C` + `CheckBlade CC80` | ported 2026-10-20, unmarked (no state line in this area). `9F1C` is MP1's Attack with Echoes' members: Activate `kAS_Ready` (this+0x6b4 = 1), `kLT_Combat`, `xbc4_ = GetTargetPos`, `x908 = xbc4_ - GetBoundingBox().GetCenterPoint()`, `x904 = 0`, clear `x8f6` bit 0x20; `!x8f5_24_noMeleeAttack && TooClose(CTriggerData(0))` sets bit 0x20 + clears `x8f8` bit 0x40; the turn test (dot forward·`x908`.AsNormalized < 0.8 = rodata B64 -> `CBCLocomotionCmd(zero, x908, 1)`) runs **unconditionally**, not as MP1's else; then `x8f6` bit 0x01 and `xb90 = 0.75` (B84). Update's melee block (only while bit 0x20): `CanIssueCommand(body, kAS_MeleeAttack)` -> `DeliverCmd(CBCMeleeAttackCmd(pas::kS_One))` (asm delivers cmd 5 then fills the melee slot), `SetTargetVector(x908)`, `CC80`, and for `x8f5_27_shadowPirate` `mAlphaDelta` (CPatterned 0x40c) = -0.4 (rodata BB0) when this+0x6b4 == 3 else 1.0 + `xb90 = 0.75`; then always D800, D720. Deactivate clears bits 0x20/0x01 and this+0x6b4 = 0. `CC80` = MP1's CheckBlade (swoosh locator box = locator translation ± half the model scale, rodata B44 = 0.5, vs the attack target's bounds -> `ApplyDamage(this, target, this, data.mBladeDamage, MakeIncludeExclude(kMT_Solid, empty), zero)`; asm r7 = this+0x804 = data+0x44 = `mBladeDamage`); it **guards on `x8f8` bit 0x20** (0xCCA4 `rlwinm. r0,r0,26,31,31`, the header's `x8f8_26_alwaysAggressive`) and only *sets* bit 0x40 (`x8f8_25_appliedBladeDamage`), where MP1 tests and sets the same bit |
| Crouch `98D8` | ported 2026-10-20, unmarked (no state line in this area). MP1's Crouch **minus** its `TargetPlayer` call: the asm's only two calls are `SetLocomotionType` and `GetCoverPoint`. Activate: `SetLocomotionType(kLT_Crouch)` (asm `li r4,0` - Echoes' 0 is `kLT_Crouch`, MP1's enum has Relaxed there), cover point -> `x908_targetDelta = cp->GetTransform().GetForward()` (cover +0x28/0x38/0x48), `x904 = 0` (rodata B9C), `xa68_coverDir = -1`; Update: `CommandMgr().SetTargetVector(x908)` (body +0x34/0x38/0x3c) then D800 with **r3 = this** (the body pointer goes to r5); no Deactivate (`cmpwi r5,1` plus `bge`/`b<0` fall to the epilogue) |
| CoverAttack `A310` | ported 2026-10-20, unmarked. Activate delivers a bare `CBodyStateCmd(kBSC_LeanFromCover)` (asm builds `{vtable, 0xB}` on the stack and calls `DeliverCmd` on body +0x4; 0xB is `kBSC_LeanFromCover`, and 0xD = `kBSC_AbortScripted` confirms the enum) and sets `x8f6_31_inAttackState` (`rlwimi ... 31,31` = 0x8f6 LSB). Update = D800 with r3 = this; Deactivate clears that bit; msg 3+ and negatives do nothing |
| TargetCover `78DC` | ported 2026-10-20, unmarked. Activate only (`cmpwi r5,0`, everything else bails before the work): cover point -> `mDestObj = x900` (`sth 0x330`) and `SetDestPos(cp +0x54)`; unconditionally `mReflectedDestPos = GetTranslation()` (`0x340..0x348`) and `mInPosition = false` (`rlwimi` clears 0x34C bit 0x80) |
| Cover `A3C0` | ported 2026-10-20, unmarked. MP1's `Cover` with Echoes inlining `TryCommand(mgr, kAS_Cover, &CPatterned::TryCover, x79c_coverDir)`: Activate bails while the body state (`lwz 0x37c`) is already 0x13 `kAS_Cover`, else takes the cover point, `xa68_coverDir = (cp->GetAttackDirection() >> 1) & 1` (asm `bl fn_800EC6D8` with the point in r3, `extrwi r0,r3,1,30`), `x6b4 = 1` (`kAS_Ready`), `SetDestPos(cp +0x54)`, and inside `CanIssueCommand(&x6b4, body, 0x13)` delivers `CBCCoverCmd(dir, cp translation, -cp forward)` (type 0x17, `{vtable, 0x17, dir, x/y/z, -fx/-fy/-fz}` on the stack, 0x24 bytes). Update repeats the guarded delivery, then inside the cover-point block `SetTargetVector(-cp forward)` (body +0x34), and calls D800 whether or not a point exists. Deactivate stores 0 into 0x6b4 (`kAS_NotReady`) |
| Skid `94D0` | ported 2026-10-20, unmarked. MP1's Skid: Activate `xb0c_strafeDelayTimer = 4.0` (rodata B50) + `x8f6_31_inAttackState`; Update delivers `CBCStepCmd(xb08_skidDir, kStep_Normal)` while the body state (0x37c) is not 3 `kAS_Step`; Deactivate clears the flag |
| GetUp `B3D4` | ported 2026-10-20, unmarked. Activate: `x6b4 = 1`, `SquadReset` (`fn_72_DEDC`), `xbb0 = 0` (B9C) and `x8fa_25_` set (0x8fa bit 0x40 - MP1's GetUp has no such bit). Update: body state 2 `kAS_LieOnGround` **and** `mPathFindSearch.Search(trans, trans) == 2` `kR_NoSourcePoint` -> `SetPendingDeath(true)` (this+0x421 bit 0x10), else `CanIssueCommand(&0x6b4, body, 1)` -> `CBCGetupCmd(kGetup_Zero)` (bare type-0 command, zero payload); both paths then the leash timer. Deactivate: `x6b4 = 0` and clear `x8fa_25_` |
| Jump `5310` + leash `5280` | ported 2026-10-20, unmarked. `5310`: Activate `x6b4 = 1` + `xbb0 = 0` (B9C); Update inside `CanIssueCommand(&0x6b4, body, 0xD kAS_Jump)` sets `x8f7_30_jumpVelSet` (0x8f7 bit 0x02) and delivers the jump command that `5510` builds at `{vtable, type 0x11 kBSC_Jump, mType 0, mAnimationVariant 0, waypoint1 = mDestPos (this+0x334), waypoint2 = sZeroVector, mInitialState 2, mFacingFlags 2, mWallJump 0}` (2 is **both** `kJS_Loop` and `kFF_AmbushJump`, so the tree's ctor order is not in question), then the leash timer; Deactivate clears the bit and zeroes `x6b4`. `5280` = MP1's `UpdateLeashTimer`: `xbb0 += dt` while `GetPercentageFrozen() != 1.0` (B54, `fcmpu`+`beq`) and `!(body+0x5b8 > 0)` - 0x5b8 is `mElectrocutionDur`, i.e. `!IsElectrocuting()` |
| `5418`, `5510` | not states: `5418` is the out-of-line delivery of a 0x34-byte command (`DeliverCmd` by type, then the command copied to mgr+0x250..0x27C - the port's `CommandMgr().DeliverCmd(cmd)` covers it), `5510` the `CBCJumpCmd` builder above. `558C` (0x2C) is the `Landed` trigger (tail call through vtable slot 0x110 = `IsOnGround`), `52F8` (0x18) the `Leash` trigger (`xbb0_leashTimer < arg`, `fcmpo` + `extrwi`) |
| WallHang `69AC` + WallDetach `68E4` | ported 2026-10-20, unmarked. `69AC`: Activate `x6b4 = 1`, `x8f7_29_inWallHang`, cover point -> the point's `kSS_Arrived`/`kSM_Next` connection resolved through `CheckConnectedObject_if(..., CValidEntityPredicate)` -> waypoint -> `mDestObj = waypoint->GetUniqueId()` (**waypoint +0x8**, where MP1 stores the `GetIdForScript` result), `SetDestPos`, `mReflectedDestPos`, `mInPosition = false`; then `x908 = cover forward` (still inside the cover branch), `x8f6_31_inAttackState`, `mBoneTracking.SetActive(false)`, `x8fa_30_`. Update: while `x6b4 == 1` and `GetAngleDiff(forward, x908) > 0xB F8 = 0.2618 rad (15 deg)` deliver `CBCLocomotionCmd(zero, x908, 1.0)` and skip the hang command; otherwise `CanIssueCommand(body, 0x14)` -> `CBCWallHangCmd(mDestObj)` (type 0x18); while body state `0x37c == 0x14` set `x8fa_30_ = !GetCurrentState()->UnkVtable2C()` (**CBodyState slot 0x28**, the tree's guessed name), and always `mBurstFire.SetBurstType(1)` (`stw 1, 0xa98` = CBurstFire's first field). Deactivate clears `x8f7_29`, `x6b4 = 0`, `x8f6_31`, `SetActive(true)`, `x8fa_30_`. `68E4` is 3 cases only: Activate sets `x8f7_29` + `x8fa_30_` + `x8fa_31_`, Update delivers a bare `CBodyStateCmd(kBSC_ExitState)` (type 0xA), Deactivate clears those three |
| JumpBack `63C8` + `CantJumpBack D2A0` | ported 2026-10-20, unmarked. `63C8` opens with `ShouldJumpBack(mgr, CTriggerData(0))` (686C) and returns on false. Activate bails when `x8f4_29_onlyAttackInRange` (0x8f4 bit 0x04) or `CantJumpBack(mgr, -forward, 5.0)` (B78) says no; then caches `GetSearchPath()->GetCharacterHeight()` (path +0xd0 = this+0x9f0), sets it to `5 + height`, builds `dest = translation + 10.0 (B58) * forward` and needs `Search(translation, dest) == kR_Success`, `|back() - dest|² < 3.0` (B5C) and `|RemainingPathDistance(translation) - 10.0| < 4.0` (B50) before `xafc_patrolDestPos = back()`, `xaf8_jumpHeight = 5.0`, `x8f9_25_useJumpBackJump`, `x6b4 = 1`; the height is restored on every exit. Update: bit clear -> `CBCStepCmd(kSD_Backward, kStep_Normal)` + `SetTargetVector(GetTargetPos - trans)`; bit set -> `CanIssueCommand(body, 0xD)` -> the `5510` jump command with `(mDestPos, kJT_Normal, kJS_IntoJump, 0, kFF_AmbushJump)`. Deactivate: `x6b4 = 0` + clear the bit when set, and always `xbac_holdPositionTime = 0`. `D2A0` is **not** MP1's three-LOS CantJumpBack: `OnPath(centre + dist*dir)` (centre = bbox centre) **returning success short-circuits to false**, and only off-path does it build a `CMaterialList(kMT_Solid)` filter (data 0x89C = 0x3B), `BuildNearList(centre, dir, dist, filter, this)` and return the **raw** `RayDynamicLineOfSightTest` byte (1 = clear) - retail's polarity, unproven here |
| DoubleSnap `99F0` | ported 2026-10-20, unmarked. MP1's DoubleSnap with Echoes' members. Activate: `kAS_Ready` only when `!x8f5_24_noMeleeAttack`, `xbc4_ = GetTargetPos` (CB30), `x908 = xbc4_ - translation`, `x904 = 0`, `x8f6_26_enableMeleeAttack`, `xb10_meleeSeverity = kS_One`, `x8f8_25_appliedBladeDamage` cleared, `x8f6_31_inAttackState`, `x8f9_30_closeMelee` cleared, then `fn_72_9E64(&bss_40, &uid)` = `mChargePlayerList.remove(GetUniqueId())` (**a module-level static `rstl::list<TUniqueId>`**, MP1's `static rstl::list<TUniqueId> mChargePlayerList`; its only other caller is ChooseTarget's EDEC path). Update: `CanIssueCommand(body, 7)` -> `CBCMeleeAttackCmd(xb10_meleeSeverity)`; then while `xb10 == 1 && x6b4 == 3` the second hit needs `|target - trans|² < mMinAttackRange²` (0x3dc) and `Dot(delta.AsNormalized(), forward) > -0.123` (rodata C08) -> `x6b4 = 1`, `xb10 = 2`, apply bit cleared, `x908 = delta`, `closeMelee`; the closeMelee branch re-reads the target delta, `SetTargetVector(x908)`, the shadow variant (0x8f5 bit 0x10) sets `mAlphaDelta` (0x40c) to -0.4 (BB0) at `x6b4 == 3` else 1.0 (B54) + `xb90 = 0.75` (B84); tail D800, D720, CC80. Deactivate clears 0x8f6 bits 0x20/0x01 and `x6b4 = 0` |
| Enraged `A2B8`, Deactivate `5BF4`, RemoveFromWorld `5B88`, Ambushing `7630`, PostWarpOut `6DA4` | ported 2026-10-20, unmarked (small Echoes-only states). `A2B8` (0x58): Activate delivers a bare `CBodyStateCmd(kBSC_ExitState)` (type 0xA). `5BF4` (0x1C, seven instructions, no switch): Activate only -> `SetPendingDeath(true)` (this+0x421 bit 0x10, the same bit `GetUp` sets). `5B88`: Activate -> `SetActive(false)` (vtable slot 0x1C) then `mgr.DeleteObjectRequest(uid)`. `7630` (0x4C): Activate -> `SetLocomotionType(x8f4_30_ ? 6 : kLT_Crouch)` (6 has no name in the tree: `pas::kLT_Internal6`). `6DA4` (0xC8): Activate -> unless `x8fa_26_` (0x8fa bit 0x20) post `CScriptMsg(uid, uid, kSM_Deactivate)` (0x44435456) to itself via `mgr.DeliverScriptMsg`, always `xc04_warpPoint = -1`, and `DeleteObjectRequest` when `x8fa_27_` (0x8fa bit 0x10) |
| WarpIn `7078`, WarpOut `6E6C`, Generate `C1A4` | ported 2026-10-20, unmarked. `6E6C`: Activate `x6b4 = 1`, `xc04_warpPoint = 1`, clear 0x8fa bit 0x08; Update `CanIssueCommand(&x6b4, body, 0xC)` -> `CBCGenerateCmd(kGType_One, Zero, false, false)` (type 0xF, `{1, zero, -1, flags 0}`), else while the body state (0x37c) is 0xC it arms 0x8fa bit 0x08 + `xbd0` = `GetAnimTimeRemaining()`, then `mColor.SetAlpha(min(1, remaining/xbd0))` (rodata B54 = 1, the `fmuls` by BB8 = 255 is `CColor::SetAlpha`'s own ToUint8, whose GQR is `OS_FASTCAST_U8`); Deactivate `SetAlpha(0)` + `mAlphaDelta = 0`. `7078`: Update sets `x6b4 = 1`, alpha 0, `mAlphaDelta` 0, `xc04_warpPoint = 0`, clears 0x8fa bit 0x08 and `RemoveMaterial(Character, Solid, Target, Orbit)`; Activate switches on `xc04_warpPoint`: at 0 it reserves `GetConnectionList().size()`, pushes every `kSS_GRNT`/`kSM_Follow` connection's `GetIdForScript` -> `CScriptWaypoint`, teleports to `points[Random()->Range(0, n-1)]` via `SetTranslation` + `SetTransform(CQuaternion::FromMatrix(wp->GetTransform()).BuildTransform4f(GetTranslation()))`, then `BuildNearList` over a +/-10 box (B58) filtered `MakeInclude(Solid, Player, Character)` and `DetectDynamicCollisionBoolean(*GetCollisionPrimitive(), GetTransform(), ...)`: clear -> `xc04_warpPoint = 1` + `AddMaterial(Character, Solid, Target, Orbit)`; at 1 the same generate command + fade-in `SetAlpha(max(0, 1 - remaining/xbd0))`; Deactivate `kAS_NotReady`, those four materials back, alpha 1, `xc04_warpPoint = -1`, clear 0x8fa bit 0x20. `C1A4`: Activate delivers `CBCJumpCmd(xafc_patrolDestPos, x8f4_25 ? kJT_Ambush : kJT_Normal, kJS_IntoJump, 0, kFF_AmbushJump)` while `CanIssueCommand(&x6b4, body, 0xD)`; the ceiling variant (`x8f4` bit 0x40) then aims at `translation + sDownVector` with `xaf8_jumpHeight = 0` and skips the rest, else `GetConnectedObject(kSS_Attack, kSM_Follow)` -> `CActor` gives the destination with height 3 (B5C) and `kAS_Over` is the fallback, then `kLT_Combat`; tail `if (x6b4 == 2) SetLocomotionType(kLT_Combat)` (2 = `CAnimationState::kAS_Repeat`) and always `SetTargetVector(GetTargetPos - trans, z = 0)`. Update sets 0x8f7 bit 0x40 (`mEnableAim`), `AbortScriptedAnimations`, `SquadJoin`, the once-ever `kSS_Attack`/`kSM_None` send on `x8f9_31`, `x6b4 = 1`, then the same ceiling-or-connection destination choice. Deactivate `kAS_NotReady`, `kLT_Combat`, `xaf8_jumpHeight = 3`, clear 0x8f4 bit 0x40, `mBoneTracking.SetActive(true)` + `SetTarget(mAttackTarget)` |
| Bounce `5D0C`, PathFindEx `5EA4`, Captured `5610` | ported 2026-10-20, unmarked. `5D0C`: Activate -> `mStateMachine->GetTime() > 0.1` (rodata B3C) and body state not 0xD = `kAS_Jump` -> `StateMachineState().SetCodeTrigger()` (the asm sets bit 0x80 of state-machine +0x3c, i.e. the `mCodeTrigger` bit). Update: `xb14_jumpPoint` -> `CScriptAiJumpPoint`, its `GetJumpTarget()` (+0x182 = `mNextWaypoint`) -> `CScriptWaypoint`, then the two-waypoint `CBCJumpCmd(xafc_patrolDestPos, wp->GetTranslation())` built by `5E28` (type 0x11, `kJT_Normal`, variant 0, `kJS_IntoJump`, `kFF_AmbushJump`, **wall jump set** - the tree's 2-arg ctor exactly). `5EA4`: `CPatterned::PathFind` runs for **every** message before the switch; Activate -> `AvoidActors`, early-out when 0x8f9 bit 0x10 (`inRange`) is set, else `xa1c = maxSpeed*((1.5dt + 0.1)*scaleY) + xa70` (BEC/B3C as in `8A90`) and `inRange = |jumpPoint - trans|^2 < xa1c^2` (`mfcr`/`rlwimi 5,27,27` copies CR0[LT]); Update and Deactivate clear the bit. `5610`: Update -> `mDisabledAnimationDeltas = 3` (the 2-bit field in 0x422), `Stop()`, `x6b4 = 1`, `CBCGenerateCmd(kGType_Seven)` (type 0xF, `{7, zero, -1, flags 0}`), `RemoveMaterial(Orbit, Target, Solid)`, `SetDrawShadow(false)`; Activate delivers the same command behind `CanIssueCommand(&x6b4, body, 0xC)`; Deactivate `x6b4 = 0` |
| 0x6b4 | is a **`CTryCommand`**, not `CAnimationState`: the reloc at fn_72_9F1C 0xA130 is `ShouldRetry__11CTryCommandFRC15CBodyControlleri` (the tree's `CAnimationState::CanIssueCommand` is the same function under an MP1-derived name, so the ported calls stand). Its 4-byte state word is `CAnimationState::EState`: 0 NotReady, 1 Ready, 3 Over |
| 0x6d4 | not `mDestPos` (that is 0x334): `mAnimationState` is 4 bytes at 0x6b4, `CWaypointNavigation` (0x3c) starts 0x6b8, its `mDestinationPosition` is +0x1c; `TargetPatrol`'s post to 0x6f0 = nav+0x38 = `mInPosition` confirms the base |
| 2026-10-20 run #3 (same area, 3200 + 1200 frames) | after Patrol: cycle still `Patrol -> Taunt -> Lurk -> TargetPlayer -> Run -> PathFind -> Lurk` 4x, 0 reach-stubs, player pushed 372.4,-284.1 -> 377.9,-285.9 |
| 2026-10-20 run #2 (same area, 3200 frames) | `ShouldStrafe` evaluated 484× for 208, all false — the 0.66 s windows and the `xa7c>1` branch never open, so `CE30` is never reached; cycle now repeats 4×; 0 reach-stubs |
| Done (`// @`) | 24C4 1980 DB48 45F8 28F0 253C E498 C538 C684 3B28 64 5C10 7B6C B59C (with DFD4 `SquadJoin`) AF70 (with D720 `UpdateHeldPosition`) 7868 BBA0 8A90 D800 (`UpdateGuardPosition`) D40C (`AvoidActors`) 7C28 82D0 960C |
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

## 7. Port status / next (2026-10-20)

- Worktree `wt-mp2-port`, branch `port`; Taunt, Lurk, TargetPlayer, Run, PathFind and the helpers
  `D720`/`D800`/`D40C` are committed, plus the five triggers of the live cycle (last: the commit
  after `cd0c05113`, `port: CSpacePirate retreat/dodge/move/attack triggers are retail's`). The live
  path after a power-beam hit on pirate 208 is `Patrol -> Taunt -> Lurk -> TargetPlayer -> Run ->
  PathFind -> Lurk`, repeating, and it is what every new state gets verified against.
- Triggers done: 7C28, 82D0, 960C marked (the run evaluates them, always false here); 7BD4 and A71C
  ported but nothing in the cycle lists them; `GetStrafeDir` CE30 ported but its caller's branch
  never opens in this area.
- Movement extras are done: `Skid 94D0`, `GetUp B3D4`, `Jump 5310` + leash `5280`, `WallHang 69AC`,
  `WallDetach 68E4`, `DoubleSnap 99F0`, `JumpBack 63C8` + `CantJumpBack D2A0` are ported (unmarked, section 6).
  `Enraged A2B8`, `Deactivate 5BF4`, `RemoveFromWorld 5B88`, `Ambushing 7630` and `PostWarpOut 6DA4` are done
  (section 6). **1 stub left, in PortCSpacePirateStates.cpp** (`grep PortSpacePirateStandIn`): only
  `LaunchGrenade 58D8` (0x254). `WarpIn 7078`, `WarpOut 6E6C`, `Generate C1A4`, `Bounce 5D0C`,
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
- **Next (last state): `LaunchGrenade 58D8` + the `4174` bouncy-grenade spawn.** `58D8` is decoded:
  Update first, when `xba0_teamAiMgrId != kInvalidUniqueId`, calls
  `CScriptTeamAiMgr::StartAttack(kAT_Projectile, mgr, xba0_teamAiMgrId, GetUniqueId())` and on refusal
  sets `x6b4 = 3` (`kAS_Over`) and returns; otherwise `x6b4 = 1` (`kAS_Ready`) and
  `xc58_ = mgr.Random()->Range(1, mData+0x120)`. Activate: `CanIssueCommand(&x6b4, body, 9 =
  kAS_LoopAttack)` -> `CBCLoopAttackCmd(kLAT_Zero, waitForAnimOver = true)` (type 7, `{0, 1, skipInto
  0}`), `else if (xc58_ <= 0)` -> bare `CBodyStateCmd(kBSC_ExitState)`. Deactivate: `x6b4 = 0`,
  `mAttackRemTime = GetAverageAttackTime()` + `Random()->Float() * mAttackTimeVariation` (CPatterned
  0x3e8) **only when `mgr.IsMultiplayer()`** (`mgr+0x16e8` bit 0x80), then a bare `kBSC_ExitState` and
  `CScriptTeamAiMgr::EndAttack(kAT_Projectile, mgr, xba0_teamAiMgrId, GetUniqueId(), false)`.
  `4174` is called from `DoUserAnimEvent 3B28`'s `kUE_Projectile` case (the port has a TODO + `--xc58_`
  stand-in at `PortCSpacePirate.cpp:1248`): it decrements `xc58_`, takes
  `GetLctrTransform(xa86_gunSeg)`, reads that transform's translation, needs a live `mAttackTarget`
  (`GetObjectById` -> else no grenade), gets the target's eye point from `4030`, shapes the launch with
  `3D7C` (0x2B4, 2D arc/Cos-Sin geometry) and two `CVector3f::Slerp` calls (rodata BD8 = 0.785 rad
  limit, then `sUpVector`), builds the transform with `CTransform4f::LookAt`, then
  `CBouncyGrenade` (0x390 bytes, name from rodata `C38+0x323`) with the tree's ctor exactly:
  `(AllocateUniqueId, name, CEntityInfo(areaId = this->mAreaId, NullConnectionList, kInvalidEditorId),
  LookAtTransform, CModelData from 452C, CActorParameters(), GetUniqueId(), f1 = mData+0x124,
  *(CBouncyGrenadeData*)(this+0x890) = mData+0xd0, f2 = 5.0 (B78), mskInvertedBox,
  TUniqueId = kInvalidUniqueId, 0.0 (B9C), 0, nullptr, nullptr)` and `mgr.AddObject`s it. Needs
  `4030` (0x144) and `3D7C` (0x2B4) decoded first, plus `452C` (the CModelData builder at 0x452C).
- **Launcher-data finding (blocks `58D8`, verify before writing it):** `CSpacePirateData` keeps a 0x6C
  launcher block at +0xc8 (`fn_72_10C64` ctor: `{r4, r5, 0x50-byte copy of *r6 via 10954, r7 int at
  +0x58, 4 floats at +0x5c..0x68}`; `fn_72_108D8` copies it into `CSpacePirateData`). The port's loader
  (`PortLoadSpacePirate`) maps only 4 of its fields, so `mData+0x120` (= block +0x58, the int
  `58D8` hands to `Range`) is **uninitialised stack** there, and `mGrenadeMinAttackDist`/
  `mGrenadeMaxAttackDist` are mapped from the SLdr attack distances onto the block's last two floats
  (+0x64/+0x68), which the ctor fills from `r6`'s tail rather than from those properties - so the
  naming may be off. Identify block +0x58 (candidate: SLdr `unknown_0xa95a025b`, default 3, or
  `grenadeNumBounces`) and the +0x5c..0x68 floats (candidates: `grenadeMass` 25,
  `unknown_0xed086ce0` 0.4, `grenadeMinLaunchSpeed` 20, `grenadeMaxLaunchSpeed` 50 - `4174` reads
  +0x124 = +0x5c as the grenade's launch speed, and 25 m/s there would match) before porting `4174`;
  map the fields properly in `CSpacePirateData` + the loader rather than reading raw offsets.
- Open with the user: whether this work moves to a `decomp/spacepirate` branch as DECOMP_WORKFLOW
  expects.

## Open questions (verify first)

1. `x8f4_24` mask is `flags & 3` (MP1 `& 1`): a 2-bit field or a different flag.
2. Identity of 0xa18 (pInfo+0x58) and 0xb30 (null pointer; ragdoll slot?).
3. The [?] rows of section 1 (`253C`, `28F0`, `3220`, `3D7C`, `CB30`, `EDEC`).
