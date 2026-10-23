#!/usr/bin/env python3
"""DarkCommando driver: every state of the module's state table, one after the other, then
damage, a kill and the rag doll.

Forcing each state makes the state machine run its enter/update/exit bodies and the triggers
the coverage build counts.  KILL=0 in the environment skips the kill.

usage: darkcommando_states.py <port> [outfile] [layer]
"""
import os
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout
LAYER = sys.argv[3] if len(sys.argv) > 3 else None

STATES = [
    "Lurk", "Taunt", "FaceTarget", "SelectAttackTarget", "SetTargetFaceVect", "SelectAction",
    "MeleeAttack", "FireEMP", "FireChargeBeam", "PostFireChargeBeam", "ActivateCloak",
    "DeactivateCloak", "SelectDashTarget", "SetDashFaceVect", "ShadowDash", "GetUp",
    "WarpOut", "WarpIn", "FireEMP", "FireChargeBeam", "MeleeAttack", "SelectAction",
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


def commandos():
    out = []
    for line in cmd("objs DarkCommando").split("\n"):
        if "CDarkCommando" in line and "(" in line:
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
    ids = commandos()
    log("commandos %s (frame %d)" % (ids, frame()))
    for target in ids:
        log("=== commando %s ===" % target)
        log("send Activate: %s" % cmd("send %s Activate" % target))
        time.sleep(1.0)
        log("place player: %s" % cmd("place %s player 5" % target))
        wait_frame(frame() + 20)
        for name in STATES:
            log("fsm %s: %s" % (name, cmd("fsm %s %s" % (target, name))))
            wait_frame(frame() + 6)
        # a small damage: the commando is still alive, so the module's KnockBack (fn_3_62D0) runs.
        log("damage: %s" % cmd("damage %s 1" % target))
        wait_frame(frame() + 10)
        log("damage: %s" % cmd("damage %s 1" % target))
        wait_frame(frame() + 10)
        # the attacks again, each given time to play out (grenade flight, beam shot, dash).
        for name in ["SelectAction", "FireEMP", "FireChargeBeam", "MeleeAttack", "ShadowDash"]:
            log("place player: %s" % cmd("place %s player 8" % target))
            log("fsm %s: %s" % (name, cmd("fsm %s %s" % (target, name))))
            wait_frame(frame() + 90)
        log("state: %s" % state_of(target))
        if os.environ.get("KILL", "1") != "0":
            log("kill: %s" % cmd("damage %s 9999" % target))
            wait_frame(frame() + 240)
            log("state: %s" % state_of(target))
        log("objs: %s" % cmd("objs DarkCommando"))


if __name__ == "__main__":
    main()
