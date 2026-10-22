#!/usr/bin/env python3
"""CommandoPirate driver: the states the area 59 run does not reach (slice 30).

The commando is woken (`send <id> Activate` - the area's layer script does not run in this
harness), the player is put next to it and the module's states that nothing in the shipped
flow enters are forced with the console's `fsm`: the WallHang (fn_9_7358), the RestoreOrientation
(fn_9_786C), the ScriptedShieldCharge (fn_9_7A98), the Cover (fn_9_6DBC) and the GetUp (fn_9_6A40).

When the area has cover points (the Swamp's 05_swamp_timber!) the commando is put on one of them
and the cover states are forced with it, so the cover arms of the WallHang and the Cover run.

usage: commandopirate_states2.py <port> [outfile]
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout


def log(msg):
    OUT.write(str(msg) + "\n")
    OUT.flush()
    print(msg, flush=True)


def connect():
    for _ in range(240):
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


def wait(n):
    for _ in range(n):
        time.sleep(0.2)


def wait_frame(target):
    while True:
        f = frame()
        if f < 0 or f >= target:
            return f
        time.sleep(0.2)


def ids_of(kind, cls):
    out = []
    for line in cmd("objs %s" % kind).split("\n"):
        if cls in line and "(" in line:
            out.append(line.split()[0])
    return out


def pos_of(oid):
    # `obj` prints no position; `objs` does (filtered on the class name).
    out = cmd("objs commando") + "\n" + cmd("objs cover")
    for line in out.split("\n"):
        if line.startswith(oid + " "):
            m = re.search(r"\((-?[\d.]+), (-?[\d.]+), (-?[\d.]+)\)", line)
            if m:
                return tuple(float(v) for v in m.groups())
    log("no position for %s" % oid)
    return None


def state_of(cid):
    for line in cmd("obj %s" % cid).split("\n"):
        if line.strip().startswith("fsm"):
            return line.strip()
    return "?"


def step(cid, c, frames):
    log("%s: %s" % (c, cmd(c % cid if "%s" in c else c)))
    wait(frames)
    log("  -> %s" % state_of(cid))


def main():
    wait_frame(200)
    ids = ids_of("commando", "CCommandoPirate")
    covers = ids_of("cover", "CScriptCoverPoint")
    log("commandos %s, cover points %s (frame %d)" % (ids, covers, frame()))
    for cid in ids:
        log("=== commando %s ===" % cid)
        step(cid, "send %s Activate", 30)
        step(cid, "place %s player 5", 20)
        for cov in covers:
            p = pos_of(cov)
            if p is None:
                continue
            log("cover %s at %s" % (cov, p))
            step(cid, "place %%s %f %f %f" % p, 10)
            step(cid, "place %s player 6", 10)
            step(cid, "fsm %s CoverFind", 20)
            step(cid, "fsm %s Cover", 60)
            step(cid, "fsm %s WallHang", 60)
            step(cid, "fsm %s CoverAttack", 60)
            step(cid, "fsm %s Cover", 60)
        # the states that need no cover point
        step(cid, "fsm %s WallHang", 60)
        step(cid, "fsm %s RestoreOrientation", 60)
        step(cid, "fsm %s ScriptedShieldCharge", 120)
        step(cid, "fsm %s GetUp", 60)
        step(cid, "fsm %s ScriptedShieldCharge", 30)
        # the Dead state before any kill: the kill deletes the actor, so the state has to run
        # while the commando is still there (the state's own activate ends it).
        step(cid, "fsm %s Dead", 60)
        step(cid, "fsm %s Lurk", 10)


main()
