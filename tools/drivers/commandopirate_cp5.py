#!/usr/bin/env python3
"""CommandoPirate driver, slice cp5.

msg: the script messages of AcceptScriptMsg (fn_9_CB40), paired with the forcing spec
  tools/cov/specs/commandopirate_cp5.py: every message the switch handles, active and inactive
  (Deactivate/Activate alternate, against the spec's per-message cycles of 3), and the shield's
  own resisted shots (ArmShield, then the dark, light and power beams at its collision actor: the
  actor's XRDG with a live weapon).
xald: two AreaLoaded messages only (the spec's kill arm, from the message's 2nd call on).
states2: the three state bodies through `statecall`, each arm on demand: the module's fields
  (`var <id> cp.*`) and the body state forced around the call and put back in the same tick.
states: the WallHang (fn_9_7358), ShieldCharge (fn_9_8028) and ScriptedShieldCharge (fn_9_7A98)
  states in the Swamp's 05_swamp_timber! (cover points, the attack pattern's waypoints): WallHang
  without a cover point, then after CoverFind; ShieldCharge from 6 and from 2 units; the
  FollowAttackPattern state (its ShouldBoost trigger enters the scripted charge).

usage: commandopirate_cp5.py <port> [outfile|-] [layer|-] [msg|xald|states|states2]
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 and sys.argv[2] != "-" else sys.stdout
LAYER = sys.argv[3] if len(sys.argv) > 3 and sys.argv[3] != "-" else None
MODE = sys.argv[4] if len(sys.argv) > 4 else "msg"


def log(msg):
    OUT.write(str(msg) + "\n")
    OUT.flush()


def connect():
    for _ in range(900):
        try:
            return socket.create_connection(("127.0.0.1", PORT), timeout=10)
        except OSError:
            time.sleep(0.5)
    raise SystemExit("no console on port %d" % PORT)


sock = connect()
sock.settimeout(300)
stream = sock.makefile("rw")


def cmd(c, replies=1):
    stream.write(c + "\n")
    stream.flush()
    lines = []
    while replies > 0:
        line = stream.readline()
        if not line:
            break
        line = line.rstrip("\n")
        lines.append(line)
        if line.startswith("=> "):
            replies -= 1
    return " | ".join(lines)


def frame():
    for line in cmd("frame").split(" | "):
        if line.startswith("frame "):
            return int(line.split()[1])
    return -1


def wait(n):
    target = frame() + n
    while True:
        f = frame()
        if f < 0 or f >= target:
            return f
        time.sleep(0.1)


def run(c):
    log("%s: %s" % (c, cmd(c)))


def objs_of(kind, cls):
    out = {}
    for line in cmd("objs %s" % kind).split(" | "):
        if cls in line:
            m = re.search(r"\((-?[\d.]+), (-?[\d.]+), (-?[\d.]+)\)", line)
            if m:
                out[line.split()[0]] = tuple(float(v) for v in m.groups())
    return out


def near_actors(cid, radius=4.0):
    me = objs_of("commando", "CCommandoPirate").get(cid)
    if me is None:
        return []
    return [aid for aid, p in objs_of("Collision", "CCollisionActor").items()
            if sum((a - b) ** 2 for a, b in zip(p, me)) <= radius * radius]


def state_of(cid):
    for line in cmd("obj %s" % cid).split(" | "):
        if line.strip().startswith("fsm"):
            return line.strip()
    return "?"


def messages(cid):
    log("=== commando %s ===" % cid)
    run("place %s player 6" % cid)
    wait(6)
    for m in ("Start", "Stop", "Start", "Alert", "Landed", "HitObject") + \
            ("OffGround", "Launching", "AIUpdateDisabled") * 3:
        run("send %s %s" % (cid, m))
        wait(1)
    run("damage %s 1" % cid)
    run("fsm %s ArmShield" % cid)
    wait(30)
    near = near_actors(cid, 8.0)
    log("near collision actors %s" % near)
    for beam, d in (("dark", 3), ("power", 1.5), ("power", 3), ("dark", 1.5)):
        run("beam %s" % beam)
        for aid in near:
            run("shoot %s %s" % (aid, d))
        wait(4)
        run("send %s ResistedDamage" % cid)
        run("send %s Damage" % cid)
        wait(2)
    # a live weapon's XDMG on the body itself (0x0000D100..0x0000D12C).
    run("fsm %s MeleeAttack" % cid)
    for _ in range(3):
        run("shoot %s" % cid)
        wait(4)
    # the shield actor's XRDG with a live shot as its last toucher (0x0000D1B0..0x0000D204): a
    # shot still in flight from 25 units, named as the message's sender.
    run("beam power")
    for _ in range(4):
        for aid in near:
            shot = re.search(r"shot (u[0-9a-f]{4}) ", cmd("shoot %s 25" % aid))
            if shot and shot.group(1) != "uffff":
                run("send %s ResistedDamage from %s" % (aid, shot.group(1)))
        wait(6)
    # XALD with the block's 0x01 bit (the spec, from the message's 2nd call on) kills the actor,
    # and the escape/zero loop below warps it out: both last.
    for _ in range(2):
        run("send %s AreaLoaded" % cid)
        wait(2)
    # the active bit from before the base's handler, both ways, against cycles of 3.
    for _ in range(3):
        for m in ("Deactivate", "Escape", "SetToZero", "Activate", "Escape", "SetToZero",
                  "Activate"):
            run("send %s %s" % (cid, m))
            wait(1)
    for _ in range(4):
        run("send %s Damage" % cid)
        run("send %s ResistedDamage" % cid)
        wait(1)


def states(cid):
    log("=== commando %s ===" % cid)
    run("send %s Activate" % cid)
    run("place %s player 6" % cid)
    wait(20)
    # WallHang with no cover point held (0x000073D4..0x000073D8).
    run("fsm %s WallHang" % cid)
    wait(10)
    run("fsm %s CoverFind" % cid)
    wait(30)
    log(state_of(cid))
    run("fsm %s WallHang" % cid)
    for _ in range(6):
        wait(25)
        log(state_of(cid))
    run("place %s player 8" % cid)
    wait(40)
    run("fsm %s WallDetach" % cid)
    wait(20)
    for d in (6, 3, 10):
        run("place %s player %d" % (cid, d))
        run("fsm %s ShieldCharge" % cid)
        for _ in range(4):
            wait(30)
            log(state_of(cid))
    run("fsm %s FollowAttackPattern" % cid)
    for i in range(12):
        wait(30)
        log(state_of(cid))
        if i == 6:
            run("fsm %s FollowAttackPattern" % cid)


def var(cid, name, value=None):
    out = cmd("var %s cp.%s%s" % (cid, name, "" if value is None else " %d" % value))
    m = re.search(r"= (-?\d+)", out)
    return int(m.group(1)) if m else None


def tick(*cmds):
    """several commands in one game tick (the console's `a; b; c`)."""
    log("%s: %s" % ("; ".join(cmds), cmd("; ".join(cmds), len(cmds))))


def pos_of(uid, kind):
    return objs_of(kind, "").get(uid)


def face_sweep(cid, state, kind):
    """x924 = 0 Update at every 10th yaw: the turn command until the facing is within 10 degrees
    of xba0 (0x8288 / 0x74C8), then x924 = 1."""
    p = pos_of(cid, kind)
    if p is None:
        return
    for yaw in range(0, 360, 10):
        tick("place %s %.3f %.3f %.3f %d" % (cid, p[0], p[1], p[2], yaw),
             "var %s cp.x924 0" % cid, "statecall %s %s 1" % (cid, state))
        if var(cid, "x924") == 1:
            log("%s: x924 = 1 at yaw %d" % (state, yaw))
            return


def forced(cid, name, value, *body):
    """a forced field around state calls, put back in the same tick."""
    old = var(cid, name)
    tick("var %s cp.%s %d" % (cid, name, value), *body, "var %s cp.%s %d" % (cid, name, old))


def bits(cid, name, mask, on, *body):
    old = var(cid, name)
    forced(cid, name, (old | mask) if on else (old & ~mask & 0xff), *body)


def states2(cid, kind, player):
    """the state bodies through statecall, every arm on demand (cp5): each forced field is set
    and put back in the same tick as the call."""
    log("=== commando %s (statecall) ===" % cid)
    run("send %s Activate" % cid)
    run("place %s player 6" % cid)
    wait(20)
    others = [u for u in objs_of(kind, "CCommandoPirate") if u != cid]
    sc, ssc, wh = ("statecall %s %s" % (cid, n) for n in
                   ("ShieldCharge", "ScriptedShieldCharge", "WallHang"))
    # -- ShieldCharge (fn_9_8028)
    log("team %s xbca %s" % (var(cid, "team"), var(cid, "xbca")))
    tick(sc + " 0", sc + " 1")
    forced(cid, "team", 0xffff, sc + " 0")                 # no team (0x80F0)
    forced(cid, "xbca", 0xffff, sc + " 0")                 # no target (0x8228)
    forced(cid, "xbca", int(player[1:], 16), sc + " 0")    # the player
    for o in others:
        forced(cid, "xbca", int(o[1:], 16), sc + " 0")     # another commando's direction
    me = pos_of(cid, kind)                                  # on the target: no direction
    tick("place %s player 0" % cid, "var %s cp.xbca %d" % (cid, int(player[1:], 16)), sc + " 0")
    if me is not None:
        run("place %s %.3f %.3f %.3f" % (cid, me[0], me[1], me[2]))
    forced(cid, "xbca", int(player[1:], 16), sc + " 0")
    face_sweep(cid, "ShieldCharge", kind)
    tick("var %s cp.x924 1" % cid, sc + " 1")              # the loop command (mState ready)
    wait(4)
    for body in ([sc + " 1"],
                 ["bodystate %s 9" % cid, sc + " 1"]):
        tick(*body)
    loop = "bodystate %s 9" % cid
    tick(loop, "var %s cp.x924 1" % cid, sc + " 1")
    c64 = var(cid, "c64")
    ahead = "var %s cp.b94ahead 20000" % cid
    t0 = "var %s cp.fsmtime 0" % cid
    forced(cid, "c64", c64 | 0x04, loop, ahead, sc + " 1")             # c64 0x04: the exit
    forced(cid, "c64", c64 & 0xf3, loop, "var %s cp.b94ahead -2000" % cid, sc + " 1")  # behind
    forced(cid, "c64", c64 & 0xf3, loop, ahead, "var %s cp.fsmtime 100000" % cid, sc + " 1")
    for speed in (0, var(cid, "chargespeed")):                         # no charge speed / its own
        tick("var %s cp.chargespeed %d" % (cid, speed))
        forced(cid, "c64", (c64 & 0xf3) | 0x08, loop, ahead, t0, sc + " 1")
    for axis in ("bb8x", "bb8y", "bb8z"):                             # the push, one axis each
        forced(cid, "c64", (c64 & 0xf3) | 0x08, loop, ahead, t0,
               "var %s cp.%s 500" % (cid, axis), sc + " 1")
    forced(cid, "c64", c64 & 0xf3, loop, ahead, t0, sc + " 1")         # not taken off
    tick(loop, "var %s cp.x924 1" % cid, sc + " 1",       # not ready (Repeat), not looping
         "bodystate %s 5" % cid, sc + " 1")
    tick("var %s cp.x924 7" % cid, sc + " 1")              # neither 0 nor 1
    bits(cid, "c64", 0x04, True, sc + " 2")
    tick(sc + " 2")
    run("bodystate %s 5" % cid)
    wait(10)
    # -- ScriptedShieldCharge (fn_9_7A98)
    tick(ssc + " 0")
    tick(ssc + " 1")                                        # the loop command (mState ready)
    wait(4)
    tick(ssc + " 1")
    loop9 = [loop]
    bits(cid, "c64", 0x04, True, *(loop9 + [ssc + " 1"]))
    for ahead in (-2000, 100, -2000, 100):                  # arrived: behind / inside the box
        bits(cid, "c64", 0x04, False, *(loop9 + ["var %s cp.b94ahead %d" % (cid, ahead), ssc + " 1"]))
    bits(cid, "c64", 0x08, True, *(loop9 + ["var %s cp.b94ahead 30000" % cid, ssc + " 1"]))
    bits(cid, "c64", 0x08, False, *(loop9 + ["var %s cp.b94ahead 30000" % cid, ssc + " 1"]))
    tick(loop, ssc + " 1", "bodystate %s 5" % cid, ssc + " 1")  # not ready, not looping
    bits(cid, "c64", 0x04, True, ssc + " 2")
    tick(ssc + " 2")
    wait(10)
    # -- WallHang (fn_9_7358)
    forced(cid, "xbd2", 0xffff, wh + " 0", wh + " 2")      # no cover point (0x73D4)
    # the spec (CP5_MODE=states) answers every other ARRV/NEXT search of WallHang's Activate with
    # this waypoint (no commando here has that link).
    wps = list(objs_of("Waypoint", "Waypoint"))
    if wps and OUT is not sys.stdout:
        import os
        with open(os.path.join(os.path.dirname(OUT.name), "waypoint.txt"), "w") as fh:
            fh.write("%d\n" % int(wps[0][1:], 16))
    log("waypoints %s" % wps[:4])
    covers = list(objs_of("Cover", "Cover"))
    log("cover points %s" % covers)
    for c in covers[:3]:
        forced(cid, "xbd2", int(c[1:], 16), wh + " 0")
        log("xbd6 %s" % var(cid, "xbd6"))
    if covers:
        face_sweep(cid, "WallHang", kind)
    tick("var %s cp.x924 1" % cid, wh + " 1")              # the wall-hang command
    wait(4)
    for ahead in (20000, -20000, 0, 3000):                  # the aim test (x3CBC_) both ways
        tick("bodystate %s 20" % cid, "var %s cp.x924 1" % cid,
             "var %s cp.b94ahead %d" % (cid, ahead), wh + " 1")
        wait(3)
    forced(cid, "xbca", 0xffff, "bodystate %s 20" % cid, "var %s cp.x924 1" % cid,
           wh + " 1")                                       # no aim target (x3CBC_ false)
    tick("bodystate %s 20" % cid, "var %s cp.x924 1" % cid, wh + " 1",  # not ready, not hanging
         "bodystate %s 5" % cid, wh + " 1")
    tick("var %s cp.x924 7" % cid, wh + " 1", wh + " 2")
    wait(10)


def main():
    wait(100)
    if LAYER is not None:
        run("layer %s" % LAYER)
        wait(20)
    ids = []
    for _ in range(30):  # the area can still be loading
        ids = [line.split()[0] for line in cmd("objs commando").split(" | ")
               if "CCommandoPirate" in line and "(" in line]
        if ids:
            break
        wait(10)
    log("commandos %s (frame %d)" % (ids, frame()))
    if MODE == "xald":  # the XALD kill arm alone (the msg run's commando is gone by its end)
        for cid in ids:
            for _ in range(2):
                run("send %s AreaLoaded" % cid)
                wait(2)
            log(state_of(cid))
        log("done (frame %d)" % frame())
        return
    if MODE == "states2":
        players = [u for u in objs_of("player", "CPlayer")]
        log("players %s" % players)
        for cid in ids:
            states2(cid, "commando", players[0] if players else "u0000")
        log("done (frame %d)" % frame())
        return
    if MODE == "states":
        for cid in ids:
            states(cid)
        log("done (frame %d)" % frame())
        return
    for item in ("darkBeam", "lightBeam", "darkAmmo 100", "lightAmmo 100"):
        run("give %s" % item)
    for cid in ids:
        run("send %s Activate" % cid)
    wait(10)
    for cid in ids:
        messages(cid)
    log("done (frame %d)" % frame())


main()
