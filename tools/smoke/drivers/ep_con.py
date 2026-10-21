#!/usr/bin/env python3
"""ElitePirate (Ingsmasher) smoke driver.

Boots the game in Sanctuary Fortress area 4 (world 1baa96c2, area b666b655) with layer 3
activated (MP2_INPUT LAYER3), then drives the console (MP2_CONSOLE) to wake the Ingsmashers
and poll their FSM state, printing the state histogram at the end.

Phases:
  A (0-35 s): player 25 m from the Ingsmasher, facing it, a few shots -> ranged attacks
  B (35-75 s): player 7 m from it -> melee / shock wave
  C (75-120 s): player 4 m from it -> shock wave / angry attack

Console commands: objs elitepirate, obj u<uid>, send u<uid> Activate|Alert, tp, face, press.
"""

import collections
import math
import os
import re
import socket
import sys
import time

PORT = int(os.environ.get("MP2_CONSOLE", "47412"))
LOGPATH = sys.argv[1] if len(sys.argv) > 1 else ""
DURATION = float(sys.argv[2]) if len(sys.argv) > 2 else 120.0

OBJ_RE = re.compile(
    r"^u([0-9a-f]{4})\s+([0-9a-f]{8})\s+(\S+)\s+a(\d+)"
    r"(?:\s+\(([-\d.]+), ([-\d.]+), ([-\d.]+)\))?", re.M)
FSM_RE = re.compile(r"^\s*fsm (.+)$", re.M)

START = time.monotonic()


def el():
    return time.monotonic() - START


def log(text):
    print("[%7.1f] %s" % (el(), text), flush=True)


class Console:
    def __init__(self, port):
        self.port = port
        self.sock = None

    def cmd(self, line, timeout=8.0):
        for _ in range(3):
            try:
                if self.sock is None:
                    self.sock = socket.create_connection(("127.0.0.1", self.port), timeout=5.0)
                self.sock.settimeout(timeout)
                self.sock.sendall((line + "\n").encode())
                buf = b""
                while b"=> " not in buf:
                    chunk = self.sock.recv(65536)
                    if not chunk:
                        raise ConnectionError("closed")
                    buf += chunk
                return buf.decode(errors="replace")
            except Exception as exc:
                log("console error on %r: %r" % (line, exc))
                try:
                    if self.sock:
                        self.sock.close()
                except Exception:
                    pass
                self.sock = None
                time.sleep(0.5)
        return ""

    def act(self, line, timeout=8.0):
        out = self.cmd(line, timeout)
        text = " ".join(out.split())
        if len(text) > 400:
            text = text[:400] + "..."
        log("### %s -> %s" % (line, text))
        return out


def wait_gameplay(timeout=300.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with open(LOGPATH, errors="replace") as fh:
                if "gameplay reached" in fh.read():
                    log("gameplay reached")
                    return True
        except OSError:
            pass
        time.sleep(1.0)
    return False


def pirates(con):
    out = con.cmd("objs elitepirate", timeout=12.0)
    found = {}
    for m in OBJ_RE.finditer(out):
        if m.group(3) == "CElitePirate" and m.group(5) is not None:
            found[m.group(1)] = (float(m.group(5)), float(m.group(6)), float(m.group(7)))
    return found


def state_of(con, uid):
    out = con.cmd("obj u%s" % uid, timeout=8.0)
    m = FSM_RE.search(out)
    return (m.group(1).strip() if m else "-"), out


def main():
    con = Console(PORT)
    if not wait_gameplay():
        log("no gameplay reached; aborting driver")
        return 2
    time.sleep(3.0)

    found = {}
    for _ in range(90):
        found = pirates(con)
        if found:
            break
        time.sleep(2.0)
    if not found:
        log("no CElitePirate objects found")
        return 3
    log("Ingsmashers: %s" % sorted(found.items()))

    uids = sorted(found)
    for uid in uids:
        con.act("send u%s Activate" % uid)
    time.sleep(2.0)
    for uid in uids:
        con.act("send u%s Alert" % uid)
    time.sleep(2.0)

    counts = {u: collections.Counter() for u in uids}
    prev = {u: "-" for u in uids}
    transitions = {u: [] for u in uids}
    shots = 0

    phases = [(0.0, 35.0, 25.0), (35.0, 75.0, 7.0), (75.0, DURATION, 4.0)]
    next_phase = 0
    next_shot = 6.0
    target = uids[0]

    while el() < DURATION:
        t = el()
        while next_phase < len(phases) and t >= phases[next_phase][0]:
            _, _, dist = phases[next_phase]
            pos = found.get(target)
            if pos:
                tx, ty, tz = pos
                # stand `dist` metres away on the +x side, same height, and face the target
                sx, sy, sz = tx + dist, ty + 0.5, tz
                con.act("tp %.2f %.2f %.2f" % (sx, sy, sz))
                con.act("face %.1f" % math.degrees(math.atan2(tx - sx, ty - sy)))
                log("phase %d: player at (%.1f, %.1f, %.1f), %.1f m from u%s"
                    % (next_phase, sx, sy, sz, dist, target))
            next_phase += 1
        if t >= next_shot and next_shot < 40.0:
            con.act("press b 6")
            shots += 1
            next_shot += 3.0
        found = pirates(con)
        if not found:
            found = {u: found.get(u) for u in uids}
        for uid in uids:
            st, _raw = state_of(con, uid)
            counts[uid][st] += 1
            if st != prev[uid]:
                transitions[uid].append("%.1f %s -> %s" % (el(), prev[uid], st))
                log("fsm u%s %s -> %s" % (uid, prev[uid], st))
                prev[uid] = st
        time.sleep(0.5)

    log("shots fired: %d" % shots)
    for uid in uids:
        log("u%s state counts: %s" % (uid, dict(counts[uid].most_common())))
        log("u%s transitions: %s" % (uid, "; ".join(transitions[uid])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
