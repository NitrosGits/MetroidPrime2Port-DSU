#!/usr/bin/env python3
"""(lum2: LUM2=1 adds AreaLoaded to every Lumite, a `kill` of the 2nd, a Delete of the 3rd and a longer run.)
Lumite natural-play driver: no `fsm` forcing.  The player is placed at several distances from
each Lumite and the state machine runs by itself (triggers, shots, taunts, hops between hints),
then a few small damages.  Run with MP2_GODMODE=1.

usage: lumite_natural.py <port> [outfile] [layer]
"""
import os
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout
LAYER = sys.argv[3] if len(sys.argv) > 3 else None
DISTS = [4, 9, 14, 20, 30, 45]


def log(msg):
    OUT.write(str(msg) + "\n")
    OUT.flush()


def connect():
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
    return [l.split()[0] for l in cmd("objs Lumite").split("\n") if "CLumite" in l and "(" in l]


wait_frame(200)
if LAYER is not None:
    log("layer %s: %s" % (LAYER, cmd("layer %s" % LAYER)))
    wait_frame(frame() + 20)
ids = lumites()
log("lumites %s" % ids)
# LUM2_FILES=<prefix>: write the uids (decimal) of the team manager and the safe zones for the gdb force
# script (tools/cov/specs/lumite_lum2_calls.py reads <prefix>.team / <prefix>.zones)
if os.environ.get("LUM2_FILES"):
    def uids(kind):
        return [str(int(l.split()[0][1:], 16)) for l in cmd("objs %s" % kind).split("\n") if l.startswith("u")]
    open(os.environ["LUM2_FILES"] + ".team", "w").write("\n".join(uids("Team")))
    open(os.environ["LUM2_FILES"] + ".zones", "w").write("\n".join(uids("SafeZone")))
# wake every safe zone and AI hint so sunlight hops / zone damage can happen
for kind in ("SafeZone", "AIHint"):
    for l in cmd("objs %s" % kind).split("\n"):
        if l.startswith("u") and "inactive" in l:
            cmd("send %s Activate" % l.split()[0])
LUM2 = os.environ.get("LUM2") == "1"
if LUM2:
    for target in ids:
        log(cmd("send %s Activate" % target))
        log(cmd("send %s AreaLoaded" % target))
for n, target in enumerate(ids):
    log(cmd("send %s Activate" % target))
    if LUM2 and n == 1:
        wait_frame(frame() + 30)
        log("kill: %s" % cmd("kill %s" % target))
        log("fsm Dead: %s" % cmd("fsm %s Dead" % target))
        wait_frame(frame() + 400)
        log(cmd("obj %s" % target).replace("\n", " | ")[:200])
        continue
    if LUM2 and n == 2:
        log("delete: %s" % cmd("send %s Delete" % target))
        wait_frame(frame() + 100)
        continue
    for d in DISTS:
        log("place %d: %s" % (d, cmd("place %s player %d" % (target, d))))
        wait_frame(frame() + 150)
        log(cmd("obj %s" % target).replace("\n", " | ")[:200])
    for _ in range(3):
        log("damage: %s" % cmd("damage %s 1" % target))
        wait_frame(frame() + 60)
log("objs: %s" % cmd("objs Lumite"))
