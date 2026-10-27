#!/usr/bin/env python3
"""Build the randomizer's donor-model table from the user's own disc.

usage: tools/rando_models.py [--write]

A randomized pickup keeps its location's model unless the port can swap in the model of the item
it now gives. Echoes has no item -> model table in code (each Pickup carries its own CMDL and
animation set), so the table is read from the disc: every PCKP of every area of the world MLVLs
the randomizer knows (the ones in platform/include/port_rando_locations.inc) is walked, and for
each item the (CMDL 0xc27ffa8f, animation set 0xe25fb08c = {ANCS, character index, initial
animation}) pairs it uses are counted.

Donor rule (deterministic):
  * Pairs whose CMDL is 0xffffffff are skipped (nothing to show).
  * A pair is *resident* in a world when that world's own pak (the one holding the MLVL) holds
    the CMDL and, when the animation set names an ANCS (id != 0xffffffff), the ANCS too. The
    game's resource lookup only sees the paks of the loaded world, so only these can be built.
  * Pairs are ranked by how many Pickups on disc use them (most first), then by (CMDL, ANCS).
    The first pair becomes the item's primary row, listing every world it is resident in.
  * For each world the primary row does not cover, the best-ranked pair resident there gets an
    extra row (repeat until no world is left that some pair covers). So an item has one row, or
    more when different worlds need different models.
An item with no PCKP on disc, or only 0xffffffff models, gets no row.

--write regenerates platform/include/port_rando_models.inc. Read-only otherwise.
"""
import collections
import pathlib
import re
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402
import rando_locations as rl  # noqa: E402

OUT = ROOT / "platform" / "include" / "port_rando_models.inc"
LOCATIONS = ROOT / "platform" / "include" / "port_rando_locations.inc"
MODEL_PROP = 0xC27FFA8F
ANIM_PROP = 0xE25FB08C
INVALID = 0xFFFFFFFF


def pickup_props(body):
    _magic, _size, count = struct.unpack_from(">IHH", body, 0)
    p, out = 8, {}
    for _ in range(count):
        pid, size = struct.unpack_from(">IH", body, p)
        out[pid] = body[p + 6:p + 6 + size]
        p += 6 + size
    return out


def location_worlds():
    """MLVL ids of platform/include/port_rando_locations.inc rows ({index, 0xMLVLu, ...})."""
    return sorted({int(line.split(",")[1].strip()[2:10], 16)
                   for line in LOCATIONS.read_text().splitlines() if line.startswith("{")})


def collect():
    iso = ed.Iso(ed.find_iso() or sys.exit("no disc image (set MP2_DISC)"))
    worlds = set(location_worlds())
    pairs = collections.defaultdict(collections.Counter)  # item -> Counter((cmdl, anim3))
    resident = {}  # world mlvl -> set of asset ids in that world's pak
    for pk in ed.load_paks(iso):
        for _nm, (t, mid) in pk.names.items():
            if t != "MLVL" or mid not in worlds or mid in resident:
                continue
            resident[mid] = set(pk.res)
            for a in ed.parse_mlvl(pk.get(mid))["areas"]:
                mrea = pk.get(a["mrea"])
                sections = [ed.scly_objects(sec) for sec in ed.mrea_layers(mrea)]
                gen = ed.mrea_generated(mrea)
                if gen is not None:
                    sections.append(ed.scgn_objects(gen))
                for objects in sections:
                    for t4, _eid, body in objects:
                        if t4 != "PCKP":
                            continue
                        props = pickup_props(body)
                        if rl.ITEM_PROP not in props or MODEL_PROP not in props:
                            continue
                        item = struct.unpack(">I", props[rl.ITEM_PROP][-4:])[0]
                        model = struct.unpack(">I", props[MODEL_PROP][:4])[0]
                        anim = struct.unpack(">3I", props[ANIM_PROP][:12]) if ANIM_PROP in props else (INVALID, 0, 0)
                        pairs[item][(model, anim)] += 1
    return pairs, resident


def rows_for(pairs, resident):
    rows = []
    for item in sorted(pairs):
        ranked = sorted((p for p in pairs[item] if p[0] != INVALID),
                        key=lambda p: (-pairs[item][p], p[0], p[1]))

        def worlds_of(pair):
            model, anim = pair
            return [w for w in sorted(resident)
                    if model in resident[w] and (anim[0] == INVALID or anim[0] in resident[w])]

        covered = set()
        for pair in ranked:
            fresh = set(worlds_of(pair)) - covered
            if fresh:
                rows.append((item, pair, worlds_of(pair)))
                covered |= fresh
        if ranked and not covered:  # resident nowhere: keep the best pair as a last-resort donor
            rows.append((item, ranked[0], []))
    return rows


def main():
    pairs, resident = collect()
    rows = rows_for(pairs, resident)
    names = rl.item_names()
    by_value = {v: k for k, v in names.items()}
    have = {r[0] for r in rows}
    missing = [by_value.get(i, str(i)) for i in range(max(names.values()) + 1) if i not in have]
    print(f"{len(rows)} donor rows for {len(have)} items; no donor for: {', '.join(missing)}")
    for item, (model, anim), worlds in rows:
        if not worlds:
            print(f"  note: {by_value.get(item, item)} model {model:08X} is resident in no world")
    if "--write" in sys.argv[1:]:
        lines = [
            "// Generated by tools/rando_models.py from the disc (see its docstring for the donor rule).",
            "// Do not edit by hand.",
            "// { item, CMDL, { ANCS, character index, initial animation }, { resident world MLVLs, 0-terminated }, name }",
        ]
        for item, (model, anim), worlds in rows:
            w = ", ".join(f"0x{x:08X}u" for x in worlds) + ", 0u" * (6 - len(worlds))
            lines.append(f'{{{item}, 0x{model:08X}u, {{0x{anim[0]:08X}u, 0x{anim[1]:08X}u, 0x{anim[2]:08X}u}}, '
                         f'{{{w}}}, "{by_value.get(item, item)}"}},')
        OUT.write_text("\n".join(lines) + "\n")
        print(f"wrote {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
