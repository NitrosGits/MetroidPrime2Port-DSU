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

The **33 local overrides** (slot → function) are the ones listed in the function map above:
2 `fn_10_1EF38` (dtor) · 3 `TypesMatch__10CDarkSamusCFi` · 5 `fn_10_1B1FC` · 6 `fn_10_1E89C` ·
9 `fn_10_18AB8` · 10 `fn_10_17F04` · 11 `fn_10_18964` · 15 `fn_10_0` · 16 `fn_10_100DC` ·
18 `fn_10_F5C0` · 20 `fn_10_10358` · 21 `fn_10_10210` · 24 `fn_10_F84C` · 28 `fn_10_1BEFC` ·
29 `fn_10_F40C` · 31 `fn_10_1AAC4` · 33 `fn_10_1C7FC` · 40 `fn_10_FF20` · 41 `fn_10_F488` ·
42 `fn_10_CE9C` · 43 `fn_10_CEA4` · 44 `fn_10_CEAC` · 45 `fn_10_CEB4` · 50 `fn_10_CE54` ·
55 `fn_10_1C6C8` · 56 `fn_10_CE4C` · 57 `fn_10_CE5C` · 61 `fn_10_CE64` · 67 `fn_10_CE6C` ·
68 `fn_10_CE7C` · 69 `fn_10_CE88` · 73 `fn_10_CE94` [all proven by the vtable entry].

Slots **82..173** are not vtable entries: 92 further function pointers (`fn_10_F840`, `fn_10_1BB08`,
`fn_10_62B4`, ... `fn_10_FE18`) sit inside the same label. They are the module's own state-machine
tables (the count objects `lbl_10_data_0`..`lbl_10_data_5C` hold 0x3B, 0x20, 0x32, 0x14, 0x21,
0x23, 0x14 - the trigger/state/code counts) [probable: the 82-entry prefix matches the chain
exactly, so the tail is the module's own tables].

## 2. CDarkSamus members (`CHECK_SIZEOF 0x1128`)

Size [proven]: the loader's `__nw__FUlPCcPCc` is `li r3, 0x1128` at 0x0000DA78. Base: `CPatterned`
(0x7c0) built with `EPatternedAI` **7** (`li r4, 0x7` at 0x0001FB00; the host's
`kPAI_DarkSamus = 7`), flavor 0 (`li r7, 0x0` at 0x0001FB08), movement 0 (ground), collider 1, body
**8** = the host's `kBT_4WayBlended` (the stack words at 0x0001FA84..0x0001FABC) [proven].

| off | member | proof / note |
|---|---|---|
| 0x7C0 | **`CPathFindSearch` (0xEC)** | `fn_8013E6A0(this+0x7C0, null, 0x401, pInfo+0xEC, 1.0f, 1.0f, 0, 0)`; `fn_10_CE4C` returns this+0x7C0. The flags are `li r5, 0x401` (0x0001FB54), the index the patterned info's pathfinding index (`lwz r6, 0xec(r30)`, 0x0001FB40), the two radii `lbl_10_rodata_3E8` = 1.0f (0x0001FB38/0x0001FB48), the area null (0x0001FB50) and the root position 0 = `kRP_Center` (0x0001FB5C) |
| 0x8AC | int = −1 | `stw r0(-1), 0x8ac` |
| 0x8B0..0x8C8 | seven zeroed floats | `sZeroVector` twice |
| 0x8CC | **`CTransform4f` = identity (0x30)** | `__ct__12CTransform4fFRC12CTransform4f(sIdentity)` at 0x0001FBC0 |
| 0x8FC / 0x900 / 0x904 | three zeroed floats | |
| 0x908 | float = the ctor's first float argument | `stfs f24, 0x908` (0x0001FBF0) |
| 0x90C | flag byte | three `rlwimi`s (bits 24, 25, 26) at 0x0001FBF8..0x0001FC18 |
| 0x910 / 0x914 / 0x918 | three zeroed floats | |
| 0x928 | byte 0 | |
| 0x92C | **`CToken`** | `fn_10_21D14(this+0x94C, ...)` builds the object at 0x94C; the token at 0x92C is the 'SCAN' tag [probable] |
| 0x930 | int 0 | |
| 0x94C | sub-object built by `fn_10_21D14` | [proven] call |
| 0x958 / 0x95C / 0x960 / 0x964 | floats | 0x960/0x964 are the ctor's float arguments |
| 0x968 | byte 0 | |
| 0x96C | float | |
| 0x970 | int = −1 | |
| 0x974 | `TUniqueId` = `kInvalidUniqueId` | `lhz`/`sth` at 0x0001FCC0/0x0001FCC4 |
| 0x978 | int 0 | |
| 0x97C | flag byte | two `rlwimi`s (bits 24, 25) at 0x0001FCD0/0x0001FCE0 |
| 0x980 / 0x984 | int 0 ×2 | |
| 0x988 | int 0 | |
| 0x98C / 0x990 | floats | |
| 0x994 | int 0 | |
| 0x9A4 | sub-object built by `fn_10_EA24` | [proven] call |
| 0x9E0 | sub-object built by `fn_10_15348` | [proven] call |
| 0xA0C | sub-object built by `fn_10_21A14` | [proven] call |
| 0xA4C | sub-object built by `fn_10_2199C` | [proven] call |
| 0xA88 | sub-object built by `fn_10_217FC` | [proven] call |
| 0xAC0 | sub-object built by `fn_10_E784` | [proven] call |
| 0xB50 | sub-object built by `fn_10_215FC` | [proven] call |
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
(`CBodyController::Activate` at 0x0001E9A4), which is a message the port's milestone-1
`AcceptScriptMsg` stand-in does not forward yet [proven: the arm, not its trigger].

What is **not** established:

1. The state machine: which of the 92 function pointers after the vtable are triggers, states and
   codes, and what they are named. The vtable keeps `CPatterned::SetupStateMachine` (slot 60), so
   the tables must be installed from `AcceptScriptMsg`'s `kSM_Create` arm or from `Think`.
2. The identities of the sixteen sub-objects the constructor builds (0x94C..0x1064) and which
   loader property each one holds. The ctor's ~50 stack arguments could not be mapped back to the
   loader's properties, so the host's `CDarkSamusData` names its fields after the properties
   instead.
3. What the byte at 0xCD0 means (it picks the collision response and the damage vulnerability) and
   which of the 0xCD0 sub-object's fields sets it.
4. The boss's combat behaviour: the melee, dive, scatter-shot, missile, freeze-beam, sweep-beam
   and boost-ball attacks, the phazon enrage and the death sequence.
