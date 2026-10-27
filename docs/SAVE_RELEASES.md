# Echoes save layouts across releases

This is research for carrying a save from one disc release to another, as the Prime 1 port can.
It describes the formats only; there is no converter yet. Measured on 2026-10-27.

Each claim carries one of two labels:

- **[M]** means measured. The note says how: a tool's output, a source line, a DOL or symbol scan.
- **[I]** means inferred from the code, and not checked against real data.

| Release | Disc used | Status |
| --- | --- | --- |
| G2ME01 (USA GC) | USA GC ISO | Measured |
| G2MP01 (PAL GC) | `G2MP01.iso` | Measured |
| R3ME01 (Trilogy USA Wii) | Trilogy USA image, partition DATA, `MP2/` | Measured |
| R3MP01 (Trilogy PAL) | — | **No disc available. Nothing in this document is measured for it.** `config/R3MP01/` holds symbols only. |

The paks and DOLs were extracted read-only, with `dolphin-tool extract`, to `/tmp/opencode/discs/<id>/`.

## 1. Container

### GameCube (G2ME01, G2MP01)

| Field | G2ME01 | G2MP01 | Source |
| --- | --- | --- | --- |
| File name | `MetroidPrime2` | same | [M] Both DOLs contain the string once; `skSaveFileName` |
| Game code / maker | `G2ME` / `01` | `G2MP` / `01` | [M] `CARDInit(<disc code>, "01")`; `platform/port_save.cpp` |
| Comment | `Metroid Prime 2 Echoes          ` + `%02d.%02d.%02d  %02d:%02d` | same | [M] The string is in both DOLs; `CMemoryCardDriver.cpp:154-158` |
| Banner | `TXTR_SaveBanner` | same | [M] The string is in both DOLs; `CSaveGameScreen.cpp:17` |
| Blocks | 3: one header block, plus 2 copies × 1 block | same | [M] `CCardFileInfo`: `mSlotBlocks*2+1`, `mAlignedUserDataSize` 0x2000 |
| Header CRC | CCRC32 of header bytes 4..0x2000, seed 0xFFFFFFFF | same | [M] `src/Kyoto/DolphinCMemoryCardSys.cpp` |
| Copy | u32 CRC (bytes 4..end), u32 generation (0/1, newest wins), 0x1ff8 user bytes | same | [M] Same source; `port_save.cpp` header comment |
| Endianness | Console: big-endian. Port-written: CRC and generation are host-LE. | same | [M] `port_save.cpp` `Inspect()` accepts either |

The `.gci` adds a 0x40 directory entry in front. The rest of the file is the card image.

The two GC DOLs have different sha1s:

- G2ME01: `6ef9b491d0cc08bc81a124fdedb8bfaec34d0010`
- G2MP01: `5a670d5da3d181e86a0df7cf7751c7055eee35fb` [M, `sha1sum` of the extracted `sys/main.dol`]

Every save-serialisation function has the same size in both, from `config/G2M?01/symbols.txt` [M]:

| Function | Size in both DOLs |
| --- | --- |
| `CGameState::PutTo` | 0x36C |
| `CGameState(CBitStreamReader&)` | 0x684 |
| `CMemoryCardDriver::InitializeFileInfo` | 0x1BC |
| `CPlayerState::PutTo` | 0x210 |
| `CWorldState::PutTo` | 0x94 |
| `CScriptMailbox::PutTo` | 0x148 |
| `CMapWorldInfo::PutTo` | 0x2D8 |
| `CWorldLayerState::PutTo` | 0xF0 |
| `CPersistentOptions::PutTo` | 0x258 |
| `CGameOptions::PutTo` | 0x23C |
| `SGameFileSlot::PutTo` | 0x30 |

`kPowerUpMax` (0x1B4) and `kShouldPersist` (0x6D, 59 persisted items) are byte-identical [M]. Measured by taking the sha1 of the rodata bytes at each DOL's symbol address.

### Trilogy (R3ME01; R3MP01 unknown)

What is measured, from string and immediate scans of the DOLs [M]:

- `rs5mp2_p.dol` (Echoes) contains `save.bin` and the RVL NAND SDK.
- `rs5fe_p.dol` (front end) contains `save.bin`, `banner.bin`, `TXTR_MP2SaveBanner`, `TXTR_MP1SaveBanner` and `TXTR_RS5SaveBanner`.
- Neither DOL contains `MetroidPrime2`, the GC comment, or `TXTR_SaveBanner`.
- `config/R3ME01/symbols.txt` names only `CSaveGameInterface` and the NAND SDK functions.

What this implies [I]:

- All three games share one NAND `save.bin`, owned by the front end.
- There is no GCI and no `CARD*` container.

**The framing of `save.bin` is unknown.** That covers the offset of the Echoes section, its size, its checksum, and whether the two-copy and generation scheme survives. Answering it needs a real Trilogy save, or decompiling `CSaveGameInterface` and the NAND path in `rs5fe_p.dol`.

## 2. Payload (user data, 0x1ff8 bytes)

Sources are `CMemoryCardDriver::BuildSaveBuffer`, `SSaveHeader`, `SGameFileSlot` and `CGameState::PutTo` in `src/` [M]. This is the G2ME layout. Function sizes say G2MP is the same; the Trilogy notes follow the tables.

### Top-level user data

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 15 | `SSaveHeader`: u32 signature, s32 saveIdx, 3 × bool slot-present, u32 `SAVH` |
| 15 | 192 | System data: `CPersistentOptions` bitstream |
| 207 | 3 × 32 | Per-slot `CGameOptions` bitstream |
| 303 | 32 | Global (multiplayer) options |
| 335 | 0xa38 each | Present `SGameFileSlot`s, in slot order. 335 + 3×0xa38 = 0x1ff7, which is ≤ 0x1ff8. |

### Signature

The signature is `0x5553413e ^ Σxor CWorldSaveGameInfo::CalculateHash()`, taken over every world in MemoryWorlds.

- A mismatch marks the save invalid.
- The hash covers areaCount, then the cinematic, relay, door, layer, scan and unmappable arrays, in that order.
- It is computed as CRC32 over the **in-memory** words. The CRC is seeded with the CRC of the env-var names, with no final xor.

### `CPersistentOptions` bitstream

The fields, in order:

1. `SYST` (32 bits).
2. saveIdx (2 bits).
3. One bit per cinematic of every SAVW, in MemoryWorlds order.
4. System env vars, in name order (`rstl::map`), each `GetBitCount(max-min)` bits.
5. `SYND`.

### `CGameOptions` bitstream

It starts with `OPTN`. The field list was not audited for this document; see `CGameOptions::PutTo`. Its size (0x23C) is equal in G2ME and G2MP.

### `SGameFileSlot` (0xa38) = `CGameState::PutTo` bitstream

| Order | Field | Bits |
| --- | --- | --- |
| 1 | `GMST`, timestamp, hardMode, initPowerups, isDarkWorld | 32, 32, 1, 1, 1 |
| 2 | desiredWorld (MLVL ID), playTime (double) | 32, 64 |
| 3 | 4 × `CPlayerState`: `PLST`, enabled 32, HP 32, beam 3, suit 2, sel 3, team 3, scheme 2 | per player |
| | `PRST`, amount and capacity for each persisted powerup | 186 |
| | `POWR`, 2 bits × (4 fixed + **scan union**), scan rates 9+9, `SCAN` | per player |
| 4 | `CHintOptions`: per `HINT_Hints` entry, a state plus a 32-bit time | count from HINT |
| 5 | `PREV` multiplayer results | |
| 6 | Byte-align. Then escapeTime f32 and the game env vars. | |
| 7 | u8 worldCount. Per world: aligned u32 MLVL ID, u16 bitCount, then the `CWorldState` bits. | |
| 8 | u32 `GMND` | |

The world records are written in MemoryWorlds order, which is every MLVL sorted by ID.

`CWorldState` bits, in order:

1. areaId 32 and desiredAreaAssetId 32.
2. One bit per SAVW relay.
3. visited, then mapped: one bit per area each.
4. One bit per door.
5. One bit per unmappable object.
6. mapStationUsed, 1 bit.
7. layerTotal, 10 bits.
8. One bit for each layer 1..n-1 of each area, with n taken from the MLVL layer table.

How the reader treats these records:

- It skips a world whose MLVL ID it does not know, using the record's bitCount.
- It parses a known world with its **own** SAVW.
- So bitCount lets a converter split the records, but the reader does not use it to resync.

### Trilogy differences

Measured by scanning the DOLs for `lis`+`addi`/`ori` FourCC pairs and `li` immediates [M]:

| Pattern | G2ME main.dol | G2MP main.dol | R3ME rs5mp2_p.dol |
| --- | --- | --- | --- |
| `SAVH` | 1 | 1 | **0** |
| `GMST` / `GMND` / `PLST` / `PRST` / `POWR` / `PREV` / `SYST` | 1 each | 1 each | 1 each |
| `SYND` | 1 | 1 | **0** |
| `OPTN` | 1 | 1 | 2 |
| `li 0xa38` | 16 | 16 | **0** |
| `li 0x1ff8` | 5 | 5 | **0** |

`kPowerUpMax` and `kShouldPersist` bytes from G2ME occur exactly once in `rs5mp2_p.dol` [M].

What this suggests [I]:

- The `CGameState` and `CPlayerState` bitstream framing survives in Trilogy.
- The GC `SSaveHeader`, the options packing and the 0xa38 slot do not.
- `SYND` is missing from the Echoes DOL but present in `rs5fe_p.dol`, so the system options may have moved to the shared front end.

The Trilogy slot size and header are **unknown**.

## 3. World data — `tools/savw_diff.py`

Every release has 12 MLVLs [M]:

- Seven are shared by ID: Temple Grounds `3bfa3eff`, Great Temple `863fcd72`, Agon `42b935e4`, Torvus `3dfd2249`, Sanctuary `1baa96c2`, and the Metroid6 group of six multiplayer worlds.
- The **FrontEnd world ID differs in each release**:
  - G2ME `69802220`
  - G2MP `7b6eaa68`
  - R3ME `daea74a3`

  Its SAVW content is identical (hash `44bc9114`). In Trilogy the new ID sorts later, which moves its record in MemoryWorlds order.
- MLVL area lists, as (MREA, save ID) pairs, are identical in every shared world [M].

Summary of the output below [M]:

| Item | G2ME01 | G2MP01 | R3ME01 |
| --- | --- | --- | --- |
| Signature, BE (console) | `44bfc715` | `c13c33f1` | `fe44ecfe` |
| Signature, LE (port in-memory, see §6) | `37d0412e` | `467484b3` | `9c307b43` |
| Scan union | 815 | 818 | 817 |
| Cinematic bits | 1064 | 1064 | 1030 |
| System / game vars | 19 / 1 | 19 / 1 | 19 / 1 |
| `HINT_Hints` count | 44 | 44 (bytes differ) | 42 |
| Temple Grounds area 6 layers | 8 | 9 (`Missile Trooper Control` appended) | 9 |

### Per-world differences against G2ME01 [M]

| World | G2MP01 | R3ME01 |
| --- | --- | --- |
| Sanctuary `1baa96c2` | Scans reordered | −8 cinematics, +1 relay, 1 door swapped, scans reordered |
| Temple Grounds `3bfa3eff` | +1 layer, 1 relay swapped, 1 scan swapped | +1 layer, −2 cinematics, relays −2/+3, scans ±2 |
| Torvus `3dfd2249` | Identical | −10 cinematics, 1 scan swapped |
| Agon `42b935e4` | +2 scans | −10 cinematics, doors reordered, scans −2/+3 |
| Great Temple `863fcd72` | Identical | −4 cinematics, −1 relay |
| FrontEnd and multiplayer worlds | Identical (FrontEnd ID renamed) | Identical (FrontEnd ID renamed) |

Full output of `cd /tmp && python3 -I <worktree>/tools/savw_diff.py`, with the defaults being the three extracted trees:

```
== G2ME01
  MemoryWorlds (12, sorted by MLVL ID):
    MLVL     SAVW     pak             areas  ver cine rely lyrs door scan unmp  wbits   hashBE   hashLE
    1baa96c2 8754d2a9 Metroid5.pak       67    5  228  180    0  124  215    2    824 ecf2d615 03d94b07
    233e42be bfc006d5 Metroid6.pak        1    5    3    0    0    0    0    0     81 33b30df0 d35bdabd
    3bfa3eff a7047a94 Metroid1.pak       63    5  163  216    0  115  333    7    814 a8f7fc2b 2680e074
    3dfd2249 a1036622 Metroid4.pak       70    5  228  138    0  154  209    2    824 7c5fe2e9 88b80949
    406add7f dc949914 Metroid6.pak        1    5    3    0    0    0    0    0     80 45eda14c 81913db9
    42b935e4 de47718f Metroid3.pak       72    5  280  238    0  162  282    2    949 82f06d8c f95814cb
    69802220 f57e664b FrontEnd.pak        1    5    1    0    0    0    0    0     80 44bc9114 b1a24f14
    7e19ed26 e2e7a94d Metroid6.pak        1    5    3    0    0    0    0    0     80 be9602ac 03254262
    863fcd72 1ac18919 Metroid2.pak       12    5  146   22    0   20   69    3    223 8b416c0e afb5af04
    a50a80cc 39f4c4a7 Metroid6.pak        1    5    3    0    0    0    2    0     80 ba64b6d3 c4ab8e3a
    ae171602 32e95269 Metroid6.pak        1    5    3    0    0    0    0    0     80 5009de08 e5da5dad
    e3b0c703 7f4e8368 Metroid6.pak        1    5    3    0    0    0    0    0     81 461e18a1 58b32000
  signature BE (console) 44bfc715   LE (TARGET_PC in-memory) 37d0412e
  scan union 815 entries (+4 fixed) -> POWR..SCAN scan bits 1638
  cinematic bits 1064; system vars 19 (29 bits); game vars 1 (1 bits)
  HINT_Hints 2255c7bc v1 count 44 crc 302f9b1d (NoARAM.pak)
== G2MP01
  MemoryWorlds (12, sorted by MLVL ID):
    MLVL     SAVW     pak             areas  ver cine rely lyrs door scan unmp  wbits   hashBE   hashLE
    1baa96c2 8754d2a9 Metroid5.pak       67    5  228  180    0  124  215    2    824 3f0b61c5 370cc7c9
    233e42be bfc006d5 Metroid6.pak        1    5    3    0    0    0    0    0     81 33b30df0 d35bdabd
    3bfa3eff a7047a94 Metroid1.pak       63    5  163  216    0  115  333    7    815 a0042ea1 523616a0
    3dfd2249 a1036622 Metroid4.pak       70    5  228  138    0  154  209    2    824 7c5fe2e9 88b80949
    406add7f dc949914 Metroid6.pak        1    5    3    0    0    0    0    0     80 45eda14c 81913db9
    42b935e4 de47718f Metroid3.pak       72    5  280  238    0  162  284    2    949 dc79fc32 c89fab4c
    7b6eaa68 e790ee03 FrontEnd.pak        1    5    1    0    0    0    0    0     80 44bc9114 b1a24f14
    7e19ed26 e2e7a94d Metroid6.pak        1    5    3    0    0    0    0    0     80 be9602ac 03254262
    863fcd72 1ac18919 Metroid2.pak       12    5  146   22    0   20   69    3    223 8b416c0e afb5af04
    a50a80cc 39f4c4a7 Metroid6.pak        1    5    3    0    0    0    2    0     80 ba64b6d3 c4ab8e3a
    ae171602 32e95269 Metroid6.pak        1    5    3    0    0    0    0    0     80 5009de08 e5da5dad
    e3b0c703 7f4e8368 Metroid6.pak        1    5    3    0    0    0    0    0     81 461e18a1 58b32000
  signature BE (console) c13c33f1   LE (TARGET_PC in-memory) 467484b3
  scan union 818 entries (+4 fixed) -> POWR..SCAN scan bits 1644
  cinematic bits 1064; system vars 19 (29 bits); game vars 1 (1 bits)
  HINT_Hints 2255c7bc v1 count 44 crc ab8f7068 (NoARAM.pak)
== R3ME01
  MemoryWorlds (12, sorted by MLVL ID):
    MLVL     SAVW     pak             areas  ver cine rely lyrs door scan unmp  wbits   hashBE   hashLE
    1baa96c2 8754d2a9 Metroid5.pak       67    5  220  181    0  124  215    2    825 f9d952e1 647696bf
    233e42be bfc006d5 Metroid6.pak        1    5    3    0    0    0    0    0     81 33b30df0 d35bdabd
    3bfa3eff a7047a94 Metroid1.pak       63    5  161  217    0  115  333    7    816 4fe85b0b 60e3dc36
    3dfd2249 a1036622 Metroid4.pak       70    5  218  138    0  154  209    2    824 41142120 88d13eed
    406add7f dc949914 Metroid6.pak        1    5    3    0    0    0    0    0     80 45eda14c 81913db9
    42b935e4 de47718f Metroid3.pak       72    5  270  238    0  162  283    2    949 c6e81ec7 0371def0
    7e19ed26 e2e7a94d Metroid6.pak        1    5    3    0    0    0    0    0     80 be9602ac 03254262
    863fcd72 1ac18919 Metroid2.pak       12    5  142   21    0   20   69    3    222 baddd4b3 dfd9890c
    a50a80cc 39f4c4a7 Metroid6.pak        1    5    3    0    0    0    2    0     80 ba64b6d3 c4ab8e3a
    ae171602 32e95269 Metroid6.pak        1    5    3    0    0    0    0    0     80 5009de08 e5da5dad
    daea74a3 461430c8 FrontEnd.pak        1    5    1    0    0    0    0    0     80 44bc9114 b1a24f14
    e3b0c703 7f4e8368 Metroid6.pak        1    5    3    0    0    0    0    0     81 461e18a1 58b32000
  signature BE (console) fe44ecfe   LE (TARGET_PC in-memory) 9c307b43
  scan union 817 entries (+4 fixed) -> POWR..SCAN scan bits 1642
  cinematic bits 1030; system vars 19 (29 bits); game vars 1 (1 bits)
  HINT_Hints e1729bc1 v1 count 42 crc e709f7c0 (NoARAM.pak)
== diff G2ME01 -> G2MP01
  worlds only in G2ME01: ['69802220']
  worlds only in G2MP01: ['7b6eaa68']
  world 1baa96c2: 
    scans: same 215 entries, different order
    hash ecf2d615 -> 3f0b61c5
  world 233e42be: identical
  world 3bfa3eff: 
    layer counts differ in 1 areas: [(6, 8, 9)]
      area 6 base   ['Default', '!No Load', '1st Pass', 'Map Station Cinematic', 'Post Dark Trooper', 'Pre Dark Trooper', 'Missile Trooper', 'Missile trooper gate']
      area 6 other  ['Default', '!No Load', '1st Pass', 'Map Station Cinematic', 'Post Dark Trooper', 'Pre Dark Trooper', 'Missile Trooper', 'Missile trooper gate', 'Missile Trooper Control']
    relays: 216 -> 216; only base 1, only other 1; common entries keep relative order
    scans: 333 -> 333; only base 1, only other 1; common entries reordered
    CWorldState bits 814 -> 815
    hash a8f7fc2b -> a0042ea1
  world 3dfd2249: identical
  world 406add7f: identical
  world 42b935e4: 
    scans: 282 -> 284; only base 0, only other 2; common entries reordered
    hash 82f06d8c -> dc79fc32
  world 7e19ed26: identical
  world 863fcd72: identical
  world a50a80cc: identical
  world ae171602: identical
  world e3b0c703: identical
  signature BE 44bfc715 -> c13c33f1
    scan union: 815 -> 818; only base 0, only other 3; common entries keep relative order
    sysvars identical (19)
    gamevars identical (1)
    HINT count 44 -> 44, bytes differ
== diff G2ME01 -> R3ME01
  worlds only in G2ME01: ['69802220']
  worlds only in R3ME01: ['daea74a3']
  world 1baa96c2: 
    cinematics: 228 -> 220; only base 8, only other 0; common entries keep relative order
    relays: 180 -> 181; only base 0, only other 1; common entries keep relative order
    doors: 124 -> 124; only base 1, only other 1; common entries keep relative order
    scans: same 215 entries, different order
    CWorldState bits 824 -> 825
    hash ecf2d615 -> f9d952e1
  world 233e42be: identical
  world 3bfa3eff: 
    layer counts differ in 1 areas: [(6, 8, 9)]
      area 6 base   ['Default', '!No Load', '1st Pass', 'Map Station Cinematic', 'Post Dark Trooper', 'Pre Dark Trooper', 'Missile Trooper', 'Missile trooper gate']
      area 6 other  ['Default', '!No Load', '1st Pass', 'Map Station Cinematic', 'Post Dark Trooper', 'Pre Dark Trooper', 'Missile Trooper', 'Missile trooper gate', 'Missile Trooper Control']
    cinematics: 163 -> 161; only base 2, only other 0; common entries keep relative order
    relays: 216 -> 217; only base 2, only other 3; common entries reordered
    scans: 333 -> 333; only base 2, only other 2; common entries reordered
    CWorldState bits 814 -> 816
    hash a8f7fc2b -> 4fe85b0b
  world 3dfd2249: 
    cinematics: 228 -> 218; only base 10, only other 0; common entries keep relative order
    scans: 209 -> 209; only base 1, only other 1; common entries reordered
    hash 7c5fe2e9 -> 41142120
  world 406add7f: identical
  world 42b935e4: 
    cinematics: 280 -> 270; only base 10, only other 0; common entries keep relative order
    doors: same 162 entries, different order
    scans: 282 -> 283; only base 2, only other 3; common entries reordered
    hash 82f06d8c -> c6e81ec7
  world 7e19ed26: identical
  world 863fcd72: 
    cinematics: 146 -> 142; only base 4, only other 0; common entries keep relative order
    relays: 22 -> 21; only base 1, only other 0; common entries keep relative order
    CWorldState bits 223 -> 222
    hash 8b416c0e -> baddd4b3
  world a50a80cc: identical
  world ae171602: identical
  world e3b0c703: identical
  signature BE 44bfc715 -> fe44ecfe
    scan union: 815 -> 817; only base 1, only other 3; common entries keep relative order
    sysvars identical (19)
    gamevars identical (1)
    cinematic bits 1064 -> 1030
    HINT count 44 -> 42, bytes differ
== diff G2MP01 -> R3ME01
  worlds only in G2MP01: ['7b6eaa68']
  worlds only in R3ME01: ['daea74a3']
  world 1baa96c2: 
    cinematics: 228 -> 220; only base 8, only other 0; common entries keep relative order
    relays: 180 -> 181; only base 0, only other 1; common entries keep relative order
    doors: 124 -> 124; only base 1, only other 1; common entries keep relative order
    scans: same 215 entries, different order
    CWorldState bits 824 -> 825
    hash 3f0b61c5 -> f9d952e1
  world 233e42be: identical
  world 3bfa3eff: 
    cinematics: 163 -> 161; only base 2, only other 0; common entries keep relative order
    relays: 216 -> 217; only base 1, only other 2; common entries reordered
    scans: 333 -> 333; only base 1, only other 1; common entries reordered
    CWorldState bits 815 -> 816
    hash a0042ea1 -> 4fe85b0b
  world 3dfd2249: 
    cinematics: 228 -> 218; only base 10, only other 0; common entries keep relative order
    scans: 209 -> 209; only base 1, only other 1; common entries reordered
    hash 7c5fe2e9 -> 41142120
  world 406add7f: identical
  world 42b935e4: 
    cinematics: 280 -> 270; only base 10, only other 0; common entries keep relative order
    doors: same 162 entries, different order
    scans: 284 -> 283; only base 2, only other 1; common entries reordered
    hash dc79fc32 -> c6e81ec7
  world 7e19ed26: identical
  world 863fcd72: 
    cinematics: 146 -> 142; only base 4, only other 0; common entries keep relative order
    relays: 22 -> 21; only base 1, only other 0; common entries keep relative order
    CWorldState bits 223 -> 222
    hash 8b416c0e -> baddd4b3
  world a50a80cc: identical
  world ae171602: identical
  world e3b0c703: identical
  signature BE c13c33f1 -> fe44ecfe
    scan union: 818 -> 817; only base 1, only other 0; common entries keep relative order
    sysvars identical (19)
    gamevars identical (1)
    cinematic bits 1064 -> 1030
    HINT count 44 -> 42, bytes differ
```

## 4. Conversion feasibility per ordered pair

The per-player options and header fields map 1:1 in every pair [I]. The `PLST` fields and `PRST` are equal, because the tables are equal.

| Pair | Verdict | What must be rewritten |
| --- | --- | --- |
| G2ME → G2MP | **Remap by ID** | See below |
| G2MP → G2ME | **Remap by ID** (lossy) | The inverse of the above. Drop the PAL-only scans, relay and layer bit. |
| GC → R3ME | **Blocked** | The `save.bin` framing, header, options packing and slot size are unknown. The payload inside it would be remap-by-ID. |
| R3ME → GC | **Blocked** | Same reason |
| any ↔ R3MP | **Blocked** | No disc, no data |

G2ME → G2MP rewrites:

- **Signature**: recompute it.
- **FrontEnd record**: rename its MLVL ID.
- **Scans**: rebuild the scan bits by asset ID against the target union. PAL adds 3 scans, which start unscanned.
- **Temple Grounds relays**: one relay ID differs, so rebuild by relay ID.
- **Layers**: add one layer bit for area 6. Its default comes from the MLVL layer bits.
- **Bit counts**: fix bitCount and layerTotal.
- **CHintOptions**: equal count (44), so copy it [I: the hint order may differ, because the bytes differ].

Remap by ID needs no game logic. Every indexed bit list is keyed by stable IDs present in the SAVW or MLVL:

- relay, door and unmappable `TEditorId`s
- cinematic IDs
- scan asset IDs
- layer names per area

Where an ID exists only in the source, the bit is dropped [I]. Within the payload, GC ↔ Trilogy would also need these [I]:

- Cinematic bits remapped by ID. Trilogy drops 34.
- The HINT list remapped (44 → 42). That needs hint names, which `HINT_Hints` carries.
- The FrontEnd record renamed, and re-sorted because its position changes.

## 5. Round trip

`~/.local/share/Metroid Prime 2: Echoes/USA/Card A` exists and is **empty** [M, `ls`]. There is no EUR folder, and no Dolphin Echoes save was found. Nothing could be decoded, so the CRC, slot count and world checks are untested.

## 6. Unknowns and risks

1. **Trilogy `save.bin` framing.** No Trilogy save is available, and `CSaveGameInterface` is not decompiled.
2. **Trilogy header and slot layout.** `SAVH` and 0xa38 are absent from the Echoes DOL.
3. **R3MP01.** There is no disc.
4. **The port's signature may differ from the console's [I].** `CalculateHash` memcpys host-endian words, and `CWorldSaveGameInfo.cpp` has no `TARGET_PC` swap. On the LE port the expected G2ME signature would then be `37d0412e`, not the console's `44bfc715`. A real console GCI would be refused as invalid even on its own release. Verify this with a Dolphin save before relying on it.
5. **HINT order.** The G2ME and G2MP lists have the same count but different bytes. Check the names before copying hint states 1:1.
6. **Untested on real data.** No real save was round-tripped, so the bit layouts in §2 come from the decompiled source and are not checked against data.
