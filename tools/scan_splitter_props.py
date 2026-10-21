#!/usr/bin/env python3
"""Print every Splitter command module (SPLU) placement's dodge and laser-pulse properties.

usage: tools/scan_splitter_props.py

Read-only, like tools/scan_pirt_props.py: tools/echoes_disc.py's readers and the user's own disc
image. The properties sit in the SLdrSplitterCommandModuleData struct (0x560cf813), a nested
property list (u16 count, then u32 id + u16 size + payload); the walk searches every nesting level.
A property absent from a placement keeps the loader's default (minDodges 1, maxDodges 3,
dodgeChance 100, maxLaserPulseShots 3, unknown_0xbd80fd94 10).
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

PROPS = {
    0x47BE3298: ("dodgeChance", ">f"),
    0xEDA45014: ("minDodges", ">i"),
    0x7DD740FE: ("maxDodges", ">i"),
    0xBD80FD94: ("flags", ">i"),
}


def walk(buf, out, depth=0):
    """Read buf as a property list; True when it parses exactly."""
    if depth > 6 or len(buf) < 2:
        return False
    count = struct.unpack_from(">H", buf, 0)[0]
    p = 2
    items = []
    for _ in range(count):
        if p + 6 > len(buf):
            return False
        pid, size = struct.unpack_from(">IH", buf, p)
        items.append((pid, buf[p + 6:p + 6 + size]))
        p += 6 + size
    if p != len(buf):
        return False
    for pid, payload in items:
        if pid in PROPS and len(payload) == 4:
            name, fmt = PROPS[pid]
            out[name] = struct.unpack(fmt, payload)[0]
        else:
            walk(payload, out, depth + 1)
    return True


def main():
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    rows = 0
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
                    for t4, eid, body in ed.scly_objects(sec):
                        if t4 != "SPLU":
                            continue
                        out = {}
                        if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                            walk(body[6:], out)
                        rows += 1
                        print(f"{nm}/{iname} L{li} eid=0x{eid:04x} " +
                              " ".join(f"{k}={v}" for k, v in sorted(out.items())))
    print(f"== {rows} SPLU placements")
    return 0


if __name__ == "__main__":
    sys.exit(main())
