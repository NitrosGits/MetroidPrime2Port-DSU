#!/usr/bin/env python3
"""Space-pirate ragdoll sanity poll (prag2): kill every CSpacePirate with `damage`, then read
`objs spacepirate` once a second and flag NaN/inf or |coordinate| > LIMIT. The ragdoll drives the
corpse's transform (CSpacePirate::Think 0x1DD8), so the corpse position is the ragdoll's.

usage: pirateragdoll_sp_poll.py <port> [layer|-] [seconds]    env PRP_LIMIT (default 5000)
"""
import math
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import spacepirate_drive as sd  # noqa: E402

port = int(sys.argv[1])
layer = sys.argv[2] if len(sys.argv) > 2 else '-'
secs = int(sys.argv[3]) if len(sys.argv) > 3 else 40
limit = float(os.environ.get('PRP_LIMIT', '5000'))
OBJ = re.compile(r'u([0-9a-f]+) ([0-9a-f]{8})\s+CSpacePirate\s+a(\d+) \(([^,]+), ([^,]+), ([^)]+)\)')

c = sd.Console(port)
if not c.connect(timeout=600):
    print('driver: no console')
    sys.exit(1)
if layer != '-':
    c.cmd('layer %s' % layer)


def read():
    return [(m.group(1), [float(m.group(i)) for i in (4, 5, 6)]) for m in OBJ.finditer(c.cmd('objs spacepirate'))]


ps = []
for _ in range(300):
    ps = read()
    if ps:
        break
    time.sleep(1)
time.sleep(3)
print('driver: %d pirates %s' % (len(ps), [(u, [round(v) for v in p]) for u, p in ps]), flush=True)
for u, _ in ps:
    c.cmd('send u%s Activate' % u)
time.sleep(2)
bad = 0
for u, _ in ps:
    print('driver: kill u%s %s' % (u, c.cmd('damage u%s 1000' % u).splitlines()[-1:]), flush=True)
    time.sleep(1)
last = {}
for t in range(secs):
    for u, p in read():
        ok = all(math.isfinite(v) and abs(v) < limit for v in p)
        if not ok:
            bad += 1
        if t % 3 == 0 or not ok:
            print('poll t=%d u%s (%.2f, %.2f, %.2f)%s' % (t, u, p[0], p[1], p[2], '' if ok else '  BAD'), flush=True)
        last[u] = p
    time.sleep(1)
print('driver: bad=%d final=%s' % (bad, {u: [round(v, 2) for v in p] for u, p in last.items()}), flush=True)
