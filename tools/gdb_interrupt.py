# gdb -x tools/gdb_interrupt.py: SIGINT the inferior after $MP_SAMPLE_SECS (default 240) so a
# following `-ex "thread apply all bt"` samples a running boot (ptrace attach is blocked).
import os, signal, threading

def _fire():
    me = os.getpid()
    for t in os.listdir("/proc/%d/task" % me):
        try:
            kids = open("/proc/%d/task/%s/children" % (me, t)).read().split()
        except OSError:
            continue
        for k in kids:
            os.kill(int(k), signal.SIGINT)

threading.Timer(float(os.environ.get("MP_SAMPLE_SECS", "240")), _fire).start()
