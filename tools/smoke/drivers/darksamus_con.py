#!/usr/bin/env python3
"""DarkSamus smoke driver: activate layer 6 of 10_sand_alamostorage, find the CDarkSamus,
`send <uid> Activate` it, then poll its FSM/hp/position.

usage: darksamus_con.py <port> [seconds] [game log]
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1])
DUR = float(sys.argv[2]) if len(sys.argv) > 2 else 100.0
LOG = sys.argv[3] if len(sys.argv) > 3 else None
S = None


def cmd(c, timeout=30):
    global S
    if S is None:
        for _ in range(180):
            try:
                S = socket.create_connection(("127.0.0.1", PORT), timeout=timeout)
                break
            except OSError:
                time.sleep(1)
        if S is None:
            raise OSError("no console")
    try:
        S.sendall((c + "\n").encode())
        buf = b""
        while b"=> " not in buf:
            d = S.recv(65536)
            if not d:
                break
            buf += d
        return buf.decode(errors="replace")
    except OSError:
        S = None
        raise


def log(m):
    print("[ds %6.1fs] %s" % (time.time() - T0, m), flush=True)


def wait_for_area(timeout=300.0):
    """The console command runs in the game loop, and the area becomes current a few frames after
    the warp: a "layer 6" sent too early lands in the area the game came from."""
    if not LOG:
        return
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with open(LOG, errors="replace") as fh:
                if "gameplay reached" in fh.read():
                    return
        except OSError:
            pass
        time.sleep(1.0)


T0 = time.time()
wait_for_area()
log("layer 6 -> %r" % cmd("layer 6").strip().splitlines()[-1:])

uid = None
for _ in range(40):
    ob = cmd("objs darksamus")
    m = re.search(r"(u[0-9a-f]{4})\s+(\S+)\s+(\S+)\s+a(\d+)(?:\s+\(([-\d.]+), ([-\d.]+), ([-\d.]+)\))?(.*)", ob)
    if m:
        uid = m.group(1)
        log("found %s class=%s area=%s pos=(%s, %s, %s)%s" %
            (uid, m.group(3), m.group(4), m.group(5), m.group(6), m.group(7), m.group(8).strip()))
        break
    time.sleep(2)

if not uid:
    log("no CDarkSamus found")
    sys.exit(1)

r = cmd("send %s Activate" % uid)
log("send %s Activate -> %r" % (uid, r.strip().splitlines()[-1:]))

states = []
hp = None
while time.time() - T0 < DUR:
    try:
        o = cmd("obj %s" % uid)
        st = re.search(r"fsm ([A-Za-z0-9_]+)", o)
        h = re.search(r"hp ([\d.]+)/([\d.]+)", o)
        if st:
            states.append(st.group(1))
        if h:
            hp = h.group(1) + "/" + h.group(2)
        ob = cmd("objs darksamus")
        pos = re.search(r"pos=\(([-\d.]+), ([-\d.]+), ([-\d.]+)\)", ob)
        log("fsm=%-12s hp=%-12s pos=%s" % (st.group(1) if st else "?", hp or "?",
                                           pos.group(0) if pos else "?"))
    except OSError as e:
        log("console gone: %s" % e)
        break
    time.sleep(3.0)

log("states seen: %s" % sorted(set(states)))
log("counts: %s" % {s: states.count(s) for s in sorted(set(states))})
log("done")
