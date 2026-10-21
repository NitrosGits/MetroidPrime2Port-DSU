#!/usr/bin/env python3
"""Drive the player turret in an MP2 port run: find it, enter it, fire, and force the script
messages the game's own flow never sends to a player turret.

usage: playerturret_drive.py <port> [outfile]

The console answers only while the game updates, so the driver waits for the world to be up (the
`status` command answers with the player's position) before it does anything.  The turret's unique
id comes from `objs turret` (the console prints the id with the turret's position), so the driver
can address the turret with `send <id> <message>`: `Stop` is the message the gallery's own script
sends when a round ends (the game never sends it to a player turret) and `Damage` is the message no
shipped code sends to a turret at all.

The driver prints every command and its reply, so the run's transcript shows which arms ran.
"""
import re
import socket
import sys
import time

port = int(sys.argv[1])
outfile = sys.argv[2] if len(sys.argv) > 2 else None
phase = sys.argv[3] if len(sys.argv) > 3 else 'main'
out_lines = []
out_file = open(outfile, 'a') if outfile else None


def emit(s):
    out_lines.append(s)
    print(s, flush=True)
    if out_file is not None:
        out_file.write(s + '\n')
        out_file.flush()


s = None
t0 = time.time()
while s is None and time.time() - t0 < 600:  # the game may wait for a run slot
    try:
        s = socket.create_connection(('127.0.0.1', port), timeout=5)
    except OSError:
        time.sleep(0.5)
if s is None:
    emit('[pt] no connect')
    sys.exit(1)
emit(f'[pt] connected at {time.time()-t0:.1f}s')
s.settimeout(60)
f = s.makefile('rw')


def request(cmd):
    """Send one command; return its reply lines and the final '=> ...' line."""
    f.write(cmd + '\n')
    f.flush()
    lines = []
    while True:
        l = f.readline()
        if not l:
            return lines, '[pt] eof'
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


def wait_frames(n):
    """Wait until the game has run n updates."""
    while True:
        fr = frame()
        if fr >= n:
            return fr
        time.sleep(0.02)


def run(cmd):
    lines, end = request(cmd)
    emit(f'[pt] > {cmd}')
    for l in lines:
        emit('   ' + l)
    emit('   ' + end)
    return lines, end


def find_turret(quiet=False):
    """The player turret's id and position, from `objs turret`."""
    if quiet:
        lines, _ = request('objs turret')
    else:
        lines, _ = run('objs turret')
    for l in lines:
        if 'CScriptPlayerTurret' not in l:
            continue
        m = re.search(r'\(([-\d.]+), ([-\d.]+), ([-\d.]+)\)', l)
        uid = l.split()[0]
        if not m:
            continue
        return uid, (float(m.group(1)), float(m.group(2)), float(m.group(3)))
    return None, None


def poll_turret(budget=6000, step=25):
    """Look for the turret every `step` updates, up to `budget` updates."""
    start = frame()
    while frame() < start + budget:
        uid, pos = find_turret(quiet=True)
        if uid is not None:
            return uid, pos
        wait_frames(frame() + step)
    return None, None


try:
    # The console answers `frame` only while the game updates, so that is the "the world is up"
    # test; the area load needs a few hundred updates more.
    while True:
        if frame() >= 0:
            break
        if time.time() - t0 > 300:
            emit('[pt] the game never started updating')
            break
        time.sleep(0.2)
    wait_frames(60)
    # The boot warp to the Agon Wastes (world 42b935e4) is served a few hundred updates in; the
    # layer command only works in the area that is loaded then.
    for _ in range(80):
        lines, _ = request('status')
        world_ok = False
        for l in lines:
            if l.startswith('world '):
                emit('   ' + l)
                world_ok = 'world 42b935e4' in l
        if world_ok:
            break
        wait_frames(30)
    wait_frames(30)
    # The player turret is on layer 3 of the area.
    uid, pos = find_turret()
    for layer in (3, 0, 1, 2):
        if uid is not None:
            break
        emit(f'[pt] no turret yet: layer {layer}')
        run(f'layer {layer}')
        wait_frames(300)
        uid, pos = poll_turret()
    if uid is None:
        emit('[pt] no player turret in this area')
    else:
        emit(f'[pt] turret {uid} at {pos} (phase {phase})')
        if phase == 'null':
            # The turret is active and has no player: the message cases take their "no player"
            # arms.
            run('send %s Start' % uid)
            wait_frames(20)
            run('send %s Stop' % uid)
            wait_frames(20)
            run('status')
        elif phase == 'stop':
            # The kSM_Stop case needs an active turret with a player in it, and the message kills
            # the player, so it goes last.  The pause lets the coverage tool's table (written
            # every 15 s of hits) catch the case's lines before the death stops the game's loop.
            time.sleep(20)
            run('tp %.1f %.1f %.1f' % pos)
            wait_frames(40)
            run('status')
            # The turret only fires when the player is in it and the turret has started (the
            # kSM_Start case sets the player's turret state to kTS_Active).
            run('send %s Start' % uid)
            wait_frames(20)
            run('press A 20')
            wait_frames(40)
            run('send %s Stop' % uid)
            wait_frames(40)
            run('status')
        else:
            # The messages the game's own flow never sends to a player turret.  "Open" is a
            # message the turret's switch does not have, so it takes the default case.
            run('send %s Activate' % uid)
            wait_frames(20)
            run('send %s Open' % uid)
            wait_frames(20)
            # The trigger that activates the turret is at the turret: stand on it.
            run('tp %.1f %.1f %.1f' % pos)
            wait_frames(40)
            run('status')
            # The turret only fires when the player is in it and the turret has started (the
            # kSM_Start case sets the player's turret state to kTS_Active).
            run('send %s Start' % uid)
            wait_frames(20)
            # The turret fires with A.  The second press is short and comes while the reload
            # timer (the guest's 0x1B8, the fire rate at 0x194) is still positive, so the shot is
            # skipped - the guest's arm at 0x12BC.  The stick command releases the button: the
            # console's `press` holds it, and a held button has no second press event.
            run('press A 1')
            wait_frames(1)
            run('stick 0 0 1')
            wait_frames(1)
            run('press A 1')
            wait_frames(40)
            run('status')
            run('send %s Damage' % uid)
            wait_frames(20)
            run('send %s Deactivate' % uid)
            wait_frames(20)
            run('send %s Start' % uid)
            wait_frames(20)
            if phase != 'forced':
                # The kSM_Stop case kills the player, and the death's prompt stops the game's
                # update loop, so a forced run leaves it out (the 'stop' phase covers that case).
                run('send %s Stop' % uid)
                wait_frames(20)
            run('send %s Deactivate' % uid)
            wait_frames(20)
            # A second damage message, now with no player in the turret: the multi-player
            # branch's "no player" arm.
            run('send %s Damage' % uid)
            wait_frames(20)
            run('status')
            run('objs turret')
except OSError as e:
    emit(f'[pt] socket error: {e}')
if out_file is not None:
    out_file.close()
