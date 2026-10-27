# Randomizer

## Decision (2026-10-27): Randovania decides, the port applies

Supersedes the 2026-10-18 decision below in one point: the pickups **can** be enumerated statically.
Randovania's Echoes logic database names, per location, the region's MLVL, the area's MREA and the
Pickup's instance id - exactly the port's `WORLD:AREA:ENTITY` key. So the port does not grow logic of
its own: Randovania generates (logic, tricks, presets, multiworld) and the port applies its seed at
runtime instead of patching the ISO. Plan, each step checkable on its own:

1. **Location table** - done. `tools/rando_locations.py RANDOVANIA_DIR [--write]` reads
   `randovania/games/prime2/logic_database/*.json` and checks every row on the user's disc: the MLVL
   lists the MREA, the MREA holds a `PCKP` with that editor id, and its item is the database's
   `original_model`. 119 locations, 115 standard, 4 custom (the energy controllers' translators,
   which have no pickup on disc); 0 failed checks against Randovania `1ec92e8f` (2026-10-26). The
   output, `platform/include/port_rando_locations.inc`, is compiled into
   `platform/port_rando_locations.cpp` (`PortRandoLocations::PickupIndexFor`, `ByPickupIndex`);
   `port_rando_locations_tests` checks its shape. Regenerate when Randovania's database changes.
2. **Seed import** - read Randovania's Echoes patch data (pickups per index with resources, model,
   HUD and scan text; starting items; spawn; elevators; translator gates; door weaknesses).
3. **Pickup presentation** - the placed item's model and acquisition text.
4. **Non-pickup grants** - the 4 custom locations and any item a script grants.
5. **Script and layer patches** - Randovania's per-instance edits, applied as areas load.
6. **Save binding and the F1 page** - seed hash in the save, spoiler.

Findings from step 1:

- Dark-world regions carry no asset id in Randovania; they are areas of their light world's MLVL.
- The Bomb Guardian's drop (index 38, Agon Temple `081A0074`) is not in any script layer but in the
  MREA's generated-object section (`SCGN`, header word 19; `tools/echoes_disc.py` `mrea_generated`).
  `CScriptObjectLoaderHelper::GenerateScriptObject` builds it with its own editor id through the same
  `LoadPickup`, so the hook keys it like any other pickup.
- The Energy Transfer Module location gives item 28 (`TranslatorUpgrade_TempETM`), not
  `EnergyTransferModule` (107). Randovania shows the Echo Visor model for the Dark Visor location.

## Decision (2026-10-18)

MP2 gets a **no-logic shuffle generator over a dumped location list**, not a port of the MP1
generator. `../MetroidPrimePort/platform/port_rando_gen.cpp` is the Archipelago apworld
(Randovania-style) in C++: its locations are the fixed table of MP1 `5031xxx` ids and its fill is
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
