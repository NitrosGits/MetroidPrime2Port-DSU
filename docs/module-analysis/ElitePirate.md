# ElitePirate REL (module 15): analysis for a host body

Analysis + host body, milestone 1 (2026-10-16). Sources: `config/G2ME01/rels/ElitePirate/symbols.txt`
(sizes), the archive build's disassembly `../MetroidPrime2Port/build/G2ME01/ElitePirate/asm/`
(`auto_*_text.s`, `auto_03_*_rodata.s`, `auto_04_*_data.s`, `auto_05_*_bss.s`, and the DOL's own
`../MetroidPrime2Port/build/G2ME01/asm/auto_11_*_sdata2.s` for the constants the REL loads), the
generated `include/MetroidPrime/ScriptLoader/SLdrElitePirate.{hpp,inc}`, and MP1's
`../MetroidPrimePort/src/MetroidPrime/Enemies/CElitePirate.cpp` for what the class *does*.
Confidence tags as in `SpacePirate.md`: **[proven]** = read from an instruction/table entry;
**[probable]** = strong evidence but not proven; **[?]** = guess. All `fn_15_X` are REL `.text`
offsets. The implementation is `src/MetroidPrime/Enemies/PortCElitePirate.cpp`,
`include/MetroidPrime/Enemies/CElitePirate.hpp`.

## 0. Which enemy this is: the Ingsmasher (2026-10-19)

`CElitePirate` is Retro's internal class name; Echoes has no "Elite Pirate". The class is the
**Ingsmasher**, the Ing-possessed Luminoth mechanoid of Sanctuary Fortress [proven, read at runtime
by a temporary loader log in world `1baa96c2`, area 4 `b666b655`, layer 3]:

- editor names `Ingsmasher Twisp` and `Insgsmasher Catsby` (sic), ANCS `6b45f06d` character 0;
- scan `72dce8a4` -> STRG `b3845097`: "Mechanism: Ingsmasher" / "Mechanoid capable of generating
  earth-shattering shock waves. Energy shield can be breached with opposite-polarity beam systems." /
  "Designed to fight alongside the Luminoth, the Ingsmasher now seeks to destroy them. In close
  combat, it smashes its energized fists into the earth to send out powerful shock waves, then
  pulverizes any immobile prey. It will fire salvos of missiles at long-distance quarry. It uses an
  energy shield in battle that can be adjusted to protect against a variety of beam weapons. The
  shield can be overloaded by opposite-polarity beams, however, damaging the Ingsmasher in the
  process."

That is the yardstick for runtime checks: ground-smash shock waves up close, missile salvos at
range, a dark/light polarity shield that opposite-polarity beams overload. The disc also places
the **Dark Ingsmasher** (same class, dark-world areas). Spawns (`docs/module_spawns.tsv`, all
Sanctuary Fortress, area/layer): `02_cliff_reactorcore_dark` L5, `03_cliff_chicane` L4,
`06_cliff_spiderballguardian` L7, `0c_cliff_hall` L3, `0m_cliff_hall_dark` L4 and L5.

### Pseudocode

One `.c` per function (all 236) in `/tmp/opencode/pseudo/ElitePirate/fn_15_<off>.c`, named by REL
offset. Regenerate with the shared `tools/rel_pseudo/` (see its `link_rel.sh` header).

### FSM tables [proven]

`fn_15_C0D0` is the module's static initialiser: it copies 12-byte PTMFs `{0, vtbl_off, 0}` into
the `{name, PTMF}` tables. Every entry is **virtual**: the function is the vtable word at
`lbl_15_data_634 + vtbl_off`. `fn_15_9840` (class virtual slot 82, called by `Think` while the state
machine has no state) is `InitializeStateMachine` + `SetTriggerFunctions(data_178, 31)` +
`SetStateFunctions(data_464, 21)` + `SetCodeFunctions(data_5CC, 2)`.

| trigger | fn | trigger | fn | state | fn |
|---|---|---|---|---|---|
| Alerted | 60C8 | ReadyToCharge | 60D4 | Alert | 5340 |
| AngryAttackOver | 18CC | ReturnedToPatrol | 60BC | AngryAttackBegin | 1900 |
| AttackPatternOver | 6F28 | ShieldKilled | 6D68 | AngryAttackEnd | 18D8 |
| BreakProjectileAttack | 50B4 | ShockwaveIsNext | 18AC | Dead | 7B00 |
| CanShockwave | 5084 | ShotAt | 63B0 | FollowAttackPattern | 6DDC |
| ClearLineOfSight | 4E90 | ShouldAlert | 684C | InvulnAlert | 52D0 |
| DonePursuing | 691C | ShouldFire | 6820 | MeleeAttack | 48BC |
| DoneTurning | 6C64 | ShouldMeleeAttack | 6500 | Patrol | 5708 |
| HasAttackPattern | 6F44 | ShouldShockwave | 63BC | PowerDown | 5FF0 |
| InDetectionRange | 6F9C | ShouldTurn | 6C84 | PowerUp | 5F08 |
| InPosition | 6368 | SpotPlayer | 6FD0 | ProjectileAttack | 50BC |
| NotReachedTarget | 6D94 | StillAngry | 18B8 | Pursue | 5BB4 |
| PickedSpreadShot | 4E7C | TargetNotOnMesh | 6240 | Shielding | 41A0 |
| PlayerInNoAttack | 15CC | TargetUnreachable | 61E4 | ShieldUp | 44A8 |
| PoweredDown | 61CC | TooClose | 62CC | Shockwave | 4AE0 |
| PoweredUp | 61C0 | | | SpreadShot | 4D18 |
| | | | | Stunned | 59BC |
| code: SelectTarget | 555C | code: PickAttackType | 54AC | TargetPatrol | 578C |
| | | | | Taunt | 5A60 |
| | | | | Turn | 5598 |
| | | | | Wait | 6064 |

MP1 similarity (`similarity_match.py`'s tokeniser, Dice over n-grams): only `fn_15_2F10` =
MP1 `IsArmClawCollider` (1.00) and `fn_15_6820`/`fn_15_24F0` ~ `ActivateGrenadeLauncher` (0.80)
carry over; everything else is decompiled from retail.

## 1. Function map

| fn | size | role | evidence |
|---|---|---|---|
| `RELMain` | 0x20 | calls `fn_15_148` | [proven] |
| `fn_15_148` | 0x30 | stores loader `fn_15_178` into `lbl_15_bss_0` (= the module's `SElitePirate_FuncPtrs`, one loader slot), then calls the DOL's `fn_80218AD4` = `SetSElitePirate_FuncPtrs` | [proven] |
| `RELExit` | 0x24 | `fn_80218AD4(nullptr)` | [proven] |
| `fn_15_178` | 0xACC | **loader** `LoadElitePirate` (section 3) | [proven] |
| `fn_15_C44` | 0x64 | destructor helper for a stack object holding two `SLdrShockWaveInfo` (at +0xe4/+0x114); `fn_15_B57C` (0x64) is its sibling and `fn_15_B82C` calls that one twice | [proven] body shape; caller [?] |
| `fn_15_10C0` | 0x268 | `SLdrElitePirate` ctor (the loader's stack object at r1+0x694) | [proven]: first call of the loader |
| `fn_15_B82C` | 0x2CC | **`CElitePirateData` builder** (the loader's stack object at r1+0x250) | [proven]: loader argument list |
| `fn_15_B360` | 0x21C | `CElitePirateData` copy into the object (`fn_15_A190` @0x0000A290) | [proven] |
| `fn_15_A190` | 0x848 | **CElitePirate ctor** (section 2) | [proven]: called after `rs_new 0xd30` |
| `fn_15_9C70` | 0x1A4 | **destructor** (vtable slot 2; tokens, `fn_15_9E...` members, base) | [proven] vtable |
| `fn_15_A9D8` | 0xC8 | copy constructor (compares this != src, copies the flag byte) | [proven] body shape |
| `fn_15_9598` | 0x2A8 | **`Think`** (slot 5; returns early unless active - a bit in this+0x20 - then runs the state machine) | [proven] vtable + body |
| `fn_15_8580` | 0x804 | **`AcceptScriptMsg`** (slot 6; switches on the message's 4CC: `XDEL`, `DCTV`, ...) | [proven] vtable + body |
| `fn_15_83C0` | 0x12C | **`PreRender`** (slot 9): `CPatterned::PreRender`, then an LCTR transform lookup by name for the shield / visor-electric effect | [proven] |
| `fn_15_84EC` | 0x20 | **`AddToRenderer`** (slot 10): only `CPatterned::AddToRenderer` | [proven] |
| `fn_15_8294` | 0x12C | **`Render`** (slot 11): the shield/electric flavour (reads the timers at 0xc64 / 0xa3c, uses `CColor::Black`) | [proven] |
| `fn_15_7BFC` | 0x2F0 | **`DoUserAnimEvent`** (slot 28): dispatches through `jumptable_15_data_864` (26 entries), falls back to the base | [proven] |
| `fn_15_C0D0` | 0x808 | static initialiser of the FSM name/PTMF tables (section 0) | [proven] |
| `fn_15_9840` | 0x8C | class virtual slot 82: installs the FSM (section 0); `Think` calls it while the machine has no state | [proven] |
| `fn_15_800C` | 0x74 | slot 81 = `RenderIngSnatchingTransition` override | [proven] vtable slot |

### Vtable `lbl_15_data_634` (140 words, 2 leading zeros)

The first **82** slots are the `CEntity`..`CPatterned` table, in the same order as
`lbl_72_data_8D8` in the SpacePirate REL (82 words, ending at
`RenderIngSnatchingTransition`) [proven by entry-by-entry comparison]. Overridden slots (all
[proven] by slot index; everything else keeps the base-class symbol, which is how the table says
what is *not* overridden):

- CEntity: dtor `9C70` (slot 2), `TypesMatch` (slot 3 - the **DOL** owns this one:
  `TypesMatch__12CElitePirateCFi`), **Think `9598`** (5), **AcceptScriptMsg `8580`** (6).
- CActor: **PreRender `83C0`** (9), **AddToRenderer `84EC`** (10), **Render `8294`** (11),
  GetHealthInfo `D8` (15, calls the base's `HealthInfo` through slot 0x38), GetTouchBounds `10`
  (18, `GetBoundingBox` then `C094`), GetAimPosition `7EEC` (21), **DoUserAnimEvent `7BFC`** (28).
- CAi: **TakeDamage `4C`** (41, a 0x10-byte body: stores `lbl_8041AAB8` into 0x448 = the damage
  cooldown timer), CanBeShot `A4` (42, true), IsListening `AC` (43, **false**), Listen `B4` (44,
  false), `BC` (45, copies 3 floats out of r4+0x54).
- CPatterned: `5C` (50, returns the byte at 0x44f), **ProjectileInfo `52C8`** (55, returns
  `this+0xcd0`), **GetSearchPath `0`** (56, returns `this+0xa50`), **GetPointSearchPath `64`**
  (57, null), **GetRagDoll `6C`** (61, null), **GetAttackTarget `74`** (67, writes
  `kInvalidUniqueId` through the return-value pointer), IsOnGround `84` (68, the base's own bit at
  0x34c), GetGravityConstant `90` (69, `lbl_8041B758` = **24.525f**), GetDeathExplosionParticle
  `9C` (73, the base member at 0x754), `800C` (81).

Slots **82..139** are not vtable entries: 58 further function pointers (`fn_15_9840` 0x8C,
`fn_15_60C8` 0xC, `fn_15_18CC` 0xC, `fn_15_6F28` 0x1C, ... `fn_15_59BC`, `fn_15_578C` 0x230,
`fn_15_27FC` 0x30C, `fn_15_24F0`, `fn_15_1A48`) sit inside the same label, and the module's own
counts (`lbl_15_data_0` and `5EC`/`604`/`60C`/`614` are all 0x3B = 59) suggest the FSM's 59 code
functions were merged into the vtable's label [probable: the 82-entry prefix matches SpacePirate;
the count-to-table correspondence is [?]].

### FSM tables (not needed by this milestone)

`lbl_15_data_4`..`lbl_15_data_16C`: 31 × 12-byte `{0, fn_15_x, 0}` triples (triggers and
states), `lbl_15_data_178`: 31 × 16-byte `{name, 0, 0, 0}` entries whose strings are
`lbl_15_rodata_198`..`344` (`"Alerted"` first), `lbl_15_data_368`..`lbl_15_data_458`: 20 more
triples, `lbl_15_data_5CC`: 2 triples naming the strings `"SelectTarget"` (`rodata_422`) and
`"PickAttackType"` (`rodata_42F`), `lbl_15_data_5EC`..`lbl_15_data_630`: 18 ints (state ids: 0x3B,
0x30, 0x1F, 0x20, 0x1C, 0x2F, 0x21, 0x14, 0x31, 0x35, 0x15), `lbl_15_data_8D0`/`8F0`/`8FC`/`908`/
`914`/`924`..`954`: 12-byte `{0, 0, fn}` triples naming `fn_15_B318`, `fn_15_AC74`, `fn_15_5450`,
`fn_15_526C`, `fn_15_4CBC`, `fn_15_4A3C`, `fn_15_4A98`. Which function is which trigger/state is
not decoded: that is milestone 2+.

## 2. CElitePirate members (`CHECK_SIZEOF 0xd30`)

Size [proven]: the loader's `rs_new` is `__nw__FUlPCcPCc(0xd30, ...)`, and the ctor's highest
member is at 0xd2c. Base: `CPatterned` (0x7c0) built with `EPatternedAI` **0xc**, flavor 0,
movement 0 (ground), collider 1, body 1 [proven: `fn_15_A190`'s register/stack arguments to
`__ct__10CPatternedF12EPatternedAI...`]. All offsets below are [proven] by a store in `fn_15_A190`
unless tagged.

| off | member (host name) | proof / note |
|---|---|---|
| 0x7c0 | int = −1 (`x7c0_`) | `stw -1, 0x7c0`; use [?] |
| 0x7c4 | `CDamageVulnerability` copy of the patterned info's (`mBodyVulnerability`) | `fn_15_2B08(this+0x7c4, pInfo+0x74)`; read by `fn_15_27FC` [proven] |
| 0x7f4 | **shield `CCollisionActorManager`** (one "Shield" OBB on L_Palm_LCTR) | built by `fn_15_36CC` [proven] |
| 0x7f8 / 0x804 | L / R palm positions (`PreRender`, `fn_15_83C0`) | [proven] |
| 0x810 | last path goal (`5CBC`/`PlanPath`); Alert (`5340`) stores its own position, `41A0` uses it as the target when the shield's target actor is gone | [proven] |
| 0x81c | the position it powered up in (`5F08`); `ShouldAlert` (`684C`) is false within 5 m of it | [proven] |
| 0x828 | **`CElitePirateData`, 0x190** | ends at 0x9b8, the next member [proven] |
| 0x9b8 | **body `CCollisionActorManager`** (6 subdivided + 8 sphere joints) | `fn_15_36CC` [proven] |
| 0x9c0 | `CCollidableAABox` (`GetCollisionPrimitive`, `fn_15_8`) | rebuilt by the bbox setup `fn_15_2CFC` |
| 0x9e8 | optional `'PART'` token for data+0x34 | ctor |
| 0x9f8 / 0x9fa | head collider uid / **grenade launcher uid** | `fn_15_3574`, `fn_15_2564` |
| 0x9fc / 0xa10 | **R** / **L** arm collider lists (`reserved_vector<TUniqueId,8>`) | `fn_15_3574` via `fn_15_2F10` [proven] |
| 0xa24 / 0xa26 | shield collider uid / uid [?] | `fn_15_36CC` |
| 0xa28 | `mSpeed` at construction (`mBaseSpeed`) | Think doubles it with the shield up [proven] |
| 0xa2c | steering speed = walk speed × 0.99 / max speed (XCRT) | [proven] |
| 0xa30 / 0xa34 / 0xa3c | HP at the last vulnerability set / count-down timer / accumulated time | `fn_15_27FC`, `fn_15_27DC`, Think |
| 0xa38 | countdown that clears the XDamaged/ShotAt bit during Shielding; only the ctor writes it (0) | `41A0`, `A190` [proven] |
| 0xa40 | seconds at the end of the path with the player off it, reset by every re-plan | `5CBC` writes, `61E4`/`691C` read [proven] |
| 0xa44 | time Pursue began; the ctor leaves it unset | `5BB4` writes, `691C` reads [proven] |
| 0xa48 / 0xa4c | time of the last push-back / claimed path region (ctor stores **0**, `fn_15_1328` tests −1) [?] | |
| 0xa50 | `CPathFindSearch`; flags `IsAnEncounter() ? 0x201 : 1` | [proven] |
| 0xb3c | TargetPatrol's goal: a copy of the base's `mDestPos` (0x334) taken at Activate | `578C` writes, `6368` reads [proven] |
| 0xb48 | `SPositionHistory(5.f)` (DOL `fn_801931E0`) | `fn_15_2204` [proven] |
| 0xc10 | flags: event 8, XDamaged, alerted ×2, launcher-Ing-possessed, 3 [?] | |
| 0xc2c | **attack kind**: 2/8 rocket, 4 melee, 7 shock wave; 3/5/6 skip push-back | DoUserAnimEvent, `fn_15_8FB8` |
| 0xc30 | `'FSM2'` token (`data+0x00`) | [proven] |
| 0xc40 | initial idle: **1**, then 10/11/12 if `FindBestAnimation(state 5, 0, n)` = the editor's initial anim | [proven] |
| 0xc44 / 0xc50 / 0xc54 | event-7 bit / last melee time (−1000) / melee + knock pending bits (event 17) | |
| 0xc58..0xc6c | `fn_15_B080` block: 0, **0xc5c = −1000** (the shield-raise time `44A8` then stores), shield damage, shield hit time, shield alpha, **polarity** (0 dark, 1 light, 2 none) | XDMG, Render [proven] |
| 0xc70 / 0xc80 / 0xc94 / 0xc9c | light / dark shield-pop `'PART'`, hand `CElementGen`, shield `CSkinnedModel` | `fn_15_3C5C`, `fn_15_8E34`, `fn_15_8080` |
| 0xcac | 0x40 shield-pop pending, 0x80 double speed | Think |
| 0xcb0 | 1; **5** selects the double shock wave (`fn_15_1A48`) | |
| 0xcd0 | `CProjectileInfo(data.rocket, data.rocketDamage)` | `fn_15_728C` |
| 0xcf8 / 0xd00 / 0xd08 | rockets fired / rocket pattern (0 homing, 1 spread) / event 18 seen | `fn_15_728C` |
| 0xcfc | rockets this salvo: 4 (`4D18`), a random data range (`50BC`, `0x8dc..0x8e0`) | `4D18`/`50BC` write, `50BC` gates on it |
| 0xd04 | the pattern `PickAttackType` copies 20% of the time; the ctor's 1 is its only write | `A190`, `54AC` |
| 0xd0c / 0xd10 | aim blend towards the shield / shield aim position | `fn_15_8D84`, `fn_15_7EEC` |
| 0xd28 / 0xd2c | energy glow / absorbing | `fn_15_8EA0` |
| 0x620 | knock-back: all anim reactions and burn/explode-death/burn/slow/freeze off, then `kAR_Flinch` on | [proven] |
| 0x681 | the knock-back mgr's flag byte (+0x61): the five Enable*(false) above | [proven] |

### CElitePirateData (0x190 at 0x828)

Built on the loader's stack by `fn_15_B82C` and copied in by `fn_15_B360`. Its parameter list is
long and mixed (8 float registers, then 7 integer/pointer registers, then 24 stack slots), so only
the fields its *reader* (`fn_15_A190`) touches are proven:

- **+0x00** = the builder's first integer argument, which the loader computes as
  `patterned+0x1e4` = `SLdrPatternedAITypedef.stateMachine2`; the ctor loads it as the **'FSM2'**
  token [proven].
- **+0x08** = a `CDamageInfo` copied field-by-field out of the builder's damage argument
  (`mWeaponMode`, four floats, two u16 sfx ids) = `SLdrElitePirate.meleeDamage` [proven].
- **+0x34** = an integer the ctor tests against −1 and, when set, loads as a **'PART'** asset
  [proven offset and use]; the loader now feeds it `alwaysFF` (0x06cf4324) [probable].
- **+0x04** = the editor's initial animation, which the ctor matches against idle 10/11/12 (0xc40).
- **+0x168 / +0x16c** = the **rocket** projectile asset and its `CDamageInfo` (the
  `CProjectileInfo` at 0xcd0) [proven].
- 0x144..0x164 = nine floats (shock-wave/taunt range block), 0x188/0x18c = two ints
  (`unknown_0x624222f8` / `unknown_0x31e43a1c`-ish) [proven offsets, [?] identities].

The host `CElitePirateData` in `CElitePirate.hpp` therefore names its fields after the SLdr
properties (assigned by the loader, host layout) rather than claiming retail offsets.

## 3. Loader `fn_15_178`

Format [proven]: u16 property count, then per property u32 id + u16 size; unknown ids are skipped
with `ReadBytes(nullptr, size)`. **43 top-level properties**, the same id set as
`include/MetroidPrime/ScriptLoader/SLdrElitePirate.inc` (checked id by id against the loader's
dispatch tree; there are no sub-property readers - `SLdrShockWaveInfo` is read by its own
`LoadTypedefShockWaveInfo`). The `SLdrElitePirate` members sit at r1+0x694 (editorProperties), 0x6d0
(patterned), 0x914 (actorInformation), 0x98c (meleeDamage), 0x99c..0x9dc (the nine range/taunt
floats), 0x9e0/0xa10 (the two shock waves), 0xa40..0xa4c, 0xa50 (rocket), 0xa54 (rocketDamage),
0xa64/0xa68, 0xa6c..0xa78, 0xa7c/0xa80, 0xa84 (rocketLauncherActorInfo), 0xafc/0xb08 (the two
animation sets), 0xb14/0xb18, 0xb1c (ingPossessionData) - which confirms the `.inc`'s field order
and is also how `SLdrElitePirate.hpp`'s own member order was verified.

Construction order [proven]: `LdrToModelData(editorProperties.transform.scale, kInvalidAssetId,
patterned.animationInformation, true)` (`fn_8023A0B8`; the optional's valid byte is r1+0x1fc, and
the loader returns null when it is clear) · `LdrToDamageInfo` ×2 · `LdrToAnimationParameters` ×2 ·
`LdrToActorParameters` · the data builder `fn_15_B82C` · `rs_new(0xd30)` ·
`LdrToPatternedInfo(patterned, &ingPossessionData)` · `AllocateUniqueId` ·
`LdrToActorParameters` · `LdrToTransform4f` · `LdrToEntityInfo` ·
`fn_15_A190(uid, name, entityInfo, transform, modelData, patternedInfo, actorParameters, data)`
**in that argument order** - note `patternedInfo` before `actorParameters`, which is the reverse of
`CSpacePirate`'s ctor.

## 4. What the module registration needed (milestone 1)

- `include/MetroidPrime/CEntityInfo.hpp`: `kET_ElitePirate = 114` (the type index
  `TypesMatch__12CElitePirateCFi` answers to; the placeholder comment in `TypesMatch.cpp` names it).
- `src/MetroidPrime/TypesMatch.cpp`: `TYPES_MATCH_IMPL(CElitePirate, CPatterned, kET_ElitePirate)`
  and the two `CAST_TO_*` entries under `#ifdef TARGET_PC`.
- `src/MetroidPrime/Enemies/SElitePirate_FuncPtrs.cpp`: the port's null guard on
  `RelProxy_LoadElitePirate` was dropped, as for Ing / MinorIng / SpacePirate.
- `tools/modules_port.txt`: `ElitePirate` now points at
  `src/MetroidPrime/Enemies/PortCElitePirate.cpp` (was `PortModuleStubs.cpp`);
  `tools/gen_files_cmake.py --check` passes (883 sources).

## 5. Status and open questions

**Slice 2 (2026-10-19) is implemented** in `PortCElitePirate.cpp`, each function behind a
`// @ fn_15_XXXX` marker checked against the pseudocode: the ctor (`A190`, bbox `2CFC`), Think
(`9598`: install the FSM via `9840` when it has no state, then launcher sync, both collision
managers, speed, timers, position history, launcher transform, aim blend, shield/glow alpha,
push-back `8FB8`), AcceptScriptMsg (`8580`), PreRender/Render/shield render/portal plane
(`83C0`/`8294`/`8080`/`800C`), GetAimPosition (`7EEC`), DoUserAnimEvent (`7BFC`), the collision
setup (`36CC`/`3574`/`2F10`), the shield vulnerability (`27FC`), the launcher (`2564`/`251C`/
`2394`/`2448`), shock wave (`70D4`), rockets (`728C`), melee (`7004`), player pushes and rumble
(`1B60`/`1C7C`), the shield pop (`3C5C`) and the AI-hint queries (`1788`/`15F8`). The body
controller is activated on XCRT (retail). The 31 triggers / 21 states / 2 code functions are
installed in `fn_15_9840`'s order; their bodies are `StandIn` shells (triggers return false).

**Triggers + code functions (slice 3, 2026-10-19)**: all 31 triggers and `SelectTarget`/`PickAttackType`
are ported (`// @ fn_15_XXXX`). Retail's virtual self-calls map through the vtable label
(`lbl_15_data_634 + off`, slot = off/4): 0x160 = `ClearLineOfSight`, 0x194 = `ShieldKilled`, 0x1a4 =
`ShouldTurn`. Retail CPatterned offsets: 0x6c0 = the waypoint navigation's destination uid (the
navigation sits at 0x6b8), 0x728 = `mPredictedLeashTime`. `0xd20` is an int (`mAngryCount`, was a
float in the header). `fn_15_6618` is `ShouldFire`'s only caller and is inlined, as is `fn_15_2284`
(launcher "lockon_target_LCTR" world position). Filters: ClearLineOfSight include Solid / exclude
Floor; ShouldFire include Solid|Character (bits 0x3b/0x1f/0x21). Resolved 2026-10-19 (section 5
"Resolved `[?]` members"): 0x194 *is* `ShieldKilled` (word 101 of the table), `x81c_` is where the
Ingsmasher powered up, `xd04_` is a ctor constant (a fixed 20% spread), and 0xa40/0xa44/0xc5c/0xb3c
are the stuck/pursue/shield-raise times and TargetPatrol's goal.

**Combat states (2026-10-19)** in `PortCElitePirateCombat.cpp`: Shielding `41A0`, ShieldUp `44A8`,
MeleeAttack `48BC`, Shockwave `4AE0`, SpreadShot `4D18`, ProjectileAttack `50BC`, AngryAttackBegin
`1900`, AngryAttackEnd `18D8`, Taunt `5A60`, PowerUp `5F08`, PowerDown `5FF0`, Dead `7B00`, plus
`ResetAttackTimer` (`1F2C`) and `TryCommand` (`BAF8`/`BC04`/`BD10`, retry a body command until the
body state is entered). They reuse the movement file's `SetAttackKind`/`UpdateAbsorb`/`TauntCmd`/
`TurnCmd`/`PathToward`. The shield's looping sound is a host member (`mShieldSfx`, after 0xc98).
Writers: `mShieldUpTime` (0xc5c) = ShieldUp, `mStuckTime` (0xa40) = PathToward, `mPursueStartTime`
(0xa44) = Pursue (`5BB4`, from 0xa3c), `mPatrolGoal` (0xb3c) = TargetPatrol (`578C`, from 0x334);
`xd1c_` is the taunt type (an int,
Taunt cycles 0 -> 6 -> 1 -> 0). Shielding's `xa38_` countdown is dead in retail: the ctor writes it
0 and `41A0` only decrements it, so it clears the XDamaged/ShotAt bit on the first update.
In game (Sanctuary area 4, layer 3), Shockwave, ProjectileAttack, SpreadShot, AngryAttackBegin/End
and PowerUp/PowerDown are entered with no fault; later runs reached the rest (item 1 below).

### Resolved `[?]` members (2026-10-19)

Writer/reader census over all 236 pseudo files, index and byte-offset forms both. Nothing else in
the REL touches these offsets.

| off | host name | type | writers | readers | meaning |
|---|---|---|---|---|---|
| 0xa38 | `mXDamagedTimer` | float | `A190` ctor `= 0`; `41A0` `-= dt` | `41A0` | countdown that clears the XDamaged/ShotAt bit (`1B50`); never armed, so the bit clears on the first Shielding update |
| 0xa40 | `mStuckTime` | float | `5CBC` `+= dt` and `= 0` (twice) | `61E4` (`1.0 <`), `691C` (`<= 0.5` gates its early-out) | seconds parked at the path's last waypoint with the player off the path |
| 0xa44 | `mPursueStartTime` | float | `5BB4` Activate `= 0xa3c` (the ctor leaves it) | `691C` | mTime when Pursue began; DonePursuing compares the last push (0xa48) and +2.5 s against it |
| 0xb3c | `mPatrolGoal` | `CVector3f` | `A190` zero; `578C` Activate copies 0x334 | `6368` (`< 5 m`), `578C` (arrival steering) | TargetPatrol's goal, i.e. the base's `mDestPos` at Activate |
| 0xc5c | `mShieldUpTime` | float | `B080` `= -1000`; `44A8` Activate `= 0xa3c` | `60D4`, `6D94` | mTime the shield went up; −1000 = never |
| 0xcfc | `mRocketsToFire` | int | `4D18` `= 4`; `50BC` `= Range(data 0x8dc, data 0x8e0)` | `50BC` | salvo size |
| 0xd04 | `mDefaultRocketPattern` | int | `A190` `= 1`, nothing else | `54AC` | with no later writer the 20% branch is a fixed spread pick |
| 0x81c | `x81c_` | `CVector3f` | `A190` zero; `5F08` Activate `= own translation` | `684C` | where it powered up; no alert within 5 m of it |

- `ReadyToCharge` (`60D4`) is `0xc5c + (HealthInfo[1] / HealthInfo[0]) * 1.7 + 0.3 < 0xa3c`: the
  `GetHealthInfo()` floats are `CHealthInfo`'s initial HP (+0) and current HP (+4) - the same ratio
  `8580` scales by 100 for its 25% test - not HP/knock-back (knock-back is +8). The port now uses
  `GetHP() / GetInitialHP()`; it had knock-back over HP.
- vt+0x194 is word 101 of `lbl_15_data_634` = `fn_15_6D68` (ShieldKilled), between `fn_15_60D4`
  (word 100) and `fn_15_18AC` (102, ShockwaveIsNext), read from
  `build/G2ME01/ElitePirate/asm/auto_04_00000000_data.s`.
- `ShouldAlert` (`684C`) tests `x81c_` against the actor's **own** translation (0x54/0x58/0x5c), so
  the old "alert here position" reading was wrong: it is "do not alert at the spot where I powered
  up", and the only writer after the ctor's zero is `5F08`.
- `PickAttackType` (`54AC`): jump-down hint (0x17) forces the spread; otherwise `Random(0,1) >= 0.8`
  copies 0xd04 and the other 80% takes its opposite. 0xd04 is a ctor constant (1), so retail is a
  fixed 20% spread / 80% homing, not a repeat of the previous pick.

Open:

1. Entered at runtime (area 4 L3, console `send u<id> Activate`, `tp`, `hurt`, polled with `obj`):
   PowerUp, PowerDown, FollowAttackPattern, Turn, Alert, ShieldUp, Shielding, MeleeAttack,
   Shockwave, SpreadShot, ProjectileAttack, Dead (600 hp, power-beam `hurt` to <=0), and
   AngryAttackBegin/End, Pursue; Taunt and Wait in the dark run (below). Not entered (no `// @`
   marker), 2026-10-19:
   - TargetPatrol and InvulnAlert are not states of the FSM2 asset graph (`fsm.txt`), so no
     retail path reaches them.
   - Patrol: neither Ingsmasher has a patrol waypoint chain, so FSM2 never goes there.
   - Stunned is reached only from Shielding on ShieldKilled (`mShieldDamage` over
     `mUnknown5236C2B6` at polarity 1, else `mUnknown01EAAB17`). Shield damage accrues only in
     kSM_Damage from a real projectile hitting the shield actor (polarity 1: weapon type 1/3,
     polarity 0: 2/3); console `hurt` bypasses it. **Entered 2026-10-20** (see "Shield gameplay
     end to end" below).
   Console notes: `hurt` does not set ShotAt (`mXDamaged`); `send u<id> Alert` powers a
   PowerDown Ingsmasher up; `obj` takes uids only and prints connections as editor ids.
   u051b's pod, entered the area's own way (2026-10-20, no console `Activate`/`Alert`; the player
   is teleported into the intro trigger u04fc's volume) and the pod is two-stage: u04fc's `Entered`
   starts sequence timer 000400af (u0483), which deactivates pod 1's lid, starts platform u0485 and
   then activates + alerts **u04ff** only. u051b is released by **u04ff's death**: u04ff's
   `DeathRattle` starts timer 000400b3 (u0454), which deactivates u051b's lid (u045b), starts
   platform u045d and activates + alerts u051b. Run `intro3`: u04fc went inactive and u04ff went
   `PowerUp` -> `FollowAttackPattern` with u051b still `(none)`/inactive; `damage u04ff 700` (hp
   600 -> -100) then 0.6 s later u051b entered `PowerUp`, its lid went inactive, platform u045d
   lowered (-152.1 -> -152.8 -> -153.5), and it walked out of the pod (`FollowAttackPattern` at
   (80.8, 48.3), down to y 25.6) through `Turn`, `Alert`, `ShieldUp`, `Shielding`, `MeleeAttack`,
   `Shockwave`, `ProjectileAttack`, `SpreadShot` and `AngryAttackBegin` for the remaining 3 min,
   with no fault. So the earlier "u051b stays at (81.1, 48.4, -152.1)" reading was the pod still
   shut: the console `Activate` never opens it, and no port bug is involved. Neither `Patrol` nor
   `TargetPatrol` was entered, so no marker was added.
   Dark Ingsmasher (`0m_cliff_hall_dark`, layers 4/5): 0 caught; u04fa stands at z -114.7 and
   enters PowerUp, Wait, Turn, Alert, Taunt, ShieldUp, Shielding, MeleeAttack and Shockwave. Fixed 2026-10-19: active Ingsmashers fell through the floor
   (free fall at the 24.525 gravity) because the ctor built `mCollidableBox` from
   `GetBoundingBox()`, the world-space box; fn_15_2CFC stores the local box. With the fix both
   stand at z -152 for 60 s and u04ff walks, pursues and attacks.
2. `[?]` members still open: 0x7c0, 0xa26, 0xc14/0xc28/0xc3c/0xc48/0xc4c, 0xc58, 0xcb4,
   0xcb8/0xcc4, 0xd1c/0xd20/0xd24, and 0xa4c's ctor value 0. 0x810/0x81c, 0xa38/0xa40/0xa44,
   0xb3c, 0xc5c, 0xcfc and 0xd04 are resolved in the table above.
3. `fn_15_728C` indexes its four spread offsets by the rocket count with no bound in the function
   itself. The volley is armed at four (`4D18` stores 0xcfc = 4; `50BC` fires only while
   0xcf8 < 0xcfc) and the table is four triplets, so the port clamps the index to 0..3, which is
   unchanged behaviour for any index retail reaches inside a volley.

### Shield gameplay end to end (2026-10-20): Stunned `59BC` reached

Area 4 layer 3, u04ff, no instrumentation in the committed tree. Console (`MP2_CONSOLE=4791`):
`give annihilatorBeam`, `give darkBeam`, `give lightBeam`, `give darkAmmo 250`,
`give lightAmmo 250`, `beam annihilator`, `send u04ff Activate`, `send u04ff Alert`, then the
driver (`/tmp/opencode/ep/stun/stun9.py`, run by `/tmp/opencode/ep/stun/stun_run9.sh`, log
`/tmp/opencode/ep/stun/stun9.log`) pins the player 6.5 m from the Ingsmasher, faces it, holds A
(charge) and releases it the frame `obj u04ff` reports `ShieldUp`, then teleports 12 m out while
the shot is in flight. t = 16.7 s after the driver start (about 35 s after boot), hp 588.

What the run proved, with the logged values:

- shield hits: polarity 1 (light shield), limit `mUnknown5236C2B6` = **55** (`mUnknown01EAAB17` =
  **110** at polarity 0). A charged annihilator hit is **80**, an uncharged one **12**; the
  `ShieldUp` window accumulated 80 + 5 x 12 = **140**, `ShieldKilled` went true, the shield burst
  (Shielding's Deactivate sets `mShieldPopPending`) and the FSM entered `Stunned` (`@ fn_15_59BC`
  in `PortCElitePirateMove.cpp`).
- Shielding's transitions are tried in the FSM2 asset's order, **`ShouldMeleeAttack` before
  `ShieldKilled`**, and the melee range is `mData.mMaxMeleeRange` = **9.00** (retail 0x97c is
  `CElitePirateData` + 0x154, since `mData` sits at 0x828). At 6.5 m every earlier run left
  Shielding for `MeleeAttack` and the accumulated damage was lost at the next `ShieldUp` (which
  zeroes `mShieldDamage`); standing > 9 m at the `ShieldUp` -> `Shielding` handover is what makes
  ShieldKilled win. `ShouldMeleeAttack` also needs `GetPlayerHintType() == -1`.
- The shield window is 0.3 + 1.7 x (HP/initial HP) s of `ShieldUp` (2.0 s at full health), and
  `ReadyToCharge` is its only exit, so damage for one burst has to land inside that window.
- kSM_Damage arrives with sender = a body/shield `CCollisionActor` whose last-touched object is
  the player's `CGameProjectile`; `IsShieldUp()` (locomotion `kLT_Crouch`, value 0) is true from
  `ShieldUp`'s Activate onwards. No port bug was found on this path.

**Next (2026-10-20, `decomp/elitepirate` at `4969dd112`):** every state the FSM2 asset graph can
reach now carries a `// @` marker (100/241 functions, 30364/51836 bytes), and the `[?]` list above
is resolved. What is left unmarked, and what to do about it:

- Loader/struct machinery the port replaces wholesale, not decomp work: `178` (the loader itself,
  `PortLoadElitePirate` in `PortCElitePirate.cpp`), `10C0` (`SLdrElitePirate` ctor), `B82C`/`B360`
  (`CElitePirateData` builder/copy), `C0D0` (the FSM name/PTMF table static initialiser - the port
  installs the tables in `fn_15_9840`), and the `SLdr*`/`CKnockBackMgr`/`CBodyStateInfo` ctors and
  dtors `9AB0 9C34 9E14 9A74 9FF8 A08C C44 CA8 CE8 D08 D30 E6C ED8 F2C F90 FF0 B57C B5E0 B674
  B6F0 B748 B7CC`. Do not mark these; most are compiler-generated.
- Unreachable states: Patrol `5708`, TargetPatrol `578C`, InvulnAlert `52D0` (item 1 above).
- ~100 small retail helpers with no marker: `2BB8 2E04 2ECC 2FC8 2FF0 306C 3180 3288 32C0 32E0
  3308 33C8 3424 3AE0 3B40 3B78 3BD8 3FC0 4088 40F4 414C 4410 4488 47DC 4848 4A3C 4A98 4CBC 526C
  5450` (collision/joint/shield setup), `776C 77B4 7868 78A8 78C8 78F0 7970 79AC 79E8 7A44 7A64
  7AAC` (launcher/render), `98CC 9934 9988` (shield), `C8D8 C99C C9C0 C9E4 CA30` (tail), plus the
  tiny virtuals `0 8 10 4C 5C 64 6C 74 84 90 9C A4 AC B4 BC D8 104 128` and `A9D8`'s copy helpers
  `AAA0..B318`. Claim one only after a runtime gap shows the port is missing its effect; a lane
  brief must name the offset and the observed symptom.
- Dead while the shield is up (2026-10-20, area 4 L3, u04ff): ShieldUp -> Shielding
  (`ReadyToCharge`, 2.1 s) and then, after 10 x `damage u04ff 60` from 10.5 m (60-point chunks keep
  every per-frame delta under `mXDamageThreshold`, so `CPatterned::Death` takes the ordinary branch
  and sets the FSM's Dead), **Shielding -> Dead with nothing in between**. `Shielding`'s Deactivate
  tore the shield down: the collision-actor list went 47 total / 41 active -> 47 / 40 at the kill
  (the shield collider becomes inactive; the body colliders stay until `Delete`, 26 total / 20
  active in the area afterwards), and `mShieldPopPending` popped exactly one `CExplosion` (0 -> 1 at
  the kill, gone 1 s later). Dead then deleted the launcher u0515 and the actor (u04ff unresolvable
  at +10 s, `send` says "no object"), and it stayed Dead: no PowerUp/PowerDown after Activate/Alert.
  A kill 0.2 s into `ShieldUp` instead also goes straight to Dead, and that kill left the count at
  47 / 41: `ShieldUp` has no Deactivate branch (retail `fn_15_44A8` has Activate and Update only),
  so on that path the shield collider stays active until `Delete` - retail behaviour, not a port
  bug. Sfx and `mShieldAlpha` are not observable from the console. Log
  `/tmp/opencode/ep/gap/a2.out` (kill in ShieldUp: `a.out`), driver `a2.py`, run `a2_run.sh`.
- Dark Ingsmasher against the same shield burst (2026-10-20, `0m_cliff_hall_dark` L4/5, u04fa,
  hp 800): PowerUp -> Turn -> ShieldUp 3.1 s after the driver's Activate/Alert, and one charged
  annihilator released on `ShieldUp` (then the step to 12 m) gave **ShieldUp -> Stunned at 5.0 s**,
  the first window, hp unchanged. A charged hit is 80, over the polarity-1 limit 55 and under
  polarity 0's 110, so that shield was polarity 1 [inference: the console has no polarity readout].
  Log `/tmp/opencode/ep/gap/b.out`, driver `b.py`, run `b_run.sh`.
Harnesses: `/tmp/opencode/ep/trace/run3.sh` (area 4), `/tmp/opencode/ep/stun/stun_run9.sh`
(Stunned), `/tmp/opencode/ep/dark/stun_run.sh` (dark), `/tmp/opencode/ep/intro/run3.sh` (the area's
own intro path: trigger -> pod 1 -> u04ff's death -> u051b's pod).
Gates: `/tmp/opencode/ep/gate_build.sh` (0 FAILED), `python3 tools/gen_files_cmake.py --check`,
`/tmp/opencode/ep/gate_run2.sh <log>` (0 crash lines), matching build 94.14%/63.29% if an
upstream file changed. Also tell the port orchestrator that `186fea1e7` touched the shared file
`platform/port_console.cpp` (`hurt`, `obj` FSM state).

**Movement states (2026-10-19)**: Patrol `5708`, TargetPatrol `578C`, Pursue `5BB4`, Turn `5598`, Alert `5340`,
InvulnAlert `52D0`, Wait `6064`, Stunned `59BC`, FollowAttackPattern `6DDC` are in `PortCElitePirateMove.cpp`, with
their callees (attack-kind history `4790`, absorb `850C`, retrying body commands `BDFC`/`BEA0`, path helpers
`5CBC`/`1FC0`/`13B8`). The attack-kind history (0xc14..) is only pushed to; `xd1c_` is the taunt type
(0 -> 6 -> 1 -> 0). In game (area 4, player teleported next to the pirates) Alert, Turn and
FollowAttackPattern were entered with 0 crashes.

## 6. Verified in game, and how its actors are reached (2026-10-16, later session)

The module is now verified running, and the reason it was invisible before is established:

- **Its actors live in a non-zero world layer.** Area 4 of world `1baa96c2` (asset `b666b655`) has
  five layers, and the log's own names and states are `0='Default' active 1='!No Load'
  2='1st Pass' active 3='2nd Pass' 4='3rd Pass'` - i.e. layers 1, 3 and 4 start **inactive**, and
  layer 2 is active. Walking the area with only the layers the save has active produces no actor,
  which is what the earlier run saw.
- **Activating layer 3 (`2nd Pass`) loads the module and creates its actors.** With
  `MP2_INPUT`'s new `LAYER3` token (`platform/README.md`), `CGameArea::LoadLayerDynamic` ->
  `UpdateDynamicLayers` reaches `kLP_Ready`, `ActivateLayerDynamic` runs the layer's script
  objects, and the log shows `RELProd/ElitePirate.rel` (70856 bytes) being read immediately before
  `[layer] area 4 layer 3 activated (755 object ids free)`. `LAYER1` (`!No Load`), `LAYER2`
  (already active) and `LAYER4` (`3rd Pass`) also work, and `LAYER5` is correctly refused ("layer 5
  is not one of them", the area has 0-4).
- **In game the actors exist and run**: `[elitepirate]` lines for `AcceptScriptMsg` during the
  layer's load, then `Think`, `PreRender`, `Render` and `DoUserAnimEvent` while the player stood in
  the area (`/tmp/layer1.log`, a run booted straight into the area with
  `MP2_WORLD="1baa96c2,b666b655"`). Everything else in them is still the once-per-method stand-in
  of section 5, so their AI is absent: the same standard as the other families' milestone 1.
- Not yet exercised for this module: being shot, the shields, and the Ing form changes.

The layer lesson generalises: **a module's actors are not reachable by walking its areas alone**,
and every remaining family has to be checked the same way - warp into one of its areas, read the
layer list the token prints, and ask for the layer the module's REL is loaded by.

### Fight reach triage (2026-10-20): the fight reaches no stand-in

Smoke (Sanctuary area 4, layer 3; the driver of `/tmp/opencode/smoke/ep_con.py`, the run of
`/tmp/opencode/smoke/ep_run_ep.sh` from this worktree's `build-port-link`): `u04ff` runs
PowerUp -> FollowAttackPattern -> Turn -> Alert -> Shockwave -> Turn -> ShieldUp -> MeleeAttack
-> Shockwave -> Turn -> Pursue -> ProjectileAttack -> Turn (state counts: Shockwave 86, Turn 20,
Pursue 11, MeleeAttack 10, ShieldUp 8, ProjectileAttack 7, FollowAttackPattern 6, Alert 5, PowerUp
4); `u051b` (the dark Ingsmasher) runs PowerUp -> FollowAttackPattern. The log has **no
`[elitepirate] ... is a port stand-in` line** (its five stand-in lines are the Splitter module's),
**no `[reach-stub ...]`** and **no `[port] caught`**: the FSM is fully ported, so the fight reaches
no shell.

What the fight's path still leans on is the retail's *internal* helpers, each of which the port
covers with a host call (so none of them is a port stand-in and there is nothing for a
once-logging shell to say). The reachable ones, with their sizes from
`config/G2ME01/rels/ElitePirate/symbols.txt` and the ported function that covers them:

| fn | size | called from | the port's cover |
|---|---|---|---|
| `fn_15_2E04` | 0xC8 | `48BC`, `7BFC` | `SetArmTouchBounds` (now marked `@ fn_15_2E04`) |
| `fn_15_776C` | 0x48 | `728C` | the host's projectile launch |
| `fn_15_7AAC` | 0x54 | `728C` | ditto (the retail's free helper) |
| `fn_15_2FC8` | 0x28 | `2F10`, `3574`, `70D4` | the host's collision setup |
| `fn_15_2730` | 0x58 | `2564`, `3C5C`, `70D4` | the host's smart pointers |
| `fn_15_2B08` | 0x5C | `27FC`, `A190` | `SetShieldVulnerability` |
| `fn_15_3AE0` | 0x60 | `36CC` | the host's collision manager |
| `fn_15_3288` | 0x38 | `36CC` | ditto |
| `fn_15_3180` | 0x108 | `36CC` | ditto (over 0x80: a host cover, not a ported body) |
| `fn_15_3424` | 0x150 | `36CC` | ditto |
| `fn_15_BF6C` | 0xC0 | `36CC` | ditto |
| `fn_15_3308` | 0xC0 | `36CC` | ditto |
| `fn_15_3BD8` | 0x84 | `36CC` | ditto |
| `fn_15_3FC0` | 0xC8 | `3C5C` | the host's particle/actor |
| `fn_15_1A84` | 0xCC | `1A48` | the host's shock wave |
| `fn_15_ABB8` | 0x48 | `A190` | the port's ctor |
| `fn_15_B080` | 0x64 | `A190` | ditto |
| `fn_15_B748` | 0x84 | `A190` | ditto |
| `fn_15_A9D8` | 0xC8 | `A190` | ditto |
| `fn_15_9988` | 0xEC | `A190` | ditto |
| `fn_15_B360` | 0x21C | `A190` | the port's data copy |
| `fn_15_CA8` | 0x40 | `178` | the port's loader |
| `fn_15_10C0` | 0x268 | `178` | ditto |
| `fn_15_B82C` | 0x2CC | `178` | ditto (the data builder) |

`fn_15_2E04` was the one reachable helper whose port body is retail's (the three bounds floats at
the collision actor's 0x35c/0x360/0x364, 0x2E70/0x2E74/0x2E78), and it is now marked; the retail
calls it with this+0x9fc and this+0xa10, the port does both lists in one call.

## 7. Every unmarked function, classified (2026-10-21)

The module has 241 functions; 104 carry a `// @` marker, so 137 are unmarked. This section
classifies all 137, so the next lane does not have to re-derive it. Method (all reproducible from
the build tree):

- **Call graph**: the REL's own disassembly (`build/G2ME01/ElitePirate/asm/auto_00_*_text.s`,
  241 `.fn` blocks) gives every call site, so the "call sites" column is the number of `bl` sites
  in the REL and "callers" is the number of distinct caller functions.
- **DOL addresses**: the DOL keeps a **CElitePirate vtable at 0x803b2458** in `.data` (the REL's
  own vtable `lbl_15_data_634` is the same table with the REL's own functions in the overridden
  slots). 52 of the REL vtable's 53 base-class slots name a DOL symbol, and the DOL's word at
  that slot is that symbol's address, so **every base virtual the REL overrides has a known DOL
  address** (e.g. slot 15 = `GetHealthInfo__6CActorCFRC13CStateManager` @ 0x8000b900, slot 18 =
  `GetTouchBounds__10CPatternedCFv` @ 0x80073bf0). The REL's copies of those are the "host
  accessor" rows.
- **Body identity**: each REL body was compared (call targets masked) with all 24580 DOL
  functions; an identical body is cited with the DOL function's address, which is the strongest
  evidence that a REL function is a copy of host code and not new behaviour.

Classes: **host accessor** (the REL's copy of a base-class one-liner; the DOL address is in the
last column), **host copy** (a body identical to a DOL function, or code of a host class the REL
compiles itself: rstl, CToken, CJointCollisionDescription, CCollisionActorManager,
CGameProjectile), **compiler-generated** (the module's own struct ctors/dtors and the weakly
linked C++ helpers), **REL runtime** (entry/exit, prolog/epilog, the module ctor/dtor table),
**loader/ctor machinery** (the loader and the data/FSM-table builders the port replaces
wholesale) and **behaviour** (the module's own game logic - the only class a marker may go on).

| fn | REL off | size | call sites | class | DOL address / evidence |
|---|---|---|---|---|---|
| `fn_15_0` | 0x00000 | 0x0008 | 0 | host accessor | DOL `GetSearchPath__10CPatternedFv` @ 0x80073c74 (vtable slot 56) |
| `fn_15_8` | 0x00008 | 0x0008 | 0 | host accessor | DOL `GetCollisionPrimitive__13CPhysicsActorCFv` @ 0x800ea1b8 (vtable slot 31) |
| `fn_15_10` | 0x00010 | 0x003C | 0 | host accessor | DOL `GetTouchBounds__10CPatternedCFv` @ 0x80073bf0 (vtable slot 18) |
| `fn_15_4C` | 0x0004C | 0x0010 | 0 | host accessor | DOL `TakeDamage__10CPatternedFRC9CVector3ff` @ 0x80073c58 (vtable slot 41) |
| `fn_15_5C` | 0x0005C | 0x0008 | 0 | host accessor | DOL `GetModelAlphau8__10CPatternedCFRC13CStateManager` @ 0x80073c64 (vtable slot 50) |
| `fn_15_64` | 0x00064 | 0x0008 | 0 | host accessor | DOL `(CPatterned::GetPointSearchPath)` @ 0x80073c7c (vtable slot 57) |
| `fn_15_6C` | 0x0006C | 0x0008 | 0 | host accessor | DOL `GetRagDoll__10CPatternedCFv` @ 0x800358e0 (vtable slot 61) |
| `fn_15_74` | 0x00074 | 0x0010 | 0 | host accessor | DOL `GetAttackTarget__10CPatternedCFv` @ 0x80073c84 (vtable slot 67) |
| `fn_15_84` | 0x00084 | 0x000C | 0 | host accessor | DOL `IsOnGround__10CPatternedFv` @ 0x80073c90 (vtable slot 68) |
| `fn_15_90` | 0x00090 | 0x000C | 0 | host accessor | DOL `GetGravityConstant__10CPatternedCFv` @ 0x80073c9c (vtable slot 69) |
| `fn_15_9C` | 0x0009C | 0x0008 | 0 | host accessor | DOL `GetDeathExplosionParticle__10CPatternedCFv` @ 0x80073ca4 (vtable slot 73) |
| `fn_15_A4` | 0x000A4 | 0x0008 | 0 | host accessor | DOL `CanBeShot__3CAiFRC13CStateManageri` @ 0x800358d8 (vtable slot 42) |
| `fn_15_AC` | 0x000AC | 0x0008 | 0 | host accessor | DOL `IsListening__3CAiCFv` @ 0x80073cac (vtable slot 43) |
| `fn_15_B4` | 0x000B4 | 0x0008 | 0 | host accessor | DOL `Listen__3CAiFRC9CVector3f16EListenNoiseType` @ 0x8003c59c (vtable slot 44) |
| `fn_15_BC` | 0x000BC | 0x001C | 0 | host accessor | DOL `GetOrigin__3CAiCFRC13CStateManagerRC11CTeamAiRoleRC9CVector3f` @ 0x80073cb4 (vtable slot 45) |
| `fn_15_D8` | 0x000D8 | 0x002C | 0 | host accessor | DOL `GetHealthInfo__6CActorCFRC13CStateManager` @ 0x8000b900 (vtable slot 15) |
| `RELExit` | 0x00104 | 0x0024 | 0 | REL runtime | REL entry/exit, prolog/epilog, module ctor/dtor table |
| `RELMain` | 0x00128 | 0x0020 | 0 | REL runtime | REL entry/exit, prolog/epilog, module ctor/dtor table |
| `fn_15_148` | 0x00148 | 0x0030 | 1 | REL runtime | REL entry/exit, prolog/epilog, module ctor/dtor table |
| `fn_15_178` | 0x00178 | 0x0ACC | 0 | loader/ctor machinery | the port replaces it wholesale (PortLoadElitePirate / the FSM table install) |
| `fn_15_C44` | 0x00C44 | 0x0064 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_CA8` | 0x00CA8 | 0x0040 | 1 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_CE8` | 0x00CE8 | 0x0020 | 1 | compiler-generated | body identical to DOL `AddChildWidget__10CGuiWidgetFP10CGuiWidgetbb` @ 0x8027d930 |
| `fn_15_D08` | 0x00D08 | 0x0028 | 1 | compiler-generated | body identical to DOL `"construct_impl<Q218CLayoutDescription24CScaledLayoutDescription>__4rstlFPvRCQ218CLayoutDescription24CScaledLayoutDescription"` @ 0x802b1490 |
| `fn_15_D30` | 0x00D30 | 0x013C | 1 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_E6C` | 0x00E6C | 0x006C | 0 | compiler-generated | body identical to DOL `"__dt__Q24rstl48optional_object<29TLockedToken<13COBBTreeGroup>>Fv"` @ 0x80070cf4 |
| `fn_15_ED8` | 0x00ED8 | 0x0054 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_F2C` | 0x00F2C | 0x0064 | 0 | compiler-generated | body identical to DOL `"__dt__Q24rstl27auto_ptr<15CCharLayoutInfo>Fv"` @ 0x802ac5ec |
| `fn_15_F90` | 0x00F90 | 0x0060 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_FF0` | 0x00FF0 | 0x00D0 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_10C0` | 0x010C0 | 0x0268 | 1 | loader/ctor machinery | the port replaces it wholesale (PortLoadElitePirate / the FSM table install) |
| `fn_15_1A0C` | 0x01A0C | 0x0008 | 0 | host accessor | DOL `(CPatterned Ing-possession virtual)` @ 0x80074774 (vtable slot 63) |
| `fn_15_1A84` | 0x01A84 | 0x00CC | 2 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_1B50` | 0x01B50 | 0x0010 | 5 | **behaviour** | the module's own logic |
| `fn_15_2284` | 0x02284 | 0x00C0 | 1 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_2344` | 0x02344 | 0x0050 | 0 | host copy | body identical to DOL `__dt__10CMemorySysFv` @ 0x802ce648 |
| `fn_15_2730` | 0x02730 | 0x0058 | 3 | host copy | body identical to DOL `"__dt__Q24rstl107pair<Ui,Q24rstl89vector<Q224CSpawnSystemKeyframeData24CSpawnSystemKeyframeInfo,Q24rstl17rmemory_allocator>>Fv"` @ 0x802f2ed0 |
| `fn_15_2788` | 0x02788 | 0x0054 | 1 | host copy | body identical to DOL `"__dt__Q24rstl46vector<P9CGuiLight,Q24rstl17rmemory_allocator>Fv"` @ 0x8027608c |
| `fn_15_2B08` | 0x02B08 | 0x005C | 2 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_2B64` | 0x02B64 | 0x0054 | 0 | host copy | body identical to DOL `"__dt__Q24rstl46vector<P9CGuiLight,Q24rstl17rmemory_allocator>Fv"` @ 0x8027608c |
| `fn_15_2BB8` | 0x02BB8 | 0x0130 | 1 | host copy | body identical to DOL `"__ct__Q24rstl61vector<24CWeaponTypeVulnerability,Q24rstl17rmemory_allocator>FRCQ24rstl61vector<24CWeaponTypeVulnerability,Q24rstl17rmemory_allocator>"` @ 0x8001c6e4 |
| `fn_15_2ECC` | 0x02ECC | 0x0044 | 0 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_2FC8` | 0x02FC8 | 0x0028 | 4 | host copy | body identical to DOL `"__eq__Q24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>CFRCQ24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>"` @ 0x8004985c |
| `fn_15_2FF0` | 0x02FF0 | 0x007C | 1 | host copy | body identical to DOL `fn_8027CCCC` @ 0x8027cccc |
| `fn_15_306C` | 0x0306C | 0x0114 | 1 | host copy | body identical to DOL `"internal_compare<Q24rstl127const_linear_iterator<c,Q24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>,Q24rstl17rmemory_allocator>>__Q24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>FQ24rstl127const_linear_iterator<c,Q24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>,Q24rstl17rmemory_allocator>Q24rstl127const_linear_iterator<c,Q24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>,Q24rstl17rmemory_allocator>Q24rstl127const_linear_iterator<c,Q24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>,Q24rstl17rmemory_allocator>Q24rstl127const_linear_iterator<c,Q24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>,Q24rstl17rmemory_allocator>"` @ 0x800210f0 |
| `fn_15_3180` | 0x03180 | 0x0108 | 1 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_3288` | 0x03288 | 0x0038 | 3 | host copy | body identical to DOL `"push_back_unsafe__Q24rstl63vector<26CJointCollisionDescription,Q24rstl17rmemory_allocator>FRC26CJointCollisionDescription"` @ 0x80137004 |
| `fn_15_32C0` | 0x032C0 | 0x0020 | 2 | host copy | body identical to DOL `AddChildWidget__10CGuiWidgetFP10CGuiWidgetbb` @ 0x8027d930 |
| `fn_15_32E0` | 0x032E0 | 0x0028 | 1 | host copy | body identical to DOL `"construct_impl<Q218CLayoutDescription24CScaledLayoutDescription>__4rstlFPvRCQ218CLayoutDescription24CScaledLayoutDescription"` @ 0x802b1490 |
| `__ct__26CJointCollisionDescriptionFRC26CJointCollisionDescription` | 0x03308 | 0x00C0 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_33C8` | 0x033C8 | 0x005C | 0 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_3424` | 0x03424 | 0x0150 | 2 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_3AE0` | 0x03AE0 | 0x0060 | 1 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_3B40` | 0x03B40 | 0x0038 | 2 | host copy | body identical to DOL `"destroy<Q24rstl399pointer_iterator<Q24rstl144pair<Q24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>,Q24rstl53vector<16CEffectComponent,Q24rstl17rmemory_allocator>>,Q24rstl189vector<Q24rstl144pair<Q24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>,Q24rstl53vector<16CEffectComponent,Q24rstl17rmemory_allocator>>,Q24rstl17rmemory_allocator>,Q24rstl17rmemory_allocator>>__4rstlFQ24rstl399pointer_iterator<Q24rstl144pair<Q24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>,Q24rstl53vector<16CEffectComponent,Q24rstl17rmemory_allocator>>,Q24rstl189vector<Q24rstl144pair<Q24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>,Q24rstl53vector<16CEffectComponent,Q24rstl17rmemory_allocator>>,Q24rstl17rmemory_allocator>,Q24rstl17rmemory_allocator>Q24rstl399pointer_iterator<Q24rstl144pair<Q24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>,Q24rstl53vector<16CEffectComponent,Q24rstl17rmemory_allocator>>,Q24rstl189vector<Q24rstl144pair<Q24rstl66basic_string<c,Q24rstl14char_traits<c>,Q24rstl17rmemory_allocator>,Q24rstl53vector<16CEffectComponent,Q24rstl17rmemory_allocator>>,Q24rstl17rmemory_allocator>,Q24rstl17rmemory_allocator>"` @ 0x802925c4 |
| `fn_15_3B78` | 0x03B78 | 0x0060 | 1 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_3BD8` | 0x03BD8 | 0x0084 | 1 | host copy | body identical to DOL `"__dt__Q24rstl63vector<26CJointCollisionDescription,Q24rstl17rmemory_allocator>Fv"` @ 0x80136318 |
| `fn_15_3FC0` | 0x03FC0 | 0x00C8 | 2 | host copy | body identical to DOL `"__as__Q24rstl40optional_object<21TLockedToken<6CModel>>FRCQ24rstl40optional_object<21TLockedToken<6CModel>>"` @ 0x8004f3cc |
| `fn_15_4088` | 0x04088 | 0x006C | 0 | host copy | body identical to DOL `"__dt__Q24rstl48optional_object<29TLockedToken<13COBBTreeGroup>>Fv"` @ 0x80070cf4 |
| `fn_15_40F4` | 0x040F4 | 0x0058 | 0 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_414C` | 0x0414C | 0x0054 | 0 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_4410` | 0x04410 | 0x0078 | 0 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_4488` | 0x04488 | 0x0020 | 0 | **behaviour** | the module's own logic |
| `fn_15_47DC` | 0x047DC | 0x006C | 1 | **behaviour** | the module's own logic |
| `fn_15_4848` | 0x04848 | 0x0074 | 1 | **behaviour** | the module's own logic |
| `fn_15_4A3C` | 0x04A3C | 0x005C | 0 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_4A98` | 0x04A98 | 0x0048 | 0 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_4CBC` | 0x04CBC | 0x005C | 0 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_526C` | 0x0526C | 0x005C | 0 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_52C8` | 0x052C8 | 0x0008 | 0 | host accessor | DOL `ProjectileInfo__10CPatternedFv` @ 0x80073c6c (vtable slot 55) |
| `fn_15_5450` | 0x05450 | 0x005C | 0 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_776C` | 0x0776C | 0x0048 | 1 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_77B4` | 0x077B4 | 0x00B4 | 1 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_7868` | 0x07868 | 0x0040 | 1 | host copy | body identical to DOL `"__ct__Q24rstl56optional_object<Q218CImpactVisorEffect15SParticleEffect>FRCQ24rstl56optional_object<Q218CImpactVisorEffect15SParticleEffect>"` @ 0x8003607c |
| `fn_15_78A8` | 0x078A8 | 0x0020 | 1 | host copy | body identical to DOL `AddChildWidget__10CGuiWidgetFP10CGuiWidgetbb` @ 0x8027d930 |
| `fn_15_78C8` | 0x078C8 | 0x0028 | 1 | host copy | body identical to DOL `"construct_impl<Q218CLayoutDescription24CScaledLayoutDescription>__4rstlFPvRCQ218CLayoutDescription24CScaledLayoutDescription"` @ 0x802b1490 |
| `fn_15_78F0` | 0x078F0 | 0x0080 | 1 | host copy | body identical to DOL `__ct__Q218CImpactVisorEffect15SParticleEffectFRCQ218CImpactVisorEffect15SParticleEffect` @ 0x80036104 |
| `fn_15_7970` | 0x07970 | 0x003C | 0 | host copy | body identical to DOL `"__dt__Q24rstl24reserved_vector<PFv_v,4>Fv"` @ 0x803274b0 |
| `fn_15_79AC` | 0x079AC | 0x003C | 0 | host copy | body identical to DOL `"__dt__Q24rstl24reserved_vector<PFv_v,4>Fv"` @ 0x803274b0 |
| `fn_15_79E8` | 0x079E8 | 0x005C | 2 | host copy | body identical to DOL `"__dt__Q24rstl56optional_object<Q218CImpactVisorEffect15SParticleEffect>Fv"` @ 0x80032d0c |
| `fn_15_7A44` | 0x07A44 | 0x0020 | 1 | host copy | body identical to DOL `AddChildWidget__10CGuiWidgetFP10CGuiWidgetbb` @ 0x8027d930 |
| `fn_15_7A64` | 0x07A64 | 0x0048 | 1 | host copy | body identical to DOL `"destroy<Q218CImpactVisorEffect15SParticleEffect>__4rstlFPQ218CImpactVisorEffect15SParticleEffect"` @ 0x80032d88 |
| `fn_15_7AAC` | 0x07AAC | 0x0054 | 1 | host copy | body identical to DOL `__dt__21CAllFormatsAnimSourceFv` @ 0x802b2f74 |
| `fn_15_955C` | 0x0955C | 0x003C | 0 | host copy | body identical to DOL `"__dt__Q24rstl24reserved_vector<PFv_v,4>Fv"` @ 0x803274b0 |
| `fn_15_98CC` | 0x098CC | 0x0068 | 0 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_9934` | 0x09934 | 0x0054 | 0 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_9988` | 0x09988 | 0x00EC | 3 | host copy | the animation setup the ctor calls: it uses the DOL's `FindBestAnimation__12CPASDatabaseCFRC16CPASAnimParmDatai` @ 0x80298d04, `NoParameter__12CPASAnimParmFv` @ 0x80297248 and `FromEnum__12CPASAnimParmFi` @ 0x80297150 |
| `fn_15_9A74` | 0x09A74 | 0x003C | 0 | host copy | body identical to DOL `"__dt__Q24rstl24reserved_vector<PFv_v,4>Fv"` @ 0x803274b0 |
| `fn_15_9AB0` | 0x09AB0 | 0x0184 | 1 | compiler-generated | body identical to DOL `__ct__16CPASAnimParmDataFQ23pas15EAnimationStateRC12CPASAnimParmRC12CPASAnimParmRC12CPASAnimParmRC12CPASAnimParmRC12CPASAnimParmRC12CPASAnimParmRC12CPASAnimParmRC12CPASAnimParm` @ 0x80079a60 |
| `fn_15_9C34` | 0x09C34 | 0x003C | 0 | host copy | body identical to DOL `"__dt__Q24rstl24reserved_vector<PFv_v,4>Fv"` @ 0x803274b0 |
| `fn_15_9C70` | 0x09C70 | 0x01A4 | 0 | **behaviour** | the module's own logic |
| `fn_15_9E14` | 0x09E14 | 0x01E4 | 1 | compiler-generated | body identical to DOL `__dt__10CPatternedFv` @ 0x80073978 |
| `fn_15_9FF8` | 0x09FF8 | 0x0094 | 1 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_A08C` | 0x0A08C | 0x0104 | 1 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_A9D8` | 0x0A9D8 | 0x00C8 | 1 | host copy | body identical to DOL `"__as__Q24rstl40optional_object<21TLockedToken<6CModel>>FRCQ24rstl40optional_object<21TLockedToken<6CModel>>"` @ 0x8004f3cc |
| `fn_15_AAA0` | 0x0AAA0 | 0x006C | 0 | compiler-generated | body identical to DOL `"__dt__Q24rstl48optional_object<29TLockedToken<13COBBTreeGroup>>Fv"` @ 0x80070cf4 |
| `fn_15_AB0C` | 0x0AB0C | 0x0054 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_AB60` | 0x0AB60 | 0x0058 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_ABB8` | 0x0ABB8 | 0x0048 | 1 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_AC00` | 0x0AC00 | 0x0074 | 1 | compiler-generated | body identical to DOL `"__ct__21TToken<11CRasterFont>FP11CRasterFont"` @ 0x80023368 |
| `fn_15_AC74` | 0x0AC74 | 0x0090 | 0 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_AD04` | 0x0AD04 | 0x0078 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_AD7C` | 0x0AD7C | 0x0090 | 1 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_AE0C` | 0x0AE0C | 0x005C | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_AE68` | 0x0AE68 | 0x0048 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_AEB0` | 0x0AEB0 | 0x0054 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_AF04` | 0x0AF04 | 0x0058 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_AF5C` | 0x0AF5C | 0x0058 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_AFB4` | 0x0AFB4 | 0x0070 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_B024` | 0x0B024 | 0x005C | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_B080` | 0x0B080 | 0x0064 | 1 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_B0E4` | 0x0B0E4 | 0x0064 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_B148` | 0x0B148 | 0x0090 | 0 | compiler-generated | body identical to DOL `__dt__Q211CSfxManager11CSfxChannelFv` @ 0x802a00ec |
| `fn_15_B1D8` | 0x0B1D8 | 0x008C | 0 | compiler-generated | body identical to DOL `"__dt__Q24rstl49reserved_vector<Q212CMidiManager12CMidiWrapper,3>Fv"` @ 0x8031562c |
| `fn_15_B264` | 0x0B264 | 0x003C | 0 | compiler-generated | body identical to DOL `"__dt__Q24rstl24reserved_vector<PFv_v,4>Fv"` @ 0x803274b0 |
| `fn_15_B2A0` | 0x0B2A0 | 0x003C | 0 | compiler-generated | body identical to DOL `"__dt__Q24rstl24reserved_vector<PFv_v,4>Fv"` @ 0x803274b0 |
| `fn_15_B2DC` | 0x0B2DC | 0x003C | 0 | compiler-generated | body identical to DOL `"__dt__Q24rstl24reserved_vector<PFv_v,4>Fv"` @ 0x803274b0 |
| `fn_15_B318` | 0x0B318 | 0x0048 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_B360` | 0x0B360 | 0x021C | 1 | loader/ctor machinery | the port replaces it wholesale (PortLoadElitePirate / the FSM table install) |
| `fn_15_B57C` | 0x0B57C | 0x0064 | 4 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_B5E0` | 0x0B5E0 | 0x0094 | 2 | compiler-generated | body identical to DOL `__ct__16CActorParametersFRC16CActorParameters` @ 0x800dfe98 |
| `fn_15_B674` | 0x0B674 | 0x007C | 1 | compiler-generated | body identical to DOL `__ct__16CLightParametersFRC16CLightParameters` @ 0x800dff2c |
| `fn_15_B6F0` | 0x0B6F0 | 0x0058 | 0 | compiler-generated | body identical to DOL `"__dt__Q24rstl84single_ptr<Q24rstl63vector<Q219CCollisionPrimitive4Type,Q24rstl17rmemory_allocator>>Fv"` @ 0x802836f8 |
| `fn_15_B748` | 0x0B748 | 0x0084 | 1 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_B7CC` | 0x0B7CC | 0x0060 | 0 | compiler-generated | ctor/dtor of a module struct (rstl/CMemory), no game logic |
| `fn_15_B82C` | 0x0B82C | 0x02CC | 1 | loader/ctor machinery | the port replaces it wholesale (PortLoadElitePirate / the FSM table install) |
| `fn_15_BAF8` | 0x0BAF8 | 0x010C | 1 | **behaviour** | the TryCommand copy for the melee command (section 8.2) |
| `fn_15_BC04` | 0x0BC04 | 0x010C | 2 | **behaviour** | the TryCommand copy for the projectile command (section 8.2) |
| `fn_15_BD10` | 0x0BD10 | 0x00EC | 1 | **behaviour** | the TryCommand copy for the loop-attack command (section 8.2) |
| `fn_15_BF6C` | 0x0BF6C | 0x00C0 | 1 | host copy | body identical to DOL `"reserve__Q24rstl63vector<26CJointCollisionDescription,Q24rstl17rmemory_allocator>Fi"` @ 0x801374c4 |
| `fn_15_C02C` | 0x0C02C | 0x0068 | 1 | host copy | body identical to DOL `"uninitialized_copy<Q24rstl146pointer_iterator<26CJointCollisionDescription,Q24rstl63vector<26CJointCollisionDescription,Q24rstl17rmemory_allocator>,Q24rstl17rmemory_allocator>,P26CJointCollisionDescription>__4rstlFQ24rstl146pointer_iterator<26CJointCollisionDescription,Q24rstl63vector<26CJointCollisionDescription,Q24rstl17rmemory_allocator>,Q24rstl17rmemory_allocator>Q24rstl146pointer_iterator<26CJointCollisionDescription,Q24rstl63vector<26CJointCollisionDescription,Q24rstl17rmemory_allocator>,Q24rstl17rmemory_allocator>P26CJointCollisionDescription"` @ 0x80137584 |
| `fn_15_C094` | 0x0C094 | 0x003C | 1 | host copy | host class code (rstl / CToken / CJointCollisionDescription / CCollisionActorManager / CGameProjectile) |
| `fn_15_C0D0` | 0x0C0D0 | 0x0808 | 0 | loader/ctor machinery | the port replaces it wholesale (PortLoadElitePirate / the FSM table install) |
| `_unresolved` | 0x0C8D8 | 0x00C4 | 0 | REL runtime | REL entry/exit, prolog/epilog, module ctor/dtor table |
| `_epilog` | 0x0C99C | 0x0024 | 0 | REL runtime | REL entry/exit, prolog/epilog, module ctor/dtor table |
| `_prolog` | 0x0C9C0 | 0x0024 | 0 | REL runtime | REL entry/exit, prolog/epilog, module ctor/dtor table |
| `ModuleDestructors` | 0x0C9E4 | 0x004C | 0 | REL runtime | REL entry/exit, prolog/epilog, module ctor/dtor table |
| `ModuleConstructors` | 0x0CA30 | 0x004C | 0 | REL runtime | REL entry/exit, prolog/epilog, module ctor/dtor table |


## 8. The five most-called unmarked behaviour functions (2026-10-21)

Metric: the REL's own call graph. "call sites" is the number of `bl` sites in the REL, "callers"
the number of distinct caller functions. Under the classes of section 7 the behaviour functions
are the module's own game logic; the most called are:

| fn | call sites | callers | what it is |
|---|---|---|---|
| `fn_15_1B50` | 5 | 3 (`5340`, `578C`, `8580`) | the XDamaged flag bit |
| `fn_15_BC04` | 2 | 2 (`4AE0`, `4D18`) | TryCommand, projectile command |
| `fn_15_BAF8` | 1 | 1 (`48BC`) | TryCommand, melee command |
| `fn_15_BD10` | 1 | 1 (`50BC`) | TryCommand, loop-attack command |
| `fn_15_47DC` (+`fn_15_4848`) | 1 | 1 (`4790`) | the attack-kind history push (+ its erase) |

The launcher group (`776C`, `77B4`, `7868`, `78A8`, `78C8`, `78F0`, `79E8`, `7A44`, `7A64`,
`7AAC`) has 2 call sites each, but they are one internal chain of the REL's launcher class
(`776C` -> `77B4` -> `7868` -> ...), i.e. one caller each, and they are the CGameProjectile glue
the port covers with the host's projectile launch; the same goes for `2FC8`/`2FF0`/`306C`,
`3180`/`3288`/`32C0`/`3424`/`3AE0`/`3B40`/`3B78`/`3BD8` (the collision managers) and
`1A84`/`3FC0`/`2B08` (the host's shock wave, particle and vulnerability code).

### 8.1 `fn_15_1B50` - the XDamaged flag: **marked**

Retail (0x1B50, four instructions, no branch):

```
lbz      r0, 0xc10(r3)        ; the flags byte
rlwimi   r0, r4, 6, 25, 25    ; bit 6 (0x40) of the byte <- bit 6 of r4
stb      r0, 0xc10(r3)
blr
```

`rlwimi` with SH=6, MB=ME=25 is the mask `1 << (31-25)` = **0x40**, so the function sets or
clears **bit 0x40 of the byte at 0xc10** (the XDamaged/ShotAt flag) from the bool in r4 - the
DOL's own `PowerPC bit N is 1u << (31-N)` rule, not `1u << N`.

The port: `CElitePirate::SetXDamaged(bool)` (`include/MetroidPrime/Enemies/CElitePirate.hpp`),
`mXDamaged = xdamaged`, where `mXDamaged` is the host's `bool mXDamaged : 1` for that bit of the
same flags byte (the header lists the byte's bits: 0x80 event 8, **0x40 XDamaged**, 0x20
alerted, 0x10, 0x08 alerted 2, 0x04, 0x02 launcher Ing-possessed, 0x01). Field by field the two
agree: same byte, same bit, same set/clear from a bool.

Call sites, retail vs port: `fn_15_5340` (Alert) and `fn_15_578C` (TargetPatrol) clear it
(`li r4,0` at 0x5410 and 0x587C); `fn_15_8580` (AcceptScriptMsg) sets it (`li r4,1`) in three
places - ReflectedDamage (0x8AA4), ResistedDamage (0x8D48) and the damage path (0x8CC8). The
port clears it in Alert and TargetPatrol (`PortCElitePirateMove.cpp:255` and `:311`) and sets it
in AcceptScriptMsg for ResistedDamage/ReflectedDamage (`PortCElitePirate.cpp:838`) and in the
damage path (`:888`) - the same three messages, two call sites for the two messages the port
shares a `case` for.

| branch | status | evidence |
|---|---|---|
| (none - the body is branch free) | (b) ran | probe `[ep-probe] 1B50 1` 1232 times (the damage path) and `[ep-probe] 1B50 0` twice (the Alert deactivate) over the two fights |

### 8.2 `fn_15_BAF8` / `fn_15_BC04` / `fn_15_BD10` - the three TryCommand copies

Retail has one copy of the template per command type: `fn_15_BAF8` (melee, called from
`fn_15_48BC`), `fn_15_BC04` (projectile, from `fn_15_4AE0` and `fn_15_4D18`) and `fn_15_BD10`
(loop attack, from `fn_15_50BC`). All three are the same shape (`fn_15_BAF8`, 0xBAF8, 0x10C):

```
0xBAF8  stwu r1,-0x10(r1) ; mflr r0
0xBB00  cmpwi r4,1        ; kStateMsg_Update
0xBB18  beq  -> Update arm
0xBB1C  bge              ; r4 >= 2
0xBB2C  cmpwi r4,3 ; 0xBB30 bge -> return      (r4 >= 3: no state message)
0xBB34  b -> 0xBBE4                            (r4 == 2: Deactivate)
0xBB20  cmpwi r4,0 ; 0xBB24 bge -> Activate arm (0xBB38)
0xBB28  b -> return                            (r4 < 0: no state message)
Activate   (0xBB38)  stw 1, 0x6b4(r3)                  ; the CTryCommand state = "ready"
                     bl DeliverCmd__16CBodyStateCmdMgrF13EBodyStateCmd (0x800ee0a0)
                     copy the command payload into 0x180..0x194
Update     (0xBB88)  r3 = &this->0x6b4, r4 = this->0x48c (the body controller), r5 = the anim state
                     bl ShouldRetry__11CTryCommandFRC15CBodyControlleri (0x801b73e4)
0xBB94           clrlwi. r3,24
0xBB98           beq -> return (no re-delivery)
                     DeliverCmd + copy the payload
Deactivate (0xBBE4) stw 0, 0x6b4(r3)                  ; the CTryCommand state = "not ready"
```

The DOL's `ShouldRetry` (0x801b73e4) is the host's `CAnimationState::CanIssueCommand`, field
for field: `state == controller->0x37c` (the body's current state) -> the command's state becomes
2 ("repeat") and no re-delivery; else the command's state 1 ("ready") -> re-deliver and keep
"ready"; else the command's state becomes 3 ("over"). The host's `CAnimationState` has exactly
these four states (`kAS_NotReady` 0, `kAS_Ready` 1, `kAS_Repeat` 2, `kAS_Over` 3) and the same
three-way test, so the port's `TryCommand` template is retail's body.

The three copies differ only in the payload they copy into the body state cmd manager: the melee
command into 0x180..0x194, the projectile command into 0x19c..0x1b0, the loop-attack command
into 0x1b8..0x1c4. The host's `CBodyStateCmdMgr::DeliverCmd(const CBodyStateCmd&)` stores the
command for all three (it keeps the base part only - a host-wide difference shared with every
other module, not this one). The port keeps one template for the three, so one body carries the
three markers.

| branch | status | evidence |
|---|---|---|
| 0xBB00/0xBB18 (Update), 0xBC0C/0xBC24, 0xBD18/0xBD30 | (b) ran | `[ep-probe] TC update ...` 3321 times over the two fights (1818 + 1503) |
| 0xBB38 (Activate), 0xBC44, 0xBD50 | (b) ran | `[ep-probe] TC activate` 13 times |
| 0xBBE4 (Deactivate), 0xBCF0, 0xBDDC | (b) ran | `[ep-probe] TC deactivate` 11 times |
| 0xBB28 (r4 < 0) and 0xBB30 (r4 >= 3) | (c) unreachable | `EStateMsg` has exactly three values (Activate 0, Update 1, Deactivate 2; `include/MetroidPrime/StateMachineCommon.hpp`), and the FSM only ever sends those three |
| 0xBB94/0xBB98 (the re-delivery) | (d) reachable but not run | see below |

Two fights were run (the 150 s one and a 170 s one that also knocks the target back with a
`damage u04ff 1 150` every poll, to get the body into a state that ignores the command). Over
3321 update samples of the three copies the body was in the command's own animation state in
3316 of them; the five samples where it was not (body 5 = locomotion, command 18 or 7) all had
the command's state already at 2 ("repeat") or 3 ("over"), never 1 ("ready"), and the six samples
with the command still "ready" all had the body in the command's state. The re-delivery arm
needs the body controller to *not* be in the command's animation state when the FSM updates. The probe prints the body's current state, the command's state and the command's
own state: `TC update body=7 cmd=7 m=2` (the body is in the melee state, so the DOL's
`ShouldRetry` takes the "repeat" path), `body=18 cmd=18 m=2` (the projectile attack) and
`body=5 cmd=7 m=2` (the body already left the melee state, so the "over" path). The only sample
with the command still "ready" (`m=1`) already had the body in the command's state
(`body=7 cmd=7 m=1`), because the body controller consumes the delivered command in the frame it
arrives (`CBSLocomotion`, `CBSAttack`, `CBSLoopAttack`, `CBSLoopReaction` and `CBSStep` all
check the delivered command and switch state), so the FSM's first update always finds the body in
the command's state. The arm is therefore reachable in principle (the body would have to be in a
state that ignores the command, e.g. the knock-back state) but did not run in the fight: **the
three copies get no marker** until a run hits it.

### 8.3 `fn_15_47DC` (+ `fn_15_4848`) - the attack-kind history: **marked**

Retail `fn_15_47DC` (0x47DC) pushes the kind onto the three-deep history at 0xc14 and calls
`fn_15_4848` (0x4848, the erase) when it is full; `fn_15_4790` (0x4790, the marked caller)
chooses the branch on the message (`cmpwi r5,2` = Deactivate) and resets 0xc2c afterwards.

```
0x47F8  lwz r0, 0x0(r3)      ; the count
0x47FC  cmpwi r0,3 ; 0x4800 bne -> skip the erase
0x4804  addi r4, r30,4 ; 0x4808 bl fn_15_4848     ; erase the oldest
0x480C  lwz r0, 0x0(r30) ; slwi r0,r0,2 ; add r3,r30,r0 ; addic. r3,r3,4
0x481C  beq -> skip the store (the element pointer is never null)
0x4820  stw r31, 0x0(r3)   ; the new element
0x4824  lwz r3, 0x0(r30) ; addi r0,r3,1 ; stw r0, 0x0(r30)   ; count++
```

`fn_15_4848` (0x4848) is the vector erase: it checks the iterator against the begin and the end
(0x4850 `cmplw r4,r0` / 0x4868 `bge`), then shifts the elements down (the loop at 0x4878) and
decrements the count (0x4898). The port's `SetAttackKind` does the same with
`rstl::reserved_vector<int,3>`: `if (size()==3) erase(begin()); push_back(mAttackKind);
mAttackKind = 0;`.

| branch | status | evidence |
|---|---|---|
| 0x47FC/0x4800 (the history is not full) | (b) ran | `[ep-probe] 47DC push (size 0)`, `(size 1)`, `(size 2)` |
| 0x4804/0x4808 -> `fn_15_4848` (the history is full) | (b) ran | `[ep-probe] 47DC erase (full)` 14 times |
| 0x481C (the null element pointer) | (a) compiler-generated | the element address of a reserved vector is never null; the store is skipped only for a null pointer the compiler cannot prove absent |
| 0x4850/0x4868 (`fn_15_4848`'s range check) | (a) compiler-generated | the erase is always of `begin()`, so the iterator is never out of range; the check is the vector's own bounds guard |

### 8.4 What this lane marked

`fn_15_1B50`, `fn_15_47DC` and `fn_15_4848` (the last two share the marked body of
`fn_15_4790`). The three TryCommand copies stay unmarked: their re-delivery arm did not run
(8.2).
