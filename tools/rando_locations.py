#!/usr/bin/env python3
"""Map Randovania's Echoes pickup indices to the port's randomizer location keys, checked on disc.

usage: tools/rando_locations.py RANDOVANIA_DIR [--write]

RANDOVANIA_DIR is a Randovania checkout (or its randovania/games/prime2/logic_database). Each
pickup node there names its region (MLVL asset id), its area (MREA asset id) and, for the
'standard' locations, the instance id of the area's Pickup object. The port keys a location by
the same three values (PortRandomizer::FormatLocationKey, fed from LoadPickup), so the mapping
is read straight from the database - and then every row is checked against the user's own disc
(tools/echoes_disc.py): the MLVL must list the MREA, the MREA must hold a PCKP with that editor
id (in a script layer or, for boss drops, the generated-object section), and the pickup's item must be the database's original_model. 'custom' locations have no
pickup on disc (Randovania spawns one, e.g. for items a script grants) and are listed with
instance 0 for later work.

Prints a summary and every failed check; exits non-zero when a check fails. --write regenerates
platform/include/port_rando_locations.inc, the table the runtime compiles in. Read-only otherwise.
"""
import json
import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

OUT = ROOT / "platform" / "include" / "port_rando_locations.inc"
ITEM_PROP = 0xA02EF0C4  # SLdrPickup itemToGive (include/MetroidPrime/ScriptLoader/SLdrPickup.inc)

# Randovania's original_model names that differ from the EItemType enumerator (kIT_ dropped).
MODEL_TO_ITEM = {
    "EnergyTank": "EnergyTanks", "MissileExpansion": "Missile", "MissileLauncher": "Missile",
    "MorphBallBomb": "MorphBallBombs", "PowerBombExpansion": "Powerbomb", "PowerBomb": "Powerbomb", "BeamAmmoExpansion": "DarkAmmo",
    # The disc's Energy Transfer Module pickup gives item 28, not kIT_EnergyTransferModule.
    "EnergyTransferModule": "TranslatorUpgrade_TempETM",
}
KEY_MODELS = ("TempleKey", "SkyTempleKey", "DarkTempleKey")  # any *Key* item of the disc
# Rows whose original_model is not the item the disc gives, checked by hand: Randovania shows
# the Echo Visor model for the Dark Visor location.
MODEL_EXCEPTIONS = {(79, "DarkVisor")}


def item_names():
    """EItemType enumerator -> value, from the header the runtime's item table also follows."""
    src = (ROOT / "include" / "MetroidPrime" / "Player" / "CPlayerState.hpp").read_text()
    body = src[src.index("enum EItemType"):]
    body = body[body.index("{") + 1:body.index("}")]
    out, nxt = {}, 0
    for line in body.split(","):
        tok = line.split("//")[0].strip()
        if not tok.startswith("kIT_"):
            continue
        if "=" in tok:
            name, val = (t.strip() for t in tok.split("=", 1))
            nxt = int(val, 0)
        else:
            name = tok
        out[name[4:]] = nxt
        nxt += 1
    return out


def db_dir(arg):
    p = pathlib.Path(arg)
    for cand in (p, p / "randovania" / "games" / "prime2" / "logic_database"):
        if (cand / "header.json").exists():
            return cand
    sys.exit(f"no prime2 logic_database under {arg}")


def db_locations(d):
    regions = [json.loads(f.read_text()) for f in sorted(d.glob("*.json")) if f.name != "header.json"]
    # A dark-world region carries no asset id: it is areas of its light world's MLVL.
    mlvl = {r["name"]: r["extra"].get("asset_id") for r in regions}
    rows = []
    for reg in regions:
        world = mlvl[reg["name"]] or mlvl[reg["extra"]["associated_region"]]
        for an, area in reg["areas"].items():
            for nn, node in area["nodes"].items():
                if node.get("node_type") != "pickup":
                    continue
                ld = node["extra"].get("location_data", {})
                rows.append({
                    "index": node["pickup_index"], "region": reg["name"], "area": an, "node": nn,
                    "world": world, "mrea": area["extra"]["asset_id"],
                    "type": ld.get("type"), "instance": ld.get("instances", {}).get("pickup", 0),
                    "model": ld.get("original_model", ""),
                })
    return sorted(rows, key=lambda r: r["index"])


def pickup_item(body):
    # Root struct: u32 0xFFFFFFFF, u16 size, u16 property count, then (u32 id, u16 size, data).
    _magic, _size, count = struct.unpack_from(">IHH", body, 0)
    p = 8
    for _ in range(count):
        pid, size = struct.unpack_from(">IH", body, p)
        if pid == ITEM_PROP:
            return struct.unpack_from(">I", body, p + 6)[0]
        p += 6 + size
    return None


def disc_pickups(worlds):
    """{(mlvl, mrea): {eid: item}} for every PCKP of the areas the database names."""
    iso = ed.Iso(ed.find_iso() or sys.exit("no disc image (set MP2_DISC)"))
    seen, out = set(), {}
    for pk in ed.load_paks(iso):
        for _nm, (t, mid) in pk.names.items():
            if t != "MLVL" or mid not in worlds or mid in seen:
                continue
            seen.add(mid)
            for a in ed.parse_mlvl(pk.get(mid))["areas"]:
                objs = out.setdefault((mid, a["mrea"]), {})
                if a["mrea"] not in worlds[mid]:
                    continue
                mrea = pk.get(a["mrea"])
                sections = [ed.scly_objects(sec) for sec in ed.mrea_layers(mrea)]
                gen = ed.mrea_generated(mrea)
                if gen is not None:
                    sections.append(ed.scgn_objects(gen))
                for objects in sections:
                    for t4, eid, body in objects:
                        if t4 == "PCKP":
                            objs[eid] = pickup_item(body)
    return out


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    rows = db_locations(db_dir(sys.argv[1]))
    items = item_names()
    by_value = {v: k for k, v in items.items()}
    worlds = {}
    for r in rows:
        worlds.setdefault(r["world"], set()).add(r["mrea"])
    disc = disc_pickups(worlds)
    bad = []
    idx = [r["index"] for r in rows]
    if idx != list(range(len(rows))):
        bad.append(f"pickup indices are not 0..{len(rows) - 1}")
    for r in rows:
        where = f"#{r['index']} {r['region']}/{r['area']}/{r['node']}"
        if (r["world"], r["mrea"]) not in disc:
            bad.append(f"{where}: MLVL {r['world']:08X} does not list MREA {r['mrea']:08X}")
            continue
        if r["type"] != "standard":
            continue
        item = disc[(r["world"], r["mrea"])].get(r["instance"], "missing")
        want = items.get(MODEL_TO_ITEM.get(r["model"], r["model"]))
        name = by_value.get(item, str(item))
        if item == "missing":
            bad.append(f"{where}: no PCKP {r['instance']:08X} in MREA {r['mrea']:08X}")
        elif r["model"] in KEY_MODELS:
            if "Key" not in name:
                bad.append(f"{where}: disc item {name} is not a key")
        elif (r["index"], name) in MODEL_EXCEPTIONS:
            pass
        elif want is None:
            bad.append(f"{where}: original_model {r['model']} has no EItemType mapping")
        elif item != want:
            bad.append(f"{where}: disc item {by_value.get(item, item)} != model {r['model']}")
    std = sum(r["type"] == "standard" for r in rows)
    print(f"{len(rows)} locations ({std} standard, {len(rows) - std} custom), "
          f"{len(worlds)} worlds; {len(bad)} failed checks")
    for b in bad:
        print("  " + b)
    if "--write" in sys.argv[2:] and not bad:
        lines = [
            "// Generated by tools/rando_locations.py from Randovania's Echoes logic database",
            "// (pickup_index order) and checked against the disc. Do not edit by hand.",
            "// { pickup index, MLVL, MREA, Pickup instance id (0: custom location), region, area }",
        ]
        for r in rows:
            lines.append(f'{{{r["index"]}, 0x{r["world"]:08X}u, 0x{r["mrea"]:08X}u, '
                         f'0x{r["instance"]:08X}u, "{r["region"]}", "{r["area"]}"}},')
        OUT.write_text("\n".join(lines) + "\n")
        print(f"wrote {OUT.relative_to(ROOT)}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
