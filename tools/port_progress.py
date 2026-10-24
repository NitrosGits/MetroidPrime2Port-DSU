#!/usr/bin/env python3
"""Count the REL functions the native port covers, for the throughput record.

A ported function is marked on the line above its definition with
    // @ fn_<module id>_<hex .text offset>
e.g. `// @ fn_15_C8D8` (module 15 = ElitePirate; the id and offset are the ones in
config/G2ME01/rels/<Module>/symbols.txt, which is also how unnamed functions are spelled). Put the
marker only on code that has been checked against the retail behaviour; a stand-in that merely logs or
returns a constant is not progress and does not get one. Named symbols (RELMain, ...) use the same
spelling with their offset: `// @ fn_15_128`. A function may carry several markers, one per retail
function the body replaces.

The `B` column is the behaviour progress of the modules whose analysis table says what each
function is: B = behaviour the port has to reproduce, H = a host copy (the host's base class
provides it), T = dead or table-only (a vtable slot, unreferenced code, the REL runtime). Only the
B rows are work, so only they are counted; see docs/module-analysis/<Module>_functions.tsv.

Usage: tools/port_progress.py [--record]   (--record appends a dated row to docs/progress.tsv)
"""
import argparse
import csv
import datetime
import json
import pathlib
import re
import sys

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(HERE))
import rel_manifest as rm  # noqa: E402

SOURCES = [ROOT / "src", ROOT / "platform", ROOT / "include"]
RECORD = ROOT / "docs/progress.tsv"
ANALYSIS = ROOT / "docs/module-analysis"
MARK_RE = re.compile(r"//\s*@\s*fn_(\d+)_([0-9A-Fa-f]+)\b")
FN_RE = re.compile(r"fn_(\d+)_([0-9A-Fa-f]+)\b")

# What the port has to do with a function, read from a module's analysis table:
# B = behaviour (the port reproduces it), H = host copy (the host's base class provides it),
# T = dead or table-only (a vtable slot, unreferenced code, the REL runtime).
CLASS_MAP = {
    "b": "B", "behaviour": "B", "behavior": "B",
    "h": "H", "host-copy": "H",
    "t": "T", "vtable": "T", "unreferenced": "T", "rel-runtime": "T", "rel-entry": "T",
}


def behaviour_class(value):
    """Map one class/kind cell to B (behaviour), H (host copy) or T (dead/table-only)."""
    v = (value or "").strip().lower()
    if v in CLASS_MAP:
        return CLASS_MAP[v]
    if v.startswith("vtable"):        # vtable, vtable-override
        return "T"
    if v.startswith("host-copy"):     # host-copy of a base class
        return "H"
    if v.startswith("rel-"):          # rel-runtime, rel-entry
        return "T"
    return "B"                        # anything else counts as behaviour


def row_offset(row):
    """The function's .text offset: the `offset` column (hex) or, without one, the `fn` column."""
    cell = (row.get("offset") or "").strip()
    if cell:
        try:
            return int(cell, 16)
        except ValueError:
            return None
    m = FN_RE.match((row.get("fn") or "").strip())
    return int(m.group(2), 16) if m else None


def module_analysis(mods, ids, marks):
    """module -> (behaviour marked, behaviour total), from docs/module-analysis/<M>_functions.tsv.

    A table names its module by file name, which does not always match the REL module
    (BoostGuardian_functions.tsv is IngBoostBallGuardian); the `fn` column then gives the module
    id (fn_30_0 -> module 30). The class column says what each function is; when a table has no
    class column its kind column is used instead.
    """
    by_id = {v: k for k, v in ids.items()}
    out = {}
    for tsv in sorted(ANALYSIS.glob("*_functions.tsv")):
        with tsv.open() as f:
            rd = csv.DictReader(f, delimiter="\t")
            rows = list(rd)
            hdr = rd.fieldnames or []
        name = tsv.name[:-len("_functions.tsv")]
        mod = name if name in ids else None
        if mod is None:
            for r in rows:
                m = FN_RE.match((r.get("fn") or "").strip())
                if m and int(m.group(1)) in by_id:
                    mod = by_id[int(m.group(1))]
                    break
        if mod is None:
            continue
        col = "class" if "class" in hdr else "kind"
        mid = ids[mod]
        total = marked = 0
        for r in rows:
            if behaviour_class(r.get(col)) != "B":
                off = row_offset(r)
                if off is not None:
                    COPIES.setdefault(mid, set()).add(off)
                continue
            total += 1
            off = row_offset(r)
            if off is not None and (mid, off) in marks:
                marked += 1
        out[mod] = (marked, total)
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


# module id -> offsets of its H/T rows, filled by module_analysis
COPIES = {}


def matched_copies(report):
    """(H/T rows objdiff reports 100%, all H/T rows). A host copy or runtime body that compiles to
    retail's bytes is the retail code, so it needs no coverage run of its own."""
    total = sum(len(v) for v in COPIES.values())
    if not report.exists():
        return None, total
    hit = 0
    bases = unit_text_bases()
    for u in json.load(report.open())["units"]:
        mid = (u.get("metadata") or {}).get("module_id")
        for f in u.get("functions", []) if mid in COPIES else ():
            if f.get("fuzzy_match_percent", 0) >= 100 and unit_base(bases, u) + int(f["address"]) in COPIES[mid]:
                hit += 1
    return hit, total


def markers():
    found = {}
    for base in SOURCES:
        for path in sorted(base.rglob("*")):
            if path.suffix not in (".c", ".cpp", ".h", ".hpp", ".inc"):
                continue
            for m in MARK_RE.finditer(path.read_text(errors="replace")):
                found.setdefault((int(m.group(1)), int(m.group(2), 16)), path)
    return found


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--record", action="store_true")
    ap.add_argument("--all", action="store_true", help="list modules with no markers too")
    args = ap.parse_args()

    marks = markers()
    mods = sorted(p.name for p in rm.RELS.iterdir() if p.is_dir())
    ids = {m: rm.module_id(m) for m in mods}
    by_id = {v: k for k, v in ids.items()}
    srcs = rm.module_sources()
    analysis = module_analysis(mods, ids, marks)

    rows, unknown = [], []
    for mod in mods:
        funcs = dict(rm.read_symbols(mod))
        cov = [o for (i, o) in marks if i == ids[mod] and o in funcs]
        unknown += [f"fn_{i}_{o:X}" for (i, o) in marks
                    if i == ids[mod] and o not in funcs]
        rows.append((mod, rm.status_of(srcs.get(mod, [])), len(cov), len(funcs),
                     sum(funcs[o] for o in cov), sum(funcs.values()),
                     *analysis.get(mod, (None, None))))
    unknown += [f"fn_{i}_{o:X}" for (i, o) in marks if i not in by_id]
    for u in sorted(unknown):
        print(f"warning: {u} is not a function in any module's symbols.txt", file=sys.stderr)

    print(f"{'module':26s} {'status':10s} {'fns':>11s} {'bytes':>15s} {'B':>11s}")
    for mod, st, cf, tf, cb, tb, bm, bt in rows:
        if cf or args.all:
            b = f"{bm:5d}/{bt:<5d}" if bm is not None else f"{'-':>11s}"
            print(f"{mod:26s} {st:10s} {cf:5d}/{tf:<5d} {cb:7d}/{tb:<7d} {b}")
    cf, tf = sum(r[2] for r in rows), sum(r[3] for r in rows)
    cb, tb = sum(r[4] for r in rows), sum(r[5] for r in rows)
    bsel = [r for r in rows if r[6] is not None]
    bm, bt = sum(r[6] for r in bsel), sum(r[7] for r in bsel)
    print(f"{'TOTAL':26s} {'':10s} {cf:5d}/{tf:<5d} {cb:7d}/{tb:<7d} {bm:5d}/{bt:<5d}  "
          f"({100 * cf / tf:.2f}% fns, {100 * cb / tb:.2f}% bytes, "
          f"{100 * bm / bt if bt else 0:.2f}% behaviour, {len(marks)} markers)")
    hm, ht = matched_copies(ROOT / "build/G2ME01/report.json")
    if hm is not None:
        print(f"  copies (H/T rows at objdiff 100%) {hm}/{ht}; behaviour + copies "
              f"{bm + hm}/{bt + ht} ({100 * (bm + hm) / (bt + ht):.2f}%)")
    for st in ("upstream", "port-body", "entry-stub"):
        sel = [r for r in rows if r[1] == st]
        print(f"  {st:10s} {len(sel):3d} modules  covered {sum(r[2] for r in sel)}/{sum(r[3] for r in sel)} fns")

    if args.record:
        new = not RECORD.exists()
        with RECORD.open("a") as f:
            if new:
                f.write("date\tcovered_fns\ttotal_fns\tcovered_bytes\ttotal_bytes\tmodules_touched\n")
            touched = ",".join(f"{r[0]}:{r[2]}" for r in rows if r[2])
            f.write(f"{datetime.date.today()}\t{cf}\t{tf}\t{cb}\t{tb}\t{touched or '-'}\n")
        print(f"recorded -> {RECORD.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
