#!/usr/bin/env python3
"""Ing FaceTarget (fn_29_7244) arm driver (lane leads1), through the console.

The Agon Wastes 01_sand_entrance_dark (MP2_WORLD="42b935e4,01_sand_entrance_dark"). The first Ing
is woken with `Activate`, then FaceTarget runs through `statecall` (every forced value printed):

  * Activate: the player put on 18 points of a circle round the Ing (`tp`; `place`'s yaw
    argument is rejected by its argument count check) so the angle to the player is both above
    and within 20 degrees (Ready / Over), and `var ing.target 65535` (no target: Over);
  * Update, forms 3 and 7: the same yaws (FaceDirection / Over) and `var ing.targetpos 1` (the
    stored point on the Ing: the unsafe-delta Over);
  * Update, form 2: Activate at a Ready yaw then Update (the locomotion command), the same with
    `ing.targetpos 1` (unsafe: Over), and Deactivate first (CanIssueCommand false);
  * Update, form 4: Over; Deactivate: NotReady.

usage: ing_facetarget.py <port> <game log>
"""
import math
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


def run(*cmds):
    c = "; ".join(cmds)  # one tick: Think cannot undo a forced field in between
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


def var(uid, name):
    m = re.search(r"=\s*(-?\d+)", run(f"var {uid} {name}"))
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
if not ings:
    print("no CIng")
    sys.exit(1)
ing, x, y, z = ings[0]
run(f"send {ing} Activate")
time.sleep(1)
target = var(ing, "ing.target")
form = var(ing, "ing.form")


def around(deg):
    a = math.radians(deg)
    return f"tp {x + 6 * math.sin(a):.2f} {y + 6 * math.cos(a):.2f} {z + 1:.2f}"

FT = f"statecall {ing} FaceTarget"

# Activate: no target, then each bearing.
run(f"var {ing} ing.target 65535", f"{FT} 0", f"var {ing} ing.target {target}")
for yaw in range(0, 360, 20):
    run(around(yaw), f"{FT} 0")
# Update, forms 3 and 7: each yaw, then the stored point on the Ing.
for f in (3, 7):
    for yaw in range(0, 360, 20):
        run(around(yaw), f"{FT} 0", f"var {ing} ing.form {f}", f"{FT} 1 0.016",
            f"var {ing} ing.form {form}")
    run(f"{FT} 0", f"var {ing} ing.targetpos 1", f"var {ing} ing.form {f}", f"{FT} 1 0.016",
        f"var {ing} ing.form {form}")
# Update, form 2: Ready yaws then the command; the unsafe point; not ready.
for yaw in range(0, 360, 20):
    run(around(yaw), f"{FT} 0", f"var {ing} ing.form 2", f"{FT} 1 0.016",
        f"var {ing} ing.form {form}")
    run(around(yaw), f"{FT} 0", f"var {ing} ing.targetpos 1",
        f"var {ing} ing.form 2", f"{FT} 1 0.016", f"var {ing} ing.form {form}")
run(f"{FT} 2", f"var {ing} ing.form 2", f"{FT} 1 0.016", f"var {ing} ing.form {form}")
# Update, form 4 (and 0); Deactivate.
for f in (4, 0, 10):
    run(f"var {ing} ing.form {f}", f"{FT} 1 0.016", f"var {ing} ing.form {form}")
run(f"{FT} 2")
print("done", flush=True)
