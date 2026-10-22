#!/usr/bin/env python3
"""CommandoPirate driver, slice tail11: the two team helpers (fn_9_19D8 the join, fn_9_1930 the
leave) and nothing else, so the pair's calls follow each other closely and the forced team ids of
tools/cov/specs/commandopirate_tail11.py land where they are meant to.

The join is the SetTargetDest state's activate message (`statecall <id> SetTargetDest 0`, the call
at 0x0000AAEC); the leave is the object's Deactivate message (`send <id> Deactivate`, the call at
0x0000CDC8). Two Deactivates in a row: the spec writes the first call's team id back at its
return, so the second finds a valid id whose manager no longer holds the commando (0x00001998).

usage: commandopirate_tail11.py <port> [outfile|-]
"""
import os
import re
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 and sys.argv[2] != "-" else sys.stdout


def log(msg):
    OUT.write(str(msg) + "\n")
    OUT.flush()
    if OUT is not sys.stdout:
        print(msg, flush=True)


def connect():
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


def main():
    wait(200)
    run("objs team")
    ids = commandos()
    log("commandos %s (frame %d)" % (ids, frame()))
    for cid in ids:
        log("=== commando %s ===" % cid)
        run("send %s Activate" % cid)
        run("place %s player 6" % cid)
        wait(20)
        for _ in range(6):
            # join (0x00001A30/0x00001A80 in the swamp), then leave twice: the second leave is
            # the "manager does not hold the commando" exit (0x00001998).
            run("statecall %s SetTargetDest 0" % cid)
            wait(3)
            run("send %s Deactivate" % cid)
            wait(3)
            run("send %s Deactivate" % cid)
            wait(3)
            run("statecall %s SetTargetDest 0" % cid)
            wait(3)
            run("send %s Activate" % cid)
            wait(3)
    wait(30)
    run("objs commando")


if __name__ == "__main__":
    main()
