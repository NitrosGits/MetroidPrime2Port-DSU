# gdb side of a forcing run (tools/cov/force.sh): at the entry of the listed bodies the given
# members are written before the body runs, so the arm the fight cannot reach is taken.  The
# arm keys are recorded before the write (so the natural arms are visible) and every forced hit
# is logged to <out>/forced.log, so the note can say which arm was forced.
#
# The spec file (COV_SPEC) defines
#   CLASS = 'CDarkSamus'                 # what `$this` is: ((CLASS*)$rdi)
#   FORCE = [(label, 'CDarkSamus::Body(', [exprs], [sets], skip, 'what it forces'), ...]
# `exprs` are the arm conditions (as in tools/cov/gdb_cov.py), `sets` the members to write, and
# `skip` the number of hits to let pass before the write.  A label that ends in '|ret' records
# the function's return value as an extra key part (the low byte).
import os
import gdb

ns = {}
exec(compile(open(os.environ['COV_SPEC']).read(), os.environ['COV_SPEC'], 'exec'), ns)
CLASS = ns.get('CLASS', '')
FORCE = ns['FORCE']
OUT = os.environ['COV_OUT']
LOG = os.path.join(OUT, 'forced.log')
TABLE = os.path.join(OUT, 'forced.txt')

syms = []
for ln in open(os.environ['COV_NM']):
    parts = ln.rstrip('\n').split('\t')
    if len(parts) == 2:
        f = parts[0].split()
        if len(f) == 3 and f[1] in 'tTW':
            syms.append((f[2], parts[1]))


def ev(expr):
    try:
        return int(gdb.parse_and_eval(expr.replace('$this', '((%s*)$rdi)' % CLASS)))
    except gdb.error as e:
        return 'err:' + str(e)[:40]


class RetBp(gdb.FinishBreakpoint):
    def __init__(self, frame, label, key):
        super().__init__(frame, internal=True)
        self.label, self.key = label, key

    def stop(self):
        try:
            rax = int(gdb.parse_and_eval('$rax'))
        except gdb.error:
            rax = 0
        with open(TABLE, 'a') as fh:
            fh.write('%-40s %-30s ret=%d\n' % (self.label, self.key, rax & 0xff))
        return False


class ForceBp(gdb.Breakpoint):
    def __init__(self, spec, label, exprs, sets, skip, what, ret):
        super().__init__(spec, internal=False)
        self.label, self.exprs, self.sets = label, exprs, sets
        self.skip, self.what, self.n, self.ret = skip, what, 0, ret

    def stop(self):
        self.n += 1
        key = ', '.join(str(ev(e)) for e in self.exprs) if self.exprs else '(entry)'
        if self.ret:
            try:
                RetBp(gdb.newest_frame(), self.label, key)
            except (gdb.error, ValueError):
                pass
        with open(TABLE, 'a') as fh:
            fh.write('%-40s %-30s %s\n' % (self.label, key, 'FORCED' if self.n > self.skip else ''))
        if self.n <= self.skip:
            return False
        for s in self.sets:
            expr = s.replace('$this', '((%s*)$rdi)' % CLASS)
            try:
                gdb.execute('set var ' + expr)
            except gdb.error:
                try:
                    gdb.execute('set ' + expr)
                except gdb.error as e:
                    with open(LOG, 'a') as fh:
                        fh.write('%s: set failed (%s): %s\n' % (self.label, s, str(e)[:60]))
        if self.n <= self.skip + 3:
            with open(LOG, 'a') as fh:
                fh.write('%s hit %d: %s\n' % (self.label, self.n, self.what))
        return False


for label, where, exprs, sets, skip, what in FORCE:
    ret = label.endswith('|ret')
    if ret:
        label = label[:-4]
    hits = [m for m, d in syms if d.startswith(where) and '.cold' not in d and 'clone' not in d]
    if not hits:
        with open(LOG, 'a') as fh:
            fh.write('%s: no symbol for %s\n' % (label, where))
    for m in hits:
        ForceBp('*' + m, label, exprs, sets, skip, what, ret)
