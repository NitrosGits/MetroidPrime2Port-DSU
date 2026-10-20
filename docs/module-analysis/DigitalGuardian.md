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
| 2 | SelectProjectileAttack | `fn_14_15448` | 17 | EnableValidTarget | `fn_14_145A4` |
| 3 | SetPlatformPosition | `fn_14_153D0` | 18 | DisableValidTarget | `fn_14_14470` |
| 4 | SetStunnedPosition | `fn_14_15284` | 19 | SelectDeathHeadAttack | `fn_14_143D8` |
| 5 | SeekLineOfSight | `fn_14_1524C` | 20 | SetInitialAttackPosition | `fn_14_14254` |
| 6 | DropShields | `fn_14_15054` | 21 | SetAttackPosition | `fn_14_14248` |
| 7 | RaiseShields | `fn_14_14F74` | 22 | ResetGravityRepulsorHP | `fn_14_14058` |
| 8 | ExtractAntenna | `fn_14_14E78` | 23 | EjectPlayerFromBombPit | `fn_14_13F70` |
| 9 | ExtractEchoTargets | `fn_14_14CF4` | 24 | GrabPlayerHintControl | `fn_14_13F0C` |
| 10 | RetractEchoTargets | `fn_14_14DA0` | 25 | SetVulnerableToDamage | `fn_14_13EB8` |
| 11 | DestroyActiveSplitters | `fn_14_14C24` | 26 | SetImmuneToDamage | `fn_14_13C6C` |
| 12 | StartSplitterDeployment | `fn_14_14B98` | 27 | SelectRetreatPlatform | `fn_14_13C1C` |
| 13 | EndSplitterDeployment | `fn_14_14B60` | 28 | StartVocalRecoverReaction | `fn_14_13B38` |
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
| 0x1088 / 0x11EC | `CTransform4f` (identity / the echo-emitter bounds) | ctor stores `sIdentity`; `GetBounds(model, &this+0x11EC)` in the ctor |
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
| 0x13B38-0x17468 | **head codes**: `fn_14_13B38` StartVocalRecoverReaction … `fn_14_1560C` SetPlayerFaceTarget (the table order is reversed in address order) |
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

### WP3 — the head module

`CDigitalGuardianHead : CPatterned` (guest size 0x10F8, `EPatternedAI 0xA`), its data struct
(c.3), its 24 local overrides and its 20/17/29 FSM tables, plus `PortLoadDigitalGuardianHead`
(the loader `fn_14_DF7C`, property block 0x0F4D73B7).

### WP4 — the sub-actors

| piece | guest class | size | where |
|---|---|---|---|
| knee armour | `CActor` subclass, vtable `lbl_14_data_694` | 0x1A0 | `fn_14_4080` |
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
