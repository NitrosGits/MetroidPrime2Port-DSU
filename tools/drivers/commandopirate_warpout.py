#!/usr/bin/env python3
"""CommandoPirate driver: the WarpOut state (fn_9_8DF0) in area 59.

The commando in 05_temple_federationsite_dark is inactive in this harness (the area's layer
script does not run), so it is woken with the console's `send <id> Activate`. The WarpOut
state is then forced with the console's `fsm` - nothing in the shipped data enters it - so
its arms run: the activate, the update (the generate command and the fade that follows it)
and the deactivate, which the state machine sends when the next `fsm` leaves the state.

usage: commandopirate_warpout.py <port> [outfile]
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
    log("commandos %s (frame %d)" % (ids, frame()))
    for target in ids:
        log("=== commando %s ===" % target)
        log("send Activate: %s" % cmd("send %s Activate" % target))
        time.sleep(1.0)
        log("state: %s" % state_of(target))
        log("fsm WarpOut: %s" % cmd("fsm %s WarpOut" % target))
        wait_frame(frame() + 60)
        log("state: %s" % state_of(target))
        # leaving the state sends the deactivate message.
        # the body keeps the generate state after its animation is over, so a second WarpOut
        # straight away (no Lurk in between, the locomotion would change the body's state)
        # starts with a remaining time of zero - the retail's epsilon guard (0x00008F8C).
        log("fsm WarpOut again: %s" % cmd("fsm %s WarpOut" % target))
        wait_frame(frame() + 30)
        log("state: %s" % state_of(target))
        log("fsm Lurk: %s" % cmd("fsm %s Lurk" % target))
        wait_frame(frame() + 20)
        log("state: %s" % state_of(target))
        # the module's own warp-out helper (the guest's fn_9_3D64) runs on the Escape message
        # when the byte at guest this+0xC63 has no 0x20 bit; it sets the 0x10 bit that the
        # PostWarpOut state's delete arm wants.
        log("send Escape: %s" % cmd("send %s Escape" % target))
        wait_frame(frame() + 30)
        log("state: %s" % state_of(target))
        # the knock-back: the shield up, the damage (the module's KnockBack override runs) and
        # then the Dead state, whose shield effect wants the byte at guest this+0xC66's 0x20 bit
        # (the shield's own animation event sets it).
        log("fsm ArmShield: %s" % cmd("fsm %s ArmShield" % target))
        wait_frame(frame() + 30)
        # the dark beam freezes the commando: the frozen knock-back reaction's rules (the
        # retail's UNFZ and STFR) set the reaction flags to 1|2 = 3, which the KnockBack
        # override's "GetUp" arm wants.
        log("darkburst: %s" % cmd("damage %s 5 darkburst" % target))
        wait_frame(frame() + 30)
        log("damage: %s" % cmd("damage %s 5" % target))
        wait_frame(frame() + 30)
        log("fsm Dead: %s" % cmd("fsm %s Dead" % target))
        wait_frame(frame() + 30)
        log("state: %s" % state_of(target))
        # the commando dies (the console's `kill`), then the damage: the KnockBack override's
        # dead arm.
        log("kill: %s" % cmd("kill %s" % target))
        wait_frame(frame() + 30)
        log("state: %s" % state_of(target))
        log("damage again: %s" % cmd("damage %s 5" % target))
        wait_frame(frame() + 30)
        log("state: %s" % state_of(target))
        log("objs: %s" % cmd("objs commando"))


if __name__ == "__main__":
    main()
