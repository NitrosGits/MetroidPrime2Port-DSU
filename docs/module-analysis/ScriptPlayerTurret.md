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
| `fn_63_84C` (the shot) | **fixed, not marked** | `press A` in the turret; the multiplayer sound arm (0xA90) is an unported path (15) |
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

## 15. Every conditional branch of the marked functions (2026-10-21)

The marker means "the body is retail's and ran", so every branch of a marked function has to be
accounted for. The classes are: **(a)** compiler-generated (the null test after `operator new`, an
inlined host helper) - it needs no run; **(b)** a real arm that ran - the once-per-arm probe line
is shown; **(c)** a real arm unreachable in the shipped data - the evidence is in the port's
comment at the branch; **(d)** a real reachable arm that did not run - the marker goes.

Seven functions were checked. One of them, `fn_63_84C`, lost its marker over its multiplayer
sound arm (its row in the table below).

The probe was a temporary once-per-arm flag array in the port (one flag per arm, printed at
exit); it is not in the committed tree. Three runs: the shooting gallery (the turret in use),
the Agon Wastes (`sand_world,06_sand_alamofront` + `layer 3`) and the Sanctuary Fortress
(`cliff_world,01_cliff_entrance` + `layer 6`) - the last two only load the area and the turrets,
so their lines are the loader's and the hull's.

### fn_63_144 (the loader, 35 branches)

| branch | class | evidence |
|---|---|---|
| 0x1A8..0x304: the property dispatch tree (18 `beq` on the property id + 12 `bge` on the binary search) | (a) | the generated property switch - the upstream's `SLdrPlayerTurret.inc` is the same generated code the REL was built from |
| 0x47C/0x480: the property loop | (a) | the generated loop of the same file |
| 0x49C: the allocation returned null | (a) | the null test after `operator new` (the guest allocates 0x230 bytes at 0x494) |
| 0x4A4, 0x4BC, 0x4D8, 0x4F0, 0x50C: the five sound ids are -1 | (c) | all three shipped turrets have real sound ids: the probe printed `[arm] 144_sfx_id_valid` in the shooting gallery, the Agon Wastes and the Sanctuary Fortress, and never `[arm] 144_sfx_id_is_minus_one`; the comment is at the port's `TranslateSfx` |

### fn_63_84C (the shot, 8 branches)

| branch | class | evidence |
|---|---|---|
| 0x89C: the player is missing | (c) | the shot is only fired from `ProcessInput`, which returns early when the player is missing (fn_63_1134's own test at 0x1168); the probe printed `[arm] 84C_player_ok`, never `84C_player_null` |
| 0x8AC: the token is not loaded | (c) | the token is loaded by the `kSM_Create` case at the area load (0x1C9C), before any shot; the probe printed `[arm] 84C_token_loaded`, never `84C_token_not_loaded` |
| 0x8CC: the allocation returned null | (a) | the null test after `operator new` |
| 0x9A0: the impact visor effect's flag | (a) | the flag is the literal 1 the code sets at 0x8EC, so the test is on a constant; the port's `CImpactVisorEffect::None()` carries the same flag |
| 0x9B4: the projectile is missing | (a) | the null test after the placement new |
| 0xA00/0xA10: the base actor or its animation data is missing | (c) | the base actor is the turret's own ATOB connection, resolved at the area load (fn_63_15B0); it resolves in all three shipped turrets; the probe printed `[arm] 84C_base_and_anim_ok`, never `84C_base_or_anim_null` |
| 0xA90: the multiplayer sound | **(d)** | Echoes' multiplayer is local split-screen for 2-4 players on one console and it ships on the disc (the M0x_* worlds, M06_ShootingGallery among them, are multiplayer arenas), so the arm is reachable - but the port cannot start a match: there is no console command and no environment variable for it, the front end's VS flow is what creates one (CScriptGuiScreen's deathmatch/coin switches, `CGameState`'s `CGMDeathMatch`/`CGMCoin`), and the harness skips the front end and sets the single-player mode itself (`PortSkipFrontEnd.cpp:313`), driving pad 0 only.  The arm did not run (the probe printed `[arm] 84C_single_player_sound`, never `84C_multiplayer_sound`), so **`fn_63_84C` is not marked** |

### fn_63_3070 (the destructor, 3 branches)

| branch | class | evidence |
|---|---|---|
| 0x308C: `this` is null | (a) | the compiler's "delete this" test |
| 0x30A0: the token's address is null | (a) | the compiler's placement-new test |
| 0x30C0: the block is not a heap block | (a) | the compiler's free test (`Free__7CMemoryFPCv` needs the block's size) |

### fn_63_15B0 (the hull actors, 2 branches)

| branch | class | evidence |
|---|---|---|
| 0x1614: the base actor is missing | (c) | the ATOB connection resolves in all three shipped turrets; the probe printed `[arm] 15B0_hull_ok` in the shooting gallery, the Agon Wastes and the Sanctuary Fortress, never `15B0_no_hull` |
| 0x1624: the animation data is missing | (c) | the same connection's model carries the animation database in all three shipped turrets (the same probe line) |

### fn_63_14B4 (the top actor becomes solid, 1 branch)

| branch | class | evidence |
|---|---|---|
| 0x14E0: the top actor is missing | (c) | the ATCL connection resolves in the shipped turrets (the gallery's turret rides it); the probe printed `[arm] 14B4_top_actor_ok`, never `14B4_no_top_actor` |

### fn_63_1460 (the top actor loses solid, 1 branch)

| branch | class | evidence |
|---|---|---|
| 0x148C: the top actor is missing | (c) | the same ATCL connection; the probe printed `[arm] 1460_top_actor_ok`, never `1460_no_top_actor` |

### fn_63_1508 (the muzzle transform, 1 branch)

| branch | class | evidence |
|---|---|---|
| 0x1554: the base actor is missing | (c) | the ATOB connection resolves in all three shipped turrets; the probe printed `[arm] 1508_base_actor_ok`, never `1508_no_base_actor` |

### The probe's lines (the shooting gallery run)

    [arm] 144_sfx_id_valid
    [arm] 84C_player_ok
    [arm] 84C_token_loaded
    [arm] 84C_base_and_anim_ok
    [arm] 84C_single_player_sound
    [arm] 15B0_hull_ok
    [arm] 14B4_top_actor_ok
    [arm] 1460_top_actor_ok
    [arm] 1508_base_actor_ok
    [arm] fn_144_entered
    [arm] fn_84C_entered
    [arm] fn_3070_entered
    [arm] fn_15B0_entered
    [arm] fn_14B4_entered
    [arm] fn_1460_entered
    [arm] fn_1508_entered
    [arm] fn_19B0_entered

and in the Agon Wastes and the Sanctuary Fortress runs:

    [arm] 144_sfx_id_valid
    [arm] 15B0_hull_ok
    [arm] fn_144_entered
    [arm] fn_3070_entered
    [arm] fn_15B0_entered
    [arm] fn_19B0_entered

One branch is (d) - `fn_63_84C`'s multiplayer sound - so that function's marker was removed and
the other six markers stand.  The port's multiplayer machinery is there (`CGMDeathMatch`,
`CGMCoin`, the front end's VS screens), but nothing in the port can start a match: the console has
no command for it, there is no environment variable for it, and the harness's boot path sets the
single-player mode itself.  The arm is therefore an unported path, not an unreachable one.

## 16. The function classification (all 59 symbols)

Three classes, applied by rule so the table can be audited.

- **H - host copy** (27 functions, 0x948 bytes): the body is a copy of something the host
  already has - a shared class body, a compiler-emitted destructor, the generated property
  class, inlined DOL code or the REL runtime - so the port writes no body for it.  These are the
  40 rows of section 13.1 minus the 14 rows of section 16.1 (the dead ones and the two table
  entries), which are T.
- **T - dead or table-only** (14 functions, 0x554 bytes): no retail caller.  Twelve are dead in
  retail (nothing calls them and no table names them), and two are table entries the port writes
  as a line in the table (`fn_63_0`, the vtable's `GetHealthInfo` thunk, and `fn_63_3510`, the
  generated owner's destructor).
- **B - module behaviour** (18 functions, 0x2B78 bytes): the port writes a body for it - the
  loader, the constructor and the destructor, the three vtable overrides, the five functions the
  module hands the DOL through `SPlayerTurret_FuncPtrs`, and the module's own helpers.

| class | functions | .text bytes | marked |
|---|---|---|---|
| H host copy | 26 | 0x874 (2,164) | 0 |
| T dead or table-only | 13 | 0x528 (1,320) | 0 |
| B module behaviour | 20 | 0x2C78 (11,384) | 20 |
| **total** | **59** | **0x3A14 (14,868)** | **20** |

(Two functions changed class in the slice of section 20: `fn_63_0` was a table entry (T) and
`fn_63_70` a host copy (H); both are the module's own behaviour (B) now, because the port writes
a body for them.)

### 16.1 The scan behind the classes

The module's 59 functions are the 59 `type:function` rows of
`config/G2ME01/rels/ScriptPlayerTurret/symbols.txt`.  The caller scan reads the module's own
disassembly (`../MetroidPrime2Port/build/G2ME01/ScriptPlayerTurret/asm/`): the 45 functions of
`auto_00_00000050_text.s`, the module head of `auto_00_00000000_text.s`, the static initializer
of `auto_fn_63_376C_text.s`, the linker's `REL/REL_Setup.s` and
`REL/global_destructor_chain.s` and the generated owner file
`MetroidPrime/ScriptObjects/CScriptPlayerTurretObjOwner.s`.  Three passes:

1. **Direct calls.** Every `bl <name>` in the module's `.text` gives the call sites.
2. **Table references.** The module's `.data` (`auto_04_00000000_data.s`) holds the vtable
   (`lbl_63_data_40`) and the owner's table; a function named there (as a symbolic `.4byte
   fn_63_XXXX`) is called *through* that table.
3. **The linker and the module's own stores.** `fn_63_70` stores the loader `fn_63_144` at
   `lbl_63_bss_20`+0 (0x98) and copies the five pointers-to-member into the same table
   (0xA8..0x11C), so the loader and the five PMF functions are reachable although nothing calls
   them with `bl`; `fn_63_376C` registers the global string's destructor `fn_63_1020` with
   `__register_global_object` (0x379C).

A function that no pass names is dead in retail.  The scan is exact for the module: every
function's name appears either in a `bl`, in a table, in the linker's own tables or nowhere.

### 16.2 The 59 functions

| fn | size | class | the scan | marked |
|---|---|---|---|---|
| `fn_63_0` | 0x2C | B | the vtable's slot 15 (`GetHealthInfo`): the module's own thunk, now a body of its own (section 20) | yes |
| `RELExit` | 0x24 | H | the REL linker's exit (the `_epilog` calls it) | - |
| `RELMain` | 0x20 | H | the REL's entry (the `_prolog` calls it) | - |
| `fn_63_70` | 0xD4 | B | the REL's setup (called by `RELMain`): it stores the loader and the five pointers-to-member - the port's `RELMain` is that body (section 20) | yes |
| `fn_63_144` | 0x4CC | B | the loader: `fn_63_70` stores it at `lbl_63_bss_20`+0 (0x98) | yes |
| `fn_63_610` | 0x64 | T | dead: no `bl` and no table names it | no |
| `fn_63_674` | 0xB8 | H | `SLdrEditorProperties`' constructor - the generated property class (the loader calls it at 0x238) | - |
| `fn_63_72C` | 0x114 | T | dead: no `bl` and no table names it (the second property reader) | no |
| `fn_63_840` | 0xC | B | the PMF table `lbl_63_data_34`: the DOL calls it | yes |
| `fn_63_84C` | 0x2CC | B | `fn_63_1134` calls it (0x12C8): the turret's shot | yes |
| `fn_63_B18` | 0x48 | H | `CImpactVisorEffect`'s constructor (a host copy) | - |
| `fn_63_B60` | 0xB4 | H | `CImpactVisorEffect`'s copy constructor (a host copy) | - |
| `fn_63_C14` | 0x40 | H | the `CToken`/`CMemory` copy chain the compiler emitted | - |
| `fn_63_C54` | 0x20 | H | the same chain | - |
| `fn_63_C74` | 0x28 | H | the same chain | - |
| `fn_63_C9C` | 0x80 | H | the same chain | - |
| `fn_63_D1C` | 0x6C | T | dead: no `bl` and no table names it | no |
| `fn_63_D88` | 0x54 | T | dead: no `bl` and no table names it | no |
| `fn_63_DDC` | 0x3C | T | dead: no `bl` and no table names it | no |
| `fn_63_E18` | 0x3C | T | dead: no `bl` and no table names it | no |
| `fn_63_E54` | 0x5C | H | the `CToken`/`CMemory` copy chain | - |
| `fn_63_EB0` | 0x20 | H | the same chain | - |
| `fn_63_ED0` | 0x48 | H | the same chain | - |
| `fn_63_F18` | 0x54 | H | `CImpactVisorEffect`'s destructor (a host copy; `fn_63_84C` calls it at 0x9AC) | - |
| `fn_63_F6C` | 0xB4 | B | the PMF table `lbl_63_data_10`: the DOL calls it | yes |
| `fn_63_1020` | 0x50 | H | the global string's destructor (registered by `fn_63_376C` at 0x379C) | - |
| `fn_63_1070` | 0xC4 | B | the PMF table `lbl_63_data_04`: the DOL calls it | yes |
| `fn_63_1134` | 0x32C | B | the PMF table `lbl_63_data_28`: the DOL calls it | yes |
| `fn_63_1460` | 0x54 | B | `fn_63_1B70` calls it (0x1BC8) | yes |
| `fn_63_14B4` | 0x54 | B | `fn_63_1BE8` calls it (0x1FF0) | yes |
| `fn_63_1508` | 0xA8 | B | `fn_63_84C` calls it (0x870) | yes |
| `fn_63_15B0` | 0x3C4 | B | `fn_63_1BE8` calls it (0x2244) | yes |
| `fn_63_1974` | 0x3C | T | dead: no `bl` and no table names it (a `CMemory` destructor) | no |
| `fn_63_19B0` | 0x184 | B | `fn_63_15B0` calls it five times (the five animation lookups) | yes |
| `fn_63_1B34` | 0x3C | T | dead: no `bl` and no table names it (a `CMemory` destructor) | no |
| `fn_63_1B70` | 0x78 | B | the PMF table `lbl_63_data_1C`: the DOL calls it | yes |
| `fn_63_1BE8` | 0x784 | B | the vtable's slot 6 (`lbl_63_data_40`) | yes |
| `fn_63_236C` | 0x54 | T | dead: no `bl` and no table names it (a `CMemory` destructor) | no |
| `fn_63_23C0` | 0x968 | B | the vtable's slot 5 | yes |
| `fn_63_2D28` | 0x40 | H | `CVector3f::Cross`, the guest's inline (`fn_63_23C0` calls it at 0x2684) | - |
| `fn_63_2D68` | 0x2CC | B | the vtable's slot 9 | yes |
| `fn_63_3034` | 0x3C | B | the vtable's slot 18 | yes |
| `fn_63_3070` | 0x78 | B | the vtable's slot 2 (the destructor) | yes |
| `fn_63_30E8` | 0x3B4 | B | `fn_63_144` calls it (the `rs_new 0x230` at 0x494) | yes |
| `fn_63_349C` | 0x74 | H | `CToken(IObj* null)`, the host's empty token (a host copy; `fn_63_30E8` calls it at 0x3384) | - |
| `fn_63_3510` | 0x90 | T | the generated owner's table (`auto_04_00000000_data.s`): a table entry, no retail caller | no |
| `fn_63_35A0` | 0x78 | T | dead: no `bl` and no table names it (the owner's destructor) | no |
| `fn_63_3618` | 0x90 | H | the generated owner's constructor (`fn_63_349C` calls it) | - |
| `fn_63_36A8` | 0x5C | T | dead: no `bl` and no table names it (the owner's destructor) | no |
| `fn_63_3704` | 0x48 | T | dead: no `bl` and no table names it (the owner's destructor) | no |
| `fn_63_374C` | 0x20 | H | `CModelData`'s default constructor (a host copy; `fn_63_30E8` calls it at 0x3268) | - |
| `fn_63_376C` | 0x90 | H | the module's static initializer (the `.ctors` entry): compiler-generated | - |
| `__destroy_global_chain` | 0x58 | H | the REL linker's global-object registry | - |
| `__register_global_object` | 0x1C | H | the REL linker's global-object registry | - |
| `_unresolved` | 0xC4 | H | the REL linker's "unlinked function called" report | - |
| `_epilog` | 0x24 | H | the REL linker's exit | - |
| `_prolog` | 0x24 | H | the REL linker's entry | - |
| `ModuleDestructors` | 0x4C | H | the REL linker's `_dtors` walker | - |
| `ModuleConstructors` | 0x4C | H | the REL linker's `_ctors` walker | - |

**The unmarked B list** (the B functions the port does not mark): none.  All 20 B functions
carry a `// @ fn_63_<hexoff>` marker in `src/MetroidPrime/ScriptObjects/PortCScriptPlayerTurret.cpp`.
The two that were unmarked when the slice of section 17 started - `fn_63_84C` (the shot) and
`fn_63_1BE8` (the messages) - are marked now; section 17 is their arm table, and section 20 the
slice that marked the module's last two.

### 16.3 The count check

```
$ python3 - <<'PY'
import re, pathlib, collections
sym = pathlib.Path('config/G2ME01/rels/ScriptPlayerTurret/symbols.txt').read_text()
ids = re.findall(r'^(\S+) = \.text:0x[0-9A-Fa-f]+; // type:function', sym, re.M)
print('symbols:', len(ids))
PY
symbols: 59
```

and the class column of the table above counts 26 H + 13 T + 20 B = 59, one row per symbol; the
sizes add up to the module's 14868 text bytes.

## 17. The two unmarked B functions: every branch (2026-10-21)

The marker means "the body is retail's and ran", so every branch of a marked function has to be
accounted for.  The classes are the ones of section 15: **(a)** compiler-generated (the null test
after `operator new`, an inlined host helper) - it needs no run; **(b)** a real arm that ran; **(c)**
a real arm unreachable in the shipped data - the evidence is at the branch; **(d)** a real
reachable arm that did not run - the marker goes; **(forced)** a real arm that only a forced
command or a forced write can reach, labelled with what forced it.

Two tools are used, both without a probe in the game code:

- **the gdb coverage** (`tools/cov/run.sh` with the spec `tools/drivers/playerturret_spec.py`): a
  breakpoint on the port's own line of each arm, so the hit count is the number of times that arm
  ran.  The spec finds the lines by their text, so the table follows the source.  The tool's note
  about optimised code applies (a line with no code of its own is merged into another line, and
  the breakpoint then reports the line it was merged into).
- **the forcing run** (`tools/cov/force.sh` with the same spec's `FORCE` list): at the entry of a
  body the state the guest's test reads is written, so the arm the game's flow never reaches is
  taken.  The three forces are in the spec, each switched by the environment
  (`PT_FORCE_MP`, `PT_FORCE_DEAD`, `PT_FORCE_NOHPI`).  The multi-player one needs the return value
  of `CStateManager::IsMultiplayer()` (the game mode object answers 'SNGL' for good, so there is
  no member to write), and the shared `tools/cov/force.py` only writes members: the module's own
  gdb script `tools/drivers/playerturret_force.py` (the shared one plus a `ret=1` set, which writes
  the return value when the function returns and only when the caller is the module's own code)
  is used with `COV_GDB_SCRIPT=` and `PT_COUNT=1` (it also runs the counting).

The runs are in the Agon Wastes, `sand_world,06_sand_alamofront`, **layer 3** (the player turret
`u061d` is on that layer; the area's layer 3 is reached with the console's `layer 3`), driven by
`tools/drivers/playerturret_drive.py` (a `tools/drivers/playerturret_run.sh` run starts the game
and the driver):

    tools/cov/run.sh tools/drivers/playerturret_spec.py <out> sand_world,06_sand_alamofront \
        --driver 'python3 tools/drivers/playerturret_drive.py {port} <out>/con.out main' \
        --frames 40000 --fast

    PT_FORCE_MP=1 COV_GDB_SCRIPT=tools/cov/cov_force.py \
        tools/cov/force.sh tools/drivers/playerturret_spec.py <out> sand_world,06_sand_alamofront \
        --driver 'python3 tools/drivers/playerturret_drive.py {port} <out>/con.out main' \
        --frames 40000 --fast

The driver's phases: `main` (the message cases the game never sends), `null` (the cases with no
player in the turret) and `stop` (the kSM_Stop case, which needs a player in the turret and kills
it, so it goes last).

### 17.1 fn_63_84C (the shot, 8 branches)

| branch | class | evidence |
|---|---|---|
| 0x89C: the player is missing | (c) | the shot is only fired from ProcessInput, which returns early when the player is missing (fn_63_1134's own test at 0x1168); the run's shot had the player in the turret (the console's `status` printed `in-turret 1`) |
| 0x8AC: the token is not loaded | (c) | the token is loaded by the kSM_Create case at the area load (0x1C9C), before any shot; the same run fired the shot |
| 0x8CC: the allocation returned null | (a) | the null test after the placement new (the guest allocates 0x568 bytes at 0x8C0) |
| 0x9A0: the impact visor effect's flag | (a) | the flag is the literal 1 the code sets at 0x8EC, so the test is on a constant |
| 0x9B4: the projectile is missing | (a) | the null test after the placement new |
| 0xA00/0xA10: the base actor or its animation data is missing | (c) | the base actor is the turret's own ATOB connection, resolved at the area load (fn_63_15B0); the shot's animation ran |
| 0xA90: the multiplayer sound | **forced (`PT_FORCE_MP=1`)** | the port cannot start a match: the harness installs `CGMSinglePlayer` and the console has no game-mode command, and the game mode object answers 'SNGL' for good (`CGMSinglePlayer::GetGameModeType`), so the mode's type cannot be written.  The forcing run makes `CStateManager::IsMultiplayer()` *return* true for the turret's own calls: the module's gdb script writes the return value when the function returns, and only when the caller is the module's own code.  The coverage of that run: the port's multi-player sound line hit **once** and the single-player line **zero** times (the natural run the other way round) |

So `fn_63_84C` is marked: every live arm ran, the one that needed help is labelled *forced*.

### 17.2 fn_63_1BE8 (the messages, 24 branches)

The switch has seven cases and a default.  The counts are from the `main` run (the message cases
the game never sends), the `null` run (the cases with no player) and the `stop` run (the kSM_Stop
case); the line in brackets is the port's line the breakpoint was merged into.

| branch | class | evidence |
|---|---|---|
| 0x1C9C: kSM_Create - the weapon effect | (b) | the area load (the case ran twice: the first load and the layer 3 reload) |
| 0x1CA8: kSM_Create - the multi-player effect | **forced (`PT_FORCE_MP=1`)** | the forcing run's `IsMultiplayer()` returns true while the case asks, so the multi-player effect (0x1E4) was taken |
| 0x1DA4: kSM_Activate - the case | (b) | the trigger (the player standing on the turret) and the forced `send <id> Activate`: the case ran twice |
| 0x1DC0: kSM_Activate - the player is missing | (b) **forced (send Activate)** | the console's `send` has no originator, so the message's +2 is invalid: the case ran twice but the player's `StartTurret` only once - the other run took the null arm |
| 0x1DEC: kSM_Activate - the player is dead | **forced (`PT_FORCE_DEAD=1`)** | the forcing run writes the player's health to -1 while the turret's case reads it (the guest's test at 0x1DE4 is (hp == 0) | (hp < 0); the write is at the entry of `CPlayer::HealthInfo`, so it happens before the case reads the health): the case took the dead-player arm - the port's `StartTurret` line was hit **zero** times in that run (once in the natural run) and the console's `status` printed `in-turret 0` after the player stepped onto the turret, while the natural run printed `in-turret 1` |
| 0x1E64/0x1E78/0x1E80: the base actor, its model or the top actor is missing | (c) | the base and the top actor are the turret's own connections, resolved at the area load (fn_63_15B0); the port's additive animation ran in both entries |
| 0x1F58: kSM_Deactivate - the case | (b) | the forced `send <id> Deactivate` (twice: with and without a player) |
| 0x1F74: kSM_Deactivate - the player is missing | (b) **forced (send Deactivate)** | the case ran twice but the player's `SetTurretState(kTS_None)` only once - the other run took the null arm |
| 0x1FB0: kSM_Start - the turret is not active | (b) **forced (send Start after send Deactivate)** | the `main` run sent `Start` twice (both answers are in the transcript) and the case's body ran once: the second `Start` came after the `send Deactivate` (which deactivates the turret) and took the early-out |
| 0x1FD8: kSM_Start - the player is missing | (b) **forced (send Start)** | the `null` run: the case ran and the body (`AddSolidToTopActor` and `AddMaterial`) hit once each, but the player's `SetTurretState` zero times - the null arm |
| 0x2008: kSM_Stop - the turret is not active | (b) **forced (send Stop after send Deactivate)** | the `main` run: the Stop case was entered but nothing after the test ran (the model's flags, the kill, the animation and the deactivation all zero hits) - the early-out |
| 0x203C: kSM_Stop - the base actor is missing | (c) | the base actor is the turret's own ATOB connection, resolved at the area load |
| 0x2068/0x2084: kSM_Stop - the player is missing | (b) **forced (send Stop)** | the `null` run: the case ran (the model's flags hit once) but the player's ejection and the kill (both merged into the kill's line) zero times - the null arm |
| 0x209C/0x20C4: kSM_Stop - the player is ejected and killed | (b) **forced (send Stop with the player in the turret)** | the `stop` run: the message was answered, the console's `status` printed `in-turret 0` afterwards (the ejection at 0x209C), the console's `objs turret` printed `inactive` (the deactivation at 0x2228) and the game stopped updating right after the message (the harness's "no game update for 30.0 s": the death's save prompt - the kill at 0x20C4) |
| 0x2124/0x2130/0x2138: the base and the top actor are missing | (c) | the same two connections; the `stop` run's animation hit once - the block ran |
| 0x224C: kSM_Damage - the case | (b) **forced (send Damage)** | the forced `send <id> Damage` (the flash timer hit once) |
| 0x225C: kSM_Damage - the multi-player sound | **forced (`PT_FORCE_MP=1`)** | the forcing run: the port's multi-player impact sound hit and the single-player emitter not; in the natural run the opposite |
| 0x2264: kSM_Damage - the single-player sound | (b) **forced (send Damage)** | the natural run: the multi-player sound hit zero times while the case ran, so the single-player emitter (the case's else branch) was taken |
| 0x2284: kSM_Damage - the player is missing | (b) **forced (send Damage after send Deactivate)** | the forced run sent a second `send Damage` after the `send Deactivate` (the player had left the turret by then) and the multi-player sound was hit only once - by the first message, the one with the player in the turret |
| 0x22E8/0x2308: kSM_Damage - the damaged actor is missing | (b) **forced (send Damage)** | the console's `send` has no sender, so the message's +0 is invalid: the case ran and the sender's test was taken with a null actor |
| 0x231C/0x2320: kSM_Damage - the actor has no health info | **forced (`PT_FORCE_NOHPI=1`)** | the forcing run writes the message's sender to the turret itself, whose `HealthInfo()` is null (`CActor::HealthInfo` returns nullptr), so the case took the branch that reads the message's originator: the port's sender test was taken with a **non-null** actor (the line's hit count is the two messages), and the only actor it can be is the turret |
| 0x1C4C/0x1C5C/0x1C84/0x1C98: the default case | (b) **forced (send Open)** | `Open` is a message the turret's switch does not have (the guest's comparison tree covers six codes), so the forced `send <id> Open` took the default and fell through to CActor's handler |

So `fn_63_1BE8` is marked: every live arm ran or was forced and labelled.

## 18. The differences this slice fixed (2026-10-21)

1. **The projectile's three floats** (`fn_63_84C`, 0x924/0x934/0x948): the guest loads
   `lbl_63_rodata_10` = 1.0 into f1, `lbl_63_rodata_14` = 4.0 into f2 and copies f2 into f3
   (`fmr f3, f2`).  The three floats are the constructor's `chargeFactor`, `closeImpactDistance`
   and `impactScaleDistance` - the DOL's own `CEnergyProjectile` constructor stores f1 into the
   charge factor (0x430), f2 into the close impact distance (0x558) and f3 into the impact scale
   distance (0x55C) (`main.elf`, 0x8016AC68) - so the port's `4.f, 1.f, 4.f` had the first two
   swapped and is now `1.f, 4.f, 4.f` (the sibling `CAIMannedTurret`'s port already had that).
2. **The kSM_Stop case's pitch reset** (`fn_63_1BE8`, 0x2128/0x212C): the guest resets the pitch
   and the pitch target *before* it tests the base and the top actor, so it happens even without a
   base actor.  The port did it inside the base actor's test; it is now where the guest has it.
3. **The kill amount** (`fn_63_1BE8`, 0x20B0): the guest passes the float at +0x14 of the player's
   `CHealthInfo` - the *bits of the last damage's weapon mode*, read as a float (the guest's
   `lfs f1, 0x14(r5)`; the guest's `CHealthInfo` layout is 0x00 initial HP, 0x04 HP, 0x08 knockback
   resistance, 0x0C the cause-of-death weapon, 0x10/0x12 the death owner and source, 0x14 the last
   damage's weapon, 0x18/0x1A the last damage's owner and source, 0x1C the flags byte - the DOL's
   copy constructor at `main.elf` 0x80070D60).  The port passed the player's current health, which
   can be negative when the player dies, and `CStateManager::KillPlayer` branches on the amount
   (`CStateManager.cpp:3016`: `if (previousHealth >= 0.f)`), so the port could skip
   `CGameMode::OnPlayerKilled`.  The port now passes the guest's value: `CHealthInfo`'s
   `GetLastDamageWeaponAsFloat()` (in the header's `TARGET_PC` block, next to
   `GetLastDamageOwner`) rebuilds the guest's word - the weapon type in the high half, the three
   flags below it, because the guest's `CWeaponMode` packs its bit fields from the top (PowerPC is
   big-endian) - and reads it as a float.
4. **The pitch clamp** (`fn_63_23C0`, 0x28DC..0x290C): the guest clamps the pitch into
   [-mMaxVertElevationDown, mMaxVertElevationUp] with its own compares (0x28EC and 0x28F8); the
   port called the shared `CMath::Clamp`, which hides them from `tools/asmcheck.py` (the tool
   matched the C++'s next compare - the `mPitch >= 0.f` test of 0x2938 - to the clamp's second
   compare and reported a CROP).  The clamp is now written out, as the guest writes it.
5. **The multi-player hook is gone**: the first version of this slice carried an `MP2_TURRET_MP`
   getenv in the module to reach the two multi-player arms.  Game code should not carry a test-only
   arm, so the hook is out and the arms are forced with `tools/cov/force.sh` instead (section 17).

## 19. The runtime check (2026-10-21)

The Agon Wastes runs: `WORLD=sand_world,06_sand_alamofront` with the console's `layer 3` (the
player turret `u061d` is on that layer), driven by `tools/drivers/playerturret_drive.py`:

- the player is teleported onto the turret (186.6, -112.1, 18.8) and the console's `status`
  prints `in-turret 1`; the turret fires with A (the `send Start` before it puts the player's
  turret state to `kTS_Active`, which the guest's input path requires);
- the console's `send <id> <message>` delivers the messages the game never sends: `Activate`
  (the null-player arm), `Open` (the default case), `Damage` (the damage case), `Deactivate`
  (the player leaves, the turret is deactivated), `Start` (the not-active arm) and `Stop` (the
  case with the player in it, which ejects and kills the player);
- the `null` phase runs the cases with no player in the turret, the `stop` phase the kSM_Stop case
  and the `forced` phase the message cases without the kSM_Stop one (the death's prompt stops the
  game's update loop, so a forced run leaves that case out).

**The gdb coverage** (`tools/cov/run.sh` with the spec `tools/drivers/playerturret_spec.py`) and
the **forcing runs** (`tools/cov/force.sh` with the same spec's `FORCE` list and the module's
`tools/drivers/playerturret_force.py` as `COV_GDB_SCRIPT`) gave the per-arm counts of section 17;
the **compiler coverage** (`-DMP_PORT_COVERAGE=ON` and `tools/cov/gcov_report.py`) the branch
coverage of the module's bodies.

- the `main` run: the single-player sound line hit once and the multi-player line not, the
  `kSM_Activate` case twice with the player's `StartTurret` once (the null arm), the
  `kSM_Deactivate` case twice with the player's `SetTurretState(kTS_None)` once (the null arm),
  the `kSM_Start` case's body once (the second `Start` took the not-active arm) and the
  `kSM_Stop` case's body not at all (the not-active arm);
- the `stop` run: the kSM_Stop case's model flags, the ejection, the kill, the animation and the
  deactivation each once - the case's own coverage table, the one the first version of this slice
  lost (the death's prompt stops the game's update loop, so the run has to be ended by the
  harness' timeout);
- the forcing runs: `PT_FORCE_MP=1` (the multi-player sound line hit once, the single-player line
  not; the damage message's multi-player sound line hit once), `PT_FORCE_DEAD=1` (the player's
  `StartTurret` line not hit at all: the case returned on the dead player) and `PT_FORCE_NOHPI=1`
  (the message's sender is the turret, whose health info is null) - the two arms the game's flow
  never reaches.

The compiler coverage (`-DMP_PORT_COVERAGE=ON`, `tools/cov/gcov_report.py <build> <gcov dir>
--module ScriptPlayerTurret`) agrees with the gdb counts on the arms the runs reached; its lines
whose branch was never taken are the null guards and the arms of the other runs.

Every run's `caught/reach-stub` count is 0.

## 20. The slice of 2026-10-21 (evening): the module's last two functions

The module's B functions were all marked by section 17; this slice took the next unmarked ones in
offset order and checked each against the asm.  Six functions: `fn_63_0` (0x0), `RELExit` (0x2C),
`RELMain` (0x50), `fn_63_70` (0x70), `fn_63_610` (0x610) and `fn_63_674` (0x674).

| fn | size | class | the asm | the port | marked |
|---|---|---|---|---|---|
| `fn_63_0` | 0x2C | B | the vtable's slot 15: it loads the vtable (0x0C), takes its slot 12 - the base's `HealthInfo` (0x10) and calls it through the count register (0x18).  No branch | the port had no body: the host's `CActor::GetHealthInfo` is the same virtual call.  It has one now - a forwarder to the same virtual call, so the port's vtable has the module's own entry like the guest's | yes |
| `RELExit` | 0x24 | H | the linker's exit: the DOL's `SetLoader_PlayerTurret` with 0 | the port's `RELExit` calls the DOL's proxy `SetSPlayerTurret_FuncPtrs(nullptr)`, which drops the table - the same call | - |
| `RELMain` | 0x20 | H | the linker's entry: it calls `fn_63_70` (0x5C) and returns | the port's `RELMain` *is* `fn_63_70`'s body (the linker calls the port's `RELMain`), so the marker below names `fn_63_70` | - (see `fn_63_70`) |
| `fn_63_70` | 0xD4 | B | the module's setup: the loader at `lbl_63_bss_20`+0 (0xB4), the five pointers-to-member tables (`lbl_63_data_4`, `_10`, `_1C`, `_28`, `_34`) copied to `lbl_63_bss_20`+4 .. +0x3C (0xF0..0x12C) and the DOL's `SetLoader_PlayerTurret` called (0x12C) | the port's `RELMain` fills the same table with the loader and the five member functions and hands it to the DOL's `SetSPlayerTurret_FuncPtrs` | yes |
| `fn_63_610` | 0x64 | T | `SLdrEditorProperties`' destructor: the `SLdrDamageInfo` member's destructor (0x638), the base's (0x644) and the block's free when the delete flag is positive (`extsh. r0, r31` + `ble` at 0x648/0x64C) | the port uses the upstream's generated `SLdrEditorProperties` - the same generated code the guest's REL was built from (section 14.3) - so the port writes no body | - |
| `fn_63_674` | 0xB8 | H | `SLdrEditorProperties`' constructor, called by the loader at 0x238 | the same: the upstream's generated class | - |

**The decode rule** (the review's): `fn_63_610`'s free is guarded by `extsh. r0, r31` + `ble` -
the signed compare skips the free when the flag is zero or negative, so the *free* needs the flag
**positive**; and the base class's call runs *after* the members' (0x638 then 0x644), i.e. the base
runs last, not only when the event was not handled (that is the DOL's `CEntity::AcceptScriptMsg`
rule, not this one's).

**The runs** (the gdb coverage, `tools/cov/run.sh` and `tools/cov/force.sh` with the spec; the
Agon Wastes, `sand_world,06_sand_alamofront` layer 3):

- the natural run: `fn_63_70` hit **once** (the module's setup runs when the area loads; the
  compiler coverage of the same run: `RELMain() 9/9 lines, 0/0 branches` - the body has no branch
  at all) and `fn_63_0` **zero** times (nothing asks a turret for its health info in that flow);
- the forced run (`PT_FORCE_NOHPI=1`, the message's sender is the turret): `fn_63_0` hit **twice** -
  the kSM_Damage case calls the sender's `GetHealthInfo` twice, and the sender is the turret, so
  the module's own thunk is the one that runs (and `fn_63_70` once again).

Both bodies are branch-free, so both are ALL-ARMS.
