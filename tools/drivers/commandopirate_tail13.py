#!/usr/bin/env python3
"""CommandoPirate driver, slice tail13: fn_9_3D64 `x3D64_SetWarpOut` (the warp-out helper).

The Cliff's entrance (world 1baa96c2, layer 3) holds six commandos whose block flag word sets
+0x91D's 0x80 bit (the body's last guard); kSM_Activate then also sets the caller's guard bit
+0xC63 0x20 (0xCD7C), which no shipped message clears - so the driver clears it through the port
console's `var <id> cp.c63` (a host debug accessor, no guest code) in the same game tick as the
Escape / SetToZero message. That reaches both call sites (0xCF4C with the argument 1, 0xCF24 with
0) without forcing anything in the body; the spec's FORCE_RET writes one guard per call instead
(the alive bit, +0xC65 0x10, +0x920, the block flag 0x80), so all four early returns run and every
fifth call is the natural full path.

Five sends per commando = one FORCE_RET period: the first four take the four early returns, the
fifth (n%5==0) is left alone and reaches the state machine and the flag write. The even instances
send Escape (warpOut = true), the odd ones SetToZero (false). After every send the driver reads
cp.c63 back: the 0x10 bit is set exactly when the argument was true.

usage: commandopirate_tail13.py <port> [outfile] [layer]
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 4841
OUT = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout
LAYER = sys.argv[3] if len(sys.argv) > 3 else None

SENDS = 5          # one FORCE_RET period
WAIT = 6           # game updates between two sends (the FSM needs one to run)


def log(msg):
    OUT.write(str(msg) + "\n")
    OUT.flush()
    print(msg, flush=True)


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


def cmd(c):
    """One console command: its reply ends with `=> ok`/`=> err: ...`."""
    stream.write(c + "\n")
    stream.flush()
    lines = []
    while True:
        line = stream.readline()
        if not line:
            break
        line = line.rstrip("\n")
        lines.append(line)
        if line.startswith("=> "):
            break
    return " | ".join(lines)


def chain(cmds):
    """`a; b; c`: the console runs them in one game tick, one reply each."""
    stream.write("; ".join(cmds) + "\n")
    stream.flush()
    replies, cur = [], []
    while len(replies) < len(cmds):
        line = stream.readline()
        if not line:
            break
        line = line.rstrip("\n")
        cur.append(line)
        if line.startswith("=> "):
            replies.append(" | ".join(cur))
            cur = []
    return replies


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
        time.sleep(0.05)


def commandos():
    out = []
    for line in cmd("objs commando").split(" | "):
        if "CCommandoPirate" in line and "(" in line:
            out.append(line.split()[0])
    return out


def var_get(cid, name):
    """`var <id> cp.<name>` -> the value, or None when the accessor is missing."""
    reply = cmd("var %s cp.%s" % (cid, name))
    m = re.search(r"= (-?\d+) \(0x", reply)
    return int(m.group(1)) if m else None


def describe(cid, what):
    reply = cmd("obj %s" % cid)
    fsm = [p.strip() for p in reply.split(" | ") if p.strip().startswith("fsm ")]
    log("  %s: %s" % (what, (", ".join(fsm) if fsm else "no fsm line: " + reply[-120:])))


def main():
    wait(100)
    if LAYER is not None:
        log("layer %s: %s" % (LAYER, cmd("layer %s" % LAYER)))
        wait(12)
    ids = []
    for _ in range(80):
        ids = commandos()
        if ids:
            break
        wait(20)
    log("commandos %s (frame %d)" % (ids, frame()))
    for i, cid in enumerate(ids):
        msg = "Escape" if i % 2 == 0 else "SetToZero"
        log("=== commando %s, sends %s (warpOut %d) ===" % (cid, msg, 1 if msg == "Escape" else 0))
        log("activate: %s" % cmd("send %s Activate" % cid))
        wait(WAIT)
        describe(cid, "after Activate")
        log("  c63 0x%x c65 0x%x x920 %s" % (var_get(cid, "c63") or 0, var_get(cid, "c65") or 0,
                                             var_get(cid, "x920")))
        for r in range(SENDS):
            c63 = var_get(cid, "c63")
            c65 = var_get(cid, "c65")
            if c63 is None or c65 is None:
                log("  object %s is gone, next" % cid)
                break
            cmds = ["var %s cp.c63 %d" % (cid, c63 & ~0x20),
                    "var %s cp.c65 %d" % (cid, c65 & ~0x10),
                    "var %s cp.x920 -1" % cid,
                    "send %s %s" % (cid, msg)]
            last = r == SENDS - 1
            if last:
                # The natural full path: read the flag byte back in the same tick, so the 0x10
                # bit shows which argument the body stored (0x3DF8).
                cmds.append("var %s cp.c63" % cid)
            reply = chain(cmds)
            wait(WAIT)
            after = var_get(cid, "c63")
            log("  send %d/%d: c63 0x%x -> 0x%x (0x10 %s)%s, c65 0x%x -> 0x%x, send: %s" %
                (r + 1, SENDS, c63, after or 0, "set" if (after or 0) & 0x10 else "clear",
                 (" [same tick: %s]" % reply[4]) if last else "", c65,
                 var_get(cid, "c65") or 0, reply[3]))
            describe(cid, "after send %d" % (r + 1))
        log("objs: %s" % cmd("objs commando"))


if __name__ == "__main__":
    main()
