#!/usr/bin/env python3
"""Boost Guardian arm driver: reach the states whose bodies this slice checks.

usage: bg_arms.py <port> [secs] [guardian id] [log]

The Boost Guardian (u052b in area 28) walks its own FSM; the driver puts the player into
its detection range, then forces the states whose arms the slice's branch table lists
(face the target, find a cover point, seek a wall point, the two path finds) and prints what
the guardian reports. The run's log is where the port's own probe lines (the temporary
MP2_BG_ARMS patch) counted the arms; without that patch the driver only reaches the states.

The guardian takes a turn command only in its corporeal form and only when the target is
more than 20 degrees off its forward axis (and inside its 80 degree detection cone), so the
driver reads the guardian's forward axis from the log and puts the player 30 degrees off it.
"""
import math
import re
import socket
import sys
import time

port = int(sys.argv[1]) if len(sys.argv) > 1 else 47292
secs = float(sys.argv[2]) if len(sys.argv) > 2 else 120.0
gid = sys.argv[3] if len(sys.argv) > 3 else ""
logp = sys.argv[4] if len(sys.argv) > 4 else ""

t0 = time.time()
s = None
while s is None and time.time() - t0 < 120:
    try:
        s = socket.create_connection(("127.0.0.1", port), timeout=5)
    except OSError:
        time.sleep(0.5)
if s is None:
    print("[drv] no connect")
    sys.exit(1)
f = s.makefile("rw")


def cmd(c, quiet=False):
    f.write(c + "\n")
    f.flush()
    out = []
    s.settimeout(30)
    try:
        while True:
            line = f.readline()
            if not line:
                break
            out.append(line.rstrip())
            if line.startswith("=> "):
                break
    except OSError:
        out.append("[timeout]")
    if not quiet:
        print(f"[drv {time.time() - t0:6.1f}s] > {c}")
        for line in out:
            print("   ", line)
        sys.stdout.flush()
    return out


def logtext():
    try:
        with open(logp, errors="replace") as fh:
            return fh.read()
    except OSError:
        return ""


# Wait for the guardian, then read its unique id out of `objs boost`.
while not gid and time.time() - t0 < 90:
    out = cmd("objs boost", quiet=True)
    m = re.search(r"\bu([0-9a-f]{4})\b.*BoostBallGuardian", " ".join(out))
    if m:
        gid = "u" + m.group(1)
        break
    time.sleep(2)
if not gid:
    print("[drv] no guardian in the object list")
    sys.exit(2)
print(f"[drv {time.time() - t0:6.1f}s] guardian {gid}")

# The guardian's forward axis, from the Lurk state's probe line, and the spot 30 degrees off
# it (inside the 80 degree detection cone, outside the 20 degree facing cone).
spot = None
deadline = time.time() + 60
while time.time() < deadline and spot is None:
    m = re.search(r"\[armf\] lurk fwd (\S+) (\S+) (\S+) pos (\S+) (\S+) (\S+)", logtext())
    if m:
        fx, fy = float(m.group(1)), float(m.group(2))
        px, py, pz = float(m.group(4)), float(m.group(5)), float(m.group(6))
        a = math.radians(30)
        dx = fx * math.cos(a) - fy * math.sin(a)
        dy = fx * math.sin(a) + fy * math.cos(a)
        spot = (px + dx * 40, py + dy * 40, pz + 3)
    time.sleep(1)
if spot is None:
    print("[drv] no forward axis in the log")
    spot = (14.9, 290.0, 58.0)
print(f"[drv {time.time() - t0:6.1f}s] player spot {spot[0]:.1f} {spot[1]:.1f} {spot[2]:.1f}")
cmd("tp %.1f %.1f %.1f" % spot)

# The states the slice's branch table lists, in the order the machine can reach them.
STATES = [
    ("SelectTarget", 3),
    ("Alert", 3),
    ("FaceTarget", 5),
    ("Lurk", 3),
    ("PathFind", 4),
    ("FindCoverPoint", 5),
    ("SeekWallPoint", 5),
    ("IngSpotPathFind", 5),
    ("IngSpotPointPathFind", 5),
    ("Taunt", 3),
    ("BecomeIngSpot", 5),
    ("BecomeCorporeal", 5),
    ("FollowAttackPattern", 4),
    ("ArmSwipe", 4),
    ("FindMiniPortals", 4),
]

round_no = 0
while time.time() - t0 < secs:
    # A real beam shot: it damages the guardian, which starts the hit-damage timer.
    cmd(f"shoot {gid}", quiet=True)
    for name, hold in STATES:
        if time.time() - t0 > secs:
            break
        cmd(f"fsm {gid} {name}", quiet=True)
        time.sleep(hold)
    cmd(f"obj {gid}")
    round_no += 1

print(f"[drv] done at {time.time() - t0:.1f}s, {round_no} rounds")
