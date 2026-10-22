#!/usr/bin/env python3
"""DigitalGuardian area 53, dg29: the body's (e00350060) unhit triggers, codes and helpers through
the console: `trigcall`/`codecall`/`fsm` and the body's `var dg.*` fields
(CDigitalGuardian::PortConsoleVar).

usage: dg29_body.py <port>   (run by tools/cov/run.sh --driver 'python3 ... {port}')
- triggers IsIdle, VortexStruckPlayer, BreakOutOfVortex, AllKneesDestroyed, HasFaceTarget,
  ShouldJumpAttack with each value of the field they test; the head's IsAntennaTransmitting
  (fn_14_70B4) with each beacon state and the 0x08 flag;
- codes SelectExitVortexTarget, DisableHeadAttacks (head there / deleted);
- VortexAttack twice (`fsm`; the sub-machine runs on to ExitVortexAttack): with the camera
  shaker, then with dg.shaker and dg.toe0 pointing nowhere (fn_14_4D54/4878 no shaker, fn_14_39CC
  missing collider);
- the toe colliders hurt outside the vortex (fn_14_5384 -> fn_14_5140): hit, flinch, destroyed,
  already destroyed;
- CrippledReaction (head there, then deleted with the DeathRattle waypoint) and Crippled with the
  beacon raised (OPEN), in the echo visor and not, then falling (dg.beacon 3) and halted
  (dg.flags 0x08);
- the second vortex also runs without its IS05/Follow object (dg.follow), its exit waypoint
  (dg.exit, fn_14_34E8), two knee armour ids (dg.armor0 invalid, dg.armor1 missing, fn_14_3DA8)
  and the IS03 camera shakers (deleted; fn_14_4E00);
- ShouldJumpAttack with no active IS02 waypoint, with the body next to the closest one, and with
  one deleted (fn_14_308C).
"""
import re
import socket
import sys
import time

port = int(sys.argv[1])
BODY, HEAD = 'e00350060', 'e0035000f'
t0 = time.time()
s = None
while s is None and time.time() - t0 < 600:
    try:
        s = socket.create_connection(('127.0.0.1', port), timeout=5)
    except OSError:
        time.sleep(0.5)
if s is None:
    print('[dg29] no connect', flush=True)
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
            raise SystemExit('[dg29] eof')
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
            raise SystemExit('[dg29] eof')
        if l.startswith('frame '):
            out = int(l.split()[1])
        if l.startswith('=> '):
            return out


def at(n, c=None):
    while frame() < n:
        time.sleep(0.02)
    return C(c) if c else None


def var(name, value=None):
    out = C(f'var {BODY} {name}' + ('' if value is None else f' {value}'))
    m = re.search(r'=\s*(-?\d+)', out)
    return int(m.group(1)) if m else None


def objpos(cls, editor):
    m = re.search(r'^\s*u[0-9a-f]+ ' + editor + r'\s+\S+\s+a\d+ \(([-\d.]+), ([-\d.]+), ([-\d.]+)\)',
                  C(f'objs {cls}'), re.M)
    return tuple(float(v) for v in m.groups()) if m else None


def state():
    m = re.search(r'fsm (\w+)', C(f'obj {BODY}'))
    return m.group(1) if m else ''


def T(trig, who=BODY):
    return C(f'trigcall {who} {trig}')


at(130, 'layer 10; god 1; give EchoVisor')
at(150, f'send {BODY} Activate; send {HEAD} Activate')
at(160)
flags = var('dg.flags')
# fn_14_9A9C IsIdle (bit 0x80), fn_14_966C VortexStruckPlayer (bit 0x04)
for v in (flags | 0x80, flags & ~0x80 & 0xff, flags | 0x04, flags & ~0x04 & 0xff):
    var('dg.flags', v)
    T('IsIdle')
    T('VortexStruckPlayer')
var('dg.flags', flags)
# fn_14_9638 BreakOutOfVortex: not in the vortex; in it with health left; with none
for mode, hp in ((0, 5), (1, 5), (1, 0)):
    var('dg.vortex', mode)
    var('dg.vortexhp', hp)
    T('BreakOutOfVortex')
var('dg.vortex', 0)
# fn_14_9A0C AllKneesDestroyed: -1, then a knee index (fn_14_705C with the knees there)
knee = var('dg.knee')
for k in (-1, 0):
    var('dg.knee', k)
    T('AllKneesDestroyed')
var('dg.knee', knee)
# fn_14_9A60 HasFaceTarget: x, then y, then z, then none
for xyz in ((1, 0, 0), (0, 1, 0), (0, 0, 1), (0, 0, 0)):
    var('dg.facex', xyz[0])
    var('dg.facey', xyz[1])
    var('dg.facez', xyz[2])
    T('HasFaceTarget')
# fn_14_9678 ShouldJumpAttack: the timer running, then elapsed with the player near / far
var('dg.jumptime', 5)
T('ShouldJumpAttack')
var('dg.jumptime', 0)
T('ShouldJumpAttack')
C(f'place {BODY} player 4')
T('ShouldJumpAttack')
# no active IS02/Follow waypoint (fn_14_308C answers none); then the player and the body at one
WAYPOINTS = ('00350017', '0035006a', '0035001e', '00350063')
C('; '.join(f'send e{w} Deactivate' for w in WAYPOINTS))
T('ShouldJumpAttack')
C('; '.join(f'send e{w} Activate' for w in WAYPOINTS))
body0 = objpos('CDigitalGuardian', '00350060')
wp = objpos('CScriptWaypoint', WAYPOINTS[0])
if wp:
    C(f'tp {wp[0]:.2f} {wp[1]:.2f} {wp[2]:.2f}; place {BODY} {wp[0] + 3:.2f} {wp[1]:.2f} {wp[2]:.2f}')
    T('ShouldJumpAttack')
if body0:
    C(f'place {BODY} {body0[0]:.2f} {body0[1]:.2f} {body0[2]:.2f}')
# fn_14_7600 SelectExitVortexTarget, fn_14_768C DisableHeadAttacks (head there)
C(f'codecall {BODY} SelectExitVortexTarget; codecall {BODY} DisableHeadAttacks')
# fn_14_70B4 through the head's IsAntennaTransmitting: beacon 0, 1, 2, 3, and the 0x08 flag
beacon = var('dg.beacon')
for b in (0, 1, 2, 3):
    var('dg.beacon', b)
    T('IsAntennaTransmitting', HEAD)
var('dg.beacon', 2)
var('dg.flags', flags | 0x08)
T('IsAntennaTransmitting', HEAD)
var('dg.flags', flags)
var('dg.beacon', beacon)

# VortexAttack with the shaker (fn_14_4D54/4878/39CC/3674/3300, then 34E8 in VortexRetreat,
# 3DA8 in ExitVortexAttack)
at(200, f'fsm {BODY} VortexAttack')
toe = [var(f'dg.toe{i}') for i in range(4)]
root = var('dg.root')
# the toe colliders hurt with the vortex's vulnerability (fn_14_39CC) but 0x11B8 not 1, so
# fn_14_5384 hands the damage to fn_14_5140: state 0 hit, state 1 flinch, destroyed, already
# destroyed; the root collider (no toe)
at(230)
var('dg.vortex', 0)
var('dg.toestate1', 0)
C(f'hurt u{toe[1]:04x} 1')
var('dg.toestate1', 1)
C(f'hurt u{toe[1]:04x} 1')
for _ in range(4):
    C(f'hurt u{toe[1]:04x} 100000')
    var('dg.toestate1')
C(f'hurt u{root:04x} 1')
var('dg.vortex', 1)
for n in range(300, 1000, 100):
    at(n, f'obj {BODY}')
# again with no shaker, toe collider 0 missing, no IS05 object, two knee armours gone, and the IS03
# camera shakers deleted
shaker = var('dg.shaker')
follow = var('dg.follow')
armor = [var(f'dg.armor{i}') for i in range(2)]
C('send e0035000e Delete; send e0035005b Delete')
var('dg.shaker', 65535)
var('dg.toe0', 65535)
var('dg.follow', 65535)
var('dg.armor0', 65535)
var('dg.armor1', 32766)
C(f'fsm {BODY} VortexAttack')
seen = False
while frame() < 1700:
    st = state()
    if st == 'VortexRetreat' and not seen:
        seen = True
        var('dg.exit', 65535)
        at(frame() + 4)
        C(f'codecall {BODY} SelectExitVortexTarget')
    at(frame() + 30)
var('dg.shaker', shaker)
var('dg.toe0', toe[0])
var('dg.follow', follow)
var('dg.armor0', armor[0])
var('dg.armor1', armor[1])
C(f'fsm {BODY} Idle')

# CrippledReaction with the head (fn_14_7D8C), then Crippled with the beacon rising (OPEN)
at(1800, f'fsm {BODY} CrippledReaction')
at(1860, f'fsm {BODY} Crippled')
at(1862, f'send {BODY} 0x4F50454E')
for n in (1900, 1925):
    at(n)
    var('dg.beacon')
    var('dg.beaconh')
at(1930, 'visor echo')
at(1970, 'visor combat')
at(2000, f'obj {BODY}')
var('dg.beacon', 3)
for n in (2005, 2020, 2060):
    at(n)
    var('dg.beacon')
    var('dg.beaconh')
at(2100)
var('dg.flags', var('dg.flags') | 0x08)
at(2110)
var('dg.flags', var('dg.flags') & ~0x08 & 0xff)
# the head deleted: DisableHeadAttacks, CrippledReaction and the beacon's aim without it
at(2150, f'send {HEAD} Delete; send e00350062 Delete')
at(2160, f'codecall {BODY} DisableHeadAttacks; fsm {BODY} CrippledReaction')
at(2220, f'fsm {BODY} Crippled')
var('dg.beacon', 2)
at(2400, f'obj {BODY}')
# an IS02 waypoint deleted: fn_14_308C's lookup finds nothing for it
var('dg.jumptime', 0)
C(f'send e{WAYPOINTS[0]} Delete')
at(frame() + 2)
T('ShouldJumpAttack')
C('status')
