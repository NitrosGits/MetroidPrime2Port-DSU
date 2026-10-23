#!/usr/bin/env python3
"""DarkCommando melee driver: the commando's own state machine picks the player as its target
(SelectAttackTarget), then it is put at melee distance, at varying distances and on both sides
of the player, so ShouldMeleeAttack fires and the blade (ApplyMeleeDamage) reaches the player.
After the natural part, MeleeAttack is forced (`fsm`) with the target already chosen, so the
variant selection (SelectMeleeVariant) sees every angle; then FireEMP is forced at several
distances, so grenades land near and far from the player.  Run with MP2_GODMODE=1.

usage: darkcommando_melee.py <port> [outfile] [layer|-] [leave]
("leave" ends with three Deactivate messages to the first commando: its LeaveTeam calls.)
(Temple Grounds 11_temple_grandpuzzle layer 8 holds three commandos with a team AI manager.)
"""
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout
LAYER = sys.argv[3] if len(sys.argv) > 3 and sys.argv[3] != "-" else None
LEAVE = len(sys.argv) > 4 and sys.argv[4] == "leave"

# distances in front of the player: inside, at and outside the melee range.
DISTANCES = [2.5, 3.5, 1.5, 5.0, 2.0, 8.0, 3.0, 0.8, 4.0, 12.0]


def log(msg):
    OUT.write(str(msg) + "\n")
    OUT.flush()
    print(msg, flush=True)


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
        if line.startswith("=> "):
            break
        lines.append(line)
    return "\n".join(lines)


def frame():
    for line in cmd("frame").split("\n"):
        if line.startswith("frame "):
            return int(line.split()[1])
    return -1


def wait_frame(target):
    while True:
        f = frame()
        if f < 0 or f >= target:
            return f
        time.sleep(0.2)


def commandos():
    out = []
    for line in cmd("objs DarkCommando").split("\n"):
        if "CDarkCommando" in line and "(" in line:
            out.append(line.split()[0])
    return out


def state_of(cid):
    for line in cmd("obj %s" % cid).split("\n"):
        if line.strip().startswith("fsm"):
            return line.strip()
    return "?"


def main():
    wait_frame(200)
    if LAYER is not None:
        log("layer %s: %s" % (LAYER, cmd("layer %s" % LAYER)))
        wait_frame(frame() + 30)
    ids = commandos()
    log("commandos %s (frame %d)" % (ids, frame()))
    log("teams: %s" % cmd("objs TeamAiMgr"))
    if not ids:
        return
    target = ids[0]
    for other in ids:
        log("send Activate: %s" % cmd("send %s Activate" % other))
    log("place: %s" % cmd("place %s player 10" % target))
    # warp-in and the first SelectAttackTarget.
    wait_frame(frame() + 300)
    log("state: %s" % state_of(target))
    for rnd in range(3):
        for d in DISTANCES:
            log("place %s: %s" % (d, cmd("place %s player %s" % (target, d))))
            wait_frame(frame() + 45)
            log("  %s" % state_of(target))
    # forced: the target is chosen now, so MeleeAttack selects a variant from the angle.
    for rnd in range(4):
        for d in [2.0, 3.0, 1.0, 2.5, -2.0, -3.0]:
            log("place %s: %s" % (d, cmd("place %s player %s" % (target, d))))
            log("fsm MeleeAttack: %s" % cmd("fsm %s MeleeAttack" % target))
            wait_frame(frame() + 70)
            log("  %s" % state_of(target))
    # EMP grenades at the player, near and far (the grenade's interference range is 20).
    for d in [12.0, 16.0, 25.0, 12.0, 14.0, 30.0]:
        log("place %s: %s" % (d, cmd("place %s player %s" % (target, d))))
        log("fsm FireEMP: %s" % cmd("fsm %s FireEMP" % target))
        wait_frame(frame() + 240)
        log("  %s" % state_of(target))
    if LEAVE:
        for _ in range(3):
            log("send Deactivate: %s" % cmd("send %s Deactivate" % target))
            wait_frame(frame() + 30)
    log("objs: %s" % cmd("objs DarkCommando"))


if __name__ == "__main__":
    main()
