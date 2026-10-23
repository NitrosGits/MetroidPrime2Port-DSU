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
aiwp: fake AI waypoints (`var <id> cp.mkaiwp <flags>`, no NEXT link) for the picker and the follow
  state's flag arms.
shield: the ShouldShieldCharge trigger (fn_9_B250) through `trigcall`, each gate forced in one tick
  (the charge band cp.chargemin/chargemax; the rays and the team answer forced by the spec).
drown: fn_9_3090 with the fluid list forced (`var <id> cp.fluid`): none, not water, the inactive
  water, an active water above and below the head. The first commando drowns; the last one dies
  in the air from a shot (the knock-back helper's clear ray).

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
    for v in [w << 4 for w in range(15)] + [0x01, 0x100, 0x1F0, 0x200, 0xF0]:
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
    tick("var %s cp.wpdest 65535" % cid, "bodystate %s 13" % cid,
         "var %s cp.xbca 65535" % cid, fap + " 1", "var %s cp.xbca %d" % (cid, num(player)))
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
    # the hit-object helper with a shield id that is no collision actor (0x00003AA8)
    tick("var %s cp.c65 %d" % (cid, c65 | 0x04), "var %s cp.xbc6 %d" % (cid, num(player)),
         hits + player, "var %s cp.xbc6 %d" % (cid, shield), "var %s cp.c65 %d" % (cid, c65))
    # the knock-back helper: Damage on the ground (the ray under the commando is blocked)
    dmg = "send %s Damage from " % cid
    up = "var %s cp.c65 %d" % (cid, c65 | 0x04)
    for c66v, c64v, sender in ((c66 & ~0x20 & 0xff, c64 & 0xfe, su), (c66 | 0x20, c64 & 0xfe, su),
                               (c66, c64 | 0x01, player), (c66, c64 & 0xfe, player)):
        tick(up, "var %s cp.c66 %d" % (cid, c66v), "var %s cp.c64 %d" % (cid, c64v), dmg + sender,
             "var %s cp.c66 %d" % (cid, c66), "var %s cp.c64 %d" % (cid, c64))
        wait(5)
    # the shield id that is no collision actor (0x00002D50)
    tick("var %s cp.xbc6 %d" % (cid, num(player)), dmg + player, "var %s cp.xbc6 %d" % (cid, shield))
    tick("var %s cp.c65 %d" % (cid, c65))
    # real weapons: at the commando (the projectile is the sender) and at the shield's collision
    # actor (the actor is the sender, the projectile its last touched object), c64's 0x01 bit off/on
    for target, c64v in ((cid, c64 & 0xfe), (cid, c64 | 0x01), (su, c64 & 0xfe), (su, c64 | 0x01)):
        for charged in ("", " charged"):
            run("place %s player 8" % cid)
            tick("var %s cp.c65 %d" % (cid, c65 | 0x04), "var %s cp.c64 %d" % (cid, c64v),
                 "shoot %s%s 4" % (target, charged))
            wait(20)
            tick("var %s cp.c65 %d" % (cid, c65), "var %s cp.c64 %d" % (cid, c64))
            wait(20)
    if last:
        # in the air: the ray under the commando is clear and the commando dies (0x00002FC4),
        # the weapon's damage recorded (0x00003068)
        p = pos_of(cid)
        # (the ray's answer forced clear by the spec while xb54 is 4242)
        if p is not None:
            tick("var %s cp.c65 %d" % (cid, c65 | 0x04), "var %s cp.xb54 4242" % cid, "shoot %s 4" % cid)
            wait(40)
            if pos_of(cid) is not None:
                tick("var %s cp.c65 %d" % (cid, c65 | 0x04), "var %s cp.xb54 4242" % cid,
                     "shoot %s charged 4" % cid)
                wait(40)
            # the dying commando once more without a weapon (the player sends): no damage source
            if pos_of(cid) is not None:
                tick("var %s cp.c65 %d" % (cid, c65 | 0x04), "var %s cp.xb54 4242" % cid,
                     dmg + player)


def aiwp(cid, player):
    log("--- aiwp %s" % cid)
    # fake AI waypoints without a NEXT link: the picker's flag 0x20 (fn_9_2A98 0x00002B80), the
    # follow state's 0x02 and 0x04 (fn_9_961C 0x00009788, 0x00009790) and none
    fakes = {f: var(cid, "mkaiwp", f) for f in (0x20, 0x02, 0x04, 0x00)}
    log("fake AI waypoints %s" % fakes)
    ssc = "statecall %s ScriptedShieldCharge" % cid
    loop = "bodystate %s 9" % cid
    c64 = var(cid, "c64")
    tick("var %s cp.wpdest %d" % (cid, fakes[0x20]), ssc + " 0")
    tick("var %s cp.wpdest %d" % (cid, fakes[0x20]), loop, "var %s cp.c64 %d" % (cid, c64 & 0xf3),
         "var %s cp.b94ahead -2000" % cid, ssc + " 1")
    tick(ssc + " 2", "var %s cp.c64 %d" % (cid, c64), "bodystate %s 5" % cid)
    fap = "statecall %s FollowAttackPattern" % cid
    aiwps = objs_of("Waypoint", "CScriptAIWaypoint")
    p = pos_of(cid)
    for f in (0x02, 0x04, 0x00):
        w = "u%04x" % fakes[f]
        wp = aiwps.get(w)
        tick("var %s cp.wpdest %d" % (cid, fakes[f]), fap + " 1")      # 10 units away
        if wp is not None:
            tick(at(cid, wp, 0), "var %s cp.wpdest %d" % (cid, fakes[f]), fap + " 1")  # on it
            tick(at(cid, (wp[0] + 60.0, wp[1], wp[2]), 0), "var %s cp.wpdest %d" % (cid, fakes[f]),
                 fap + " 1")
    tick(fap + " 2", "var %s cp.wpdest 65535" % cid)
    if p is not None:
        run(at(cid, p, 0))
    run("place %s player 6" % cid)
    wait(20)


def shield(cid, player):
    """ShouldShieldCharge (fn_9_B250) through `trigcall`, every gate forced in one tick."""
    log("--- shield %s" % cid)
    tc = "trigcall %s ShouldShieldCharge" % cid
    names = ("c64", "flags1", "xb50", "xb54", "xbca", "team", "chargemin", "chargemax")
    old = {n: var(cid, n) for n in names}
    log("shield vars %s" % old)
    pl = objs_of("player", "CPlayer").get(player)
    others = [line.split()[0] for line in cmd("objs Relay").split(" | ")
              if line.startswith("u") and "Relay" in line]
    log("relays %s" % others[:3])

    def go(*sets, place=None):
        body = ["var %s cp.%s %d" % (cid, n, v) for n, v in sets]
        if place is not None:
            body.insert(0, place)
        restore = ["var %s cp.%s %d" % (cid, n, old[n]) for n in names]
        log("%s" % cmd("; ".join(body + [tc] + restore), len(body) + 1 + len(restore)))

    base = [("c64", (old["c64"] | 0x40) & ~0x01 & 0xff), ("flags1", old["flags1"] & ~0x40 & 0xff),
            ("xb50", 10), ("xb54", 0), ("xbca", num(player)), ("team", 0xffff),
            ("chargemin", 2000), ("chargemax", 20000)]

    def with_(**kw):
        return [(n, kw.get(n, v)) for n, v in base]

    go(*with_(c64=old["c64"] & ~0x40 & 0xff))                         # 0x0000B2BC
    go(*with_(flags1=old["flags1"] | 0x40))                           # 0x0000B2C8
    go(*with_(c64=old["c64"] | 0x41))                                 # 0x0000B2D0
    go(*with_(xb50=0, xb54=0))                                        # 0x0000B2E0
    go(*with_(xbca=0xffff))                                           # 0x0000B2FC
    if others:
        go(*with_(xbca=num(others[0])))                               # not an actor (host)
    if pl is None:
        return
    def near(dx, dz=0.0):
        return "place %s %.3f %.3f %.3f 0" % (cid, pl[0] + dx, pl[1], pl[2] + dz)
    go(*base, place=near(10.0, 8.0))                                  # |dz| > 5, 0x0000B348
    go(*base, place=near(1.0))                                        # too near, 0x0000B370
    go(*base, place=near(40.0))                                       # too far, 0x0000B384
    go(*(with_(chargemin=0) + [("attarget", 1)]))                     # no direction, 0x0000B394
    go(*with_(team=num(player)), place=near(10.0))                    # the team says no (natural)
    for k in range(4):                                                # the rays: F, TF, TTF, TTT
        go(*with_(team=0xffff if k == 0 else num(player)), place=near(10.0))
    run("place %s player 6" % cid)
    wait(20)


def drown(cid, player):
    log("--- drown %s" % cid)
    waters = objs_of("Water", "CScriptWater")
    inactive = None
    for line in cmd("objs Water").split(" | "):
        if "CScriptWater" in line and "inactive" in line:
            inactive = num(line.split()[0])
    log("waters %s inactive %s" % (list(waters), inactive))
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
        for _ in range(5):
            tick(at(cid, (wp[0], wp[1], wp[2] - 30.0), 0), "var %s cp.fluid %d" % (cid, num(w)))
            wait(2)
            if pos_of(cid) is None:
                break
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
    log("commandos %s player %s aiwps %d wps %d (frame %d)"
        % (ids, player, len(aiwps), len(wps), frame()))
    # the first commando runs every section and drowns; the last one (if another) dies in the air
    for i, cid in enumerate(ids):
        first, last = i == 0, i == len(ids) - 1
        log("=== commando %s ===" % cid)
        run("send %s Activate" % cid)
        run("place %s player 6" % cid)
        wait(20)
        if first:
            if want("jump"):
                jump(cid, player)
            if want("follow"):
                follow(cid, player, aiwps, wps)
            if want("charge"):
                charge(cid, aiwps, wps)
            if want("aiwp"):
                aiwp(cid, player)
            if want("shield"):
                shield(cid, player)
        if want("hit"):
            hit(cid, player, last and not first)
        if want("drown") and first:
            drown(cid, player)
        log("state %s" % cmd("obj %s" % cid))
    log("done (frame %d)" % frame())


main()
