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


def commando_pos(cid):
    for line in cmd("obj %s" % cid).split("\n"):
        if "(" in line and ")" in line:
            body = line[line.index("(") + 1:line.rindex(")")]
            parts = [p.strip() for p in body.split(",")]
            if len(parts) == 3:
                try:
                    return (float(parts[0]), float(parts[1]), float(parts[2]))
                except ValueError:
                    return None
    return None


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
        # the module's Listen override (fn_9_BE4C): the noises the actor hears. The player is put
        # five metres in front of the commando first, so the shot's distance test is inside the
        # hearing radius, and the five noise types cover the body's four arms.
        log("place: %s" % cmd("place %s player 5" % target))
        wait_frame(frame() + 10)
        # the commando's own position, so a noise can be raised by 5 cm: the height-range arm
        # of the Listen override (fn_9_BE4C) needs the noise's height distance over the base's
        # height range (which the area's commando has at 0).
        pos = commando_pos(target)
        for noise in (0, 4, 1, 2, 3):
            if noise == 0 and pos is not None:
                log("noise 0 (raised): %s" % cmd("noise 0 %.2f %.2f %.2f"
                                                  % (pos[0], pos[1], pos[2] + 0.05)))
            else:
                log("noise %d: %s" % (noise, cmd("noise %d" % noise)))
            wait_frame(frame() + 10)
        # and the same two noises from far away: the body has a distance test of its own.
        log("place far: %s" % cmd("place %s player 80" % target))
        wait_frame(frame() + 10)
        for noise in (0, 4):
            log("noise %d (far): %s" % (noise, cmd("noise %d" % noise)))
            wait_frame(frame() + 10)
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
        log("damage: %s" % cmd("damage %s 5" % target))
        wait_frame(frame() + 30)
        log("fsm Dead: %s" % cmd("fsm %s Dead" % target))
        wait_frame(frame() + 30)
        log("state: %s" % state_of(target))
        # the damage again, now that the commando is dead: the KnockBack override's dead arm.
        log("damage again: %s" % cmd("damage %s 5" % target))
        wait_frame(frame() + 30)
        log("state: %s" % state_of(target))
        log("objs: %s" % cmd("objs commando"))


if __name__ == "__main__":
    main()
