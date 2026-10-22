#!/usr/bin/env python3
"""SwampBossStage2 (module 79): the light chain's uid (fn_79_4F88 SendDecrement).

The retail's fn_79_4F88 has no caller in the module, so the coverage spec forces the call from
gdb.  What the game has to do is fill mLightChain: the boss reads it when it gets the
area-loaded message (`mLightChain = FindConnectedObject(mgr, kSS_InternalState2, kSM_None)`),
and it is the uid of the light chain actor, which is the actor the message goes to.

usage: swampboss2_decrement.py <port>
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

    # the area-loaded message fills mLightChain (the light chain actor's uid).
    lines, end = con.cmd("send %s AreaLoaded" % boss)
    print("[sb2] send %s AreaLoaded -> %s %s" % (boss, end, lines), flush=True)
    con.wait_frame(con.frame() + 60)
    print("[sb2] frame %d" % con.frame(), flush=True)
    # a coverage build writes its gcov counters on this command, so the run's data survives
    # even if the game is killed afterwards.
    lines, end = con.cmd("covdump")
    print("[sb2] covdump -> %s %s" % (end, lines), flush=True)
    con.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
