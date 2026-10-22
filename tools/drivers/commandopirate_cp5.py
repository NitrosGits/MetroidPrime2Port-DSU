#!/usr/bin/env python3
"""CommandoPirate driver, slice cp5.

msg: the script messages of AcceptScriptMsg (fn_9_CB40), paired with the forcing spec
  tools/cov/specs/commandopirate_cp5.py: every message the switch handles, active and inactive
  (Deactivate/Activate alternate, against the spec's per-message cycles of 3), and the shield's
  own resisted shots (ArmShield, then the dark, light and power beams at its collision actor: the
  actor's XRDG with a live weapon).
xald: two AreaLoaded messages only (the spec's kill arm, from the message's 2nd call on).
states: the WallHang (fn_9_7358), ShieldCharge (fn_9_8028) and ScriptedShieldCharge (fn_9_7A98)
  states in the Swamp's 05_swamp_timber! (cover points, the attack pattern's waypoints): WallHang
  without a cover point, then after CoverFind; ShieldCharge from 6 and from 2 units; the
  FollowAttackPattern state (its ShouldBoost trigger enters the scripted charge).

usage: commandopirate_cp5.py <port> [outfile|-] [layer|-] [msg|xald|states]
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


def cmd(c):
    stream.write(c + "\n")
    stream.flush()
    lines = []
    while True:
        line = stream.readline()
        if not line:
            break
        line = line.rstrip("\n")
        lines.append(line)
        if line.startswith("=> "):
            break
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
