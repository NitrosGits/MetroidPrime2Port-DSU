#!/usr/bin/env python3
"""Dump and diff the save-relevant world data (MLVL + SAVW + HINT) of extracted Echoes discs.

Research tool for save conversion between releases (docs/SAVE_RELEASES.md). Read-only: it only
opens the .pak files under each directory given. Run with `python3 -I`, from outside the
extracted trees:

  python3 -I tools/savw_diff.py G2ME01=/tmp/opencode/discs/G2ME01/files \
      G2MP01=/tmp/opencode/discs/G2MP01/files R3ME01=/tmp/opencode/discs/R3ME01/DATA/files/MP2

The first release is the base the others are diffed against. What it reports, and the code each
number mirrors:

- MemoryWorlds: every MLVL in the paks, sorted by asset ID (CMemoryCard::CMemoryCard).
- per SAVW (CWorldSaveGameInfo): areaCount, cinematics, relays, layers, doors, scans, system and
  game variables, unmappable objects; and CalculateHash() over the big-endian (console) and
  little-endian (TARGET_PC host) in-memory images.
- the save signature, 0x5553413e ^ hash of every world (GetSaveSignature in CMemoryCardDriver).
  BE is the console value, which the port also writes since CalculateHash swaps its words; LE is
  what older port builds wrote and still accept ("legacy port"). The G2ME01 values, 44bfc715 and
  37d0412e, equal the two `[save] ... signature` lines the port logs.
- per MLVL: the per-area layer counts (CWorldLayers::ReadWorldLayers), which size the
  CWorldLayerState bits.
- the CWorldState record size in bits as CGameState::PutTo writes it, and the global lists
  (scan union sorted by ID, cinematic bits, env-var bits, HINT count).
"""
import os
import struct
import sys
import zlib

ROOT = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, ROOT)
import echoes_disc as ed  # noqa: E402

SIGNATURE_SEED = 0x5553413E
# CGameStateEnvVarManager::LoadFields (system scope), added before the SAVW variables.
SYSTEM_BUILTINS = [
    ("FreezeInstructionsFirstPerson", 0, 3), ("FreezeInstructionsMorphBall", 0, 3),
    ("PowerbombPickupMessages", 0, 1), ("PercentScans", 0, 100), ("NormalModeCompleted", 0, 1),
    ("HardModeCompleted", 0, 1), ("AllPickupsFound", 0, 1), ("AutoMapperPaneMode", 0, 2),
    ("LogbookLegendVisible", 0, 1), ("IngAttachedWarningCount", 0, 3), ("SeenIntroText", 0, 1),
]


class Dir:
    """echoes_disc.Iso stand-in over an extracted directory."""

    def __init__(self, root):
        self.root, self.files = root, {}
        for dp, _, fs in os.walk(root):
            for f in fs:
                p = os.path.join(dp, f)
                self.files[os.path.relpath(p, root)] = (p, os.path.getsize(p))

    def read(self, path, off=0, n=None):
        with open(self.files[path][0], "rb") as f:
            f.seek(off)
            return f.read(-1 if n is None else n)


def bitcount(v):
    """CEnvironmentVariable::GetBitCount."""
    return max(v, 0).bit_length()


def cstr(b, p):
    e = b.index(b"\0", p)
    return b[p:e].decode("latin1"), e + 1


def parse_savw(b):
    r = ed.Rd(b)
    s = {"magic": r.u32(), "version": r.u32(), "areaCount": r.u32()}

    def vec(n):
        c = r.u32()
        return [tuple(r.u32() for _ in range(n)) if n > 1 else r.u32() for _ in range(c)]

    s["cinematics"], s["relays"], s["layers"] = vec(1), vec(1), vec(2)
    s["doors"], s["scans"] = vec(1), vec(2)

    def envs():
        out = []
        for _ in range(r.u32()):
            name, r.p = cstr(b, r.p)
            out.append((name, *struct.unpack_from(">iii", b, r.p)))
            r.p += 12
        return out

    s["sysvars"], s["gamevars"] = (envs(), envs()) if s["version"] > 3 else ([], [])
    s["unmappable"] = vec(1) if s["version"] > 4 else []
    s["trailing"] = len(b) - r.p
    s["trailingNonPad"] = any(c not in (0, 0xFF) for c in b[r.p:])
    return s


def savw_hash(s, endian):
    """CWorldSaveGameInfo::CalculateHash over the in-memory image in the given byte order."""
    words = [s["areaCount"], *s["cinematics"], *s["relays"], *s["doors"]]
    words += [x for pair in s["layers"] for x in pair]
    words += [x for pair in s["scans"] for x in pair]
    words += s["unmappable"]
    data = struct.pack(endian + "%dI" % len(words), *words)
    h = 0
    for name, *_ in s["sysvars"] + s["gamevars"]:
        h = zlib.crc32(name.encode("latin1"), h ^ 0xFFFFFFFF) ^ 0xFFFFFFFF  # seed 0, no xorout
    return zlib.crc32(data, h ^ 0xFFFFFFFF) ^ 0xFFFFFFFF


def parse_mlvl(b):
    """echoes_disc.parse_mlvl plus the SAVW ID, area save IDs and the trailing layer table."""
    r = ed.Rd(b)
    r.u32()
    ver = r.u32()
    m = {"version": ver, "strg": r.u32()}
    if ver >= 22:
        r.u32()
    if ver >= 23:
        r.u32()
    m["savw"] = r.u32()
    r.u32()  # skybox
    assert ver >= 18, ver
    areas = []
    for _ in range(r.u32()):
        r.u32()
        r.skip(48 + 24)
        mrea = r.u32()
        save_id = r.u32()
        r.skip(2 * r.u32())
        r.skip(8 * r.u32())
        r.skip(8 * r.u32())
        r.skip(4 * r.u32())
        for _ in range(r.u32()):
            r.skip(8 * r.u32())
            r.skip(12 * r.u32())
        for _ in range(r.u32()):
            r.cstr()
        r.skip(4 * r.u32())
        r.cstr()
        areas.append((mrea, save_id))
    m["areas"] = areas
    r.u32()  # MAPW
    r.u32()
    r.cstr()  # default audio track
    m["layerCounts"] = []
    for _ in range(r.u32()):
        m["layerCounts"].append(r.u32())
        r.skip(8)  # default layer bits
    m["layerNames"] = [r.cstr() for _ in range(r.u32())]
    m["layerStart"] = [r.u32() for _ in range(r.u32())]
    return m


def area_layers(m, i):
    start = m["layerStart"][i]
    return m["layerNames"][start:start + m["layerCounts"][i]]


def load_release(label, root):
    d = Dir(root)
    rel = {"label": label, "mlvl": {}, "savw": {}, "hint": None}
    for p in sorted(d.files):
        if not p.lower().endswith(".pak"):
            continue
        pak = ed.Pak(d, p)
        for rid, (t, *_) in pak.res.items():
            if t == "MLVL" and rid not in rel["mlvl"]:
                rel["mlvl"][rid] = (parse_mlvl(pak.get(rid)), p)
            elif t == "SAVW" and rid not in rel["savw"]:
                rel["savw"][rid] = parse_savw(pak.get(rid))
        if rel["hint"] is None and "HINT_Hints" in pak.names:
            hb = pak.get(pak.names["HINT_Hints"][1])
            magic, ver, count = struct.unpack_from(">III", hb, 0)
            rel["hint"] = {"id": pak.names["HINT_Hints"][1], "version": ver, "count": count,
                           "crc": zlib.crc32(hb), "pak": p}
    rel["worlds"] = sorted(rel["mlvl"])  # MemoryWorlds order
    sysvars = {n: (lo, hi) for n, lo, hi in SYSTEM_BUILTINS}
    gamevars, scans, cines = {}, set(), 0
    sig_be = sig_le = SIGNATURE_SEED
    for w in rel["worlds"]:
        s = rel["savw"][rel["mlvl"][w][0]["savw"]]
        s["hashBE"], s["hashLE"] = savw_hash(s, ">"), savw_hash(s, "<")
        sig_be ^= s["hashBE"]
        sig_le ^= s["hashLE"]
        scans.update(s["scans"])
        cines += len(s["cinematics"])
        for n, lo, hi, _ in s["sysvars"]:
            sysvars.setdefault(n, (lo, hi))
        for n, lo, hi, _ in s["gamevars"]:
            gamevars.setdefault(n, (lo, hi))
    rel["sigBE"], rel["sigLE"] = sig_be, sig_le
    rel["scanUnion"] = sorted(scans)
    rel["cinematicBits"] = cines
    rel["sysvars"], rel["gamevars"] = sysvars, gamevars
    return rel


def world_bits(m, s):
    """CWorldState::PutTo body size in bits (after the u32 MLVL + u16 bitCount prefix)."""
    layers = sum(c - 1 for c in m["layerCounts"])
    return (64 + len(s["relays"]) + 2 * s["areaCount"] + len(s["doors"]) +
            len(s["unmappable"]) + 1 + 10 + layers)


def envbits(v):
    return sum(bitcount(hi - lo) for lo, hi in v.values())


def h(v):
    return "%08x" % v


def dump(rel):
    print("== %s" % rel["label"])
    print("  MemoryWorlds (%d, sorted by MLVL ID):" % len(rel["worlds"]))
    print("    %-8s %-8s %-15s %5s %4s %4s %4s %4s %4s %4s %4s %6s %8s %8s" % (
        "MLVL", "SAVW", "pak", "areas", "ver", "cine", "rely", "lyrs", "door", "scan", "unmp",
        "wbits", "hashBE", "hashLE"))
    for w in rel["worlds"]:
        m, pak = rel["mlvl"][w]
        s = rel["savw"][m["savw"]]
        print("    %s %s %-15s %5d %4d %4d %4d %4d %4d %4d %4d %6d %s %s" % (
            h(w), h(m["savw"]), pak, s["areaCount"], s["version"], len(s["cinematics"]),
            len(s["relays"]), len(s["layers"]), len(s["doors"]), len(s["scans"]),
            len(s["unmappable"]), world_bits(m, s), h(s["hashBE"]), h(s["hashLE"])))
        if len(m["areas"]) != s["areaCount"] or len(m["layerCounts"]) != s["areaCount"]:
            print("      !! MLVL areas %d, layer table %d" % (len(m["areas"]), len(m["layerCounts"])))
        if s["trailingNonPad"]:
            print("      !! %d trailing SAVW bytes that are not 00/FF padding" % s["trailing"])
    print("  signature console (BE words; port writes) %s   legacy port (LE words) %s" % (
        h(rel["sigBE"]), h(rel["sigLE"])))
    print("  scan union %d entries (+4 fixed) -> POWR..SCAN scan bits %d" % (
        len(rel["scanUnion"]), 2 * (len(rel["scanUnion"]) + 4)))
    print("  cinematic bits %d; system vars %d (%d bits); game vars %d (%d bits)" % (
        rel["cinematicBits"], len(rel["sysvars"]), envbits(rel["sysvars"]),
        len(rel["gamevars"]), envbits(rel["gamevars"])))
    hi = rel["hint"]
    print("  HINT_Hints %s" % ("not found" if hi is None else "%s v%d count %d crc %s (%s)" % (
        h(hi["id"]), hi["version"], hi["count"], h(hi["crc"]), hi["pak"])))


def diff_list(name, a, b, out):
    if a == b:
        return
    sa, sb = set(a), set(b)
    if sa == sb:
        out.append("    %s: same %d entries, different order" % (name, len(a)))
        return
    out.append("    %s: %d -> %d; only base %d, only other %d%s" % (
        name, len(a), len(b), len(sa - sb), len(sb - sa),
        "; common entries keep relative order" if [x for x in a if x in sb] ==
        [x for x in b if x in sa] else "; common entries reordered"))


def diff(base, other):
    print("== diff %s -> %s" % (base["label"], other["label"]))
    wa, wb = base["worlds"], other["worlds"]
    print("  worlds only in %s: %s" % (base["label"], [h(w) for w in wa if w not in wb] or "-"))
    print("  worlds only in %s: %s" % (other["label"], [h(w) for w in wb if w not in wa] or "-"))
    common = [w for w in wa if w in wb]
    if [w for w in wb if w in wa] != common:
        print("  !! MemoryWorlds order of common worlds differs")
    for w in common:
        ma, mb = base["mlvl"][w][0], other["mlvl"][w][0]
        sa, sb = base["savw"][ma["savw"]], other["savw"][mb["savw"]]
        out = []
        if ma["savw"] != mb["savw"]:
            out.append("    SAVW id %s -> %s" % (h(ma["savw"]), h(mb["savw"])))
        for k in ("version", "areaCount"):
            if sa[k] != sb[k]:
                out.append("    %s %d -> %d" % (k, sa[k], sb[k]))
        if ma["areas"] != mb["areas"]:
            out.append("    MLVL (MREA, area save ID) list differs")
        if ma["layerCounts"] != mb["layerCounts"]:
            d = [(i, x, y) for i, (x, y) in enumerate(zip(ma["layerCounts"], mb["layerCounts"]))
                 if x != y]
            out.append("    layer counts differ in %d areas: %s" % (len(d), d[:8]))
            for i, _, _ in d[:8]:
                out.append("      area %d base   %s" % (i, area_layers(ma, i)))
                out.append("      area %d other  %s" % (i, area_layers(mb, i)))
        for k in ("cinematics", "relays", "layers", "doors", "scans", "sysvars", "gamevars",
                  "unmappable"):
            diff_list(k, sa[k], sb[k], out)
        if world_bits(ma, sa) != world_bits(mb, sb):
            out.append("    CWorldState bits %d -> %d" % (world_bits(ma, sa), world_bits(mb, sb)))
        if sa["hashBE"] != sb["hashBE"]:
            out.append("    hash %s -> %s" % (h(sa["hashBE"]), h(sb["hashBE"])))
        print("  world %s: %s" % (h(w), "identical" if not out else ""))
        for line in out:
            print(line)
    print("  signature console %s -> %s" % (h(base["sigBE"]), h(other["sigBE"])))
    out = []
    diff_list("scan union", base["scanUnion"], other["scanUnion"], out)
    for line in out or ["    scan union identical (%d)" % len(base["scanUnion"])]:
        print(line)
    for k in ("sysvars", "gamevars"):
        if base[k] != other[k]:
            print("    %s differ: only base %s, only other %s, range changed %s" % (
                k, sorted(set(base[k]) - set(other[k])), sorted(set(other[k]) - set(base[k])),
                sorted(n for n in base[k] if n in other[k] and base[k][n] != other[k][n])))
        else:
            print("    %s identical (%d)" % (k, len(base[k])))
    if base["cinematicBits"] != other["cinematicBits"]:
        print("    cinematic bits %d -> %d" % (base["cinematicBits"], other["cinematicBits"]))
    hb, ho = base["hint"], other["hint"]
    if hb and ho:
        print("    HINT count %d -> %d, bytes %s" % (
            hb["count"], ho["count"], "identical" if hb["crc"] == ho["crc"] else "differ"))


def main(argv):
    if not argv:
        argv = ["G2ME01=/tmp/opencode/discs/G2ME01/files",
                "G2MP01=/tmp/opencode/discs/G2MP01/files",
                "R3ME01=/tmp/opencode/discs/R3ME01/DATA/files/MP2"]
    rels = [load_release(*a.split("=", 1)) for a in argv]
    for r in rels:
        dump(r)
    for r in rels[1:]:
        diff(rels[0], r)
    if len(rels) > 2:
        diff(rels[1], rels[2])


if __name__ == "__main__":
    main(sys.argv[1:])
