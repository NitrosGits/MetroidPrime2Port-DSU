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
| `fn_16_51D4` | 0x128 | the heart's health becomes the boss's (heart damage sound, the mode 0/1/2 thresholds) - ported, `UpdateHeartHealth` | [proven] |
| `fn_16_4EB8` | 0x31C | the body follows the heart: the rotate actor on the heart's "brainSpine_scale", the 'PLAY' actor turned along the current angle, the body at the 'PLAY' actor's segment 1 - ported, `UpdateTentacleAnchor` | [proven] |
| `fn_16_3AFC` | 0x74 | the three collision actor managers' update | [proven] |
| `fn_16_4C0C` | 0xBC | the module's track-player update | [probable] |
| `fn_16_4C8` | 0x368 | the melee sweep's contact damage to the player (the 0x80 flag; every box grows 4.0f) - ported, `UpdateMeleeHits` | [proven] |
| `fn_16_8C` | 0x3B0 | the sweeping tentacles' contact damage to the player - ported, `UpdateTentacleHits` | [proven] |
| `fn_16_6F28` | 0x244 | the state "MeleeAttack": the melee flag (0xA74 bit 0x80), the turn angle, the loop attack command - ported, `MeleeAttack` | [proven] |
| `fn_16_4C0C` | 0xBC | the track-player update: the "Damage" and "Telegraph" effect states | [probable] |
| `fn_16_2B0C` | 0x50 | the per-tentacle update | [proven] |
| `fn_16_24AC` | 0x78 | the tentacle's dark and light vulnerability effect states | [proven] |
| `fn_16_2070` | 0x12C | the effect state a tentacle carries - ported `SetTentacleEffectState` | [probable] |
| `fn_16_26B8` | 0x3B0 | the tentacle's own update (state 0..3, hits, retract timer) - ported `UpdateTentacleCore` | [probable] |
| `fn_16_2524` | 0x194 | the tentacle's motion update - ported `UpdateTentacleMotion` | [probable] |
| `fn_16_302C` | 0x34 | the actor the module tracks (the id at guest 0xA1A) | [proven] |
| `fn_16_2FA8` | 0x50 | the collision actor of the second tentacle's collision actor manager | [proven] |
| `fn_16_4E44` | 0x24 | takes the effect's particle-system reference - folded into `GetEffectSystem` | [probable] |
| `fn_16_BDD0` | 0x64 | releases that reference - folded into `GetEffectSystem` | [probable] |
| `fn_16_6D10` | 0x84 | external parameter on a system and its spawned systems - `SetSystemParam` | [probable] |
| `fn_16_3094` | 0x34 | the actor connected on CONNECT/'PLAY' | [proven] |
| `fn_16_30C8` | 0x278 | the three tentacle animation ids | [proven] |
| `fn_16_36E8` | 0x29C | the tentacle joint descriptions (eight slots) | [proven] |
| `fn_16_337C` | 0x184 | fills a CPASAnimParmData's parm list - the port's own constructor does this | [probable] |
| `fn_16_2B68` | 0xD4 | fills the joint description in | [proven] |
| `fn_16_1E6C` | 0xC8 | gives the tentacle actor the module's health info, then its materials | [probable] |
| `fn_16_1F34` | 0x20 | the tentacle actor's materials | [proven] |
| `fn_16_3984` | 0x90 | adds the joint to the vector at guest 0x7D4 | [proven] |
| `fn_16_3AA8` | 0x54 | the tentacle's collision actor (the owner's first manager, five descriptions per tentacle, the index 4 + index * 5) | [proven] |
| `fn_16_39CC` | 0x20 | the element's placement copy (calls fn_16_39EC, the null test, then fn_16_3A14, the joint's copy constructor) - the host assignment in `AddTentacleJointToVector` | [proven] |
| `fn_16_7CE8` | 0x1C0 | `AcceptScriptMsg` (vtable slot 4): the message switch | [proven] |
| `fn_16_3B70` | 0x54 | destroys the three collision actor managers | [proven] |
| `fn_16_30C8` | 0x278 | builds the tentacle animation parameter sets | [probable] |
| `fn_16_3EAC` | 0x794 | creates the three collision actor managers (section "fn_16_3EAC") | [proven] |
| `fn_16_3D64` | 0x148 | sets a group's collision actors active (0..7 a tentacle's five at 0x7C8, 8 the heart 0x7CC, 9 the body 0x7D0; CEntity vtable slot 5 SetActive) | [proven] |
| `fn_16_36E8` | 0x29C | builds the tentacle joint collision descriptions | [probable] |
| `fn_16_56D8` | 0xC4 | `Think` (vtable slot 3) | [proven] |
| `fn_16_7CE8` | 0xC4 | `AcceptScriptMsg` (vtable slot 4) | [proven] |
| `fn_16_52FC` | 0x20 | `PreRender` (vtable slot 7): the base's own body | [proven] |
| `fn_16_531C` | 0x2B8 | `Render` (vtable slot 9) - ported | [proven] |
| `fn_16_1D38` | 0xC4 | the tentacle's draw flags for the model's draw pass - ported | [proven] |
| `fn_16_7F8C` | 0x5B0 | `DoUserAnimEvent` (vtable slot 26); case 0 (charged projectile), 7, 17, 22, 27 and 28 ported | [proven] |
| `fn_16_7EA8` | 0xC4 | `ProcessSoundEvent` (vtable slot 28) - ported, marked (section 11.3) | [proven] |
| `fn_16_B958` | 0x30 | `GetTouchBounds` (vtable slot 16): the actor's bounding box | [proven] |
| `fn_16_BA30` | 0x30 | builds the `optional<CAABox>` from the bounding box | [proven] |
| `fn_16_9AB0` | 0xC4 | copies the `SLdrHealthInfo` into the actor's `CHealthInfo` (0x1E bytes) | [proven] |
| `fn_16_98D0` | 0xC4 | copies an `SLdrDamageVulnerability` into a `CDamageVulnerability` (0x54 bytes) | [proven] |
| `fn_16_9700` | 0xC4 | builds the shock-wave block at guest 0xC40 from the taunt attack's `SLdrShockWaveInfo` | [proven] |
| `fn_16_9610` | 0x94 | copies the `SLdrPlasmaBeamInfo` (0x48 bytes) into guest 0xCA8 | [proven] |
| `fn_16_830` | 0xB4 | sends IS01 (1), IS02 (2) or IS00 (anything else) to the script - ported, `SendStageMessage` | [proven] |
| `fn_16_920` | 0xA0 | the tracked actor's "brainSpine_scale" locator position - ported, `GetBrainSpinePosition` | [proven] |
| `fn_16_8E4` | 0x3C | the attack timer (guest 0xA4C) takes Random Range(attackIntervalMin, attackIntervalMax) - ported, `ResetAttackTimer` | [proven] |
| `fn_16_353C` | 0x12C | tentacle vulnerability by mode (0 immune, 2 melee, 3 projectile + DarkVulnerability, 4 warp + LightVulnerability, else normal) - ported, `SetTentacleVulnerability` | [proven] |
| `fn_16_1E20` | 0x4C | one slot's tentacle collision actor gets a damage vulnerability - ported, `SetJointVulnerability` | [proven] |
| `fn_16_7B40` | 0xE8 | horizontal direction from the tracked actor's "sphere_base" to the player -> fn_16_2C9C - ported, `LookAtPlayer` | [proven] |
| `fn_16_2C9C` | 0x140 | signed angle (guest 0xA2C) between the 'PLAY' actor's Y column and a direction - ported, `UpdateTurnAngle` | [proven] |
| `fn_16_2DDC` | 0x40 | CVector3f cross product (inlined as `CVector3f::Cross`) | [proven] |
| `fn_16_2C3C` | 0x60 | fn_16_2C9C with the 'PLAY' actor's own Y column - ported, `ResetTurnAngle` | [proven] |
| `fn_16_3060` | 0x34 | same body as fn_16_3094 (id 0xA1E) - the port uses `GetPlayActor` | [proven] |
| `fn_16_2FF8` | 0x34 | the CONNECT/ACTIVATE actor (id 0xA1C) - ported, `GetActivateActor` | [proven] |
| `fn_16_CD0` | 0x324 | creates the heart stage's beam (rs_new 0x6A8, guest 0xA98) - ported `FireBeam` | [probable] |
| `fn_16_A64` | 0x26C | aims and updates the beam (CBeamProjectile slots 35 Fire, 33 UpdateFx; emitter 0xA9C) - ported, `UpdateBeam` | [probable] |
| `fn_16_9C0` | 0xA4 | resets (slot 34 ResetBeam) and deletes the beam - ported, `StopBeam` | [probable] |
| `fn_16_9BE8` | 0x260 | the portal's effect (a CScriptEffect) is created - ported | [proven] |
| `fn_16_9E9C` | 0x68 | the CGameSplineDesc from a CMayaSpline (the type 3, the duration 1.0f, no closed loop) | [proven] |
| `fn_16_1720` | 0x1C8 | the module's own CPhysicsActor subclass - the "Attack Tip" portal actor (a stand-in) | [proven] |
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
| 0xA2C, 0xA30, 0xA38, 0xA3C, 0xA44, 0xA48, 0xA50, 0xA58-0xA6C | floats | 0 (0xA2C the turn angle, 0xA30 the current turn angle (fn_16_4EB8), 0xA3C the heart-stage timer, 0xA50 the taunt timer) |
| 0xA34 | float | maxTurnSpeedNormal |
| 0xA40 | float | the initial health (from `GetHealthInfo()`) |
| 0xA4C | float | 4.0f |
| 0xA70 | int | 0 |
| 0xA74 | byte | five bits cleared |
| 0xA78 | float | 0 |
| 0xA80-0xA8C | four ints | 0 (0xA8C the sound emitter's handle) |
| 0xA18, 0xA1A, 0xA1C, 0xA1E | u16 | `kInvalidUniqueId` (the four connected actors: IS3/ACTIVATE, CONNECT/ATTACH, CONNECT/ACTIVATE and CONNECT/'PLAY') |
| 0xA20, 0xA24, 0xA28 | ints | -1 (the three tentacle animation ids) |
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
| 0x5748 | fn_16_51D4: the heart's health becomes the boss's |
| 0x5758 | fn_16_4EB8: the body follows the heart |
| 0x5768 | fn_16_3AFC: the three collision actor managers (guest 0x7C8, 0x7CC, 0x7D0) get their update |
| 0x5778 | fn_16_4C0C: the module's track-player update |
| 0x578C | the effect-state timer (guest 0xA78) counts down |
| 0x57CC | `SetEffectState("DarkProjectileCharge", timer > 0.0f)` - the name is the string lbl_16_rodata_340 points at |
| 0x57E4 | fn_16_4C8: the melee sweep's contact damage |
| 0x57F4 | fn_16_8C: the sweeping tentacles' contact damage |
| 0x57F8..0x5900 | the emitter that follows the target: the handle (guest 0xA8C) must be playing, the target id (guest 0xA90) must exist, and the emitter sits on the target's momentum (its X) in the horizontal direction of the player |

Ported with it (both at or under the slice's 0x80 line): fn_16_3668 (0x80, the loop over the live
tentacles) and fn_16_3AFC (0x74, the collision actor managers' update). The six calls that are
bigger keep a named private stand-in that logs once: fn_16_2B0C (0x50 - it is small, but it calls
three more module functions), fn_16_51D4 (0x128), fn_16_4EB8 (0x31C), fn_16_4C0C (0xBC), fn_16_4C8
(0x368) and fn_16_8C (0x3B0; the 0x11C4 was a wrong extent).

### The Think's callees

Ported: fn_16_2B0C (0x50, the per-tentacle update), fn_16_24AC (0x78, the tentacle's two
vulnerability effect states), fn_16_302C (0x34, the tracked actor) and fn_16_2FA8 (0x50, the
second tentacle's collision actor).

Ported in slice 3: fn_16_26B8, fn_16_2524, fn_16_2070 (the tentacle's work) and fn_16_4E44 /
fn_16_BDD0 / fn_16_6D10 (the +0x78/+0x7C counted reference is CParticleGenInfoGeneric's
`ncrc_ptr<CParticleGen>`, so they become `GetEffectSystem` + `SetSystemParam`). fn_16_51D4 is
not a sound update: the +4 it reads is the heart collision actor's CHealthInfo::mHealth (vtable slot
14, HealthInfo), ported in slice 4.

### The animation builders (fn_16_30C8 and fn_16_36E8)

**fn_16_30C8** (0x30C8..0x3340) finds the three tentacle animation ids:

- the tracked actor is the one connected on CONNECT/'PLAY' (the message 0x504C4159, looked up at
  0x3100 and stored at guest 0xA1E at 0x3110);
- fn_16_3094 (0x3094..0x30C8) looks that actor up in the state manager and casts it to an actor;
- the database is that actor's model's animation data at +0x3C (0x3118..0x3124);
- three parameter sets are searched for the best animation (`FindBestAnimation`, -1): the first
  with six NoParameter and FromEnum(0), FromEnum(0) (0x3128..0x319C), the second with FromEnum(0),
  FromEnum(7) (0x31D0..0x3258) and the third with FromEnum(0), FromEnum(6) (0x3280..0x3308). An id
  is only taken when the search found a time above 1.1920929e-07f (lbl_16_rodata_3A8): 0xA20 at
  0x31CC, 0xA24 at 0x3278 and 0xA28 at 0x3324.

**fn_16_36E8** (0x36E8..0x3984) builds the joint descriptions: for each of the eight tentacle slots
(the loop at 0x395C..0x3964) three sets are searched - FromEnum(0) with FromInt32(the index),
FromEnum(2) with FromInt32(2) and FromEnum(1) with FromInt32(the index) - and when the first two
found an animation (the tests at 0x3918 and 0x3920) the joint is built and added to the vector at
guest 0x7D4 (0x3928..0x3958).

fn_16_337C (0x184) fills the parm list of a CPASAnimParmData, which the port's own
`CPASAnimParmData` constructor does, so it is not a separate function here. The three functions
that build and add the joint are ported with the joint work (fn_16_2B68, fn_16_1E6C and
fn_16_3984 below).

### The joint work (fn_16_2B68, fn_16_1E6C, fn_16_3984)

**fn_16_2B68** (0x2B68..0x2C3C) fills a joint description in (the retail element is 0x48 bytes
and the module has its own type, so the port has its own struct): the owner (0x2B6C), the radius
(0x2B88, 0.0f from lbl_16_rodata_380), the index and the three animation ids (0x2B98..0x2BA4),
the time and the damage hit time (0x2BA8, 0x2BAC, the latter CPatterned::skDamageHitTime), the speed
(0x2BB0), the flag byte with its six high bits cleared (0x2BB4..0x2BF8: 0x80, 0x40, 0x20, 0x10, 0x08
and 0x04; the low two bits stay), the state (0x2BFC), the id (0x2C04, kInvalidUniqueId) and the
zero and up vectors (0x2C08..0x2C34).

**fn_16_1E6C** (0x1E6C..0x1F34) finds the tentacle's collision actor (fn_16_3AA8) and copies the
module's health info into its (0x1EB8..0x1F04 take the module's guest 0xAAC..0xAC8), then calls
fn_16_1F34 with the flag 1 (0x1F14).

**fn_16_1F34** (0x1F34..0x1FC0) adds the materials kMT_Target (0x28), kMT_Orbit (0x29),
kMT_SeekerTarget (0x3F) and kMT_ExcludeFromRadar (0x39) to the tentacle actor when the flag is
set (0x1F74..0x1F8C) and removes them when it is not (0x1F90).

**fn_16_3984** (0x3984..0x39CC) adds the joint to the vector at guest 0x7D4: the retail's vector
copy-constructs the element at the count (fn_16_39CC, 0x20 -> fn_16_39EC -> fn_16_3A14) and the
count is raised (0x39B4); the vector has a fixed capacity, so it never grows.

**fn_16_3AA8** (0x3AA8..0x3AFC) finds the tentacle's collision actor: the owner's first collision
actor manager (guest 0x7C8) holds five joint descriptions per tentacle - the index is scaled by
five (0x3AB4) and the group starts four earlier (the callers pass 4 in r6: `li r6, 0x4` at
0x1E2C, 0x1E74, 0x1F58, 0x1FE0, 0x2560 and 0x7FDC), so the tentacle's own description is the last
of its five, at 4 + index * 5. The description's collision actor id (at +0x3C) is looked up in the
state manager (0x3AE0) and cast to a collision actor (0x3AE4). It is 0x54 bytes, under the slice's
0x80 line, so the port has it; the manager is fn_16_3EAC's.

### The message switch (fn_16_7CE8, vtable slot 4)

The base's own handler runs first (the call at 0x7D14), then the message decides:

| message | compared at | what happens |
|---|---|---|
| `kSM_Create` (0x58435254) | 0x7D24 | the body controller (guest 0x48C) is activated with `kAS_Invalid` (0x7D78) |
| `kSM_Delete` (0x5844454C) | 0x7D64 | fn_16_3B70: the three collision actor managers are destroyed (0x7D88) |
| `kSM_AreaLoaded` (0x58414C44) | 0x7D3C | three connected objects are looked up - IS3/ACTIVATE into guest 0xA18 (0x7DAC), CONNECT/ATTACH into 0xA1A (0x7DD4) and CONNECT/ACTIVATE into 0xA1C (0x7DFC) - and the tentacle work is set up: fn_16_30C8 (0x7E10), fn_16_3EAC (0x7E1C), fn_16_36E8 (0x7E28) |
| `kSM_Activate` (0x41435456) | 0x7D50 | `SetBossParams`: the actor's id (guest +0x08), the health info's initial health (the vtable slot 13 call at 0x7E40, the float at +0x00 of the result) and the string table index of "BossTempleBossStage1" (lbl_16_rodata_41C+0x670) (0x7E6C, 0x7E80) |

The second lookup's message is `kSM_Attach` 0x41544348 (`lis r7, 0x4154` + `addi 0x4348`,
0x7DB8/0x7DD0); an earlier reading of 0x41574348 left guest 0xA1A invalid.

fn_16_3B70 (0x54) is ported with it, and so are the three tentacle functions: fn_16_30C8 and
fn_16_36E8 (the animation builders below) and fn_16_3EAC (the collision managers, last section).

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

### Slice 2: the states TrackPlayer, BeginHeartStage, HeartStage and Attack

Ported: TrackPlayer (fn_16_7C28: IS01 + normal vulnerability; update stands still and turns to
the player), BeginHeartStage (fn_16_78AC: IS00, immune tentacles, heart sphere on, the 'PLAY'
actor plays animation xa24_, ACTIVATE to the CONNECT/ACTIVATE actor, IS04; animation state over
when "Whole Body" ends; deactivate hides the 'PLAY' actor and the body sphere), HeartStage
(fn_16_774C: heartExposedTime, beam delay 3.0, firing time 3.75, refire delay 0.5, locomotion 9 then
1) and Attack (fn_16_6D94: per-mode attack lists rodata_34C/358/368 = {0,0,3}, {1,0,3,0},
{2,3,1,0}; 0 ProjectileAttack, 1 WarpAttack, 2 TauntAttack, 3 JumpRope; index guest 0xA70).
(Slice 3 below ports the rest of this walk up to EndHeartStage.)

The retail CPatterned word at guest 0x6B4 is the host's `mAnimationState` (0 not ready, 1 ready,
3 over). The beam (guest 0xA98) is a `CBeamProjectile`; its +0x444 is `mCollisionPoint`.

### Slice 3: tentacle motion, ProjectileAttack, FireBeam, Taunt, JumpRope

Ported: fn_16_26B8 `UpdateTentacleCore`, fn_16_2524 `UpdateTentacleMotion`, fn_16_2070
`SetTentacleEffectState`, fn_16_1FC0 `RestoreTentacleHealth`, fn_16_3C58 `GetTentacleIndex`,
fn_16_3BC4 `SendTentacleDestroyed`, fn_16_68F4 `ProjectileAttack`, fn_16_2E7C
`RetractedTentacleCount`, fn_16_CD0 `FireBeam`, fn_16_5C4C `Taunt`, fn_16_5928 `JumpRope`,
fn_16_2E1C `SetTentacleMaterials`, and case 0 of fn_16_7F8C `DoUserAnimEvent` (the charged
projectile; without it the loop attack's projectile event fell through to CPatterned and crashed
in LaunchProjectile).

Joint fields: +0x0C out animation, +0x10 back animation, +0x14 retracted idle, +0x1C hit time,
+0x20 last health. Joint flags: 0x04 forces state 2, 0x10 shootable, 0x20 collision on, 0x40
collision wanted, 0x80 fn_16_2EE0's flag. States: 0 retracted, 1 coming out, 2 out, 3 going back.
Guest 0xA74 flags: 0x04 ?, 0x08 projectile charged, 0x10 ?, 0x20 ?, 0x40 jump rope, 0x80 the melee
sweep (fn_16_6F28 sets it at 0x6F8C and clears it at 0x7144; fn_16_4C8's gate). Retail
`CTryCommand::ShouldRetry(0x6B4,
bc, n)` is host `mAnimationState.CanIssueCommand(bc, n)`; the +0x350 vtable 0x30 call is
`StateMachineState().GetTime()`.

Member fixes: guest 0xBB8/0xBC4 are `projectile.projectile` + `projectile.projectileDamage`
(data+0x594), now `mProjectile` (the old `mBeamDamage` read beamAttack.damage); 0xBE0 is
`projectile.projectileVisorEffect`; 0xC80 is the beam's CProjectileInfo (beamInfo.weaponSystem +
beamAttack.damage at data+0x65C), 0xCA8 the SLdrPlasmaBeamInfo, 0xCF0 beamAttack.sound; 0xBF8 is
`unknown_0xe5a7c358`, the jump rope's turn speed in degrees; 0xA34 the current turn speed, 0xA38
the jump rope duration (3.5/5.0/6.5 by mode 0x7C0).

Runtime (area 11, `send <uid> Activate`, console `damage` on the tentacle boxes): the walk cycles
TrackPlayer -> Attack (ProjectileAttack charges and sets 0x08; JumpRope) -> TrackPlayer -> Taunt;
damaged tentacles go 3 -> 0 and BeginHeartStage follows. The 'PLAY' actor stays inactive when the
boss is activated from the console (the script that wakes it is skipped), so BeginHeartStage
waits until it is sent Activate too; then HeartStage fires the beam three times (guest 0xA98 takes
new ids) and the heart-exposed time runs out into EndHeartStage (fn_16_71BC, 0x534, a stand-in that
never ends). 0 caught/reach-stub.

Still stand-ins: EndHeartStage fn_16_71BC (0x534), fn_16_8C (0x3B0, tentacle/heart hit test -
heart damage does not reach the boss) and fn_16_51D4 (0x128).

fn_16_4C8 is not the "facing": it is fn_16_8C's body with two differences - the gate is the
0x80 flag (the melee sweep) instead of 0x40 (the jump rope), and every box grows by 4.0f
(lbl_16_rodata_38C: the box's minimum drops by 4.0f at 0x704, its maximum rises by 4.0f at
0x734) instead of the tip pair reaching 8.0f past its end. The melee damage, the player's touch
bounds and the 0.2f cooldown are the same. Ported as `UpdateMeleeHits`.

fn_16_4EB8 (0x4EB8..0x51D4) is the body following the heart, not the facing: the CONNECT/ACTIVATE
actor (fn_16_2FF8) goes to the heart's (fn_16_302C) "brainSpine_scale" locator (0x4F10), the 'PLAY'
actor (fn_16_3060) is turned along its own flattened Y column, rotated by the current angle (guest
0xA30) times dt and looked at the heart's "sphere_base" (rodata_37C) with the heart's Z column as
up (0x5060..0x5110), and the body itself goes to the 'PLAY' actor's segment 1, 1.5f (rodata_394)
along its X column (0x5120..0x51A8). The angle (guest 0xA2C) is approached at the accel (guest
0xBFC, degrees per second, turned into radians with rodata_3DC = 0.017453292) and the current
angle is clamped to the turn speed (guest 0xA34). Ported as `UpdateTentacleAnchor`; the guest
0xA30 member is new to the port.

Runtime (area 11, 2026-10-20, `WORLD=863fcd72,07_temple_mothlordsanctuary_dark`, `MP2_GODMODE=1`,
`send u04e3 Activate`): the boss moves from (1.7, -43.5, 105.0) to (3.2, -43.5, 104.9) - the body
now rides the heart - and the run ends on the frame limit with 0 caught/reach-stub. fn_16_4C8 needs
the player within the melee range (the trigger ShouldMeleeAttack, 30 units), so it does not run in
that scenario; with the player teleported next to the boss (`tp 5.0 -43.5 104.9`) the FSM enters
the state "MeleeAttack" (fn_16_6F28), which sets the 0x80 flag, and a gdb breakpoint on
`CEmperorIngStage1::UpdateMeleeHits` with the condition `xa74_ & 0x80` is hit from
`CEmperorIngStage1::Think` - fn_16_4C8's body runs.

fn_16_6F28 (0x6F28..0x716C) is the state "MeleeAttack" (lbl_16_data_94 entry 5), not part of
fn_16_6D94 (that is the state "Attack", already ported): on activate it sends IS00 (fn_16_830,
0x6F6C), gives the tentacles the melee vulnerability (mode 2, fn_16_353C, 0x6F7C), sets the
animation state ready (0x6F84), sets the melee flag (guest 0xA74 bit 0x80, 0x6F8C) and the melee
turn speed (guest 0xBF4, 0x6F98). On update it asks the animation state for the loop attack
(CanIssueCommand, pas 9 = kAS_LoopAttack, 0x6FAC) and delivers CBCLoopAttackCmd(kLAT_Four)
(the retail's direction 4, 0x7004); otherwise it sets the turn angle (guest 0xA2C) to the melee
turn speed in radians (lbl_16_rodata_3DC), positive once the state has run for 0.5f
(lbl_16_rodata_3F4) and negative before, and after 3.0f (lbl_16_rodata_3C0) the melee trigger
(fn_16_4B20) decides whether the animation state is over (0x70E8). On deactivate the body gets
ExitState (0x7110), the animation state is not ready, the attack timer is re-rolled (fn_16_8E4),
the melee flag is cleared (0x7144) and the turn speed is the normal one (guest 0xBF0). Ported as
`MeleeAttack`.

The rodata also holds the tentacle locator names ("one_Spine_1_SDK" ... "eight_Spine_7_SDK", the
"portal1_LCTR_SDK" .. "portal8_LCTR_SDK" set, "DarkProjectileCharge", "DarkVulnerability",
"LightVulnerability", "brainSpine_scale") - the tentacle and attack work, milestone 2.

### Slice 4: the stage can be finished

Extents (`.fn` markers of the retail listing, next prologue after each `blr`): fn_16_8C is
0x8C..0x43C (0x3B0; fns.txt agrees, the old 0x11C4/0x1A50 were wrong); fn_16_39CC is 0x39CC..0x39EC
(0x20, a call to fn_16_39EC; the old 0xDC was wrong).

- fn_16_8C is not the heart's hit test: it is the tentacles' contact damage while flag 0x40 (JumpRope)
  is set - 8 x 5 locator boxes (the tip box stretched 8.0f horizontally), player touch bounds,
  melee damage (guest 0xB8C), 0.2 s cooldown (guest 0xA48).
- The heart's hits land on its own collision actor (a copy of the boss's health info); fn_16_51D4
  (Think, 0x5748) copies its health back to the boss, plays heartDamageSound (guest 0xCFC) on a drop
  (guest 0xA40 the last value), pulses the "Damage" effect and steps the mode 0 -> 1 -> 2 at
  unknown_0x905938b8 / unknown_0xb826317a (guest 0xBAC / 0xBB0, `mModeOneHealth` / `mModeTwoHealth`).
- EndHeartStage fn_16_71BC: body sphere and 'PLAY' active, CONNECT/ACTIVATE actor DEACTIVATE, 'PLAY'
  anim 3 + IS05; update issues a generate (type by sub-stage 0/1/2 -> 0/2/1) or sets used slots to
  state 2 (fn_16_7AD4); over when "Whole Body" ends; deactivate restores anim 1, flags, attack timer,
  normal vulnerability.
- The body command id of that generate is 0xF (`li r4, 0xf` at 0x73D8, 0x74AC and 0x7580 before
  `DeliverCmd__16CBodyStateCmdMgrF13EBodyStateCmd`), and 0xF is the retail's Generate: the DOL's
  `CBodyStateCmdMgr` ctor (0x800EE990) stores {&__vt__14CBCGenerateCmd, 0xF} at 0x208/0x20C. The
  DOL's order is 0 Getup, 1 Step, 2 Die, 3 KnockDown, 4 KnockBack, 5 MeleeAttack, 6
  ProjectileAttack, 7 LoopAttack, 8 LoopReaction, 9 LoopHitReaction, 10..14 the base
  CBodyStateCmd, 15 (0xF) Generate, 16 Hurled, 17 Jump, 18 Unknown18, 19 Unknown19, 20 Slide, 21
  Taunt, 22 Scripted, 23 Cover, 24 WallHang, 25 Locomotion, 26 AdditiveIdle, 27 AdditiveAim, 28
  AdditiveFlinch. The host's `EBodyStateCmd` (include/Kyoto/Animation/CharacterCommon.hpp) is the
  same order, so `kBSC_Generate` is 15 = 0xF (14 is `kBSC_MaintainVelocity`).

### Slice 5: the boss's own sound handling and the user-event cases

**fn_16_7EA8 (ProcessSoundEvent, vtable slot 28, 0x7EA8..0x7F8C)**: the sound is swallowed while
the locator belongs to a tentacle that cannot be shot. The tentacle is fn_16_3C58 (0x7F08, the
segment's spine locator); -1 means there is none (0x7F0C) and the base's own handling runs. With a
tentacle, the joint's flag byte (the joint's +0x24; the guest 0x7D8 + index * 0x48 + 0x24 = 0x7FC,
read at 0x7F1C) is tested: the extrwi at 0x7F20 takes one bit at the bit 27, the bit 4 of the byte,
so the mask is 1u << (31 - 27) = 0x10 - the "shootable" bit. Clear means no sound at all (0x7F24),
set means the base's own `ProcessSoundEvent` runs (the call at 0x7F6C). Ported.

**fn_16_7F8C (DoUserAnimEvent, vtable slot 26, 0x7F8C..0x853C)**: the switch is the jump table at
lbl_16_data_314 (29 entries, the event type is the index); the default entry (fn_16_7F8C+0x58C)
and every type above 28 (the bgt at 0x7FAC) call the base's own DoUserAnimEvent (0x8524). The
cases that are not the base's:

| type | name | entry | what it does |
|---|---|---|---|
| 0 | kUE_Projectile | +0x98 (0x8024) | the charged projectile (ported in slice 3) |
| 7 | kUE_GenerateEnd | +0x404 (0x8390) | the flag byte at guest 0xA74 gets its bit 4 (0x10) set (the rlwimi at 0x8398 rotates 1 left by 4 into the bit 27) |
| 17 | kUE_BeginAction | +0x418 (0x83A4) | the taunt's shock wave (below) |
| 22 | kUE_BreakLockOn | +0x3C (0x7FC8) | every tentacle's collision actor becomes the player's orbit target (the request 8, `li r5, 0x8` at 0x7FF8) |
| 27 | kUE_EventStart | +0x26C (0x81F8) | the tentacle's portal starts |
| 28 | kUE_EventStop | +0x360 (0x82EC) | the tentacle's portal stops |

**Case 17 (the shock wave)**: a new unique id from the state manager (0x83A8) is kept at guest
0xA90 (0x83B8) and handed to the CShockWave (0x8464). The tracked actor's transform (fn_16_302C,
the retail's +0x24) is the shock wave's; the CShockWave is 0x9F8 bytes, its name is the string at
lbl_16_rodata_41C's 0x685 ("Shockwave"), the entity info is the area (the guest +0x04), no
connections, active and no editor id, the parent is this actor's id (the guest +0x08), the shock
wave info is the one at guest 0xC40 (data+0x5E0, `tauntAttack.shockWaveInfo`), and the two floats
are 4.0f (lbl_16_rodata_38C) and 0.4f (lbl_16_rodata_40C). The wave is added at 0x84AC and the
emitter for the shock wave's travel sound (the guest 0xC7C, data+0x610) is created on this actor's
position (the guest 0x54) and kept at guest 0xA8C (0x8510). The host's `CEntity::NullConnectionList`
is the empty connection list.

**Cases 27 and 28 (the portal)**: the eight portal locators are lbl_16_rodata_1E0 ("portal1" ..
"portal8" with the seventh before the sixth); the event's locator name is compared with them
case-insensitively (0x8260, 0x8330) and the name's place in the list is the tentacle's index.
Case 27 only runs while the flag byte's bit 5 (0x20) is set (the extrwi at 0x81FC takes one bit at
the bit 26, so the mask is 1u << (31 - 26) = 0x20); it takes the locator's world transform
(0x8204, `GetTransform() * GetScaledLocatorTransform`), the translation (the fourth column) and
the Y column (the second basis vector, negated at 0x82A8). Both cases stop at the sub-stage's
tentacle count (fn_16_2F70, 0x8284/0x8354). Case 28 computes the same world transform
(0x82F0..0x8314) but never reads it.

**fn_16_1A50 (0x1A50..0x1C5C, 0x20C) and fn_16_1950 (0x1950..0x1A14, 0xC4)**: the tentacle's
portal effect. Both are over the slice's 0x80 line, so they stay stand-ins. fn_16_1A50 sets the
joint's flag 0x08 (the rlwimi at 0x1A84), writes the position and the up vector into the joint
(0x1B88..0x1BBC), turns the portal's effect towards the player (0x1BFC) and sends "ACTV" (0x1BC8);
fn_16_1950 clears the joint's flag 0x08 (the rlwimi at 0x1980), zeroes the joint's position and
sets its up vector (0x1990..0x19C0) and sends "ACTV" (0x1A18).

Runtime (area 11, 2026-10-20, `WORLD=863fcd72,07_temple_mothlordsanctuary_dark`, `MP2_GODMODE=1`,
`send <uid> Activate`, the player teleported next to the boss): the run ends on the frame limit
with 0 caught/reach-stub and only the Render stand-in left in the log. An instrumented build shows
that the **only** user event the area-11 scenario fires is type 2 (kUE_LoopedSoundStop, twice), so
none of the new cases runs there and no CShockWave object appears (the taunt attack, which would
fire case 17, is still a stand-in). The new cases need their own scenario.

**Render (fn_16_531C, 0x531C..0x55D4) is ported.** The model is drawn eight times, once per
tentacle: the particle POI mask and target (the flag byte at the guest 0x421's bit 0 = the host's
`mDrawParticles`, 0x5340), `RenderSystemsToBeDrawnFirst` (the vtable slot 46, 0x535C), the point
generator particles (the byte at the guest 0x152's bit 6 = the host's
`GetPointGeneratorParticles`, 0x537C), the rendering model and the alpha buffer (0x5394..0x53D8),
the eight passes (0x53DC..0x54E0) and `RenderModelMultipleTimesWithFlags` (0x54FC, the count 8),
the alpha buffer again (0x5518), the point generator particles' own draw (0x5580) and
`RenderSystemsToBeDrawnLast` (the vtable slot 47, 0x55A0). The host's masked draw keeps the
surfaces whose material mask intersects the pass's mask, so every surface is drawn once, by the
pass that owns it.

The four values each pass gets (the joint's slot at guest 0x7D8 + index * 0x48):

| fn | size | what it builds |
|---|---|---|
| fn_16_1D38 | 0xC4 | the draw flags: opaque (the blend mode 0), the shader set 0, the depth flags 0x03 (the depth compare and the depth update) and the colour white (the CColor at 0x1D88); the joint's flag 0x08 (the extrwi at 0x1D50, the bit 28) adds the 0x80 flag (the ori at 0x1D94). The retail leaves the flags' first word (the host's `x0_`, which nothing reads) uninitialised; the port zeroes it. |
| fn_16_1C94 | 0x60 | the colour: `CColor::Lerp(skDamageColor, Black, hitTime / skDamageHitTime)` (the fdivs at 0x1CD4), so a tentacle that was just hit is dark red and one that was never hit is black |
| fn_16_1C5C | 0x38 | the plane: the joint's up vector and the position's dot with it (the fmadds at 0x1C78 and 0x1C84) |
| fn_16_1CF4 | 0x44 | the draw mask: the joint's index picks one of the eight u64s at lbl_16_rodata_300 (their high half is zero and their low half holds 0x200, 0x400, 0x100, 0x1000, 0x8, 0x4, 0x10 and 0x40); the index 0 also gets the bit 0x02000000 (the lis 0x200 at 0x1D00) |

**The draw mask's byte order**: the host reads a material's mask as a native u64 (the material
data stays big-endian in memory, `include/Kyoto/Graphics/CCubeMaterial.hpp`'s `GetMaterialMask`),
so the host's masks are the retail's byte-swapped. The model's eight surfaces have the host's
masks 0x0500002000000000, 0x0900002000000000, 0x1100002000000000, 0x4100002000000000,
0x0101002000000000, 0x0102002000000000, 0x0104002000000000 and 0x0110002000000000, whose
retail's values are 0x0000000020000005, 0x0000000020000009, 0x0000000020000011,
0x0000000020000041, 0x0000000020010001, 0x0000000020020001, 0x0000000020040001 and
0x0000000020100001 - each of them matches exactly one of the retail's table entries (the fifth,
the fourth, the sixth, the seventh, the second, the zeroth, the first and the third). So the port
passes the byte-swapped entries, and the host's masked draw (the material mask AND the pass's
mask) keeps each surface in exactly one pass.

### Slice 6: the taunt attack and the tentacle's portal

**fn_16_5DBC (TauntAttack, 0x5DBC..0x601C, 0x260)** - ported. Two function-local statics with
their own "initialized" guard bytes (lbl_16_bss_C with lbl_16_bss_10 and lbl_16_bss_14 with
lbl_16_bss_18; both start at 0, and the second one takes the retracted tentacle count at the
activate, 0x5EB4). Activate: the script hears IS00 (fn_16_830, 0x5E60), the tentacles get the mode
1 vulnerability (fn_16_353C, 0x5E7C) and the state's own timer (the guest 0xA54) is 0.0f
(0x5EA8). Update: the actor looks at the player (fn_16_7B40, 0x5EC4), the timer counts up (0x5ED4)
and the animation state is asked for the loop attack (ShouldRetry, pas 9 = kAS_LoopAttack,
0x5EE0); if it may, the body gets the loop attack command (the command id 7 with the type 2,
0x5EEC), otherwise the timer over the taunt's loop duration (the guest 0xC38, 0x5F78) gives the
exit state command (the id 10, 0x5FA8). Deactivate: the body gets the abort scripted command
(the id 13, 0x5FDC) and the animation state is not ready (0x5FF8).

**fn_16_1A50 (0x1A50..0x1C5C, 0x20C) and fn_16_1950 (0x1950..0x1A14, 0xC4)** - the tentacle's
portal, ported. The portal's event (case 27) sets the joint's flag 0x08 (the rlwimi at 0x1A84),
writes the position and the up vector into the joint (0x1A88..0x1ABC) and turns the portal's effect
(the joint's id, the joint's +0x2C) along the up vector (the fadds at 0x1B34..0x1B4C and the LookAt
at 0x1B60); the effect hears "ACTV" (kSM_Activate, 0x41435456, 0x1B84). The stop (case 28) clears
the flag (0x1980), the position and the up vector (0x1990..0x19C0) and the effect hears "DCTV"
(kSM_Deactivate, 0x44435456, 0x19F8). The effect's creation (fn_16_9BE8) and the tail of
fn_16_1A50 are ported in slice 7 below.

**fn_16_601C (WarpAttack, 0x601C..0x67EC, 0x7D0)** is ported in slice 7 below.

Runtime (area 11, 2026-10-20, a temporary hook that forces the mode to 2 in the heart's health
update, reverted before the commit): the FSM's Attack state uses the mode 2 list {2, 3, 1, 0} -
the taunt attack (2), the jump rope (3), the warp attack (1) and the projectile attack (0) - and
the run shows the taunt attack's activate, update and deactivate and, with a longer run, the
warp attack's stand-in as well. **No user event fires in area 11 at all** (not the portal's 27 and
28 and not any other type), so the two portal functions are not exercised by this scenario. 0
caught/reach-stub.

### Slice 7: the warp attack, the portal's effect and the warp sound

**fn_16_601C (WarpAttack, 0x601C..0x67EC, 0x7D0)** - ported. The switch on the message is the
retail's: 0 activate (0x60D8), 1 update (0x613C), 2 deactivate (0x6720), everything else returns.

| at | what |
|---|---|
| 0x6084 | the function's own statics: the portal counter (lbl_16_bss_4, an int) and its initialisation guard (lbl_16_bss_8, a byte); the guard sets the counter to -1 on the first call |
| 0x60E4 | the script hears IS02 (fn_16_830 with 2) |
| 0x60F0 | the animation state is ready |
| 0x60FC | the tentacles take the warp vulnerability (mode 4, fn_16_353C) |
| 0x6100 | the portal timer (the guest 0xA44) is 0.0f |
| 0x6120 / 0x612C | the flag byte (the guest 0xA74): the bit 27 (the byte's 0x10) is cleared and the bit 26 (the byte's 0x20) is set |
| 0x6134 | the portal counter is -1 |
| 0x6158 | past 4.0f (lbl_16_rodata_38C) the tentacles are immune (fn_16_353C with 0) and lose their materials (fn_16_2E1C with 0) |
| 0x618C | the portal counter takes the tentacles that are out (fn_16_2F70 - fn_16_2E7C) |
| 0x61BC | the loop attack (ShouldRetry, pas 9): the body gets CBCLoopAttackCmd(kLAT_One) - the id 7 with the type 1 |
| 0x6258 | otherwise the flag byte's bit 27 (0x10, the "GenerateEnd" event) has to be set |
| 0x6270 | the portal timer counts down |
| 0x62B0 | one portal: the counter is spent, the timer is 0.75f (lbl_16_rodata_3A4) and the portal is made |
| 0x66D0 | the state is over when the counter is spent and the state has run for more than 5.0f (lbl_16_rodata_3AC) |
| 0x6740 | the body gets the exit state command (the id 10) |
| 0x6760 / 0x6774 / 0x677C | the flag byte's bit 26 (0x20) is cleared, the tentacles get their materials back and the animation state is not ready |

The two body-command records are lbl_16_data_3B0 {0, 0, fn_16_5BA8} (the loop attack) and
lbl_16_data_3BC {0, 0, fn_16_5C04} (the exit state); the retail tells the two apart by the
address of the record.

**The portal (0x62B0..0x66AC)**: the "Attack Tip" model data is the CAnimRes of the guest 0xC04
(the ANCS id), 0xC08 (the character index), 0xC0C (the default animation) with the one vector as
its scale and no loop. The portal's position has two paths: the random one (0x6364) and the ray
one (0x6458). The player's **morph ball state** (the retail's CPlayer+0x38C, the host's
CPlayer::mMorphBallState) picks between them: zero gives the random path, anything else the ray
path. The random path puts the portal at the player's eye position plus the player's transform
applied to the local vector (range2, 10.0f, range1), with range1 = Random Range(2.0f, 5.0f)
(lbl_16_rodata_3B0/3AC) and range2 = Random Range(-10.0f, 10.0f) (lbl_16_rodata_3EC/390), and the
portal looks at the eye position. The ray path drops a ray of 50.0f (lbl_16_rodata_3BC) from the
player's position plus the up vector and takes the hit point (the look at point stays the
origin). The portal actor itself (fn_16_1720, 0x1C8: the module's own CPhysicsActor subclass
with the portal's effect, the warp stab damage and the two warp sounds) is over the slice's 0x80
line here and stays a stand-in - slice 8 ports it.

**fn_16_9BE8 (the portal's effect, 0x9BE8..0x9E48, 0x260)** - ported. The id comes from the state
manager (0x9C08); the effect is a CScriptEffect (0x2D0 bytes) named "WarpPortalEffect" (the
string at lbl_16_rodata_41C's 0x68F) with the identity transform, the one vector as the scale,
the entity info (the area, no connections, active and no editor id), the light parameters
(CLightParameters(false, 1.0f, kST_Zero, 0.0f, 0.0f, Black, true, kLO_NoShadowCast,
kLR_FourFrames, the zero vector, 2, 2, false, 0, true)) and the spline (fn_16_9E9C: a default
CMayaSpline, the type 3 (kST_Bezier), the duration 1.0f and no closed loop).

**fn_16_1A50's tail (0x1BD0..0x1C3C)** - ported: the emitter for the warp sound (the guest 0xC2E)
at the effect's own position, the max distance 50.0f (lbl_16_rodata_3BC), the distance
compensation 0.1f (lbl_16_rodata_39C), the flags 1, the max volume 0x7F and the min volume 0; the
emitter's area is the effect's own area and the retail throws the handle away.

Runtime (area 11, 2026-10-20, `WORLD=863fcd72,07_temple_mothlordsanctuary_dark`, `MP2_GODMODE=1`,
`send u04e3 Activate`, a temporary hook that forces the mode to 2 in the heart's health update,
reverted before the commit): the warp attack runs (its portal block logs the fn_16_1720
stand-in), **four portal effects are created** (the ids 414, 416, 418 and 420 in area 11 - one
per tentacle that gets the portal's "EventStart" event) and the run ends on the frame limit with
0 caught/reach-stub.

### Slice 8: the ray path's origin and the portal actor

**CPlayer's vtable slot 21** (the word at 0x54 of `__vt__7CPlayer`, the DOL's
`GetAimPosition__7CPlayerCFRC13CStateManagerf`) is `GetAimPosition` - the retail's warp attack
asks the player for it (0x6470) with the state manager and 0.75f (lbl_16_rodata_3A4, 0x6474) and
drops the ray from that point plus the up vector.

**The portal actor (fn_16_1720, 0x1720..0x18E8)** - ported as the module's own CPhysicsActor
subclass (`CEmperorIngStage1WarpPortal`, the retail's vtable lbl_16_data_27C). The retail's object
is 0x300 bytes; its own fields are the timer (the guest 0x2D0), the effect id (0x2D4), the hit
flag (0x2D6), the portal's effect asset (0x2D8), the stab damage (0x2DC) and the two warp sounds
(0x2F8 and 0x2FA). The base call (0x1848) is CPhysicsActor with the flag 0, an empty material
list, the bounding box (-2.0f..2.0f, 0x1800), the mover data (fn_16_18E8: the zero velocity, the
identity angular velocity, the zero momentum, the identity and the mass 1.0f), the actor
parameters and the default step data.

Its overrides:

| fn | what |
|---|---|
| fn_16_0 | the destructor (the vtable back and the base's own) |
| fn_16_1150 (0x424) | the think: the timer counts up, past 0.75f the portal goes inactive, when the "Whole Body" animation is over the portal goes inactive and its effect gets the emitter for the second warp sound and the "ACTV" message, past 5.0f the portal asks to be deleted, the animation is updated, and once the player's touch bounds overlap one of the seven "eight_Spine_*" locator boxes (2.0f around the locator) the stab damage is applied |
| fn_16_1574 (0x1AC) | the script messages: kSM_Create creates the portal's effect (fn_16_9BE8) and gives it the portal's transform and the "ACTV" message, kSM_Delete deletes the effect |
| fn_16_1088 (0xC8) | the render: the transform's Y column is the plane's normal and the position's dot with it the constant |

The retail's animation data flag byte (the guest CAnimData+0x2AC, 0x1194) has no member in the
host's CAnimData, so that one line is not ported.

Runtime (area 11, 2026-10-20, `WORLD=863fcd72,07_temple_mothlordsanctuary_dark`, `MP2_GODMODE=1`,
`send u04e3 Activate`, a temporary hook that forces the mode to 2 in the heart's health update,
reverted before the commit): the warp attack runs and **the portal actor appears** (16 of them
over the run, each created with the state manager's id and deleted again after 5.0f), the four
portal effects are created and the run ends on the frame limit with 0 caught/reach-stub.

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
constructor's values: x7c0_ (0), x7c4_ (0), `mJoints` (guest 0x7D4: the count, 0, then eight 0x48-byte slots at 0x7D8..0xA18),
xa3c_ (0.0f), xa4c_ (4.0f) and xa50_ (0.0f). The loader's data block ends at guest 0x6B4, so
none of them is a script value - they are the module's own state.

Not ported yet (milestone 2): the state functions' bodies, the tentacles and their collision
actors, the five attacks, the heart stage, the shock wave, the projectile and the two tokens.

**Entity type**: the REL does **not** override `TypesMatch` (vtable slot 1 is the DOL's
`TypesMatch__10CPatternedCFi`), so the actor keeps the base class's type (`kET_Patterned`) and
`CEntityInfo.hpp` gets no new value.

**Finding the actor's id**: the actor's unique id is not fixed - the area-11 run answers to
`u0687` here, while a smoke run found `u04e3`. Look it up instead of hard-coding it: the port
console's `objs` command lists the world's objects with their class, and
`objs emperoringstage1` prints the one line for this actor, e.g.
`u0687 000b0003  CEmperorIngStage1  a11 (1.7, -43.5, 105.0)  inactive`. The first column is the
id the console's `send <id> <message>` wants (`send u0687 Activate`).

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
CPatterned::Death's `SetState("Dead")`). ChooseRandomTentacleCount is a code node of the
heart stage (section 10), which that early scenario did not reach - so the four bodies ported
in this slice did not run then (they all run on the natural path now, section 12).

With the joint work ported (fn_16_2B68, fn_16_1E6C, fn_16_3984) the area load fills the joint
descriptions and the joint vector: the run reaches the element's copy (fn_16_39CC) and the tentacle actor lookup
(fn_16_3AA8), which finds the tentacle's collision actor in fn_16_3EAC's manager.

With the two animation builders ported (fn_16_30C8, fn_16_36E8) the `kSM_AreaLoaded` message
fills the three animation ids (guest 0xA20, 0xA24 and 0xA28) and the eight joint slots, and the
run reaches the joint stand-ins fn_16_2B68, fn_16_1E6C and fn_16_3984 - so the searches did find
animations.

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

### fn_16_3EAC (0x3EAC..0x4640): the three collision actor managers

Called from `kSM_AreaLoaded` (0x7E1C). Each manager is `rs_new(0x18)` + the manager's constructor
(area = this actor's, active = 1) and replaces the old one (single_ptr members; the destructor
fn_16_8B80 deletes them at 0x8CDC/0x8CF0/0x8D04).

| member | owner | descriptions | then |
|---|---|---|---|
| 0x7CC (heart) | the tracked actor, guest 0xA1A (CONNECT/ATTACH) | one `SphereCollision` on that actor's "brainSpine_scale" locator, radius 5.0 (rodata_3AC), "HeartCollision", mass 0.001 (rodata_3CC) | its actor (fn_16_2FA8) gets this actor's health info (slot 13 -> slot 12 copy, 0x404C..0x4098), `SetDamageVulnerability(*GetDamageVulnerability())` (slot 14, 0x40B8) and Target/Orbit/SeekerTarget (0x40D0) |
| 0x7D0 (body) | the 'PLAY' actor, guest 0xA1E | one `SphereCollision` on CSegId(1), radius 9.0 (rodata_3D0), "SphereCollision", mass 0.001 | its actor's filter: include Projectile(0x23)+Player(0x20), exclude Platform(0x31), CollisionActor(0x2F), NoStaticCollision(0x26), NoPlatformCollision(0x3C), type 3 (data_250..264) |
| 0x7C8 (tentacles) | this actor | 8 x 5 `OBBAutoSizeCollision` from lbl_16_rodata_0 ({from, to, extent}, stride 0xC/0x3C; extents 2.0/1.5/0.75/0.75/1.75; order one,two,three,four,five,seven,six,eight), name from+"->"+to, orientation 1, mass 10.0 (rodata_390) | - |

Then this actor's filter (include Solid 0x3B; exclude CollisionActor, Character, AIPassthrough,
Player; data_268..278), `RemoveMaterial(Target, Orbit, SeekerTarget)`, and fn_16_3D64 for 0..7
(off), 9 (on), 8 (off). fn_16_3D64 is ported (slice 2), so the tentacle boxes and the heart start
inactive.

**The damage vulnerability**: retail's is 0x54 bytes and the host's 0x38, but fn_16_3EAC copies
none of it: it passes the actor's own `GetDamageVulnerability()` (vtable slot 14) to
`CCollisionActor::SetDamageVulnerability`, both host types. The health info is likewise a host
`CHealthInfo` assignment.

**Runtime** (2026-10-20, `WORLD=863fcd72,07_temple_mothlordsanctuary_dark`, `MP2_GODMODE=1`,
`send <uid> Activate`): `objs collisionactor` lists 42 new collision actors in a11 (40 tentacle
boxes along the spines, the heart and the body sphere), 0 `[port] caught`/reach-stub. The FSM
went TrackPlayer -> BeginHeartStage in slice 1 only because `AllTentaclesRetracted` read a separate
count that was always 0. After the slice-2 fold (one `mJoints` at guest 0x7D4) ResetTentacles
(code function, runs: mode 0, count 8) puts tentacles out, so the walk is TrackPlayer -> Attack ->
ProjectileAttack (fn_16_68F4, a stand-in that never ends). BeginHeartStage/HeartStage are ported
but not reached until the tentacles can be retracted (fn_16_26B8 and the attack states).

## 6. The vtable's local slots

`lbl_16_data_3C8` (CEmperorIngStage1) has 80 slots: 82 words, two leading zeros, then 80
entries, of which **24 are the module's own** (`fn_16_*`); the rest are the inherited DOL entries,
spelled out by name in the table. The WarpPortal's own vtable `lbl_16_data_27C` has 36 slots (38
words, 2 + 36) and five local entries: the destructor `fn_16_0`, `Think` `fn_16_1150`,
`AcceptScriptMsg` `fn_16_1574`, `Render` `fn_16_1088` and `fn_16_60` (slot 13).

The module's local entries and what the host already does:

| slot | fn | host virtual | the port |
|---|---|---|---|
| 0 | `fn_16_8B80` | `~CEmperorIngStage1()` | ported |
| 3 | `fn_16_56D8` | `CEntity::Think` | ported |
| 4 | `fn_16_7CE8` | `CEntity::AcceptScriptMsg` | ported |
| 7 | `fn_16_52FC` | `CActor::PreRender` | ported (the base's body) |
| 9 | `fn_16_531C` | `CActor::Render` | ported |
| 13 | `fn_16_60` (0x2C) | `CActor::GetHealthInfo` | **not ported**: the retail body is the base's own (a virtual call to slot 12, `HealthInfo`), and so is the host's `CActor::GetHealthInfo`. Both module classes use it (it is slot 13 of `lbl_16_data_27C` too) |
| 16 | `fn_16_B958` (0x3C) | `CActor::GetTouchBounds` | **not ported**: the retail body is `CPhysicsActor::GetBoundingBox` in an `optional_object<CAABox>` (`fn_16_BA30`), which is what the host's `CPatterned::GetTouchBounds` returns |
| 26 | `fn_16_7F8C` | `CActor::DoUserAnimEvent` | ported |
| 28 | `fn_16_7EA8` | `CActor::ProcessSoundEvent` | ported |
| 39 | `fn_16_B994` | `CAi::TakeDamage` | the host's own |
| 40 | `fn_16_B9FC` | `CAi::CanBeShot` | ported (`true`) |
| 41 | `fn_16_BA04` | `CAi::IsListening` | ported (`false`) |
| 42 | `fn_16_BA0C` | `CAi::Listen` | the host's own |
| 43 | `fn_16_BA14` | `CAi::GetOrigin` | the host's own |
| 48 | `fn_16_B9A4` | `CPatterned::GetModelAlphau8` | the host's own |
| 53 | `fn_16_B9AC` | `CPatterned::ProjectileInfo` | the host's own |
| 54 | `fn_16_B9B4` | `CPatterned::GetSearchPath` | the host's own |
| 55 | `fn_16_B9BC` | `CPatterned::GetPointSearchPath` | the host's own |
| 58 | `fn_16_8AF4` | `CPatterned::SetupStateMachine` | ported |
| 59 | `fn_16_B9C4` | `CPatterned::GetRagDoll` | the host's own |
| 65 | `fn_16_B9CC` | `CPatterned::GetAttackTarget` | the host's own |
| 66 | `fn_16_B9DC` | `CPatterned::IsOnGround` | the host's own |
| 67 | `fn_16_B9E8` | `CPatterned::GetGravityConstant` | the host's own |
| 71 | `fn_16_B9F4` | `CPatterned::GetDeathExplosionParticle` | the host's own |

The module's **deleting destructors** are the slot 0 of the classes it creates. They are thunks:
the host's own classes destroy their members, so none of them is ported.

| fn | size | class vtable | destroys |
|---|---|---|---|
| `fn_16_48C` `fn_16_3340` `fn_16_3500` `fn_16_55D4` `fn_16_5610` `fn_16_8844` `fn_16_8880` | 0x3C | REL-local (no vtable in the REL `.data`) | nothing (free only) |
| `fn_16_2A68` `fn_16_2AB8` | 0x50/0x54 | REL-local | the counted reference release `fn_16_BD6C` |
| `fn_16_4DF0` | 0x54 | REL-local | the counted reference release `fn_16_BDD0` |
| `fn_16_87F0` `fn_16_9824` `fn_16_987C` `fn_16_96A4` `fn_16_97CC` | 0x54-0x5C | REL-local | a `CToken` |
| `fn_16_8784` `fn_16_A780` | 0x6C | REL-local | a `CToken`, only when the byte at +0xC is set |
| `fn_16_89B8` | 0x70 | REL-local | a `CToken` |
| `fn_16_A840` | 0x64 | REL-local | a `CAnimData` |
| `fn_16_47DC` | 0x5C | REL-local | the `rstl::string` at +0x2C |
| `fn_16_564C` `fn_16_9B04` | 0x8C | REL-local | the block whose size is the word at +0x00 |
| `fn_16_A904` | 0x7C | REL-local | the loader's script block (`SLdrActorParameters`, `SLdrPatternedAITypedef`, `SLdrEditorProperties`) |
| `fn_16_5D60` | 0x5C | `lbl_16_data_3A4` (a `CBodyStateCmd` subclass) | the vtable back to `lbl_16_data_3A4`, then the base's `lbl_16_data_3BC` |
| `fn_16_76F0` | 0x5C | `lbl_16_data_398` (a `CBodyStateCmd` subclass) | the vtable back to `lbl_16_data_398`, then the base's `lbl_16_data_3BC` |

`lbl_16_data_3BC` is the base `CBodyStateCmd` (its only virtual is the destructor) and
`lbl_16_data_3B0` a third subclass (`fn_16_5BA8`); the JumpRope state (fn_16_5928) hands them
to `CBodyStateCmdMgr::DeliverCmd` at 0x00005A60. The whole map (with the sizes) is in
`/tmp/opencode/emp1_vt.tsv`.

## 7. The helper chains the host already does

The module's own functions that are not vtable entries fall into chains whose work the host
does in the caller. Each one is cited on the port's code that replaces it and is not ported.

| chain | functions | what they do | the host's own |
|---|---|---|---|
| ← `fn_16_3EAC` | `fn_16_4838` `fn_16_48F4` | the joint vector's destructor and, per element, the release of the string at +0x2C | `rstl::vector` and `rstl::string` |
| ← `fn_16_22D0` | `fn_16_22F8` `fn_16_231C` `fn_16_2398` | the name comparison of the effect lookup (`fn_16_219C` -> `fn_16_2204` -> `fn_16_22B0` -> `fn_16_22D0`) | `rstl::find_by_key` |
| ← `fn_16_9BE8` | `fn_16_9E48` `fn_16_9FB0` `fn_16_A1D4` | the `CGameSplineDesc`'s and the `CMayaSpline`'s deleting destructors | the host's own classes |
| ← `fn_16_8B80` | `fn_16_8D40` `fn_16_8F24` `fn_16_8FB8` | `CPatterned`'s destructor, the `CAiKnockBackMgr`'s deleting destructor and the `rstl::vector<CSegId>`'s deleting destructor (the call at 0x8D38) | the host's `CPatterned`, `CAi` and `rstl::vector` |
| ← `fn_16_7F8C` | `fn_16_853C` `fn_16_85C8` `fn_16_867C` `fn_16_86BC` `fn_16_86DC` `fn_16_8704` `fn_16_8980` | the copy of the visor effect's particle descriptor into the projectile: the emitter's token and vectors, the `CToken`'s copy and lock (0x8704), and the flag (0x8980) | `CImpactVisorEffect::ParticleEffect` |
| ← `fn_16_601C` | `fn_16_6898` | the field-by-field copy of the `CRayCastResult` the ray cast returns | the host's assignment |
| ← `fn_16_A2A0` | `fn_16_A5BC` `fn_16_A5FC` `fn_16_A61C` `fn_16_A644` | the loader's `optional<CModelData>` copy and the `CModelData` copy constructor (the fields and the three locked tokens) | the host's `CModelData` copy |
| ← `fn_16_ABD0` | `fn_16_B130` `fn_16_B188` `fn_16_B1D8` `fn_16_B378` `fn_16_B3DC` `fn_16_B434` `fn_16_B52C` `fn_16_B584` `fn_16_B5C8` `fn_16_B668` `fn_16_B6BC` `fn_16_B6EC` `fn_16_B86C` | the data block's property readers: the `SLdr` structs' constructors and destructors and the per-type readers (the animation set, the damage info, the health info with its four vulnerabilities) | the generated `SLdrEmperorIngStage1.inc` |

## 8. The branch sweep (the area-11 hooks)

The branches the first run only entered early were driven with temporary hooks - the tested
flags, modes, timers and ids forced, the hooks and their once-logs reverted afterwards.

* fn_16_1C94 and fn_16_1C5C **are** called in the port, in the retail's order: the render path
  (fn_16_531C) asks for the flags (fn_16_1D38, 0x540C), the colour (fn_16_1C94, 0x545C), the
  plane (fn_16_1C5C, 0x5484) and the mask (fn_16_1CF4, 0x54C4) of each of the eight tentacle
  passes before RenderModelMultipleTimesWithFlags (0x54E4), exactly as the retail does. An entry
  once-log proved both run in area 11.
* fn_16_4E44 (the effect's particle system, called from fn_16_4C0C) and fn_16_3AA8 (the
  tentacle's collision actor) run in area 11: the first probe's once-log had sat after the
  return statement, and with it before the return both logged.
* fn_16_7F8C dispatches every event type: the hook forced the types 0..28 in turn and the log
  shows all 29. The retail's jump table (jumptable_16_data_314) has six case bodies - the types
  0 (fn_16_7F8C+0x98), 7 (+0x404), 17 (+0x418), 23 (+0x3C), 27 (+0x26C) and 28 (+0x360) - and
  the other 23 point at the default (+0x58C), the base's own DoUserAnimEvent, which is what the
  port's default case does.
* The module's own deleting destructors that the port never names - fn_16_4E68, fn_16_992C,
  fn_16_9B90, fn_16_A7EC, fn_16_A8A4, fn_16_A980, fn_16_AA70, fn_16_AAD4, fn_16_BA6C,
  fn_16_BB1C, fn_16_BD04, fn_16_BE34, fn_16_BF14 - and the five entries of the particle
  system's own vtable (lbl_16_data_510: fn_16_BE98, fn_16_BE9C, fn_16_BEA4, fn_16_BEB0 and
  fn_16_BEB8) are the host's own: its collision actor manager, its model data, its particle
  generator and its script loader destroy their members and dispatch their own tables.
* The ten ported bodies that had no evidence yet all run in area 11: an entry once-log
  (temporary, reverted) showed fn_16_1DFC, fn_16_52FC, fn_16_56D8, fn_16_7CE8, fn_16_8AF4,
  fn_16_8B80, fn_16_900C and fn_16_BA04; fn_16_0 (the warp portal's destructor) and fn_16_B9FC
  (CanBeShot) do not - the portal actor is never created there and nothing shoots the boss.
* The bodies area 11 never reaches were driven with temporary hooks (reverted): the follow-target
  emitter (fn_16_56D8) took the handle from the shock wave's travel sound (the kUE_BeginAction
  case, 0x84E8) and the boss as its target; the warp portal actor was spawned the way retail's
  WarpAttack does (0x6660), made active and sent its kSM_Create, so it thought (fn_16_1150),
  drew (fn_16_1088), was deleted (fn_16_0) and created the effect (fn_16_9BE8); the tentacle's
  portal started and stopped (fn_16_1A50, fn_16_1950); the FSM was forced into the heart stage
  (fn_16_774C) and the end of it (fn_16_71BC), which fired the beam (fn_16_CD0), updated it
  (fn_16_A64) and stopped it (fn_16_9C0) and evaluated the two triggers (fn_16_4BB4,
  fn_16_4B6C); and CanBeShot (fn_16_B9FC) was called. fn_16_4954 is not driven by a hook:
  the actor runs an FSM2 (section 10) and its code node hangs off the heart stage, which the
  natural fight reaches (section 12). fn_16_7EA8 (ProcessSoundEvent) was **not** unreachable: a run on
  2026-10-21 took its early return 5627 times and ran the base's own handling 11838 times, so
  it is marked now (section 11.3).

## 9. The branch table (the ten functions with conditional branches)

The once-logs (temporary, reverted) sat at the end of every arm of the C++ conditions; the
retail's offsets come from the asm. `L<n>` is the C++ line the arm's log stood on. The
retail's extra branches are the short-circuit arms of a C++ condition that is one
expression (`&&`, `||`), so they are not instrumented separately.

### fn_16_0 (2 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x001C | stw r30, 0x8(r1) | the C++ condition at L0 (a short-circuit arm) | n/a |
| 0x0038 | extsh. r0, r31 | the C++ condition at L0 (a short-circuit arm) | n/a |

### fn_16_9C0 (1 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x09F4 | cmplwi r3, 0x0 | L1477 then | no |

### fn_16_A64 (4 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x0AA8 | sth r0, 0x8(r1) | L1449 then | no |
| 0x0AC0 | lbz r0, 0x20(r31) | L1454 then | no |
| 0x0AD4 | fcmpo cr0, f1, f0 | L1463 then | yes |
| 0x0BEC | fsubs f1, f2, f1 | the C++ condition at L1463 (a short-circuit arm) | n/a |

### fn_16_CD0 (4 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x0D24 | mr r30, r4 | L1406 then | yes |
| 0x0DD4 | stb r6, 0x76(r1) | L1422 then | yes |
| 0x0EAC | extsb. r0, r28 | the C++ condition at L1422 (a short-circuit arm) | n/a |
| 0x0EBC | cmplwi r31, 0x0 | the C++ condition at L1422 (a short-circuit arm) | n/a |

### fn_16_71BC (10 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x7204 | mr r28, r3 | L2400 then | yes |
| 0x7208 | mr r28, r3 | L2420 then | yes |
| 0x7210 | cmpwi r27, 0x0 | the C++ condition at L2420 (a short-circuit arm) | n/a |
| 0x721C | cmpwi r27, 0x3 | the C++ condition at L2420 (a short-circuit arm) | n/a |
| 0x7354 | addi r3, r29, 0x6b4 | the C++ condition at L2420 (a short-circuit arm) | n/a |
| 0x7360 | cmpwi r0, 0x1 | the C++ condition at L2420 (a short-circuit arm) | n/a |
| 0x7364 | cmpwi r0, 0x1 | the C++ condition at L2420 (a short-circuit arm) | n/a |
| 0x736C | cmpwi r0, 0x0 | the C++ condition at L2420 (a short-circuit arm) | n/a |
| 0x7378 | cmpwi r0, 0x3 | the C++ condition at L2420 (a short-circuit arm) | n/a |
| 0x763C | addi r3, r1, 0x20 | the C++ condition at L2420 (a short-circuit arm) | n/a |

### fn_16_774C (8 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x7778 | mr r31, r4 | L2344 then | yes |
| 0x777C | mr r31, r4 | L2349 then | no |
| 0x7784 | cmpwi r5, 0x0 | the C++ condition at L2349 (a short-circuit arm) | n/a |
| 0x7790 | cmpwi r5, 0x3 | the C++ condition at L2349 (a short-circuit arm) | n/a |
| 0x77D8 | fcmpo cr0, f2, f0 | the C++ condition at L2349 (a short-circuit arm) | n/a |
| 0x77E4 | fcmpo cr0, f2, f0 | the C++ condition at L2349 (a short-circuit arm) | n/a |
| 0x7820 | lhz r0, kInvalidUniqueId@l(r3) | the C++ condition at L2349 (a short-circuit arm) | n/a |
| 0x7834 | fcmpo cr0, f1, f0 | the C++ condition at L2349 (a short-circuit arm) | n/a |

### fn_16_1150 (13 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x11D0 | fcmpo cr0, f1, f0 | L2020 then | yes |
| 0x11F8 | cmplwi r4, 0x0 | L2024 then | no |
| 0x1204 | cmplwi r0, 0x0 | L2041 then | yes |
| 0x1210 | cmplwi r0, 0x0 | L2046 then | yes |
| 0x1254 | addi r3, r1, 0x90 | the C++ condition at L2046 (a short-circuit arm) | n/a |
| 0x128C | sth r0, 0x38(r1) | the C++ condition at L2046 (a short-circuit arm) | n/a |
| 0x1298 | lbz r0, 0x20(r29) | the C++ condition at L2046 (a short-circuit arm) | n/a |
| 0x132C | fcmpo cr0, f1, f0 | the C++ condition at L2046 (a short-circuit arm) | n/a |
| 0x1368 | cmplwi r0, 0x0 | the C++ condition at L2046 (a short-circuit arm) | n/a |
| 0x1390 | stb r0, 0x120(r1) | the C++ condition at L2046 (a short-circuit arm) | n/a |
| 0x13CC | cmplwi r0, 0x0 | the C++ condition at L2046 (a short-circuit arm) | n/a |
| 0x1494 | addi r4, r1, 0xf0 | the C++ condition at L2046 (a short-circuit arm) | n/a |
| 0x1530 | cmpwi r28, 0x7 | the C++ condition at L2046 (a short-circuit arm) | n/a |

### fn_16_1A50 (3 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x1A78 | cmpwi r0, 0x0 | L1925 then | no |
| 0x1ACC | lhz r0, kInvalidUniqueId@l(r3) | L1931 then | yes |
| 0x1B18 | sth r0, 0x18(r1) | L1938 then | no |

### fn_16_1950 (1 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x19D8 | cmplwi r3, 0x0 | L2135 then | no |

### fn_16_9BE8 (4 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x9CD4 | addi r4, r3, 0x60b | L1906 then | yes |
| 0x9DEC | extsb. r0, r24 | the C++ condition at L1906 (a short-circuit arm) | n/a |
| 0x9DFC | extsb. r0, r25 | the C++ condition at L1906 (a short-circuit arm) | n/a |
| 0x9E10 | extsb. r0, r26 | the C++ condition at L1906 (a short-circuit arm) | n/a |

The arms that did not come up: fn_16_0: none; fn_16_9C0: 0x09F4; fn_16_A64: 0x0AA8, 0x0AC0; fn_16_CD0: none; fn_16_71BC: none; fn_16_774C: 0x777C; fn_16_1150: 0x11F8; fn_16_1A50: 0x1A78, 0x1B18; fn_16_1950: 0x19D8; fn_16_9BE8: none.

## 9. The branch table (the ten functions with conditional branches)

The once-logs (temporary, reverted) sat at the end of every arm of the C++ conditions; the
retail's offsets come from the asm. `L<n>` is the C++ line the arm's log stood on. The
retail's extra branches are the short-circuit arms of a C++ condition that is one
expression (`&&`, `||`), so they are not instrumented separately.

### fn_16_0 (2 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x001C | stw r30, 0x8(r1) | the C++ condition at L0 (a short-circuit arm) | n/a |
| 0x0038 | extsh. r0, r31 | the C++ condition at L0 (a short-circuit arm) | n/a |

### fn_16_9C0 (1 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x09F4 | cmplwi r3, 0x0 | L1478 then | yes |

### fn_16_A64 (4 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x0AA8 | sth r0, 0x8(r1) | L1450 then | yes |
| 0x0AC0 | lbz r0, 0x20(r31) | L1455 then | yes |
| 0x0AD4 | fcmpo cr0, f1, f0 | L1464 then | yes |
| 0x0BEC | fsubs f1, f2, f1 | the C++ condition at L1464 (a short-circuit arm) | n/a |

### fn_16_CD0 (4 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x0D24 | mr r30, r4 | L1407 then | yes |
| 0x0DD4 | stb r6, 0x76(r1) | L1423 then | yes |
| 0x0EAC | extsb. r0, r28 | the C++ condition at L1423 (a short-circuit arm) | n/a |
| 0x0EBC | cmplwi r31, 0x0 | the C++ condition at L1423 (a short-circuit arm) | n/a |

### fn_16_71BC (10 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x7204 | mr r28, r3 | L2403 then | yes |
| 0x7208 | mr r28, r3 | L2423 then | yes |
| 0x7210 | cmpwi r27, 0x0 | the C++ condition at L2423 (a short-circuit arm) | n/a |
| 0x721C | cmpwi r27, 0x3 | the C++ condition at L2423 (a short-circuit arm) | n/a |
| 0x7354 | addi r3, r29, 0x6b4 | the C++ condition at L2423 (a short-circuit arm) | n/a |
| 0x7360 | cmpwi r0, 0x1 | the C++ condition at L2423 (a short-circuit arm) | n/a |
| 0x7364 | cmpwi r0, 0x1 | the C++ condition at L2423 (a short-circuit arm) | n/a |
| 0x736C | cmpwi r0, 0x0 | the C++ condition at L2423 (a short-circuit arm) | n/a |
| 0x7378 | cmpwi r0, 0x3 | the C++ condition at L2423 (a short-circuit arm) | n/a |
| 0x763C | addi r3, r1, 0x20 | the C++ condition at L2423 (a short-circuit arm) | n/a |

### fn_16_774C (8 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x7778 | mr r31, r4 | L2346 then | yes |
| 0x777C | mr r31, r4 | L2351 then | yes |
| 0x7784 | cmpwi r5, 0x0 | the C++ condition at L2351 (a short-circuit arm) | n/a |
| 0x7790 | cmpwi r5, 0x3 | the C++ condition at L2351 (a short-circuit arm) | n/a |
| 0x77D8 | fcmpo cr0, f2, f0 | the C++ condition at L2351 (a short-circuit arm) | n/a |
| 0x77E4 | fcmpo cr0, f2, f0 | the C++ condition at L2351 (a short-circuit arm) | n/a |
| 0x7820 | lhz r0, kInvalidUniqueId@l(r3) | the C++ condition at L2351 (a short-circuit arm) | n/a |
| 0x7834 | fcmpo cr0, f1, f0 | the C++ condition at L2351 (a short-circuit arm) | n/a |

### fn_16_1150 (13 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x11D0 | fcmpo cr0, f1, f0 | L2022 then | yes |
| 0x11F8 | cmplwi r4, 0x0 | L2026 then | no |
| 0x1204 | cmplwi r0, 0x0 | L2043 then | yes |
| 0x1210 | cmplwi r0, 0x0 | L2048 then | yes |
| 0x1254 | addi r3, r1, 0x90 | the C++ condition at L2048 (a short-circuit arm) | n/a |
| 0x128C | sth r0, 0x38(r1) | the C++ condition at L2048 (a short-circuit arm) | n/a |
| 0x1298 | lbz r0, 0x20(r29) | the C++ condition at L2048 (a short-circuit arm) | n/a |
| 0x132C | fcmpo cr0, f1, f0 | the C++ condition at L2048 (a short-circuit arm) | n/a |
| 0x1368 | cmplwi r0, 0x0 | the C++ condition at L2048 (a short-circuit arm) | n/a |
| 0x1390 | stb r0, 0x120(r1) | the C++ condition at L2048 (a short-circuit arm) | n/a |
| 0x13CC | cmplwi r0, 0x0 | the C++ condition at L2048 (a short-circuit arm) | n/a |
| 0x1494 | addi r4, r1, 0xf0 | the C++ condition at L2048 (a short-circuit arm) | n/a |
| 0x1530 | cmpwi r28, 0x7 | the C++ condition at L2048 (a short-circuit arm) | n/a |

### fn_16_1A50 (3 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x1A78 | cmpwi r0, 0x0 | L1926 then | yes |
| 0x1ACC | lhz r0, kInvalidUniqueId@l(r3) | L1932 then | yes |
| 0x1B18 | sth r0, 0x18(r1) | L1939 then | yes |

### fn_16_1950 (1 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x19D8 | cmplwi r3, 0x0 | L2137 then | yes |

### fn_16_9BE8 (4 branches)
| retail offset | the retail's test | the C++ arm | ran |
|---|---|---|---|
| 0x9CD4 | addi r4, r3, 0x60b | L1907 then | yes |
| 0x9DEC | extsb. r0, r24 | the C++ condition at L1907 (a short-circuit arm) | n/a |
| 0x9DFC | extsb. r0, r25 | the C++ condition at L1907 (a short-circuit arm) | n/a |
| 0x9E10 | extsb. r0, r26 | the C++ condition at L1907 (a short-circuit arm) | n/a |

The arms that did not come up: fn_16_0: none; fn_16_9C0: none; fn_16_A64: none; fn_16_CD0: none; fn_16_71BC: none; fn_16_774C: none; fn_16_1150: 0x11F8; fn_16_1A50: none; fn_16_1950: none; fn_16_9BE8: none.

## 10. The state machine is an FSM2 one, and the code nodes are dispatched

The retail's `CPatterned` has **one** state-machine runtime, at `this + 0x350`. The DOL's
constructor (0x80079E38) picks the FSM2 runtime (`__ct__17CGenericFSM2StateFv`, 0x54 bytes)
unless the patterned info's second machine id is -2, in which case it creates the FSM1 runtime
(`fn_801956BC`, 0x40 bytes, vtable `lbl_803B5D28`). The host does the same
(`src/MetroidPrime/Enemies/CPatterned.cpp:84`). **This actor takes the FSM2 path**: a probe of
the runtime's `GetType()` in area 11 gives 1.

So the module's three code functions are dispatched - by the host's FSM2
(`CGenericFSM2State::CallCode`, `include/MetroidPrime/CGenericFSM2StateImpl.hpp:233`), and by
the retail's FSM2. The host's **FSM1** does not dispatch code nodes
(`TStateMachineState::SetCodeFunctions`, `include/MetroidPrime/TStateMachineState.hpp:304`, is
an empty body) - and neither does the retail's FSM1: the FSM1 runtime's vtable `lbl_803B5D28`
slot 0x1C is `fn_80194BF0` (0x80194BF0), a single `blr`, and the runtime has only two function
vectors (the states at +0x10, the triggers at +0x20); its update (`fn_80194F38`, 0x80194F38)
calls only the state and the trigger functions.

The shipped FSM2 machine of this actor has 10 states, 4 codes and 12 triggers (the dump below
is the machine the actor runs, printed from the host's `CGenericFSM2`):

| node | goes to |
|---|---|
| state "Start" | code "ResetTentacles" |
| state "TrackPlayer" | triggers "ShouldAttack", "ShouldMeleeAttack", "ShouldTaunt", "AllTentaclesRetracted" |
| state "BeginHeartStage" | trigger "AnimOver" |
| state "HeartStage" | triggers "HeartStageOver", "IsDead" |
| state "EndHeartStage" | trigger "AnimOver" |
| state "MeleeAttack" | triggers "AllTentaclesRetracted", "AnimOver" |
| state "Attack" | triggers "AllTentaclesRetracted", "AnimOver" |
| state "Taunt" | triggers "AnimOver", "AllTentaclesRetracted" |
| state "Dead" | - |
| code "BeginSubStage" | state "TrackPlayer" |
| code "ResetTentacles" (two of them) | code "BeginSubStage", state "Taunt" |
| code "ChooseRandomTentacleCount" | state "EndHeartStage" |
| trigger "AllTentaclesRetracted" | state "BeginHeartStage" |
| trigger "HeartStageOver" | code "ChooseRandomTentacleCount" |
| trigger "AnimOver" (four of them) | state "HeartStage", state "TrackPlayer" (twice), code "ResetTentacles" |
| trigger "ShouldMeleeAttack" | state "MeleeAttack" |
| trigger "ShouldAttack" | state "Attack" |
| trigger "ShouldTaunt" | state "Taunt" |
| trigger "IsDead" | state "Dead" |

**fn_16_4954 (ChooseRandomTentacleCount)** therefore hangs off the "HeartStageOver" trigger of
the "HeartStage" state: it runs only when the fight reaches the heart stage (the FSM2's
"AllTentaclesRetracted" -> "BeginHeartStage" -> "AnimOver" -> "HeartStage"), and the natural
fight does get there once the tentacles' own boxes are hit and the 'PLAY' actor is active
(sections 15-20; the gcov run in section 12). The other two code functions do run: "ResetTentacles" and "BeginSubStage" once each, at
the "Start" state. Nothing in the module's own code blocks fn_16_4954: the module's table
(lbl_16_data_220) and the machine's code node have the same name, and the host's FSM2 binds
them. There is no dispatch gap: the host's FSM2 calls the code node (section 12's gcov run).

## 11. The classification of the unmarked functions

Every function of the REL that carries no `// @` marker is listed here with its class:
**(a)** a copy of host/DOL code (compiler-generated, rstl, a weak destructor, a thunk, the REL
runtime) - never marked, the DOL address is cited instead; **(b)** a behaviour function of the
module - marked once a run exercises it.

### 11.1 The eight functions the map lists but no other section classifies

| fn | what it is | class |
|---|---|---|
| `fn_16_920` | the tracked actor's "brainSpine_scale" locator position | (b) - marked |
| `fn_16_1E20` | one slot's tentacle collision actor gets the damage vulnerability | (b) - marked |
| `fn_16_2C3C` | the turn angle against the 'PLAY' actor's own Y column | (b) - marked |
| `fn_16_2C9C` | the signed turn angle between the 'PLAY' actor's facing and a direction | (b) - marked |
| `fn_16_2DDC` | the REL's own copy of the DOL's `Cross__9CVector3fFRC9CVector3fRC9CVector3f` (0x8001994C, 0x40 bytes - the same body, branch free) | (a) - the marker was taken off; the host's `CVector3f::Cross` is the same code |
| `fn_16_9610` | the compiler's copy constructor of the 0x48-byte `SLdrPlasmaBeamInfo` | (a) - the host's own copy assignment |
| `fn_16_98D0` | the copy of the `SLdrDamageVulnerability` into the `CDamageVulnerability` - the DOL's `LdrToDamageVulnerability__FRC23SLdrDamageVulnerability` (0x8023ACFC) | (a) - the host's own `LdrToDamageVulnerability` |
| `fn_16_A270` | the REL's module head: it stores the loader `fn_16_A2A0` into `lbl_16_bss_0` and calls the DOL's `fn_8022756C` (0x8022756C, the store into `gLoader_EmperorIngStage1`) | (a) - the REL runtime |

### 11.2 The rest of the unmarked functions

| class | the functions |
|---|---|
| (a) the module's own deleting destructors and the free-only thunks | `fn_16_43C` `fn_16_48C` `fn_16_2A68` `fn_16_2AB8` `fn_16_3340` `fn_16_3500` `fn_16_47DC` `fn_16_4838` `fn_16_48BC` `fn_16_48F4` `fn_16_4DF0` `fn_16_4E68` `fn_16_55D4` `fn_16_5610` `fn_16_564C` `fn_16_5BA8` `fn_16_5C04` `fn_16_5D60` `fn_16_67EC` `fn_16_6844` `fn_16_76F0` `fn_16_8784` `fn_16_87F0` `fn_16_8844` `fn_16_8880` `fn_16_88BC` `fn_16_8918` `fn_16_8938` `fn_16_8A28` `fn_16_8AA0` `fn_16_8D40` `fn_16_8F24` `fn_16_8FB8` `fn_16_96A4` `fn_16_97CC` `fn_16_9824` `fn_16_987C` `fn_16_992C` `fn_16_9B04` `fn_16_9B90` `fn_16_9E48` `fn_16_9FB0` `fn_16_A1D4` `fn_16_A780` `fn_16_A7EC` `fn_16_A840` `fn_16_A8A4` `fn_16_A904` `fn_16_AA70` `fn_16_AAD4` `fn_16_B130` `fn_16_B188` `fn_16_B378` `fn_16_B3DC` `fn_16_B52C` `fn_16_B584` `fn_16_B668` `fn_16_B6BC` `fn_16_B86C` `fn_16_B8F4` `fn_16_BB1C` `fn_16_BC44` `fn_16_BD04` `fn_16_BD6C` `fn_16_BDD0` `fn_16_BE34` `fn_16_BE98` `fn_16_BE9C` `fn_16_BEA4` `fn_16_BEB0` `fn_16_BEB8` `fn_16_BF14` |
| (a) the rstl and compiler-generated helpers | `fn_16_219C` `fn_16_2204` `fn_16_22B0` `fn_16_22D0` `fn_16_22F8` `fn_16_231C` `fn_16_2398` `fn_16_337C` `fn_16_39CC` `fn_16_39EC` `fn_16_3A14` `fn_16_4640` `fn_16_46D4` `fn_16_46F4` `fn_16_471C` `fn_16_9980` `fn_16_9AB0` `fn_16_9E9C` `fn_16_9F04` `fn_16_A004` `fn_16_A644` `fn_16_BA6C` `fn_16_BB3C` `fn_16_FF4` |
| (a) the loader's readers and the model's copies | `fn_16_A2A0` `fn_16_A5BC` `fn_16_A5FC` `fn_16_A61C` `fn_16_A980` `fn_16_ABD0` `fn_16_AEDC` `fn_16_AF7C` `fn_16_B014` `fn_16_B1D8` `fn_16_B434` `fn_16_B5C8` `fn_16_B6EC` `fn_16_9700` |
| (a) the base classes' own bodies | `fn_16_60` (`CActor::GetHealthInfo` - a thunk to the base's slot 12), `fn_16_B958` (`CActor::GetTouchBounds` - the base's `GetBoundingBox` in an `optional`), `fn_16_BA30` (that `optional`'s builder), `fn_16_18E8` (the `SMoverData` copy constructor), `fn_16_6898` (the `CRayCastResult` copy) |
| (a) the REL runtime | `fn_16_A270`, `fn_16_BF88` (the static initialiser of the three FSM tables), `fn_16_C3B0` (`ModuleDestructors`), `fn_16_C3FC` (`ModuleConstructors`), `fn_16_469C` (`push_back_unsafe`), `RELMain`, `RELExit` |
| (b) the module's behaviour functions | `fn_16_3060` (the 'PLAY' actor - the same body as the marked `fn_16_3094`), `fn_16_7EA8` (`ProcessSoundEvent` - **marked now**, both arms ran), `fn_16_4954` (`ChooseRandomTentacleCount` - **marked now**, section 12) |

### 11.3 The branch tables of the five most-called unmarked functions

The call counts are the static `bl` counts in the REL's own text. `L<n>` is the C++ line the
probe stood on.

#### fn_16_BD6C (6 calls) - (a) the counted reference release

| retail offset | the retail's test | class |
|---|---|---|
| 0xBD90 | `bgt .L_0000BDBC` | (a) the reference count's test - the host's `ncrc_ptr` release |
| 0xBD9C | `beq .L_0000BDB4` | (a) the count is zero -> only the free |

#### fn_16_337C (6 calls) - (a) the CPASAnimParmData's parm list

| retail offset | the retail's test | class |
|---|---|---|
| - | no branch | (a) branch free; the host's `CPASAnimParmData` constructor fills the list |

#### fn_16_BDD0 (5 calls) - (a) the counted reference release

| retail offset | the retail's test | class |
|---|---|---|
| 0xBDF4 | `bgt .L_0000BE20` | (a) the reference count's test |
| 0xBE00 | `beq .L_0000BE18` | (a) the count is zero -> only the free |

#### fn_16_3060 (5 calls) - (b) the 'PLAY' actor

| retail offset | the retail's test | class |
|---|---|---|
| - | no branch | (b) branch free (a lookup and a cast); the body is the marked `fn_16_3094`'s, so no second marker |

#### fn_16_98D0 (4 calls) - (a) the damage vulnerability copy

| retail offset | the retail's test | class |
|---|---|---|
| - | no branch | (a) branch free; the DOL's `LdrToDamageVulnerability` (0x8023ACFC) |

#### fn_16_7EA8 (the most-called one at runtime: 17465 calls) - (b) `ProcessSoundEvent`, marked

| retail offset | the retail's test | class |
|---|---|---|
| 0x7F10 | `beq .L_00007F28` (the tentacle index is -1) | (b) ran - the base's own `ProcessSoundEvent` (11838 calls) |
| 0x7F24 | `beq .L_00007F70` (the joint's flag byte has its bit 27 clear) | (b) ran - 5627 early returns (the tentacle index 0..5, the flag byte 0x00, 0x60, 0x80) |

#### fn_16_4954 - (b) `ChooseRandomTentacleCount`: marked (section 12)

| retail offset | the retail's test | class |
|---|---|---|
| 0x4988 | `beq .L_000049C4` (the mode is 1) | (b) ran (forced) |
| 0x498C | `bge .L_000049E8` (the mode is 2 or more) | (b) ran (forced) |
| 0x4994 | `bge .L_000049A0` (the mode is 0) | (b) ran (forced) |
| 0x4998 | `b .L_000049E8` (the mode is below 0) | (c) unreachable in shipped data |
| 0x49AC | `bge .L_000049B8` (the roll is 50 or more) | (b) ran (forced) |
| 0x49B4 | `b .L_000049BC` (the roll is below 50) | (b) ran (forced) |
| 0x49D0 | `bge .L_000049DC` (the roll is 50 or more) | (b) ran (forced) |
| 0x49D8 | `b .L_000049E0` (the roll is below 50) | (b) ran (forced) |

## 12. fn_16_4954 (ChooseRandomTentacleCount) is marked - on the natural path

The code node is the FSM2 machine's, hanging off the "HeartStageOver" trigger of the
"HeartStage" state (section 10), so it runs when the heart-stage timer (guest 0xA3C) runs out.
**The host dispatches it and the natural fight reaches it**: there is no FSM1/FSM2 code-node
gap (this actor runs the FSM2, section 10, and `CGenericFSM2State::CallCode` calls the node).
The earlier "unreachable" claims came from runs that never killed the tentacles (the smoke
family's `cmd_emp1.txt` only activates the boss, so its coverage never shows the heart stage).

The run (2026-10-22, coverage build `build-port-cov` of 275125673, `WORLD=863fcd72,
07_temple_mothlordsanctuary_dark`, `MP2_GODMODE=1`, `MP2_FRAME_LIMIT=0 MP2_TICKS_PER_FRAME=8`,
30000 updates, `tools/drivers/emperoring1_heartstage.py <port> <out> cycles 0 0 0 0 250 0 0 250 0`,
no `fsm`, 0 `[port] caught`/`reach-stub`): the driver activates the boss and the 'PLAY' actor,
hits the eight health-bearing tentacle boxes on every poll until the FSM walks
"AllTentaclesRetracted" -> "BeginHeartStage" -> "HeartStage", hits the heart's collision actor
with that cycle's damage there, and waits for "EndHeartStage".  Nine heart stages ran (f910,
f2327, f3887, f5326, f6831, f8310, f9809, f11299, f12753), four at 699 hp (mode 0), three at
449 hp (mode 1, below the 466 of 0x527C), two at 199 hp (mode 2, below the 233 of 0x5294).

`tools/cov/gcov_report.py build-port-cov <run>/gcov --module EmperorIngStage1 --fn fn_16_4954`:
`9/10 lines, 9/10 branches, untaken @ 3058:1/2` - the one untaken branch is the false side of
`x7c0_ >= 0`, the mode < 0 arm.

| retail offset | the retail's test | class |
|---|---|---|
| 0x4988 | `beq .L_000049C4` (the mode is 1) | (b) ran - the mode 1 cycles |
| 0x498C | `bge .L_000049E8` (the mode is 2 or more) | (b) ran - the mode 2 cycles |
| 0x4994 | `bge .L_000049A0` (the mode is 0) | (b) ran - the mode 0 cycles |
| 0x4998 | `b .L_000049E8` (the mode is below 0) | (c) dead: the only stores to guest 0x7C0 are 0x909C (`li r6,0` at 0x9088, the constructor), 0x5284 (`li r0,1`) and 0x529C (`li r0,2`) |
| 0x49AC | `bge .L_000049B8` (the roll is 50 or more, the mode 0) | (b) ran - both sides (gcov: no untaken branch on the mode 0 line) |
| 0x49D0 | `bge .L_000049DC` (the roll is 50 or more, the mode 1) | (b) ran - both sides (gcov: no untaken branch on the mode 1 line) |

The same run covers the rest of the heart stage on the natural path: fn_16_920, fn_16_A64 and
fn_16_9BE8 ALL-ARMS; fn_16_CD0, fn_16_9C0, fn_16_774C, fn_16_71BC, fn_16_78AC, fn_16_4BB4,
fn_16_4B6C, fn_16_1A50, fn_16_1950, fn_16_1150 and the portal's constructor fn_16_1720 ran (the
untaken branches are the states' default message arms and the arms the hooks of section 8
forced).  fn_16_1088 (the warp portal's Render) still did not run (0/6 lines) although the
portal is created.

## 13. Why a hit on the collision actors does not destroy the tentacles

The tentacle's health is on **the tentacle's own collision actor**: the manager at guest 0x7C8
holds five descriptions per tentacle and fn_16_3AA8 takes the last of them (4 + index * 5,
0x3AB4/0x3AC0), fn_16_1E6C copies the module's tentacle health (guest 0xAAC, 40 hp) into it
(0x1EB8..0x1F04) and fn_16_2524 reads it back (0x2584), compares it with the joint's last health
(+0x20) and, at 0 or below, clears the joint's shootable flag (0x10), sets the tentacle's state
to 3 and calls fn_16_3BC4 (the IS03 message).  The other four boxes of each tentacle (the
segments) have **no health info at all** - `obj` prints no hp for 32 of the 40 boxes - so a hit
on them does nothing, which is what made "hurt the collision actors" look broken.  The tentacle
also only watches its health while the joint's flag 0x10 is set (fn_16_1FC0 sets it when the
tentacle comes out and clears it when it dies), and a hit **before** the tentacle is out is
undone: fn_16_1FC0 restores the health to the initial 40 while the flag is clear.  Only the
tentacles of the current sub-stage (the first four, fn_16_2F70) have the flag set, so a hit on
the fifth tentacle's box does nothing either.  The retail sends no message for the damage: the
module polls the collision actor's health itself; the only message is the IS03 of the dead
tentacle.  A run (2026-10-21) that hit all eight boxes killed exactly the three tentacles of
the live sub-stage whose flag was set ("tentacle 0/1/2 dead" in the probe log) and left the
others at 40 hp.

## 14. The branch tables of the five most-called unmarked functions

The call counts are the static `bl` counts in the REL's own text; the class is (a) a copy of
host/DOL code, (b) the module's own behaviour, (c) unreachable in the shipped data, (d)
reachable but not run.

| fn | calls | class | branches |
|---|---|---|---|
| `fn_16_BD6C` | 6 | (a) the counted reference release (the host's `ncrc_ptr`) | 0xBD90 `bgt` (the count is above 1), 0xBD9C `beq` (the count is 0 -> only the free) |
| `fn_16_337C` | 6 | (a) the CPASAnimParmData's parm list (the host's constructor fills it) | none - branch free |
| `fn_16_BDD0` | 5 | (a) the counted reference release | 0xBDF4 `bgt`, 0xBE00 `beq` |
| `fn_16_3060` | 5 | (b) the 'PLAY' actor - the same body as the marked `fn_16_3094`, so no second marker | none - branch free (a lookup and a cast) |
| `fn_16_98D0` | 4 | (a) the damage vulnerability copy (the DOL's `LdrToDamageVulnerability`, 0x8023ACFC) | none - branch free |
| `fn_16_471C` | 4 | (a) the `CJointCollisionDescription`'s copy constructor | none - branch free |
| `fn_16_6844` | 4 | (a) the connection list's destructor | none - branch free |
| `fn_16_4640` | 2 | (a) the description's name (a string copy and an append) | none - branch free |

## 15. The tentacle's state 1 -> 2: the joint animation searches had their parameters in the wrong order

**The bug (module-local, fixed)**: fn_16_36E8 builds the three animation parameter sets of every
joint and searches the PAS database with them.  The retail's parameter ORDER is
**FromInt32(the index) first and the FromEnum second**: fn_16_337C gets r5 = the FromInt32
(0x378C, 0x382C, 0x38CC) and r6 = the FromEnum (0x3790, 0x3830, 0x38D0), and the second search's
FromInt32 is the index too (r22 at 0x3818), not the constant 2.  The port had the FromEnum first
and the constant 2 in the second search, so the searches answered with the **same** animation for
several tentacles: the joints 0..2 got (10, 21, 22), (11, 21, 23) and (12, 21, 1), and the joints
3..7 all got (10, 21, 1) - the joint 0's "out" animation id 10 among them.

The parameter list is matched positionally, so the collision had a visible effect: the joint 3's
state-2 arm (fn_16_26B8, 0x28A0: it deletes all three animations) deleted the anim id 10, which
was joint 0's "out" animation.  Joint 0's state-1 arm then found its animation gone
(`IsAdditiveAnimationActive` false, 0x2798), added it again (0x2808) and the animation restarted -
so the tentacle never left the state 1 and the "out" animation never reached its end (the state
becomes 2 when the tree has no time left, 0x2884).  The tentacles stayed half out forever, and
with them the FSM never reached "AllTentaclesRetracted" (fn_16_4BD4) and so never the heart
stage.

After the fix every joint has its own three animation ids (10/12/11, 22/1/23, 19/21/20, 7/9/8,
4/6/5, 13/15/14, 16/18/17, 24/3/2), the tentacle's out animation plays to its end (the tree's
time 0.500 -> 0.000) and the tentacle reaches the state 2.

**The natural path now works up to the heart stage**: a run that hits the health-bearing
collision actor of every tentacle (the tentacle's own box, section 13) kills them, the FSM walks
"Attack" -> "BeginHeartStage" on its own (the console's `obj` prints the state), and the code node
of section 12 is one step further: the "BeginHeartStage" state's "AnimOver" trigger does not fire
in the port (the state plays the animation on the connected 'PLAY' actor, while the trigger
watches the boss's own animation state), so the FSM stays in "BeginHeartStage" and the
"HeartStage" state is still entered by forcing it.  That trigger is the next thing to look at; the
fn_16_4954 marker stands on the forced run.

## 16. The "AnimOver" trigger of "BeginHeartStage" - what it reads and why it does not fire

The trigger is the DOL's `CPatterned::GetAnimOver` (fn_80151990, 0x80151990): it reads the
**owner's own** animation state (the word at the actor + 0x6B4) and answers `state == 3`
(kAS_Over).  The host's `CPatterned::GetAnimOver`/`AnimOver` does the same through
`mAnimationState.IsOver()`.  So the trigger watches **the boss's own** animation state, not the
connected 'PLAY' actor's.

The state writes that word: fn_16_78AC's update (0x7A3C) builds the string "Whole Body"
(lbl_16_rodata_41C+0x627, 0x7A4C) and calls `IsAnimTimeRemaining(dt, "Whole Body")` on the
**'PLAY' actor's** animation data (0x7A50: the actor at 0x60(r28) -> its CAnimData at +0x10;
the call at 0x7A60), and when it answers false the boss's own animation state becomes 3
(kAS_Over, 0x7A7C).  The port's `BeginHeartStage` does exactly that (`IsAnimTimeRemaining(dt,
"Whole Body")` on the 'PLAY' actor, then `mAnimationState.SetState(kAS_Over)`), so **the port
matches the retail here** - there is no mismatch to fix in the module.

Why the trigger does not fire in the port's area-11 scenario (2026-10-21, probe on the state):

* the 'PLAY' actor is the CScriptActor the body is played on (the probe named it: uid u04e8,
  the CScriptActor at the body's position (1.7, -43.5, 98.1); the boss is u04e3 at
  (1.7, -43.5, 105.0)).  It is **inactive**: the area's script activates it when the fight
  starts, and the console's `send u04e3 Activate` only reaches the boss.  An inactive
  CScriptActor does not think (`CScriptActor::Think`, src/MetroidPrime/ScriptObjects/
  CScriptActor.cpp:89, returns at once when `!GetActive()`), so its animation never runs.
  The driver now sends it the message too (`send u04e8 Activate`).
* even then the animation never ends: the host's `CAnimData::IsAnimTimeRemaining`
  (src/MetroidPrime/CAnimData.cpp:668) **ignores the "Whole Body" name** and answers with the
  *root* animation's time remaining, while the retail's takes the animation name - so "the named
  animation has ended" cannot be asked on the host.  That is shared code and is not changed
  here; it is reported for the orchestrator.

So the natural heart stage is still one step away: the tentacles are killed and the FSM walks
into "BeginHeartStage" on its own (section 15), but the state's "AnimOver" trigger needs the
'PLAY' actor's named animation to end, which the host cannot answer.

## 17. The 'PLAY' actor's "Whole Body" animation never ends: it loops

Probe on the state (2026-10-21, the 'PLAY' actor = the CScriptActor u04e8, the editor id
000b0016, the boss u04e3):

| every 20 ticks | value |
|---|---|
| the actor's active flag | 1 (the driver sends it `Activate`; the area's script does that in the game) |
| `GetModelData()->GetIsLoop()` | **1** |
| `IsAnimating()` | 1 |
| the current animation | 2 (= the module's xa24_) |
| the anim root's `VGetTimeRemaining()` | 2.650 -> 2.317 -> 1.983 -> ... -> 0.317 -> **2.650 again** |

So the animation plays and advances, but it **loops**: the model data's loop flag is 1
(`CModelData::GetIsLoop` -> `CAnimData::GetIsLoop` -> `mLoop`), the anim root is a loop node and
its remaining time wraps back to the duration instead of resting at 0.  The module's test
(`IsAnimTimeRemaining(dt, "Whole Body")` on the 'PLAY' actor, the retail's call at 0x7A60) then
never answers false and the boss's own animation state never becomes kAS_Over, so the state's
"AnimOver" trigger (the DOL's `CPatterned::GetAnimOver`, 0x80151990, the actor's own word at
+0x6B4) never fires.

The host is faithful here, twice over:
* `CAnimData::IsAnimTimeRemaining` (src/MetroidPrime/CAnimData.cpp:668) is the retail's
  `IsAnimTimeRemaining__9CAnimDataCFfRCQ24rstl66basic_string...` (0x80029084): it reads the anim
  root at +0x278 and compares its `VGetTimeRemaining()` with the tolerance, and the name is not
  read.
* `CScriptActor::Think` (src/MetroidPrime/ScriptObjects/CScriptActor.cpp:89) is the retail's
  `Think__12CScriptActorFfR13CStateManager` (0x800701EC): the same `IsAnimTimeRemaining`
  (0x800702B8), the same loop flag (`CModelData::GetIsLoop`, 0x800702CC) and the same "only stop
  a non-looping animation" rule, so a looping animation is never stopped on either side.

So the module's "the 'PLAY' actor's animation has ended" test can only ever see the end of a
**non-looping** animation, and the 'PLAY' actor's model loops.  What makes the retail's heart
stage end is still open: either the retail's 'PLAY' actor plays a non-looping animation (the
script's own animation setup, which the console cannot reproduce), or the loop flag differs
between the two.  The next lane should look at the 'PLAY' actor's script data (its init
animation and its loop flag) and at the CScriptActor's "play once" path.

## 18. The script objects around the 'PLAY' actor (u04e8, the editor id 000b0016)

Dumped from the running game (a probe over the state manager's object list, 2026-10-21) and
cross-checked with the console's `obj` (which prints an object's own connection list).

**Who is connected to the 'PLAY' actor** (the module's CONNECT/'PLAY' target, 000b0016):

| sender | class | state | message | meaning |
|---|---|---|---|---|
| 000b0003 (the boss) | CEmperorIngStage1 | "CONN" | "PLAY" | the module's own lookup (fn_16_3094) |
| 000b0011 (u04e5) | CScriptRelay | "ZERO" (0x5A45524F) | "ACTV" (0x41435456) | the relay sends kSM_Activate in the kSS_Zero state to the 'PLAY' actor; the host's CScriptRelay sends kSS_Zero when it gets kSM_SetToZero (src/MetroidPrime/ScriptObjects/CScriptRelay.cpp:26) |

**The boss's own connections** (the console's `obj u04e3`) include
`on InternalState0 send Action -> 000b0194 CScriptActorKeyframe` - the module's IS00 (sent by
fn_16_78AC at 0x7920) reaches that keyframe.  But the keyframe's **only** connection is
`(state "PLAY", msg "PLAY") -> 000b0015`, a *different* CScriptActor (the boss's "Attach"
actor), so it never animates the 'PLAY' actor.  The probe agrees: the 'PLAY' actor's animation
and loop flag are the same before and after the IS00 (loop=1, anim=1), and the module's own
SetAnimation(xa24_) (0x799C) is what gives it its animation.

**So the 'PLAY' actor's animation is the module's own and it loops**: the model data's loop flag
is 1 (section 17), so the anim root's remaining time wraps and the "Whole Body has ended" test
(the retail's IsAnimTimeRemaining at 0x7A60) can never be true.  Nothing in area 11's script
layers (the SCLY layers of the MREA, read with tools/echoes_disc.py) targets 000b0011, 000b0015
or 000b0016 - these are **placed actors** (the area's actor list, which the MLVL reader skips),
and the relay's kSM_SetToZero must come from the level's script objects, which the console
cannot run.  The next lane should read the area's *actor list* (the placed entities and their
script data: the 'PLAY' actor's own init animation and loop flag, the relay's script data and
the keyframe's "passive"/"use originator" flags) to find what gives the 'PLAY' actor its
one-shot animation in the retail.

## 19. The 'PLAY' actor's animation: the shipped data loops, and the host's loop flag is the wrong one

**The disc says the 'PLAY' actor loops.**  The area-11 MREA's SCLY layers hold the placed actors
as objects whose editor id is the *layer* id | the entity id; the 'PLAY' actor is the layer-2
object **0x080b0016** (the area 0x00b = 11, the entity id 0x0016 - the runtime editor id
0x000b0016) and its property 0xc08d1b93 (**isLoop**) is **1**.  The port reads it correctly
(the runtime's `CModelData::GetIsLoop()` is 1), so the loop is the shipped data's.

**The host keeps the animation alive because of that flag, and the retail does not.**  The host's
`CAnimData` has a `mLoop` member (src/MetroidPrime/CAnimData.cpp:100, from the constructor's
bool) and `DoAdvance` branches on it (src/MetroidPrime/CAnimData.cpp:467 `if (mLoop) {`): the
loop branch advances the tree again and again, so the anim root's `VGetTimeRemaining()` wraps
(2.650 -> 2.317 -> ... -> 0.317 -> 2.650) and the module's "Whole Body has ended" test (the
retail's IsAnimTimeRemaining at 0x7A60) can never be true.  The script data's isLoop reaches
that member through `CScriptActor`'s constructor (src/MetroidPrime/ScriptObjects/CScriptActor.cpp:55
`ModelData()->EnableLooping(looping)`; the loader passes `sldrThis.isLoop` at
src/MetroidPrime/ScriptObjects/CScriptActor.cpp:320 and :338).

The DOL's `CAnimData` has **no such flag**: its `SetAnimation`
(build/G2ME01/asm/MetroidPrime/CAnimData.s:4434, 0x80029904) writes only the playback parms'
byte 0x21 into `this + 0x2AC` (0x80029AA8) and its `DoAdvance` (CAnimData.s:4746) branches on
**that** byte (0x80029F44 `lbz r0, 0x2ac(r26)` / 0x80029F4C `beq .L_8002A06C`) - the flag the
animation was set with, not the script data's isLoop.  The host keeps the two apart (`mAnimating`
at src/MetroidPrime/CAnimData.cpp:460/:567 and `mLoop` at :100) and branches on the wrong one
for this case.

**Correction (the same evening): the loop flag is *not* what blocked the heart stage.**  The
module's test fires on the tick where the looping animation's remaining time passes 0 (the
probe that sampled every 20 ticks had missed that one tick), and a clean run of the committed
build - with the driver sending the 'PLAY' actor its `Activate` (the area's script, which does
that in the game, cannot run under the console) - walks the FSM
**"BeginHeartStage" -> "HeartStage" (12 s) -> "EndHeartStage"** on its own: the "HeartStageOver"
trigger fires and the "ChooseRandomTentacleCount" code node runs.  Sections 17-18 had the cause
wrong: the blocker was the **inactive** 'PLAY' actor (its animation did not advance at all), not
the loop.

What is left from this section is a reconstruction difference in shared code: the DOL's
`CAnimData` has one flag byte at +0x2AC (written by SetAnimation from the parms' byte 0x21 and
branched on in DoAdvance), while the host splits it into `mLoop` (the script data's isLoop,
CAnimData.cpp:100) and `mAnimating` (:460) and branches on `mLoop` (:467).  For this actor both
are 1, so the branches agree; they would differ for an actor with isLoop = 0 that plays an
animation with the parms' byte 1.  Reported, not changed (shared code).

## 20. The heart stage on its own, and the functions it reaches

**The natural path works** (2026-10-21): with the 'PLAY' actor activated (the area's script does
that in the game; the console cannot run it, so the driver does -
`tools/drivers/emperoring1_heartstage.py`, and the smoke family's `tools/smoke/cmd_emp1.txt` now
sends it the message too), the FSM walks

    "BeginHeartStage" -> "HeartStage" (7 polls, 12 s) -> "EndHeartStage" -> "Taunt" -> ...

on its own: the tentacles are killed with a hit on their health-bearing collision actor
(section 13), "AllTentaclesRetracted" fires, the heart stage's timer runs out, the
"HeartStageOver" trigger fires and the **"ChooseRandomTentacleCount" code node runs** - no `fsm`
anywhere.  The beam fires 694 times during the heart stage (the probe on `FireBeam`).

**The functions the heart stage and the stage after it reach** (the transitive closure of the
REL's own calls from fn_16_774C and fn_16_71BC): 18 functions, 16 of them already marked.  The
two that are not, and the next five unmarked functions in this note's order:

| fn | calls | class | the branches |
|---|---|---|---|
| `fn_16_FF4` | 1 (from fn_16_CD0) | (a) the 0x48-byte `CBeamInfo` copy (the DOL's `CBeamInfo` copy assignment; the retail copies the info the `TLdrToBeamInfo` built at 0xD34 into the projectile's slot at 0xD40) | branch free (37 instructions) - it ran 694 times with the beam |
| `fn_16_3060` | 5 | (b) the 'PLAY' actor lookup | branch free (13 instructions); it ran (the probe counted 421 calls during the heart stage) |
| `fn_16_BD6C` | 6 | (a) the counted reference release (the host's `ncrc_ptr`) | 0xBD90 `bgt` (the count is above 1), 0xBD9C `beq` (the count is 0 -> only the free) |
| `fn_16_337C` | 6 | (a) the `CPASAnimParmData`'s parm list (the host's constructor) | branch free (97 instructions) |
| `fn_16_BDD0` | 5 | (a) the counted reference release | 0xBDF4 `bgt`, 0xBE00 `beq` |
| `fn_16_98D0` | 4 | (a) the `SLdrDamageVulnerability` copy (the DOL's `LdrToDamageVulnerability`, 0x8023ACFC) | branch free (23 instructions) |

**No new marker**: four of the six are (a) - copies of host/DOL code, which are never marked -
and `fn_16_3060` is the only (b) one, but the port has **one** function for it and for
`fn_16_3094` (`GetPlayActor`, marked `fn_16_3094`): the two retail bodies differ only in the
lookup they call (the DOL's `ObjectById` for fn_16_3060 and `GetObjectById` for fn_16_3094 -
the same lookup in the host, src/MetroidPrime/CStateManager.cpp:616), so a second marker on the
same port function would be wrong.  The module's own remaining unmarked functions are all (a)
(section 11.2), so the module is decompiled as far as the host's shared code allows.

## 21. The function classification (all 241 symbols)

Three classes, applied by rule so the table can be audited (the machine-readable copy is
`docs/module-analysis/EmperorIngStage1_functions.tsv`, one row per function, with the class, the
callers and the tables that reference it):

- **H - host copy** (126 functions, 0x3C38 bytes): the body is a copy of something the host
  already has - a shared class body, a compiler-emitted destructor, rstl / template code, inlined
  DOL math or the REL runtime - so the port writes no body for it.
- **T - data / vtable thunk** (17 functions, 0x108 bytes): no retail caller: the function is a
  vtable entry the port writes as one line in the table (the host's own body) or it is dead.
- **B - module behaviour** (98 functions, 0x8708 bytes): the port writes a body for it. **All 98
  carry a `// @` marker** - there is no unmarked behaviour function left in the module.

| class | functions | .text bytes | marked |
|---|---|---|---|
| H host copy | 126 | 0x3C38 (15,416) | 0 |
| T data / vtable thunk | 17 | 0x108 (264) | 0 |
| B module behaviour | 98 | 0x8708 (34,568) | 98 |
| **total** | **241** | **0xE448 (58,248)** | **98** |

- **68** - the module's own deleting destructor / free-only thunk - the host's own classes destroy their members (note section 6): `fn_16_43C` `fn_16_48C` `fn_16_2A68` `fn_16_2AB8` `fn_16_3340` `fn_16_3500` `fn_16_47DC` `fn_16_4838` `fn_16_48BC` `fn_16_48F4` `fn_16_4DF0` `fn_16_4E68` `fn_16_55D4` `fn_16_5610` `fn_16_564C` `fn_16_5BA8` `fn_16_5C04` `fn_16_5D60` `fn_16_67EC` `fn_16_6844` `fn_16_76F0` `fn_16_8784` `fn_16_87F0` `fn_16_8844` `fn_16_8880` `fn_16_88BC` `fn_16_8918` `fn_16_8938` `fn_16_89B8` `fn_16_8A28` `fn_16_8AA0` `fn_16_8D40` `fn_16_8F24` `fn_16_8FB8` `fn_16_96A4` `fn_16_97CC` `fn_16_9824` `fn_16_987C` `fn_16_992C` `fn_16_9B04` `fn_16_9B90` `fn_16_9E48` `fn_16_9FB0` `fn_16_A1D4` `fn_16_A780` `fn_16_A7EC` `fn_16_A840` `fn_16_A8A4` `fn_16_A904` `fn_16_AA70` `fn_16_AAD4` `fn_16_B130` `fn_16_B188` `fn_16_B378` `fn_16_B3DC` `fn_16_B52C` `fn_16_B584` `fn_16_B668` `fn_16_B6BC` `fn_16_B86C` `fn_16_B8F4` `fn_16_BB1C` `fn_16_BC44` `fn_16_BD04` `fn_16_BD6C` `fn_16_BDD0` `fn_16_BE34` `fn_16_BF14`

- **25** - the rstl / compiler-generated helper - the host's rstl provides it: `fn_16_BB3C` (a byte vector's reserve: allocate, the byte copy loop at 0xBBAC, Free of the old buffer at 0xBC1C) `fn_16_FF4` `fn_16_219C` `fn_16_2204` `fn_16_22B0` `fn_16_22D0` `fn_16_22F8` `fn_16_231C` `fn_16_2398` `fn_16_337C` `fn_16_39CC` `fn_16_39EC` `fn_16_3A14` `fn_16_4640` `push_back_unsafe__28CEmperorIngStage1JointVectorFRC26CJointCollisionDescription` `fn_16_46D4` `fn_16_46F4` `fn_16_471C` `fn_16_9980` `fn_16_9AB0` `fn_16_9E9C` `fn_16_9F04` `fn_16_A004` `fn_16_A644` `fn_16_BA6C`

- **13** - the loader's reader / the model's copy - the host's ScriptLoaderRel and the generated SLdr do it: `fn_16_9700` `fn_16_A5BC` `fn_16_A5FC` `fn_16_A61C` `fn_16_A980` `fn_16_ABD0` `fn_16_AEDC` `fn_16_AF7C` `fn_16_B014` `fn_16_B1D8` `fn_16_B434` `fn_16_B5C8` `fn_16_B6EC`

- **7** - the visor effect's particle descriptor copy - the host's CImpactVisorEffect::ParticleEffect does it: `fn_16_853C` `fn_16_85C8` `fn_16_867C` `fn_16_86BC` `fn_16_86DC` `fn_16_8704` `fn_16_8980`

- **5** - the base class's own body - the host's CActor / CPhysicsActor / CModelData provide it: `fn_16_60` `fn_16_18E8` `fn_16_6898` `fn_16_B958` `fn_16_BA30`

- **5** - the REL runtime - the host's platform/rel.cpp and the crtexe glue provide it: `_unresolved` `_epilog` `_prolog` `ModuleDestructors` `ModuleConstructors`

- **2** - a struct copy - the host's copy constructor / LdrToDamageVulnerability does it: `fn_16_9610` `fn_16_98D0`

- **1** - the DOL's inline code copied into the REL - the host has the same function (the DOL's Cross__9CVector3fFRC9CVector3fRC9CVector3f, 0x8001994C): `fn_16_2DDC`

- **T** (the vtable entries the port writes as a line; the host's own body): `fn_16_B994` `fn_16_B9A4` `fn_16_B9AC` `fn_16_B9B4` `fn_16_B9BC` `fn_16_B9C4` `fn_16_B9CC` `fn_16_B9DC` `fn_16_B9E8` `fn_16_B9F4` `fn_16_BA0C` `fn_16_BA14` `fn_16_BE98` `fn_16_BE9C` `fn_16_BEA4` `fn_16_BEB0` `fn_16_BEB8`

### 21.1 The six behaviour functions that were unmarked until now

| fn | what the port writes | the marker |
|---|---|---|
| `fn_16_3060` | the 'PLAY' actor lookup - the port's `GetPlayActor` (already marked `fn_16_3094`; the two retail bodies differ only in the DOL's `ObjectById` vs `GetObjectById`, the same lookup in the host) | a second marker on `GetPlayActor` (`// @ fn_16_3060`); the tool counts a marker per retail function, so both are counted |
| `fn_16_A2A0` | the module's loader - the port's `PortLoadEmperorIngStage1` (it reads the SLdr block, builds the model and constructs the object, as the retail's loader does) | `// @ fn_16_A2A0` |
| `fn_16_A250` | the REL's entry point - the port's `RELMain` | `// @ fn_16_A250` |
| `fn_16_A270` | the REL's registration - the body of the port's `RELMain` (it stores the loader and calls the DOL's `SetSEmperorIngStage1_FuncPtrs`, 0x8022756C) | `// @ fn_16_A270` (a second marker on `RELMain`) |
| `fn_16_A22C` | the REL's exit - the port's `RELExit` | `// @ fn_16_A22C` |
| `fn_16_BF88` | the static initialiser of the three FSM tables - the port's `skTriggers` / `skStates` / `skCodes` (the retail's 24 records each) | `// @ fn_16_BF88` above the table |

None of them has a live arm of its own that a run could miss: `fn_16_3060` is branch free (13
instructions) and ran 421 times during the heart stage (the probe of the last slice), the loader
and the entry points run at the module's load, and the tables are static data.  The loader's
`asmcheck` findings are the ones the note's section 11.2 lists as (a): the REL's own reader
chain (fn_16_ABD0, fn_16_AEDC, ...) is the host's generated `SLdrEmperorIngStage1.inc` in the
port, so the C++ never names it.

### 21.1a The T rows are the DOL's own bodies (2026-10-22)

Each T entry was compared instruction by instruction with the DOL body at the same vtable
entry (`EmperorIngStage1_functions.tsv` column "what" has the name and address): the twelve in
the boss's vtable (lbl_16_data_3C8) are the entries where the DOL's CPatterned vtable
(lbl_803B2458) has an inline body - `CPatterned::TakeDamage` 0x80073C58, `GetModelAlphau8`
0x80073C64, `ProjectileInfo` 0x80073C6C, `GetSearchPath` 0x80073C74, `fn_80073c7c` 0x80073C7C,
`GetRagDoll` 0x800358E0, `GetAttackTarget` 0x80073C84, `IsOnGround` 0x80073C90,
`GetGravityConstant` 0x80073C9C, `GetDeathExplosionParticle` 0x80073CA4, `CAi::Listen`
0x8003C59C, `CAi::GetOrigin` 0x80073CB4 - and every one is byte-identical (the REL loads the
globals through `@ha/@l` where the DOL uses `@sda21`).  lbl_16_data_510 is a copy of
`__vt__12CParticleGen` (its entries 11/12/20/22/23 against the DOL's `SetGeneratorRate`
0x800534B0, `SetDrawFlags` 0x800534B4, `GetGeneratorRate` 0x800534BC, `GetDrawFlags`
0x800534C4, `ShouldDraw` 0x800534CC; the other entries are DOL relocations).  None of them is a
module override, so none is marked; the host's CPatterned / CParticleGen bodies are the code.

### 21.2 Count check

Every function id of the module's symbol list appears exactly once in the table:

```
$ python3 - <<'PY'
import re, pathlib, collections
sym = pathlib.Path('config/G2ME01/rels/EmperorIngStage1/symbols.txt').read_text()
ids = re.findall(r'^(\S+) = \.text:0x[0-9A-Fa-f]+; // type:function', sym, re.M)
rows = [l.split('\t') for l in pathlib.Path('docs/module-analysis/EmperorIngStage1_functions.tsv').read_text().split('\n')[1:] if l.strip()]
print('symbols:', len(ids), 'rows:', len(rows))
print(collections.Counter(r[2] for r in rows))
PY
symbols: 241 rows: 241
Counter({'H': 126, 'B': 98, 'T': 17})
```

The machine-readable copy has one row per function:

```
$ awk -F'\t' 'NR>1{print $1}' docs/module-analysis/EmperorIngStage1_functions.tsv | sort -u | wc -l
241
```
