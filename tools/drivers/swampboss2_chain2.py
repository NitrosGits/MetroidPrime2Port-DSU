#!/usr/bin/env python3
"""SwampBossStage2 (module 79): the chain walk (fn_79_5830, fn_79_50AC) and the edge-radius
arm of fn_79_527C.

usage: swampboss2_chain2.py <port> [options]

The driver talks to the port's debug console (MP2_CONSOLE=<port>) and reads the game's log
(SB2_LOG, set by tools/drivers/swampboss2_run.sh) for the temporary chain-walk probe, which
prints the chain node list, the centre and the edge radius.

  * it waits for the area to be loaded and then asks for layer 6, where the boss lives, and
    waits for the boss.  A layer request that arrives while the boot request is still pending
    never finishes, so the wait matters.
  * --send (the default) sends the boss the kSM_AreaLoaded message, which runs the retail's
    chain walk (fn_79_5830 -> fn_79_50AC).  The engine delivers that message at the area load
    as well, so the chain walk runs without any forcing; the send only adds a second one.
  * --states (the default) forces the boss's AI states one by one, which makes the light
    phase's Lurk animation run again - the anim event 0x1b that launches the chain
    (fn_79_55B0 -> fn_79_5030 -> fn_79_527C) is carried by that animation.
  * --place puts the last node of every chain far outside the edge radius and sends
    kSM_AreaLoaded again, so the far position becomes the node's recorded position
    (fn_79_5030 puts every node back on its recorded position before the swing, so without
    the re-record the placement would be undone).  The edge arm of fn_79_527C runs without it.
  * --wait <frames> waits that many game updates instead of forcing the states.
"""
import os
import re
import socket
import sys
import time

STATES = [
    "Lurk", "DarkLurk", "Level", "Swoop", "Circle", "Taunt", "SpitWater", "SpitBarrage",
    "Blow", "DropFlyer", "Reel", "Recover", "Flinch", "BecomeLightFlyer", "BecomeDarkFlyer",
    "Start", "DeathSequence",
]

PROBE_RE = re.compile(
    r"\[probe\] 5830 chain u([0-9a-f]+) centre ([-\d.]+) ([-\d.]+) ([-\d.]+) edge ([-\d.]+) "
    r"nodes (\d+):(.*)")
NODE_RE = re.compile(r"u([0-9a-f]+)\(([-\d.]+) ([-\d.]+) ([-\d.]+)\)")


class Console:
    def __init__(self, port, timeout=300.0):
        self.port = port
        self.timeout = timeout
        self.open()

    def open(self):
        self.sock = socket.create_connection(("127.0.0.1", self.port), timeout=self.timeout)
        self.sock.settimeout(self.timeout)
        self.f = self.sock.makefile("rw")

    def cmd(self, line):
        """One command, its output lines and the '=> ...' end line ('[con] ...' on EOF)."""
        try:
            self.f.write(line + "\n")
            self.f.flush()
        except (OSError, ValueError):
            return [], "[con] closed"
        out = []
        while True:
            try:
                raw = self.f.readline()
            except (OSError, socket.timeout):
                return out, "[con] timeout"
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

    def wait_frame(self, target, timeout=300.0):
        start = time.time()
        while time.time() - start < timeout:
            f = self.frame()
            if f >= 0 and f >= target:
                return f
            time.sleep(0.01 if f >= 0 else 0.2)
        return -1

    def close(self):
        try:
            self.f.close()
            self.sock.close()
        except OSError:
            pass


def read_log(path):
    try:
        with open(path, "r", errors="replace") as f:
            return f.read()
    except OSError:
        return ""


def find_boss(con, timeout=600.0):
    """The boss's unique id, or None: the console's `objs` filter prints the class name."""
    start = time.time()
    while time.time() - start < timeout:
        lines, end = con.cmd("objs swampboss")
        for line in lines:
            fields = line.split()
            if fields and fields[0].startswith("u") and "SwampBoss" in line:
                return fields[0]
        if end.startswith("[con]"):
            print(f"[sb2] the console is gone ({end})", flush=True)
            return None
        con.wait_frame((con.frame() + 10) if con.frame() > 0 else 30)
    return None


def main():
    port = int(sys.argv[1])
    args = sys.argv[2:]
    logfile = os.environ.get("SB2_LOG")
    if logfile is None and args and not args[0].startswith("--"):
        logfile = args.pop(0)
    if logfile is None:
        print("[sb2] no log file (SB2_LOG)", flush=True)
        return 1
    send = "--no-send" not in args
    states = "--no-states" not in args
    place = "--place" in args
    wait = 3000
    for a in args:
        if a.startswith("--wait="):
            wait = int(a.split("=")[1])

    con = Console(port)
    # the area must be loaded before the layer request, or the layer load never finishes.
    con.wait_frame(400)
    lines, end = con.cmd("layer 6")
    print(f"[sb2] layer 6 -> {end}", flush=True)
    boss = find_boss(con)
    if boss is None:
        print("[sb2] no boss in the world", flush=True)
        return 1
    print(f"[sb2] the boss is {boss}", flush=True)

    if send:
        lines, end = con.cmd(f"send {boss} AreaLoaded")
        print(f"[sb2] send {boss} AreaLoaded -> {end}", flush=True)

    # the chain node lists out of the probe's output in the log.
    probes = []
    for _ in range(600):
        probes = [p for p in (PROBE_RE.search(l) for l in read_log(logfile).splitlines()) if p]
        if probes:
            break
        time.sleep(0.1)
    if not probes:
        print("[sb2] no chain node list in the log", flush=True)
    for probe in probes:
        centre = (float(probe.group(2)), float(probe.group(3)), float(probe.group(4)))
        edge = float(probe.group(5))
        nodes = NODE_RE.findall(probe.group(7))
        print(f"[sb2] chain u{probe.group(1)} centre {centre} edge {edge} "
              f"nodes {probe.group(6)}: {[n[0] for n in nodes[:8]]}", flush=True)
        if place and len(nodes) >= 2:
            uid = nodes[-1][0]
            far = (centre[0] + 4.0 * edge + 300.0, centre[1], float(nodes[-1][3]))
            lines, end = con.cmd(f"place u{uid} {far[0]:.2f} {far[1]:.2f} {far[2]:.2f}")
            print(f"[sb2] place u{uid} -> {end} {lines}", flush=True)
            con.wait_frame(con.frame() + 2)
    if place:
        lines, end = con.cmd(f"send {boss} AreaLoaded")
        print(f"[sb2] send {boss} AreaLoaded (re-record) -> {end}", flush=True)

    if states:
        for state in STATES:
            lines, end = con.cmd(f"fsm {boss} {state}")
            print(f"[sb2] fsm {boss} {state} -> {end}", flush=True)
            for l in lines:
                print("   " + l, flush=True)
            con.wait_frame(con.frame() + 40 if con.frame() > 0 else 40)
            if "[arm] 77 527C" in read_log(logfile):
                print("[sb2] the edge arm ran", flush=True)
                break
    else:
        con.wait_frame(con.frame() + wait if con.frame() > 0 else wait)
    for line in read_log(logfile).splitlines():
        if "[arm] " in line or "[probe] " in line:
            print("LOG " + line[:300], flush=True)
    con.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
