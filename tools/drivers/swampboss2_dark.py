#!/usr/bin/env python3
"""SwampBossStage2 (module 79): the dark phase - the growth kills, the Blow and the chains.

usage: swampboss2_dark.py <port> [logfile] [--grapple]

The driver talks to the port's debug console (MP2_CONSOLE=<port>) and reads the game's log
(SB2_LOG, set by tools/drivers/swampboss2_run.sh) for the temporary state dump.

  * the growths are only damageable by a real beam hit (the retail's ProcessDamage casts the
    joint's last toucher to a CWeapon, and only a projectile is one) and only while the boss
    is stunned (the Reel state sets the growths vulnerable) with the player behind it (the
    growths are only active then).  So the driver stuns the boss with the console's `damage`
    on a body joint (alternating with a beam hit, because the boss ignores a second hit from
    the same toucher) and then shoots the four growth joints with `shoot`.
  * in the dark phase the driver forces the Blow state (`fsm`, which reaches the FSM2's
    sub-machines) and stands in front of the boss, so the blow window (the anim event 0x2a)
    reaches the player.  Then it leaves the state with `fsm`, which is the only way the
    deactivate's pop (fn_79_7E24's 0x8214) can run.
  * with --grapple the player grapples a grapple point in front of the boss instead, which is
    fn_79_7E24's 0x804C arm (the window's grapple test).
"""
import math
import os
import re
import socket
import sys
import time

INTS = ("phase", "atk", "rd", "gvuln", "gdam", "lf")
FLOATS = ("head", "ovip")
TUPLES = {"boss": 3, "fwd": 3, "g": 4, "blow": 2}


class Console:
    def __init__(self, port, timeout=300.0):
        self.port = port
        self.timeout = timeout
        self.dead = False
        self.open()

    def open(self):
        last = None
        for _ in range(120):
            try:
                self.sock = socket.create_connection(("127.0.0.1", self.port), timeout=self.timeout)
                self.sock.settimeout(self.timeout)
                self.f = self.sock.makefile("rw")
                return
            except OSError as err:
                last = err
                time.sleep(0.5)
        raise last

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
        """The game update count, or -1 when the console's answer was an error."""
        lines, end = self.cmd("frame")
        if end.startswith("[con]"):
            self.dead = True
            return -1
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


def read_log(path, tail=200000):
    try:
        with open(path, "rb") as f:
            f.seek(0, os.SEEK_END)
            size = f.tell()
            f.seek(max(0, size - tail))
            return f.read().decode("utf-8", "replace")
    except OSError:
        return ""


def _parse_line(best):
    f = best.split()
    st = {}
    i = 1
    while i < len(f):
        key = f[i]
        if key == "j":
            i += 1
            joints = []
            while i + 5 < len(f) + 1 and f[i].startswith("u"):
                joints.append({
                    "uid": f[i][1:],
                    "pos": (float(f[i + 1]), float(f[i + 2]), float(f[i + 3])),
                    "active": int(f[i + 4]),
                    "hp": float(f[i + 5]),
                })
                i += 6
            st["joints"] = joints
            continue
        if key in TUPLES:
            n = TUPLES[key]
            st[key] = tuple(float(v) for v in f[i + 1:i + 1 + n])
            i += 1 + n
            continue
        if key in INTS:
            st[key] = int(f[i + 1])
            i += 2
            continue
        if key in FLOATS:
            st[key] = float(f[i + 1])
            i += 2
            continue
        if key == "st":
            st[key] = f[i + 1]
            i += 2
            continue
        i += 1
    return st


def parse_state(text):
    """The last state line in the log, as a dict, or None."""
    best = None
    for line in text.splitlines():
        if line.startswith("[sb2] boss "):
            best = line
    if best is None:
        return None
    try:
        return _parse_line(best)
    except (IndexError, ValueError):
        return None


def find_boss(con, timeout=600.0):
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


def grapple_points(con):
    lines, _ = con.cmd("objs grapple")
    points = []
    for line in lines:
        fields = line.split()
        if fields and fields[0].startswith("u"):
            m = re.search(r"\(([-\d.]+), ([-\d.]+), ([-\d.]+)\)", line)
            pos = (float(m.group(1)), float(m.group(2)), float(m.group(3))) if m else None
            points.append((fields[0][1:], pos))
    return points


def main():
    port = int(sys.argv[1])
    args = sys.argv[2:]
    logfile = os.environ.get("SB2_LOG")
    if logfile is None and args and not args[0].startswith("--"):
        logfile = args.pop(0)
    if logfile is None:
        print("[sb2] no log file (SB2_LOG)", flush=True)
        return 1
    grapple_mode = "--grapple" in args
    con = Console(port)
    con.wait_frame(400)
    lines, end = con.cmd("layer 6")
    print(f"[sb2] layer 6 -> {end}", flush=True)
    boss = find_boss(con)
    if boss is None:
        print("[sb2] no boss in the world", flush=True)
        return 1
    print(f"[sb2] the boss is {boss}", flush=True)
    points = []
    if grapple_mode:
        con.cmd("give grappleBeam 1")
        points = grapple_points(con)
        print(f"[sb2] the grapple points: {points}", flush=True)

    stun_toggle = False
    last_frame = -1
    last_blow_try = -1000
    acted = False
    while True:
        text = read_log(logfile)
        st = parse_state(text)
        if st is None:
            time.sleep(0.1)
            continue
        frame = con.frame()
        if frame < 0:
            if con.dead:
                print("[sb2] the console is gone", flush=True)
                return 1
            # the console's queue was full: wait a moment and read the state again
            time.sleep(0.05)
            continue
        if frame == last_frame:
            time.sleep(0.02)
            continue
        last_frame = frame
        joints = st.get("joints", [])
        growths = joints[:4]
        bodies = joints[4:]
        if st.get("phase", 0) >= 1:
            # the dark phase: the Blow's deactivate (fn_79_7E24's 0x8214) needs the state left
            # while the blow window is still open and the player is being blown.  The window
            # is tested against the player's aim point, so the player stands in front of the
            # boss and looks past it.  The probe's line marks the frame the player starts
            # being blown, and the state is left right then: the window is only open for a
            # few dozen updates, and the driver's round trip is longer than that.
            if not acted and "7E24 the player is being blown" in text:
                acted = True
                print(f"[sb2] the player is being blown: leaving the state ({frame})", flush=True)
                lines, end = con.cmd(f"fsm {boss} Start")
                print(f"[sb2] fsm {boss} Start -> {end}", flush=True)
            bx, by, bz = st["boss"]
            fx, fy, _ = st["fwd"]
            px, py, pz = bx + fx * 20.0, by + fy * 20.0, bz + 4.0
            aimx, aimy, aimz = bx + fx * 60.0, by + fy * 60.0, bz + 4.0
            yaw = math.degrees(math.atan2(aimx - px, aimy - py))
            pitch = math.degrees(math.atan2(aimz - pz, math.hypot(aimx - px, aimy - py)))
            blow = st.get("blow", (0, 0))
            if st.get("st") == "Blow":
                if grapple_mode:
                    # the grapple point in front of the boss: the aim is the lock-on target
                    best = None
                    for uid, pos in points:
                        if pos is None:
                            continue
                        dot = (pos[0] - bx) * fx + (pos[1] - by) * fy
                        if best is None or dot > best[0]:
                            best = (dot, uid)
                    if best is not None and best[0] > 0.0:
                        con.cmd(f"grapple u{best[1]}")
                else:
                    con.cmd(f"tp {px:.2f} {py:.2f} {pz:.2f}; aim {yaw:.1f} {pitch:.1f}")
            else:
                if frame - last_blow_try > 20:
                    # the dark phase's own path to the Blow needs the attack to be ready, which
                    # the flight does not give, so the state is forced.
                    last_blow_try = frame
                    lines, end = con.cmd(f"fsm {boss} Blow")
                    print(f"[sb2] fsm {boss} Blow -> {end}", flush=True)
                con.cmd(f"tp {px:.2f} {py:.2f} {pz:.2f}; aim {yaw:.1f} {pitch:.1f}")
            con.wait_frame(frame + 2)
            continue
        # the light phase: kill the growths
        if st.get("gvuln"):
            # the boss is stunned: the growths are vulnerable.  The player must be behind the
            # boss for the growths to be active at all.
            alive = [(i, j) for i, j in enumerate(growths) if st["g"][i] > 0.0]
            if alive and not all(j["active"] for _, j in alive):
                bx, by, bz = st["boss"]
                fx, fy, _ = st["fwd"]
                lines, end = con.cmd(f"tp {bx - fx * 30.0:.2f} {by - fy * 30.0:.2f} {bz:.2f}")
                if end.startswith("=> err"):
                    print(f"[sb2] tp -> {end}", flush=True)
                con.wait_frame(frame + 3)
                continue
            for i, j in alive:
                lines, end = con.cmd(f"shoot u{j['uid']} 30")
                if end.startswith("=> err"):
                    print(f"[sb2] shoot u{j['uid']} -> {end}", flush=True)
            con.wait_frame(frame + 6)
            continue
        # not stunned: stun the boss.  The boss ignores a hit from the same toucher, so the
        # console's damage and a real beam hit have to alternate.
        if bodies:
            body = bodies[0] if stun_toggle else bodies[1 % len(bodies)]
            if stun_toggle:
                lines, end = con.cmd(f"damage u{body['uid']} 25")
                print(f"[sb2] damage u{body['uid']} 25 -> {end}", flush=True)
            else:
                lines, end = con.cmd(f"shoot u{body['uid']} 30")
                print(f"[sb2] shoot u{body['uid']} 30 -> {end}", flush=True)
            stun_toggle = not stun_toggle
            con.wait_frame(frame + 2)
            continue
        time.sleep(0.05)


if __name__ == "__main__":
    raise SystemExit(main())
