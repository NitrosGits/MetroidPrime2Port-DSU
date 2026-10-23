#!/usr/bin/env python3
"""Print every ScriptPlayerProxy (PLCT) placement: layer, editor id, name, the
SLdrPlayerController properties it carries and its connections (with the target's type).

usage: tools/scan_playerproxy_props.py [--summary]

Read-only, like tools/scan_wallwalker_props.py: tools/echoes_disc.py's readers and the user's own
disc image. A property absent from a placement keeps the loader's default (SLdrPlayerController.hpp:
proxyType 0, ints 0, floats 0). --summary prints only the per-proxyType counts and the
(state, message) pairs sent to each proxy type.
"""
import collections
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402
from scan_wallwalker_props import walk, objects  # noqa: E402

PLCT_PROPS = {
    0xCA56A18A: ("proxyType", ">i"),
    0xCB753319: ("int1", ">i"),
    0xD9C09CF7: ("int2", ">i"),
    0xF09C2B4B: ("float1", ">f"),
    0x760859E5: ("float2", ">f"),
    0xBD548A40: ("float3", ">f"),
}
TYPES = ["EventRelay", "CoinCollector", "ArchenemyMarker", "PlayerFollower", "PersistentCounterReset",
         "CombatVisorWatcher", "ScanVisorWatcher", "DarkVisorWatcher", "EchoVisorWatcher",
         "PlayerMessageRelay", "DamageOverTime"]


def main():
    summary = "--summary" in sys.argv
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    count = collections.Counter()
    msgs_in = collections.defaultdict(collections.Counter)
    msgs_out = collections.defaultdict(collections.Counter)
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
                proxies = {k: o for k, o in objs.items() if o[1] == "PLCT"}
                if not proxies:
                    continue
                props = {}
                for k, (li, t4, eid, conns, body) in proxies.items():
                    out = {}
                    if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                        walk(body[6:], PLCT_PROPS, out)
                    props[k] = out
                    ty = out.get("proxyType", 0)
                    count[ty] += 1
                    for s, m, tg in conns:
                        tgt = objs.get(tg & 0x3FFFFFF)
                        msgs_out[ty][(s, m, tgt[1] if tgt else "?")] += 1
                for k, (li, t4, eid, conns, body) in objs.items():
                    for s, m, tg in conns:
                        if (tg & 0x3FFFFFF) in proxies:
                            ty = props[tg & 0x3FFFFFF].get("proxyType", 0)
                            msgs_in[ty][(t4, s, m)] += 1
                if summary:
                    continue
                print(f"== {nm}/{iname} (mlvl {mid:08x})")
                for k, (li, t4, eid, conns, body) in proxies.items():
                    out = dict(props[k])
                    ty = out.pop("proxyType", 0)
                    tn = TYPES[ty] if 0 <= ty < len(TYPES) else str(ty)
                    print(f"PLCT L{li} eid=0x{eid:08x} {tn} " + " ".join(f"{a}={v}" for a, v in out.items()))
                    for s, m, tg in conns:
                        tgt = objs.get(tg & 0x3FFFFFF)
                        print(f"    out {s} {m} -> 0x{tg:08x} {tgt[1] if tgt else '?'}")
                    for k2, (l2, t2, e2, c2, _) in objs.items():
                        for s, m, tg in c2:
                            if (tg & 0x3FFFFFF) == k:
                                print(f"    in  {t2} 0x{e2:08x} {s} {m}")
    for ty in sorted(count):
        tn = TYPES[ty] if 0 <= ty < len(TYPES) else str(ty)
        print(f"## {tn}: {count[ty]} placements")
        print("   in:  " + ", ".join(f"{a}/{s}/{m}x{n}" for (a, s, m), n in sorted(msgs_in[ty].items())))
        print("   out: " + ", ".join(f"{s}/{m}->{a}x{n}" for (s, m, a), n in sorted(msgs_out[ty].items())))
    return 0


if __name__ == "__main__":
    sys.exit(main())
