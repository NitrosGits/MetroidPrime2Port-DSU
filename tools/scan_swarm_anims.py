#!/usr/bin/env python3
"""List every FSWM/IBSM/MSWM/PSSM swarm placement with its animationInformation ANCS id.

usage: tools/scan_swarm_anims.py

Read-only, over the user's own disc (tools/echoes_disc.py, like tools/scan_ibsm_props.py). These
four classes reach CSwarmBasics::PreRenderBoid (fn_80_4194) with a model list of size 4 when the
ANCS is valid (CSwarmBasics ctor reserves and fills four CModelDatas) and size 0 when it is
0xffffffff; the TARGET_PC `idx < 32` guard (line 1072) only matters for the empty list.
Property 0xe25fb08c (SLdrAnimationSet: ancs, character, animation) is the same in all four loaders.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402
from scan_wallwalker_props import objects  # noqa: E402

ANIM = 0xE25FB08C
TYPES = ("FSWM", "IBSM", "MSWM", "PSSM")


def main():
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    rows = bad = 0
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
                    for t4, eid, conns, body in objects(sec):
                        if t4 not in TYPES:
                            continue
                        i = body.find(struct.pack(">I", ANIM))
                        ancs = struct.unpack_from(">I", body, i + 6)[0] if i >= 0 else None
                        rows += 1
                        if ancs in (None, 0xFFFFFFFF):
                            bad += 1
                        print(f"{nm}/{iname} {t4} L{li} eid=0x{eid:08x} ancs="
                              + ("absent" if ancs is None else "0x%08x" % ancs))
    print(f"== {rows} placements, {bad} without a valid ANCS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
