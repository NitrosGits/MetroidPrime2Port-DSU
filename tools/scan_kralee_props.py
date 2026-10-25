#!/usr/bin/env python3
"""Print every Kralee (KRAL) placement of the disc with the properties the loader/ctor arms need.

usage: tools/scan_kralee_props.py

Read-only (tools/echoes_disc.py, the user's own disc image; override with MP2_DISC). For each
KRAL placement of every area/layer and every SCGN generated-object record:

  ancs / type   SLdrPatternedAITypedef.animationInformation (property 0xe25fb08c, SLdrAnimationSet:
                ancs, character_index, initial_anim) and the FourCC the pak reports for the ancs
                id. LoadKralee (CKralee.cpp, `if (!modelData) return nullptr`) passes
                kInvalidAssetId as the model and this ancs to LdrToModelData (ScriptLoader.cpp
                :446-459), which returns an empty optional only when BOTH resource types are
                zero, i.e. when the ancs id is in no pak (type NOWHERE).
  warpIn/warpOut  the two ids CKralee's ctor tests against kInvalidAssetId
                (0x351dbc73 / 0x2d72ba7b, SLdrKralee.inc:100-107), with their pak type.
  vis/invis     visibleTime / invisibleTime (0x5704897c / 0xbbd4b10c), the Think timers.
  initiallyInvisible  0x738d1c80 (SLdrKralee.inc:120-122), explicit byte or loader default false.
  props         the top-level property ids the placement carries, so an unknown id is visible.

Scan all MLVL resources, including unnamed worlds. Report section/object coverage and fail
on parse errors instead of silently skipping areas. An arm is data-dead only with full coverage.
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
INITIALLY_INVISIBLE = 0x738D1C80
NAME = 0x494E414D
# every property id SLdrKralee.inc switches on
KNOWN = {
    0x255A4580, 0xBE73724A, 0xB3774750, 0x7E397FED, 0x733BD27C, 0xA72530E8, 0xAC47C628,
    0x8E4F7B29, 0x3D3C1B76, 0xD5C25506, 0x742EAB20, 0x80A81909, 0xED6A9353, 0x033153A0,
    0x5704897C, 0x7BBA36FF, 0xBBD4B10C, 0x4E4AE0E4, 0xAD69AC32, 0xC7D2EDE8, 0xB1A26335,
    0x8590483B, 0x214E48A0, 0x351DBC73, 0x2D72BA7B, 0x80B58324, 0xA4EF7B42, 0xC3CC437F,
    INITIALLY_INVISIBLE,
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
        if p + 6 + size > len(buf):
            return None
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


def mrea_scripts(buf):
    """Extract all SCLY sections plus SCGN, including sections in multi-section blocks."""
    h = struct.unpack_from(">32I", buf)
    if h[0] != 0xDEADBEEF or h[1] != 0x19:
        raise ValueError("expected version-0x19 MREA")
    nsec, nlay, first, nblk = h[16], h[15], h[18], h[28]
    if first + nlay >= nsec:
        raise ValueError("script section range exceeds MREA section count")
    sizes = struct.unpack_from(f">{nsec}I", buf, 0x80)
    pos = 0x80 + ((nsec * 4 + 31) & ~31)
    blocks = [struct.unpack_from(">4I", buf, pos + 16 * i) for i in range(nblk)]
    pos += (nblk * 16 + 31) & ~31
    if sum(sc for _, _, _, sc in blocks) != nsec:
        raise ValueError("MREA block section counts do not cover section table")
    sections = [None] * (nlay + 1)
    si = 0
    for size, dl, cl, sc in blocks:
        rd = ((cl or size) + 31) & ~31
        if pos + rd > len(buf):
            raise ValueError("truncated MREA block")
        lo, hi = max(si, first), min(si + sc, first + nlay + 1)
        if lo < hi:
            raw = buf[pos:pos + rd]
            data = ed.inflate_chunks(raw[rd - cl:], dl) if cl else raw[:dl]
            if len(data) != dl or sum(sizes[si:si + sc]) > len(data):
                raise ValueError("truncated MREA section data")
            off = sum(sizes[si:lo])
            for target in range(lo, hi):
                sec = data[off:off + sizes[target]]
                magic = b"SCGN" if target == first + nlay else b"SCLY"
                if sec[:4] != magic:
                    raise ValueError(f"expected {magic!r} at MREA section {target}")
                sections[target - first] = sec
                off += sizes[target]
        pos += rd
        si += sc
    if any(sec is None for sec in sections):
        raise ValueError("missing script section")
    return sections


def objects(sec):
    if sec[:4] not in (b"SCLY", b"SCGN"):
        raise ValueError("expected SCLY or SCGN")
    count_offset = 10 if sec[:4] == b"SCLY" else 6
    count = struct.unpack_from(">I", sec, count_offset)[0]
    p = count_offset + 4
    out = []
    for _ in range(count):
        if p + 6 > len(sec):
            raise ValueError("truncated script object header")
        t = sec[p:p + 4].decode("latin1")
        (ln,) = struct.unpack_from(">H", sec, p + 4)
        if ln < 6 or p + 6 + ln > len(sec):
            raise ValueError("truncated script object body")
        body = sec[p + 6:p + 6 + ln]
        eid, nc = struct.unpack_from(">IH", body, 0)
        if 6 + 12 * nc > len(body):
            raise ValueError("truncated script object connections")
        out.append((t, eid, body[6 + 12 * nc:]))
        p += 6 + ln
    return out


def kralee_properties(body):
    if len(body) < 8 or struct.unpack_from(">I", body)[0] != 0xFFFFFFFF:
        raise ValueError("invalid KRAL property wrapper")
    top = items(body[6:])
    if top is None:
        raise ValueError("invalid KRAL property list")
    d = {}
    walk(body[6:], d)
    # Only the top-level property controls the ctor; omitted means the SLdr default.
    initially_invisible, source = False, "default"
    for pid, payload in top:
        if pid == INITIALLY_INVISIBLE:
            if len(payload) != 1:
                raise ValueError("initiallyInvisible is not a one-byte bool")
            initially_invisible, source = bool(payload[0]), f"explicit:{payload.hex()}"
    return d, top, initially_invisible, source


def main():
    path = ed.find_iso()
    if path is None:
        print("no disc image (set MP2_DISC)", file=sys.stderr)
        return 1
    iso = ed.Iso(path)
    paks = ed.load_paks(iso)

    def kind(aid):
        for q in paks:
            if aid in q.res:
                return q.res[aid][0]
        return "NOWHERE"

    rows = generated_krals = errors = 0
    worlds = areas = layers = generated_sections = generated_objects = 0
    expected_worlds = sum(t == "MLVL" for pk in paks for t, *_ in pk.res.values())
    expected_areas = expected_layers = 0
    arms = {"ancs NOWHERE": 0, "ancs present": 0, "warpIn valid": 0, "warpOut valid": 0,
            "warpIn unset": 0, "warpOut unset": 0, "unknown prop id": 0}
    invisible = {"false": 0, "true": 0, "explicit": 0, "default": 0}
    for pk in paks:
        names = {mid: nm for nm, (t, mid) in pk.names.items() if t == "MLVL"}
        for mid, (t, *_) in pk.res.items():
            if t != "MLVL":
                continue
            nm = names.get(mid, f"{pk.path}/unnamed-{mid:08x}")
            try:
                w = ed.parse_mlvl(pk.get(mid))
            except Exception as exc:
                print(f"ERROR {nm}: {exc}", file=sys.stderr)
                errors += 1
                continue
            worlds += 1
            expected_areas += len(w["areas"])
            for a in w["areas"]:
                iname = a["iname"] or f"area{a['index']}"
                try:
                    buf = pk.get(a["mrea"])
                    expected_layers += struct.unpack_from(">I", buf, 15 * 4)[0]
                    sections = mrea_scripts(buf)
                    krals = []
                    for li, sec in enumerate(sections):
                        generated = sec[:4] == b"SCGN"
                        records = objects(sec)
                        generated_sections += generated
                        generated_objects += len(records) if generated else 0
                        layers += not generated
                        for t4, eid, body in records:
                            if t4 == "KRAL":
                                krals.append(("SCGN" if generated else f"L{li}", eid,
                                              kralee_properties(body)))
                    areas += 1
                except Exception as exc:
                    print(f"ERROR {nm}/{iname}: {exc}", file=sys.stderr)
                    errors += 1
                    continue
                if not krals:
                    continue
                print(f"== {nm}/{iname} (mlvl {mid:08x})")
                for location, eid, (d, top, initially_invisible, source) in krals:
                    utop = [p for p, _ in top if p not in KNOWN]
                    arms["unknown prop id"] += bool(utop)
                    ancs = d.get("ancs")
                    akind = kind(ancs) if ancs is not None else "-"
                    arms["ancs present" if akind not in ("NOWHERE", "-") else "ancs NOWHERE"] += 1
                    wi, wo = d.get("warpIn"), d.get("warpOut")
                    arms["warpIn valid"] += bool(wi not in (None, 0xFFFFFFFF))
                    arms["warpOut valid"] += bool(wo not in (None, 0xFFFFFFFF))
                    arms["warpIn unset"] += bool(wi in (None, 0xFFFFFFFF))
                    arms["warpOut unset"] += bool(wo in (None, 0xFFFFFFFF))
                    rows += 1
                    generated_krals += location == "SCGN"
                    value = str(initially_invisible).lower()
                    invisible[value] += 1
                    invisible[source.split(":")[0]] += 1
                    print(f"KRAL {location} eid=0x{eid:08x} {d.get('name', '?')[:24]:24} "
                          f"ancs={'-' if ancs is None else '0x%08x' % ancs} ancsType={akind} "
                          f"warpIn={'-' if wi is None else '0x%08x' % wi}"
                          f"({'-' if wi is None else kind(wi)}) "
                          f"warpOut={'-' if wo is None else '0x%08x' % wo}"
                          f"({'-' if wo is None else kind(wo)}) "
                          f"vis={d.get('visibleTime', 'default')} inv={d.get('invisibleTime', 'default')} "
                          f"initiallyInvisible={value}({source})"
                          + (f" UNKNOWN_TOP={[hex(p) for p in utop]}" if utop else ""))
    print(f"== {rows - generated_krals} KRAL placements, {generated_krals} generated KRAL objects: "
          + ", ".join(f"{k}={v}" for k, v in arms.items()))
    print("== initiallyInvisible: " + ", ".join(f"{k}={v}" for k, v in invisible.items()))
    print(f"== coverage: worlds={worlds}/{expected_worlds}, areas={areas}/{expected_areas}, "
          f"SCLY layers={layers}/{expected_layers}, SCGN sections={generated_sections}/{expected_areas}, "
          f"SCGN objects={generated_objects}, parse_errors={errors}")
    complete = (expected_worlds > 0 and errors == 0 and worlds == expected_worlds
                and areas == expected_areas and layers == expected_layers
                and generated_sections == expected_areas)
    if not complete:
        print("INCOMPLETE: scan cannot establish absence of any KRAL arm", file=sys.stderr)
    return 0 if complete else 1


if __name__ == "__main__":
    sys.exit(main())
