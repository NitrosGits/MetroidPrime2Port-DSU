# CommandoPirate driver: the six one-answer triggers and the JumpPointFind state, called
# directly (the console's `trigcall` and `statecall`).
#
# usage: commandopirate_trigcalls.py <port> [outfile]
import os
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout

TRIGGERS = [
    "AbortSeekCover", "ShouldCoverAttack", "FoundCover", "IsAggressive",
    "AbortShieldCharge", "HeardShot",
]
STATES = [("JumpPointFind", 0), ("JumpPointFind", 1), ("PathFind", 0), ("Drown", 1)]


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
# A command's reply can take minutes in an attached run (the game is busy with the gdb), the
# same reason tools/drivers/con.py honours CON_REPLY_TIMEOUT.
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
        if line.startswith("=> "):
            lines.append(line)
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


def main():
    wait_frame(200)
    ids = commandos()
    log("commandos %s (frame %d)" % (ids, frame()))
    for target in ids:
        log("=== commando %s ===" % target)
        log("send Activate: %s" % cmd("send %s Activate" % target))
        time.sleep(1.0)
        log("place player: %s" % cmd("place %s player 5" % target))
        wait_frame(frame() + 20)
        for name in TRIGGERS:
            log("trigcall %s: %s" % (name, cmd("trigcall %s %s" % (target, name))))
            wait_frame(frame() + 2)
        # the knock-back: the module's own knock-back helper only runs when the byte at guest
        # this+0xC65 has its 0x04 or 0x08 bit (the shield's charge or the shield arming), so the
        # ArmShield state goes first; then a small damage on the live commando, and the module's
        # KnockBack override (fn_9_C300) is entered and reads the reaction flags.
        log("fsm ArmShield: %s" % cmd("fsm %s ArmShield" % target))
        wait_frame(frame() + 10)
        log("darkburst: %s" % cmd("damage %s 5 darkburst" % target))
        wait_frame(frame() + 10)
        log("damage: %s" % cmd("damage %s 1" % target))
        wait_frame(frame() + 10)
        log("damage: %s" % cmd("damage %s 1" % target))
        wait_frame(frame() + 10)
        # the dead arm: the commando dies and the damage lands on the dead actor.
        log("kill: %s" % cmd("kill %s" % target))
        wait_frame(frame() + 10)
        log("damage dead: %s" % cmd("damage %s 1" % target))
        wait_frame(frame() + 10)
        for name, msg in STATES:
            log("statecall %s %d: %s" % (name, msg, cmd("statecall %s %s %d" % (target, name, msg))))
            wait_frame(frame() + 2)
        log("objs: %s" % cmd("objs commando"))


if __name__ == "__main__":
    main()
