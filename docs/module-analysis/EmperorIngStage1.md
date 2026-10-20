# EmperorIngStage1 REL (module 16): analysis for a host body

Sources: the archive build's disassembly `../MetroidPrime2Port/build/G2ME01/EmperorIngStage1/asm/`
(`auto_00_00000000_text.s`, `auto_00_000046D4_text.s`, `auto_00_0000A2A0_text.s`,
`auto_00_0000BA30_text.s`, `auto_00_0000C2A4_text.s`, `auto_fn_16_BF88_text.s`,
`auto_03_00000000_rodata.s`, `auto_04_00000000_data.s`, `auto_05_00000000_bss.s`) and the DOL
symbol names it calls. `fn_16_X` are REL `.text` offsets; every offset below was read from an
instruction. Confidence: **[proven]** = read from an instruction or a table entry;
**[?]** = not yet established.

The module is the Emperor Ing's first stage (the boss body with the tentacles and the heart).
`docs/modules.tsv` puts it in Great Temple / 07_temple_mothlordsanctuary_dark, layer 2
(`docs/module_spawns.tsv`); the Stage 2 tentacle is layer 3 and the Stage 3 body layer 4.

## 1. Function map

| fn | size | role | tag |
|---|---|---|---|
| `RELMain` | 0x20 | calls `fn_16_A270` | [proven] |
| `fn_16_A270` | 0x30 | stores the loader `fn_16_A2A0` into `lbl_16_bss_0` (the module's `SEmperorIngStage1_FuncPtrs`) and calls the DOL's `SetSEmperorIngStage1_FuncPtrs` | [proven] |
| `RELExit` | 0x24 | clears the registration (`fn_8022756C` with null) | [proven] |
| `fn_16_A2A0` | 0x31C | **loader** (section 3) | [proven] |
| `fn_16_A5BC` | 0x40 | moves the loader's `optional<CModelData>` into the object's `CModelData` | [proven] |
| `fn_16_ABD0` | 0x30C | reads the script data block (the 18 properties of `SLdrEmperorIngStage1Data`) | [proven] |
| `fn_16_900C` | 0x710 | **constructor** (section 2) | [proven] |
| `fn_16_8B80` | 0x1C0 | **destructor** (vtable slot 0): destroys the tokens, the beam info, the audio parms, the animation set | [proven] |
| `fn_16_8AF4` | 0xC4 | `SetupStateMachine` (vtable slot 58): installs the three FSM tables | [proven] |
| `fn_16_56D8` | 0x250 | `Think` (vtable slot 3): the timers, the effect state and the emitter that follows the target | [proven] |
| `fn_16_3668` | 0x80 | every live tentacle's per-frame update | [proven] |
| `fn_16_2B0C` | 0x50 | the per-tentacle update (it calls three more module functions) | [proven] |
| `fn_16_51D4` | 0x128 | the module's own sound update | [proven] |
| `fn_16_4EB8` | 0x31C | moves the actor onto the target's locator | [probable] |
| `fn_16_3AFC` | 0x74 | the three collision actor managers' update | [proven] |
| `fn_16_4C0C` | 0xBC | the module's track-player update | [probable] |
| `fn_16_4C8` | 0x368 | aims the actor at the player | [probable] |
| `fn_16_8C` | 0x11C4 | the tentacles' hit test and damage | [probable] |
| `fn_16_4C0C` | 0xBC | the track-player update: the "Damage" and "Telegraph" effect states | [probable] |
| `fn_16_51D4` | 0x128 | the module's own sound update (it reads a float at the damage vulnerability's +4) | [probable] |
| `fn_16_2B0C` | 0x50 | the per-tentacle update | [proven] |
| `fn_16_24AC` | 0x78 | the tentacle's dark and light vulnerability effect states | [proven] |
| `fn_16_2070` | 0x194 | the effect state a tentacle carries | [probable] |
| `fn_16_26B8` | 0x3B0 | the tentacle's own update | [?] |
| `fn_16_2524` | 0x194 | the tentacle's motion update | [?] |
| `fn_16_302C` | 0x34 | the actor the module tracks (the id at guest 0xA1A) | [proven] |
| `fn_16_2FA8` | 0x50 | the collision actor of the second tentacle's collision actor manager | [proven] |
| `fn_16_4E44` | 0x24 | takes the particle effect's reference | [probable] |
| `fn_16_BDD0` | 0x64 | releases that reference | [probable] |
| `fn_16_6D10` | 0x84 | the particle effect's own update | [probable] |
| `fn_16_7CE8` | 0x1C0 | `AcceptScriptMsg` (vtable slot 4): the message switch | [proven] |
| `fn_16_3B70` | 0x54 | destroys the three collision actor managers | [proven] |
| `fn_16_30C8` | 0x278 | builds the tentacle animation parameter sets | [probable] |
| `fn_16_3EAC` | 0x794 | creates the three collision actor managers and their tentacle actors | [probable] |
| `fn_16_36E8` | 0x29C | builds the tentacle joint collision descriptions | [probable] |
| `fn_16_56D8` | 0xC4 | `Think` (vtable slot 3) | [proven] |
| `fn_16_7CE8` | 0xC4 | `AcceptScriptMsg` (vtable slot 4) | [proven] |
| `fn_16_52FC` | 0x20 | `PreRender` (vtable slot 7): the base's own body | [proven] |
| `fn_16_531C` | 0x2B8 | `Render` (vtable slot 9) | [proven] |
| `fn_16_7F8C` | 0x2C4 | `DoUserAnimEvent` (vtable slot 26) | [proven] |
| `fn_16_7EA8` | 0xC4 | `ProcessSoundEvent` (vtable slot 28) | [proven] |
| `fn_16_B958` | 0x30 | `GetTouchBounds` (vtable slot 16): the actor's bounding box | [proven] |
| `fn_16_BA30` | 0x30 | builds the `optional<CAABox>` from the bounding box | [proven] |
| `fn_16_9AB0` | 0xC4 | copies the `SLdrHealthInfo` into the actor's `CHealthInfo` (0x1E bytes) | [proven] |
| `fn_16_98D0` | 0xC4 | copies an `SLdrDamageVulnerability` into a `CDamageVulnerability` (0x54 bytes) | [proven] |
| `fn_16_9700` | 0xC4 | builds the shock-wave block at guest 0xC40 from the taunt attack's `SLdrShockWaveInfo` | [proven] |
| `fn_16_9610` | 0x94 | copies the `SLdrPlasmaBeamInfo` (0x48 bytes) into guest 0xCA8 | [proven] |
| `fn_16_8C` | 0x3B0 | tentacle hit/damage helper (accumulated bounds, `ApplyDamage`) | [?] |
| `fn_16_830` | 0x2C | sends a state message (`IS0`/`IS1`/`IS2` tags) to the actor | [?] |
| `fn_16_920` | 0xB0 | reads the "brainSpine_scale" locator of the model | [?] |
| `fn_16_8E4` | 0x10 | picks a random tentacle retraction time | [?] |
| `fn_16_1DFC` | 0x24 | resets one tentacle slot (the timer, the state) | [proven] |
| `fn_16_2F70` | 0x38 | the number of tentacle slots the sub-stage uses (4, 6 or 8) | [proven] |
| `fn_16_2EE0` | 0x90 | sets the 0x80 flag of the live tentacles' slots | [proven] |

### The accessors (EmperorIngStage1Accessors.s, one-line bodies)

| fn | vtable slot | method | body |
|---|---|---|---|
| `fn_16_B994` | 39 | `TakeDamage` | `mDamageCooldownTimer (0x448) = skDamageHitTime` |
| `fn_16_B9FC` | 40 | `CanBeShot` | `li r3, 0x1` (true) |
| `fn_16_BA04` | 41 | `IsListening` | `li r3, 0x0` (false) |
| `fn_16_BA0C` | 42 | `Listen` | `li r3, 0x0` (false) |
| `fn_16_BA14` | 43 | `GetOrigin` | copies the three floats at 0x54/0x58/0x5C |
| `fn_16_B9A4` | 48 | `GetModelAlphau8` | `lbz 0x44F` |
| `fn_16_B9AC` | 53 | `ProjectileInfo` | null |
| `fn_16_B9B4` | 54 | `GetSearchPath` | null |
| `fn_16_B9BC` | 55 | `GetPointSearchPath` | null |
| `fn_16_B9C4` | 59 | `GetRagDoll` | null |
| `fn_16_B9CC` | 65 | `GetAttackTarget` | `kInvalidUniqueId` |
| `fn_16_B9DC` | 66 | `IsOnGround` | bit 3 of the byte at 0x34C |
| `fn_16_B9E8` | 67 | `GetGravityConstant` | the DOL's float `lbl_8041B758` |
| `fn_16_B9F4` | 71 | `GetDeathExplosionParticle` | `this + 0x754` (a base member) |
| `fn_16_60` | 13 | `GetHealthInfo` | calls vtable slot 12 (`HealthInfo`) |

## 2. The class

`CEmperorIngStage1 : CPatterned`, retail size **0xD18** (the loader's `rs_new(0xD18)` at
0x0000A4B8), vtable **`lbl_16_data_3C8`** (82 words: two leading zeros, then 80 entries; slot 0 is
the destructor). CPatterned is 0x7C0, so the derived members live at 0x7C0 and above.

### Constructor (fn_16_900C, 0x0000900C)

Base call: `EPatternedAI 0x0D` (`li r4, 0xd`, 0x00009068), flavor 0 (`li r7, 0x0`, 0x0000905C),
movement 1 = `kMT_Flyer`, collider 1 = `kCT_One`, body 2 = `kBT_Restricted` (the three stack
constants at 0x00009060..0x00009070). The patterned info is the stack slot 0 at 0x00009054.

| at | what |
|---|---|
| 0x0000907C | installs the vtable `lbl_16_data_3C8` |
| 0x0000909C | clears 0x7C0..0x7D4 (six ints) |
| 0x000090BC | 0xA18..0xA1E = `kInvalidUniqueId` (four u16) |
| 0x000090DC | 0xA20..0xA28 = -1 (three ints) |
| 0x000090E8 | 0xA2C, 0xA30 = 0.0f (`rodata_380`) |
| 0x000090F0 | 0xA34 = data+0x698 (maxTurnSpeedNormal) |
| 0x00009100 | calls vtable slot 13 (`GetHealthInfo`) and stores the first float of the result at 0xA40 (the initial health) |
| 0x00009154 | 0xA4C = 4.0f (`rodata_38C`) |
| 0x00009178 | 0xA74: five bits cleared (`rlwimi` with 0 at bits 24..28) |
| 0x000091C8 | 0xA90, 0xA98 = `kInvalidUniqueId` |
| 0x000091E0 | 0xAA0 = the zero vector |
| 0x000091F8 | 0xAAC = the `CHealthInfo` (fn_16_9AB0 from data+0x000) |
| 0x00009208 | 0xACC = the normal vulnerability (data+0x008) |
| 0x00009250 | 0xAFC = the warp vulnerability (data+0x164) |
| 0x00009274 | 0xB2C = the melee vulnerability (data+0x2C0) |
| 0x00009298 | 0xB5C = the projectile vulnerability (data+0x41C) |
| 0x000092A8 | 0xB8C = the melee damage (data+0x580) |
| 0x00009304 | 0xBA8..0xBB0 = data+0x670/0x674/0x678 |
| 0x0000931C | 0xBB4 = data+0x578 (stayRetractedTime) |
| 0x00009324 | 0xBB8 = the `CToken` built from the tag "WPSC" and data+0x590 (the projectile) |
| 0x00009370 | 0xBC4 = the beam damage (data+0x594 is the projectile's damage; the beam's own is data+0x65C) |
| 0x000093DC | 0xBE0 = the `CToken` built from the tag "PART" and data+0x5A4 (the projectile's visor effect), then locked |
| 0x00009460 | 0xBF0..0xBF8 = data+0x698/0x69C/0x6A0, 0xBFC = data+0x694 |
| 0x00009488 | 0xC00..0xC0C = data+0x5A8..0x5B4 (the warp portal effect and the attack tip's animation) |
| 0x000094A8 | 0xC10 = the warp stab damage (data+0x5B8) |
| 0x00009518 | 0xC2C = data+0x57C, 0xC2E = data+0x5D0, 0xC30 = data+0x5D4 |
| 0x00009530 | 0xC34 = data+0x6A8 (tauntFrequency) |
| 0x00009538 | 0xC38 = data+0x5D8 (the taunt loop duration) |
| 0x00009540 | 0xC3C = data+0x5DC (destroyPercentage) as a float divided by 100.0f |
| 0x0000955C | 0xC40 = the shock-wave block (fn_16_9700 from data+0x5E0) |
| 0x00009560 | 0xC7C = data+0x610 (the shock-wave travel sound) |
| 0x00009574 | 0xC80 = the `CProjectileInfo` (data+0x618 and the beam damage) |
| 0x00009584 | 0xCA8 = the `SLdrPlasmaBeamInfo` (data+0x614) |
| 0x00009590 | 0xCF0 = data+0x66C (the beam sound) |
| 0x000095A0 | 0xCF4, 0xCF8 = data+0x6AC/0x6B0 (the attack interval) |
| 0x000095B4 | 0xCFC = data+0x67C, 0xD00..0xD10 = data+0x680..0x690 (the heart damage sound) |
| 0x000095E0 | `SetDrawShadow(this, 0)` |
| 0x000095EC | `EnableAllAnimReactions(&this->0x620, 0)` - the port's `CAiKnockBackMgr` has no such call yet |

### Member map (guest offsets, all [proven] from the constructor above)

| off | member | value |
|---|---|---|
| 0x7C0-0x7D4 | six ints | 0 (0x7C0 the mode, 0x7C4 the sub-stage, 0x7C8/0x7CC/0x7D0 the tentacle collision actor managers, 0x7D4 the live tentacle count) |
| 0xA18-0xA1E | four u16 | `kInvalidUniqueId` |
| 0xA20-0xA28 | three ints | -1 |
| 0xA2C, 0xA30, 0xA38, 0xA3C, 0xA44, 0xA48, 0xA50, 0xA58-0xA6C | floats | 0 (0xA3C the heart-stage timer, 0xA50 the taunt timer) |
| 0xA34 | float | maxTurnSpeedNormal |
| 0xA40 | float | the initial health (from `GetHealthInfo()`) |
| 0xA4C | float | 4.0f |
| 0xA70 | int | 0 |
| 0xA74 | byte | five bits cleared |
| 0xA78 | float | 0 |
| 0xA80-0xA8C | four ints | 0 (0xA8C the sound emitter's handle) |
| 0xA18, 0xA1A, 0xA1C | u16 | `kInvalidUniqueId` (the three connected actors: IS3/ACTIVATE, CONNECT/0x41574348 and CONNECT/ACTIVATE) |
| 0xA58, 0xA5C | floats | 0 (the "Damage" and "Telegraph" effect timers) |
| 0xA90, 0xA98 | u16 | `kInvalidUniqueId` (0xA90 the emitter's target) |
| 0xA94, 0xA9C | ints | 0 |
| 0xAA0 | CVector3f | zero |
| 0xAAC | CHealthInfo | the tentacle's health |
| 0xACC / 0xAFC / 0xB2C / 0xB5C | CDamageVulnerability | normal / warp / melee / projectile |
| 0xB8C | CDamageInfo | the melee damage |
| 0xBA8-0xBB0 | three floats | heartExposedTime, unknown_0x905938b8, unknown_0xb826317a |
| 0xBB4 | float | stayRetractedTime |
| 0xBB8, 0xBC0 | CToken + int | the projectile token ("WPSC") |
| 0xBC4 | CDamageInfo | the projectile's damage |
| 0xBE0, 0xBE8, 0xBEC | CToken + int + byte | the projectile's visor effect token ("PART") |
| 0xBF0-0xBF8 | three floats | maxTurnSpeedNormal, maxTurnSpeedMelee, unknown_0xe5a7c358 |
| 0xBFC | float | turnSpeedAccel |
| 0xC00-0xC0C | four ints | the warp portal effect + the attack tip's animation |
| 0xC10 | CDamageInfo | the warp stab damage |
| 0xC2C, 0xC2E, 0xC30 | u16 | the tentacle damaged sound, the two warp sounds |
| 0xC34 | float | tauntFrequency |
| 0xC38 | float | the taunt loop duration |
| 0xC3C | float | destroyPercentage / 100.0f |
| 0xC40 | 0x3C bytes | the shock-wave block |
| 0xC7C | u16 | the shock-wave travel sound |
| 0xC80 | CProjectileInfo | 0x28 bytes |
| 0xCA8 | SLdrPlasmaBeamInfo | 0x48 bytes |
| 0xCF0 | u16 | the beam sound |
| 0xCF4, 0xCF8 | floats | attackIntervalMin, attackIntervalMax |
| 0xCFC-0xD10 | the heart damage sound | 0x18 bytes |
| 0xD18 | end | |

### The script data block (guest offsets inside `SLdrEmperorIngStage1Data`)

| off | field |
|---|---|
| 0x000 | tentacle (0x578): health 0x000, normalVulnerability 0x008, warpAttackVulnerability 0x164, meleeAttackVulnerability 0x2C0, projectileAttackVulnerability 0x41C, stayRetractedTime 0x578, tentacleDamagedSound 0x57C |
| 0x580 | melee (0x10): damage |
| 0x590 | projectile (0x18): projectile 0x590, projectileDamage 0x594, projectileVisorEffect 0x5A4 |
| 0x5A8 | warp (0x30): portalEffect 0x5A8, attackTip 0x5AC, stabDamage 0x5B8, unknown_0xecfab026 0x5C8, unknown_0x94880277 0x5CC, sound 0x5D0, sound_0xa93f0198 0x5D4 |
| 0x5D8 | tauntAttack (0x3C): loopDuration 0x5D8, destroyPercentage 0x5DC, shockWaveInfo 0x5E0, shockWaveTravelSound 0x610 |
| 0x614 | beamAttack (0x5C): beamInfo 0x614, damage 0x65C, sound 0x66C |
| 0x670-0x678 | heartExposedTime, unknown_0x905938b8, unknown_0xb826317a |
| 0x67C | heartDamageSound (0x18) |
| 0x694-0x6A0 | turnSpeedAccel, maxTurnSpeedNormal, maxTurnSpeedMelee, unknown_0xe5a7c358 |
| 0x6A4 | vulnerabilityChangeSound |
| 0x6A8-0x6B0 | tauntFrequency, attackIntervalMin, attackIntervalMax |

The REL's reader (fn_16_ABD0) reads the same 18 property ids as the generated
`SLdrEmperorIngStage1.hpp` (0x255a4580 editor properties, 0xb3774750 patterned, 0x7e397fed actor
parameters, 0x4a1e493b data).

## 3. The loader (fn_16_A2A0)

1. Reads the property block: editor properties, patterned, actor parameters, data.
2. `LdrToModelData(scale, kInvalidAssetId, the patterned info's animation set, true)` at
   0x0000A41C, then moves the result at 0x0000A434. If the model is empty the loader returns null
   (the branch at 0x0000A444).
3. `LdrToPatternedInfo` (0x0000A4D0), `AllocateUniqueId` (0x0000A4EC),
   `LdrToActorParameters` (0x0000A500), `LdrToTransform4f` (0x0000A50C),
   `LdrToEntityInfo` (0x0000A518).
4. `rs_new(0xD18, "CEmperorIngStage1")` at 0x0000A4B8 and the constructor call at 0x0000A540.

## 4. The state machine (fn_16_8AF4)

`SetupStateMachine` (vtable slot 58, 0x8AF4..0x8B80) calls the base's setup on the state machine
at `this + 0x350` (the call at 0x00008B08) and then hands three name tables to it: the triggers
(lbl_16_data_19C, 6 entries) through the state manager's vtable slot +0x18 (0x00008B20), the
states (lbl_16_data_94, 12 entries) through +0x14 (0x00008B40) and the codes (lbl_16_data_220,
3 entries) through +0x1C (0x00008B60).

The tables are built by the static initialiser **fn_16_BF88** (`.ctors` at 0x0000C2A4): each entry
is 16 bytes {name, 0, 0xFFFFFFFF, fn}, and the function pointers come from the three-word records
{0, 0xFFFFFFFF, fn} in `.data` (the states' twelve at 0x4..0x88, the triggers' six at 0x154..0x190
and the codes' three at 0x1FC..0x214). fn_16_BF88's stores: 0x0000C0C0..0x0000C124 and
0x0000C214 for the states, 0x0000C234..0x0000C284 for the triggers, 0x0000C284..0x0000C294 for the
codes.

### States (lbl_16_data_94, .data 0x94, 12 entries)

| # | name (.rodata) | fn | record |
|---|---|---|---|
| 0 | TrackPlayer (0x41C) | fn_16_7C28 | .data 0x4 |
| 1 | BeginHeartStage (0x428) | fn_16_78AC | .data 0x10 |
| 2 | HeartStage (0x438) | fn_16_774C | .data 0x1C |
| 3 | EndHeartStage (0x443) | fn_16_71BC | .data 0x28 |
| 4 | Dead (0x451) | fn_16_716C | .data 0x34 |
| 5 | MeleeAttack (0x456) | fn_16_6F28 | .data 0x40 |
| 6 | Attack (0x462) | fn_16_6D94 | .data 0x4C |
| 7 | ProjectileAttack (0x469) | fn_16_68F4 | .data 0x58 |
| 8 | WarpAttack (0x47A) | fn_16_601C | .data 0x64 |
| 9 | TauntAttack (0x485) | fn_16_5DBC | .data 0x70 |
| 10 | Taunt (0x491) | fn_16_5C4C | .data 0x7C |
| 11 | JumpRope (0x497) | fn_16_5928 | .data 0x88 |

### Triggers (lbl_16_data_19C, .data 0x19C, 6 entries)

| # | name (.rodata) | fn | record |
|---|---|---|---|
| 0 | AllTentaclesRetracted (0x4A0) | fn_16_4BD4 | .data 0x154 |
| 1 | HeartStageOver (0x4B6) | fn_16_4BB4 | .data 0x160 |
| 2 | IsDead (0x4C5) | fn_16_4B6C | .data 0x16C |
| 3 | ShouldMeleeAttack (0x4CC) | fn_16_4B20 | .data 0x178 |
| 4 | ShouldAttack (0x4DE) | fn_16_4B04 | .data 0x184 |
| 5 | ShouldTaunt (0x4EB) | fn_16_4AEC | .data 0x190 |

### Codes (lbl_16_data_220, .data 0x220, 3 entries)

| # | name (.rodata) | fn | record |
|---|---|---|---|
| 0 | BeginSubStage (0x4F7) | fn_16_4AE8 | .data 0x1FC |
| 1 | ResetTentacles (0x505) | fn_16_4A04 | .data 0x208 |
| 2 | ChooseRandomTentacleCount (0x514) | fn_16_4954 | .data 0x214 |

### The ported bodies (slice 3)

| fn | what it does | the data it reads |
|---|---|---|
| fn_16_4BD4 (AllTentaclesRetracted) | true while every live tentacle's state is 0 | the count at guest 0x7D4, the elements' int at +0x28 |
| fn_16_4B20 (ShouldMeleeAttack) | the squared horizontal distance to the player against 900 (30 units) | the player is the state manager's first player actor (retail CStateManager+0x14FC, `mPlayers[0]`); the position is `CActor::mPosition` at +0x54, so the two floats are X and Y |
| fn_16_4B04 (ShouldAttack) | the attack timer has run below zero | the float at guest 0xA4C (4.0f in the constructor) |
| fn_16_4AEC (ShouldTaunt) | the taunt timer is past the taunt frequency | the float at guest 0xA50 (0.0f in the constructor) against the float at 0xC34 |
| fn_16_4AE8 (BeginSubStage) | retail's body is a single `blr` | - |
| fn_16_4A04 (ResetTentacles) | resets the current sub-stage's tentacle slots and tells the script which sub-stage this is | fn_16_1DFC (the slot reset), fn_16_2EE0 (the flags), the mode at guest 0x7C0 (2 -> the message IS7, 1 -> IS6) |
| fn_16_4BB4 (HeartStageOver) | the heart-stage timer has run out | the float at guest 0xA3C (0.0f in the constructor) against 0.0f; the asm folds "less than" and "equal" into one bit (cror eq,lt,eq at 0x4BC4), so the test is `<= 0.0f` |
| fn_16_4B6C (IsDead) | the actor's health has run out | the health info from the vtable slot at +0x3C (slot 13, GetHealthInfo), its second float (CHealthInfo::mHealth at +0x04) against 0.0f (cror eq,lt,eq at 0x4B98) |
| fn_16_716C (Dead) | on the activate message the script is told the actor died | the message is the state's second argument (`cmpwi r5, 0` at 0x7174); the state tag is 0x44454144 ('DEAD', 0x7198) |
| fn_16_4954 (ChooseRandomTentacleCount) | the sub-stage is picked from a roll of 0..100 against 50 | the state manager's random (retail CStateManager+0x16E4) asked for Range(0.0f, 100.0f) at 0x497C (0.0f from lbl_16_rodata_380, 100.0f from lbl_16_rodata_3D4) against 50.0f (lbl_16_rodata_3BC); the mode at guest 0x7C0 picks the pair (0 -> 0/1, 1 -> 1/2, anything else 2) and the result is the sub-stage at guest 0x7C4 |

Their callees, all small enough to port with them:

| fn | size | what it does |
|---|---|---|
| fn_16_1DFC | 0x24 | the slot's timer is cleared and its state becomes 1, unless it is already 2 |
| fn_16_2F70 | 0x38 | how many tentacle slots the sub-stage uses: 0 -> 4, 1 -> 6, else 8 (the sub-stage is the int at guest 0x7C4) |
| fn_16_2EE0 | 0x90 | every live tentacle's flag byte (+0x24) gets its 0x80 bit set when the argument is non-zero and the tentacle is one of the sub-stage's slots |

The tentacle element is 0x48 bytes in retail and starts with the tentacle actor's pointer (+0x00)
and its id (+0x08), which the FSM's tentacle code (fn_16_1F34) uses; the tentacles themselves are
separate actors the state machine spawns, so the port models only the four fields the ported
functions read: the timer (+0x04), the retraction timer (+0x18), the flag byte (+0x24) and the
state (+0x28). The eight slots are the array at guest 0x7D8 and the constructor leaves them
empty (the live count at 0x7D4 is 0), so the ported triggers see no tentacles yet.

### The Think (fn_16_56D8, 0x56D8..0x5928, vtable slot 3)

The body only runs while the actor is active: the asm's gate is the byte at 0x20 with the mask
0x80 (0x5708), which is `CEntity::mActive` (the first bit of the bitfield word at 0x20).

| at | what |
|---|---|
| 0x5710 | fn_16_3668: every live tentacle's per-frame update (fn_16_2B0C each) |
| 0x5720 | `CPatterned::Think` - the base's own Think |
| 0x5734 / 0x5740 | the attack timer (guest 0xA4C) counts down, the taunt timer (guest 0xA50) counts up |
| 0x5748 | fn_16_51D4: the module's own sound update |
| 0x5758 | fn_16_4EB8: moves the actor onto the target's locator |
| 0x5768 | fn_16_3AFC: the three collision actor managers (guest 0x7C8, 0x7CC, 0x7D0) get their update |
| 0x5778 | fn_16_4C0C: the module's track-player update |
| 0x578C | the effect-state timer (guest 0xA78) counts down |
| 0x57CC | `SetEffectState("DarkProjectileCharge", timer > 0.0f)` - the name is the string lbl_16_rodata_340 points at |
| 0x57E4 | fn_16_4C8: aims the actor at the player |
| 0x57F4 | fn_16_8C: the tentacles' hit test and damage |
| 0x57F8..0x5900 | the emitter that follows the target: the handle (guest 0xA8C) must be playing, the target id (guest 0xA90) must exist, and the emitter sits on the target's momentum (its X) in the horizontal direction of the player |

Ported with it (both at or under the slice's 0x80 line): fn_16_3668 (0x80, the loop over the live
tentacles) and fn_16_3AFC (0x74, the collision actor managers' update). The six calls that are
bigger keep a named private stand-in that logs once: fn_16_2B0C (0x50 - it is small, but it calls
three more module functions), fn_16_51D4 (0x128), fn_16_4EB8 (0x31C), fn_16_4C0C (0xBC), fn_16_4C8
(0x368) and fn_16_8C (0x11C4).

### The Think's callees

Ported: fn_16_2B0C (0x50, the per-tentacle update), fn_16_24AC (0x78, the tentacle's two
vulnerability effect states), fn_16_302C (0x34, the tracked actor) and fn_16_2FA8 (0x50, the
second tentacle's collision actor).

Stand-ins, each a named private method that logs once: fn_16_26B8 (0x3B0), fn_16_2524 (0x194)
and fn_16_2070 (0x194) for the tentacle's work, fn_16_4E44 (0x24), fn_16_BDD0 (0x64) and
fn_16_6D10 (0x84) for the particle effect's reference and update - the reference counter the
retail takes at the effect's +0x78/+0x7C lives in the retail particle effect, which the port does
not have - and fn_16_51D4 (0x128) for the sound update, which reads a float at the damage
vulnerability's +4 (0x5214): the retail's CDamageVulnerability is 0x54 bytes and the port's is 0x38
(upstream's layout), so the port has no float there and the value's meaning is not established.

### The message switch (fn_16_7CE8, vtable slot 4)

The base's own handler runs first (the call at 0x7D14), then the message decides:

| message | compared at | what happens |
|---|---|---|
| `kSM_Create` (0x58435254) | 0x7D24 | the body controller (guest 0x48C) is activated with `kAS_Invalid` (0x7D78) |
| `kSM_Delete` (0x5844454C) | 0x7D64 | fn_16_3B70: the three collision actor managers are destroyed (0x7D88) |
| `kSM_AreaLoaded` (0x58414C44) | 0x7D3C | three connected objects are looked up - IS3/ACTIVATE into guest 0xA18 (0x7DAC), CONNECT/0x41574348 into 0xA1A (0x7DD4) and CONNECT/ACTIVATE into 0xA1C (0x7DFC) - and the tentacle work is set up: fn_16_30C8 (0x7E10), fn_16_3EAC (0x7E1C), fn_16_36E8 (0x7E28) |
| `kSM_Activate` (0x41435456) | 0x7D50 | `SetBossParams`: the actor's id (guest +0x08), the health info's initial health (the vtable slot 13 call at 0x7E40, the float at +0x00 of the result) and the string table index of "BossTempleBossStage1" (lbl_16_rodata_41C+0x670) (0x7E6C, 0x7E80) |

The retail's second lookup asks for the message 0x41574348, which the port's `EScriptObjectMessage`
has no name for (its `kSM_Attach` is 0x41544348), so the port uses the raw value.

fn_16_3B70 (0x54) is ported with it; the three tentacle functions are too big and stay stand-ins:
fn_16_30C8 (0x278) builds three sets of five CPASAnimParm from the tracked actor's animation data
(the model's animation data at +0x3C), fn_16_3EAC (0x794) creates the three CCollisionActorManagers
(`rs_new` + the manager's constructor with a joint description vector), gives their collision
actors the damage vulnerability and adds the materials, and fn_16_36E8 (0x29C) builds three sets of
27 CPASAnimParm and fills the joint vector at guest 0x7D4. They are the spawn slice's work.

### The target's float at guest +0x1C0

The Think's emitter tail (0x58A8) reads a float at the target's +0x1C0, and it is
**CPhysicsActor::mMomentum's X**:

- the retail's CActor is 0x158 bytes (include/MetroidPrime/CActor.hpp's `CHECK_SIZEOF(CActor,
  0x158)`), so the retail's CPhysicsActor starts at 0x158;
- the port's CActor is 0x1B8 (the port's own presentation members), so the port's CPhysicsActor
  starts 0x60 later - the port's mMass (its first member) is at 0x1B8;
- CPhysicsActor's member order is upstream's (the port did not touch
  include/MetroidPrime/CPhysicsActor.hpp), and mMomentum is the member at the CPhysicsActor's
  +0x68 (the port's mMomentum is at 0x220 = 0x1B8 + 0x68);
- 0x158 + 0x68 = 0x1C0, the same member in both.

### Still a stand-in: fn_16_7C28 (the state "TrackPlayer")

Its body is three calls, all of them too big for the slice's rule: fn_16_830 (0xB4, the
IS0/IS1/IS2 message), fn_16_353C (0x12C, the attack vulnerability and the tentacle effect states)
and fn_16_7B40 (0xE8, the look-at-the-player helper). fn_16_353C itself calls fn_16_1E20 (0x214),
which applies the damage vulnerability to a tentacle - a class the port does not have yet, so
porting it would need an unported class. fn_16_7C28 therefore stays a stand-in, and the only
ported state function is Dead (fn_16_716C).

The rodata also holds the tentacle locator names ("one_Spine_1_SDK" ... "eight_Spine_7_SDK", the
"portal1_LCTR_SDK" .. "portal8_LCTR_SDK" set, "DarkProjectileCharge", "DarkVulnerability",
"LightVulnerability", "brainSpine_scale") - the tentacle and attack work, milestone 2.

## 5. The port (milestone 1)

`include/MetroidPrime/Enemies/CEmperorIngStage1.hpp` and
`src/MetroidPrime/Enemies/PortCEmperorIngStage1.cpp`. The loader and the constructor are ported;
the six non-trivial vtable overrides (`Think`, `AcceptScriptMsg`, `Render`, `DoUserAnimEvent`,
`ProcessSoundEvent`) forward to `CPatterned` behind a once-only `[emp1] ... is a port stand-in`
log. The accessors that the vtable gives one-line bodies are real.

`SetupStateMachine` (fn_16_8AF4) is ported: the base's setup first, then the module's three
tables on the state machine (6 triggers, 12 states, 3 codes) with retail's name strings. The bodies
of the six triggers, the two codes and the Dead state are retail's bodies (section 4); the rest
are stand-ins: each logs `[emp1] stand-in fn_16_XXXX` once and returns the neutral value (false
for a trigger). No `// @` marker - only the ported bodies are retail's.

The members the ported bodies need are named from their retail offsets and carry the
constructor's values: x7c0_ (0), x7c4_ (0), x7d4_ (0), the eight tentacle slots x7d8_ (empty),
xa3c_ (0.0f), xa4c_ (4.0f) and xa50_ (0.0f). The loader's data block ends at guest 0x6B4, so
none of them is a script value - they are the module's own state.

Not ported yet (milestone 2): the state functions' bodies, the tentacles and their collision
actors, the five attacks, the heart stage, the shock wave, the projectile and the two tokens.

**Entity type**: the REL does **not** override `TypesMatch` (vtable slot 1 is the DOL's
`TypesMatch__10CPatternedCFi`), so the actor keeps the base class's type (`kET_Patterned`) and
`CEntityInfo.hpp` gets no new value.

**Runtime**: Great Temple world `863fcd72`, area 11 (`07_temple_mothlordsanctuary_dark`). The
actor's script leaves it inactive until the fight, so the FSM is reached only after the console
activates it: `objs emperoringstage1` gives the uid (u0687) and `send <uid> Activate` (the port
console, `MP2_CONSOLE=<port>`) wakes it. With the ported bodies the run answers:
**AllTentaclesRetracted true** (no tentacles are out yet), ShouldMeleeAttack false (the player
spawns 56 units away, the range is 30), ShouldAttack false (the timer is 4.0f) and ShouldTaunt
false (the timer is 0.0f). The two timers are only updated by the module's Think (fn_16_56D8),
which is still a stand-in, so those two triggers stay false until it is ported.

With the ported bodies the FSM advances from TrackPlayer to BeginHeartStage (AllTentaclesRetracted
is true, so the TrackPlayer state's transition fires at once) and evaluates that state's four
triggers. HeartStageOver and IsDead belong to the heart-stage states the FSM does not reach yet
(their state body fn_16_76F0 is not ported), the Dead state is entered only on a normal death
(the console's `damage` is an X-damage, which takes the massive-death path instead of
CPatterned::Death's `SetState("Dead")`), and ChooseRandomTentacleCount is a code node of a state
the FSM does not reach - so the four bodies ported in this slice did not run in that scenario.

With `AcceptScriptMsg` ported (fn_16_7CE8) the `kSM_AreaLoaded` message runs the three tentacle
stand-ins (fn_16_30C8, fn_16_3EAC, fn_16_36E8) and the `kSM_Activate` message (the console's
`send <uid> Activate`) hands the boss bar its parameters: id 0x0687, health 700 and the string
table index 55 of "BossTempleBossStage1".

With the Think's track-player update ported (fn_16_4C0C) the "Damage" and "Telegraph" effect
states are turned on and their timers (guest 0xA58 and 0xA5C) count down; the particle effect's
own update and the tentacle work stay stand-ins, and with no tentacle spawned the tentacle
stand-ins do not even run.

With the Think ported (fn_16_56D8) its timers run every frame: the attack timer (guest 0xA4C)
counts down from 4.0f and the taunt timer (guest 0xA50) counts up from 0.0f, so the conditions
the ported triggers read (`xa4c_ < 0.0f` and `xa50_ > tauntFrequency`) are met a few seconds
after the actor is activated. They do not flip in the run, because the host's FSM evaluates a
state's triggers only while the FSM is in that state: the FSM leaves TrackPlayer in its first
frame (AllTentaclesRetracted is true until the tentacles are spawned) and BeginHeartStage's
transitions do not use those two triggers.
