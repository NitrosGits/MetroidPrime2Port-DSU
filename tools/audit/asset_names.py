#!/usr/bin/env python3
"""Audit sweep 1: asset-name string literals vs the disc's CINF bones and ANCS/CHAR tables.

usage: tools/audit/asset_names.py [--cache FILE]            disc mode (MP2_DISC points at the ISO)
       tools/audit/asset_names.py --rel [--all] [--retail DIR]  retail-string mode

Read-only, over the user's own disc (tools/echoes_disc.py's readers). It collects every string
literal passed to a locator/bone lookup (GetLocatorSegId, GetLocatorTransform, GetLctrTransform,
GetScaledLocatorTransform, GetSegIdFromString), an effect lookup (SetEffectState,
GetFirstParticleEffect) or an animation lookup (GetAnimTimeRemaining) in src/ and platform/
(upstream code, Port*.cpp, TARGET_PC blocks), attributes each to a module through
docs/modules.tsv (source file -> script_types -> area placements -> the ANCS ids found in the
placement's properties), and checks the name, case-insensitively, against the CINF bone names,
the CHAR effect tables and the CHAR animation names of every character of those ANCS.
A name missing from the module's characters is a miss; one also missing from every ANCS on the
disc is a "disc-wide" miss (confirmed); the others are "suspected" (attribution is by ANCS id
bytes in the placement properties, not by the character index). Names not built from a single
literal, files outside modules.tsv and modules with no placement go to the unchecked list.
Prints ledger rows `| A1-k | area | file:line | asset-name | severity | evidence | open |`.
--cache FILE keeps the (slow) placement scan between runs.
--rel: every string literal in the code the port compiles (src/, platform/, and the headers of the
same stem) must appear NUL-terminated in the retail REL of its module (DIR/<M>/<M>.rel, default
../wt-mp2-audit5b/build/G2ME01) or, for non-REL code, in orig/G2ME01/sys/main.dol. Misses are split
into port-only (log/debug/console text) and findings, with the nearest retail string. --all also
lists the literals found in retail.

Layouts: CINF (CCharLayoutInfo), CHAR (CCECharacterInfo, CAnimCharacterSet), effect component
(CEffectComponent: name, tag, u32 segment id, scale, parented mode, flags).
"""
import json
import pathlib
import re
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402


class R:
    def __init__(s, b, p=0): s.b = b; s.p = p
    def u16(s): v = struct.unpack_from(">H", s.b, s.p)[0]; s.p += 2; return v
    def u32(s): v = struct.unpack_from(">I", s.b, s.p)[0]; s.p += 4; return v
    def skip(s, n): s.p += n
    def cstr(s):
        e = s.b.index(b"\0", s.p); v = s.b[s.p:e].decode("latin1"); s.p = e + 1; return v


def parse_cinf(b):
    r = R(b); segs = {}
    for _ in range(r.u32()):
        sid = r.u32(); r.u32(); r.skip(44); k = r.u32(); r.skip(4 * k); segs[sid] = 1
    r.skip(4 * r.u32())
    names = {}
    for _ in range(r.u32()):
        nm = r.cstr(); names[nm.lower()] = r.u32()
    return names, set(segs)


def vec_ids(r):
    return [r.u32() for _ in range(r.u32())]


def parse_char(r):
    c = {}
    tc = r.u16(); r.cstr()
    r.u32(); r.u32(); c["cinf"] = r.u32()
    c["anims"] = {nm.lower() for _ in range(r.u32()) for _i, nm in [(r.u32(), r.cstr())]}
    r.u32(); sc = r.u32(); r.u32()
    for _ in range(sc):
        r.u32(); pc = r.u32(); ac = r.u32(); types = []
        for _ in range(pc):
            t = struct.unpack_from(">i", r.b, r.p)[0]; r.skip(12); types.append(t)
            if t in (0, 1, 2, 4): r.skip(8)
            elif t == 3: r.skip(2)
        for _ in range(ac):
            r.skip(4)
            for t in types:
                r.skip(1 if t == 3 else 4 if t in (0, 1, 2, 4) else 0)
    vec_ids(r); vec_ids(r); vec_ids(r)
    if tc > 5: vec_ids(r)
    if tc > 8: vec_ids(r); vec_ids(r)
    r.u32()
    if tc > 1:
        for _ in range(r.u32()): r.cstr(); r.skip(24)
    c["effects"] = {}
    if tc > 2:
        for _ in range(r.u32()):
            nm = r.cstr(); comps = []
            for _ in range(r.u32()):
                r.cstr(); r.skip(8); comps.append(r.u32()); r.skip(12)
            c["effects"][nm.lower()] = comps
    if tc > 3: r.skip(8)
    if tc > 4: r.skip(4 * r.u32())
    if tc > 6: r.skip(4)
    if tc > 7: r.skip(1)
    if tc > 9:
        for _ in range(r.u32()): r.skip(28)
    return c


def parse_ancs(b):
    r = R(b); r.u16(); r.u16(); chars = []
    r.p = 4; n = struct.unpack_from(">i", b, r.p)[0]; r.p += 4
    for _ in range(n):
        r.skip(4); chars.append(parse_char(r))
    return chars


def load_disc():
    iso = ed.Iso(ed.find_iso())
    paks = ed.load_paks(iso)
    ancs, cinfs, seen = {}, {}, {}
    for pk in paks:
        for rid, (t, _f, _s, _o) in pk.res.items():
            if rid in seen:
                continue
            if t == "ANCS":
                seen[rid] = 1
                try:
                    ancs[rid] = parse_ancs(pk.get(rid))
                except Exception:
                    pass
            elif t == "CINF":
                seen[rid] = 1
                try:
                    cinfs[rid] = parse_cinf(pk.get(rid))
                except Exception:
                    pass
    return iso, paks, ancs, cinfs


def scan_placements(paks, ancs_ids):
    """{fourcc: [ancs ids]} from the ANCS ids found anywhere in each placement's properties."""
    out = {}
    done = set()
    for pk in paks:
        for nm, (t, mid) in pk.names.items():
            if t != "MLVL" or mid in done:
                continue
            done.add(mid)
            try:
                w = ed.parse_mlvl(pk.get(mid))
            except Exception:
                continue
            for a in w["areas"]:
                try:
                    layers = ed.mrea_layers(pk.get(a["mrea"]))
                except Exception:
                    continue
                for sec in layers:
                    try:
                        objs = ed.scly_objects(sec)
                    except Exception:
                        continue
                    for t4, _eid, body in objs:
                        s = out.setdefault(t4, set())
                        for o in range(len(body) - 3):
                            v = struct.unpack_from(">I", body, o)[0]
                            if v in ancs_ids:
                                s.add(v)
    return {k: sorted(v) for k, v in out.items()}


CALLS = {
    "GetLocatorSegId": "loc", "GetLocatorTransform": "loc", "GetLctrTransform": "loc",
    "GetScaledLocatorTransform": "loc", "GetSegIdFromString": "loc",
    "SetEffectState": "fx", "GetFirstParticleEffect": "fx",
    "GetAnimTimeRemaining": "anim",
}
CALL_RE = re.compile(r"\b(%s)\s*\(\s*(?:\w+::)*(?:string_l\s*\(\s*)?(\"(?:[^\"\\\n]|\\.)*\"|[^\"\s)][^,)]*)"
                     % "|".join(CALLS))
FALLBACK = {
    "loc": ("gameplay", "failed lookup gives CSegId::Invalid, the transform falls back to identity/feet"),
    "fx": ("visual", "failed lookup is silently ignored, the effect never plays"),
    "anim": ("gameplay", "failed lookup returns no animation, timing falls back to 0"),
}


def const_literals(ident, texts):
    """The string literals of a named constant's initialiser (`const char* kName = "x"` or an array)."""
    for t in texts:
        m = re.search(r"\b%s\b\s*(?:\[[^\]]*\])*\s*=\s*(\{[^;]*?\}\s*;|\"(?:[^\"\\\n]|\\.)*\")" % re.escape(ident), t)
        if m:
            lits = re.findall(r'"((?:[^"\\\n]|\\.)*)"', m.group(1))
            if lits:
                return lits
    return None


def modules():
    mods = []
    for ln in (ROOT / "docs/modules.tsv").read_text().splitlines()[1:]:
        f = ln.split("\t")
        if len(f) < 7:
            continue
        mods.append({"name": f[0], "src": [s.strip() for s in f[5].split(",")],
                     "types": [t.strip().rstrip("*") for t in f[6].split(",") if t.strip()]})
    return mods


def lev(a, b):
    prev = list(range(len(b) + 1))
    for i, ca in enumerate(a, 1):
        cur = [i]
        for j, cb in enumerate(b, 1):
            cur.append(min(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + (ca != cb)))
        prev = cur
    return prev[-1]


STRIP_RE = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\'', re.S)
LIT_RE = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
LOGCTX = re.compile(r"(printf|snprintf|fprintf|puts|Report|LOG|Log|log|cout|cerr|abort|assert|ASSERT|PANIC|"
                    r"Console|console|Debug|debug|DEBUG|PORT_|Warn|warn|Error|error|trace|TRACE|OS_?Panic)")
ASSET_RE = re.compile(r"^[A-Za-z0-9_ .\-]{2,}$")
KINDS = [("sound", re.compile(r"Sfx|Sound|Stream|Audio|Snd|Music")), ("particle", re.compile(r"Particle|Effect|Gen|Elsc|Swhc|Spsc")),
         ("anim", re.compile(r"Anim|Seg|Locator|Lctr|Joint|Bone")), ("res", re.compile(r"Res|Asset|Factory|Path|Pak|File"))]


PORT_FNS = {"asm", "mpReachStub", "TraceCall", "getenv", "strcmp", "strncmp", "strcat", "Equals", "DumpFsm2Nodes",
            "__declspec", "MING_B", "MING_F", "string_l", "strstr", "strcpy", "sscanf", "fopen", "memcmp"}


def strip_comments(t):
    def f(m):
        x = m.group(0)
        return "".join("\n" if c == "\n" else " " for c in x) if x.startswith("/") else x
    return STRIP_RE.sub(f, t)


def retail_strings(blob):
    return {m.group(0).decode() for m in re.finditer(rb"[\x20-\x7e]{3,}(?=\x00)", blob)}


def rel_mode(args):
    rd = args[args.index("--retail") + 1] if "--retail" in args else str(ROOT.parent / "wt-mp2-audit5b/build/G2ME01")
    show_all = "--all" in args
    dol = (ROOT / "orig/G2ME01/sys/main.dol").read_bytes()
    mods = modules()
    tri = [q for q in (ROOT / "orig/R3ME01/files/rs5mp2_p.dol", ROOT.parent / "MetroidPrime2Port/orig/R3ME01/files/rs5mp2_p.dol")
           if q.exists()]
    tri = tri[0].read_bytes() if tri else b""
    src2mod, stem2mod = {}, {}
    for m in mods:
        for s_ in m["src"]:
            src2mod[s_] = m
            stem2mod[pathlib.Path(s_).stem] = m
    blobs = {}

    def blob(m):
        if m["name"] not in blobs:
            f = pathlib.Path(rd) / m["name"] / (m["name"] + ".rel")
            blobs[m["name"]] = f.read_bytes() if f.exists() else None
        return blobs[m["name"]]
    allrel = None
    cands = {}
    miss, port_only, nlit, nret = [], [], 0, 0
    nomod = {}
    built = set(re.findall(r"^\s*((?:src|platform)/\S+\.cpp)", (ROOT / "files.cmake").read_text(), re.M))
    files = sorted(f for f in list((ROOT / "src").rglob("*.cpp")) + list((ROOT / "platform").rglob("*.cpp"))
                   if str(f.relative_to(ROOT)) in built)
    for f in files:
        rel = str(f.relative_to(ROOT))
        stem = f.stem
        mod = src2mod.get(rel) or stem2mod.get(re.sub(r"^Port|PortStub$", "", stem))
        if mod is None and stem.startswith("Port") is False and stem.endswith("PortStub"):
            mod = stem2mod.get(stem[:-8])
        texts = [(rel, f.read_text(errors="replace"))]
        if f.is_relative_to(ROOT / "src"):
            hp = ROOT / "include" / f.relative_to(ROOT / "src").with_suffix(".hpp")
            if hp.exists():
                texts.append((str(hp.relative_to(ROOT)), hp.read_text(errors="replace")))
        if mod is not None and blob(mod) is None:
            nomod[mod["name"]] = nomod.get(mod["name"], 0) + 1
            continue
        for path, text in texts:
            text = strip_comments(text)
            for ln_no, ln in enumerate(text.split("\n"), 1):
                if ln.lstrip().startswith("#include") or ln.lstrip().startswith("#error"):
                    continue
                for m in LIT_RE.finditer(ln):
                    lit = m.group(1)
                    if not lit or "\\" in lit and "\\n" in lit and not ASSET_RE.match(lit):
                        pass
                    if len(lit) < 2 or lit in ("C", "rb", "wb", "r", "w", "rt"):
                        continue
                    nlit += 1
                    b = lit.encode("latin-1", "replace") + b"\0"
                    wb = b"".join(b"\0" + bytes([c]) for c in b[:-1]) + b"\0\0"
                    if mod is not None:
                        where, data = mod["name"] + ".rel", blob(mod)
                        hit = b in data or b in dol or wb in data or wb in dol
                    else:
                        where, data = "main.dol", dol
                        if allrel is None:
                            allrel = b"\0".join(x for x in (blob(mm) for mm in mods) if x)
                        hit = b in dol or b in allrel or wb in dol
                        if b in allrel and b not in dol:
                            where = "any REL"
                    if hit:
                        nret += 1
                        if show_all:
                            print(f"| in-retail | {path}:{ln_no} | `{lit}` | {where} |")
                        continue
                    pre = ln[:m.start()]
                    cm = re.findall(r"(\w+)\s*\(", pre)
                    fn = cm[-1] if cm else ""
                    po = None
                    if lit.startswith(("_Z", ".")) or fn in PORT_FNS:
                        po = "port token (%s)" % (fn or lit[:2])
                    elif b in tri:
                        po = "Trilogy-only string (found in rs5mp2_p.dol)"
                    elif re.match(r"^[a-z][a-z0-9]*(?:[. ][a-z0-9]+)*$", lit) or re.match(r"^x[0-9a-f]{3}$", lit):
                        po = "lowercase debug/dump token"
                    elif len(lit) < 4 and not any(ch.isupper() for ch in lit):
                        po = "short lowercase token"
                    elif not ASSET_RE.match(lit):
                        po = "non-identifier text (format/path/message)"
                    elif path.startswith("platform/") or "Port" in pathlib.Path(path).name and False:
                        po = "platform/ code"
                    elif LOGCTX.search(fn) or LOGCTX.search(pre):
                        po = "log/debug call"
                    elif " " in lit and len(lit) > 24:
                        po = "message text"
                    key = (mod["name"] if mod else "DOL")
                    if key not in cands:
                        data = blob(mod) if mod else dol
                        cands[key] = sorted(retail_strings(data))
                    near = None
                    if po is None:
                        best = None
                        for c in cands[key]:
                            if len(c) < 4 or not re.match(r"^[A-Za-z][A-Za-z0-9_ .\-]*$", c):
                                continue
                            if abs(len(c) - len(lit)) > 4:
                                continue
                            d = lev(lit.lower(), c.lower())
                            if best is None or d < best[0]:
                                best = (d, c)
                                if d <= 1:
                                    break
                        near = best
                    kind = "other"
                    for k, rx in KINDS:
                        if rx.search(fn):
                            kind = k
                            break
                    (port_only if po else miss).append((path, ln_no, lit, fn, kind, mod["name"] if mod else "(DOL/unmapped)", po, near))
    print(f"# literals={nlit} in_retail={nret} port_only={len(port_only)} findings={len(miss)} "
          f"skipped_modules_without_rel={nomod}")
    for k, (path, ln, lit, fn, kind, mn, po, near) in enumerate(miss, 5):
        nr = f"nearest retail `{near[1]}` (distance {near[0]})" if near else "no retail string within length +-4"
        print(f"| A1-{k} | {fn or '-'}({kind}) | {path}:{ln} | `{lit}` | module {mn}: absent as NUL-terminated string; {nr} | open |")
    print("# port-only (not in retail, classified by context):")
    for path, ln, lit, fn, kind, mn, po, near in port_only:
        print(f"#   {path}:{ln} {lit!r} [{po}]")


def main():
    args = sys.argv[1:]
    cache = args[args.index("--cache") + 1] if "--cache" in args else None
    if "--rel" in args:
        return rel_mode(args)
    iso, paks, ancs, cinfs = load_disc()
    print(f"# disc: {len(ancs)} ANCS, {len(cinfs)} CINF", file=sys.stderr)
    if cache and pathlib.Path(cache).exists():
        place = json.load(open(cache))
    else:
        place = scan_placements(paks, set(ancs))
        if cache:
            json.dump(place, open(cache, "w"))
    allcinf = {n for nm, _ in cinfs.values() for n in nm}
    alleff = {e for cs in ancs.values() for c in cs for e in c["effects"]}
    allanim = {a for cs in ancs.values() for c in cs for a in c["anims"]}
    glob = {"loc": allcinf, "fx": alleff, "anim": allanim}
    src2mod = {}
    for m in modules():
        ids = sorted({a for t in m["types"] for a in place.get(t, [])})
        m["ancs"] = ids
        for s in m["src"]:
            src2mod[s] = m
    unchecked, rows = [], []
    cnt = {"chk": 0, "hit": 0}

    def check(rel, line, fn, name, kind):
        lname = name.lower()
        mod = src2mod.get(rel)
        if mod is None:
            cnt["chk"] += 1
            if lname in glob[kind]:
                cnt["hit"] += 1
            else:
                rows.append((rel, line, fn, name, kind, "absent from every ANCS/CINF on the disc; "
                             "file not in modules.tsv (unattributed)", "suspected"))
            return
        chars = [c for a in mod["ancs"] for c in ancs.get(a, [])]
        if not chars:
            unchecked.append((rel, line, fn, f"{name}: module {mod['name']} has no placement/ANCS"))
            if lname not in glob[kind]:
                rows.append((rel, line, fn, name, kind, "absent disc-wide; module %s has no placement"
                             % mod["name"], "suspected"))
            return
        cnt["chk"] += 1
        ok = False
        for c in chars:
            if kind == "loc":
                ok = ok or lname in cinfs.get(c["cinf"], ({}, 0))[0]
            elif kind == "fx":
                ok = ok or lname in c["effects"]
            else:
                ok = ok or lname in c["anims"]
        if ok:
            cnt["hit"] += 1
            return
        disc = lname in glob[kind]
        ev = ("absent from the %d characters of %d ANCS of %s and from the whole disc" if not disc
              else "absent from the %d characters of %d ANCS of %s (exists on another actor)") % (
                  len(chars), len(mod["ancs"]), mod["name"])
        rows.append((rel, line, fn, name, kind, ev, "confirmed" if not disc else "suspected"))

    files = sorted(list((ROOT / "src").rglob("*.cpp")) + list((ROOT / "platform").rglob("*.cpp")))
    for f in files:
        rel = str(f.relative_to(ROOT))
        text = f.read_text(errors="replace")
        texts = None
        for m in CALL_RE.finditer(text):
            fn, arg = m.group(1), m.group(2)
            line = text.count("\n", 0, m.start()) + 1
            kind = CALLS[fn]
            if arg.startswith('"'):
                names = [arg[1:-1]]
            else:
                if texts is None:
                    hp = ROOT / "include" / f.relative_to(ROOT / "src").with_suffix(".hpp") \
                        if f.is_relative_to(ROOT / "src") else None
                    texts = [text] + ([hp.read_text(errors="replace")] if hp and hp.exists() else [])
                ident = re.match(r"\w+", arg.strip())
                names = const_literals(ident.group(0), texts) if ident else None
                if not names:
                    unchecked.append((rel, line, fn, "runtime name `%s`" % arg.strip()[:40]))
                    continue
            for name in names:
                check(rel, line, fn, name, kind)
    nchk, hits = cnt["chk"], cnt["hit"]
    order = {"crash": 0, "gameplay": 1, "visual": 2, "minor": 3}
    rows.sort(key=lambda r: (order[FALLBACK[r[4]][0]], r[0], r[1]))
    for k, (rel, line, fn, name, kind, ev, st) in enumerate(rows, 1):
        sev, fb = FALLBACK[kind]
        print(f"| A1-{k} | {fn} | {rel}:{line} | `{name}` | {sev} | {ev}; {fb} [{st}] | open |")
    print(f"\n# checked={nchk} hits={hits} missed={len(rows)} unchecked={len(unchecked)}")
    print("# unchecked:")
    for rel, line, fn, why in unchecked:
        print(f"#   {rel}:{line} {fn} {why}")


if __name__ == "__main__":
    main()
