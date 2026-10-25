#!/usr/bin/env python3
"""Loader-arm data scan for ScriptPlayerProxy (PLCT): which property ids, model asset ids and
proxyTypes the shipped placements carry (LoadPlayerController, fn_62_188). Read-only, like
tools/scan_playerproxy_props.py. usage: tools/scan_playerproxy_loader.py"""
import collections
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402
from scan_wallwalker_props import items, objects  # noqa: E402

KNOWN = {0x255A4580, 0xE71DE331, 0xC27FFA8F, 0xE25FB08C, 0x7E397FED, 0xCA56A18A, 0x1D8B933F,
         0xCB753319, 0xD9C09CF7, 0xF09C2B4B, 0x760859E5, 0xBD548A40, 0xC012F196, 0x70BC90A6}


def main():
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    ids = collections.Counter()
    models = collections.Counter()
    types = collections.Counter()
    n = bad = 0
    for pk in ed.load_paks(iso):
        for nm, (t, mid) in pk.names.items():
            if t != "MLVL":
                continue
            try:
                w = ed.parse_mlvl(pk.get(mid))
            except Exception:
                continue
            for a in w["areas"]:
                try:
                    layers = ed.mrea_layers(pk.get(a["mrea"]))
                except Exception:
                    continue
                for sec in layers:
                    for t4, eid, conns, body in objects(sec):
                        if t4 != "PLCT":
                            continue
                        n += 1
                        if len(body) < 8 or struct.unpack_from(">I", body, 0)[0] != 0xFFFFFFFF:
                            bad += 1
                            continue
                        its = items(body[6:])
                        if its is None:
                            bad += 1
                            continue
                        top = {pid: pl for pid, pl in its}
                        for pid in top:
                            ids[pid] += 1
                        if 0xC27FFA8F in top:
                            models[struct.unpack(">I", top[0xC27FFA8F])[0]] += 1
                        if 0xCA56A18A in top:
                            types[struct.unpack(">i", top[0xCA56A18A])[0]] += 1
                        else:
                            types[0] += 1
    print(f"PLCT placements {n} (unparsed {bad})")
    for pid, c in sorted(ids.items()):
        print(f"  prop 0x{pid:08x} x{c} {'' if pid in KNOWN else 'UNKNOWN (loader default arm)'}")
    print("  model ids:", {f"0x{k:08x}": v for k, v in models.items()} or "no placement carries 0xc27ffa8f")
    print("  proxyTypes:", dict(sorted(types.items())))
    return 0


if __name__ == "__main__":
    sys.exit(main())
