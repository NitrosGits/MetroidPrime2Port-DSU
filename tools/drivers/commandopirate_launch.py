#!/usr/bin/env python3
"""CommandoPirate driver: the module's launching message (fn_9_3900's caller).

The commando in 05_temple_federationsite_dark is inactive in this harness (the area's layer
script does not run), so it is woken with the console's `send <id> Activate`.  The module's own
launching message (the fourcc 0x584C4155, kSM_Launching) is then sent to it with the console's
`send <id> <msg>`: the module's message switch (fn_9_CB40) runs its kSM_Launching case, which
calls the module's jump-back velocity helper (fn_9_3900).  It is sent twice: the helper sets the
0x08 bit of the flag byte at guest this+0xC63 (0x00003A24), so the second call takes the early
out (0x0000393C).

usage: commandopirate_launch.py <port> [outfile]
"""
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout

# the module's own launching message: the fourcc "XLAU" (kSM_Launching)
LAUNCH = "0x584c4155"


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
        wait_frame(frame() + 20)
        # the module's launching message: the kSM_Launching case of fn_9_CB40 calls fn_9_3900.
        # It is sent several times in a row: the helper sets the 0x08 bit of the flag byte at
        # guest this+0xC63 (0x00003A24) and the next call then takes the early out
        # (0x0000393C).  The bit is cleared by the module's landed message (0x0000D054), so the
        # sends have to follow each other without a landing in between.
        for i in range(4):
            log("send launching %d: %s" % (i, cmd("send %s %s" % (target, LAUNCH))))
        wait_frame(frame() + 20)
        # and once more after a while, in case the bit was cleared in between.
        log("send launching later: %s" % cmd("send %s %s" % (target, LAUNCH)))
        wait_frame(frame() + 20)
        log("state: %s" % cmd("obj %s" % target))
    log("done")


main()
