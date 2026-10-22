#!/usr/bin/env python3
"""CommandoPirate driver, slice cp8 (paired with tools/cov/specs/commandopirate_cp8.py).

shield: the ArmShield state (fn_9_86B4) and the shield charge (fn_9_8028) forced (`fsm`), so the
  shield animation's kUE_Activate/kUE_Deactivate events run fn_9_1F88/fn_9_1EF8 and the charge's
  exit runs fn_9_1EF8; the turn-on sound's handle (guest this+0xC38) feeds fn_9_4BC0.
hits: alive hits with the knock-back powers of the c300 driver (kAR_Hurled: the hurled sound,
  guest this+0xC04), waited out so fn_9_4BC0 sees the handle playing, queued and over.
effect: the shield effect (fn_9_2048) from the Dead state (fn_9_69C8): cp.c64's 0x01 bit cleared,
  cp.c66's 0x20 bit set and the state word cp.c4c written, then `fsm Dead` (Lurk in between).
dead: the console's `kill` and hits in the same tick (the death sound, also guest this+0xC04).

usage: commandopirate_cp8.py <port> [outfile|-] [sections, e.g. shield,hits,dead (default: all)]
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


def bits(cid, name, mask, on, *body):
    """a flag bit forced around the calls, put back in the same tick."""
    old = var(cid, name)
    new = (old | mask) if on else (old & ~mask & 0xff)
    tick("var %s cp.%s %d" % (cid, name, new), *body, "var %s cp.%s %d" % (cid, name, old))


ALIVE_HITS = ["1 200", "1 200 missile", "1 60 supermissile", "1 60 darkburst", "1 60 sunburst",
              "1 60 sonicboom"]
DEAD_HITS = ["1 60 supermissile", "1 60 darkburst", "1 200 missile", "1 200", "1 60 sunburst",
             "1 60 sonicboom"]


def cp8(cid, last):
    log("=== commando %s ===" % cid)
    run("send %s Activate" % cid)
    run("place %s player 6" % cid)
    wait(20)
    if want("shield"):
        for _ in range(3):
            run("fsm %s ArmShield" % cid)
            wait(200)
            log("obj: %s" % cmd("obj %s" % cid))
            run("place %s player 14" % cid)
            run("fsm %s ShieldCharge" % cid)
            wait(200)
            run("place %s player 6" % cid)
            wait(60)
    if want("effect"):
        for i, (c4c, c64) in enumerate([(1, 0), (0, 0), (1, 0), (0, 0), (2, 0), (-1, 0), (1, 1),
                                        (1, 0), (0, 0), (1, 0), (0, 0)]):
            run("fsm %s Lurk" % cid)
            wait(20)
            if i % 2 == 0:
                # the shield's turn-on sound (guest this+0xC38): fn_9_1F88 (the shield animation's
                # kUE_Activate) starts it and sets cp.c66's 0x20 bit, so the bit is cleared first
                # and the Dead entry follows the frame the bit comes back
                run("var %s cp.c66 %d" % (cid, var(cid, "c66") & ~0x20 & 0xff))
                run("fsm %s ArmShield" % cid)
                for _ in range(300):
                    if var(cid, "c66") & 0x20:
                        break
                    wait(1)
                log("shield on at frame %d" % frame())
            # the state word is reset between ticks (0x00001D0C), so all in the Dead entry's tick
            tick("var %s cp.c64 %d" % (cid, (var(cid, "c64") & ~1 & 0xff) | c64),
                 "var %s cp.c66 %d" % (cid, var(cid, "c66") | 0x20),
                 "var %s cp.c4c %d" % (cid, c4c), "fsm %s Dead" % cid)
            wait(40)
            log("c4c after: %s" % var(cid, "c4c"))
    if want("hits"):
        for hit in ALIVE_HITS:
            run("damage %s %s" % (cid, hit))
            wait(240)
            run("place %s player 6" % cid)
            wait(10)
    if want("dead") and last:
        tick("kill %s" % cid, *["damage %s %s" % (cid, h) for h in DEAD_HITS])
        wait(300)
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
    for i, cid in enumerate(ids):
        cp8(cid, True)
    log("done (frame %d)" % frame())


main()
