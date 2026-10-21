#!/usr/bin/env python3
"""List, per area and layer, the SpacePirate placements and the script objects they need.

usage: tools/scan_area_actors.py [fourcc]   (fourcc defaults to PIRT, the SpacePirate placement)

Read-only, over the user's own disc (tools/echoes_disc.py's readers). For every area it prints
the SpacePirate placements with their data flags, the radii that matter and the equipped
weapon, and it counts the script objects the pirate's states look for: the cover points (COVR,
with the crouch and wall-hang flags), the AI jump points (AJMP), the AI waypoints (AIWP) and
the team AI managers (TMAI). The property ids are the generated readers':
SLdrSpacePirate (0xa64ab9b8 flags, 0x9579b1f2 aggressiveness, 0xf89ab419 cover check,
0xed9bf5a3 search radius, 0xed69488f hearing radius, 0xdc89cc3c the weapon data with
0x647670ac the equipped weapon) and SLdrCoverPoint (0x969de5ff the flags, 0x8001c3be the
crouch flag).
"""
import math
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

PIRT_PROPS = {
    0x9579B1F2: ("aggr", ">f"),
    0xF89AB419: ("cover", ">f"),
    0xED9BF5A3: ("search", ">f"),
    0xED69488F: ("hearing", ">f"),
    0xA64AB9B8: ("flags", ">I"),
}
WEAPON_PROP = 0xDC89CC3C
EQUIPPED_PROP = 0x647670AC
COVER_FLAGS = 0x969DE5FF
COVER_CROUCH = 0x8001C3BE


def props_of(body):
    """The named properties of a SCLY property block (u32 magic, u16 size, u16 count, ...)."""
    out = {}
    try:
        magic, _size, count = struct.unpack_from(">IHH", body, 0)
        if magic != 0xFFFFFFFF:
            return out
        p = 8
        for _ in range(count):
            pid, size = struct.unpack_from(">IH", body, p)
            data = body[p + 6:p + 6 + size]
            p += 6 + size
            if pid in PIRT_PROPS and size >= 4:
                fmt = PIRT_PROPS[pid][1]
                out[PIRT_PROPS[pid][0]] = struct.unpack_from(fmt, data, 0)[0]
            elif pid == WEAPON_PROP:
                out["weapon"] = equipped(data)
    except struct.error:
        return out
    return out


def equipped(block):
    """The equipped weapon of a weapon-data block (the property 0x647670ac)."""
    try:
        count = struct.unpack_from(">H", block, 0)[0]
        p = 2
        for _ in range(count):
            pid, size = struct.unpack_from(">IH", block, p)
            data = block[p + 6:p + 6 + size]
            p += 6 + size
            if pid == EQUIPPED_PROP and size >= 4:
                return struct.unpack_from(">i", data, 0)[0]
    except struct.error:
        pass
    return 0


def position(body):
    """The editor properties' transform position (the property 0x5846524d) of an object."""
    try:
        magic, _size, count = struct.unpack_from(">IHH", body, 0)
        if magic != 0xFFFFFFFF:
            return None
        p = 8
        for _ in range(count):
            pid, size = struct.unpack_from(">IH", body, p)
            data = body[p + 6:p + 6 + size]
            p += 6 + size
            if pid == 0x5846524D and size >= 36:
                return struct.unpack_from(">3f", data, 0)
    except struct.error:
        pass
    return None


def cover_flags(body):
    """(flags, crouch) of a cover point."""
    flags = 0
    crouch = False
    try:
        count = struct.unpack_from(">H", body, 0)[0]
        p = 2
        for _ in range(count):
            pid, size = struct.unpack_from(">IH", body, p)
            data = body[p + 6:p + 6 + size]
            p += 6 + size
            if pid == COVER_FLAGS and size >= 4:
                flags = struct.unpack_from(">i", data, 0)[0] & 0xFFFFFFFF
            elif pid == COVER_CROUCH and size >= 1:
                crouch = data[0] != 0
    except struct.error:
        pass
    return flags, crouch


def main():
    fourcc = sys.argv[1] if len(sys.argv) > 1 else "PIRT"
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    for pk in ed.load_paks(iso):
        for nm, (t, mid) in pk.names.items():
            if t != "MLVL":
                continue
            try:
                w = ed.parse_mlvl(pk.get(mid))
            except Exception:
                continue
            for a in w["areas"]:
                iname = a["iname"] or f"area{a['index']}"
                try:
                    layers = ed.mrea_layers(pk.get(a["mrea"]))
                except Exception:
                    continue
                for li, sec in enumerate(layers):
                    pirates = []
                    covers = []
                    counts = {"AJMP": 0, "AIWP": 0, "TMAI": 0}
                    for t4, eid, body in ed.scly_objects(sec):
                        if t4 == fourcc:
                            pirates.append((eid, props_of(body), position(body)))
                        elif t4 == "COVR":
                            covers.append((eid,) + cover_flags(body) + (position(body),))
                        elif t4 in counts:
                            counts[t4] += 1
                    if not pirates and not covers and not any(counts.values()):
                        continue
                    print(f"{nm}/{iname} L{li}: {fourcc}={len(pirates)} COVR={len(covers)} "
                          f"AJMP={counts['AJMP']} AIWP={counts['AIWP']} TMAI={counts['TMAI']}")
                    for eid, p, pos in pirates:
                        print(f"    {fourcc} eid=0x{eid:04x} flags=0x{p.get('flags', 0):08x} "
                              f"aggr={p.get('aggr', 0)} cover={p.get('cover', 0)} "
                              f"search={p.get('search', 0)} hearing={p.get('hearing', 0)} "
                              f"weapon={p.get('weapon', 0)} pos={pos}")
                    for eid, flags, cr, cpos in covers:
                        tag = "wall-hang" if (flags >> 4) & 1 else ("crouch" if cr else "")
                        print(f"    COVR eid=0x{eid:04x} {tag} pos={cpos}")
                    hang = [c for c in covers if (c[1] >> 4) & 1]
                    crouch = [c for c in covers if c[2]]
                    for eid, flags, cr, cpos in hang:
                        print(f"    COVR eid=0x{eid:04x} wall-hang pos={cpos}")
                    for eid, flags, cr, cpos in crouch:
                        print(f"    COVR eid=0x{eid:04x} crouch pos={cpos}")
                    # the pairs that could work: a pirate whose cover search radius reaches a
                    # cover point at nearly the same height (|dz| < 3).
                    for eid, p, pos in pirates:
                        if pos is None:
                            continue
                        for ceid, cflags, ccr, cpos in covers:
                            if cpos is None:
                                continue
                            d = math.dist(pos, cpos)
                            dz = abs(pos[2] - cpos[2])
                            radius = p.get("search", 0.0)
                            if d <= radius and dz < 3.0:
                                print(f"    PAIR {fourcc} eid=0x{eid:04x} cover=0x{ceid:04x} "
                                      f"dist={d:.1f} dz={dz:.1f} radius={radius} "
                                      f"crouch={ccr} wallhang={(cflags >> 4) & 1}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
