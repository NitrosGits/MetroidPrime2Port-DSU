#!/usr/bin/env python3
"""DigitalGuardian area 53, dg26: the head's echo-target kills (fn_14_11FEC, F2B8), its bomb slots
(fn_14_12B04, 13248) and triggers, through the console.

usage: dg26_slots.py <port> [variant]   (run by tools/cov/run.sh --driver 'python3 ... {port} a')
variant a: the IS04 actor (0035005f) is deleted before the last echo kill;
variant b: it is deleted before the bomb slot (ball 0);
variant c: ball 1 and the root (the body deleted first).
Ids are those of a run whose layers load after the warp: the head's balls u0572/u0573, its root
u0574, the echo targets u0575..u0577 (XCRT builds the colliders before the targets).
"""
import re
import socket
import sys
import time

port = int(sys.argv[1])
variant = sys.argv[2] if len(sys.argv) > 2 else 'a'
HEAD = 'e0035000f'
t0 = time.time()
s = None
while s is None and time.time() - t0 < 600:
    try:
        s = socket.create_connection(('127.0.0.1', port), timeout=5)
    except OSError:
        time.sleep(0.5)
if s is None:
    print('[dg26] no connect', flush=True)
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
            raise SystemExit('[dg26] eof')
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
            raise SystemExit('[dg26] eof')
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


at(130, f'layer 10; god 1; fastmorph 1')
at(150, f'send e00350060 Activate; send {HEAD} Activate')
at(152, f'place {HEAD} player 20; fsm {HEAD} FaceTarget')
# the Use* triggers with each attack; the burst count 0, then 3 (the first shot is the pulse)
for a in (3, 2, 1):
    C(f'var {HEAD} dghd.attack {a}; trigcall {HEAD} UseMachineGun; trigcall {HEAD} UseLockOnMissiles;'
      f' trigcall {HEAD} UseAnnihilatorCharge; trigcall {HEAD} IsBurstOver')
C(f'codecall {HEAD} SelectProjectileAttack; var {HEAD} dghd.attack; trigcall {HEAD} IsBurstOver')
# before any kill: targets left, no bomb pit, the body there; no splitters
# AcceptScriptMsg (fn_14_17D38): ACTV while active (skipped), DCTV while active, DCTV while
# inactive (skipped), ACTV while inactive
C(f'send {HEAD} Activate; send {HEAD} Deactivate; send {HEAD} Deactivate; send {HEAD} Activate')
C(f'trigcall {HEAD} AllEchoTargetsDestroyed; trigcall {HEAD} BombPitDestroyed;'
  f' trigcall {HEAD} IsAntennaTransmitting; trigcall {HEAD} AllSplittersDead')
# two splitters (chassis 00350041/46, command modules 00350040/47); then without the command
# modules; then none (the chassis deleted). SelectDeathHeadAttack (fn_14_14470) with them alive,
# then a dozen picks with none
at(162, 'send e00350042 SetToZero; send e00350045 SetToZero')
at(300, f'objs CSplitter; trigcall {HEAD} AllSplittersDead; codecall {HEAD} SelectDeathHeadAttack; var {HEAD} dghd.attack')
C('send e00350040 Delete; send e00350047 Delete')
at(304, f'objs CSplitter; trigcall {HEAD} AllSplittersDead')
C('send e00350041 Delete; send e00350046 Delete')
at(308, f'objs CSplitter; trigcall {HEAD} AllSplittersDead')
for _ in range(12):
    C(f'codecall {HEAD} SelectDeathHeadAttack; var {HEAD} dghd.attack')
# the echo targets: retract before any extends, then while extending (the same tick), again
# while retracting
at(310, f'codecall {HEAD} RetractEchoTargets')
C(f'codecall {HEAD} ExtractAntenna; codecall {HEAD} ExtractEchoTargets; codecall {HEAD} RetractEchoTargets')
at(320, f'codecall {HEAD} ExtractEchoTargets')
at(330, f'codecall {HEAD} RetractEchoTargets')
at(340, f'codecall {HEAD} RetractEchoTargets')
n = 400
targets = ['u0575', 'u0576', 'u0577']
for i, t in enumerate(targets):
    at(n, f'codecall {HEAD} ExtractEchoTargets')
    if i == 2:
        C('send e00350319 Delete' + ('; send e0035005f Delete' if variant == 'a' else ''))
    at(n + 70, f'objs EchoTarget; damage {t} 1000')
    at(n + 80, f'objs EchoTarget; trigcall {HEAD} AllEchoTargetsDestroyed; trigcall {HEAD} HeadArmorDestroyed')
    C(f'codecall {HEAD} RetractEchoTargets')
    n += 150
# the bomb slots: the morph ball held in a slot (fn_14_12818 gives the ball 0xDD0), a bomb hit
# destroys it. The head has two lives (0x10E4): a/b destroy ball 0 and hit it again (its 0xF28
# sound already set), which ends the head; c destroys ball 1, then (the body deleted) the root,
# which is no ball (13248 answers -1).
at(n, f'codecall {HEAD} EnableDynamicCollision; fsm {HEAD} Idle; var {HEAD} dghd.flags')
C('press x 10')
ball = 'u0573' if variant == 'c' else 'u0572'
if variant == 'b':
    C('send e0035005f Delete')
for _ in range(6):
    p = pos(ball)
    if p is None:
        break
    C(f'tp {p[0]:.2f} {p[1]:.2f} {p[2] - 0.7:.2f}')
    at(frame() + 4)
C(f'damage {ball} 1000 bomb')
C(f'trigcall {HEAD} BombPitDestroyed; obj {HEAD}')
if variant != 'c':
    C(f'damage {ball} 1000 bomb')
else:
    C('press x 10')
    C('send e00350060 Delete')
    at(frame() + 2, f'trigcall {HEAD} IsAntennaTransmitting')
    C(f'codecall {HEAD} SetVulnerableToDamage; damage u0574 100000')
C(f'objs CCollisionActor; obj {HEAD}')
# XAUD (0x58415544), then XDEL with the manager built
C(f'send {HEAD} 0x58415544')
at(frame() + 2, f'send {HEAD} Delete')
C('status')
