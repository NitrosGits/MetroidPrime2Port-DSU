#!/usr/bin/env python3
"""Build the randomizer's world graph and location list from the user's own disc.

usage: tools/rando_world.py [--write]

Walks the single-player worlds (the MLVLs in Metroid1.pak..Metroid5.pak; Metroid6.pak holds the
multiplayer arenas) and writes platform/include/port_rando_world.inc:

  * worlds - MLVL id and name, in pak order (Temple Grounds, Great Temple, Agon, Torvus, Sanctuary).
  * areas  - MREA id, MLVL area name, dark flag (the name's `_dark` suffix, as tools/world_areas.py).
  * docks  - every connected MLVL dock as a directed edge (area, dock) -> (area, dock), with
             `virtual` from the area's DOCK script object of that dock number. The MLVL dock
             table is symmetric on this disc; the tool fails if that ever stops being true.
  * teleporters - every TEL1 (WorldTeleporter): the area and layer it sits in and its destination
             MLVL/MREA. Elevators and cutscene warps alike; telling them apart is logic's job.
  * locations - the pickups a seed may change, keyed WORLD:AREA:ENTITY like the runtime:
      - every PCKP in a script layer, except refills: HealthRefill, Powerbomb, Missile, DarkAmmo
        and LightAmmo pickups whose itemPercentageIncrease is 0 (expansions raise it, refills do
        not), and except EXCLUDED below;
      - PCKPs in the MREA's generated-object section (SCGN) only when they raise the item
        percentage: the rest of that section is enemy drops (about 2400 refills).
    Items the game grants by script (the translators, ...) have no PCKP and are not listed.

The rule is the disc's, the exclusions are judgement; each one says why.
"""
import pathlib
import re
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402
import rando_models as rm  # noqa: E402

OUT = ROOT / "platform" / "include" / "port_rando_world.inc"
SINGLE_PLAYER_PAKS = rm.SINGLE_PLAYER_PAKS

ITEM_PROP = rm.ITEM_PROP
CAPACITY_PROP = 0x28C71B54  # SLdrPickup capacityIncrease (default 1)
PERCENT_PROP = 0x165AB069   # itemPercentageIncrease (default 0)
AMOUNT_PROP = 0x94AF1445    # amount (default 1)
REFILLS = {41, 43, 44, 45, 46}  # HealthRefill, Powerbomb, Missile, DarkAmmo, LightAmmo

TEL_WORLD_PROP = 0x31EC14BC
TEL_AREA_PROP = 0xE0C17804

# (area name, layer, item) -> reason. Pickups that are copies of a key held elsewhere.
EXCLUDED = {
    ("12_temple_finalgate_dark", 3, 29):
        "Sky Temple Gateway: nine TempleKey1 copies on one layer, the deposited keys",
    ("03_sand_temple_dark", 2, 33): "Dark Agon Temple: AgonKey2 copy; the key is in Portal Observation",
    ("03_sand_temple_dark", 2, 34): "Dark Agon Temple: AgonKey3 copy; the key is in the Alamo front",
}


def sldr_prop(name, field):
    """Property id of `field` in include/MetroidPrime/ScriptLoader/SLdr<name>.inc."""
    src = (ROOT / "include" / "MetroidPrime" / "ScriptLoader" / f"SLdr{name}.inc").read_text()
    m = re.search(r"case (0x[0-9a-f]+): \{\s*sldrThis\." + field + r"\b", src)
    if not m:
        sys.exit(f"[rando_world] no {field} in SLdr{name}.inc")
    return int(m.group(1), 16)


def i32(props, key, default):
    return struct.unpack(">i", props[key][-4:])[0] if key in props else default


def collect():
    iso = ed.Iso(ed.find_iso() or sys.exit("no disc image (set MP2_DISC)"))
    dock_number = sldr_prop("Dock", "dockNumber")
    dock_virtual = sldr_prop("Dock", "isVirtual")
    worlds, areas, docks, teleporters, locations = [], [], [], [], []
    excluded_hits = set()
    for pk in ed.load_paks(iso):
        if not SINGLE_PLAYER_PAKS.search(pk.path):
            continue
        for name, (kind, mlvl) in sorted(pk.names.items(), key=lambda kv: kv[1][1]):
            if kind != "MLVL":
                continue
            world = len(worlds)
            mlvl_areas = ed.parse_mlvl(pk.get(mlvl))["areas"]
            worlds.append((mlvl, name))
            base = len(areas)
            for a in mlvl_areas:
                gi = base + a["index"]
                areas.append((world, a["mrea"], a["iname"].lower().endswith("_dark"), a["iname"]))
                mrea = pk.get(a["mrea"])
                sections = list(enumerate(ed.mrea_layers(mrea)))
                virtual = {}
                for layer, sec in sections:
                    objects = ed.scly_objects(sec)
                    for t4, eid, body in objects:
                        props = rm.pickup_props(body)
                        if t4 == "DOCK":
                            virtual[i32(props, dock_number, -1)] = bool(props.get(dock_virtual, b"\0")[-1])
                        elif t4 == "TEL1":
                            teleporters.append((gi, layer, eid, i32(props, TEL_WORLD_PROP, -1) & 0xFFFFFFFF,
                                                i32(props, TEL_AREA_PROP, -1) & 0xFFFFFFFF))
                        elif t4 == "PCKP":
                            add_pickup(locations, excluded_hits, a["iname"], gi, layer, eid, props, False)
                gen = ed.mrea_generated(mrea)
                if gen is not None:
                    for t4, eid, body in ed.scgn_objects(gen):
                        if t4 == "PCKP":
                            add_pickup(locations, excluded_hits, a["iname"], gi, -1, eid,
                                       rm.pickup_props(body), True)
                for di, conns in enumerate(a["docks"]):
                    for ca, cd in conns:
                        back = mlvl_areas[ca]["docks"][cd]
                        if (a["index"], di) not in back:
                            sys.exit(f"[rando_world] {name} area {a['index']} dock {di}: no reverse dock")
                        docks.append((gi, di, base + ca, cd, virtual.get(di, False)))
    missing = set(EXCLUDED) - excluded_hits
    if missing:
        sys.exit(f"[rando_world] exclusions that matched nothing: {sorted(missing)}")
    return worlds, areas, docks, teleporters, locations


def add_pickup(locations, excluded_hits, area_name, area, layer, eid, props, generated):
    item = i32(props, ITEM_PROP, -1)
    percent = i32(props, PERCENT_PROP, 0)
    if percent == 0 and (generated or item in REFILLS):
        return
    key = (area_name, layer, item)
    if key in EXCLUDED:
        excluded_hits.add(key)
        return
    locations.append((area, layer, eid, item, i32(props, AMOUNT_PROP, 1), i32(props, CAPACITY_PROP, 1)))


def main():
    worlds, areas, docks, teleporters, locations = collect()
    names = {v: k for k, v in rm.item_names().items()}
    keys = {(worlds[areas[a][0]][0], areas[a][1], eid) for a, _l, eid, *_ in locations}
    if len(keys) != len(locations):
        sys.exit("[rando_world] duplicate WORLD:AREA:ENTITY keys")
    print(f"{len(worlds)} worlds, {len(areas)} areas, {len(docks)} dock edges "
          f"({sum(d[4] for d in docks)} virtual), {len(teleporters)} teleporters, "
          f"{len(locations)} locations")
    if "--write" not in sys.argv[1:]:
        return
    lines = [
        "// Generated by tools/rando_world.py from the disc (see its docstring for the rules).",
        "// Do not edit by hand.",
        "#ifdef PORT_RANDO_WORLD",
        "// PORT_RANDO_WORLD(MLVL, name)",
    ]
    lines += [f'PORT_RANDO_WORLD(0x{m:08X}u, "{n}")' for m, n in worlds]
    lines += ["#endif", "#ifdef PORT_RANDO_AREA", "// PORT_RANDO_AREA(world, MREA, dark, name)"]
    lines += [f'PORT_RANDO_AREA({w}, 0x{m:08X}u, {"true" if d else "false"}, "{n}")'
              for w, m, d, n in areas]
    lines += ["#endif", "#ifdef PORT_RANDO_DOCK",
              "// PORT_RANDO_DOCK(area, dock, to area, to dock, virtual); areas index the area list"]
    lines += [f'PORT_RANDO_DOCK({a}, {d}, {ta}, {td}, {"true" if v else "false"})'
              for a, d, ta, td, v in docks]
    lines += ["#endif", "#ifdef PORT_RANDO_TELEPORTER",
              "// PORT_RANDO_TELEPORTER(area, layer, editor id, destination MLVL, destination MREA)"]
    lines += [f"PORT_RANDO_TELEPORTER({a}, {l}, 0x{e:08X}u, 0x{w:08X}u, 0x{m:08X}u)"
              for a, l, e, w, m in teleporters]
    lines += ["#endif", "#ifdef PORT_RANDO_LOCATION",
              "// PORT_RANDO_LOCATION(area, layer (-1 = generated section), editor id, item, amount,"
              " capacity, item name)"]
    lines += [f'PORT_RANDO_LOCATION({a}, {l}, 0x{e:08X}u, {i}, {am}, {c}, "{names.get(i, i)}")'
              for a, l, e, i, am, c in locations]
    lines.append("#endif")
    OUT.write_text("\n".join(lines) + "\n")
    print(f"wrote {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
