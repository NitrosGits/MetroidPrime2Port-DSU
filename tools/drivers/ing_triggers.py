#!/usr/bin/env python3
"""Ing trigger/state arms driver (lane ing3): the arms a fight rarely draws, through the console.

The Agon Wastes 01_sand_entrance_dark (MP2_WORLD="42b935e4,01_sand_entrance_dark"). Every Ing is
woken with `Activate` (once), then, on the first Ing:

  * ShouldFleeSafeZone (fn_29_AA94): `send XENZ`/`XEXZ` move the safe zone count (0xe2c) 0 -> 1 -> 0;
  * FoundMovementPos (fn_29_A4E0): `var ing.found 0/1` (0xe5c bit 3);
  * ShouldBecomeIngSpot (fn_29_AA04) / ShouldBecomeCorporeal (fn_29_AA4C): `var ing.desired` 3/2/-1
    with `var ing.form` 3/2 - every arm of both;
  * StartRangedAttack (fn_29_5190): `codecall`;
  * TakeDamage (fn_29_B890): `var ing.form` 0, 1, 5 and 2, each followed by a `hurt` on one of the
    Ing's joint collision actors (the joint relays kSM_Damage -> CollisionActorDamage ->
    TakeDamage); the form is put back after each hit;
  * Evaporate (fn_29_5C28): `fsm Evaporate` on the last Ing, then four seconds for the state
    time's two-second arm (the death delete).

Each forced value is printed, so the report can label the arms as forced (`var`/`fsm`/`send`).

usage: ing_triggers.py <port> <game log>
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1])
LOGPATH = sys.argv[2]


def cmd(c):
    s = None
    for _ in range(200):
        try:
            s = socket.create_connection(("127.0.0.1", PORT), timeout=10)
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
    return buf.decode(errors="replace").strip()


def run(c):
    out = cmd(c)
    print(f"> {c}\n{out}", flush=True)
    return out


def objs(cls):
    found = []
    for line in cmd(f"objs {cls}").splitlines():
        m = re.match(r"\s*(u[0-9a-f]{4})\s.*\(([-0-9.e]+),\s*([-0-9.e]+),\s*([-0-9.e]+)\)", line)
        if m and cls in line:
            found.append((m.group(1), float(m.group(2)), float(m.group(3)), float(m.group(4))))
    return found


def joints(ing, radius=5.0):
    return [j for j in objs("CCollisionActor")
            if sum((a - b) ** 2 for a, b in zip(j[1:], ing[1:])) <= radius * radius]


def var(uid, name, value=None):
    out = run(f"var {uid} {name}" + ("" if value is None else f" {value}"))
    m = re.search(r"=\s*(-?\d+)", out)
    return int(m.group(1)) if m else None


for _ in range(400):
    try:
        if "gameplay reached" in open(LOGPATH, errors="replace").read():
            break
    except OSError:
        pass
    time.sleep(1)
time.sleep(3)

ings = []
for _ in range(60):
    ings = objs("CIng")
    if ings:
        break
    time.sleep(1)
print("== ings", ings, flush=True)
for i in ings:
    run(f"send {i[0]} Activate")
time.sleep(3)
ing = ings[0][0]

print("== AA94 ShouldFleeSafeZone", flush=True)
run(f"trigcall {ing} ShouldFleeSafeZone")
run(f"send {ing} XENZ")
run(f"trigcall {ing} ShouldFleeSafeZone")
run(f"send {ing} XEXZ")
run(f"trigcall {ing} ShouldFleeSafeZone")

print("== A4E0 FoundMovementPos", flush=True)
found = var(ing, "ing.found")
for v in (0, 1):
    var(ing, "ing.found", v)
    run(f"trigcall {ing} FoundMovementPos")
var(ing, "ing.found", found if found is not None else 0)

print("== AA04/AA4C ShouldBecomeIngSpot/ShouldBecomeCorporeal", flush=True)
form = var(ing, "ing.form")
desired = var(ing, "ing.desired")
for d, f in ((-1, 2), (3, 2), (3, 3), (2, 3), (2, 2)):
    var(ing, "ing.desired", d)
    var(ing, "ing.form", f)
    run(f"trigcall {ing} ShouldBecomeIngSpot")
    run(f"trigcall {ing} ShouldBecomeCorporeal")
var(ing, "ing.form", form if form is not None else 2)
var(ing, "ing.desired", desired if desired is not None else -1)

print("== 5190 StartRangedAttack", flush=True)
run(f"codecall {ing} StartRangedAttack")
run(f"codecall {ing} EndRangedAttack")

print("== B890 TakeDamage", flush=True)
for f in (0, 1, 5, 2, 3):
    pos = [i for i in objs("CIng") if i[0] == ing]
    js = joints(pos[0]) if pos else []
    if not js:
        print("   no joint near", ing, flush=True)
        time.sleep(1)
        continue
    form = var(ing, "ing.form")
    var(ing, "ing.form", f)
    run(f"hurt {js[0][0]} 1")
    var(ing, "ing.form", form if form is not None else 2)
    time.sleep(0.5)

print("== 5C28 Evaporate", flush=True)
last = ings[-1][0]
run(f"fsm {last} Evaporate")
for t in range(8):
    time.sleep(0.5)
    print(f"   t{t} ings", [i[0] for i in objs("CIng")], flush=True)
time.sleep(2)
print("== covdump", cmd("covdump"), flush=True)
print("== done", flush=True)
