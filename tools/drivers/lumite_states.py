#!/usr/bin/env python3
"""Lumite driver: every state of the module's state table, one after the other (hops between AI
hints, small and big shots, taunts, turns), then damage and a kill.

Forcing each state makes the state machine run its enter/update/exit bodies and the triggers
the coverage build counts.  KILL=0 in the environment skips the kill; FORCE_DEAD=1 forces the Dead state first.

usage: lumite_states.py <port> [outfile] [layer]
"""
import os
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout
LAYER = sys.argv[3] if len(sys.argv) > 3 else None

STATES = [
    "Pause", "Turn", "SmallShot", "Taunt", "BigShot", "TurnToHopPoint", "Hop", "Taunt", "Turn",
    "TurnToHopPoint", "Hop", "SmallShot", "BigShot", "Taunt", "Null", "Pause",
]


def log(msg):
    OUT.write(str(msg) + "\n")
    OUT.flush()
    print(msg, flush=True)


def connect():
    # a world load under gdb takes longer than the connect timeout, so the loop is long.
    for _ in range(900):
        try:
            return socket.create_connection(("127.0.0.1", PORT), timeout=10)
        except OSError:
            time.sleep(0.5)
    raise SystemExit("no console on port %d" % PORT)


sock = connect()
sock.settimeout(300)
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
        if line.startswith("=> "):
            break
        lines.append(line)
    return "\n".join(lines)


def frame():
    for line in cmd("frame").split("\n"):
        if line.startswith("frame "):
            return int(line.split()[1])
    return -1


def wait_frame(target):
    while True:
        f = frame()
        if f < 0 or f >= target:
            return f
        time.sleep(0.2)


def lumites():
    out = []
    for line in cmd("objs Lumite").split("\n"):
        if "CLumite" in line and "(" in line:
            out.append(line.split()[0])
    return out


def state_of(cid):
    for line in cmd("obj %s" % cid).split("\n"):
        if line.strip().startswith("fsm"):
            return line.strip()
    return "?"


def main():
    wait_frame(200)
    if LAYER is not None:
        log("layer %s: %s" % (LAYER, cmd("layer %s" % LAYER)))
        wait_frame(frame() + 20)
    ids = lumites()
    log("lumites %s (frame %d)" % (ids, frame()))
    for target in ids:
        log("=== lumite %s ===" % target)
        log("send Activate: %s" % cmd("send %s Activate" % target))
        time.sleep(1.0)
        log("place player: %s" % cmd("place %s player 5" % target))
        wait_frame(frame() + 20)
        for name in STATES:
            log("fsm %s: %s" % (name, cmd("fsm %s %s" % (target, name))))
            wait_frame(frame() + 6)
        # a small damage first: the lumite is still alive, so the knockback and hurt paths run.
        log("damage: %s" % cmd("damage %s 1" % target))
        wait_frame(frame() + 10)
        log("damage: %s" % cmd("damage %s 1" % target))
        wait_frame(frame() + 10)
        # the attacks and a hop again, each given time to play out (shot flight, the hop arc).
        for name in ["SmallShot", "BigShot", "TurnToHopPoint", "Hop", "Taunt"]:
            log("place player: %s" % cmd("place %s player 8" % target))
            log("fsm %s: %s" % (name, cmd("fsm %s %s" % (target, name))))
            wait_frame(frame() + 90)
        log("state: %s" % state_of(target))
        if os.environ.get("FORCE_DEAD") == "1":
            # forced (fsm Dead): the Dead state on a living Lumite, for its bodies' arms.
            log("fsm Dead: %s" % cmd("fsm %s Dead" % target))
            for _ in range(4):
                wait_frame(frame() + 30)
                log("obj: %s" % cmd("obj %s" % target).replace("\n", " | "))
        if os.environ.get("KILL", "1") != "0":
            # just past the remaining health: an overkill takes CPatterned's massive-death delete
            # and the module's Dead state never runs.
            probe = cmd("damage %s 0.1" % target)
            left = float(probe.split("->")[-1].split()[0]) if "->" in probe else 10.0
            log("kill: %s" % cmd("damage %s %.2f" % (target, left + 0.5)))
            for _ in range(8):
                wait_frame(frame() + 30)
                log("obj: %s" % cmd("obj %s" % target).replace("\n", " | "))
        log("objs: %s" % cmd("objs Lumite"))


if __name__ == "__main__":
    main()
