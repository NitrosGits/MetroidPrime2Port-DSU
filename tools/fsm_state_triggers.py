#!/usr/bin/env python3
"""Parse a state-machine asset and list its states' triggers.

usage: tools/fsm_state_triggers.py [asset-id]   (default: the SpacePirate's 0x5652a12f)
       tools/fsm_state_triggers.py --list [tag] (asset ids of that tag, default FSM2)
       tools/fsm_state_triggers.py --all [tag]  (parse every asset of that tag)

Two layouts exist in this build, and the tool picks one from the asset's first four bytes:

* AFSM (the SpacePirate's state machine) - the layout src/MetroidPrime/Enemies/CStateMachine.cpp
  reads: u32 state count, then one null-terminated name per state, u32 trigger count, then per
  state a u32 trigger count and, for each of those, a u32 transition count and per transition a
  null-terminated name, a float argument and (for the first transition only) the u32 index of the
  state it switches to.  Printed unchanged since this tool was added.

* FSM2 (the `'FSM2'` tag, CGenericFSM2, src/MetroidPrime/Enemies/CGenericFSM2.cpp): the four
  bytes 'FSM2', u32 version (1), u32 state count, u32 trigger count, u32 code count, u32
  sub-machine count, then the four node lists in that order.  A node is a null-terminated name
  followed by a u32 transition count and then that many transitions; a transition is a
  null-terminated name (empty in 3045 of the 3138 transitions of the 89 shipped FSM2 assets) and a u32 target.  A
  trigger node also carries a f32 argument before its transitions and a u8 negate flag after
  them, a sub-machine a u32 after them.  The target's top byte is the node kind (0 trigger,
  1 state, 2 code, 3 sub-machine) and its low 24 bits the index into that kind's list - the
  split CGenericFSM2::ResolveNode makes.  The asset is padded with 0xff to the pak entry's size.

  A state's transition list is the set of trigger nodes it evaluates, and a trigger leads on
  through its own (single) transition, so a state's entry is printed as the trigger name and the
  node that trigger chain ends at: `bTrigger->Destination`, the same `name->target` shape the
  AFSM listing uses.  Codes and sub-machines are printed after the states, prefixed so their
  names cannot be confused with a state's.
"""
import struct
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import echoes_disc as ed  # noqa: E402

DEFAULT_ID = 0x5652A12F
FSM2_MAGIC = b"FSM2"

TRIGGER, STATE, CODE, SUBMACHINE = 0, 1, 2, 3
KIND_NAME = {TRIGGER: "trigger", STATE: "state", CODE: "code", SUBMACHINE: "sub-machine"}


def paks():
    return ed.load_paks(ed.Iso(ed.find_iso()))


def load(rid):
    for pk in paks():
        if rid in pk.res:
            return pk.get(rid)
    return None


def list_assets(tag):
    seen = set()
    for pk in paks():
        for rid, res in pk.res.items():
            if res[0] == tag:
                seen.add(rid)
    return sorted(seen)


# ---------------------------------------------------------------- AFSM (CStateMachine)
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
                # the state's slot holds the last transition of the entry; the chain (the
                # "and" links) runs from there back to the first, which owns the target state.
                trigs.append((tname, targ, states[target] if target is not None and
                              target < len(states) else "AND"))
        out.append((sname, trigs))
    return out


# ---------------------------------------------------------------- FSM2 (CGenericFSM2)
class Stream:
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def u32(self):
        (v,) = struct.unpack_from(">I", self.data, self.pos)
        self.pos += 4
        return v

    def f32(self):
        (v,) = struct.unpack_from(">f", self.data, self.pos)
        self.pos += 4
        return v

    def u8(self):
        v = self.data[self.pos]
        self.pos += 1
        return v

    def name(self):
        end = self.data.index(b"\0", self.pos)
        s = self.data[self.pos:end].decode("latin1")
        self.pos = end + 1
        return s


class Fsm2:
    """The four node lists CGenericFSM2 builds, each entry (name, ..., transitions)."""

    def __init__(self, data):
        self.magic = data[0:4]
        in_ = Stream(data)
        in_.pos = len(self.magic)  # the 'FSM2' fourcc, then the version word
        self.version = in_.u32()
        num_states = in_.u32()
        num_triggers = in_.u32()
        num_codes = in_.u32()
        num_sub_machines = in_.u32()

        def transitions():
            return [(in_.name(), in_.u32()) for _ in range(in_.u32())]

        self.states = [(in_.name(), transitions()) for _ in range(num_states)]
        self.triggers = []
        for _ in range(num_triggers):
            name = in_.name()
            argument = in_.f32()
            trans = transitions()
            self.triggers.append((name, argument, trans, in_.u8()))
        self.codes = [(in_.name(), transitions()) for _ in range(num_codes)]
        self.sub_machines = []
        for _ in range(num_sub_machines):
            name = in_.name()
            trans = transitions()
            self.sub_machines.append((name, trans, in_.u32()))
        self.used = in_.pos
        self.size = len(data)
        self.tail = data[self.used:self.size]

    def lists(self):
        return (self.triggers, self.states, self.codes, self.sub_machines)

    def label(self, target):
        """The node a target word names, `state:` and friends except for a plain state."""
        kind, index = target >> 24, target & 0xFFFFFF
        if kind not in KIND_NAME:
            return "?%08x" % target
        names = [entry[0] for entry in self.lists()[kind]]
        if index >= len(names):
            return "?%08x" % target
        if kind == STATE:
            return names[index]
        return "%s:%s" % (KIND_NAME[kind], names[index])

    def destination(self, target, depth=0):
        """Follow a trigger chain (each trigger has one transition) to the node it ends at."""
        kind, index = target >> 24, target & 0xFFFFFF
        if kind != TRIGGER or depth > 8:
            return self.label(target)
        if index >= len(self.triggers):
            return "?%08x" % target
        for _, nxt in self.triggers[index][2]:
            return self.destination(nxt, depth + 1)
        return self.label(target)

    def leftovers_are_padding(self):
        return all(b == 0xFF for b in self.tail)


def parse_fsm2(data):
    """One rendered entry per state transition: `trigger->destination`, or the node's name when
    the entry points straight at a state, code or sub-machine (an unconditional link)."""
    machine = Fsm2(data)
    if machine.version != 1:
        print("warning: FSM2 version %d, this reader knows version 1" % machine.version,
              file=sys.stderr)
    rows = []
    for name, trans in machine.states:
        # a state's transitions are the trigger nodes it evaluates; the transition's own name is
        # empty in the shipped data, so fall back to the node the entry points at.
        entries = []
        for tname, target in trans:
            if target >> 24 != TRIGGER:
                entries.append(tname or machine.label(target))
            else:
                entries.append("%s->%s" % (tname or machine.label(target),
                                           machine.destination(target)))
        rows.append((name, entries))
    return machine, rows


def is_fsm2(data):
    return data[0:4] == FSM2_MAGIC


def print_fsm1(rows):
    for sname, trigs in rows:
        names = ["%s->%s" % (t[0].lstrip("!#0123456789"), t[2]) for t in trigs]
        print(f"{sname}: {', '.join(names)}")


def print_fsm2(machine, rows):
    for sname, entries in rows:
        print(f"{sname}: {', '.join(entries)}")
    if machine.triggers:
        print("--- triggers")
        for name, argument, trans, negate in machine.triggers:
            targets = [tname or machine.destination(target) for tname, target in trans]
            print("%s (%g%s): %s" % (name, argument, ", negated" if negate else "",
                                     ", ".join(targets)))
    for title, entries in (("codes", machine.codes), ("sub-machines", machine.sub_machines)):
        if not entries:
            continue
        print("--- " + title)
        for name, trans, *_ in entries:
            targets = [tname or machine.destination(target) for tname, target in trans]
            print("%s: %s" % (name, ", ".join(targets)))
    if not machine.leftovers_are_padding():
        print("warning: %d bytes after the last node are not 0xff padding"
              % (machine.size - machine.used), file=sys.stderr)


def show(rid):
    data = load(rid)
    if data is None:
        print("no such asset", file=sys.stderr)
        return 1
    try:
        if is_fsm2(data):
            machine, rows = parse_fsm2(data)
            print_fsm2(machine, rows)
        else:
            print_fsm1(parse(data))
    except (ValueError, struct.error) as exc:
        print("%#010x: parse failed: %s" % (rid, exc), file=sys.stderr)
        return 1
    return 0


def main():
    args = sys.argv[1:]
    if args and args[0] in ("--list", "--all"):
        tag = args[1] if len(args) > 1 else "FSM2"
        ids = list_assets(tag)
        if not ids:
            print("no %s asset in the paks" % tag, file=sys.stderr)
            return 1
        rc = 0
        for rid in ids:
            if args[0] == "--list":
                print("%#010x %s" % (rid, tag))
            else:
                print("=== %#010x" % rid)
                rc = show(rid) or rc
        return rc
    rid = int(args[0], 0) if args else DEFAULT_ID
    return show(rid)


if __name__ == "__main__":
    sys.exit(main())
