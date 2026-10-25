#!/usr/bin/env python3
"""Print every Splinter (SPTR) placement's Mega Splinter properties.

usage: tools/scan_splinter_props.py

Read-only, like tools/scan_splitter_props.py: tools/echoes_disc.py's readers and the user's own disc
image. LoadSplinter (SLdrSplinter.inc) reads isMegaSplinter (0x7dc82f46, bool),
megaSplinterSpitProjectile (0x42518359) and megaSplinterSpitVisorEffect (0x496f191b) from the
top-level property list; absent properties keep the SLdrSplinter defaults (false, invalid ids).
SSpitData's mVisorEffect is engaged when the ctor gets isMegaSplinter and visorEffect != 0xffffffff.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

PROPS = {
    0x7DC82F46: ("mega", ">?"),
    0x42518359: ("spit", ">I"),
    0x496F191B: ("visor", ">I"),
}


def props(buf):
    """The top-level property list of one SPTR body (struct id + u16 size first)."""
    out = {}
    if len(buf) < 8:
        return None
    p = 6
    (count,) = struct.unpack_from(">H", buf, p)
    p += 2
    for _ in range(count):
        if p + 6 > len(buf):
            return None
        pid, size = struct.unpack_from(">IH", buf, p)
        payload = buf[p + 6:p + 6 + size]
        if pid in PROPS:
            name, fmt = PROPS[pid]
            out[name] = struct.unpack(fmt, payload[:struct.calcsize(fmt)])[0]
        p += 6 + size
    return out


def main():
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    paks = ed.load_paks(iso)
    have = {}
    for pk in paks:
        for rid, (t, *_rest) in pk.res.items():
            have.setdefault(rid, t)
    rows = mega = engaged = bad = 0
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
                        if t4 != "SPTR":
                            continue
                        out = props(body)
                        rows += 1
                        if out is None:
                            bad += 1
                            print(f"{pk.path}:{nm}/{iname} L{li} eid=0x{eid:08x} UNPARSED")
                            continue
                        m = out.get("mega", False)
                        v = out.get("visor", 0xFFFFFFFF)
                        e = m and v != 0xFFFFFFFF
                        mega += m
                        engaged += e
                        print(f"{pk.path}:{nm}/{iname} L{li} eid=0x{eid:08x} mega={int(m)} "
                              f"spit=0x{out.get('spit', 0xFFFFFFFF):08x} visor=0x{v:08x}"
                              f"{'(' + have.get(v, 'absent') + ')' if v != 0xFFFFFFFF else ''} "
                              f"engaged={int(e)}")
    print(f"== {rows} SPTR placements, {mega} mega, {engaged} with an engaged visor effect, "
          f"{bad} unparsed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
