#!/usr/bin/env python3
"""Print every Kralee (KRAL) placement of the disc with the properties the loader/ctor arms need.

usage: tools/scan_kralee_props.py

Read-only (tools/echoes_disc.py, the user's own disc image). For each KRAL placement of every
area and every layer:

  ancs / type   SLdrPatternedAITypedef.animationInformation (property 0xe25fb08c, SLdrAnimationSet:
                ancs, character_index, initial_anim) and the FourCC the pak reports for the ancs
                id. LoadKralee (CKralee.cpp, `if (!modelData) return nullptr`) passes
                kInvalidAssetId as the model and this ancs to LdrToModelData (ScriptLoader.cpp
                :446-459), which returns an empty optional only when BOTH resource types are
                zero, i.e. when the ancs id is in no pak (type NOWHERE).
  warpIn/warpOut  the two ids CKralee's ctor tests against kInvalidAssetId
                (0x351dbc73 / 0x2d72ba7b, SLdrKralee.inc:100-107), with their pak type.
  vis/invis     visibleTime / invisibleTime (0x5704897c / 0xbbd4b10c), the Think timers.
  props         the top-level property ids the placement carries, so an unknown id is visible.

An arm is data-dead only when no placement at any layer of any area takes it.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

ANIM = 0xE25FB08C
WARP_IN = 0x351DBC73
WARP_OUT = 0x2D72BA7B
VISIBLE = 0x5704897C
INVISIBLE = 0xBBD4B10C
NAME = 0x494E414D
# every property id SLdrKralee.inc switches on
KNOWN = {
    0x255A4580, 0xBE73724A, 0xB3774750, 0x7E397FED, 0x733BD27C, 0xA72530E8, 0xAC47C628,
    0x8E4F7B29, 0x3D3C1B76, 0xD5C25506, 0x742EAB20, 0x80A81909, 0xED6A9353, 0x033153A0,
    0x5704897C, 0x7BBA36FF, 0xBBD4B10C, 0x4E4AE0E4, 0xAD69AC32, 0xC7D2EDE8, 0xB1A26335,
    0x8590483B, 0x214E48A0, 0x351DBC73, 0x2D72BA7B, 0x80B58324, 0xA4EF7B42, 0xC3CC437F,
    0x738D1C80,
}


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


def walk(buf, out, depth=0):
    its = items(buf)
    if its is None or depth > 6:
        return
    for pid, payload in its:
        if pid == NAME:
            out.setdefault("name", payload.rstrip(b"\0").decode("latin1"))
        elif pid == ANIM and len(payload) == 12:
            out["ancs"], out["char"], out["anim"] = struct.unpack_from(">3I", payload, 0)
        elif pid == WARP_IN and len(payload) == 4:
            out["warpIn"] = struct.unpack_from(">I", payload)[0]
        elif pid == WARP_OUT and len(payload) == 4:
            out["warpOut"] = struct.unpack_from(">I", payload)[0]
        elif pid == VISIBLE and len(payload) == 4:
            out["visibleTime"] = struct.unpack_from(">f", payload)[0]
        elif pid == INVISIBLE and len(payload) == 4:
            out["invisibleTime"] = struct.unpack_from(">f", payload)[0]
        else:
            walk(payload, out, depth + 1)


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

    def kind(aid):
        for q in paks:
            if aid in q.res:
                return q.res[aid][0]
        return "NOWHERE"

    rows = 0
    arms = {"ancs NOWHERE": 0, "ancs present": 0, "warpIn valid": 0, "warpOut valid": 0,
            "warpIn unset": 0, "warpOut unset": 0, "unknown prop id": 0}
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
                krals = [(li, o) for li, sec in enumerate(layers) for o in objects(sec)
                         if o[0] == "KRAL"]
                if not krals:
                    continue
                print(f"== {nm}/{iname} (mlvl {mid:08x})")
                for li, (t4, eid, body) in krals:
                    d = {}
                    if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                        walk(body[6:], d)
                    top = items(body[6:]) if len(body) >= 8 else None
                    utop = [p for p, _ in (top or []) if p not in KNOWN]
                    arms["unknown prop id"] += bool(utop or top is None)
                    ancs = d.get("ancs")
                    akind = kind(ancs) if ancs is not None else "-"
                    arms["ancs present" if akind not in ("NOWHERE", "-") else "ancs NOWHERE"] += 1
                    wi, wo = d.get("warpIn"), d.get("warpOut")
                    arms["warpIn valid"] += bool(wi not in (None, 0xFFFFFFFF))
                    arms["warpOut valid"] += bool(wo not in (None, 0xFFFFFFFF))
                    arms["warpIn unset"] += bool(wi in (None, 0xFFFFFFFF))
                    arms["warpOut unset"] += bool(wo in (None, 0xFFFFFFFF))
                    rows += 1
                    print(f"KRAL L{li} eid=0x{eid:08x} {d.get('name', '?')[:24]:24} "
                          f"ancs={'-' if ancs is None else '0x%08x' % ancs} ancsType={akind} "
                          f"warpIn={'-' if wi is None else '0x%08x' % wi}"
                          f"({'-' if wi is None else kind(wi)}) "
                          f"warpOut={'-' if wo is None else '0x%08x' % wo}"
                          f"({'-' if wo is None else kind(wo)}) "
                          f"vis={d.get('visibleTime', 'default')} inv={d.get('invisibleTime', 'default')}"
                          + (f" UNKNOWN_TOP={[hex(p) for p in utop]}" if utop or top is None else ""))
    print(f"== {rows} KRAL placements: " + ", ".join(f"{k}={v}" for k, v in arms.items()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
