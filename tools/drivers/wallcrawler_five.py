#!/usr/bin/env python3
"""Force the kART_Five bend: put a crawler on a wall with the console's `place`.

usage: wallcrawler_five.py <port> [area slot] [samples]

The driver warps into the area, lists the crawlers with `objs` (the console prints their positions),
picks the crawler whose position is the highest above the floor - a wall crawler - moves a second
crawler onto that spot with `place <id> <x> <y> <z>` and then keeps listing the crawlers, so the
run's log shows the re-orient (the 0x04 flag) and the kART_Five bend.

The bend itself is only visible in the run's log (the temporary probe in PortCWallCrawler.cpp prints
each delivery), so this driver is the setup, not the proof.
"""
import re
import socket
import sys
import time

port = int(sys.argv[1])
area = sys.argv[2] if len(sys.argv) > 2 else '40'
rounds = int(sys.argv[3]) if len(sys.argv) > 3 else 24

s = socket.create_connection(('127.0.0.1', port), timeout=5)
s.settimeout(60)
f = s.makefile('rw')


def request(c):
    f.write(c + '\n')
    f.flush()
    lines = []
    while True:
        l = f.readline()
        if not l:
            return lines, '[wf] eof'
        l = l.rstrip()
        if l.startswith('=> '):
            return lines, l
        lines.append(l)


def frame():
    lines, _ = request('frame')
    for l in lines:
        if l.startswith('frame '):
            return int(l.split()[1])
    return -1


def crawlers():
    lines, _ = request('objs pillbug')
    out = []
    for l in lines:
        m = re.match(r'\s*(u[0-9a-f]{4})\s+\S+\s+(\S+)\s+a(\d+)\s+\(([-\d.]+), ([-\d.]+), ([-\d.]+)\)', l)
        if m:
            out.append((m.group(1), float(m.group(4)), float(m.group(5)), float(m.group(6))))
    return out


while frame() < 2000:
    time.sleep(0.2)
print('warp', request(f'warp {area}')[1])
time.sleep(3)
best = None
# the boost-ball arm first: the player morphs, boosts, and a crawler is put on it
print('morph', request('press x 30')[1])
time.sleep(1.0)
print('boost', request('boost')[1])
time.sleep(0.5)
cs = crawlers()
if cs:
    print('place on the player', request(f'place {cs[0][0]} player 0')[1])
for i in range(8):
    time.sleep(0.4)
    print('  after boost', crawlers())
# then the kART_Five arm: a crawler is put on the wall spot of the highest crawler
for i in range(rounds):
    cs = crawlers()
    if cs:
        hi = max(cs, key=lambda c: c[3])
        print(f'round {i}: {len(cs)} crawlers, highest {hi}')
        if len(cs) >= 2:
            target = cs[0]
            src = hi
            print('place', request(f'place {target[0]} {src[1]:.2f} {src[2]:.2f} {src[3]:.2f}')[1])
    time.sleep(1.0)
print('final:', crawlers())
