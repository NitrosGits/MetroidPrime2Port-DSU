#!/usr/bin/env python3
"""Branch coverage of the port's REL module bodies, from the gcov data of one or more runs.

The decomp lanes prove "every arm of a ported body ran" with gdb breakpoints, one spec per
function (tools/cov/run.sh). This tool answers the same question for every function of every
port module body at once, from the counters the coverage build (CMake option MP_PORT_COVERAGE)
writes: one ordinary run of the game is enough, and the arms of several runs (a natural one and
a forced one) add up.

    tools/cov/gcov_report.py <build dir> <gcov prefix dir> [<gcov prefix dir> ...]
                             [--module <Name>] [--fn fn_10_1D794 ...]

  <build dir>       a coverage build (build-port-cov): it holds the .gcno files
  <gcov prefix dir> the directory the run wrote its .gcda files to (GCOV_PREFIX); several of
                    them are merged, so the arms of several runs add up
  --module <Name>   only the bodies of that port module (the name in tools/modules_port.txt, or
                    the retail module id, e.g. 56)
  --fn <id>         only the bodies with that retail id (repeatable, or comma-separated)

One row per function: the retail id, the C++ name, the lines hit, the branches taken, the lines
whose branch was never taken, and ALL-ARMS when every branch of the body was taken.

The build must have been made with -DMP_PORT_COVERAGE=ON, and the run must have set
GCOV_PREFIX (tools/cov/run.sh and tools/smoke/run.sh do that when the binary is a coverage
build), so that concurrent runs do not write over each other's .gcda files.
"""
import argparse
import gzip
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

# fn_56_28F0: the retail module id and the retail offset of the body.
FN_RE = re.compile(r"fn_(\d+)_([0-9A-Fa-f]+)")
# The marker the lanes put above a body they have checked.
MARK_RE = re.compile(r"//\s*@\s*(fn_\d+_[0-9A-Fa-f]+)")


def log(msg):
    print(f"[gcov-report] {msg}", file=sys.stderr)


def read_cache(build):
    """The source root of a build: CMAKE_HOME_DIRECTORY out of the CMake cache."""
    cache = pathlib.Path(build) / "CMakeCache.txt"
    if cache.exists():
        for line in cache.read_text(errors="replace").splitlines():
            if line.startswith("CMAKE_HOME_DIRECTORY:"):
                return pathlib.Path(line.split("=", 1)[1].strip())
    return None


def module_sources(root):
    """{module name: [source paths]} and {source path: module name}, from tools/modules_port.txt."""
    by_module, by_source = {}, {}
    path = pathlib.Path(root) / "tools" / "modules_port.txt"
    if not path.exists():
        return by_module, by_source
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        name, _, src = line.partition(";")
        name, src = name.strip(), src.strip()
        if not name or not src:
            continue
        by_module.setdefault(name, [])
        if src not in by_module[name]:
            by_module[name].append(src)
        by_source.setdefault(src, name)
    return by_module, by_source


def coverage_sources(root):
    """Every module body source: the ones tools/modules_port.txt lists and their siblings."""
    sources = []
    for srcs in module_sources(root)[0].values():
        for src in srcs:
            p = pathlib.Path(src)
            stem = p.stem
            for sib in sorted(p.parent.glob(f"{stem}*.cpp")):
                rel = str(sib)
                if rel not in sources:
                    sources.append(rel)
    return sources


def comment_blocks(path):
    """The comment blocks of a source file: [(last line, [ids], [marked ids])].

    A body is marked either by the `// @ fn_..` marker or by a comment block directly above it
    that names the retail id. The block is what the lanes write when a body needs a note as well
    (what the marker says), so both are read the same way, and the explicit marker wins when a
    block names more than one id.
    """
    try:
        lines = pathlib.Path(path).read_text(errors="replace").splitlines()
    except OSError:
        return []
    blocks = []
    run = None
    for i, line in enumerate(lines, start=1):
        stripped = line.strip()
        if stripped.startswith("//"):
            ids = FN_RE.findall(stripped)
            marked = MARK_RE.findall(stripped)
            if run is None:
                run = [i, i, [], []]
            run[1] = i
            run[2].extend(f"fn_{a}_{b.upper()}" for a, b in ids)
            run[3].extend(marked)
        else:
            if run is not None:
                blocks.append((run[1], run[2], run[3]))
                run = None
    if run is not None:
        blocks.append((run[1], run[2], run[3]))
    return blocks


def source_lines(path):
    try:
        return pathlib.Path(path).read_text(errors="replace").splitlines()
    except OSError:
        return []


def attach_id(blocks, lines, start_line):
    """The retail id of the body that starts at `start_line`, or None.

    The block the lanes wrote for a body sits directly above it, so the block that ends on the
    line before the definition is the one that marks it. A definition whose block is a little
    above it (blank lines between) is found too, but only when the lines between are blank: a
    block inside the body above belongs to that body, not to this one.
    """
    exact = [b for b in blocks if b[0] == start_line - 1]
    if exact:
        group = exact
    else:
        group = [b for b in blocks
                 if 0 < start_line - 1 - b[0] <= 2
                 and all(not lines[i].strip() for i in range(b[0], start_line - 2))]
    if not group:
        return None
    ids = set()
    marked = []
    for _, block_ids, block_marked in group:
        ids.update(block_ids)
        marked.extend(block_marked)
    if marked:
        return marked[0]
    if len(ids) == 1:
        return next(iter(ids))
    return None



def find_gcda(prefix):
    out = []
    for path in sorted(pathlib.Path(prefix).rglob("*.gcda")):
        out.append(path)
    return out


def merge_prefixes(prefixes, work):
    """Merge the .gcda files of several runs into one directory (the counters add up)."""
    if len(prefixes) == 1:
        return pathlib.Path(prefixes[0])
    current = pathlib.Path(prefixes[0])
    for i, prefix in enumerate(prefixes[1:], start=1):
        out = work / f"merged{i}"
        out.mkdir(parents=True, exist_ok=True)
        cmd = ["gcov-tool", "merge", "-o", str(out), str(current), str(prefix)]
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
            raise SystemExit(f"gcov-tool merge failed: {res.stderr.strip()}")
        current = out
    return current


def gcno_for(build, gcda, prefix):
    """The .gcno of a .gcda: same path under the build dir (GCOV_PREFIX_STRIP keeps the layout)."""
    rel = gcda.relative_to(prefix)
    cand = pathlib.Path(build) / rel.with_suffix(".gcno")
    if cand.exists():
        return cand
    # Fall back to a search by name, for a build whose layout differs from the prefix's.
    matches = sorted(pathlib.Path(build).rglob(gcda.name.replace(".gcda", ".gcno")))
    return matches[0] if matches else None


def run_gcov(build, gcda_dirs):
    """gcov over every .gcda; returns the parsed JSON of every file it knows."""
    docs = []
    with tempfile.TemporaryDirectory(prefix="gcov-report-") as tmp:
        tmp = pathlib.Path(tmp)
        for prefix, gcdas in gcda_dirs:
            for gcda in gcdas:
                gcno = gcno_for(build, gcda, prefix)
                if gcno is None:
                    log(f"no .gcno for {gcda} (looked in {build})")
                    continue
                work = tmp / f"w{len(docs)}"
                work.mkdir()
                # gcov reads the .gcda and the .gcno from the same directory (-o), so both are
                # linked into a scratch directory: the run's own tree stays untouched.
                (work / gcda.name).symlink_to(gcda.resolve())
                (work / gcno.name).symlink_to(gcno.resolve())
                res = subprocess.run(
                    ["gcov", "-j", "-b", "-c", "-o", str(work), str(gcda)],
                    capture_output=True, text=True, cwd=str(work))
                if res.returncode != 0:
                    log(f"gcov failed on {gcda}: {res.stderr.strip()[:200]}")
                    continue
                for out in sorted(work.glob("*.gcov.json.gz")):
                    with gzip.open(out, "rt", errors="replace") as fh:
                        docs.append(json.load(fh))
    return docs


def collect(docs, sources):
    """{source path: {function name: {info}}} from the gcov JSON documents."""
    out = {}
    for doc in docs:
        for entry in doc.get("files", []):
            path = entry.get("file", "")
            rel = None
            for src in sources:
                if path == src or path.endswith("/" + src):
                    rel = src
                    break
            if rel is None:
                continue
            lines = entry.get("lines", [])
            funcs = {}
            for fn in entry.get("functions", []):
                funcs[fn["name"]] = {
                    "demangled": fn.get("demangled_name", fn["name"]),
                    "start_line": fn.get("start_line", 0),
                    "end_line": fn.get("end_line", 0),
                    "lines": [],
                }
            for line in lines:
                owner = line.get("function_name")
                if owner is None:
                    continue
                funcs.setdefault(owner, {
                    "demangled": owner,
                    "start_line": 0,
                    "end_line": 0,
                    "lines": [],
                })
                funcs[owner]["lines"].append(line)
            out.setdefault(rel, {}).update(funcs)
    return out


def function_stats(fn):
    """The lines and branches of one function, and the lines whose branch was never taken."""
    hit = total = 0
    branches = taken = 0
    untaken = []
    for line in fn["lines"]:
        total += 1
        if line.get("count", 0) > 0:
            hit += 1
        for branch in line.get("branches", []):
            branches += 1
            if branch.get("count", 0) > 0:
                taken += 1
            else:
                untaken.append(line.get("line_number", 0))
    return hit, total, branches, taken, sorted(set(untaken))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("build", help="the coverage build directory (holds the .gcno files)")
    ap.add_argument("prefix", nargs="+", help="the GCOV_PREFIX directories of the run(s)")
    ap.add_argument("--module", help="only this port module (name or retail id)")
    ap.add_argument("--fn", action="append", default=[],
                    help="only these retail ids (repeatable, or comma-separated)")
    ap.add_argument("--source-root", default=None, help="the source tree (default: the build's)")
    args = ap.parse_args()

    build = pathlib.Path(args.build).resolve()
    if not build.is_dir():
        raise SystemExit(f"no such build directory: {build}")
    root = pathlib.Path(args.source_root).resolve() if args.source_root else read_cache(build)
    if root is None or not root.is_dir():
        root = pathlib.Path(__file__).resolve().parent.parent.parent
    sources = coverage_sources(root)
    if not sources:
        raise SystemExit(f"no module body sources found under {root}")

    wanted = set()
    for item in args.fn:
        for x in item.split(","):
            x = x.strip()
            if not x:
                continue
            m = re.fullmatch(r"fn_(\d+)_([0-9A-Fa-f]+)", x)
            wanted.add(f"fn_{m.group(1)}_{m.group(2).upper()}" if m else x)

    prefixes = [pathlib.Path(p).resolve() for p in args.prefix]
    with tempfile.TemporaryDirectory(prefix="gcov-merge-") as tmp:
        merged = merge_prefixes(prefixes, pathlib.Path(tmp))
        # The merged tree keeps the layout of the prefix, so the .gcno lookup still works.
        docs = run_gcov(build, [(merged, find_gcda(merged))])

    data = collect(docs, sources)
    blocks = {src: comment_blocks(pathlib.Path(root) / src) for src in sources}
    texts = {src: source_lines(pathlib.Path(root) / src) for src in sources}
    by_source = module_sources(root)[1]

    rows = []
    for src in sorted(data):
        module = by_source.get(src, "?")
        for name, fn in sorted(data[src].items(), key=lambda kv: kv[1]["start_line"]):
            ident = attach_id(blocks.get(src, []), texts.get(src, []), fn["start_line"])
            hit, total, branches, taken, untaken = function_stats(fn)
            rows.append({
                "module": module,
                "id": ident or "",
                "name": fn["demangled"],
                "source": src,
                "start_line": fn["start_line"],
                "hit": hit,
                "lines": total,
                "branches": branches,
                "taken": taken,
                "untaken": untaken,
            })

    if args.module:
        want = args.module.strip()
        rows = [r for r in rows if r["module"].lower() == want.lower()
                or (r["id"] and r["id"].split("_")[1] == want)]
    if wanted:
        rows = [r for r in rows if r["id"] in wanted]

    print(f"{'module':<12} {'id':<14} {'function':<52} {'lines':>9} {'branches':>10}  arms")
    print("-" * 120)
    all_arms = 0
    with_arms = 0
    for r in rows:
        if r["branches"]:
            arms = "ALL-ARMS" if not r["untaken"] else \
                f"untaken @ {','.join(str(x) for x in r['untaken'])}"
        else:
            arms = "no branches"
        if r["branches"] and not r["untaken"]:
            all_arms += 1
        if r["branches"]:
            with_arms += 1
        print(f"{r['module']:<12} {r['id'] or '-':<14} {r['name'][:52]:<52} "
              f"{str(r['hit']) + '/' + str(r['lines']):>9} "
              f"{str(r['taken']) + '/' + str(r['branches']):>10}  {arms}")
    print("-" * 120)
    print(f"{len(rows)} functions, {with_arms} with branches, {all_arms} with every arm taken")
    if wanted:
        missing = wanted - {r["id"] for r in rows}
        if missing:
            print(f"not found: {', '.join(sorted(missing))}")


if __name__ == "__main__":
    main()
