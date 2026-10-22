#!/usr/bin/env python3
"""CommandoPirate driver, slice cp3: the triggers no FSM2 state of the shipped areas evaluates
(called with the console's `trigcall`), the states the slice closes (`statecall` and `fsm`) and
the team helpers (`send Deactivate`/`Activate` around the SetTargetDest state's join).

Pair it with tools/cov/specs/commandopirate_cp3.py (tools/cov/force_ret.py): the spec flips the
members each trigger or state reads on alternate calls, so the repeated calls here take both arms.

usage: commandopirate_cp3.py <port> [outfile|-] [layer|-] [cover]
  cover: only the cover part - each commando is put on each cover point of the area (the
  Swamp's 05_swamp_timber!) before CoverFind, so it holds one for the cover triggers and the
  SetCoverDest state; the non-activate messages of SetJumpDest/SetCoverDest; PathFind together.
"""
import os
import re
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 and sys.argv[2] != "-" else sys.stdout
LAYER = sys.argv[3] if len(sys.argv) > 3 and sys.argv[3] != "-" else None
MODE = sys.argv[4] if len(sys.argv) > 4 else "all"

TRIGGERS = ["HeardShot", "ShouldCoverAttack", "AbortSeekCover", "AbortShieldCharge",
            "IsAggressive", "ShouldCrouch", "ShouldWallHang"]


def log(msg):
    OUT.write(str(msg) + "\n")
    OUT.flush()
    if OUT is not sys.stdout:
        print(msg, flush=True)


def connect():
    # a world load under gdb takes minutes, so the loop waits long.
    for _ in range(900):
        try:
            return socket.create_connection(("127.0.0.1", PORT), timeout=10)
        except OSError:
            time.sleep(0.5)
    raise SystemExit("no console on port %d" % PORT)


sock = connect()
sock.settimeout(int(os.environ.get("CON_REPLY_TIMEOUT", "300")))
stream = sock.makefile("rw")


def cmd(c):
    stream.write(c + "\n")
    stream.flush()
    lines = []
    while True:
        line = stream.readline()
        if not line:
            break
        line = line.rstrip("\n")
        lines.append(line)
        if line.startswith("=> "):
            break
    return " | ".join(lines)


def frame():
    for line in cmd("frame").split(" | "):
        if line.startswith("frame "):
            return int(line.split()[1])
    return -1


def wait(n):
    target = frame() + n
    while True:
        f = frame()
        if f < 0 or f >= target:
            return f
        time.sleep(0.2)


def commandos():
    out = []
    for line in cmd("objs commando").split(" | "):
        if "CCommandoPirate" in line and "(" in line:
            out.append(line.split()[0])
    return out


def run(c):
    log("%s: %s" % (c, cmd(c)))


def objs_of(kind, cls):
    out = {}
    for line in cmd("objs %s" % kind).split(" | "):
        if cls in line:
            m = re.search(r"\((-?[\d.]+), (-?[\d.]+), (-?[\d.]+)\)", line)
            if m:
                out[line.split()[0]] = tuple(float(v) for v in m.groups())
    return out


def cover_part(ids):
    covers = objs_of("cover", "CScriptCoverPoint")
    log("cover points %s" % sorted(covers))
    for cid in ids:
        log("=== commando %s (cover) ===" % cid)
        for msg in (1, 2):
            run("statecall %s SetJumpDest %d" % (cid, msg))
            run("statecall %s SetCoverDest %d" % (cid, msg))
        for cov, p in sorted(covers.items()):
            run("place %s %f %f %f" % ((cid,) + p))
            wait(10)
            run("place %s player 6" % cid)
            wait(10)
            run("fsm %s CoverFind" % cid)
            wait(20)
            run("trigcall %s ShouldCrouch" % cid)
            run("trigcall %s ShouldWallHang" % cid)
            run("statecall %s SetCoverDest 0" % cid)
            run("statecall %s SetCoverDest 0" % cid)
            run("fsm %s BreakCover" % cid)
            wait(10)
    for cid in ids:
        run("place %s player 3" % cid)
        run("fsm %s PathFind" % cid)
    wait(200)
    run("objs commando")


def main():
    wait(200)
    if LAYER is not None:
        run("layer %s" % LAYER)
        wait(20)
    ids = commandos()
    log("commandos %s (frame %d)" % (ids, frame()))
    run("objs cover")
    for cid in ids:
        run("send %s Activate" % cid)
        run("place %s player 6" % cid)
    wait(30)
    if MODE == "cover":
        cover_part(ids)
        return
    for cid in ids:
        log("=== commando %s ===" % cid)
        run("place %s player 6" % cid)
        wait(10)
        for _ in range(4):
            for name in TRIGGERS:
                run("trigcall %s %s" % (cid, name))
        for _ in range(4):
            run("statecall %s Lurk 0" % cid)
        for _ in range(3):
            run("statecall %s Alert 0" % cid)
            for _ in range(3):
                run("statecall %s Alert 1" % cid)
                wait(2)
            run("statecall %s Alert 2" % cid)
        # the team: SetTargetDest joins (fn_9_19D8); repeated, the id is already there.
        run("fsm %s SetTargetDest" % cid)
        wait(10)
        for _ in range(6):
            run("statecall %s SetTargetDest 0" % cid)
        # the cover point: CoverFind picks one where the area has them (the Swamp).
        run("fsm %s CoverFind" % cid)
        wait(20)
        for _ in range(2):
            run("trigcall %s ShouldCrouch" % cid)
            run("trigcall %s ShouldWallHang" % cid)
        for _ in range(4):
            run("statecall %s SetCoverDest 0" % cid)
            run("statecall %s SetJumpDest 0" % cid)
        run("fsm %s JumpBack" % cid)
        wait(40)
        for _ in range(4):
            run("statecall %s JumpBack 0" % cid)
            run("statecall %s JumpBack 1" % cid)
            wait(2)
        run("statecall %s JumpBack 2" % cid)
        run("fsm %s PathFind" % cid)
        wait(60)
        # the team helper that leaves (fn_9_1930): the deactivate message calls it.
        for _ in range(4):
            run("send %s Deactivate" % cid)
            wait(4)
            run("send %s Activate" % cid)
            wait(4)
            run("statecall %s SetTargetDest 0" % cid)
            wait(4)
        run("obj %s" % cid)
    # every commando near the player at once: the PathFind state's avoidance (fn_9_1A9C).
    for cid in ids:
        run("place %s player 3" % cid)
        run("fsm %s PathFind" % cid)
    wait(120)
    run("objs commando")


if __name__ == "__main__":
    main()
