#!/usr/bin/env python3
"""DigitalGuardian area 53, dg33: the head's fn_14_12818 with an inactive ball collider (a console DCTV
while the morph ball sits in slot 0) and XRDG fn_14_12D98 from beam shots at the root once the armour
is down (charged, normal, then zero damage under tools/cov/specs/digitalguardian_dg33_head.py).

usage: dg33_head.py <port>   (tools/cov/force.sh ... --driver 'python3 tools/drivers/dg33_head.py {port}')
Ids as in dg26_slots.py: the balls u0572/u0573, the root u0574, the echo targets u0575..u0577.
"""
import re
import socket
import sys
import time

port = int(sys.argv[1])
HEAD = 'e0035000f'
t0 = time.time()
s = None
while s is None and time.time() - t0 < 600:
    try:
        s = socket.create_connection(('127.0.0.1', port), timeout=5)
    except OSError:
        time.sleep(0.5)
if s is None:
    print('[dg33] no connect', flush=True)
    sys.exit(1)
f = s.makefile('rw')
s.settimeout(120)


def C(c):
    f.write(c + '\n')
    f.flush()
    lines, pending = [], c.count(';') + 1
    while True:
        l = f.readline()
        if not l:
            raise SystemExit('[dg33] eof')
        l = l.rstrip()
        if l.startswith('=> '):
            pending -= 1
            if pending == 0:
                break
        else:
            lines.append(l)
    print(f'> {c}\n   ' + '\n   '.join(lines), flush=True)
    return '\n'.join(lines)


def frame():
    f.write('frame\n')
    f.flush()
    out = -1
    while True:
        l = f.readline()
        if not l:
            raise SystemExit('[dg33] eof')
        if l.startswith('frame '):
            out = int(l.split()[1])
        if l.startswith('=> '):
            return out


def at(n, c=None):
    while frame() < n:
        time.sleep(0.02)
    return C(c) if c else None


def pos(uid):
    m = re.search(r'^\s*' + uid + r' \S+\s+\S+\s+a\d+ \(([-\d.]+), ([-\d.]+), ([-\d.]+)\)',
                  C('objs CCollisionActor'), re.M)
    return tuple(float(v) for v in m.groups()) if m else None


at(130, 'layer 10; god 1; fastmorph 1')
at(150, f'send e00350060 Activate; send {HEAD} Activate')
at(152, f'place {HEAD} player 20; fsm {HEAD} FaceTarget')
# XRDG with the armour still on (0x10F4 bit 0x10 clear, beq at 0x12DCC)
at(200, 'damage u0574 1 cannonball; damage u0574 1')
# the armour: the three echo targets killed (HeadArmorDestroyed sets 0x10F4 bit 0x10)
n = 400
for t in ('u0575', 'u0576', 'u0577'):
    at(n, f'codecall {HEAD} ExtractEchoTargets')
    at(n + 70, f'objs EchoTarget; damage {t} 1000')
    at(n + 80, f'trigcall {HEAD} AllEchoTargetsDestroyed; trigcall {HEAD} HeadArmorDestroyed')
    C(f'codecall {HEAD} RetractEchoTargets')
    n += 150
# fn_14_12818: the morph ball held in slot 0, then the slot's ball collider deactivated (the
# inactive-ball arm, beq at 0x1295C) and activated again
at(n, f'codecall {HEAD} EnableDynamicCollision; fsm {HEAD} Idle; var {HEAD} dghd.flags')
# a ball's XRDG (the sender is not the root, bne at 0x12DDC); the retreat platform (0x1038) set, so the
# hold excludes the player from it
C(f'damage u0573 1; codecall {HEAD} SelectRetreatPlatform; var {HEAD} dghd.flags')
C('press x 10')
for _ in range(6):
    p = pos('u0572')
    if p is None:
        break
    C(f'tp {p[0]:.2f} {p[1]:.2f} {p[2] - 0.7:.2f}')
    at(frame() + 4)
at(frame() + 30, 'send u0572 Deactivate; objs CCollisionActor')
at(frame() + 60, 'send u0572 Activate')
# a bomb destroys slot 0's ball while it is held (fn_14_12B04 sets its 0xF28 sound: the
# destroyed-slot arm, 0x128A4)
at(frame() + 30, 'damage u0572 1000 bomb; objs CCollisionActor')
at(frame() + 30, 'press x 10')
# XRDG fn_14_12D98: beam shots at the root (u0574) once it is vulnerable: charged (> 25, the
# flinch), normal, then the spec's zero damage
at(frame() + 30, f'codecall {HEAD} SetVulnerableToDamage; send u0574 Activate; objs CCollisionActor')
for opt in ('charged', '', '', '', ''):
    C(f'shoot u0574 {opt}'.strip())
    at(frame() + 40, f'var {HEAD} dghd.flags')
C(f'objs CCollisionActor; obj {HEAD}; status')
at(frame() + 5, 'covdump')
