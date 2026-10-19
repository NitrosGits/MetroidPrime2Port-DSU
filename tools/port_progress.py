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

Usage: tools/port_progress.py [--record]   (--record appends a dated row to docs/progress.tsv)
"""
import argparse
import datetime
import pathlib
import re
import sys

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(HERE))
import rel_manifest as rm  # noqa: E402

SOURCES = [ROOT / "src", ROOT / "platform", ROOT / "include"]
RECORD = ROOT / "docs/progress.tsv"
MARK_RE = re.compile(r"//\s*@\s*fn_(\d+)_([0-9A-Fa-f]+)\b")


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

    rows, unknown = [], []
    for mod in mods:
        funcs = dict(rm.read_symbols(mod))
        cov = [o for (i, o) in marks if i == ids[mod] and o in funcs]
        unknown += [f"fn_{i}_{o:X}" for (i, o) in marks
                    if i == ids[mod] and o not in funcs]
        rows.append((mod, rm.status_of(srcs.get(mod, [])), len(cov), len(funcs),
                     sum(funcs[o] for o in cov), sum(funcs.values())))
    unknown += [f"fn_{i}_{o:X}" for (i, o) in marks if i not in by_id]
    for u in sorted(unknown):
        print(f"warning: {u} is not a function in any module's symbols.txt", file=sys.stderr)

    print(f"{'module':26s} {'status':10s} {'fns':>11s} {'bytes':>15s}")
    for mod, st, cf, tf, cb, tb in rows:
        if cf or args.all:
            print(f"{mod:26s} {st:10s} {cf:5d}/{tf:<5d} {cb:7d}/{tb:<7d}")
    cf, tf = sum(r[2] for r in rows), sum(r[3] for r in rows)
    cb, tb = sum(r[4] for r in rows), sum(r[5] for r in rows)
    print(f"{'TOTAL':26s} {'':10s} {cf:5d}/{tf:<5d} {cb:7d}/{tb:<7d}  ({100 * cf / tf:.2f}% fns, "
          f"{100 * cb / tb:.2f}% bytes, {len(marks)} markers)")
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
