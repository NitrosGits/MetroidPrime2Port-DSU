#!/usr/bin/env python3
"""CommandoPirate driver: the core states (PortCCommandoPirateCore.cpp) one after another.

Every commando of the area is woken (`send <id> Activate`, the layer script does not run in this
harness), the player is put next to it (`place`) so the module's aim helper fn_9_3CBC finds a
target, and the states are forced with `fsm` - nothing in the shipped flow enters most of them in
these areas. Listen runs from the console's `noise` at chosen offsets. With `cover` (the Swamp's
05_swamp_timber!, the only area with cover points near commandos) the commando is put on a cover
point first, so CoverFind holds it and the cover states take their cover arms.

usage: commandopirate_core.py <port> [outfile] [cover]
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout
COVER = len(sys.argv) > 3 and sys.argv[3] == "cover"


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
# A world load under gdb (a forcing run) can hold the console for longer than the connect timeout.
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
    target = frame() + n
    while True:
        f = frame()
        if f < 0 or f >= target:
            return f
        time.sleep(0.2)


def ids_of(what, cls):
    return [l.split()[0] for l in cmd("objs %s" % what).split("\n") if cls in l and "(" in l]


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


def noises(cid):
    p = pos_of(cid)
    if p is None:
        return
    x, y, z = p
    for typ, dx, dz in ((4, 2, 0), (4, 60, 0), (0, 2, 0), (0, 2, 30), (0, 200, 0), (1, 2, 0)):
        log("noise %d +%g/+%g: %s" % (typ, dx, dz, cmd("noise %d %f %f %f" % (typ, x + dx, y, z + dz))))
        wait(2)


def main():
    wait(200)
    ids = ids_of("commando", "CCommandoPirate")
    log("commandos %s (frame %d)" % (ids, frame()))
    covers = ids_of("cover", "CScriptCoverPoint") if COVER else []
    log("cover points %s" % covers)
    for n, cid in enumerate(ids):
        log("=== commando %s ===" % cid)
        step(cid, "send %s Activate", 30)
        step(cid, "place %s player 5", 30)
        noises(cid)
        if COVER:
            for cov in covers:
                p = pos_of(cov)
                log("cover %s at %s" % (cov, p))
                if p is None:
                    continue
                step(cid, "place %%s %f %f %f" % p, 10)
                step(cid, "place %s player 6", 10)
                step(cid, "fsm %s CoverFind", 10)
                step(cid, "fsm %s Crouch", 40)
                step(cid, "fsm %s Cover", 60)
                step(cid, "fsm %s CoverAttack", 60)
                step(cid, "fsm %s Lurk", 10)
            continue
        step(cid, "fsm %s SelectTarget", 10)
        step(cid, "fsm %s EGrenadeAttack", 60)
        step(cid, "fsm %s PostEGrenadeAttack", 200)
        step(cid, "fsm %s MeleeAttack", 20)
        step(cid, "fsm %s JumpPointFind", 5)
        step(cid, "fsm %s JetBoost", 120)
        step(cid, "fsm %s Crouch", 20)
        step(cid, "fsm %s Cover", 20)
        step(cid, "fsm %s CoverAttack", 20)
        step(cid, "fsm %s WarpIn", 240)
        step(cid, "fsm %s Lurk", 20)
        step(cid, "fsm %s GetUp", 60)
        step(cid, "fsm %s ArmShield", 40)
        step(cid, "damage %s 1", 30)
        step(cid, "fsm %s Lurk", 20)
        step(cid, "damage %s 1 20", 60)
        step(cid, "fsm %s GetUp", 60)
        if n == len(ids) - 1:
            step(cid, "kill %s", 60)
            noises(cid)
    log("objs: %s" % cmd("objs commando"))


if __name__ == "__main__":
    main()
