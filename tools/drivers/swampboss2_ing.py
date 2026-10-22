#!/usr/bin/env python3
"""SwampBossStage2 (module 79): the ing-snatch helpers (fn_79_5D40 GetIngSnatchingPoint and
fn_79_5E3C GetIngSnatchingNormal).

Both bodies are dead code for this class: the module overrides Render (fn_79_A78C) and the
retail's Render does not call CPatterned::Render, whose IsBeingSnatched() branch is the only
caller of the ing-snatch transition (CPatterned::RenderIngSnatchingTransition).  The coverage
spec (tools/cov/specs/swampboss2_ingsnatch.py) therefore forces the calls from gdb.

What has to happen in the game is the ing possession itself: the boss is put on layer 6 and
sent the head-mode anim event (0x0e), which the retail's anim-event handler (fn_79_A838) turns
into SetIngPossessed(true, 15.f) - the boss is ing-possessed for 15 s and the possession blend
ramps from 0 to 1, so the transition would run if the render path reached it.

usage: swampboss2_ing.py <port>
"""
import socket
import sys
import time


class Console:
    def __init__(self, port, timeout=300.0):
        self.port = port
        self.timeout = timeout
        # the game needs a while to boot before the console port listens.
        start = time.time()
        while True:
            try:
                self.sock = socket.create_connection(("127.0.0.1", self.port), timeout=self.timeout)
                break
            except OSError:
                if time.time() - start > 600:
                    raise
                time.sleep(0.5)
        self.sock.settimeout(self.timeout)
        self.f = self.sock.makefile("rw")

    def cmd(self, line):
        try:
            self.f.write(line + "\n")
            self.f.flush()
        except (OSError, ValueError):
            return [], "[con] closed"
        out = []
        while True:
            try:
                raw = self.f.readline()
            except (OSError, socket.timeout):
                return out, "[con] timeout"
            if not raw:
                return out, "[con] eof"
            raw = raw.rstrip("\n")
            if raw.startswith("=> "):
                return out, raw
            out.append(raw)

    def frame(self):
        for l in self.cmd("frame")[0]:
            if l.startswith("frame "):
                return int(l.split()[1])
        return -1

    def wait_frame(self, target, timeout=300.0):
        start = time.time()
        while time.time() - start < timeout:
            f = self.frame()
            if f >= target:
                return f
            time.sleep(0.05)
        return -1

    def close(self):
        try:
            self.f.close()
            self.sock.close()
        except OSError:
            pass


def find_boss(con, timeout=600.0):
    """The boss's unique id, or None: the console's `objs` filter prints the class name."""
    start = time.time()
    while time.time() - start < timeout:
        lines, end = con.cmd("objs swampboss")
        for line in lines:
            fields = line.split()
            if fields and fields[0].startswith("u") and "SwampBoss" in line:
                return fields[0]
        if end.startswith("[con]"):
            print("[sb2] the console is gone (%s)" % end, flush=True)
            return None
        con.wait_frame((con.frame() + 10) if con.frame() > 0 else 30)
    return None


def main():
    port = int(sys.argv[1])
    con = Console(port)
    # the area must be loaded before the layer request, or the layer load never finishes.
    con.wait_frame(1200)
    lines, end = con.cmd("layer 6")
    print("[sb2] layer 6 -> %s" % end, flush=True)
    boss = find_boss(con)
    if boss is None:
        print("[sb2] no boss in the world", flush=True)
        return 1
    print("[sb2] the boss is %s" % boss, flush=True)

    # the head-mode anim event: the retail's handler calls SetIngPossessed(true, 15.f).
    lines, end = con.cmd("uevent %s 14" % boss)
    print("[sb2] uevent %s 14 -> %s %s" % (boss, end, lines), flush=True)
    # the possession blend ramps over 15 s of game time; wait it out so the render path
    # would have reached the ing-snatch transition if it could.
    con.wait_frame(con.frame() + 400)
    print("[sb2] frame %d" % con.frame(), flush=True)
    # a coverage build writes its gcov counters on this command, so the run's data survives
    # even if the game is killed afterwards.
    lines, end = con.cmd("covdump")
    print("[sb2] covdump -> %s %s" % (end, lines), flush=True)
    con.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
