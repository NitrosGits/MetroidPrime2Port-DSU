#!/usr/bin/env python3
"""Ing row arms driver (lane ing4): the arms of the bodies ing3 left unmarked, through the console.

The Agon Wastes 01_sand_entrance_dark (MP2_WORLD="42b935e4,01_sand_entrance_dark"). Every Ing is
woken with `Activate`, then, on the first Ing (every forced value is printed, so the module note
can label the forced arms):

  * fn_29_1DF4 (Listen's safe-zone arm): `noise 6 x y z` around the Ing with `var ing.lurezone`
    invalid, a safe zone, and an object that is not a safe zone;
  * fn_29_1C98 / 53E0 / 51D0: `codecall SetLuredDest|SetLuredPointDest` with the lure zone set to
    each safe zone, to an invalid id and to an object that is not a safe zone;
  * fn_29_5714: `codecall SetWallPointCoverDest` with `var ing.coverhint` invalid and each hint;
  * fn_29_75C0 SeekWallPoint: `statecall` with each hint, the Ing placed on and away from it;
  * fn_29_8CF4 SafeZoneReaction: `statecall` with `var ing.hurtful|ing.evap`, then `fsm` and
    two seconds for the state time arm;
  * fn_29_8FE8 BecomeWallProjectile: `fsm` + `statecall` with `var ing.takeoff`/`ing.form 4`;
  * fn_29_3684 (the team leave): `var ing.team` invalid / a non-team object / each team, then
    `send Deactivate` + `send Activate`, and `codecall SetTargetDest` (the team join) first;
  * fn_29_B534 CollidedWith / fn_29_10D50: `fsm BecomeBodyProjectile` near the player, and
    `fsm ExitHost` (the exit host effect's particle builder).

usage: ing_rows.py <port> <game log>
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


def var(uid, name, value=None):
    out = run(f"var {uid} {name}" + ("" if value is None else f" {value}"))
    m = re.search(r"=\s*(-?\d+)", out)
    return int(m.group(1)) if m else None


def chain(*cmds):
    """`a; b` runs both in one tick, so Think cannot reset a forced field in between."""
    run("; ".join(cmds))


def pos_of(uid, cls="CIng"):
    for o in objs(cls):
        if o[0] == uid:
            return o[1:]
    return None


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
hints = objs("CScriptAIHint")
zones = objs("CScriptSafeZone")
teams = objs("CScriptTeamAiMgr")
print("== hints", hints, "\n== zones", zones, "\n== teams", teams, flush=True)
other = hints[0][0] if hints else ings[-1][0]  # an object that is not a safe zone / team

print("== 3684 LeaveTeam", flush=True)
for t in [INVALID, packed(other)] + [packed(t[0]) for t in teams]:
    var(ing, "ing.team", t)
    if t not in (INVALID, packed(other)):
        run(f"codecall {ing} SetTargetDest")
        var(ing, "ing.team")
        var(ing, "ing.team", t)
    run(f"send {ing} Deactivate")
    var(ing, "ing.team")
    run(f"send {ing} Activate")
for t in [packed(t[0]) for t in teams]:
    # Already joined (SetTargetDest), so IsPartOfTeam is true on this leave.
    var(ing, "ing.team", t)
    run(f"codecall {ing} SetTargetDest")
    print("   team after join", var(ing, "ing.team"), flush=True)
    run(f"send {ing} Deactivate")
    print("   team after leave", var(ing, "ing.team"), flush=True)
    run(f"send {ing} Activate")
var(ing, "ing.team", INVALID)

print("== 1DF4 Listen safe zone", flush=True)
p = pos_of(ing)
for lure in [INVALID] + [packed(z[0]) for z in zones] + [packed(other)]:
    for d in (0.5, 5.0, 15.0, 40.0, 400.0):
        var(ing, "ing.lurezone", lure)
        if p:
            run(f"noise 6 {p[0] + d:.2f} {p[1]:.2f} {p[2]:.2f}")
        var(ing, "ing.lurezone")
for z in zones:
    var(ing, "ing.lurezone", INVALID)
    run(f"noise 6 {z[1]:.2f} {z[2]:.2f} {z[3]:.2f}")
    var(ing, "ing.lurezone")
    # The lure zone set, a second noise at each zone (nearer or farther than the kept one).
    for z2 in zones:
        run(f"noise 6 {z2[1]:.2f} {z2[2]:.2f} {z2[3]:.2f}")
        var(ing, "ing.lurezone")

print("== 1C98/53E0/51D0 lure dests", flush=True)
for lure in [INVALID, packed(other)] + [packed(z[0]) for z in zones]:
    chain(f"var {ing} ing.lurezone {lure}", f"codecall {ing} SetLuredDest", f"var {ing} ing.found")
    chain(f"var {ing} ing.lurezone {lure}", f"codecall {ing} SetLuredPointDest",
          f"var {ing} ing.found")
for z in zones:
    # Listen's kept-zone arms: a zone set in the same tick, then a noise at every zone.
    for z2 in zones[:6]:
        chain(f"var {ing} ing.lurezone {packed(z[0])}", f"noise 6 {z2[1]:.2f} {z2[2]:.2f} {z2[3]:.2f}",
              f"var {ing} ing.lurezone")

print("== 5714 SetWallPointCoverDest / 75C0 SeekWallPoint", flush=True)
for h in [INVALID] + [packed(h[0]) for h in hints[:20]]:
    chain(f"var {ing} ing.coverhint {h}", f"codecall {ing} SetWallPointCoverDest", f"var {ing} ing.found")
    for msg in (0, 1, 2):
        chain(f"var {ing} ing.coverhint {h}", f"statecall {ing} SeekWallPoint {msg} 0.016")
for h in hints[:3]:
    run(f"place {ing} {h[1]:.3f} {h[2]:.3f} {h[3]:.3f}")
    chain(f"var {ing} ing.coverhint {packed(h[0])}", f"statecall {ing} SeekWallPoint 1 0.016")
    chain(f"var {ing} ing.coverhint {packed(h[0])}", f"statecall {ing} SeekWallPoint 1 0")
for h in hints[-3:]:
    # Far from the hint: the move arm (|delta| > dt * the wall speed).
    chain(f"var {ing} ing.coverhint {packed(h[0])}", f"statecall {ing} SeekWallPoint 1 0.016")
    chain(f"var {ing} ing.coverhint {packed(h[0])}", f"statecall {ing} SeekWallPoint 1 0.5")
var(ing, "ing.coverhint", INVALID)
run(f"place {ing} player 8")

print("== 8FE8 BecomeWallProjectile", flush=True)
form = var(ing, "ing.form")
run(f"fsm {ing} BecomeWallProjectile")
for t in range(6):
    time.sleep(0.5)
    run(f"obj {ing}")
for tk, f in ((1, 3), (0, 4), (0, 3)):
    for _ in range(4):
        chain(f"var {ing} ing.takeoff {tk}", f"var {ing} ing.form {f}",
              f"statecall {ing} BecomeWallProjectile 1 0.016")
        time.sleep(0.3)
run(f"statecall {ing} BecomeWallProjectile 2")
var(ing, "ing.takeoff", 0)
var(ing, "ing.form", form if form is not None else 2)
run(f"statecall {ing} BecomeWallProjectile 0")
run(f"fsm {ing} Lurk")
time.sleep(1)

print("== B534 body projectile", flush=True)
run(f"place {ing} player 6")
run(f"fsm {ing} BecomeBodyProjectile")
for t in range(16):
    time.sleep(0.5)
    run(f"obj {ing}")
run(f"fsm {ing} Lurk")
time.sleep(1)

print("== 8CF4 SafeZoneReaction", flush=True)
run(f"statecall {ing} SafeZoneReaction 0")
chain(f"var {ing} ing.hurtful 0", f"var {ing} ing.evap 0", f"statecall {ing} SafeZoneReaction 1 0.016")
run(f"fsm {ing} SafeZoneReaction")
for t in range(8):
    time.sleep(0.5)
    chain(f"var {ing} ing.hurtful 0", f"var {ing} ing.evap 0", f"statecall {ing} SafeZoneReaction 1 0.016")
run(f"statecall {ing} SafeZoneReaction 2")
# The knock-back arms last: the evaporating Ing is deleted.
for h, e in ((1, 0), (1, 0), (0, 1), (1, 1)):
    chain(f"var {ing} ing.hurtful {h}", f"var {ing} ing.evap {e}", f"statecall {ing} SafeZoneReaction 1 0.016")
    time.sleep(0.3)
run(f"obj {ing}")
print("== 10D50 exit host", flush=True)
run(f"place {ing} player 4")
run(f"fsm {ing} ExitHost")
for t in range(10):
    time.sleep(0.5)
print("== objs", cmd("objs CIngExitHostEffect"), flush=True)
time.sleep(3)
print("== covdump", cmd("covdump"), flush=True)
print("== done", flush=True)
