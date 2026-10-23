#!/usr/bin/env python3
"""Ragdoll driver for the SpacePirate module (sp14): kill every pirate of the area the ordinary
way (`damage <id> 1000`, so the death animation's BecomeRagDoll event arms the Think tail), let
the ragdolls fall, shoot each corpse (the projectile's GetRagDoll), send each one Decrement
(AcceptScriptMsg's ragdoll arm; forced) and wait until the ragdolls are over and fade.

usage: spacepirate_ragdoll_drive.py <port> [layer|-] [settle seconds]
env:   SPR_DECREMENT=0  skip the Decrement messages
       SPR_ATTACH=1     the first pirate takes and lets go of the second one (`pirateattach`, the
                        module's attach/detach entries) before the kills and again on its ragdoll
       SPR_SHOTS=<n>    shots at each corpse (3)
       SPR_WAIT=<s>     seconds between the kills and the shots (6; longer under gdb, so the
                        ragdolls are there)
       SPR_WATER_OFF=1  deactivate the area's waters once the corpses float (Think's inactive-water
                        arm; the inactive water keeps its inhabitants)
"""
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import spacepirate_drive as sd  # noqa: E402  (Console, pirates)

port = int(sys.argv[1])
layer = sys.argv[2] if len(sys.argv) > 2 else '-'
settle = float(sys.argv[3]) if len(sys.argv) > 3 else 25.0


def main():
    c = sd.Console(port)
    if not c.connect(timeout=600):
        print('driver: no console')
        return 1
    if layer != '-':
        c.cmd('layer %s' % layer)
    ps = []
    for _ in range(300):
        ps = sd.pirates(c.cmd('objs spacepirate'))
        if ps:
            break
        time.sleep(1)
    time.sleep(3)
    print('driver: %d pirates: %s' % (len(ps), [(hex(p[0]), hex(p[4])) for p in ps]), flush=True)
    for p in ps:
        c.cmd('send u%04x Activate' % p[0])
    time.sleep(2)
    attach = os.environ.get('SPR_ATTACH', '0') != '0' and len(ps) > 1

    def attach_pair(when, pair):
        a, b = pair[0][0], pair[1][0]
        for arg in ('u%04x' % b, 'u%04x' % b, '-'):
            print('driver: attach %s u%04x %s %s' % (when, a, arg,
                  c.cmd('pirateattach u%04x %s' % (a, arg)).splitlines()[-1:]), flush=True)

    if attach:
        attach_pair('alive', ps)
    for p in ps:
        print('driver: kill u%04x %s' % (p[0], c.cmd('damage u%04x 1000' % p[0]).splitlines()[-1:]),
              flush=True)
        time.sleep(1)
    time.sleep(float(os.environ.get('SPR_WAIT', '6')))
    corpses = sd.pirates(c.cmd('objs spacepirate'))
    if attach and len(corpses) > 1:
        attach_pair('ragdoll', corpses)
    for p in sd.pirates(c.cmd('objs spacepirate')):
        for _ in range(int(os.environ.get('SPR_SHOTS', '3'))):
            print('driver: shoot u%04x %s' % (p[0], c.cmd('shoot u%04x' % p[0]).splitlines()[-1:]),
                  flush=True)
            time.sleep(0.5)
        if os.environ.get('SPR_DECREMENT', '1') != '0':
            print('driver: decrement u%04x %s' % (p[0], c.cmd('send u%04x Decrement' % p[0]).splitlines()[-1:]),
                  flush=True)
    if os.environ.get('SPR_WATER_OFF', '0') != '0':
        time.sleep(8)
        for uid in sorted(set(re.findall(r'\bu([0-9a-f]{4})\b', c.cmd('objs water')))):
            print('driver: water off u%s %s' % (uid, c.cmd('send u%s Deactivate' % uid).splitlines()[-1:]),
                  flush=True)
    time.sleep(settle)
    print('driver: left %s' % [hex(p[0]) for p in sd.pirates(c.cmd('objs spacepirate'))], flush=True)
    # a forcing run under gdb may not reach its frame limit: write the coverage counters now
    # (a no-op answer on a normal build).
    print('driver: covdump %s' % c.cmd('covdump').splitlines()[-1:], flush=True)
    print('driver: done', flush=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
