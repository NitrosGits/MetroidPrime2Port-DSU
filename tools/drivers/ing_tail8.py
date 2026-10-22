#!/usr/bin/env python3
"""Ing small-row arms driver (lane tail8): fn_29_1DF4, 53E0, 5714, 8CF4, 10D50, 10E64.

The Agon Wastes 01_sand_entrance_dark (MP2_WORLD="42b935e4,01_sand_entrance_dark"). Every Ing is
woken with `send <ing> Activate`; the first Ing carries all the forced values (each one is printed,
so the module note's arm table can say which arm was forced). The arms:

  * fn_29_1DF4 (Listen's safe-zone arm, PortIngSafeZoneNoise): `noise 6 [x y z]` at the Ing itself
    (hearing taken), far away (hearing exit), with `var ing.lurezone` invalid (unset -> take the
    zone), holding the zone the noise is in (equal distance -> keep the held one), holding the zone
    while the noise is nearer the Ing than the zone is (replace), and holding an object that is not
    a safe zone (replace);
  * fn_29_53E0 SetLuredDest: the Ing is `place`d **onto** each safe zone and the lure zone set to it
    in the same tick as `codecall SetLuredDest`, so the mesh path from the Ing's own position to the
    point below the zone exists (the found arm); then from 60 units above the zone (the not-found
    arm);
  * fn_29_5714 SetWallPointCoverDest: invalid hint (the first exit), the Ing placed on each hint
    (a closest point exists), placed 1000 units up (no closest point), and the point search's two
    answers;
  * fn_29_8CF4 SafeZoneReaction: the three messages, an unknown message either side of the range
    (3 and -5), the hurtful-zone knock-back (Activate then Update in one line: CanIssueCommand needs
    the animation state Ready, which Activate sets), the evaporating form, and the one-second time
    arm (the state left running with both flags clear);
  * fn_29_10D50 ConstructEffect / fn_29_10E64 Render: `fsm ExitHost` near the player spawns the
    swarm effect (the ctor builds both child particle systems), then `objs CIngExitHostEffect`.

usage: ing_tail8.py <port> <game log>
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1])
LOGPATH = sys.argv[2]


def cmd(c, want=None):
    want = c.count(";") + 1 if want is None else want
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


def var(uid, name, value=None):
    out = run(f"var {uid} {name}" + ("" if value is None else f" {value}"))
    m = re.search(r"=\s*(-?\d+)", out)
    return int(m.group(1)) if m else None


def pos_of(uid):
    for o in objs("CIng"):
        if o[0] == uid:
            return o[1:]
    return None


def chain(*cmds):
    """`a; b` runs both in one tick, so Think cannot reset a forced field in between."""
    return run("; ".join(cmds))


INVALID = 0xFFFF

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
zones = objs("CScriptSafeZone")
hints = objs("CScriptAIHint")
other = hints[0][0] if hints else (zones[0][0] if zones else None)
print("== zones", zones, "\n== hints", hints, "\n== not-a-zone", other, flush=True)
# An object of the area that is not a CScriptSafeZone, for fn_29_1DF4's TCastToPtr arm.
notzone = packed(other) if other else 1
p = None
for o in objs("CIng"):
    if o[0] == ing:
        p = o[1:]

print("== 1DF4 Listen safe zone", flush=True)
if p:
    # The hearing radius (0x7c0) is 40: 400 units away is outside it.
    for lure in (INVALID, packed(other) if other else 1):
        var(ing, "ing.lurezone", lure)
        run(f"noise 6 {p[0] + 400.0:.2f} {p[1]:.2f} {p[2]:.2f}")
    for z in zones:
        # unset -> take the zone the noise is in
        chain(f"var {ing} ing.lurezone {INVALID}", f"noise 6 {z[1]:.3f} {z[2]:.3f} {z[3]:.3f}",
              f"var {ing} ing.lurezone")
        # held zone, noise at the zone: equal distance -> keep the held zone
        chain(f"var {ing} ing.lurezone {packed(z[0])}",
              f"noise 6 {z[1]:.3f} {z[2]:.3f} {z[3]:.3f}", f"var {ing} ing.lurezone")
        # held zone, noise nearer the Ing than the zone's own position -> replace
        chain(f"var {ing} ing.lurezone {packed(z[0])}",
              f"noise 6 {p[0]:.3f} {p[1]:.3f} {p[2]:.3f}", f"var {ing} ing.lurezone")
    # a held id that is not a safe zone -> replace
    chain(f"var {ing} ing.lurezone {notzone}",
          f"noise 6 {p[0]:.3f} {p[1]:.3f} {p[2]:.3f}", f"var {ing} ing.lurezone")
    # held zone, the noise farther from the Ing than the held zone's own position (the Ing stands on
    # the zone): the distances compare the other way, so the held zone is kept (asm 0x1EE4 bge).
    for z in zones:
        chain(f"place {ing} {z[1]:.3f} {z[2]:.3f} {z[3]:.3f}",
              f"var {ing} ing.lurezone {packed(z[0])}",
              f"noise 6 {z[1] + 8.0:.3f} {z[2]:.3f} {z[3]:.3f}", f"var {ing} ing.lurezone")
    var(ing, "ing.lurezone", INVALID)

print("== 53E0 SetLuredDest", flush=True)
for z in zones:
    chain(f"place {ing} {z[1]:.3f} {z[2]:.3f} {z[3]:.3f}", f"var {ing} ing.lurezone {packed(z[0])}",
          f"codecall {ing} SetLuredDest", f"var {ing} ing.found")
    chain(f"place {ing} {z[1]:.3f} {z[2]:.3f} {z[3] + 60.0:.3f}",
          f"var {ing} ing.lurezone {packed(z[0])}", f"codecall {ing} SetLuredDest",
          f"var {ing} ing.found")
for lure in (INVALID, notzone):
    chain(f"var {ing} ing.lurezone {lure}", f"codecall {ing} SetLuredDest", f"var {ing} ing.found")

print("== 5714 SetWallPointCoverDest", flush=True)
if p:
    run(f"place {ing} {p[0]:.3f} {p[1]:.3f} {p[2]:.3f}")
chain(f"var {ing} ing.coverhint {INVALID}", f"codecall {ing} SetWallPointCoverDest",
      f"var {ing} ing.found")
for h in hints:
    chain(f"place {ing} {h[1]:.3f} {h[2]:.3f} {h[3]:.3f}",
          f"var {ing} ing.coverhint {packed(h[0])}", f"codecall {ing} SetWallPointCoverDest",
          f"var {ing} ing.found")
for h in hints[:4]:
    # No point within 20 of the Ing: the first closest-point query answers "no point".
    chain(f"place {ing} {h[1]:.3f} {h[2]:.3f} {h[3] + 1000.0:.3f}",
          f"var {ing} ing.coverhint {packed(h[0])}", f"codecall {ing} SetWallPointCoverDest",
          f"var {ing} ing.found")
var(ing, "ing.coverhint", INVALID)

print("== 8CF4 SafeZoneReaction", flush=True)
# The message switch first, while the Ing is certainly alive: 0 Activate, 2 Deactivate and an
# unknown message either side of 0..2 (asm 0x8D1C and 0x8D20).
run(f"statecall {ing} SafeZoneReaction 0")
run(f"statecall {ing} SafeZoneReaction 2")
for msg in (3, -5, 7):
    run(f"statecall {ing} SafeZoneReaction {msg}")
# The time arm: the state runs on its own with both flags clear for over a second.
run(f"fsm {ing} SafeZoneReaction")
for _ in range(6):
    time.sleep(0.5)
chain(f"var {ing} ing.hurtful 0", f"var {ing} ing.evap 0",
      f"statecall {ing} SafeZoneReaction 1 0.016")
run(f"statecall {ing} SafeZoneReaction 2")
run(f"fsm {ing} Lurk")

print("== 10D50/10E64 exit host effect", flush=True)
if p:
    run(f"place {ing} {p[0]:.3f} {p[1]:.3f} {p[2]:.3f}")
    p = pos_of(ing) or p
run(f"fsm {ing} ExitHost")
for _ in range(12):
    time.sleep(0.5)
    print("== objs", cmd("objs CIngExitHostEffect"), flush=True)
time.sleep(2)
print("== objs", cmd("objs CIngExitHostEffect"), flush=True)
run(f"fsm {ing} Lurk")
time.sleep(1)

print("== 8CF4 knock-back arms", flush=True)
# The knock-back arms: Activate first (the animation state must be Ready for CanIssueCommand), and a
# second Update in the same line so the body is already in KnockBack and CanIssueCommand says no.
for h, e in ((1, 0), (0, 1), (1, 1)):
    chain(f"statecall {ing} SafeZoneReaction 0", f"var {ing} ing.hurtful {h}",
          f"var {ing} ing.evap {e}", f"statecall {ing} SafeZoneReaction 1 0.016",
          f"statecall {ing} SafeZoneReaction 1 0.016")
    time.sleep(0.3)
var(ing, "ing.hurtful", 0)
var(ing, "ing.evap", 0)

print("== 5714 cross-hint pairs", flush=True)
# The cross-hint pairs: the Ing stands on one hint while the cover hint is another, so the two
# closest points can be in regions the graph does not connect (the point search's failure arm).
for i, h in enumerate(hints):
    j = hints[(i + len(hints) // 2) % len(hints)]
    chain(f"place {ing} {h[1]:.3f} {h[2]:.3f} {h[3]:.3f}",
          f"var {ing} ing.coverhint {packed(j[0])}", f"codecall {ing} SetWallPointCoverDest",
          f"var {ing} ing.found")
var(ing, "ing.coverhint", INVALID)
print("== done", flush=True)
