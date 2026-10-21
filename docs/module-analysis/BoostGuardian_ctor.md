# `CIngBoostBallGuardian` ctor `fn_30_EC6C` — argument analysis (REL module 30)

Source: `build/G2ME01/IngBoostBallGuardian/asm/auto_00_00000000_text.s`, `.fn fn_30_EC6C` at
[0x0000EC6C], loader `fn_30_130` calls it at [0x0000051C].

Frame: `stwu r1, -0x350(r1)` [0x0000EC6C] → the two incoming stack arguments live at
0x358(r1) (caller SP+8) and 0x35c(r1) (caller SP+12); the callee's own stack arguments go to
its SP+8, +12, … (i.e. `0x8(r1)`, `0xc(r1)`, …).
ABI note: only **r3..r10** are argument registers (r11/r12 are volatile scratch — that is what
makes the `mr r11,r7` / `mr r12,r6` pairs harmless), so a 12-parameter callee puts params 8..12
on the stack.

## 1. Parameter list of `fn_30_EC6C`, in order

| # | reg / slot | type | what the body does with it | first use |
|---|---|---|---|---|
| 1 | r4 | **pointer** to a 2-byte value (`TUniqueId`; mangled `9TUniqueId` with no `R`, so by value in source, invisible-reference in the ABI) | `lhz r0, 0x0(r4)` loads the u16 [0x0000EC84]; the u16 is copied to the local `r1+0x58` [0x0000ECB0] and *that copy's address* is handed to `CPatterned` as its uid | [0x0000EC84] |
| 2 | r5 | `const rstl::string&` (name) | moved unchanged into `CPatterned`'s `const rstl::basic_string&` slot [0x0000EC80]; no other use in the body | [0x0000EC80] |
| 3 | r6 | `const CEntityInfo&` | `CPatterned` param 5: `mr r12,r6` [0x0000EC74] → `mr r8,r12` [0x0000ECA4] | [0x0000EC74] |
| 4 | r7 | `const CTransform4f&` | `CPatterned` param 6: `mr r11,r7` [0x0000EC78] → `mr r9,r11` [0x0000ECD0] | [0x0000EC78] |
| 5 | r8 | `const CModelData&` | `CPatterned` param 7: `mr r10,r8` [0x0000EC94] | [0x0000EC94] |
| 6 | r9 | `const CActorParameters&` | `CPatterned` param 12 (stack slot +16): `stw r9, 0x18(r1)` [0x0000ECCC] | [0x0000ECCC] |
| 7 | r10 | `const CPatternedInfo&` | `CPatterned` param 8 (stack slot +0): `mr r25,r10` [0x0000EC90] → `stw r25, 0x8(r1)` [0x0000ECB8]; also `lwz r6, 0xec(r25)` = pathfinding index fed to the `CPathFindSearch` ctor [0x0000ED0C] | [0x0000EC90] |
| 8 | stack+0 → 0x358(r1) | `const <0x320-byte struct>&` (type name **?**) | copied into `this+0x7C0` by `fn_30_1056C` [0x0000ECDC]; its asset-id fields +0x40 / +0x50 / +0x20 / +0x24 are read and turned into `CToken` members (`this+0xFDC`, `0xFEC`, `0x1000`, …) [0x0000EDC0], [0x0000EF2C], [0x0000F0C0], [0x0000F228] | [0x0000EC98] |
| 9 | stack+4 → 0x35c(r1) | `const <0x110-byte struct>&` (type name **?**) | passed on to `fn_30_10250(this+0xDEC, arg9)` [0x0000ED60] | [0x0000EC9C] |

No argument is `unused` — all nine are read.

## 2. The `CPatterned::CPatterned(...)` call at [0x0000ECD4]

Setup immediately before the `bl` (callee = `__ct__10CPatternedF12EPatternedAI9TUniqueIdRCQ24rstl66basic_string<c,…>Q210CPatterned11EFlavorTypeRC11CEntityInfoRC12CTransform4fRC10CModelDataRC14CPatternedInfoQ210CPatterned13EMovementTypeQ210CPatterned13EColliderType9EBodyTypeRC16CActorParameters`):

| callee slot | value | source | addr |
|---|---|---|---|
| r3 = `this` | `this` | unchanged since entry (`mr r31,r3`) | [0x0000ECA0] |
| r4 = param1 `EPatternedAI` | `0x19` (25) | constant `li r4, 0x19` | [0x0000ECC0] |
| r5 = param2 `TUniqueId` | `&r1+0x58` | local copy of arg 1 (`addi r5,r1,0x58`; copy made at [0x0000ECB0]) | [0x0000ECA8] |
| r6 = param3 `const rstl::basic_string&` | **arg 2** | `mr r6, r5` | [0x0000EC80] |
| r7 = param4 `EFlavorType` | `0` (`kFT_Zero`) | constant `li r7, 0x0` | [0x0000ECAC] |
| r8 = param5 `const CEntityInfo&` | **arg 3** | `mr r12,r6` [0x0000EC74] → `mr r8,r12` | [0x0000ECA4] |
| r9 = param6 `const CTransform4f&` | **arg 4** | `mr r11,r7` [0x0000EC78] → `mr r9,r11` | [0x0000ECD0] |
| r10 = param7 `const CModelData&` | **arg 5** | `mr r10, r8` | [0x0000EC94] |
| SP+8 (stack+0) = param8 `const CPatternedInfo&` | **arg 7** | `mr r25,r10` [0x0000EC90] → `stw r25, 0x8(r1)` | [0x0000ECB8] |
| SP+12 (stack+4) = param9 `EMovementType` | `0` (`kMT_Ground`) | constant `li r4, 0x0` [0x0000EC88] → `stw r4, 0xc(r1)` | [0x0000ECBC] |
| SP+16 (stack+8) = param10 `EColliderType` | `1` (`kCT_One`) | constant `li r0, 0x1` [0x0000ECB4] → `stw r0, 0x10(r1)` | [0x0000ECC4] |
| SP+20 (stack+12) = param11 `EBodyType` | `1` (`kBT_BiPedal`) | constant `li r0, 0x1` → `stw r0, 0x14(r1)` | [0x0000ECC8] |
| SP+24 (stack+16) = param12 `const CActorParameters&` | **arg 6** | `stw r9, 0x18(r1)` | [0x0000ECCC] |

`0x19` is not present in the port's `EPatternedAI` enum (`0x1a` = `kPAI_IngSpaceJumpGuardian`,
`0x1b` = `kPAI_IngSpiderballGuardian`) → the constant's *name* is **?**.

## 3. Pointer or by value?

* **Arg 1 (r4) is a pointer**, not a value: the body does `lhz r0, 0x0(r4)` [0x0000EC84]
  (a 16-bit load *through* the register) and then copies the u16 into a local
  (`sth r0, 0x58(r1)` [0x0000ECB0]); the *address* of that local is what is forwarded
  (`addi r5, r1, 0x58` [0x0000ECA8]). The retail `TUniqueId` is a class type
  (mangled `9TUniqueId`, no `R`), so it is a by-value parameter that the old GCC ABI passes
  as an invisible reference. The caller makes the same move: it stores the `AllocateUniqueId`
  result as a u16 (`sth r0, 0x4c(r1)` [0x000004CC]) and passes `&r1+0x4c` [0x00000500].
* **Args 2..7 and 9 are references** (pointers that are never dereferenced or copied by the
  ctor): each is passed straight through into the matching `const …&` slot of `CPatterned`
  (arg 2 → [0x0000EC80], arg 3 → [0x0000ECA4], arg 4 → [0x0000ECD0], arg 5 → [0x0000EC94],
  arg 6 → [0x0000ECCC], arg 7 → [0x0000ECB8]).
* **Arg 8 is a reference** to a 0x320-byte struct: it is read *through* the pointer
  (`lwz r5, 0x40(r30)` [0x0000EDC0], `lwz r5, 0x50(r30)` [0x0000EF2C]) and copied into
  `this+0x7C0` (`mr r4, r30` [0x0000ECDC]).
* **Arg 9 is a reference** to a 0x110-byte struct: only its address is taken
  (`mr r4, r24` [0x0000ED60]).

## 4. Caller-side cross-check (`fn_30_130`, call at [0x0000051C])

Confirmed: r4 = `&r1+0x4C` (uid) [0x00000500]; r6 = `LdrToEntityInfo` result [0x000004F0];
r9 = `&r1+0x270` (`LdrToActorParameters`) [0x00000514]; r10 = `&r1+0x608`
(`LdrToPatternedInfo`) [0x00000518]; stack+0 = `&r1+0x8BC` (0x320 B, destroyed by
`fn_30_2C44` [0x000005A4]) [0x000004F4]; stack+4 = `&r1+0x41C` (0x110 B, destroyed by
`fn_30_5E4` [0x00000540]) [0x00000504].

Corrections to the brief's caller list:
* **r5 is not the transform.** `LdrToTransform4f` writes its result to `r1+0xC0`
  (`addi r3, r1, 0xc0` [0x000004D4]), and `r1+0xC0` is passed as **arg 4 (r7)**
  [0x0000050C]. `r5` = `r1+0xBE0` [0x00000508] is the `SLdrEditorProperties` (loaded at
  [0x00000200], destroyed at [0x000005C8]); its **first member is `rstl::string name`**
  (`include/MetroidPrime/ScriptLoader/Structs/SLdrEditorProperties.hpp:24`), so the same
  address serves as arg 2, the name string.
* **r7 is not the model data**, it is the `CTransform4f` (see above); **r8 is the one and
  only `CModelData`** (`LdrToModelData` fills `r1+0x2D0`, moved to `r1+0x320` by `fn_30_10DC`
  [0x00000288]; destroyed at [0x000005BC]).
