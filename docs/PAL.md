# PAL disc (G2MP01), experimental

The port runs the **USA 1.00 code on PAL data**. PAL's own DOL/REL changes are not ported (the port
compiles every module natively, so they never load); only the disc's resources come from PAL. This is the
Prime 1 port's approach to its later revisions.

## What differs and what the port does

- Accepted discs: `G2ME01` rev 0 (USA 1.00) and `G2MP01` rev 0 (PAL); anything else is refused at
  mount (`port::entry::IdentifyDisc`, `platform/entry.cpp`; unit-tested in `tests/port_entry_tests.cpp`).
  The mount logs `disc: Metroid Prime 2: Echoes PAL (G2MP01), experimental: USA 1.00 code on PAL data`.
- Saves: `CARDInit(port::entry::CurrentGameCode(), "01")` (`DolphinCMemoryCardSys.cpp`, TARGET_PC),
  so a PAL game saves in `<user>/EUR/Card A`, a USA one in `<user>/USA/Card A`; the two never share a
  file. `port_save.cpp` and the F1 Saves page (Dolphin import from `GC/EUR/Card A`) follow the mounted
  disc. USA saves and save states don't carry over to PAL (different world/area data).
- Disc check: `platform/port_disc_verify_table.inc` has a PAL row (sizes and CRCs of every file).
- Resource formats are identical. Script data differs in 122 rooms (`tools/pal_room_diff.py <usa> <pal>`,
  SCLY object/property/connection diff); none hits a hard-coded id in the port, the property loaders
  skip properties they don't know (BALT's new `boundsSizeMultiplier`, 1.0), and the changed objects
  are ones the USA code already handles. The FrontEnd MLVL's area id changed (cee3af0d -> dc0d2745).

## Verified (2026-10-25, desktop)

- New game on an empty card (`MP2_NEW_GAME=1`) reaches gameplay in 2.2 s; front-end boot with intro
  (5000 frames) runs without a crash. The screen (fonts, intro text) was not checked.
- `WORLD=all` area sweep on PAL with `MP2_NEW_GAME=1`: 341 of 352 slots played, all OK except one POOL
  (Sanctuary 16); the 11 missing are the Temple Grounds slots after the crash below. The Temple Grounds long shard crashes entering slot 53
  (`CCollisionCacheWriter::BeginGeometry`, null), and **USA crashes the same way on the same route**,
  so it is a port bug, not a PAL one; slots 52->53->56 alone are clean on both discs.

## Not verified

Language selection and PAL's 50 Hz / 60 Hz prompt (not in the USA code the port runs),
the SPFN->SWTC rooms on foot, Agon `0a_sand_hall_dark` (new MNNG 000d0016) beyond the sweep's visit,
Android.
