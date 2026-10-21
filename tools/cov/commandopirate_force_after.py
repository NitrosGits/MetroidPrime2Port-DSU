# gdb side of a forcing run that writes a member *after* a call into the base: the
# CommandoPirate's KnockBack override (fn_9_C300) reads the knock-back manager's active reaction
# flags (the word at guest this+0x630) right after it called the base's own KnockBack, and the
# base rewrites those flags - so a write at the override's entry is lost.  This script breaks on
# the base's CPatterned::KnockBack *when it is called from the override* and writes the flags to
# 3 when it returns.
#
#   COV_GDB_SCRIPT=tools/cov/commandopirate_force_after.py tools/cov/force.sh ...
import os

import gdb

# The usual sides first: tools/cov/force.py (the spec's FORCE list) and tools/cov/gdb_cov.py (its
# SPEC list, the arm counts), each in its own namespace - the same as tools/cov/cov_force.py.
HERE = os.path.dirname(os.path.abspath(__file__)) if '__file__' in globals() else 'tools/cov'
for name in ('force.py', 'gdb_cov.py'):
    path = os.path.join(HERE, name)
    exec(compile(open(path).read(), path, 'exec'), {'__name__': '__main__', '__file__': path})

OUT = os.environ['COV_OUT']
LOG = os.path.join(OUT, 'forced.log')
TABLE = os.path.join(OUT, 'forced.txt')


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


class WriteOnReturn(gdb.FinishBreakpoint):
    """Writes the knock-back manager's active reaction flags to 3 when the frame returns."""

    def __init__(self, frame, actor, label):
        super().__init__(frame, internal=True)
        self.actor = actor
        self.label = label

    def stop(self):
        try:
            gdb.execute('set var ((CCommandoPirate*)0x%x)->mKnockBackController'
                        '.mActiveParameters.mFlags = 3' % self.actor)
            with open(TABLE, 'a') as fh:
                fh.write('%-40s %-30s %s\n' % (self.label, '(on return)', 'FORCED'))
            log('%s: the knock-back flags written to 3 on the base\'s return' % self.label)
        except gdb.error as e:
            log('%s: the write failed: %s' % (self.label, str(e)[:60]))
        return False


class BaseKnockBackBp(gdb.Breakpoint):
    def __init__(self, spec, label):
        super().__init__(spec, internal=False)
        self.label = label

    def stop(self):
        frame = gdb.newest_frame()
        caller = ''
        try:
            caller = frame.older().name() or ''
        except gdb.error:
            pass
        if 'CCommandoPirate::KnockBack' not in caller:
            return False
        try:
            actor = int(gdb.parse_and_eval('(unsigned long)$rdi'))
        except gdb.error:
            actor = 0
        try:
            WriteOnReturn(frame, actor, self.label)
        except (gdb.error, ValueError):
            pass
        return False


def find(prefix):
    import subprocess
    nm = os.environ.get('COV_NM', '')
    out = []
    for line in open(nm):
        parts = line.rstrip('\n').split('\t')
        if len(parts) == 2:
            f = parts[0].split()
            if len(f) == 3 and f[1] in 'tTW' and parts[1].startswith(prefix) \
                    and '.cold' not in parts[1] and 'clone' not in parts[1]:
                out.append(f[2])
    return out


for addr in find('CPatterned::KnockBack('):
    BaseKnockBackBp('*' + addr, 'C300 flags on return')
log('commandopirate_force_after: the base\'s KnockBack is watched')
