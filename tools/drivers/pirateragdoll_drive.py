#!/usr/bin/env python3
"""Ragdoll driver for the PirateRagDoll module (prd1): kill every actor of one class in the area
(`damage <id> 1000`), let the ragdolls fall and settle, shoot each corpse (the owner's torso
impulse on a settled ragdoll), then switch the area's AI waypoints off and on again (the ragdoll's
pinned-particle arms: CPirateRagDoll::Update re-reads each pin's active flag and position).

usage: pirateragdoll_drive.py <port> <objs filter> [layer|-] [settle seconds]
env:   PRD_WAIT=<s>    seconds between the kills and the shots (12)
       PRD_SHOTS=<n>   shots at each corpse (3)
       PRD_PINS=0      leave the AI waypoints alone
       PRD_ATTACH=1    (SpacePirate) the first pirate takes and lets go of the second one
                       (`pirateattach`) on its ragdoll, so the ragdoll's attached-actor arms run
"""
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
_argv, sys.argv = sys.argv, sys.argv[:1]  # spacepirate_drive parses argv at import
import spacepirate_drive as sd  # noqa: E402  (Console)
sys.argv = _argv

port = int(sys.argv[1])
what = sys.argv[2]
layer = sys.argv[3] if len(sys.argv) > 3 else '-'
settle = float(sys.argv[4]) if len(sys.argv) > 4 else 20.0
OBJ = re.compile(r'u([0-9a-f]+) ([0-9a-f]{8})\s+(\S+)\s+a(\d+) ')


def objs(c, flt):
    return [(int(m.group(1), 16), m.group(2), m.group(3)) for m in OBJ.finditer(c.cmd('objs %s' % flt))]


def say(*a):
    print('driver:', *a, flush=True)


def main():
    c = sd.Console(port)
    if not c.connect(timeout=600):
        say('no console')
        return 1
    if layer != '-':
        for ly in layer.split(','):
            c.cmd('layer %s' % ly)
    actors = []
    for _ in range(120):
        actors = objs(c, what)
        if actors:
            break
        time.sleep(1)
    time.sleep(3)
    actors = objs(c, what)
    say('%d actors: %s' % (len(actors), actors))
    for a in actors:
        c.cmd('send u%04x Activate' % a[0])
    time.sleep(2)
    for a in actors:
        say('kill u%04x %s' % (a[0], c.cmd('damage u%04x 1000' % a[0]).splitlines()[-1:]))
        time.sleep(1)
    time.sleep(float(os.environ.get('PRD_WAIT', '12')))
    corpses = objs(c, what)
    if os.environ.get('PRD_ATTACH', '0') != '0' and len(corpses) > 1:
        a, b = corpses[0][0], corpses[1][0]
        for arg in ('u%04x' % b, '-'):
            say('attach u%04x %s %s' % (a, arg, c.cmd('pirateattach u%04x %s' % (a, arg)).splitlines()[-1:]))
            time.sleep(4)
    for a in corpses:
        for _ in range(int(os.environ.get('PRD_SHOTS', '3'))):
            say('shoot u%04x %s' % (a[0], c.cmd('shoot u%04x' % a[0]).splitlines()[-1:]))
            time.sleep(0.5)
    if os.environ.get('PRD_PINS', '1') != '0':
        time.sleep(6)
        wps = objs(c, 'aiwaypoint')
        say('%d waypoints' % len(wps))
        for w in wps[::2]:
            c.cmd('send u%04x Deactivate' % w[0])
        time.sleep(6)
        for w in wps[::2]:
            c.cmd('send u%04x Activate' % w[0])
        time.sleep(6)
        for w in wps[1::2]:
            c.cmd('send u%04x Delete' % w[0])
        say('waypoints toggled/deleted')
    time.sleep(settle)
    say('left %s' % objs(c, what))
    return 0


if __name__ == '__main__':
    sys.exit(main())
