# ds41: the arms of DarkSamus rows the rooftop fight (tools/drivers/darksamus_ds40.con) does not
# take, forced on the coverage build (the gdb side is this spec's own code, as in ds40):
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_INPUT=LAYER2@400 \
#     tools/cov/force.sh tools/cov/specs/darksamus_ds41_force.py <out> "1baa96c2,10_cliff_rooftop" \
#     --con tools/drivers/darksamus_ds40.con --fast --frames 5900
# Every write below is undone when the call that saw it returns (see darksamus_ds40_force.py).
#   14284 CanMissileDeflect  call 3k+1: GetBattleStage answers null (0x00014298 not taken);
#                            3k+2: the stage's deflect time (its info's +0x24) -1.0f (the
#                            fcmpo at 0x000142B0 not greater)
#   996C GetBoostBallActor (const)  GetOrbitPosition / GetAimPosition call 2k+1: the ball's uid
#                            0xE38 names the player (an actor with no ball out, the fight never
#                            asks the boss's orbit/aim point while the ball rolls), so fn_10_9624
#                            answers true and fn_10_996C's lookup arm (0x00009994) runs
import os
import gdb

CLASS = 'CDarkSamus'
FORCE_RET = []
PLAYER_UID = '((CStateManager*)$rsi)->mPlayers[0]->mUniqueId.value'
ENTRY = [
    ('996C ball = player (orbit)', 'CDarkSamus::GetOrbitPosition(', 2,
     {1: [('$this->mBoostBallUid.value', PLAYER_UID)]}),
    ('996C ball = player (aim)', 'CDarkSamus::GetAimPosition(', 2,
     {1: [('$this->mBoostBallUid.value', PLAYER_UID)]}),
]
STAGE = '((CDarkSamusBattleStage*)%d)'
CALLEE = [
    ('14284 stage null / below zero', 'CDarkSamus::CanMissileDeflect(', 'CDarkSamus::GetBattleStage(', 3,
     {1: ('ret', '0'), 2: ('obj', [(STAGE + '->mStageInfo.missiles.unknown_0x7ee77018', '-1')])}),
]

OUT = os.environ['COV_OUT']
LOGF = os.path.join(OUT, 'ds41.log')
counts = {}
syms = []
for ln in open(os.environ['COV_NM']):
    parts = ln.rstrip('\n').split('\t')
    if len(parts) == 2:
        f = parts[0].split()
        if len(f) == 3 and f[1] in 'tTW':
            syms.append((f[2], parts[1]))


def addrs(prefix):
    return [m for m, d in syms if d.startswith(prefix) and '.cold' not in d and 'clone' not in d]


def log(key, msg):
    counts[key] = counts.get(key, 0) + 1
    if counts[key] <= 4:
        with open(LOGF, 'a') as fh:
            fh.write('%s #%d: %s\n' % (key, counts[key], msg))


def dump():
    with open(os.path.join(OUT, 'ds41_counts.txt'), 'w') as fh:
        for k, v in sorted(counts.items()):
            fh.write('%-40s %d\n' % (k, v))


def this_expr():
    return '((%s*)%d)' % (CLASS, int(gdb.parse_and_eval('$rdi')))


def write(addr, ctype, val):
    if isinstance(addr, str):  # a bit-field or an enum, restored through its expression
        gdb.execute('set var %s = %s' % (addr, val))
    else:
        gdb.execute('set var *(%s*)%d = %s' % (ctype, addr, val))


class Undo(gdb.FinishBreakpoint):
    def __init__(self, frame, label, saved, after=None):
        super().__init__(frame, internal=True)
        self.label, self.saved, self.after = label, saved, after

    def stop(self):
        for addr, ctype, old in reversed(self.saved):
            try:
                write(addr, ctype, old)
            except gdb.error as e:
                log(self.label + ' undo failed', str(e)[:60])
        if self.after:
            self.after()
        return False

    def out_of_scope(self):
        log(self.label + ' left without a return', '')
        if self.after:
            self.after()


def resolve(lv, this):
    v = gdb.parse_and_eval(lv.replace('$this', this))
    code = v.type.strip_typedefs().code
    if code == gdb.TYPE_CODE_FLT:
        return int(v.address), 'float', repr(float(v))
    size = v.type.strip_typedefs().sizeof
    ctype = {1: 'unsigned char', 2: 'unsigned short', 4: 'unsigned int', 8: 'unsigned long'}[size]
    return int(v.address), ctype, str(int(v))


class EntryBp(gdb.Breakpoint):
    def __init__(self, spec, label, period, phases):
        super().__init__(spec, internal=True)
        self.label, self.period, self.phases, self.n = label, period, phases, 0

    def stop(self):
        self.n += 1
        writes = self.phases.get(self.n % self.period)
        if not writes:
            return False
        this = this_expr()
        resolved = []
        for lv, val in writes:
            try:
                addr, ctype, old = resolve(lv, this)
                resolved.append((addr, ctype, old, val.replace('$this', this)))
            except gdb.error as e:
                log(self.label + ' resolve failed', '%s: %s' % (lv, str(e)[:60]))
        saved = []
        for addr, ctype, old, val in resolved:
            try:
                newv = gdb.parse_and_eval(val)
                write(addr, ctype, repr(float(newv)) if ctype == 'float' else str(int(newv)))
                saved.append((addr, ctype, old))
            except gdb.error as e:
                log(self.label + ' set failed', str(e)[:60])
        Undo(gdb.newest_frame(), self.label, saved)
        log(self.label, 'call %d phase %d' % (self.n, self.n % self.period))
        return False


class CalleeBp(gdb.Breakpoint):
    def __init__(self, spec, owner):
        super().__init__(spec, internal=True)
        self.owner = owner
        self.enabled = False

    def stop(self):
        o = self.owner
        frame = gdb.newest_frame()
        older = frame.older()
        if older is None or older.pc() != o.caller_frame_pc_check(older):
            return False
        act = o.action
        o.hits += 1
        if act[0] == 'retn':
            if o.hits != act[1]:
                return False
            act = ('ret', act[2])
        if act[0] == 'outf':
            ptr = int(gdb.parse_and_eval(act[1]))
            ValueFin(frame, o, lambda: write(ptr, 'float', act[2]))
        elif act[0] == 'ret':
            ValueFin(frame, o, lambda: gdb.execute('set var $rax = %s' % act[1]))
        elif act[0] == 'far':
            this = o.this

            def far():
                x = float(gdb.parse_and_eval(this + '->mPosition.mX')) + 100.0
                y = float(gdb.parse_and_eval(this + '->mPosition.mY'))
                z = float(gdb.parse_and_eval(this + '->mPosition.mZ'))
                gdb.execute('set var $xmm0.v4_float[0] = %r' % x)
                gdb.execute('set var $xmm0.v4_float[1] = %r' % y)
                gdb.execute('set var $xmm1.v4_float[0] = %r' % z)
            ValueFin(frame, o, far)
        elif act[0] == 'obj':
            def obj():
                p = int(gdb.parse_and_eval('$rax'))
                if p == 0:
                    return
                for fmt, val in act[1]:
                    try:
                        expr = fmt % p
                        o.saved.append((expr, None, str(int(gdb.parse_and_eval(expr)))))
                        gdb.execute('set var %s = %s' % (expr, val))
                    except gdb.error as e:
                        log(o.label + ' obj failed', str(e)[:60])
            ValueFin(frame, o, obj)
        return False


class ValueFin(gdb.FinishBreakpoint):
    def __init__(self, frame, owner, fn):
        super().__init__(frame, internal=True)
        self.owner, self.fn = owner, fn

    def stop(self):
        try:
            self.fn()
            log(self.owner.label, 'call %d phase %d' % (self.owner.n, self.owner.n % self.owner.period))
        except gdb.error as e:
            log(self.owner.label + ' force failed', str(e)[:60])
        return False

    def out_of_scope(self):
        pass


class CallerBp(gdb.Breakpoint):
    def __init__(self, spec, label, callee, period, phases):
        super().__init__(spec, internal=True)
        self.label, self.period, self.phases, self.n = label, period, phases, 0
        self.callees = [CalleeBp('*' + m, self) for m in addrs(callee)]
        if not self.callees:
            log(label + ' no callee symbol', callee)
        self.action, self.ret_pc, self.this, self.saved = None, None, None, []

    def caller_frame_pc_check(self, older):
        # the callee's direct caller is this body when its frame's function is ours
        return older.pc() if older.function() is not None and \
            older.function().name.startswith(self.prefix) else -1

    def stop(self):
        self.n += 1
        act = self.phases.get(self.n % self.period)
        if act is None or self.action is not None:
            return False
        self.action, self.this, self.saved, self.hits = act, this_expr(), [], 0
        for c in self.callees:
            c.enabled = True

        def done():
            for c in self.callees:
                c.enabled = False
            self.action = None
        Undo(gdb.newest_frame(), self.label, self.saved, done)
        return False


for label, where, period, phases in ENTRY:
    ms = addrs(where)
    if not ms:
        log(label + ' no symbol', where)
    for m in ms:
        EntryBp('*' + m, label, period, phases)

for label, caller, callee, period, phases in CALLEE:
    ms = addrs(caller)
    if not ms:
        log(label + ' no caller symbol', caller)
    for m in ms:
        bp = CallerBp('*' + m, label, callee, period, phases)
        bp.prefix = caller.rstrip('(')

import atexit
atexit.register(dump)
gdb.events.exited.connect(lambda ev: dump())
