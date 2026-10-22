#!/usr/bin/env python3
"""Splitter (REL 75) gameplay driver (the helpers lane's copy): the lane's plans S/H/C/D/K/E/F/G/N/R
plus the helpers lane's plan Z, timed in wall seconds (run at normal speed).
usage: MP2_CONSOLE=<port> splitter_helpers_drv.py <game log> <duration> <after>"""
import collections
import math
import os
import re
import socket
import sys
import time

PORT = int(os.environ.get("MP2_CONSOLE", "47412"))
LOGPATH = sys.argv[1] if len(sys.argv) > 1 else "/tmp/opencode/smoke/ep.log"
DURATION = float(sys.argv[2]) if len(sys.argv) > 2 else 240.0
# How long the driver keeps running after it activated the Splitter.
AFTER = float(sys.argv[3]) if len(sys.argv) > 3 else 60.0

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


def splitters(con):
    """The Splitter's chassis and head, as (uid, class name) pairs."""
    out = con.cmd("objs splitter", timeout=12.0)
    found = []
    for m in OBJ_RE.finditer(out):
        if m.group(3) in ("CSplitterMainChassis", "CSplitterCommandModule"):
            found.append((m.group(1), m.group(3)))
    return found

def ring(r, z=0.0, t0=0.0, step=3.0):
    out = []
    for k in range(8):
        a = math.radians(45 * k)
        out.append((t0 + step * k, "tp:%.1f:%.1f:%.1f" % (r * math.sin(a), r * math.cos(a), z)))
    return out

def missiles(t0, n):
    out = [(t0, "give Missile 30"), (t0 + 0.2, "give UnlimitedMissiles 1"), (t0 + 0.5, "press l 0")]
    for k in range(n):
        out += [(t0 + 1 + 2 * k, "press l+y 8"), (t0 + 1.5 + 2 * k, "press l 0")]
    return out

def boost(t0, tp):
    return [(t0, tp), (t0 + 0.5, "press x 4"), (t0 + 2.0, "stick 0 127 0"), (t0 + 2.2, "press b 50"),
            (t0 + 3.5, "press b 50"), (t0 + 5.0, "press b 50"), (t0 + 7.0, "stick 0 0 0"), (t0 + 7.5, "press x 4")]


PLANS = {
    # quadrants near (stab legs), far (sweep), above (sweep up), lock-on missiles from 4 sides
    "A": missiles(1, 4) + [(10, "press l+a 3"), (11, "press l+y 8"), (12, "press l+a 3"), (13, "press l+y 8")]
         + ring(25, 0, 16, 2.5) + ring(40, 0, 36, 2.5) + ring(15, 30, 56, 2.5) + ring(25, -3, 76, 2.5)
         + [(96, "tp:0:14:0")] + missiles(97, 4) + [(106, "press l+a 3"), (107, "press l+y 8"), (108, "press l+a 3"), (109, "press l+y 8")]
         + [(112, "tp:14:0:0")] + missiles(113, 4) + [(122, "press l+a 3"), (123, "press l+y 8")]
         + [(126, "tp:-14:0:0")] + missiles(127, 4) + [(136, "tp:0:-14:0")] + missiles(137, 4)
         + ring(25, 0, 148, 2.5) + ring(15, 30, 168, 2.5),
    "B": [(0, "give MorphBall 1"), (0.3, "give BoostBall 1"), (0.6, "mid")] + missiles(1, 6)
         + [(14, "mid")] + missiles(15, 6) + [(28, "mid")] + missiles(29, 6)
         + boost(42, "tp:0:7:0") + boost(52, "tp:7:0:0") + boost(62, "tp:-7:0:0") + boost(72, "tp:0:-7:0")
         + boost(82, "tp:0:5:0") + boost(92, "tp:5:0:0")
         + [(102, "mid")] + missiles(103, 6) + ring(25, 0, 116, 2.5) + [(137, "dmgc:30")]
         + ring(20, 0, 140, 3.0) + [(165, "mid")] + missiles(166, 6),
    "K": [(1, "pick:0532"), (2, "tp:0:9:0"), (3, "give Missile 30")]
         + [(4 + 1.5 * k, "damage u%04x 2" % (0x677 + k % 6)) for k in range(24)]
         + [(4.7 + 1.5 * k, "dump:obj u0532") for k in range(0, 24, 3)]
         + ring(18, 0, 42, 2.5) + ring(10, 6, 62, 2.5) + missiles(80, 4) + ring(25, 0, 90, 3)
         + [(110, "pick:0521"), (111, "give MorphBall 1"), (111.5, "give BoostBall 1")]
         + boost(113, "tp:4:0:0") + boost(123, "tp:-4:0:0") + boost(133, "tp:0:4:0") + boost(143, "tp:0:-4:0")
         + boost(153, "tp:3:3:0") + boost(163, "tp:-3:-3:0") + [(173, "dump:obj u0521"), (174, "dump:objs splitter")],
    "S": [(1, "pick:0532")] + ring(30, 0, 2, 2.5) + ring(35, 0, 32, 2.5) + ring(28, 8, 62, 2.5)
         + [(5, "sweepdmg:0532:0678:135"), (141, "dump:obj u0531"), (142, "dump:objs splitter")]
         + ring(20, 0, 112, 2.5) + ring(12, 4, 132, 2.5)
         + [(150, "pick:0521"), (151, "give MorphBall 1"), (151.5, "give BoostBall 1")]
         + boost(153, "tp:4:0:0") + boost(163, "tp:-4:0:0") + boost(173, "tp:0:4:0") + boost(183, "tp:0:-4:0")
         + [(193, "dump:obj u0521"), (194, "dump:objs splitter")],
    "H": [(1, "pick:0521"), (2, "give MorphBall 1"), (2.5, "give BoostBall 1")]
         + boost(4, "tp:4:0:0") + boost(13, "tp:-4:0:0") + boost(22, "tp:0:4:0") + boost(31, "tp:0:-4:0")
         + boost(40, "tp:3:3:0") + boost(49, "tp:-3:-3:0") + boost(58, "tp:2:0:0")
         + [(68 + k, "damage u0520 25 0 %s" % ("power", "missile", "cannonball")[k % 3]) for k in range(15)]
         + [(84, "dump:obj u0520"), (85, "dump:objs splitter"), (86, "sweepdmg:0521:067e:110")]
         + ring(30, 0, 87, 2.5) + ring(35, 0, 117, 2.5) + ring(25, 8, 147, 2.5) + [(195, "dump:obj u0521")],
    "C": [(1, "pick:0532"), (2, "give MorphBall 1"), (2.5, "give MorphBallBombs 1"), (3, "sweepbomb:0532:0678:200")]
         + ring(30, 0, 4, 2.5) + ring(35, 0, 24, 2.5) + ring(28, 8, 44, 2.5) + ring(30, 0, 64, 2.5)
         + ring(35, 0, 84, 2.5) + ring(28, 8, 104, 2.5) + ring(30, 0, 124, 2.5) + ring(35, 0, 144, 2.5)
         + [(205, "dump:obj u0532"), (206, "dump:objs splitter")]
         + [(210 + k, "damage u0520 25 0 power") for k in range(8)] + [(225, "dump:obj u0521")],
    "E": [(1, "pick:0521"), (2, "give MorphBall 1"), (2.5, "give MorphBallBombs 1")]
         + [(4 + k, "damage u0520 25 0 power") for k in range(7)] + [(12, "dump:obj u0520")]
         + [(14, "sweepbomb:0521:067e:90")] + ring(30, 0, 15, 2.5) + ring(35, 0, 35, 2.5) + ring(28, 8, 55, 2.5)
         + ring(30, 0, 75, 2.5) + [(104, "pick:0532"), (105, "sweepbomb:0532:0678:90")]
         + ring(30, 0, 106, 2.5) + ring(35, 0, 126, 2.5) + ring(28, 8, 146, 2.5) + ring(30, 0, 166, 2.5)
         + [(212, "dump:objs splitter"), (214, "damage u0531 200 0 power"), (216, "dump:objs splitter")],
    "F": [(1, "pick:0532"), (2, "give MorphBall 1"), (2.5, "give MorphBallBombs 1"), (3, "sweepbomb:0532:0678:120")]
         + ring(30, 0, 4, 2.5) + ring(35, 0, 24, 2.5) + ring(28, 8, 44, 2.5) + ring(30, 0, 64, 2.5)
         + ring(20, 0, 84, 2.5) + ring(12, 4, 104, 2.5) + [(125, "dump:objs splitter")]
         + [(130 + k, "damage u0520 25 0 power") for k in range(7)]
         + [(150, "forcekill:0521:067e"), (156, "dump:obj u0521"), (157, "dump:objs splitter")],
    "G": [(1, "pick:0532"), (2, "give MorphBall 1"), (2.5, "give MorphBallBombs 1"), (3, "sweepbomb:0532:0678:140")]
         + ring(30, 0, 4, 2.5) + ring(35, 0, 24, 2.5) + ring(28, 8, 44, 2.5) + ring(30, 0, 64, 2.5)
         + ring(20, 0, 84, 2.5) + ring(12, 4, 104, 2.5) + ring(30, 0, 124, 2.5) + [(144, "dump:objs splitter")]
         + [(150 + 0.5 * k, "damage u0520 25 0 power") for k in range(7)] + [(156, "dump:obj u0521")]
         + [(165 + k, "damage u0521 %d 0 %s" % (4, ("power", "missile", "cannonball", "bomb")[k % 4])) for k in range(8)]
         + [(175, "dump:obj u0521"), (176, "dump:objs splitter")],
    # N: kill the 0532 chassis with bombs, then circle its undocked head (pick:0531) and park the
    # 0521 chassis on the head's right (flank R) or left (flank L) side, to block one dodge side;
    # from 146 s also hold the head at a fixed height above its nav area (lift) for the dodge's
    # OnPath test; every third 20 s block leaves both sides open. At 214 s a shot kills the head.
    "N": [(1, "pick:0532"), (2, "give MorphBall 1"), (2.5, "give MorphBallBombs 1"), (3, "sweepbomb:0532:0678:76")]
         + ring(30, 0, 4, 2.5) + ring(35, 0, 24, 2.5) + ring(28, 8, 44, 2.5) + ring(30, 0, 64, 2.5)
         + [(79, "pick:0531")] + [(80 + 4.0 * k, "tp:%.1f:%.1f:1" % (14 * math.sin(math.radians(45 * k)), 14 * math.cos(math.radians(45 * k)))) for k in range(33)]
         + [(81 + 0.8 * k, "flank:0531:0521:%s" % "RL"[(k // 25) % 3]) for k in range(160) if (k // 25) % 3 < 2]
         + [(146 + 0.8 * k, "lift:0531:-60") for k in range(80)]
         + [(212, "dump:objs splitter"), (214, "damage u0531 200 0 power"), (216, "dump:objs splitter")],
    # Z (the helpers lane): the head's Listen arms - the noise type 4 (kLNT_PathObstruction) and 3,
    # inside the 20-unit radius (the player 9 units above the picked chassis, the head rides 5 above
    # it) and outside it (60 units up) - and the chassis's own flags (the IM00 message sets the byte
    # at 0xD41's 0x80 bit, which the chassis's CanBeIngPossessed tests).
    "Z": [(1, "pick:0532"), (1.5, "tp:0:9:0"), (3, "noise 4"), (5, "noise 3"), (7, "tp:0:60:0"),
          (9, "noise 4"), (11, "tp:0:9:0"), (13, "noise 4"), (15, "send u0532 0x494d3030"),
          (17, "tp:0:9:0"), (19, "noise 4"), (22, "dump:objs splitter")],
    # R: N without the flank/lift, the head forced back into LaserPulse every 6 s (fsm reaches the
    # DetachedAttackLogic sub-machine), so each pulse ends in a dodge pick with both sides open.
    "R": [(1, "pick:0532"), (2, "give MorphBall 1"), (2.5, "give MorphBallBombs 1"), (3, "sweepbomb:0532:0678:76")]
         + ring(30, 0, 4, 2.5) + ring(35, 0, 24, 2.5) + ring(28, 8, 44, 2.5) + ring(30, 0, 64, 2.5)
         + [(79, "pick:0531")] + [(80 + 4.0 * k, "tp:%.1f:%.1f:1" % (14 * math.sin(math.radians(45 * k)), 14 * math.cos(math.radians(45 * k)))) for k in range(33)]
         + [(82 + 6.0 * k, "fsm u0531 LaserPulse") for k in range(22)]
         + [(212, "dump:objs splitter")],
    # S (the helpers lane): the chassis's Dodge state entered directly with the console, so its
    # three messages run (statecall runs one state body once with a message: 0 activate,
    # 1 update, 2 deactivate), and the head's PathFind state, whose Update asks the separation.
    "S": [(1, "pick:0532"), (3, "statecall u0532 Dodge 0"), (4, "statecall u0532 Dodge 1"),
          (6, "statecall u0532 Dodge 1"), (8, "statecall u0532 Dodge 2"),
          (10, "fsm u0531 PathFind"), (12, "statecall u0532 Dodge 1"),
          (14, "statecall u0532 Dodge 2")],
    # K2 (the helpers lane): the chassis takes a cannonball hit (its knock-back, fn_75_EBF4) and
    # the head is put into the floor (its collision, fn_75_6178).
    "K2": [(1, "pick:0532"), (2, "damage u0532 25 0 cannonball"), (4, "pick:0531"),
           (6, "lift:0531:-127.0"), (8, "damage u0531 25 0 power"),
           (12, "damage u0532 25 0 missile"), (16, "damage u0531 25 0 missile")],
    # E2/E3 (the helpers lane): the chassis takes a boost ball hit, so fn_75_EBF4's knock-back
    # runs; E3 puts the chassis into its spin attack first, so fn_75_BAA8 has set the flag byte
    # 0xD41's 0x80 bit (the arm that turns both reactions off).
    "E2": [(1, "pick:0532"), (3, "damage u0532 5 boostball")],
    "E3": [(1, "pick:0532"), (3, "fsm u0532 SpinAttack"), (6, "damage u0532 5 boostball")],
    # E4: the chassis's laser sweep; its beam's collision actor is the chassis itself, so the
    # chassis takes the damage and fn_75_EBF4's knock-back runs.
    "E4": [(1, "pick:0532"), (2, "tp:0:9:0"), (4, "fsm u0532 LaserSweepAttack")],
    # K3 (the helpers lane): kill the 0532 chassis, so the 0531 head loses its chassis (its
    # LostChassisReaction state turns it upright, fn_75_2994) and re-docks to the 0521 chassis
    # (its SeekMainChassis state moves it, fn_75_2154, and the chassis reserves the head,
    # fn_75_E788).
    "K3": [(1, "pick:0532"), (2, "dump:objs splitter"), (3, "kill u0532"), (6, "dump:objs splitter"),
           (12, "dump:objs splitter"), (20, "dump:objs splitter")],
    # K5 (the helpers lane): the chassis alive, with dumps so the death timer's effect on the
    # listing can be seen (the Digital Guardian's DestroyActiveSplitters gives the first
    # chassis the delay 0.0, so its timer at 0xD20 is zero and it dies at the next update).
    "K5": [(1, "pick:0532"), (4, "dump:objs splitter"), (8, "dump:objs splitter"),
           (12, "dump:objs splitter"), (16, "dump:objs splitter")],
    "D": [(2, "dump:objs collision"), (3, "dump:objs splitter"), (4, "dump:obj u%s" % "0532")],
    "T": ring(10, 0, 10) + ring(30, 0, 36) + ring(25, 12, 62) + missiles(88, 6) + [(101, "tp:0:20:0")]
         + missiles(102, 6) + [(115, "tp:20:0:0")] + missiles(116, 6) + [(129, "tp:-20:0:0")]
         + missiles(130, 6) + [(143, "tp:0:-20:0")] + missiles(144, 6) + ring(8, 0, 158, 2.0)
         + [(175, "press a 4")],
}

def chassis_all(con):
    out = con.cmd("objs chassis", timeout=12.0)
    return [(m.group(1), float(m.group(5)), float(m.group(6)), float(m.group(7)))
            for m in OBJ_RE.finditer(out) if m.group(3) == "CSplitterMainChassis" and m.group(5)]

PICK = [None]
SWEEP = [None]
SWK = [0]
LAST_TP = [(0.0, 1.0)]

def chassis_pos(con):
    out = con.cmd("objs splitter", timeout=12.0)
    for m in OBJ_RE.finditer(out):
        if m.group(3) in ("CSplitterMainChassis", "CSplitterCommandModule") and m.group(5) \
                and (PICK[0] == m.group(1) or (PICK[0] is None and m.group(3) == "CSplitterMainChassis")):
            return float(m.group(5)), float(m.group(6)), float(m.group(7))
    return None

def main():
    con = Console(PORT)
    if not wait_gameplay():
        log("no gameplay"); return 2
    time.sleep(3.0)
    if os.environ.get("LAYER"):
        con.act("layer %s" % os.environ["LAYER"])
    plan = PLANS[os.environ.get("PLAN", "T")]
    found = []
    for _ in range(25):
        found = splitters(con)
        if found:
            break
        time.sleep(2.0)
    if not found:
        log("no splitters")
        for f in ("splitter", "chassis", "module", "patterned", "ai "):
            out = con.cmd("objs %s" % f, timeout=12.0)
            log("objs %s: %s" % (f, " ".join(out.split()[-3:])))
            log("  " + " | ".join(out.splitlines()[:8]))
        return 3
    time.sleep(2.0)
    found = splitters(con)
    log("splitters: %s" % found)
    for uid, _ in found:
        con.act("send u%s Activate" % uid)
    t_act = el()
    done = set()
    last_dump = 0.0
    while el() - t_act < AFTER:
        dt = el() - t_act
        if SWEEP[0] and el() < SWEEP[0][2]:
            cu, au = SWEEP[0][0], SWEEP[0][1]
            st, raw = state_of(con, cu)
            if "LaserSweep" in st:
                if len(SWEEP[0]) == 3:
                    for _ in range(5):
                        con.act("damage u%s 5 0 cannonball" % au); SWK[0] += 1
            if "LaserSweep" in st and len(SWEEP[0]) > 3:
                out = con.cmd("objs collision", timeout=12.0)
                for m in OBJ_RE.finditer(out):
                    if m.group(1) == au and m.group(5):
                        con.act("tp %s %s %.2f" % (m.group(5), m.group(6), float(m.group(7)) - 0.6))
                        log("BOMB at u%s (%s, %s, %s)" % (au, m.group(5), m.group(6), m.group(7)))
                        con.act("press x 4"); time.sleep(0.8)
                        for _ in range(3):
                            con.act("press a 4"); time.sleep(0.4)
                        break
                if "damage" in "":
                    pass
            elif "LaserSweep" in st:
                pass
            if "no object" in raw:
                SWEEP[0] = None
        for (t0, act) in plan:
            if dt >= t0 and (t0, act) not in done:
                done.add((t0, act))
                if act.startswith("sweepdmg:"):
                    _, cu, au, dur = act.split(":")
                    SWEEP[0] = (cu, au, el() + float(dur))
                elif act.startswith("sweepbomb:"):
                    _, cu, au, dur = act.split(":")
                    SWEEP[0] = (cu, au, el() + float(dur), "bomb")
                elif act.startswith("flank:") or act.startswith("lift:"):
                    parts = act.split(":")
                    out = con.cmd("objs splitter", timeout=12.0)
                    for m in OBJ_RE.finditer(out):
                        if m.group(1) != parts[1] or m.group(5) is None:
                            continue
                        x, y, z = float(m.group(5)), float(m.group(6)), float(m.group(7))
                        if parts[0] == "lift":
                            con.act("place u%s %.2f %.2f %s" % (parts[1], x, y, parts[2]))
                            continue
                        # The head faces the player, who stands at the last tp offset from it; its
                        # right (transform X) is forward x up = (fy, -fx).
                        fx, fy = LAST_TP[0]
                        n = math.hypot(fx, fy) or 1.0
                        side = 1.6 if parts[3] == "R" else -1.6
                        con.act("place u%s %.2f %.2f %.2f" % (parts[2], x + side * fy / n, y - side * fx / n, z))
                elif act.startswith("forcekill:"):
                    _, cu, au = act.split(":")
                    log("FORCED fsm u%s LaserSweepAttack; damage u%s" % (cu, au))
                    con.act("fsm u%s LaserSweepAttack" % cu)
                    for _ in range(4):
                        con.act("damage u%s 5 0 cannonball" % au)
                elif act.startswith("pick:"):
                    PICK[0] = act[5:]
                elif act.startswith("dump:"):
                    out = con.cmd(act[5:], timeout=12.0)
                    log("DUMP %s\n%s" % (act[5:], out))
                elif act == "mid":
                    cs = chassis_all(con)
                    if cs:
                        mx = sum(c[1] for c in cs) / len(cs); my = sum(c[2] for c in cs) / len(cs)
                        mz = max(c[3] for c in cs)
                        con.act("tp %.2f %.2f %.2f" % (mx, my, mz + 0.5))
                        con.act("face %.1f" % math.degrees(math.atan2(cs[0][1] - mx, cs[0][2] - my)))
                        log("CHASSIS %s" % cs)
                elif act.startswith("dmgc:"):
                    cs = chassis_all(con)
                    if cs:
                        con.act("damage u%s %s" % (cs[0][0], act[5:]))
                        log("CHASSIS %s" % cs)
                elif act.startswith("tp:"):
                    _, ox, oy, oz = act.split(":")
                    ox, oy, oz = float(ox), float(oy), float(oz)
                    p = chassis_pos(con)
                    if p:
                        cx, cy, cz = p
                        LAST_TP[0] = (ox, oy)
                        con.act("tp %.2f %.2f %.2f" % (cx + ox, cy + oy, cz + 0.5 + oz))
                        con.act("face %.1f" % math.degrees(math.atan2(-ox, -oy)))
                else:
                    con.act(act)
        if dt - last_dump > 6.0:
            last_dump = dt
            for suid, sclass in splitters(con):
                st, raw = state_of(con, suid)
                log("SPL %s u%s %s | %s" % (sclass, suid, st, " ".join(raw.split())[:200]))
        time.sleep(0.5)
    log("done")
    return 0

if __name__ == "__main__":
    sys.exit(main())
