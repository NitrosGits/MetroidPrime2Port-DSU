#!/usr/bin/env python3
"""Print every SpacePirate placement's SLdrSpacePirate properties, straight from the disc.

usage: tools/scan_pirt_props.py [fourcc]   (fourcc defaults to PIRT, the SpacePirate placement)

Read-only: it uses tools/echoes_disc.py's readers (ISO, PAK, MLVL, MREA, SCLY) and the user's own
disc image. A SCLY property block is: u32 magic (0xffffffff), u16 block size, u16 property count,
then per property u32 id + u16 size + payload, all big endian. The flags property (0xa64ab9b8) is
the data flag word the port unpacks into 0x8f4..0x8fa: bit 0 is x8f4_24, bit 8 x8f5_24, bit 16
x8f6_24, bit 20 x8f7_24, bit 24 x8f8_24, bit 26 x8fa_26 - so the shadow bit is 0x2000 and the
trooper bit 0x10000. The scan is what shows that no shipped SpacePirate carries the shadow flag.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

FLOATS = {
    0x9579B1F2: "aggr",
    0xF89AB419: "cover",
    0xED9BF5A3: "search",
    0xC3A27CF8: "fallback",
    0xF0CF5DD7: "fallbackR",
    0xED69488F: "hearing",
}
INT_PROPS = {0xA64AB9B8: "flags"}


def parse_props(body):
    """The named SLdrSpacePirate properties of one SCLY property block."""
    out = {}
    try:
        magic, _blksize, count = struct.unpack_from(">IHH", body, 0)
        if magic != 0xFFFFFFFF:
            return out
        p = 8
        for _ in range(count):
            pid, size = struct.unpack_from(">IH", body, p)
            payload = body[p + 6:p + 6 + size]
            p += 6 + size
            if pid in FLOATS and size >= 4:
                out[FLOATS[pid]] = struct.unpack_from(">f", payload, 0)[0]
            elif pid in INT_PROPS and size >= 4:
                out[INT_PROPS[pid]] = struct.unpack_from(">i", payload, 0)[0] & 0xFFFFFFFF
    except struct.error:
        return {}
    return out


def main():
    fourcc = sys.argv[1] if len(sys.argv) > 1 else "PIRT"
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    paks = ed.load_paks(iso)
    rows = []
    for pk in paks:
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
                        if t4 != fourcc:
                            continue
                        props = parse_props(body)
                        rows.append((nm, iname, li, eid, props))
                        flags = props.get("flags", 0)
                        print(f"{nm}/{iname} L{li} eid=0x{eid:04x} " +
                              " ".join(f"{k}={v}" for k, v in sorted(props.items())) +
                              f" shadow={bool(flags & 0x2000)} trooper={bool(flags & 0x10000)}"
                              f" amb={bool(flags & 0x1)} warp={bool(flags & 0x04000000)}")
    print(f"== {len(rows)} {fourcc} placements")
    return 0


if __name__ == "__main__":
    sys.exit(main())
