#!/usr/bin/env python3
"""CommandoPirate driver: the KnockBack override (fn_9_C300) in area 59.

The commando in 05_temple_federationsite_dark is inactive in this harness (the area's layer
script does not run), so it is woken with the console's `send <id> Activate` and put in front of
the player. It is then hit alive with a range of weapons and knock-back powers, and finally killed
(the console's `kill`) and hit again in the same tick, so the override's alive and dead arms see
the reactions the rule set gives (the arms want kAR_Hurled).

usage: commandopirate_c300.py <port> [outfile]
"""
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout


def log(msg):
    OUT.write(str(msg) + "\n")
    OUT.flush()
    print(msg, flush=True)


def connect():
    for _ in range(240):
        try:
            return socket.create_connection(("127.0.0.1", PORT), timeout=10)
        except OSError:
            time.sleep(0.5)
    raise SystemExit("no console on port %d" % PORT)


sock = connect()
# A world load under gdb (a forcing run) can hold the console for longer than the connect
# timeout (the same fix as the core lane's driver).
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
    for line in cmd("objs commando").split("\n"):
        if "CCommandoPirate" in line and "(" in line:
            out.append(line.split()[0])
    return out


def state_of(cid):
    for line in cmd("obj %s" % cid).split("\n"):
        if line.strip().startswith("fsm"):
            return line.strip()
    return "?"


# The hits: <amount> [knock-back power] [weapon]. The rule set (the knock-back manager's RULE)
# picks the reaction from the weapon type and the power; the override's arms want kAR_Hurled.
ALIVE_HITS = ["1", "1 20", "1 200", "1 200 missile", "1 60 supermissile", "1 60 darkburst",
              "1 60 sunburst", "1 60 sonicboom"]
# Hit in the same tick as the kill, so the dying actor is still there; the ones that may end it
# at once (the power bomb, the annihilator) go last.
DEAD_HITS = ["1 60 supermissile", "1 60 darkburst", "1 200 missile", "1 200", "1 60 sunburst",
             "1 60 sonicboom", "1 0 light", "1 0 dark", "1 60 cannonball", "1 60 annihilator",
             "1 60 powerbomb"]


def main():
    wait_frame(200)
    ids = []
    for _ in range(120):
        ids = commandos()
        if ids:
            break
        time.sleep(1.0)
    log("commandos %s (frame %d)" % (ids, frame()))
    if not ids:
        return
    target = ids[0]
    log("send Activate: %s" % cmd("send %s Activate" % target))
    wait_frame(frame() + 30)
    log("place: %s" % cmd("place %s player 6" % target))
    wait_frame(frame() + 30)
    for hit in ALIVE_HITS:
        log("damage %s: %s | %s" % (hit, cmd("damage %s %s" % (target, hit)), state_of(target)))
        wait_frame(frame() + 90)
        cmd("place %s player 6" % target)
        wait_frame(frame() + 10)
    # The dead arm: the console's `kill` runs the Death (the alive flag at guest this+0x420 goes)
    # and the hits land in the same tick, before the dying actor can be deleted.
    line = "kill %s; " % target + "; ".join("damage %s %s" % (target, h) for h in DEAD_HITS)
    log("dead: %s" % cmd(line))
    wait_frame(frame() + 60)
    log("objs: %s" % cmd("objs commando"))


if __name__ == "__main__":
    main()
