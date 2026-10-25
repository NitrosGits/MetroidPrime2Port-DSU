#!/usr/bin/env python3
"""Print every PARA / BRZG / CRLT placement of the disc and the property ids it carries, for the
arms of the three Parasite loaders (fn_47_1568 LoadParasite, fn_47_CF8 LoadBrizgee, fn_47_148
LoadCrystallite) that no natural load can reach.

usage: tools/scan_parasite_loader.py [--quiet] [--type PARA|BRZG|CRLT]

Read-only (tools/echoes_disc.py, the user's own disc image). The three loaders registered in
src/MetroidPrime/ScriptLoader.cpp:165/82/120 are one CParasite class:

  PARA LoadParasite    fn_47_1568   passes CParasite::kPT_Parasite    (0), shipped data has none
  BRZG LoadBrizgee     fn_47_CF8    passes CParasite::kPT_IceZoomer   (3)
  CRLT LoadCrystallite fn_47_148    passes CParasite::kPT_Crystallite (10)

Columns per placement:
  ids      the top-level property ids of the object body, checked against the ids the loader's
           dispatch compares against (the `case` list of include/MetroidPrime/ScriptLoader/
           SLdr<Type>.inc, read from the tree). An id in no placement of the type is a dead
           `case` arm; an id outside the set takes the `default: ReadBytes(nullptr, size)` arm.
  ancs     SLdrPatternedAITypedef.animationInformation (property 0xe25fb08c, SLdrAnimationSet:
           ancs, character_index, initial_anim). The loader passes kInvalidAssetId as the model
           and this ancs to LdrToModelData, which returns an empty optional only when the ancs
           id is in no pak (type NOWHERE). That is the `if (!modelData) return nullptr` arm.
  sounds   BRZG only: shellBreakSound (0x6a942a60), poisonHitSound (0x808392ec),
           playerPoisonSound (0xdf2d8017) - the SLdrBrizgee ctor stores -1 and the loader passes
           the values through, so a placement that omits one keeps -1.

An arm is data-dead only when no placement at any layer of any area takes it.
"""
import pathlib
import re
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

ANIM = 0xE25FB08C
NAME = 0x494E414D
BRZG_SOUNDS = {0x6A942A60: "shellBreak", 0x808392EC: "poisonHit", 0xDF2D8017: "playerPoison"}
# The include the loader's body pulls in, per object type.
SLDR_INC = {
    "PARA": "include/MetroidPrime/ScriptLoader/SLdrParasite.inc",
    "BRZG": "include/MetroidPrime/ScriptLoader/SLdrBrizgee.inc",
    "CRLT": "include/MetroidPrime/ScriptLoader/SLdrCrystallite.inc",
}


def known_ids(inc):
    """The `case 0x...:` ids of a generated loader include, in file order."""
    text = (ROOT / inc).read_text()
    return [int(m, 16) for m in re.findall(r"^  case 0x([0-9a-fA-F]+):", text, re.M)]


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
    quiet = "--quiet" in sys.argv
    only = None
    if "--type" in sys.argv:
        only = sys.argv[sys.argv.index("--type") + 1].upper()
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    paks = ed.load_paks(iso)
    known = {t: set(known_ids(i)) for t, i in SLDR_INC.items()}
    seen = {t: {} for t in SLDR_INC}
    arms = {t: {"placements": 0, "ancs present": 0, "ancs NOWHERE": 0, "unknown id": 0,
                "no editor id": 0, "no patterned id": 0} for t in SLDR_INC}
    for pk in paks:
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
                for li, sec in enumerate(layers):
                    for t4, eid, body in objects(sec):
                        if t4 not in known or (only and t4 != only):
                            continue
                        d = {}
                        top = None
                        if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                            top = items(body[6:])
                            walk(body[6:], d)
                        ids = [p for p, _ in (top or [])]
                        vals = {p: payload for p, payload in (top or [])}
                        for p in ids:
                            seen[t4][p] = seen[t4].get(p, 0) + 1
                        ancs = d.get("ancs")
                        where = [q.res[ancs][0] for q in paks if ancs is not None and ancs in q.res]
                        arm = arms[t4]
                        arm["placements"] += 1
                        arm["ancs present" if where else "ancs NOWHERE"] += 1
                        unk = [p for p in ids if p not in known[t4]]
                        arm["unknown id"] += bool(unk)
                        arm["no editor id"] += 0x255A4580 not in ids
                        arm["no patterned id"] += 0xB3774750 not in ids

                        def snd(pid):
                            if pid not in vals:
                                return "absent(-1)"
                            (v,) = struct.unpack_from(">i", vals[pid], 0)
                            return v

                        extra = ""
                        if t4 == "BRZG":
                            extra = " " + " ".join(f"{k}={snd(p)}"
                                                   for p, k in BRZG_SOUNDS.items())
                        if quiet:
                            continue
                        print(f"{t4} L{li} eid=0x{eid:08x} {d.get('name', '?')[:20]:20} "
                              f"ids={len(ids)} ancs={'-' if ancs is None else '0x%08x' % ancs} "
                              f"type={where[0] if where else 'NOWHERE'}{extra}"
                              + (f" UNKNOWN={[hex(p) for p in unk]}" if unk else ""))
    for t4 in sorted(known):
        missing = sorted(known[t4] - set(seen[t4]))
        print(f"== {t4}: {arms[t4]['placements']} placements, "
              + ", ".join(f"{k}={v}" for k, v in arms[t4].items() if k != "placements"))
        print(f"== {t4}: {len(known[t4])} dispatch ids, no placement carries ({len(missing)}): "
              f"{[hex(p) for p in missing]}")
        print(f"== {t4}: per-id placement counts: "
              + ", ".join(f"{hex(k)}:{v}" for k, v in sorted(seen[t4].items())))
    return 0


if __name__ == "__main__":
    sys.exit(main())
