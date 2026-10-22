#!/usr/bin/env python3
"""DigitalGuardian area 53, dg27: RetractEchoTargets (fn_14_14DA0) on fully extended echo targets.
The targets extend at 0.25/s (fn_14_19CE0), 4 s; the retract comes 600 frames after the extract
(dg26 retracted at most 80 frames after one, always while a target was still extending).

usage: dg27_retract.py <port>   (run by tools/cov/run.sh --driver 'python3 ... {port}')
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
    print('[dg27] no connect', flush=True)
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
            raise SystemExit('[dg27] eof')
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
            raise SystemExit('[dg27] eof')
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


at(130, 'layer 10; god 1')
at(150, f'send e00350060 Activate; send {HEAD} Activate')
at(152, f'place {HEAD} player 20; fsm {HEAD} FaceTarget')
at(200, f'codecall {HEAD} ExtractAntenna; codecall {HEAD} ExtractEchoTargets')
at(800, f'objs EchoTarget; codecall {HEAD} RetractEchoTargets')
at(820, f'objs EchoTarget')
C('status')
