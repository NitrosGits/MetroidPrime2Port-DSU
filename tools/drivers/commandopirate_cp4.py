#!/usr/bin/env python3
"""CommandoPirate driver, slice cp4: the script messages of AcceptScriptMsg (fn_9_CB40) and the
shield's CollidedWith (fn_9_BF44). Each commando gets every message the switch handles, active and
inactive (the cases test the entity's active bit from before the base's handler), real beam shots
(`shoot`, a CWeapon sender) at the commando and at its shield collision actor while the shield is
armed, and the ShieldCharge state near the player and near a wall. At the end one commando warps out
on Escape, one on SetToZero and one is deleted.

Pair it with tools/cov/specs/commandopirate_cp4.py (tools/cov/force_ret.py).

usage: commandopirate_cp4.py <port> [outfile|-] [layer|-] [col]
  col: only the contacts (the ShieldCharge state near the player, the commandos walking), for
  the CP4_MODE=col forcing run.
"""
import os
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
    if OUT is not sys.stdout:
        print(msg, flush=True)


def connect():
    # a world load under gdb takes minutes, so the loop waits long.
    for _ in range(900):
        try:
            return socket.create_connection(("127.0.0.1", PORT), timeout=10)
        except OSError:
            time.sleep(0.5)
    raise SystemExit("no console on port %d" % PORT)


sock = connect()
sock.settimeout(int(os.environ.get("CON_REPLY_TIMEOUT", "300")))
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
        time.sleep(0.2)


def commandos():
    out = []
    for line in cmd("objs commando").split(" | "):
        if "CCommandoPirate" in line and "(" in line:
            out.append(line.split()[0])
    return out


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


def shield_actor():
    for line in cmd("objs Shield_CollisionActor").split(" | "):
        if "CCollisionActor" in line:
            return line.split()[0]
    return None


def messages(cid):
    log("=== commando %s ===" % cid)
    run("place %s player 6" % cid)
    wait(10)
    for m in ("Start", "Stop", "Start", "Alert", "Landed", "HitObject") + \
            ("OffGround", "Launching", "AIUpdateDisabled") * 4:
        run("send %s %s" % (cid, m))
        wait(2)
    for _ in range(2):
        run("shoot %s" % cid)
        wait(20)
    run("damage %s 1" % cid)
    wait(4)
    # the armed shield: the Damage/ResistedDamage cases test +0xC65 & 0x0C.
    run("fsm %s ArmShield" % cid)
    wait(60)
    run("objs Collision")
    sid = shield_actor()
    log("shield actor %s" % sid)
    for _ in range(3):
        if sid is not None:
            run("shoot %s" % sid)
            wait(20)
        run("shoot %s" % cid)
        wait(20)
        run("send %s Damage" % cid)
        run("send %s ResistedDamage" % cid)
    # the shield charge: CollidedWith with the +0xC65 0x04 bit, near the player.
    run("place %s player 12" % cid)
    run("fsm %s ShieldCharge" % cid)
    wait(150)
    run("place %s player 6" % cid)
    run("fsm %s ShieldCharge" % cid)
    wait(150)
    # the active bit from before the base's handler, both ways.
    for _ in range(3):
        for m in ("Deactivate", "Deactivate", "Escape", "SetToZero", "Activate", "Activate",
                  "Escape", "SetToZero"):
            run("send %s %s" % (cid, m))
            wait(4)
    # the messages that may end in the module's knock-back (a death): last.
    for _ in range(6):
        run("send %s Damage" % cid)
        wait(2)
        run("send %s ResistedDamage" % cid)
        wait(2)
    run("obj %s" % cid)


def main():
    wait(200)
    if LAYER is not None:
        run("layer %s" % LAYER)
        wait(20)
    ids = commandos()
    log("commandos %s (frame %d)" % (ids, frame()))
    for cid in ids:
        run("send %s Activate" % cid)
        run("place %s player 6" % cid)
    wait(30)
    if MODE == "col":
        for _ in range(3):
            for cid in ids:
                run("place %s player 6" % cid)
                run("fsm %s ShieldCharge" % cid)
            wait(150)
            for cid in ids:
                run("place %s player 2" % cid)
                run("fsm %s PathFind" % cid)
            wait(100)
        log("done (frame %d)" % frame())
        return
    for cid in ids:
        messages(cid)
    # the end: a warp-out on Escape, one on SetToZero, a delete (the rest in order).
    # the block's 0x01 bit on a late AreaLoaded (forced) kills the last commando.
    for _ in range(4):
        run("send %s AreaLoaded" % ids[-1])
        wait(4)
    for cid, m in zip(ids, ("Escape", "SetToZero", "Delete")):
        run("send %s %s" % (cid, m))
        wait(60)
    run("objs commando")
    wait(200)
    log("done (frame %d)" % frame())


main()
