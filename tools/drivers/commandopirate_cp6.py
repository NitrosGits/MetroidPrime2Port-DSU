#!/usr/bin/env python3
"""CommandoPirate driver, slice cp6: the state bodies and triggers through `statecall`/`trigcall`,
each arm on demand, with the module's fields (`var <id> cp.*`), the body state (`bodystate`) and
the actor's place/rotation (`place`) forced around the call and put back in the same tick.

GetUp (fn_9_6A40), Cover (fn_9_6DBC), RestoreOrientation (fn_9_786C), CoverBlown (fn_9_AFD4),
EGrenadeAttack (fn_9_A2EC), PostEGrenadeAttack (fn_9_A110), JetBoost (fn_9_9BB4),
JumpPointFind (fn_9_9F24), WarpIn (fn_9_9064).

usage: commandopirate_cp6.py <port> [outfile|-] [sections, e.g. cover,warpin (default: all)]
Run (the coverage build):
  BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/commandopirate_cp6.py <out>
    swamp,05_swamp_timber! --fast --frames 3000
    --driver 'python3 tools/drivers/commandopirate_cp6.py {port} <out>/drv.txt'
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
        if cls in line:
            m = re.search(r"\((-?[\d.]+), (-?[\d.]+), (-?[\d.]+)\)", line)
            if m:
                out[line.split()[0]] = tuple(float(v) for v in m.groups())
    return out


def var(cid, name, value=None):
    out = cmd("var %s cp.%s%s" % (cid, name, "" if value is None else " %d" % value))
    m = re.search(r"= (-?\d+)", out)
    return int(m.group(1)) if m else None


def forced(cid, name, value, *body):
    """a forced field around calls, put back in the same tick."""
    old = var(cid, name)
    tick("var %s cp.%s %d" % (cid, name, value), *body, "var %s cp.%s %d" % (cid, name, old))


def uid(u):
    return int(u[1:], 16)


def at(cid, p, yaw=None, pitch=0.0):
    rot = "" if yaw is None else " %d %.3f" % (yaw, pitch)
    return "place %s %.3f %.3f %.3f%s" % (cid, p[0], p[1], p[2], rot)


def cp6(cid, player):
    log("=== commando %s ===" % cid)
    run("send %s Activate" % cid)
    run("place %s player 6" % cid)
    wait(20)
    me = objs_of("commando", "CCommandoPirate").get(cid)
    covers = list(objs_of("Cover", "Cover").items())
    jumps = list(objs_of("JumpPoint", "JumpPoint").items())
    wps = list(objs_of("Waypoint", "Waypoint").items())
    log("me %s covers %s jumps %s waypoints %d" % (me, covers[:6], jumps[:6], len(wps)))
    run("obj %s" % cid)
    back = [at(cid, me, 0), "bodystate %s 5" % cid] if me else ["bodystate %s 5" % cid]
    pl = "var %s cp.xbca %d" % (cid, uid(player))

    # -- GetUp (fn_9_6A40)
    if want("getup"):
        gu = "statecall %s GetUp" % cid
        tick(gu + " 0", "bodystate %s 5" % cid, gu + " 1")       # not lying; the getup command
        tick(gu + " 2", gu + " 1")                                # not ready: CanIssueCommand false
        tick(gu + " 0", "bodystate %s 2" % cid, gu + " 1")       # lying, on the mesh
        if me:                                                    # lying, off the mesh: the death
            tick(at(cid, (me[0], me[1], me[2] + 800.0)), "bodystate %s 2" % cid, gu + " 1",
                 "var %s cp.pendingdeath" % cid, "var %s cp.pendingdeath 0" % cid, *back)
        tick(gu + " 2", *back)
        wait(4)

    # -- Cover (fn_9_6DBC)
    if want("cover"):
        cv = "statecall %s Cover" % cid
        tick("bodystate %s 19" % cid, cv + " 0", *back)          # already in the cover state
        tick(cv + " 0")
        forced(cid, "xbd2", 0xffff, cv + " 1")                    # no cover point
        for c, _ in covers[:8]:
            forced(cid, "xbd2", uid(c), cv + " 0", cv + " 1")     # the cover command
            forced(cid, "xbd2", uid(c), cv + " 0", "bodystate %s 19" % cid, cv + " 1", *back)
            forced(cid, "xbd2", uid(c), cv + " 2", "bodystate %s 0" % cid, cv + " 1", *back)
            forced(cid, "xbd2", uid(c), cv + " 2", cv + " 1")    # cannot issue, can shoot
        tick(cv + " 2")
        wait(4)

    # -- RestoreOrientation (fn_9_786C)
    if want("restore"):
        ro = "statecall %s RestoreOrientation" % cid
        if me:
            tick(at(cid, me, 0), ro + " 0", ro + " 1")            # upright
            tick(at(cid, me, 0, 90.0), ro + " 0", ro + " 1", ro + " 1", ro + " 2", *back)  # the turn
            tick(at(cid, me, 0, 1.0), ro + " 1 1.0", *back)       # within one frame's turn
            tick(at(cid, me, 30, 1.0), ro + " 1 0.0001", ro + " 2", *back)
        wait(4)

    # -- CoverBlown (fn_9_AFD4)
    if want("coverblown"):
        cb = "trigcall %s CoverBlown" % cid
        forced(cid, "xbca", 0xffff, cb)                           # no target
        tick(pl, "place %s player 0.2" % cid, cb, *back)          # target within the minimum range
        tick(pl, "place %s player 40" % cid, "var %s cp.xbd2 65535" % cid, cb, *back)  # no cover
        for c, cp_ in covers[:2]:
            for o, op in covers[:3] + wps[:2]:
                if o == c:
                    continue
                tick("var %s cp.xbca %d" % (cid, uid(o)), "var %s cp.xbd2 %d" % (cid, uid(c)),
                     at(cid, (op[0] + 30.0, op[1], op[2])), cb, *back)
        tick(pl)
        wait(4)

    # -- EGrenadeAttack (fn_9_A2EC)
    if want("egrenade"):
        eg = "statecall %s EGrenadeAttack" % cid
        tick(pl, eg + " 0", eg + " 1")                            # the projectile command
        forced(cid, "xbca", 0xffff, eg + " 0", eg + " 1")         # no aim position
        tick(eg + " 2", eg + " 1")                                # cannot issue
        tick(eg + " 2")
        wait(4)

    # -- PostEGrenadeAttack (fn_9_A110)
    if want("postegrenade"):
        pe = "statecall %s PostEGrenadeAttack" % cid
        t0 = "var %s cp.fsmtime 0" % cid
        forced(cid, "xbca", 0xffff, pe + " 0")                    # no target: over
        tick(pl, pe + " 0")
        tick("var %s cp.fsmtime 100000" % cid, pe + " 1", t0)     # the pause is over
        forced(cid, "xbca", 0xffff, t0, pe + " 1")                # no aim position
        if me:
            for yaw in range(0, 360, 30):                         # the turn and no turn
                tick(pl, t0, at(cid, me, yaw), pe + " 1", *back)
        if covers and me:                                         # on the target: no direction
            c, cp_ = covers[0]
            tick("place %s 1 2 3" % c, "place %s 1 2 3" % cid, "var %s cp.xbca %d" % (cid, uid(c)), t0,
                 pe + " 1", at(c, cp_), pl, *back)
        tick(pe + " 2")
        wait(4)

    # -- JetBoost (fn_9_9BB4) and JumpPointFind (fn_9_9F24)
    if want("jump"):
        jb = "statecall %s JetBoost" % cid
        jf = "statecall %s JumpPointFind" % cid
        forced(cid, "xbd0", 0xffff, jb + " 0")                    # no jump point
        for j, _ in jumps[:6]:
            forced(cid, "xbd0", uid(j), jb + " 0")
        for c, _ in covers[:1]:
            forced(cid, "xbd0", uid(c), jb + " 0")                # not a jump point
        if jumps and me:
            for yaw in range(0, 360, 30):
                forced(cid, "xbd0", uid(jumps[0][0]), at(cid, me, yaw), jb + " 0", jb + " 1", *back)
            forced(cid, "xbd0", uid(jumps[0][0]), jb + " 2", jb + " 1")
        tick(jb + " 2")
        tick(jf + " 1", jf + " 2")                                # not the activate
        tick("var %s cp.xb78 0" % cid, jf + " 0")                 # timer below 2 s
        tick("var %s cp.xb78 5000" % cid, "var %s cp.xbca 65535" % cid, jf + " 0", pl)  # no aim
        tick("var %s cp.xb78 5000" % cid, pl, jf + " 0")
        log("xbd0 after JumpPointFind %s" % var(cid, "xbd0"))
        if me:
            for j, jp in jumps[:6]:
                tick("var %s cp.xb78 5000" % cid, pl, at(cid, jp), jf + " 0", *back)
                log("xbd0 %s" % var(cid, "xbd0"))
        wait(4)

    # -- WarpIn (fn_9_9064)
    if want("warpin"):
        wi = "statecall %s WarpIn" % cid
        tick(wi + " 0")
        tick("var %s cp.x920 0" % cid, wi + " 1")                 # the waypoints and the near list
        if me:
            tick("var %s cp.x920 0" % cid, "place %s player 0" % cid, wi + " 1", *back)  # collides
        tick(wi + " 0", "var %s cp.x920 1" % cid, "bodystate %s 5" % cid, wi + " 1")  # generate cmd
        c65 = var(cid, "c65")
        tick("var %s cp.c65 %d" % (cid, c65 & ~0x40 & 0xff), "var %s cp.x920 1" % cid,
             "bodystate %s 12" % cid, wi + " 1",                  # generating: keep the time
             wi + " 1",                                           # the fade
             "var %s cp.xc58 1" % cid, wi + " 1",                 # the fade below zero
             "var %s cp.xc58 0" % cid, wi + " 1",                 # no kept time
             "var %s cp.c65 %d" % (cid, c65), *back)
        tick(wi + " 2", "var %s cp.x920 1" % cid, wi + " 1")      # cannot issue, not generating
        tick("var %s cp.x920 7" % cid, wi + " 1", wi + " 2")
        wait(10)
    log("state %s" % cmd("obj %s" % cid))


def main():
    wait(100)
    ids = []
    for _ in range(30):  # the area can still be loading
        ids = [line.split()[0] for line in cmd("objs commando").split(" | ")
               if "CCommandoPirate" in line and "(" in line]
        if ids:
            break
        wait(10)
    log("commandos %s (frame %d)" % (ids, frame()))
    players = list(objs_of("player", "CPlayer"))
    for cid in ids:
        cp6(cid, players[0] if players else "u0000")
    log("done (frame %d)" % frame())


main()
