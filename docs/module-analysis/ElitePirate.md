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
| 0x810 / 0x81c | zeroed `CVector3f` | use [?] |
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
| 0xa38 / 0xa40 / 0xa44 | floats [?] | |
| 0xa48 / 0xa4c | time of the last push-back / claimed path region (ctor stores **0**, `fn_15_1328` tests −1) [?] | |
| 0xa50 | `CPathFindSearch`; flags `IsAnEncounter() ? 0x201 : 1` | [proven] |
| 0xb48 | `SPositionHistory(5.f)` (DOL `fn_801931E0`) | `fn_15_2204` [proven] |
| 0xc10 | flags: event 8, XDamaged, alerted ×2, launcher-Ing-possessed, 3 [?] | |
| 0xc2c | **attack kind**: 2/8 rocket, 4 melee, 7 shock wave; 3/5/6 skip push-back | DoUserAnimEvent, `fn_15_8FB8` |
| 0xc30 | `'FSM2'` token (`data+0x00`) | [proven] |
| 0xc40 | initial idle: **1**, then 10/11/12 if `FindBestAnimation(state 5, 0, n)` = the editor's initial anim | [proven] |
| 0xc44 / 0xc50 / 0xc54 | event-7 bit / last melee time (−1000) / melee + knock pending bits (event 17) | |
| 0xc58..0xc6c | `fn_15_B080` block: 0, −1000, shield damage, shield hit time, shield alpha, **polarity** (0 dark, 1 light, 2 none) | XDMG, Render [proven] |
| 0xc70 / 0xc80 / 0xc94 / 0xc9c | light / dark shield-pop `'PART'`, hand `CElementGen`, shield `CSkinnedModel` | `fn_15_3C5C`, `fn_15_8E34`, `fn_15_8080` |
| 0xcac | 0x40 shield-pop pending, 0x80 double speed | Think |
| 0xcb0 | 1; **5** selects the double shock wave (`fn_15_1A48`) | |
| 0xcd0 | `CProjectileInfo(data.rocket, data.rocketDamage)` | `fn_15_728C` |
| 0xcf8 / 0xd00 / 0xd08 | rockets fired / rocket pattern (0 homing, 1 spread) / event 18 seen | `fn_15_728C` |
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
Floor; ShouldFire include Solid|Character (bits 0x3b/0x1f/0x21). `[?]`: `ReadyToCharge` calling
`ShieldKilled` when the player is in a no-attack hint (literal vtable reading), `x81c_` as an
alert position in `ShouldAlert`, `xd04_`/the 20% repeat in `PickAttackType`, and the identity of
`xa40_`/`xa44_`/`xc5c_`/`xb3c_` (read, not yet written by any ported code).

**Combat states (2026-10-19)** in `PortCElitePirateCombat.cpp`: Shielding `41A0`, ShieldUp `44A8`,
MeleeAttack `48BC`, Shockwave `4AE0`, SpreadShot `4D18`, ProjectileAttack `50BC`, AngryAttackBegin
`1900`, AngryAttackEnd `18D8`, Taunt `5A60`, PowerUp `5F08`, PowerDown `5FF0`, Dead `7B00`, plus
`ResetAttackTimer` (`1F2C`) and `TryCommand` (`BAF8`/`BC04`/`BD10`, retry a body command until the
body state is entered). They reuse the movement file's `SetAttackKind`/`UpdateAbsorb`/`TauntCmd`/
`TurnCmd`/`PathToward`. The shield's looping sound is a host member (`mShieldSfx`, after 0xc98).
Writers: `xc5c_` = ShieldUp (shield-raise time), `xa40_` = PathToward (time stuck), `xa44_` = Pursue
(`5BB4`, from 0xa3c), `xb3c_` = TargetPatrol (`578C`, from 0x334); `xd1c_` is the taunt type (an int,
Taunt cycles 0 -> 6 -> 1 -> 0). `[?]`: Shielding's `xa38_` countdown (only the ctor writes 0xa38).
In game (Sanctuary area 4, layer 3), Shockwave, ProjectileAttack, SpreadShot, AngryAttackBegin/End
and PowerUp/PowerDown are entered with no fault; later runs reached the rest (item 1 below).

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
     polarity 0: 2/3); console `hurt` bypasses it and annihilator fire in the test did not
     register a shield hit.
   Console notes: `hurt` does not set ShotAt (`mXDamaged`); `send u<id> Alert` powers a
   PowerDown Ingsmasher up; `obj` takes uids only and prints connections as editor ids.
   u051b in area 4 stays in FollowAttackPattern at (81.1, 48.4, -152.1): its pattern is one
   waypoint (u0518, 71.3, 38.6), the navigation commands (-3.3, -3.3) every frame (body state 5,
   not frozen), but it sits inside its pod (CScriptActors u0455..u0464 at its position) with
   platform u045d on the path. The intro trigger u04fc (inactive until the player enters) starts
   sequence timer 000400af, which releases it; the console `Alert` skips that, so this is a test
   artifact, not a port bug.
   Dark Ingsmasher (`0m_cliff_hall_dark`, layers 4/5): 0 caught; u04fa stands at z -114.7 and
   enters PowerUp, Wait, Turn, Alert, Taunt, ShieldUp, Shielding, MeleeAttack and Shockwave. Fixed 2026-10-19: active Ingsmashers fell through the floor
   (free fall at the 24.525 gravity) because the ctor built `mCollidableBox` from
   `GetBoundingBox()`, the world-space box; fn_15_2CFC stores the local box. With the fix both
   stand at z -152 for 60 s and u04ff walks, pursues and attacks.
2. `[?]` members: 0x7c0, 0x810/0x81c, 0xa26, 0xa38/0xa40/0xa44, 0xc14/0xc28/0xc3c/0xc48/0xc4c,
   0xc58/0xc5c, 0xcb4, 0xcb8/0xcc4, 0xcfc, 0xd04, 0xd1c/0xd20/0xd24, and 0xa4c's ctor value 0.
3. `fn_15_728C` indexes its four spread offsets by the rocket count with no bound.

**Next (2026-10-19, `decomp/elitepirate` at the commit adding this line):**
1. The `[?]` items above, as narrow `worker` lanes (decomp lanes are worker-only), each from a
   named commit.
2. Optional: reach Stunned `59BC` with real shield hits (enter area 4 normally so the intro
   sequence runs, raise the shield, fire the matching beam), then mark it.
3. Item 3 (`fn_15_728C` unbounded index): check retail's rocket count bound before guarding.
Harnesses: `/tmp/opencode/ep/trace/run3.sh` (area 4), `/tmp/opencode/ep/dark/stun_run.sh` (dark).
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
