#!/usr/bin/env python3
"""CommandoPirate driver: the WarpOut state (fn_9_8DF0) in area 59.

The commando in 05_temple_federationsite_dark is inactive in this harness (the area's layer
script does not run), so it is woken with the console's `send <id> Activate`. The WarpOut
state is then forced with the console's `fsm` - nothing in the shipped data enters it - so
its arms run: the activate, the update (the generate command and the fade that follows it)
and the deactivate, which the state machine sends when the next `fsm` leaves the state.

usage: commandopirate_warpout.py <port> [outfile]
"""
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
# A world load under gdb (a forcing run) can hold the console for longer than the connect
# timeout (the same fix as the core lane's driver).
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
    for line in cmd("objs commando").split("\n"):
        if "CCommandoPirate" in line and "(" in line:
            out.append(line.split()[0])
    return out


def state_of(cid):
    for line in cmd("obj %s" % cid).split("\n"):
        if line.strip().startswith("fsm"):
            return line.strip()
    return "?"


def main():
    wait_frame(200)
    ids = []
    # under gdb the frames are slow and the area loads late: wait for the commando.
    for _ in range(120):
        ids = commandos()
        if ids:
            break
        time.sleep(1.0)
    log("commandos %s (frame %d)" % (ids, frame()))
    for target in ids:
        log("=== commando %s ===" % target)
        log("send Activate: %s" % cmd("send %s Activate" % target))
        time.sleep(1.0)
        # the commando is frozen with the dark beam (the frozen knock-back reaction's flags are
        # 1|2 = 3) and then hit while it is still alive: the KnockBack override's alive branch
        # reads the flags right after the base's call.
        log("darkburst: %s" % cmd("damage %s 5 darkburst" % target))
        wait_frame(frame() + 10)
        log("damage alive: %s" % cmd("damage %s 1" % target))
        wait_frame(frame() + 10)
        log("state: %s" % state_of(target))
        # and the dead arm: the commando dies (the console's `kill`) and the damage lands on the
        # dead actor.
        log("kill: %s" % cmd("kill %s" % target))
        wait_frame(frame() + 10)
        log("kill: %s" % cmd("kill %s" % target))
        wait_frame(frame() + 10)
        log("state: %s" % state_of(target))
        log("damage: %s" % cmd("damage %s 5" % target))
        wait_frame(frame() + 10)
        log("state: %s" % state_of(target))
        log("objs: %s" % cmd("objs commando"))


if __name__ == "__main__":
    main()
