#!/usr/bin/env python3
"""Console driver for MP2 port harness runs (MP2_CONSOLE=<port>).

usage: con.py <port> <cmdfile> [outfile]

cmdfile lines:
  @<secs> <command>   send at <secs> of wall time since the driver started
  @f<n> <command>     run at game update <n> (the `frame` command, what MP2_FRAMES counts) - use
                      this with MP2_TICKS_PER_FRAME, which runs the game faster than real time, so
                      wall-clock times no longer line up
  <command>           send as soon as the previous one is answered
Blank lines and lines starting with '#' are skipped. Replies end with "=> ok" / "=> err: ...".

A run of consecutive @f lines is queued at once with the console's `at <n> <command>` (each part of
an `a; b` line gets its own `at`, all for the same update, so they still run in one tick), and the
replies are collected as the game reaches each update: the command runs exactly at update n however
fast the game goes (an unpaced fast run does thousands of updates a second, so polling `frame` landed
commands hundreds of updates late). A line whose update has already passed runs on the next update;
its tag then says the update it ran at. CON_POLL=1 restores the old polling (send once `frame` >= n,
a few updates late).
"""
import os
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
while s is None and time.time() - t0 < 600:  # the game may wait for a run slot
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
    # An attached gdb (force.sh --attach) can stall the game for minutes: raise CON_REPLY_TIMEOUT.
    s.settimeout(float(os.environ.get('CON_REPLY_TIMEOUT', '60')))
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


def read_block():
    """One reply block: lines up to and including its '=> ' line; None at EOF."""
    lines = []
    while True:
        l = f.readline()
        if not l:
            return None
        l = l.rstrip()
        lines.append(l)
        if l.startswith('=> '):
            return lines


def run_at_batch(batch):
    """Queue a run of @f lines with `at`, then emit each one's replies once the game ran it."""
    parts_of = [[p.strip() for p in c.split(';') if p.strip()] for _, _, c in batch]
    f.write(''.join('; '.join(f'at {v} {p}' for p in parts) + '\n'
                    for (_, v, _), parts in zip(batch, parts_of)))
    f.flush()
    want_queued = sum(len(p) for p in parts_of)
    # (update, part) -> its result blocks, in the order the game ran them
    done = {}
    late = {}
    queued = 0
    ran = 0
    s.settimeout(None)  # a deferred reply comes when the game gets there; EOF ends a dead game
    while queued < want_queued or ran < want_queued:
        b = read_block()
        if b is None:
            return False
        if b[0].startswith('[at '):
            head = b[0]
            upd = int(head[4:head.index(']')])
            part = head[head.index('> ') + 2:]
            done.setdefault((upd, part), []).append(b[1:])
            ran += 1
        else:
            queued += 1
            if b[-1].startswith('=> err'):
                ran += 1  # refused, nothing will run; it is reported below
                emit('   [con] at refused: ' + ' | '.join(b))
            for l in b:
                if l.startswith('queued for update ') and '(now ' in l:
                    n = int(l.split()[3])
                    now = int(l[l.index('(now ') + 5:].split(')')[0])
                    if now > n:
                        late[n] = max(late.get(n, 0), now + 1)
    for (kind, value, c), parts in zip(batch, parts_of):
        out = []
        first_err = None
        last = '=> err: never ran'
        for p in parts:
            got = done.get((value, p))
            blk = got.pop(0) if got else ['=> err: never ran']
            out += blk[:-1]
            last = blk[-1]
            if first_err is None and last.startswith('=> err'):
                first_err = last
            if len(parts) > 1:
                out.append(last)
        if len(parts) > 1:
            out.pop()  # the final '=> ' line is the result itself, as request() returns it
        emit(f'[con f{late.get(value, value)}] > {c}')
        for l in out:
            emit('   ' + l)
        emit('   ' + (first_err or last))
    return True


frame = -1
use_at = os.environ.get('CON_POLL', '0') != '1'
try:
    i = 0
    while i < len(cmds):
        if use_at and cmds[i][0] == 'f':
            j = i
            while j < len(cmds) and cmds[j][0] == 'f':
                j += 1
            ok = run_at_batch(cmds[i:j])
            i = j
            if not ok:
                emit('   [con] eof')
                break
            continue
        kind, value, c = cmds[i]
        i += 1
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
