"""Ing lane ing30 driver: the arms of fn_29_51D0, 5714, 8FE8 and B534 that ing_rows.py left open.

The Agon Wastes 01_sand_entrance_dark (MP2_WORLD="42b935e4,01_sand_entrance_dark"), the first Ing,
under tools/cov/force.sh with tools/cov/specs/ing30_force.py (the gdb side cycles the point-search
forcings per call and writes the surface triangle for 8FE8):

  * 51D0 / 5714: `codecall SetLuredPointDest|SetWallPointCoverDest` with each safe zone / hint, many
    times, so every forcing mode of the spec (natural, first query, second query, Search) comes up;
  * 5714 also with no hint (ing.coverhint 0xffff);
  * 8FE8: `fsm BecomeWallProjectile` (Activate on a forced floor or wall triangle, alternately),
    `bodystate 9` (kAS_LoopAttack) and, in one tick each, the Update with no target, with a
    target 10 units along +x (the forced wall normal), with form 4 and with take-off; then, the
    animation state over and `bodystate 5`, the Update with neither arm; then messages 3 and -1;
  * B534: `fsm BecomeBodyProjectile` near the player (the spec zeroes the first contact's material).

usage: ing30.py <port> <game log>
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1])
LOGPATH = sys.argv[2]


def cmd(c):
    want = c.count(";") + 1
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
        while buf.count(b"=> ") < want:
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
        m = re.match(r"\s*u([0-9a-f]{4})\s.*\(([-0-9.e]+),\s*([-0-9.e]+),\s*([-0-9.e]+)\)", line)
        if m and cls in line:
            found.append(("u" + m.group(1), float(m.group(2)), float(m.group(3)),
                          float(m.group(4))))
    return found


def packed(uid):
    return int(uid[1:], 16)


def pos_of(uid, cls="CIng"):
    for o in objs(cls):
        if o[0] == uid:
            return o[1:]
    return None


INVALID = 0xFFFF

for _ in range(600):
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
hints = objs("CScriptAIHint")
zones = objs("CScriptSafeZone")
print("== hints", len(hints), "zones", len(zones), flush=True)

print("== 51D0 SetLuredPointDest", flush=True)
for rnd in range(3):
    for z in zones:
        run(f"var {ing} ing.lurezone {packed(z[0])}; codecall {ing} SetLuredPointDest; var {ing} ing.found")
    # the Ing next to each zone, so the lure mesh point (fn_29_1C98) and a path to it exist
    for z in zones[:8]:
        run(f"place {ing} {z[1]:.2f} {z[2]:.2f} {z[3]:.2f}")
        for _ in range(4):
            run(f"var {ing} ing.lurezone {packed(z[0])}; codecall {ing} SetLuredPointDest; var {ing} ing.found")

print("== 5714 SetWallPointCoverDest", flush=True)
for h in hints[:24]:
    for _ in range(4):
        run(f"var {ing} ing.coverhint {packed(h[0])}; codecall {ing} SetWallPointCoverDest; var {ing} ing.found")
# no hint (fn_29_45BC returns null, asm 0x5748 beq)
for _ in range(2):
    run(f"var {ing} ing.coverhint {INVALID}; codecall {ing} SetWallPointCoverDest; var {ing} ing.found")
run(f"var {ing} ing.lurezone {INVALID}")
run(f"place {ing} player 8")
time.sleep(1)

print("== 8FE8 BecomeWallProjectile", flush=True)
form = None
m = re.search(r"=\s*(-?\d+)", run(f"var {ing} ing.form"))
if m:
    form = int(m.group(1))
target = hints[0][0]
for rnd in range(3):
    run(f"fsm {ing} BecomeWallProjectile")
    time.sleep(0.5)
    p = pos_of(ing)
    if p:
        run(f"place {target} {p[0] + 10:.2f} {p[1]:.2f} {p[2]:.2f}")
    run(f"bodystate {ing} 9")
    run(f"var {ing} ing.target {INVALID}; statecall {ing} BecomeWallProjectile 1 0.016")
    run(f"bodystate {ing} 9")
    run(f"var {ing} ing.target {packed(target)}; statecall {ing} BecomeWallProjectile 1 0.016")
    run(f"bodystate {ing} 9")
    run(f"var {ing} ing.form 4; statecall {ing} BecomeWallProjectile 1 0.016; var {ing} ing.form 3")
    run(f"bodystate {ing} 9")
    run(f"var {ing} ing.takeoff 1; statecall {ing} BecomeWallProjectile 1 0.016; var {ing} ing.takeoff 0")
    # the animation state is now over and the body leaves the loop attack: neither Update arm
    run(f"bodystate {ing} 5; statecall {ing} BecomeWallProjectile 1 0.016")
    run(f"statecall {ing} BecomeWallProjectile 3")
    run(f"statecall {ing} BecomeWallProjectile -1")
    run(f"statecall {ing} BecomeWallProjectile 2")
    run(f"var {ing} ing.form {form if form is not None else 3}")
    run(f"fsm {ing} Lurk")
    time.sleep(1)

print("== B534 body projectile", flush=True)
for rnd in range(2):
    run(f"place {ing} player 6")
    run(f"fsm {ing} BecomeBodyProjectile")
    for t in range(16):
        time.sleep(0.5)
        run(f"obj {ing}")
    run(f"fsm {ing} Lurk")
    time.sleep(1)
print("== done", flush=True)
