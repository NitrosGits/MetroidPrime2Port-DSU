# gdb side of a forcing run whose writes last one call: at the entry of a listed body the given
# members are written on the calls the entry's phase picks, and the old values are written back
# when that call returns (a gdb FinishBreakpoint), so the rest of the game never sees them.
# force.py's writes persist; when several bodies force the same member (a target id, a flag
# byte), their saves and restores interleave and the member sticks at a forced value. This
# script keeps every write inside its own call.
#
#   COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh <spec> <out> <world,area> ...
#
# The spec (COV_SPEC) defines
#   CLASS = 'CCommandoPirate'            # what `$this` is: ((CLASS*)$rdi)
#   FORCE_RET = [(label, 'CCommandoPirate::Body(', period, [(phase, [(lvalue, value), ...]), ...],
#                 'what it forces'), ...]
# An lvalue that starts with '!' is written but not restored (a member the body itself writes,
# e.g. an animation state the body moves on). A value may test the entry registers
# (`$rdx == 1 ? ... : <the lvalue>`, a state's EStateMsg).
# On the n-th call of the body (n from 1) every phase with n % period == phase writes its
# lvalues (`$this` is bound to the object of that call, so the restore at the return writes the
# same object). A spec SPEC list, if any, is counted by tools/cov/gdb_cov.py as in run.sh (its
# breakpoints come after these, so a key shows the forced value). The forced hits are logged to
# <out>/forced.log (the first three per entry) and counted in <out>/forced.txt at exit.
import atexit
import os
import gdb

ns = {}
exec(compile(open(os.environ['COV_SPEC']).read(), os.environ['COV_SPEC'], 'exec'), ns)
CLASS = ns.get('CLASS', '')
FORCE = ns.get('FORCE_RET', [])
OUT = os.environ['COV_OUT']
LOG = os.path.join(OUT, 'forced.log')
TABLE = os.path.join(OUT, 'forced.txt')
counts = {}

syms = []
for ln in open(os.environ['COV_NM']):
    parts = ln.rstrip('\n').split('\t')
    if len(parts) == 2:
        f = parts[0].split()
        if len(f) == 3 and f[1] in 'tTW':
            syms.append((f[2], parts[1]))


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


class RestoreBp(gdb.FinishBreakpoint):
    def __init__(self, frame, label, saved):
        super().__init__(frame, internal=True)
        self.label, self.saved = label, saved

    def stop(self):
        for lv, old in reversed(self.saved):
            try:
                gdb.execute('set var %s = %s' % (lv, old))
            except gdb.error as e:
                log('%s: restore failed (%s): %s' % (self.label, lv, str(e)[:60]))
        return False

    def out_of_scope(self):
        log('%s: the call left without a return (not restored)' % self.label)


class ForceBp(gdb.Breakpoint):
    def __init__(self, spec, label, period, phases, what):
        super().__init__(spec, internal=True)
        self.label, self.period, self.phases, self.what, self.n = label, period, phases, what, 0

    def stop(self):
        self.n += 1
        writes = [w for p, ws in self.phases if self.n % self.period == p for w in ws]
        if not writes:
            return False
        this = '((%s*)%d)' % (CLASS, int(gdb.parse_and_eval('$rdi')))
        saved = []
        for lv, val in writes:
            keep = lv.startswith('!')
            lv, val = lv.lstrip('!').replace('$this', this), val.replace('$this', this)
            try:
                v = gdb.parse_and_eval(lv)
                old = repr(float(v)) if v.type.strip_typedefs().code == gdb.TYPE_CODE_FLT else str(int(v))
                gdb.execute('set var %s = %s' % (lv, val))
                if not keep:
                    saved.append((lv, old))
            except gdb.error as e:
                log('%s: set failed (%s): %s' % (self.label, lv, str(e)[:60]))
        try:
            RestoreBp(gdb.newest_frame(), self.label, saved)
        except (gdb.error, ValueError) as e:
            log('%s: no restore (%s)' % (self.label, str(e)[:60]))
        counts[self.label] = counts.get(self.label, 0) + 1
        if counts[self.label] <= 3:
            log('%s hit %d: %s' % (self.label, self.n, self.what))
        return False


def dump():
    with open(TABLE, 'w') as fh:
        for label, n in sorted(counts.items()):
            fh.write('%-40s forced %d\n' % (label, n))


atexit.register(dump)
gdb.events.exited.connect(lambda ev: dump())

for label, where, period, phases, what in FORCE:
    hits = [m for m, d in syms if d.startswith(where) and '.cold' not in d and 'clone' not in d]
    if not hits:
        log('%s: no symbol for %s' % (label, where))
    for m in hits:
        ForceBp('*' + m, label, period, phases, what)

if ns.get('SPEC'):
    HERE = os.path.dirname(os.path.abspath(__file__)) if '__file__' in globals() else 'tools/cov'
    path = os.path.join(HERE, 'gdb_cov.py')
    exec(compile(open(path).read(), path, 'exec'), {'__name__': '__main__', '__file__': path})
