#!/usr/bin/env python3
"""Force the Elite Pirate's TryCommand re-delivery arm (fn_15_BAF8/BC04/BD10).

The three TryCommand copies re-deliver their command when the body controller is not in the
command's animation state while the FSM updates (retail 0xBB94).  The body consumes a command in
the frame it arrives, so the FSM's first update always finds the body in the command's state - the
arm needs a command that arrives while the body is busy with another attack animation.

This driver wakes the Ingsmasher, then forces the FSM into two attack states in consecutive game
ticks: the first puts the body into its attack animation, the second is delivered while the body
is still in that animation (the body states only look for their own command), so the FSM's update
re-delivers it.

The lane's run had a temporary probe in the port (`[ep-probe] TC update cmd=.. body=.. m=..
can=..`, where can=1 is the re-delivery); the driver reads those lines when the log has them and
reports the samples.

usage: ep_retry.py <console port> <log file> [seconds]
"""
import collections
import re
import socket
import sys
import time

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 47717
LOGPATH = sys.argv[2] if len(sys.argv) > 2 else "/tmp/opencode/ep2/ep_retry.log"
DURATION = float(sys.argv[3]) if len(sys.argv) > 3 else 180.0

OBJ_RE = re.compile(
    r"^u([0-9a-f]{4})\s+([0-9a-f]{8})\s+(\S+)\s+a(\d+)"
    r"(?:\s+\(([-\d.]+), ([-\d.]+), ([-\d.]+)\))?", re.M)
PROBE_RE = re.compile(r"\[ep-probe\] TC update cmd=(\d+) body=(\d+) m=(\d+) can=(\d+)")

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
        if len(text) > 300:
            text = text[:300] + "..."
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


def probe_lines():
    try:
        with open(LOGPATH, errors="replace") as fh:
            return PROBE_RE.findall(fh.read())
    except OSError:
        return []


# The pairs: the first state puts the body into its attack animation, the second one is delivered
# while the body is still in it.
SEQUENCES = [
    ("MeleeAttack", "ProjectileAttack"),   # melee body state, then the loop-attack command
    ("ProjectileAttack", "MeleeAttack"),   # loop body state, then the melee command
    ("MeleeAttack", "Shockwave"),          # melee body state, then the projectile command
    ("Shockwave", "MeleeAttack"),          # projectile body state, then the melee command
    ("ProjectileAttack", "Shockwave"),
    ("SpreadShot", "MeleeAttack"),
    ("MeleeAttack", "SpreadShot"),
]


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
    log("Ingsmashers: %s" % sorted(found.items()))
    uid = sorted(found)[0]
    x, y, z = found[uid]
    # stand next to the pirate so it has a target
    con.act("tp %.2f %.2f %.2f" % (x + 5.0, y + 0.5, z))
    con.act("send u%s Activate" % uid)
    con.act("send u%s Alert" % uid)
    # let the fight run: the FSM's SelectTarget code picks the player, which the projectile
    # command needs (without a target the shockwave state never issues its command)
    log("letting the fight run for 25 s")
    time.sleep(25.0)

    seen = collections.Counter()
    hits = {}
    counted = 0
    round_no = 0
    while el() < DURATION:
        for first, second in SEQUENCES:
            round_no += 1
            con.act("fsm u%s %s" % (uid, first))
            # the console drains its queue once per game update: wait past this one so the second
            # command lands in a later tick than the first
            time.sleep(0.25)
            con.act("fsm u%s %s" % (uid, second))
            time.sleep(1.0)
            lines = probe_lines()
            for cmd, body, mstate, can in lines[counted:]:
                seen[(cmd, body, mstate, can)] += 1
                if can == "1":
                    hits.setdefault(cmd, (body, mstate, can))
            counted = len(lines)
            log("round %d %s->%s: re-deliveries so far %s"
                % (round_no, first, second, sorted(hits)))
            if len(hits) >= 3:
                log("all three copies re-delivered: %s" % sorted(hits.items()))
                break
        if len(hits) >= 3:
            break
    # kill the pirate: the death path destroys the actor, which runs its destructor
    for _ in range(6):
        con.act("damage u%s 5000" % uid)
        time.sleep(2.0)
    time.sleep(5.0)
    try:
        with open(LOGPATH, errors="replace") as fh:
            text = fh.read()
        log("destructor runs: %d" % text.count("[ep-probe] dtor"))
    except OSError:
        pass
    if hits:
        log("probe samples: %s" % sorted(seen.items(), key=lambda kv: -kv[1]))
        return 0
    log("no re-delivery for every copy; hits %s" % sorted(hits.items()))
    log("probe samples: %s" % sorted(seen.items(), key=lambda kv: -kv[1]))
    return 4


if __name__ == "__main__":
    sys.exit(main())
