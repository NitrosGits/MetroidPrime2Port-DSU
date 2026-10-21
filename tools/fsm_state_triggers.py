#!/usr/bin/env python3
"""Parse a CStateMachine asset (the AFSM the SpacePirate uses) and list its states' triggers.

usage: tools/fsm_state_triggers.py [asset-id]   (default: the SpacePirate's 0x5652a12f)

The format is the one src/MetroidPrime/Enemies/CStateMachine.cpp reads: u32 state count, then
one null-terminated name per state, u32 trigger count, then per state a u32 trigger count and,
for each of those, a u32 transition count and per transition a null-terminated name, a float
argument and (for the first transition only) the u32 index of the state it switches to.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

DEFAULT_ID = 0x5652A12F


def load(rid):
    iso = ed.Iso(ed.find_iso())
    for pk in ed.load_paks(iso):
        if rid in pk.res:
            return pk.get(rid)
    return None


def parse(data):
    pos = 0

    def u32():
        nonlocal pos
        (v,) = struct.unpack_from(">I", data, pos)
        pos += 4
        return v

    def f32():
        nonlocal pos
        (v,) = struct.unpack_from(">f", data, pos)
        pos += 4
        return v

    def name():
        nonlocal pos
        end = data.index(b"\0", pos)
        s = data[pos:end].decode("latin1")
        pos = end + 1
        return s

    state_count = u32()
    states = [name() for _ in range(state_count)]
    u32()  # the trigger pool size
    out = []
    for sname in states:
        n = u32()
        if n == 0:
            out.append((sname, []))
            continue
        trigs = []
        for _ in range(n):
            tcount = u32()
            for k in range(tcount):
                tname = name()
                targ = f32()
                target = u32() if k == 0 else None
                trigs.append((tname, targ, targ))
        out.append((sname, trigs))
    return out


def main():
    rid = int(sys.argv[1], 0) if len(sys.argv) > 1 else DEFAULT_ID
    data = load(rid)
    if data is None:
        print("no such asset", file=sys.stderr)
        return 1
    for sname, trigs in parse(data):
        names = [t[0].lstrip("!#0123456789") for t in trigs]
        print(f"{sname}: {', '.join(names)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
