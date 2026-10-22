#!/usr/bin/env python3
"""CommandoPirate driver, slice tail12: every reader of the loader's block flag bytes.

The Cliff's entrance (world 1baa96c2, layer 3) holds six commandoes; the driver wakes each one,
puts the player next to it and forces the module's states one after the other, so every trigger
that reads the block's flag bytes is evaluated; then the Activate/Deactivate/AreaLoaded/Escape
messages (the AcceptScriptMsg readers) and small damages (the KnockBack reader). The first two
commandoes die at different times (`kill`) and their corpses are shot, so the rag doll exists
for fn_9_3E1C's flags argument and fn_9_C300's rag-doll arm.

usage: commandopirate_tail12.py <port> [outfile] [layer]
"""
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout
LAYER = sys.argv[3] if len(sys.argv) > 3 else None

STATES = [
    "Lurk", "Ambush", "BreakAmbush", "SelectTarget", "PathFind", "JumpBack", "Dodge",
    "ArmShield", "ScriptedShieldCharge", "ShieldCharge", "MeleeAttack", "Cover", "CoverFind",
    "SetRetreatDest", "WarpIn", "WarpOut", "GetUp",
]


def log(msg):
    OUT.write(str(msg) + "\n")
    OUT.flush()
    print(msg, flush=True)


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


def main():
    wait(100)
    if LAYER is not None:
        log("layer %s: %s" % (LAYER, cmd("layer %s" % LAYER)))
        wait(12)
    ids = []
    for _ in range(80):
        ids = commandos()
        if ids:
            break
        wait(20)
    log("commandos %s (frame %d)" % (ids, frame()))
    for i, cid in enumerate(ids[:1]):
        log("=== commando %s ===" % cid)
        log("activate: %s" % cmd("send %s Activate" % cid))
        wait(4)
        if i == 0:
            log("place: %s" % cmd("place %s player 5" % cid))
            wait(8)
        for name in STATES:
            cmd("fsm %s %s" % (cid, name))
            wait(3)
        # the AcceptScriptMsg readers: the Activate block flag, the AreaLoaded block flag and
        # the warp-out helper behind the Escape message
        log("deactivate: %s" % cmd("send %s Deactivate" % cid))
        wait(4)
        log("activate: %s" % cmd("send %s Activate" % cid))
        wait(4)
        log("arealoaded: %s" % cmd("send %s AreaLoaded" % cid))
        wait(4)
        log("escape: %s" % cmd("send %s Escape" % cid))
        wait(6)
        log("damage: %s" % cmd("damage %s 1" % cid))
        wait(6)
        if i == 0:
            # a corpse with a rag doll: fn_9_3E1C creates it, the knock-back reaches the
            # KnockBack override's rag-doll arm
            log("kill: %s" % cmd("kill %s" % cid))
            wait(30)
            log("damage corpse: %s" % cmd("damage %s 5" % cid))
            wait(10)
        log("objs: %s" % cmd("objs commando"))


if __name__ == "__main__":
    main()
