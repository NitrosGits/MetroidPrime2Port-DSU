#!/usr/bin/env python3
"""CommandoPirate driver: the module's rag doll (fn_9_3E1C and the rag-doll arms of the Think,
PreThink, PreRender, PreRenderAllViewports and KnockBack).

Each commando is woken (`send <id> Activate`). The first one dies on land (`kill <id>`): its death
animation's BecomeRagDoll event starts the module's timer, the rag doll is created, primed and
stepped until it is over and the fade starts; the corpse is shot on the way (KnockBack with a rag
doll). When the area has a water, the next one is put in the water's centre (`place`) so it drowns
(fn_9_3090) and its rag doll is stepped with the water's top. The rest die on land too.

usage: commandopirate_ragdoll.py <port> [outfile]
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


def position(oid):
    # `obj` prints a water's volume as "bounds <min x y z> <max x y z>": the centre, 2 units under
    # the surface (or halfway down a shallower one)
    for line in cmd("obj %s" % oid).split("\n"):
        parts = line.split()
        if len(parts) == 7 and parts[0] == "bounds":
            lo = [float(v) for v in parts[1:4]]
            hi = [float(v) for v in parts[4:7]]
            z = hi[2] - 2.0 if hi[2] - lo[2] > 4.0 else (lo[2] + hi[2]) / 2
            return ["%.2f" % ((lo[0] + hi[0]) / 2), "%.2f" % ((lo[1] + hi[1]) / 2), "%.2f" % z]
    return None


def waters():
    # the active waters only; the area that holds them (a30 next to Torvus's a24) loads on the
    # wall clock, so a slow (gdb) run polls for it
    out = []
    for _ in range(90):
        for line in cmd("objs water").split("\n"):
            if "CScriptWater" in line and "inactive" not in line:
                out.append(line.split()[0])
        if out:
            return out
        time.sleep(1.0)
    return out


def watch(target, frames, shots):
    start = frame()
    step = frames // (shots + 1)
    for i in range(shots):
        wait_frame(start + step * (i + 1))
        log("damage corpse: %s" % cmd("damage %s 5" % target))
    wait_frame(start + frames)
    log("obj: %s" % cmd("obj %s" % target).replace("\n", " | ")[:400])


def main():
    wait_frame(200)
    ids = []
    for _ in range(120):
        ids = commandos()
        if ids:
            break
        time.sleep(1.0)
    log("commandos %s (frame %d)" % (ids, frame()))
    ws = waters()
    log("waters %s" % ws)
    for w in ws:
        log("water %s: %s" % (w, cmd("obj %s" % w).replace("\n", " | ")[:400]))
    drown = None
    for w in ws:
        if position(w) is not None:
            drown = w
            break
    for n, target in enumerate(ids):
        log("=== commando %s ===" % target)
        log("send Activate: %s" % cmd("send %s Activate" % target))
        wait_frame(frame() + 30)
        if n == 1 and drown is not None:
            pos = position(drown)
            log("place in water %s: %s" % (drown, cmd("place %s %s %s %s" % (target, pos[0], pos[1], pos[2]))))
            wait_frame(frame() + 60)
            log("state: %s" % state_of(target))
            watch(target, 1500, 2)
            continue
        log("place: %s" % cmd("place %s player 8" % target))
        wait_frame(frame() + 30)
        log("kill: %s" % cmd("kill %s" % target))
        watch(target, 1500, 4)
    log("objs: %s" % cmd("objs commando"))


if __name__ == "__main__":
    main()
