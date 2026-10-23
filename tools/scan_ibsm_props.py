#!/usr/bin/env python3
"""Print every IngBlobSwarm (IBSM) placement: area, layer, connections, and the properties that
pick CIngBlobSwarm's arms (intoAttackAnimation/attackAnimation -1 skip the attack models and the
shared attack model; attackerCount sizes them; the attack proximity, timer, angle, mass and height).

usage: tools/scan_ibsm_props.py

Read-only, like tools/scan_glowbug_props.py: tools/echoes_disc.py and the user's own disc image.
A property absent from a placement keeps the loader's default (SLdrIngBlobSwarm.hpp:
animations -1, maxAttackAngle 30, mass 2, maxAttackHeight 0.5; SLdrBasicSwarmProperties.hpp).
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402
from scan_wallwalker_props import objects, walk  # noqa: E402

IBSM_PROPS = {
    0x7399ABBB: ("intoAttackAnimation", ">i"),
    0x734D923B: ("attackAnimation", ">i"),
    0xF11F7384: ("maxAttackAngle", ">f"),
    0xCA761DCD: ("intoAttackSpeed", ">f"),
    0x6C0A2BC8: ("attackSpeed", ">f"),
    0x75DBB375: ("mass", ">f"),
    0xE1AE51D8: ("maxAttackHeight", ">f"),
    0x523A405C: ("attackerCount", ">i"),
    0x1CA0E760: ("attackProximity", ">f"),
    0x88DF3EA8: ("attackTimer", ">f"),
}
ANIM = 0xE25FB08C  # animationInformation: ANCS, character, animation


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
                    if t4 != "IBSM":
                        continue
                    out = {}
                    if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                        walk(body[6:], IBSM_PROPS, out)
                        i = body.find(struct.pack(">I", ANIM))
                        if i >= 0:
                            out["ancs"] = "0x%x" % struct.unpack_from(">I", body, i + 6)[0]
                    rows += 1
                    print(f"{nm}/{iname} IBSM L{li} eid=0x{eid:08x} " +
                          " ".join(f"{k}={v}" for k, v in out.items()))
                    for s, m, tg in conns:
                        tgt = objs.get(tg & 0x3FFFFFF)
                        print(f"    {s} {m} -> 0x{tg:08x} {tgt[1] if tgt else '?'}")
    print(f"== {rows} IBSM placements")
    return 0


if __name__ == "__main__":
    sys.exit(main())
