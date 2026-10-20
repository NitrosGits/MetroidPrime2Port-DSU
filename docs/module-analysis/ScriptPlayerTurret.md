# ScriptPlayerTurret REL (module 63): analysis for a host body

Analysis + host body, milestones 1-3 (2026-10-20). Sources: the archive build's disassembly
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
| `fn_63_84C` | 0x184 | the turret's **shot**: the energy projectile, spawned at the muzzle (the `muzzleleen_LCTR` locator) | [proven] call at 0x000012C8 |
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
| 0x1B8, 0x1BC, 0x1C0 | three floats zeroed at birth | [proven] |
| 0x1C4..0x1CC | the editor transform's forward vector (m01, m11, m21) | [proven] the loads from the transform |
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
(0x19C, 0x1A0), moves the top actor (0x19A) onto the base actor's `"upwardballsnapp_LCTR"` locator,
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
| 0x07 | `upwardballsnapp_LCTR` | `fn_63_F6C`, `fn_63_23C0`, `fn_63_1BE8` - the base actor's locator |
| 0x1B | `Whole Body` | `fn_63_1134` - the animation the input path watches |
| 0x26 | `muzzleleen_LCTR` | unreferenced in this milestone |

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
  (0x19A) goes onto the base actor's `"upwardballsnapp_LCTR"` locator (0x1E88..0x1F3C).
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
- The guest's `CGameState` byte 0xA4 bit 0x40 (`extrwi. r0,r0,1,25`) is the game's **invert Y**
  option: it swaps the two stick axes of the turret's pitch (`CGameOptions::GetInvertYAxis`).
  [probable]: the bit's position in the options' bitfield byte is the same as the host's.

## 9. Milestone 2 (the message switch) - host body and runtime

`AcceptScriptMsg` (fn_63_1BE8) now has retail's switch with **all seven cases ported**;
`fn_63_14B4` (0x54) and `fn_63_1460` (0x54) are ported as named methods, and `ProcessInput`
(fn_63_1134, 0x32C) is ported. Of `fn_63_15B0` (0x3C4) only the two `FindConnectedObject` calls and
the base actor's model flags are ported - the animation lookup (0x2B4 of it) stays a stand-in -
because the DOL's turret HUD (`CTurretHud::UpdateEnergy`) reads the hull actor's id and
dereferences the actor it names: without the connections the run **crashes** (SIGSEGV in
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

**Milestone 3 (next).** `fn_63_23C0` (Think, 0x968 - parked for an Opus lane) is what makes the
turret aim and move the hull; then `fn_63_2D68` (PreRender, 0x2CC) and the rest of `fn_63_15B0`
(the five animation ids), and `fn_63_84C` (0x184) for the shot itself.
