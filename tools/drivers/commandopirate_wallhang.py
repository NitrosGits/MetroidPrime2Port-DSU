#!/usr/bin/env python3
"""CommandoPirate driver: the WallHang state (fn_9_7358) and the WallDetach state's arm.

The commando with cover points around it lives in 05_swamp_timber! (the Swamp) - the layer has
eleven cover points and two commandos (tools/scan_commando_pattern.py lists them). The commando
is inactive in this harness, so it is woken with the console's `send <id> Activate`; the
CoverFind state is then forced so that the commando holds a cover point, the WallHang state is
forced (nothing in the shipped data enters it) and the WallDetach state is forced afterwards -
its arm wants the body to be in the wall-hang state, which the WallHang state's command gives.

usage: commandopirate_wallhang.py <port> [outfile]
"""
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4846
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout
# The console's layer (the Cliff's commandoes live in the layer 3).
LAYER = sys.argv[3] if len(sys.argv) > 3 else None


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
    if LAYER is not None:
        log("layer %s: %s" % (LAYER, cmd("layer %s" % LAYER)))
        wait_frame(frame() + 20)
    ids = commandos()
    log("commandos %s (frame %d)" % (ids, frame()))
    for target in ids:
        log("=== commando %s ===" % target)
        log("send Activate: %s" % cmd("send %s Activate" % target))
        time.sleep(1.0)
        # the CoverFind state needs the target's aim position, so the player is put next to the
        # commando first (the console's `place`).
        log("place player: %s" % cmd("place %s player 5" % target))
        wait_frame(frame() + 30)
        log("fsm CoverFind: %s" % cmd("fsm %s CoverFind" % target))
        wait_frame(frame() + 20)
        log("state: %s" % state_of(target))
        log("fsm WallHang: %s" % cmd("fsm %s WallHang" % target))
        wait_frame(frame() + 40)
        log("state: %s" % state_of(target))
        log("fsm WallDetach: %s" % cmd("fsm %s WallDetach" % target))
        wait_frame(frame() + 20)
        log("state: %s" % state_of(target))
        log("fsm Lurk: %s" % cmd("fsm %s Lurk" % target))
        wait_frame(frame() + 20)
        log("state: %s" % state_of(target))
        log("objs: %s" % cmd("objs commando"))


if __name__ == "__main__":
    main()
