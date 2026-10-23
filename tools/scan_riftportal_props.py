#!/usr/bin/env python3
"""Print every RiftPortal (RPTL) placement: area, layer, connections, and the SLdrRiftPortal
properties its arms depend on (the four CMDL/ANCS pairs LoadRiftPortal builds CModelData from,
ripPortal, projectileAttraction and the projectile box/angle/radius).

usage: tools/scan_riftportal_props.py

Read-only, like tools/scan_glowbug_props.py (same readers): tools/echoes_disc.py and the user's
own disc image. A property absent from a placement keeps the loader's default
(SLdrRiftPortal.hpp). LdrToModelData builds an animated model when the ANCS is valid, so an
ancs other than 0xffffffff means HasAnimation() is true for that model.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402
from scan_wallwalker_props import objects, walk  # noqa: E402

RPTL_PROPS = {
    0xC27FFA8F: ("model", ">I"),
    0x90C42387: ("backgroundModel", ">I"),
    0xA71696B0: ("incandescentModel", ">I"),
    0xF284D838: ("lineModel", ">I"),
    0x87D5A35F: ("projectileAttraction", ">i"),
    0x709C1413: ("projectileBoxWidth", ">f"),
    0x3B692A03: ("projectileAngle", ">f"),
    0xE1E5551F: ("projectileDestructionRadius", ">f"),
}
# AnimationSet typedefs: ANCS id first.
ANIMS = {
    0xE25FB08C: "modelAncs",
    0x80C6A38D: "backgroundAncs",
    0x5CB18EB4: "incandescentAncs",
    0xE845FA67: "lineAncs",
}
RIP = 0xF5B73AF8


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
                objs = {}
                for li, sec in enumerate(layers):
                    for t4, eid, conns, body in objects(sec):
                        objs[eid & 0x3FFFFFF] = (li, t4, eid, conns, body)
                for li, t4, eid, conns, body in objs.values():
                    if t4 != "RPTL":
                        continue
                    out = {}
                    if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                        walk(body[6:], RPTL_PROPS, out)
                        for pid, name in ANIMS.items():
                            i = body.find(struct.pack(">I", pid))
                            if i >= 0:
                                out[name] = "0x%x" % struct.unpack_from(">I", body, i + 6)[0]
                        i = body.find(struct.pack(">IH", RIP, 1))
                        if i >= 0:
                            out["ripPortal"] = body[i + 6]
                    for k in ("model", "backgroundModel", "incandescentModel", "lineModel"):
                        if isinstance(out.get(k), int):
                            out[k] = "0x%x" % (out[k] & 0xFFFFFFFF)
                    rows += 1
                    print(f"{nm}/{iname} RPTL L{li} eid=0x{eid:08x} " +
                          " ".join(f"{k}={v}" for k, v in out.items()))
                    for s, m, tg in conns:
                        tgt = objs.get(tg & 0x3FFFFFF)
                        print(f"    {s} {m} -> 0x{tg:08x} {tgt[1] if tgt else '?'}")
    print(f"== {rows} RPTL placements")
    return 0


if __name__ == "__main__":
    sys.exit(main())
