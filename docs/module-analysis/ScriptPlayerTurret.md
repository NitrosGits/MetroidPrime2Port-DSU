# ScriptPlayerTurret REL (module 63): analysis for a host body

Analysis + host body, milestones 1-5 (2026-10-20). Sources: the archive build's disassembly
`../MetroidPrime2Port/build/G2ME01/ScriptPlayerTurret/asm/` (`auto_00_00000050_text.s` is the whole
`.text`, `auto_03/04/05_*` are rodata/data/bss, `MetroidPrime/ScriptObjects/CScriptPlayerTurretRel.s`
is the REL's own file), the m2c pseudo-C in `/tmp/opencode/pseudo/ScriptPlayerTurret/`, the
generated `include/MetroidPrime/ScriptLoader/SLdrPlayerTurret.{hpp,inc}` and the DOL's
`config/G2ME01/symbols.txt`. Confidence tags as in `SnakeWeedSwarm.md`: **[proven]** = read from an
instruction/table entry; **[probable]** = strong evidence but not proven; **[?]** = guess. All
`fn_63_X` are REL `.text` offsets. The implementation is
`src/MetroidPrime/ScriptObjects/PortCScriptPlayerTurret.cpp` and the `TARGET_PC` half of
`include/MetroidPrime/ScriptObjects/CScriptPlayerTurret.hpp`.

**Shape in one line:** a **script object** (`CActor`, not `CPatterned`): the guest object is 0x230
bytes and Echoes' `CActor` is 0x158, so the turret owns 0xD8 bytes of its own. It has no model of
its own; it points at two other actors - the base actor (which carries the model) and the top
actor - and the player enters it to aim.

Module: id 63, 14868 text bytes, 59 functions, script type `PLRT`, entity type
`kET_ScriptPlayerTurret` = 138 (already in `include/MetroidPrime/CEntityInfo.hpp`).

## 1. Function map

| fn | size | role | evidence |
|---|---|---|---|
| `RELMain` | 0x20 | calls `fn_63_70` | [proven] |
| `fn_63_70` | 0xD4 | stores the loader `fn_63_144` at `lbl_63_bss_20`+0, copies the five pointers-to-member `lbl_63_data_4` = {0, -1, `fn_63_1070`}, `lbl_63_data_10` = {0, -1, `fn_63_F6C`}, `lbl_63_data_1C` = {0, -1, `fn_63_1B70`}, `lbl_63_data_28` = {0, -1, `fn_63_1134`}, `lbl_63_data_34` = {0, -1, `fn_63_840`} to +0x4 .. +0x3C (0x40 table = `SPlayerTurret_FuncPtrs`), then the DOL's `SetSPlayerTurret_FuncPtrs` | [proven] |
| `RELExit` | 0x24 | `SetSPlayerTurret_FuncPtrs(nullptr)` | [proven] |
| `fn_63_0` | 0x2C | `GetHealthInfo` (the DOL's inline, emitted in the REL): `bctrl` through vtable +0x38 = slot 14 `HealthInfo` | [proven] vtable slot 15 + body |
| `fn_63_144` | 0x4CC | **loader** `LoadPlayerTurret` (section 3) | [proven] `bss_20`+0 |
| `fn_63_30E8` | 0x3B4 | **constructor** | [proven]: called right after `rs_new 0x230` (@0x00000494) |
| `fn_63_3070` | 0x78 | **destructor**: the vtable is reset, the token at 0x1E8 is destroyed, `~CActor`, then the free | [proven] vtable slot 2 |
| `fn_63_23C0` | 0x968 | **`Think`**: the aim, the sounds, the hull transform (section 4) | [proven] slot 5 |
| `fn_63_1BE8` | 0x784 | **`AcceptScriptMsg`** (section 4) | [proven] slot 6 |
| `fn_63_2D68` | 0x2CC | **`PreRender`**: the hull's light colour | [proven] slot 9 |
| `fn_63_3034` | 0x3C | **`GetTouchBounds`**: copy the box at 0x158 into the returned optional | [proven] slot 18 + whole body |
| `fn_63_1070` | 0xC4 | **`GetCameraTransform(mgr)`** (the PMF at `bss_20`+0xC) | [proven] PMF + body |
| `fn_63_F6C` | 0xB4 | **`GetTurretTransform(mgr)`** (the PMF at +0x18) | [proven] PMF + body |
| `fn_63_1B70` | 0x78 | **`ExitTurret(mgr)`** (the PMF at +0x24) | [proven] PMF + body |
| `fn_63_1134` | 0x32C | **`ProcessInput(input, mgr)`** (the PMF at +0x30) | [proven] PMF + body |
| `fn_63_840` | 0xC | **`GetHullActorId()`** (the PMF at +0x3C): returns the u16 at 0x19A | [proven] PMF + body |
| `fn_63_1460` | 0x54 | helper of `ExitTurret`: the actor at 0x19A loses `kMT_Solid` | [proven] call at 0x00001BC8 |
| `fn_63_14B4` | 0x54 | helper of the `0x53545254` message: the actor at 0x19A gains `kMT_Solid` | [proven] call at 0x00001FF0 |
| `fn_63_15B0` | 0x3C4 | reads the two connections (`FindConnectedObject`) and stores the base actor at 0x198 and the top actor at 0x19A, then looks up five animation ids (0x19C..0x1AC) and the base actor's light colour (0x1D4..0x1DC); called from the `0x58414C44` message | [proven] the two stores at 0x00001818/0x00001860 |
| `fn_63_374C` | 0x20 | `CModelData`'s default constructor (the turret's empty model) | [proven] call at 0x00003268 |
| `fn_63_84C` | 0x184 | the turret's **shot**: the energy projectile, spawned at the muzzle (the `muzzleend_LCTR` locator) | [proven] call at 0x000012C8 |
| `fn_63_349C` | 0x74 | `CToken(IObj* null)`: the turret's empty token at 0x1E8 | [proven] call at 0x00003384 |
| `fn_63_674` | 0xB8 | `SLdrEditorProperties`' constructor (the loader's defaults) | [proven] call at 0x00000238 |
| `fn_63_610` | 0x64 | `SLdrEditorProperties`' destructor | [proven] |
| `fn_63_72C` | 0x114 | a second property reader (0x4D577910, 0xF3EC8748, 0xF598739, 0x56F98C49) - unreachable from this module's loader | [proven] no caller in the REL |

### Vtable `lbl_63_data_40` (31 words, 2 leading zeros)

Slot numbers count the two header words, as in the other analyses. Overridden (REL address):
2 dtor `fn_63_3070`, 5 Think `fn_63_23C0`, 6 AcceptScriptMsg `fn_63_1BE8`, 9 PreRender `fn_63_2D68`,
15 GetHealthInfo `fn_63_0`, 18 GetTouchBounds `fn_63_3034`.
Inherited (DOL symbols in the slot): 3 `CScriptPlayerTurret::TypesMatch`, 4 `CEntity::PreThink`,
7 `CActor::SetActive`, 8 `ClearFluidList`, 10 `AddToRenderer`, 11 `Render`, 12 `CanRenderUnsorted`,
13 `PreRenderAllViewports`, 14 `HealthInfo`, 16/17 `GetDamageVulnerability`, 19 `Touch`,
20-30 `GetOrbitPosition` .. `ProcessSoundEvent`.

## 2. CScriptPlayerTurret members (0x230, the loader's `rs_new`)

All from the constructor `fn_63_30E8` unless noted. `CActor` is 0x158 in the guest, so the turret's
own fields run 0x158..0x22F. The host's layout is its own (Echoes' `CActor` grows under
`TARGET_PC`), so the offsets below are the guest's and are the reference for milestone 2.

| off | member | evidence |
|---|---|---|
| 0x158 | `CAABox` - the actor's position grown by 0.5 (`lbl_63_rodata_5C`) | [proven] the `CAABox` constructor call at 0x00003240 |
| 0x170 | `uint` - the `flagsPlayerTurret` property | [proven] store of the loader's r8 |
| 0x174..0x184 | five floats: max horizontal rotation left/right, max vertical elevation up/down, damage angle (all radians) | [proven] stores of f24..f28; [probable] the order, from the loader's argument order |
| 0x188 | `u16` - the player in the turret (`kInvalidUniqueId` at birth) | [proven] store of `kInvalidUniqueId` |
| 0x18C, 0x190, 0x194 | horizontal speed, vertical speed, fire rate | [proven] stores of f29..f31 |
| 0x198 | `u16` - the **base actor** (the one with the model) | [proven] |
| 0x19A | `u16` - the **top actor** (the one moved onto the hull's locator); also what `fn_63_840` returns | [proven] |
| 0x19C, 0x1A0 | two animation ids (the additive animations) | [proven] zeroed |
| 0x1A4, 0x1A8, 0x1AC | three zeroed words | [proven] |
| 0x1B0 | the **pitch the input asks for**: `fn_63_1134` writes it (the stick's forward axis times the elevation limit) and `fn_63_23C0` reads it | [proven] the store at 0x00001430/0x00001440 |
| 0x1B4 | the **pitch the turret has reached**: `fn_63_23C0` moves it toward 0x1B0 and clamps it to -0x180..0x17C | [proven] the clamp at 0x00002B00 |
| 0x1B8, 0x1BC, 0x1C0 | three floats zeroed at birth: the reload timer, `Think`'s rotation-sound debounce (0..0.05, lbl_63_rodata_48) and its alignment (top actor's forward . last frame's, forced to 1 with no input) | [proven] 0x2B28, 0x2BFC |
| 0x1C4..0x1CC | the editor transform's forward vector (m01, m11, m21); `Think` overwrites it with the top actor's forward each frame | [proven] the loads from the transform, the stores at 0x2B78 |
| 0x1D0 | the **yaw the input asks for**: `fn_63_1134` writes the turn axis into it | [proven] the store at 0x00001334 |
| 0x1C0 | the yaw the turret has reached: `fn_63_23C0` writes the top actor's forward dotted with the turret's | [proven] the store at 0x00002A50 |
| 0x1D4..0x1DC | the base actor's **model flags**: a word, two bytes, a u16 and the packed `CColor(1,1,1,1)` - the flags 3 and the blend modes 0 and 5 in `fn_63_2D68` are `CModelFlags`' | [proven] the `CColor` constructor call at 0x00003348 and the blend modes at 0x00002E54/0x00002ED4 |
| 0x1E0, 0x1E4 | the two weapon effect asset ids | [proven] the loader's args |
| 0x1E8 | `CToken` - the hull's model token, created empty; the `kSM_Create` message fills it with the weapon effect's resource | [proven] `fn_63_349C`, the store at 0x00001D08 |
| 0x1F0..0x20A | the weapon damage (`CDamageInfo`: mode, four floats, three ids, a byte) | [proven] the copy pattern at 0x00003398 |
| 0x204 | overwritten with **0x2812** after the damage copy | [proven] the store at 0x00003444; the host's `CDamageInfo` has no setter for that field |
| 0x20C | a float zeroed at birth | [proven] |
| 0x210..0x21C | the editor transform's translation (m03, m13, m23) | [proven] |
| 0x21C | the sound handle (`CSfxHandle`), zeroed | [proven] |
| 0x220 | `u16` - the hull actor id, `kInvalidUniqueId` at birth | [proven] |
| 0x222..0x22A | five sound ids: turret rotation, single/multi player impact, single/multi player projectile | [probable]: the order, from the loader's stack arguments |
| 0x22C | flag byte: bit 0x80 = the player is in the turret (set by the activate message, cleared by `fn_63_1B70`), bit 0x40 cleared at birth | [proven] the two `rlwimi` at 0x00003430 |

## 3. The loader (fn_63_144)

The property loop is the generated one (`SLdrPlayerTurret.inc`): 0x255a4580 editor properties,
0xEEADEFA6 flags, 0x17CD8B2A/0x1473DAD2 max horizontal rotation left/right,
0x3650CE75/0x78520E6E max vertical elevation up/down, 0xA39A5D72 damage angle, 0xFB2E32DB
horizontal speed, 0x1B3C8683 vertical speed, 0xC6E48F18 fire rate, 0x8E5F7E96 weapon damage,
0xC43360A7/0xA99D3DBE the two weapon effects and 0xE7234F72/0x3E2F7AFB/0x7CABD1F1/0x7EF976EB/
0x035459FD the five sound ids.

Then `rs_new 0x230` and the constructor with: the uid, the editor properties' name, the entity info,
the editor transform, `inGrave` 0, an **empty** `CModelData`, the material list, a default
`CActorParameters`, `kInvalidUniqueId` as the next draw node, and on the stack the flags, the
weapon damage, the weapon effect, the multi-player weapon effect, the five sound ids and the
**seven angles and speeds converted from degrees** with `lbl_63_rodata_0` = 0.017453292 (the fire
rate is left alone). A sound id of -1 becomes the invalid sfx id.

Material list: `lbl_63_data_0` = 0x15 through the DOL's `__shl2i` helper (0x000031C8) - the same
shape the SnakeWeedSwarm loader uses, where the shift is the material index, so this is
**kMT_CameraPassthrough**. [probable]: the helper also sets the high word's bit (21 + 32 = 53), which
the SnakeWeedSwarm port drops as well.

## 4. What the big two do (milestone 2)

`fn_63_23C0` (**Think**, slot 5): while the turret is **active** (the guest's byte 0x20 bit 0x80 -
`CEntity`'s active flag, read with `extrwi. r0,r0,1,24` at 0x000023F0; the *player's* flag is the
turret's own byte 0x22C) it counts down 0x20C, looks at the player (0x188) and the base actor
(0x198), clamps the aim to the rotation limits (0x174..0x180), drives the two additive animations
(0x19C, 0x1A0), moves the top actor (0x19A) onto the base actor's `"upwardballsnap_LCTR"` locator,
and runs the turret's sound (0x222).

`fn_63_1BE8` (**AcceptScriptMsg**, slot 6) switches on the message word:
0x41435456 `kSM_Activate` (the player enters: the turret's transform and position go to the base
actor, the top actor goes onto the locator), 0x44435456 `kSM_Deactivate`, 0x53545254 `kSM_Start`,
0x53544f50 `kSM_Stop` (the player is ejected and killed), 0x58435254 and 0x58414C44 (the hull's
model token), 0x58444d47 (the damage notification).

`fn_63_2D68` (**PreRender**, slot 9): the base actor's light colour, white while the player is in
the turret and black-to-red while 0x20C counts down.

## 5. The strings (lbl_63_rodata_60)

| off | string | used by |
|---|---|---|
| 0x00 | `??(??)` | the allocation's file tag (the `__nw__` source name) |
| 0x07 | `upwardballsnap_LCTR` | `fn_63_F6C`, `fn_63_23C0`, `fn_63_1BE8` - the base actor's locator |
| 0x1B | `Whole Body` | `fn_63_1134` - the animation the input path watches |
| 0x26 | `muzzleend_LCTR` | `fn_63_1508` reads it from `lbl_63_bss_C`, the static string the module's initializer `fn_63_376C` builds from it (0x00003788) |

## 6. Host body (milestone 1)

`PortCScriptPlayerTurret.cpp`: the loader, the constructor, `GetTouchBounds` and the five PMFs.
`Think`, `PreRender` and `ProcessInput` are too big for that slice and forward to `CActor` (or, for
`ProcessInput`, only log) behind a once-only `[turret] stand-in fn_63_XXXX` line. The DOL's proxies
in `src/MetroidPrime/ScriptLoaderRel.cpp` call through `SPlayerTurret_FuncPtrs`, so the turret
camera (`CPlayer::SetTurretState`), the exit path and the turret HUD (`CTurretHud::UpdateEnergy`)
keep working. Milestone 2 (section 8 and 9) adds the message switch of `AcceptScriptMsg`.

The upstream header `include/MetroidPrime/ScriptObjects/CScriptPlayerTurret.hpp` only named two
fields of the object (the aim limit at 0x184 and the target position at 0x210) and left the rest
opaque; the port adds the whole guest layout in its `TARGET_PC` half, so the upstream text stays
byte-identical for the matching build.

## 7. Where its actors are

From `docs/modules.tsv`: **Agon Wastes/06_sand_alamofront[L3]**, **M06_ShootingGallery/06_ShootingGallery**
and **Sanctuary Fortress/01_cliff_entrance[L6]** (3 areas).

**Runtime check (2026-10-20).** `WORLD=M06_ShootingGallery,06_ShootingGallery FRAMES=900 GODMODE=1
tools/run_gameplay.sh` (the shooting gallery is the one spawn area whose turret sits on layer 0,
so it loads with the area): the world jump resolves to `M06_ShootingGallery` (7e19ed26) area 0
(d97a600b), and the log shows

    [turret] stand-in fn_63_1BE8 (CScriptPlayerTurret::AcceptScriptMsg) (see PortCScriptPlayerTurret.cpp)
    [turret] stand-in fn_63_23C0 (CScriptPlayerTurret::Think) (see PortCScriptPlayerTurret.cpp)

i.e. the loader built the object, `kSM_Create` reached it and `Think` runs every frame. 900 game
updates (the harness: "gameplay reached at 13.7 s (frame 512)", "the frame limit is up at 20.1 s:
world 7e19ed26, area 0 of 1"), `exit=0`, no `[port] caught` and no `reach-stub` (both counts 0).
`PreRender` did not fire: the turret has no model of its own, so it is never drawn.

The other two spawn areas need a layer: Agon Wastes/06_sand_alamofront is layer 3 and Sanctuary
Fortress/01_cliff_entrance is layer 6, so the turret only appears once that layer is active (the
console's `layer <n>` verb).

**Milestone 2 (next).** `fn_63_23C0` and `fn_63_1BE8` are the two big ones, then `fn_63_2D68`; the
hull's model token (0x1E8) and the damage sfx id 0x2812 (0x204) come with them.

## 8. The message cases of fn_63_1BE8 (milestone 2)

The switch compares the message's word (the guest's `CScriptMsg` + 0x8) against the four-character
codes; every case ends in `CActor::AcceptScriptMsg` (0x2348). The cases and their sizes:

| msg | code | asm | size | what it does |
|---|---|---|---|---|
| `kSM_Create` | 0x58435254 | 0x1C9C | 0x108 | the hull's model token (0x1E8) from the weapon effect asset (ported) |
| `kSM_Activate` | 0x41435456 | 0x1DA4 | 0x1B4 | the player enters (ported) |
| `kSM_Deactivate` | 0x44435456 | 0x1F58 | 0x58 | the player leaves (ported) |
| `kSM_Start` | 0x53545254 | 0x1FB0 | 0x58 | the turret starts (ported) |
| `kSM_Stop` | 0x53544F50 | 0x2008 | 0x23C | the player is ejected and killed (ported) |
| `kSM_AreaLoaded` | 0x58414C44 | 0x2244 | 0x8 | `fn_63_15B0` (0x3C4): only its two connections are ported |
| `kSM_Damage` | 0x58444D47 | 0x224C | 0xFC | the damage flash and the impact sound (ported) |

Facts the ported cases established:

- The message's **originator** (the guest's `CScriptMsg` + 0x2) is the player: the trigger's
  `kSM_Activate` carries the player that entered it, and the turret stores it at 0x188.
- The player may enter only with health: the case calls the player's `GetHealthInfo` (the vtable's
  slot 15, 0x3C) and reads the float at + 4 of it - the guest's `CHealthInfo` starts with the
  *initial* health, so + 4 is the current one (0x1DD8..0x1E00).
- The base actor (0x198) keeps its own position but takes the turret's rotation; the top actor
  (0x19A) goes onto the base actor's `"upwardballsnap_LCTR"` locator (0x1E88..0x1F3C).
- `kSM_Start` needs the turret to be **active** (the guest's byte 0x20, `CEntity`'s active flag),
  sets the player's turret state to 3 (kTS_Active), gives the top actor `kMT_Solid` (0x3b,
  `fn_63_14B4`) and the turret itself `kMT_Orbit` (0x29).
- The turret's base and top actors are its **connections**: `kSS_AttachedAnimatedObject` (0x41544F42,
  "ATOB") for the base actor and `kSS_AttachedCollisionObject` (0x4154434C, "ATCL") for the top
  actor, both with the message `kSM_Attach` (0x41544348) - `fn_63_15B0` at 0x15E4 and 0x1930.
- `kSM_Stop` gives the base actor the turret's saved model flags (0x1D4..0x1DC) back, ejects the
  player (`EjectFromTurret`), kills it (`KillPlayer` with the player's own health and the hull
  actor id 0x220), zeroes both rotations and plays the turret's animation (0x1AC) on the base
  actor; the animation data's flag byte at 0x2AC loses bit 0x40 (the loop flag) and gains bit 0x80
  (the animating flag) - the host's `EnableLooping(false)` does both.
- `kSM_Damage` sets the flash timer 0x20C to 0.15 (lbl_63_rodata_20), plays the multi-player
  impact sound (0x226) at the player's sound pan or, in single player, the impact emitter (0x224)
  at the turret's target position (0x210), and takes the hull actor id (0x220) from the damaged
  actor's health info - the owner of the last damage (the guest's `CHealthInfo` + 0x18), or the
  message's originator when the actor has no health info.
- The guest's `CGameState` byte 0xA4 bit 0x40 (`extrwi. r0,r0,1,25` - the word's bit 25 is the
  byte's 0x40) is the game's **invert Y** option: it swaps the two stick axes of the turret's
  pitch. The host's member is `CGameOptions::GetInvertYAxis()` (the options' bitfield byte has
  `invertY` as its second field, so the bit is the same one the guest reads).
  **[probable]**: the guest's `CGameState` and the host's have the same size (0x2f0) and the same
  member order, but the offset 0xA4 was not confirmed inside the options' bitfield byte.

## 9. Milestone 2 (the message switch) - host body and runtime

`AcceptScriptMsg` (fn_63_1BE8) now has retail's switch with **all seven cases ported**;
`fn_63_14B4` (0x54) and `fn_63_1460` (0x54) are ported as named methods, and `ProcessInput`
(fn_63_1134, 0x32C) is ported. `fn_63_15B0` (0x3C4) is ported as well: the two
`FindConnectedObject` calls, the base actor's model flags and the five animation ids (0x19C..0x1AC)
- the getup twice, the step, the death and the locomotion animation, each looked up with one enum
parameter and seven empty ones (fn_63_19B0, 0x184, now a named method). The connections have to be
real because the DOL's turret HUD (`CTurretHud::UpdateEnergy`) reads the hull actor's id and
dereferences the actor it names: without them the run **crashes** (SIGSEGV in
`CTurretHud::UpdateEnergy`) as soon as the player is in the turret.

The module's own helper that is too big for a slice stays a named stand-in: `fn_63_84C` (0x184,
the turret's shot - the energy projectile).

**Runtime check (2026-10-20).** `WORLD=M06_ShootingGallery,06_ShootingGallery FRAMES=2400
GODMODE=1 MP2_CONSOLE=4780 tools/run_gameplay.sh` with the console's `tp 0 -27 11` (the turret and
the trigger that activates it are at (0, -27, 10); the player spawns at (-100, 20, 4)):

- The stand-ins that log: `fn_63_15B0` (the hull's animation ids, at the area load),
  `fn_63_23C0` (Think), `fn_63_2D68` (PreRender, once the player is in the turret) and
  `fn_63_84C` (the turret's shot, once the player fires).
- The player **gets on the turret**: the console's `status` prints `in-turret 1`, the player sits
  at the turret's position (0, -27, 10) and the camera is the turret's (`GetCameraTransform` over
  the base actor's locator: (0, -24, 10)).
- **The input reaches the turret**: the console's `press A 20` makes the turret fire, and the
  `fn_63_84C` stand-in logs - so `ProcessInput` (the ported fn_63_1134) sees the fire button and
  the reload timer (0x1B8) runs.
- **`kSM_Stop` ejects the player**: the console's `send u040d Stop` (0x53544F50) prints
  `in-turret 0` afterwards and the camera is the player's again. The case also kills the player
  (`KillPlayer`), and the port then waits at the death memo - the same state the console's
  `kill` leaves the game in (checked: `kill` freezes the updates the same way), not a crash.
- 2400 game updates, `exit=0`, no `[port] caught` and no `reach-stub` (both counts 0).

## 10. Milestone 4 (the shot) - host body and runtime

`fn_63_84C` (0x184) is ported: the muzzle transform (`fn_63_1508`, 0x68, a named method), the
projectile itself - a `CEnergyProjectile` with the turret's own token (0x1E8) as its weapon
description, the exclude material 0x14 (kMT_ProjectilePassthrough), the damage's weapon mode as
its type and the player's area - the top actor (0x19A) as its collision cooldown owner (the
guest's call with lbl_63_rodata_18 = 3.4028235e38f), the base actor's shot animation (0x1A4) and
the shot's sound (0x22A in multiplayer at the player's pan, 0x228 at the fixed pan 0x40 in single
player). The guest's `fn_63_B18`/`fn_63_B60` build the projectile's visor effect; the host's
`CImpactVisorEffect::None()` is the same empty effect (the retail's AI turret uses it too).

The guest's muzzle locator is `lbl_63_bss_C`: a **static** `rstl::string` that the module's
static initializer `fn_63_376C` fills with `lbl_63_rodata_60 + 0x26` = `muzzleend_LCTR`
(0x00003788). A BSS string that the module only reads is not empty - `grep -n bss_X
auto_fn_*_text.s` finds the initializer that fills it.

**Runtime check (2026-10-20).** The same run as in section 9, with `press A 20`: the console's
`objs projectile` lists nothing before the shot and

    u0662 03ffffff  CEnergyProjectile          a00 (-0.0, -27.0, 10.0)

after it - **the projectile spawns** at the turret's position. 2400 game updates, `exit=0`, no
`[port] caught` and no `reach-stub` (both counts 0). The turret still does not aim (Think is
parked), so the shot flies straight ahead and does not reach the gallery's targets.

## 11. Milestone 5 (PreRender) - the guest's model flags

`fn_63_2D68` (0x2CC) writes the base actor's **model flags**: 12 bytes at the guest's actor
offset 0xFC - a word, the blend mode byte (0x100), the material set byte (0x101), the flags half
(0x102) and the packed colour (0x104). The blend modes it writes are 5 (`kT_Blend`) for the white
light and 2 (`kT_Two`) for the damage flash, the flags 3 (`kF_DepthCompare|kF_DepthUpdate`) and
the material set 0 - the same values the host's `CModelFlags` holds, whose members are private, so
`include/Kyoto/Graphics/CModelFlags.hpp` gets three TARGET_PC-guarded setters
(`SetTrans`, `SetMatSetIdx`, `SetFlags`).

The case: the base actor (0x198) is looked up (0x2D88); with a player in the turret (0x188) that
belongs to this game (0x2DF4) and is not leaving (the guest's player + 0x12F8 != 2, 0x2E24) the
flags become white with the alpha 0.25 (lbl_63_rodata_54); while the flash timer (0x20C) runs the
colour is `Lerp(black, red, timer / 0.33)` (lbl_63_rodata_58) with the blend mode 2; the flags
go back into the base actor (0x2F8C). The guest also raises a bit on the base actor's
`CScriptActor` (0x396, bit 1) when the colour changed - the host has no such flag, so that store
is not ported.

**Runtime check (2026-10-20).** The same run: the player gets on the turret, the turret's own
model is drawn from inside (screenshot `/tmp/opencode/turret_render.png`, the "Turret" HUD and
the lit hull in the foreground), 2400 game updates, `exit=0`, no `[port] caught` and no
`reach-stub` (both counts 0).

**Milestone 6 (next).** Only `fn_63_23C0` (Think, 0x968) is left, and it is parked for an Opus
lane. `fn_63_23C0` (Think, 0x968 - parked for an Opus lane) is what makes the
turret aim and move the hull; then `fn_63_2D68` (PreRender, 0x2CC), the rest of `fn_63_15B0`
(the five animation ids - its own helper `fn_63_19B0`, 0x184, is over the 0x80 callee cap, so it
stays a named stand-in too).

## 12. Milestone 6 (Think) - host body and runtime

`fn_63_23C0` (0x968) is ported in retail order, `// @` marked. It does **not** call
`CActor::Think`. Per frame, while active: the flash timer 0x20C counts down; the base actor's
flattened forward is turned by `ZRotation(dt * yawInput * 0x18C)` (only for |yaw input| >= 1e-5),
or, with a player not in control (turret state != 3), swung back to the turret's forward with
`CQuaternion::LookAt` at 4x speed; past the left/right limits (0x174/0x178, side from the cross
product's z - `fn_63_2D28` is the inline `CVector3f::Cross`) the base keeps its forward, else it
gets `CTransform4f::LookAt(pos, pos + dir, up)`; the yaw input 0x1D0 is zeroed. The pitch 0x1B4
slews to 0x1B0 at `dt * 0x190 * (1 - PhongBlob(clamp(|d|/(pi/2)), 8))` (4x and no ease without
control), clamped to -0x180..0x17C, and drives the additive animations 0x19C (up) / 0x1A0 (down)
with weight |pitch|/(pi/2). The top actor goes onto the base's `upwardballsnap_LCTR`. With the
player in control the rotation sound 0x222 starts/stops (looped, acoustics, the player's pan 4)
once the alignment crosses `1 - 60 * 1.001358e-5 * dt` and the 0.05 s debounce ends, and is
pitch-bent by `8192 + 4000 * pitch/(pi/2)`; without control the sound is stopped but the handle
0x21C is left set (retail).

asmcheck findings, all explained: x20 bit 0x80 is `GetActive()`; the 0x28F8 compare is inside
`CMath::Clamp`; `string_l`/`internal_dereference` are the locator string temporary;
`fn_63_2D28` is the host's `CVector3f::Cross`.

**Runtime check (2026-10-20).** `/tmp/opencode/pt_think/run.sh` (06_ShootingGallery, console
`tp 0 -27 11`, `stick 127 0 120`, `stick 0 127 90`, `press A 20`, `stick -127 -127 120`, `press B 20`):
`in-turret 1`, the camera turns from (0, -24.05) to (-2.10, -24.92) around the turret, the shot
spawns at (9.8, -26.7, 17.6) (aimed right), `B` gets off (`in-turret 0`); 4000 updates, `exit=0`,
0 `[port] caught`/`reach-stub`; screenshots `/tmp/opencode/pt_think/grid.png`.

## 13. The function inventory (2026-10-21) and the six most-called functions

The module has 59 functions. One was marked before this slice (`fn_63_23C0`). The other 58 split
into the module's own code (18, listed below) and 40 copies of host/DOL or compiler-generated code
that must never carry a marker.

### 13.1 The copies of host/DOL code (cited, never marked)

| fn | size | what it is |
|---|---|---|
| `RELMain`, `RELExit` | 0x20, 0x24 | the REL's entry and exit (they call `fn_63_70` / `SetSPlayerTurret_FuncPtrs(nullptr)`) |
| `fn_63_70` | 0xD4 | the REL's setup: the five pointers-to-member are copied into `lbl_63_bss_20` and the DOL's `SetSPlayerTurret_FuncPtrs` is called (the port does this in `RELMain`) |
| `_prolog`, `_epilog` | 0x24, 0x24 | the REL linker's own entry/exit (`ModuleConstructors`/`ModuleDestructors` + `RELMain`/`RELExit`) |
| `ModuleConstructors`, `ModuleDestructors` | 0x4C, 0x4C | the REL linker's `_ctors`/`_dtors` walkers |
| `__register_global_object`, `__destroy_global_chain` | 0x1C, 0x58 | the REL linker's global-object registry |
| `_unresolved` | 0xC4 | the REL linker's "unlinked function called" report |
| `fn_63_376C` | 0x90 | the module's static initializer (the `.ctors` entry): it builds `lbl_63_bss_C` ("muzzleend_LCTR"), registers `lbl_63_bss_0` and builds `lbl_63_bss_1C`, the module's damage colour `CColor(0.5, 0, 0, 1)` (0x000037A4) |
| `fn_63_3618`, `fn_63_36A8`, `fn_63_3704` | 0x90, 0x5C, 0x48 | the generated owner file `CScriptPlayerTurretObjOwner.cpp`: its constructor and its two destructors |
| `fn_63_3510`, `fn_63_35A0` | 0x90, 0x78 | the same owner's destructor family (the `CMemory`/refcount ones) |
| `fn_63_0` | 0x2C | the vtable slot 15 (`GetHealthInfo`): a forwarder through the vtable + 0x38, the DOL's `HealthInfo` |
| `fn_63_610`, `fn_63_674`, `fn_63_72C` | 0x64, 0xB8, 0x114 | `SLdrEditorProperties`' destructor, constructor and a second property reader (unreachable from this module's loader) |
| `fn_63_B18`, `fn_63_B60` | 0x48, 0xB4 | `CImpactVisorEffect`'s constructor - the projectile's visor effect (the host's `CImpactVisorEffect::None()` is the same empty effect) |
| `fn_63_C14`, `fn_63_C54`, `fn_63_C74`, `fn_63_C9C`, `fn_63_D1C`, `fn_63_D88`, `fn_63_DDC`, `fn_63_E18`, `fn_63_E54`, `fn_63_EB0`, `fn_63_ED0`, `fn_63_F18` | 0x40..0x5C | the `CToken`/`CMemory` copy-constructor and destructor family the compiler emitted (the guest's token is ref-counted) |
| `fn_63_1020` | 0x50 | the `rstl::string` global's destructor (registered by `__register_global_object`) |
| `fn_63_1974`, `fn_63_1B34`, `fn_63_236C` | 0x3C, 0x3C, 0x54 | the `CMemory` destructors (`Free__7CMemoryFPCv` wrappers) the compiler emitted for the placement news |
| `fn_63_349C` | 0x74 | `CToken(IObj* null)`, the turret's empty token (the host's `rstl::single_ptr< CToken >` with `nullptr` is the same thing) |
| `fn_63_374C` | 0x20 | `CModelData`'s default constructor (the turret has no model of its own) |
| `fn_63_2D28` | 0x40 | `CVector3f::Cross`, the guest's inline (the host's `CVector3f::Cross`) |

### 13.2 The module's own functions

`fn_63_144` (the loader), `fn_63_840`, `fn_63_84C` (the shot), `fn_63_F6C`, `fn_63_1070`,
`fn_63_1134`, `fn_63_1460`, `fn_63_14B4`, `fn_63_1508`, `fn_63_15B0`, `fn_63_19B0`, `fn_63_1B70`,
`fn_63_1BE8`, `fn_63_23C0`, `fn_63_2D68`, `fn_63_3034`, `fn_63_3070` (the destructor), `fn_63_30E8`
(the constructor).

### 13.3 The six most-called ones

The counts are the runtime calls of one natural run (06_ShootingGallery, the player enters the
turret, fires and aims): `fn_63_3034` 7277, `fn_63_F6C` 4849, `fn_63_23C0` 2880 (already marked),
`fn_63_1070` 2425, `fn_63_2D68` 2423, `fn_63_840` 2304, `fn_63_1134` 2303. The next ones are far
behind (`fn_63_84C` 1, `fn_63_1BE8` 6), so the six below are the ones a run can really cover.

| fn | verdict | what the run proved |
|---|---|---|
| `fn_63_3034` | **marked** | branch-free (the guest copies the box at 0x158 and sets the optional's flag); 7277 calls |
| `fn_63_F6C` | **marked** | the base actor is found (4849 calls); the other arm (no base actor) is a guard the game's flow cannot reach: the hull's connections are set at the area load, before the camera ever asks for the turret's transform |
| `fn_63_1070` | **marked** | branch-free: the guest's `LookAt(position, position + (forward.x, forward.y, forward.z + 0.0), Up)` - the z term is `lbl_63_rodata_8` = 0.0, so the port's `position.GetZ() + 0.f` is retail's; 2425 calls |
| `fn_63_840` | **marked** | branch-free (`lhz 0x19A`): 2304 calls |
| `fn_63_1134` | **marked** | every arm the game's flow can produce ran: the reload timer (61), the "Whole Body" animation block (3), the shot (3), the pitch below zero (60) and above (2420), and the inverted-Y arm (2303, the game's invert-stick option on). The arms left are the two null guards, the "not active" arm (the DOL only calls `ProcessInput` while the turret state is 3) and the `|difference| > 1` clamp (the turn axes are mutually exclusive, so the difference cannot exceed 1) - retail cannot reach them either |
| `fn_63_2D68` | **fixed, not marked** | three behaviour differences fixed (below); the damage-flash arm (0x2E9C) needs a `kSM_Damage` message, and the port's damage path only sends that message to the object that was damaged (`CStateManager::ApplyDamage`), never to the turret script object - the arm is unreachable in the port, and covering it would need a forced `send` |

### 13.4 The three differences fn_63_2D68 had

1. **The copy the guest works on** (0x2DAC): the guest starts from the turret's *own* saved flags
   (the guest's 0x1D4..0x1DC, the copy `fn_63_15B0` took from the base actor at the area load),
   not from the base actor's flags as they are now.
2. **The white flags** (0x2E6C..0x2E94): the flags half (the guest's 0x102) is `(3 & ~3) | 1`, so
   only the depth compare (0x1) is set and the depth update (0x2) is off (`clrrwi` at 0x2E74
   clears the low two bits, `ori` at 0x2E7C sets bit 0). The port set 3.
3. **The flash colour** (0x2F48): the module's own `lbl_63_bss_1C` is `CColor(0.5, 0, 0, 1)`, not
   `CColor::Red()` (`CColor(1, 0, 0, 1)`).

A fourth difference, in the same function's condition (0x2DF4), was the important one: the guest
compares `mgr + 0x24DC` with `MaskUIdNumPlayers(mPlayerId)`. `mgr + 0x24DC` is
`CStateManager::mCurrentRenderPlayerIndex` (`CFrustumPlanes` is 0x64 bytes at 0x2478, so the index
lands on 0x24DC), not the player count: the port's `mgr.GetNumPlayers() == mgr.MaskUIdNumPlayers(...)`
was `1 == 0` in single player, so **the turret's hull light never came on**. With
`GetCurrentRenderPlayerIndex()` the white case runs on every frame the player is in the turret.

### 13.5 Runtime check (2026-10-21)

`WORLD=M06_ShootingGallery,06_ShootingGallery FRAMES=3000 GODMODE=1 MP2_CONSOLE=<port>
tools/run_gameplay.sh` with the console's `tp 0 -27 11` (the player enters the turret),
`press A 20` (the turret fires), `stick 127 0 60`, `stick 0 127 60`, `stick -127 0 60`,
`stick 0 -127 60` (the aim) and `hurt u040d 10` / `hurt u0418 10`. The run's own log:

    [harness] gameplay reached at 12.5 s (frame 489)
    [harness] the frame limit is up at 51.4 s: world 7e19ed26, area 0 of 1
    exit=0

`grep -c '^\[port\] caught'` = 0 and `grep -c reach-stub` = 0. The console's `status` printed
`in-turret 1` and the camera moved around the turret with the stick, so `fn_63_1070`,
`fn_63_F6C`, `fn_63_840`, `fn_63_1134` and `fn_63_2D68` all ran.

An instrumented build of the same inputs (the counters are not in the committed tree) gave the
per-function call counts and the arm coverage of section 13.3: `call_3034 7277`, `call_F6C 4849`,
`call_1070 2425`, `call_2D68 2423`, `call_840 2304`, `call_1134 2303`, `arm_2D68_white 2423`,
`arm_1134_reloadTimer 61`, `arm_1134_animBlock 3`, `arm_1134_fire 3`,
`arm_1134_invertY 2303`, `arm_1134_pitchNeg 60`, `arm_1134_pitchPos 2243`.

The inverted-Y arm needs the game's "invert stick aim Y" option. The port keeps the game's
options in `<user>/settings.cfg` as the `game_options` blob (the bit stream `CGameOptions::PutTo`
writes); the option is bit 87 (byte 10, mask 0x01 - the flags are written after 85 bits: the magic
32, the sound mode 2, the brightness 4, the two offsets 6 each, the stretch 5, the two volumes 7
each and the two alphas 8 each).

asmcheck (`tools/asmcheck.py check ScriptPlayerTurret
src/MetroidPrime/ScriptObjects/PortCScriptPlayerTurret.cpp fn_63_3034 fn_63_1070 fn_63_F6C
fn_63_840 fn_63_1134 fn_63_2D68`) reports no correctness tag; the six findings are completeness
ones: `TCastToPtr<6CActor>` (the port uses the const cast), the two `rstl` string calls (the port
uses a literal) and the `x1d0` compare (the tool tracked the local difference back to
`mYawTarget`; the guest compares the difference of the two analog inputs, which the port clamps).

## 14. The remaining module functions (2026-10-21): the damage arm and the twelve functions

### 14.1 Who sends kSM_Damage to a player turret? Nobody.

The turret's flash arm (the damage case of `fn_63_1BE8`, 0x224C, and the red branch of
`fn_63_2D68`, 0x2E9C) needs a `kSM_Damage` message addressed to the turret. In the shipped game
there is no such sender:

- **The turret is not damageable.** `CActor::HealthInfo()` returns `nullptr` (the DOL's code, the
  upstream's `src/MetroidPrime/CActor.cpp:518`), so the turret has no health info, and
  `CStateManager::ApplyDamage` returns early for an actor without one (`CStateManager.cpp:2941`).
  The console confirms it: `hurt u040d 10` prints no hp line, while the hull (`u0418`) prints
  `hp -5/5`.
- **The hull is damageable but does not forward it.** The hull is a `CScriptActor`
  (`u0418`, at (0, -27, 10)); its `AcceptScriptMsg` has no `kSM_Damage` case
  (`CScriptActor.cpp:129`) and `CEntity::AcceptScriptMsg` only handles
  activate/deactivate/toggle (`CEntity.cpp:43`), so a hit on the hull never reaches the turret.
- **Nothing else sends it.** The only `kSM_Damage` senders in the tree are
  `CStateManager::ApplyDamage` (to the damaged object), `CScriptSafeZoneCrystal` (to itself) and
  two ported modules' relays; the shooting gallery has neither.

So the arm is unreachable in retail as well - it is a relic of the REL. The port keeps retail's
case and says so in a comment at both places (the `kSM_Damage` case of `AcceptScriptMsg` and the
red branch of `PreRender`), and `fn_63_2D68` is marked with the white arm (the one a run reaches)
exercised.

### 14.2 The twelve remaining module functions

| fn | verdict | evidence |
|---|---|---|
| `fn_63_30E8` (the ctor) | **marked** (one difference fixed) | the turret's construction at the area load |
| `fn_63_3070` (the dtor) | **marked** | the world teardown at the end of the run (the host's destructor chain does retail's vtable reset, token and `~CActor`; the pool frees the object) |
| `fn_63_144` (the loader) | **marked** | the area load; the property loop is the upstream's generated `SLdrPlayerTurret.inc`, the same code retail's REL was built from |
| `fn_63_15B0` (the hull actors) | **marked** (three differences fixed) | the area load |
| `fn_63_19B0` (the anim parameters) | **marked** | five calls from `fn_63_15B0` |
| `fn_63_14B4` (the top actor becomes solid) | **marked** | the `kSM_Start` case |
| `fn_63_1460` (the top actor loses solid) | **marked** | `fn_63_1B70` when the player leaves |
| `fn_63_1508` (the muzzle transform) | **marked** | every shot |
| `fn_63_84C` (the shot) | **marked** | `press A` in the turret |
| `fn_63_1B70` (ExitTurret) | **marked** | `press B` in the turret |
| `fn_63_2D68` (PreRender) | **marked** | every frame the player is in the turret (the white arm) |
| `fn_63_1BE8` (AcceptScriptMsg) | **fixed, not marked** | five of the seven cases ran (Create, AreaLoaded, Activate, Start, Deactivate); the `kSM_Stop` case (0x2008) is only sent by the gallery's own script when the round ends - neither the hull's destruction (`hurt u0418 100`, hp -95/5) nor the player's death (`kill`) produces it - and the `kSM_Damage` case is unreachable (14.1) |

### 14.3 The differences this slice fixed

1. **The three animation lookups** (`fn_63_15B0`, 0x1634..0x18E8): only the two getup lookups pass
   a parameter (the guest's `FromEnum(0)` at 0x1670 and `FromEnum(1)` at 0x1704); the step, death
   and locomotion lookups pass **eight empty** parameters (the guest's eight `NoParameter` calls
   at 0x175C..0x18B4). The port passed `FromEnum(3)`, `FromEnum(4)` and `FromEnum(5)`, so the
   three animation ids could be the wrong ones. `BuildAnimParmData` now takes the guest's eight
   slots and the three lookups pass none.
2. **The dead player** (`fn_63_1BE8`, 0x1DEC): when the player that enters has no health left the
   guest returns from the whole function - the rest of the `kSM_Activate` case *and*
   `CActor::AcceptScriptMsg` are skipped. The port ran on and called the base handler.
3. **The weapon damage's sound** (`fn_63_30E8`, 0x3444): after copying the loader's damage info
   the guest overwrites its damage sound id with `0x2812`
   (`SFXsa2_r_hitheavy_00_oneshot`, `include/MetroidPrime/SFX/SamusImpact_MP.h`). The host's
   `CDamageInfo` had no setter for that field; it has one now, under `TARGET_PC`.

### 14.4 Runtime check (2026-10-21)

`WORLD=M06_ShootingGallery,06_ShootingGallery FRAMES=3000 GODMODE=1 MP2_CONSOLE=<port>
tools/run_gameplay.sh` with the console's `tp 0 -27 11` (the player enters the turret), `press A`
(the turret fires), `tp -40 -20 20` and `tp 0 -27 11` again (the trigger deactivates and
activates), `press B` (the player leaves) and `objs`:

    [harness] the frame limit is up at 59.9 s: world 7e19ed26, area 0 of 1
    exit=0

`grep -c '^\[port\] caught'` = 0 and `grep -c reach-stub` = 0. The console's `status` printed
`in-turret 1` three times and `in-turret 0` after the `press B`.

An instrumented build of the same inputs (the counters are not in the committed tree) counted:
`call_144 1`, `call_84C 1`, `call_1460 1`, `call_14B4 1`, `call_1508 1`, `call_15B0 1`,
`call_19B0 5`, `call_1B70 1`, `call_2D68 1350`, `call_3070 1`, `call_30E8 1`, `case_Create 1`,
`case_Activate 1`, `case_Deactivate 1`, `case_Start 1`, `case_AreaLoaded 1`, `arm_white 1286`.
A second run of 12000 updates (the player stays in the turret) counted `call_3070 1` - the
destructor runs when the world is torn down at the end of the run.

asmcheck (`tools/asmcheck.py check ScriptPlayerTurret
src/MetroidPrime/ScriptObjects/PortCScriptPlayerTurret.cpp fn_63_144 fn_63_84C fn_63_1460
fn_63_14B4 fn_63_1508 fn_63_15B0 fn_63_19B0 fn_63_1B70 fn_63_2D68 fn_63_3070 fn_63_30E8`) reports no
correctness tag for any of them. The completeness findings: the loader's `BRANCH`/`CALLS` (the
property loop is the upstream's generated `.inc`, an include the tool cannot parse) and its
`CALL`s (`fn_63_674` is the properties' constructor, `LoadTypedefEditorProperties`,
`ReadFloat`, `LoadTypedefDamageInfo` and `ReadBytes` are the generated loop, `AllocateUniqueId`,
`LdrToDamageInfo`, `LdrToTransform4f` and `LdrToEntityInfo` are the host's names for the guest's),
its `XCALL` (`TranslateSfx` is the port's helper for the guest's inline `-1` test at 0x4A4);
`fn_63_15B0`'s `NoParameter` call (the port's default arguments live in the header) and `CALLS`;
`fn_63_84C`'s `fn_63_B18`/`fn_63_F18` (the host's `CImpactVisorEffect::None()` and the token
copy); `fn_63_3070`'s `Free` (the host's pool frees the object) and `fn_63_30E8`'s `fn_63_374C`
(the host's `CModelData()`).
