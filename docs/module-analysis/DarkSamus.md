# DarkSamus REL (module 10): analysis for a host body

Analysis + host body, milestone 1 (2026-10-20). Sources: `config/G2ME01/rels/DarkSamus/symbols.txt`
(sizes), the archive build's disassembly
`../MetroidPrime2Port/build/G2ME01/DarkSamus/asm/` (`auto_00_*_text.s`, `auto_03_*_rodata.s`,
`auto_04_*_data.s`, `auto_05_*_bss.s`), the m2c pseudo-C under `/tmp/opencode/pseudo/DarkSamus/`,
and the generated `include/MetroidPrime/ScriptLoader/SLdrDarkSamus.{hpp,inc}`. Confidence tags as in
`ElitePirate.md`: **[proven]** = read from an instruction/table entry; **[probable]** = strong
evidence but not proven; **[?]** = guess. All `fn_10_X` are REL `.text` offsets. The
implementation is `src/MetroidPrime/Enemies/PortCDarkSamus.cpp`,
`include/MetroidPrime/Enemies/CDarkSamus.hpp`.

## 1. Function map

| fn | size | role | evidence |
|---|---|---|---|
| `RELMain` | 0x20 | calls `fn_10_CF14` | [proven] |
| `fn_10_CF14` | 0x30 | stores the loader `fn_10_CF44` into `lbl_10_bss_110` (the module's `SDarkSamus_FuncPtrs`) and calls the DOL's `fn_802188E4` (= `SetSDarkSamus_FuncPtrs`) | [proven] |
| `RELExit` | 0x24 | `fn_802188E4(nullptr)` | [proven] |
| `fn_10_CF44` | 0xE98 | **loader** `LoadDarkSamus` (section 3) | [proven] |
| `fn_10_1FA14` | 0xB98 | **CDarkSamus ctor** (section 2) | [proven]: called after `rs_new 0x1128` |
| `fn_10_1EF38` | 0x300 | **destructor** (vtable slot 2; the sub-objects and the base) | [proven] vtable |
| `fn_10_1B1FC` | 0x27C | **`Think`** (slot 5) | [proven] vtable + body |
| `fn_10_18D78` | 0x58 | **the module's state-machine setup**: calls `InitializeStateMachine__10CPatterned` and then hands the machine two tables (section 7) | [proven] the two table calls |
| `fn_10_221C0` | 0xD20 | the **`.ctors` initialiser**: copies the 92 12-byte records into the 92 table entries | [proven] the copies (section 7) |
| `fn_10_1E89C` | 0x5B0 | **`AcceptScriptMsg`** (slot 6; the four-character codes `XCRT`, `ACTV`, `ALRT`, `DCTV`, `XALD`, `XDEL`, `XDMG`, `XLAU`, `XLND`, `XLSG`, `XOFF` - the `XCRT` arm also activates the body controller, 0x0001E9A4) | [proven] vtable + body |
| `fn_10_18AB8` | 0x2C0 | **`PreRender`** (slot 9) | [proven] vtable |
| `fn_10_17F04` | 0x2D8 | **`AddToRenderer`** (slot 10) | [proven] vtable |
| `fn_10_18964` | 0x154 | **`Render`** (slot 11) | [proven] vtable |
| `fn_10_0` | 0x2C | **`GetHealthInfo`** (slot 15): calls the vtable's `HealthInfo` slot (0x38) and returns its result | [proven] |
| `fn_10_100DC` | 0x134 | **`GetDamageVulnerability`** (slot 16) | [proven] vtable |
| `fn_10_F5C0` | 0x2C | **`GetTouchBounds`** (slot 18): `CPhysicsActor::GetBoundingBox` | [proven] |
| `fn_10_10358` | 0x12C | **`GetOrbitPosition`** (slot 20) | [proven] vtable |
| `fn_10_10210` | 0x148 | **`GetAimPosition`** (slot 21) | [proven] vtable |
| `fn_10_F84C` | 0x18 | **`GetCollisionResponseType`** (slot 24): 0x6A when the byte at 0xCD0 is 1, else 0x2E | [proven] |
| `fn_10_1BEFC` | 0x7CC | **`DoUserAnimEvent`** (slot 28) | [proven] vtable |
| `fn_10_F40C` | 0x7C | **`GetScannableObjectInfo`** (slot 29): the 'SCAN' token at 0x92C when the actor is alive, frozen and visible | [proven] |
| `fn_10_1AAC4` | 0x1DC | **`GetCollisionPrimitive`** (slot 31) | [proven] vtable |
| `fn_10_1C7FC` | 0x1A0 | **`CollidedWith`** (slot 33) | [proven] vtable |
| `fn_10_FF20` | 0x94 | **`DamageVulnerability`** (slot 40): the phazon sub-object at 0xCD4 when the byte at 0xCD0 is 1, the combo vulnerability at 0xFD0 otherwise | [proven] |
| `fn_10_F488` | 0x138 | **`TakeDamage`** (slot 41) | [proven] vtable |
| `fn_10_CE9C` | 0x8 | **`CanBeShot`** (slot 42): true | [proven] |
| `fn_10_CEA4` | 0x8 | **`IsListening`** (slot 43): false | [proven] |
| `fn_10_CEAC` | 0x8 | **`Listen`** (slot 44): false | [proven] |
| `fn_10_CEB4` | 0x1C | **`GetOrigin`** (slot 45): the three floats at this+0x54/0x58/0x5C, the actor's translation | [proven] |
| `fn_10_CE54` | 0x8 | **`GetModelAlphau8`** (slot 50): the byte at this+0x44F | [proven] |
| `fn_10_1C6C8` | 0xB0 | **`ProjectileInfo`** (slot 55) | [proven] vtable |
| `fn_10_CE4C` | 0x8 | **`GetSearchPath`** (slot 56): this+0x7C0 | [proven] |
| `fn_10_CE5C` | 0x8 | **`GetPointSearchPath`** (slot 57): null | [proven] |
| `fn_10_CE64` | 0x8 | **`GetRagDoll`** (slot 61): null | [proven] |
| `fn_10_CE6C` | 0x10 | **`GetAttackTarget`** (slot 67): `kInvalidUniqueId` | [proven] |
| `fn_10_CE7C` | 0xC | **`IsOnGround`** (slot 68): bit 28 of the byte at this+0x34C (the `extrwi` at 0xCE80 keeps the byte's bit 28, the mask `1u << (31-28)` = 0x8) | [proven] |
| `fn_10_CE88` | 0xC | **`GetGravityConstant`** (slot 69): `lbl_8041B758` = **24.525f** (= `kDefaultGravityAccel`) | [proven] |
| `fn_10_CE94` | 0x8 | **`GetDeathExplosionParticle`** (slot 73): this+0x754, the base class's own member | [proven] |

The module's own 535 functions are mostly the boss's AI: the state machine's triggers, states and
codes (the 92 function pointers that follow the vtable inside `lbl_10_data_CBC`, see below), the
attacks (melee, dive, scatter shot, the missiles, the freeze beam, the sweep beam, the boost
ball) and the phazon enrage. Those are milestones 2+.

### Vtable `lbl_10_data_CBC` (174 words, 2 leading zeros)

The first **82** slots are the `CEntity`..`CPatterned` table, in the order of
`docs/module-analysis/vtable-offsets.md` (which was derived from the SpacePirate REL's
`lbl_72_data_8D8` and holds for every class in the chain). The entries the REL inherits keep the
DOL's own symbols, so the table also says what is *not* overridden: slot 4 is
`CPatterned::PreThink`, slot 19 `Touch`, slot 38 `Death`, slot 39 `KnockBack`, slot 60
`SetupStateMachine`, slot 70 `IsScanVisorSelfRender`.

The **32 local overrides** (slot → function) are the ones listed in the function map above:
2 `fn_10_1EF38` (dtor) · 3 `TypesMatch__10CDarkSamusCFi` · 5 `fn_10_1B1FC` · 6 `fn_10_1E89C` ·
9 `fn_10_18AB8` · 10 `fn_10_17F04` · 11 `fn_10_18964` · 15 `fn_10_0` · 16 `fn_10_100DC` ·
18 `fn_10_F5C0` · 20 `fn_10_10358` · 21 `fn_10_10210` · 24 `fn_10_F84C` · 28 `fn_10_1BEFC` ·
29 `fn_10_F40C` · 31 `fn_10_1AAC4` · 33 `fn_10_1C7FC` · 40 `fn_10_FF20` · 41 `fn_10_F488` ·
42 `fn_10_CE9C` · 43 `fn_10_CEA4` · 44 `fn_10_CEAC` · 45 `fn_10_CEB4` · 50 `fn_10_CE54` ·
55 `fn_10_1C6C8` · 56 `fn_10_CE4C` · 57 `fn_10_CE5C` · 61 `fn_10_CE64` · 67 `fn_10_CE6C` ·
68 `fn_10_CE7C` · 69 `fn_10_CE88` · 73 `fn_10_CE94` [all proven by the vtable entry].

Slots **82..173** are not vtable entries: 92 further function pointers (`fn_10_F840`, `fn_10_1BB08`,
`fn_10_62B4`, ... `fn_10_FE18`) sit inside the same label. They are the module's own state-machine
handlers (section 7): the 49 triggers in the words 82..130 and the 43 states in the words 131..173.
The count objects in front of the vtables are `lbl_10_data_0`..`lbl_10_data_3C` - 16 words
holding 0x3B, 0x20, 0x32, 0x14, 0x2F, 0x21, 0x23, 0x14, 0x3B, 0x14, 0x20, 0x31, 0x3B, 0x14,
0x3B, 0x14 - the trigger/state/code counts (`lbl_10_data_40` starts the CScattershotProjectile
vtable) [probable: the 82-entry prefix matches the chain exactly, so the tail is the module's own
tables].

## 2. CDarkSamus members (`CHECK_SIZEOF 0x1128`)

Size [proven]: the loader's `__nw__FUlPCcPCc` is `li r3, 0x1128` at 0x0000DA78. Base: `CPatterned`
(0x7c0) built with `EPatternedAI` **7** (`li r4, 0x7` at 0x0001FB00; the host's
`kPAI_DarkSamus = 7`), flavor 0 (`li r7, 0x0` at 0x0001FB08), movement 0 (ground), collider 1, body
**8** = the host's `kBT_4WayBlended` (the stack words at 0x0001FA84..0x0001FABC) [proven].

| off | member | proof / note |
|---|---|---|
| 0x7C0 | **`CPathFindSearch` (0xEC)** | `fn_8013E6A0(this+0x7C0, null, 0x401, pInfo+0xEC, 1.0f, 1.0f, 0, 0)`; `fn_10_CE4C` returns this+0x7C0. The flags are `li r5, 0x401` (0x0001FB54), the index the patterned info's pathfinding index (`lwz r6, 0xec(r30)`, 0x0001FB40), the two radii `lbl_10_rodata_3E8` = 1.0f (0x0001FB38/0x0001FB48), the area null (0x0001FB50) and the root position 0 = `kRP_Center` (0x0001FB5C) |
| 0x8AC | int = −1 | `stw r0(-1), 0x8ac` |
| 0x8B0 | **the elapsed time**: the Think adds dt to it | the constructor writes 0.0f (`stfs f0, 0x8b0` at 0x0001FB88, f0 = `lbl_10_rodata_3FC`); `fn_10_1B1FC` grows it by dt (0x0001B2DC) [proven] |
| 0x8B4 / 0x8B8 / 0x8BC | a position | the constructor writes the zero vector; `fn_10_1C778` writes a vector into it (0x0001C7A4..0x0001C7BC) [proven] |
| 0x8C0 / 0x8C4 / 0x8C8 | three zeroed floats | `sZeroVector` |
| 0x8CC | **`CTransform4f` = identity (0x30)** | `__ct__12CTransform4fFRC12CTransform4f(sIdentity)` at 0x0001FBC0 |
| 0x8FC / 0x900 / 0x904 | three zeroed floats | |
| 0x908 | float = the ctor's first float argument | `stfs f24, 0x908` (0x0001FBF0) |
| 0x90C | flag byte | three `rlwimi`s (bits 24, 25, 26) at 0x0001FBF8..0x0001FC18 |
| 0x910 / 0x914 / 0x918 | three zeroed floats | |
| 0x928 | byte 0 | |
| 0x92C | **`CToken`** | `fn_10_21D14(this+0x94C, ...)` builds the object at 0x94C; the token at 0x92C is the 'SCAN' tag [probable] |
| 0x930 | int 0 | |
| 0x94C | sub-object built by `fn_10_21D14` | [proven] call |
| 0x958 / 0x95C / 0x960 | the position the still timer measures from | the constructor writes the zero vector (`stfs` at 0x0001FC64..0x0001FC74); `fn_10_1D794` moves it to the actor's position when the actor walks away [proven] |
| 0x964 | the still timer: how long the actor has stood still | the constructor writes 0.0f (`stfs f2, 0x964` at 0x0001FCB0, f2 = `lbl_10_rodata_3FC`); `fn_10_1D794` adds dt to it while the actor stays within 0.6 of 0x958 [proven] |
| 0x968 | byte 0 | |
| 0x96C | float | |
| 0x970 | int = −1 | |
| 0x974 | `TUniqueId` = `kInvalidUniqueId` | `lhz`/`sth` at 0x0001FCC0/0x0001FCC4 |
| 0x978 | int 0 | |
| 0x97C | flag byte | two `rlwimi`s (bits 24, 25) at 0x0001FCD0/0x0001FCE0 |
| 0x980 | **the attack the actor is performing** (the same id space as 0x984), 0xFFFF = none | the constructor writes 0xFFFF (`stw r10, 0x980` at 0x0001FCE4, r10 = 0xFFFF); `fn_10_1B478` writes the attack id into it (0x0001B4CC) and 0xFFFF into it when the attack ends (0x0001B5A8) [proven: the call sites pass the ids, e.g. `lis r4, 0x2` = 0x20000 = the boost ball at 0x00009758 in `fn_10_9724`, and `lis r4, 0x40` = 0x400000 = the phazon attack at 0x00008210 in `fn_10_81D4`] |
| 0x984 | the attack the fight script picked | the constructor writes 0xFFFF (`stw r10, 0x984` at 0x0001FCE8); the Picked* triggers compare it [proven] |
| 0x988 | int 0 | |
| 0x98C / 0x990 | floats | 0x98C/0x990 = `lbl_10_rodata_410` = −1000.0f (0x0001FCF0/0x0001FCF4); 0x990 is the time the current attack started (`fn_10_1B478` adds the elapsed time to it at 0x0001B5B8) [proven] |
| 0x994 | the number of ids in 0x998 | the constructor writes 0 (`stw r9, 0x994` at 0x0001FCF8); `fn_10_1B478` counts the finished attacks up (0x0001B5A4) [proven] |
| 0x998 | the ids of the attacks the actor has finished (at most three) | `fn_10_1B478` pushes the current attack here (0x0001B590) and trims the oldest entry when there are three (`fn_10_1B5E4` at 0x0001B574) [proven] |
| 0x9A4 | sub-object built by `fn_10_EA24` | [proven] call |
| 0x9E0 | sub-object built by `fn_10_15348` | [proven] call |
| 0xA0C | sub-object built by `fn_10_21A14` | [proven] call |
| 0xA4C | sub-object built by `fn_10_2199C` | [proven] call |
| 0xA88 | sub-object built by `fn_10_217FC` | [proven] call |
| 0xAC0 | sub-object built by `fn_10_E784` | [proven] call |
| 0xB50 | sub-object built by `fn_10_215FC` | [proven] call |
| 0xC24 | a `TUniqueId` | the constructor writes `kInvalidUniqueId` (0x0001FF08); `fn_10_1E26C` writes a uid into it (0x0001E288) [proven] |
| 0xC28 | the number of uids in 0xC2C | the constructor writes 0 (0x0001FF14); `fn_10_1E26C` counts them up (0x0001E2D0) [proven] |
| 0xC2C | the uids (at most two) | `fn_10_1E26C` pushes a uid here (0x0001E2C4) and trims the oldest when there are two (`fn_10_1E2E8` at 0x0001E2B0) [proven] |
| 0xC44 | **the time of the last wall collision** (inside the 0xB50 sub-object) | the constructor writes 0.0f (0x0001FF38); `fn_10_1CF84` (0x0001D02C/0x0001D074) and `fn_10_1DF74` (0x0001E0BC) write it [proven] |
| 0xCC0 | the damage the actor has taken | the constructor writes 0.0f (0x0001FF58); `fn_10_1E178` adds the health that is gone and decays it (0x0001E200/0x0001E238) [proven] |
| 0xCC4 | the health at the last damage check | the constructor writes `lbl_10_rodata_3EC` = −1.0f (0x0001FF5C); `fn_10_1E178` compares it with the health info's current health (the vtable slot 0x3C, `GetHealthInfo`, and the float at its +4) [proven] |
| 0xCC8 | the threshold `TooMuchDamage` compares the damage with | the constructor writes its 8th float argument (0x0001FF60), which is the loader's `damageInterruptThreshold` (the property id 0x8f70d3f2) [proven: the loader's dispatch compares the property id at 0x0000D00C] |
| 0xCCC | the rate the damage decays at | the constructor writes `lbl_10_rodata_3E8` = 1.0f (0x0001FF64); `fn_10_1E178` subtracts it times dt (0x0001E22C) [proven] |
| 0xCD0 | sub-object built by `fn_10_E560`; **its first byte** is what `fn_10_F84C` and `fn_10_FF20` test | [proven] |
| 0xD64 | the CMdl/Cskr token pair built by `fn_10_20600` | [probable: the 'CMdl'/'Cskr' tag is read at 0x000202E8/0x0002033C] |
| 0xD74 | sub-object built by `fn_10_8DB4` | [proven] call |
| 0xDF0 | sub-object built by `fn_10_EDF4` | [proven] call |
| 0xE10 | sub-object built by `fn_10_895C` | [proven] call |
| 0xEF0 | sub-object built by `fn_10_45F8` | [proven] call |
| 0xF68 | sub-object built by `fn_10_21104` | [proven] call |
| 0xF80 | sub-object built by `fn_10_20F50` | [proven] call |
| 0x1048..0x1054 | floats (0x1054 = `lbl_10_rodata_3FC` = 0.0) | |
| 0x1058 | int 4 | `li r0, 0x4` at 0x000201E8 |
| 0x105C | int 0 | |
| 0x1060 | flag byte (bit 24) | |
| 0x1064 | sub-object built by `fn_10_20BFC` | [proven] call |
| 0x1090..0x109C | floats | |
| 0x10A0 | float = the ctor's eighth float argument | `stfs f23, 0x10a0` |
| 0x10A4 / 0x10A8 | byte / int | |
| 0x620 | `CAiKnockBackMgr` (base): **`EnableKnockBackPhysics(false)`** | `addi r3, r31, 0x620` (0x00020244), `li r4, 0x0` (0x0002024C), the call at 0x000202B0 |
| 0x681 | five bits of a base flag byte cleared (bits 25, 27, 28, 29, 30) | the `rlwimi` chain at 0x00020274..0x000202AC |

### The host's member map

The host does not reproduce the guest layout (the host object is larger than retail's), so
`CDarkSamus` holds only what milestone 1 needs:

| host member | guest | note |
|---|---|---|
| `mPathFindSearch` | 0x7C0 | the `CPathFindSearch` the retail builds; `GetSearchPath` returns it |
| `mData` (`CDarkSamusData`) | - | the loader's 60 properties, named after the property each comes from. Retail spreads them over the sub-objects above; the host keeps them in one block for the later milestones |
| `mGuestCD0` | 0xCD0 | the first byte of the sub-object `fn_10_E560` builds; `GetCollisionResponseType` reads it (0x6A when it is 1, else 0x2E). It stays 0 in the port, so the actor always answers 0x2E |
| `mPissedOff` | 0xD58 | the flag the PissedOff state clears (fn_10_F930) and the fight script sets; fn_10_FD30 reads it |
| `mFadeOut` | 0xDEC | the fade flag; fn_10_117A0 and fn_10_BBE4 read it |
| `mPickedAttack` | 0x984 | the word the fight script writes: the picked attack's id (the `Picked*` triggers compare it) |
| `mLastAttackerUid` | 0xC24 | the uid of the attacker that hit the actor last: fn_10_1E26C pushes it and fn_10_1E740 (`ShouldForgetLastAttacker`) decides when the actor drops it again |

**The guest's `CPathFindSearch` (0xEC at 0x7C0)** [proven from the DOL's constructor
`__ct__15CPathFindSearchFP7CPFAreaUiiffUiQ29CPFRegion13ERootPosition` at 0x8013E6A0 and its
`Search`]: +0x00 the area, **+0x04 the number of waypoints** (the `Search` appends at
+0x08 + count*12 and counts up at 0x8013DD78..0x8013DD80), +0x08 the 16 reserved waypoints
(0xC0 bytes), **+0xC8 the current waypoint** (the `Search` clears it at 0x8013DCF8),
+0xCC the result, +0xD0 the height, +0xD4 the radius, +0xD8 the padding, +0xDC the avoidance
filter, +0xE0 the flags, +0xE4 the index mask, +0xE8 the root position. So the guest's 0x7C4 is
the waypoint count and the guest's 0x888 is the current waypoint: fn_10_1E740's test
`0x888 >= 0x7C4 - 1` (0x0001E798..0x0001E7A8) is "the path find is over", which the host spells
`GetSearchPath()->GetCurrentWaypoint() >= GetWaypoints().size() - 1`.

## 3. Loader `fn_10_CF44`

Format [proven]: u16 property count, then per property u32 id + u16 size; unknown ids are skipped
with `ReadBytes(nullptr, size)`. **63 top-level properties**, the same id set as
`include/MetroidPrime/ScriptLoader/SLdrDarkSamus.inc` (checked id by id against the loader's
dispatch tree) [proven].

Construction order [proven]: `LdrToPatternedInfo(patterned, nullptr)` (`fn_8023A1D8` with r5 = 0,
0x0000DAC8) · `AllocateUniqueId` (0x0000DAD8) · `LoadActorParameters` (0x0000DAEC) ·
`fn_8023B2F8` ×12 (the damage infos, the animation set and the audio parms) ·
`LoadEditorTransform` (0x0000DB88) · `LdrToEntityInfo` (0x0000DB94) · `rs_new(0x1128)`
(0x0000DA84) · `fn_10_1FA14(uid, name, entityInfo, transform, modelData, patternedInfo, f1..f8,
about fifty stack arguments)` (0x0000DD84) - the ctor's argument order is
**uid, name, entityInfo, transform, modelData, patternedInfo**, then the floats and the stack
block.

The loader returns null when the model data is empty (the optional's valid byte at r1+0x36c,
0x0000DA3C) [proven].

## 4. What the module registration needed (milestone 1)

- `include/MetroidPrime/CEntityInfo.hpp`: `kET_DarkSamus = 111` already exists upstream, so no
  enum value was added.
- `src/MetroidPrime/TypesMatch.cpp`: `TYPES_MATCH_IMPL(CDarkSamus, CPatterned, kET_DarkSamus)` and
  the two `CAST_TO_*` entries under `#ifdef TARGET_PC` (the two "class not declared yet (DarkSamus
  REL)" placeholders are replaced).
- `src/MetroidPrime/Enemies/SDarkSamus_FuncPtrs.cpp`: the port's null guard on
  `RelProxy_LoadDarkSamus` was dropped, as for Ing / MinorIng / ElitePirate.
- `tools/modules_port.txt`: `DarkSamus` now points at
  `src/MetroidPrime/Enemies/PortCDarkSamus.cpp` (was `PortModuleStubs.cpp`);
  `tools/gen_files_cmake.py --check` passes.

## 5. Spawn area

`docs/modules.tsv` lists the module's areas: Agon Wastes/10_sand_alamostorage[L6];
Sanctuary Fortress/0j_cliff_hall_loop[L2]; Sanctuary Fortress/10_cliff_rooftop[L2];
Temple Grounds/12_temple_finalgate_dark[L4]. A module's actors only exist once the world layer
that holds them is active (`LAYER<n>` in `MP2_INPUT`, see `platform/README.md`).

**The area that works: Agon Wastes / 10_sand_alamostorage (area 46), layer 6** ('Dark Samus AI';
the area's nine layers are 0='Default', 1='!No Load', 2='Dark Samus', 3='Dark Samus Intro
Cinema', 4='Post Dark Samus', 5='Dark Samus Death Cinema', 6='Dark Samus AI', 7="Gamer's Day",
8='WAR CHEST'). The recipe (the same shape as the other families' smoke runs):

    MP2_WORLD="42b935e4,10_sand_alamostorage"  MP2_INPUT="LAYER6@400"  MP2_FRAMES=4000
    # then the console: `objs dark`, `send u0732 Activate`, `objs darksamus`, `obj u0732`

The run: layer 6 activated at frame 400 (17.0 s), the `CDarkSamus` appeared (uid u0732, editor id
002e00c4) and took `kSM_Create`; the actor is placed **inactive** (as the other families' bosses
are), and the console's `send u0732 Activate` wakes it - after that it moves (its z goes 9.4 ->
4.1), `obj u0732` shows `hp 600/600` and its repulsor / camera-shaker / battle-stage connections,
and the stand-ins for `Think`, `PreRender`, `AddToRenderer`, `Render`, `GetOrbitPosition`,
`GetAimPosition`, `GetTouchBounds`, `GetScannableObjectInfo`, `GetCollisionPrimitive`,
`CollidedWith` and `AcceptScriptMsg` all fire. `grep -c '[port] caught|reach-stub'` is 0.

## 6. Status and open questions

**Milestone 1 is implemented**: the loader, the constructor (base arguments, the path search, the
knock-back physics), the vtable overrides that matter (Think, AcceptScriptMsg, PreRender,
AddToRenderer, Render, the accessors) and the port-only module registration. Everything else is a
once-per-method `[darksamus] stand-in fn_10_XXXX` log. The body controller is activated on
`kSM_Create` and in `Think`; retail activates it in `AcceptScriptMsg`'s **'XCRT'** arm
(`CBodyController::Activate` at 0x0001E9A4) and nowhere else [proven: the arm, not its trigger].

**Milestone 2, slice 1** (2026-10-21) is in: the 'XCRT' arm activates the body controller in
retail's order (activate, then forward to CPatterned) and the port's `Think` activation is gone
(retail has none), and the state machine's two tables and their 92 handlers are installed (section
7). The 92 bodies are still stand-ins: they log once and do nothing else.

**Milestone 2, slice 2** (2026-10-21) is in: the `MP2_DS_FSM` probe dumps the actor's FSM2 machine
(every state, its transitions, the trigger, the parameter and the target state) and logs the state
changes, and the four handlers the walk reaches - `Start` (`fn_10_F920`), `StartsInTheAir`
(`fn_10_F924`), `TurnToFacePlayer` (`fn_10_FE18`) and `DoneTurning` (`fn_10_F864`) - are ported
(section 7). The walk is now `Start` -> `TurnToFacePlayer` -> `TravelToCoverPoints`; the next
state and its triggers are the next slice.

**Milestone 2, slice 3** (2026-10-21) is in: `TravelToCoverPoints` (`fn_10_1DF74`) and its helpers
(the repulsion point and separation 1D9A4/1DD14, the additive animation 1D860, the glide emitter
11334, the cover-point scoring 1E360/1E3C0/1E4C4, the safe position EFC8/128D8) and the path
search's area (`fn_10_1E864`, called from the 'XALD' = kSM_AreaLoaded arm of AcceptScriptMsg;
without it every search answered kR_InvalidArea, every update forgot the cover point and the actor
hit a wall into `Dash`). The run now stays in `TravelToCoverPoints` and walks cover to cover (the
path find's waypoints advance, the near/path-over forgets fire). No Picked* trigger fires because
the rest of the 'XALD' arm - the battle-stage lookup that sets 0x974 - is not ported: that is the
next slice.

What is **not** established:

1. ~~The state machine: which of the 92 function pointers after the vtable are triggers, states and
   codes, and what they are named.~~ **Answered in section 7**: 49 triggers (words 82..130 of
   `lbl_10_data_CBC`) and 43 states (words 131..173), named by the module's own strings.
2. The identities of the sixteen sub-objects the constructor builds (0x94C..0x1064) and which
   loader property each one holds. The ctor's ~50 stack arguments could not be mapped back to the
   loader's properties, so the host's `CDarkSamusData` names its fields after the properties
   instead.
3. What the byte at 0xCD0 means (it picks the collision response and the damage vulnerability) and
   which of the 0xCD0 sub-object's fields sets it.
4. The boss's combat behaviour: the melee, dive, scatter-shot, missile, freeze-beam, sweep-beam
   and boost-ball attacks, the phazon enrage and the death sequence.

## 7. The state machine

`fn_10_18D78` is the module's own setup: it calls `InitializeStateMachine__10CPatterned` (the DOL's)
and then hands the machine two tables through the state machine's vtable - the 49-entry table
`lbl_10_data_450` (slot 0x18) and the 43-entry table `lbl_10_data_964` (slot 0x14) [proven: the
`lis/addi` pairs at 0x00018D94 and 0x00018DB4 and the counts 0x31/0x2B]. The vtable keeps
`CPatterned::SetupStateMachine` (slot 60), so the module installs its tables itself, from the first
`Think` (`fn_10_1B1FC` calls `fn_10_18D78` when the machine has no state yet) [proven].

Each table entry is 16 bytes: `{const char* name; u32 a; u32 b; u32 c}`; the names are the module's
own strings in `.rodata` (`lbl_10_rodata_579` for the first trigger, `lbl_10_rodata_871` for the
first state) [proven]. The entry's payload comes from a 12-byte record beside it - the 49 records
at `lbl_10_data_204` and the 43 at `lbl_10_data_760` - and the `.ctors` initialiser `fn_10_221C0`
copies each record into its entry (`entry+4 = record[0]`, `entry+8 = record[1]`,
`entry+12 = record[2]`; the copies at 0x000222B8..) [proven].

The record's second word is the **byte offset of the handler inside `lbl_10_data_CBC`** - the blob
that holds the actor's vtable (the words 0..81) and then the 92 handlers (the words 82..173). The
trigger table's records name the words 82..130 (0x148..0x208) and the state table's the words
131..173 (0x20C..0x2B4), and the handlers split the same way: the first 49 return a bool and take
`(mgr, float)` (`fn_10_F840` reads a flag byte and returns it: `lbz`/`extrwi` at 0x0000F840), the
last 43 return nothing and take `(mgr, EStateMsg, float)` (`fn_10_B164` keeps r5 and compares it
with 1 at 0x0000B1B4) [proven]. So the names, the tables and the name-to-function mapping are
`docs/module-analysis/DarkSamus_fsm.tsv` (92 rows).

**In the port**: `CDarkSamus::SetupStateMachine` (the host's hook for the tables) calls
`CPatterned::SetupStateMachine` and then `InstallStateMachineFunctions()`, which hands the machine
the two tables under the REL's own names; the machine finds a node's handler by name, so the
FSM2 asset's node names are the REL's names. The 92 bodies are stand-ins (they log once and do
nothing else): `src/MetroidPrime/Enemies/PortCDarkSamusFsm.cpp`.

**The probe** (`MP2_DS_FSM`, `src/MetroidPrime/Enemies/PortCDarkSamusFsm.cpp`): the transitions are
not in the REL's .data - they come from the actor's 'FSM2' asset - so the probe dumps the machine:
every state with the trigger of each of its transitions, the parameter that trigger carries and the
state it leads to, plus the trigger list; the state changes are logged as they happen (the state
body's `Activate` message). It has the shape of the Boost Guardian's `MP2_BG_FSM` probe. The
machine is **59 states, 92 triggers, no codes and no sub-machines**; the 43 state names of the
REL's table are the distinct names in it (several machine states share a name, e.g. `Interrupt`
and `MissileDeflect`).

**The walk** (Agon Wastes/10_sand_alamostorage, layer 6, the actor woken with the console's
`send u0732 Activate`): the machine is set up and the actor enters **`Start`** (`fn_10_F920`),
evaluates the trigger **`StartsInTheAir`** (`fn_10_F924`), enters **`TurnToFacePlayer`**
(`fn_10_FE18`), evaluates **`DoneTurning`** (`fn_10_F864`) and enters **`TravelToCoverPoints`**
(`fn_10_1DF74`), which then evaluates its 21 triggers. The last two steps need the ported
handlers: with the stand-in triggers answering false the walk stopped at `TurnToFacePlayer`.

**The ported handlers** (2026-10-21): `fn_10_F920` (the state body is a single `blr`),
`fn_10_F924` (the flag byte at 0x90C's bit 6, the `extrwi` at 0x0000F928), `fn_10_F864` (the byte
at 0xD4C) and `fn_10_FE18` (the flag byte's bit 7 and the turn flag cleared on `Activate`; the
turn ends at once when `fn_10_142F4` finds the player within 90 degrees of the actor's facing
axis - the guest's 0x28/0x38/0x48 = the transform's forward column, as in the SpacePirate's
note - and otherwise the taunt command `fn_10_8698` runs, `kBSC_Taunt` with the type 6 =
`pas::kTT_Six`, and the base's `AnimOver` ends the turn). The three branch-free bodies are marked;
`fn_10_FE18` is not, because in the run the player is already in front of the actor (the probe
read `inFront=1`), so the taunt command and the `AnimOver` branch never ran.

**The guest's flag bytes** the port keeps: 0x90C (bit 6 = "starts in the air", bit 7 = "alerted";
the constructor's `rlwimi` chain at 0x0001FBF8.. only clears them, so they start at 0), 0xD4C (the
"done turning" flag) and **0x6B4, which is the base class's animation state and not a pending
flag**: `fn_10_8698` stores 1 to it on `Activate` (0x000087E0) and 0 to it on `Deactivate`
(0x00008820), and the DOL's `ShouldRetry` (the call at 0x00008710, with 0x10 = `pas::kAS_Taunt`)
advances it to kAS_Repeat and then to kAS_Over. It is the member the base's `GetAnimOver` reads,
so the host drives it the way the other ported actors do (`CDarkCommando::Lurk`).

**The `fn_10_FE18` branches** (2026-10-21, resolved): a temporary hook (reverted) that put the
player behind the actor drove the state past the 90-degree test, so the taunt command path ran.
With the animation state driven as above the taunt now *ends*: the probe printed anim=1 (Ready)
on `Activate`, anim=2 (Repeat) while the body was in `kAS_Taunt`, and anim=3 (Over) when the
taunt was over - then the base's `AnimOver` answered true, the turn ended and the machine walked
on to `TravelToCoverPoints`. Both arms ran (the player in front ends the turn at once, the player
behind runs the taunt), so `fn_10_FE18` is marked.

**The `TravelToCoverPoints` triggers** (2026-10-21): the state checks 20 triggers (the FSM2 dump):
`ShouldGetPissedOff`, `ShouldReact`, `CanMissileDeflect`, `ShouldFadeOut`, the eleven `Picked*`
triggers of the fight script, `TooMuchDamage` and `WallCollision`. Ported so far: the ten
`Picked*` triggers whose whole body is the word at 0x984 compared with the picked attack's id
(`fn_10_117D8` melee 1, `fn_10_116F8` boost ball 2, `fn_10_11710` melee dash 4, `fn_10_11738`
scatter shot 8, `fn_10_11724` moving scatter shot 0x10, `fn_10_1174C` dive attack 0x20,
`fn_10_1178C` missile jump 0x80, `fn_10_11778` super missile 0x100, `fn_10_117EC` taunt 0x200,
`fn_10_11800` pause 0x400), `fn_10_117A0` (`PickedNormalMissile`: the byte at 0xDEC picks the
dive attack's id 0x20 or the normal missile's id 0x40) and `fn_10_FD30` (`ShouldGetPissedOff`:
the byte at 0x420's bit 6 and then the byte at 0xD58). The port keeps the word at 0x984 as
`mPickedAttack`, the byte at 0xD58 as `mPissedOff` and the byte at 0xDEC as `mFadeOut`.

The byte at 0x420's bit 6 (0x40) is a *base class* flag (0x420 lies inside the guest's CPatterned,
0x158..0x7C0) and the three places that test it (fn_10_F40C the scannable info, fn_10_17F04 the
render pass, fn_10_1D6CC the fade) test it together with the frozen and the visible flags - so it
is the alive flag, the host's `GetAlive()`.

**The picked attack's ids** (2026-10-21, proven): the word at 0x984 is a *bit mask* of the attack
the fight script picked, and the ids are 1 melee, 4 melee dash, 8 scatter shot, 0x10 moving scatter
shot, 0x20 dive attack, 0x40 normal missile, 0x80 missile jump, 0x100 super missile, 0x200 taunt,
0x400 pause, 0x10000 sweep beam, 0x20000 boost ball, 0x100000 freeze beam, 0x200000 the fade
variant of the normal missile, 0x400000 phazon attack, 0x800000 phazon enrage [proven: the
archive tree's matched decomp of the module (`../MetroidPrime2Port`, `src/MetroidPrime/ScriptObjects/
CDarkSamusState.cpp` and `CDarkSamusCDA0.cpp`/`CDarkSamus8680.cpp`/`CDarkSamus8114.cpp`, whose
module sha1 is preserved) spells the constants out, and the retail bytes agree]. The trap: retail
materialises a comparison as `subfic r0,r0,K` when K fits a signed 16-bit immediate and as
`lis r0,hi` + `subf` when it does not - **`lis r0,N` builds N << 16**, so `lis r0,0x2` is 0x20000
(the boost ball) and not 2. The port's `PickedBoostBall` and `PickedNormalMissile` had it wrong
and are fixed.

**The three DOL triggers** (2026-10-21): `PickedSweepBeam` (fn_10_CDA0), `PickedPhazonAttack`
(fn_10_8680) and `PickedPhazonEnrage` (fn_10_8114) are not in the REL's .text - the REL's table
(`lbl_10_data_CBC`, the words 113/109/110) points at the DOL's copies of them, whose bodies are the
same one-word comparison (the ids 0x10000, 0x400000 and 0x800000). The DOL's copies are not in the
port's sources, so the port implements the bodies itself against the host's member.

**The three ported triggers** (2026-10-21): `ShouldReact` (`fn_10_11158`, 0x154 bytes),
`TooMuchDamage` (`fn_10_10698`) and `WallCollision` (`fn_10_F800`). Two corrections to the earlier
reading of them:

- **The word at 0x980 is the attack the actor is performing, not a `TUniqueId`.** The constructor
  writes 0xFFFF to it (0x0001FCE4) and `fn_10_1B478` writes the attack id into it when an attack
  starts (0x0001B4CC) and 0xFFFF when it ends (0x0001B5A8); the call sites pass the ids of the
  module's own attacks (0x20000 the boost ball at 0x00009758, 0x400000 the phazon attack at
  0x00008210). The array at 0x998 with the count at 0x994 holds the **finished** attack ids, not
  uids. `TooMuchDamage`'s `cmplwi` against 0x8000 compares the whole word with 0x00008000 (see
  the mask note below); 0x8000 is bit 15 of the attack id.
- **`clrlwi rD, rS, 24` clears the HIGH 24 bits and keeps the low byte** (the mask is `& 0xFF`,
  not `& 0xFFFFFF`). `ShouldReact` reads the four trigger answers that way: the byte against 1
  (0x00011190, 0x000111C0, 0x000111E4, 0x00011210).

- **`cmplwi` compares the whole 32-bit register against the 16-bit immediate.** The guest's
  `addis r0, r4, 0x0` is a plain copy of the word into the register `cmplwi` destroys, so
  `TooMuchDamage`'s `cmplwi r0, 0x8000` (0x000106A0) is the test `word == 0x00008000` - not a
  half-word test. The archive tree's matched decomp shows the same idiom for a constant with the
  high bit set (`addis r0,r3,-29495 ; cmplwi r0,53261` is `word + 0x8CC90000 == 0xD00D`,
  `src/MetroidPrime/CMainShutdownSubsystems.cpp`).

`ShouldReact` reads the four module triggers through the handler table the vptr points at: the
word at 0x208 is `WasHeavyHit` (`fn_10_112AC`) and answers the trigger at once, and then the
words at 0x174 (`CanMissileDeflect`), 0x184 (`FacingPlayer`) and 0x14C (`AnyIncomingMissiles`)
must all answer true; the switch over the attack id answers true for 0x8, 0x40, 0x200, 0x400,
0x10000 and 0x200000, the fade flag at 0xDEC for 0xFFFF and false otherwise. `TooMuchDamage`
answers when the damage at 0xCC0 is above the threshold at 0xCC8 (the `extrwi` takes the CR0 bit
1, the "greater than" of an `fcmpo`), unless the current or the last finished attack id is
0x8000. `WallCollision` answers when the actor has stood still for more than 1.2 seconds (the
timer at 0x964) or when less than 0.2 seconds have passed since the last wall collision (the
time at 0xC44 against the elapsed time at 0x8B0).

**The attack ids** (2026-10-21, proven): they are bits. The fight script's picked ones (the word
at 0x984) are bit 0 the melee (1), bit 2 the melee dash (4), bit 3 the scatter shot (8), bit 4
the moving scatter shot (0x10), bit 5 the dive attack (0x20), bit 6 the normal missile (0x40),
bit 7 the missile jump (0x80), bit 8 the super missile (0x100), bit 9 the taunt (0x200), bit 10
the pause (0x400), bit 20 the freeze beam (0x100000) and bit 21 the fade variant of the normal
missile (0x200000); the DOL's two more are bit 22 the phazon attack (0x400000) and bit 23 the
phazon enrage (0x800000). The module's own attacks (the ids `fn_10_1B478` writes into 0x980)
are bit 16 the sweep beam (0x10000, `lis r4, 0x1` at 0x0000CA44 in `fn_10_CA0C`), bit 17 the
boost ball (0x20000, 0x00009758 in `fn_10_9724`), bit 18 the fade in (0x40000, 0x0000BD60 in
`fn_10_BD40`), bit 19 the fade out (0x80000, 0x0000BC74 in `fn_10_BC54`), bit 22 the phazon
attack (0x400000, 0x00004AE8 in `fn_10_4AC8`) and bit 23 the phazon enrage (0x800000,
0x00006BF0 in `fn_10_6BA0`). **0x8000 is bit 15**, in the same family as the sweep beam, and no
Picked\* trigger watches it.

**The ported helpers** (2026-10-21): `fn_10_1D794` (the still timer at 0x964 and the
position it measures from at 0x958), `fn_10_1E178` (the damage at 0xCC0, the health at 0xCC4
and the decay rate at 0xCCC) and `fn_10_1C778` (the path find's destination at
0x8B4/0x8B8/0x8BC and the path find itself, through the base class' navigation at 0x6F4).
`fn_10_1D794` and `fn_10_1E178` are called from `fn_10_1DF74` (the `TravelToCoverPoints`
state) and, for `fn_10_1D794`, also from `fn_10_FB8C` (`TravelToPissedOff`). `fn_10_1E26C`
(the last attacker's uid at 0xC24, the count at 0xC28 and the uids at 0xC2C) and its helper
`fn_10_1E2E8` (the oldest uid is dropped) are ported as well; `fn_10_1DF74` calls
`fn_10_1E26C` at 0x0001E088 with the uid it read from 0xC24.

**The walk** (2026-10-21, with the three triggers ported): `Start` -> `TurnToFacePlayer` ->
`TravelToCoverPoints` -> `Dash`. The last step is `WallCollision` (the FSM2 dump: the trigger
`WallCollision` of `TravelToCoverPoints` leads to `Dash`), and it fires because the wall
collision time at 0xC44 is 0.0f after the constructor and the elapsed time at 0x8B0 is still
below 0.2 seconds - the port grew the elapsed time only while the actor was active, while
retail's `fn_10_1B1FC` grows it in every Think, also while the actor is inactive. With the
elapsed time grown in every Think the walk stays in `TravelToCoverPoints`.

**Next**: the rest of the `TravelToCoverPoints` state. `fn_10_1DF74` calls ten module helpers
of its own (`fn_10_1D860`, `fn_10_128D8`, `fn_10_11334`, `fn_10_1E740`, `fn_10_1E3C0`,
`fn_10_1E26C`, `fn_10_1C778`, `fn_10_1DD14`, `fn_10_1E178`, `fn_10_1D794`), so it is a chain of
slices, not one body. The ones that are ported: `fn_10_1D794`, `fn_10_1E178`, `fn_10_1C778`,
`fn_10_1E26C`, `fn_10_1E2E8` and `fn_10_1E740` (2026-10-21: `ShouldForgetLastAttacker` - the
actor drops the attacker's uid when the path is shagged, when the path find is over, when the
attacker has no cover point, or when the cover point is within 2 units (the squared distance
against `lbl_10_rodata_408` = 4.0f at 0x0001E840); its only caller, `fn_10_1DF74`, clears the
uid at 0xC24 when it answers true, 0x0001E048..0x0001E050). The ones that are left, and the
helper each one needs (none of the helpers is in the REL's own tables, so each is a slice of its
own): `fn_10_128D8` (0xDC bytes: the position at 0x91C/0x920/0x924 and the flag byte at 0x928,
but it calls `fn_10_EFC8`, a 0x1DC-byte body that walks the AI hints of the fight script's
object), `fn_10_11334` (0xF4 bytes: the emitter around the actor - the sound id at 0xC48 and the
handle at 0xC4C; the DOL's `AddEmitter` takes the priority 0 (`lbl_8041E2E4`), rejects the
invalid id 0x7FFF (`lbl_8041E2E6`) and the update's volume is `lbl_8041E9C8` = 0x7F), `fn_10_1E3C0`
(0xDC bytes: it walks the cover points of the fight script's object and needs
`CScriptCoverPoint`; it also calls `fn_10_1E360` and `fn_10_1E4C4`), `fn_10_1D860` (0x144 bytes:
the additive animation - it calls `fn_10_13FFC`, a static helper that picks the animation
parms), `fn_10_1DD14` (0x160 bytes: the separation - it calls `fn_10_1D9A4`, a 0x31C-byte helper
that walks the repulsors and the path find regions) and then `fn_10_1DF74` itself (0x400 bytes).

**The guest's 0xC48 / 0xC50** (2026-10-21): two sound ids the constructor writes from its stack
arguments (the `lhz` at 0x0001FF3C and 0x0001FF40); 0xC48 drives the looping emitter of
`fn_10_11334` (both of the DOL's `AddEmitter` flags set, 0x00011394/0x00011398) and 0xC50 an
emitter in the Think with one of them off (0x0001B7AC/0x0001B7C4), but **which** loader property
(`glideSound` / `missileRicochetSound`) lands in which of the two is **not** established - the
constructor's ~50 stack arguments are still not mapped back to the loader's properties.

**The fight script's object** (2026-10-21): the helper `fn_10_17E14` gets it from the uid at
0x974 (`fn_10_17DE0`: the lhz at 0x00017DEC, `GetObjectById` at 0x00017DFC and the cast to
`CScriptDarkSamusBattleStage` at 0x00017E00) and module 11's accessor `fn_11_B70` returns the
object's 0x24. The guest's object holds the stage info there in the loader's
`SLdrDSStageInfo` layout (the guest's ctor `fn_11_90C` stores the nine floats at 0x00..0x20, the
missiles at 0x24, the six bools at 0x30..0x35, the floats at 0x38/0x3C, the byte at 0x40, the
floats at 0x44..0x54, the bytes at 0x58/0x59 and the floats from 0x5C - the host's struct's own
layout), so the two triggers read the host's `CDarkSamusBattleStage`'s stage info: `ShouldFadeOut`
(`fn_10_BBE4`) the second bool (`unknown_0x49b9936d`, the guest's +0x31) and `CanMissileDeflect`
(`fn_10_14284`) the first missile float (`missiles.unknown_0x7ee77018`, the guest's +0x24, read
against zero). The host keeps the uid at 0x974 in `mBattleStageUid`, but nothing sets it yet: the
retail code that does (the ray cast `fn_10_193A0`, which stores the uid at 0x00019410) is not
ported, so the lookup finds nothing and both triggers answer false - as they do in retail before
the actor has its stage.

`fn_10_1DF74` is not a small body: it calls ten module helpers of its own (`fn_10_1D860`,
`fn_10_128D8`, `fn_10_11334`, `fn_10_1E740`, `fn_10_1E3C0`, `fn_10_1E26C`, `fn_10_1C778`,
`fn_10_1DD14`, `fn_10_1E178`, `fn_10_1D794`), so it is a chain of slices, not one body.

The three state names the base class also has - `Start` (`fn_10_F920`), `Dead` (`fn_10_143A8`) and
`Patrol` (`fn_10_1142C`) - forward to `CPatterned`'s body after the log, the way the port's other
stand-ins do, so the death path keeps working until the module's own bodies are ported: three
`damage u0732 200` hits take the actor to `hp 0/600`, the machine enters `Dead` and the actor
leaves the area (checked in the same run).

**The battle-stage link** (2026-10-21, slice 4): 0x974 has two writers - the 'XALD' arm of
`fn_10_1E89C` (0x0001EBB8: the object connected with 'IS05' = kSS_InternalState5 and any message,
cast to CScriptDarkSamusBattleStage; its uid also goes to 0xD50 and the stage count 0x978 becomes 1)
and `fn_10_193A0` (0x00019410), which is **not a ray cast**: it is the stage advance (the stage
info's +0x0 threshold against `fn_10_17E90`, the health fraction; the next stage is the current
one's 'CONN' connection, `fn_10_194AC`; the count 0x978 grows by 1). The 'XALD' link is ported; in
the run the stage is u0339. The picked attack (0x984) is written only by `fn_10_1954C` (0x0001965C,
0x00019670) and `fn_10_1B478` (0x0001B4E0); `fn_10_1B1FC` (Think) calls `fn_10_193A0` and
`fn_10_1954C` at 0x0001B308/0x0001B314 when alive. `fn_10_1954C` needs `fn_10_1AA6C` (0x58,
calls `fn_10_17CEC`), `fn_10_197D8`/`fn_10_1969C` (0x140/0x13C: eight per-attack collectors
1A5BC, 1A554, 1A278, 1A440, 1A34C, 1A1D4, 19B64, 19A1C, then `fn_10_14648`/`fn_10_1A980`, the
copy into 0xC54/0xC58 and the weighted pick `fn_10_19918` into 0xCBC). **Slice 5** (2026-10-21): the chain is in (1905C, 19A1C, 14648, 197D8, 1969C, 1954C; 19B64 is a
stand-in that clears nothing) and Think calls 1954C when alive (0x0001B314; 193A0 and the rest of
Think are not ported). Walk (user_b save, layer 6, GODMODE): Start -> TurnToFacePlayer ->
TravelToCoverPoints -> PickedScatterShot (0x984 = 8 at t=2.5 s, mask 0x6D8) -> ScatterShot
(stand-in fn_10_175AC, never ends). Later picks: 0x80, 0x10, 0x40, 0x200; 0x980 stays 0xFFFF
because fn_10_1B478 (the attack start) is not ported, so 1954C keeps picking. 0x9C0 has one
writer (fn_10_17BC0, MeleeAttack, 0x00017C9C); the constructor does not write it.

**Slice 6** (2026-10-21): 0x98C/0x990 start at -1000.0f (fixed; with that the first random pick in
the run is MissileJump, not ScatterShot). In: fn_10_1B478 (attack start/end: 0x980, 0x984 cleared,
0x990 = random stage gap, then += 0x8B0 at the end; history push + fn_10_1B5E4 erase), ScatterShot
fn_10_175AC with 10520 (0xD28/0xD34 orbit/aim), 1AEC0 (collision failsafe), 16B64/16B8C (turn
toward player, 1.7453293 rad/s), 11974 (fire timer while 0xBAD; its fire fn_10_12F60 is a stand-in)
and 21F14 (CBCProjectileAttackCmd severity 1, kAS_ProjectileAttack). The 0xB50 sub-object
(fn_10_215FC): 0xB7C/0xB88/0xB94 line start/aim/end, 0xBA0 count, 0xBA4/0xBA8 delay/timer,
0xBAD firing (set only by fn_10_11814, from the anim-event handler at 0x0001C25C), 0xBDC =
unknown_0x378285b9, 0xBE0 = scatterShotProjectilesPerSecond, 0xBE4 projectile uid. Walk (pick
forced to 8 by a reverted hook): TravelToCoverPoints -> ScatterShot (gap 2.0..3.0 -> 2.55 s) ->
AnimOver after ~2.4 s -> Dash (stand-in fn_10_150DC, BreakDash stand-in fn_10_149B4); picks
resume at 0x990 = 6.37 s. Unforced: TravelToCoverPoints -> MissileJump (stand-in fn_10_177BC).

**Slice 7** (2026-10-21): the scatter shot fires. DoUserAnimEvent's kUE_EventStart arm (case 27 of
jumptable_10_data_FB4, 0x0001C1F0; never reaches the base) starts it with fn_10_11814 (side 0 for
attack 8 at 0x0001C25C, the byte 0xBAC for 0x10 at 0x0001C268); 0xBA4 is the start time (0x8B0),
not a delay. fn_10_13230 is the jittered aim point (0.47 s sweep for 8, 0.67 s otherwise).
fn_10_12F60 spawns the shot: the module's own CScattershotProjectile (CWeapon subclass, 0x27B0,
ctor fn_10_2AD0, vtable lbl_10_data_40, code 0x0..0x44D0; ported in Slice dsscat, which replaced
the CEnergyProjectile stand-in), fed by fn_10_C98 (add a shot to the live swarm). 0xB50/0xBB0 are the two optional CProjectileInfos (scatterShotProjectile/2, one damage;
valid bytes 0xB78/0xBD8), ProjectileInfo (fn_10_1C6C8) picks 0xBB0 when 0xBAE = (rand < 0xBDC,
/100 when > 1). 0x8CC = GetLctrTransform("GUN_Particle_LCTR") in PreRender (0x00018C18). Forced
walk: ScatterShot -> 5 shots from the gun locator, each gone after 0.15..0.45 s -> Dash.
**Dash** (fn_10_150DC) is a chain, not one body: fn_10_15348 (the 0x9E0..0xA08 dash record),
fn_10_154FC (0x600 bytes, the dash path), 14A00, 14D14 (with 12C2C/14C50/14FE0, calls 1B478),
12CE0 (21DF8/EDE0, calls 1B478); 0xA08 bits 0x40 (Range(0,100) < stage+0x20) and 0x80
(BreakDash fn_10_149B4: 0x80 set -> true, else 0xA04 > 0 -> false, else GetAnimOver).

**Slice 8** (2026-10-21): the stand-in shots are not killed by the boss: the player stood on a
ledge ~15 units above the boss (z 19 vs 4), shots fly ~72 u/s along the right line to the
jittered aim (player z -1.5..+7.5) and hit static render geometry (ledge lip / walls, material
0x1B/0x1D00002000000000) after 0.2..0.55 s; no hit on the first frame, none on the owner.
Retail's CScattershotProjectile is a plain CWeapon (no CGameProjectile ray/WPSC flight): its
ctor (fn_10_2AD0) stores 45.0f at 0x1D4 and seeds shots via fn_10_C98 with 0.5/2.0 parameters.
MovingScatterShot fn_10_11AD8 = ScatterShot's Activate/Deactivate without the failsafe, turn +
11974 + 1B478(0x10) on every message, then the step command fn_10_21DF8 (kAS_Step, BreakDodge,
right unless 0xBAC, then left). 0xBAC is set only by the unported picker collector fn_10_19B64
(0x0001A020/0x0001A0D0). 0x9E0..0xA08 is CDarkSamusDash (fn_10_15348; 0x9FC was mGuest9FC).

**Slice 9** (2026-10-21): the Dash chain. Dash fn_10_150DC (double-dash roll: Range(0,100) <
the stage info's +0x20 = `chanceToDoubleDash`), the slot pick fn_10_154FC (eight slots, the end
point = the step animation's distance along fn_10_EB90's direction, slot -> step direction table
fn_10_EDE0 {0,1,2,3,6,7,8,9}), the path clearance fn_10_16048 (three rays, 0.49 off per blocked
side ray), the stuck blast fn_10_14A00 (guest 0x968 = `mDashStuckCount`, radius 15 + count, 100
damage, instant kill), the double dash fn_10_14D14 with 14FE0 (pick, no target), 14C50 (next step)
and 14E5C (step anim, advanced 0.3 s), and the step 12CE0/12C2C (attack id 0x8000). The
CDarkSamusDash members are named: 9E0 `mSlot`, 9E4 `mDestination`, 9F0 `mStartPosition`, 9FC
`mLastSlot`, A00 `mWeight`, A04 `mStepsLeft`, A08 0x80 `mBlocked`, 0x40 `mDoubleDash`. Runs (pick
forced to 8, reverted hooks; z 9.43 -> 4.12 floor, no falls; 0 caught/reach-stub): 15 single
dashes (step ~16-18.7 u, one blocked pick of weight 0 -> BreakDash), and with the double dash
forced 13 double dashes (two picks each, the second with the last slot and its opposite zeroed,
weights = squared distance gained). Unmarked: 150DC/14FE0 (the stuck arm, still > 1.2 s, never
ran; no natural double-dash roll on this stage), 154FC (the failed-height and still >= 1.2 s
clearance arms), 16048 (the centre ray's static vs dynamic block not told apart), 14A00 (never
ran), 14D14 (blocked arm). Next: fn_10_19B64 (sets 0xBAC) is a tree, not one slice: 17CEC/1A980
are in; new are 16418 (-> 164C0, 0x140), 16600 (-> 166AC 0x1B4, 16ADC), 12B38, 141BC, 13E2C
(-> 13EE0), 16B1C, 145E4 (-> 1444C 0x198). SideDash fn_10_153EC also calls 154FC/12CE0.

**Slice 10** (2026-10-21): the reach collector fn_10_19B64 is in (it replaces the stand-in), with
its whole tree: 16418 (gun-locator line +5 z to the player's aim position, vtable 0x54 =
GetAimPosition) / 164C0 (RayStaticIntersection, include Solid|AIBlock), 16600 / 166AC (eight ring
rays, world offsets 16984, count 16ADC = 8, radius 1.1, reach 0.75; include Solid|AIBlock, exclude
Platform), 12B38 (dive target: the player when morphed, else 5 past it), 141BC (player distance in
[min, max] x the anim distance x scale y; melee kAS_MeleeAttack (2 | 0xA, locomotion type) with
0.7..1.3 for 0x4 and 0.9..3.0 for 0x80), 13E2C / 13EE0 (slot 2/3 break-dodge step offset from
the unflattened forward column), 16B1C (OnPath), 145E4 / 1444C (line on the mesh within 5 of the
actor's height, 0.1 steps). 0xBAC = 1 when slot 2's end is nearer the player, else 0. This stage's
mask is 0x6D8 (0x1/0x4/0x20/0x100 off), so the runs forced those bits in (a reverted hook that
also took them back out after the collector) and teleported the player (`tp 410.1 88.1 5.1`, then
419.1, then morph ball): all bits cleared and kept, both sides, morph 0/1, the anim reach above /
below / inside. Unmarked: 19B64 (the 145E4-false arm after both OnPath tests pass never ran),
166AC (the line <= 0.5 arm), 1444C (the height-band arm). Boss z 9.43 -> 4.12, no falls.

**Slice 11** (2026-10-21). *The shipped stage data* (each DSBS script object read from the MREAs,
SLdrDSStageInfo weights; a bit is on when its weight > 0, fn_10_17D50). Agon Wastes /
10_sand_alamostorage (area 46) layer 6: the actor links stage 182e0198 first (IS05), which goes
(kSS_Connect) to 182e019f and then 182e009e. 0198 (advance below 50 % health): meleeAttack 1,
meleeDash 8, scatter 10, movingScatter 5, missile 5, missileJump 15, taunt/pause 0.02 = mask 0x6D8
plus 0x1/0x4. 019f (below 10 %): meleeAttack 1, meleeDash 8, **dive 20, superMissile 10**. 009e
(final, threshold -1): dive 30. Sanctuary / 10_cliff_rooftop (area 65) layer 2: six stages
(80/60/40/20/10 % and a final -1); 08410175 has dive 50 + superMissile 10, 08410177 superMissile 15,
08410174 dive 60 + meleeAttack 15. So 0x1, 0x4, 0x20 and 0x100 are all enabled by shipped data in
the encounter the runs use: 0x6D8 was only the first stage. The slice-10 markers stay. Their arms
that ran only under the forced mask: 164C0/16418 (0x100 path), 16600/16984/16ADC (0x20 ring) - all
reachable once 019f is active with an unmorphed player in band 1; 141BC's "above" arm (0x4 forced
with an unmorphed player; shipped 0x4 survives only with a morphed player, fn_10_1A278); 12B38's
morph = 1 arm, which 19B64 cannot produce from shipped data (1A278 clears 0x20 when morphed) but its
other caller, the dive state fn_10_11FA4, can (a player who morphs during the dive); not run yet.
13EE0/13E2C/16B1C/145E4 ran on the natural 0x10 path.

*The stage advance* is in: fn_10_193A0 (from Think before the attack pick, 0x0001B308), 194AC (the
next stage, the first kSS_Connect link), 17E90 (health percent = 100 x hp / initial hp), B86C (wakes
the AI-hint chains linked with kSS_InternalState6 when the new stage's +0x35 is set) and the 0x97C
flags (`mUseAltScan` = +0x34, `mCanDie` = +0x32 not 1; F40C/F488 that read them are not ported).
Runs (reverted probes, `damage` steps from the console): Agon 0198 -> 019f at 40 % (300/600 = 50 %
did not advance, strict `<`); rooftop (`WORLD=10_cliff_rooftop`, `LAYER2@400`, boss u04f7, 600 hp)
went 0173 -> 0172 (79 %, mCanDie 0) -> 0175 (58 %, 1) -> 0176 (37 %, 0) -> 0177 (19.5 %, hints on:
B86C walked 58 type-0x11 chains, each to four active radius-0 hints) -> 0174 (9 %, threshold -1, no
further advance). Marked 193A0, 194AC, 17E90 (the null-stage and no-link arms are unreachable: every
encounter links a first stage, and only the final stage, threshold -1, lacks a link). B86C unmarked:
the inactive-next-hint, chain-end and non-0x11-start arms never ran (the shipped chains are loops of
already-active hints).

*SideDash* fn_10_153EC (+ fn_10_EDB8, the slot weights 0.6/0.7/1/1/0.8/0.8/0.9/0.9) is ported but
unmarked: the actor's FSM2 (59 states, the MP2_DS_FSM dump) has no SideDash node, so no shipped
transition enters it.

## Slice 12 (2026-10-21)

- fn_10_F40C (GetScannableObjectInfo) and fn_10_F488 (TakeDamage) ported, unmarked. New members:
  `mPhazonHealthMark` (0x1054), `mPhazonPhase` (0x1060 bit 0x80), `mGuestD24` (0xD24),
  `mAltScanInfo` (0x92C, the constructor locks the data's alternateScannableInfo).
- Temple Grounds `12_temple_finalgate_dark` layer 4: stage 102a0045 (advance below 78%,
  0xc96b8223 = 1, so the boss cannot die) links to 102a0044 (usesAlternateScannableInfo = 1, can die).
  Boss u0564 has 1380 hp. Runs with `damage` from the console (it goes through TakeDamage):
  - In 0045, 1380 damage: hp -20 is held at 1.0.
  - After the advance, 50 damage: hp -49, the boss dies (Dead state).
  - With the scan visor on, F40C answered the alternate token (alt=1, tok=1) after the advance and
    the base path before it (alt=0) or while inactive.
- Not run, so neither is marked:
  - F488: the inactive arm, and the phazon-phase arms. mPhazonPhase is set only by fn_10_6BA0
    (unported). Temple 0045 has phazonAttack 50, so shipped data reaches them.
  - F40C: the not-alive arm, and the null-token arm with alt=1. That arm is unreachable on shipped
    data: only Temple sets alt, and its boss has the token.
- Agon natural run (console damage 30 hp per step to 240/600 hp; real fire with `press L+A` did no
  damage): only the arms already seen ran naturally. They are 12B38 unmorphed, 141BC "inside"
  (0.9), 164C0 and 16418 blocked, and 16984 on its first ray. A morphed player (`fastmorph`, X)
  next to the boss never reached the collectors: the boss stayed in stand-in states
  (MissileJump, NormalMissileAttack). The morph arm of 12B38 needs DiveAttack fn_10_11FA4
  (0x764, a stand-in).
- Think tail: FD4C is the stage-change watcher. On a new stage count (0x978 vs 0xD54, which
  starts at 1) the previous stage's info +0x84 (`unknown_0x911a2476`) sets 0xD58 PissedOff, else
  0xD59. Agon 182e0198 has it set, so porting FD4C alone would send the Agon boss into the
  stand-in PissedOff/TravelToPissedOff states. Port it together with F930, FB8C, FAF4, FC64,
  105F4 and 18F90.
  1D6CC, 1D5C4 and 1B0E0 are the fade/invisibility block (0xDE8, 0xDEC, 0xDED, stage +0x33,
  fn_10_18628). F654 ramps 0xD24 from the 0xCD0 sub-object's vector at 0xD18. Neither is
  ported.

## Slice 13 (2026-10-21): real fire, the pissed-off chain

- Real fire does hurt her: with the player 9 units from her, facing her, 50 power-beam hits reached
  her CDarkSamus (eid 002e00c4, mode 0, multiplier 1.00) and each applied 2.0 damage. Slice 12's
  "no damage" was shots fired from the spawn point missing her. Nothing shared is wrong here.
- Ported and marked: FD4C (WatchStageChange, called from Think after the attack pick, ahead of the
  unported fn_10_1ACA0/fn_10_10A64), FAF4 (ReachedPissedOff), FB8C (TravelToPissedOff), F930
  (PissedOff), FC64 (FindAreaHint), 105F4 (UpdateShieldFlag), 18F90 (UpdateGroundCollision).
  0xD50 is renamed mSeenStageUid; 0xD54 mSeenStageCount, 0xD59 mStageChanged, 0xD18 mGuestD18 are new.
- Agon run (damage to 240/600 before Activate, so the 0198 advance lands while she is in
  TravelToCoverPoints, the only state with ShouldGetPissedOff): FD4C pissed -> TravelToPissedOff
  (Activate, Update, Deactivate by Dash, re-entry) -> FAF4 distance 25.8 .. 2.9 -> PissedOff with
  the type-0xE hint (005e) and the type-0xF face hint, 18F90 Activate/Deactivate -> AnimOver ->
  TurnToFacePlayer. Rooftop run: FD4C "changed" arm on five advances, then Dead.
- Not run, unreachable on shipped data: no type-0xE hint (FAF4/FB8C/F930/FC64 null arms), no type-0xF
  hint (F930 player turn), and FD4C's no-stage arm. Only Agon 0198 sets +0x84, and Agon has both hints.
- When the stage advances during a stand-in state (MissileJump), she never gets back to
  TravelToCoverPoints, so the reaction waits on those stand-ins.

## Slice 14 (2026-10-21): the dive

- Ported (`8feff8481`, `a55d1351d`): DiveAttack 11FA4 with FindDiveLanding 12960,
  SetDiveTargetVector 12B00, DoDiveJumpCmd 1285C/12764, UpdateDiveFlight 11C60, BounceDive 1CDD0,
  UpdateDiveStuck 11EB8, SendDiveAttack 11DDC, the bubble 12E90/12EF8, 18628, 18FCC, UpdateDodge
  1707C, CheckDiveCollision 1C99C (from CollidedWith), the dive record (ctor E784, Reset E910),
  BreakDiveAttack 11C54, and the XLAU/XLND and UE Landing arms. Also `d06bbbb92`: 100DC/FF20
  (unmarked; the enrage arm needs the phazon-enrage states). Stand-ins: 1B658 (reflect), 138A4 (the
  dive's hit on the player).
- Reaching the dive: damage before the first pick lands her in the final stage. A stage with
  melee, boost ball, sweep beam or missile jump soon traps her in a stand-in state with no exit;
  the dive also needs an unmorphed player in band 1 whom she can walk to. The Agon harness
  (damage 570, `stick 0 127` held into a wall, then a jump or a sideways teleport about 0.8 s
  into each dive) gave 3 and 10 dives, wall bounces among them. Rooftop runs (no teleports)
  covered the 0x58 = 1 sides (fast turn, 1.15 flight, dodge call).
- Ran: every DiveAttack arm except the jump re-issue (0x0001239C) and the static wall hit
  (0x00012534); bounce (both sides of 0.6 s, flat normal); morph target (12B38's morph arm,
  slice 10); landing found/none; flight set/impulse/fast; stuck moved/still; collision
  wall/floor/player; UE Landing (both arms); XLND (both arms).
- Marked: E910, 12B00, 12E90, 12EF8, 18628, 18FCC, 11C54 (all branch-free, all ran). Left unmarked,
  with a note above each: 11FA4, 12960 and 11C60 (their non-normalizable lines), 1CDD0 (no
  normal), 11EB8 (stuck bounce), 1707C (no dodge started), and 1C99C (WeaponHits arm). 11DDC's
  0xCD0 arm would need a dive stage without +0x59; none was seen in the two surveyed encounters.
- Next: MissileJump 177BC, MeleeAttack 17BC0, BoostBallInto B164 and SweepBeam CA0C are the
  traps that end dive runs.

## Slice 15 (2026-10-21): the melee states

- 16418 (IsPlayerAimLineClear) is not always-true: it matches the asm (fn_10_164C0 with the
  (0, 0, 5) offset and the player's aim position) and the real static ray test. No change.
- Ported: MissileJump 177BC, MeleeAttack 17BC0, MeleeDash 17ABC, the melee command runner
  RunMeleeAttackCmd 8764, the melee contact record's Reset EA08 (0x9A4: only +0x38/+0x3A on the
  host), and DoUserAnimEvent's kUE_Projectile arm (0x0001C134). That arm never reaches the base;
  its callees 13AB4/1353C/133D0/9164/5840 are not ported, so for now the melee and missile events
  do nothing. Before this, the base's LaunchProjectile crashed on the null ProjectileInfo the
  first time MeleeAttack's animation fired the event.
- Reaching them on Agon (damage 570, Activate; `/tmp` harness run_m.sh teleports the player on
  each TravelToCoverPoints entry): MeleeAttack in band 0 (close); MeleeDash only with a morphed
  player (1A278) 8.6-16 from her (141BC: 0.7-1.3 x 12.33); MissileJump with an unmorphed player
  26.4-32 away (0.9-3.0 x 29.34, inside band 1). Each exits by AnimOver to Dash.
- Runs: MeleeAttack 3x, MeleeDash 2x, MissileJump 10x; 0 caught.

| body | arm | ran |
| --- | --- | --- |
| 17BC0 | Activate (contact reset) / Update / Deactivate (0xC00, 0xC24) | yes / yes / yes |
| 17ABC | Activate / Update / Deactivate (0xC00, 0x9DC) | yes / yes / yes |
| 177BC | branch-free | yes |
| EA08 | branch-free | yes |
| 8764 | Activate / Update retry true / Update retry false / Deactivate | yes / **no** / yes / yes |

- Marked: 17BC0, 17ABC, 177BC, EA08. 8764 stays unmarked (its retry arm).
- Note: run_m.sh runs hit `DebugDrawAABox`, a reach stub reached from CGunTurretBase.cpp:636,
  not Dark Samus. The run_r2.sh runs never hit it.

## Slice 16 (2026-10-21): BoostBallInto

- Ported: BoostBallInto B164 with the boost hint search B6C4, the arrived hint B458, the chain test
  BA38 (15 steps), the ball actor 9920 and the ball/emitter update A06C. New members: the 0xE10
  sub-object's 0xE38 ball uid, 0xE68 roll sound ([probable] boostBallRollSound, by fn_10_895C's
  argument order), 0xE6C handle, 0xEBC bit 0x40, and 0xEC0..0xED8 (hint uid, distance, 0xEC8,
  0xECC, clamped value). B570/B670 are the rstl vector copy/dtor around FindConnectedObjects.
- Reach: `fsm <id> BoostBallInto` (drivers `tools/drivers/darksamus_boost.con` on Agon,
  `darksamus_boost_rooftop.con` on the rooftop, `MP2_WORLD="1baa96c2,10_cliff_rooftop"`,
  `LAYER2@400`, boss u04f7, 1500 hp). Natural picks reach it too (PickedBoostBall seen on the
  rooftop). It exits by AnimOver to BoostBallRoll (fn_10_A3B0, a stand-in, which traps the run).
  Agon's boss has no kSS_InternalState6 links (no hint) and no roll sound; the rooftop's links 37
  type-0x11 hints, all active from the start, with value parameters 0.74-0.95.

| body | arm | ran |
| --- | --- | --- |
| B164 | Activate / Update / Deactivate | yes / yes / yes |
| B164 | no start hint (Agon) / start, arrived hint found (rooftop) / start, no arrived hint | yes / yes / **no** |
| B164 | value > 0 in 0..2 / value <= 0 / value > 2 (`0.f > value` is dead behind `> 0`) | yes / **no** / **no** |
| B164 | turn to player / turn to hint actor / hint uid not an actor | yes / yes / no (hints are actors) |
| B6C4 | null cast / inactive / not 0x11 / outside radius / not nearer / BA38 true / kept | no / **no** / **no** / yes / yes / **no** / yes |
| BA38 | no hint / inactive 0x11 next (true) / 15 steps (false) | **no** / **no** / yes |
| B458 | no hint / inactive skipped / active found / none | **no** / **no** / yes / **no** |
| A06C | ball actor / roll emitter | **no** (0xE38 is set by BoostBallRoll/Fire paths) / yes |
| 9920 | invalid uid / ball | yes / **no** |

- None marked. `send <hint> Deactivate` on a chain hint would force the inactive arms of B6C4, BA38
  and B458 (forced (send)); the value arms need a hint with value <= 0 or > 2 (none on the rooftop;
  Temple and 0j_cliff_hall_loop not surveyed).
- Not started: SweepBeam CA0C (needs C2B0 beam aim + C470 ray/decal/collision response, C104 with
  BE18 near-list damage, C798 the 0xD78 effect release, and the 0xD74 sub-object), 1B658, 138A4,
  11FA4's two open arms.

## Slice 17 (2026-10-21): the forced boost hints, SweepBeam

- Forced (send) on the rooftop (`tools/drivers/darksamus_boost_inactive.con`): at f700 the boss
  stands in hint u01b2's radius only, whose kSS_Arrived link is u0140. `send u0140 Deactivate` ran
  B6C4's inactive arm (u0140 is linked too), BA38's inactive answer (true at step 0) and B6C4's
  BA38-true skip of u01b2; B164 then took u017f -> u0197. Still unmarked:
  - B6C4: the null-cast and not-0x11 arms need a non-hint or non-0x11 link (data; no delete command).
  - BA38: its no-hint arm is dead from its only caller (B6C4 passes a hint's uid).
  - B458: its no-hint arm is reachable only from BoostBallRoll A3B0 (0x0000AC08, a stand-in); its
    skip-inactive arm needs an inactive non-0x11 link ahead of the first type-0x11 Arrived link,
    because BA38 already drops a start whose first arrived hint is inactive.
  - A06C / 9920: 0xE38 is written only by fn_10_9AC4 (clear) and fn_10_9E00 (set); 9E00's callers
    are BoostBallRoll A3B0 and DoUserAnimEvent 1BEFC, both stand-ins.
- Ported: SweepBeam CA0C, the beam start C85C and release C798, the aim C2B0, the end point C470
  (static ray, collision response, decal), the swept rays C104 and the damage ray BE18, the 0xD74
  sub-object's members (fn_10_8DB4: 0xD74 PART, 0xD78 effect, 0xD80 damage, 0xD9C/0xDA8/0xDB4
  directions and end, 0xDC0 length, 0xDC4 saved speed, 0xDC8 bits 0x80/0x40, 0xDCC CRSC, 0xDD8..0xDE4
  sounds), and DoUserAnimEvent's kUE_BeginAction / kUE_EndAction sweep arms (0x10000 starts / stops
  the beam; both cases return without the base, as retail does). [probable] fn_10_8DB4's arguments
  follow the data order (sweepSwoosh, sweepBeamDamage, sweepBeamCollisionEffect, sweepBeamSound,
  unknown_0x0ef8dc15).
- Host guard: Agon's boss leaves sweepSwoosh as kInvalidAssetId and a forced SweepBeam there crashed
  in CElementGen's constructor; StartSweepBeam makes no beam for an invalid PART.
- Reach: `fsm <id> SweepBeam` (`tools/drivers/darksamus_sweep.con`, rooftop u04f7, PART a4381217).
  BeginAction does not fire on every pass (it fired on all three rooftop passes, on one of three on Agon).
  Neither world sets sweepBeamSound or unknown_0x0ef8dc15 (both 0xFFFF), so no emitter is made.

| body | arm | ran |
| --- | --- | --- |
| CA0C | Activate / Update / Deactivate | yes / yes / yes |
| CA0C | Update: beam on / off; beam sound / none | yes / yes; **no** (data) / yes |
| CA0C | Deactivate: each sound set / not set | **no** (data) / yes |
| C85C | release, build, emission on, bit 0x40 | yes |
| C798 | old effect deleted / none | yes / yes |
| C2B0 | effect / none; normalized / not; end sound / none | yes / yes; yes / **no**; **no** (data) / yes |
| C470 | normalized / not; hit, beam on / hit, off / miss; decal / none | yes / **no**; yes / yes / yes; **no** / yes |
| C104 | no PART (0) / previous zero / previous; one ray / subdivided | **no** (data) / yes / yes; yes / yes |
| BE18 | player first / player again / other / none | yes / yes / yes / yes |

- Marked: C85C, C798, BE18. CA0C and C2B0 wait on a world with the sweep sounds; C470 waits on a
  decal-bearing CRSC (rooftop's has no type-1 decal) and a vertical gun; C104 waits on a 0 PART.

## Slice 18 (2026-10-21): the projectile reflect, the dive hit

- Ported: the reflect fn_10_1B658 (near list, root-bone range 0xC00, ricochet sound 0xC50, the
  randomized send-back through Explode with response 0xD and ReflectVulnerabilty, the five-uid list
  0xBEC/0xBF0, 0xC08), its predicate fn_10_1B9B0, the dive hit fn_10_138A4 and its callees 1382C
  (orbit target cleared), 13750 (flat push) and 1367C (impulse, kMS_ApplyJump, camera-shake rumble).
  New host members: mRootPos (0x8FC, PreRender's "Skeleton_Root" translation, lbl_10_rodata_560 +
  0x551), mReflectedUids, mLastReflectTime (-1000), mRicochetSfxId (missileRicochetSound, loader
  0x928); the dive record's impulses +0x50/+0x54 (0xB10 horizontal = loader 0x964, 0xB14 vertical
  = 0x960).
- Reach: `tools/drivers/darksamus_reflect.con` (rooftop u04f7: shots every 15 frames through two
  dives, then `press X` and a morphed dive). DiveAttack calls the reflect only while the stage's
  unknown_0xf5cf3c0f is set; Think's call (0x0001B384) waits on the Think port.

| body | arm | ran |
| --- | --- | --- |
| 1B658 | predicate false / too far / reflected | yes / yes / yes |
| 1B658 | normalized / not normalizable; erase oldest / not | yes / dead; yes / yes |
| 1B9B0 | null-own-exploded / in list / any type; attack 0x800000; typed roll | yes / yes / yes; **no**; **no** (the typed caller is outside the port, Slice 20) |
| 138A4 | unmorphed push / morphed | yes / yes |
| 1382C, 13750, 1367C | single path | yes |
| 11FA4 | jump re-issue | forced (fsm MeleeAttack, then DiveAttack) |
| 11FA4 | static wall hit | yes (Slice 19: forced, fsm DiveFromBoost then place into the wall) |

- Marked: 1B658 (its not-normalizable arm is dead: a unit forward plus an offset of at most
  |(0.4, 0.4, 0.55)|, squared 0.6225 < 1, is never zero), 138A4, 1382C, 13750, 1367C.
- Unmarked: 1B9B0. Its typed arm needs fn_10_F298 and the typed callers in AnyIncomingMissiles
  1BB08, and the 0x800000 arm needs that attack; the only port caller passes -1.
  11FA4 is unmarked too. Its static wall hit needs the dive record's boost bit 0x01 (0xB20), which
  only DiveFromBoost 9424 sets (0x00009460), and 9424 is a stand-in. Without the bit, the collision
  failsafe (0x00012018) runs first and returns a boss placed into a wall to its safe position
  (observed: placed at the wall (68.59,466.5) in flight, it flew on from 451).
- The forced SweepBeam's kUE_BeginAction is missing on Agon because of animation data, not a port
  bug. On Agon, MeleeAttack severity 5 plays an animation whose events are kUE_Projectile,
  EffectOn 33, DamageOn 8, EffectOff 34 and LoopedSoundStop 2, with no BeginAction 17 or
  EndAction 18 (none in 5 of 5 passes). The rooftop's sweep animation sends 17/18 on every pass.
  An entry made while the body is still in kAS_MeleeAttack (Agon pass 1, from MissileJump) gets
  Repeat from CanIssueCommand, so the earlier attack's animation keeps playing with that
  animation's events. Agon's sweepSwoosh is also kInvalidAssetId, so that fight has no sweep beam
  at all.

## Slice 19 (2026-10-21): the dive out of the boost ball

- Ported: DiveFromBoost fn_10_9424 with fn_10_9644 (the turn around) and fn_10_9AC4 (the boost
  ball's deletion). Only Activate runs the body (`mr. r31, r5` at 0x00009444, `bne .L_0000954C`
  at 0x00009454): the dive record's boost bit 0x01 (the byte 0xB20, `rlwimi r0, r3, 0, 31, 31`
  at 0x00009460), the roll emitter removed (0x00009474) and its handle cleared (0x00009480),
  fn_10_9644 (0x00009484), 0xD24 = lbl_10_rodata_284 = 0.5f (0x000094A8), 0xEBC bit 0x40
  (0x000094B0), fn_10_9AC4 (0x0000953C) and fn_10_99B8 (0x00009548). Deactivate (`cmpwi r31,
  0x2` at 0x0000956C) clears the boost bit again (0x0000957C).
- The state's base call is the vtable's word 136 (`lwz r12, 0x220(r12)` at 0x00009560): the REL's
  vtable lbl_10_data_CBC holds the actor's vtable (words 0..81) and the 92 handlers (words
  82..173), the 49 triggers take 82..130 and the 43 states 131..173 - so word 136 is the sixth
  state, DiveAttack (fn_10_11FA4). The boost dive *is* the dive.
- Reach: `fsm <id> DiveFromBoost`. The FSM2 has no transition into the node (no state lists the
  DiveAfterBoost trigger, fn_10_95A8), so the console is the only way in. The dive launches
  167 to 200 ticks after the state change (the jump's XLAU event) and the flight lasts about
  20 ticks (the dive lands on the player, who stands close), so the placement has to be
  repeated every 10 ticks to land in the flight.
- The static wall hit (0x00012534) ran: the boost bit skips the collision failsafe (the
  `clrlwi.` at 0x00012018), so an actor placed inside a wall stays there and the static
  collision test (kMT_Wall only, `li r5, 0x1` at 0x000124E8) finds the wall and the dive
  bounces off it (fn_10_1CDD0 with the zero normal, 0x0001254C). Without the bit the boss
  placed in the wall was returned to its safe position (Slice 18). The wall is at
  (68.59, 466.5, 164.3); the driver is `tools/drivers/darksamus_boost_dive.con` (rooftop u04f7,
  the placement every 10 ticks from 160 ticks after the state change). The boss ends the run
  against the wall instead of at the player.

| body | arm | ran |
| --- | --- | --- |
| 9424 | the Activate body / the other messages | yes / yes |
| 9424 | Deactivate clears the boost bit | yes |
| 9644 | the turn around | yes (the "cannot be normalized" arm is dead: the transform's Y column is a unit vector) |
| 9AC4 | no boost ball / a boost ball to delete | yes / **no** (the uid 0xE38 is set by the BoostBallRoll (fn_10_A3B0) and BoostBallFire (fn_10_9724) paths, which are outside the port) |
| 11FA4 | static wall hit | forced (fsm DiveFromBoost, then place into the wall) |

- Marked: 11FA4 (every arm ran: the jump re-issue was forced in Slice 18, the static wall hit
  here), 9644.
- Unmarked: 9424 - fn_10_99B8, which stops the boost ball's particle effects, is outside the
  port: the two swooshes (guest 0xE3C / 0xE4C) and the element generator (guest 0xE60 / 0xE64)
  are never built (fn_10_9B24 has no host counterpart and the loader's swoosh tokens are not in
  the host), so its "stop the old effect" calls (0x000099F4, 0x00009A20, 0x00009A68) and
  fn_10_9424's (0x000094EC) have nothing to stop. 9AC4 is unmarked for the arm above.

## Slice 20 (2026-10-21): the reflect chance cache, fn_10_F298

- **The table at guest 0x10A8 is a run-time cache, not loader data.** (Slice 21 adds the two
  callers of the roll functions: fn_10_F100 from the hit arm and fn_10_F298 from the
  AnyIncomingMissiles trigger.) Every write to the count
  (0x10A8) and the entries (0x10AC, 12 bytes each: the projectile's uid, a roll at +0x04 and a
  roll at +0x08) in the REL:
  - the constructor writes the count 0 (`stw r5, 0x10a8(r31)`, 0x00020270);
  - fn_10_F100 (0x0000F100) writes an entry and the count + 1 (0x0001F1CC, 0x0001F1E4) - it is
    called from fn_10_1EE4C (0x0001EED8), the arm of AcceptScriptMsg that handles a hit;
  - fn_10_F298 (0x0000F298) writes an entry and the count + 1 (0x0001F3C8, 0x0001F3E0) - it is
    called from fn_10_1B9B0 (0x0001BACC);
  - fn_10_F210 (0x0000F210) erases the first entry (the shift at 0x0001F240..0x0001F254) and the
    count - 1 (0x0001F27C).
  No store to 0x10A8..0x1124 exists in the loader, in AcceptScriptMsg or in any other helper: the
  count is 0 after the constructor and the entries only ever come from those two roll functions.
  So the typed arms of fn_10_1B9B0 are alive in retail, but the port has no way in: the only
  caller in the port is fn_10_1B658, which passes -1 (0x0001B748), and the typed caller
  (0x0001BBF8, AnyIncomingMissiles fn_10_1BB08) is outside the port.
- Ported: fn_10_F298 (the roll for a projectile's uid) and fn_10_F210 (the cache's erase), the
  cache itself (the count and ten entries) and the two floats fn_10_F298 compares - guest 0xC0C
  (the elapsed time of the last hit, written by AcceptScriptMsg at 0x0001EAF8) and 0xC20 (the
  elapsed time of the last heavy hit, the HeavyHit state's Deactivate at 0x00010F4C); the
  constructor writes them lbl_10_rodata_3EC = -1.0f and lbl_10_rodata_410 = -1000.0f
  (0x0001FEE4, 0x0001FF04). fn_10_1B9B0 now compares fn_10_F298's roll with the stage info's
  +0x24 (the missiles' first float) as the asm does (`fcmpo` at 0x0001BAD4).
- The roll: the cache is walked (0x0001F2E4..0x0001F2FC) and the entry's +0x04 answers when the
  uid is in it (0x0001F2F0). Otherwise, with ten entries (0x0001F300) the first is erased
  (0x0001F310) and two rolls are made (Range(0.0f, lbl_10_rodata_3F0 = 100.0f), 0x0001F32C and
  0x0001F348); the time since the last hit or heavy hit (the larger of 0xC0C and 0xC20) shifts
  them - zero for the first four seconds (lbl_10_rodata_408 = 4.0f, 0x0001F378) and -50.0f
  (lbl_10_rodata_418 = 50.0f, 0x0001F3B0) for the next four (lbl_10_rodata_414 = 8.0f,
  0x0001F39C). The uid and the two rolls go into the next entry (0x0001F3C8..0x0001F3D4).

| body | arm | ran |
| --- | --- | --- |
| F298 | the uid in the cache / not in it | yes / yes |
| F298 | the cache with ten entries (the erase) / fewer | yes / yes |
| F298 | the rolls zeroed (4 s) / -50 (8 s) / unchanged | yes / yes / yes |
| F210 | the first entry erased | yes |

- The arms ran through a temporary probe (a call to fn_10_F298 in the Think, gone from the tree):
  1080 rolls on the rooftop (10_cliff_rooftop, boss u04f7), the count 1..10 and then 10 for
  every roll after the eleventh (the erase), 87 rolls that repeated an uid and its roll (the
  cache hit), the rolls 0.0 for the elapsed time 0.02..2.98, in [-49.4, 48.8] for 3.02..6.98 and
  in [0.1, 100.0] from 7.02 - the three time windows.
- Marked: F298, F210.
- Unmarked: 1B9B0 - its typed arm needs the typed caller (outside the port) and the 0x800000 arm
  needs that attack, whose phazon and cannon-ball projectiles the fight does not have.
- fn_10_F100 (the other writer, the one the hit arm reaches) is outside the port with that arm,
  so it is not ported; the cache's +0x08 field is written by fn_10_F298 as the asm does.
- The review's XSTORE hits in fn_10_193A0 (mUseAltScan and mCanDie) were a documentation slip:
  the header's comments gave the bit masks (0x80, 0x40) where the tool reads the guest's byte
  offset, so it looked for a store to 0x80. The asm stores the byte 0x97C (0x00019470 and
  0x0001948C) with the bits 0x80 (the stage's +0x34) and 0x40 (the stage's +0x32 not 1) set by
  the rlwimi at 0x0001946C and 0x00019488 - the comments now name the byte 0x97C first.

## Slice 21 (2026-10-21): the reflect path's callers

- Ported: fn_10_1BB08 (the AnyIncomingMissiles trigger), fn_10_F100 (the cache's other roll) and
  fn_10_1EE4C (the hit's damage may be sent back), plus the 'XDMG' arm of AcceptScriptMsg
  (fn_10_1E89C) that calls it, and the two floats fn_10_F298 and fn_10_1EE4C compare
  (guest 0xC0C and 0xC20) where the retail writes them.
- **fn_10_1BB08** (0x0001BB08, 0x100 bytes): the near list (the box of half-size
  lbl_10_rodata_3F0 = 100.0f around the actor, 0x0001BB30..0x0001BB7C) with the default filter
  and the actor itself (BuildNearList at 0x0001BBB8); every entry is cast to a
  CEnergyProjectile (0x0001BBDC) and fn_10_1B9B0 with the weapon type 6 (kWT_Missile,
  0x0001BBF4) must let it through (0x0001BBFC) and fn_10_145E4 must find the line to the
  projectile's position on the mesh (0x0001BC10) for the trigger to answer true (0x0001BC20).
  The trigger itself has no transition in the FSM2 (the MP2_DS_FSM dump lists it only in the
  machine's 92 triggers), but the module's own ShouldReact (fn_10_11158) calls it (0x00011210)
  and the FSM2 uses ShouldReact, so the body does run in retail.
- **fn_10_F100** (0x0000F100, 0x110 bytes): the cache's other roll - the same walk and the same
  two rolls as fn_10_F298 but without the time window, and the entry's +0x08 answers on a hit
  (0x0000F158). It is called only from fn_10_1EE4C (0x0001EED8).
- **fn_10_1EE4C** (0x0001EE4C, 0xE4 bytes): the time since the last hit or heavy hit (the
  larger of guest 0xC0C and 0xC20, 0x0001EE74 and 0x0001EE78) plus lbl_10_rodata_438 = 3.0f
  must be past the elapsed time 0x8B0 (0x0001EE90), the damage's weapon type must be 6
  (kWT_Missile, 0x0001EEA8) and the stage must exist (0x0001EEB8); then fn_10_F100's roll is
  compared with the stage info's +0x2C (the third missile float) when the damage info's flag
  bit 0x40 is set (0x0001EEDC) and with its +0x28 (the second missile float, 0x0001EF0C)
  otherwise.
- **The 'XDMG' arm of AcceptScriptMsg** (0x0001EA30): the sender must be a CGameProjectile
  (0x0001EA40), the vulnerability (the vtable's slot 16, fn_10_100DC, 0x0001EA5C) must be hurt by
  the projectile's damage info (fn_800DB834, the host's WeaponHurts, 0x0001EA64) and the last hit
  must be lbl_10_rodata_438 = 3.0f old (0x0001EAB4). The hit direction (the projectile's
  position minus the actor's) goes to 0x8C0/0x8C4/0x8C8 (0x0001EA70..0x0001EAA4) and, when
  fn_10_1EE4C says yes, 0xC0C takes the elapsed time (0x0001EAF8) and 0xC10/0xC14/0xC18 the
  flattened direction (0x0001EB04..0x0001EB18), normalized (0x0001EB30).
- **The writes of 0xC0C and 0xC20**: every store in the REL is 0x0001EAF8 (the 'XDMG' arm,
  0xC0C = the elapsed time), 0x00010F44 and 0x00010F4C (the HeavyHit state's Deactivate:
  0xC0C = 0.0f and 0xC20 = the elapsed time) and the constructor (0x0001FEE4 and 0x0001FF04).
  The port writes all four.
- Reach: the missile is the console's Y button (the tweak's default useItem; 0x2c is the missile
  ammo): `give missile 30`, `press Y 45` charges and fires it at the boss. On the rooftop
  (10_cliff_rooftop, boss u04f7) 16 of 34 missiles hit her; the same in the Temple
  (12_temple_finalgate_dark, boss u0564).

| body | arm | ran |
| --- | --- | --- |
| F100 | the uid not in the cache | yes (16 missiles) |
| F100 | the cache with ten entries (the erase) / fewer | yes (the count stops at 10) / yes |
| F100 | the uid in the cache | **no** - dead: the key is the projectile's uid and a projectile hits once |
| 1EE4C | the time window passed / too early | yes / **no** (the stage's chance is below every roll) |
| 1EE4C | a missile / not a missile | yes / **no** |
| 1EE4C | a stage / none | yes / **no** (the fight always has one) |
| 1EE4C | the stage's +0x2C / its +0x28 | **no** / yes |
| 1BB08 | a missile in the near list | **no** - the trigger never ran |

- The typed arm of fn_10_1B9B0 (the weapon type 6, 0x0001BBF8) needs fn_10_1BB08, and that
  needs the module's own ShouldReact (fn_10_11158), which asks CanMissileDeflect first: the
  stage info's +0x24 (the missiles' first float) must be below zero, and in the rooftop, Agon
  and Temple stages it is not (the trigger answers false, so the short-circuit skips
  AnyIncomingMissiles). FacingPlayer answers false there too. So the typed arm is out of reach
  in the fights that were run, and the trigger's body did not run.
- Marked: F100.
- Unmarked: 1BB08 (its body never ran) and 1EE4C (three of its arms did not run).

## 8. Function classification (all 535 functions)

`docs/module-analysis/DarkSamus_functions.tsv` has one row per function of
`config/G2ME01/rels/DarkSamus/symbols.txt` (offset, size, class, kind, name, the number of
REL callers, whether the port has a `// @ fn_10_` marker, and the FSM state or trigger it
belongs to). The classes follow the Sandworm note's rule set (section g there), so the table can
be audited:

- **H - host copy** (80 functions, 0x1CDC bytes): the body is a copy of something the host
  already has, so the port writes no body for it. The crtexe runtime (7 symbols: the prolog,
  the epilog, the unresolved stub, the global chain, the module constructors and destructors),
  the compiler-emitted destructors of the loader's token members (the bodies that call
  `Free__7CMemoryFPCv` and `__dt__6CTokenFv`), the DOL class members the REL copies (the
  weapon, the collision actor manager, the cube renderer, the particle system) and the REL's
  copies of the base class's own vtable entries (the ones the host's base class already has).
- **T - data / vtable thunk** (50 functions, 0x3004 bytes): the body only returns a constant, a
  member address or a flag - the port writes it as one line in the vtable (the vtable thunks the
  port overrides, the trivial FSM handlers whose body is a single flag) and the functions
  nothing calls.
- **B - module behaviour** (405 functions, 0x1E418 bytes): real behaviour - these are the port
  bodies. 109 of them carry a `// @ fn_10_` marker (slice 25 dropped the three Break* markers whose
  writer is still a stand-in).

| class | functions | .text bytes |
|---|---|---|
| H host copy | 80 | 0x1CDC (7,420) |
| T data / vtable thunk | 50 | 0x3004 (12,292) |
| B module behaviour | 405 | 0x1E418 (123,672) |
| **total** | **535** | **0x254F8 (153,384)** |

The B list (293 unmarked) is in `DarkSamus_functions.tsv` with the `state` column, so the port
order follows the FSM: the 62 unmarked handlers first (by the state's name), then the helpers.

## Slice 22 (2026-10-21): the classification and the triggers

- **The classification** (the section above and `DarkSamus_functions.tsv`): the 535 functions
  split into 78 H, 52 T and 405 B. The rules are mechanical: the crtexe runtime and the bodies
  that only call the host's own code (the token destructors, the DOL class copies) are H; a body
  with no call, no branch and at most six instructions is a thunk (T); everything else is B.
- **Ported: eight triggers and the Alerted one.** fn_10_10634 (CanMeleeAttack: fn_10_17D50 with
  the attack 1), fn_10_142CC (FacingPlayer: fn_10_142F4 with lbl_10_rodata_4C4 = 0.17453292f),
  fn_10_17CB8 (OutOfMeleeRange: the distance to the player against lbl_10_rodata_460 = 30.0f),
  fn_10_112E8 (PlayerIsMorphball: fn_10_11308), fn_10_10658 (PlayerTooClose: the distance
  against the actor's own minimum attack range 0x3DC), fn_10_6B78 (ShouldEnragedMelee:
  fn_10_1AA6C, the range band 0: `cntlzw; srwi 5` is "== 0"), fn_10_BB78 (ShouldFadeIn: no fight script, or the stage info's
  +0x31 is 0) and fn_10_112AC (WasHeavyHit: the last hit's elapsed time 0xC0C is not 0.0f and
  lbl_10_rodata_46C = 0.3f plus it is past the actor's elapsed time 0x8B0), plus fn_10_F840
  (Alerted: the flag byte 0x90C's bit 0x80).
- **The 'ALRT' arm of AcceptScriptMsg** (0x0001EA1C): the alerted flag - the byte 0x90C's bit 7
  (the rlwimi at 0x0001EA24 keeps the mask 0x80) - so the trigger fn_10_F840 (Alerted) can
  answer true; the console's `send <id> Alert` delivers the message.
- Marked: fn_10_10658 (PlayerTooClose) - both outcomes ran (the boss far away and next to the
  player) - fn_10_17CEC (GetDistanceToPlayer), which has no branch and runs in every fight, and
  fn_10_F840 (Alerted): the FSM2 evaluates the trigger in its TravelToCoverPoints node (the
  MP2_DS_FSM dump) and the run put the boss there, before and after the 'Alert' message.
- The other seven are ported but unmarked: their arms did not all run. The FSM2 evaluates a
  state's transitions in order and stops at the first that fires, and the module's own
  ShouldReact asks CanMissileDeflect first, so fn_10_142CC (FacingPlayer) is never reached in
  the shipped stages (the stage info's +0x24 is not below zero); fn_10_112AC's true arm needs
  0xC0C, which the 'XDMG' arm writes only when the reflect chance passes (the stage's chance
  is below every roll that came up); fn_10_10634, fn_10_17CB8 and fn_10_112E8 have no FSM2
  transition at all; fn_10_6B78 belongs to the EnragedTravel state, which the phazon enrage
  never reaches; fn_10_BB78 is behind the earlier transitions of TravelToCoverPoints.
- The run: Sanctuary 10_cliff_rooftop, boss u04f7, 3200 frames - the boss next to the player
  (`place u04f7 player 3`), the missiles at it (the Y button, `press Y 45`), the 'Alert' message
  (`send u04f7 Alert`) and the states that use the triggers forced one after the other
  (`fsm u04f7 Taunt` and so on). 0 caught, 0 reach-stub, exit 0; the MP2_DS_FSM log shows the
  boss in TravelToCoverPoints and in Taunt, so both nodes' transition lists were evaluated.

## Slice 23 (2026-10-21): the marking rate - the ported-but-unmarked bodies

- The ported bodies that had no marker are now marked: the attack pick's tree and the helpers it
  calls. They all ran in every fight (the attack pick runs in the Think, 0x0001B314, and the
  slices 7 and 8 record the picks: 0x8 the scatter shot, then 0x80, 0x10, 0x40 and 0x200, with
  the mask 0x6D8 and the stage's weights), so their arms are covered by the recorded runs:
  - the collectors and the setters: fn_10_1A5BC (CollectStageWeights), 1A554 (the fade
    attacks), 1A278 (the morph ball attacks), 1A440 (the range attacks), 1A34C (the history
    penalties), 1A1D4 (the facing attacks), 19A1C (the line attacks), 1905C
    (ClassifyLineToPlayer), 14648 (CanWalkToPlayer), 1A980 (SetAttackBits), 1A8F8
    (SetAttackBitsExcept), 1AA30 (SetAttackBit), 1AA5C (IsAttackTwo), 10BBC
    (GetAttackBitIndex), 1A6C4 (GetAttackWeight), 17D50 (IsAttackEnabled), 11308
    (IsPlayerMorphed), 142F4 (IsPlayerInFront), 1AA6C (GetPlayerRangeBand);
  - the pick itself: fn_10_1954C (UpdateAttackPick), 197D8 (PickVisibleAttack), 1969C
    (PickFadedAttack), 19918 (PickWeightedAttack), 1B478 / 1B5E4 (UpdateCurrentAttack);
  - the dive and the reach: fn_10_12B38 (the dive target), 1285C / 12764 (the jump command),
    16418 / 164C0 (the aim line), 166AC / 16600 (the ring path), 16B1C (OnPath);
  - the stage: fn_10_17E14 / 17DE0 (GetBattleStage).
- The 'ALRT' arm of AcceptScriptMsg and the Alerted trigger (fn_10_F840) are in, with the asm
  offset on the mask: `mFlags90C & 0x80` (0x0000F844 `extrwi r3, r0, 1, 24` - the PowerPC bit 24
  is 1u << (31 - 24) = 0x80).
- Still unmarked on purpose: fn_10_19B64 (the 145E4-false arm after both OnPath tests pass
  never ran), 150DC / 14FE0 (the dash's stuck arm), B86C (the hint chain's arms), 153EC (the
  FSM2 has no SideDash node), 100DC / FFB4 (the enrage arm needs the phazon states), 12960 /
  11C60 (the non-normalizable lines), 1CDD0 (the bounce with no normal), 11EB8 (the stuck
  bounce), 1C99C (the WeaponHits arm), 8764 (the retry arm), 9424 / 9AC4 (the boost ball's
  particles), 1B9B0 / 1BB08 / 1EE4C (the reflect path) and F40C / F488 (the scan info and the
  damage arms).
- The classification changed with the markers: the REL's copies of the base class's own vtable
  entries are H (the host's base class has them), so the counts are 80 H, 50 T and 405 B.
- The run: Sanctuary 10_cliff_rooftop, boss u04f7, 2200 frames - the boss next to the player,
  the missiles at it, the 'Alert' message, and the dive, the boost ball and the taunt states
  forced one after the other. 0 caught, 0 reach-stub, exit 0.

## Slice 24 (2026-10-21): the marking rule - the gdb re-check, the Break* triggers

- The 28 bodies of slice 23 were re-checked under gdb (no probes in the source): the game ran
  under gdb with a breakpoint on every body, each hit printing its arguments and its result
  (`$al` for the booleans, `$eax` for the integers) and the program continuing. The driver
  (`/tmp` only) sent the boss after the player and far away, morphed the player
  (`fastmorph 1`, `press X 8`), fired the missiles and forced the states.
- **Dropped (their live arms did not all show up):**
  - fn_10_16418 / 164C0 (IsPlayerAimLineClear): the body never ran - its only caller is the
    reach collector fn_10_19B64, which is still a stand-in.
  - fn_10_1905C (ClassifyLineToPlayer): only the class 0 was seen.
  - fn_10_16B1C (IsOnPath): only the "on the path" answer was seen.
  - fn_10_1A554 (CollectFadeAttacks): the faded arm needs the fade state, which the fight never
    reaches.
- **Kept (both arms seen in the run):** fn_10_11308 (the player morphed and not), 142F4 (the
  player in front and not), 17D50 (the attack on and off), 1AA6C (the three range bands), 14648
  (walkable and not), 1AA5C (the bit 2 and the others), 10BBC (the bit indices; the "no bit" arm
  is dead by arithmetic), 17E14 / 17DE0 (a stage and none), 166AC / 16600 (clear and blocked),
  17CEC (the distances), 1A6C4 (the 23 attack weights), 1A980 / 1A8F8 / 1AA30 (the set flag
  and the bits), 1A5BC / 1A278 / 1A440 / 1A1D4 / 19A1C / 1A34C (the collectors), 1954C /
  197D8 / 1969C / 19918 (the picks), 1B478 / 1B5E4 (the attack ids and the messages), 12B38
  (the dive target), 1285C / 12764 (the jump command).
- **New: the four Break* triggers**, with the bytes the retail reads as new members (the port has
  no writer for them yet): fn_10_16B48 (BreakFreezeBeam: the byte 0xAB8's bit 0x80),
  fn_10_16B54 (BreakSuperMissile: the byte 0xA80), fn_10_16B5C (BreakNormalMissile: the byte
  0xA40) and fn_10_106F8 (BreakPause: the float 0xC30 against the elapsed time 0x8B0). Their
  arms ran with the bytes forced under gdb (`set var this->mGuestAB8 = 0x80` and so on,
  alternating), the states that use the triggers forced with `fsm`; the arms are labelled
  forced in the table below.
- The DarkSamus module is split: the other lane (branch decomp/darksamus-helpers) takes the
  unmarked class-B rows with an empty `state` column and an offset below 0xA000; this note and
  `DarkSamus_functions.tsv` stay here.

| body | arm | ran |
| --- | --- | --- |
| 16B48 | the byte 0xAB8's bit 0x80 set / clear | forced (gdb `set var`), yes |
| 16B54 | the byte 0xA80 set / clear | forced (gdb `set var`), yes |
| 16B5C | the byte 0xA40 set / clear | forced (gdb `set var`), yes |
| 106F8 | 0xC30 below the elapsed time / not | forced (gdb `set var`), yes |

## Slice 25 (2026-10-21): the reflect record bytes - their writers, and the Break* triggers

- **Every writer of the guest's 0xA40, 0xA80, 0xAB8 and 0xC30 in the REL** (the rel_pseudo
  dumps; the stores and the bit writes):
  | byte | writer | what it writes |
  |---|---|---|
  | 0xC30 | fn_10_10710 (the Pause state) | 0x00010768: the elapsed time 0x8B0; 0x000107AC: that plus the stage's pause duration (+0x18..+0x1C, the random range at 0x000107A0) - the time the pause ends |
  | 0xA40 | fn_10_17448 (the NormalMissileAttack state) | 0x000174A0: 0 (with 0xA38 and 0xA3C, 0x00017498/0x0001749C) on Activate |
  | 0xA40 | fn_10_1BEFC (the anim-event handler) | 0x0001C30C |
  | 0xA80 | fn_10_16ECC (the SuperMissileAttack state) | 0x00016F54: 0 (with 0xA78 and 0xA7C, 0x00016F50/0x00016F4C) on Activate |
  | 0xA80 | fn_10_1BEFC (the anim-event handler) | 0x0001C31C |
  | 0xAB8 | fn_10_16CDC (the FreezeBeam state) | 0x00016D70 and 0x00016D80: the bit 0x40 cleared and the bit 0x80 set to the old bit 0x40 (0x00016D6C and 0x00016D7C) |
  | 0xAB8 | fn_10_1BEFC (the anim-event handler) | 0x0001C250 and 0x0001C298 (the bit 0x80) |
- Only the Pause state's write is ported (it is the one whose value the trigger reads); the
  NormalMissileAttack's and the SuperMissileAttack's Activate writes still sit in stand-in
  states (they only clear bytes that are 0 anyway), and the anim-event handler (fn_10_1BEFC)
  is still a stand-in, so the bytes it alone sets stay 0.
- **The triggers:** fn_10_16B48 (BreakFreezeBeam), fn_10_16B54 (BreakSuperMissile) and
  fn_10_16B5C (BreakNormalMissile) read the bytes the anim-event handler writes, so they go
  back to their FsmStandIn bodies (unmarked); their members (mGuestA40, mGuestA80, mGuestAB8) stay
  for the writers. fn_10_106F8 (BreakPause) reads 0xC30, which the Pause state writes, so it
  stays ported: with the Pause state's write ported the trigger is false while the pause runs and
  true when it ends.
- The arms ran with natural writes (no gdb forcing): tools/cov with
  `tools/cov/specs/darksamus.py` and `tools/cov/specs/darksamus_pause.con` (the rooftop, the
  boss, `fsm u04f7 Pause`) - `106F8 BreakPause: 101 hits, key 0 -> ret 0 (100), key 1 -> ret 1
  (1)`; `10710 Pause: 211 hits, -> CRandom16::Range 1` (the pause's duration drawn once).
- The Pause state's own body is still a stand-in apart from its 0xC30 write, so fn_10_10710 keeps
  no marker; the module's markers went from 112 to 109 (the three Break* markers were dropped).
- The retail's null check is kept: fn_10_17E14's result is tested at 0x00010770 (cmplwi r3, 0) and
  the branch at 0x00010774 skips the rest, so with no stage the 0xC30 write stays at the elapsed
  time. The random range is the state manager's own CRandom16 (r29+0x16E4, the Range call at
  0x000107A0), the stage's +0x18 and +0x1C are the loader's pauseDurationMin and pauseDurationMax
  (the SLdrDSStageInfo layout: +0x00..+0x20 are the nine floats).

## Slice 26 (2026-10-21): the FSM handlers in the tsv's order

Ten handlers ported from the REL's asm, each with the offsets of its branches and its calls.

| fn | the REL's body | the port | marker |
|---|---|---|---|
| fn_10_10710 | 0x00010710, 0x154 bytes | the Pause state | **marked** |
| fn_10_62B4 | 0x000062B4, 0xDC bytes | the Arrived trigger | not marked (the fight never evaluates it) |
| fn_10_9724 | 0x00009724, 0xDC bytes | the BoostBallFire state | not marked (fn_10_99B8 is still a stand-in) |
| fn_10_9314 | 0x00009314, 0xDC bytes | the DoneRolling trigger | **marked** |
| fn_10_7A14 | 0x00007A14, 0xDC bytes | the EnrageHitReact state | not marked (fn_10_7F98) |
| fn_10_6E38 | 0x00006E38, 0xDC bytes | the EnragedTravel state | not marked (fn_10_6DAC) |
| fn_10_BD40 | 0x0000BD40, 0xDC bytes | the FadeIn state | **marked** |
| fn_10_BC54 | 0x0000BC54, 0xDC bytes | the FadeOut state | **marked** |
| fn_10_F86C | 0x0000F86C, 0xDC bytes | the FallFromTheAir state | **marked** |
| fn_10_16CDC | 0x00016CDC, 0x1F0 bytes | the FreezeBeam state | not marked (fn_10_1707C, fn_10_8870) |

The five unmarked ones call a helper the other lanes port: fn_10_99B8 (the boost ball's
particle effects), fn_10_7F98 (the enrage helper), fn_10_6DAC, fn_10_1707C (the dodge step) and
fn_10_8870 (the freeze beam's command). Each of those calls is a `FsmStandIn` in the port, so
the bodies are the states' own work but not the whole function.

### The bodies

- **fn_10_10710 (the Pause state).** The attack bookkeeping with id 0x400 (0x00010750).
  Activate (0x00010754): 0xC30 takes the elapsed time 0x8B0 (0x00010768) and the stage's pause
  duration (the stage info's +0x18 and +0x1C, the random range at 0x000107A0, the sum at
  0x000107AC). Deactivate (0x000107B4): 0xC24 = kInvalidUniqueId (0x000107C4). Then the damage
  (fn_10_1E178 at 0x000107D4), the turn toward the player (fn_10_16B64 at 0x000107F0, the rate
  lbl_10_rodata_44C = 1.3962634f) and the locomotion command (fn_800EE3C4 at 0x00010838, the
  CBCLocomotionCmd with the zero move and face vectors and the weight 1.0f).
- **fn_10_62B4 (the Arrived trigger).** The shield flag byte 0xCD0 (0x000062C0) short-circuits
  the time test: with it set the trigger is the distance test alone (0x0000630C). Without it the
  float 0xF64 plus lbl_10_rodata_190 = 6.0f against the elapsed time (0x000062D0..0x000062E4)
  answers false (0x000062E8) and lbl_10_rodata_194 = 20.0f plus the same offset
  (0x000062F0..0x00006300) answers true (0x00006304). Otherwise the distance to the point
  0xEE4/0xEE8 (the helpers branch's mGuestEE4, the path find's fallback destination) in the XY
  plane - the z is lbl_10_rodata_13C = 0.0f, the store at 0x00006348 overwrites the z difference
  - against lbl_10_rodata_15C = 3.0f decides (the magnitude at 0x0000634C, the compare at
  0x00006358).
- **fn_10_9724 (the BoostBallFire state).** The attack bookkeeping with id 2 (0x00009760).
  Activate (0x00009764): the roll emitter removed (0x00009778) and its handle cleared
  (0x00009784), the turn around (fn_10_9644 at 0x00009788), 0xD24 = lbl_10_rodata_284 = 0.5f
  (0x000097AC) and the boost ball's bit 0x40 of 0xEBC (0x000097B4, the PowerPC bit 25 being
  1u << (31 - 25) = 0x40). Update (0x0000983C): the boost ball deleted (fn_10_9AC4 at
  0x0000984C). Deactivate (0x00009854): fn_10_99B8 (0x00009864) - a stand-in, the port has no
  particle effects. Then the turn toward the player (0x00009880, lbl_10_rodata_28C =
  5.2359877f) and the melee attack command (fn_10_8764 at 0x000098E0, the severity 8 =
  pas::kS_Eight).
- **fn_10_9314 (the DoneRolling trigger).** Without the boost ball actor (the uid 0xE38,
  0x0000932C) the trigger answers false (0x0000933C). With it the roll's time 0xEA4 against
  lbl_10_rodata_27C = 2.0f (0x00009350) answers true when it is over (0x00009358). With a boost
  hint (the uid 0xEC0, 0x00009360) the hint's byte 0xEC8 (0x0000936C) answers. With the bit
  0x80 of 0xEBC (`extrwi r0, r0, 1, 24` at 0x00009378, the PowerPC bit 24 being
  1u << (31 - 24) = 0x80) the trigger answers true (0x00009384). With the roll's other time
  0xEA8 plus lbl_10_rodata_274 = 5.0f against the elapsed time (0x0000939C) it answers true
  (0x000093A8). Otherwise the distance to the point 0xEAC/0xEB0/0xEB4 against
  lbl_10_rodata_280 = 12.0f plus the radius 0xEB8 decides (the magnitude at 0x000093F0, the
  compare at 0x00009404, the "greater than" bit `extrwi r3, r0, 1, 1`).
- **fn_10_7A14 (the EnrageHitReact state).** Every message sets the body's locomotion type 10
  (pas::kLT_Internal10, 0x00007A4C). Deactivate (0x00007A50): the time 0xFB0 from
  lbl_10_rodata_1CC = 0.2f and lbl_10_rodata_1D0 = 4.5f (0x00007A70), the guest's own random
  stepped (the `mulhw` with 0x55555556 at 0x00007A8C: 0x105C grows by the high word of
  0x55555556 * 0x1058 and its carry bit) and the health stored (0xFC4, 0x00007AB0). Then
  fn_10_7F98 (0x00007AC8, a stand-in) and the melee attack command (0x00007B28, the severity 8).
- **fn_10_6E38 (the EnragedTravel state).** Every message reads the player's aim position (the
  state manager's player at 0x14FC, the vtable's slot 0x54 at 0x00006E78; the guest's extra
  argument &lbl_10_rodata_1AC = 0.5f has no host counterpart, the dt is 0.5f). Activate
  (0x00006E90): the travel (fn_10_6F90, the helpers branch's FindPathToDestination) and the
  target forgotten (0x9FC = -1, 0x00006EC4). The other messages measure the aim position against
  the point 0x1048/0x104C/0x1050 (the squared distance at 0x00006EEC..0x00006EF4 against
  lbl_10_rodata_1B0 = 16.0f at 0x00006EF8) and run the travel when it is bigger (0x00006F10);
  then the player's halfword at its +8 goes to 0x718 (0x00006F30 - the host's player has no
  counterpart) and the separation runs (fn_10_1DD14 at 0x00006F40). Every message then runs
  fn_10_6DAC (0x00006F4C, a stand-in), the additive animation (fn_10_1D860 at 0x00006F5C) and the
  glide emitter (fn_10_11334 at 0x00006F68).
- **fn_10_BD40 (the FadeIn state).** The attack bookkeeping with id 4 (0x0000BD70), the ground
  collision (fn_10_18F90 at 0x0000BD80) and the melee attack command (fn_10_8764 at 0x0000BDE0,
  the severity 4 = pas::kS_Four).
- **fn_10_BC54 (the FadeOut state).** The attack bookkeeping with id 8 (0x0000BC84), the ground
  collision (0x0000BC94), the shield flag (fn_10_105F4 at 0x0000BCA8, with the zero vector) and
  the melee attack command (0x0000BD08, the severity 3 = pas::kS_Three).
- **fn_10_F86C (the FallFromTheAir state).** The attack bookkeeping with id 0x4000
  (0x0000F89C), the ground collision (0x0000F8AC) and the taunt command (fn_10_8698 at
  0x0000F8E8: the command id 0x15 = 21 = kBSC_Taunt, the taunt type 0 = pas::kTT_Zero, the
  animation state 0x10 = pas::kAS_Taunt) - the helpers branch's RunTauntCmd.
- **fn_10_16CDC (the FreezeBeam state).** The attack bookkeeping with id 0x10 (0x00016D18), the
  collision failsafe (0x00016D24), the shield flag (0x00016D38) and the ground collision
  (0x00016D48). Activate (0x00016D4C): the beam's counter 0xAB0 cleared (0x00016D5C), the bits
  0x40 and 0x80 of 0xAB8 cleared (the `rlwimi` at 0x00016D6C clears the bit 0x40 - the PowerPC
  bit 25 - and the one at 0x00016D7C sets the bit 0x80 - the PowerPC bit 24 - to the bit 0x40's
  value, which the first one just cleared) and the dodge record zeroed (0x1090 = 0.0f at
  0x00016D9C, the direction 0x1094..0x109C at 0x00016D88..0x00016D98). The other messages deliver
  the exit state command (fn_800EE45C at 0x00016DD0, the command id 0xA = 10 = kBSC_ExitState)
  when the counter 0xAB0 is at least 1 (0x00016DA8). With the bit 0x40 of 0xAB8 set
  (0x00016DEC..0x00016DF4) the actor turns toward the player (0x00016E1C, the rate
  lbl_10_rodata_454 = 0.017453292 radians a degree times lbl_10_bss_118 = 160.0f, the module's
  own turn speed) and dodges (fn_10_1707C at 0x00016E3C, with lbl_10_bss_11C = 22.0f and
  lbl_10_bss_120 = 0.5f - a stand-in). Then fn_10_8870 (0x00016E8C - a stand-in).

### The new members

- guest 0xF64 (the Arrived trigger's time offset), 0xEA4/0xEA8 (the DoneRolling trigger's two
  times), 0xEAC/0xEB0/0xEB4 (its point) and 0xEB8 (its radius) - the boost ball's record; the
  helpers branch's mGuestEE4 (0xEE4) is the Arrived trigger's point.
- guest 0xAB0 (the FreezeBeam's counter), 0x9FC (the EnragedTravel's target), 0x1048/0x104C/
  0x1050 (the EnragedTravel's point), 0xFB0 and 0xFC4 (the EnrageHitReact's time and health) and
  the guest's own random pair 0x1058/0x105C.
- lbl_10_bss_118 = 160.0f: fn_10_CDB8 (the module's init, 0x0000CDB8) writes it from
  lbl_10_rodata_320; it is the freeze beam's turn speed in degrees a second (the rate is
  lbl_10_rodata_454 = 0.017453292 radians a degree times it). The init also writes
  lbl_10_bss_11C = lbl_10_rodata_328 = 22.0f, lbl_10_bss_120 = lbl_10_rodata_32C = 0.5f and
  lbl_10_bss_124 = lbl_10_rodata_324 = 1.9f (the first two are the dodge step's arguments, which
  the port does not run yet).
- The boost ball's bit 0x80 of 0xEBC (the DoneRolling trigger reads it).

### The runs

- The coverage run (tools/cov/run.sh with `tools/cov/specs/darksamus.py` and
  `tools/cov/specs/darksamus_states.con`, the rooftop, the boss u04f7, `fsm <boss> <state>` for
  each state): every state ran with all three messages - the Activate (0), the Update (1) and
  the Deactivate (2) - and the BreakPause trigger's two arms (the key 0 -> ret 0 and the key 1
  -> ret 1).
- The forcing run (the same fight with the members written at the trigger's entry, the arms
  cycling over the hits): the DoneRolling trigger's eight arms, seven of them forced (the first
  one, no boost ball, is the natural one):
  | arm | the members written | the answer |
  |---|---|---|
  | no boost ball (the uid 0xE38 is invalid) | the natural one | false |
  | the roll's time 0xEA4 is over | mBoostBallUid = 1, mGuestEA4 = 3.0 | true |
  | a boost hint and its byte 0xEC8 is set | mBoostBallUid = 1, mGuestEA4 = 0, mBoostHintUid = 2, mGuestEC8 = 1 | true |
  | a boost hint and its byte 0xEC8 is clear | the same with mGuestEC8 = 0 | false |
  | the bit 0x80 of 0xEBC | mBoostBallUid = 1, mGuestEA4 = 0, mBoostHintUid = invalid, mBoostBallFlag80 = 1 | true |
  | the roll's other time 0xEA8 is over | the same with mGuestEA8 = -1000.0 | true |
  | the distance test, the point at the origin | the same with mGuestEA8 = 1000.0 and the point 0 | true |
  | the distance test, the point at the actor | the same with the point at the actor | false |
- The Arrived trigger is not evaluated by the fight (the coverage run: 0 hits), so its arms were
  not run and it keeps no marker.

## Slice 27 (2026-10-21): the helpers behind the states' stand-ins, and the gcov coverage

### The helpers

- **fn_10_99B8 (0x000099B8, 0xDC bytes) - `StopBoostBallParticles`.** The boost ball's old
  particle effects are stopped and forgotten: the two swoosh handles at 0xE3C and 0xE4C are
  cleared and their objects told to stop (the vtable's word 2 with the argument 1, 0x000099F4
  and 0x00009A20) and the element generator's record at 0xE60/0xE64 is cleared
  (0x00009A68..0x00009A80). The port has no counterpart for any of them - the module's own
  swooshes (fn_10_9B24) and the generator are not in the port - so the pointers are always null
  and the body does nothing.
- **fn_10_6DAC (0x00006DAC, 0x8C bytes) - `UpdateShieldFromHealth`.** Deactivate (0x00006DB4)
  clears the shield flag byte 0xCD0 (0x00006DCC). The other messages read the health (the
  vtable's slot 0x3C at 0x00006DD4, the float at its +4 at 0x00006DE8) and, when it is not above
  lbl_10_rodata_14C = 1.0f (`fcmpo` at 0x00006DF0, `cror eq, lt, eq`, `bne` skip at 0x00006DF8 -
  a NaN health skips, so the test is `health <= 1.0f`), set the flag (0x00006E08), 0xD24 = 1.0f
  (0x00006DFC) and the direction 0xD18..0xD20 to lbl_10_bss_128 = (0.3, 0.6, 1.0) (
  0x00006E10..0x00006E20).
- **fn_10_8870 (0x00008870, 0xE0 bytes) - `RunFreezeBeamCmd`.** Activate (0x000088B0) stores 1
  (kAS_Ready) to the animation state 0x6B4, delivers the command (0x000088C8) and copies it into
  the command manager's slot at 0x1B8 (0x000088D4..0x000088E8); Update (0x000088F0) re-delivers
  it while the DOL's ShouldRetry answers true (0x000088F8, the animation state 0x9 =
  pas::kAS_LoopAttack); Deactivate (0x0000893C) stores 0 (kAS_NotReady). The command is the
  loop hit reaction (the id 0x9 = 9 = kBSC_LoopHitReaction, 0x00016E70); the retail's command
  carries three more words (7, 4 and the byte 0, 0x00016E80..0x00016E88) that the host's command
  class does not model.

The fourth helper behind a stand-in, fn_10_1707C (0x0001707C, 0x1F4 bytes - the freeze beam's
dodge step), is not ported in this slice, so the FreezeBeam state keeps its stand-in for it and
keeps no marker.

### The states

The BoostBallFire state's Deactivate arm (0x00009854) now calls fn_10_99B8 and the EnragedTravel
state's every-message arm (0x00006F4C) calls fn_10_6DAC, so both states are whole again; the
FreezeBeam state's command (0x00016E8C) calls fn_10_8870, but its dodge step (0x00016E3C) is
still a stand-in.

### The gcov coverage (the marker gate)

The coverage build (`-B build-port-cov -DMP_PORT_COVERAGE=ON`) and the rooftop run over it
give the branch coverage of every module body at once
(`tools/cov/gcov_report.py build-port-cov <run's gcov dir> --module DarkSamus`). The marked
bodies and their verdict:

| fn | the verdict |
|---|---|
| fn_10_10710 (the Pause state) | 16/16 lines, 6/6 branches, **ALL-ARMS** - the "no fight stage" arm (the retail's own null check at 0x00010770) is forced by the coverage spec (the second Activate invalidates the battle stage's uid) |
| fn_10_9314 (the DoneRolling trigger) | 10/10 lines, 10/10 branches, **ALL-ARMS** - the eight arms are written at the trigger's entry from the coverage spec's expressions (one per hit) |
| fn_10_BD40 (the FadeIn state) | 6/6 lines, no branches - ALL-ARMS |
| fn_10_BC54 (the FadeOut state) | 7/7 lines, no branches - ALL-ARMS |
| fn_10_F86C (the FallFromTheAir state) | 6/6 lines, no branches - ALL-ARMS |
| fn_10_99B8 (the boost ball's old effects) | 1/1 line, no branches, but **unmarked**: the virtual calls on the particle objects have no host counterpart, so the body is not the whole function |
| fn_10_6DAC (the shield flag from the health) | 4/6 branches - **unmarked**: the Deactivate arm was not reached in the run |
| fn_10_9724 (the BoostBallFire state) | 5/6 branches - not marked: the Deactivate arm did not run |
| fn_10_6E38 (the EnragedTravel state) | 4/6 branches - not marked: the "the aim is far" arm (0x00006EF8) did not run |
| fn_10_16CDC (the FreezeBeam state) | 4/6 branches - not marked: fn_10_1707C is still a stand-in |
| fn_10_62B4 (the Arrived trigger) | 0/7 lines - not marked: the fight never evaluates it |

### The coverage

The compiler coverage build (`-B build-port-cov -DMP_PORT_COVERAGE=ON`) and the ordinary
rooftop run over it give the branch coverage of every module body at once
(`tools/cov/gcov_report.py build-port-cov <run's gcov dir> --module DarkSamus`); the arms that
the fight cannot reach are forced from the coverage spec's own expressions
(`tools/cov/specs/darksamus.py`: the DoneRolling trigger's eight arms are written at the
trigger's entry, one per hit, from gdb convenience variables).

## Slice 28 (2026-10-21): the fsm2 merge, fn_10_8870's command, and the crash

- **The merge** (`decomp/darksamus-fsm2` af6729d14): the conflicts were in the header (the
  mGuestFC4/mGuest102C block) and in the helpers file (their new bodies against my appended
  ones). Their text is kept for their own bodies and members, mine follow theirs. The link check
  reports no duplicate definition and no undefined symbol (tools/link_check.sh).
- **fn_10_8870 (the loop attack command)** - corrected in slice 29: the retail's fn_10_8870 is
  generic (the message in r4, the animation state in r5, the command in r6) and the FreezeBeam's
  caller builds a **loop attack** command, not a loop hit reaction (see slice 29).

- **The crash is gone.** Delivering the plain `CBodyStateCmd(kBSC_LoopHitReaction)` left the
  command manager's slot with no type, and the body's loop reaction then read a garbage
  animation index (1798152516) and died in `CBSLoopReaction::Start`. The rooftop run after the
  merge (the coverage driver, the normal build) ends with the game's exit code 0 and no
  SIGSEGV in the log.
- **Still open.** fn_10_1707C (the freeze beam's dodge step, 0x0001707C, 0x1F4 bytes) is not
  ported, so the FreezeBeam state keeps that stand-in and keeps no marker (its arms did not run
  in the run either - the state was not entered). The Arrived trigger (fn_10_62B4) still has no
  arms: the fight never evaluates it (0 hits, 0/7 lines in the gcov report) and the state that
  would evaluate it has not been found, so neither `statecall` nor force.sh can reach it.

## FSM2 lane slice 6 (2026-10-21): the CBSLoopReaction crash, twelve triggers

- **The crash** (decomp/darksamus2 0473a8b4e's coverage run `/tmp/opencode/covg`, frame ~3600:
  `CBSLoopReaction::Start` -> `LoopBestAnimation` with the animation 1798152516): its gcov shows
  FreezeBeam (16CDC) ran once and its fn_10_8870 call delivered once, right before the fault. That
  8870 delivered a plain `CBodyStateCmd(kBSC_LoopHitReaction)`. The retail call is a loop
  attack: at 0x00016E40..0x00016E88 the object at r1+0x10 is the vtable lbl_10_data_1B0
  (CBCLoopAttackCmd's, its base lbl_10_data_1D4), +0x4 the id 7 = kBSC_LoopAttack
  (0x00016E70), +0x8 the type 4 (0x00016E80), +0xC the wait flag 0 (0x00016E84), and +0x10 the
  byte with bit 0x80 (skip-into) cleared (`rlwimi r0, r7, 7, 24, 24` at 0x00016E54). The `li r5,
  0x9` (0x00016E78) is 8870's animation-state argument, pas::kAS_LoopAttack, not a command id. So
  FreezeBeam's call is `CBCLoopAttackCmd(pas::kLAT_Four, false)` through the typed DeliverCmd,
  the same shape SuperMissileAttack (type 0), NormalMissileAttack (type 1) and the 0x85DC caller
  (type 3) already inline. No port-only class is needed. Neither this branch nor 0473a8b4e
  crashes on the link build with the lane's own driver (the drive.log replayed) or with
  darksamus_fsm2{,_force2}.con and darksamus_phazon.con.
- **Marked** with `trigcall` (tools/cov/specs/darksamus_triggers2.{py,con}): 16B5C, 16B54, 112AC,
  11760, 117A0, FD30, 10698, F800 (each ALL-ARMS or no branches, the members forced at the
  trigger's entry by key) and the wrappers 17CB8, 142CC, 10634, 112E8 (callees already marked).
  16B5C/16B54 are true only when forced: their natural writer is the anim-event handler
  fn_10_1BEFC, still a stand-in.
- Unmarked: 14284, BB78, BBE4 (their no-stage arm needs fn_10_17E14 to return null, which an
  entry-only force on the -O2 cov build can't do), 11158, 1BB08.

## Slice 29 (2026-10-21): fn_10_8870 is the loop attack command

- **The merge** (`decomp/darksamus-fsm2` 0c5f871dc, twelve triggers marked with `trigcall`):
  the only conflict was this note; both sides' sections are kept.
- **fn_10_8870 (0x00008870, 0xE0 bytes)** is generic: the message in r4, the animation state for
  the DOL's ShouldRetry in r5 (0x000088F8) and the command in r6. The id at cmd+4 is delivered
  (0x000088B4, 0x00008908) and cmd+4..+0x10 is copied into the command manager's slot
  (0x000088D4..0x000088E8, 0x0000891C..0x00008934). Activate (0x000088B0) stores 1
  (kAS_Ready) to the animation state 0x6B4, Update (0x000088F0) re-delivers the command while
  ShouldRetry answers true, and Deactivate (0x0000893C) stores 0 (kAS_NotReady).
  The port's helper is `RunLoopAttackCmd(msg, attack)` - the shape the fsm2 lane's states already
  use inline (PortCDarkSamusFsm2.cpp, the SuperMissileAttack, the NormalMissileAttack and the
  0x85DC caller).
- **The FreezeBeam's command** (0x00016E40..0x00016E88) is a **loop attack**: the vtable
  lbl_10_data_1B0 (the base lbl_10_data_1D4), the id 7 = kBSC_LoopAttack (0x00016E70), the type
  4 = pas::kLAT_Four (0x00016E80), the wait flag 0 (0x00016E84) and the byte with its bit 0x80
  (the skip-into) cleared (`rlwimi r0, r7, 7, 24, 24` at 0x00016E54). The `li r5, 0x9` at
  0x00016E78 is fn_10_8870's animation-state argument (pas::kAS_LoopAttack), not a command id.
  So the port delivers `CBCLoopAttackCmd(pas::kLAT_Four, false)` through the typed DeliverCmd -
  **no port-only command class and no change to a shared BodyState file are needed**. (Slice 28
  read it as a loop hit reaction; that was wrong.)
- **The rooftop run** (the coverage driver, the normal build): the game's exit code 0, **no
  SIGSEGV**, no reach stub. The FreezeBeam state is run with the console's `statecall` (it has
  no node in the shipped FSM2): `statecall u04f7 FreezeBeam 0/1/2` runs the Activate, the Update
  and the Deactivate, and `RunLoopAttackCmd` runs in all three - the loop attack plays, without
  the crash.
- **Still open**: fn_10_1707C (the freeze beam's dodge step, 0x0001707C, 0x1F4 bytes) is not
  ported, so the FreezeBeam state keeps that stand-in and keeps no marker (its 0xAB0 >= 1 arm
  and its 0xAB8 bit 0x40 arm did not run either). The Arrived trigger (fn_10_62B4) still has no
  arms: the fight never evaluates it and the state that would evaluate it has not been found.

## FSM2 lane slice 7 (2026-10-21): the anim-event handler, nine marks

- **fn_10_1BEFC (DoUserAnimEvent) ported in full, unmarked.** The jump table
  (jumptable_10_data_FB4, `cmplwi r31, 0x2e` at 0x0001BF28): cases 0, 8, 9, 12, 17, 18, 27, 28,
  33, 34 return without the base (0x0001C6A0); every other type ends in
  CPatterned::DoUserAnimEvent. Type 43 is not in EUserEventType (static_cast; 0xD24 = 0.9f). The
  vtable 0x14C call at 0x0001C050 is slot 83 = fn_10_1BB08 (AnyIncomingMissiles, CTriggerData(0)).
  Stand-in callees keep it unmarked: 13998, 13AB4, 1353C, 133D0 (helpers2 lane), 9164, 5840,
  77B8, 9E00 (helpers lane). Case 19 writes 0xE38 as a halfword (the boost ball uid) though the
  header also names a float mGuestE38 there: check when 9E00 lands.
- **Marked:** 1BDF0 (GetMeleeReach = 2.5f + 0x3DC), 1BE04 / 1BE80 (FadeEnvFxOut/In: FadeDensity
  0/1 over 4000, then 'EXIT'/'ENTR'), 16B48 (BreakFreezeBeam: 0xAB8 & 0x80, written by Landing
  0x100000 at 0x0001C350), 14284, BB78, BBE4, 11158 (ShouldReact), 1BB08 (AnyIncomingMissiles).
  A natural Landing with attack 0x40 arrives in the fight, so BreakNormalMissile goes true unforced.
- **Forcing keys** (tools/cov/specs/darksamus_uevent.{py,con}, darksamus_react.{py,con}): the
  attack by the `uevent` dt; no stage by writing mBattleStageUid = 0xffff at
  `CDarkSamus::GetBattleStage(` (an out-of-line call, so the entry-only force works there); the
  stage's flags at `TCastToPtr<CDarkSamusBattleStage>(CEntity*)` ($rdi = the entity). The missile
  float +0x24 is both CanMissileDeflect's "< 0" and the reflect chance, so AnyIncomingMissiles is
  made true by forcing CanReflectProjectile's type `$rcx = -1` and a `shoot`.
- **Not covered in 1BEFC:** BeginAction's deflect slow-down (a missile in flight past 28 units),
  Landing's dive bounce, type 46 with dash steps left.
- **PreRender 18AB8 / Render 18964 blocked** (left as stand-ins): unported callees 18238 (0x1B0),
  183E8 (0x1E4), 1890C (0x58; needs 185CC, 1863C), 10864 (0x17C; needs 10A20, 109E0), 12DD0
  (0xC0; needs 8F50), and unmapped members 0xDE8 (an alpha float Render gates the body on), 0xDED
  bits 0x40/0x80 (echo/dark visor), 0xE64, 0x90C bit 0x20, CActor 0x44C..0x44F, the model flags
  at 0xFC. A partial Render risks an invisible boss.

## Slice 30 (2026-10-21): the port merge, the Arrived trigger's arms, and fn_10_1707C's reading

- **The merge** (`port`, with the fsm2 lane's ddac9d34b): the only conflict was this note, both
  sides' sections are kept. The merge brought the members mGuestAB0, mBoostBallFlag80,
  mGuestEA8, mGuestEAC and mGuestEB8 a second time; the fsm2 lane's declarations stay (their
  mGuestEAC is the 0xEAC/0xEB0/0xEB4 vector and their mGuestEB8 the radius - the same bytes the
  DoneRolling trigger reads) and the DoneRolling trigger now reads their vector. The link check
  reports 0 duplicate definitions and 0 undefined symbols.
- **The Arrived trigger (fn_10_62B4) is marked.** The module's FSM2 dump (the port's
  `MP2_DS_FSM` probe) shows which state evaluates it: `state 43 GoToPhazonAttack -> trigger
  Arrived -> state PhazonSuck` (and `state 44 GoToPhazonEnrage -> trigger Arrived -> state
  PhazonRise`), so the fight calls it when the boss is put in that state (`fsm u04f7
  GoToPhazonAttack`). The forcing run (the members written at the trigger's entry, the arms
  cycling over the hits) ran all five arms:
  | arm | the members written | the answer |
  |---|---|---|
  | the shield flag 0xCD0 is set, the distance test (the point is the origin) | mGuestCD0 = 1, mGuestEE4 = 0 | false |
  | the shield flag is set, the distance test (the point is the actor) | mGuestCD0 = 1, mGuestEE4 = the actor's position | true |
  | the pause is still running | mGuestCD0 = 0, mGuestF64 = 100000.0 | false |
  | the pause is long over | mGuestCD0 = 0, mGuestF64 = -100000.0 | true |
  | the distance test after the pause | mGuestCD0 = 0, mGuestF64 = the elapsed time - 10.0 | false |
- **fn_10_1707C (the freeze beam's dodge step) is still not ported.** Its reading, for the next
  slice: the dodge timer 0x1090 loses dt (0x000170D0) and is cleared with the direction
  0x1094..0x109C when it reaches zero (0x000170E8..0x00017104); the body runs only when the
  timer is zero (0x00017118). It reads the player's aim position (the vtable's slot 0x54 at
  0x00017134, the dt 0.5f), takes the direction from the actor to the aim flattened (the z is
  0.0f, 0x00017174..0x0001718C) and normalizes it (0x00017190..0x000171B0); the step side is
  the perpendicular of that direction times lbl_10_rodata_3E4 = 0.5f (0x000171D4..0x00017200) and
  the other side is tried when the first is blocked (the line check fn_10_164C0 at 0x00017240 and
  0x0001728C); the step's length is Range(lbl_10_rodata_438, lbl_10_rodata_47C) (0x000172E4) and
  the direction that is clear becomes 0x1094..0x109C, normalized (0x00017388..0x000173A4), with
  the timer 0x1090 set to the weight (0x000173A8); the impulse is the direction times the speed
  0x158 times the rate (f2 = lbl_10_bss_11C = 22.0f) (0x000173C0..0x000173FC) and it is applied
  with ApplyImpulseWR and an identity axis angle (0x00017400..0x00017410).
- The FreezeBeam state keeps its stand-in for fn_10_1707C and keeps no marker.

## Slice 31 (2026-10-21): the FreezeBeam's dodge step and its arms

- **The merge** (`port`): the only conflict was this note, both sides' sections are kept; the
  link check reports 0 duplicate definitions and 0 undefined symbols.
- **fn_10_1707C was already ported** - the port's `UpdateDodge` (PortCDarkSamusFsm.cpp, whose own
  comment gives the reading) - so the FreezeBeam's stand-in for it is replaced by the call the
  retail makes at 0x00016E3C, with lbl_10_bss_11C = 22.0f (the impulse) and lbl_10_bss_120 = 0.5f
  (the dodge's time): `UpdateDodge(mgr, dt, 22.f, 0.5f);` - the same shape the fsm2 lane's
  SuperMissileAttack uses (0x00016FF0).
- **The FreezeBeam state (fn_10_16CDC) is marked.** Its arms all ran in the coverage run (the
  state's Activate, Update and Deactivate with `statecall u04f7 FreezeBeam 0/1/2`, the state's
  command fn_10_8870 in all three, the exit state command with the counter 0xAB0 at least 1 and
  the turn and the dodge with the bit 0x40 of 0xAB8 set). The last two need the members written
  at the Update's entry, because the Activate clears both (0x00016D5C and 0x00016D6C): the
  coverage spec's expression writes the counter 0xAB0 = 1 on the first Update and the bit 0x40 of
  0xAB8 on the second (`tools/cov/specs/darksamus.py`, the FreezeBeam entry's expressions). The
  run: 4 hits of the state, `CBodyStateCmdMgr::DeliverCmd` 3, `TurnTowardPlayer` 2,
  `UpdateDodge` 2, the game's exit code 0 and no SIGSEGV.
- fn_10_1707C itself keeps no marker: its own arms (a dodge that starts, the side that is
  chosen) did not all run - the coverage build's gcov report would say which.

## Slice 8 (dsfsm2, 2026-10-21)

- Ported DoUserAnimEvent's callees into `PortCDarkSamusHelpers.cpp`: ClearEnrageSphere 77B8,
  ApplyMeleeDamage 13998, SpawnMeleePulse 13AB4, FireMissile 1353C, FireSuperMissile 133D0,
  FireFreezeBeam 9164. New host members (slice-8 TARGET_PC blocks): the melee 'PART' tokens
  0x9C4/0x9D0, the missile infos 0xA0C/0xA4C (ProjectileInfo 1C6C8 now answers 0x40/0x80/0x100),
  the freeze beam token 0xA88, the flash record 0xDF0 (`mFlashTexture`, `mFlashSize` 0xE00).
- Marked (ALL-ARMS, `tools/cov/specs/darksamus_attacks.py`, run `/tmp/opencode/dsfsm2/s8/a6`):
  77B8, 13998, 1353C, 133D0, 9164.
- 13AB4 unmarked: its floor-ray miss arm never ran. With the actor moved 1000 up and 100000
  across, the host's RayWorldIntersection still answered a hit (explosion z = player z + 1.9).
  This looks like a host collision anomaly (dynamic near-list hit?) worth checking separately.
- Still stand-ins in DoUserAnimEvent (kept unmarked): 5840 (needs 50B8, 4C40, 0xF80/0x1064 infos)
  and 9E00 (builds module classes 3BC4/3FD8). The 0xE38 halfword/float conflict is unresolved.
- Assumptions: 0xEE0/0xEDC = iceSpreadSound/iceSpreadFX; fn_10_4EB4 = `CImpactVisorEffect::None()`.
- Render chain: DrawFlashQuad 1863C and RenderFlash 1890C ported, unwired and unmarked (no caller
  until PreRender/Render land). 18238/183E8 not started: they need CopyScreenTex, CGX::SetDstAlpha,
  RenderSilhouette, CAnimData::Render with CModelFlags, the model at 0xD6C and members 0xDE8
  (1.0f), 0xDEC, 0xDED bits, 0xD24, the 0x1060 bit.
- CanReflectProjectile not attempted.

## Slice 9 (dsfsm2, 2026-10-21)

- Ported and marked (ALL-ARMS): FirePhazonVolley 5840, FirePhazonShot 50B8, FirePhazonSuperMissile
  4C40 (run `/tmp/opencode/dsfsm2/s9/a4`, darksamus_attacks.py keys 10..28) and DoUserAnimEvent 1BEFC
  (`s9/u3`, darksamus_uevent.py; the deflect slow-down by keys 42/43 with `shoot`, the dive
  bounce by key 40, type 46 by key 41).
- New host members: the phazon infos `mPhazonShotInfo` (guest 0xF80, from phazonProjectile /
  phazonProjectileDamage) and `mPhazonSuperMissileInfo` (0x1064, phazonSuperMissile*); ProjectileInfo
  answers them for 0x800000 by 0xFB8 0 / 1. The pairing of the two loader stack words with the two
  infos is by name; the asm only shows 0x280/0x284 to 0xF80 and 0x288/0x28C to 0x1064.
- SpawnBoostBall 9E00 (unmarked, ALL-ARMS): the call is ported; the ball class (vtable
  lbl_10_data_104, ctors 3FD8 / 3BC4, Think 3B08 with a CCollisionActorManager, AcceptScriptMsg 3998,
  render 3008 / 3174, touch damage) is a job of its own. The host has a construction-only
  `CDarkSamusBoostBallPortStub` (drawn, moved by UpdateBoostBallAtActor, no collision, no damage).
  fn_10_99B8 (the darksamus2 lane's StopBoostBallParticles, 613d2cbfb) and fn_10_9B24 (the swooshes)
  are logged stand-ins in it. The 0xE38 conflict: the halfword is the uid (`sth` at 0x00009E4C),
  so `mBoostBallUid`; the float `mGuestE38` is unused.
- 13AB4: the "host collision anomaly" was the spec. Key 21 moved `mTransform`, but the ray starts at
  `GetTranslation()` (`mPosition`), so it ran from (68, 412, 165) and hit the static floor at z 164
  (t 1, materials 0x81000005, hit id none). With the position moved too, the ray misses and every
  arm runs (`s9/a4`). It's left unmarked at the orchestrator's request.

## Slice 10 (dsfsm2, 2026-10-21)

- Merged port b4f3292a5 (3b54a85ce). DeliverCmd check: every derived command the module sends
  (LoopAttack, Getup, Locomotion, Jump, MeleeAttack, Step, ProjectileAttack, Taunt) has its own
  overload; none binds to `CBodyStateCmd&`.
- CanReflectProjectile 1B9B0: fixed to call GetBattleStage once (0x0001BAA8) after the untyped
  return (0x0001BA9C); marked.
- Render chain ported, wired and marked: Render 18964, GetSilhouetteColor 181E0,
  RenderSilhouetteModel 18238, RenderInvulnerableModel 183E8, DrawFlashQuad 1863C. PreRender 18AB8
  ported, unmarked (fn_10_10864 and fn_10_12DD0 stand-ins). RenderFlash 1890C unmarked: its
  no-texture return never ran. 13AB4 marked (s9/a4).
- Header (TARGET_PC, appended): `mGuestDE8 = 1.f`, `mDarkVisorDED80` / `mEchoVisorDED40`,
  `mInvulnerableSkinnedModel` (retail optional at 0xD6C, valid byte 0xD70; Render reads
  `**(*opt)`), `mBoostBallElementGen` (0xE64; always null on the host).
- Runs: `dsfsm2-runs/s10/u4,u9,u10` (on the Leo disk; /tmp hit its quota), darksamus_uevent.py with
  `--frames 1340`. Forcing keys for the render arms:

  | key | forces | arm |
  |-----|--------|-----|
  | 45..49 | null / own / exploded / other type / sent back | CanReflectProjectile rejects |
  | 50 | 0xD14 = 1 | silhouette pass (combat), min side of 181E0 |
  | 51 / 52 | 0xD24 = 0.5 / 1 | 183E8 wave under / over 1.0 |
  | 53 | phazon phase bit | 183E8 alpha 1.0 |
  | 54 | 0xDE8 = 0 | Render's dark/echo visor test, PreRender alpha |
  | 55 | flash window (E04/E08/E0C) | GetFadeFactor > 0, RenderFlash |
  | 56 | boost-ball uid invalid | Render's model arms |
  | 57 | 0xFC0 = 0.5 | PreRender's colour lerp |
  | 63 / 64 | stage +0x33 set / restored (only the actor's stage, by uid) | Render's flash-only arm |

  Visors: `give DarkVisor 1; give EchoVisor 1`, then `visor combat/dark/echo` (the cov save owns
  neither). Not run (dead or host-only): Render's 0xD14 > 0 with 0x90C bit 0x20 clear (PreRender sets
  it first), the 0xE64 generator, the host GetActiveVisor null-camera branch; 18238's
  optional_object exception edges.
- Re-read against the asm (compares, strictness, load sources, return vs base call): 77B8, 13998,
  1353C, 133D0, 9164, 1863C, 1890C, 5840, 50B8, 4C40, 1BEFC. No mismatches. 9164's sfx: retail
  converts -1 to kInternalInvalidSfxId in the ctor (0x0002015C) and stores a halfword at 0xEE0; the
  port converts at use, which gives the same value.
- Open: RollReflectChance F298 (14/18 lines); DE8 fades in Think (1D5C4/1D6CC) unported; the
  stage-cast breakpoint's pre-existing FLAG writes (keys 61/62) are not uid-guarded like key 63.
## Slice 32 (2026-10-21): the review fixes, the port merge and the sweep beam's damage mode

- **The merges**: `review12/darksamus2` (e824da91d) and then `port` (batch 12, with the dsfsm2
  lane's b5dec578a). The port merge conflicted in PortCDarkSamusHelpers.cpp and in this note;
  both sides' functions are kept (the link check reports 0 duplicate definitions and 0 undefined
  symbols).
- **The attack ids** are the `lis` (X << 16) ones: the BoostBallFire 0x20000 (`lis r4, 0x2` at
  0x00009758), the FadeIn 0x40000 (0x0000BD60), the FadeOut 0x80000 (0x0000BC74) and the
  FreezeBeam 0x100000 (0x00016D10). The other states use the `li` ids (the Pause 0x400 at
  0x00010748, the FallFromTheAir 0x4000 at 0x0000F88C, the DiveAttack 0x20 at 0x00011FE4, the
  MeleeDash 0x4 at 0x00017AE4, the SideDash 0x80 at 0x000177F0, the ScatterShot 0x8 at 0x000175EC),
  so the comments that said "id 2/4/8" are fixed.
- **The FreezeBeam returns after the exit state command** (the branch to the epilogue at
  0x00016DE8): the turn and the dodge (0x00016DF0) no longer run in the same Update as the
  command.
- **The EnragedTravel** sets the navigation's face target and runs the path find (0x00006F30 and
  0x00006F34).
- **The EnrageHitReact** divides signed by 3 (0x00007A8C): `mulhw` is the signed high word and
  `srwi r0, r4, 31` is the logical shift (the sign bit), so the step is the signed division by 3
  rounding to zero.
- **The module's globals**: lbl_10_bss_128 is the module's static vector - the module's data
  initialiser fn_10_221C0 writes it at 0x0002224C and the constructor copies it into the actor's
  0xD18 (0x0000E5C8); it is (0.3, 0.6, 1.0) (rodata 46C/424/3E8 at 0x0002224C..0x000222A8), not
  zero, and fn_10_6DAC's direction write (0x00006E10..0x00006E20) stores the same value. lbl_10_bss_11C = 22.0f and lbl_10_bss_120 =
  0.5f are written by the module's init fn_10_CDB8 at 0x0000CE38 and 0x0000CE3C (the freeze beam's
  dodge step takes them as its impulse and its time).
- **The audit's two findings**:
  - fn_10_C85C (StartSweepBeam): the guard around the CElementGen stays (the pool has no object
    for an invalid 'PART' id), but 0xDC8 bit 0x40 is set either way (0x0000C9E4), so the damage
    rays (fn_10_C104) and the collision response (fn_10_C470) run as in the retail - the ray code
    tolerates a null generator (0x0000C32C).
  - fn_10_BE18 (SweepBeamRay): the "any other hit" damage uses the module's weapon mode
    lbl_10_bss_114 (the read at 0x0000C024, the store into the damage info at 0x0000C05C). The
    module's init fn_10_CDB8 writes it at 0x0000CE30 as 0x000A0000 - the guest's CWeaponMode keeps
    the type in the high half, so the type is 10 = kWT_Phazon and the flags are 0. The port keeps
    it in mSweepBeamWeaponMode.
- **The FreezeBeam's arms on the merged code**: the coverage run (the coverage driver, the
  normal build) - 4 hits of the state, the Activate, two Updates and the Deactivate; the first
  Update (the counter 0xAB0 = 1, written by the coverage spec's expression) delivers the exit
  state command and returns (no turn, no dodge), the second (the bit 0x40 of 0xAB8) turns and
  dodges. The game's exit code 0 and no SIGSEGV. The coverage build's report agrees: fn_10_16CDC
  is 20/20 lines, 6/6 branches, **ALL-ARMS** (fn_10_10710 and fn_10_9314 are ALL-ARMS too).

## Slice 33 (2026-10-22): the port merge, the dive record's +0x4C, and two unmarked functions

- **The merge** (`port`, with the DarkSamus-A audit fixes f5802a210): no conflict - the CanMissileDeflect
  trigger (fn_10_14284) is now `GetMissileDeflectTime() > 0.f` (the `extrwi r3, r0, 1, 1` at
  0x000142B8 takes the "greater than" bit) and fn_10_F100 (RollReflectChanceForHit) answers the
  first roll (the `fmr f1, f31` at 0x0000F1DC).
- **fn_10_E910 (CDarkSamusDive::Reset)**: the record's float +0x4C (0xB0C) takes
  lbl_10_rodata_3EC = -1.0f (the store at 0x0000E9E0). The constructor leaves the field alone
  (the guest's damage info ends at +0x1B, so +0x4C is the dive record's own float) and the REL
  reads it nowhere else, so the port keeps it in mGuestD18's neighbour mGuest4C.
- **The re-runs**:
  - the Arrived trigger (fn_10_62B4) on the merged code: all five arms ran again (the forcing run
    with the members written at the trigger's entry, the arms cycling over the hits) - the shield
    flag set with the distance test (the point at the origin -> false, the point at the actor ->
    true), the pause still running (false), the pause long over (true) and the distance test after
    the pause (false). Its marker stands.
  - fn_10_14284 (CanMissileDeflect): only the shipped arm ran (the deflect time 45.0f or 50.0f, the
    trigger answers true - 1267 hits). The other arm (a deflect time below zero) could not be run:
    gdb cannot call the port's GetBattleStage to write the stage's info ("Cannot resolve method
    CDarkSamus::GetBattleStage"), so **the marker is removed**.
  - fn_10_F100 (RollReflectChanceForHit): the fight does not reach the function (0 hits - the
    reflect chance is only rolled for a projectile that comes at the boss), so its two arms (the
    cache miss and the cache hit) were not run and **the marker is removed**.

## Slice 11 (dsfsm2, 2026-10-21)

- Ported: 10A64 UpdateHeadAim (Think 0x0001B330), 10864 TwistHead and 12DD0
  UpdateMeleePulseAndEffects (PreRender's two former stand-ins), 1D6CC UpdateFade, 1D5C4
  UpdateTargetable; Think's tail after the alive block now runs retail's order (0x0001B3DC..0x0001B448:
  1D6CC, `FadeDensity(0, 4000)` while not fading and alive, 1D5C4, 12DD0 with dt). 8F50
  UpdateBoostBallEffects is ported but unmarked (the host builds none of 0xE64/0xE3C/0xE4C).
- New members (appended TARGET_PC blocks): 0xD40 mHeadYaw, 0xD44 mHeadSegId ("Head_1", ctor),
  0xD48 mHeadTwistTime (-1000).
- Marks (run `dsfsm2-runs/s11/u8`): 10A64, 10864, 12DD0, 1D6CC, 1D5C4, 1890C, 18AB8 (PreRender: its
  one untaken branch is the TARGET_PC GetPlayerState fallback); F298 cites u8 too; 13AB4 cites s11/a1.
- F298/F210 citations fixed: 0x0001F2xx/0x0001F3xx → 0x0000Fxxx.
- Keys 61/62 are uid-guarded like 63. New keys:

  | key | forces | arm |
  |---|---|---|
  | 23 | attack 0x8000 | 1D5C4's first arm |
  | 65 | reflect cache count 10 | F298's erase |
  | 66 / 67 | C0C, C20 at elapsed / elapsed - 6 | F298's 4 s / 8 s windows |
  | 68 | RenderFlash's texture invalid once | 1890C no texture |
  | 69 / 71 / 72 | no forward and on the player / forward ±90° (one UpdateHeadAim) | 10A64 normalize and clamps |
  | 73 / 74 | dead / 0xD48 = elapsed at the next TwistHead | 10864's returns |
  | 75 / 76 | fading held / dead from UpdateFade to UpdateTargetable | 1D6CC, Think's FadeDensity |
  | 77 | stage +0x33 reads 0 (it is 1 there naturally) | 1D5C4's stage arm |
  | 78 | boost ball uid reads 1 for one PreRender | PreRender's alpha 0 |

- Forcing artefact fixed: key 99 restored the saved 0xFFFF over an attack the FSM had started in a
  held window, so its Deactivate pushed 0xFFFF into the history and CollectHistoryPenalties wrote
  `weights[0xFFFF]`. `$dsk_fa` now tracks the forced attack; 99 restores only while it still holds.
  Retail pushes unconditionally too, so the port is unchanged.
- Open: Think 313 (dead at the alive check) not forced; Think stays a stand-in (fn_10_1ACA0 ..
  fn_10_1B154 unported).

## Slice 34 (2026-10-22): DS2-FOLLOWUP - the three review fixes

- The merge of `port` (e85ad6ac1, batch 14) was a fast-forward; no conflict.
- **fn_10_16600 is IsRingPathClear**: the function loops over the ring's rays (the counter at
  0x0001663C) and calls fn_10_166AC (the ray check) at 0x00016658 - so the `// @ fn_10_16600` on
  IsRingRayClear (which is fn_10_166AC) was a slip and is removed; IsRingPathClear keeps the
  marker.
- **fn_10_9E00 (SpawnBoostBall)** calls fn_10_99B8 at 0x00009E30 - the port's
  StopBoostBallParticles - and no longer the stand-in (the helpers lane's comment said the merge
  would swap the call in; fn_10_9B24, the swooshes, is still a stand-in because it is not in the
  port).
- The DigitalGuardianHead's damage colour is stored at 0x18E64..0x18E6C (the store follows the
  armour model's constructor call at 0x18E60), not at 0x18F28.

## Slice 35 (2026-10-22): the next unmarked functions - four bodies, no markers yet

- The merge of `port` (e85ad6ac1) was a fast-forward; no conflict.
- **fn_10_2EFC** (0x00002EFC, 8 bytes): the float at guest 0x314 (`lfs f1, 0x314(r3)`) - the
  port's GetGuest314.
- **fn_10_2F04** (0x00002F04, 0xC bytes): bit 0x40 of the byte at guest 0x318 (`lbz` at
  0x00002F04, the `extrwi` at 0x00002F08 takes the bit 25 = 1u << (31 - 25) = 0x40) - the port's
  GetGuest318Bit6.
- **fn_10_1B24** (0x00001B24, 0xDC bytes): the cross product of two vectors - the x
  a.y * b.z - b.y * a.z (the `fmsubs` at 0x00001B44), the y a.z * b.x - b.z * a.x (0x00001B4C) and
  the z a.x * b.y - b.x * a.y (0x00001B54). The host's CVector3f::Cross is the same formula; the
  port's new CrossProduct helper is used by UpdateHeadAim (fn_10_10A64, the cross at 0x00010B34).
- **fn_10_E5C** (0x0000E5C, 0xDC bytes): the base's EnsureRendered (the host's
  CActor::EnsureRendered(const CStateManager&)); the port's new EnsureRenderedActor calls it from
  AddToRenderer.
- **No markers**: the coverage run (the rooftop, the coverage build) shows 0 hits for
  fn_10_2EFC, fn_10_2F04 and fn_10_E5C - their only callers are the BoostBallRoll state
  (fn_10_A3B0, still a stand-in) and the fade/visor path in AddToRenderer - and the compiler
  inlines fn_10_1B24, so the report has no row for it. Without an ALL-ARMS run there is no
  marker, so the four bodies are unmarked.
- The remaining candidates in the tsv's order (fn_10_2C, fn_10_324, fn_10_A08, fn_10_A60,
  fn_10_B04, fn_10_B28, fn_10_B78, fn_10_C98, fn_10_DC8, fn_10_E10, fn_10_1B64, fn_10_1C08,
  fn_10_1D8C, fn_10_2040, fn_10_2098, fn_10_213C, fn_10_2254, fn_10_2798) are the module's own
  sub-object constructors and destructors: they need port-only classes for the guest's own
  records (the boost ball, the enrage sphere, the scatter shot's projectile), which is a bigger
  job than this slice.

## Slice 36 (2026-10-22): the BoostBallRoll state (fn_10_A3B0)

- The merge of `port` was a fast-forward (the base stays d4d094ded).
- **fn_10_A3B0 (0x0000A3B0, 0x384 bytes) - the BoostBallRoll state.** Every message runs the
  attack bookkeeping with id 0x20000 (fn_10_1B478, the `lis r4, 0x2` at 0x0000A3EC, the call at
  0x0000A3F4), the shield flag (fn_10_105F4 at 0x0000A408) and turns the "BoostThingy" effect on
  (the string at lbl_10_rodata_2E4 + 7, the SetEffectState at 0x0000A434).
  - Activate (0x0000A464): the script state "SI20" (kSS_InternalState2, 0x0000A48C); the boost ball
    is spawned (fn_10_9E00 at 0x0000A4AC) when there is none (the uid 0xE38 is kInvalidUniqueId,
    0x0000A49C); the actor's position goes to the ball's target 0xE98..0xEA0 and the roll's time
    0xEA4 = 0.0f (0x0000A4B0..0x0000A4D0).
  - Update (0x0000A4D8): the boost ball's bit 0x20 of 0xEBC must be clear (the `extrwi` at
    0x0000A4DC takes the bit 26 = 1u << (31 - 26) = 0x20), the ball's actor must exist
    (fn_10_9920 at 0x0000A4EC) and the bit 0x40 of 0x318 must be set (fn_10_2F04 at 0x0000A4FC, the
    `clrlwi` at 0x0000A500 takes the bit 24 = 1u << (31 - 24) = 0x80 of it). Then fn_10_A1C0
    (0x0000A514), the shield flag 0xCD0 is cleared (0x0000A520) and the ball's actor takes the
    damage (0x0000A654).
  - Deactivate (0x0000A6DC): the script state "SI21" (kSS_InternalState3, 0x0000A704).
  - Every message then rolls the ball to the hint: with a hint (the uid 0xEC0, 0x0000A66C)
    fn_10_AA04 (0x0000A68C), else the actor moves by (0.0f, lbl_10_rodata_26C = 1.25f, 0.0f)
    (MoveInOneFrameOR at 0x0000A6BC); fn_10_A06C (0x0000A6C8) and fn_10_A154 (0x0000A6D4) follow.
- **fn_10_A154 (0x0000A154, 0x6C bytes) - `RollBoostBallToPosition`** (the small leaf, ported):
  the actor's position (0x54/0x58) against the ball's target (0xE98/0xE9C) in the XY plane (the
  squares at 0x0000A180 and 0x0000A184, the sum at 0x0000A188) against lbl_10_rodata_278 = 4.0f
  (0x0000A18C); when the actor is close enough (0x0000A1B0) the roll's time 0xEA4 grows by dt
  (0x0000A1B4), else the ball's target becomes the actor's position (0x0000A194..0x0000A1A4) and
  the time is reset to 0.0f (0x0000A1A8).
- **The coverage run** (the rooftop, the coverage build, the BoostBallRoll state in it):
  fn_10_A154 (RollBoostBallToPosition) is 10/10 lines, 6/6 branches, **ALL-ARMS** - it is
  marked. fn_10_2F04 (GetGuest318Bit6) is 1/1 line and no branches - the state calls it
  (0x0000A4FC), so it is marked too. fn_10_2EFC (GetGuest314) and fn_10_E5C
  (EnsureRenderedActor) still have 0 lines: the bit 0x40 of 0x318 is clear in the run, so the
  Update arm's body (the getter's only caller) never runs, and the EnsureRendered call is behind
  the fade factor and the dark visor. fn_10_A3B0 itself has no row in the report (its body is in
  PortCDarkSamusFsm.cpp, which the coverage build does not instrument) and it keeps no marker
  anyway: fn_10_A1C0, fn_10_AA04 and fn_10_A06C are stand-ins.
- **The callees that are not ported** (their calls stay stand-ins, so the state keeps no marker):
  fn_10_A1C0 (0x1F0 bytes - it picks the roll's hint and the roll's parameters), fn_10_AA04
  (0x760 bytes - the roll to the hint) and fn_10_A06C (0xE8 bytes - the ball's transform and the
  roll emitter). The damage the Update arm applies to the ball's actor is built by the retail on
  the stack (the damage min(the health - 1.0f, the float at 0x314), the radius 0.0f, the
  knockback 0.0f and the weapon mode 0x13 with the bits 16, 17, 18, 24 and 25 cleared); the port
  does not reproduce that hit.


## Slice 12 (dsfsm2, 2026-10-22)

- Merged `port` (e30f30958); its 14284 `> 0` and F100 first-roll lines are kept unchanged.
- Ported: Think 1B1FC in retail order, without the stand-in. It sets up the machine until it has a
  state, then faces the player through the waypoint navigation's 0x6E0, runs CPatterned::Think and
  grows 0x8B0. Retail has no active test of its own; the alive block runs the 14 calls, and the
  tail is slice 11's. Also ported: 1ACA0 UpdateDiveContact, 13868 PushPlayerAfterBounce, F710
  UpdateSilhouetteFade (0xD14), F654 UpdateInvulnerableFade (0xD24), 1AE08 UpdateMeleeHit, 1B0E0
  UpdateStageFlash, 1B154 CheckHealthThreshold, and F5C0/F5EC GetTouchBounds (always the bounding
  box, valid).
- 0x10A0 is the constructor's **first** float (`fmr f23, f1` at 0x0001FA94), not its eighth (the
  table above): the loader's property 0x72EDEB7D (ReadFloat, `stfs f1, 0x91c(r1)` at 0x0000D4FC),
  the host's `mData.mUnknown72EDEB7D`. New member 0x10A4 is `mHealthThresholdSent`, in an appended
  TARGET_PC block.
- Forcing keys 80..96 (tools/cov/specs/darksamus_uevent.{py,con}; meanings in the spec's comment).
  -O2 places a line breakpoint before that line's code, so the post-call forces (85, 87, 95) break
  on the instruction after the call, written as a mangled symbol plus an offset (`after_call`)
  because the binary is PIE. The spec now starts every `$dsk_*` at its "nothing forced" value:
  gdb's `&&` evaluates both sides, so before key 30 every set had failed and been logged, which
  made run t1 time out.
- 8F50 (item 3): this is a **port gap**, not the shipped data. Retail's fn_10_9B24 (0x00009B24,
  called by SpawnBoostBall at 0x0000A03C) builds 0xE3C (a CParticleSwoosh, `new 0x218`, from the
  optional swoosh token at 0xE40 whose valid byte is 0xE48, 0x00009B44), 0xE4C (the same from
  0xE50/0xE58, 0x00009BE4) and 0xE64 (a CElementGen, `new 0x338`, from the 'PART' id at 0xE5C
  through gpSimplePool, 0x00009C84). The host has none of the three: 9B24 is a logged stand-in in
  SpawnBoostBall, and the 0xE10 sub-object (fn_10_895C) holds no swoosh or PART tokens. Fixing it
  takes 895C's tokens (from the loader's properties) and a 9B24 port; that is left for its own
  slice.
- Marks (run `dsfsm2-runs/s12/t3`, every arm): 1B1FC Think, 13868, F710, F654, 1AE08, 1B0E0, 1B154;
  F5C0/F5EC have no branches. 1ACA0 is unmarked: its `else if (mBounced)` false arm (the hit
  made, no bounce, the boxes overlapping, 1148:1/2) did not run. Key 96 (that state at each call
  until 99, with the player placed on the actor at f1534..f1537) is written but not yet run:
  runs t4, t5, t7, t9 and t10 all died with VK_ERROR_DEVICE_LOST ("this context is innocent",
  so another process on the GPU caused the reset), and Aurora skips llvmpipe.

## Slice 37 (2026-10-22): the rework of the BoostBallRoll's Update arm

- **The merges**: `port` (2e5ff4f85, with the GPU-hang fix) and then `3272cab53` (the
  darksamus-895c lane). The conflicts were in this note and in PortCDarkSamusHelpers.cpp: both
  sides' functions are kept, and fn_10_99B8 (StopBoostBallParticles) is the 895c lane's body -
  the two swooshes at 0xE3C/0xE4C are deleted (0x000099F4, 0x00009A20) and the element
  generator's auto_ptr at 0xE60/0xE64 takes an empty one (0x00009A68..0x00009A7C).
- **The BoostBallRoll's Update arm** (fn_10_A3B0, 0x0000A524..0x0000A668) is now whole:
  - the damage value (0x0000A54C..0x0000A558): the `fcmpo` of the float at 0x314 (fn_10_2EFC,
    the call's result in f1) with the health - 1.0f - `bge` takes the health - 1.0f, the other
    way the float at 0x314 (the unordered case too): the damage is min(the float at 0x314, the
    health - 1.0f);
  - the damage info (0x0000A564..0x0000A5EC): the weapon mode word 0x13 in the high half (the
    `li r0, 0x13` and the `rlwimi` at 0x0000A560/0x0000A564) with the low half and the bits 16,
    17, 18, 24 and 25 cleared - the type is 0x13 = 19 = kWT_UnknownSource and the mode's flags
    (the charged, the comboed, the instant-kill) are all false; the damage, the radius damage
    amount (the same value, 0x0000A568), the damage radius 0.0f, the knockback 0.0f, the three
    sound ids 0 and the flags byte with its bit 0x80 cleared. The host's CDamageInfo defaults the
    sound ids to 0xffff.
  - the filter (0x0000A5B4..0x0000A5F0, 0x0000A5F8/0x0000A62C): the include list is the material
    bit 1 << 0x3B (lbl_10_data_1E0 = 0x3B through the `__shl2i` at 0x0000A5F0), which is 59 =
    kMT_Solid, the exclude list is empty and the type is 3 = kFT_IncludeExclude.
  - the damage (0x0000A654): all three uids are this actor's own (`lhz 0x8(r30)` at
    0x0000A630..0x0000A650) - the retail damages the Dark Samus herself; the ball's actor is not
    used. The position is sZeroVector (0x0000A600).
  - **What the port missed**: the bit 0x20 of 0xEBC is set (`rlwimi` at 0x0000A660) and the
    shield flag 0xCD0 becomes 1 (0x0000A668). Without them the arm re-ran every frame.
- **The corrected bit comments**: the `clrlwi` at 0x0000A500 takes the bool's low byte, so the
  check in fn_10_A3B0 is the bit 0x40 of 0x318 itself - not "the bit 0x80".

## Slice 35 (2026-10-22): the next unmarked functions - four bodies, no markers yet

- The merge of `port` (e85ad6ac1) was a fast-forward; no conflict.
- **fn_10_2EFC** (0x00002EFC, 8 bytes): the float at guest 0x314 (`lfs f1, 0x314(r3)`) - the
  port's GetGuest314.
- **fn_10_2F04** (0x00002F04, 0xC bytes): bit 0x40 of the byte at guest 0x318 (`lbz` at
  0x00002F04, the `extrwi` at 0x00002F08 takes the bit 25 = 1u << (31 - 25) = 0x40) - the port's
  GetGuest318Bit6.
- **fn_10_1B24** (0x00001B24, 0xDC bytes): the cross product of two vectors - the x
  a.y * b.z - b.y * a.z (the `fmsubs` at 0x00001B44), the y a.z * b.x - b.z * a.x (0x00001B4C) and
  the z a.x * b.y - b.x * a.y (0x00001B54). The host's CVector3f::Cross is the same formula; the
  port's new CrossProduct helper is used by UpdateHeadAim (fn_10_10A64, the cross at 0x00010B34).
- **fn_10_E5C** (0x0000E5C, 0xDC bytes): the base's EnsureRendered (the host's
  CActor::EnsureRendered(const CStateManager&)); the port's new EnsureRenderedActor calls it from
  AddToRenderer.
- **No markers**: the coverage run (the rooftop, the coverage build) shows 0 hits for
  fn_10_2EFC, fn_10_2F04 and fn_10_E5C - their only callers are the BoostBallRoll state
  (fn_10_A3B0, still a stand-in) and the fade/visor path in AddToRenderer - and the compiler
  inlines fn_10_1B24, so the report has no row for it. Without an ALL-ARMS run there is no
  marker, so the four bodies are unmarked.
- The remaining candidates in the tsv's order (fn_10_2C, fn_10_324, fn_10_A08, fn_10_A60,
  fn_10_B04, fn_10_B28, fn_10_B78, fn_10_C98, fn_10_DC8, fn_10_E10, fn_10_1B64, fn_10_1C08,
  fn_10_1D8C, fn_10_2040, fn_10_2098, fn_10_213C, fn_10_2254, fn_10_2798) are the module's own
  sub-object constructors and destructors: they need port-only classes for the guest's own
  records (the boost ball, the enrage sphere, the scatter shot's projectile), which is a bigger
  job than this slice.

## Slice ds895c (2026-10-22): the boost ball's effects (895C tokens, 9B24, 99B8, 8F50) and 1ACA0

- fn_10_895C (the 0xE10 sub-object) loads its tokens in data order: r8 = boostBallGlow → 0xE5C
  (the 'PART' id), r9 = boostBallTrailSwoosh → 0xE40 (optional, valid byte 0xE48), r10 = the
  sWHC → 0xE50 (valid byte 0xE58). The host's members are `mBoostBallSwooshTokenE40/E50` and
  `mBoostBallGlowPart`. 0xE70, the collision PART that fn_10_8D30 loads, is not ported because
  nothing reads it.
- fn_10_9B24 replaces SpawnBoostBall's logged stand-in. Its "delete the old non-null" path is
  dead: its only caller is SpawnBoostBall at 0xA03C, and that runs fn_10_99B8 first (0x9E30).
  fn_10_99B8 now deletes both swooshes and the generator. BoostBallFire (fn_10_9724) and
  DiveFromBoost (fn_10_9424, which calls 99B8 at 0x9548) clear the 0xE60 generator.
  ~CDarkSamus deletes the swooshes, matching the sub-object dtor fn_10_1F72C. fn_10_9424 has
  all of its effect work ported and could now be checked for a mark.
- 8F50 matches the asm (vtable 0x14 SetOrientation, 0x18 SetTranslation, 0xC Update,
  0x24 SetGlobalScale, 0x2C SetParticleEmission, 0x30 SetModulationColor).
- Marks 9B24, 99B8 and 8F50 come from these runs (tools/cov/specs/darksamus_boostfx.py,
  tools/drivers/darksamus_boostfx.con): b1 (the natural rooftop run), f1 (forced DSB_MODE=1:
  no 0xE40 token) and f2 (forced DSB_MODE=2: no 0xE50 token, then the kill). Every arm ran, with
  no caught errors and no reach stubs.
- 1ACA0 is marked. tools/drivers/darksamus_divecontact.con runs keys 86/96 on their own (the
  boss u04f7 placed on the player at f623..f627), then keys 40 (bounced) and 87 (no touch
  bounds) at f650..f656. Key 96 has to be armed with 86, before the first overlap: run u4
  armed it later, so every contact took the first-hit arm. Runs u4 + u6 give 10/10 branches.

## Slice dsvt1 (2026-10-22): the vtable stand-ins

The five `StandIn` calls in `PortCDarkSamus.cpp` are now the retail bodies: AcceptScriptMsg
(fn_10_1E89C), GetOrbitPosition (fn_10_10358), GetAimPosition (fn_10_10210),
GetCollisionPrimitive (fn_10_1AAC4) and CollidedWith (fn_10_1C7FC). CollidedWith's unported
callees are ported with it, as new `CDarkSamus` methods declared in `CDarkSamus.hpp` and
defined in `PortCDarkSamus.cpp`: RecordPlatformContact (fn_10_1D0A0), RecordWallHit
(fn_10_1CF84), CheckBoostBallWallHit (fn_10_A734), CrushPlatform (fn_10_1D194) and AddPhazonBob
(fn_10_10484, const). `CDarkSamusDive` gains `mCollidable` (+0x68, guest 0xB28), the dive's
CCollidableAABox that GetCollisionPrimitive returns. Each body's comment cites its asm.

Fixes against the stand-ins: XDMG skips while `1.0 + 0xC20 >= now` (the stand-in used
`3.0 + max(0xC0C, 0xC20)`), and the hit direction 0x8C0 is stored before that test; ACTV's
boss bar HP is 0x908 when above 0, else the health; XALD's filter includes kMT_Solid and
excludes kMT_CollisionActor | kMT_AIPassthrough; XLSG shares XLND's arm.

Coverage (build-port-cov, rooftop `1baa96c2,10_cliff_rooftop`, `LAYER2@400`, boss u04f7):

    BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/darksamus_vt.py <out> \
      1baa96c2,10_cliff_rooftop --con tools/cov/specs/darksamus_vt.con --fast --frames 2000 \
      --env MP2_INPUT=LAYER2@400
    tools/cov/gcov_report.py build-port-cov <out1>/gcov ... --module DarkSamus

All ten bodies are ALL-ARMS over the union of five runs (r1: the natural run, with no writes;
r2..r5: `darksamus_vt.py` as it grew; r5 is the committed spec). No single run is ALL-ARMS,
because the fight is not deterministic: the shots that land and the hits that come in each
attack change from run to run. The forced arms (the gdb writes in `darksamus_vt.py`, each
written once at a body's entry and put back afterwards):
- ACTV with 0x908 at 700 and at -1. The dive's XLND/XLSG with the bounce bit set and clear.
  XDMG inside 0xC20's 1.0 s. XDMG with the attack 0x800000, where nothing but phazon hurts
  (the WeaponHurts false arm). CanReflectHit true: a missile, past 3 s, with a cached roll of
  -1. A vertical hit direction (CanBeNormalized false).
- XALD's FindConnectedObject with a state that has no connection (invalid uid), and with an
  IS06 connection, which is an AI hint and not a battle stage (the cast is null).
- Orbit/aim: a boost ball uid with no actor, and an attack position of (0,5,0) and of (0,0,5).
- AddPhazonBob inside the 5.3 s window and past it.
- GetCollisionPrimitive in the dive at the target (dist 0) and bounced.
- CollidedWith: the dash start/target positions for the `<` test, more than two stuck dashes,
  and a dive (0x20) that has not launched and has no boost ball (attack 0x80, rewritten for one
  hit).
- First contact's left material: Platform and Platform|Floor (RecordPlatformContact), Wall and
  Ceiling with no object in the attacks 0xFFFF/0x2000 (RecordWallHit; the invalid id is forced
  in another attack), Wall and Floor with a boost ball uid (CheckBoostBallWallHit).
- CrushPlatform from platform u0423 with Floor, Platform and neither.

Natural (r1): XDMG from the console (no projectile), shot hits, XLAU, ALRT, DCTV/XDEL, the
platform casts and the dash arms of CollidedWith.

## Slice ds2opus (2026-10-22): BoostBallRoll's callees (A1C0, A814, AA04), A06C and 9424 marked

- Asm read: fn_10_A06C 0xA06C..0xA150, fn_10_A1C0 0xA1C0..0xA3AC, fn_10_A3B0 0xA3B0..0xA730,
  fn_10_A814 0xA814..0xAA00, fn_10_AA04 0xAA04..0xB160, fn_10_9424 0x9424..0x95A4; CGenericFSM2
  EvaluateTransitions 0x801FC4C0..0x801FC5B0 and CPhysicsActor::GetMoveToORImpulseWR 0x800EA76C.
- Bug fixed: BoostBallRoll ran its movement tail (0xA66C..0xA6D4: AA04 or MoveInOneFrameOR, A06C,
  A154) on every message. Retail runs it on Update only: Activate leaves through `b .L_0000A708`
  at 0xA4D4 and Deactivate falls into the epilogue after 0xA704. CGenericFSM2 passes dt = 0.0f
  (lbl_8041D6B0) to Activate, so the port's Activate built a 1/dt impulse: NaN position, then
  CMetroidAreaCollider::BuildOctreeLeafCache spun forever once the roll ended.
- Runs (tools/cov/specs/darksamus_boostroll.py, tools/drivers/darksamus_boostroll.con, rooftop
  10_cliff_rooftop, build-port-cov): f1 = DSR_MODE=1, f2 = DSR_MODE=2; both rc 0, 0 caught,
  0 reach-stub. Breakpoints are `loc:*<mangled>+<off>` after a call; offsets in the spec header.
- Branch table (natural unless marked forced):
  - A06C: ball null (natural, 774); roll emitter null (natural after DiveFromBoost, 208).
  - A3B0: Activate/Update/Deactivate natural; `default` dead (three EStateMsg values); Activate
    with a ball (driver: two `fsm BoostBallRoll`); flag 0x20 set (after A1C0); ball null forced
    (DSR_MODE=1, A3B0+628); bit 0x40 of 0x318 forced (DSR_MODE=2, A3B0+654); no hint natural.
  - A1C0: Normalize false forced (DSR_MODE=2, A1C0+150), true natural.
  - A814: token absent forced (DSR_MODE=1, `m_valid` = 0 at entry, restored at AA04+1832);
    the two null-after-`rs_new` arms are class (a), compiler-generated.
  - AA04: hint null forced (DSR_MODE=1, AA04+92); toHint Normalize forced (DSR_MODE=2,
    AA04+153); next null forced (DSR_MODE=2, AA04+580, also the arrival without a next hint);
    toNext/fromLast/dir/face Normalize forced (DSR_MODE=1, AA04+641/+832/+908/+1878); last hint
    zero and the zero direction natural (before the first arrival); bit 0x80 forced (DSR_MODE=1,
    entry write); the hint values forced (DSR_MODE=1, AA04+1602/+1761 -1.0/0.05/5.0 for the
    1.0f/0.1f/2.0f clamps, AA04+1645 value2 1.0); the 25.0f arrival and the plane test natural.
  - 9424: Activate and Deactivate both natural (driver `fsm DiveFromBoost`).
- Marks: A06C, A1C0, A814, AA04, 9424. A3B0 stays unmarked: every arm ran, but fn_10_9E00
  spawns CDarkSamusBoostBallPortStub, not the retail ball. fn_10_2EFC is not called (the port
  reads mGuest314 directly).
- Spec gotcha: gdb counters must not be named after registers (`$pc` advanced the PC).

## Slice dsball (2026-10-22): the retail boost ball

- `CDarkSamusBoostBall` (`include/MetroidPrime/Enemies/CDarkSamusBoostBall.hpp`, bodies in
  `PortCDarkSamusHelpers.cpp`; vtable lbl_10_data_104) replaces CDarkSamusBoostBallPortStub. 0x318
  bits: 0x80 = `mSilhouette`, 0x40 = `mDamaged` (bitfields with stated positions); 0x314 = damage
  taken; 0x34C = `mRelToken`, CRELFileToken("DarkSamus.rel", 0) (lbl_10_bss_5C, built by fn_10_44D0).
  `mBoostBallFlag20` (0xEBC 0x20) is a bitfield too, cleared in the ctor body. New DS member 0xE6A `mBoostBallHitPlayerSfxId`; the
  `mGuest314/318/E6A` stand-ins are gone. A3B0 now reads the ball through 2F04/2EFC.
- Spec `tools/cov/specs/darksamus_ball.py` + driver `tools/drivers/darksamus_ball.con` (gdb force.sh,
  command in the spec header), 0 caught / reach-stub:

| fn | arms | run |
|---|---|---|
| 9E00 SpawnBoostBall | 0xD14 == 0 / != 0 | natural 0, 0.5 forced on the 1st spawn (natural at hp 1) |
| 3FD8 / 3BC4 ctors | plain / silhouette | both |
| 2F04 / 2EFC | ret 0 / 1 | both (A3B0 reads) |
| 2F10 touch damage | cooldown > 0 / hit | both |
| 3008 Render / 3174 PreRender | silhouette 0 / 1; pre-render combat / other visor | all (`visor scan`) |
| 31E4 / 3B08 Think | manager null (create) / set; cooldown | all |
| 3998 AcceptScriptMsg | XDEL with / without manager (forced), XHIT player / other / null, XDMG colAct / null, others | all |
| 3934 / 38D0 | straight-line | run |
| A3B0 BoostBallRoll | ball damaged; damage < hp-1 / >= hp-1 | all (`damage u0703 50` / `3000 darkburst`) |

- Marks: 3FD8 3BC4 2E64 2EFC 2F04 2F10 3008 3174 31E4 3B08 3998 3934 38D0 9E00 A3B0.
- Driver uids u0702 (ball) / u0703 (its collision actor) are the first ball's in every run so far.
  XDMG adds `100000 - HP` per message, as retail does.
- asmcheck (rework): 21 findings remain, all explained in the comments. The ctors' inlined helpers
  (4368 / 3F70 / 2040 / 2098, CAxisAngle::Identity, the name string's internal_dereference, the
  CRELFileToken ctor) sit in the init list, which the tool does not read; 2E64's CMemory::Free and
  four conditions are the deleting-destructor wrapper; ImmuneRicochetVulnerability is the retail
  name of ReflectVulnerabilty (both 0x800DBB40); internal_dereference in 31E4 / A3B0 is a temporary
  string's destructor.

## Slice dsfsm (2026-10-22): the FsmStandIns (95A8, 14448, 143A8, 7F98 call, C98 blocked)

- Spec `tools/cov/specs/darksamus_dsfsm.py` (force.sh + cov_force.py, command in its header), drivers
  `tools/drivers/darksamus_dsfsm.con` (Dead during BoostBallInto) and `darksamus_dsfsm_dead.con`
  (Dead without the roll). Rooftop, 1100 fast frames, 0 caught / reach-stub.

| fn | arms | run |
|---|---|---|
| 95A8 DiveAfterBoost | 0xEBC 0x20 set -> false | forced (key 110) |
| | no stage -> false | forced (key 112, uid 0xFFFF) |
| | stage +0x78 = 0 -> false | natural (shipped 0) |
| | stage +0x78 = 1 -> true | forced (key 113, DiveAfterBoost+30, host offset 0xAC) |
| 143A8 Dead | Activate with roll emitter (RemoveEmitter) / without; Update; Deactivate | natural |
| 14448 Null | `blr`, no arm | never entered: the rooftop FSM2 has no Null node |
| 7A14 EnrageHitReact | calls UpdatePhazonEnrage (7F98, marked earlier) on every message | natural |

- fn_10_C98 (CScattershotProjectile add shot) stays a stand-in: it appends a 0x20-byte shot record
  to the projectile's own list at +0x22C (cap 0x12C = 300, `cmpwi r9, 0x12c`; flag byte 4, | 8 every
  17th, | 0x10 every 13th, | 0x20 every 5th shot; fn_10_DC8 / fn_10_E10), so it needs the
  CScattershotProjectile class (vtable lbl_10_data_40, 0x0..0x44D0) ported first (done: Slice dsscat).

## Slice dsscat (2026-10-22): CScattershotProjectile (vtable lbl_10_data_40) and fn_10_C98

- Port class `include/MetroidPrime/Enemies/CScattershotProjectile.hpp` +
  `src/MetroidPrime/Enemies/PortCScattershotProjectile.cpp` (TypesMatch id 21 and the TCastToPtr
  specializations live there). Names from MP3 symbols; shot record SShot {pos, target, distance,
  flags: 1 dead, 2 hit, 4 needs sweep, 8 hit sfx, 0x10 hit explosion, 0x20 slow}.
- FireScatterShotProjectile (fn_10_12F60) now adds to the live projectile through
  TCastToPtr (AddShot at 0x0001308C), else spawns `CScattershotProjectile(..., 45.0f)`.
- Bodies: 2AD0 ctor, 29FC dtor, 2918 Think, 29DC AcceptScriptMsg, E5C AddToRenderer, E7C Render,
  C5C GetTouchBounds, C98 AddShot, 2C CheckPlayerHit, 324 Sweep, B78 CanHit, 1C08 CheckNewShots,
  1D8C SpawnHitExplosion, 213C PlayHitSfx, 2254 UpdateShots, 2798 InitRenderParams. The TEV setup
  (lbl_10_bss_0/4, fn_10_1B64) is a function-static CTevPass in Render.
- Runs: gcov build `build-port-cov`, rooftop, driver `tools/drivers/darksamus_scat.con`, spec
  `tools/cov/specs/darksamus_scat.py` (natural, 2000 fast frames) and
  `tools/cov/specs/darksamus_scat_force.py` (force.sh); 0 caught / reach-stub in both.

| fn | arms | run |
|---|---|---|
| 2AD0 ctor | all 15 | natural |
| 1C08 CheckNewShots | all 10 (alive / not, needs sweep / not, hit / none) | natural |
| 2918 Think | dt <= 0 (0x00002950) | forced (`$th % 97`, dt = 0) |
| | lifetime > 10 (0x000029A0) | forced (`$th % 211`, mLifetime = 11) |
| | the rest | natural |
| 29DC / E5C / C5C | no branch in retail or port | natural |

- Unmarked (arms missing in the natural run): C98 cap 300 (0x00000CA8); 2C dir not normalizable
  (0x000000A0); 324 Sweep point-inside / box arms; B78 pass-through (0x00000BC4); 1D8C dist < 2,
  no COLR, no particle (0x00001DB8/0x00001DC4/0x00001E58); 213C no COLR / sfx < 0; 2254 4 arms;
  2798 the missing-texture / missing-colour arms; E7C Render 24 arms; 29FC dtor (empty-vector arm).
- asmcheck false positives (commented in the cpp): x22C is mShots.size(); x24C_* bits are the shot
  flags; the flagged calls are inlined helpers (2C8, B04, A08, A60, 2040, 1B64, 2E44 = the
  CModelData ctor) and token/string helpers; E7C's call/branch counts sit in StreamBillboard /
  BillboardAxes; the dtor's 9 conditions are the vector teardown, 1D8C's 18 the optional/token ones.

## Slice ds38 (2026-10-22): the CScattershotProjectile rows marked (C98 2C 324 B78 1D8C 213C 2254 2798)

- Fix: 2254 UpdateShots' pass test compares the move's own magnitude (`bl Magnitude` at
  0x000023E8 on the move at sp+0xAC, `fcmpo cr0, f1, f31` at 0x000023F0, f31 = the distance left), not
  the step; the distance flown adds that magnitude too (0x000023F8).
- Runs on `build-port-cov`, rooftop, `tools/drivers/darksamus_scat.con`, 2000 fast frames: natural
  `tools/cov/specs/darksamus_scat.py`; forced `tools/cov/specs/darksamus_scat_arms.py` (force.sh +
  `COV_GDB_SCRIPT=tools/cov/force_ret.py`, custom breakpoints in the spec). 0 caught / reach-stub.
  Merged report (forced + natural): all eight ALL-ARMS.

| fn | forced arms (everything else natural) |
|---|---|
| C98 AddShot | 300 shots, the cap at 0x00000CA8 (mCount = 300 every 7th call) |
| 2C CheckPlayerHit | the last shot's target = its position, no direction (0x000000A0) |
| 324 Sweep | to = from (0x000003EC); a stale uid in the near list, ObjectById null (0x0000057C); the own uid appended with CanHit true and its touch box moved: around `from` (PointInside, 0x00000888), 1% along (nearer hit, 0x00000824), 99.5% along, 50 off the ray, none (0x0000074C) |
| B78 CanHit | GetVulnerability effect 2 = pass-through (0x00000BC4) |
| 1D8C SpawnHitExplosion | COLR absent (0x00001DC4); shot flown 1.0 (0x00001DB8); empty particle optional (0x00001E58) |
| 213C PlayHitSfx | COLR absent (0x00002170); sound id -1 (0x00002198) |
| 2254 UpdateShots | target = pos (0x00002334); a shot 0.001 short of a target still to sweep (0x00002404); toEye zeroed (the eye at the box centre) |
| 2798 InitRenderParams | sub-elements null; textures null; B1PO/B2PO/TLPO set to the description's IVEC (null in the shipped data) |

- retail_diff leads: 324's fn_10_B04 (CRayCastResult() via B28), fn_10_A60 (the CCollidableAABox
  copy) and fn_10_A08 (the valid CRayCastResult ctor) are the header's inline ctors; 1D8C/213C's
  GetObj is TToken's operator*.
- Also marked: E784 CDarkSamusDive ctor (fix: the collidable box gets the retail material list
  1 << lbl_10_data_CB8 = 0x3B, kMT_Solid, `__shl2i` at 0x0000E880, not an empty one; no branch in
  retail) and 1C778 SetPathFindDestination (no branch); plain fsm2 run
  (`darksamus_fsm2.py` + `darksamus_fsm2.con`, 4000 frames). 4868 DeleteGuestActor all arms
  natural. 48C8 / 69C0 with `tools/cov/specs/darksamus_guest_force.py` (force.sh, same driver):

| fn | forced arms (everything else natural) |
|---|---|
| 48C8 DeleteGuestActorIfExpired | 0xF7C = elapsed - 1, the delete (0x000048F4 not taken; bge is taken on NaN, as `!(f7c < t)`) |
| 69C0 DeleteGuestUids | the count 0x930 is 0 in every run (nothing ported fills it); forced 2 with slot 0 invalid (0x000069F8) and slot 1 a stale uid (0x00006A0C) |

## Slice ds39 (2026-10-22): bodied B rows marked (6DAC 6F90 6374 9624 8698 6E38 7A14 1E2E8 8764 21F14 11C60)

- Fix: 6DAC UpdateShieldFromHealth is `health <= 1.0f` (`fcmpo` 0x00006DF0, `cror eq, lt, eq`
  0x00006DF4, `bne` skip 0x00006DF8: NaN skips); the port had `if (health > 1.f) return;`, which
  sets the shield on NaN. Sizes corrected: 6DAC 0x8C, 6E38 0x158, 7A14 0x154, 1E2E8 0x78.
- Runs on `build-port-cov`, rooftop, `tools/drivers/darksamus_ds39.con`, 5700 fast frames: natural
  `tools/cov/specs/darksamus_ds39.py`; forced `tools/cov/specs/darksamus_ds39_force.py` (force.sh +
  `COV_GDB_SCRIPT=tools/cov/force_ret.py`). 0 caught / reach-stub. Merged report: 6374 6DAC 6E38
  6F90 7A14 11C60 ALL-ARMS; 8764 8698 21F14 every arm but the dead `default:`; 1E2E8 9624 no branch.

| fn | forced arms (everything else natural) |
|---|---|
| 6DAC UpdateShieldFromHealth | health 0.5 (the set arm) |
| 6374 FindPathToWaypoint | path not shagged, on the last waypoint and before it (the search's result, waypoint, count) |
| 11C60 UpdateDiveFlight | the dive target = its start (CanBeNormalized false) |
| 21F14 DoProjectileAttackCmd | Update with the animation state Ready (CanIssueCommand true) |

- Dead arms with proof comments: the `default:` of 8764 / 8698 / 21F14 (EStateMsg has three
  values; retail's out-of-range exits are cited per function); 1E2E8's range checks and loop count
  (the only caller, 1E26C, passes begin with the count at 2).
- Still unmarked, checked partially: 1E740 ShouldForgetLastAttacker (7/8, the null cover point
  untaken - needs a forced uid whose cover lookup fails), 175AC ScatterShot (3/4, dead switch path),
  9724 BoostBallFire (5/6, dead final else-if), 1DF74 TravelToCoverPoints (15/16, the null cover
  return at 0x0001E0A8), 16B8C TurnTowardPoint (4/6, zero flat forward / zero target), 16048
  GetDashPathClearance (14/16); not yet looked at: 812C 895C 1444C B458 BA38 1E4C4 1DD14 EFC8.

## Slice ds40 (2026-10-22): 13 B rows marked (175AC 9724 1E740 1DF74 16B8C 16048 812C 1444C B458 BA38 1E4C4 1DD14 EFC8)

- No behaviour change: every body read against its asm (NaN senses included) matched. Sizes
  corrected: 9724 0x1FC, 1E740 0x124. The 6E38 comment now says what the body does at 0x00006F30 /
  0x00006F34 (the navigation's face target and PathFind).
- Run on `build-port-cov`, rooftop, `tools/drivers/darksamus_ds40.con` (ds39's drive + eight
  BoostBallInto Activates, two EventStarts in PhazonAttack, AI-hint edits for EFC8 via the console's
  `hint <id> [type] [area n] [radius r]`), 5900 fast frames, forced with
  `tools/cov/specs/darksamus_ds40_force.py` (force.sh + `COV_GDB_SCRIPT=tools/cov/force_ret.py`;
  the spec adds per-caller callee forcing). 0 caught / reach-stub. Every function below ALL-ARMS
  except 175AC / 9724, whose one untaken arm is dead (proof comments: EStateMsg has only 0..2).

| fn | forced arms (everything else natural or driven) |
|---|---|
| 1E740 ShouldForgetLastAttacker | search not shagged on 1 / 3 waypoints; GetCoverPoint null (0x0001E7CC) |
| 1DF74 TravelToCoverPoints | GetCoverPoint null (the return at 0x0001E0A8) |
| 16B8C TurnTowardPoint | forward straight up (flat zero, 0x00016C00); point on the actor's x/y (0x00016C5C) |
| 16048 GetDashPathClearance | dest on the actor's x/y (0x00016124); right static / left dynamic side ray blocked; every dynamic ray blocked (0x0001638C) |
| 1444C IsLineOnMesh | height query fails (0x0001454C); height 1e6 / -1e6 (0x00014560 / 0x00014568) |
| 1E4C4 ScoreCoverPoint | min range 1e9; min -1 max 0; cover on target, target on actor, cover on actor x/y; forward up |
| 1DD14 Separate | repulsion point 100 units off (0x0001DEDC); contact time = now (0x0001DE60) |
| B458 FindArrivedHint | start uid no object (0x0000B48C); links no object (0x0000B4F4); links inactive (0x0000B50C) |
| BA38 BoostChainHasInactiveHint | uid no object (0x0000BA80); links no object / type 0 / inactive or active type 0x11 |
| 812C StopGuestSfx | the beam sound 1 instead of the rooftop's -1, so a live emitter is stopped (0x00008158) |
| EFC8 IsInUnsafeSpot | driven, not forced: u066a deactivated, u066b/u066d moved to area 7, u066c type 0x1C radius 20 with the boss placed on it |

- 1E4C4's attacker loop is `uint i < mAttackerCount`; retail tests the count signed (`cmpwi` /
  `ble`). The count is at most 2, so it is the same.
- 895C left unmarked: it is not a body but the CDarkSamus.hpp member initializers
  (mBoostBallSwooshTokenE40/E50, mBoostBallGlowPart, mBoostBallCollisionToken, ...); marking it
  needs a body or a tsv class change (orchestrator).
- review23: 895C is class H (the host's member initializers). Checked word by word against
  the asm; it set 0xEA8 = -1000.0f (rodata_260, +0x98) and zeroed 0xE98 (+0x88..0x90), now the
  initializers too. Its copies (damage info +0x00, model words 0xE2C..0xE34, collision sounds
  +0x70) the port reads from mData instead.

## Slice ds41 (2026-10-22): the 40 smallest B rows (6 marked, 32 H, 1 T, 1 left)

- Marked: 17E48 GetStageCount (FD4C now calls it at 0x0000FD68 instead of reading 0x978),
  14284 CanMissileDeflect, 19014 `LeaveGround` (new; 18F90's Activate at 0x00018FA4, its
  Deactivate now `SetGrounded(mgr, true)` as at 0x00018FBC), 996C the const
  `GetBoostBallActor` (new; replaces the four `const_cast` calls in GetOrbitPosition /
  GetAimPosition), 4368 `BoostBallActorParameters()` (Helpers.cpp, the boost-ball ctor's
  argument), CF14 `RegisterDarkSamusLoader()` (RELMain at 0x0000CF30; inlined, RELMain 1/1).
- Run on `build-port-cov`, rooftop, `tools/drivers/darksamus_ds40.con`, 5900 fast frames,
  forced with `tools/cov/specs/darksamus_ds41_force.py`. 0 caught / reach-stub. 14284 and 18F90
  ALL-ARMS; 17E48 / 19014 / 4368 / CF14 branch-free, every line hit; 996C 3/3 lines, the null arm
  (0x0000998C) dead: both callers gate on fn_10_9624 finding the same uid 0xE38 valid
  (0x0001024C/0x00010250, 0x00010388/0x0001038C).

| fn | forced arms |
|---|---|
| 14284 CanMissileDeflect | GetBattleStage null; the stage's missiles unknown_0x7ee77018 = -1 |
| 996C GetBoostBallActor (const) | 0xE38 = the player's uid in GetOrbitPosition / GetAimPosition (never asked while the ball rolls) |

- H (host copy; the tsv state names each): 100D0 20D58 20D78 20C94 20BFC 219D4 2199C 21A14 2E44
  3520 3540 34E8 36AC 4E4C 4E6C 4FF0 5010 4FB0 4EB4 DE1C DE3C DDDC B04 B28 22184 109E0 10A20 1B24
  20774 DC8 E10 1B00C: rstl construct / optional / vector templates and header inlines
  (CModelDataNull, CImpactVisorEffect::None, CRayCastResult, CVector3f::Cross, CHierarchyPoseBuilder
  Insert / GetSegRotation in host `CIkChain.cpp`).
- T: EDB8, whose only caller 153EC SideDash has no node in the shipped FSM2 (`fsm u04f7 SideDash`
  enters and leaves at once; 0 calls).
- Left unmarked: CEB4 GetOrigin, 0 calls: only CScriptTeamAiMgr and CMetroid ask it, and the boss
  is in no team.

## Slice ds42 (2026-10-22): the next 44 B rows (3 marked, 41 H)

- Marked: F40C GetScannableObjectInfo, FF20 DamageVulnerability, 1C6C8 ProjectileInfo; the port
  bodies already matched the asm (F40C: 0x420 bit 0x40 at 0x0000F41C, 0x20 bit 0x80 at 0x0000F438,
  0x97C bit 0x80 at 0x0000F454, the 0x92C null test at 0x0000F464, the token's +8 at 0x0000F46C,
  else fn_8007427C; FF20: 0xCD0 == 1, 0x980 == 0x800000 by subis/cmplwi at 0x0000FF4C, FFB4 copy
  of fn_800742F8 then SetComboVulnerability(3, (0.0f, kE_Normal, false)) via 0x800DB550 /
  0x800DC02C, else CAi; 1C6C8: the switch as commented). 1C6C8 port limit unchanged: retail
  answers the optional's address even when invalid, the host null for an unbuilt info.
- Run: `build-port-link`, rooftop, boss u04f7 activated at f600, fast; gdb boots the game and
  stops at the 1500th `CDarkSamus::Think`, then forces each arm by writing the members and
  calling the body (gdb call), restoring every member after. 0 caught / reach-stub.

| fn | forced arms (gdb call) | returned |
|---|---|---|
| F40C | alt 0 / alt 1 + token / alt 1 + token + inactive / + dead / alt 1 + null token (fake calloc'd TLockedToken, mItem 0x1234) | base, 0x1234, base, base, base |
| FF20 | 0xCD0 = 1 / 0x980 = 0x800000 / 0x980 = 0x20 | &mGuestCD4, &mEnrageVulnerability (combo slot 3 = index 4), the CAi member |
| 1C6C8 | 8 and 0x10 with 0xBAE 0/1, 0x40, 0x80, 0x100, 0x800000 with 0xFB8 0/1/2/-1, 0x20, 0 | sc1, sc2, missile, missile, super, phazon, phazon super, null x4 |

- H (host copy; the tsv state names each, 41 rows): the CDarkSamus dtor's member dtors (1F534
  1F588 1F5DC 1F650 1F9C0 1F928 1F87C E4DC 1F6A4 1F4B0) and 1F41C (CPatterned 0x620 knockback
  manager, from 1F238); temporaries and containers (2040 2098 B670 1AFB4 1B05C 4D9C 4DF0 3628 36E4
  4468 220EC 10010 1B938); header ctors / copies / assigns (A08 2C8 A60 3744 FFB4 3F70 207BC 20938
  5038 4EFC 20DA0 20CC4 8D30 EED8 1B64). 20C2C: valid=0 at 0x00020C4C and assign + Lock only when
  the asset != -1 (0x00020C38), which the host ctor (PortCDarkSamus.cpp:192-240) does; no struct
  fix needed. 21D14: the 'FSM2' (0x46534D32) TToken of the typedef's stateMachine2 (+0x1E4) into
  0x94C, which host CAi (CAi.cpp:24-29) loads.
- Follow-up: the tsv calls E7C "unreferenced", but it is the scattershot Render vtable entry (1B64's
  caller).

## Slice ds43 (2026-10-22): the next B rows (16 marked, 17 H, 1 T)

- Marked: 100DC F488 F100 1EE4C 11DDC 11EB8 1CDD0 12960 1E3C0 1D860 B6C4 150DC C2B0 C104 14A00
  22020. Every body was read against the asm, including the NaN senses.
- Fixes: 0xE04 and 0x9C0 now start at -1000.0f (`stfs f1, 0x14(r31)` at 0x0000EE34 in EDF4,
  `stfs f0, 0x1c(r31)` at 0x0000EAAC in EA24). 1EE4C tests the weapon mode's comboed bit (`lbz r0,
  0x2(r29)` + `extrwi r3, r0, 1, 25` at 0x0001EEDC). 22020 is now `RunGetupCmd`, moved out of GetUp.
  B6C4's best-distance test stays `!(distSq < best)`: the `bge` at 0x0000B7EC branches when cr0.lt
  is clear, unordered included, so a NaN distance is skipped.
- Dead arms: 22020's default arm (0x00022050 `b`, 0x00022058 `bge`); 14A00's null object
  (`mr. r6, r3` at 0x00014BE4): the uids come from BuildNearList in the same frame.
- C104: retail interpolates the rays with `fmadds`; the host multiplies, then adds. This is a known
  fma knife-edge, as in the mathdiff ruling; no change.
- H (tsv state "ds43: ..."): 3568 43A8 20600 B570 37A0 DE64 E124 1F238 1F72C 217FC 215FC 20F50 8DB4
  EDF4 EA24 E560 13FFC. E560: the PhazonIndirectTexture token 0xD04 is never read; only the
  0x1FED8 ctor and the 0x1F0D0 dtor touch 0xCD0. 153EC SideDash is T: it has no FSM2 node (as
  EDB8).
- Left: B86C (one forced call; five arms still short: the start tests, the arrived-link loop and
  its null test). E23C is the loader data-block ctor (0x490 bytes, from 0x0000CF64) and has not
  been checked field by field. CEB4 needs team AI.
- Runs: `build-port-cov`, rooftop, `tools/drivers/darksamus_ds43.con` (GetUp statecalls at f5650).
  gdb stops at the 4600th Think and runs `tools/cov/specs/darksamus_ds43_calls.gdb`; the .py
  forces return values through gated breakpoints. Every member it writes is restored. 0
  caught/reach-stub.

      COV_GDB_SCRIPT=$PWD/tools/cov/specs/darksamus_ds43_calls.py BUILD=$PWD/build-port-cov \
        MP2_INPUT=LAYER2@400 tools/cov/force.sh tools/cov/specs/darksamus_ds43_calls.py <out> \
        "1baa96c2,10_cliff_rooftop" --con tools/drivers/darksamus_ds43.con --fast --frames 5900

| fn | forced (gdb call / forced return) |
|---|---|
| F488 / 100DC / F100 / 1EE4C | as in the .gdb comments (inactive, die/hold, phazon mark; CD0/enrage; 11 uids + hit; reflect gates) |
| 11EB8 / 1CDD0 / 11DDC / 12960 | stuck timer; flat normal; sent/bounced/no stage/+0x59; on-actor target, ring paths false |
| 1D860 | FindBestAnimation score 0; Deactivate with -1 |
| 1E3C0 | scored cover point inactive, then in another area |
| 150DC | no stage, double dash, stuck counts, FindBestCoverPoint -> 0xFFFF, PickDash -> 0 |
| B6C4 | hint inactive / type 0 / chain true (radius 1e6); GetObjectById -> null |
| C2B0 / C104 | gun forward (0,0,1); a stale end sfx handle; mSweepBeamPart 0 |

## Slice ds44 (2026-10-22)

- E23C = inline `SLdrDarkSamus()` (SLdrDarkSamus.hpp). Field map vs retail: all match except seven sound
  defaults the REL sets to -1 (`li r0,-0x1` at 0xE264/0xE2EC/0xE314/0xE338; stores 0x300/0x304 at
  0xE26C/0xE270, 0x3D0 at 0xE2F8, 0x408/0x40C at 0xE328/0xE32C, 0x430 at 0xE344, 0x43C at 0xE350).
  Fixed in a TARGET_PC block, marked. Editor 0x38=3, patterned +0x4C=2.0, +0x230=1 already match the sub-ctors.
- 1EF38 = CDarkSamus deleting dtor -> H (RAII owners; swooshes deleted in ~CDarkSamus).
- Read against the asm, no differences: B86C, 12F60, B164, C470, 1905C, 1D9A4. Not marked: live arms
  still untaken (forced ds43 run gcov). B164 `0.f > value` (Fsm.cpp ~1723) is dead: gated by `value > 0`
  at 0xB2C8 (retail arm 0xB2E8). Arms left to force:
  - B86C: non-hint start; type != 0x11; exit via null next (hint with no hint arrivals); inactive arrived hint.
  - 12F60: scatter uid not a CScattershotProjectile; chance > 1.
  - B164: FindBoostBallHint null; FindArrivedHint null; valueParm <= 0; mBoostHintUid invalid / stale.
  - C470: hit while mSweepBeamOn; decal token valid/invalid (CCollisionResponseData::mDecalTokens.at(1)).
  - 1905C: DS xy == player xy; valid ray hits (ForceRet RayWorldIntersection sret; dist > 9 for the 2 result).
  - 1D9A4: invalid uid; repulsor inactive/other area/repel-player/cylinder; strength null; path area null; obstruction > 0.
  - CA0C: Update with mSweepBeamOn, sfx valid; Deactivate with emitters set. Asm not yet compared.
- Not started: 1707C, 1C99C, 19B64, 154FC, 1FA14, CEB4.

## Slice ds45 (2026-10-23): 11 B rows marked (B86C 1D9A4 B164 12F60 C470 1905C CA0C 1707C 1C99C 19B64 CEB4)

- Harness: `tools/cov/specs/darksamus_ds44_calls.{py,gdb}` (gdb boots the game; at the 4600th
  CDarkSamus::Think the .gdb forces each arm with member writes and forced callee answers, then
  writes every member back). Run: see the .py header; one run gives all arms below.
- Read against the asm (no differences): CA0C, 1707C, 1C99C (.data C30=0x31 platform, C34=0x1D
  ceiling, C38=0x1E wall, C3C/C40=0x1F floor, C44/C48=0x20 player), 19B64, CEB4 (0x54..0x5C = mPosition).
- gcov (forced run): B86C, 1D9A4, 12F60, 1905C, 1707C, 1C99C, CA0C, 19B64 all arms; CEB4 no branches.
  B164 15/16: the `0.f > value` arm (0xB2E8) is dead under `value > 0` (0xB2C8); the retail_diff
  nan finding there is a false positive (NaN takes `ble` 0xB2CC to 1.0f, as the port's `>`).
  C470 14/17: left are the static filter's guard abort and the decal path's exception pads; the
  decal arm ran with the player power beam's CRSC type-1 entry swapped into the sweep CRSC (the
  sweep CRSC, mScatterShotInfo and mScatterShotInfo2 have no valid decals: data-dead in retail on
  the rooftop, live code).
- CA0C: the rooftop's sweep sounds give null handles; the remove arms ran with stand-in handles
  that StopSound ignores.
- asmcheck: only host copies (fn_10_2C8 CRayCastResult, fn_10_13FFC CPASAnimParmData, B570/B670
  vector, Lock = TLockedToken) and the 0xB20 dive bits (mDive bitfield names).
- Harness gotchas: `$cl` (and every x86 register name) is a gdb register, not a variable; gdb-called
  allocators handed back 32-bit-cut pointers here, so scratch memory comes from `ds45-scratch`.
- Left: 154FC PickDash (3 callers: 0x1505C, 0x15298 and SideDash 0x154A8, so not T) and 1FA14 (ctor).
