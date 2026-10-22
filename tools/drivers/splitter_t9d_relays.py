#!/usr/bin/env python3
"""Splitter (REL 75): Hive Temple's generated Splitters (09_cliff_temple_dark, a92f00b3).

Layer 10 holds the relays SRLY 28350006/11/28; activating them and sending SetToZero makes the
generators (GENR 28350045/42/4a) spawn the SpawnDrop/SpawnIdle chassis and the SpawnIdle heads.
The driver then reads every Splitter (`obj`) once a second so their FSM states land in the log.

usage: splitter_t9d_relays.py <console port> <out file>
  with tools/cov/run.sh:  ... <dir> 1baa96c2,a92f00b3 --frames 2700 \
      --driver 'python3 tools/drivers/splitter_t9d_relays.py {port} <dir>/con.out'
"""
import re
import socket
import sys
import time

PORT = int(sys.argv[1])
OUT = sys.argv[2]
RELAYS = ("350006", "350011", "350028")  # editor-id suffixes of the three SRLY

out = []
FR = [0]


def emit(text):
    out.append(text)
    with open(OUT, "w") as fh:
        fh.write("\n".join(out) + "\n")


t0 = time.time()
sock = None
while sock is None and time.time() - t0 < 600:
    try:
        sock = socket.create_connection(("127.0.0.1", PORT), timeout=5)
    except OSError:
        time.sleep(0.5)
if sock is None:
    sys.exit("no console on port %d" % PORT)
sock.settimeout(300)
f = sock.makefile("rw")


def req(c, log=True):
    f.write(c + "\n")
    f.flush()
    lines = []
    pending = c.count(";") + 1
    while True:
        line = f.readline()
        if not line:
            raise SystemExit
        line = line.rstrip()
        if line.startswith("=> "):
            pending -= 1
            if pending == 0:
                if log:
                    emit("[f%d] > %s | %s | %s" % (FR[0], c, " / ".join(lines)[:600], line))
                return lines
        else:
            lines.append(line)


def frame():
    for line in req("frame", False):
        if line.startswith("frame "):
            FR[0] = int(line.split()[1])
            return FR[0]
    return -1


def wait_until(n):
    while frame() < n:
        time.sleep(0.01)


def at(df, c):
    wait_until(FR[0] + df)
    return req(c)


def objs(flt):
    res = []
    for line in req("objs " + flt):
        m = re.match(r"\s*(u[0-9a-f]{4})\s+(\S+)\s+(\S+)", line)
        if m:
            res.append(m.groups())
    return res


while frame() < 0:
    time.sleep(0.2)

wait_until(300)
req("layer 10")
at(60, "objs Splitter")
relays = [u for u, eid, _ in objs("") if eid[-6:] in RELAYS]
emit("relays %s" % relays)
for r in relays:
    req("obj %s" % r)
    req("send %s Activate" % r)
    req("send %s SetToZero" % r)
for _ in range(35):
    at(60, "objs Splitter")
    ids = [u for u, _, _ in objs("Splitter")]
    req("; ".join("obj " + u for u in ids) or "frame")
emit("[done]")
