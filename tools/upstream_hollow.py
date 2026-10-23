#!/usr/bin/env python3
"""Find hollow upstream bodies: functions the port compiles from PrimeDecomp's sources as-is
(docs/modules.tsv status 'upstream', optionally the DOL units files.cmake builds) whose
definition is a skeleton of the retail function, or which upstream does not define at all.

    python3 configure.py && $T/ninja           # once: build/G2ME01/report.json + the .o files
    tools/upstream_hollow.py [--dol] [--md OUT.md] [--min 0x40] [--binary <port binary>]
                             [--markers-from <lane worktree>] [--top N] [Module...]

Inputs (all from the matching build and the tree, nothing is run):
  build/G2ME01/report.json   objdiff: every retail function (name, size, offset) per unit, and
                             its fuzzy match % against upstream's object (None = upstream's
                             object has no symbol of that name)
  build/G2ME01/src/**.o      upstream's compiled object: the size of each compiled body
  the unit's source file     file:line and the text of the definition
  src/**                     `// @ fn_<mod>_<off>` markers (a port body exists: not a hole)
  src/MetroidPrime/PortLinkStubs.cpp   reach stubs (the port resolves the symbol to a stub)
  the port binary (nm -C)    whether a missing function is defined anywhere in the port
  /tmp/opencode/reports/up1.md, up2.md   module coverage of the upstream survey (if present)

A retail function of at least --min bytes (default 0x40) is flagged:
  EMPTY     the source body has no statement, or only `return <constant/member>;`, or the
            compiled body is <= 8 bytes
  TINY      the compiled body is < 25% of the retail size
  LOWMATCH  fuzzy match < 10% with a compiled body >= 25% of retail (different code; weaker)
  STUB      upstream does not define it and the port resolves it to a PortLinkStubs reach stub
  MISSING   upstream does not define it and the port binary has no definition of that name
  UNPAIRED  an unnamed retail function (fn_<mod>_<off>) with no upstream symbol, after pairing
            the unit's unnamed retail functions with upstream's extra symbols (the ones retail's
            symbol list does not name: rstl/template copies, loader ctors) by size within 12.5%,
            and dropped when nothing in build/G2ME01/<Module>/asm references it (dead) or, to a
            fixpoint, every referrer is a real upstream body (fuzzy >= 30) or a cleared fn_:
            upstream reproduces it inline there (SLdr* loader ctors mostly). A vtable/pointer
            reference keeps it.
Constructors and destructors are judged by compiled size only (an empty body is normal there);
implicit/template functions (ctors, dtors, operator=, anything templated or in rstl::) that the
port binary lacks are not flagged MISSING: the host compiler inlines or names them differently.
The compiled size is the GameCube side; the port compiles the TARGET_PC side, so a flagged body
whose TARGET_PC text differs and is not trivial is the port's own (listed at the end, not
flagged). A flag is also cleared by a `// @ fn_` marker for that function (--markers-from adds
a lane worktree's markers). Score = missing retail bytes (retail - compiled; half the size for
UNPAIRED, a tenth for LOWMATCH), x0.25 for modules no shipped area spawns.
"""
import argparse
import json
import os
import pathlib
import re
import struct
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
BUILD = ROOT / "build" / "G2ME01"
REPORTS = pathlib.Path("/tmp/opencode/reports")


def elf_sizes(path):
    """{symbol: size} of the FUNC symbols in a big-endian ELF32 relocatable."""
    try:
        d = path.read_bytes()
    except OSError:
        return None
    if d[:4] != b"\x7fELF":
        return None
    shoff, = struct.unpack_from(">I", d, 0x20)
    shentsize, shnum = struct.unpack_from(">HH", d, 0x2E)
    secs = [struct.unpack_from(">IIIIIIIIII", d, shoff + i * shentsize) for i in range(shnum)]
    out = {}
    for s in secs:
        if s[1] != 2:  # SHT_SYMTAB
            continue
        strtab = secs[s[6]]
        for off in range(s[4], s[4] + s[5], 16):
            name, value, size, info, other, shndx = struct.unpack_from(">IIIBBH", d, off)
            if info & 0xF != 2:  # STT_FUNC
                continue
            st = strtab[4] + name
            n = d[st:d.index(b"\0", st)].decode("latin-1")
            out[n] = max(out.get(n, 0), size)
    return out


def qual_name(demangled):
    """'A::B<x>::f(int) const' -> 'A::B<x>::f' (the part before the argument list)."""
    depth = 0
    for i, c in enumerate(demangled):
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
        elif c == "(" and depth == 0:
            return demangled[:i].strip()
    return demangled.strip()


class Source:
    """Definitions of one source file: qualified name -> [(line, body text)]."""

    def __init__(self, path, pc=None):
        self.path = path
        try:
            self.text = path.read_text(errors="replace")
        except OSError:
            self.text = ""
        code = strip_comments_keep_lines(self.text)
        self.code = code if pc is None else select_target(code, pc)

    def find(self, qname, nargs):
        parts = qname.split("::")
        if len(parts) < 2:
            pat = r"(?<![\w:~.>])" + re.escape(parts[-1]) + r"\s*\("
        else:
            cls = re.sub(r"<.*>", "", parts[-2])
            meth = parts[-1]
            pat = re.escape(cls) + r"\s*(?:<[^{};]*?>)?\s*::\s*" + re.escape(meth) + r"\s*\("
        best = None
        for m in re.finditer(pat, self.code):
            body = self._body(m.end() - 1)
            if body is None:
                continue
            line = self.code.count("\n", 0, m.start()) + 1
            cand = (line, body[0], body[1])
            if best is None or body[1] == nargs:
                best = cand
                if body[1] == nargs:
                    break
        return best

    def _body(self, lp):
        c = self.code
        depth, i = 0, lp
        while i < len(c):
            if c[i] == "(":
                depth += 1
            elif c[i] == ")":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        args = c[lp + 1:i].strip()
        nargs = 0 if args in ("", "void") else args.count(",") + 1 - len(re.findall(r"<[^<>]*,[^<>]*>", args))
        j = i + 1
        # skip qualifiers and a ctor initialiser list up to the body's '{'
        while j < len(c) and c[j] not in "{;":
            if c[j] == "(":
                d2 = 0
                while j < len(c):
                    if c[j] == "(":
                        d2 += 1
                    elif c[j] == ")":
                        d2 -= 1
                        if d2 == 0:
                            break
                    j += 1
            j += 1
        if j >= len(c) or c[j] != "{":
            return None
        depth, k = 0, j
        while k < len(c):
            if c[k] == "{":
                depth += 1
            elif c[k] == "}":
                depth -= 1
                if depth == 0:
                    return c[j + 1:k], nargs
            k += 1
        return None


def select_target(code, pc):
    """The text the PC (pc=True) or the GameCube (pc=False) build compiles: lines in the other
    side of an `#ifdef TARGET_PC` / `#ifndef TARGET_PC` / `#if [!]defined(TARGET_PC)` are blanked,
    other conditionals are kept as they are."""
    out, stack = [], []
    for line in code.split("\n"):
        d = line.strip()
        m = re.match(r"#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)", d)
        if m:
            kw, rest = m.group(1), m.group(2)
            if kw in ("ifdef", "ifndef", "if"):
                v = None
                if "TARGET_PC" in rest and "&&" not in rest and "||" not in rest:
                    neg = kw == "ifndef" or "!" in rest
                    v = (not neg) == pc
                stack.append(v)
            elif kw in ("else", "elif") and stack and stack[-1] is not None:
                stack[-1] = not stack[-1]
            elif kw == "endif" and stack:
                stack.pop()
            out.append("")
            continue
        out.append(line if all(v is not False for v in stack) else "")
    return "\n".join(out)


def strip_comments_keep_lines(t):
    t = re.sub(r"//[^\n]*", "", t)
    return re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), t, flags=re.S)


def trivial_body(body):
    b = re.sub(r"^\s*#.*$", "", body, flags=re.M).strip()
    if not b:
        return "empty body"
    stmts = [s.strip() for s in b.split(";") if s.strip()]
    if len(stmts) == 1 and "{" not in b:
        s = stmts[0]
        if re.fullmatch(r"return(\s+[\w.:>\-\[\]\(\)!&*+ ]*)?", s) and "(" not in s.replace("()", ""):
            return "only `%s;`" % s
    return None


def asm_refs(asmdir):
    """fn name -> set of referrers in build/G2ME01/<module>/asm: the function whose body
    branches to it, or '<data>' for a reference outside any function body (vtable, pointer)."""
    refs = {}
    for p in asmdir.rglob("*.s"):
        cur = None
        for line in p.read_text(errors="replace").splitlines():
            if line.startswith(".fn "):
                cur = line[4:].split(",")[0].strip().strip('"')
                continue
            if line.startswith(".endfn") or line.startswith(".obj ") or line.startswith(".sym "):
                if line.startswith(".endfn"):
                    cur = None
                continue
            for m in re.finditer(r"\bfn_[0-9A-F_]+\b", line):
                refs.setdefault(m.group(0), set()).add(cur or "<data>")
    return refs


def load_modules():
    rows = {}
    with open(ROOT / "docs" / "modules.tsv") as f:
        hdr = f.readline().rstrip("\n").split("\t")
        for line in f:
            r = dict(zip(hdr, line.rstrip("\n").split("\t")))
            rows[r["module"]] = r
    return rows


def load_coverage():
    """module -> spawned / line% from the up1/up2 survey reports, when present."""
    cov = {}
    p = REPORTS / "up1.md"
    if p.exists():
        for line in p.read_text().splitlines():
            c = [x.strip() for x in line.strip("|").split("|")]
            if len(c) >= 8 and c[2] in ("yes", "no") and c[0] != "module":
                cov[c[0]] = {"spawned": c[2], "line": c[5], "area": c[1]}
        txt = p.read_text()
        mm = re.search(r"\n(ChozoGhost[^\n]*?):\s*\*\*unreachable\*\*", txt)
        if mm:
            for name in re.split(r",\s*", mm.group(1)):
                cov.setdefault(name.strip(), {"spawned": "no", "line": "-", "area": "-"})["unreachable"] = True
    p = REPORTS / "up2.md"
    if p.exists():
        for line in p.read_text().splitlines():
            c = [x.strip() for x in line.strip("|").split("|")]
            if len(c) == 3 and c[0] in cov and "/" in c[2]:
                cov[c[0]]["line"] += " -> " + c[2].split("/")[0] + " (up2)"
    return cov


def port_markers(trees):
    ids = set()
    for p in (f for t in trees for f in (t / "src").rglob("*.cpp")):
        try:
            for m in re.finditer(r"//\s*@\s*(fn_\d+_[0-9A-Fa-f]+)", p.read_text(errors="replace")):
                ids.add(m.group(1).upper().replace("FN_", "fn_"))
        except OSError:
            pass
    return ids


def reach_stubs():
    out = set()
    p = ROOT / "src" / "MetroidPrime" / "PortLinkStubs.cpp"
    for m in re.finditer(r'mpReachStub\("[^"]*", "([^"]*)"\)', p.read_text()):
        out.add(qual_name(m.group(1)))
    return out


def binary_defs(path):
    if not path or not os.path.exists(path):
        return None
    r = subprocess.run(["nm", "-C", "--defined-only", path], capture_output=True, text=True)
    out = set()
    for line in r.stdout.splitlines():
        parts = line.split(" ", 2)
        if len(parts) == 3 and parts[1].lower() in "tw":
            out.add(qual_name(parts[2]))
    return out


def nargs_of(demangled):
    i = demangled.find("(")
    if i < 0:
        return -1
    a = demangled[i + 1:demangled.rfind(")")]
    if a.strip() in ("", "void"):
        return 0
    depth, n = 0, 1
    for ch in a:
        depth += ch == "<"
        depth -= ch == ">"
        n += ch == "," and depth == 0
    return n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("modules", nargs="*")
    ap.add_argument("--dol", action="store_true", help="also the DOL game units files.cmake builds")
    ap.add_argument("--min", type=lambda s: int(s, 0), default=0x40)
    ap.add_argument("--binary", default=None)
    ap.add_argument("--md", default=None, help="write the markdown table here")
    ap.add_argument("--top", type=int, default=150)
    ap.add_argument("--markers-from", action="append", default=[], metavar="TREE",
                    help="also count `// @ fn_` markers in another worktree (a lane branch)")
    args = ap.parse_args()

    rep = BUILD / "report.json"
    if not rep.exists():
        sys.exit("no build/G2ME01/report.json: run `python3 configure.py && $T/ninja` first")
    report = json.loads(rep.read_text())
    mods = load_modules()
    upstream = {m: r for m, r in mods.items() if r.get("status") == "upstream"}
    if args.modules:
        upstream = {m: r for m, r in upstream.items() if m in args.modules}
    cov = load_coverage()
    markers = port_markers([ROOT] + [pathlib.Path(t) for t in args.markers_from])
    stubs = reach_stubs()
    binpath = args.binary
    if binpath is None:
        for c in (ROOT / "build-port-link" / "metroid_prime2_port",
                  ROOT.parent / "wt-mp2-port" / "build-port-link" / "metroid_prime2_port"):
            if c.exists():
                binpath = str(c)
                break
    bindefs = binary_defs(binpath)
    built = set()
    if args.dol:
        built = set(re.findall(r"^\s+(src/\S+)", (ROOT / "files.cmake").read_text(), re.M))

    fuzzy_of = {}
    for u in report["units"]:
        for f in u.get("functions", []):
            fuzzy_of[(u.get("metadata", {}).get("module_name"), f["name"])] = f.get("fuzzy_match_percent")
    refs_cache = {}
    findings, summary, portpc = [], {}, []
    for u in report["units"]:
        md = u.get("metadata", {})
        mod = md.get("module_name")
        src = md.get("source_path")
        if not src:
            continue
        if mod in upstream:
            modid = int(upstream[mod]["id"])
            label = mod
        elif args.dol and mod == "main" and src in built and not args.modules:
            if "/Port" in src or "PortStub" in src:
                continue
            modid, label = None, "DOL:" + src.split("/")[-2]
        else:
            continue
        obj = elf_sizes(BUILD / pathlib.Path(src).with_suffix(".o"))
        if obj is None:
            obj = {}
        source = Source(ROOT / src, pc=False)
        pcsource = Source(ROOT / src, pc=True)
        rnames = {f["name"] for f in u.get("functions", [])}
        extra = sorted((sz for n, sz in obj.items() if n not in rnames), reverse=True)
        unnamed = sorted(((int(f["size"]), f) for f in u.get("functions", [])
                          if re.fullmatch(r"fn_[0-9A-Fa-f_]+|lbl_\w+", f["name"])
                          and f.get("fuzzy_match_percent") is None and f["name"] not in obj),
                         key=lambda t: -t[0])
        unpaired = set()
        for sz, f in unnamed:
            best = None
            for i, e in enumerate(extra):
                if abs(e - sz) <= sz / 8 and (best is None or abs(e - sz) < abs(extra[best] - sz)):
                    best = i
            if best is None:
                unpaired.add(f["name"])
            else:
                extra.pop(best)
        if unpaired:
            if mod not in refs_cache:
                refs_cache[mod] = asm_refs(BUILD / mod / "asm" if modid is not None else BUILD / "asm")
            refs = refs_cache[mod]
            # Drop what nothing references (dead code) and, to a fixpoint, what only real
            # upstream bodies (fuzzy >= 30) or already-cleared functions call: inlined there.
            changed = True
            while changed:
                changed = False
                for n in sorted(unpaired):
                    rf = refs.get(n, set()) - {n}
                    if not rf or all(c != "<data>" and c not in unpaired and
                                     (c.startswith("fn_") or "<" in c or (fuzzy_of.get((mod, c)) or 0) >= 30)
                                     for c in rf):
                        unpaired.discard(n)
                        changed = True
        s = summary.setdefault(label, {"retail": 0, "hollow": 0, "n": 0, "unpaired_n": 0,
                                       "unpaired_b": 0, "units": set()})
        s["units"].add(src)
        for f in u.get("functions", []):
            name, size = f["name"], int(f["size"])
            fuzzy = f.get("fuzzy_match_percent")
            dem = f.get("metadata", {}).get("demangled_name", name)
            off = int(f["metadata"].get("virtual_address", f.get("address", 0)))
            fid = ("fn_%d_%X" % (modid, off)) if modid is not None else ("fn_%08X" % off)
            s["retail"] += size
            ported = fid in markers
            q = qual_name(dem)
            parts = q.split("::")
            special = len(parts) >= 2 and (parts[-1].lstrip("~") == re.sub(r"<.*", "", parts[-2])
                                           or parts[-1].startswith("operator"))
            implicit = special or "<" in q or q.startswith("rstl::") or name.startswith("__")
            flag = why = loc = None
            if name in unpaired:
                if size < args.min:
                    continue
                s["unpaired_n"] += 1
                s["unpaired_b"] += size
                flag, why, comp = "UNPAIRED", "unnamed retail fn; no upstream symbol of its size", 0
            elif re.fullmatch(r"fn_[0-9A-Fa-f_]+|lbl_\w+", name) or size < args.min:
                continue
            else:
                comp = obj.get(name)
            if flag:
                pass
            elif fuzzy is None and comp is None:
                if q in stubs:
                    flag, why = "STUB", "no upstream definition; PortLinkStubs reach stub"
                elif bindefs is not None and q not in bindefs and not implicit:
                    flag, why = "MISSING", "no upstream definition; not in the port binary"
                else:
                    continue  # defined elsewhere (header inline, another unit, port file)
                comp = 0
            else:
                comp = comp if comp is not None else size
                d = source.find(q, nargs_of(dem))
                body = d[1] if d else None
                if d:
                    loc = "%s:%d" % (src, d[0])
                triv = trivial_body(body) if body is not None and not special else None
                if (triv and comp < size * 0.5) or comp <= 8:
                    flag, why = "EMPTY", triv or "compiled 0x%X bytes" % comp
                elif comp < size * 0.25:
                    flag, why = "TINY", "compiled 0x%X of 0x%X (%d%%)" % (comp, size, 100 * comp // size)
                elif fuzzy is not None and fuzzy < 10:
                    flag, why = "LOWMATCH", "compiled 0x%X" % comp
                else:
                    continue
                # The port compiles the TARGET_PC side: a body that differs there is the port's.
                pd = pcsource.find(q, nargs_of(dem))
                pbody = pd[1] if pd else None
                if pbody is not None and norm(pbody) != norm(body or ""):
                    if special or not trivial_body(pbody):
                        portpc.append((label, fid, dem, "%s:%d" % (src, pd[0]), size))
                        continue
                    why += "; TARGET_PC body also trivial"
            if fuzzy is not None:
                why += "; fuzzy %.1f%%" % fuzzy
            if ported:
                why += "; PORTED (marker)"
            miss = max(size - comp, 0) if flag not in ("LOWMATCH", "UNPAIRED") else size // 2
            base = label.split(":")[0] if modid is not None else label
            c = cov.get(base, {})
            weight = 0.25 if c.get("unreachable") else 1.0
            if ported:
                weight = 0
            s["n"] += 1 if not ported else 0
            s["hollow"] += miss if not ported else 0
            findings.append({"module": label, "fid": fid, "name": dem, "size": size, "comp": comp,
                             "loc": loc or src, "flag": flag, "why": why,
                             "score": (miss or size * 0.1) * weight, "ported": ported,
                             "reach": reach_str(c, modid is None)})
    findings.sort(key=lambda r: -r["score"])
    out = render(findings, summary, cov, args, binpath, portpc)
    if args.md:
        pathlib.Path(args.md).write_text(out)
    print(out)


def reach_str(c, dol=False):
    if dol:
        return "DOL (resident)"
    if not c:
        return "?"
    if c.get("unreachable"):
        return "unreachable (no placement)"
    line = c.get("line", "-").replace(" -> ", "%, up2 ").replace(" (up2)", "")
    return "%s; module lines %s%%" % ("spawned" if c.get("spawned") == "yes" else "not spawned", line)


def norm(t):
    return re.sub(r"\s+", "", t)


def render(findings, summary, cov, args, binpath, portpc):
    L = ["# Upstream hollow bodies (tools/upstream_hollow.py)", "",
         "Threshold: retail >= 0x%X bytes. Binary for MISSING: `%s`. Coverage: up1/up2 reports%s." %
         (args.min, binpath, "" if cov else " (absent)"), "",
         "## Modules (by hollow bytes)", "",
         "| module | retail bytes | flagged fns | hollow bytes (est.) | % | of which UNPAIRED fn_ (n / bytes) | reach |",
         "|---|---|---|---|---|---|---|"]
    for m, s in sorted(summary.items(), key=lambda kv: -kv[1]["hollow"]):
        if not s["n"] and not s["unpaired_b"]:
            continue
        pct = 100 * s["hollow"] // max(s["retail"], 1)
        L.append("| %s | %d | %d | %d | %d | %d / %d | %s |" % (
            m, s["retail"], s["n"], s["hollow"], pct, s["unpaired_n"], s["unpaired_b"],
            reach_str(cov.get(m, {}), m.startswith("DOL:"))))
    L += ["", "## Functions (ranked by score; top %d)" % args.top, "",
          "| # | module | function | retail id / size | compiled | upstream | flag | why | reached |",
          "|---|---|---|---|---|---|---|---|---|"]
    for i, r in enumerate([r for r in findings if not r["ported"]][:args.top], 1):
        L.append("| %d | %s | `%s` | %s / 0x%X | 0x%X | %s | %s | %s | %s |" % (
            i, r["module"], r["name"].replace("|", "\\|"), r["fid"], r["size"], r["comp"],
            r["loc"], r["flag"], r["why"], r["reach"]))
    rest = len([r for r in findings if not r["ported"]]) - args.top
    if rest > 0:
        L.append("\n(%d more below the cut; rerun with --top)" % rest)
    ported = [r for r in findings if r["ported"]]
    if ported:
        L += ["", "Flagged upstream bodies with a port marker (not holes): " +
              ", ".join("%s %s" % (r["module"], r["fid"]) for r in ported[:60])]
    if portpc:
        L += ["", "Hollow on the GameCube side but with a TARGET_PC body (the port's own; not a hole): " +
              ", ".join("%s %s %s" % (m, f, loc) for m, f, d, loc, sz in portpc)]
    return "\n".join(L) + "\n"


if __name__ == "__main__":
    main()
