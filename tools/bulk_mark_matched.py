#!/usr/bin/env python3
"""Mark REL functions whose upstream body objdiff reports as a 100% match against retail.

A body that compiles to retail's exact bytes behaves like retail by construction, so it needs no
coverage run. A function is marked only when all of these hold:
  - report.json gives it fuzzy_match_percent 100 and it has no `// @` marker yet;
  - config symbols.txt names the same symbol at that module offset;
  - its unit's source file (report metadata source_path) has exactly one definition of the
    demangled name with the same argument count;
  - that definition's body has no TARGET_PC in it and does not sit inside a TARGET_PC #if block
    (on PC such a body is not the one objdiff compared);
  - the source is not a port file (Port*.cpp, *PortStub.cpp, platform/).
The marker goes on the line above the definition's first line, like hand-placed ones.

Usage: tools/bulk_mark_matched.py [--report build/G2ME01/report.json] [--write] [--module M]
Without --write it only prints what it would mark and why the rest was skipped.
"""
import argparse
import collections
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
MARK_RE = re.compile(r"//\s*@\s*fn_(\d+)_([0-9A-Fa-f]+)\b")
SOURCES = [ROOT / "src", ROOT / "platform", ROOT / "include"]


def existing_marks():
    out = set()
    for base in SOURCES:
        for p in base.rglob("*"):
            if p.suffix in (".c", ".cpp", ".h", ".hpp", ".inc"):
                for m in MARK_RE.finditer(p.read_text(errors="replace")):
                    out.add((int(m.group(1)), int(m.group(2), 16)))
    return out


def unit_text_bases():
    """(module name, unit path without extension) -> the unit's .text start in its module, from
    config/G2ME01/rels/<M>/splits.txt. objdiff gives function addresses relative to the unit."""
    out = {}
    for sp in (ROOT / "config/G2ME01/rels").glob("*/splits.txt"):
        cur = None
        for ln in sp.read_text().splitlines():
            if ln and not ln[0].isspace() and ln.rstrip().endswith(":"):
                cur = ln.rstrip()[:-1].rsplit(".", 1)[0]
            elif cur and ln.strip().startswith(".text") and "start:" in ln:
                out[(sp.parent.name, cur)] = int(ln.split("start:")[1].split()[0], 16)
    return out


def unit_base(bases, unit):
    """objdiff unit name 'Module/path/to/unit' -> its .text start (0 when the module has one unit)."""
    mod, _, path = unit["name"].partition("/")
    return bases.get(((unit.get("metadata") or {}).get("module_name", mod), path), 0)


def split_args(s):
    """Top-level comma count of a demangled argument list."""
    s = s.strip()
    if s in ("", "void"):
        return 0
    depth, n = 0, 1
    for ch in s:
        if ch in "(<[":
            depth += 1
        elif ch in ")>]":
            depth -= 1
        elif ch == "," and depth == 0:
            n += 1
    return n


def parse_demangled(d):
    """'A::B::f(int, float) const' -> ('A::B::f', 2); None for templates and operators we skip."""
    i = d.find("(")
    if i <= 0 or "<" in d[:i] or "operator" in d[:i]:
        return None
    depth, j = 0, i
    for j in range(i, len(d)):
        if d[j] == "(":
            depth += 1
        elif d[j] == ")":
            depth -= 1
            if depth == 0:
                break
    return d[:i].strip(), split_args(d[i + 1:j])


def strip_comments(text):
    """Blank out comments and string literals, keeping offsets and newlines."""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            for k in range(i, j):
                out[k] = " "
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != "\n":
                    out[k] = " "
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            for k in range(i + 1, min(j, n)):
                if out[k] != "\n":
                    out[k] = " "
            i = j + 1
        else:
            i += 1
    return "".join(out)


def pc_regions(lines):
    """Per line: True when inside an #if/#ifdef/#ifndef block (any branch) naming TARGET_PC."""
    stack, inside = [], []
    for ln in lines:
        s = ln.strip()
        if re.match(r"#\s*if", s):
            stack.append("TARGET_PC" in s)
        elif re.match(r"#\s*endif", s):
            if stack:
                stack.pop()
        inside.append(any(stack))
    return inside


def find_defs(clean, qual, nargs):
    """Definitions of qual(...) { in comment-free text: list of (start offset, body start, body end)."""
    name = re.escape(qual)
    pat = re.compile(r"(?<![\w:~])" + name + r"\s*\(")
    found = []
    for m in pat.finditer(clean):
        # argument list
        i, depth = m.end() - 1, 0
        for j in range(i, len(clean)):
            if clean[j] == "(":
                depth += 1
            elif clean[j] == ")":
                depth -= 1
                if depth == 0:
                    break
        else:
            continue
        args = clean[i + 1:j]
        # after the arguments: optional const/initialiser list, then '{' (a definition) or ';'
        k = j + 1
        while k < len(clean) and clean[k] not in "{;":
            k += 1
        if k >= len(clean) or clean[k] != "{":
            continue
        tail = clean[j + 1:k].strip()
        if tail and not (tail == "const" or tail.startswith(":")):
            continue
        # a call inside another function would not be at statement start; require the name to
        # follow a line start, a type, '*' or '&'
        before = clean[clean.rfind("\n", 0, m.start()) + 1:m.start()]
        if re.search(r"[=(,!?]|return\s*$|\.\s*$|->\s*$", before):
            continue
        if split_args(re.sub(r"=[^,]*", "", args)) != nargs:
            continue
        # body
        depth = 0
        for e in range(k, len(clean)):
            if clean[e] == "{":
                depth += 1
            elif clean[e] == "}":
                depth -= 1
                if depth == 0:
                    break
        found.append((m.start(), k, e))
    return found


def decl_first_line(lines, idx):
    """Walk up from the name's line to the first line of the declaration (return type, template)."""
    while idx > 0:
        prev = lines[idx - 1].strip()
        if not prev or prev.endswith((";", "}", "{", "*/")) or prev.startswith(("#", "//")):
            break
        idx -= 1
    return idx


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--report", default=str(ROOT / "build/G2ME01/report.json"))
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--module")
    args = ap.parse_args()

    report = json.load(open(args.report))
    marks = existing_marks()
    bases = unit_text_bases()
    syms = {}
    for st in (ROOT / "config/G2ME01/rels").glob("*/symbols.txt"):
        for ln in st.read_text().splitlines():
            m = re.match(r"(\S+) = \.text:0x([0-9A-Fa-f]+);", ln)
            if m:
                syms[(st.parent.name, int(m.group(2), 16))] = m.group(1)
    todo = collections.defaultdict(list)  # path -> [(fid, qual, nargs)]
    skip = collections.Counter()
    for u in report["units"]:
        md = u.get("metadata") or {}
        mid, src = md.get("module_id"), md.get("source_path")
        if not mid or (args.module and md.get("module_name") != args.module):
            continue
        base = unit_base(bases, u)
        for f in u.get("functions", []):
            if f.get("fuzzy_match_percent", 0) < 100:
                continue
            off = base + int(f["address"])
            if (mid, off) in marks:
                continue
            fid = f"fn_{mid}_{off:X}"
            if syms.get((md.get("module_name"), off)) not in (f["name"], fid):
                skip["symbols.txt disagrees on offset"] += 1
                continue
            if not src:
                skip["no source"] += 1
                continue
            p = ROOT / src
            if p.name.startswith("Port") or p.name.endswith("PortStub.cpp") or "platform/" in src:
                skip["port source"] += 1
                continue
            dm = (f.get("metadata") or {}).get("demangled_name")
            parsed = parse_demangled(dm) if dm else None
            if not parsed:
                skip["template/operator/unnamed"] += 1
                continue
            todo[p].append((fid, *parsed))

    marked = 0
    for p, fns in sorted(todo.items()):
        if not p.exists():
            skip["source missing"] += len(fns)
            continue
        text = p.read_text()
        clean = strip_comments(text)
        lines = text.split("\n")
        inpc = pc_regions(lines)
        starts = [0]
        for ln in lines:
            starts.append(starts[-1] + len(ln) + 1)
        line_of = lambda off: next(i for i in range(len(starts) - 1) if starts[i + 1] > off)
        inserts = {}
        for fid, qual, nargs in fns:
            defs = find_defs(clean, qual, nargs)
            if len(defs) != 1:
                skip["no unique definition" if defs else "definition not found"] += 1
                continue
            s, b, e = defs[0]
            if "TARGET_PC" in text[b:e + 1]:
                skip["TARGET_PC in body"] += 1
                continue
            li = line_of(s)
            if inpc[li]:
                skip["inside TARGET_PC block"] += 1
                continue
            first = decl_first_line(lines, li)
            if MARK_RE.search(lines[first - 1] if first else ""):
                skip["line above already marked"] += 1
                continue
            indent = re.match(r"\s*", lines[first]).group(0)
            inserts.setdefault(first, []).append(f"{indent}// @ {fid}")
            print(f"{p.relative_to(ROOT)}:{first + 1}: {fid} {qual}")
        if inserts and args.write:
            out = []
            for i, ln in enumerate(lines):
                out += inserts.get(i, [])
                out.append(ln)
            p.write_text("\n".join(out))
        marked += sum(len(v) for v in inserts.values())

    print(f"{'marked' if args.write else 'would mark'} {marked}", file=sys.stderr)
    for k, v in skip.most_common():
        print(f"  skipped {v:5d}  {k}", file=sys.stderr)


if __name__ == "__main__":
    main()
