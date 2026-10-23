#!/usr/bin/env python3
"""Print every Glowbug (GBUG, Lightflyer) placement: area, layer, connections, and the
SLdrGlowbug properties it carries (the ANCS LoadGlowbug builds the model from, the asset ids its ctor tests against kInvalidAssetId, the
patterned attack range window and state machine, isInLightWorld).

usage: tools/scan_glowbug_props.py

Read-only, like tools/scan_wallwalker_props.py (whose readers it reuses): tools/echoes_disc.py and
the user's own disc image. A property absent from a placement keeps the loader's default
(SLdrGlowbug.hpp: assets -1, isInLightWorld false).
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402
from scan_wallwalker_props import objects, walk  # noqa: E402

GBUG_PROPS = {
    0xD75426F9: ("deathFlashEffect", ">I"),
    0x8985B339: ("deathBreakApartEffect", ">I"),
    0xB258D3E8: ("attackEffect", ">I"),
    0xB06815B3: ("attackTelegraphEffect", ">I"),
    0xAB378A64: ("attackEchoEffect", ">I"),
    0xA9482EB1: ("scanModel", ">I"),
    0x58434916: ("minAttackRange", ">f"),
    0xFF77C96F: ("maxAttackRange", ">f"),
    0x55744160: ("stateMachine", ">I"),
}
LIGHT = 0x1917A180
ANIM = 0xE25FB08C  # patterned.animationInformation: ANCS, character, animation


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
                    if t4 != "GBUG":
                        continue
                    out = {}
                    if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                        walk(body[6:], GBUG_PROPS, out)
                        # isInLightWorld is a 1-byte bool: walk() only decodes 4-byte payloads.
                        i = body.find(struct.pack(">IH", LIGHT, 1))
                        if i >= 0:
                            out["isInLightWorld"] = body[i + 6]
                        # LoadGlowbug returns null without a model: the ANCS LdrToModelData reads.
                        i = body.find(struct.pack(">I", ANIM))
                        if i >= 0:
                            out["ancs"] = "0x%x" % struct.unpack_from(">I", body, i + 6)[0]
                    rows += 1
                    print(f"{nm}/{iname} GBUG L{li} eid=0x{eid:08x} " +
                          " ".join(f"{k}={v}" for k, v in out.items()))
                    for s, m, tg in conns:
                        tgt = objs.get(tg & 0x3FFFFFF)
                        print(f"    {s} {m} -> 0x{tg:08x} {tgt[1] if tgt else '?'}")
    print(f"== {rows} GBUG placements")
    return 0


if __name__ == "__main__":
    sys.exit(main())
