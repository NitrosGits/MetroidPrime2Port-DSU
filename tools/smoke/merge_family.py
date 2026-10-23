#!/usr/bin/env python3
"""Carry the smoke families a branch adds into the working tree's run.sh and compare.py.

    tools/smoke/merge_family.py <branch>

Smoke-family branches all append to the same table lines in tools/smoke/run.sh and compare.py, and
lanes working in parallel pick the same next console port, so a plain merge conflicts on every one.
This reads the families <branch> adds relative to its merge base with HEAD (keys new in each
`declare -A` table, names new in ALL and FAMILIES), appends them to the working tree's copies and
gives each new family the next free port in P. Changed values of families HEAD already has (a lane
retuning its own family) are carried too. The branch's comment edits are not: re-add those by hand.
Run it during `git merge --no-commit <branch>` after `git checkout --ours` on the two files (or on a
clean tree, then commit), then `git add` them.
"""
import re
import subprocess
import sys

RUN = "tools/smoke/run.sh"
CMP = "tools/smoke/compare.py"
DECL = re.compile(r"^declare -A (\w+)=\((.*?)\)\s*$", re.S | re.M)
ENTRY = re.compile(r'\[([\w-]+)\]=("[^"]*"|\S+?)(?=\s|$)')
ALL_RE = re.compile(r"^ALL=\((.*?)\)", re.S | re.M)
FAM_RE = re.compile(r"^FAMILIES = \[(.*?)\]", re.S | re.M)


def git(*a):
    return subprocess.run(["git", *a], check=True, capture_output=True, text=True).stdout


def tables(text):
    return {m.group(1): dict(ENTRY.findall(m.group(2))) for m in DECL.finditer(text)}


def names(regex, text, quoted=False):
    body = regex.search(text).group(1)
    return re.findall(r'"([^"]+)"', body) if quoted else body.split()


def wrap(head, items, indent, close, width=100):
    lines, cur = [], head
    for it in items:
        if len(cur) + 1 + len(it) > width and cur.strip() != head.strip():
            lines.append(cur)
            cur = indent + it
        else:
            cur = cur + ("" if cur == head else " ") + it
    return "\n".join(lines + [cur + close])


def main():
    br = sys.argv[1]
    base = git("merge-base", "HEAD", br).strip()
    b_run, n_run = git("show", f"{base}:{RUN}"), git("show", f"{br}:{RUN}")
    b_cmp, n_cmp = git("show", f"{base}:{CMP}"), git("show", f"{br}:{CMP}")
    cur = open(RUN).read()
    ct, bt, nt = tables(cur), tables(b_run), tables(n_run)
    cur_all = names(ALL_RE, cur)
    new_fams = [f for f in names(ALL_RE, n_run) if f not in names(ALL_RE, b_run) and f not in cur_all]
    ports = {int(v) for v in ct["P"].values()}
    for name, ntab in nt.items():
        tab = ct.setdefault(name, {})
        for k, v in ntab.items():
            if bt.get(name, {}).get(k) == v:
                continue
            if name == "P" and k in new_fams:
                v = str(max(ports) + 1)
                ports.add(int(v))
            tab[k] = v

    def redecl(m):
        name = m.group(1)
        items = [f"[{k}]={v}" for k, v in ct[name].items()]
        return wrap(f"declare -A {name}=( ", items, " " * 15, " )")

    cur = DECL.sub(redecl, cur)
    cur = ALL_RE.sub(lambda m: wrap("ALL=(", cur_all + new_fams, "     ", ")"), cur, count=1)
    open(RUN, "w").write(cur)

    c = open(CMP).read()
    cf = names(FAM_RE, c, True)
    add = [f for f in names(FAM_RE, n_cmp, True) if f not in names(FAM_RE, b_cmp, True) and f not in cf]
    c = FAM_RE.sub(lambda m: wrap("FAMILIES = [", [f'"{f}",' for f in cf + add], " " * 12, "")
                   .rstrip(",") + "]", c, count=1)
    open(CMP, "w").write(c)
    print(f"{br}: added {' '.join(new_fams) or '-'} ports "
          + " ".join(f"{f}={ct['P'].get(f)}" for f in new_fams))


if __name__ == "__main__":
    main()
