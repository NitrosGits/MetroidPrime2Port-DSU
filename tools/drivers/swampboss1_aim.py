#!/usr/bin/env python3
"""Drive the MP2 port's SwampBossStage1 (Chykka Larva) over the debug console.

The port's live-aim driver for the larva's joint spheres: it reads the joint positions from
`objs collision`, puts the player next to the target joint, aims from the gun at the joint's
live position and fires (charged or not).  The charge input is the A button (the tweak binds
`charge` to the same button as `fire`), and the shot leaves when the charge completes, so the
aim has to be refreshed while the charge runs - the player falls in the water.

The console (MP2_CONSOLE=<port>) answers one command per line; every reply ends with
"=> ok" or "=> err: ...".

Every round:
  * read the 16 joint spheres ("objs collision" gives uid + position) and the boss's own
    position ("objs swamp");
  * put the player next to the target joint (the camera sits ~2.5 above the feet, so the
    joint is at the camera's height), then aim from the gun (2.15 above the feet) at the
    joint's live position;
  * hold fire: the charge builds over ~1.8 s and the shot leaves when it completes, so the
    aim is refreshed (and the player put back) while the charge runs - a teleport during
    the charge cancels the shot, so only the aim is refreshed;
  * the boss's HP ("obj u05bb") is the hit oracle.

usage: drive.py <port> [logfile] [target-uid] [charged]
"""
import math
import socket
import sys
import time

GUN_HEIGHT = 2.15          # the shot origin above the player's feet
JOINT_UIDS = [f"u{0x62c + i:04x}" for i in range(16)]   # the 16 joint spheres, kJoints order
BELLY = "u063b"            # kJoints[15] = L_middle2_boob_LCTR, the weak spot
HEAD = "u062c"             # kJoints[0] = head, the biggest sphere (radius 5)


class Console:
    def __init__(self, port, timeout=60.0):
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=timeout)
        self.sock.settimeout(60.0)
        self.f = self.sock.makefile("rw")

    def read_reply(self):
        out = []
        while True:
            raw = self.f.readline()
            if not raw:
                raise EOFError("console closed")
            raw = raw.rstrip("\n")
            if raw.startswith("=> ok") or raw.startswith("=> err:"):
                return "\n".join(out), raw
            out.append(raw)

    def cmd(self, line):
        self.f.write(line + "\n")
        self.f.flush()
        return self.read_reply()

    def batch(self, lines):
        self.f.write("".join(l + "\n" for l in lines))
        self.f.flush()
        return [self.read_reply() for _ in lines]

    def close(self):
        try:
            self.f.close()
            self.sock.close()
        except OSError:
            pass


def positions(text):
    """uid -> (x, y, z) for every object line with a position."""
    out = {}
    for line in text.splitlines():
        f = line.split()
        if len(f) < 7 or not f[0].startswith("u"):
            continue
        if "(" not in line:
            continue
        try:
            p = line[line.index("(") + 1:line.index(")")].split(",")
            out[f[0]] = (float(p[0]), float(p[1]), float(p[2]))
        except (ValueError, IndexError):
            pass
    return out


def player_pos(text):
    for line in text.splitlines():
        f = line.split()
        if f and f[0] == "position" and len(f) >= 4:
            return (float(f[1]), float(f[2]), float(f[3]))
    return None


def boss_hp(text):
    for line in text.splitlines():
        f = line.split()
        if len(f) >= 3 and f[0] == "hp":
            try:
                return float(f[1].split("/")[0])
            except ValueError:
                return None
    return None


def aim_at(player, target):
    dx = target[0] - player[0]
    dy = target[1] - player[1]
    dz = target[2] - (player[2] + GUN_HEIGHT)
    flat = math.hypot(dx, dy)
    pitch = math.degrees(math.atan2(dz, flat)) if flat > 0.01 else 0.0
    return math.degrees(math.atan2(dx, dy)), pitch


def main():
    port = int(sys.argv[1])
    logpath = sys.argv[2] if len(sys.argv) > 2 else None
    target_uid = sys.argv[3] if len(sys.argv) > 3 else BELLY
    charged = (len(sys.argv) > 4 and sys.argv[4] == "charged")
    log = open(logpath, "w", buffering=1) if logpath else sys.stdout

    def emit(msg):
        msg = f"[{time.time() - t_start:7.2f}s] {msg}"
        log.write(msg + "\n")
        if log is not sys.stdout:
            sys.stdout.write(msg + "\n")
            sys.stdout.flush()

    t_start = time.time()
    con = None
    t0 = time.time()
    while con is None and time.time() - t0 < 180:
        try:
            con = Console(port)
        except OSError:
            time.sleep(1.0)
    if con is None:
        emit("[drive] no console connection")
        return 1
    emit("[drive] connected")

    # wait for the area to be loaded (the console only works once the game is in it)
    t0 = time.time()
    while time.time() - t0 < 180:
        try:
            text, status = con.cmd("status")
        except EOFError:
            emit("[drive] console closed (game gone)")
            return 1
        if "area 32" in text:
            break
        time.sleep(1.0)
    emit(f"[drive] area 32 reached after {time.time() - t0:.1f}s")
    time.sleep(22.0)

    # the area's own chain: open the switch the layer controller 002001d7 opens on "Arrived"
    # and set the relay 00200228 the switch 002001e4's "on Open" reaches
    for c in ("send u044d Open", "send u0476 SetToZero", "damage u05bb 1199"):
        text, status = con.cmd(c)
        emit(f"[drive] {c} -> {status}")
    time.sleep(3.0)

    shots = 0
    hits = 0
    last_hp = None
    deadline = time.time() + 900
    while time.time() < deadline:
        try:
            jtext, _ = con.cmd("objs collision")
            btext, _ = con.cmd("objs swamp")
        except EOFError:
            emit("[drive] console closed (game gone)")
            break
        joints = positions(jtext)
        boss = positions(btext)
        # the boss's joint spheres: the collision actors near the boss, in creation order
        bpos = None
        for line in btext.splitlines():
            if "CSwampBossStage1" in line and "(" in line:
                q = line[line.index("(") + 1:line.index(")")].split(",")
                bpos = (float(q[0]), float(q[1]), float(q[2]))
        # the boss's 16 joint spheres are the run of 16 consecutive collision-actor uids
        juids = sorted(joints)
        best = []
        run = []
        for u in juids:
            if run and int(u[1:], 16) == int(run[-1][1:], 16) + 1:
                run.append(u)
            else:
                run = [u]
            if len(run) > len(best):
                best = list(run)
        joint_list = [(u, joints[u]) for u in best]
        if len(joint_list) < 16:
            emit(f"[drive] only {len(joint_list)} joints near the boss")
        if not joint_list:
            emit("[drive] no joints at all")
            time.sleep(0.5)
            continue
        want_index = 15 if target_uid == BELLY else 0
        if want_index >= len(joint_list):
            want_index = 0
        target_uid, target = joint_list[want_index]
        boss_uid = None
        for line in btext.splitlines():
            if "CSwampBossStage1" in line:
                boss_uid = line.split()[0]
                break
        if boss_uid is None:
            emit("[drive] no boss in the list")
            time.sleep(0.5)
            continue
        if target is None:
            if shots == 0 and not hasattr(main, "dumped"):
                main.dumped = True
                emit("[drive] joint reply:\n" + "\n".join(jtext.splitlines()[:8]))
            time.sleep(0.5)
            continue
        stext, _ = con.cmd("status")
        player = player_pos(stext)
        if player is None:
            time.sleep(0.3)
            continue
        # stand off 3 units to the side, at the joint's height (the camera is 2.5 up)
        want = (target[0] + 3.0, target[1], target[2] - 2.5)
        cmds = []
        if player is None or math.dist(player, want) > 2.0:
            cmds.append(f"tp {want[0]:.2f} {want[1]:.2f} {want[2]:.2f}")
            stext, _ = con.cmd("status")
            player = player_pos(stext) or want
        yaw, pitch = aim_at(player, target)
        cmds.append(f"aim {yaw:.1f} {pitch:.1f}")
        if charged:
            cmds.append("press a 0")
            con.batch(cmds)
            # the charge runs ~1.8 s and the shot leaves when it completes: keep the player
            # and the aim on the joint while it moves
            for _ in range(11):
                time.sleep(0.12)
                try:
                    jtext2, _ = con.cmd("objs collision")
                except EOFError:
                    break
                j2 = positions(jtext2)
                t2 = j2.get(target_uid)
                if t2 is None:
                    continue
                con.cmd(f"tp {t2[0]+3.0:.2f} {t2[1]:.2f} {t2[2]-2.5:.2f}")
                stext, _ = con.cmd("status")
                p2 = player_pos(stext)
                if p2 is None:
                    continue
                yaw, pitch = aim_at(p2, t2)
                con.cmd(f"aim {yaw:.1f} {pitch:.1f}")
            con.cmd("stick 0 0 0")
        else:
            cmds.append("press a 2")
            con.batch(cmds)
        shots += 1
        htext, _ = con.cmd(f"obj {boss_uid}")
        hp = boss_hp(htext)
        if hp is not None and last_hp is not None and hp < last_hp - 0.01:
            hits += 1
            emit(f"[drive] HIT boss hp {last_hp:.1f} -> {hp:.1f} (-{last_hp - hp:.2f}) "
                 f"target {target_uid} {target} player {player} yaw {yaw:.1f} pitch {pitch:.1f}")
        if hp is not None:
            last_hp = hp
        if shots % 10 == 0:
            emit(f"[drive] shots {shots} hits {hits} boss hp {hp} target {target_uid} {target}")

    emit(f"[drive] done: shots {shots} hits {hits}")
    con.close()
    log.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
