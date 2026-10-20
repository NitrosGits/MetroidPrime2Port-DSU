# IngBoostBallGuardian (REL module 30) — data-block field mapping

Static extraction from `build/G2ME01/IngBoostBallGuardian/asm/auto_00_00000000_text.s`.
All "guest" offsets are offsets inside the object built by `fn_30_EC6C`.

## 0. Structure of the two copies

`fn_30_EC6C` (actor ctor, [0x0000EC6C]) receives two loader temps as stack args:

* stack arg0 = the **guardian** runtime temp (built by `fn_30_10C24` at [0x00010C24] from the
  `SLdrIngBoostBallGuardianData` at `r1+0x0ED8`, loaded by `fn_30_2344` at [0x00002344]).
* stack arg1 = the **boost-ball** runtime temp (built by `fn_30_748` at [0x00000748] from the
  `SLdrBoostBallGuardianData` at `r1+0x1174`, loaded by `fn_30_1988` at [0x00001988]).

| copy | helper | call site | extent |
|---|---|---|---|
| guardian → `obj+0x07C0` | `fn_30_1056C` | [0x0000ECEC] | 0x320 bytes (obj+0x07C0..0x0AE0) |
| boost-ball → `obj+0x0DEC` | `fn_30_10250` | [0x0000ED98] | inline part 0x110 bytes (obj+0x0DEC..0x0EFC) |

`fn_30_1056C` copies `obj+0x07C0+0x000..0x29C` verbatim (via `fn_30_10694`, [0x00010588]) and then
re-writes `+0x2CC..+0x320` field by field. `fn_30_10250` copies `+0x000..+0x04C` and `+0x090..+0x10F`
inline, `+0x04C..+0x090` through `fn_30_CB4` ([0x0001032C]); the remaining `+0x110..` of the
boost-ball block (`boostBallVulnerability` + phase vector) is copied by `fn_30_AD4`
([0x00010470]) and `fn_30_9E0` ([0x0001047C]) and is **outside** the 0x110-byte block.

`SLdrIngBoostBallGuardianData` is 0x29C bytes and `SLdrBoostBallGuardianData` is 0x2E8 bytes:
the two loader temps are adjacent (`r1+0x0ED8` and `r1+0x1174`, i.e. 0x29C apart).

## 1. The 0x320-byte block at `obj+0x07C0`

`obj+0x07C0+n`. The first 0x29C bytes are a **verbatim copy of the SLdr struct**
(`fn_30_10694` [0x00010694] copies 0x00..0xEF word by word, `fn_30_10B90` [0x00010B90] copies
0xF0..0x138, `fn_30_108D4` [0x000108D4] copies 0x140..0x29C), so the source column is the SLdr
field at the *same* offset. The last 0x84 bytes are the runtime-converted values.

| guest off | width | source | asm |
|---|---|---|---|
| 0x07C0 +0x000 | 4 | `unknown_0x01789abd` (int) | [0x000027E0] |
| 0x07C0 +0x004 | 4 | `hearingRadius` | [0x000027F8] |
| 0x07C0 +0x008 | 4 | `ingSpotMaxSpeed` | [0x00002808] |
| 0x07C0 +0x00C | 4 | `ingSpotMaxWallSpeed` | [0x00002818] |
| 0x07C0 +0x010 | 4 | `ingSpotBallPursuitSpeed` | [0x00002828] |
| 0x07C0 +0x014 | 4 | `unknown_0x50398a06` | [0x00002838] |
| 0x07C0 +0x018 | 4 | `ingSpotTurnSpeed` | [0x00002848] |
| 0x07C0 +0x01C | 4 | `ingSpotBlobEffect` (CAssetId) | [0x00002858] |
| 0x07C0 +0x020 | 4 | `ingSpotHitNormalDamage` | [0x00002870] |
| 0x07C0 +0x024 | 4 | `ingSpotHitHeavyDamage` | [0x00002888] |
| 0x07C0 +0x028 | 4 | `ingSpotDeath` | [0x000028A0] |
| 0x07C0 +0x02C | 4 | `sound_IngSpotIdle` | [0x000028B8] |
| 0x07C0 +0x030 | 4 | `sound_IngSpotMove` | [0x000028D0] |
| 0x07C0 +0x034 | 4 | `sound_HitNormalDamage` | [0x000028E8] |
| 0x07C0 +0x038 | 4 | `sound_HitHeavyDamage` | [0x00002900] |
| 0x07C0 +0x03C | 4 | `sound_IngSpotDeath` | [0x00002918] |
| 0x07C0 +0x040 | 4 | `pART` | [0x00002930] |
| 0x07C0 +0x044 | 4 | `sRSC` | [0x00002948] |
| 0x07C0 +0x048 | 4 | `pART_0x3da219c7` | [0x00002960] |
| 0x07C0 +0x04C | 4 | `unknown_0x23271976` | [0x00002978] |
| 0x07C0 +0x050 | 4 | `pART_0x081e9e6c` | [0x00002988] |
| 0x07C0 +0x054 | 4 | `unknown_0xcb39eccb` | [0x000029A0] |
| 0x07C0 +0x058 | 4 | `unknown_0x587ca175` | [0x000029B0] |
| 0x07C0 +0x05C | 4 | `unknown_0x0bd7d5a9` | [0x000029C0] |
| 0x07C0 +0x060 | 4 | `sound_SwarmMove` | [0x000029D0] |
| 0x07C0 +0x064 | 4 | `sound_InsideHost` | [0x000029E8] |
| 0x07C0 +0x068 | 4 | `sound_ExitHost` | [0x00002A00] |
| 0x07C0 +0x06C | 4 | `sound_ExitHostSafeZone` | [0x00002A18] |
| 0x07C0 +0x070 | 4 | `sound` | [0x00002A30] |
| 0x07C0 +0x074 | 4 | `unknown_0x5d0d2c40` | [0x00002A48] |
| 0x07C0 +0x078 | 4 | `unknown_0xc620183a` | [0x00002A58] |
| 0x07C0 +0x07C | 4 | `frustrationTime` | [0x00002A68] |
| 0x07C0 +0x080 | 4 | `tauntChance` | [0x00002A78] |
| 0x07C0 +0x084 | 4 | `aggressiveness` | [0x00002A88] |
| 0x07C0 +0x088 | 4 | `armSwipeDamage.dI_WeaponType` | [0x00002A98] |
| 0x07C0 +0x08C | 4 | `armSwipeDamage.dI_Damage` | [0x00002A98] |
| 0x07C0 +0x090 | 4 | `armSwipeDamage.dI_Radius` | [0x00002A98] |
| 0x07C0 +0x094 | 4 | `armSwipeDamage.dI_KnockBackPower` | [0x00002A98] |
| 0x07C0 +0x098 | 4 | `bodyProjectileContactDamage.dI_WeaponType` | [0x00002AA8] |
| 0x07C0 +0x09C | 4 | `bodyProjectileContactDamage.dI_Damage` | [0x00002AA8] |
| 0x07C0 +0x0A0 | 4 | `bodyProjectileContactDamage.dI_Radius` | [0x00002AA8] |
| 0x07C0 +0x0A4 | 4 | `bodyProjectileContactDamage.dI_KnockBackPower` | [0x00002AA8] |
| 0x07C0 +0x0A8 | 4 | `unknown_0xa0d63374` | [0x00002AB8] |
| 0x07C0 +0x0AC | 4 | `bodyProjectileSuckTime` | [0x00002AC8] |
| 0x07C0 +0x0B0 | 4 | `bodyProjectileSplatEffect` | [0x00002AD8] |
| 0x07C0 +0x0B4 | 4 | `bodyProjectileSpeed` | [0x00002AF0] |
| 0x07C0 +0x0B8 | 4 | `bodyProjectileDropTime` | [0x00002B00] |
| 0x07C0 +0x0BC | 4 | `bodyProjectileMinAttackDist` | [0x00002B10] |
| 0x07C0 +0x0C0 | 4 | `bodyProjectileMaxAttackDist` | [0x00002B20] |
| 0x07C0 +0x0C4 | 4 | `sound_BodyProjectile` | [0x00002B30] |
| 0x07C0 +0x0C8 | 4 | `sound_BodyProjectileSplatWall` | [0x00002B48] |
| 0x07C0 +0x0CC | 4 | `bodyProjectileOdds` | [0x00002B60] |
| 0x07C0 +0x0D0 | 4 | `miniPortalMinAttackDist` | [0x00002B70] |
| 0x07C0 +0x0D4 | 4 | `miniPortalMaxAttackDist` | [0x00002B80] |
| 0x07C0 +0x0D8 | 4 | `miniPortalEffect` | [0x00002B90] |
| 0x07C0 +0x0DC | 4 | `sound_MiniPortal` | [0x00002BA8] |
| 0x07C0 +0x0E0 | 4 | `miniPortalProjectileDamage.dI_WeaponType` | [0x00002BC0] |
| 0x07C0 +0x0E4 | 4 | `miniPortalProjectileDamage.dI_Damage` | [0x00002BC0] |
| 0x07C0 +0x0E8 | 4 | `miniPortalProjectileDamage.dI_Radius` | [0x00002BC0] |
| 0x07C0 +0x0EC | 4 | `miniPortalProjectileDamage.dI_KnockBackPower` | [0x00002BC0] |
| 0x07C0 +0x0F0 | 0x48 | `miniPortalBeamInfo` (whole `SLdrPlasmaBeamInfo`, verbatim) | [0x00002BD0] |
| 0x07C0 +0x138 | 4 | `lightColor` (CColor, 4 bytes in retail) | [0x00002BF0] |
| 0x07C0 +0x13C | 4 | `lightAttenuation` | [0x00002C00] |
| 0x07C0 +0x140 | 0x15C | `ingSpotVulnerability` (whole `SLdrDamageVulnerability`, verbatim, 29 x 12-byte `SLdrWeaponVulnerability`) | [0x00002C08] |
| 0x07C0 +0x29C | 0x30 | runtime `CDamageVulnerability` converted from `ingSpotVulnerability` (`LdrToDamageVulnerability` [0x00010C48] -> `fn_30_AD4` [0x00010C58]); counts at +0x24/+0x28, element-array pointer at +0x2C (`fn_30_B84` [0x00000B84]) | [0x00010C58] |
| 0x07C0 +0x2CC | 0x1B | runtime `CDamageInfo` from `miniPortalProjectileDamage` (`LdrToDamageInfo` [0x00010C70]) | [0x00010C74] |
| 0x07C0 +0x2E8 | 0x1B | runtime `CDamageInfo` from `armSwipeDamage` (`LdrToDamageInfo` [0x00010CC4]) | [0x00010CC8] |
| 0x07C0 +0x304 | 0x1B | runtime `CDamageInfo` from `bodyProjectileContactDamage` (`LdrToDamageInfo` [0x00010D18]) | [0x00010D1C] |
| 0x07C0 +0x320 | 1 | flag byte built from `unknown_0x01789abd`: b25=bit6, b26=bit4, b27=(v&3)!=0, b28=bit1, b29=(v&5)!=0, b24/b30=0 | [0x00010D98] |

`miniPortalBeamInfo` sub-layout (from the `SLdrPlasmaBeamInfo` header order, 0x48 bytes):
+0x00 `unknown_0xff713aad`, +0x04 `weaponSystem`, +0x08 `contactEffect`, +0x0C `pulseEffect`,
+0x10 `beamTexture`, +0x14 `glowTexture`, +0x18 `length`, +0x1C `radius`, +0x20 `expansionSpeed`,
+0x24 `lifeTime`, +0x28 `pulseSpeed`, +0x2C `shutdownTime`, +0x30 `contactEffectScale`,
+0x34 `pulseEffectScale`, +0x38 `travelSpeed`, +0x3C `innerColor`, +0x40 `outerColor`,
+0x44 `beamStreaks`. (Loader is external: `LoadTypedefPlasmaBeamInfo` [0x00002BD8].)

`ingSpotVulnerability` sub-layout: 29 x 12 bytes (`float damageMultiplier; int effect;
bool ignoreRadius;`) at +0x140+12k, in `SLdrDamageVulnerability` declaration order:
k0 `power`, k1 `dark`, k2 `light`, k3 `annihilator`, k4 `boostBall`, k5 `cannonBall`,
k6 `screwAttack`, k7 `bomb`, k8 `powerBomb`, k9 `missile`, k10 `phazon`, k11 `ai`,
k12 `poisonWater`, k13 `darkWater`, k14 `lava`, k15 `areaDamageHot`, k16 `areaDamageCold`,
k17 `areaDamageDark`, k18 `areaDamageLight`, k19 `powerCharge`, k20 `entangler`,
k21 `lightBlast`, k22 `sonicBoom`, k23 `superMissle`, k24 `blackHole`, k25 `sunburst`,
k26 `imploder`, k27 `weaponVulnerability`, k28 `normalSafeZone`.

## 2. The 0x110-byte block at `obj+0x0DEC`

Source = boost-ball runtime temp built by `fn_30_748` [0x00000748]; `obj+0x0DEC+n` = temp+n.

| guest off | width | source | asm |
|---|---|---|---|
| 0x0DEC +0x000 | 4 | `boostBallScale.x` (SLdr+0x00) | [0x00000798] |
| 0x0DEC +0x004 | 4 | `boostBallScale.y` (SLdr+0x04) | [0x000007A0] |
| 0x0DEC +0x008 | 4 | `boostBallScale.z` (SLdr+0x08) | [0x000007B0] |
| 0x0DEC +0x00C | 4 | `boostBallMass` (SLdr+0x0C) | [0x000007C0] |
| 0x0DEC +0x010 | 4 | `unknown_0xbea96fb7` (SLdr+0x10) | [0x000007CC] |
| 0x0DEC +0x014 | 0x1B | runtime `CDamageInfo` from `boostBallDamageToMorphBall` (SLdr+0x58): +0 weaponType, +4 damage, +8 radius, +0xC knockBackPower, +0x10 ?, +0x14/+0x16/+0x18 u16, +0x1A u8 | [0x000007D4] |
| 0x0DEC +0x030 | 0x1B | runtime `CDamageInfo` from `boostBallDamageToFirstPerson` (SLdr+0x68) | [0x00000820] |
| 0x0DEC +0x04C | 0x44 | `boostBallSpeed` spline (SLdr+0x14, `CMayaSpline`) via `fn_30_CB4` | [0x0000089C] |
| 0x0DEC +0x090 | 4 | `boostBallMinDamageSpeed` (SLdr+0x78) | [0x000008A0] |
| 0x0DEC +0x094 | 4 | ? (never written by `fn_30_748`) | ? |
| 0x0DEC +0x098 | 4 | `boostIngSpotBlobEffect` (SLdr+0x234) | [0x000008B0] |
| 0x0DEC +0x09C | 4 | `boostIngSpotHitNormalDamage` (SLdr+0x238) | [0x000008B4] |
| 0x0DEC +0x0A0 | 4 | `boostIngSpotHitHeavyDamage` (SLdr+0x23C) | [0x000008BC] |
| 0x0DEC +0x0A4 | 4 | `boostBallModel` (SLdr+0x80) | [0x000008C0] |
| 0x0DEC +0x0A8 | 4 | `boostBallTrailEffect` (SLdr+0x8C) | [0x000008C4] |
| 0x0DEC +0x0AC | 4 | `boostBallShieldEffect` (SLdr+0x88) | [0x000008C8] |
| 0x0DEC +0x0B0 | 4 | `boostBallChargeUpEffect` (SLdr+0x84) | [0x000008CC] |
| 0x0DEC +0x0B4 | 2 | `sound_Bounce` (SLdr+0x90) | [0x000008D0] |
| 0x0DEC +0x0B6 | 2 | `sound_IntoBall` (SLdr+0x94) | [0x000008D4] |
| 0x0DEC +0x0B8 | 2 | `sound_OutofBall` (SLdr+0x98) | [0x000008D8] |
| 0x0DEC +0x0BA | 2 | `sound` (SLdr+0x9C) | [0x000008DC] |
| 0x0DEC +0x0BC | 2 | `sound_Boost` (SLdr+0xA0) | [0x000008E0] |
| 0x0DEC +0x0BE | 2 | `sound_Rolling` (SLdr+0xA4) | [0x000008E4] |
| 0x0DEC +0x0C0 | 4 | rodata constant 1000.0f (`lbl_30_rodata_68`) | [0x000008E8] |
| 0x0DEC +0x0C4 | 4 | rodata constant 1.0f (`lbl_30_rodata_64`) | [0x000008EC] |
| 0x0DEC +0x0C8 | 4 | `unknown_0xee69b993` (SLdr+0x204) | [0x000008F0] |
| 0x0DEC +0x0CC | 4 | `unknown_0x4b2de673` (SLdr+0x208) | [0x000008F4] |
| 0x0DEC +0x0D0 | 4 | `searchConeAngle` (SLdr+0x20C) | [0x000008F8] |
| 0x0DEC +0x0D4 | 4 | `unknown_0xb0e85d53` (SLdr+0x210) | [0x000008FC] |
| 0x0DEC +0x0D8 | 0x1B | runtime `CDamageInfo` from `ingSpotDamageToMorphBall` (SLdr+0x214) | [0x00000900] |
| 0x0DEC +0x0F4 | 0x1B | runtime `CDamageInfo` from `ingSpotDamageToFirstPerson` (SLdr+0x224) | [0x00000948] |

## 3. The three phase structs — CONFIRMED

`fn_30_130` builds them at [0x000002F0]..[0x00000350]. Source structs are the three
`SLdrIngBoostBallGuardianStruct` members of `SLdrBoostBallGuardianData` at SLdr+0x240
(`ingBoostBallGuardianStruct`), +0x278 (`..._0xfe18a18f`), +0x2B0 (`..._0xc2784287`), loaded by
`fn_30_20A0` at [0x000020A0].

| # | `fn_30_18F0` dest / src | `fn_30_F78` dest | vector slot (`fn_30_F30`) | final guest offset |
|---|---|---|---|---|
| 0 | r1+0x200 / SLdr+0x240 [0x000002F0] | r1+0x238 [0x000002FC] | r1+0x374 [0x00000308] | obj+0x0F30 (vec count at obj+0x0F2C) |
| 1 | r1+0x190 / SLdr+0x278 [0x00000314] | r1+0x1C8 [0x00000320] | r1+0x3AC [0x0000032C] | obj+0x0F68 |
| 2 | r1+0x120 / SLdr+0x2B0 [0x00000338] | r1+0x158 [0x00000344] | r1+0x3E4 [0x00000350] | obj+0x0FA0 |

`fn_30_F78` [0x00000F78] is a plain 0x38-byte copy; `fn_30_18F0` [0x000018F0] copies the same
14 words through registers into `fn_30_194C` [0x0000194C]. Field map (identical for all three,
`fn_30_20A0` store addresses in brackets):

| off in struct | SLdr field | asm |
|---|---|---|
| +0x00 | `boostPuddleModeTime` | [0x000021B0] |
| +0x04 | `unknown_0xabe99de0` | [0x0000214C] |
| +0x08 | `breakBoostPuddleModeDamage` | [0x00002160] |
| +0x0C | `breakBoostModeDamage` | [0x0000219C] |
| +0x10 | `nextBoostTimeMin` | [0x00002200] |
| +0x14 | `nextBoostTimeMax` | [0x00002210] |
| +0x18 | `unknown_0x285d67ad` | [0x000021D8] |
| +0x1C | `unknown_0x6d5f242f` | [0x000021EC] |
| +0x20 | `unknown_0xe3ff2ed6` | [0x00002188] |
| +0x24 | `unknown_0xa6fd6d54` | [0x00002138] |
| +0x28 | `unknown_0xecb314b6` | [0x000020FC] |
| +0x2C | `unknown_0xa9b15734` | [0x00002124] |
| +0x30 | `locomotionSpeedScale` | [0x000021C4] |
| +0x34 | `ingSpotSpeedScale` | [0x00002174] |

The vector (`r1+0x370`: `u32 count` + 3 x 0x38) is copied into the boost-ball temp at +0x140
by `fn_30_9E0` [0x0000099C] and from there into the object by `fn_30_9E0` [0x0001047C].

## 4. Not mapped / uncertain

1. `obj+0x0DEC+0x094` — 4 bytes never written by `fn_30_748` (hole/padding). Lost at [0x000008A4].
2. Runtime `CDamageInfo` tail: the float at +0x10 and the u16/u16/u16/u8 at +0x14..+0x1A are
   produced by the external `LdrToDamageInfo` (not in this module) — their SLdr origin is unknown.
   Lost at [0x000007E0] / [0x00000830] / [0x00000908] / [0x00000950].
3. Runtime `CDamageVulnerability` header at `obj+0x07C0+0x29C..+0x2CB`: `LdrToDamageVulnerability`
   is an external import, so the 7 leading u16 words are unmapped; only the counts (+0x24,
   +0x28) and the element-array pointer (+0x2C) are visible via `fn_30_B84` [0x00000B84].
4. `SLdrPlasmaBeamInfo` and `SLdrDamageVulnerability` member offsets are taken from the generated
   header order, not from asm in this module (their loaders are external imports).
5. `SLdrWeaponVulnerability` order inside `ingSpotVulnerability` relies on the generated header's
   declaration order; the loader `LoadTypedefDamageVulnerability` is external.
6. The 6 unknown_* floats at SLdr guardian +0x54/+0x58/+0x5C and +0x74/+0x78 have no name in the
   SLdr header — kept as `unknown_*` rather than guessed.
