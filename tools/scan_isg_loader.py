#!/usr/bin/env python3
"""Print the loader inputs that gate the two data-dependent arms of LoadIngSpiderballGuardian (fn_35_178).

usage: tools/scan_isg_loader.py

Read-only, like tools/scan_darktrooper_loader.py (tools/echoes_disc.py readers, the user's own disc
image). Every 'ISBG' placement in every layer of every area of every MLVL is listed with:

  ids      the top-level property ids of the placement. The loader's switch (retail 0x264-0x2f4)
           knows 0x255a4580 (editor), 0xb3774750 (patterned), 0x7e397fed (actor parameters) and
           0xa1389799 (the Data); any other id takes `default: ReadBytes` (retail 0x2f0). The
           default arm is data-dead when no placement carries an id outside that set.
  ancs     SLdrPatternedAITypedef.animationInformation (0xe25fb08c: ancs, character, initial_anim)
           and the FourCC the paks report for it. LdrToModelData returns an empty optional (the
           loader's `return nullptr`, retail 0x344-0x398) only when BOTH resource types are zero,
           i.e. the ancs id is in no pak; 'ANCS' is the normal model path.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

ANIM = 0xE25FB08C
NAME = 0x494E414D
KNOWN = {0x255A4580, 0xB3774750, 0x7E397FED, 0xA1389799}


def items(buf):
    if len(buf) < 2:
        return None
    count = struct.unpack_from(">H", buf, 0)[0]
    p = 2
    out = []
    for _ in range(count):
        if p + 6 > len(buf):
            return None
        pid, size = struct.unpack_from(">IH", buf, p)
        out.append((pid, buf[p + 6:p + 6 + size]))
        p += 6 + size
    return out if p == len(buf) else None


def props(buf, out, depth=0):
    its = items(buf)
    if its is None or depth > 6:
        return
    for pid, payload in its:
        if pid == NAME:
            out.setdefault("name", payload.rstrip(b"\0").decode("latin1"))
        elif pid == ANIM and len(payload) == 12:
            out["ancs"], out["char"], out["anim"] = struct.unpack_from(">3I", payload, 0)
        else:
            props(payload, out, depth + 1)


def objects(sec):
    count = struct.unpack_from(">I", sec, 10)[0]
    p = 14
    out = []
    for _ in range(count):
        t = sec[p:p + 4].decode("latin1")
        (ln,) = struct.unpack_from(">H", sec, p + 4)
        body = sec[p + 6:p + 6 + ln]
        eid, nc = struct.unpack_from(">IH", body, 0)
        out.append((t, eid, body[6 + 12 * nc:]))
        p += 6 + ln
    return out


def main():
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    paks = ed.load_paks(iso)
    rows = 0
    arms = {"model NOWHERE": 0, "model ANCS": 0, "model other": 0, "unknown top-level id": 0}
    for pk in paks:
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
                seen = False
                for li, sec in enumerate(layers):
                    for t4, eid, body in objects(sec):
                        if t4 != "ISBG":
                            continue
                        d = {}
                        ids = []
                        if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                            props(body[6:], d)
                            its = items(body[6:])
                            ids = [i for i, _ in its] if its else None
                        ancs = d.get("ancs")
                        where = []
                        if ancs is not None:
                            where = [f"{q.path.rsplit('/', 1)[-1]}:{q.res[ancs][0]}"
                                     for q in paks if ancs in q.res]
                        kind = where[0].split(":")[1] if where else "NOWHERE"
                        if not seen:
                            print(f"== {nm}/{iname} (mlvl {mid:08x})")
                            seen = True
                        rows += 1
                        arms["model ANCS" if kind == "ANCS" else ("model other" if where else "model NOWHERE")] += 1
                        unk = [i for i in (ids or []) if i not in KNOWN]
                        arms["unknown top-level id"] += 1 if (unk or ids is None) else 0
                        print(f"ISBG L{li} eid=0x{eid:08x} {d.get('name', '?')[:24]:24} "
                              f"ancs={'-' if ancs is None else '0x%08x' % ancs} type={kind} "
                              f"ids={'UNPARSED' if ids is None else ','.join('%08x' % i for i in ids)} "
                              f"unknown={['%08x' % i for i in unk]}")
    print(f"== {rows} ISBG placements: " + ", ".join(f"{k}={v}" for k, v in arms.items()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
