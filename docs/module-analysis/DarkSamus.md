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
