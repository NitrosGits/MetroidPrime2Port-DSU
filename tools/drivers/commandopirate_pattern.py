#!/usr/bin/env python3
"""CommandoPirate driver: the attack pattern in the Swamp's timber area.

The commando with the attack pattern lives in 05_swamp_timber! (the Swamp, Temple Grounds),
layer '1st Pass' - the only shipped CommandoPirate whose connections include "ATTK"/"FOLW"
(tools/scan_commando_pattern.py lists them). The area's AI waypoints carry the 0x20 flag the
ShouldBoost trigger (fn_9_B1E0) wants, so the commando's FollowAttackPattern state (fn_9_961C)
walks the chain and the scripted shield charge (fn_9_7A98) follows.

usage: commandopirate_pattern.py <port> [outfile]

The steps:
  * the commando is made active with the console's `send <id> Activate` (the area's own layer
    script does not run in this harness, so the actor stays inactive);
  * the FollowAttackPattern state is forced with the console's `fsm` - the area's own route into
    it goes through the RestoreOrientation state, which is still a port stand-in, so the state
    would never be entered on its own;
  * the run then watches the commando's state machine: the chain's waypoints carry the 0x20 flag,
    so the ShouldBoost trigger answers true and the scripted charge runs.
"""
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4796
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


def commando():
    cid = None
    pos = None
    for line in cmd("objs commando").split("\n"):
        if "CCommandoPirate" in line and "(" in line:
            cid = line.split()[0]
            try:
                pos = [float(x) for x in line[line.index("(") + 1:line.rindex(")")].split(",")]
            except ValueError:
                pass
    return cid, pos


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


def main():
    wait_frame(200)
    ids = commandos()
    cid, pos = commando()
    log("commandos %s; %s at %s (frame %d)" % (ids, cid, pos, frame()))
    if not ids:
        return
    for target in ids:
        log("=== commando %s ===" % target)
        # the actor is inactive in this harness (the area's layer script does not run), so it is
        # woken with the console's `send`.
        log("send Activate: %s" % cmd("send %s Activate" % target))
        time.sleep(1.0)
        log("state: %s" % state_of(target))
        log("fsm: %s" % cmd("fsm %s FollowAttackPattern" % target))
        seen = []
        for i in range(120):
            st = state_of(target)
            if not seen or seen[-1] != st:
                seen.append(st)
                log("frame %d: %s" % (frame(), st))
            if i % 24 == 12:
                # the state is forced again now and then: the commando charges away from its
                # waypoints, so a later entry sees it facing the other way.
                cmd("fsm %s FollowAttackPattern" % target)
            time.sleep(0.25)
        log("states seen: %s" % seen)
        # the JumpBack state: the commando jumps away from its target; the console forces it when
        # the area's own state machine does not go there.
        log("fsm: %s" % cmd("fsm %s JumpBack" % target))
        for i in range(10):
            wait_frame(frame() + 10)
        # the RestoreOrientation state: the commando only reaches it after a warp, so the console
        # forces it. The first force leaves the state (its animation is already "over"), and its
        # deactivate tips the commando over - so the second force runs the state's own turn.
        log("fsm: %s" % cmd("fsm %s RestoreOrientation" % target))
        wait_frame(frame() + 30)
        log("fsm: %s" % cmd("fsm %s RestoreOrientation" % target))
        for i in range(20):
            wait_frame(frame() + 10)
            st = state_of(target)
            if i % 5 == 0:
                log("frame %d: %s" % (frame(), st))
    log("commando: %s" % cmd("objs commando"))


if __name__ == "__main__":
    main()
