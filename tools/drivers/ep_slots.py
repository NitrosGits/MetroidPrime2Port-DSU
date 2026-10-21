#!/usr/bin/env python3
"""Exercise the Elite Pirate's one-line virtual overrides (the REL's own bodies).

The port implements the REL's own copies of a few base-class virtuals as one-liners
(GetSearchPath, TakeDamage, CanBeShot, IsListening, Listen, GetAttackTarget, GetRagDoll,
GetGravityConstant, GetCollisionPrimitive, ProjectileInfo, CanBeUnPossessed).  Most of them are
called by the host in a normal fight; three need a setup:

  - `shoot` fires a projectile (CGameProjectile), and a projectile's collision asks the target
    whether it can be shot (CanBeShot) and whether it has a ragdoll (GetRagDoll);
  - `noise` makes the state manager tell the listeners about a noise (Listen);
  - the rest run on their own.

usage: ep_slots.py <console port> <log file> [seconds]
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 47717
LOGPATH = sys.argv[2] if len(sys.argv) > 2 else "/tmp/opencode/ep2/ep_slots.log"
DURATION = float(sys.argv[3]) if len(sys.argv) > 3 else 120.0

OBJ_RE = re.compile(
    r"^u([0-9a-f]{4})\s+([0-9a-f]{8})\s+(\S+)\s+a(\d+)"
    r"(?:\s+\(([-\d.]+), ([-\d.]+), ([-\d.]+)\))?", re.M)
PROBE_RE = re.compile(r"\[ep-probe\] (fn_15_[0-9A-F]+)")

START = time.monotonic()


def el():
    return time.monotonic() - START


def log(text):
    print("[%7.1f] %s" % (el(), text), flush=True)


class Console:
    def __init__(self, port):
        self.port = port
        self.sock = None

    def cmd(self, line, timeout=15.0):
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

    def act(self, line, timeout=15.0):
        out = self.cmd(line, timeout)
        text = " ".join(out.split())
        if len(text) > 200:
            text = text[:200] + "..."
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
    out = con.cmd("objs elitepirate", timeout=20.0)
    found = {}
    for m in OBJ_RE.finditer(out):
        if m.group(3) == "CElitePirate" and m.group(5) is not None:
            found[m.group(1)] = (float(m.group(5)), float(m.group(6)), float(m.group(7)))
    return found


def probes():
    try:
        with open(LOGPATH, errors="replace") as fh:
            return PROBE_RE.findall(fh.read())
    except OSError:
        return []


def main():
    con = Console(PORT)
    if not wait_gameplay():
        log("no gameplay reached; aborting")
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
    uid = sorted(found)[0]
    x, y, z = found[uid]
    log("Ingsmashers: %s" % sorted(found.items()))
    con.act("tp %.2f %.2f %.2f" % (x + 6.0, y + 0.5, z))
    con.act("send u%s Activate" % uid)
    con.act("send u%s Alert" % uid)
    log("letting the fight run for 20 s")
    time.sleep(20.0)
    while el() < DURATION:
        # a projectile at the pirate: its collision asks CanBeShot and GetRagDoll
        con.act("shoot u%s" % uid)
        time.sleep(1.0)
        # a noise at the pirate: the state manager tells the listeners (Listen)
        con.act("noise 0 %.2f %.2f %.2f" % (x, y, z))
        time.sleep(1.0)
        con.act("shoot u%s charged 8" % uid)
        time.sleep(1.0)
        con.act("noise 2 %.2f %.2f %.2f" % (x, y, z))
        time.sleep(1.0)
    counts = {}
    for p in probes():
        counts[p] = counts.get(p, 0) + 1
    log("probe counts: %s" % sorted(counts.items()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
