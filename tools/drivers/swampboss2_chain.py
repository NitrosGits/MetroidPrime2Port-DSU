#!/usr/bin/env python3
"""SwampBossStage2 (module 79) driver: the chain walk and the anim event 0x1b.

usage: swampboss2_chain.py <port> [--no-states]

The driver talks to the port's debug console (MP2_CONSOLE=<port>):

  1. it waits until the boss is in the world (the `objs` list) and then sends it the
     kSM_AreaLoaded message - that message is what drives the retail's chain walk
     (fn_79_5830 -> fn_79_50AC), and the harness creates the boss with the layer, so it never
     gets the area load for free. This is a *forced* setup (see the module note).
  2. it then forces the boss's AI states one by one (`fsm <id> <state>`) to find the state
     whose animation carries the anim event 0x1b - the one that launches the platform chain
     (fn_79_55B0 -> fn_79_5030 / fn_79_527C).

The branch evidence lands in the game's log (the port's temporary arm probe), not here: this
script only drives and logs the console replies.
"""
import socket
import sys
import time

# The boss's states, in the order the port registers them.
STATES = [
    "Lurk", "DarkLurk", "Level", "Swoop", "Circle", "Taunt", "SpitWater", "SpitBarrage",
    "Blow", "DropFlyer", "Reel", "Recover", "Flinch", "BecomeLightFlyer", "BecomeDarkFlyer",
    "Start", "DeathSequence",
]


class Console:
    def __init__(self, port, timeout=60.0):
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=timeout)
        self.sock.settimeout(timeout)
        self.f = self.sock.makefile("rw")

    def cmd(self, line):
        self.f.write(line + "\n")
        self.f.flush()
        out = []
        while True:
            raw = self.f.readline()
            if not raw:
                return out, "[con] eof"
            raw = raw.rstrip("\n")
            if raw.startswith("=> "):
                return out, raw
            out.append(raw)

    def frame(self):
        lines, _ = self.cmd("frame")
        for l in lines:
            if l.startswith("frame "):
                return int(l.split()[1])
        return -1

    def wait_frame(self, target, timeout=120.0):
        start = time.time()
        while time.time() - start < timeout:
            f = self.frame()
            if f >= 0 and f >= target:
                return f
            time.sleep(0.02)
        return -1

    def close(self):
        try:
            self.f.close()
            self.sock.close()
        except OSError:
            pass


def find_boss(con):
    """The boss's unique id, or None: the console's `objs` filter prints the class name."""
    for _ in range(600):
        lines, _ = con.cmd("objs swampboss")
        for line in lines:
            fields = line.split()
            if fields and fields[0].startswith("u") and "SwampBoss" in line:
                return fields[0]
        con.wait_frame((con.frame() + 10) if con.frame() > 0 else 30)
    return None


def main():
    port = int(sys.argv[1])
    args = [a for a in sys.argv[2:] if not a.startswith("--")]
    states = args if args else ([] if "--no-states" in sys.argv else STATES)
    con = Console(port)
    con.wait_frame(30)
    # The boss lives in layer 6 of area 32.
    lines, end = con.cmd("layer 6")
    print(f"[sb2] layer 6 -> {end}", flush=True)
    boss = find_boss(con)
    if boss is None:
        print("[sb2] no boss in the world", flush=True)
        return 1
    print(f"[sb2] the boss is {boss}", flush=True)
    # The chain walk: the boss's kSM_AreaLoaded message (forced).
    lines, end = con.cmd(f"send {boss} AreaLoaded")
    print(f"[sb2] send {boss} AreaLoaded -> {end}", flush=True)
    for l in lines:
        print("   " + l, flush=True)
    # The anim event 0x1b: force the states one by one and let each play a while.
    for state in states:
        lines, end = con.cmd(f"fsm {boss} {state}")
        print(f"[sb2] fsm {boss} {state} -> {end}", flush=True)
        for l in lines:
            print("   " + l, flush=True)
        con.wait_frame(con.frame() + 40 if con.frame() > 0 else 40)
    con.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
