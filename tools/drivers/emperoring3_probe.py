#!/usr/bin/env python3
"""Console driver for the EmperorIngStage3 REL (module 18): watch the boss's state machine.

usage: emperoring3_probe.py <port> [outfile] [seconds]

The ing's unique id changes from run to run, so the driver finds it first
(`objs emperoring` -> the CEmperorIngStage3 line), then polls `obj <id>` for the FSM state and
the health and prints every state change.  With `damage` every few seconds the fight runs and
the walk covers the attack states; without it the probe only shows the idle walk.

Replies end with "=> ok" / "=> err: ..."; the game's own log is not read.
"""
import re
import socket
import sys
import time

port = int(sys.argv[1])
outfile = sys.argv[2] if len(sys.argv) > 2 else None
budget = float(sys.argv[3]) if len(sys.argv) > 3 else 180.0

lines = []


def emit(text):
    lines.append(text)
    print(text, flush=True)


s = None
t0 = time.time()
while s is None and time.time() - t0 < 600:  # the game may wait for a run slot
    try:
        s = socket.create_connection(('127.0.0.1', port), timeout=5)
    except OSError:
        time.sleep(0.5)
if s is None:
    emit('[emp3] no connect')
    sys.exit(1)
emit(f'[emp3] connected at {time.time() - t0:.1f}s')
s.settimeout(30)  # a reply can take a while when the machine is loaded
f = s.makefile('rw')


def request(cmd):
    """Send a command and return its reply lines (the final "=> ..." line excluded)."""
    try:
        f.write(cmd + '\n')
        f.flush()
    except (BrokenPipeError, OSError):
        return None  # the game is gone (the frame limit is up)
    out = []
    while True:
        try:
            line = f.readline()
        except (TimeoutError, OSError):
            return None  # the game is gone or not answering
        if not line:
            return None
        line = line.rstrip('\n')
        if line.startswith('=> '):
            return out
        out.append(line)


# The ing lives on layer 4 of the moth lord sanctuary (dark).
if request('layer 4') is None:
    emit('[emp3] the game is gone')
    sys.exit(1)

uid = None
deadline = time.time() + budget
seen = []
last = None
next_damage = time.time() + 8.0
while time.time() < deadline:
    objs = request('objs emperoring')
    if objs is None:
        emit('[emp3] the game is gone')
        break
    for line in objs:
        m = re.search(r'\bu([0-9a-f]+)\b.*CEmperorIngStage3', line)
        if m:
            uid = 'u' + m.group(1)
            break
    if uid is None:
        time.sleep(2.0)
        continue
    detail = request(f'obj {uid}')
    if detail is None:
        emit('[emp3] the game is gone')
        break
    state = None
    hp = None
    for line in detail:
        m = re.search(r'^\s*fsm (.+)$', line)
        if m:
            state = m.group(1).strip()
        m = re.search(r'^\s*hp ([0-9.]+)/([0-9.]+)', line)
        if m:
            hp = f'{m.group(1)}/{m.group(2)}'
    if state is None:
        state = '(no state)'
    if state != last:
        emit(f'[emp3] {time.time() - t0:6.1f}s fsm {state}' + (f' hp {hp}' if hp else ''))
        if state not in seen:
            seen.append(state)
        last = state
    if time.time() >= next_damage:
        request(f'damage {uid} 40 power')
        next_damage = time.time() + 6.0
    time.sleep(1.0)

emit('[emp3] states: ' + ', '.join(seen))
if outfile:
    with open(outfile, 'w') as fh:
        fh.write('\n'.join(lines) + '\n')
