#!/usr/bin/env python3
"""Print every PlantScarabSwarm (PSSM, Seedburster) placement: area, layer, connections, and the
SLdrPlantScarabSwarm properties the CPlantScarabSwarm ctor and boid update branch on (the ANCS,
intoAttackAnimation / attackAnimation against -1, basicSwarmProperties.attackerCount and
isVulnerableToSafeZone).

usage: tools/scan_pssm_props.py

Read-only, like tools/scan_glowbug_props.py (whose readers it reuses): tools/echoes_disc.py and
the user's own disc image. A property absent from a placement keeps the loader's default
(SLdrPlantScarabSwarm.hpp: animations -1; SLdrBasicSwarmProperties.hpp for the swarm ones).
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402
from scan_wallwalker_props import objects, walk  # noqa: E402

PSSM_PROPS = {
    0x7399ABBB: ("intoAttackAnimation", ">i"),
    0x734D923B: ("attackAnimation", ">i"),
    0x523A405C: ("attackerCount", ">i"),
    0x1CA0E760: ("attackProximity", ">f"),
    0x88DF3EA8: ("attackTimer", ">f"),
    0xF11F7384: ("maxAttackAngle", ">f"),
}
SAFE_ZONE = 0xC2208B0F  # basicSwarmProperties.isVulnerableToSafeZone, a 1-byte bool
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
                    if t4 != "PSSM":
                        continue
                    out = {}
                    if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                        walk(body[6:], PSSM_PROPS, out)
                        # walk() only decodes 4-byte payloads.
                        i = body.find(struct.pack(">IH", SAFE_ZONE, 1))
                        if i >= 0:
                            out["isVulnerableToSafeZone"] = body[i + 6]
                        i = body.find(struct.pack(">I", ANIM))
                        if i >= 0:
                            out["ancs"] = "0x%x" % struct.unpack_from(">I", body, i + 6)[0]
                    rows += 1
                    print(f"{nm}/{iname} PSSM L{li} eid=0x{eid:08x} " +
                          " ".join(f"{k}={v}" for k, v in out.items()))
                    for s, m, tg in conns:
                        tgt = objs.get(tg & 0x3FFFFFF)
                        print(f"    {s} {m} -> 0x{tg:08x} {tgt[1] if tgt else '?'}")
    print(f"== {rows} PSSM placements")
    return 0


if __name__ == "__main__":
    sys.exit(main())
