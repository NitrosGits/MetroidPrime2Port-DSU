#!/usr/bin/env python3
"""List every script connection (state MDFY, message FOLW) that targets an AI waypoint (AIWP).

usage: tools/scan_ragdoll_pins.py

Read-only, over the user's own disc (tools/echoes_disc.py's readers). CPirateRagDoll's ctor
(PirateRagDoll fn_50_1938) pins ragdoll particles to the AI waypoints its owner is connected to
with FindConnectedObjects_if(kSS_Modify, kSM_Follow, CAIWaypointPredicate); this scan shows which
placements (any type) carry such a connection, per area and layer. A connection's target may live
in another layer of the same area, so the targets are looked up over all the area's layers.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

MDFY = 0x4D444659
FOLW = 0x464F4C57


def objects_with_connections(sec):
    """[(fourcc, editor id, [(state, msg, target)])] of one SCLY layer section."""
    count = struct.unpack_from(">I", sec, 10)[0]
    p = 14
    out = []
    for _ in range(count):
        t = sec[p:p + 4].decode("latin1")
        (ln,) = struct.unpack_from(">H", sec, p + 4)
        body = sec[p + 6:p + 6 + ln]
        eid, nc = struct.unpack_from(">IH", body, 0)
        conns = [struct.unpack_from(">III", body, 6 + 12 * i) for i in range(nc)]
        out.append((t, eid, conns))
        p += 6 + ln
    return out


def main():
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    total = 0
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
                    layers = [objects_with_connections(s) for s in ed.mrea_layers(pk.get(a["mrea"]))]
                except Exception:
                    continue
                types = {eid & 0xFFFFFF: t4 for objs in layers for t4, eid, _ in objs}
                for li, objs in enumerate(layers):
                    for t4, eid, conns in objs:
                        for state, msg, target in conns:
                            if state == MDFY and msg == FOLW and types.get(target & 0xFFFFFF) == "AIWP":
                                total += 1
                                print(f"{nm}/{iname} L{li}: {t4} 0x{eid:08x} -> AIWP 0x{target:08x}")
    print(f"total {total}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
