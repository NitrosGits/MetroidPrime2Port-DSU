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
  * dock locks - what passing each dock side needs, one row per dock edge (classify_locks):
      - beam: the DOOR wired to the dock (any connection with message INCR to its DOCK, as
        CScriptDoor finds it) and the player weapons its vulnerability lets through (multiplier
        != 0 and effect != Immune, CWeaponTypeVulnerability::DoesDamage, with LdrToDamage-
        Vulnerability's defaults): Light / Dark / Annihilator, Morph (health-5 door that power
        and boost both damage), else Any. NoDoor when the dock has no door.
      - shield: an immune door made shootable by RSET (CScriptDoor::ResetDoor) from a MemoryRelay.
        The relay is activated by a health-5 ACTR's DEAD, whose only damaging weapon names the
        shield (Missile / SuperMissile / PowerBomb), or by a relay behind a CNTR counting the
        DEADs of seeker-lockable DTRGs (Seeker). The shield door's beam is Any.
      - translator: a CRLY testing a translator item (> 0) activates a scan POIN named "...Gate...";
        nothing wires the gate to a door, so it belongs to the docked door nearest the POIN.
        translatorLayered: the CRLY sits on a layer that is off by default (MLVL layer flags).
      - scriptLock: LOCK sent to the door (or isLocked at load): Temporary when something in the
        area sends ULCK, else Permanent (isLocked counts as Permanent). scriptLockDefaultLayer:
        a LOCK sender (or the isLocked door) is on a layer that is on by default.
    Ing webs, grapple points and story layer switches are not wired to docks and are not read.

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
import savw_diff  # noqa: E402  (MLVL layer table)

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


EDITOR_PROP = 0x255A4580       # EditorProperties: name 'INAM', transform 'XFRM' (position first)
VULN_PROP = 0x7B71AE90         # DamageVulnerability (DOOR, ACTR, DTRG)
DOOR_LOCKED_PROP = 0xDEE730F5  # SLdrDoor isLocked
SEEKER_PROP = 0x5DFD7820       # SLdrDamageableTrigger enableSeekerLockOn
MULT_PROP, EFFECT_PROP = 0x852D3871, 0x68ACBD86  # WeaponVulnerability damageMultiplier, effect
EFFECT_IMMUNE = 3
# Player weapon slots of DamageVulnerability (ScriptLoader.cpp LdrToDamageVulnerability); the
# charged and combo shots count as their beam. Multiplier defaults to 0 for dark/light/annih/cannon.
WEAPONS = {
    0xAC8BB2A7: "power", 0xA442B4A3: "power", 0xB19B6D6C: "dark", 0x2A0F0524: "dark",
    0x1AB78BEF: "dark", 0x002444C6: "light", 0x949E3C86: "light", 0xFCDCB7D4: "light",
    0xE6BACDE5: "annih", 0x5BA991B1: "annih", 0x311FB061: "annih", 0x00EFE8CB: "missile",
    0x6924B8C1: "superMissile", 0x68AE13A0: "bomb", 0xF0B2F4CF: "pb", 0xD99C0400: "boost",
    0x866F91BF: "screw", 0xC70D8B8B: "cannon",
}
ZERO_DEFAULT = {0xB19B6D6C, 0x002444C6, 0xE6BACDE5, 0xC70D8B8B}
# CRLY conditional1..4: boolean (0 off), playerItem (enum in the last word), condition, value.
CRLY_TESTS = (0xCEC16932, 0xE709DDC0, 0x49614C51, 0xB498B424)
CRLY_BOOL, CRLY_ITEM = 0xDE3E40A3, 0xD3AF8D72
TRANSLATORS = {0x61: "Violet", 0x62: "Amber", 0x63: "Emerald", 0x64: "Cobalt"}
SHIELDS = {("missile",): "Missile", ("superMissile",): "SuperMissile", ("pb",): "PowerBomb"}
# Measured on G2ME01 (2026-10-27): docked doors, shields, translator gates per world, in pak order.
EXPECTED_LOCKS = [(111, 27, 7), (20, 0, 3), (162, 31, 2), (152, 27, 2), (124, 12, 2)]


def sub_props(raw):
    """A nested property struct: u16 count, then (u32 id, u16 size, data) each."""
    (count,) = struct.unpack_from(">H", raw, 0)
    p, out = 2, {}
    for _ in range(count):
        pid, size = struct.unpack_from(">IH", raw, p)
        out[pid] = raw[p + 6:p + 6 + size]
        p += 6 + size
    return out


def scly_entities(sec):
    """ed.scly_objects plus each object's connections: [(fourcc, eid, [(state, msg, target)], body)]."""
    count = struct.unpack_from(">I", sec, 10)[0]
    p, out = 14, []
    for _ in range(count):
        (ln,) = struct.unpack_from(">H", sec, p + 4)
        body = sec[p + 6:p + 6 + ln]
        eid, nc = struct.unpack_from(">IH", body, 0)
        conns = [(body[6 + 12 * i:10 + 12 * i].decode("latin1"), body[10 + 12 * i:14 + 12 * i].decode("latin1"),
                  struct.unpack_from(">I", body, 14 + 12 * i)[0]) for i in range(nc)]
        out.append((sec[p:p + 4].decode("latin1"), eid, conns, body[6 + 12 * nc:]))
        p += 6 + ln
    return out


def hurts(props):
    """Player weapons the object's DamageVulnerability lets damage through, as a sorted tuple."""
    slots = sub_props(props[VULN_PROP]) if VULN_PROP in props else {}
    out = set()
    for pid, weapon in WEAPONS.items():
        mult, effect = (0.0 if pid in ZERO_DEFAULT else 100.0), 0
        if pid in slots:
            f = sub_props(slots[pid])
            mult = struct.unpack(">f", f[MULT_PROP])[0] if MULT_PROP in f else mult
            effect = struct.unpack(">i", f[EFFECT_PROP])[0] if EFFECT_PROP in f else effect
        if mult != 0 and effect != EFFECT_IMMUNE:
            out.add(weapon)
    return tuple(sorted(out))


def editor(props, key):
    return sub_props(props[EDITOR_PROP]).get(key, b"") if EDITOR_PROP in props else b""


def classify_locks(where, ents, layer_on, dock_number):
    """{dock number: lock fields} for one area. ents: [(layer, fourcc, eid, conns, props)]."""
    by = {e[2]: e for e in ents}
    inc = {}
    for layer, t4, eid, conns, props in ents:
        for state, msg, target in conns:
            inc.setdefault(target, []).append((eid, state, msg))
    srcs = lambda eid, kind=None: [(s, st, m) for s, st, m in inc.get(eid, ()) if s in by and
                                   (kind is None or by[s][1] == kind)]  # noqa: E731
    on = lambda eid: bool(layer_on >> by[eid][0] & 1)  # noqa: E731
    name = lambda eid: editor(by[eid][4], 0x494E414D).rstrip(b"\0").decode("latin1")  # noqa: E731
    pos = lambda eid: struct.unpack(">3f", editor(by[eid][4], 0x5846524D)[:12])  # noqa: E731

    locks, door_dock = {}, {}
    for layer, t4, eid, conns, props in ents:
        if t4 != "DOOR":
            continue
        docks = [i32(by[t][4], dock_number, -1) for _s, m, t in conns
                 if m == "INCR" and t in by and by[t][1] == "DOCK"]
        if not docks:
            continue  # cutscene copies and in-room morph doors
        if len(set(docks)) != 1 or docks[0] in locks:
            sys.exit(f"[rando_world] {where} door {eid:#x}: not exactly one door per dock")
        dock, door_dock[eid] = docks[0], docks[0]
        weapons = hurts(props)
        shields = set()
        for relay, _st, msg in srcs(eid, "MRLY"):
            if msg != "RSET":
                continue
            for s, st, _m in srcs(relay):
                if by[s][1] == "ACTR" and st == "DEAD":
                    shields.add(SHIELDS.get(hurts(by[s][4]), "?"))
                elif by[s][1] == "SRLY":
                    seekers = [d for c, _x, _y in srcs(s, "CNTR") for d, dst, _z in srcs(c, "DTRG")
                               if dst == "DEAD" and by[d][4].get(SEEKER_PROP, b"\0")[-1]]
                    shields.add("Seeker" if seekers else "?")
        if len(shields) > 1 or "?" in shields or bool(shields) != (not weapons):
            sys.exit(f"[rando_world] {where} door {eid:#x}: weapons {weapons}, shields {shields}")
        if not weapons or "boost" not in weapons and "power" in weapons:
            beam = "Any"
        elif "power" in weapons:
            beam = "Morph"
        else:
            beam = {("light",): "Light", ("dark",): "Dark", ("annih",): "Annihilator"}.get(
                tuple(sorted({w for w in weapons if w in ("light", "dark", "annih")})))
            if beam is None:
                sys.exit(f"[rando_world] {where} door {eid:#x}: unclassified weapons {weapons}")
        lockers = [s for s, _st, m in srcs(eid) if m == "LOCK"]
        unlocked = any(m == "ULCK" for _s, _st, m in srcs(eid))
        initially = bool(props.get(DOOR_LOCKED_PROP, b"\0")[-1])
        script = "None"
        if initially or (lockers and not unlocked):
            script = "Permanent"
        elif lockers:
            script = "Temporary"
        locks[dock] = dict(beam=beam, shield=shields.pop() if shields else "None", translator="None",
                           translatorLayered=False, scriptLock=script,
                           scriptLockDefaultLayer=script != "None" and any(
                               on(s) for s in lockers + ([eid] if initially else [])))
    gates = 0
    for layer, t4, eid, conns, props in ents:
        if t4 != "CRLY":
            continue
        colours = {TRANSLATORS[i32(t, CRLY_ITEM, -1)] for t in
                   (sub_props(props[k]) for k in CRLY_TESTS if k in props)
                   if i32(t, CRLY_BOOL, 0) and i32(t, CRLY_ITEM, -1) in TRANSLATORS}
        if not colours:
            continue
        for _st, msg, target in conns:
            if msg != "ACTV" or target not in by or by[target][1] != "POIN" or "Gate" not in name(target):
                continue
            if len(colours) != 1 or not door_dock:
                sys.exit(f"[rando_world] {where} gate {target:#x}: {colours}, {len(door_dock)} docked doors")
            p = pos(target)
            door = min(door_dock, key=lambda d: sum((a - b) ** 2 for a, b in zip(pos(d), p)))
            lock = locks[door_dock[door]]
            lock["translator"], lock["translatorLayered"] = next(iter(colours)), not on(eid)
            gates += 1
    return locks, gates


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
    dock_locks, lock_totals = {}, []
    excluded_hits = set()
    for pk in ed.load_paks(iso):
        if not SINGLE_PLAYER_PAKS.search(pk.path):
            continue
        for name, (kind, mlvl) in sorted(pk.names.items(), key=lambda kv: kv[1][1]):
            if kind != "MLVL":
                continue
            world = len(worlds)
            mlvl_areas = ed.parse_mlvl(pk.get(mlvl))["areas"]
            layer_on = savw_diff.parse_mlvl(pk.get(mlvl))["layerDefaults"]
            totals = [0, 0, 0]
            worlds.append((mlvl, name))
            base = len(areas)
            for a in mlvl_areas:
                gi = base + a["index"]
                areas.append((world, a["mrea"], a["iname"].lower().endswith("_dark"), a["iname"]))
                mrea = pk.get(a["mrea"])
                sections = list(enumerate(ed.mrea_layers(mrea)))
                virtual, ents = {}, []
                for layer, sec in sections:
                    for t4, eid, conns, body in scly_entities(sec):
                        props = rm.pickup_props(body)
                        ents.append((layer, t4, eid, conns, props))
                        if t4 == "DOCK":
                            virtual[i32(props, dock_number, -1)] = bool(props.get(dock_virtual, b"\0")[-1])
                        elif t4 == "TEL1":
                            teleporters.append((gi, layer, eid, i32(props, TEL_WORLD_PROP, -1) & 0xFFFFFFFF,
                                                i32(props, TEL_AREA_PROP, -1) & 0xFFFFFFFF))
                        elif t4 == "PCKP":
                            add_pickup(locations, excluded_hits, a["iname"], gi, layer, eid, props, False)
                locks, gates = classify_locks(f"{name} {a['iname']}", ents, layer_on[a["index"]], dock_number)
                totals[0] += len(locks)
                totals[1] += sum(lk["shield"] != "None" for lk in locks.values())
                totals[2] += gates
                for di in range(len(a["docks"])):
                    dock_locks[gi, di] = locks.get(di, dict(
                        beam="NoDoor", shield="None", translator="None", translatorLayered=False,
                        scriptLock="None", scriptLockDefaultLayer=False))
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
            lock_totals.append(tuple(totals))
    missing = set(EXCLUDED) - excluded_hits
    if missing:
        sys.exit(f"[rando_world] exclusions that matched nothing: {sorted(missing)}")
    if lock_totals != EXPECTED_LOCKS:
        sys.exit(f"[rando_world] (docked doors, shields, translator gates) per world {lock_totals},"
                 f" expected {EXPECTED_LOCKS}")
    return worlds, areas, docks, teleporters, locations, dock_locks


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
    worlds, areas, docks, teleporters, locations, dock_locks = collect()
    names = {v: k for k, v in rm.item_names().items()}
    keys = {(worlds[areas[a][0]][0], areas[a][1], eid) for a, _l, eid, *_ in locations}
    if len(keys) != len(locations):
        sys.exit("[rando_world] duplicate WORLD:AREA:ENTITY keys")
    print(f"{len(worlds)} worlds, {len(areas)} areas, {len(docks)} dock edges "
          f"({sum(d[4] for d in docks)} virtual), {len(teleporters)} teleporters, "
          f"{len(locations)} locations")
    for field in ("beam", "shield", "translator", "scriptLock"):
        counts = {}
        for lock in dock_locks.values():
            counts[lock[field]] = counts.get(lock[field], 0) + 1
        print(f"dock sides by {field}: " + ", ".join(f"{k} {v}" for k, v in sorted(counts.items())))
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
    lines += ["#endif", "#ifdef PORT_RANDO_DOCK_LOCK",
              "// PORT_RANDO_DOCK_LOCK(area, dock, beam, shield, translator, translatorLayered,"
              " scriptLock, scriptLockDefaultLayer); one row per PORT_RANDO_DOCK, same order"]
    for a, d, *_ in docks:
        lk = dock_locks[a, d]
        lines.append(f'PORT_RANDO_DOCK_LOCK({a}, {d}, {lk["beam"]}, {lk["shield"]}, {lk["translator"]}, '
                     f'{"true" if lk["translatorLayered"] else "false"}, {lk["scriptLock"]}, '
                     f'{"true" if lk["scriptLockDefaultLayer"] else "false"})')
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
