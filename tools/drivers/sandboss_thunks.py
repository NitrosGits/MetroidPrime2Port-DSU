#!/usr/bin/env python3
"""Reaches the CSandBoss vtable thunks (module 55, docs/module-analysis/SandBoss.md section 7.3).

usage: sandboss_thunks.py <port> [outfile]

Boots in Agon Wastes 03_sand_temple_dark (42b935e4,7575048a): god mode, layer 5 (Amorbis Boss),
Activate to every CSandBoss, then per worm: a scan (CScanDisplay asks IsScanVisorSelfRender),
beam shots from close range (CGameProjectile::CanCollideWithGameObject asks GetRagDoll and
CanBeShot), a power bomb and damage (knock-back asks IsOnGround / GetGravityConstant), and `kill`.
Every reply is logged. Count the hits with the gcov build (build-port-cov, tools/cov/gcov_report.py
--module SandBoss): the host linker folds the identical thunks, so gdb breakpoints cannot.
"""
import re
import socket
import sys
import time

port = int(sys.argv[1])
outfile = sys.argv[2] if len(sys.argv) > 2 else None


def emit(s):
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
    emit('[thunks] no connect')
    sys.exit(1)
f = s.makefile('rw')
s.settimeout(120)


def req(c, quiet=False):
    try:
        f.write(c + '\n')
        f.flush()
    except OSError:
        emit('[thunks] console closed')
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
                break
            continue
        lines.append(l)
    if not quiet:
        emit('> %s\n  %s' % (c, '\n  '.join(lines[:6])))
    return lines


def frame():
    for l in req('frame', True):
        m = re.search(r'frame (\d+)', l)
        if m:
            return int(m.group(1))
    return -1


def wait(n):
    # n game updates, polled through `frame`
    end = frame() + n
    while frame() < end:
        time.sleep(0.05)


OBJ = re.compile(r'(u[0-9a-f]{4}) [0-9a-f]{8}\s+(\S+)\s+a\d+ \(([-\d.]+), ([-\d.]+), ([-\d.]+)\)(.*)')


def worms():
    return [m.group(1) for m in map(OBJ.match, req('objs sandboss')) if m]


# Wait for gameplay (the timing tools/drivers/sandboss_fight.py uses).
while frame() < 250:
    time.sleep(0.2)
req('god 1')
while frame() < 300:
    time.sleep(0.1)
req('layer 5')
while frame() < 345:
    time.sleep(0.1)
ws = worms()
for w in ws:
    req('send %s activate' % w)
emit('[thunks] worms %s' % ' '.join(ws))
wait(240)
for w in ws:
    # `scan` holds L for a while per command: repeat it until the scan completes
    for _ in range(12):
        r = ' '.join(req('scan %s 30 240' % w))
        if 'repeat' not in r:
            wait(260)  # the hold the command started
            break
        wait(30)
    req('scan')
    req('visor combat')
    wait(30)
    for _ in range(6):
        req('shoot %s 4' % w)
        wait(20)
    req('shoot %s charged 4' % w)
    wait(60)
    req('damage %s 5 10' % w)
    req('damage %s 5 10 powerbomb' % w)
    wait(60)
for w in ws:
    req('kill %s' % w)
    wait(240)
req('status')
emit('[thunks] done')
