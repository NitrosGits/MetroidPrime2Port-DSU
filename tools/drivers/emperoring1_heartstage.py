#!/usr/bin/env python3
"""Emperor Ing stage 1 (module 16): drive the boss into the heart stage.

The boss's state machine is an FSM2 one, and its "ChooseRandomTentacleCount"
code node hangs off the "HeartStageOver" trigger of the "HeartStage" state, so
the code only runs when the fight reaches the heart stage.  This driver

  * finds the boss (`objs emperoring`) and activates it (`send <uid> Activate`),
  * finds the heart's collision actor (the one at the boss's own position - the
    damage on it is the boss's health, which steps the mode 0 -> 1 -> 2),
  * forces the heart stage (`fsm <uid> HeartStage`) and waits for the
    heart-stage timer to run out, which runs the code node,
  * optionally damages the heart between the cycles, so the code node runs with
    the other modes.

usage: emperoring1_heartstage.py <port> [outfile] [damage ...]

  <damage ...>  the amounts to damage the heart with, one per cycle (the heart
                starts at the boss's 700 health; the mode steps down at the
                heart-stage thresholds, so a small amount gives mode 1 and a
                large one mode 2).
"""
import re
import socket
import sys
import time

port = int(sys.argv[1])
outfile = sys.argv[2] if len(sys.argv) > 2 else None
damages = [float(a) for a in sys.argv[3:] if a != 'natural']
natural = 'natural' in sys.argv[3:]

WAIT_FRAMES = 1200  # the heart-stage timer is 16 s of game time (960 updates)
out_lines = []


def emit(s):
    out_lines.append(s)
    print(s, flush=True)


s = None
t0 = time.time()
while s is None and time.time() - t0 < 120:
    try:
        s = socket.create_connection(('127.0.0.1', port), timeout=5)
    except OSError:
        time.sleep(0.5)
if s is None:
    emit('[con] no connect')
    sys.exit(1)
emit(f'[con] connected at {time.time()-t0:.1f}s')
f = s.makefile('rw')


def request(cmd, timeout=60):
    """Send one command; return its reply lines (without the final '=> ...') and that line."""
    f.write(cmd + '\n')
    f.flush()
    lines = []
    s.settimeout(timeout)
    while True:
        l = f.readline()
        if not l:
            return lines, '[con] eof'
        l = l.rstrip()
        if l.startswith('=> '):
            return lines, l
        lines.append(l)


def frame():
    lines, _ = request('frame', timeout=20)
    for l in lines:
        if l.startswith('frame '):
            return int(l.split()[1])
    return -1


def wait_frames(target, timeout=300):
    end = time.time() + timeout
    while time.time() < end:
        fr = frame()
        if fr >= target:
            return fr
        time.sleep(0.05)
    return frame()


def run(cmd):
    lines, end = request(cmd)
    emit(f'[con f{frame()}] > {cmd}')
    for l in lines:
        emit('   ' + l)
    emit('   ' + end)
    return lines


# the boss
uid = None
bpos = None
for _ in range(240):
    lines, _ = request('objs emperoring')
    for l in lines:
        m = re.match(r'\s*(u[0-9a-f]{4})\s+\S*\s+CEmperorIngStage1\s+a\d+\s+'
                     r'\(([-\d.]+), ([-\d.]+), ([-\d.]+)\)', l)
        if m:
            uid = m.group(1)
            bpos = (float(m.group(2)), float(m.group(3)), float(m.group(4)))
    if uid:
        break
    time.sleep(0.5)
if uid is None:
    emit('[con] no boss')
    sys.exit(1)
emit(f'[con] boss {uid} at {bpos}')

run(f'send {uid} Activate')
run('objs')

# The connected 'PLAY' actor (the CScriptActor the boss plays its body animation on) has to be
# active for its animation to run: the retail's script activates it when the fight starts, and
# the console cannot run that script, so the driver sends it the message itself.  The editor id
# 000b0016 is the CONNECT/'PLAY' actor (a probe of fn_16_78AC named it: uid u04e8, the
# CScriptActor at the body's position).
for l in [x.strip() for x in out_lines]:
    m = re.match(r'(u[0-9a-f]{4}) 000b0016\s+CScriptActor\b', l)
    if m:
        emit(f'[con] the PLAY actor {m.group(1)} gets Activate')
        run(f'send {m.group(1)} Activate')
        break
wait_frames(frame() + 120)

def dist(a):
    return sum((a[i + 1] - bpos[i]) ** 2 for i in range(3)) ** 0.5


# the collision actors: the heart is the one at the boss's own position
actors = []
for _ in range(60):
    lines, _ = request('objs collisionactor')
    actors = []
    for l in lines:
        m = re.match(r'\s*(u[0-9a-f]{4})\s+\S*\s+CCollisionActor\s+a\d+\s+'
                     r'\(([-\d.]+), ([-\d.]+), ([-\d.]+)\)', l)
        if m:
            actors.append((m.group(1), float(m.group(2)), float(m.group(3)), float(m.group(4))))
    if actors:
        break
    time.sleep(0.5)
emit(f'[con] {len(actors)} collision actors')

# the collision actors that hold a health: the tentacle's own box (the module's health info,
# fn_16_1E6C) and the heart (the boss's). The segment boxes have none.
alive = []
for (cu, x, y, z) in actors:
    lines, _ = request(f'obj {cu}')
    hp = None
    for l in lines:
        m = re.match(r'\s*hp ([-\d.]+)/([-\d.]+)', l)
        if m:
            hp = (float(m.group(1)), float(m.group(2)))
    if hp is not None and hp[1] > 0.0:
        alive.append((cu, hp, dist((cu, x, y, z))))
        emit(f'[con] {cu} at ({x},{y},{z}) hp {hp[0]}/{hp[1]}')


actors.sort(key=lambda a: dist(a))
heart = actors[0][0]
emit(f'[con] heart candidate {heart} at {actors[0][1:]} (distance {dist(actors[0]):.2f})')

# the heart's health, before and after a small hit: it is the boss's health
run(f'obj {heart}')
run(f'obj {uid}')
run(f'damage {heart} 1')
run(f'obj {uid}')

# the tentacles: the module's health is on the tentacle's own collision actor (the 5th of each
# group of five, fn_16_3AA8), so a hit on it kills the tentacle - but only while the tentacles
# carry a vulnerability the weapon hits (fn_16_353C: the Attack state gives them the melee one).
# the tentacle's own box: the module's tentacle health (40) is on it, the segment boxes have
# none. The tentacles are out a few frames after the activate, and they carry the normal
# vulnerability until the FSM reaches the Attack state (the melee one after that).
boxes = [cu for (cu, hp, d) in alive if hp[1] == 40.0]
emit(f'[con] {len(boxes)} tentacle boxes with a health')
if boxes:
    wait_frames(frame() + 150)
    emit('[con] the tentacles are out: a power-beam hit on every tentacle box')
    for cu in boxes:
        run(f'damage {cu} 9999')
        wait_frames(frame() + 5)
    for cu in boxes:
        run(f'obj {cu}')

# the cycles: force the heart stage, wait for the timer, damage the heart, repeat
for i, amount in enumerate([0.0] + damages):
    if i:
        run(f'damage {heart} {amount}')
        wait_frames(frame() + 60)
    if natural:
        emit('[con] natural: the FSM walks to the heart stage on its own')
        for _ in range(40):
            wait_frames(frame() + 120)
            lines = run(f'obj {uid}')
            for l in lines:
                if l.strip().startswith('fsm '):
                    emit('[con] state: ' + l.strip())
    else:
        run(f'fsm {uid} HeartStage')
        wait_frames(frame() + WAIT_FRAMES)
    run(f'obj {uid}')

emit('[con] done')
if outfile:
    open(outfile, 'w').write('\n'.join(out_lines) + '\n')
