# DigitalGuardian REL (module 14): `CDigitalGuardian` / `CDigitalGuardianHead` (Guardian)

Host-port analysis for the Sanctuary Fortress Guardian boss, dated 2026-10-20. Milestone 1 only:
the actor loads, registers and thinks. The FSM, the head module, the toe target, the knee armour,
the transmission beacon and the vortex are not ported yet.

**Sources (all read-only):**

- m2c pseudo-C, one file per function: `/tmp/opencode/pseudo/DigitalGuardian/` (392 `.c` files,
  named `fn_14_<hexoff>.c`, plus `RELMain.c`, `RELExit.c`, `_prolog.c`, `_epilog.c`).
- function inventory with offsets and sizes: `config/G2ME01/rels/DigitalGuardian/symbols.txt`
  (874 lines, 420 functions; `fn_14_<hexoff> = .text:0x<off>; size:0x<n>`).
- section dumps with labels: `build/G2ME01/DigitalGuardian/asm/auto_04_00000000_data.s` (.data,
  0x114C bytes, vtables and tables), `auto_03_00000000_rodata.s` (.rodata, strings),
  `auto_05_00000000_bss.s`, `auto_00_00000000_text.s` (0x0..0xD8FC),
  `auto_fn_14_D904_text.s` (0xD904..0xDF6C, ends with the `.ctors` entry),
  `auto_00_0000DF6C_text.s` (0xDF6C..0x1AD28), `auto_fn_14_1AD2C_text.s` (0x1AD2C.., `.ctors`).
- the parameter surface is already decompiled upstream: `include/MetroidPrime/ScriptLoader/SLdrDigitalGuardian.hpp`
  and `SLdrDigitalGuardianHead.hpp` (both generated), so every property is known before code is written.
- the loader proxies the port already has: `src/MetroidPrime/Enemies/SDigitalGuardian_FuncPtrs.cpp`
  (`SDigitalGuardian_FuncPtrs` = { `mLoadDigitalGuardian`, `mLoadDigitalGuardianHead` },
  `include/MetroidPrime/ScriptLoaderRel.hpp` lines 327-333).
- vtable slot order: `docs/module-analysis/vtable-offsets.md` (CEntity → CActor → CPhysicsActor →
  CAi → CPatterned, 80 slots after two leading words).

## 0. Facts already established (do not re-derive)

- Module id 14, `DigitalGuardian`, 112796 text bytes, 420 functions (`docs/modules.tsv` line 14).
  Script types `DGRD` (the guardian body) and `DGHD` (the head module).
- `RELMain` = `.text:0x15C` (size 0x20) calls `fn_14_17C` (0x17C, size 0x3C). `fn_14_17C` fills the
  module's loader table `lbl_14_bss_C` with `unk0 = fn_14_1B8` and `unk4 = fn_14_DF7C`, then calls the
  DOL's `fn_8021F9B0(&lbl_14_bss_C, fn_14_DF7C, fn_14_1B8)`.
  `fn_8021F9B0` is `SetSDigitalGuardian_FuncPtrs__FP25SDigitalGuardian_FuncPtrs` (`config/G2ME01/symbols.txt`
  line 9608), so the table's two entries are the two loaders: `mLoadDigitalGuardian` (unk0 =
  `fn_14_1B8`) and `mLoadDigitalGuardianHead` (unk4 = `fn_14_DF7C`).
- `RELExit` = `.text:0x138` (size 0x24).
- Spawn area: `Sanctuary Fortress/09_cliff_temple_dark`, layers **3** and **11**
  (`docs/module_spawns.tsv` lines 64-65). `Splitter` spawns in the same area, layer 10
  (`docs/module_spawns.tsv` line 972) and in eight other Sanctuary Fortress areas.
- Upstream loader structs: `SLdrDigitalGuardian` = `SLdrEditorProperties` + `SLdrPatternedAITypedef`
  + `SLdrActorParameters` + `SLdrDigitalGuardianData`; `SLdrDigitalGuardianHead` = the same three
  plus `SLdrDigitalGuardianHeadData`. `SLdrDigitalGuardianData` carries the leg-stab ranges, the toe
  target, the vortex, the knee armour and the transmission beacon; `SLdrDigitalGuardianHeadData`
  carries the shields, the echo targets, the bomb slot, the annihilator pulse/charge, the lock-on
  beams/missiles and the machine gun. Both default to `health.health = 150`,
  `hI_KnockBackResistance = 2`, `turnSpeed = 360`, `creatureSize = 2`.
- The port's model of the shape is `src/MetroidPrime/Enemies/PortCIngBoostBallGuardian.cpp`
  (module 30, milestone 1) and the analysis format is `docs/module-analysis/BoostGuardian.md`.

**Note on the two files named above:** neither exists in this tree
(`src/MetroidPrime/Enemies/PortCIngBoostBallGuardian.cpp`, `docs/module-analysis/BoostGuardian.md`).
The milestone-1 body that does exist and that this note imitates is
`src/MetroidPrime/Enemies/PortCElitePirate.cpp` (module 15) with
`docs/module-analysis/ElitePirate.md`; the vtable slot order comes from
`docs/module-analysis/vtable-offsets.md`.

## a. Class inventory

Guest size = the `li r3, 0xNNN` immediately before `bl __nw__`; vtable = the `.data` label the
ctor stores into `this+0`. The five vtables are four-byte aligned tables of function pointers.

| class (name guess) | guest size | vtable (.data) | ctor | dtor | base | evidence |
|---|---|---|---|---|---|---|
| `CDigitalGuardian` — the boss body, script type **DGRD** ("Mechanism: Quadraxis") | **0x1290** | `lbl_14_data_484` (82 words) | `fn_14_B100` (0xB100, 0xF90) | `fn_14_AB98` (0xAB98, 0x2F0) | `CPatterned` | loader `fn_14_1B8` `__nw__(0x1290)` [0x000003F0]; vtable store [0x0000B170]; dtor re-stores it [0x0000ABC0]; `EPatternedAI 9` [0x0000B154] |
| `CDigitalGuardianHead` — the detachable head module, script type **DGHD** ("Mechanism: Shielded/Stunned Head Module") | **0x10F8** | `lbl_14_data_EB4` (82 words) | `fn_14_18264` (0x18264, 0xE7C) | `fn_14_17F64` (0x17F64, 0x300) | `CPatterned` | loader `fn_14_DF7C` `__nw__(0x10F8)` [0x0000E1C8]; vtable store [0x000182CC]; dtor [0x00017F88]; `EPatternedAI 0xA` [0x000182B8] |
| knee-armour actor (guess `CKneeArmour`) | **0x1A0** | `lbl_14_data_694` (31 words) | `fn_14_D4D0` (0xD4D0, 0x194) | `fn_14_D460` (0xD460, 0x70) | `CActor` | spawned by `fn_14_4080` with the string "Knee Armor " [rodata 0x8EE]; ctor calls `__ct__6CActorF…`; vtable stores [0x0000D520]/[0x0000D480] |
| echo-target actor (guess `CEchoTarget`) | **0x790** | `lbl_14_data_10D0` (31 words) | `fn_14_1A698` (0x1A698, 0x3AC) | `fn_14_1A5F4` (0x1A5F4, 0xA4) | `CActor` | spawned by `fn_14_12314` with the string "EchoTarget" [rodata 0xF2E]; vtable stores [0x0001A720]/[0x0001A614] |
| guardian energy projectile (guess `CGuardianProjectile`) | **0x568** | `lbl_14_data_1034` (39 words) | `fn_14_19838` (0x19838, 0xD0) | `fn_14_1AAC4` (0x1AAC4, 0x60) | `CEnergyProjectile` | `fn_14_10CA0`; ctor calls `__ct__17CEnergyProjectileF…`; vtable stores [0x000198E4]/[0x0001AAE4] |
| the vortex | **0x9F8** | – (plain `CShockWave`) | `__ct__10CShockWaveF…` | – | `CShockWave` | `fn_14_458C` builds it from `shockWaveInfo` (this+0x864 = data+0xA4), string "Shock Wave" [rodata 0x8FA] |

Not classes (helpers allocated by the module): `0x180` = `CExplosion` (×7: `fn_14_1A68`,
`fn_14_266C`, `fn_14_2E60`, `fn_14_F424`, `fn_14_11FEC`, `fn_14_12B04`, `fn_14_15054`);
`0x18` = `CCollisionActorManager` (`fn_14_5C30`, `fn_14_61DC`, `fn_14_13318`); `0x338` = `CElementGen`
(`fn_14_D664`, `fn_14_1A698`); `0x6A8` = `CPlasmaProjectile` (`fn_14_FE14`, "RezBitCuttingLaser");
`0xC` = `CToken` (`fn_14_B100`, `fn_14_18264`).

The `__nw__` memory-node name is the literal string `??(??)` for every allocation
(rodata 0x8AD and 0xEEE) — the same placeholder SpacePirate uses — so the class names above are
guesses from behaviour, not from the allocator.

## b. FSM

### b.1 The tables

`CPatterned::SetupStateMachine` is called first, then three tables are registered on the state
manager reached through `this+0x350` (slot `unk14` = states, `unk18` = triggers, `unk1C` = codes).

| class | SetupStateMachine | triggers (name, count) | states | codes |
|---|---|---|---|---|
| body | `fn_14_9DFC` (vtable slot 60, byte offset 0xF0) | `lbl_14_data_9C`, 12 (0xC) | `lbl_14_data_204`, 14 (0xE) | `lbl_14_data_380`, 13 (0xD) |
| head | `fn_14_17468` (vtable slot 60) | `lbl_14_data_808`, 20 (0x14) | `lbl_14_data_A14`, 17 (0x11) | `lbl_14_data_C80`, 29 (0x1D) |

A table entry is **16 bytes**: `{const char* name, 0, 0xFFFFFFFF, fn}` (`fn_14_9DFC` registers
`&table` and the count; the name pointers are in `.data` and the strings in `.rodata`).
The function pointers live in three-word records `{0, 0xFFFFFFFF, fn}` in `.data` (0xC..0x374 for the
body, 0x718..0xC74 for the head) and are copied into the tables by the module's **static
initialisers**:

| initialiser | size | what it fills | evidence |
|---|---|---|---|
| `fn_14_D904` | 0x668 | the body's three tables | in `.ctors` (`auto_fn_14_D904_text.s` ends `.section .ctors` / `.4byte fn_14_D904`); writes `lbl_14_data_9C`, `_204`, `_380` |
| `fn_14_1AD2C` | 0x9CC | the head's three tables | in `.ctors` (`auto_fn_14_1AD2C_text.s`); writes `lbl_14_data_808`, `_A14`, `_C80` |

Seven more three-word records sit at `.data 0x640..0x694` (`fn_14_9014`, `fn_14_8C70`, `fn_14_8978`,
`fn_14_8738`, `fn_14_8328`, `fn_14_7FEC`, `fn_14_8048`), used in pairs by the crippled/stunned
reactions [0x00007F20]. Their role is **unknown** (they are not in any of the three tables).

### b.2 Body triggers (`lbl_14_data_9C`)

The table is `.data 0x9C..0x15C` (12 entries of 16 bytes); the names are the `.rodata` strings
below, the functions the three-word records in `.data 0xC..0x90` (fn_14_D904's stores at
0xDB50..0xDCC8).

| # | name | rodata | fn | # | name | rodata | fn |
|---|---|---|---|---|---|---|---|
| 0 | StateOver | 0x2F4 | `fn_14_9AA8` | 6 | ShouldVortexAttack | 0x346 | `fn_14_9930` |
| 1 | IsIdle | 0x2FE | `fn_14_9A9C` | 7 | ShouldMeleeAttack | 0x359 | `fn_14_97F0` |
| 2 | HasFaceTarget | 0x305 | `fn_14_9A60` | 8 | ShouldProjectileAttack | 0x36B | `fn_14_97A4` |
| 3 | KneeDestroyed | 0x313 | `fn_14_9A48` | 9 | ShouldJumpAttack | 0x382 | `fn_14_9678` |
| 4 | AllKneesDestroyed | 0x321 | `fn_14_9A0C` | 10 | VortexStruckPlayer | 0x393 | `fn_14_966C` |
| 5 | ShouldProtectKnees | 0x333 | `fn_14_9994` | 11 | BreakOutOfVortex | 0x3A6 | `fn_14_9638` |

`IsIdle` reads bit 7 of `this+0x128C` [fn_14_9A9C]; `StateOver` compares `this+0x6B4` with 3
[fn_14_9AA8] (the base class's animation-state id).

### b.3 Body states (`lbl_14_data_204`)

The table is `.data 0x204..0x2E4` (14 entries); the records are in `.data 0x15C..0x1F8`
(fn_14_D904's stores at 0xDCD0..0xDEB8).

| # | name | rodata | fn | # | name | rodata | fn |
|---|---|---|---|---|---|---|---|
| 0 | Start | 0x3B7 | `fn_14_9634` (empty body) | 7 | MeleeAttackTelegraph | 0x409 | `fn_14_8CCC` |
| 1 | Idle | 0x3BD | `fn_14_95B8` | 8 | MeleeAttack | 0x41E | `fn_14_89D4` |
| 2 | FaceTarget | 0x3C2 | `fn_14_94BC` | 9 | ProjectileAttack | 0x42A | `fn_14_8794` |
| 3 | VortexAttack | 0x3CD | `fn_14_9260` | 10 | JumpAttack | 0x43B | `fn_14_8580` |
| 4 | VortexEndAttack | 0x3DA | `fn_14_9164` | 11 | KneeDestroyedReaction | 0x446 | `fn_14_8090` |
| 5 | VortexRetreat | 0x3EA | `fn_14_9070` | 12 | CrippledReaction | 0x45C | `fn_14_7D8C` |
| 6 | ExitVortexAttack | 0x3F8 | `fn_14_8E44` | 13 | Crippled | 0x46D | `fn_14_7CB4` |

### b.4 Body codes (`lbl_14_data_380`)

The table is `.data 0x380..0x450` (13 entries); the records are in `.data 0x2E4..0x374`
(fn_14_D904's stores at 0xDEBC..0xE00C).

| # | name | rodata | fn | # | name | rodata | fn |
|---|---|---|---|---|---|---|---|
| 0 | SetupBossEnergyBar | 0x476 | `fn_14_7BD4` | 7 | DisableHeadAttacks | 0x500 | `fn_14_768C` |
| 1 | ResetProtectKneesTime | 0x489 | `fn_14_7B90` | 8 | ResetHeadAttacks | 0x513 | `fn_14_7648` |
| 2 | ResetJumpTime | 0x49F | `fn_14_7B84` | 9 | SelectExitVortexTarget | 0x524 | `fn_14_7600` |
| 3 | ResetAttackTimes | 0x4AD | `fn_14_7B24` | 10 | SelectBestMeleeAttackLeg | 0x53B | `fn_14_7504` |
| 4 | SetProtectKneesFaceTarget | 0x4BE | `fn_14_77EC` | 11 | SelectMeleeAttackRange | 0x554 | `fn_14_73D4` |
| 5 | SetVortexAttackMode | 0x4D8 | `fn_14_775C` | 12 | SetJumpFaceTarget | 0x56B | `fn_14_7208` |
| 6 | SetNormalAttackMode | 0x4EC | `fn_14_76D0` | | | | |

### b.5 Head triggers (`lbl_14_data_808`)

| # | name | fn | # | name | fn |
|---|---|---|---|---|---|
| 0 | StateOver | `fn_14_17454` | 10 | UseAnnihilatorCharge | `fn_14_17338` |
| 1 | MainBodyDestroyed | `fn_14_17408` | 11 | AllSplittersDead | `fn_14_17298` |
| 2 | ShouldProjectileAttack | `fn_14_173CC` | 12 | HeadArmorDestroyed | `fn_14_1728C` |
| 3 | IsBurstOver | `fn_14_173B8` | 13 | IsAntennaTransmitting | `fn_14_17244` |
| 4 | IsIdle | `fn_14_173AC` | 14 | HasLineOfSight | `fn_14_171F8` |
| 5 | IsStunned | `fn_14_17398` | 15 | EchoTargetDestroyed | `fn_14_171EC` |
| 6 | IsGuarding | `fn_14_17384` | 16 | AllEchoTargetsDestroyed | `fn_14_171D8` |
| 7 | UseMachineGun | `fn_14_17370` | 17 | ShouldRetreat | `fn_14_171B8` |
| 8 | UseLockOnMissiles | `fn_14_1735C` | 18 | BombPitDestroyed | `fn_14_17198` |
| 9 | UseAnnihilatorPulse | `fn_14_1734C` | 19 | IsDead | `fn_14_17184` |

### b.6 Head states (`lbl_14_data_A14`)

| # | name | fn | # | name | fn |
|---|---|---|---|---|---|
| 0 | Start | `fn_14_17180` | 9 | DetachFromBody | `fn_14_16160` |
| 1 | Idle | `fn_14_17134` | 10 | StunnedReaction | `fn_14_15F3C` |
| 2 | FaceTarget | `fn_14_1709C` | 11 | DeploySplitters | `fn_14_15E18` |
| 3 | ProjectileTelegraph | `fn_14_16F74` | 12 | BombedReaction | `fn_14_15CE8` |
| 4 | PulseAttack | `fn_14_16D64` | 13 | SeekAttackPosition | `fn_14_15B94` |
| 5 | ChargeAttack | `fn_14_16A50` | 14 | SeekMotionPath | `fn_14_158F8` |
| 6 | MissileAttack | `fn_14_167EC` | 15 | FollowRetreatPath | `fn_14_1569C` |
| 7 | MachineGunAttack | `fn_14_16408` | 16 | Dead | `fn_14_1561C` |
| 8 | AttachedToBody | `fn_14_1633C` | | | |

### b.7 Head codes (`lbl_14_data_C80`)

| # | name | fn | # | name | fn |
|---|---|---|---|---|---|
| 0 | SetPlayerFaceTarget | `fn_14_1560C` | 15 | EnableDynamicCollision | `fn_14_14828` |
| 1 | ResetProjectileAttackTimes | `fn_14_15598` | 16 | DisableDynamicCollision | `fn_14_14664` |
| 2 | SelectProjectileAttack | `fn_14_15448` | 17 | EnableValidTarget | `fn_14_14604` |
| 3 | SetPlatformPosition | `fn_14_153D0` | 18 | DisableValidTarget | `fn_14_145A4` |
| 4 | SetStunnedPosition | `fn_14_15284` | 19 | SelectDeathHeadAttack | `fn_14_14470` |
| 5 | SeekLineOfSight | `fn_14_1524C` | 20 | SetInitialAttackPosition | `fn_14_143D8` |
| 6 | DropShields | `fn_14_15054` | 21 | SetAttackPosition | `fn_14_14254` |
| 7 | RaiseShields | `fn_14_14F74` | 22 | ResetGravityRepulsorHP | `fn_14_14248` |
| 8 | ExtractAntenna | `fn_14_14E78` | 23 | EjectPlayerFromBombPit | `fn_14_14058` |
| 9 | ExtractEchoTargets | `fn_14_14CF4` | 24 | GrabPlayerHintControl | `fn_14_13F70` |
| 10 | RetractEchoTargets | `fn_14_14DA0` | 25 | SetVulnerableToDamage | `fn_14_13F0C` |
| 11 | DestroyActiveSplitters | `fn_14_14C24` | 26 | SetImmuneToDamage | `fn_14_13EB8` |
| 12 | StartSplitterDeployment | `fn_14_14B98` | 27 | SelectRetreatPlatform | `fn_14_13C6C` |
| 13 | EndSplitterDeployment | `fn_14_14B60` | 28 | StartVocalRecoverReaction | `fn_14_13C1C` |
| 14 | Stage3Start | `fn_14_149F8` | | | |

## c. Parameters

### c.1 What the loaders read

| loader | size | property blocks it reads |
|---|---|---|
| `fn_14_1B8` (**DGRD**) | 0x330 | `0x255A4580` editor properties, `0xB3774750` patterned AI, `0x7E397FED` actor parameters, `0xC55918CC` guardian data → `fn_14_8AC` (0x82C) |
| `fn_14_DF7C` (**DGHD**) | 0x344 | `0x255A4580`, `0xB3774750`, `0x7E397FED`, `0x0F4D73B7` head data → `fn_14_E2C0` (0x920) |

Retail quirk: the three standard blocks all land in **one** stack object — `sp400` is an
`SLdrEditorProperties`, whose patterned typedef sits at +0x3C (`= sp43C`) and whose actor parameters
at +0x280 (`= sp680`), and the loader re-reads those two in place. Defaults written before the read
(`fn_14_1B8`): turnSpeed 360, minAttackRange 0, health 150, knock-back resistance 2, creatureSize 2 —
matching the upstream `SLdrDigitalGuardian` defaults.

The data block is read into a stack struct and copied **verbatim** into the object:
`fn_14_C68C(this+0x7C0, data)` for DGRD (0x640 bytes) and `fn_14_1912C(this+0x7C0, data)` for DGHD
(0x5B0 bytes). The loader therefore drops nothing: the last property it reads ends exactly at the
struct's end (DGRD `toeTargetVulnerability` at 0x4E4 + 0x15C = 0x640; DGHD `echoTargetVulnerability`
at 0x454 + 0x15C = 0x5B0). The fields the *class* does not keep are re-derived in the constructor
(this+0xE00 onward), not lost.

### c.2 DGRD: `SLdrDigitalGuardianData` → host struct (0x640 bytes at this+0x7C0)

Offsets are the loader's temp-struct offsets; the host struct is 1:1 with them.

| off | host field (= SLdr field) | type |
|---|---|---|
| 0x00 | mScannableInfoCrippled | CAssetId |
| 0x04, 0x08, 0x0C | mNearLegStabRange, mFarLegStabRange, mMinLegStabAttackTime | float |
| 0x10 | mLegStabDamage | SLdrDamageInfo (0x10) |
| 0x20 | mToeTargetHP | float |
| 0x24, 0x28 | mToeTargetModel, mToeTargetExplosion | CAssetId |
| 0x2C–0x74 | mSoundToeTarget, mSoundToeTargetAttack, mSoundToeTargetExplosion, mSoundToeTargetHit | SLdrAudioPlaybackParms (0x18 each) |
| 0x8C | mSoundShockWave | SLdrAudioPlaybackParms |
| 0xA4 | mShockWaveInfo | SLdrShockWaveInfo (0x30) |
| 0xD4–0xE4 | mVortexAttackDuration, mVortexAttractionForce, mMaxVortexAttractionDistance, mVortexLinearVelocity, mVortexLinearAcceleration | float |
| 0xE8 | mVortexDamage | SLdrDamageInfo |
| 0xF8, 0xFC | mUnknownFB5263E8, mUnknown6AAF33E3 | int |
| 0x100 | mUnknown4F5D725C | float |
| 0x104 | mSoundVortexFlash | SLdrAudioPlaybackParms |
| 0x11C, 0x120 | mLegModel, mShinArmor | CAssetId |
| 0x124 | mKneeArmorHP | float |
| 0x128, 0x140 | mSoundKneeArmorHit, mSoundKneeVulnerable | SLdrAudioPlaybackParms |
| 0x158 | mKneeArmor | CAssetId |
| 0x15C | mKneeEchoParameters | SLdrEchoParameters (0x14) |
| 0x170–0x188 | mUnknownA324E26C, mUnknown6A754EBD, mJumpTimer, mUnknown8106CDA9, mUnknown9E1B8105, mUnknownA08FCC70, mUnknown3254A16B | float |
| 0x18C, 0x190 | mTransmissionBeacon, mTransmissionBeaconFx | CAssetId |
| 0x194 | mTransmissionBeaconEchoParameters | SLdrEchoParameters |
| 0x1A8, 0x1C0, 0x1D8 | mSoundTransmissionBeacon, mSoundBeaconExtract, mSoundBeaconRetract | SLdrAudioPlaybackParms |
| 0x1F0 | mTransmissionBeaconHP | float |
| 0x1F4, 0x1F8 | mTransmissionBeaconExplosion, mTransmissionBeaconEchoHit | CAssetId |
| 0x1FC, 0x214 | mSoundBeaconExplode, mSoundBeaconHit | SLdrAudioPlaybackParms |
| 0x22C, 0x388, 0x4E4 | mKneeVulnerability, mVortexVulnerability, mToeTargetVulnerability | SLdrDamageVulnerability (0x15C each) |

### c.3 DGHD: `SLdrDigitalGuardianHeadData` → host struct (0x5B0 bytes at this+0x7C0)

| off | host field (= SLdr field) | type |
|---|---|---|
| 0x00, 0x04, 0x08, 0x0C | mScannableInfoShieldHead, mScannableInfoStunnedHead, mScannableInfoFinalHead, mHeadArmor | CAssetId |
| 0x10–0x20 | mMaxTurnSpeed, mMaxLinearVelocity, mGravityRepulsorHP, mMaxStunnedLinearVelocity, mMaxStage2StunnedTime | float |
| 0x24, 0x28, 0x44 | mUnknown8A83A097, mUnknownD919FB13, mUnknown1CC6D870 | float |
| 0x2C | mSoundHeadModuleHit | SLdrAudioPlaybackParms |
| 0x40 | mDropShieldsFx | CAssetId |
| 0x4C, 0x64, 0x7C | mSoundRaiseShields, mSoundDropShields, mSoundEjectSplitter | SLdrAudioPlaybackParms |
| 0x94 | mPART | CAssetId (the 'PART' tag the ctor locks into this+0x1100) |
| 0x98 | mEchoTargetHP | float |
| 0x9C, 0xA0, 0xA4 | mEchoTargets, mEchoTargetEmitterFx, mEchoTargetDestroyedFx | CAssetId |
| 0xA8–0x108 | mSoundEchoTargetExtract, mSoundEchoTargetRetract, mSoundEchoTargetTransmit, mSoundEchoTargetHit, mSoundEchoTargetExplosion | SLdrAudioPlaybackParms (0x18 each) |
| 0x120 | mEchoTargetsParameters | SLdrEchoParameters |
| 0x134 | mUnknown8317610F | float |
| 0x138 | mBombSlotDestroyedFx | CAssetId |
| 0x13C, 0x154, 0x16C | mSoundBombSlotExplosion, mSoundBombSlotDestroyed, mSoundStage3Recover | SLdrAudioPlaybackParms |
| 0x184 | mAnnihilatorPulse | CAssetId |
| 0x188 | mAnnihilatorPulseDamage | SLdrDamageInfo |
| 0x198 | mAnnihilatorCharge | CAssetId |
| 0x19C | mAnnihilatorChargeDamage | SLdrDamageInfo |
| 0x1AC | mUnknownFF7688BF | float |
| 0x1B0 | mUnknown12EBB390 | int |
| 0x1B4 | mFRME | CAssetId |
| 0x1B8 | mLockOnBeams | SLdrPlasmaBeamInfo |
| 0x1FC | mLockOnMissiles | CAssetId |
| 0x204 | mLockOnMissilesDamage | SLdrDamageInfo |
| 0x214 | mMachineGun | CAssetId |
| 0x218 | mMachineGunDamage | SLdrDamageInfo |
| 0x228 | mSoundMachineGun | SLdrAudioPlaybackParms |
| 0x240–0x24C | mMachineGunHalfSweepDist, mMachineGunSweepDuration, mMachineGunChargeDuration, mMachineGunFireTime | float |
| 0x250, 0x26C, 0x288, 0x2A4, 0x2C0, 0x2DC | the six `SLdrDigitalGuardianHeadStruct` blocks (0x1C each: firstShotType, projectileTelegraphTime, projectileAttackTime, annihilatorPulseOdds, annihilatorChargeOdds, machineGunOdds, lockOnMissilesOdds) | struct |
| 0x2F8, 0x454 | mBombPitVulnerability, mEchoTargetVulnerability | SLdrDamageVulnerability (0x15C each) |

### c.4 Recommended host structs

- `class CDigitalGuardianData` (0x640) — one member per row of c.2, `m`-prefixed; held by value at
  `this+0x7C0`.
- `class CDigitalGuardianHeadData` (0x5B0) — one member per row of c.3; held by value at
  `this+0x7C0` of the head.
- `class CDigitalGuardian : public CPatterned` with `CDigitalGuardianData mData;` plus the
  constructor-only members of section e; `class CDigitalGuardianHead : public CPatterned` with
  `CDigitalGuardianHeadData mData;`.
- Both loaders convert the SLdr blocks with the DOL helpers the retail loader uses:
  `LdrToEntityInfo`, `LdrToTransform4f`, `LdrToActorParameters`, `LdrToPatternedInfo`,
  `LdrToModelData` — the same calls `fn_14_1B8` makes.

## d. Spawn and the run harness

- Area: **Sanctuary Fortress / `09_cliff_temple_dark`**; the guardian is spawned in **layer 3** and
  **layer 11** (`docs/module_spawns.tsv` lines 64-65). `Splitter` (the head's sub-actor) is in
  layer 10 of the same area. In-game names: DGRD "Mechanism: Quadraxis", DGHD "Mechanism: Shielded
  Head Module" / "Stunned Head Module" (`docs/modules.tsv` line 14).
- Boot into the area: `MP2_TELEPORT=09_cliff_temple_dark` (the area's MLVL name) or
  `MP2_WORLD=cliff,09_cliff_temple_dark` (the world name is a part of the MLVL name —
  `platform/port_warp.cpp` lines 152-165); via the harness:
  `USERDIR=/tmp/opencode/u_a10 TELEPORT=09_cliff_temple_dark FRAMES=6000 LOG=/tmp/opencode/dg.log
  tools/run_gameplay.sh`. `WORLD=`/`TELEPORT=` accept an MLVL name or a hex asset id.
- Console verbs (`platform/port_console.cpp`): `layer <n>` → `PortWarp::Layer` (activates a layer,
  line 698); `objs [filter]` lists objects of the current area (line 1257); `obj <id>` prints one
  object with its HP (line 1277); `world <world> [area]` warps (line 709).
- Expected run: the log shows the DigitalGuardian module prolog, **no** `[port] caught` or
  `reach-stub` lines, `objs` lists one `CDigitalGuardian` (and one `CDigitalGuardianHead` once WP3
  lands), and the actor thinks without a crash. Layer 3 holds the body; layer 11 the second copy.
- Note: the game is a boss fight — `MP2_GODMODE=1` (harness `GODMODE=1`) keeps the player alive
  while the actor is exercised, and a paused game runs no update (do not press START).

## e. Layout

Base class size: **`CPatterned` is 0x7C0** (`include/MetroidPrime/Enemies/CPatterned.hpp:428`,
`CHECK_SIZEOF(CPatterned, 0x7c0)`), so every offset ≥ 0x7C0 below is `CDigitalGuardian`'s own.
All offsets are guest (32-bit GC) offsets.

| off | what | evidence |
|---|---|---|
| 0x000 | `CPatterned` base (0x7C0) | vtable stored [0x0000B170]; `this+0x6B4` = base anim state id [fn_14_9AA8]; `this+0x754` = base death-explosion token [fn_14_D8]; `this+0x448` = base damage cooldown [fn_14_78]; `this+0x34C` = base on-ground flag [fn_14_C0]; `this+0x4A0` = base attack target [fn_14_B0]; `this+0x44F` = base colour alpha [fn_14_88]; `this+0x454` = base origin vector [fn_14_F0] |
| 0x7C0 | `CDigitalGuardianData` (0x640) copied from the loader struct | `fn_14_C68C(this+0x7C0, data)` [0x0000B17C]; data+0xA4 = `shockWaveInfo` read at this+0x864 [fn_14_458C] |
| 0xE00 | secondary struct (0x5C) built from the `CPatternedInfo` | `fn_14_C57C(this+0xE00, patternedInfo)` [0x0000B180]; it copies patternedInfo+0x3C..0x5C |
| 0xE68 | `CToken` (the 'PART' asset, locked in the ctor) | locked at [0x0000B3xx]; the ctor's `unk1100` is the same token used by the toe-target explosion [fn_14_1A68] |
| 0xE6C, 0xEBC, 0xF0C, 0xF5C, 0x11EC | five `CModelData` (leg / shin / knee / toe / body models) | `fn_14_6864(this)` writes them at the end of the ctor; 0xF5C is built from `toeTargetModel` (data+0x24 = this+0x7E4) [0x000069B0] |
| 0xE6C… | (the region 0xE00-0xE6C holds the secondary struct and four s32 words) | [0x0000B194…] |
| 0xFA8 | u8 | ctor |
| 0xFAC, 0xFB8, 0x100C, 0x1074, 0x10BC, 0x10C4, 0x10D0, 0x10D8 | s32 counters set to 4 | ctor stores |
| 0xFB0…0xFC2 | six u16 ids, all `kInvalidUniqueId` | ctor |
| 0xFC4 | count of the 12-byte "hit direction" table at 0xFC8 | ctor pushes 4 entries (-1,1,0), (1,-1,0), (1,1,0), (-1,-1,0) |
| 0xFF8 | count of the angle table at 0xFFC | ctor pushes 0.7853982, 2.3561945, -0.7853982, -2.3561945 (`lbl_14_data_45C`..`468`) |
| 0x1014 / 0x1044 / 0x1110 / 0x1188 | four `CDamageVulnerability` (one = immune ricochet) | `fn_14_3B4C(this+…, &ldr)` calls in the ctor |
| 0x1088 | `CTransform4f` (the identity) | ctor stores `sIdentity` |
| 0x11EC | `CModelData` (the echo emitter's model) | `fn_14_2578` takes its bounds (the `GetBounds` call at 0x269C) and `fn_14_17DC` draws it |
| 0x10B8, 0x10B9, 0x10C0…0x10C3, 0x10C8…0x10CB, 0x10D4…0x10D7 | locator-segment ids, all 0xFF | ctor: `GetLocatorSegId("Head_1"/"L_front_knee_2"/…)` |
| 0x1100 | `CToken` (locked 'PART' token) | ctor; `arg0->unk110C` is its "present" flag |
| 0x1144…0x1150 | four floats = the patterned info's turn speed | ctor: `arg0->unk1144 = arg_sp8->unk20` (×4) |
| 0x116C…0x1186 | `legStabDamage` (SLdrDamageInfo) | ctor: `LdrToDamageInfo` |
| 0x11B8, 0x11C0, 0x11C4, 0x11D8, 0x11DC, 0x1284 | s32 (`0`, `kInvalidUniqueId`, `-1`, `-1`, `-1`) | ctor |
| 0x11CC…0x11D4, 0x1278…0x1280 | `CVector3f` zero | ctor |
| 0x1238 | u8 | ctor |
| 0x123C | `void*` echo emitter | `fn_14_D664` result; `AllocateEchoEmitter` in the ctor |
| 0x1244 / 0x1254 | two `CToken` (destroyed by the dtor) | dtor [0x0000ABD0]/[0x0000ABF4] |
| 0x1264 / 0x1268 / 0x126C / 0x1274 | f32/s32 (`0`, `0`, `0`, `0`) | ctor |
| 0x1270 | u32 = `Black(lbl_14_rodata_264)` | ctor |
| **0x128C** | u8 flag byte — bit 7 = "is idle" | the `IsIdle` trigger reads `(this+0x128C >> 7) & 1` [fn_14_9A9C] |
| 0x128D | u8 flag byte (bits 7 and 6 cleared by the ctor) | ctor |
| 0x1290 | end of object | loader `__nw__(0x1290)` |

Head (`CDigitalGuardianHead`, 0x10F8) for comparison: base 0x7C0, `CDigitalGuardianHeadData`
0x5B0 at 0x7C0 (copied by `fn_14_1912C`), four words at 0xD70, `CVisualKnowledge` at 0xD80, two
`CProjectileInfo` at 0xE04 and 0xE2C (built from the data's `machineGun` / lock-on assets),
`CToken`s at 0xEFC, 0xF0C, 0xF1C, 0xFD8.

## f. Vtable

Slot order from `docs/module-analysis/vtable-offsets.md`; both tables have 82 words = 2 leading
words (RTTI null, zero) + 80 slots, so slot *n* is at byte offset `0x08 + 4*(n-2)`.

### f.1 `CDigitalGuardian` — `lbl_14_data_484` (82 words)

Local overrides (33). Every other slot keeps the DOL base-class symbol.

| slot | byte off | method | REL fn | note |
|---|---|---|---|---|
| 2 | 0x08 | `~CEntity()` (deleting dtor) | `fn_14_AB98` | stores the vtable back [0x0000ABC0] |
| 4 | 0x10 | `PreThink` | `fn_14_A688` | |
| 5 | 0x14 | `Think` | `fn_14_A5BC` | |
| 6 | 0x18 | `AcceptScriptMsg` | `fn_14_A6A8` | 0x4F0 — the biggest body function |
| 9 | 0x24 | `PreRender` | `fn_14_A384` | |
| 10 | 0x28 | `AddToRenderer` | `fn_14_A004` | |
| 11 | 0x2C | `Render` | `fn_14_A3D4` | |
| 13 | 0x34 | `PreRenderAllViewports` | `fn_14_A138` | |
| 15 | 0x3C | `GetHealthInfo() const` | `fn_14_10C` | forwards to slot 0x38 (`HealthInfo`) |
| 16 | 0x40 | `GetDamageVulnerability() const` | `fn_14_9FE4` | |
| 18 | 0x48 | `GetTouchBounds() const` | `fn_14_10` | uses `CPhysicsActor::GetBoundingBox` |
| 21 | 0x54 | `GetAimPosition` | `fn_14_9E88` | |
| 28 | 0x70 | `DoUserAnimEvent` | `fn_14_A468` | the toe-target effect handler |
| 29 | 0x74 | `GetScannableObjectInfo` | `fn_14_9DBC` | |
| 41 | 0xA4 | `TakeDamage` | `fn_14_78` | stores `skDamageHitTime` at this+0x448 |
| 42 | 0xA8 | `CanBeShot` | `fn_14_E0` | returns 1 |
| 43 | 0xAC | `IsListening` | `fn_14_0` | returns 1 |
| 44 | 0xB0 | `Listen` | `fn_14_E8` | returns 0 |
| 45 | 0xB4 | `GetOrigin` | `fn_14_F0` | the base vector at this+0x454 |
| 47 | 0xBC | `RenderSystemsToBeDrawnLast` | `fn_14_A3A4` | |
| 50 | 0xC8 | `GetModelAlphau8` | `fn_14_88` | byte at this+0x44F |
| 55 | 0xDC | `ProjectileInfo` | `fn_14_90` | null |
| 56 | 0xE0 | `GetSearchPath` | `fn_14_98` | null |
| 57 | 0xE4 | `GetPointSearchPath` | `fn_14_A0` | null |
| 60 | 0xF0 | `SetupStateMachine` | `fn_14_9DFC` | registers the three body tables |
| 61 | 0xF4 | `GetRagDoll` | `fn_14_A8` | null |
| 67 | 0x10C | `GetAttackTarget` | `fn_14_B0` | u16 at this+0x4A0 |
| 68 | 0x110 | `IsOnGround` | `fn_14_C0` | bit in this+0x34C |
| 69 | 0x114 | `GetGravityConstant` | `fn_14_CC` | a float constant from .rodata+0x23C |
| 70 | 0x118 | `IsScanVisorSelfRender` | `fn_14_8` | returns 1 |
| 71 | 0x11C | `GetScanVisorRenderBounds` | `fn_14_9CA4` | |
| 72 | 0x120 | `ScanVisorRender` | `fn_14_9ABC` | |
| 73 | 0x124 | `GetDeathExplosionParticle` | `fn_14_D8` | base member this+0x754 |

Inherited (47): slot 3 `TypesMatch` (`fn_8009B404`, the DOL's), 7 `SetActive`, 8 `ClearFluidList`,
12 `CanRenderUnsorted`, 14 `HealthInfo`, 17 `GetDamageVulnerability(pos,dir,dmg)`, 19 `Touch`,
20 `GetOrbitPosition`, 22 `GetHomingPosition`, 23 `GetScanObjectIndicatorPosition`,
24 `GetCollisionResponseType`, 25 `FluidFXThink`, 26 `OnScanStateChange`, 27 `GetSortingBounds`,
30 `ProcessSoundEvent`, 31 `GetCollisionPrimitive`, 32 `GetPrimitiveTransform`, 33 `CollidedWith`,
34 `GetStepDownHeight`, 35 `GetStepUpHeight`, 36 `IsOnStaticGround`, 37 `GetWeight`, 38 `Death`,
39 `KnockBack`, 40 `DamageVulnerability`, 46 `RenderSystemsToBeDrawnFirst`, 48 `Freeze`,
49 `ThinkAboutMove`, 51 `Burn`, 52 `Shock`, 53 `MassiveDeath`, 54 `MassiveFrozenDeath`,
58 `GetContactDamage`, 59 `UpdateHitDamageTime`, 62 `CanBeIngPossessed`, 63 `CanBeUnPossessed`,
64/65 `SetIngPossessed` (×2), 66 `SetAttackTarget`, 74 `GetDeathTimeScale`, 75 `TryToBeCaptured`,
76 `IssueDeathBodyCommand`, 77 `GetFadeOnDeathTime`, 78 `GetIngSnatchingNormal`,
79 `GetIngSnatchingPoint`, 80 `GetIngSnatchingModelOverlapSize`, 81 `RenderIngSnatchingTransition`.

`DoUserAnimEvent` dispatches through a jump table of 25 entries at `.data 0x5CC`
(`jumptable_14_data_5CC`, all arms inside `fn_14_A468`).

### f.2 `CDigitalGuardianHead` — `lbl_14_data_EB4` (82 words)

Local overrides (31): 2 `fn_14_17F64` (dtor), 5 `fn_14_17C98` (Think), 6 `fn_14_17D38`
(AcceptScriptMsg), 9 `fn_14_17A44` (PreRender), 10 `fn_14_17A00` (AddToRenderer),
11 `fn_14_17B28` (Render), 15 `fn_14_10C` (GetHealthInfo const), 16 `fn_14_179E0`
(GetDamageVulnerability const), 18 `fn_14_10` (GetTouchBounds), 21 `fn_14_17890` (GetAimPosition),
28 `fn_14_17BBC` (DoUserAnimEvent), 29 `fn_14_177C8` (GetScannableObjectInfo), 41 `fn_14_78`
(TakeDamage), 42 `fn_14_E0` (CanBeShot), 43 `fn_14_DF6C` (IsListening), 44 `fn_14_E8` (Listen),
45 `fn_14_F0` (GetOrigin), 47 `fn_14_17A64` (RenderSystemsToBeDrawnLast), 50 `fn_14_88`
(GetModelAlphau8), 55 `fn_14_17840` (ProjectileInfo), 56 `fn_14_98` (GetSearchPath),
57 `fn_14_A0` (GetPointSearchPath), 60 `fn_14_17468` (SetupStateMachine), 61 `fn_14_A8`
(GetRagDoll), 67 `fn_14_B0` (GetAttackTarget), 68 `fn_14_C0` (IsOnGround), 69 `fn_14_CC`
(GetGravityConstant), 70 `fn_14_DF74` (IsScanVisorSelfRender), 71 `fn_14_17638`
(GetScanVisorRenderBounds), 72 `fn_14_174F4` (ScanVisorRender), 73 `fn_14_D8`
(GetDeathExplosionParticle).
Inherited (49): slot 3 is `fn_8009B3CC` and slot 4 is `CPatterned::PreThink` (both DOL symbols);
the rest are the same base-class symbols as the body's inherited list.

## g. Function map

| range | what |
|---|---|
| 0x0000-0x0138 | tiny accessors (`fn_14_0`…`fn_14_10C`): the vtable one-liners (IsListening, IsScanVisorSelfRender, TakeDamage, GetModelAlphau8, ProjectileInfo, GetSearchPath, GetPointSearchPath, GetRagDoll, GetAttackTarget, IsOnGround, GetGravityConstant, GetDeathExplosionParticle, CanBeShot, Listen, GetOrigin) and `fn_14_10` (GetTouchBounds) |
| 0x0138-0x01B8 | `RELExit`, `RELMain` |
| 0x01B8-0x04E8 | **`fn_14_1B8` DGRD loader** (0x330) |
| 0x04E8-0x08AC | loader helpers: `fn_14_4E8` (model data), `fn_14_570`/`6AC`/`76C`/`7D0`/`830` (SLdr sub-struct ctors/dtors) |
| 0x08AC-0x1220 | **`fn_14_8AC` DGRD data loader** (0x82C), `fn_14_10D8` (its dtor), `fn_14_1220` (SLdr editor-props init) |
| 0x1220-0x1A68 | SLdr helper ctors/copy helpers (`fn_14_143C`…`fn_14_1994`) |
| 0x1A68-0x1FE8 | `fn_14_1A68` — the shared **toe-target effect helper**: the four legs' `ToeTargetFL/BL/FR/BR`, `SWBuildUpFL/…`, `ToeFlinchFL/…` effect states + the "Toe Target Explosion Fx" `CExplosion`; called from the melee, jump and crippled states and from `fn_14_458C` |
| 0x1FE8-0x3BFC | body helpers: `fn_14_20E4`…`fn_14_3BFC` — actor-info building, transforms, leg/knee maths, `fn_14_2578` (the bounds used by the echo emitter) |
| 0x3BFC-0x458C | body helper maths: `fn_14_3BFC`…`fn_14_44F8`; `fn_14_4464`/`fn_14_44F8` are the two leg-pick helpers |
| 0x458C-0x476C | **`fn_14_458C`** — spawn the vortex (`CShockWave` 0x9F8) from `shockWaveInfo`, then the toe targets |
| 0x476C-0x5C30 | `fn_14_476C` (shock-wave info builder), `fn_14_4878`…`fn_14_59BC` — body helpers (attack selection, timers) |
| 0x5C30-0x6864 | `fn_14_5C30`/`fn_14_61DC` — the 0x18 `CCollisionActorManager` + `CJointCollisionDescription` builders (knee joint colliders) |
| 0x6864-0x7208 | **`fn_14_6864`** — the ctor's tail: loads the five `CModelData`s (legs, shin, knee, toe target); `fn_14_6E04`…`fn_14_7168` follow |
| 0x7208-0x7CB4 | **body codes**: `fn_14_7208` SetJumpFaceTarget, `fn_14_73D4` SelectMeleeAttackRange, `fn_14_7504` SelectBestMeleeAttackLeg, `fn_14_7600` SelectExitVortexTarget, `fn_14_7648` ResetHeadAttacks, `fn_14_768C` DisableHeadAttacks, `fn_14_76D0` SetNormalAttackMode, `fn_14_775C` SetVortexAttackMode, `fn_14_77EC` SetProtectKneesFaceTarget |
| 0x7B24-0x7CB4 | body codes `fn_14_7B24` ResetAttackTimes, `fn_14_7B84` ResetJumpTime, `fn_14_7B90` ResetProtectKneesTime, `fn_14_7BD4` SetupBossEnergyBar |
| 0x7CB4-0x9634 | **body states**: `fn_14_7CB4` Crippled, `fn_14_7D8C` CrippledReaction, `fn_14_8090` KneeDestroyedReaction, `fn_14_8580` JumpAttack, `fn_14_8794` ProjectileAttack, `fn_14_89D4` MeleeAttack, `fn_14_8CCC` MeleeAttackTelegraph, `fn_14_8E44` ExitVortexAttack, `fn_14_9070` VortexRetreat, `fn_14_9164` VortexEndAttack, `fn_14_9260` VortexAttack, `fn_14_94BC` FaceTarget, `fn_14_95B8` Idle, `fn_14_9634` Start |
| 0x9638-0x9AA8 | **body triggers**: `fn_14_9638` BreakOutOfVortex … `fn_14_9AA8` StateOver (the table order is reversed in address order) |
| 0x9ABC-0x9DFC | body overrides: `fn_14_9ABC` ScanVisorRender, `fn_14_9CA4` GetScanVisorRenderBounds |
| 0x9DFC-0x9E88 | **`fn_14_9DFC` SetupStateMachine (body)** |
| 0x9E88-0xA6A8 | body overrides: `fn_14_9E88` GetAimPosition, `fn_14_9FE4` GetDamageVulnerability, `fn_14_A004` AddToRenderer, `fn_14_A138` PreRenderAllViewports, `fn_14_A384` PreRender, `fn_14_A3A4` RenderSystemsToBeDrawnLast, `fn_14_A3D4` Render, `fn_14_A468` DoUserAnimEvent, `fn_14_A5BC` Think, `fn_14_A688` PreThink, `fn_14_A6A8` AcceptScriptMsg (0x4F0) |
| 0xAB98-0xB100 | `fn_14_AB98` body dtor (0x2F0), `fn_14_AE88`/`fn_14_B06C` its helpers |
| 0xB100-0xC090 | **`fn_14_B100` body ctor** (0xF90) |
| 0xC090-0xCF94 | body data helpers: `fn_14_C57C`/`fn_14_C610` (the 0xE00 struct), **`fn_14_C68C`** (the 0x640-byte data copy, 0x470), `fn_14_CAFC`/`fn_14_CDB8` (sub-struct copies), `fn_14_CE1C`/`fn_14_CE94` |
| 0xCF94-0xD320 | `fn_14_CF94` PreThink, `fn_14_D320` SetActive (the knee-armour actor's overrides) |
| 0xD320-0xD664 | `fn_14_D320` SetActive, `fn_14_D460` knee-armour dtor, **`fn_14_D4D0` knee-armour ctor** (0x194) |
| 0xD664-0xD89C | `fn_14_D664` (the 0x338 `CElementGen`), `fn_14_D7DC` |
| 0xD89C-0xDF6C | `fn_14_D89C`, **`fn_14_D904` — the body's static table initialiser** (`.ctors`) |
| 0xDF6C-0xE2C0 | `fn_14_DF6C`, **`fn_14_DF7C` DGHD loader** (0x344) |
| 0xE2C0-0xEBE0 | **`fn_14_E2C0` DGHD data loader** (0x920) |
| 0xEBE0-0xF424 | head helpers: `fn_14_EBE0`, `fn_14_ED70`, `fn_14_F0B8` (the `SLdrDigitalGuardianHeadStruct` loader), `fn_14_F2B8`/`fn_14_F390` |
| 0xF424-0xFE14 | `fn_14_F424` — the "B_ball_LCTR" explosion (the head's bomb-slot effect); `fn_14_F6A4`, `fn_14_FA50`…`fn_14_FB70` |
| 0xFE14-0x101E0 | `fn_14_FE14` — the 0x6A8 `CPlasmaProjectile` (lock-on beam, "RezBitCuttingLaser"), `fn_14_FFFC`, `fn_14_1008C`, `fn_14_10120` |
| 0x101E0-0x11FEC | head weapon spawners: `fn_14_101E0` (annihilator pulse), `fn_14_10A2C` (annihilator charge), `fn_14_10CA0` (the 0x568 `CEnergyProjectile`), `fn_14_111E0` (another 0x568 projectile), `fn_14_114C8`…`fn_14_11EA8` |
| 0x11FEC-0x126C8 | `fn_14_11FEC` — the "EchoTargetExplosion" effect; **`fn_14_12314` — spawns the echo target** (0x790 `CActor`) |
| 0x126C8-0x13B38 | head helpers (`fn_14_126C8`, `fn_14_12818`, `fn_14_1303C`, `fn_14_13318` — the 0x18 collision manager, `fn_14_13830`, `fn_14_139C4`) |
| 0x13C1C-0x17468 | **head codes**: `fn_14_13C1C` StartVocalRecoverReaction … `fn_14_1560C` SetPlayerFaceTarget (the table order is reversed in address order) |
| 0x1561C-0x17298 | **head states**: `fn_14_1561C` Dead … `fn_14_17180` Start |
| 0x17298-0x17468 | **head triggers**: `fn_14_17298` AllSplittersDead … `fn_14_17454` StateOver |
| 0x17468-0x174F4 | **`fn_14_17468` SetupStateMachine (head)** |
| 0x174F4-0x18264 | head overrides: `fn_14_174F4` ScanVisorRender, `fn_14_17638` GetScanVisorRenderBounds, `fn_14_177C8` GetScannableObjectInfo, `fn_14_17840` ProjectileInfo, `fn_14_17890` GetAimPosition |
| 0x17A64-0x18264 | head overrides: `fn_14_17A00` AddToRenderer, `fn_14_17A44` PreRender, `fn_14_17A64` RenderSystemsToBeDrawnLast, `fn_14_17B28` Render, `fn_14_17BBC` DoUserAnimEvent, `fn_14_17C98` Think, `fn_14_17D38` AcceptScriptMsg, `fn_14_17F64` head dtor |
| 0x18264-0x190E0 | **`fn_14_18264` head ctor** (0xE7C) |
| 0x1912C-0x196F0 | **`fn_14_1912C`** — the 0x5B0-byte head-data copy (0x5C4) |
| 0x196F0-0x1A698 | projectile class: `fn_14_19784` PreThink, `fn_14_19838` ctor, `fn_14_19908`/`1998C`/`19A08`/`19B70`/`19CE0`/`19DF8`/`19F18`/`19FD0`/`1A12C`/`1A168`/`1A1F8`/`1A430` — the `CEnergyProjectile` overrides |
| 0x1A5F4-0x1AA44 | `fn_14_1A5F4` echo-target dtor, **`fn_14_1A698` echo-target ctor** (0x3AC) |
| 0x1AAC4-0x1AD2C | `fn_14_1AAC4` projectile dtor, `fn_14_1AC14`, `fn_14_1ACB8` |
| 0x1AD2C-0x1B6F8 | **`fn_14_1AD2C` — the head's static table initialiser** (`.ctors`) |
| 0x1B6F8-0x1B7BC | `_unresolved` (0xC4) — not decompiled |

## i. Port plan

### WP1 — milestone 1 (the actor loads, registers and thinks)

New port-only files (the upstream tree has no `CDigitalGuardian`; the module is an entry stub):

- `include/MetroidPrime/Enemies/CDigitalGuardian.hpp` — `class CDigitalGuardianData` (0x640, the
  c.2 fields), `class CDigitalGuardian : public CPatterned` with the ctor
  `(TUniqueId, const rstl::string&, const CEntityInfo&, const CTransform4f&, const CModelData&,
  const CPatternedInfo&, const CActorParameters&, const CDigitalGuardianData&)`, the overridden
  methods, and `CHECK_SIZEOF` is not meaningful (the host class is its own layout — do **not**
  copy the guest offsets into the host class; keep the guest offsets in the analysis only).
- `src/MetroidPrime/Enemies/PortCDigitalGuardian.cpp` — the class, `PortLoadDigitalGuardian`,
  `RELMain`, `RELExit`.
- `src/MetroidPrime/Enemies/PortCDigitalGuardianHead.hpp` / `…Head.cpp` can be a second pair for
  WP3; for WP1 one file may hold both loaders.

Shared files that change (one module's lines only; the orchestrator merges):

| file | change |
|---|---|
| `tools/modules_port.txt` | `DigitalGuardian;src/MetroidPrime/PortModuleStubs.cpp` → `DigitalGuardian;src/MetroidPrime/Enemies/PortCDigitalGuardian.cpp` (add a second line if the head lives in its own file) |
| `files.cmake` | regenerate with `python3 tools/gen_files_cmake.py` (it picks up `src/**/Port*.cpp`) |
| `include/MetroidPrime/CEntityInfo.hpp` | add `kET_DigitalGuardian = 112` and `kET_DigitalGuardianHead = 113` — the two ids `src/MetroidPrime/TypesMatch.cpp` already reserves ("112/113: class not declared yet (DigitalGuardian REL); parent CPatterned", lines 329-330 and 600-601) |
| `src/MetroidPrime/TypesMatch.cpp` | guarded `#ifdef TARGET_PC` include + `TYPES_MATCH_IMPL(CDigitalGuardian, CPatterned, kET_DigitalGuardian)`, `TYPES_MATCH_IMPL(CDigitalGuardianHead, CPatterned, kET_DigitalGuardianHead)` and the matching `CAST_TO_REF_IMPL` / `CAST_TO_PTR_IMPL` pairs — follow the `CElitePirate` precedent so the non-PC path stays byte-identical |

No change needed: `src/MetroidPrime/Enemies/SDigitalGuardian_FuncPtrs.cpp` (the proxy already calls
through `gLoader_DigitalGuardian`), `include/MetroidPrime/ScriptLoaderRel.hpp`
(`SDigitalGuardian_FuncPtrs` already has both slots), `src/MetroidPrime/ScriptLoader.cpp`
(`DGRD`/`DGHD` are already registered).

`RELMain` must mirror `fn_14_17C`:

```cpp
static SDigitalGuardian_FuncPtrs REL_loader_DigitalGuardian;
void RELMain() {
  REL_loader_DigitalGuardian.mLoadDigitalGuardian = PortLoadDigitalGuardian;
  REL_loader_DigitalGuardian.mLoadDigitalGuardianHead = PortLoadDigitalGuardianHead;
  SetSDigitalGuardian_FuncPtrs(&REL_loader_DigitalGuardian);
}
void RELExit() { SetSDigitalGuardian_FuncPtrs(nullptr); }
```

The overrides are the 33 local slots of section f.1, each forwarding to `CPatterned` behind a
once-only `StandIn(...)` log (the `CElitePirate` pattern). The small ones are the retail bodies and
need no stand-in: `CanBeShot`→true, `IsListening`→true, `Listen`→false, `ProjectileInfo`→null,
`GetSearchPath`→null, `GetPointSearchPath`→null, `GetRagDoll`→null, `GetModelAlphau8`→
`mColor.GetAlphau8()`, `TakeDamage`→`mDamageCooldownTimer = skDamageHitTime`, `GetAttackTarget`→
the base u16, `IsOnGround`→the base flag, `GetHealthInfo() const`→the base `HealthInfo`.

The loader `PortLoadDigitalGuardian` reads the same four property blocks as `fn_14_1B8`
(0x255A4580 / 0xB3774750 / 0x7E397FED / 0xC55918CC), builds `CDigitalGuardianData` field by field
per c.2, then `rs_new CDigitalGuardian(...)` with `EPatternedAI 9`, flavor 0, movement 0, collider 1,
body 1.

**WP1 done (2026-10-20).** `include/MetroidPrime/Enemies/CDigitalGuardian.hpp` and
`src/MetroidPrime/Enemies/PortCDigitalGuardian.cpp` landed: the class, its 0x640-byte
`CDigitalGuardianData` (the c.2 rows, one member per property, the loader's own types, so the
host block is the same size as the guest's), the loader, `RELMain`/`RELExit` and all 33 vtable
overrides of f.1. The retail bodies of the small accessors are the module's own
(`fn_14_0`/`fn_14_8`/`fn_14_78`/`fn_14_88`/`fn_14_90`/`fn_14_98`/`fn_14_A0`/`fn_14_A8`/`fn_14_B0`/
`fn_14_C0`/`fn_14_CC`/`fn_14_D8`/`fn_14_E0`/`fn_14_E8`/`fn_14_10C`); the other 17 forward to
`CPatterned` behind a once-only `StandIn` log. Two facts the note did not have:
`fn_14_CC` (`GetGravityConstant`) loads the DOL's imported `kDefaultGravityAccel` (the
`R_PPC_ADDR16_HA/LO` relocations at 0xCE/0xD2 name it), i.e. the same constant the base class
returns, and `fn_14_8` (`IsScanVisorSelfRender`) returns 1 while the base returns false - both are
ported as the retail bodies, not as stand-ins. The host class is 0xEF0 bytes (the base 0x7C0 plus
the 0x640 data block), not the guest's 0x1290, so it has no `CHECK_SIZEOF`; the header's
non-PC branch keeps the retail byte array (`uchar x7c0_[0xAD0]`).

What did **not** land: the head module. `mLoadDigitalGuardianHead` is filled with a documented
stand-in that logs once and returns null (the script loader helper skips the object), so the
`DGHD` actor in layer 3 is not created - WP3 is where it comes. The FSM's *bodies* (WP2 part 2),
the sub-actors (WP4) and the toe target / beacon (section j) are also still open (WP2 part 1, the
tables and the setup, landed later the same day). A run in
`09_cliff_temple_dark` (area 53 of world 1baa96c2, layer 3 `CliffsideBoss` and layer 11
`CliffsideBoss Body`) lists one `CDigitalGuardian` (`u054d`, area a53) that thinks without a
crash, with no `[port] caught` and no `reach-stub` in the log.

### WP2 — the FSM

Port `fn_14_9DFC` (SetupStateMachine) plus the body's 12 triggers, 14 states and 13 codes, and the
table layout of section b. The static initialiser `fn_14_D904` becomes the host's table builder
(one `TStateMachineFunctionTypes<CPatterned>` per entry); the table addresses themselves are
irrelevant on the host.

**WP2 part 1 done (2026-10-20): the tables and the setup.** `fn_14_9DFC` is ported as
`CDigitalGuardian::SetupStateMachine`: the base's setup first (the call at 0x9ED0), then the
module's three tables on the state manager at `this+0x350` - triggers (vtable slot +0x18, count
0xC), states (+0x14, count 0xE), codes (+0x1C, count 0xD), the three calls at 0x9EE8 / 0x9F10 /
0x9F30. On the host the tables are three `CPatterned::StateMachine::S{Trigger,State,Code}Function`
arrays in `src/MetroidPrime/Enemies/PortCDigitalGuardian.cpp`; the entries carry retail's name
strings (the state machine asset's nodes are matched by name) and the module's own member
functions, cast the way `CAIMannedTurret` and the SpacePirate port do it. The 39 bodies are
**stand-ins in this slice**: each logs `[dg] stand-in fn_14_XXXX` once and returns the neutral
value (false for a trigger). No `// @` marker: none of them is retail's body yet.

The names, the counts and the function pointers were re-derived from the asm for this slice and
agree with section b: `auto_04_00000000_data.s` gives the tables (0x9C/0x204/0x380) and the
three-word records (0xC..0x374), `auto_03_00000000_rodata.s` the name strings, and
`auto_fn_14_D904_text.s` the stores that put every record into its table (triggers 0xDB50..
0xDCC8, states 0xDCD0..0xDEB8, codes 0xDEBC..0xE00C). The host's FSM1 does not dispatch code
nodes yet (`TStateMachineState::SetCodeFunctions` is empty), so the code table is registered but
never called.

Runtime (area 53 `09_cliff_temple_dark` of world 1baa96c2, layers 3 and 11): the guardian
`u054d` has to be activated by the console (`send u054d Activate`; the layer script does it in
retail) before the FSM runs. Then the log shows `fn_14_9634` (Start), the codes `fn_14_7BD4`
(SetupBossEnergyBar) and `fn_14_76D0` (SetNormalAttackMode), the state `fn_14_95B8` (Idle) and
the triggers `fn_14_9A48` (KneeDestroyed), `fn_14_97F0` (ShouldMeleeAttack), `fn_14_9930`
(ShouldVortexAttack), `fn_14_9678` (ShouldJumpAttack), `fn_14_9994` (ShouldProtectKnees) and
`fn_14_97A4` (ShouldProjectileAttack). No `[port] caught` and no `reach-stub`.

**WP2 part 2 (next): the bodies.** The 12 triggers and the 14 states are the next slice (the
codes are one-liners); the tables' names and the state machine asset's transitions tell which
state follows which.

**WP2 part 2a done (2026-10-20): six triggers.** The bodies of `StateOver`, `IsIdle`,
`HasFaceTarget`, `KneeDestroyed`, `AllKneesDestroyed` and `ShouldProtectKnees` are ported
(`src/MetroidPrime/Enemies/PortCDigitalGuardian.cpp`); the other six triggers and the 14 states
are still stand-ins. The bodies, with the asm they come from:

| trigger | fn | what it answers | asm |
|---|---|---|---|
| StateOver | `fn_14_9AA8` | the base's animation state id (guest `this+0x6B4`) is 3 - the host's `CAnimationState::kAS_Over` | `subfic r0, r0, 3; cntlzw; srwi r3, r0, 5` (0x9AAC-0x9AB4) |
| IsIdle | `fn_14_9A9C` | bit 7 (mask 0x80) of the flag byte at `this+0x128C` | `lbz r0, 0x128c(r3); rlwinm r3, r0, 25, 31, 31` (0x9A9C-0x9AA0) |
| HasFaceTarget | `fn_14_9A60` | any of the three floats at `this+0x1278/0x127C/0x1280` is not 0.0f | three `fcmpu`/`bne` pairs (0x9A70-0x9A8C) against `lbl_14_rodata_264` (0.0f) |
| KneeDestroyed | `fn_14_9A48` | the word at `this+0x1284` is not -1 | `~value \| (value + 1)`, top bit (0x9A4C-0x9A58) |
| AllKneesDestroyed | `fn_14_9A0C` | the word at `this+0x1284` is not -1 **and** `fn_14_705C() == 0` | `cmpwi r0, -1; beq` (0x9A1C), `bl fn_14_705C; cntlzw; srwi` (0x9A24-0x9A2C) |
| ShouldProtectKnees | `fn_14_9994` | the float at `this+0x1274` is at most 0.0f | `fcmpo cr0; cror eq, lt, eq; mfcr; extrwi` (0x99E0-0x99EC) |

`fn_14_705C` (the helper `AllKneesDestroyed` calls) counts how many of the four knee ids
(`this+0xFB0`, `0xFB2`, `0xFB4`, `0xFB6`) are not `kInvalidUniqueId`; it is ported as
`CDigitalGuardian::x705C_()`. The guest members the six triggers read have no host counterpart
yet, so the host class grew one member per guest offset (`xFB0_`, `xFB2_`, `xFB4_`, `xFB6_`,
`x1082_`, `x1274_`, `x1278_`, `x1284_`, `x128C_`), each initialised with the value the retail
constructor (fn_14_B100) stores.

Three asm facts the port needed, all re-derived from the module's own code (the disassembly in
`build/G2ME01/DigitalGuardian/asm/` prints the X-form's operands in the ISA's `rA, rS` order, and
m2c reads the same bytes the same way):

- `subfic rD, rA, SI` computes `SI - rA` (not `~rA + SI`): the head's triggers (`subfic r0, r0,
  1` = "value == 1", m2c's `fn_14_17338.c`) are the calibration.
- `rlwinm`'s mask bits (`MB`, `ME`) are numbered from the MSB, so `rlwinm rD, rS, 25, 31, 31`
  keeps the byte's bit 7 (`(rS >> 7) & 1`, the `IsIdle` test) and `rlwinm rD, rS, 3, 31, 31`
  keeps bit 29; the dump's `extrwi rD, rS, 1, b` means the same bit as `1u << (31 - b)`.
- `mfcr` puts CR0 in the **top** four bits of the GPR (CR0's eq is bit 29, lt is bit 31), which
  is what `fn_14_9994`'s `cror eq, lt, eq` + `mfcr` + `extrwi` pair reads.

`ShouldProtectKnees` also looks the head module up by the uid at `this+0x1082` and, if the head
exists and its flag byte (`head+0x10F4`, bit 5) is set, answers false. The head module is WP3
and its loader is a stand-in, so no head can exist in the port and that branch cannot be taken:
the port answers the float test only, and WP3 has to add the head's flag byte.

**WP2 part 2b done (2026-10-20): five more triggers.** `ShouldVortexAttack` (fn_14_9930),
`ShouldMeleeAttack` (fn_14_97F0), `ShouldProjectileAttack` (fn_14_97A4), `VortexStruckPlayer`
(fn_14_966C) and `BreakOutOfVortex` (fn_14_9638) are ported; `ShouldJumpAttack` (fn_14_9678) is
**not** - it calls fn_14_308C (the call at 0x96D8), 0x120 bytes, which picks the target the
trigger measures the player against, and that helper is too big for one slice.

| trigger | fn | what it answers | asm |
|---|---|---|---|
| ShouldVortexAttack | `fn_14_9930` | bit 0 (mask 0x01) of the flag byte at `this+0x128C`, and false when the head's flag byte (`head+0x10F4`, bit 5) is set | `clrlwi r3, r0, 31` (0x9974); the head test at 0x9968 |
| ShouldMeleeAttack | `fn_14_97F0` | the float at `this+0x11E0` is at most zero **and** the player is within the far leg stab range of one of the four locator ids at `this+0x10C8` | the float test at 0x9860-0x9870, the locator loop at 0x98E0-0x98EC, the distance at 0x98B0-0x98D0 |
| ShouldProjectileAttack | `fn_14_97A4` | the head's flag byte (`head+0x10F4`, bit 5) - false when there is no head | `extrwi r3, r0, 1, 26` (0x97D4), the null test at 0x97C8 |
| VortexStruckPlayer | `fn_14_966C` | bit 2 (mask 0x04) of the flag byte at `this+0x128C` | `extrwi r3, r0, 1, 29` (0x9670) |
| BreakOutOfVortex | `fn_14_9638` | the word at `this+0x11B8` is 1 **and** the float at `this+0x11C8` (the data block's +0x100, `mUnknown4F5D725C`) is at most zero | `cmpwi r0, 1` (0x963C), the float test at 0x9650-0x965C |

New host members, one per guest offset: `x10C4_` (the count of the locator ids, 4 in the ctor),
`x10C8_` (the four locator segment ids, all invalid in the ctor), `x11B8_` (0 in the ctor) and
`x11E0_` (0.0f in the ctor). The guest's `this+0x7C8` (the melee range) is the data block's
+0x08, i.e. `mFarLegStabRange`, and `this+0x11C8` is the data block's +0x100, i.e.
`mUnknown4F5D725C` - both named already.

Two more facts the port needed:

- `mgr+0x14FC` is the player (its position is read at +0x54/+0x58/+0x5C); the port uses
  `mgr.GetPlayer(0)->GetTranslation()`. The melee check compares the player's x and y with the
  locator's (the transform's `m03` and `m13`, i.e. its translation's x and y) - the horizontal
  plane, the game's z being up.
- The locator ids at `this+0x10C8` are one byte each (the ctor stores four `0xFF` bytes and the
  loop at 0x98E0 steps by one), and the retail's `CSegId` is a byte as well, so the host's
  `CSegId` fits. With the ids still invalid the retail's locator transform is the identity, so
  the melee range is measured from the guardian's own origin - the same as retail before the
  legs are set up.

The head module is WP3 and its loader is a stand-in, so the four triggers that look the head up
(`fn_14_9930`, `fn_14_97A4`, `fn_14_97F0`, `fn_14_9994`) cannot take retail's head branch in
the port; WP3 has to add the head's flag byte (`head+0x10F4`, bit 5) to all four.

**WP2 part 2c done (2026-10-20): the jump trigger and the first two states.** `ShouldJumpAttack`
(fn_14_9678) and its helper fn_14_308C are ported, and so are the two states that ran in the
area-53 run, `Start` (fn_14_9634) and `Idle` (fn_14_95B8).

| piece | fn | what it does | asm |
|---|---|---|---|
| ShouldJumpAttack | `fn_14_9678` | the float at `this+0x11E4` is at most zero, and the closest active waypoint (fn_14_308C) is further than 100.0f (`lbl_14_rodata_290`) from the guardian while the player is closer to it than to the guardian | the float test at 0x96A0-0x96A8, the helper call at 0x96D8, the 100.0f test at 0x9728-0x9738, the two distance sums at 0x973C-0x9774 |
| the helper | `fn_14_308C` | the objects connected with `kSS_InternalState2` (0x49533032) and `kSM_Follow` (0x464F4C57), and among the active waypoints the one closest to the given position; its id is the answer | the FindConnectedObjects call at 0x30CC, the active test at 0x3118 (the byte at +0x20, bit 7), the distance at 0x3130-0x3150 against `lbl_14_rodata_2AC` (3.4028235e+38f) |
| Start | `fn_14_9634` | nothing (the body is a single `blr`, like `CPatterned::Start`) | 0x9634 |
| Idle | `fn_14_95B8` | on Activate the locomotion type becomes `pas::kLT_Relaxed` (1) and bit 7 of the flag byte at `this+0x128C` is set; on Deactivate that bit is cleared; Update does nothing | the message chain at 0x95C0-0x95E8, the locomotion call at 0x95F8, the rlwimi pair at 0x9604/0x9618 |

fn_14_308C's own two callees are the retail's hand-written vector copy and free (fn_14_31AC,
0x100 bytes, and fn_14_32AC, 0x54); the host's `rstl::vector` is their equivalent, so nothing is
ported for them. New host member: `x11E4_` (0x11E4, 0.0f in the ctor).

The flag byte at `this+0x128C` is now written by a state (the Idle state's bit 7) and read by
three triggers (`fn_14_9930` bit 0, `fn_14_966C` bit 2, `fn_14_9A9C` bit 7), so the "is idle"
trigger and the Idle state agree on the same byte, as in retail.

**WP2 part 2d done (2026-10-20): six code functions.** The bodies of `ResetJumpTime`
(fn_14_7B84), `ResetProtectKneesTime` (fn_14_7B90), `ResetHeadAttacks` (fn_14_7648),
`DisableHeadAttacks` (fn_14_768C), `SelectExitVortexTarget` (fn_14_7600) and `ResetAttackTimes`
(fn_14_7B24) are ported. `SetNormalAttackMode` (fn_14_76D0) is **not**: it calls fn_14_3A44
(the call at 0x7718), 0x108 bytes, which walks the four knees, resets their damage
vulnerabilities (guest `this+0x1014` and `this+0x1044`) and calls fn_14_CE94 - too big for one
slice.

| code | fn | what it does | asm |
|---|---|---|---|
| ResetJumpTime | `fn_14_7B84` | the jump timer (guest `this+0x938` = the data block's +0x178, `mJumpTimer`) into the member at `this+0x11E4` | the load at 0x7B84 and the store at 0x7B88 |
| ResetProtectKneesTime | `fn_14_7B90` | the member at `this+0x1274` becomes the data block's +0x174 (`mUnknown6A754EBD`) when all four knees are set (fn_14_705C's count >= 4, the cmpwi at 0x7BA8), otherwise the block's +0x170 (`mUnknownA324E26C`) | the call at 0x7BA4, the loads at 0x7BB0/0x7BB8, the store at 0x7BBC |
| ResetAttackTimes | `fn_14_7B24` | the member at `this+0x11E0` becomes the data block's +0x0C (`mMinLegStabAttackTime`) and the one at `this+0x11E4` the jump timer | the loads/stores at 0x7B3C-0x7B48 |
| DisableHeadAttacks | `fn_14_768C` | the head module (looked up by the uid at `this+0x1082`) is asked to disable its attacks with the argument 3 | the lhz at 0x7698, the li r4, 3 at 0x76B8, the call at 0x76BC |
| ResetHeadAttacks | `fn_14_7648` | the same lookup, the head is asked to reset its attacks with the argument 4 | the lhz at 0x7654, the li r4, 4 at 0x7674, the call at 0x7678 |
| SelectExitVortexTarget | `fn_14_7600` | the closest active waypoint to the player (fn_14_308C) and its id into the member at `this+0x1168` | the lwz at 0x761C, the addi at 0x7624, the call at 0x7628, the store at 0x7630 |

New host member: `x1168_` (0x1168, the exit vortex target's waypoint id; the ctor stores 0
there, the sth at 0x0000B67C). `ResetHeadAttacks` and `DisableHeadAttacks` are empty in the
port: the head class is WP3 and its loader is a stand-in, so no head can exist and the call
cannot be made - WP3 has to add both.

**WP2 part 2e done (2026-10-20): seven vtable overrides.** `GetTouchBounds` (fn_14_10),
`RenderSystemsToBeDrawnLast` (fn_14_A3A4), `GetOrigin` (fn_14_F0), `GetDamageVulnerability`
(fn_14_9FE4), `PreThink` (fn_14_A688), `PreRender` (fn_14_A384) and `GetScannableObjectInfo`
(fn_14_9DBC) are ported; all seven are under 0x80 bytes.

| override | fn | what it does | asm |
|---|---|---|---|
| GetTouchBounds | `fn_14_10` | the actor's bounding box, always valid | the GetBoundingBox call at 0x28, the valid byte at 0x30 |
| RenderSystemsToBeDrawnLast | `fn_14_A3A4` | the base's call, but only while the module's render flag (`lbl_14_bss_0`) is set | the lbz at 0xA3B4, the call at 0xA3C0 |
| GetOrigin | `fn_14_F0` | the actor's translation (guest `this+0x54`) | the three loads at 0xF0/0xF8/0x100 |
| GetDamageVulnerability | `fn_14_9FE4` | the "pass through" vulnerability - the guardian is not hurt, the shots pass through it | the call at 0x9FF0 |
| PreThink | `fn_14_A688` | the base's PreThink | the call at 0xA694 |
| PreRender | `fn_14_A384` | the base's PreRender | the call at 0xA390 |
| GetScannableObjectInfo | `fn_14_9DBC` | the scannable token's object when bit 6 (0x40) of the flag byte at `this+0x128C` is set and the token (guest `this+0xE68`) is there, otherwise the base's | the extrwi at 0x9DCC, the lwz at 0x9DD4/0x9DE0, the call at 0x9DE8 |

Two notes:

- The module's render flag (`lbl_14_bss_0`) is set by the module's render (fn_14_A3D4, the
  stores at 0xA3FC and 0xA44C), which is 0x94 bytes and is not ported yet, so the flag stays
  clear and `RenderSystemsToBeDrawnLast` does not forward to the base yet.
- `GetScannableObjectInfo`'s branch needs the scannable token the constructor locks at guest
  `this+0xE68`; the port's constructor does not lock it yet, so only the base's answer is
  reachable.

**WP2 part 2f done (2026-10-20): the module's render.** `Render` (fn_14_A3D4, 0x94 bytes) is
ported, which closes the gap above: the module's render flag (`lbl_14_bss_0`) is set while the
base renders (the store at 0xA3FC) and cleared again (the store at 0xA44C), so
`RenderSystemsToBeDrawnLast` (fn_14_A3A4) does reach the base's call while the actor renders.

| piece | fn | what it does | asm |
|---|---|---|---|
| Render | `fn_14_A3D4` | the module's render flag around the base's render | the flag set at 0xA3FC, the base's render (the virtual call at 0xA440), the flag cleared at 0xA44C |

The retail also asks for the particle POI mask itself before the base's render (the
GenerateParticlePOIMask call at 0xA420, under the flag byte at guest `this+0x421`, bit 0); the
host's `CPatterned::Render` does that itself when the particles are drawn, so the port leaves it
to the base.

**The guardian's model stopped drawing when `Render` was ported** - that is retail's structure,
not a port mistake: the retail's `Render` (fn_14_A3D4) never calls the base's render, it calls
its own `RenderSystemsToBeDrawnLast` (vtable slot 47) with the character's render mask and
target, and the base's `RenderSystemsToBeDrawnLast` only draws the particle systems to be
drawn last (the host's `CPatterned::RenderSystemsToBeDrawnLast`: `if (mDrawParticles) ...
RenderSystemsToBeDrawnLastPOICheck(mask, target)`). The model is drawn by the module's own
`AddToRenderer` (fn_14_A004), which calls the base's `Render` directly (the call at 0xA04C).

**WP2 part 2g done (2026-10-20): `AddToRenderer`.** fn_14_A004 (0x134 bytes) is ported, so the
guardian's model draws again (the area-53 screenshot shows its four legs on the platform and its
body hanging from the ceiling). The sequence, with the offsets: the base's `AddToRenderer`
(0xA01C), the module's GX register colour (0xA020 - the global `lbl_14_bss_30`, the static
initialiser's (0.25, 0.25, 0.25, 1)), the base's `Render` (0xA04C, the direct call that draws
the model), the character's alpha buffer alpha (0xA058) pushed to the renderer when it is not
-1 (0xA068), the module's own model renders (0xA0A0, 0xA0B4, 0xA0E8, 0xA0FC), the GX register
colour back to black (0xA0B8/0xA0BC) and the alpha again (0xA108; the cmpwi at 0xA100 skips it
when the alpha is -1).

The four module calls are all over 0x80 bytes, so they are named once-logging stand-ins for now:

| module call | size | what its pseudo-C does |
|---|---|---|
| fn_14_1994 | 0xD4 | if the flag byte at guest `this+0xEB8` is set, every locator id in the array at `this+0x10C0` is walked and the model at `this+0xE6C` is drawn at that locator's scaled transform |
| fn_14_18B4 | 0xE0 | the same walk, but when bit 4 of the flag byte at `this+0x128C` is clear and the byte at `this+0xF08` is set, drawing the model at `this+0xEBC` |
| fn_14_17DC | 0xD8 | when bit 3 of the flag byte at `this+0x128C` is clear, the byte at `this+0x1238` is set and the float at `this+0x1264` is positive, the model at `this+0x11EC` is drawn at the transform fn_14_2578 builds; the echo emitter at `this+0x123C` is then drawn through the renderer |
| fn_14_1700 | 0xDC | the locator ids at `this+0x10D4` are walked and the model at `this+0xF5C` is drawn at each of their scaled transforms |

`PreRenderAllViewports` (fn_14_A138) is 0x210 bytes - over the 0x100 limit of this slice, so it
stays a stand-in.






**WP2 part 2h done (2026-10-20): the four model renders.** `fn_14_1994` (0xD4), `fn_14_18B4`
(0xE0), `fn_14_17DC` (0xD8) and `fn_14_1700` (0xDC) - the four calls `fn_14_A004` makes at
0xA0A0, 0xA0B4, 0xA0E8 and 0xA0FC - are ported. The four model data blocks they draw are
still empty (fn_14_6864, the constructor's tail, is not ported), so nothing new draws yet.

| fn | what it does | the fields it reads | asm |
|---|---|---|---|
| fn_14_1994 | when the flag byte at 0xEB8 is set, the four locator ids at 0x10C0 are walked and the model at 0xE6C (the leg model) is drawn at each locator's scaled transform | 0xEB8, 0x10C0..0x10C3, 0xE6C | the test at 0x19B4, the loop at 0x19D4-0x1A50, the Render at 0x1A40 |
| fn_14_18B4 | the same walk, but only when bit 4 of the flag byte at 0x128C is clear and the byte at 0xF08 is set; the model at 0xEBC (the shin model) is drawn | 0x128C bit 4, 0xF08, 0x10C0..0x10C3, 0xEBC | the extrwi at 0x19D8 (the byte's bit 4, the mask 0x10), the test at 0x19E4, the Render at 0x196C |
| fn_14_17DC | when bit 3 of the flag byte at 0x128C is clear, the byte at 0x1238 is set and the float at 0x1264 is positive, the model at 0x11EC is drawn at the transform fn_14_2578 builds, with the blend mode 2 (0x1848), the shader set 0 (0x1854), the flags 3 (0x1860) and the packed colour at 0x1270 (0x1868); then the echo emitter at 0x123C, when it is there, is drawn through the renderer | 0x128C bit 3, 0x1238, 0x1264, 0x11EC, 0x1270, 0x123C | the extrwi at 0x17FC (the byte's bit 3, the mask 0x08), the fcmpo at 0x181C, the Render at 0x1870, the renderer's vtable call at 0x1958 |
| fn_14_1700 | the four locator ids at 0x10D4 are walked and the model at 0xF5C (the toe target's model) is drawn at each of them, unless the word at 0x10DC is 3 | 0x10D4..0x10D7, 0x10DC..0x10E8, 0xF5C | the loop at 0x1738-0x17C4, the cmpwi at 0x173C, the Render at 0x17B0 |

All four draw at the same transform: the actor's transform (guest this+0x24) times the
locator's scaled transform (`GetScaledLocatorTransform`) times the segment's rotation. That
rotation is the character's rest pose rotation for the segment: the retail reads it from the
actor's model data (this+0x60), the anim data (that+0x10), the character layout (that+0x10C)
and the layout's linear rotations (that+0x68), indexed with the segment id - the chain
`CScriptSpecialFunction::ThinkObjectFollowJoint` walks as well. The port reads it from
`GetModelData()->GetAnimationData()->GetCharLayoutInfo()->GetLinearRotations()`.

The locator ids: 0x10C0..0x10C3 are the four knee locators ("L_front_knee_2",
"L_back_knee_2", "R_front_knee_2", "R_back_knee_2") and 0x10D4..0x10D7 the four "*_col_LCTR"
ones; the constructor resolves them with `GetLocatorSegId` (the calls from 0xBAF0 on), which
is not ported yet, so they are invalid (0xFF) in the port and the locator transform is the
identity. 0x10B9 is a locator id fn_14_2578 walks; its name is not decoded yet.

`fn_14_2578` (0xF4 bytes) builds the echo emitter's transform and is over this slice's limit,
so it is a named once-logging stand-in: it walks the locator at guest 0x10B9, multiplies the
actor's transform with it and, when the byte at 0x1238 is set, moves the result along its own
z axis by (this+0x1264 - 0.5f) * (0.8f * the model's height) - the constants are .rodata 0x29C
and 0x2A0 and the height comes from the model at 0x11EC (the `GetBounds` call at 0x269C).
The next slice can port it.

The last call of fn_14_17DC is the renderer's vtable slot at 0x44 (the lwz at 0x1954 and the
bctrl at 0x1958), with no argument; the slot is not identified, so the port has a named
once-logging stand-in for it. The echo emitter at 0x123C is the 0x338 `CElementGen` that
fn_14_D664 allocates (not ported), so that branch cannot be taken in the port.

The host members this slice added (one per guest offset): the four model data blocks (0xE6C,
0xEBC, 0xF5C, 0x11EC), the three flag bytes (0xEB8, 0xF08, 0x1238), the eight locator ids
(0x10C0..0x10C3 and 0x10D4..0x10D7), the four per-leg words (0x10DC..0x10E8), the echo
emitter pointer (0x123C), the float at 0x1264 and the packed colour at 0x1270. Their initial
values are the ones the retail constructor stores: the flags 0, the ids 0xFF, the words -1
(`lbl_14_data_454`), the float 0.0f and the colour word 0 (`lbl_14_rodata_264`).

Runtime (area 53 `09_cliff_temple_dark` of world 1baa96c2, layers 3 and 11, 1800 frames): the
guardian u054d is there (hp 150/150) and its model draws (the base's render through the
module's AddToRenderer, `/tmp/opencode/dg_render_models.png`); the four module renders draw
nothing yet, since their model data is empty. No "[port] caught" and no reach-stub in the log.


**WP2 part 2i done (2026-10-20): the models and the locator ids.** `fn_14_6864` (0x1BC, the
constructor's tail) is ported, and so are the constructor's `GetLocatorSegId` calls for the ids
the four renders read. The five models are real now and the ids resolve, so the toe target's
model draws at the guardian's four ankles.

`fn_14_6864` builds the module's five models, each from a `CStaticRes` {the asset id, the
actor's model scale} (the scale comes from the actor's model data at 0x60 - the three lfs at
0x68A0, 0x68A4 and 0x68A8) and moved into the member (fn_14_6A20, 0x30 bytes - the `CModelData` assignment at
0x68D4; the host's implicit copy-assignment is the same thing, the auto_ptr's assignment
transferring the anim data, and the temporary is destroyed right after, the __dt__ at 0x68E0).
An invalid asset id (the cmplwi at 0x689C) leaves the member empty.

| member | asset id (guest) | the data block's field |
|---|---|---|
| 0xE6C, the leg model | 0x8DC | `mLegModel` (the block's +0x11C) |
| 0xEBC, the shin model | 0x8E0 | `mShinArmor` (+0x120) |
| 0xF0C, the knee model | 0x918 | `mKneeArmor` (+0x158) |
| 0xF5C, the toe target's model | 0x7E4 | `mToeTargetModel` (+0x24) |
| 0x11EC, the transmission beacon's model | 0x94C | `mTransmissionBeacon` (+0x18C) |

**Each model is an `rstl::optional_object<CModelData>`**, not a bare `CModelData`: the guest's
CModelData is 0x4C bytes and the byte right after it is the optional's valid flag - 0xE6C's at
0xEB8, 0xEBC's at 0xF08, 0xF0C's at 0xF58, 0xF5C's at 0xFA8 and 0x11EC's at 0x1238. The
constructor's stores of 0 at 0xEB8, 0xF08, 0xF58 and 0xFA8 are the empty optionals, not flags
that gate the draws.

`fn_14_6A20` (0x30 bytes) is the **optional's assignment**, not the model's: `fn_14_6A50` reads
the valid flag at +0x4C, and when the optional was empty it constructs the model in place
(fn_14_528) and sets the flag (the `li r0, 1; stb r0, 0x4c(r31)` at 0x6B74), and when it was
already valid it copies the model into it (fn_14_6A98). The host's `rstl::optional_object` does
the same, so the port assigns into the optionals.

The draws test the flag: fn_14_1994 tests the leg model's (the byte at 0xEB8, the test at
0x19B4), fn_14_18B4 the shin model's (0xF08, the test at 0x19E4) and fn_14_17DC the beacon's
(0x1238, the test at 0x1808). fn_14_1700 does **not** test the toe model's flag (0xFA8): the
retail's optional default-constructs the model, so the draw is of an empty model; the host's
optional leaves the data unconstructed while it is invalid, so the port skips the draw.

The sixth `fn_14_6A20` call site is in `fn_14_137A8` (0x88 bytes), which the head module's
constructor (`fn_14_18264`) calls: it builds the **head's** own `optional_object<CModelData>` at
0xEA4 from the head's `mHeadArmor` (guest 0x7CC, the head data block's +0x0C) with the head's
model scale. It is not a body member.

The constructor's locator ids (the `GetLocatorSegId` calls from 0xBAF0 on):

| member | the name | the store |
|---|---|---|
| 0x10B9 | "neck_SDK" (.rodata 0x91A) | 0xBAFC |
| 0x10C0..0x10C3 | "L_front_knee_2", "L_back_knee_2", "R_front_knee_2", "R_back_knee_2" (.rodata 0x57D, 0x58C, 0x59A, 0x5A9) | 0xBB30, 0xBB64, 0xBB98, 0xBBCC |
| 0x10D4..0x10D7 | "L_front_ankle", "L_back_ankle", "R_front_ankle", "R_back_ankle" (.rodata 0x5F9, 0x607, 0x614, 0x622, through the pointer table at 0x20) | 0xBCD0, 0xBD04, 0xBD38, 0xBD6C |

The ids at 0x10C8..0x10CB (the four "*_col_LCTR" names, the pointer table at .rodata 0x10) are
the melee trigger's and are still not resolved.

**The renderer's vtable slot 0x44** (the lwz at 0x1954 and the bctrl at 0x1958) is
`CCubeRenderer::AddParticleGen(const CParticleGen&)`: the DOL's `__vt__13CCubeRenderer` is at
0x803B8C10 and its entry at 0x44 (the 18th) is that function - the counting the SpacePirate
lane used for 0x128 (`SetDestinationAlpha`) ✓. The retail passes the echo emitter in r4 (the
lwz at 0x1874), so the object at 0x123C is a particle generator (the host's `CElementGen`, a
`CParticleGen`) and not an "echo emitter" as the note used to call it. The port now calls
`gpRender->AddParticleGen(*x123C_)`; the object itself is still missing (fn_14_D664 allocates
it), so the branch is not reachable yet.

Runtime (area 53 `09_cliff_temple_dark` of world 1baa96c2, layers 3 and 11, the guardian
activated with `send u054d Activate`, 1800 frames): the guardian u054d is active (hp 150/150)
and draws with the leg model, the shin model (both at the four knee locators) and the toe
target's model (at the four ankle locators) - the draw calls go from 324 to 356 and the
vertices from 914044 to 1024628 (`/tmp/opencode/dg_render_models.png`); the toe model's four
draws land at the four ankle locators (the locators at (+-19, +-12, 7) around the guardian).
The transmission beacon's model does not draw: the retail's test at 0x1264 (> 0.0f) fails, the
ctor leaves that float at 0.0f. No "[port] caught" and no reach-stub in the log. The
guardian's render path only runs while the actor is active, so a run that does not activate it
shows the arena but not the guardian.


**WP2 part 2j done (2026-10-20): the beacon.** The three pieces the beacon needs are ported:
`fn_14_2578` (0xF4, the beacon's transform), `fn_14_D664` (0x178, the particle generator) and
`fn_14_2CC4` (0x19C, the beacon's update - the only writer of the height at 0x1264). The
constructor also resolves the melee trigger's locators (0x10C8..0x10CB) now.

| piece | what it does |
|---|---|
| fn_14_2578 | the beacon's transform: the locator at 0x10B9 scaled (the GetScaledLocatorTransform call at 0x26A4), times the actor's transform (the __ml__ at 0x26B4), and - when the beacon's model is there (the byte at 0x1238, the model's valid flag, the test at 0x26C4) - moved along the transform's z axis by (0x1264 - 0.5f) * (0.8f * the model's height): the constants are .rodata 0x29C (0.5f) and 0x2A0 (0.8f) and the height is the model's bounds (the GetBounds call at 0x269C, the box's min z at 0x10(sp) and max z at 0x1c(sp)) |
| fn_14_D664 | the beacon's particle generator: the "PART" object (the SObjectTag the retail builds at 0xD690 - the type "PART" and the id) becomes a CElementGen with the normal orientation and the kOSF_One flag (the li r5, 0 and li r6, 1 at 0xD720). The constructor calls it with the beacon's effect asset (guest 0x950, the data block's mTransmissionBeaconFx, the call at 0xB6AC) and keeps the pointer at 0x123C. The retail never frees it (its destructor leaves 0x123C alone), so the port does not either |
| fn_14_2CC4 | the beacon's update: it runs while bit 3 of the flag byte at 0x128C is clear (the extrwi at 0x2CF0 - the byte's bit 3, the mask 0x08). The state word at 0x1268 drives the height (0x1264): in state 2 it rises by the frame time, clamped to 1.0 (the fcmpo at 0x2D28, the constant 1.0f at .rodata 0x270), and when it reaches 1.0 (the fcmpo at 0x2D48 with the cror eq, gt, eq) fn_14_28D0 runs and the state becomes 1 (the store at 0x2D64); in state 3 it falls by twice the frame time (the constant 2.0f at .rodata 0x26C), clamped to 0.0 (the fcmpo at 0x2D8C), and when it reaches 0.0 (the fcmpo at 0x2DA8 with the cror eq, lt, eq) the state becomes 0 (the store at 0x2DB8). The sound emitter (the handle at 0x1240, set by fn_14_28D0) follows the neck locator (the GetLctrTransform call at 0x2DD4) and its volume is 0x7F while the echo visor is active (the GetActiveVisor call at 0x2DF8 - the retail's visor 1, which is the echo visor) and the low byte of the transmission beacon's sound parms' maxVolume otherwise (the clrlwi at 0x2E10; the guest's 0x974 is the data block's +0x1B4) |

`fn_14_2CC4`'s callees are over this slice's callee limit and are named once-logging stand-ins:
**fn_14_28D0** (0x158 - the beacon's arrival: it hands the collision actor at 0x10CC a flag, starts
the particle generator, plays the arrival sound, which is what sets the emitter handle at 0x1240,
and lets the visor orbit the beacon) and **fn_14_2A28** (0x29C - the beacon's effect update: it
aims the particle generator at the player and drives its external parameter).

fn_14_2CC4's caller is **fn_14_7CB4** (the Crippled state), which is still a stand-in, so the
beacon's height stays at 0 in the port and **the beacon's model does not draw yet** - the
beacon's particle generator exists and is handed to the renderer every frame (fn_14_17DC's
AddParticleGen), but nothing drives it until the Crippled state is ported.

The constructor's melee locators (0x10C8..0x10CB) are the four "*_col_LCTR" names, taken from
the pointer table at .rodata 0x10 (the stores at 0xBC00, 0xBC34, 0xBC68 and 0xBC9C): "L_front_
col_LCTR", "L_back_col_LCTR", "R_front_col_LCTR" and "R_back_col_LCTR".

Runtime (area 53 `09_cliff_temple_dark` of world 1baa96c2, layers 3 and 11, the guardian
activated with `send u054d Activate`, 1800 frames): the guardian u054d is active (hp 150/150)
and draws with the leg, shin and toe models; the beacon does not draw yet (its height is 0, see
above). No "[port] caught" and no reach-stub in the log.


**WP2 part 2k done (2026-10-20): the Crippled state and the beacon's arrival.** `fn_14_7CB4`
(the Crippled state, 0xD8) and `fn_14_28D0` (the beacon's arrival, 0x158) are ported;
`fn_14_2A28` (the beacon's effect update, 0x29C) stays a stand-in.

`fn_14_7CB4` dispatches on the state message the retail's way (the cmpwi at 0x7CBC against 1 and
the cmpwi at 0x7CE0 against 0 - the host's `kStateMsg_Update` and `kStateMsg_Activate` have the
same values): on Update the beacon's update runs (fn_14_2CC4, the call at 0x7D6C) and on
Activate the knee joint colliders are woken and every collision actor but the guard's is put to
sleep (the SetActive call at 0x7CF4 and the loop at 0x7D00-0x7D64). The activate path needs the
collision actor managers at 0xE60 and 0xE64, which the constructor does not build yet, so it is
a named once-logging stand-in.

`fn_14_28D0` is the beacon's arrival: the guard's collision actor (the uid at 0x10CC) is woken -
when its flag word's bit 24 is clear (the extrwi at 0x2910 - the byte's bit 7, the mask 0x80 -
MWCC packs the bitfields from the MSB, so this is the CEntity's **first** bitfield, `mActive`,
not `mUpdateDuringCinematicSkip`) the `CEntity::SetActive(true)` runs (the vtable slot 0x1C) - the particle generator starts (the CParticleGen's vtable slot 0x2C with the argument
1, i.e. `SetGeneratorRate(1.f)`), the arrival sound plays (the PlayCustomSound call at 0x29E8: the
position is the neck locator's translation, the direction the up vector and the parms are the
transmission beacon's sound parms, the guest's 0x968..0x97F, with the maxVolume at 0x7F while the
echo visor is active) and its handle goes to 0x1240, and the dark visor may orbit the beacon
(the SetVisorOrbitableFlags call at 0x2A0C - the flag 8 is `kVOF_Dark`).

**How the fight reaches Crippled** (the retail's chain, none of it in the port yet): the
"VCTA"/"VCTD" script messages (the module's AcceptScriptMsg, fn_14_A6A8) call fn_14_3E88, which
wakes the knee joint colliders; a knee's destruction (the helper that writes the word at 0x1284
and asks for the knee's deletion) is what the `KneeDestroyed` trigger reads (fn_14_9A48, ported),
and the state machine's transition then enters Crippled. In the port the word at 0x1284 stays -1:
the module's AcceptScriptMsg (fn_14_A6A8, 0x4F0) is a stand-in and the port has no knee actors
(since 2026-10-20 the knee chain fn_14_3A44/fn_14_5838/fn_14_5470 is ported and sets 0x1284,
see "Knee chain" below; KneeDestroyedReaction fn_14_8090 is still a stand-in), so the
Crippled state is still unreachable - the beacon's pieces are in place, but nothing runs them. The
console's `damage` does not help either: the guardian's vulnerability is pass-through
(fn_14_9FE4) and the retail's damage path (fn_14_78) is not ported.

Runtime (area 53 `09_cliff_temple_dark` of world 1baa96c2, layers 3 and 11, the guardian
activated with `send u054d Activate` and hit with `damage u054d 500`, 1800 frames): the run is
clean, the guardian u054d is active (hp 150/150 - the damage passes through it) and draws with
the leg, shin and toe models; the Crippled state is never entered, so the beacon does not draw
yet. No "[port] caught" and no reach-stub in the log.


**WP2 part 2l done (2026-10-20): the knee colliders' wake path, and how Crippled is reached.**
`fn_14_3E88` (0x88) is ported and the module's AcceptScriptMsg handles the two messages that
call it; the `fn_14_28D0` flag test is fixed (the byte's bit 7 is the CEntity's `mActive`, the
first bitfield MWCC packs from the MSB).

`fn_14_3E88` (0x88 bytes) walks the four knee ids (0xFB0, 0xFB2, 0xFB4 and 0xFB6 - the loop at
0x3EB0) and sets the active flag of every knee that exists (the compare with kInvalidUniqueId at
0x3EB4, the ObjectById call at 0x3ED0 and the `CEntity::SetActive` through the vtable slot 0x1C
at 0x3EE4) to the flag the message carries (the clrlwi at 0x3EA4, the low byte of the third
argument). The module's AcceptScriptMsg (fn_14_A6A8) calls it for the messages "ACTV"
(0x41435456) and "DCTV" (0x44435456); the port handles those two cases and the rest of that
function is still a stand-in.

**Where the knee collision actors come from** (the retail's chain, and why the port cannot reach
Crippled yet): the managers at 0xE60 and 0xE64 are **not** built by the constructor or by the
AcceptScriptMsg - `fn_14_676C` (0xF8) builds them, and it is called from the module's
`Think` (fn_14_A5BC, the call at 0xA604) - see WP2 part 2m, which corrects the call site this
section guessed. It calls `fn_14_61DC` (0x590, the manager at 0xE60) and `fn_14_5C30` (0x2F4,
the manager at 0xE64), which build the joint collision descriptions; both are over this
slice's ~0x200 limit, so they would be stand-ins and the managers would stay empty - and the
knee actors themselves belong to the knee armour sub-actor (fn_14_4080, not ported), so the
knee ids at 0xFB0..0xFB6 stay invalid.
With no knee actor the helper that writes the word at 0x1284 (the one that also asks for the
knee's deletion) has nothing to delete, the word stays -1, the `KneeDestroyed` trigger
(fn_14_9A48, ported) never fires and the state machine never enters Crippled. So 7CB4's
activate path stays a stand-in and the beacon does not draw yet.

Runtime (area 53 `09_cliff_temple_dark` of world 1baa96c2, layers 3 and 11, the guardian
activated with `send u054d Activate` and hit with `damage u054d 500`, 1800 frames): the run is
clean, the guardian u054d is active (hp 150/150 - the damage passes through it) and draws with
the leg, shin and toe models; there are no knee collision actors and the Crippled state is not
entered, so the beacon does not draw. No "[port] caught" and no reach-stub in the log.

**WP2 part 2m done (2026-10-20): the collision actor managers' builder.** `fn_14_676C` (0xF8)
is ported, its three builder callees are named stand-ins, and it is wired in at its retail
call site.

**The call site is the Think, not an animation event.** The call at 0xA604 is in
`fn_14_A5BC` (0xCC bytes), the module's `Think` (vtable slot 5) - not in `DoUserAnimEvent`
(fn_14_A468), as WP2 part 2l guessed. The Think's body is: the active bit test (the byte at
guest 0x20, bit 0, read at 0xA5E0), the base's `CPatterned::Think` (the call at 0xA5EC), then
the manager at guest 0xE60 (the load at 0xA5F0): when it is null the retail calls
`fn_14_676C` (0xA604) to build both managers, otherwise it updates them (the two
`CCollisionActorManager::Update` calls at 0xA610 and 0xA620, the options word 0 =
`kUO_ObjectSpace`). So the trigger is **the manager at guest 0xE60 being null** - there is no
animation event type in it. The Think then calls `fn_14_6E04`, `fn_14_2358`, `fn_14_38BC` and
`fn_14_3F90`, which the port does not have yet.

**`fn_14_676C` (0xF8 bytes)**, in retail order: `fn_14_61DC` (the call at 0x678C, 0x590
bytes - the manager at 0xE60), `fn_14_5C30` (0x6798, 0x2F4 bytes - the manager at 0xE64),
`fn_14_59BC` (0x67A4, 0x274 bytes), `fn_14_3974` (0x67B4, with the third argument 0) and the
material filter (the `SetMaterialFilter` call at 0x6844). The three builders are over the
~0x200 callee limit, so they are named once-logging stand-ins: the managers at 0xE60 and 0xE64
stay null and the knee ids at 0xFB0..0xFB6 keep their invalid value - Crippled is still
unreachable, exactly as WP2 part 2l described.

**`fn_14_3974` (0x58 bytes)** is ported: the object with the id at guest 0x1080 (the halfword
load at 0x3988) is looked up (the `ObjectById` call at 0x3998) and its active flag is set
(the virtual call at 0x39B4, the vtable slot 0x1C = `CEntity::SetActive`) to the flag the
third argument carries (the `clrlwi` at 0x39A8, its low byte).

**The material filter** (0x67B8..0x6844) is built from the four `.rodata` words 0x470 (0x2F),
0x474 (0x1C), 0x478 (0x20) and 0x46C (0x3B) with the MWCC helper `__shl2i` (the DOL's
0x80345A44, 0x24 bytes). The helper is a 64-bit shift of the pair (r3:r4) by r5 that returns
its high word in r3 and its low word in r4; for the call shape `__shl2i(0, 1, word)` it
computes `1 << (word & 31)` in **both** words. The include list is the fourth word's shift
(0x08000000 in both halves, the material types 27 and 59) and the exclude list is the or of
the first three (0x10008001 in both halves, the types 0, 15, 27, 28, 32, 35, 59 and 60); the
type word is 3, `kFT_IncludeExclude`. The port sets exactly those two lists.

Runtime (area 53 `09_cliff_temple_dark` of world 1baa96c2, layers 3 and 11, the guardian
activated with `send u054d Activate` and hit with `damage u054d 500`, 1800 frames): the run is
clean, the guardian u054d is alive, and the log shows the three builder stand-ins once each -
so the Think's manager branch runs. No "[port] caught" and no reach-stub in the log.

**WP2 part 2n done (2026-10-20): the module's AcceptScriptMsg cases.** `fn_14_A6A8` (0x4F0)
switches on the message's four-character code - a big-endian u32 of the four characters, each
compare an `lis`/`addi` pair. The dispatch runs from 0xA6BC (the message load) to 0xA818, and
the cases are:

| message | FourCC | case | body |
|---|---|---|---|
| "IM01" | 0x494D3031 | 0xAAC8 | the byte at 0x1010 = 1, then fn_14_3A44 |
| "IM02" | 0x494D3032 | 0xAAE0 | the byte at 0x1010 = 0, then fn_14_3A44 |
| "IM03" | 0x494D3033 | 0xAAF8 | the byte at 0x1012 = 1, then fn_14_3A44 |
| "IM04" | 0x494D3034 | 0xAB10 | the byte at 0x1012 = 0, then fn_14_3A44 |
| "IM05" | 0x494D3035 | 0xAB28 | the byte at 0x1013 = 1, then fn_14_3A44 |
| "IM06" | 0x494D3036 | 0xAB40 | the byte at 0x1013 = 0, then fn_14_3A44 |
| "IM07" | 0x494D3037 | 0xAB58 | the byte at 0x1011 = 1, then fn_14_3A44 |
| "IM08" | 0x494D3038 | 0xAB70 | the byte at 0x1011 = 0, then fn_14_3A44 |
| "IM00" | 0x494D3030 | 0xAAB8 | fn_14_3F14 |
| "XALC" | 0x58414C44 | 0xA998 | fn_14_5910 |
| "XAUD" | 0x58415544 | 0xA9A8 | the two managers' `SetPhysicsActive(mgr, false)` |
| "XDMG" | 0x58444D47 | 0xA9DC | fn_14_5838 with the message's first halfword |
| "XRDG" | 0x58524447 | 0xA9FC | fn_14_5644 with the message's first halfword |
| "XHIT" | 0x58484954 | 0xAA1C | fn_14_4F2C with the message's first halfword |
| "ACTV" | 0x41435456 | 0xA8DC | the managers' `SetActive(mgr, true)`, fn_14_3E88, fn_14_7B24 |
| "DCTV" | 0x44435456 | 0xA94C | the managers' `SetActive(mgr, false)`, fn_14_3E88 |
| "OPEN" | 0x4F50454E | 0xAA3C | the state word at 0x1268 = 2 and the beacon's sound |
| "CLOS" | 0x434C4F53 | 0xAA80 | the flag byte's bits 3 and 4, then the delete request |
| "XDEL" | 0x5844454C | 0xA880 | the managers' `Destroy`, the sound emitter, fn_14_3F14 |
| "XCRT" | 0x58435254 | 0xA81C | body controller `Activate`, fn_14_4080, fn_14_24E4, visor flags (0xF off, 2 on), `RemoveMaterial(0x3F)`, fn_14_14F8 |

All cases are ported (the "XCRT" case since 2026-10-20, see "Knee chain" below). The helpers
the ported cases call - fn_14_3F14, fn_14_5910, fn_14_5644 and fn_14_4F2C - are not ported
either (fn_14_3A44 and fn_14_5838 are, 2026-10-20), so each case logs them once.

The "ACTV" and "DCTV" cases only run when the actor's active bit is clear resp. set (the retail
reads the byte at guest 0x20, bit 7, at 0xA6C4, before it calls the base), and the "ACTV"
case only wakes the second manager when the flag byte's bit 6 is set (the `extrwi` at 0xA90C).
The "OPEN" case needs the flag byte's bit 3 clear (the `extrwi` at 0xAA40) and the state word
at 0x1268 different from 1 (the `cmpwi` at 0xAA4C); it plays the data block's +0x1C0 parms
(sound_BeaconExtract) at the actor's translation. The "CLOS" case sets the flag byte's bits 3
and 4 (the two `rlwimi` at 0xAA88 and 0xAA9C) and asks for the actor's own object to be
deleted (the halfword at guest 0x8, the actor's id).

Runtime (the same area-53 run): the run is clean, the guardian is alive, and the log shows the
new case stand-ins ("XCRT", "XALC", "XRDG" and the "XDEL" case's helper) - the area's scripts
do send those messages. No "[port] caught" and no reach-stub in the log.


### Knee chain (2026-10-20)

Ported: the collider builders fn_14_61DC (the xE60 manager: spheres, OBBs, per-collider
vulnerabilities 0x1044/0x10CC/0x1110/0x1188), fn_14_5C30 (the xE64 manager: four knee-to-ankle
OBBs, built inactive) and fn_14_59BC (collider health, knockback resistance 10); the knee-armour
actor `CDigitalGuardianKneeArmor` (port-only `PortCDigitalGuardianKneeArmor.cpp`: ctor fn_14_D4D0,
Think fn_14_CF94, AcceptScriptMsg fn_14_D320, GetDamageVulnerability fn_14_D7BC = pass-through,
GetTouchBounds fn_14_D774, the armoured toggle fn_14_CE94); fn_14_4080 (one armour per knee
locator, ids to 0xFB0..0xFB6), fn_14_24E4/fn_14_1A68 (toe-target state: effects "ToeTarget*",
"ToeFlinch*", "SWBuildUp*", sounds, the "Toe Target Explosion Fx"), fn_14_14F8 (CPatterned's
speed, guest 0x3C8, from data +0x17C..+0x188 by armours left) and the "XCRT" case. New host
members: 0xE00 (the actor parameters, fn_14_C57C), 0x1100 (the explosion token), 0x10F0, 0x1144,
0x1158.

Retail damage flow: a knee collider's "XDMG" reaches fn_14_5838, which picks the head
collider (0x10CC, fn_14_5274), a knee (fn_14_3E2C finds it in 0xFBC..; fn_14_5470 removes the
armour and fires KneeDestroyed when its health is gone) or the root (fn_14_43AC / 0x1080,
fn_14_5384). The knee colliders start with the ricochet vulnerability (0x1044); the "IM0x"
messages' helper fn_14_3A44 swaps in the vulnerable one (0x1014) and calls fn_14_CE94 on the
armour. Ported since the second slice: fn_14_3A44 (+ the 0x1014 vulnerability), fn_14_3E2C,
fn_14_5470 (with fn_14_143C, which moves the IS06/Follow waypoint to the armour and sends IS07;
0x10CE, which it decrements, is only set by the stand-in fn_14_5910) and fn_14_5838 (its head and
root paths fn_14_5274/fn_14_5384 are stand-ins).

Runtime, first slice (area 53, layers 3/11, `send u054d Activate`): clean; four
`CDigitalGuardianKneeArmor` at the knee locators (inactive, like the guardian at XCRT); the knee
colliders take `damage <id> 500` as 300 -> 300 (ricochet).

Runtime, second slice (`/tmp/opencode/dgknee/run2.sh`): clean (0 caught, 0 reach-stub). After
`send u054d 0x494d3031/3033/3035/3037` (IM01/03/05/07) each knee takes `damage 100` as 300 -> 200
(the armour gets "XDMG"); `damage 500` takes it to -300, deletes its armour and sets 0x1284, and
the FSM enters KneeDestroyedReaction (the fn_14_8090 stand-in logs). The level's IS07 links
re-close the other knees (a later `damage` ricochets until IM0x is resent); with IM0x resent all
four knees and armours go. Crippled is not entered: KneeDestroyedReaction (fn_14_8090) is a
stand-in, so the walk stops there (guardian 150/150).

### WP3 — the head module

`CDigitalGuardianHead : CPatterned` (guest size 0x10F8, `EPatternedAI 0xA`), its data struct
(c.3), its 24 local overrides and its 20/17/29 FSM tables, plus `PortLoadDigitalGuardianHead`
(the loader `fn_14_DF7C`, property block 0x0F4D73B7).

### WP4 — the sub-actors

| piece | guest class | size | where |
|---|---|---|---|
| knee armour | `CDigitalGuardianKneeArmor : CActor` (ported), vtable `lbl_14_data_694` | 0x1A0 | `fn_14_4080` |
| echo target | `CActor` subclass, vtable `lbl_14_data_10D0` | 0x790 | `fn_14_12314` |
| guardian projectile | `CEnergyProjectile` subclass, vtable `lbl_14_data_1034` | 0x568 | `fn_14_10CA0` |
| vortex | `CShockWave` | 0x9F8 | `fn_14_458C` |
| toe target | **not identified** — see j | ? | ? |
| transmission beacon | **not identified** — see j | ? | ? |

The entity-type tags for these are **not** in the reserved 112/113 pair (those are the two
`CPatterned` classes); the sub-actors would need fresh ids in `CEntityInfo.hpp`.

## j. Open

1. The **toe target** and the **transmission beacon** have no identified class. `toeTargetModel`
   (data+0x24 = this+0x7E4) is loaded into a `CModelData` at this+0xF5C by `fn_14_6864`, so the toe
   target may be the guardian's own model rather than a separate actor; the same is not yet shown
   for the beacon (its only traces are the "Beacon Explosion Fx" / "Beacon Hit Fx" `CExplosion`
   spawners `fn_14_266C` / `fn_14_2E60`).
2. `EPatternedAI` values 9 (body) and 0xA (head) are not named in
   `include/MetroidPrime/Enemies/CPatterned.hpp`'s enum; the port needs new names (the DOL's
   `EPatternedAI` numbering is not in the upstream header).
3. The guest member layout of section e is derived from the ctor's stores; the *host* class should
   not mirror it, so any port that wants the guest offsets must keep them in a separate struct.
4. `GetGravityConstant` returns a float constant from `.rodata+0x23C`; its value was not decoded.
5. The seven three-word records at `.data 0x640..0x694` (`fn_14_9014`, `fn_14_8C70`, `fn_14_8978`,
   `fn_14_8738`, `fn_14_8328`, `fn_14_7FEC`, `fn_14_8048`) are used by the crippled/stunned
   reactions but are not in any of the three FSM tables; their role is unknown.
6. `fn_14_1A68` plays the per-leg effect states `ToeTargetFL/BL/FR/BR`,
   `SWBuildUpFL/BL/FR/BR`, `ToeFlinchFL/BL/FR/BR` — whether the toe target is a separate actor or a
   model node decides whether the port needs one.
7. `Splitter` (the head's sub-actor, layer 10 of the same area) is module 15/16-era and is still a
   stub in this port (`tools/modules_port.txt`); the head's `DeploySplitters` state cannot be
   exercised until it lands.

### Knee reactions to Crippled (2026-10-20)

- Ported: fn_14_8090 KneeDestroyedReaction, fn_14_3D2C (bit 5 of 0x128C + IS10/IS11), fn_14_7D8C CrippledReaction (no head path: the head class is unported), fn_14_5910 XALC (0x11C0/0x10CE/0x116A), fn_14_5384 + 5140 + 4350 (toe/root damage; vortex health 0x11C8, which BreakOutOfVortex now reads).
- Armours were inactive because fn_14_3E88 cast the 0xFB0 ids to CCollisionActor; retail calls SetActive on the plain ObjectById result. Fixed: they activate on ACTV.
- Area-53 run: each knee kill gives Idle -> KneeDestroyedReaction -> Idle and the 0x10CE counter drops 9->5. The 4th kill gives CrippledReaction -> Crippled (fn_14_7CB4 is still a stand-in). Knees close during the reaction (bit 5), so a kill needs ~15 s between hits.
- Open: fn_14_5274 (head collider) needs fn_14_266C/2E60 and the beacon explosion token at 0x1244/0x1250 loaded in the ctor. fn_14_5140's toe path hasn't run yet, so it is unmarked.

### Crippled and the beacon (2026-10-20)

- Ported: fn_14_7CB4 Crippled (wakes 0xE64; 0xE60 goes to sleep except the head collider 0x10CC), and fn_14_5274/266C/2E60 for beacon damage. The ctor now locks the beacon tokens 0x1244/0x1254 (data +0x1F4/+0x1F8). 0x1168 starts as invalid. All eight manager null tests in AcceptScriptMsg are retail's and cite their offsets.
- Area-53 run: the beacon (head collider) goes 240 → 140 (fn_14_2E60 returns early: the collider is asleep) → −160 (back to 250; fn_14_266C returns early: x1268 is 0, the beacon is closed). The guardian stays at 150/150, and `damage u054d 50` does nothing.
- Blocker: OPEN (which opens the beacon) crashes in x28D0_ through `mgr.GetPlayerState()->GetActiveVisor` because the pointer is null. Retail writes CStateManager 0x150C only in fn_80040D88, to mPlayerStates[i]. Upstream sets it in the per-viewport render setup and nulls it in EndPlayerRender, so it is null during Think.

### Head module (2026-10-20)

- New `CDigitalGuardianHead` (DGHD, guest 0x10F8) in `PortCDigitalGuardianHead.cpp` + `CDigitalGuardianHead.hpp`. Ported: the loader, ctor, Think and AcceptScriptMsg (all cases; their big callees are stand-ins), fn_14_13830 (XALD link-up), fn_14_119E4, fn_14_13AE0, render and vulnerability overrides, SetupStateMachine, all 20 triggers, and the Start/Idle states. The other 15 states and all 29 codes are named stand-ins. So are CVisualKnowledge::Update (no host class), fn_14_13318, fn_14_12314, fn_14_12818, fn_14_1223C, fn_14_15598 (it needs fn_14_10120), XDMG fn_14_1303C and XRDG fn_14_12D98.
- Body side: fn_14_7168 (head link) and fn_14_70B4 are ported. fn_14_7B24 calls the head's fn_14_13B08. The body attack triggers honour head flag 0x20. 0x1082/0x1084/0x1086 are TUniqueIds.
- Area-53 run: the head spawns (u051b, 150/150) and XALD links it to the body (u054e). After Activate its FSM goes Start → Idle → FaceTarget → DetachFromBody; those last two are stand-ins, so it goes no further. Its marked triggers are exercised: StateOver, MainBodyDestroyed, IsStunned, IsGuarding and HeadArmorDestroyed. The guardian stays at 150/150 and the fight does not end. No `[port] caught` or reach-stub.
- Fight path (second slice): the code table's addresses were off by one from index 17 (`fn_14_14604` is EnableValidTarget; `fn_14_13B38` is a helper, not StartVocalRecoverReaction = `fn_14_13C1C`). Ported in run order: FaceTarget (+ SetPlayerFaceTarget, fn_14_13B38/11C08/11D64), DetachFromBody (+ the body's fn_14_70E4), SeekAttackPosition (+ SetInitialAttackPosition, fn_14_11698/11788), StunnedReaction (+ SetStunnedPosition, SetAttackPosition, fn_14_118B0), DeploySplitters (+ StartSplitterDeployment, fn_14_F390, fn_14_F424 with the 'PART' token at 0xF04). fn_14_114C8 (needs the line of sight) is a stand-in.
- Area-53 run (knees, OPEN, beacon): Start → FaceTarget → Idle → DetachFromBody (the head moves to its IS05 waypoint) → SeekAttackPosition → StunnedReaction → DeploySplitters (one splitter is ejected and the Splitter module wakes) → FaceTarget → SeekAttackPosition, which then repeats. Guardian and head stay at 150/150; console damage to the head does nothing, because retail damage lands on the head's collision actors (XDMG fn_14_1303C), which are not built yet. The fight does not end.
- fn_14_13318 is ported: "L_ball_LCTR" and "R_ball_LCTR" (radius 1.5, pass-through, ids at 0xDC4) and "Skeleton_Root" (radius 6, immune-ricochet, id at 0xE00). In the area-53 run they are three CCollisionActors, and they come before the knee armours, so those ids shift by 3. Console damage on them changes nothing (0 HP, no XDMG yet).
- The critical path runs through the echo targets. XDMG's armour branch (lives at 0x10E4 drop) needs bit 0x10 (HeadArmorDestroyed), and only fn_14_11FEC sets it, reached from XDMG's echo-target branch (an echo target in state 0x708 == 4). fn_14_12314 needs the 0x790 echo-target class: ctor fn_14_1A698 (0x3AC), dtor 1A5F4, Think 1A1F8 with fn_14_19A08/19B70/19CE0, AcceptScriptMsg 1A430, Render 1A168, and the vtable slots 1ABD8 (HealthInfo), 10C, 1A12C (GetDamageVulnerability), 1AB50 (GetTouchBounds), 1AB98 and 19FD0 (aim position). That is about 1000 asm lines. The class also allocates an echo emitter (AllocateEchoEmitter, already on the host) and a CElementGen from data +0xA0.
- Ported (2026-10-20): the echo-target class, fn_14_12314/1223C, Extract/RetractEchoTargets, ExtractAntenna (fn_14_14E78, the only code that activates the echo targets: they are built inactive), and XDMG fn_14_1303C with F2B8/13248/11F30/11FEC/12B04. In an area-53 run the three targets died (0xFE0 3→0, bit 0x10 set) and ball-collider XDMGs took 0x10E4 2→1→0; the FSM then ran Stage3Start through the head's Dead state (fn_14_1561C, still a stand-in) and the body was removed. Two things still need forcing in a run: after one echo target dies the FSM stalls after the second ExtractAntenna (DropShields/ExtractEchoTargets never recur), and the balls only take damage once fn_14_12818 (stand-in) gives them the 0xDD0 vulnerability, when a morph ball sits in a bomb slot. The head has no IS07/FOLW link in area 53, so fn_14_F2B8 moves nothing there.
- Remaining, in fight order: fn_14_12314 (echo targets, the 0xF30/0xF34 ids, transforms at 0xF40/0xF70/0xFA0) with the class above, then XDMG fn_14_1303C (armour destroyed: lives at 0x10E4 drop, IS08; echo target hit: IS09, bit 0x02) with fn_14_F2B8/13248/12B04/11F30/11FEC; the codes still logged in the run (ExtractAntenna, RaiseShields, GrabPlayerHintControl, DropShields, ExtractEchoTargets, RetractEchoTargets, EndSplitterDeployment, SelectDeathHeadAttack, SeekLineOfSight); a host CVisualKnowledge (owner 0xD80, target 0xDA0, the seen time 0xDB0 and flag 0xDB8 that SetAttackPosition reads; Update is DOL 0x801F55F0) with fn_14_114C8; the attack states (ProjectileTelegraph, Pulse/Charge/Missile/MachineGun attacks), AttachedToBody, Dead; XRDG fn_14_12D98, fn_14_12818/1223C, fn_14_15598 with fn_14_10120, the head paths in fn_14_7D8C and fn_14_2A28, fn_14_5644.

### Head cycle without hooks (area-53 run, 2026-10-21, base d99bc2718)

- The head's FSM2 graph, dumped at load, shows that the DeathHeadAttacks sub-machine leaves only on
  `IsAntennaTransmitting` (fn_14_17244 → body fn_14_70B4) → DropShields → SetStunnedPosition →
  ExtractEchoTargets → StunnedReaction. The trigger fires once the beacon is destroyed (body
  fn_14_266C, which runs only in Crippled), so the beacon must be shot on **every** re-extension.
  A run that shot it once stalls in DeathHeadAttacks. That is a harness limitation, not a port bug.
- A harness that shoots the beacon and then the echo targets every round, with no env hooks, counted
  0xFE0 down 3→2→1→0. The body got CLOS, and the head ran Stage3Start: 0 caught/reach-stub.
- The harness must retry the knees until all four are down: one knee can resist a single round, and
  the body then never reaches Crippled.

### Bomb slots and the head's death (area-53 run, 2026-10-21, base d503a8674)

- fn_14_12818 holds a fully morphed player on an intact ball's bomb slot (within the sphere radius
  1.5) and gives that ball 0xDD0 (`bombPitVulnerability`, data +0x2F8); fn_14_126C8 pulls the bombs
  there. With a harness that teleports the morph ball onto each ball (positions from a temporary
  trace) and lays bombs, real bomb XDMGs took 0x10E4 2→1→0, the FSM entered Dead (fn_14_1561C:
  boss bar cleared, MassiveDeath) and the head was removed: 0 caught/reach-stub. Head HP stays
  150/150 throughout (TakeDamage only sets the hit timer).
- The player must morph after the stage-3 cinematic (`press x` before it does nothing).
- XRDG fn_14_12D98: bombs touching the root collider (vulnerable after SetVulnerableToDamage) fail
  its WeaponHits test, so the gravity repulsor (0x10E0) only drops on beam hits; not yet exercised.

## 2026-10-21: root hits, part 3 codes

- Harness: the boss intro (camera chain 0297..027d, ~35 s) and later cinematics set
  `IsInCinematicCamera()`, which blocks all player input. Wait for the cinematic id to return to
  `03ff` before firing. `press a` needs at least 40 updates, because shorter holds end while the
  gun is still drawing.
- With that done, beams from 12 m reach root collider u0574. XRDG (fn_14_12D98) ran 72 times
  (armour bit set, sender = root) and found the touching CWeapon each time. After
  SetVulnerableToDamage (fn_14_13F0C, once), `WeaponHits` was false for power, dark, light and
  annihilator alike. The damage arm never ran, 0x10E0 stayed full, ShouldRetreat never fired, and
  SetImmuneToDamage / SelectRetreatPlatform / the platform arms stayed unreached. Next suspect:
  the head's own CPatterned vulnerability as the port loads it.
- Ported (unmarked, no run evidence yet): DropShields 15054, RaiseShields 14F74,
  EndSplitterDeployment 14B60, SelectDeathHeadAttack 14470 (block 6 at data +0x2DC; new 0x10DC),
  GrabPlayerHintControl 13F70 (CScriptPlayerHint::SetActorId, TARGET_PC). The ctor now locks
  dropShieldsFx (data +0x48) into 0xEF4.
- SeekLineOfSight 1524C stays a stand-in. SetVisualTarget only writes 0xDA0/0xDB8/0xDAC..0xDB4,
  but CVisualKnowledge::Update is unported, so HasLineOfSight would stay false.

## 2026-10-21: root vulnerability settled

- The table is loaded right. The area-53 DGHD's raw patterned vulnerability (dumped from the loader,
  6172-byte instance) gives power 100/effect 3, dark 50/3, light 200/3, missile 100/3, and the
  charged/combo entries 100/50/200 with effect 3; everything else is 0. LdrToWeaponVulnerability
  (fn_8023B298) passes the effect through unchanged, and the runtime table after
  SetVulnerableToDamage matched (1.0/0.5/2.0, effect 3).
- The bug was the predicate. Retail 12D98 calls the one-argument `WeaponHits` (0x800DB834),
  which treats effect 3 as a hit. That is the host's `WeaponHurts`. The host's two-argument
  `WeaponHits` is retail's other overload (0x800DBA50). With 12D98 on `WeaponHurts`, beams take
  0x10E0 200 → -7.5 (power 2, dark 7.5, light 40 per shot). Then SetImmuneToDamage, SelectRetreatPlatform
  (17 waypoints, both compare arms) and SetStunnedPosition's platform path ran, the 12818
  platform arm held the ball 1931 times, and the head was destroyed.
- Marked: 13EB8, 13C6C, 15054, 14F74, 14B60, 13F70 (their unrun arms are null and empty-data guards
  only). Unmarked: 12D98 (armour-on, no-weapon and zero-damage arms unrun), 12818 (inactive-ball
  arm unrun, out-of-range arm untraced), 14470 (random picks 0 and 1 unseen; 1/2/3 ran).

## 2026-10-21: head arm coverage (runs 35/36)

- Harness inputs: before the echo kills, `damage <root> 1 cannonball` and `damage <root> 1` send
  XRDG with the armour on. After SetVulnerableToDamage, `damage <root> 1 cannonball` sends XRDG
  with the player as the sender, which is the no-weapon arm. Once morphed after the retreat,
  `send <ball0> Deactivate` for 3 s and then `Activate` gives the inactive-ball arm in 12818. The
  root is u0174 and the balls u0172/u0173 in these runs; take them from the XALD probe. The head
  makes no attack picks while idle after the armour drops, so 14470 makes one random pick per fight.
- Shipped-data evidence, from the only DGHD placement (area 53, 0x0035000F), dumped at XALD:
  - IS02/ATCH links only to PlayerHint 0x0C35001D.
  - MDFY/PLAY links to platforms 0x0C350003 and 0x0C350302. Both CONN/ATCH into the same
    16-waypoint loop (z -18.12..-9.21).
  - The attack-area centre is (74.46, 30.38, -16.74).
  - dropShieldsFx is PART 0xCF46E5D8.
- Marked:
  - 15054, 13F70 and 13C6C, with comments carrying the data above.
  - Triggers ShouldRetreat 171B8 (25.7k calls), EchoTargetDestroyed 171EC (1.1k) and
    UseAnnihilatorPulse 1734C, all with both results.
  - Body GetTouchBounds fn_14_10 (32k) and GetHealthInfo fn_14_10C (9k).
- Unmarked:
  - 15284: its marker is removed. Its no-platform and unsafe arms are not excluded by the data.
  - 12D98: zero damage needs a 0-damage CWeapon.
  - 12818: in range with no platform (morphed in a slot before the retreat) never ran.
  - 14470: random 1 and 3 not seen.
  - More-called functions not fully run: head Think (calls the CVisualKnowledge stand-in), IsDead
    and IsIdle (false only), ShouldProjectileAttack (true only), AllSplittersDead (no
    command-module arm), and UseMachineGun / UseLockOnMissiles (one result each).
- Trace recipe for the run scripts (`/tmp/opencode/dghd2/run3[4-6].sh`, `probe35.py`). Re-add
  these as `// DGPT` lines; strip them with `sed -i '/\/\/ DGPT$/d'`.
  - CCameraManager::SetCinematicCameraId, after the id store:
    `fprintf(stderr, "DGP cinecam %04x\n", uid.Value() & 0xffff);` (plus `<stdio.h>`).
    `wait_cine` waits for `03ff`.
  - x11FEC_, inside the "last target" block: `DGHD11FEC fe0=%d` (xFE0_). This breaks the echo loop.
  - SetVulnerableToDamage: `DGVULN setvuln n0`. The VULN wait reads it.
  - x12D98_: `DG12T XRDG bit=%d root=%d` at entry; `DG12T XRDG weapon type=%d ch=%d co=%d hits=%d
    same=%d` before the weapon test; `DG12T XRDG dmg %.1f 10e0=%.1f` after `x1040_ = touched`.
  - SetImmuneToDamage: `DG12T SetImmune`. SelectRetreatPlatform: `DG13T SRP` at entry. The FIRE
    loop stops on it.
  - x12818_, before the range test: `DG12T 12818 out %d ball %.2f %.2f %.2f r`, throttled to every
    30th frame per ball. The DRV loop teleports onto that position. Inside the range test:
    `DG12T 12818 hold %d`.
  - x13830_, after x119E4_: `DGXALD balls %04x %04x root %04x` (xDC4_[0], xDC4_[1], xE00_). The
    run scripts read ROOT and BALL0 from it.
  - The DGNEW lines in 14470 and the other bodies, and the `DGC` arm counters (`probe35.py`), are
    evidence only.

## 2026-10-21: CVisualKnowledge, body aim/pre-render, run 37

- `fn_14_10` and `fn_14_10C` are REL copies of DOL functions, so they are cited, not marked:
  `CPatterned::GetTouchBounds` (0x80073BF0, the same 26 instructions) and
  `CActor::GetHealthInfo` (0x8000B900, 11 instructions).
- CVisualKnowledge (DOL 0x801F55F0..0x801F59D0) is now a shared port-only class:
  `include/MetroidPrime/CVisualKnowledge.hpp`, `src/MetroidPrime/PortCVisualKnowledge.cpp` (listed
  in `tools/gen_files_cmake.py` PORT_ONLY). CommandoPirate's stand-ins can use it too. The head
  keeps one at 0xD80 (ctor 0x183F8: no locator, 0.1 + 0.05 × Float() s). Think updates it, 1524C
  sets the player as its target, and HasLineOfSight / SetAttackPosition read it.
- Body: `x10B8_` is the "Head_1" locator (.rodata 0x6BF, store 0xBAC8). A138 (pre-render bounds and
  echo emitter) and 9E88 (aim position) are ported.
- Run 37: one full fight plus four short ones (stopped 20 s after VULN), `/tmp/opencode/dghd2/run37.sh
  <tag> <port> <display> full|short` and `probe37.py` (probe35 plus a head FSM state trace `DGST`,
  CVisualKnowledge / A138 / 9E88 counters and a dump in the head's destructor). Save clean copies
  of the probed files first and restore them afterwards: the sed strip breaks one-line bodies.
  - CVisualKnowledge works: VK.visible 12.2k, VK.hidden 2.0k, HasLOS 1 ×4 and 0 ×2.3k. With
    runs 35/36 (no target), 171F8 ran every arm and is marked. 1524C and Think 17C98 are marked
    too. Think's retail-absent `xD70_` null test is gone: AddObject delivers XCRT (and so
    fn_14_13318) before any Think.
  - 12818: morphed in ball slot 0 before the retreat, 479 holds with no 0x1038 platform. That
    leaves only the inactive ball, which needs the 0xF28 sound to return no handle.
  - 15284: armour-on calls only. The bombs laid in the slot destroyed nothing.
  - 14470: every fight's first picks were first-shot (1) and splitters (3), so no random pick
    happened. A random pick needs a longer fight past the splitters.
  - IsIdle: the FSM entered Idle (0x10F4 0x88) but no trigger read IsIdle there. IsDead: the full
    fight crashed before the head's death.
  - A138 ran the no-emitter arm only, and 9E88 the active collision actor arm only. Both are
    unmarked.
  - The full fight crashed at the morph-out (`press x` after the bombs). The crash was in
    `CPlayer::Render` → `CModelData::Render` → `CTexture::Load`, which is player model texture
    code, not DG code.

## 2026-10-21: unmorph crash fixed, runs 38/39

- The run-37 crash (`CPlayer::Render` → `CTexture::Load`, at the morph-out in the fight) was a null
  CTexture in the player's ball-transition beam model (CPlayer.cpp ~3303). It came from
  `CModel::UnlockTextures` (DolphinCModel.cpp): when `CModelData::LockTextures` releases a model's
  initial texture locks after the model has been drawn, the current material set's tokens are
  released through the shader. That nulls the cached textures but leaves the instance's
  `mLoadTextures` set, so `TryLockTextures` skips relocking. The host fix, under TARGET_PC,
  releases the current set through `CCubeModel::UnlockTextures`, as `FrameDone` does.
  - Before the fix, runs 380/381 (morph and bomb in slot 0 before the retreat, then unmorph)
    logged a null texture (tag b6b1c040, index 3/5, unlocked) and then the SIGSEGV.
  - After the fix, runs 390-392 hit the stale case once each, with no null texture and no crash.
  - This is not the stale copy-texture mapping that `GXDestroyCopyTex` in DolphinCTexture.cpp
    addresses: there the object is valid but the wrong image is drawn.
- `/tmp/opencode/dghd2/run38.sh <tag> <port> <display> hold|holdtp|safe` is run37 with modes:
  - `hold` and `holdtp` morph, bomb slot 0 before the retreat until a slot dies, then unmorph in
    place (`holdtp` teleports away first). `safe` skips that step.
  - Every mode then runs the FIRE and DRV loops to the head's death.
- Results from seven fights to the head's death:
  - IsDead returned true (head deleted), so 17184 is marked.
  - IsIdle was never read with bit 0x80 set.
  - 14470 made no random pick in any of them, only first-shot (1) and splitters (3); only run
    36's fight made one.
  - 15284 ran only its armour-on and platform arms. The hold runs destroyed slot 0 before the
    retreat, but no SetStunnedPosition followed with the armour off.
  - 9E88 ran the active collision-actor arm and the locator arm (actor inactive, 0x11B8 clear),
    but not the 0x11B8 arm.
  - A138 ran the no-emitter arm only.

## 2026-10-21: forced arms, runs 400-404

- Drivers: `tools/drivers/dg_area53.sh <tag> <port> <display> <steps>` with the step files
  `dg_steps_beacon_rounds.sh`, `dg_steps_armour_off_stun.sh` and `dg_steps_close.sh`. The `fsm`
  step needs port's console. Pick counts come from a local probe that was not committed.
- The body constructor now allocates the beacon's echo emitter (fn_14_B100 at 0xC01C..0xC074), so
  pre-render's emitter arms can run.

| Function | Arm | Run | Result |
|---|---|---|---|
| A138 | no emitter | 37-39 | ran |
| A138 | beacon model with height | 402 | ran |
| A138 | emitter, no beacon height | 402, 404 | ran |
| A138 | bit 0x08 (inactive) | 404, forced (`send <body> CLOS`) | unreachable: CLOS deletes the body before its next pre-render |
| 14470 | random 1 and 3 | 402/403 (11 picks) | not run: the splitter main chassis is a pass-through stand-in, so the splitters never all die |
| 15284 | armour off, no platform | 403, forced (`fsm <head> StunnedReaction`) | not run: StunnedReaction is a sub-machine state, which `fsm` cannot enter |
| 173AC (IsIdle) | bit 0x80 set | FSM2 dump | not run: its triggers sit in sub-machines (`fsm <head> Idle` fails the same way) |
| 9E88 | 0x11B8 set | - | unreachable until the vortex states (fn_14_9260/9070/8E44/76D0) are ported |

- Marked: fn_14_A138. Still unmarked: 14470, 15284, 173AC, 9E88.
- Console asks:
  - `fsm <id> <Machine>/<State>`, to enter FSM2 sub-machine states. This would unblock IsIdle
    and the 15284 armour-off arm.
  - `delete <id>` (`CStateManager::FreeScriptObject`), to remove the undamageable splitter
    chassis. This would unblock 14470's random arms.

## 2026-10-21: vortex states, runs 410-413

- Ported: fn_14_9260 VortexAttack, fn_14_9164 VortexEndAttack, fn_14_9070 VortexRetreat,
  fn_14_8E44 ExitVortexAttack, fn_14_76D0 SetNormalAttackMode, fn_14_775C SetVortexAttackMode and
  the helpers 34E8, 3674, 39CC, 3DA8, 4838, 4878, 4D54, plus the knee's fn_14_CE1C SetOrbitable.
- Constructor fix: 0x128C starts at 0x80 (the rlwimi at 0xBA30 with r6 = 1); the port had 0.
- Step files: `dg_steps_vortex.sh` (`fsm <body> VortexAttack`, then the sub-machine runs on its
  own) and `dg_steps_head_stun.sh`. `dg_steps_beacon_rounds.sh` still needs the uncommitted
  14470 probe for its counts (see its header).

| Function | Arm | Run | Result |
|---|---|---|---|
| 9260 | act, no drift, over, deact | 410, 412, forced (`fsm <body> VortexAttack`) | ran |
| 9260 | drift (0x128D bit 0x40) | - | not run: only fn_14_A468 (DoUserAnimEvent, stand-in) sets it |
| 3674 | no 0x40 | 410, 412 | ran; the pull arm needs 0x128D 0x40 (A468) |
| 9164, 9070, 8E44 | every arm | 412, forced as above | ran |
| 34E8 | no target / target: dist, near, step, accel, clamp | 410, 412 | ran; `unsafe` (dist > 0, direction not magnitude-safe) not run |
| 76D0 | head, no head | 410-413 | ran |
| 775C, 3DA8 off, CE1C off, 39CC null | - | - | not run: 775C runs on the root transition into the vortex, which `fsm` skips |
| 4D54, 4878 | shaker | 410, 412 | ran; the no-shaker arm not run |
| 173AC | bit 0x80 set | 410-413 | ran (no branch) |
| 9E88 | 0x11B8 set | 410-412 | not run: no aim-position query during the vortex |

- Marked: 9070, 9164, 8E44, 76D0, 4838, 173AC. Unmarked: 9260, 3674 (need A468 + 3300, 4E00,
  458C, 73D4: a follow-up slice), 34E8, 39CC, 3DA8, 4D54, 4878, 775C, CE1C, 9E88, 15284.

## 2026-10-21: DoUserAnimEvent, runs 420-423

- Ported: fn_14_A468 DoUserAnimEvent and its handlers fn_14_3300 (vortex drift), fn_14_4E00
  (locator camera shake), fn_14_458C (toe shock wave) with fn_14_20E4 (leg from locator), and
  fn_14_73D4 SelectMeleeAttackRange (the A468 type-3 handler). New members 0x11D8/0x11DC (-1).
- `dg_steps_vortex.sh` with `SHOOT=1` runs `shoot <body>` on each poll (`obj` shows only the root
  state, so the polls cannot tell the vortex sub-state).

| Function | Arm | Run | Result |
|---|---|---|---|
| A468 | 5, 12, 13 (vortex), 16, 17 | 420, 423, forced (`fsm <body> VortexAttack`) | ran |
| A468 | 3, 27, 13 outside the vortex, other types (CPatterned) | - | not run: no such event in the vortex animations; 3/27 come from the melee/jump states (stand-ins) |
| 9260 | drift (0x128D 0x80) | 420, 423 | ran: every arm has run, marked |
| 3674 | no 0x40, pull | 420, 423 | ran |
| 3674 | 0x128C 0x04 | - | not run: only fn_14_4F2C (XHIT helper, stand-in) and the ctor set it |
| 3674 | player beyond max distance | - | not run |
| 3300 | object, time > 0 | 420, 423 | ran; no-object and take-off/landing-span arms not run |
| 4E00 | Attach, Action, shaker | 423 | ran; no-shaker arm not run |
| 9E88 | 0x11B8 set | 423, forced (`shoot <body>`) | ran: every arm has run, marked |
| 458C, 20E4, 73D4 | - | - | not run (melee/jump states are stand-ins; 0x11D8 stays -1) |
| 15284 | no platform | 421 | not run: aborted at the 32 MiB staging window too (33555040 bytes, frame 35700) |
| 775C, 3DA8 off, CE1C off | - | - | not run: no natural vortex entry in these runs |

- Marked: 9260, 9E88.
