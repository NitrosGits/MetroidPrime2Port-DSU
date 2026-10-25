#!/usr/bin/env python3
"""Print every GRCH (Grenchler / Grapple Guardian / Dark Grenchler) placement of the disc and the
property ids it carries, for the arms of LoadGrenchler (fn_27_168) that no natural load can reach.

usage: tools/scan_grenchler_loader.py [--quiet]

Read-only (tools/echoes_disc.py, the user's own disc image). Columns per GRCH placement:

  ids      the top-level property ids of the object body, checked against the 62 ids retail's
           switch dispatches on (LoadGrenchler, 0x200..0xBA8). An id in no placement is a dead
           case arm; an id outside the set takes the `default: ReadBytes(nullptr, propertySize)`
           arm (0xBA8).
  ancs     SLdrPatternedAITypedef.animationInformation (property 0xe25fb08c, SLdrAnimationSet:
           ancs, character_index, initial_anim). LoadGrenchler passes kInvalidAssetId as the model
           and this ancs to LdrToModelData (ScriptLoader.cpp), which returns an empty optional only
           when BOTH resource types are zero, i.e. when the ancs id is in no pak (type NOWHERE).
           That is the `if (!modelData) return nullptr` arm (0xBFC lbz 0x2e0; cmplwi 0; 0xC24).
  hit/dest SLdrGrenchler.tailHitSound (0x55c51213) and tailDestroyedSound (0x385e4738). The loader
           turns -1 into CSfxManager::kInternalInvalidSfxId (0xC54-0xC7C); a placement that omits
           either property keeps the ctor's -1 (SLdrGrenchler.hpp), so both arms of both compares
           stay live unless every placement carries the property with a value other than -1.

An arm is data-dead only when no placement at any layer of any area takes it.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

ANIM = 0xE25FB08C
NAME = 0x494E414D
HIT = 0x55C51213
DEST = 0x385E4738
# The 63 ids retail's dispatch compares against (LoadGrenchler 0x190 lis 0xa73/addi 0x26a3 for the
# first, 0x200..0xB98 for the tree), equal to the `case` list of
# include/MetroidPrime/ScriptLoader/SLdrGrenchler.inc.
KNOWN = {
    0x04D51E3A, 0x05439A08, 0x05FC6001, 0x06F7CEED, 0x0A7326A3, 0x0ABEF809,
    0x0DABF0AF, 0x13E30E4D, 0x13E5B580, 0x255A4580, 0x262B2508, 0x2CE7520F,
    0x2E6096EE, 0x30B81A7E, 0x33408E7F, 0x385E4738, 0x401BC111, 0x46B8B69B,
    0x49632B31, 0x49FAE143, 0x4F3A4566, 0x5285DB00, 0x54B6BFA1, 0x55C51213,
    0x5CF705F2, 0x5D8F2BEE, 0x66645D7E, 0x67B6EA0B, 0x680CE795, 0x6E2FEE98,
    0x6EC26414, 0x70247A6E, 0x72258FE7, 0x7BD1A35F, 0x7E397FED, 0x7EF9AA67,
    0x909E6095, 0x98D5D373, 0x9B193AE8, 0xA18F626B, 0xAA04F0BE, 0xAD47FEBE,
    0xAE1F2A26, 0xB00775E6, 0xB0428B70, 0xB3774750, 0xB6B9074B, 0xB9F9F4F2,
    0xBF4DAAE6, 0xC24CF580, 0xD0D118E3, 0xD4753FF4, 0xD89AAB05, 0xDF636C4B,
    0xE51746D1, 0xE5FBA24C, 0xE61748ED, 0xEA4B88C8, 0xF00DF306, 0xF60AC5CC,
    0xF6502596, 0xFC6F199D, 0xFFCEE1A9,
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
    iso = ed.Iso(ed.find_iso())
    if iso is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    paks = ed.load_paks(iso)
    rows = 0
    seen = {}
    arms = {"ancs NOWHERE": 0, "ancs present": 0, "hit -1": 0, "hit other": 0, "hit absent": 0,
            "dest -1": 0, "dest other": 0, "dest absent": 0, "unknown id": 0, "no editor id": 0}
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
                objs = [(li, o) for li, sec in enumerate(layers) for o in objects(sec)]
                grch = [(li, o) for li, o in objs if o[0] == "GRCH"]
                if not grch:
                    continue
                for li, (t4, eid, body) in grch:
                    d = {}
                    top = None
                    if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                        top = items(body[6:])
                        walk(body[6:], d)
                    ids = [p for p, _ in (top or [])]
                    vals = {p: payload for p, payload in (top or [])}
                    for p in ids:
                        seen[p] = seen.get(p, 0) + 1
                    ancs = d.get("ancs")
                    where = [q.res[ancs][0] for q in paks if ancs is not None and ancs in q.res]
                    kind = where[0] if where else "NOWHERE"
                    arms["ancs present" if where else "ancs NOWHERE"] += 1
                    unk = [p for p in ids if p not in KNOWN]
                    arms["unknown id"] += bool(unk)
                    arms["no editor id"] += 0x255A4580 not in ids

                    def snd(pid, key):
                        if pid not in vals:
                            arms[key + " absent"] += 1
                            return "absent(-1)"
                        (v,) = struct.unpack_from(">i", vals[pid], 0)
                        arms[key + (" -1" if v == -1 else " other")] += 1
                        return v

                    hit = snd(HIT, "hit")
                    dest = snd(DEST, "dest")
                    rows += 1
                    if quiet:
                        continue
                    print(f"GRCH L{li} eid=0x{eid:08x} {d.get('name', '?')[:20]:20} "
                          f"ids={len(ids)} ancs={'-' if ancs is None else '0x%08x' % ancs} type={kind} "
                          f"hit={hit} dest={dest}" + (f" UNKNOWN={[hex(p) for p in unk]}" if unk else ""))
    missing = sorted(KNOWN - set(seen))
    print(f"== {rows} GRCH placements: " + ", ".join(f"{k}={v}" for k, v in arms.items()))
    print(f"== ids no placement carries ({len(missing)}): {[hex(p) for p in missing]}")
    print("== per-id placement counts: " + ", ".join(f"{hex(k)}:{v}" for k, v in sorted(seen.items())))
    return 0


if __name__ == "__main__":
    sys.exit(main())
