#!/usr/bin/env python3
"""DigitalGuardian area 53, dg30 run C: the knee armour under
tools/cov/specs/digitalguardian_dg30_knees.py.

usage: dg30_knees.py <port>
Hits on armoured knees (the flash runs out), knee 0 unarmoured (InternalMessage1) and hit, re-armoured
(InternalMessage2); the knees made unorbitable and orbitable (SetVortexAttackMode, ExitVortexAttack's
Deactivate), then the same with knee 0's id unset.
Knee 2 is unarmoured, re-armed, unarmoured and deleted before the vortex calls (they set 0x128C's
0x20, after which no knee opens).
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


at(130, 'layer 10; god 1; give EchoVisor')
at(150, f'send {BODY} Activate; send {HEAD} Activate')
at(170)
knees = [var(f'dg.armor{i}') for i in range(4)]
K = ['u%04x' % (k & 0xffff) for k in knees]
at(180, f'send {K[0]} Damage; send {K[2]} Damage')     # armoured hits; knee 2 has no emitter
at(300, f'send {BODY} InternalMessage1; send {BODY} InternalMessage3')   # knees 0 and 2 unarmoured
at(320, f'send {K[0]} Damage; send {K[1]} Damage')
at(440, f'obj {K[0]}')
at(450, f'send {BODY} InternalMessage2; send {BODY} InternalMessage4')   # both armoured again
at(456, f'send {BODY} InternalMessage3')      # knee 2 unarmoured again (its handle set, the spec)
at(462, f'send {K[2]} Delete')                # and deleted with the handle set
at(470, f'codecall {BODY} SetVortexAttackMode')   # SetOrbitable false
at(490, f'fsm {BODY} ExitVortexAttack')
at(510, f'fsm {BODY} Idle')                   # ExitVortexAttack's Deactivate: SetOrbitable true
at(530)
var('dg.armor0', 65535)
at(540, f'codecall {BODY} SetVortexAttackMode')
at(550, f'send {BODY} InternalMessage1; send {BODY} InternalMessage3')
at(570, f'fsm {BODY} ExitVortexAttack')
at(590, f'fsm {BODY} Idle')
at(600)
var('dg.armor0', knees[0])
at(640)
C('status')
