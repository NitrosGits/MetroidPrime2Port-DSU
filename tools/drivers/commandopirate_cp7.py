#!/usr/bin/env python3
"""CommandoPirate driver, slice cp7 (paired with tools/cov/specs/commandopirate_cp7.py).

ssc: the ScriptedShieldCharge (fn_9_7A98) arrival, whose next-waypoint pick (fn_9_2A98 with
  useNext) runs the module's waypoint predicate (fn_9_F6DC) over the waypoint's ARRV/NEXT links.
egrenade: the EGrenadeAttack state forced (`fsm`) with the commando near and far from the player,
  so the launch event (fn_9_5410) asks for the aim point (fn_9_52BC).

usage: commandopirate_cp7.py <port> [outfile|-] [sections, e.g. ssc,egrenade (default: all)]
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 and sys.argv[2] != "-" else sys.stdout
ONLY = set(sys.argv[3].split(",")) if len(sys.argv) > 3 else None


def want(section):
    return ONLY is None or section in ONLY


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


def cmd(c, replies=1):
    stream.write(c + "\n")
    stream.flush()
    lines = []
    while replies > 0:
        line = stream.readline()
        if not line:
            break
        line = line.rstrip("\n")
        lines.append(line)
        if line.startswith("=> "):
            replies -= 1
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
        time.sleep(0.1)


def run(c):
    log("%s: %s" % (c, cmd(c)))


def tick(*cmds):
    """several commands in one game tick (the console's `a; b; c`)."""
    log("%s: %s" % ("; ".join(cmds), cmd("; ".join(cmds), len(cmds))))


def objs_of(kind, cls):
    out = {}
    for line in cmd("objs %s" % kind).split(" | "):
        if cls in line:
            m = re.search(r"\((-?[\d.]+), (-?[\d.]+), (-?[\d.]+)\)", line)
            if m:
                out[line.split()[0]] = tuple(float(v) for v in m.groups())
    return out


def var(cid, name, value=None):
    out = cmd("var %s cp.%s%s" % (cid, name, "" if value is None else " %d" % value))
    m = re.search(r"= (-?\d+)", out)
    return int(m.group(1)) if m else None


def bits(cid, name, mask, on, *body):
    """a flag bit forced around the calls, put back in the same tick."""
    old = var(cid, name)
    new = (old | mask) if on else (old & ~mask & 0xff)
    tick("var %s cp.%s %d" % (cid, name, new), *body, "var %s cp.%s %d" % (cid, name, old))


def cp7(cid, player):
    log("=== commando %s ===" % cid)
    run("send %s Activate" % cid)
    run("place %s player 6" % cid)
    wait(20)
    pl = "var %s cp.xbca %d" % (cid, int(player[1:], 16))
    if want("ssc"):
        ssc = "statecall %s ScriptedShieldCharge" % cid
        loop = "bodystate %s 9" % cid
        for _ in range(3):
            tick(ssc + " 0")
            tick(ssc + " 1")
            wait(4)
            for ahead in (100, -2000, 100):                 # arrived: inside the box / behind
                bits(cid, "c64", 0x04, False, loop, "var %s cp.b94ahead %d" % (cid, ahead), ssc + " 1")
            tick(ssc + " 2")
            wait(10)
        run("bodystate %s 5" % cid)
        wait(10)
    if want("egrenade"):
        for d in (12, 3, 20, 4, 9, 2):
            tick("place %s player %d" % (cid, d), pl, "fsm %s EGrenadeAttack" % cid)
            wait(150)
            log("obj: %s" % cmd("obj %s" % cid))
    log("state %s" % cmd("obj %s" % cid))


def main():
    wait(100)
    ids = []
    for _ in range(30):  # the area can still be loading
        ids = [line.split()[0] for line in cmd("objs commando").split(" | ")
               if "CCommandoPirate" in line and "(" in line]
        if ids:
            break
        wait(10)
    log("commandos %s (frame %d)" % (ids, frame()))
    players = list(objs_of("player", "CPlayer"))
    for cid in ids:
        cp7(cid, players[0] if players else "u0000")
    log("done (frame %d)" % frame())


main()
