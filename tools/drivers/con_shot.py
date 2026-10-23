#!/usr/bin/env python3
"""tools/drivers/con.py plus screenshots: the same cmdfile, and a command `!shot <png>` grabs the
X display named by $SHOT_DISPLAY (ffmpeg x11grab, $SHOT_SIZE, default 1280x720) instead of going
to the game. The transcript is rewritten after every command, so a crashed game keeps it.

usage: SHOT_DISPLAY=:<n> con_shot.py <port> <cmdfile> <outfile>
"""
import os
import socket
import subprocess
import sys
import time

port, cmdfile, outfile = int(sys.argv[1]), sys.argv[2], sys.argv[3]
cmds = []
for ln in open(cmdfile):
    ln = ln.strip()
    if not ln or ln.startswith('#'):
        continue
    if ln.startswith('@'):
        t, c = ln.split(' ', 1)
        cmds.append(('f', int(t[2:]), c) if t.startswith('@f') else ('s', float(t[1:]), c))
    else:
        cmds.append(('s', 0.0, ln))
out_lines = []


def emit(line):
    out_lines.append(line)
    with open(outfile, 'w') as o:
        o.write('\n'.join(out_lines) + '\n')


t0 = time.time()
s = None
while s is None and time.time() - t0 < 600:
    try:
        s = socket.create_connection(('127.0.0.1', port), timeout=5)
    except OSError:
        time.sleep(0.5)
if s is None:
    emit('[con] no connect')
    sys.exit(1)
s.settimeout(60)
f = s.makefile('rw')


def request(c):
    f.write(c + '\n')
    f.flush()
    lines, pending, first_err = [], c.count(';') + 1, None
    while True:
        l = f.readline()
        if not l:
            return lines, '[con] eof'
        l = l.rstrip()
        if l.startswith('=> '):
            pending -= 1
            first_err = first_err or (l if l.startswith('=> err') else None)
            if pending == 0:
                return lines, first_err or l
        lines.append(l)


def game_frame():
    lines, _ = request('frame')
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
            while True:
                frame = game_frame()
                if frame >= value:
                    break
                time.sleep(0.02)
        tag = f'f{frame}' if kind == 'f' else f'{time.time()-t0:.0f}s'
        if c.startswith('!shot '):
            png = c.split(None, 1)[1]
            disp = os.environ.get('SHOT_DISPLAY', ':86')
            r = subprocess.run(['ffmpeg', '-loglevel', 'error', '-f', 'x11grab', '-video_size',
                                os.environ.get('SHOT_SIZE', '1280x720'), '-i', disp,
                                '-frames:v', '1', '-y', png], capture_output=True, text=True)
            emit(f'[con {tag}] shot {png} rc={r.returncode} {r.stderr.strip()}')
            continue
        lines, end = request(c)
        emit(f'[con {tag}] > {c}')
        for l in lines:
            emit('   ' + l)
        emit('   ' + end)
        if end == '[con] eof':
            break
except OSError as e:
    emit(f'[con] socket error: {e}')
