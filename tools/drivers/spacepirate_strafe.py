#!/usr/bin/env python3
"""Coverage driver for SpacePirate GetStrafeDir (fn_72_CE30) through `trigcall <id> ShouldStrafe`
(07_sand_alamoportalobservation L3, troopers 002a00f3 = A and 002a00f4 = B).

usage: spacepirate_strafe.py <port> [spots]
env:   SP_STRAFE_RADII  the distances B is put at around A (default "0.9 1.4 2.0 5.0")

At each spot the player stands still facing +y, A is put 6 units in front of it and given time to
turn to the player; then, for every radius and every 30 degrees around A, `send A Activate` (an
active pirate takes it as a player hit, so the next Think zeroes xb24 and ShouldStrafe's 0.66 s
window opens) and `place B <point>; trigcall A ShouldStrafe` in one tick. A's right axis is taken
from its facing (the player), so the ring covers B on A's right and left, near (the magSquared < 3
arms) and nearer (the 0.866 * magSquared arms), at 5 units (in range, no side dropped, but in the
way of that side's dynamic ray), plus B 30 units off (the distance skip). Last, B and a third
pirate C stand 5 units to A's right and left, so both rays are blocked (no direction).
Then (fn_72_58D8 LaunchGrenade, by `statecall`): both troopers join their squad (Taunt's
Activate runs SquadJoin), A claims the team's projectile slot (granted) and B right after it (the
team's projectile interval has not passed: denied), A's Update issues the loop attack and then
waits, B's Update (not Ready, no grenades left) exits, A's Deactivate runs under `mpforce 1` (the
multiplayer attack-time variation) and B's without; msg 3 is the switch's default. The other
pirates of the area claim and release once each (those in no squad take the no-team arm).
Each console command is sent on its own: a `a; b` line answers with one prompt per command.
"""
import math
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from spacepirate_drive import Console, pirates  # noqa: E402

port = int(sys.argv[1]) if len(sys.argv) > 1 else 50360
nspots = int(sys.argv[2]) if len(sys.argv) > 2 else 4
RADII = [float(r) for r in os.environ.get('SP_STRAFE_RADII', '0.9 1.4 2.0 5.0').split()]
A_EID, B_EID = 0x002A00F3, 0x002A00F4
PLAYER = re.compile(r'CPlayer\s+a\d+ \(([-\d.]+), ([-\d.]+), ([-\d.]+)\)')


def read_prompt(c):
    """The next reply of a `a; b` line (Console.cmd returns at the first prompt)."""
    out = []
    while True:
        r = c.file.readline()
        if not r:
            return '\n'.join(out)
        out.append(r.rstrip())
        if r.startswith('=> '):
            return '\n'.join(out)


def find(c, eid):
    for p in pirates(c.cmd('objs spacepirate')):
        if p[4] == eid:
            return p
    return None


def main():
    c = Console(port)
    if not c.connect(timeout=600):
        print('driver: no console')
        return 1
    c.cmd('layer 3')
    ps = []
    for _ in range(300):
        ps = pirates(c.cmd('objs spacepirate'))
        if find(c, A_EID) and find(c, B_EID):
            break
        time.sleep(1)
    time.sleep(3)
    ps = pirates(c.cmd('objs spacepirate'))
    print('driver: %d pirates' % len(ps), flush=True)
    for p in ps:
        c.cmd('send u%04x Activate' % p[0])
    time.sleep(2)
    spots = [(p[1], p[2] - 6.0, p[3]) for p in ps][:nspots]
    counts = {}
    for si, (px, py, pz) in enumerate(spots):
        a, b = find(c, A_EID), find(c, B_EID)
        if a is None or b is None:
            break
        ua, ub = 'u%04x' % a[0], 'u%04x' % b[0]
        c.cmd('tp %.2f %.2f %.2f' % (px, py, pz))
        c.cmd('face 0')
        time.sleep(0.5)
        c.cmd('place %s player 6' % ua)
        c.cmd('place %s player 30' % ub)
        time.sleep(2.5)
        a = find(c, A_EID)
        m = PLAYER.search(c.cmd('objs cplayer'))
        if a is None or m is None:
            continue
        qx, qy = float(m.group(1)), float(m.group(2))
        fx, fy = qx - a[1], qy - a[2]
        n = math.hypot(fx, fy) or 1.0
        fx, fy = fx / n, fy / n
        rx, ry = fy, -fx  # right = forward x up
        def at(r, deg):
            t = math.radians(deg)
            return (a[1] + r * (math.cos(t) * rx + math.sin(t) * fx),
                    a[2] + r * (math.cos(t) * ry + math.sin(t) * fy), a[3])

        def strafe(label):
            c.cmd('send %s Activate' % ua)
            time.sleep(0.15)
            out = c.cmd('trigcall %s ShouldStrafe' % ua)
            res = 'true' if '= true' in out else 'false' if '= false' in out else 'err'
            counts[res] = counts.get(res, 0) + 1
            print('driver: spot %d %s -> %s' % (si, label, res), flush=True)

        points = [(30.0, 0)] + [(r, d) for r in RADII for d in range(0, 360, 30)]
        points += [(0.9, d) for d in (-20, -10, 10, 20, 160, 170, 190, 200)]
        for r, deg in points:
            c.cmd('place %s %.3f %.3f %.3f' % ((ub,) + at(r, deg)))
            strafe('r %.1f deg %3d' % (r, deg))
        others = [p for p in pirates(c.cmd('objs spacepirate')) if p[4] not in (A_EID, B_EID)]
        if others:
            uc = 'u%04x' % others[0][0]
            for _ in range(4):
                c.cmd('place %s %.3f %.3f %.3f' % ((ub,) + at(5.0, 0)))
                c.cmd('place %s %.3f %.3f %.3f' % ((uc,) + at(5.0, 180)))
                strafe('B right 5, C left 5')
            c.cmd('place %s player 30' % uc)
    print('driver: results %s' % counts, flush=True)
    a, b = find(c, A_EID), find(c, B_EID)
    if a is not None and b is not None:
        ua, ub = 'u%04x' % a[0], 'u%04x' % b[0]
        steps = ['statecall %s Taunt 0' % ua, 'statecall %s Taunt 0' % ub, 'wait 0.5',
                 'statecall %s LaunchGrenade 0' % ua, 'statecall %s LaunchGrenade 0' % ub, 'wait 0.1',
                 'statecall %s LaunchGrenade 1' % ua, 'wait 0.3', 'statecall %s LaunchGrenade 1' % ua,
                 'statecall %s LaunchGrenade 1' % ub,
                 'mpforce 1; statecall %s LaunchGrenade 2; mpforce 0' % ua,
                 'statecall %s LaunchGrenade 2' % ub, 'statecall %s LaunchGrenade 3' % ua]
        # the other pirates of the area: the layer-0 ones are in no squad (claim with no team)
        for p in pirates(c.cmd('objs spacepirate')):
            if p[4] not in (A_EID, B_EID):
                steps += ['statecall u%04x LaunchGrenade 0' % p[0], 'statecall u%04x LaunchGrenade 2' % p[0]]
        for st in steps:
            if st.startswith('wait '):
                time.sleep(float(st.split()[1]))
                continue
            outs = [c.cmd(st)] + [read_prompt(c) for _ in range(st.count(';'))]
            print('driver: %s -> %s' % (st, ' | '.join(o.splitlines()[-1] if o else '' for o in outs)),
                  flush=True)
    print('driver: done', flush=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
