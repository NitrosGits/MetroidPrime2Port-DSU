#!/usr/bin/env python3
"""DigitalGuardian area 53, dg30: the beacon and the render bodies (fn_14_1994/18B4/17DC/1700/
2578/2A28/28D0/2CC4/A3D4) under the forcing specs tools/cov/specs/digitalguardian_dg30_*.py.

usage: dg30_beacon.py <port> a|b
- a: the beacon raised and left up (the models are unset by the spec), shin flag 0x10 set a while;
- b: three beacon arrivals: combat visor; echo visor (the spec clears the sound handle); and one
  with the collision actor unset by the spec. The echo visor stays on while the beacon is up.
"""
import re
import socket
import sys
import time

port, mode = int(sys.argv[1]), sys.argv[2]
BODY, HEAD = 'e00350060', 'e0035000f'
t0 = time.time()
s = None
while s is None and time.time() - t0 < 900:
    try:
        s = socket.create_connection(('127.0.0.1', port), timeout=5)
    except OSError:
        time.sleep(0.5)
if s is None:
    print('[dg30] no connect', flush=True)
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
            raise SystemExit('[dg30] eof')
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


def arrive(start):
    """Raise the beacon from the bottom and wait past its arrival (1.0 at 1/60 s a frame)."""
    at(start)
    var('dg.beacon', 2)
    at(start + 80)
    var('dg.beacon')
    var('dg.beaconh')


at(130, 'layer 10; god 1; give EchoVisor')
at(150, f'send {BODY} Activate; send {HEAD} Activate')
at(170)
flags = var('dg.flags')
var('dg.flags', flags | 0x10)
at(190)
var('dg.flags', flags)
# the beacon only updates in Crippled (fn_14_7CB4 runs fn_14_2CC4)
at(200, f'fsm {BODY} CrippledReaction')
at(250, f'fsm {BODY} Crippled')
if mode == 'a':
    arrive(260)
    at(450, f'obj {BODY}')
    var('dg.beacon', 3)
    at(500)
else:
    arrive(260)                    # arrival 1: combat visor, no handle yet
    at(350, 'visor echo')          # fn_14_2CC4's 0x7F volume with the handle set
    at(380)
    var('dg.beacon', 3)
    at(430)
    arrive(440)                    # arrival 2: echo visor, handle cleared by the spec
    var('dg.beacon', 3)
    at(560, 'visor combat')
    arrive(580)                    # arrival 3: collision actor unset by the spec
    at(700, f'obj {BODY}')
C('status')
