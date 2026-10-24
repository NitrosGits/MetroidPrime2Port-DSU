#!/usr/bin/env python3
"""Ing smoke driver: give the light beam, wake the Ing and shoot it from three distances.

usage: ing_con.py <game log>   (the console port comes from MP2_CONSOLE)
"""
import socket, sys, time, re, math

import os
PORT = int(os.environ.get("MP2_CONSOLE", "47611"))

def cmd(c):
    s = None
    for _ in range(200):
        try:
            s = socket.create_connection(("127.0.0.1", PORT), timeout=5)
            break
        except OSError:
            time.sleep(1)
    if s is None:
        return ""
    try:
        s.sendall((c + "\n").encode())
        buf = b""
        while b"=> " not in buf:
            d = s.recv(65536)
            if not d:
                break
            buf += d
    except OSError:
        return ""
    finally:
        s.close()
    return buf.decode(errors="replace")

def objs(cls):
    o = cmd(f"objs {cls}")
    rows = []
    for line in o.split("\n"):
        m = re.match(r"\s*(u[0-9a-f]{4})\s+(\S+)\s+(\S+)\s+a(\d+)(?:\s+\((-?[\d.]+), (-?[\d.]+), (-?[\d.]+)\))?", line)
        if m and m.group(5):
            rows.append({"uid": m.group(1), "cls": m.group(3),
                         "x": float(m.group(5)), "y": float(m.group(6)), "z": float(m.group(7)),
                         "inactive": "inactive" in line})
    return rows

def find(uid):
    for r in objs("CIng"):
        if r["uid"] == uid:
            return r
    return None

def hp(uid):
    o = cmd(f"obj {uid}")
    m = re.search(r"hp ([\d.]+)/([\d.]+)", o)
    return None if m is None else (float(m.group(1)), float(m.group(2)))

def log(*a):
    print(*a, flush=True)

logpath = sys.argv[1]
for _ in range(300):
    try:
        if "gameplay reached" in open(logpath, errors="replace").read():
            break
    except OSError:
        pass
    time.sleep(1)
time.sleep(3)

# The beam switch first, at the spawn point and with nothing else in flight: a switch made while the
# player is teleported into the level may never finish, and an unfinished switch blocks fire.
log("== give", cmd("give lightBeam 1") + cmd("give lightAmmo 50").strip())
log("== beam", cmd("beam light").strip())
time.sleep(3.0)
log("== beam now", cmd("beam").strip())
log("== shots in flight before", len(objs("CEnergyProjectile")))

ings = objs("CIng")
log("== ings", [(i["uid"], i["x"], i["y"], i["z"], i["inactive"]) for i in ings])
for i in ings:
    cmd(f"send {i['uid']} Activate")
TARGET = ings[0]["uid"]

def stand(dx, dy, dz):
    t = find(TARGET)
    if t is None:
        return None
    sx, sy, sz = t["x"] + dx, t["y"] + dy, t["z"] + dz
    cmd(f"tp {sx:.1f} {sy:.1f} {sz:.1f}")
    yaw = math.degrees(math.atan2(t["x"] - sx, t["y"] - sy))
    cmd(f"face {yaw:.1f}")
    return t

def shoot(tag, shots=4):
    log(f"== {tag} hp {hp(TARGET)}")
    for _ in range(shots):
        cmd("press a 10")
        time.sleep(0.4)
        log(f"== {tag} flight {len(objs('CEnergyProjectile'))}")
    time.sleep(0.5)
    log(f"== {tag} -> hp {hp(TARGET)}")

t = stand(0.0, -6.0, 0.0)
log("== A ing", t)
shoot("A-light-corp-6u", 6)

# Same place with the power beam, as the control that the harness itself still fires.
log("== beam power", cmd("beam power").strip())
time.sleep(3.0)
shoot("B-power-corp-6u", 3)
log("== beam light", cmd("beam light").strip())
time.sleep(3.0)

# Puddle: stand on it so the muzzle starts inside the touch box.
for _ in range(120):
    try:
        txt = open(logpath, errors="replace").read()
    except OSError:
        txt = ""
    if f"[ing] {TARGET[1:]} " in txt and "form 3" in txt:
        break
    time.sleep(1)
log("== C form 3 seen")
t = stand(0.0, -1.5, -0.8)
log("== C ing", t)
shoot("C-light-spot", 4)

log("== D damage 5, hp before", hp(TARGET))
log(cmd(f"damage {TARGET} 5").strip())
time.sleep(1)
log("== D damage, hp after", hp(TARGET))

# --- appended checks: fsm change, kill ---
def _state(uid):
    o = cmd(f"obj {uid}")
    m = re.search(r"fsm (.+)", o)
    return (m.group(1).strip() if m else None), o

prev_state, _ = _state(TARGET)
log("== fsm change: state before %s" % prev_state)
log(cmd(f"damage {TARGET} 5").strip())
time.sleep(1)
changed = False
for _ in range(10):
    time.sleep(0.5)
    st, _ = _state(TARGET)
    if st is not None and st not in ("-", prev_state):
        log("== fsm change: natural %s -> %s" % (prev_state, st))
        changed = True
        break
if not changed:
    log("== fsm change: no natural change; forcing fsm %s Patrol" % TARGET)
    log(cmd(f"fsm {TARGET} Patrol").strip())
    time.sleep(1)
    st, _ = _state(TARGET)
    log("== fsm change: after force %s" % st)

before = len(objs("CIng"))
log("== kill: objs CIng before %d" % before)
log(cmd(f"kill {TARGET}").strip())
# poll for natural drop up to 10s; always log obj at 1s and 5s
after = before
natural_t = None
logged1 = False
logged5 = False
for i in range(20):
    time.sleep(0.5)
    elapsed = (i + 1) * 0.5
    if not logged1 and elapsed >= 1.0:
        log("== kill: obj at 1s: %s" % cmd(f"obj {TARGET}").strip().replace("\n", " "))
        logged1 = True
    if not logged5 and elapsed >= 5.0:
        log("== kill: obj at 5s: %s" % cmd(f"obj {TARGET}").strip().replace("\n", " "))
        logged5 = True
    after = len(objs("CIng"))
    if after < before and natural_t is None:
        natural_t = elapsed
        log("== kill: natural drop after %.1fs" % natural_t)
if not logged1:
    log("== kill: obj at 1s: %s" % cmd(f"obj {TARGET}").strip().replace("\n", " "))
if not logged5:
    log("== kill: obj at 5s: %s" % cmd(f"obj {TARGET}").strip().replace("\n", " "))
if natural_t is None:
    log("== kill: NO natural drop, forcing fsm %s Dead" % TARGET)
    log(cmd(f"fsm {TARGET} Dead").strip())
    after = len(objs("CIng"))
    for _ in range(12):
        time.sleep(0.5)
        after = len(objs("CIng"))
        if after < before:
            break
log("== kill: objs CIng after %d" % after)
log("== done")
