# ds40: the arms of DarkSamus rows the rooftop fight (tools/drivers/darksamus_ds40.con) does not
# take, forced on the coverage build with tools/cov/force_ret.py as the gdb side:
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_INPUT=LAYER2@400 \
#     tools/cov/force.sh tools/cov/specs/darksamus_ds40_force.py <out> "1baa96c2,10_cliff_rooftop" \
#     --con tools/drivers/darksamus_ds40.con --fast --frames 5900
# Every write below is undone when the call that saw it returns. Entry writes (ENTRY) resolve
# their lvalues to addresses at the entry, so an argument register may name them. Callee forces
# (CALLEE) break on a callee only inside one call of the named caller (the callee breakpoint is
# enabled at the caller's entry and disabled at its return), so hot callees such as ObjectById
# cost nothing elsewhere. Logged to <out>/ds40.log, counted in <out>/ds40_counts.txt.
#   1E740 ShouldForgetLastAttacker  call 3k+1/3k+2: the search not shagged, of 1 / 3 waypoints
#                                   (the ds39 forcing); 6k+5: GetCoverPoint answers null (0x0001E7CC)
#   1DF74 TravelToCoverPoints       4k+3: GetCoverPoint answers null (the return at 0x0001E0A8)
#   16B8C TurnTowardPoint           5k+1: the forward axis straight up (flat zero, 0x00016C00);
#                                   5k+3: the point on the actor's x/y (0x00016C5C)
#   16048 GetDashPathClearance      4k+1: dest on the actor's x/y (0x00016124); 4k+2: the right
#                                   static ray and the left dynamic one blocked (0.49f off each);
#                                   4k+3: every dynamic ray blocked (the centre one answers 0.0f,
#                                   0x0001638C)
#   1444C IsLineOnMesh              4k+1: the height query fails (0x0001454C); 4k+2: the height
#                                   1e6 (0x00014560); 4k+3: -1e6 (0x00014568)
#   1E4C4 ScoreCoverPoint           7k+1: min range 1e9 (0x0001E594); 7k+2: min -1, max 0
#                                   (0x0001E5A0); 7k+3: cover on the target's x/y (0x0001E5C0);
#                                   7k+4: the target on the actor's x/y (0x0001E61C); 7k+5: the
#                                   forward axis straight up (0x0001E688); 7k+6: the cover on the
#                                   actor's x/y (0x0001E6E4)
#   1DD14 Separate                  3k+1: the repulsion point 100 units off (the separation zero,
#                                   0x0001DEDC); 3k+2: the contact time = now (0x0001DE60)
#   812C StopGuestSfx               every StartPhazonBeamSfx with the sound 1 (the rooftop's is -1, no
#                                   emitter): the second EventStart stops a live one (0x00008158)
#   B458 FindArrivedHint            4k+1: the start uid no object (0x0000B48C); 4k+2: every link no
#                                   object (0x0000B4F4, the loop ends: 0x0000B550); 4k+3: every link
#                                   inactive (0x0000B510)
#   BA38 BoostChainHasInactiveHint  8k+1: no object at uid (0x0000BA80); 8k+2: every link an
#                                   inactive type 0x11 (0x0000BAFC); 8k+3: every link of type 0
#                                   (0x0000BAF0, the 15 steps run out: 0x0000BB54); 8k+4: every link
#                                   an active type 0x11 (0x0000BB18); 8k+6: every link no object
#                                   (0x0000BAE0)
import os
import gdb

CLASS = 'CDarkSamus'
FORCE_RET = []
S = '$this->mPathFindSearch'
SEARCH1 = [(S + '.mResult', '0'), (S + '.mCurWaypoint', '0'), (S + '.mWaypoints.mCount', '1')]
SEARCH3 = [(S + '.mResult', '0'), (S + '.mCurWaypoint', '0'), (S + '.mWaypoints.mCount', '3')]
P = '$this->mPosition'
XF = '$this->mTransform'
ARG = lambda reg: '(*(CVector3f*)%s)' % reg
COVER = '((CScriptCoverPoint*)$rsi)->mPosition'


def onto(dst, src):
    return [(dst + '.mX', src + '.mX'), (dst + '.mY', src + '.mY')]


UP = [(XF + '.m01', '0'), (XF + '.m11', '0')]

# (label, function prefix, period, {phase: [(lvalue, value)]})
ENTRY = [
    ('1E740 search', 'CDarkSamus::ShouldForgetLastAttacker(', 3, {1: SEARCH1, 2: SEARCH3}),
    ('16B8C forward up / point on actor', 'CDarkSamus::TurnTowardPoint(', 5,
     {1: UP, 3: onto(ARG('$rsi'), P)}),
    ('16048 dest on actor', 'CDarkSamus::GetDashPathClearance(', 4, {1: onto(ARG('$rdx'), P)}),
    ('1E4C4 ranges / zero flats', 'CDarkSamus::ScoreCoverPoint(', 7,
     {1: [('$this->mMinAttackRange', '1e9')],
      2: [('$this->mMinAttackRange', '-1'), ('$this->mMaxAttackRange', '0')],
      3: onto(COVER, ARG('$rdx')), 4: onto(ARG('$rdx'), P), 5: UP, 6: onto(COVER, P)}),
    ('1DD14 contact now', 'CDarkSamus::Separate(', 3, {2: [('$this->mContactTime', '$this->mElapsedTime')]}),
    ('812C beam sound', 'CDarkSamus::StartPhazonBeamSfx(', 1, {0: [('$this->mData.mSound', '1')]}),
]

# (label, caller prefix, callee prefix, period, {phase: action}); actions:
#   ('ret', v)            the callee answers v ($rax)
#   ('retn', n, v)        only the callee's n-th call in the caller's call answers v
#   ('outf', reg, v)      the float the callee's pointer/reference argument reg points at becomes v
#   ('far',)              the CVector3f answer (xmm0 x/y, xmm1 z) 100 units off the caller's actor
#   ('obj', [(fmt, v)])   the entity the callee answers gets the fields (fmt % obj), undone at the
#                         caller's return
HINT = '((CScriptAIHint*)%d)'
CALLEE = [
    ('1E740 cover null', 'CDarkSamus::ShouldForgetLastAttacker(', 'CPatterned::GetCoverPoint(', 6,
     {5: ('ret', '0')}),
    ('1DF74 cover null', 'CDarkSamus::TravelToCoverPoints(', 'CPatterned::GetCoverPoint(', 4,
     {3: ('ret', '0')}),
    ('16048 dynamic blocked', 'CDarkSamus::GetDashPathClearance(',
     'CGameCollision::RayDynamicLineOfSightTest(', 4, {2: ('retn', 2, '0'), 3: ('ret', '0')}),
    ('16048 static side blocked', 'CDarkSamus::GetDashPathClearance(',
     'CGameCollision::RayStaticLineOfSightTest(', 4, {2: ('retn', 2, '0')}),
    ('1444C height fail', 'CDarkSamus::IsLineOnMesh(', 'CPathFindSearch::GetHeightOfPointAboveMesh(', 4,
     {1: ('ret', '1'), 2: ('outf', '$rdx', '1e6'), 3: ('outf', '$rdx', '-1e6')}),
    ('1DD14 far point', 'CDarkSamus::Separate(', 'CDarkSamus::GetRepulsionPoint(', 3, {1: ('far',)}),
    ('B458 from null', 'CDarkSamus::FindArrivedHint(', 'CStateManager::GetObjectById(', 4,
     {1: ('ret', '0')}),
    ('B458 links null / inactive', 'CDarkSamus::FindArrivedHint(', 'CStateManager::ObjectById(', 4,
     {2: ('ret', '0'), 3: ('obj', [(HINT + '->mActive', '0')])}),
    ('BA38 from null', 'CDarkSamus::BoostChainHasInactiveHint(', 'CStateManager::GetObjectById(', 8,
     {1: ('ret', '0')}),
    ('BA38 links null / inactive / type', 'CDarkSamus::BoostChainHasInactiveHint(',
     'CStateManager::ObjectById(', 8,
     {2: ('obj', [(HINT + '->mHintType', '17'), (HINT + '->mActive', '0')]),
      3: ('obj', [(HINT + '->mHintType', '0')]), 4: ('obj', [(HINT + '->mHintType', '17')]),
      6: ('ret', '0')}),
]

OUT = os.environ['COV_OUT']
LOGF = os.path.join(OUT, 'ds40.log')
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
    with open(os.path.join(OUT, 'ds40_counts.txt'), 'w') as fh:
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
