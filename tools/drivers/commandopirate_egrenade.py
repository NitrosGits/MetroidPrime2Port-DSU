#!/usr/bin/env python3
"""CommandoPirate driver: the E-grenade states only, to reproduce the hang.

The states driver hung when it forced the module's E-grenade states.  This driver forces them
one after the other and prints what the console answers, so the hang can be seen (and the
backtrace taken) with the game under gdb.

usage: commandopirate_egrenade.py <port> [outfile]
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
    ids = commandos()
    log("commandos %s (frame %d)" % (ids, frame()))
    for target in ids:
        log("=== commando %s ===" % target)
        log("send Activate: %s" % cmd("send %s Activate" % target))
        time.sleep(1.0)
        log("place player: %s" % cmd("place %s player 5" % target))
        wait_frame(frame() + 20)
        for name in ("EGrenadeAttack", "PostEGrenadeAttack"):
            log("fsm %s: %s" % (name, cmd("fsm %s %s" % (target, name))))
            wait_frame(frame() + 10)
            log("state: %s" % state_of(target))
        log("objs: %s" % cmd("objs commando"))


if __name__ == "__main__":
    main()
