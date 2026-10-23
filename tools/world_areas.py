#!/usr/bin/env python3
"""A world's area slots, read from the disc's MLVL: what `tools/sweep_areas.sh WORLD=` walks.

    python3 tools/world_areas.py <world>        # the world's header line, then one line per slot
    python3 tools/world_areas.py --list         # every MLVL on the disc, one line each
    python3 tools/world_areas.py --campaign     # the five campaign worlds' ids, sweep order

`<world>` is an MLVL id (`3bfa3eff`, `0x3bfa3eff`), a display-name alias (`temple grounds`,
`agon`, `torvus`, `sanctuary`, `great temple`, case and spaces ignored) or a case-insensitive part
of the MLVL name (`sand_world`, `Temple_Int`). The output is plain words for the shell:

    world 42b935e4 03_Sand_World_20041018 72 Agon Wastes
    0 <mrea id> light 00_Elev_Sand_Temple_A
    7 <mrea id> dark 01_sand_entrance_dark

The slot index is the one `[area] N` and the warp's `kArea` print; the MREA id is what a
`MP2_WORLD="<world>,<mrea>"` boot takes (a plain index there is read as an asset id, not a slot).
Light/dark is the MLVL name's `_dark` suffix, which is how every Dark Aether area on the disc is
named (Great Temple 9-11 included); the ending's `game_end_partN` slots count as light.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import echoes_disc as ed  # noqa: E402

# Display name -> MLVL id. The game's own names live in STRGs; these are the five campaign worlds.
CAMPAIGN = [
    ("3bfa3eff", "Temple Grounds", ("templegrounds", "temple", "grounds", "tg", "templehub")),
    ("863fcd72", "Great Temple", ("greattemple", "great", "gt", "templeint")),
    ("42b935e4", "Agon Wastes", ("agonwastes", "agon", "aw", "sand", "sandworld")),
    ("3dfd2249", "Torvus Bog", ("torvusbog", "torvus", "tb", "swamp", "swampworld")),
    ("1baa96c2", "Sanctuary Fortress", ("sanctuaryfortress", "sanctuary", "sf", "cliff",
                                        "cliffworld")),
]


def mlvls():
    path = ed.find_iso()
    if path is None:
        sys.exit("[world_areas] no disc image (set MP2_DISC)")
    iso = ed.Iso(path)
    seen = {}
    for pak in ed.load_paks(iso):
        for name, (kind, rid) in pak.names.items():
            if kind == "MLVL" and rid not in seen:
                seen[rid] = (name, pak)
    return seen


def display_name(rid):
    for wid, name, _ in CAMPAIGN:
        if int(wid, 16) == rid:
            return name
    return ""


def resolve(text, worlds):
    key = text.lower().replace(" ", "").replace("_", "").replace("-", "")
    for wid, name, aliases in CAMPAIGN:
        if key in aliases:
            return int(wid, 16)
    hexpart = text[2:] if text.lower().startswith("0x") else text
    try:
        rid = int(hexpart, 16)
        if rid in worlds:
            return rid
    except ValueError:
        pass
    hits = [rid for rid, (name, _) in worlds.items() if text.lower() in name.lower()]
    if len(hits) == 1:
        return hits[0]
    if hits:
        sys.exit("[world_areas] '%s' matches several MLVLs: %s" %
                 (text, " ".join(worlds[r][0] for r in hits)))
    sys.exit("[world_areas] no world matches '%s' (try --list)" % text)


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    arg = sys.argv[1]
    if arg == "--campaign":
        print(" ".join(wid for wid, _, _ in CAMPAIGN))
        return 0
    worlds = mlvls()
    if arg == "--list":
        for rid, (name, pak) in sorted(worlds.items(), key=lambda kv: kv[1][1].path):
            count = len(ed.parse_mlvl(pak.get(rid))["areas"])
            print("%08x %-28s %3d %s" % (rid, name, count, display_name(rid)))
        return 0
    rid = resolve(arg, worlds)
    name, pak = worlds[rid]
    areas = ed.parse_mlvl(pak.get(rid))["areas"]
    print("world %08x %s %d %s" % (rid, name, len(areas), display_name(rid)))
    for a in areas:
        side = "dark" if a["iname"].lower().endswith("_dark") else "light"
        print("%d %08x %s %s" % (a["index"], a["mrea"], side, a["iname"] or "-"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
