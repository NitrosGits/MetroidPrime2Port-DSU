#!/usr/bin/env python3
"""SwampBossStage2 (module 79): the fight's grapple points, woken the way the shipped area does it.

usage: swampboss2_grapple.py <port> [--frames=<n>]

In 04_swamp_temple_dark (area 32) the three grapple points (GRAP 1820006a/6d/6f, layer 6) start
inactive. The fight's start switch (SWTC 00200232, OPEN) fires relay SRLY 00200235 (layer 0), whose
ZERO goes to relay 18200260 (ACTV -> the three GRAPs and three PLATs) and relay 18200265 (ACTV -> the
boss SBS2 182002ca). So the console's `grapple <id>` (which wakes a point itself) is not needed: this
driver sends that relay SetToZero, checks the points are active, gives the grapple beam and then
lets `grapple <point>` put the player in front of the first point and hold L.
"""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from swampboss2_chain2 import Console, find_boss  # noqa: E402

START_RELAY = 0x00200235


def find_editor(con, cls, editor):
    """The uid of the object of class filter `cls` with that editor id, or None."""
    lines, _ = con.cmd("objs " + cls)
    for line in lines:
        f = line.split()
        if len(f) >= 2 and f[0].startswith("u") and int(f[1], 16) == editor:
            return f[0]
    return None


def main():
    port = int(sys.argv[1])
    frames = 600
    for a in sys.argv[2:]:
        if a.startswith("--frames="):
            frames = int(a.split("=")[1])
    con = None
    for _ in range(600):  # the console listens only once the game is up
        try:
            con = Console(port)
            break
        except OSError:
            time.sleep(0.5)
    if con is None:
        print("[sb2g] no console on port", port, flush=True)
        return 1
    con.wait_frame(400)
    print("[sb2g] layer 6 ->", con.cmd("layer 6")[1], flush=True)
    boss = find_boss(con)
    print("[sb2g] boss", boss, flush=True)
    relay = find_editor(con, "relay", START_RELAY)
    print("[sb2g] start relay", relay, flush=True)
    if relay is None:
        return 1
    lines, end = con.cmd("send %s SetToZero" % relay)
    print("[sb2g] send ->", end, *lines, flush=True)
    con.wait_frame(con.frame() + 10)
    lines, _ = con.cmd("objs grapple")
    print("\n".join(lines), flush=True)
    points = [l.split()[0] for l in lines if l.split() and l.split()[0].startswith("u")]
    print("[sb2g]", con.cmd("give grappleBeam 1")[0], flush=True)
    if not points:
        return 1
    print("[sb2g]", *con.cmd("grapple %s" % points[0])[0], flush=True)
    start = con.frame()
    while con.frame() < start + frames:
        lines, _ = con.cmd("status")
        print("[sb2g] f%d" % con.frame(), *[l.strip() for l in lines if "position" in l or "orbit" in l],
              flush=True)
        con.wait_frame(con.frame() + 40)
        time.sleep(0.01)
    con.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
