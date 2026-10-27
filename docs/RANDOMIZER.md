# Randomizer

## Decision (2026-10-27): no Randovania in the port

The port carries no Randovania code or data: no patch-data importer, no location table taken from
its logic database. Seeds use the port's own format (`"locations"` keyed `WORLD:AREA:ENTITY`, below)
and come from the port's own generator; any logic the generator grows is written here, from the
disc. Logic may also live outside the port: an Archipelago apworld (a separate program, licensed
on its own terms, GPL if it derives from Randovania's database) generates the placements and hands
the port `WORLD:AREA:ENTITY` keys through slot data; the port is then only a client (checks out,
items in) and stays MIT. (Superseded the same day's "Randovania decides, the port applies" plan; its
importer and table were removed in the commit that records this.)

Still true from that work: pickups **can** be enumerated statically from the disc, so the dump sweep
is no longer the only source of locations. Plan, each step checkable on its own:

1. **Location list from the disc** - walk every `PCKP` of the single-player worlds (`Metroid1.pak`
   ..`Metroid5.pak`; `Metroid6.pak` is the multiplayer arenas), script layers and the MREA's
   generated-object section, into a `WORLD:AREA:ENTITY` table the generator reads instead of the
   dump log. `tools/rando_models.py` already does this walk for models.
   **Done (2026-10-27), apart from the generator switch:** `tools/rando_world.py --write` writes
   `platform/include/port_rando_world.inc` (read by `platform/port_rando_world.cpp`, tested by
   `port_rando_world_tests`): 5 worlds, 284 areas, 678 MLVL dock edges (all symmetric, 78 virtual),
   53 `TEL1` teleporters and 114 locations. A location is a layer `PCKP` that is not a refill
   (items 41/43-46 with itemPercentageIncrease 0), or a generated-section `PCKP` that raises the
   percentage (only the Bomb Guardian's bombs). Excluded: the nine TempleKey1 copies in
   `12_temple_finalgate_dark` layer 3 and the AgonKey2/3 copies in `03_sand_temple_dark` layer 2.
   The rules are in the tool's docstring. `port_rando_gen.cpp` still reads the dump log.
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
4. **Logic** - from an apworld (port as AP client), or the port's own for offline beatable seeds.
   **Dock locks from the disc: done (2026-10-27).** `tools/rando_world.py` also writes
   `PORT_RANDO_DOCK_LOCK` rows, one per `PORT_RANDO_DOCK` edge (`PortRandoWorld::DockLockAt`).
   - The door is the DOOR that sends INCR to the dock's DOCK. 569 doors are on docks, and 6 are
     not: cutscene copies and in-room morph doors. 110 edges have no door.
   - Beam is the set of player weapons the door's vulnerability lets through (multiplier != 0,
     effect != Immune). Edges: Any 434, Light 60, Dark 48, Annihilator 14, Morph 12.
   - A shield door is immune until a MemoryRelay sends it RSET (`ResetDoor`). The relay is
     activated by a health-5 ACTR's DEAD, and that actor's only damaging weapon names the shield:
     Missile 35, SuperMissile 24, PowerBomb 24. For Seeker (14) it is a relay behind a CNTR that
     counts 5 seeker-lockable DTRGs.
   - A translator gate is a CRLY that tests a translator item and activates a scan POIN named
     "…Gate…". There are 16: Violet 7, Amber 4, Emerald 3, Cobalt 2.
   - Script lock: LOCK sent to the door, or `isLocked` at load. Temporary (54) when the area also
     sends ULCK; Permanent (3) otherwise, with `isLocked` counting as Permanent.
   - The tool fails unless docked doors, shields and gates match the measured counts per world.

   Ambiguities:
   - Nothing wires a translator gate to a door. It is assigned to the nearest docked door, 6 to 45
     units away (0u_temple_hall is 44.9).
   - Obstacles are extracted separately and are not wired to docks. `classify_obstacles` lists
     708 in-room obstacles (`PORT_RANDO_OBSTACLE`, `ObstacleAt()`), each with its position, the
     nearest dock and the nearest location. Barrier PLATs that power does not hurt need the
     weapons that do. A PLAT that a DTRG removes on DEAD (Agon's Dark Webbing) needs what hurts
     the DTRG. The other rows are grapple points, spinners, cannon balls, ball triggers, spider
     tracks (one row per area), portals and safe-zone crystals. Each requirement is an OR of the
     `ObstacleNeed` items; the docstring has the item mapping and the skipped names.
     `--dump-obstacles` prints every area's docks, locations and obstacles.
   - Topology is not derived: nothing says which docks or locations an obstacle separates, so
     that needs hand rules. The PATH navmesh cannot stand in for it: 150 of 284 areas have no
     regions, only 141 of 441 docks lie within 3 units of a region, and only 94 of 131 multi-dock
     areas have all their docks in one component.
   - Layer state is read only as the MLVL default flags. Which events switch a layer on is not
     traced: 2 translator checks and roughly half of the LOCK/ULCK senders sit on layers that are
     off by default.
   - Temporary locks are fight and cutscene locks, mostly harmless to logic but not proven so.

   **Hand rules (format).** One file per world, `tools/rando_rules/<MLVL internal name>.rules`,
   compiled by `tools/rando_rules.py` into `platform/include/port_rando_logic.inc`:

   ```
   area 0a_temple_hall                       # MLVL internal area name; one block per area
     dock0 - dock1: any                      # '-' both ways, '>' one way
     dock1 > loc0x00020034: SpaceJumpBoots | (MorphBall & MorphBallBombs)
     @ledge - dock2: GrappleBeam             # @name: an in-room node
     tel0x0C06013A - dock0: any              # a teleporter (editor id)
     covers 0x00020041 0x00020042            # obstacles these rules account for
     ignore 0x00020050: decorative webbing   # obstacles that block nothing, with a reason
   ```

   - Requirements: `any`, `never`, item names as spelled in `port_randomizer.cpp`'s table,
     `Name>=N` for counts, `&`, `|`, parentheses. `Missile` means at least one Missile.
   - Every obstacle of the area (`--dump-obstacles`) must be in a `covers` or an `ignore` line,
     and every `dockN`/`locX`/`telX` must exist in the area, or the compiler fails.
   - Fail closed: an area with no block connects nothing, so its docks are dead ends.
   - Dock edges come from `PORT_RANDO_DOCK`; the evaluator adds their locks: both sides' beam
     and shield, the translator, and `never` for a Permanent script lock on the side you leave
     from. Temporary locks are ignored. Rules only describe movement inside a room.
   - When unsure whether a route works without an item, require the item. A rule that is too
     strict only shrinks the seed space; one that is too loose makes unbeatable seeds.
   - State (2026-10-27): all five worlds have rules (284 areas). With every item, all 114
     locations are reachable from the Landing Site. With the starting inventory and the vanilla
     placement, only the first sphere (the `01_temple_hive01` Missile) opens: some early rules are
     stricter than the game, since vanilla is beatable by definition. Elevator rooms need an explicit
     `dock0 - telX` edge, or their world is cut off.
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
