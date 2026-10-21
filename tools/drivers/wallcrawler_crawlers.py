#!/usr/bin/env python3
"""Console driver for the WallCrawler module's runs (MP2_CONSOLE=<port>).

usage: wallcrawler_crawlers.py <port> [area slot] [outfile]

What it does, in the order the module's arms need:

  1. `warp <slot>` - the Agon Wastes area with the PillBugs (slot 40 is 0b_sand_hall, the area
     whose PillBugs carry a `planarConstraint`); the crawlers only think once the area is loaded.
  2. `objs pillbug` in a loop until the crawlers are there (the area streams in over a few
     hundred updates), then the first crawler's id.
  3. `place <id> player 1` - the crawler is moved one unit in front of the player, so their touch
     bounds overlap: the contact damage of fn_83_580 runs.
  4. `press x 1` and `boost` - the player morphs (X is the morph button) and boosts, so the
     boost-ball arm of
     fn_83_580 runs (it damages the crawler instead of the player).

Every reply is echoed, so the run's log and this output hold the same record.
"""
import re
import socket
import sys
import time

port = int(sys.argv[1])
slot = sys.argv[2] if len(sys.argv) > 2 else '40'
outfile = sys.argv[3] if len(sys.argv) > 3 else None

out_lines = []


def emit(s):
    out_lines.append(s)
    print(s, flush=True)


t0 = time.time()
s = None
while s is None and time.time() - t0 < 600:  # the game may wait for a run slot
    try:
        s = socket.create_connection(('127.0.0.1', port), timeout=5)
    except OSError:
        time.sleep(0.5)
if s is None:
    emit('[wc] no connect')
    sys.exit(1)
emit(f'[wc] connected at {time.time()-t0:.1f}s')
f = s.makefile('rw')


def request(c):
    """Send one command; return its reply lines (without the final '=> ...') and that line."""
    f.write(c + '\n')
    f.flush()
    lines = []
    s.settimeout(60)
    while True:
        l = f.readline()
        if not l:
            return lines, '[wc] eof'
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


def run(c):
    lines, end = request(c)
    emit(f'[wc f{frame()}] > {c}')
    for l in lines:
        emit('   ' + l)
    emit('   ' + end)
    return lines, end


try:
    # Wait for the gameplay: a warp during the world's own switch finds the object-id pool
    # half-empty and the area load is handed to the game's transition, which never happens.
    while frame() < 2000:
        time.sleep(0.5)
    run('warp ' + slot)
    # The area streams in: the crawlers appear a few hundred updates later.
    ids = []
    for _ in range(60):
        time.sleep(1.0)
        lines, end = request('objs pillbug')
        ids = re.findall(r'\bu[0-9a-f]{4}\b', '\n'.join(lines))
        if ids:
            emit(f'[wc] {len(ids)} pillbugs: {ids[:8]}')
            break
    if not ids:
        emit('[wc] no pillbug found')
    else:
        cid = ids[0]
        # The contact damage: the crawler next to the player.
        run(f'place {cid} player 0')
        time.sleep(1.0)
        run('status')
        run(f'obj {cid}')
        # The boost ball: the player morphs and boosts, so the crawler takes the damage.
        run('press x 30')   # the morph button is X on the pad; the hold has to outlast the
                            # update that reads the press edge
        time.sleep(1.0)
        run('status')
        run('boost')
        time.sleep(1.0)
        run(f'place {cid} player 0')
        time.sleep(1.0)
        run(f'obj {cid}')
        run('status')
except OSError as e:
    emit(f'[wc] socket error: {e}')
if outfile:
    open(outfile, 'w').write('\n'.join(out_lines) + '\n')
