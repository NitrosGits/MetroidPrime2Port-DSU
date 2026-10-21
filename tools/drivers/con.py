#!/usr/bin/env python3
"""Console driver for MP2 port harness runs (MP2_CONSOLE=<port>).

usage: con.py <port> <cmdfile> [outfile]

cmdfile lines:
  @<secs> <command>   send at <secs> of wall time since the driver started
  @f<n> <command>     send once the game has run <n> updates (the `frame` command, what
                      MP2_FRAMES counts) - use this with MP2_TICKS_PER_FRAME, which runs the game
                      faster than real time, so wall-clock times no longer line up
  <command>           send as soon as the previous one is answered
Blank lines and lines starting with '#' are skipped. Replies end with "=> ok" / "=> err: ...".
"""
import socket
import sys
import time

port, cmdfile = int(sys.argv[1]), sys.argv[2]
outfile = sys.argv[3] if len(sys.argv) > 3 else None
cmds = []  # (kind, value, command): kind 's' = wall seconds, 'f' = game updates
for ln in open(cmdfile):
    ln = ln.strip()
    if not ln or ln.startswith('#'):
        continue
    if ln.startswith('@'):
        t, c = ln.split(' ', 1)
        if t.startswith('@f'):
            cmds.append(('f', int(t[2:]), c))
        else:
            cmds.append(('s', float(t[1:]), c))
    else:
        cmds.append(('s', 0.0, ln))

out_lines = []


def emit(s):
    out_lines.append(s)
    print(s, flush=True)


t0 = time.time()
s = None
while s is None and time.time() - t0 < 120:
    try:
        s = socket.create_connection(('127.0.0.1', port), timeout=5)
    except OSError:
        time.sleep(0.5)
if s is None:
    emit('[con] no connect')
    sys.exit(1)
emit(f'[con] connected at {time.time()-t0:.1f}s')
f = s.makefile('rw')


def request(c):
    """Send one command; return its reply lines (without the final '=> ...'), and that line.

    `a; b` runs both in one game tick and answers once per command: the earlier '=> ' lines stay
    in the reply lines, and the first error (else the last result) is returned."""
    f.write(c + '\n')
    f.flush()
    lines = []
    pending = c.count(';') + 1
    first_err = None
    s.settimeout(60)
    while True:
        l = f.readline()
        if not l:
            return lines, '[con] eof'
        l = l.rstrip()
        if l.startswith('=> '):
            pending -= 1
            if first_err is None and l.startswith('=> err'):
                first_err = l
            if pending == 0:
                return lines, first_err or l
        lines.append(l)


def game_frame():
    lines, end = request('frame')
    for l in lines:
        if l.startswith('frame '):
            return int(l.split()[1])
    return -1


frame = -1
try:
    for kind, value, c in cmds:
        if kind == 's':
            d = value - (time.time() - t0)
            if d > 0:
                time.sleep(d)
        else:
            # The console answers only in game (`frame` fails before gameplay); poll until then.
            while True:
                frame = game_frame()
                if frame >= value:
                    break
                time.sleep(0.02)
        lines, end = request(c)
        tag = f'f{frame}' if kind == 'f' else f'{time.time()-t0:.0f}s'
        emit(f'[con {tag}] > {c}')
        for l in lines:
            emit('   ' + l)
        emit('   ' + end)
        if end == '[con] eof':
            break
except OSError as e:
    emit(f'[con] socket error: {e}')
if outfile:
    open(outfile, 'w').write('\n'.join(out_lines) + '\n')
