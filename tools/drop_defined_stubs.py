#!/usr/bin/env python3
"""Delete the reach/data stubs in src/MetroidPrime/PortLinkStubs.cpp that real objects now define.

usage: tools/drop_defined_stubs.py <object.o>...   (e.g. build-port-link/CMakeFiles/mp_game.dir/src/Kyoto/Math/CAABox.cpp.o)

Each stub entry is a blank-line-separated block carrying `asm("<mangled>")`; a block is dropped when
one of the objects defines that symbol (`nm --defined-only`, weak definitions excluded).
"""
import re
import subprocess
import sys

STUBS = "src/MetroidPrime/PortLinkStubs.cpp"


def defined(objs):
    out = subprocess.run(["nm", "--defined-only", *objs], capture_output=True, text=True, check=True).stdout
    names = set()
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] not in ("W", "V", "w", "v"):
            names.add(parts[2])
    return names


def main():
    names = defined(sys.argv[1:])
    blocks = open(STUBS).read().split("\n\n")
    kept, dropped = [], []
    for b in blocks:
        m = re.search(r'asm\("([^"]+)"\)', b)
        (dropped if m and m.group(1) in names else kept).append(b)
    open(STUBS, "w").write("\n\n".join(kept))
    for b in dropped:
        print("dropped", re.search(r'asm\("([^"]+)"\)', b).group(1))
    print(f"{len(dropped)} stubs dropped")


if __name__ == "__main__":
    main()
