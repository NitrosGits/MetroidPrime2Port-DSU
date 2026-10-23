#!/usr/bin/env python3
"""Print every Dark Samus (DRKS) placement's top-level property ids against fn_10_CF44's switch.

usage: tools/scan_darksamus_props.py

Read-only, like tools/scan_splitter_props.py: tools/echoes_disc.py's readers and the user's own
disc image. Per placement: the property count, the ids the loader's switch has no case for (its
default arm, ReadBytes at 0x0000D9F0), the switch ids the placement omits (they keep fn_10_E23C's
default), and the sound ids (the -1 defaults of the header's TARGET_PC block).
"""
import pathlib
import re
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

INC = ROOT / "include/MetroidPrime/ScriptLoader/SLdrDarkSamus.inc"
SOUNDS = {0x1F468967: "glideSound", 0x1A08AADC: "missileRicochetSound", 0xEA17CB66: "sweepBeamSound",
          0x2C72576B: "boostBallRollSound", 0x9E02691C: "boostBallHitPlayerSound",
          0xD3593630: "iceSpreadSound", 0xA861649F: "sound"}


def main():
    ids = {int(m, 16) for m in re.findall(r"case (0x[0-9a-f]+)", INC.read_text())}
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
                for li, sec in enumerate(layers):
                    for t4, eid, body in ed.scly_objects(sec):
                        if t4 != "DRKS":
                            continue
                        buf = body[6:] if struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF else body
                        count = struct.unpack_from(">H", buf, 0)[0]
                        p, seen, snd = 2, set(), {}
                        for _ in range(count):
                            pid, size = struct.unpack_from(">IH", buf, p)
                            seen.add(pid)
                            if pid in SOUNDS:
                                snd[SOUNDS[pid]] = struct.unpack_from(">i", buf, p + 6)[0]
                            p += 6 + size
                        rows += 1
                        print(f"{nm}/{iname} L{li} eid=0x{eid:08x} props={count} "
                              f"unknown={sorted(hex(i) for i in seen - ids)} "
                              f"omitted={sorted(hex(i) for i in ids - seen)} "
                              + " ".join(f"{k}={v}" for k, v in sorted(snd.items())))
    print(f"== {rows} DRKS placements, {len(ids)} switch ids")
    return 0


if __name__ == "__main__":
    sys.exit(main())
