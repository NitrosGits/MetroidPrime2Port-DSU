#!/usr/bin/env python3
"""Disc-data checks for host-safety findings A3-2..A3-4 (see docs/AUDIT.md).

  python3 tools/audit/disc_scan.py poi        # A3-2: Int32 POIs of type kPT_RandRate (7) with value <= 0
  python3 tools/audit/disc_scan.py chog       # A3-3: ChozoGhost chance rows with 0 bolts
  python3 tools/audit/disc_scan.py splinter   # A3-4: Splinter placements with i2 + 1 == i1

Reads the ISO through tools/echoes_disc.py (find_iso(), or MP2_ISO).  ANCS carry their event sets
at the tail of the animation set, so `poi` finds them by parsing candidate offsets until one
consumes the file exactly.
"""
import struct, sys, collections
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed


class Bad(Exception):
    pass


class R:
    def __init__(s, b, o=0):
        s.b, s.o = b, o

    def u(s, fmt):
        n = struct.calcsize(">" + fmt)
        if s.o + n > len(s.b):
            raise Bad
        v = struct.unpack_from(">" + fmt, s.b, s.o)[0]
        s.o += n
        return v

    def cstr(s):
        e = s.b.find(b"\0", s.o)
        if e < 0 or e - s.o > 256:
            raise Bad
        v = s.b[s.o:e]
        s.o = e + 1
        return v


def poi(r):
    ver = r.u("H")
    if not 1 <= ver <= 5:
        raise Bad
    if ver >= 3:
        r.cstr(); r.u("I")
    else:
        r.cstr()
    ty = r.u("H")
    r.u("f"); r.u("I")          # CCharAnimTime
    idx = r.u("i"); r.u("B"); r.u("f"); r.u("i"); r.u("i")
    return ver, ty, idx


def count(r, lim=20000):
    n = r.u("I")
    if n > lim:
        raise Bad
    return n


def event_set(r):
    ver = r.u("I")
    if not 1 <= ver <= 3:
        raise Bad
    out = []
    for _ in range(count(r)):
        poi(r); r.u("B")                      # bool node
    for _ in range(count(r)):                 # int32 node
        v, ty, idx = poi(r)
        val = r.u("i"); r.cstr()
        out.append((ty, val))
    for _ in range(count(r)):                 # particle node
        poi(r); r.u("I"); r.u("4s" if False else "I"); r.u("I"); r.u("I"); r.u("f"); r.u("I")
    if ver > 1:
        for _ in range(count(r)):             # sound node
            v, ty, idx = poi(r)
            r.u("i"); r.u("f"); r.u("f")
            if v > 1:
                r.u("i"); r.u("H"); r.u("H"); r.u("f")
    return out


def tail_sets(b):
    for o in range(0, len(b) - 8):
        r = R(b, o)
        try:
            n = r.u("I")
            if not 1 <= n <= 2000:
                continue
            sets = [event_set(r) for _ in range(n)]
        except (Bad, struct.error):
            continue
        if len(b) - r.o <= 32 and set(b[r.o:]) <= {0, 0xff}:
            return o, sets
    return None, None


def load():
    return ed.load_paks(ed.Iso(ed.find_iso()))


def scan_poi():
    ancs = parsed = 0
    hist = collections.Counter()
    bad = []
    seen = set()
    for pk in load():
        for rid, (t, f, s, o) in pk.res.items():
            if t != "ANCS" or rid in seen:
                continue
            seen.add(rid)
            ancs += 1
            off, sets = tail_sets(pk.get(rid))
            if sets is None:
                continue
            parsed += 1
            for i, ev in enumerate(sets):
                for ty, val in ev:
                    hist[(ty, val <= 0)] += 1
                    if ty == 7 and val <= 0:
                        bad.append((rid, i, val))
    print(f"ANCS {ancs}, event sets located in {parsed}")
    print("Int32 POIs by (type, value<=0):", sorted(hist.items()))
    print("kPT_RandRate with value <= 0:", bad or "none")


def props(body):
    p = 6
    (cnt,) = struct.unpack_from(">H", body, p); p += 2
    out = {}
    for _ in range(cnt):
        pid, size = struct.unpack_from(">IH", body, p); p += 6
        out[pid] = body[p:p + size]; p += size
    return out


def objects(want):
    for pk in load():
        for nm, (t, mid) in pk.names.items():
            if t != "MLVL":
                continue
            w = ed.parse_mlvl(pk.get(mid))
            for a in w["areas"]:
                for li, sec in enumerate(ed.mrea_layers(pk.get(a["mrea"]))):
                    for t4, eid, body in ed.scly_objects(sec):
                        if t4 in want:
                            yield nm, a, li, t4, eid, body


def scan_chog():
    # CHOG far/mid/near rows 0xe832241f/0x1e2c8483/0x78d76034, numBolts 0x5ab228b6; defaults 1/3/2
    rows = {0xe832241f: ("far", 1), 0x1e2c8483: ("mid", 3), 0x78d76034: ("near", 2)}
    n = 0
    bad = []
    seen = collections.Counter()
    for nm, a, li, t4, eid, body in objects({"CHOG"}):
        n += 1
        pr = props(body)
        for pid, (rn, dflt) in rows.items():
            val = dflt
            if pid in pr:
                sub = pr[pid]
                (c,) = struct.unpack_from(">H", sub, 0)
                q = 2
                for _ in range(c):
                    i2, sz = struct.unpack_from(">IH", sub, q); q += 6
                    if i2 == 0x5ab228b6:
                        (val,) = struct.unpack_from(">i", sub, q)
                    q += sz
            seen[(rn, val)] += 1
            if val <= 0:
                bad.append((nm, hex(eid), rn, val))
    print(f"CHOG placements {n}; (row, numBolts) histogram: {sorted(seen.items())}")
    print("rows with numBolts <= 0:", bad or "none")


def scan_splinter():
    n = 0
    hist = collections.Counter()
    bad = []
    for nm, a, li, t4, eid, body in objects({"SPTR"}):
        n += 1
        pr = props(body)
        i1 = struct.unpack(">i", pr[0x726cd31d])[0] if 0x726cd31d in pr else 1
        i2 = struct.unpack(">i", pr[0x376e909f])[0] if 0x376e909f in pr else 2
        hist[(i1, i2)] += 1
        if i2 + 1 == i1:
            bad.append((nm, hex(eid), i1, i2))
    print(f"SPTR placements {n}; (i1, i2) histogram: {sorted(hist.items())}")
    print("placements with i2 + 1 == i1:", bad or "none")


if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else ""
    if mode == "poi":
        scan_poi()
    elif mode == "chog":
        scan_chog()
    elif mode == "splinter":
        scan_splinter()
    else:
        sys.exit(__doc__)
