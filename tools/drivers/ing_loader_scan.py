#!/usr/bin/env python3
"""Which arms of the Ing loader (fn_29_130) the shipped data can take.

usage: tools/drivers/ing_loader_scan.py [<Ing text .s>]  (default: this tree's build/G2ME01 asm;
       a lane worktree without the matching build passes the main tree's)

Read-only, over the user's own disc (tools/echoes_disc.py, the SCLY readers of
tools/scan_ing_pattern.py). The loader's property loop (asm 0x1A0-0xC60) compares each property
id against 69 constants (the `cmpw r0, ...` binary search) and skips an unknown id
(0xC4C ReadBytes); after the loop it returns null when the model data is empty (0xCA0 lbz
0x3f4 / 0xCA8 bne). For every INGS placement this prints its top-level property ids against the
loader's 69, and whether its animation set (0xe25fb08c inside the patterned block) names an ANCS:
  * an id present in every placement is a case arm every load takes;
  * an id no placement has is the default (skip) arm; none means that arm is dead in the data;
  * a placement with no ANCS takes the no-model arm.
"""
import pathlib
import re
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402
import scan_ing_pattern as sip  # noqa: E402

ANIMATION_SET = 0xE25FB08C
ASM = ROOT / "build/G2ME01/Ing/asm/auto_00_00000000_text.s"


def loader_ids(asm):
    """The 69 property ids fn_29_130 compares against (lis/addi or subi, then cmpw r0, rN)."""
    text = pathlib.Path(asm).read_text().split(".fn fn_29_130,", 1)[1].split(".endfn fn_29_130", 1)[0]
    regs, ids = {}, set()
    for line in text.splitlines():
        ins = line.split("*/", 1)[-1].strip()
        m = re.match(r"lis (r\d+), (-?0x[0-9a-f]+|\d+)$", ins)
        if m:
            regs[m[1]] = (int(m[2], 0) << 16) & 0xFFFFFFFF
            continue
        m = re.match(r"(addi|subi) (r\d+), (r\d+), (-?0x[0-9a-f]+|\d+)$", ins)
        if m and m[3] in regs:
            v = int(m[4], 0)
            regs[m[2]] = (regs[m[3]] + (-v if m[1] == "subi" else v)) & 0xFFFFFFFF
            continue
        m = re.match(r"cmpw r0, (r\d+)$", ins)
        if m:
            ids.add(regs[m[1]])
    return ids


def find_ancs(data, depth=0):
    """The ANCS of the first animation set (0xe25fb08c) in a nested struct (u16 count, props)."""
    if depth > 4 or len(data) < 2:
        return None
    try:
        (count,) = struct.unpack_from(">H", data, 0)
        p = 2
        for _ in range(count):
            pid, size = struct.unpack_from(">IH", data, p)
            body = data[p + 6:p + 6 + size]
            p += 6 + size
            if pid == ANIMATION_SET and size >= 4:
                return struct.unpack_from(">I", body, 0)[0]
            if size >= 8:
                r = find_ancs(body, depth + 1)
                if r is not None:
                    return r
    except struct.error:
        return None
    return None


def main():
    ids = loader_ids(sys.argv[1] if len(sys.argv) > 1 else ASM)
    print(f"loader ids: {len(ids)}")
    iso = ed.Iso(ed.find_iso())
    n = 0
    seen_in_all = set(ids)
    extra = {}
    no_model = []
    for pk in ed.load_paks(iso):
        for nm, (t, mid) in sorted(pk.names.items()):
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
                    if sec[:4] != b"SCLY":
                        continue
                    for t4, eid, _c, props in sip.scly_objects(sec):
                        if t4 != sip.INGS:
                            continue
                        n += 1
                        seen_in_all &= set(props)
                        for pid in set(props) - ids:
                            extra.setdefault(pid, []).append(eid)
                        ancs = None
                        for pid, body in props.items():
                            ancs = find_ancs(body)
                            if ancs is not None:
                                break
                        if ancs in (None, 0xFFFFFFFF):
                            no_model.append((nm, a["iname"], li, eid, ancs))
    print(f"INGS placements: {n}")
    print(f"loader ids present in every placement: {len(seen_in_all)} of {len(ids)}")
    for pid in sorted(ids - seen_in_all):
        print(f"  missing somewhere: 0x{pid:08x}")
    print(f"ids not in the loader (the skip arm): {len(extra)}")
    for pid, eids in sorted(extra.items()):
        print(f"  0x{pid:08x} in {len(eids)} placements")
    print(f"placements with no ANCS (the no-model arm): {len(no_model)}")
    for row in no_model[:20]:
        print("  ", row)
    return 0


if __name__ == "__main__":
    sys.exit(main())
