# Shared helpers for the call-scoped gdb forcing specs (the rez5 way): `exec` this file from a spec after setting
# OUT (COV_OUT), a LOG file name and, per spec, GATE_SYMS = {name: symbol prefix} / AFTER_SYMS.
#   run(cmd), ev(expr), w(lvalue, value), flush() (PortCoverageFlush: __gcov_dump + reset),
#   Gate: a callee's return value answered by sticky/seq values while enabled (gates_on/gates_off),
#   After: writes made at the return of a callee, Queue: per-call phases on a body of the object `$t`.
# Pitfalls (rez5): a gdb `condition` next to a Python stop() is ignored (test inside stop()); gdb.Value of a memory
# lvalue is lazy (convert before the write); `$al` is a register; console `send <id> Create` crashes the host.
import os
import gdb

OUT = os.environ['COV_OUT']
syms = []
for ln in open(os.environ['COV_NM']):
    parts = ln.rstrip('\n').split('\t')
    if len(parts) == 2:
        f = parts[0].split()
        if len(f) == 3 and f[1] in 'tTW':
            syms.append((f[2], parts[1]))


def addr(prefix):
    hits = [m for m, d in syms if d.startswith(prefix) and '.cold' not in d and 'clone' not in d]
    if len(hits) != 1:
        raise gdb.GdbError('%s: %d symbols' % (prefix, len(hits)))
    return '*' + hits[0]


def addrs(prefix):
    hits = [m for m, d in syms if d.startswith(prefix) and '.cold' not in d and 'clone' not in d]
    if not hits:
        raise gdb.GdbError('%s: no symbol' % prefix)
    return ['*' + h for h in sorted(set(hits))]


def log(msg):
    with open(LOG, 'a') as fh:
        fh.write(msg + '\n')


def run(cmd):
    try:
        out = gdb.execute(cmd, to_string=True)
        log('> %s\n%s' % (cmd, out.rstrip()))
        return out
    except gdb.error as e:
        log('> %s\n  ERROR %s' % (cmd, e))
        return ''


def ev(expr):
    return gdb.parse_and_eval(expr)


def w(lv, v):
    run('set var %s = %s' % (lv, v))


def flush():
    run('call (void)PortCoverageFlush()')


gdb.execute('set $busy = 0')
gdb.execute('set $tvalid = 0')

class GateRet(gdb.FinishBreakpoint):
    def __init__(self, frame, val):
        super().__init__(frame, internal=True)
        self.val = val

    def stop(self):
        gdb.execute('set var $rax = %d' % self.val)
        return False

    def out_of_scope(self):
        pass


class Gate(gdb.Breakpoint):
    """A callee's return value is overridden: `seq` answers are used in order, then `sticky` (None: real).
    The breakpoint stays disabled except inside a phase, so a hot callee costs nothing otherwise."""

    def __init__(self, sym):
        super().__init__(addr(sym), internal=True)
        self.seq = []
        self.sticky = None
        self.calls = 0
        self.enabled = False

    def stop(self):
        self.calls += 1
        val = self.seq.pop(0) if self.seq else self.sticky
        if val is not None:
            GateRet(gdb.newest_frame(), val)
        return False


GATES = {n: Gate(sym) for n, sym in GATE_SYMS.items()}


class AfterRet(gdb.FinishBreakpoint):
    def __init__(self, frame, writes):
        super().__init__(frame, internal=True)
        self.writes = writes

    def stop(self):
        for lv, v in self.writes:
            gdb.execute('set var %s = %s' % (lv, v))
        return False

    def out_of_scope(self):
        pass


class After(gdb.Breakpoint):
    """At the return of `sym` the pending `writes` are made on the object of the call (`$t` = rdi)."""

    def __init__(self, sym):
        super().__init__(addr(sym), internal=True)
        self.writes = []
        self.enabled = False

    def stop(self):
        if self.writes:
            obj = '((%s*)%d)' % (CLASS, int(ev('$rdi')))
            AfterRet(gdb.newest_frame(), [(lv.replace('$t', obj), v) for lv, v in self.writes])
        return False


AFTER = {n: After(sym) for n, sym in AFTER_SYMS.items()}


def gates_on(spec):
    """spec: {name: (sticky, [seq])}"""
    for n, (sticky, seq) in spec.items():
        g = GATES[n]
        g.sticky = sticky
        g.seq = list(seq)
        g.enabled = True


def gates_off(names=None):
    for n, g in GATES.items():
        if names is None or n in names:
            g.enabled = False
            g.sticky = None
            g.seq = []


class QueueRet(gdb.FinishBreakpoint):
    def __init__(self, frame, saved, gates, afters=()):
        super().__init__(frame, internal=True)
        self.saved = saved
        self.gates = gates
        self.afters = afters

    def stop(self):
        for lv, v in self.saved:
            gdb.execute('set var %s = %s' % (lv, v))
        gates_off(self.gates)
        return False

    def out_of_scope(self):
        pass


class Queue(gdb.Breakpoint):
    """At each call of `sym` on the object `$t` the next phase's writes are applied (restored at the return);
    phase = {'w': [(lvalue, value)], 'g': {gate: (sticky, [seq])}, 'when': gdb expression}.
    cyclic: phases repeat, at most `cap` calls are used; otherwise the breakpoint disables after the last phase."""

    def __init__(self, sym, phases, tag, cyclic=False, cap=0, anyobj=False):
        super().__init__(addr(sym))
        self.anyobj = anyobj
        self.phases = phases
        self.tag = tag
        self.i = 0
        self.cyclic = cyclic
        self.cap = cap

    def stop(self):
        try:
            if int(ev('$rdi')) != int(ev('(long)$t')) and not self.anyobj:   # a gdb condition is not honoured next to stop()
                return False
            obj = '((%s*)%d)' % (CLASS, int(ev('$rdi'))) if self.anyobj else '$t'
            sub = lambda x: x.replace('$t', obj) if isinstance(x, str) else x
            ph = self.phases[self.i % len(self.phases)]
            if 'when' in ph and not int(ev(sub(ph['when']))):
                return False
            self.i += 1
            log('# queue %s phase %d' % (self.tag, (self.i - 1) % len(self.phases)))
            saved = []
            for lv, val in ph.get('w', []):
                lv, val = sub(lv), sub(val)
                if lv.startswith('!'):
                    gdb.execute('set var %s = %s' % (lv[1:], val))
                else:
                    old = ev(lv)   # lazy: convert before the write
                    saved.append((lv, repr(float(old)) if old.type.code == gdb.TYPE_CODE_FLT else str(int(old))))
                    gdb.execute('set var %s = %s' % (lv, val))
            gates_on(ph.get('g', {}))
            for name, wl in ph.get('after', {}).items():
                AFTER[name].writes = [(sub(lv), sub(v)) for lv, v in wl]
                AFTER[name].enabled = True
            QueueRet(gdb.newest_frame(), saved, list(ph.get('g', {}).keys()), list(ph.get('after', {}).keys()))
            if (not self.cyclic and self.i >= len(self.phases)) or (self.cyclic and self.i >= self.cap):
                self.enabled = False
        except Exception as e:
            log('QUEUE EXC %s %r' % (self.tag, e))
        return False


