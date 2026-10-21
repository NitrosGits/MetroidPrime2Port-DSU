# gdb side of the player turret's forcing runs (tools/cov/force.sh with COV_GDB_SCRIPT=this file).
# It is the shared tools/cov/force.py with two changes: the gdb errors of a write are logged (the
# shared one swallows them) and the write can be a plain expression.
#
#   tools/cov/force.sh <spec.py> <out dir> <world,area> --driver '<cmd {port}>' [--frames n] [--fast]
#   COV_GDB_SCRIPT=tools/drivers/playerturret_force.py
#
# The spec defines FORCE = [(label, 'Body(', [exprs], [sets], skip, 'what'), ...]; the class is
# the spec's CLASS.  Each hit is logged to <out>/forced.log and <out>/forced.txt.
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


class RetWriteBp(gdb.FinishBreakpoint):
    """Writes the function's return value when it returns (a bool comes back in al)."""

    def __init__(self, frame, value):
        super().__init__(frame, internal=True)
        self.value = value

    def stop(self):
        try:
            gdb.execute('set var $rax = %d' % self.value)
        except gdb.error:
            pass
        return False


class ForceBp(gdb.Breakpoint):
    def __init__(self, spec, label, exprs, sets, skip, what):
        # internal: the breakpoint must not stop the game.
        super().__init__(spec, internal=True)
        self.label, self.exprs, self.sets = label, exprs, sets
        self.skip, self.what, self.n = skip, what, 0

    def stop(self):
        self.n += 1
        key = ', '.join(str(ev(e)) for e in self.exprs) if self.exprs else '(entry)'
        if self.n <= 200:  # the table would otherwise be megabytes (the hit is per call)
            with open(TABLE, 'a') as fh:
                fh.write('%-40s %-40s %s\n' % (self.label, key, 'FORCED' if self.n > self.skip else ''))
        if self.n <= self.skip:
            return False
        for s in self.sets:
            if s.startswith('ret='):
                # The return value: written when the function returns, and only when the caller is
                # the module's own code (the port's game mode object answers 'SNGL' always).
                frame = gdb.newest_frame()
                older = frame.older()
                caller = (older.name() or '') if older is not None else ''
                if 'CScriptPlayerTurret' in caller:
                    try:
                        RetWriteBp(frame, int(s.split('=')[1]))
                    except (gdb.error, ValueError):
                        pass
                continue
            try:
                gdb.execute('set var ' + s.replace('$this', '((%s*)$rdi)' % CLASS))
            except gdb.error as e:
                with open(LOG, 'a') as fh:
                    fh.write('%s hit %d: the write failed: %s (%s)\n' % (self.label, self.n, s, str(e)[:80]))
        if self.n <= self.skip + 3:
            with open(LOG, 'a') as fh:
                fh.write('%s hit %d: %s\n' % (self.label, self.n, self.what))
        return False


for label, where, exprs, sets, skip, what in FORCE:
    hits = [m for m, d in syms if d.startswith(where) and '.cold' not in d and 'clone' not in d]
    if not hits:
        with open(LOG, 'a') as fh:
            fh.write('%s: no symbol for %s\n' % (label, where))
    for m in hits:
        ForceBp('*' + m, label, exprs, sets, skip, what)

# The counting too (tools/cov/gdb_cov.py's SPEC), as the shared tools/cov/cov_force.py does: the
# force's breakpoints are internal and made first, so at a shared entry the write happens before
# the arm key is read.
_gcov = os.environ.get('PT_GCOV_SCRIPT') or (
    '/run/media/odran/Leo/projects/Restored-projects/Chatgpt/wt-mp2-port/tools/cov/gdb_cov.py')
if os.environ.get('PT_COUNT') and os.path.exists(_gcov):
    exec(compile(open(_gcov).read(), _gcov, 'exec'), {'__name__': '__main__', '__file__': _gcov})
