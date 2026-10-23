#!/usr/bin/env python3
"""Print every SpankWeed (SPNK) placement: area, layer, connections and its SLdrSpankWeed
properties (wakeUpRadius, searchRadius, attackRadius, hurtSleepDelay), then a census of every
object type seen, so "0 SPNK placements" is checkable against the totals.

usage: tools/scan_spnk_props.py

Read-only, like tools/scan_ibsm_props.py: tools/echoes_disc.py and the user's own disc image.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402
from scan_wallwalker_props import objects, walk  # noqa: E402

SPNK_PROPS = {
    0x5CDC877D: ("unknown_0x5cdc877d", ">?"),
    0x835ADCA1: ("wakeUpRadius", ">f"),
    0xED9BF5A3: ("searchRadius", ">f"),
    0x7C11B995: ("attackRadius", ">f"),
    0x9B5A4744: ("hurtSleepDelay", ">f"),
}
ANIM = 0xE25FB08C  # animationInformation: ANCS, character, animation


def main():
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    rows = 0
    census = {}
    nareas = 0
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
                nareas += 1
                objs = {}
                for li, sec in enumerate(layers):
                    for t4, eid, conns, body in objects(sec):
                        objs[eid & 0x3FFFFFF] = (li, t4, eid, conns, body)
                for li, t4, eid, conns, body in objs.values():
                    census[t4] = census.get(t4, 0) + 1
                    if t4 != "SPNK":
                        continue
                    out = {}
                    if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                        walk(body[6:], SPNK_PROPS, out)
                        i = body.find(struct.pack(">I", ANIM))
                        if i >= 0:
                            out["ancs"] = "0x%x" % struct.unpack_from(">I", body, i + 6)[0]
                    rows += 1
                    print(f"{nm}/{iname} SPNK L{li} eid=0x{eid:08x} " +
                          " ".join(f"{k}={v}" for k, v in out.items()))
                    for s, m, tg in conns:
                        tgt = objs.get(tg & 0x3FFFFFF)
                        print(f"    {s} {m} -> 0x{tg:08x} {tgt[1] if tgt else '?'}")
    print(f"== {rows} SPNK placements in {nareas} areas, {sum(census.values())} objects, {len(census)} types")
    print(" ".join(f"{k}={v}" for k, v in sorted(census.items())))
    return 0


if __name__ == "__main__":
    sys.exit(main())
