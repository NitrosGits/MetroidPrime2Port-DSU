# IngBoostBallGuardian (REL module 30) — object layout, extracted from asm

Module: `G2ME01/IngBoostBallGuardian` (REL 30), class `CIngBoostBallGuardian`.
Size 0x11F0, base `CPatterned` = 0x7C0 bytes (occupies 0x0000-0x7BFB), so this
class's own members start at 0x07C0.

Sources (read-only):
- `build/G2ME01/IngBoostBallGuardian/asm/auto_00_00000000_text.s` — ctor `fn_30_EC6C`
  (0xEC6C-0x101A4), dtor `fn_30_E5EC` (0xE5EC-0xE850), helper dtors `fn_30_2C44`,
  `fn_30_5E4`, `fn_30_E4EC`, `fn_30_EACC`, member ctor `fn_30_10250` (0x10250),
  member ctor `fn_30_1056C` (0x1056C).
- `.../asm/auto_03_00000000_rodata.s` — float/string constants.
- `.../asm/auto_04_00000000_data.s` — vtable `lbl_30_data_9C0`.

Conventions: `this` = r31 (from `mr r31, r3` at [0x0000ECA0]). r3=this, r4..r10 =
args 1..7, stack args at 0x358(r1)=arg 8, 0x35C(r1)=arg 9. `stb`=1 byte,
`sth`=2 bytes, `stw`/`stfs`=4 bytes. "width" below = bytes covered by the store.
`?` = value not determinable from the asm alone. Every row cites its asm address.

`lbl_30_rodata_64` = 1.0f, `_70` = 0.0f, `_8C` = 5.0f, `_98` = 1.5f,
`_FC` = 0.75f, `_124` = 0.1f, `_128` = 0.05f, `_CC` = 100.0f.
`kInvalidUniqueId` and `mskNullBox__6CAABox` are external (other module).

## 1. Member table (ascending offset)

Rows marked `DOL` are not stored directly by this ctor; the ctor calls a
constructor on that address (see section 2) — they are listed so the table is
complete and section 3 sizes are computable.

| offset | width | value written / where it comes from | asm |
|---|---|---|---|
| 0x0000 | 4 | vptr = `lbl_30_data_9C0` | [0x0000ECE8] |
| 0x07C0 | DOL | member ctor `fn_30_1056C` (copies 0x320 bytes from stack arg 8) | [0x0000ECEC] |
| 0x0AEC | DOL | `__ct__15CPathFindSearchFP7CPFAreaUiiffUiQ29CPFRegion13ERootPosition` | [0x0000ED24] |
| 0x0AE4 | 4 | 2 (`li r0,0x2`) | [0x0000ED00] |
| 0x0AE8 | 4 | -1 (`li r0,-0x1`) | [0x0000ED10] |
| 0x0BBC | 4 | float: `lbl_30_rodata_8C` (5.0f) * `*(float*)(*(u32*)(this+0x60)+0x8)` | [0x00010190] |
| 0x0BC0 | 4 | float: `lbl_30_rodata_A4` (3.0f) * `*(float*)(*(u32*)(this+0x60)+0x4)` | [0x0001018C] |
| 0x0BD8 | DOL | `__ct__20CPathFindPointSearchFP7CPFArea` | [0x0000ED30] |
| 0x0D60 | 4 | 0 (null) | [0x0000ED3C] |
| 0x0D64 | 4 | 0 (null) | [0x0000ED40] |
| 0x0D68 | DOL | `__ct__23CSurfaceAlignmentHelperFv` | [0x0000ED44] |
| 0x0DC0 | DOL | `__ct__26CIngSpotPathFindNavigationFv` | [0x0000ED4C] |
| 0x0DD0 | 4 | `mskNullBox__6CAABox` word 0 (0.0f) | [0x0000ED70] |
| 0x0DD4 | 4 | `mskNullBox__6CAABox` word 1 | [0x0000ED74] |
| 0x0DD8 | 4 | `mskNullBox__6CAABox` word 2 | [0x0000ED7C] |
| 0x0DDC | 4 | `mskNullBox__6CAABox` word 3 | [0x0000ED88] |
| 0x0DE0 | 4 | `mskNullBox__6CAABox` word 4 | [0x0000ED8C] |
| 0x0DE4 | 4 | `mskNullBox__6CAABox` word 5 (0xDD0-0x0DE7 = one 24-byte CAABox) | [0x0000ED94] |
| 0x0DE8 | 1 | 1 | [0x0000ED58] |
| 0x0DEC | DOL | member ctor `fn_30_10250` (copies 0x110 bytes from stack arg 9) | [0x0000ED98] |
| 0x0FDC | DOL | `__ct__6CTokenFRC6CToken` | [0x0000EE98] |
| 0x0FD8 | 2 | `kInvalidUniqueId` (u16) | [0x0000EDAC] |
| 0x0FDA | 2 | `kInvalidUniqueId` (u16) | [0x0000EDBC] |
| 0x0FE8 | 1 | valid-flag byte (+0xC) of the CToken at 0x0FDC, from stack temp CToken r1+0x314 | [0x0000EE78] |
| 0x0FEC | DOL | `__ct__6CTokenFRC6CToken` | [0x0000F014] |
| 0x0FF8 | 1 | valid-flag byte (+0xC) of the CToken at 0x0FEC, from stack temp CToken r1+0x2E8 | [0x0000EFF4] |
| 0x0FFC | 4 | 0 | [0x0000F0B0] |
| 0x1000 | DOL | `__ct__6CTokenFRC6CToken` | [0x0000F194] |
| 0x100C | 1 | valid-flag byte (+0xC) of the CToken at 0x1000, from stack temp CToken r1+0x2BC | [0x0000F174] |
| 0x1010 | DOL | `__ct__6CTokenFRC6CToken` | [0x0000F310] |
| 0x101C | 1 | valid-flag byte (+0xC) of the CToken at 0x1010, from stack temp CToken r1+0x290 | [0x0000F2F0] |
| 0x1020 | DOL | `__ct__6CTokenFRC6CToken` | [0x0000F48C] |
| 0x102C | 1 | valid-flag byte (+0xC) of the CToken at 0x1020, from stack temp CToken r1+0x264 | [0x0000F46C] |
| 0x1030 | DOL | `__ct__6CTokenFRC6CToken` | [0x0000F608] |
| 0x103C | 1 | valid-flag byte (+0xC) of the CToken at 0x1030, from stack temp CToken r1+0x238 | [0x0000F5E8] |
| 0x1040 | DOL | `__ct__6CTokenFRC6CToken` | [0x0000F784] |
| 0x104C | 1 | valid-flag byte (+0xC) of the CToken at 0x1040, from stack temp CToken r1+0x20C | [0x0000F764] |
| 0x1050 | DOL | `__ct__6CTokenFRC6CToken` | [0x0000F900] |
| 0x105C | 1 | valid-flag byte (+0xC) of the CToken at 0x1050, from stack temp CToken r1+0x1E0 | [0x0000F8E0] |
| 0x1060 | DOL | `__ct__6CTokenFRC6CToken` | [0x0000FA7C] |
| 0x106C | 1 | valid-flag byte (+0xC) of the CToken at 0x1060, from stack temp CToken r1+0x1B4 | [0x0000FA5C] |
| 0x1070 | 4 | 0 | [0x0000FB18] |
| 0x1074 | 2 | `kInvalidUniqueId` (u16) | [0x0000FB38] |
| 0x1076 | 2 | `kInvalidUniqueId` (u16) | [0x0000FB60] |
| 0x1078 | 4 | 0 | [0x0000FB68] |
| 0x107C | 4 | 0 | [0x0000FB6C] |
| 0x1080 | 4 | float `lbl_30_rodata_70` (0.0f) | [0x0000FB70] |
| 0x1084 | 2 | `kInvalidUniqueId` (u16) | [0x0000FB78] |
| 0x1086 | 2 | `kInvalidUniqueId` (u16) | [0x0000FB80] |
| 0x1088 | 2 | `kInvalidUniqueId` (u16) | [0x0000FB88] |
| 0x108A | 2 | `kInvalidUniqueId` (u16) | [0x0000FB90] |
| 0x108C | 2 | `kInvalidUniqueId` (u16) | [0x0000FB98] |
| 0x108E | 2 | `kInvalidUniqueId` (u16) | [0x0000FBA0] |
| 0x1090 | 2 | `kInvalidUniqueId` (u16) | [0x0000FBA8] |
| 0x1092 | 2 | `kInvalidUniqueId` (u16) | [0x0000FBB0] |
| 0x1094 | 2 | `kInvalidUniqueId` (u16) | [0x0000FBB8] |
| 0x1096 | 2 | `kInvalidUniqueId` (u16) | [0x0000FBC0] |
| 0x1098 | 2 | `kInvalidUniqueId` (u16) | [0x0000FBC8] |
| 0x10A0 | 4 | 0 | [0x0000FBCC] |
| 0x10A4 | 4 | 0 | [0x0000FBD0] |
| 0x10A8 | 4 | 0 | [0x0000FBD4] |
| 0x10AC | 1 | 0xFF, then overwritten with the u8 result of `GetLocatorSegId__9CAnimDataCFRC...` | [0x0000FBD8] / [0x0000FFAC] |
| 0x10AD | 1 | 0xFF, then overwritten with a `GetLocatorSegId__9CAnimDataCFRC...` result | [0x0000FBDC] / [0x0000FF78] |
| 0x10AE | 1 | 0xFF, then overwritten with a `GetLocatorSegId__9CAnimDataCFRC...` result | [0x0000FBE0] / [0x0000FFE0] |
| 0x10AF | 1 | 0xFF, then overwritten with a `GetLocatorSegId__9CAnimDataCFRC...` result | [0x0000FBE4] / [0x00010014] |
| 0x10B0 | 1 | 0xFF, then overwritten with a `GetLocatorSegId__9CAnimDataCFRC...` result | [0x0000FBE8] / [0x00010048] |
| 0x10B1 | 1 | 0xFF, then overwritten with a `GetLocatorSegId__9CAnimDataCFRC...` result | [0x0000FBEC] / [0x0001007C] |
| 0x10B2 | 1 | 0xFF, then overwritten with a `GetLocatorSegId__9CAnimDataCFRC...` result | [0x0000FBF0] / [0x000100B0] |
| 0x10B3 | 1 | 0xFF, then overwritten with a `GetLocatorSegId__9CAnimDataCFRC...` result | [0x0000FBF4] / [0x000100E4] |
| 0x10B4 | 1 | 0xFF, then overwritten with a `GetLocatorSegId__9CAnimDataCFRC...` result | [0x0000FBF8] / [0x00010118] |
| 0x10B5 | 1 | 0xFF, then overwritten with a `GetLocatorSegId__9CAnimDataCFRC...` result | [0x0000FBFC] / [0x0001014C] |
| 0x10B8 | DOL | `__ct__19CLineOfSightTrackerF9TUniqueId6CSegIdff` | [0x0000FC10] |
| 0x10BA | 1 | copy of the byte at 0x10AD | [0x00010164] |
| 0x10F8 | 4 | float `sZeroVector__9CVector3f[0]` (0.0f) | [0x0000FC34] |
| 0x10FC | 4 | float `sZeroVector__9CVector3f[1]` (0.0f) | [0x0000FC48] |
| 0x1100 | 4 | float `sZeroVector__9CVector3f[2]` (0.0f) | [0x0000FC50] |
| 0x1104 | 4 | float `sZeroVector__9CVector3f[0]` (0.0f) | [0x0000FC58] |
| 0x1108 | 4 | float `sZeroVector__9CVector3f[1]` (0.0f) | [0x0000FC60] |
| 0x110C | 4 | float `sZeroVector__9CVector3f[2]` (0.0f) | [0x0000FC68] |
| 0x1110 | 4 | float `sZeroVector__9CVector3f[0]` (0.0f) | [0x0000FC70] |
| 0x1114 | 4 | float `sZeroVector__9CVector3f[1]` (0.0f) | [0x0000FC78] |
| 0x1118 | 4 | float `sZeroVector__9CVector3f[2]` (0.0f) | [0x0000FC80] |
| 0x111C | 4 | -1 | [0x0000FC84] |
| 0x1120 | 4 | float: x of `Normalize__9CVector3fFv` of (0.0f, 0.0f, 1.0f) (rodata_70, rodata_70, rodata_64; `mPortalPlaneNormal`) | [0x0000FCA8] |
| 0x1124 | 4 | float: y of that normalized vector | [0x0000FCC8] |
| 0x1128 | 4 | float: z of that normalized vector | [0x0000FCD4] |
| 0x112C | 4 | float: dot of the translation (this+0x54/0x58/0x5C) with the normalized dir (`mPortalPlaneConstant`) | [0x0000FCFC] |
| 0x1130 | 4 | 0 | [0x0000FD00] |
| 0x1158 | 4 | -1 | [0x0000FD04] |
| 0x115C | 4 | 0 | [0x0000FD08] |
| 0x1160 | 4 | float `sZeroVector__9CVector3f[0]` (0.0f) | [0x0000FD10] |
| 0x1164 | 4 | float `sZeroVector__9CVector3f[1]` (0.0f) | [0x0000FD18] |
| 0x1168 | 4 | float `sZeroVector__9CVector3f[2]` (0.0f) | [0x0000FD20] |
| 0x116C | 4 | float `sZeroVector__9CVector3f[0]` (0.0f) | [0x0000FD28] |
| 0x1170 | 4 | float `sZeroVector__9CVector3f[1]` (0.0f) | [0x0000FD30] |
| 0x1174 | 4 | float `sZeroVector__9CVector3f[2]` (0.0f) | [0x0000FD38] |
| 0x1178 | DOL | `__ct__20CDamageVulnerabilityFRC24CWeaponTypeVulnerability` | [0x0000FD48] |
| 0x11A8 | 4 | float `*(float*)(this+0x3C8)` (base-class field) | [0x0000FD64] |
| 0x11AC | 4 | float `lbl_30_rodata_98` (1.5f) | [0x0000FD74] |
| 0x11B0 | 4 | float `lbl_30_rodata_FC` (0.75f) | [0x0000FD84] |
| 0x11B4 | 4 | float `lbl_30_rodata_64` (1.0f) | [0x0000FD94] |
| 0x11B8 | 4 | float `lbl_30_rodata_70` (0.0f) | [0x0000FD98] |
| 0x11BC | 4 | float `lbl_30_rodata_70` (0.0f) | [0x0000FD9C] |
| 0x11C0 | 4 | float `lbl_30_rodata_70` (0.0f) | [0x0000FDA0] |
| 0x11C4 | 4 | float `lbl_30_rodata_64` (1.0f) | [0x0000FDA4] |
| 0x11C8 | 4 | float `lbl_30_rodata_70` (0.0f) | [0x0000FDA8] |
| 0x11CC | 4 | 0 (index into the 0x38-stride array at this+0xF2C+4) | [0x0000FDAC] |
| 0x11D0 | 4 | this + 0xF2C + 4 (element 0 of that array) | [0x0000FDC0] |
| 0x11D4 | 4 | float `lbl_30_rodata_70` (0.0f) | [0x0000FDC4] |
| 0x11D8 | 4 | float `lbl_30_rodata_70` (0.0f) | [0x0000FDC8] |
| 0x11DC | 4 | float `lbl_30_rodata_70` (0.0f) | [0x0000FDCC] |
| 0x11E0 | 4 | float `lbl_30_rodata_70` (0.0f) | [0x0000FDD0] |
| 0x11E4 | 4 | float `*(float*)(this+0xF30)` (first float of the array element at 0x11D0) | [0x0000FDDC] |
| 0x11E8 | 4 | float `lbl_30_rodata_70` (0.0f) | [0x0000FDE0] |
| 0x11EC | 1 | bitfield byte, read-modify-written: bit31=1, bit27=1, bits 24-26/28-30=0 | [0x0000FDEC] .. [0x0000FE40] |
| 0x11ED | 1 | bitfield byte: bit25 = byte at (stack arg 8)+0x320, bits 24/26-31=0 | [0x0000FE54] .. [0x0000FEAC] |
| 0x11EE | 1 | bitfield byte: bit26=1, all other bits 0 | [0x0000FEB8] .. [0x0000FF0C] |
| 0x11EF | 1 | bitfield byte: bit26=1, bit27=1, bits 24/25/28-31=0 | [0x0000FF18] .. [0x0000FF3C] |

## 2. DOL constructor calls in the ctor

| member offset | called constructor | asm |
|---|---|---|
| 0x07C0 | `fn_30_1056C` (copy-style ctor, src = stack arg 8) | [0x0000ECEC] |
| 0x0AEC | `__ct__15CPathFindSearchFP7CPFAreaUiiffUiQ29CPFRegion13ERootPosition` | [0x0000ED24] |
| 0x0BD8 | `__ct__20CPathFindPointSearchFP7CPFArea` | [0x0000ED30] |
| 0x0D68 | `__ct__23CSurfaceAlignmentHelperFv` | [0x0000ED44] |
| 0x0DC0 | `__ct__26CIngSpotPathFindNavigationFv` | [0x0000ED4C] |
| 0x0DEC | `fn_30_10250` (copy-style ctor, src = stack arg 9) | [0x0000ED98] |
| 0x0FDC | `__ct__6CTokenFRC6CToken` | [0x0000EE98] |
| 0x0FEC | `__ct__6CTokenFRC6CToken` | [0x0000F014] |
| 0x1000 | `__ct__6CTokenFRC6CToken` | [0x0000F194] |
| 0x1010 | `__ct__6CTokenFRC6CToken` | [0x0000F310] |
| 0x1020 | `__ct__6CTokenFRC6CToken` | [0x0000F48C] |
| 0x1030 | `__ct__6CTokenFRC6CToken` | [0x0000F608] |
| 0x1040 | `__ct__6CTokenFRC6CToken` | [0x0000F784] |
| 0x1050 | `__ct__6CTokenFRC6CToken` | [0x0000F900] |
| 0x1060 | `__ct__6CTokenFRC6CToken` | [0x0000FA7C] |
| 0x10B8 | `__ct__19CLineOfSightTrackerF9TUniqueId6CSegIdff` | [0x0000FC10] |
| 0x1178 | `__ct__20CDamageVulnerabilityFRC24CWeaponTypeVulnerability` | [0x0000FD48] |

Base class: `__ct__10CPatternedF12EPatternedAI9TUniqueIdRCQ24rstl66basic_string<c,...>...`
is called on `this` (r3 = this) at [0x0000ECD4].

## 3. Minimum sizes (next higher member offset - this offset)

| offset | next higher member | min size |
|---|---|---|
| 0x07C0 | 0x0AEC | 0x32C (ctor copies only 0x320 bytes; 4 bytes padding) |
| 0x0AEC | 0x0AE4 | 0xF8 |
| 0x0BD8 | 0x0D60 | 0x188 |
| 0x0D68 | 0x0DC0 | 0x58 |
| 0x0DC0 | 0x0DD0 | 0x10 |
| 0x0DEC | 0x0FDC | 0x110 (dtor also destroys a CDamageVulnerability at +0x110 and an object at +0x4C) |
| 0x0FDC | 0x0FEC | 0x10 (CToken; its valid byte is at +0xC = 0x0FE8) |
| 0x0FEC | 0x1000 | 0x10 |
| 0x1000 | 0x1010 | 0x10 |
| 0x1010 | 0x1020 | 0x10 |
| 0x1020 | 0x1030 | 0x10 |
| 0x1030 | 0x1040 | 0x10 |
| 0x1040 | 0x1050 | 0x10 |
| 0x1050 | 0x1060 | 0x10 |
| 0x1060 | 0x1070 | 0x10 |
| 0x10B8 | 0x10F8 | 0x40 |
| 0x1178 | 0x11F0 (end of object) | 0x78 |

Other spans implied by section 1: 0x0DD0-0x0DE7 = 24-byte CAABox; 0x1070-0x107F =
8 bytes; 0x1084-0x1099 = 11 x u16; 0x10AC-0x10B5 = 10 x u8; 0x10F8-0x111B =
3 x CVector3f; 0x1160-0x1177 = 2 x CVector3f; 0x11A8-0x11CF = 10 x float;
0x11EC-0x11EF = 4 x u8 flag bytes.

## 4. Destructor

Class destructor: `fn_30_E5EC` at [0x0000E5EC]. It is vtable word 2 of
`lbl_30_data_9C0` (`auto_04_00000000_data.s`); words 0 and 1 of that object are
0x00000000 (offset-to-top / typeinfo slots), so `fn_30_E5EC` is virtual-function
index 0 = the deleting destructor (r3 = this, r4 = deleting flag).

DOL destructors it calls, in call order:

| offset | destructor | asm |
|---|---|---|
| 0x1178 | `__dt__20CDamageVulnerabilityFv` | [0x0000E620] |
| 0x109C | `fn_30_E4EC` (frees `*(u32*)(this+0x10A8)`, then the object itself) | [0x0000E62C] |
| 0x1060 | `__dt__6CTokenFv` | [0x0000E658] |
| 0x1050 | `__dt__6CTokenFv` | [0x0000E684] |
| 0x1040 | `__dt__6CTokenFv` | [0x0000E6B0] |
| 0x1030 | `__dt__6CTokenFv` | [0x0000E6DC] |
| 0x1020 | `__dt__6CTokenFv` | [0x0000E708] |
| 0x1010 | `__dt__6CTokenFv` | [0x0000E734] |
| 0x1000 | `__dt__6CTokenFv` | [0x0000E760] |
| 0x0FEC | `__dt__6CTokenFv` | [0x0000E78C] |
| 0x0FDC | `__dt__6CTokenFv` | [0x0000E7B8] |
| 0x0DEC | `fn_30_5E4` | [0x0000E7C4] |
| 0x0D68 | `fn_30_EACC` | [0x0000E7D0] |
| 0x0D64 | `__dt__22CCollisionActorManagerFv` (r4 = 1, deleting) | [0x0000E7E4] |
| 0x0D60 | `__dt__22CCollisionActorManagerFv` (r4 = 1, deleting) | [0x0000E7F8] |
| 0x0A5C | `__dt__20CDamageVulnerabilityFv` (inside the 0x07C0 member) | [0x0000E80C] |
| 0x07C0 | `fn_30_2C44` | [0x0000E818] |
| 0x0000 | `fn_30_E854` (base `CPatterned` destructor) | [0x0000E824] |
| 0x0000 | `Free__7CMemoryFPCv` (only when the deleting flag is set) | [0x0000E834] |

No destructor is called for the members at 0x0AEC, 0x0BD8, 0x0DC0, 0x10B8.

## Notes / caveats

- Offsets 0x10AC-0x10B5 are written twice (0xFF first, then the locator-segment
  byte) and 0x11EC-0x11EF are written repeatedly (bit by bit) — always with the
  same width, so no offset is written with two different widths.
- The ctor never writes 0x109C, yet `fn_30_E4EC` frees it (and frees
  `*(this+0x10A8)`, which the ctor does zero). The 0x109C pointer is left
  uninitialised by this constructor.
- `this+0x620` is passed to `EnableKnockBackPhysics__15CAiKnockBackMgrFb`
  (r3 set at [0x0000FE4C], r4 = 0 at [0x0000FE50], call at [0x0000FF40]); 0x620
  lies inside the 0x7C0-byte `CPatterned` base, so it is not a member of this
  class.
- The ctor also calls `Normalize__9CVector3fFv` [0x0000FC94] and ten
  `GetLocatorSegId__9CAnimDataCFRC...` calls that fill 0x10AC-0x10B5.
