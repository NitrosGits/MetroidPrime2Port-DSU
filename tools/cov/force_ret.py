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
# A label ending in `|once` disables its breakpoint after the first call that wrote.
# `$mgr` / `$P` in an lvalue or value: the CStateManager& argument (rsi) and its first player.
# An lvalue that starts with '!' is written but not restored (a member the body itself writes,
# e.g. an animation state the body moves on). A value may test the entry registers
# (`$rdx == 1 ? ... : <the lvalue>`, a state's EStateMsg).
# On the n-th call of the body (n from 1) every phase with n % period == phase writes its
# lvalues (`$this` is bound to the object of that call, so the restore at the return writes the
# same object). A spec SPEC list, if any, is counted by tools/cov/gdb_cov.py as in run.sh (its
# breakpoints come after these, so a key shows the forced value). The forced hits are logged to
# <out>/forced.log (the first three per entry) and counted in <out>/forced.txt at exit.
#
# FORCE_INNER = [(label, 'CCommandoPirate::Outer(', 'CStateManager::Inner(', period,
#                 {phase: [(lvalue, value), ...]}, 'what it forces'), ...]
# writes at the *return* of a callee, only while the outer body runs: on the n-th call of the
# callee from inside the outer body (n from 1, counted over the whole run) every phase with
# n % period == phase writes its lvalues once the callee returns and they are not restored.
# `$this` is the outer body's object; `$rax` is the callee's return value (a bool answer).
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
        # `$mgr` is the CStateManager& in rsi, `$P` the first player (the restore at the return runs
        # with other registers, so both are bound to their addresses now)
        mgr = '((CStateManager*)%d)' % int(gdb.parse_and_eval('$rsi'))
        player = '((CPlayer*)%s->mPlayers[0])' % mgr
        saved = []
        for lv, val in writes:
            keep = lv.startswith('!')
            lv = lv.lstrip('!')
            lv, val = [x.replace('$this', this).replace('$P', player).replace('$mgr', mgr) for x in (lv, val)]
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
        if self.label.endswith('|once'):  # a capture of an object: one sighting is enough
            self.enabled = False
        return False


class InnerRetBp(gdb.FinishBreakpoint):
    def __init__(self, frame, label, writes):
        super().__init__(frame, internal=True)
        self.label, self.writes = label, writes

    def stop(self):
        for lv, val in self.writes:
            try:
                gdb.execute('set var %s = %s' % (lv, val))
            except gdb.error as e:
                log('%s: set failed (%s): %s' % (self.label, lv, str(e)[:60]))
        return False

    def out_of_scope(self):
        pass


class InnerBp(gdb.Breakpoint):
    def __init__(self, spec, owner):
        super().__init__(spec, internal=True)
        self.owner = owner
        self.enabled = False

    def stop(self):
        o = self.owner
        if o.depth <= 0:  # a missed outer exit: off again
            self.enabled = False
            return False
        o.n += 1
        writes = [(lv.replace('$this', o.this), val.replace('$this', o.this))
                  for p, ws in o.phases.items() if o.n % o.period == p for lv, val in ws]
        if writes:
            try:
                InnerRetBp(gdb.newest_frame(), o.label, writes)
            except (gdb.error, ValueError) as e:
                log('%s: no return hook (%s)' % (o.label, str(e)[:60]))
            counts[o.label] = counts.get(o.label, 0) + 1
            if counts[o.label] <= 3:
                log('%s inner call %d: %s' % (o.label, o.n, o.what))
        return False


class OuterExitBp(gdb.FinishBreakpoint):
    def __init__(self, frame, owner):
        super().__init__(frame, internal=True)
        self.owner = owner

    def stop(self):
        self.owner.depth -= 1
        if self.owner.depth == 0:
            for bp in self.owner.inner:
                bp.enabled = False
        return False

    def out_of_scope(self):
        self.stop()


class OuterBp(gdb.Breakpoint):
    def __init__(self, spec, label, inner_specs, period, phases, what):
        super().__init__(spec, internal=True)
        self.label, self.period, self.phases, self.what = label, period, phases, what
        self.n, self.depth, self.this = 0, 0, ''
        self.inner = [InnerBp(s, self) for s in inner_specs]

    def stop(self):
        self.this = '((%s*)%d)' % (CLASS, int(gdb.parse_and_eval('$rdi')))
        try:
            OuterExitBp(gdb.newest_frame(), self)
        except (gdb.error, ValueError) as e:
            log('%s: no exit hook (%s)' % (self.label, str(e)[:60]))
            return False
        self.depth += 1
        for bp in self.inner:
            bp.enabled = True
        return False


class StubRetBp(gdb.Breakpoint):
    # a scenario's third item [('Class::Fn(', value)] (value a list: the 1st call returns [0], the 2nd [1], ... the last repeats): while the scenario's call runs, that
    # function returns `value` at once (the entry pops the return address, as `ret` would)
    def __init__(self, where, value):
        m = [m for m, d in syms if d.startswith(where) and '.cold' not in d and 'clone' not in d]
        super().__init__('*' + m[0], internal=True)
        self.value, self.where = value, where
        self.base = counts.get('stub ' + where, 0)

    def stop(self):
        counts['stub ' + self.where] = counts.get('stub ' + self.where, 0) + 1
        k = counts.get('stub ' + self.where, 1) - 1 - self.base
        gdb.execute('set $rax = %s' % self.value[min(k, len(self.value) - 1)])
        gdb.execute('set $rip = *(unsigned long*)$rsp')
        gdb.execute('set $rsp = $rsp + 8')
        return False


class CallBp(gdb.Breakpoint):
    # CALLS = [(label, 'Class::Body(', 'when', [([(lvalue, value), ...], 'call expr' [, stubs]), ...], 'what')]:
    # at the first entry of the body where the gdb expression `when` is non-zero, run each
    # scenario: write its members, evaluate the call expression (an inferior call; `$this`,
    # `$mgr`, `$P` as in FORCE_RET), write the old values back.  For arms a fight reaches
    # once in a run, not once per scenario.
    def __init__(self, spec, label, when, scenarios, what):
        super().__init__(spec, internal=True)
        self.label, self.when, self.scenarios, self.what, self.done = label, when, scenarios, what, False

    def stop(self):
        if self.done:
            return False
        try:
            if not int(gdb.parse_and_eval(self.when)):
                return False
        except gdb.error as e:
            if not getattr(self, 'warned', False):
                self.warned = True
                log('%s: when failed: %s' % (self.label, str(e)[:100]))
            return False
        self.done = True
        self.enabled = False
        pending.append(self)
        return True  # the calls run from the hook-backtrace below: gdb cannot make inferior calls inside stop()

    def run(self):
        this = '((%s*)%d)' % (CLASS, int(gdb.parse_and_eval('$rdi')))
        mgr = '((CStateManager*)%d)' % int(gdb.parse_and_eval('$rsi'))
        player = '((CPlayer*)%s->mPlayers[0])' % mgr
        sub = lambda x: x.replace('$this', this).replace('$P', player).replace('$mgr', mgr)
        for i, sc in enumerate(self.scenarios):
            writes, call = sc[0], sc[1]
            stubs = [StubRetBp(sym, [int(gdb.parse_and_eval(sub(str(v)))) for v in (val if isinstance(val, list) else [val])]) for sym, val in (sc[2] if len(sc) > 2 else [])]
            for b in stubs:
                log('%s #%d: stub %s returns %s' % (self.label, i, b.where, ' then '.join('%#x' % v for v in b.value)))
            saved = []
            for lv, val in writes:
                lv, val = sub(lv), sub(val)
                try:
                    v = gdb.parse_and_eval(lv)
                    old = repr(float(v)) if v.type.strip_typedefs().code == gdb.TYPE_CODE_FLT else str(int(v))
                    gdb.execute('set var %s = %s' % (lv, val))
                    saved.append((lv, old))
                except gdb.error as e:
                    log('%s #%d: set failed (%s): %s' % (self.label, i, lv, str(e)[:80]))
            try:
                r = gdb.parse_and_eval(sub(call))
                log('%s #%d: call ok (%s)' % (self.label, i, str(r)[:30]))
                counts[self.label] = counts.get(self.label, 0) + 1
            except gdb.error as e:
                log('%s #%d: call failed: %s' % (self.label, i, str(e)[:120]))
                try:
                    log(gdb.execute('bt 6', to_string=True)[:900])
                except gdb.error:
                    pass
            for lv, old in reversed(saved):
                try:
                    gdb.execute('set var %s = %s' % (lv, old))
                except gdb.error:
                    pass
            for b in stubs:
                b.delete()


pending = []


def run_pending():
    while pending:
        pending.pop(0).run()


# force.sh's batch is `-ex run -ex 'bt 20'`: the stop at a CallBp lands on the `bt` (hook-backtrace), whose hook runs
# the calls and lets the game go on (a `continue` there returns when it exits)
gdb.execute('define hook-backtrace\npython run_pending()\ncontinue\nend')


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

for label, where, when, scenarios, what in ns.get('CALLS', []):
    hits = [m for m, d in syms if d.startswith(where) and '.cold' not in d and 'clone' not in d]
    if not hits:
        log('%s: no symbol for %s' % (label, where))
    for m in hits:
        CallBp('*' + m, label, when, scenarios, what)

for label, outer, inner, period, phases, what in ns.get('FORCE_INNER', []):
    inner_hits = ['*' + m for m, d in syms if d.startswith(inner) and '.cold' not in d and 'clone' not in d]
    outer_hits = [m for m, d in syms if d.startswith(outer) and '.cold' not in d and 'clone' not in d]
    if not inner_hits or not outer_hits:
        log('%s: no symbol for %s / %s' % (label, outer, inner))
    for m in outer_hits:
        OuterBp('*' + m, label, inner_hits, period, phases, what)

if ns.get('SPEC'):
    HERE = os.path.dirname(os.path.abspath(__file__)) if '__file__' in globals() else 'tools/cov'
    path = os.path.join(HERE, 'gdb_cov.py')
    exec(compile(open(path).read(), path, 'exec'), {'__name__': '__main__', '__file__': path})
