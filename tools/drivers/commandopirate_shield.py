#!/usr/bin/env python3
"""CommandoPirate driver: fight the commando in area 59 and force the states the area's own
state machine cannot reach.

The driver's steps are timed in game updates (the console's `frame`), not in wall seconds, so
the script works with the fast harness runs (MP2_TICKS_PER_FRAME).

usage: commandopirate_shield.py <port> [outfile]

The scenario:
  * the player is teleported 13 units from the commando - the commando's block puts its shield
    charge band at 10 to 20 units, and the commando walks to about two units, so the charge only
    starts while it approaches;
  * the player aims at the commando (the console's `aim` with the yaw and the pitch) and fires
    (the console's `press a`);
  * the ArmShield state is forced with the console's `fsm`, because area 59's own state machine
    gives the ShouldArmShield trigger no target state.
"""
import math
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4791
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout


def log(msg):
    OUT.write(msg + "\n")
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


def commando():
    cid = None
    pos = None
    for line in cmd("objs commando").split("\n"):
        if "CCommandoPirate" in line and "(" in line:
            cid = line.split()[0]
            try:
                pos = [float(x) for x in line[line.index("(") + 1:line.rindex(")")].split(",")]
            except ValueError:
                pass
    return cid, pos


def player():
    for line in cmd("status").split("\n"):
        if line.startswith("position "):
            return [float(x) for x in line.split()[1:4]]
    return None


def wait_frame(target):
    while True:
        f = frame()
        if f < 0 or f >= target:
            return f
        time.sleep(0.2)


def aim_and_fire(cid, pos, rounds=4):
    p = player()
    if p is None or pos is None:
        return
    dx = pos[0] - p[0]
    dy = pos[1] - p[1]
    dz = pos[2] - p[2]
    flat = math.hypot(dx, dy)
    yaw = math.degrees(math.atan2(dx, dy))
    pitch = math.degrees(math.atan2(dz, flat)) if flat > 0.0 else 0.0
    cmd("aim %.1f %.1f" % (yaw, pitch))
    cmd("face %.1f" % yaw)
    for _ in range(rounds):
        cmd("press a 6")
        time.sleep(0.4)


def main():
    wait_frame(300)
    cid, pos = commando()
    log("commando %s at %s" % (cid, pos))
    if pos is None:
        return
    # stand 13 units from the commando: the charge band is 10 to 20 units
    p = player()
    if p is not None:
        dx = p[0] - pos[0]
        dy = p[1] - pos[1]
        d = math.hypot(dx, dy)
        if d < 0.1:
            dx, dy, d = 1.0, 0.0, 1.0
        cmd("tp %.2f %.2f %.2f" % (pos[0] + dx / d * 13.0, pos[1] + dy / d * 13.0, pos[2]))
        log("tp %.2f %.2f %.2f" % (pos[0] + dx / d * 13.0, pos[1] + dy / d * 13.0, pos[2]))
    # fight: the commando charges, the shots land. The commando walks to about two units, so the
    # player steps back to thirteen every round: that keeps the commando crossing the charge band
    # (10 to 20 units) until its aggressiveness roll wins.
    for _ in range(10):
        wait_frame(frame() + 40)
        cid, pos = commando()
        p = player()
        if pos is not None and p is not None:
            d = math.hypot(pos[0] - p[0], pos[1] - p[1])
            if d < 9.0:
                dx = (p[0] - pos[0]) / max(d, 0.001)
                dy = (p[1] - pos[1]) / max(d, 0.001)
                cmd("tp %.2f %.2f %.2f" % (pos[0] + dx * 13.0, pos[1] + dy * 13.0, pos[2]))
                log("step back to 13 units (d=%.1f)" % d)
        aim_and_fire(cid, pos)
        log("round: commando %s" % (pos,))
    # the ArmShield state: area 59's asset gives the trigger no target, so force it
    log("fsm ArmShield: %s" % cmd("fsm %s ArmShield" % cid))
    for _ in range(4):
        wait_frame(frame() + 40)
        cid, pos = commando()
        aim_and_fire(cid, pos)
    log("frame %d" % frame())


if __name__ == "__main__":
    main()
