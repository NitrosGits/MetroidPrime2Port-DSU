#!/usr/bin/env python3
"""Ing mini-portal driver: make an Ing open its mini portals (CIngMiniPortalAttack, fn_29_11E8C..).

The Agon Wastes 01_sand_entrance_dark (MP2_WORLD="42b935e4,01_sand_entrance_dark"): wake every
Ing with `Activate`, bring the first one next to the player, then force the pair of states the
FSM runs for the attack - `FindMiniPortals` (fn_29_61B4, queues up to three portal points) and
`MiniPortalAttack` (fn_29_6008, asks the body for the projectile attack, whose kUE_Projectile
events spawn the portals in fn_29_C228). A portal ages, fires its plasma beam after one second
and deletes itself at two (fn_29_11CA4), so the rounds are a few seconds apart.

usage: ing_miniportal.py <port> <game log> [rounds]
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1])
LOGPATH = sys.argv[2]
ROUNDS = int(sys.argv[3]) if len(sys.argv) > 3 else 6


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


def log(*a):
    print(*a, flush=True)


def uids(cls):
    return re.findall(r"^\s*(u[0-9a-f]{4})\s", cmd(f"objs {cls}"), re.M)


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
    ings = uids("CIng")
    if ings:
        break
    time.sleep(1)
log("== ings", ings)
for i in ings:
    log("== wake", i, cmd(f"send {i} Activate"))
time.sleep(2)
ing = ings[0]
for r in range(ROUNDS):
    log(f"== round {r}")
    log(cmd(f"place {ing} player 10"))
    if r % 2:
        # The portals start on the Ing's own forward (fn_29_11570 turns at most 70 degrees from
        # it), so on odd rounds the Ing faces the player first.
        log(cmd(f"fsm {ing} FaceTarget"))
        time.sleep(1.5)
    log(cmd(f"fsm {ing} FindMiniPortals"))
    time.sleep(1.0)
    log(cmd(f"fsm {ing} MiniPortalAttack"))
    for t in range(8):
        time.sleep(0.5)
        log(f"   t{t} portals", uids("CIngMiniPortalAttack"), "beams", uids("CPlasmaProjectile"))
    log(cmd(f"obj {ing}"))
# (The portal's XDEL arm, fn_29_11C14, runs on its own: its Think deletes it through
# CStateManager::DeleteObjectRequest, which delivers kSM_Delete.)
time.sleep(3)
log("== covdump", cmd("covdump"))
log("== done")
