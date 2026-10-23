#!/usr/bin/env python3
"""DarkSamus smoke driver: activate layer 6 of 10_sand_alamostorage, find the CDarkSamus
actor (exact class, not CDarkSamusBattleStage), `send <uid> Activate` it and its first
BattleStage, damage it a few times, then poll its FSM/hp/position.

usage: darksamus_con.py <port> [seconds] [game log]
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1])
DUR = min(float(sys.argv[2]) if len(sys.argv) > 2 else 45.0, 45.0)
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


LINE_RE = re.compile(r"^\s*(u[0-9a-f]{4})\s+[0-9a-f]{8}\s+(\S+)\s+a(\d+)(?:\s+\(([^)]*)\))?(.*)$")


def find_uids():
    uid = None
    battle = None
    for _ in range(40):
        ob = cmd("objs darksamus")
        for line in ob.splitlines():
            m = LINE_RE.match(line)
            if not m:
                continue
            candidate_uid, cls = m.group(1), m.group(2)
            if cls == "CDarkSamus" and uid is None:
                uid = candidate_uid
            elif cls == "CDarkSamusBattleStage" and battle is None:
                battle = candidate_uid
        if uid is not None:
            return uid, battle
        time.sleep(2.0)
    return uid, battle


T0 = time.time()
wait_for_area()
log("layer 6 -> %r" % cmd("layer 6").strip().splitlines()[-1:])

uid, battle = find_uids()
if uid is None:
    log("no CDarkSamus found")
    sys.exit(1)
log("found uid=%s battle=%s" % (uid, battle))

if battle is not None:
    r = cmd("send %s Activate" % battle)
    log("send %s Activate -> %r" % (battle, r.strip().splitlines()[-1:]))
r = cmd("send %s Activate" % uid)
log("send %s Activate -> %r" % (uid, r.strip().splitlines()[-1:]))

states = []
hp = None
next_damage = 10.0
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
        pos = re.search(r"\(([-\d.]+), ([-\d.]+), ([-\d.]+)\)", ob and ob or "")
        if time.time() - T0 >= next_damage and next_damage <= 30.0:
            dr = cmd("damage %s 100" % uid)
            log("damage %s 100 -> %r" % (uid, dr.strip().splitlines()[-1:]))
            next_damage += 10.0
        log("fsm=%-18s hp=%-10s pos=%s" % (st.group(1) if st else "?", hp or "?",
                                             pos.group(0) if pos else "?"))
    except OSError as e:
        log("console gone: %s" % e)
        break
    time.sleep(3.0)

log("states seen: %s" % sorted(set(states)))
log("counts: %s" % {s: states.count(s) for s in sorted(set(states))})
log("done")
