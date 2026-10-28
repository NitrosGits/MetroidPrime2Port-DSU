#!/usr/bin/env python3
"""Sweep 4: port edits to upstream files that change the non-PC path.

Usage: python3 tools/audit/target_pc_hygiene.py [--base upstream/main] [--head HEAD] [--rows]

For every upstream-tracked file (present in --base; src/, include/, configure.py; minus
Port*.cpp and *PortStub.cpp) the head version is reduced to its non-PC form: TARGET_PC is
treated as undefined (`#ifdef TARGET_PC` then-branches dropped, `#ifndef TARGET_PC` kept,
`#if defined(TARGET_PC)` / `!defined` handled, #else flipped) while every other directive is
kept verbatim. Comments are stripped and the result is token-diffed against the base file.
Any remaining difference is an edit outside a TARGET_PC block (or inside an #ifndef TARGET_PC
branch, which also changes the non-PC path). Output is one row per differing token run:
`| file:line | kind | detail |`, kind = harmless (only #include/forward decl/blank-ish,
or configure.py build-only) or behaviour. Read-only.
"""
import argparse
import difflib
import re
import subprocess
import sys

TOK = re.compile(r"[A-Za-z_]\w*|\d[\w.]*|\"(?:\\.|[^\"\\])*\"|'(?:\\.|[^'\\])*'|\S")


def git(*a):
    return subprocess.run(["git", *a], capture_output=True, text=True, check=True).stdout


def strip_comments(t):
    t = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), t, flags=re.S)
    return re.sub(r"//[^\n]*", "", t)


def nonpc(text):
    """Return list of (lineno, line) with TARGET_PC undefined; unrelated directives kept."""
    stack = []  # [kind(pc|npc|other), active_now]
    out = []
    for n, line in enumerate(text.split("\n"), 1):
        m = re.match(r"\s*#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)", line)
        emit = True
        if m:
            d, rest = m.groups()
            has = re.search(r"\bTARGET_PC\b", rest) is not None
            if d in ("ifdef", "ifndef", "if") and has:
                neg = d == "ifndef" or (d == "if" and re.search(r"!\s*(defined\b)?\s*\(?\s*TARGET_PC", rest))
                stack.append(["npc" if neg else "pc", neg])
                emit = False
            elif d in ("ifdef", "ifndef", "if"):
                stack.append(["other", True])
            elif d in ("else", "elif") and stack and stack[-1][0] != "other":
                stack[-1][1] = not stack[-1][1]
                emit = False
            elif d == "endif" and stack:
                emit = stack[-1][0] == "other"
                stack.pop()
        if emit and all(f[1] for f in stack if f[0] != "other"):
            out.append((n, line))
    return out


def toks(lines):
    res = []
    text = strip_comments("\n".join(l for _, l in lines))
    ln_of = [n for n, _ in lines]
    for i, l in enumerate(text.split("\n")):
        for t in TOK.findall(l):
            res.append((t, ln_of[i] if i < len(ln_of) else ln_of[-1]))
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", default="upstream/main")
    ap.add_argument("--head", default="HEAD")
    a = ap.parse_args()
    names = git("diff", "--name-status", "--diff-filter=M", a.base, a.head, "--",
                "src", "include", "configure.py").splitlines()
    rows = []
    for l in names:
        path = l.split("\t", 1)[1]
        b = path.rsplit("/", 1)[-1]
        if b.startswith("Port") or b.endswith("PortStub.cpp"):
            continue
        old = toks(nonpc(git("show", f"{a.base}:{path}")))
        new = toks(nonpc(git("show", f"{a.head}:{path}")))
        ot, nt = [t for t, _ in old], [t for t, _ in new]
        sm = difflib.SequenceMatcher(None, ot, nt, autojunk=False)
        for op, i1, i2, j1, j2 in sm.get_opcodes():
            if op == "equal":
                continue
            added, removed = " ".join(nt[j1:j2]), " ".join(ot[i1:i2])
            ln = new[j1][1] if j1 < len(new) else (new[-1][1] if new else 0)
            if path == "configure.py":
                kind = "harmless"
            elif re.fullmatch(r"(#\s*include\s*(<[^>]*>|\"[^\"]*\")\s*|class \w+ ;\s*)*", " ".join(nt[j1:j2]).replace("# include", "#include")) and not removed:
                kind = "harmless"
            else:
                kind = "behaviour"
            rows.append((path, ln, kind, f"+[{added[:90]}] -[{removed[:90]}]"))
    for p, ln, k, d in rows:
        print(f"| {p}:{ln} | outside-block | {k} | {d} |")
    c = {}
    for r in rows:
        c[r[2]] = c.get(r[2], 0) + 1
    print("diff runs:", len(rows), c, "files:", len({r[0] for r in rows}), file=sys.stderr)


if __name__ == "__main__":
    main()
