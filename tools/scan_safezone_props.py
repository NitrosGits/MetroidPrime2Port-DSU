#!/usr/bin/env python3
"""Print every SafeZone (SAFE) and SafeZone Crystal (SFZC) placement's loaded properties.

usage: tools/scan_safezone_props.py

Read-only, like tools/scan_splinter_props.py: tools/echoes_disc.py's readers and the user's own disc
image. Mirrors LoadSafeZone (fn_66_CC, SLdrSafeZone.inc) for the values that reach the ctor:
absent properties keep the SLdrSafeZone defaults (SLdrSafeZone.hpp). A SAFE placement whose three
attribute structs lack a valid shellEnvironmentMap/shell1Texture/shell2Texture makes LoadSafeZone
return nullptr, so the scan flags those as NOENT. The fade/flash columns are the loaded values:
insideFadeTime(ctor) = insideFadeStart + insideFadeTime; flashTime(ctor) is the file's
flashBrightness and flashBrightness(ctor) is the file's flashTime (TARGET_PC swap).
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

INVALID = 0xFFFFFFFF
# (name, fmt, default) -- SLdrSafeZone defaults from SLdrSafeZone.hpp
TOP = {
    0x08CCFFD0: ("fadeStart", ">f", 0.0),
    0x7FEBBFE7: ("fadeTime", ">f", 0.0),
    0x6C14904C: ("fadeMinAlpha", ">f", 0.0),
    0x48B4B865: ("flashTime", ">f", 0.0),
    0x452F7876: ("flashBright", ">f", 0.0),
    0x4FAAC896: ("flashSound", ">i", -1),
    0xD5869B0B: ("shape", ">i", 0),
    0x222A258E: ("mobile", ">?", False),
    0x6C90E396: ("mobLight", ">?", False),
}
ATTR_IDS = {0xB4A293C7: "normal", 0xDAE8C14E: "hurtful", 0x6471D643: "echo"}
SHELL = {0x74F8A729: "env", 0x1E712EE2: "shell1", 0xA3BB422C: "shell2"}


def plist(buf, p):
    """(pid -> payload bytes) for the property list at offset p (u16 count first)."""
    if p + 2 > len(buf):
        return None
    (count,) = struct.unpack_from(">H", buf, p)
    p += 2
    out = {}
    for _ in range(count):
        if p + 6 > len(buf):
            return None
        pid, size = struct.unpack_from(">IH", buf, p)
        out[pid] = buf[p + 6:p + 6 + size]
        p += 6 + size
    return out


def main():
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    rows = bad = noent = crys = 0
    shapes = {}
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
                        if t4 not in ("SAFE", "SFZC"):
                            continue
                        tag = f"{pk.path}:{nm}/{iname} L{li} eid=0x{eid:08x} {t4}"
                        top = plist(body, 6)
                        if top is None:
                            bad += 1
                            print(f"{tag} UNPARSED")
                            continue
                        if t4 == "SFZC":
                            crys += 1
                            ty = struct.unpack(">i", top[0x1115FB68][:4])[0] if 0x1115FB68 in top else 0
                            print(f"{tag} type={ty} props={len(top)}")
                            continue
                        rows += 1
                        v = {}
                        for pid, (n, fmt, d) in TOP.items():
                            v[n] = struct.unpack(fmt, top[pid][:struct.calcsize(fmt)])[0] if pid in top else d
                        miss = []
                        for aid, an in ATTR_IDS.items():
                            sub = plist(top[aid], 0) if aid in top else {}
                            for sid, sn in SHELL.items():
                                val = struct.unpack(">I", sub[sid][:4])[0] if sub and sid in sub else INVALID
                                if val == INVALID:
                                    miss.append(f"{an}.{sn}")
                        if miss:
                            noent += 1
                        shapes[v["shape"]] = shapes.get(v["shape"], 0) + 1
                        print(f"{tag} fade={v['fadeStart']:g}..{v['fadeStart'] + v['fadeTime']:g} "
                              f"minA={v['fadeMinAlpha']:g} ctorFlashTime={v['flashBright']:g} "
                              f"ctorFlashBright={v['flashTime']:g} flashSnd={v['flashSound']} "
                              f"shape={v['shape']} mobile={int(v['mobile'])}/{int(v['mobLight'])}"
                              f"{' NOENT(' + ','.join(miss) + ')' if miss else ''}")
    print(f"== {rows} SAFE placements ({noent} NOENT, shapes {dict(sorted(shapes.items()))}), "
          f"{crys} SFZC, {bad} unparsed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
