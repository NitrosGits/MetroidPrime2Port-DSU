#!/usr/bin/env python3
"""Print every MYSF (MysteryFlyer, War Wasp) placement of the disc and the TMAI (TeamAiMgr) objects
of its area, for the two bodies mfly3 needs data for.

usage: tools/scan_mysteryflyer_loader.py

Read-only (tools/echoes_disc.py, the user's own disc image). Columns per MYSF placement:

  ancs / type   SLdrPatternedAITypedef.animationInformation (property 0xe25fb08c, SLdrAnimationSet:
                ancs, character_index, initial_anim). LoadMysteryFlyer (CMysteryFlyer.cpp, the
                `if (!modelData) return nullptr` arm, retail 0x300-0x368) passes kInvalidAssetId as
                the model and this ancs to LdrToModelData (ScriptLoader.cpp:446-459), which returns
                an empty optional only when BOTH resource types are zero, i.e. when the ancs id is
                in no pak (type NOWHERE). `type` is the FourCC the pak reports for that id.
  props         the MYSF-specific property ids present (0x44080565 block), so a loader switch arm
                no placement carries is visible (the shipped lists carry only the six known ids).
  tmai          TMAI objects in the same area (all layers): JoinTeam's TeamAiMgr arms need one.
An arm is data-dead only when no placement at any layer of any area takes it.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

ANIM = 0xE25FB08C
MYSF_BLOCK = 0x44080565
NAME = 0x494E414D
TOP = {0x255A4580, 0xB3774750, 0x7E397FED, MYSF_BLOCK}  # the four top-level ids LoadMysteryFlyer switches on
KNOWN = {0x51253BA3, 0xCEA30138, 0x845EF489, 0xC75998AA, 0x01559F27, 0x5F3FFFD6}


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
        elif pid == MYSF_BLOCK:
            sub = items(payload)
            out["props"] = sorted(p for p, _ in sub) if sub is not None else None
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
    rows = 0
    arms = {"ancs NOWHERE": 0, "ancs present": 0, "unknown prop id": 0, "unknown top-level id": 0,
            "ancs not in the area's own pak": 0, "areas with TMAI": 0}
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
                mysf = [(li, o) for li, o in objs if o[0] == "MYSF"]
                if not mysf:
                    continue
                tmai = [(li, o[1]) for li, o in objs if o[0] == "TMAI"]
                arms["areas with TMAI"] += bool(tmai)
                print(f"== {nm}/{iname} (mlvl {mid:08x}) tmai="
                      + (",".join(f"L{li}:0x{e:08x}" for li, e in tmai) or "none"))
                for li, (t4, eid, body) in mysf:
                    d = {}
                    if len(body) >= 8 and struct.unpack_from(">I", body, 0)[0] == 0xFFFFFFFF:
                        walk(body[6:], d)
                    ancs = d.get("ancs")
                    where = [q.res[ancs][0] for q in paks if ancs is not None and ancs in q.res]
                    kind = where[0] if where else "NOWHERE"
                    arms["ancs present" if where else "ancs NOWHERE"] += 1
                    arms["ancs not in the area's own pak"] += bool(ancs is not None and ancs not in pk.res)
                    top = items(body[6:]) if len(body) >= 8 else None
                    utop = [p for p, _ in (top or []) if p not in TOP]
                    arms["unknown top-level id"] += bool(utop or top is None)
                    props = d.get("props")
                    unk = [p for p in (props or []) if p not in KNOWN]
                    arms["unknown prop id"] += bool(unk)
                    rows += 1
                    print(f"MYSF L{li} eid=0x{eid:08x} {d.get('name', '?')[:24]:24} "
                          f"ancs={'-' if ancs is None else '0x%08x' % ancs} type={kind} "
                          f"props={[hex(p) for p in props] if props is not None else '?'}"
                          + (f" UNKNOWN={[hex(p) for p in unk]}" if unk else "")
                          + (f" UNKNOWN_TOP={[hex(p) for p in utop]}" if utop or top is None else ""))
    print(f"== {rows} MYSF placements: " + ", ".join(f"{k}={v}" for k, v in arms.items()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
