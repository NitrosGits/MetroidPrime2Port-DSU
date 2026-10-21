#!/usr/bin/env python3
"""Plays the Amorbis (CSandBoss, module 55) fight through the console, no probes.

usage: sandboss_fight.py <port> <frames> [outfile]

Boots in Agon Wastes 03_sand_temple_dark (42b935e4,7575048a): god mode, layer 5 (Amorbis Boss),
Activate to every CSandBoss (the intro cinema's job in retail), then reacts to each worm's FSM2
state as `obj` prints it: charged shots at a worm that is stampeding or attached; in SuckAir the
player is stood in front of the head collider (the worm's uid + 1, Head_1 is the first joint) and
morphs; in BallInMouth it lays bombs. Logs every state change with the frame and the worm's
position, a z track of the stampede leap and the boss bar's HP (`obj` of the MDFY/ACTV actor).
"""
import re
import socket
import sys
import time

port, frames = int(sys.argv[1]), int(sys.argv[2])
outfile = sys.argv[3] if len(sys.argv) > 3 else None
out = []


def emit(s):
    out.append(s)
    print(s, flush=True)
    if outfile:
        with open(outfile, 'a') as o:
            o.write(s + '\n')


t0 = time.time()
s = None
while s is None and time.time() - t0 < 600:
    try:
        s = socket.create_connection(('127.0.0.1', port), timeout=5)
    except OSError:
        time.sleep(0.5)
if s is None:
    emit('[fight] no connect')
    sys.exit(1)
f = s.makefile('rw')
s.settimeout(60)


def req(c):
    try:
        f.write(c + '\n')
        f.flush()
    except OSError:  # the game exited
        emit('[fight] console closed')
        sys.exit(0)
    lines = []
    pending = c.count(';') + 1
    while True:
        l = f.readline()
        if not l:
            return lines
        l = l.rstrip()
        if l.startswith('=> '):
            pending -= 1
            if pending == 0:
                return lines
            continue
        lines.append(l)


def frame():
    for l in req('frame'):
        m = re.search(r'frame (\d+)', l)
        if m:
            return int(m.group(1))
    return -1


OBJ = re.compile(r'(u[0-9a-f]{4}) [0-9a-f]{8}\s+(\S+)\s+a\d+ \(([-\d.]+), ([-\d.]+), ([-\d.]+)\)(.*)')


def objs(kind):
    res = {}
    for l in req('objs ' + kind):
        m = OBJ.search(l)
        if m:
            res[m.group(1)] = (tuple(float(m.group(i)) for i in (3, 4, 5)), 'inactive' not in m.group(6))
    return res


def fsm_hp(uid):
    st, hp = None, None
    for l in req('obj ' + (uid if uid.startswith('u') else '0x' + uid)):
        l = l.strip()
        if l.startswith('fsm '):
            st = l[4:]
        elif l.startswith('hp '):
            hp = l[3:]
    return st, hp


def player():
    for l in req('status'):
        m = re.search(r'position ([-\d.]+) ([-\d.]+) ([-\d.]+).*morph (\d+)', l)
        if m:
            return tuple(float(m.group(i)) for i in (1, 2, 3)), int(m.group(4))
    return None, None


def hexid(u, d):
    return 'u%04x' % (int(u[1:], 16) + d)


# Wait for gameplay.
while frame() < 250:
    time.sleep(0.2)
req('god 1')
while frame() < 300:
    time.sleep(0.1)
req('layer 5')
while frame() < 345:
    time.sleep(0.1)
worms = list(objs('sandboss'))
for w in worms:
    req('send %s activate' % w)
emit('[fight] worms %s' % ' '.join(worms))
# The boss bar's source: the worm's "on Modify send Activate -> <id> CScriptActor" link.
bar = None
for l in req('obj ' + worms[0]):
    m = re.search(r'on Modify send Activate -> ([0-9a-f]{8}) CScriptActor', l)
    if m:
        bar = m.group(1)
emit('[fight] boss bar actor %s' % bar)

state = {}
last_shot = 0
last_bomb = 0
last_hp = None
morph_req = 0
fr = frame()
while fr < frames and fr >= 0:
    fr = frame()
    pos = objs('sandboss')
    if not pos:
        emit('[fight f%d] no CSandBoss left' % fr)
        break
    ppos, morph = player()
    if bar:
        _, hp = fsm_hp(bar)
        if hp != last_hp:
            emit('[fight f%d] boss bar hp %s' % (fr, hp))
            last_hp = hp
    if fr // 200 != getattr(player, 'last', -1):
        player.last = fr // 200
        emit('[player f%d] %s morph %s' % (fr, ppos, morph))
    acting = None
    for w, (p, active) in sorted(pos.items()):
        if not active:
            continue
        st, _ = fsm_hp(w)
        if state.get(w) != st:
            emit('[fight f%d] %s %s -> %s at (%.1f, %.1f, %.1f)' % (fr, w, state.get(w), st, *p))
            state[w] = st
        if st == 'Stampede':
            emit('[leap f%d] %s z %.1f' % (fr, w, p[2]))
        if st in ('SuckAir', 'SuckBall', 'BallInMouth', 'SpitOutBall', 'DestroySphere'):
            acting = (w, st)
        elif acting is None and st in ('Stampede', 'SeekStampedePoint', 'RotateToPlayer',
                                       'AttachToSphere', 'SnapJaws', 'RepeaterAttack',
                                       'DarkBeamAttack', 'DoubleCharge', 'TripleCharge'):
            acting = (w, st)
    if acting is None:
        time.sleep(0.05)
        continue
    w, st = acting
    if st in ('SuckAir',):
        # IsBallInSuckRange (fn_55_5B38) wants the ball within 15 degrees of the worm's facing; the
        # head (uid + 1) lies along the facing from the root, so stand on that line past the head.
        head = objs('collision').get(hexid(w, 1))
        if head and ppos:
            hx, hy, hz = head[0]
            rx, ry, _ = pos[w][0]
            dx, dy = hx - rx, hy - ry
            d = max((dx * dx + dy * dy) ** 0.5, 1e-3)
            tp = (hx + dx / d * 4.0, hy + dy / d * 4.0, hz - 1.0)
            if morph == 0:
                req('tp %.2f %.2f %.2f' % tp)
                if fr - morph_req > 60:
                    emit('[fight f%d] SuckAir: player to (%.1f, %.1f, %.1f), head (%.1f, %.1f, %.1f), morph' %
                         (fr, *tp, hx, hy, hz))
                    req('press x 4')
                    morph_req = fr
            elif morph == 1 and fr - morph_req > 30:
                req('tp %.2f %.2f %.2f' % tp)
                morph_req = fr
    elif st in ('BallInMouth',):
        if fr - last_bomb > 40:
            req('press a 2')
            last_bomb = fr
    elif st in ('SpitOutBall', 'DestroySphere', 'SuckBall'):
        pass
    else:
        if morph == 1 and fr - morph_req > 60:
            req('press x 4')  # unmorph to shoot again
            morph_req = fr
        elif morph == 0 and fr - last_shot > 20:
            r = req('shoot %s charged 6' % w)
            emit('[shot f%d] %s %s: %s' % (fr, w, st, ' | '.join(x.strip() for x in r)))
            last_shot = fr
    time.sleep(0.02)
emit('[fight f%d] end; states %s' % (fr, state))
