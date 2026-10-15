#!/usr/bin/env python3
"""What is left in the port's link stubs, and which of it can be written today.

`src/MetroidPrime/PortLinkStubs.cpp` accumulates every symbol a link once asked
for, so it mixes three very different things:

  * entries a compiled object still references - the real missing-code backlog;
  * entries nothing references any more - leftovers from older links, deletable;
  * entries whose retail asm *is* in `build/G2ME01/asm`, because a claimed unit's
    asm covers the functions the source has not written yet. Those can be written
    from the disassembly instead of from scratch, and the listing gives their size.

A stub with no asm belongs to a unit nobody has claimed, whose body is only in the
retail REL/DOL images (and possibly unnamed there) - that is the expensive half.

usage:
  tools/stub_backlog.py [build-dir] [repo]     # defaults: build-port-link .
"""
import os
import re
import subprocess
import sys
from collections import defaultdict

build = sys.argv[1] if len(sys.argv) > 1 else "build-port-link"
repo = sys.argv[2] if len(sys.argv) > 2 else "."

stub_file = os.path.join(repo, "src/MetroidPrime/PortLinkStubs.cpp")
asm_root = os.path.join(repo, "build/G2ME01/asm")
text = open(stub_file).read()

# Each stub is an asm() alias, usually with the mpReachStub call beside it carrying the
# demangled name; the zeroed data stubs have no call, so they keep their symbol name.
stubs = {}
for m in re.finditer(r'asm\("([^"]+)"\)', text):
    tail = text[m.end():m.end() + 400]
    call = re.search(r'mpReachStub\([^,]+, "((?:[^"\\]|\\.)*)"\)', tail)
    stubs[m.group(1)] = call.group(1).replace('\\"', '"') if call else m.group(1)

# Retail asm, indexed by the `# Class::Method(args)` comment above each `.fn`.
bodies = {}
short = {}
for root, _dirs, files in os.walk(asm_root):
    for name in files:
        if not name.endswith(".s"):
            continue
        path = os.path.join(root, name)
        lines = open(path, errors="replace").read().splitlines()
        for i, line in enumerate(lines):
            if not line.startswith(".fn "):
                continue
            size, demangled = None, None
            for j in (i - 1, i - 2, i - 3):
                if j < 0:
                    continue
                m = re.search(r"size: (0x[0-9A-Fa-f]+)", lines[j])
                if m and size is None:
                    size = int(m.group(1), 16)
                if lines[j].startswith("# ") and "|" not in lines[j]:
                    demangled = lines[j][2:].strip()
            if demangled is None:
                continue
            where = os.path.relpath(path, asm_root)
            if demangled not in bodies or size is not None:
                bodies[demangled] = (size, where, i + 1)
            short.setdefault(demangled.split("(")[0].strip(), bodies[demangled])

# Which of them a built object still asks for (the stub file itself defines them all).
refs = defaultdict(set)
for root, _dirs, files in os.walk(build):
    for name in files:
        if not name.endswith(".o"):
            continue
        path = os.path.join(root, name)
        for line in subprocess.run(["nm", "-u", path], capture_output=True,
                                   text=True).stdout.splitlines():
            parts = line.split()
            if parts and parts[-1] in stubs:
                refs[parts[-1]].add(os.path.relpath(path, build).replace(
                    "CMakeFiles/mp_game.dir/", ""))

demangled = subprocess.run(["c++filt"], input="\n".join(stubs.values()), capture_output=True,
                           text=True).stdout.splitlines()
names = list(stubs)
for i, name in enumerate(names):
    stubs[name] = demangled[i] if i < len(demangled) else stubs[name]


def body(sig):
    hit = bodies.get(sig)
    if hit is None:
        hit = short.get(sig.split("(")[0].strip())
    return hit


needed = [(n, sorted(refs[n])[0]) for n in names if n in refs]
dead = [n for n in names if n not in refs]
print(f"{len(names)} stubs: {len(needed)} referenced by the build, {len(dead)} dead\n")
print("== referenced (the backlog), cheapest first")
for size, n, sig, origin in sorted((body(stubs[n])[0] if body(stubs[n]) else 10 ** 9, n,
                                    stubs[n], body(stubs[n])) for n, _ in needed):
    shown = f"{size:>5} B" if size != 10 ** 9 else "no asm"
    where = f"  {origin[1]}:{origin[2]}" if origin else ""
    print(f"  {shown}  {sig[:80]:<80}{where}")
print("\n== dead (no built object references them)")
for n in dead:
    print(f"          {stubs[n][:80]}")
