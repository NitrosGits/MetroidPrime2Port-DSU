# Randomizer

## Decision (2026-10-27): no Randovania in the port

The port carries no Randovania code or data: no patch-data importer, no location table taken from
its logic database. Seeds use the port's own format (`"locations"` keyed `WORLD:AREA:ENTITY`, below)
and come from the port's own generator; any logic the generator grows is written here, from the
disc. An Archipelago client stays possible on the port side, but its logic cannot come from
Randovania either. (Superseded the same day's "Randovania decides, the port applies" plan; its
importer and table were removed in the commit that records this.)

Still true from that work: pickups **can** be enumerated statically from the disc, so the dump sweep
is no longer the only source of locations. Plan, each step checkable on its own:

1. **Location list from the disc** - walk every `PCKP` of the single-player worlds (`Metroid1.pak`
   ..`Metroid5.pak`; `Metroid6.pak` is the multiplayer arenas), script layers and the MREA's
   generated-object section, into a `WORLD:AREA:ENTITY` table the generator reads instead of the
   dump log. `tools/rando_models.py` already does this walk for models.
2. **Seed format** - the native JSON only (done; see Workflow).
3. **Pickup presentation** - the placed item's model and acquisition text.
   *Done (builds, unit-tested, not yet seen in game):* `tools/rando_models.py` reads every PCKP on
   disc and writes `platform/include/port_rando_models.inc` (44 rows: item -> CMDL + animation set,
   with the world paks that hold them). `LoadPickup` swaps in the donor only when
   `gpResourceFactory->GetResourceTypeById` resolves its CMDL (and ANCS), since lookup only sees
   loaded paks (`CResLoader::FindResourceForLoad`); otherwise the original model stays. ETM (107)
   borrows item 28's model, MissileLauncher (73) the Missile's. Items with no vanilla Pickup (Power
   Beam, Combat/Scan Visor, Varia, Morph Ball, Charge, Cannon Ball, ...) have no donor yet.
   Acquisition text: pickups reach their MEMO through SRLY/MRLY relays on `ZERO` (120 of 132 within
   4 hops), so `Touch` suppresses MEMO display while it sends `Arrived` and shows
   "<ItemName> acquired!" itself. A MEMO behind a timer would still show vanilla text. Scans are
   out of scope: vanilla pickups carry no scan.
4. **Logic** - the port's own, so seeds can be beatable.
5. **Non-pickup grants** - items a script grants (the energy controllers' translators, ...).
6. **Save binding and the F1 page** - seed hash in the save, spoiler. The F1 Randomizer page is
   hidden (`platform/debug_ui/search.cpp`) until then.

Disc findings:

- Dark-world areas are areas of their light world's MLVL.
- The Bomb Guardian's drop (Agon Temple `081A0074`) is not in any script layer but in the MREA's
  generated-object section (`SCGN`, header word 19; `tools/echoes_disc.py` `mrea_generated`).
  `CScriptObjectLoaderHelper::GenerateScriptObject` builds it with its own editor id through the same
  `LoadPickup`, so the hook keys it like any other pickup.
- The Energy Transfer Module location gives item 28 (`TranslatorUpgrade_TempETM`), not
  `EnergyTransferModule` (107).

## Decision (2026-10-18)

MP2 gets a **no-logic shuffle generator over a dumped location list**, not a port of the MP1
generator. `../MetroidPrimePort/platform/port_rando_gen.cpp` is the Archipelago apworld
in C++: its locations are the fixed table of MP1 `5031xxx` ids and its fill is
driven by region/item logic (`PortApLogic`, elevators, blast shields, start rooms). MP2 has none of
that data, and its pickups cannot be enumerated statically: they exist only as `Pickup` script
objects inside each area's MREA, so the only list the port can get is the one the game itself reports
as areas load (`MP_RANDO_DUMP`). The phone's `/sdcard/MetroidPrime/randomizer_seeds/*.json` are MP1
Archipelago seeds (`placements` keyed by `5031xxx`, `start_items`, `slot_data`), not MP2 seeds, and
the MP2 loader's grammar (`"locations"` keyed `WORLD:AREA:ENTITY`) does not read them. So:
dump the locations by visiting areas, then shuffle their items with a deterministic PRNG. Seeds may
be unbeatable (no logic) and only cover areas that were dumped.

## Workflow

1. Dump: `MP_RANDO_DUMP=1` and play/sweep (`tools/sweep_areas.sh`). Each pickup load appends
   `LOC <WORLD>:<AREA>:<ENTITY> <Item> amount=<n> capacity=<n>` to `<user>/randomizer_locations.log`.
2. Generate: `MP_RANDO_GEN=<seed name>` (without dump mode) reads that log and writes the seed JSON to
   `MP_RANDO_SEED` (default `<user>/randomizer_seed.json`), then loads it in the same process.
   Same name + same log = same seed. The last record per key wins.
3. Play: leave `MP_RANDO_GEN` off; the seed file is loaded at startup.

`<user>` is `MP2_USER_PATH` (then `MP_USER_PATH`, the Prime 1 port's name) or SDL's pref path. Code: `platform/port_rando_gen.cpp`,
`platform/port_randomizer.cpp`.

## Notes

- Everything is lazy: the seed is loaded (or generated) at the first pickup load, so a run that never
  enters an area with a pickup (Temple Grounds area 0, for one) prints nothing and writes no file. Use
  area 7 to see it in a short run.
- `kItems` in `port_randomizer.cpp` is Echoes' `EItemType`. It used to be Prime 1's, so every Echoes
  item but the bombs came out as `Unknown`.
- Verified 2026-10-18: a 5-shard sweep (`tools/sweep_areas.sh`, `DWELL=500`) with `MP_RANDO_DUMP=1`
  logged 18 distinct locations (Temple Grounds); `MP_RANDO_GEN=echoes1` then wrote a seed with all 18
  and the next run loaded it (`randomizer: seed 'echoes1', 18 placements`).
- Not done: logic (beatable seeds), start items, a second world's locations, and applying a
  placement's model/scan text; the pickup keeps its original model.
