#!/usr/bin/env python3
"""Print every Splitter placement's (SPLL and SPLU) top-level property ids and its ANCS id.

usage: tools/scan_splitter_loader.py

Read-only, like tools/scan_splitter_props.py. The loaders fn_75_82B0 (SPLL) and fn_75_FC (SPLU)
switch on the top-level property ids (0x255a4580, 0xb3774750, 0x7e397fed and 0x15e03a2f/0x560cf813)
and skip any other (ReadBytes); they return null when LdrToModelData gives no model, which needs
an invalid ANCS id in the patterned block's animation set (0xe25fb08c).
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

KNOWN = {"SPLL": {0x255A4580, 0xB3774750, 0x7E397FED, 0x15E03A2F},
         "SPLU": {0x255A4580, 0xB3774750, 0x7E397FED, 0x560CF813}}


def items(buf):
    """The (id, payload) list of a property list, or None when buf is not one."""
    if len(buf) < 2:
        return None
    count = struct.unpack_from(">H", buf, 0)[0]
    p = 2
    out = []
    for _ in range(count):
        if p + 6 > len(buf):
            return None
        pid, size = struct.unpack_from(">IH", buf, p)
        out.append((pid, buf[p + 6:p + 6 + size]))
        p += 6 + size
    return out if p == len(buf) else None


def main():
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    rows = unknown = noancs = 0
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
                        if t4 not in KNOWN:
                            continue
                        top = items(body[6:]) if len(body) >= 8 else None
                        ids = [pid for pid, _ in top or []]
                        extra = [pid for pid in ids if pid not in KNOWN[t4]]
                        ancs = None
                        for pid, payload in top or []:
                            if pid == 0xB3774750:
                                for qid, q in items(payload) or []:
                                    if qid == 0xE25FB08C and len(q) >= 4:
                                        ancs = struct.unpack_from(">I", q, 0)[0]
                        rows += 1
                        unknown += bool(extra)
                        noancs += ancs in (None, 0xFFFFFFFF)
                        print(f"{t4} {nm}/{iname} L{li} eid=0x{eid:04x} ids=" +
                              ",".join(f"{i:08x}" for i in ids) +
                              f" extra={len(extra)} ancs=" +
                              ("none" if ancs is None else f"{ancs:08x}"))
    print(f"== {rows} placements, {unknown} with an unknown top-level id, "
          f"{noancs} without an ANCS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
