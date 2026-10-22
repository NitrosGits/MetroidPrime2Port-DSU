#!/usr/bin/env python3
"""DigitalGuardian area 53, dg32: the head's overrides (fn_14_17840 ProjectileInfo, 17A64/17B28
render, 174F4/17638 scan display, 17890 aim), the body's fn_14_9DBC scan token, and both dtors
(fn_14_17F64, fn_14_AB98), under any spec (gcov is what counts).

usage: dg32_head.py <port>   (tools/cov/force.sh ... --driver 'python3 tools/drivers/dg32_head.py {port}')
"""
import re
import socket
import sys
import time

port = int(sys.argv[1])
BODY, HEAD = 'e00350060', 'e0035000f'
t0 = time.time()
s = None
while s is None and time.time() - t0 < 900:
    try:
        s = socket.create_connection(('127.0.0.1', port), timeout=5)
    except OSError:
        time.sleep(0.5)
if s is None:
    print('[dg32] no connect', flush=True)
    sys.exit(1)
f = s.makefile('rw')
s.settimeout(300)


def C(c):
    f.write(c + '\n')
    f.flush()
    lines, pending = [], c.count(';') + 1
    while True:
        l = f.readline()
        if not l:
            raise SystemExit('[dg32] eof')
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
    m = re.search(r'frame (\d+)', C('frame'))
    return int(m.group(1)) if m else -1


def at(n, c=None):
    while frame() < n:
        time.sleep(0.05)
    return C(c) if c else None


def var(name, value=None):
    out = C(f'var {BODY} {name}' + ('' if value is None else f' {value}'))
    m = re.search(r'=\s*(-?\d+)', out)
    return int(m.group(1)) if m else None


def head_var(name, value=None):
    out = C(f'var {HEAD} {name}' + ('' if value is None else f' {value}'))
    m = re.search(r'=\s*(-?\d+)', out)
    return int(m.group(1)) if m else None


at(30, 'layer 3; god 1')
at(60, 'layer 11')
at(140, f'send {BODY} Activate; send {HEAD} Activate')
at(200)
# fn_14_17840: every projectile attack value, the out-of-range ones included
attack = head_var('dghd.attack')
for a in (-1, 0, 1, 2, 3, 4):
    head_var('dghd.attack', a)
    print(f'[dg32] attack {a} -> projinfo {head_var("dghd.projinfo")}', flush=True)
head_var('dghd.attack', attack)
# fn_14_9DBC: the body's scan info with 0x128C bit 0x40 clear and set
bflags = var('dg.flags')
C(f'scaninfo {BODY}')
var('dg.flags', bflags | 0x40)
C(f'scaninfo {BODY}')
var('dg.flags', bflags & ~0x40)
C(f'scaninfo {BODY}')
var('dg.flags', bflags)
# the scan display: fn_14_9ABC/9CA4 call the head's fn_14_174F4/17638 while the body holds the
# head's id (0x1082); the fight may have detached it (fn_14_70E4), so "XALD" relinks it first
# (fn_14_13830 -> fn_14_7168)
at(290, f'send {HEAD} 0x58414C44')
at(300, f'scan {BODY}')
# the scan display draws the body (fn_14_9ABC) and so the head once the scan has downloaded
# (about 3 s of the 300 held updates); the render spec sets the head's flag 0x10 there
at(360, f'scan {BODY}')
at(420, 'scan')
# the dtors: the head (fn_14_17F64), then the body (fn_14_AB98)
at(600, f'send {HEAD} Delete')
at(640, f'objs DigitalGuardian; send {BODY} Delete')
at(680, 'objs DigitalGuardian; status')
at(690, 'covdump')
