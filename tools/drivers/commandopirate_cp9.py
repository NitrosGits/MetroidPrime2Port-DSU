#!/usr/bin/env python3
"""CommandoPirate driver, slice cp9 (paired with tools/cov/specs/commandopirate_cp9.py; arms from the
coverage build's gcov counters).

jump: fake AiJumpPoints (`var <id> cp.mkjump`, none ships in a commando area), then the
  JumpPointFind (fn_9_9F24) and JetBoost (fn_9_9BB4) states through `statecall`.
follow: the FollowAttackPattern state (fn_9_961C): xbda forced, the destination forced to every AI
  waypoint, the body state forced to the jump (13).
charge: the waypoint picker (fn_9_2A98) and the turn helper (fn_9_2474) from the scripted shield
  charge (fn_9_7A98): the destination, the charge speed, the time to the waypoint and the target's
  side forced.
hit: the hit-object helper (fn_9_3A5C) and the knock-back helper (fn_9_2D0C): HitObject/Damage
  messages from the shield's and the blade's collision actors (`send ... from`), `touch` for the
  collision actor's last touched object, the flag bytes forced.
drown: fn_9_3090 with the fluid list forced (`var <id> cp.fluid`): none, not water, the inactive
  water, an active water above and below the head. The last commando drowns.

usage: commandopirate_cp9.py <port> [outfile|-] [sections (default: all)]
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 and sys.argv[2] != "-" else sys.stdout
ONLY = set(sys.argv[3].split(",")) if len(sys.argv) > 3 else None


def want(section):
    return ONLY is None or section in ONLY


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


def tick(*cmds):
    """several commands in one game tick (the console's `a; b; c`)."""
    log("%s: %s" % ("; ".join(cmds), cmd("; ".join(cmds), len(cmds))))


def objs_of(kind, cls):
    out = {}
    for line in cmd("objs %s" % kind).split(" | "):
        parts = line.split()
        if len(parts) > 3 and parts[2] == cls and "inactive" not in line:
            m = re.search(r"\((-?[\d.]+), (-?[\d.]+), (-?[\d.]+)\)", line)
            if m:
                out[parts[0]] = tuple(float(v) for v in m.groups())
    return out


def num(uid):
    return int(uid[1:], 16)


def var(cid, name, value=None):
    out = cmd("var %s cp.%s%s" % (cid, name, "" if value is None else " %d" % value))
    m = re.search(r"= (-?\d+)", out)
    return int(m.group(1)) if m else None


def forced(cid, name, value, *body):
    """a forced field around the calls, put back in the same tick."""
    old = var(cid, name)
    tick("var %s cp.%s %d" % (cid, name, value), *body, "var %s cp.%s %d" % (cid, name, old))


def bits(cid, name, mask, on, *body):
    old = var(cid, name)
    forced(cid, name, (old | mask) if on else (old & ~mask & 0xff), *body)


def pos_of(cid):
    return objs_of("commando", "CCommandoPirate").get(cid)


def at(cid, p, yaw, dz=0.0):
    return "place %s %.3f %.3f %.3f %d" % (cid, p[0], p[1], p[2] + dz, yaw)


def jump(cid, player):
    log("--- jump %s" % cid)
    # fake jump points: type 0 with waypoints 0..4, type 1, another area, inactive, no waypoint
    jps = {}
    for v in (0x00, 0x10, 0x20, 0x30, 0x40, 0x01, 0x100, 0x200, 0xF0):
        jps[v] = var(cid, "mkjump", v)
    log("jump points %s" % jps)
    jpf = "statecall %s JumpPointFind" % cid
    tick(jpf + " 1", jpf + " 2")
    tick("var %s cp.xb78 0" % cid, jpf + " 0")                              # the timer < 2
    tick("var %s cp.xb78 3000" % cid, "var %s cp.xbca 65535" % cid, jpf + " 0")  # no target
    for _ in range(3):
        tick("var %s cp.xb78 3000" % cid, "var %s cp.xbca %d" % (cid, num(player)), jpf + " 0")
        log("xbd0 %s" % var(cid, "xbd0"))
    p = pos_of(cid)
    if p is not None:
        # far above the jump points: no path to them
        tick(at(cid, p, 0, 60.0), "var %s cp.xb78 3000" % cid,
             "var %s cp.xbca %d" % (cid, num(player)), jpf + " 0", at(cid, p, 0))
    jb = "statecall %s JetBoost" % cid
    for xbd0 in (0xffff, num(cid), jps[0xF0], jps[0x10], jps[0x00]):
        tick("var %s cp.xbd0 %d" % (cid, xbd0 & 0xffff), jb + " 0")
        log("anim %s" % var(cid, "anim"))
    p = pos_of(cid)
    for anim, yaw in ((0, 0), (1, 0), (1, 180), (1, 90), (3, 0), (1, 0)):
        body = ["var %s cp.anim %d" % (cid, anim)]
        if p is not None:
            # the facing: the jump direction xba0 set to +y, the commando turned by yaw
            body = [at(cid, p, yaw), "var %s cp.ba0x 0" % cid, "var %s cp.ba0y 1000" % cid,
                    "var %s cp.ba0z 0" % cid] + body
        tick(*(body + [jb + " 1"]))
    tick(jb + " 2")
    wait(30)
    run("place %s player 6" % cid)
    wait(20)


def follow(cid, player, aiwps, wps):
    log("--- follow %s" % cid)
    fap = "statecall %s FollowAttackPattern" % cid
    p = pos_of(cid)
    forced(cid, "xbda", 0xffff, fap + " 0")                                # the ATTK/FOLW link
    forced(cid, "xbda", num(player), fap + " 0")                           # not a waypoint
    for w, wp in list(wps.items())[:2]:
        for yaw in (0, 180):
            if p is not None:
                # the waypoint ahead of or behind the commando: the commando on the line
                tick(at(cid, (wp[0], wp[1] - 5.0, wp[2]), yaw), "var %s cp.xbda %d" % (cid, num(w)),
                     fap + " 0", "var %s cp.xbda 65535" % cid)
    if p is not None:
        run(at(cid, p, 0))
    forced(cid, "wpdest", 0xffff, fap + " 1")
    forced(cid, "wpdest", num(player), fap + " 1")
    for w, wp in aiwps.items():
        # far from and on the AI waypoint, then in the jump body state with and without a target
        tick("var %s cp.wpdest %d" % (cid, num(w)), fap + " 1")
        tick(at(cid, wp, 0), "var %s cp.wpdest %d" % (cid, num(w)), fap + " 1")
        tick("var %s cp.wpdest %d" % (cid, num(w)), "bodystate %s 13" % cid,
             "var %s cp.xbca %d" % (cid, num(player)), fap + " 1")
        tick("var %s cp.wpdest %d" % (cid, num(w)), "bodystate %s 13" % cid,
             "var %s cp.xbca 65535" % cid, fap + " 1")
    tick("var %s cp.wpdest 65535" % cid, "bodystate %s 13" % cid,
         "var %s cp.xbca %d" % (cid, num(player)), fap + " 1")
    tick(fap + " 2", "bodystate %s 5" % cid, "var %s cp.xbca %d" % (cid, num(player)))
    run("place %s player 6" % cid)
    wait(20)


def charge(cid, aiwps, wps):
    log("--- charge %s" % cid)
    ssc = "statecall %s ScriptedShieldCharge" % cid
    speed = var(cid, "chargespeed")
    c64 = var(cid, "c64")
    loop = "bodystate %s 9" % cid
    tick("var %s cp.chargespeed 0" % cid, ssc + " 0", "var %s cp.chargespeed %d" % (cid, speed))
    forced(cid, "wpdest", 0xffff, ssc + " 0")
    for w in list(wps)[:2]:
        forced(cid, "wpdest", num(w), ssc + " 0")                          # not an AI waypoint
    for w in aiwps:
        # the picker without and with the next waypoint (arrived: the target behind)
        tick("var %s cp.wpdest %d" % (cid, num(w)), ssc + " 0")
        tick("var %s cp.wpdest %d" % (cid, num(w)), loop, "var %s cp.c64 %d" % (cid, c64 & 0xf3),
             "var %s cp.b94ahead -2000" % cid, ssc + " 1")
        tick("var %s cp.wpdest %d" % (cid, num(w)), loop, "var %s cp.c64 %d" % (cid, c64 & 0xf3),
             "var %s cp.chargespeed 0" % cid, "var %s cp.b94ahead -2000" % cid, ssc + " 1",
             "var %s cp.chargespeed %d" % (cid, speed))
    p = pos_of(cid)
    turn = "var %s cp.c64 %d" % (cid, (c64 & 0xf3) | 0x08)
    if p is not None:
        for yaw, ahead, xb88, push in ((0, 30000, 1000, None), (90, 30000, 1000, None),
                                       (270, 30000, 1000, None), (0, 30000, 0, None),
                                       (0, 30000, 100000, "bb8x"), (0, 30000, 1000, "bb8y"),
                                       (0, 30000, 1000, "bb8z"), (0, 0, 1000, None)):
            body = [at(cid, p, 0), "var %s cp.b94ahead %d" % (cid, ahead), at(cid, p, yaw), loop,
                    turn, "var %s cp.xb88 %d" % (cid, xb88)]
            if push:
                body.append("var %s cp.%s 500" % (cid, push))
            tick(*(body + [ssc + " 1"]))
        tick("var %s cp.chargespeed 0" % cid, loop, turn, "var %s cp.b94ahead 30000" % cid,
             ssc + " 1", "var %s cp.chargespeed %d" % (cid, speed))
        run(at(cid, p, 0))
    tick(ssc + " 2", "var %s cp.c64 %d" % (cid, c64))
    run("bodystate %s 5" % cid)
    run("place %s player 6" % cid)
    wait(20)


def hit(cid, player, last):
    log("--- hit %s" % cid)
    shield, blade = var(cid, "xbc6"), var(cid, "xbc8")
    log("shield u%04x blade u%04x" % (shield, blade))
    su, bu = "u%04x" % shield, "u%04x" % blade
    c65, c64, c66 = var(cid, "c65"), var(cid, "c64"), var(cid, "c66")
    hits = "send %s HitObject from " % cid
    tick("var %s cp.c65 %d" % (cid, c65 & ~0x24 & 0xff), hits + su, hits + bu)   # neither bit
    for bit, actor in ((0x04, su), (0x20, bu)):
        others = ["var %s cp.c65 %d" % (cid, (c65 & ~0x24 & 0xff) | bit)]
        tick(*(others + [hits + player]))                                    # another sender
        tick(*(others + ["touch %s %s" % (actor, cid), hits + actor]))      # not the player
        for _ in range(2):                                                  # damage, then the timer
            tick(*(others + ["touch %s %s" % (actor, player), hits + actor]))
        wait(40)
    tick("var %s cp.c65 %d" % (cid, c65))
    # the knock-back helper: Damage with the shield up (0x0C), in the air (no ray hit) except last
    p = pos_of(cid)
    dmg = "send %s Damage from " % cid
    up = "var %s cp.c65 %d" % (cid, c65 | 0x04)
    if p is not None:
        for c66v, c64v, sender in ((c66 & ~0x20 & 0xff, c64 & 0xfe, su), (c66 | 0x20, c64 & 0xfe, su),
                                   (c66, c64 | 0x01, player), (c66, c64 & 0xfe, player)):
            tick(at(cid, p, 0, 40.0), up, "var %s cp.c66 %d" % (cid, c66v),
                 "var %s cp.c64 %d" % (cid, c64v), dmg + sender)
            wait(5)
        run(at(cid, p, 0))
        wait(20)
        tick("var %s cp.c65 %d" % (cid, c65), "var %s cp.c66 %d" % (cid, c66), "var %s cp.c64 %d" % (cid, c64))
        # real weapons at the commando (the projectile is the sender), shield up and down
        for charged in ("", " charged"):
            run("var %s cp.c65 %d" % (cid, c65 | 0x04))
            run("shoot %s%s 4" % (cid, charged))
            wait(30)
        run("var %s cp.c65 %d" % (cid, c65))
    if last:
        # on the ground: the ray under the commando hits, the commando dies (0x00002FC4)
        tick(up, dmg + su)
        wait(60)


def drown(cid, waters, inactive, player):
    log("--- drown %s" % cid)
    for fluid in (num(player), inactive, 0xffff):
        if fluid is not None:
            tick("var %s cp.fluid %d" % (cid, fluid))
            wait(3)
    p = pos_of(cid)
    for w, wp in waters.items():
        tick("var %s cp.fluid %d" % (cid, num(w)))                          # the head above
        wait(3)
    if waters and p is not None:
        w, wp = next(iter(waters.items()))
        tick("var %s cp.fluid %d" % (cid, num(w)), at(cid, (p[0], p[1], wp[2] - 30.0), 0))
        wait(10)
        log("after drown: %s" % cmd("obj %s" % cid))
    tick("var %s cp.fluid 65535" % cid)


def main():
    wait(100)
    ids = []
    for _ in range(30):  # the area can still be loading
        ids = [line.split()[0] for line in cmd("objs commando").split(" | ")
               if "CCommandoPirate" in line and "(" in line]
        if ids:
            break
        wait(10)
    players = list(objs_of("player", "CPlayer"))
    player = players[0] if players else "u0000"
    aiwps = objs_of("Waypoint", "CScriptAIWaypoint")
    wps = objs_of("Waypoint", "CScriptWaypoint")
    waters = objs_of("Water", "CScriptWater")
    inactive = None
    for line in cmd("objs Water").split(" | "):
        if "CScriptWater" in line and "inactive" in line:
            inactive = num(line.split()[0])
    log("commandos %s player %s aiwps %d wps %d waters %s inactive %s (frame %d)"
        % (ids, player, len(aiwps), len(wps), list(waters), inactive, frame()))
    for i, cid in enumerate(ids):
        last = i == len(ids) - 1
        log("=== commando %s ===" % cid)
        run("send %s Activate" % cid)
        run("place %s player 6" % cid)
        wait(20)
        if want("jump"):
            jump(cid, player)
        if want("follow"):
            follow(cid, player, aiwps, wps)
        if want("charge"):
            charge(cid, aiwps, wps)
        if want("hit"):
            hit(cid, player, last and not want("drown"))
        if want("drown") and last:
            drown(cid, waters, inactive, player)
        log("state %s" % cmd("obj %s" % cid))
    log("done (frame %d)" % frame())


main()
