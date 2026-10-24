"""Offline regression tests for tools/cov/specs/swb15_updateall.py (hardened swb15 fixture).

Runs the fixture against a stub `gdb` with a modelled stack (frames with level + name) and modelled
swarm memory, and asserts BOTH the final counters/status AND the restored memory.  No gdb, no game.

    python3 tools/cov/specs/swb15_updateall_selftest.py [--keep]

Exit code 0 means every check passed.  The log is written to $COV_OUT (default: a temp dir).
"""
import os
import sys
import types

OUT = os.environ.get('COV_OUT', '/tmp/opencode/swb15-selftest')
os.makedirs(OUT, exist_ok=True)
LOG = os.path.join(OUT, 'swb15_geom.log')
if os.path.exists(LOG):
    os.remove(LOG)

BODY = 'CSwarmBasics::UpdateAllBoidMovement(CStateManager&, float)'
THINK = 'CSwarmBasics::Think(float, CStateManager&)'
MOVE = 'CSwarmBasics::MoveBoid(CStateManager&, CSwarmBasics::CBoid&, CVector3f const&, float)'

B, MD, AD, BI, DI = 0x9000, 0x9100, 0x9200, 0x9500, 0x9600
MEM = {}            # (swarm, field) -> value ; field in {'animated', 'count'}
MODEL = {'cap': 50, 'models': 4, 'adv': 4}
REGS = {}
STACK = {'top': None}
WRITES = []
FAIL_WRITE_AT = {'n': None}
ARM_FAIL = {'value': False}


class Frame:
    def __init__(self, level, name, older=None):
        self._level, self._name, self._older = level, name, older

    def level(self):
        return self._level

    def name(self):
        return self._name

    def older(self):
        if self._older is None:
            raise RuntimeError('no older frame')
        return self._older


def body_frame(level=1):
    return Frame(level, BODY, Frame(level + 1, THINK))


def move_frame(level=1):
    """A MoveBoid frame whose immediate caller is the body frame at `level`."""
    return Frame(level - 1, MOVE, body_frame(level))


class BP:
    def __init__(self, *a, **k):
        self.enabled = True


class FB(BP):
    made = []

    def __init__(self, fr=None, internal=True, win=None, **k):
        if ARM_FAIL['value']:
            raise RuntimeError('cannot arm the finish breakpoint')
        super().__init__()
        self.frame, self.win = fr, win
        FB.made.append(self)


class Ev:
    def __init__(self):
        self.handlers = []

    def connect(self, f):
        self.handlers.append(f)


def _mem(swarm, field, default):
    return MEM.get((swarm, field), default)


def ev(e):
    e = e.strip()
    if e in ('$rdi', '$rsi', '$rdx', '$rcx'):
        return REGS[e]
    if e == '$xmm0.v4_float[0]':
        return 1.0 / 60.0
    if e == '(long)sizeof(CSwarmBasics::CBoid)':
        return 192
    if e == '(long)sizeof(CAdvancementDeltas)':
        return 28
    if e.startswith('&((CSwarmBasics*)') and 'mBoids' in e:
        return B
    if e.startswith('&((CSwarmBasics*)') and 'mModelDatas' in e:
        return MD
    if e.startswith('&((CSwarmBasics*)') and 'mAdvancementDeltas' in e:
        return AD
    if '->mAnimated' in e:
        return _mem(REGS['$rdi'], 'animated', 0)
    if ('*)%d)->mCount' % B) in e:
        return _mem(REGS['$rdi'], 'count', 0)
    if ('*)%d)->mCapacity' % B) in e:
        return MODEL['cap']
    if ('*)%d)->mItems' % B) in e:
        return BI
    if ('*)%d)->mCount' % MD) in e:
        return MODEL['models']
    if ('*)%d)->mCount' % AD) in e:
        return MODEL['adv']
    if ('*)%d)->mItems' % AD) in e:
        return DI
    raise KeyError(e)


def ex(eq):
    WRITES.append(eq)
    if FAIL_WRITE_AT['n'] is not None and len(WRITES) == FAIL_WRITE_AT['n']:
        raise RuntimeError('simulated write failure')
    swarm = REGS['$rdi']
    if '->mAnimated =' in eq:
        MEM[(swarm, 'animated')] = int(eq.rsplit('=', 1)[1])
    if ('*)%d)->mCount =' % B) in eq:
        MEM[(swarm, 'count')] = int(eq.rsplit('=', 1)[1])


g = types.ModuleType('gdb')
g.Breakpoint = BP
g.FinishBreakpoint = FB
g.error = Exception
g.parse_and_eval = ev
g.execute = ex
g.newest_frame = lambda: STACK['top']
g.events = types.SimpleNamespace(exited=Ev())
sys.modules['gdb'] = g

ns = {'__name__': 'spec'}
exec(compile(open(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                               'swb15_updateall.py')).read(), 'swb15_updateall.py', 'exec'), ns)

FAILED = []


def check(name, cond, detail=''):
    print('%-52s %s %s' % (name, 'PASS' if cond else 'FAIL', detail))
    if not cond:
        FAILED.append(name)


def reset():
    MEM.clear()
    REGS.clear()
    WRITES.clear()
    FAIL_WRITE_AT['n'] = None
    ARM_FAIL['value'] = False
    FB.made = []
    STACK['top'] = None
    ns['_st'].update({'calls': 0, 'apply': 0, 'open': {}, 'order': [], 'opens': 0, 'closed': 0,
                      'unwound': 0, 'pending': 0, 'dupEntry': 0, 'unattributed': 0, 'mvHits': 0,
                      'restoreFail': 0, 'noRestore': 0, 'refused': 0, 'err': 0, 'status': [],
                      'rollbackFail': 0})


def enter(swarm, count, animated, level=1):
    MEM[(swarm, 'count')] = count
    MEM[(swarm, 'animated')] = animated
    REGS.update({'$rdi': swarm, '$rsi': 0x2000})
    STACK['top'] = body_frame(level)
    ns['body_fixture'](g)
    return ns['_st']['order'][-1] if ns['_st']['order'] else None


def hit(swarm, i, level=1, caller=None):
    REGS.update({'$rdi': swarm, '$rsi': 0x2000, '$rdx': BI + i * 192,
                 '$rcx': DI + (i & (MODEL['models'] - 1)) * 28})
    STACK['top'] = caller if caller is not None else move_frame(level)
    ns['moveboid_oracle'](g)


def finish_all():
    for bp in list(FB.made):
        bp.stop()


# ---------------------------------------------------------------- phase 0: forced mAnimated = 0
reset()
ns['_st']['calls'] = 2                                    # next call is phase 0
enter(0x1000, count=5, animated=1)
check('ph0 wrote mAnimated = 0', MEM[(0x1000, 'animated')] == 0)
finish_all()
check('ph0 restored the original memory',
      MEM[(0x1000, 'animated')] == 1 and MEM[(0x1000, 'count')] == 5,
      'anim=%d count=%d' % (MEM[(0x1000, 'animated')], MEM[(0x1000, 'count')]))
check('ph0 counters: 1 open, 1 closed, no failures',
      ns['_st']['opens'] == 1 and ns['_st']['closed'] == 1 and ns['_st']['restoreFail'] == 0
      and ns['_st']['unattributed'] == 0 and ns['_st']['err'] == 0)
check('ph0 raw readback logged', 'RESTORE swb15-3-ph0 OK' in open(LOG).read())

# ---------------------------------------------------------------- phase 1: the loop, 5 boids
reset()
ns['_st']['calls'] = 0                                    # next call is phase 1
enter(0x1000, count=5, animated=1)
for i in range(5):
    hit(0x1000, i)
finish_all()
check('ph1 attributed 5 MoveBoid calls', ns['_st']['mvHits'] == 5
      and ns['_st']['unattributed'] == 0)
check('ph1 memory untouched', MEM[(0x1000, 'animated')] == 1 and MEM[(0x1000, 'count')] == 5)
check('ph1 counters', ns['_st']['opens'] == 1 and ns['_st']['closed'] == 1
      and ns['_st']['restoreFail'] == 0)

# ---------------------------------------------------------------- phase 2: forced count 0
reset()
ns['_st']['calls'] = 1                                    # next call is phase 2
enter(0x1000, count=7, animated=1)
check('ph2 wrote mBoids.mCount = 0', MEM[(0x1000, 'count')] == 0)
finish_all()
check('ph2 restored the original count', MEM[(0x1000, 'count')] == 7
      and MEM[(0x1000, 'animated')] == 1)
check('ph2 zero-boid window is OK', ns['_st']['opens'] == 1 and ns['_st']['closed'] == 1
      and not any(s.startswith('loop:') for s in ns['_st']['status']))

# ---------------------------------------------------------------- same this, wrong caller frame
reset()
enter(0x1000, count=5, animated=1)
hit(0x1000, 0, caller=Frame(0, MOVE, Frame(9, THINK)))     # caller frame is Think, not the body
check('same-this wrong caller rejected', ns['_st']['unattributed'] == 1
      and ns['_st']['mvHits'] == 0 and 'move-no-owner' in ns['_st']['status'])
check('wrong caller logged with frame identity',
      'caller=(level 9, CSwarmBasics::Think' in open(LOG).read())
finish_all()

# ---------------------------------------------------------------- unreadable / missing caller frame
reset()
enter(0x1000, count=5, animated=1)
hit(0x1000, 0, caller=Frame(0, MOVE, None))                # older() raises
check('missing caller frame rejected', ns['_st']['unattributed'] == 1
      and 'caller-frame-unreadable' in ns['_st']['status'])
finish_all()

# ---------------------------------------------------------------- nested distinct owners
reset()
outer = enter(0x1000, count=5, animated=1, level=1)
inner = enter(0x2000, count=3, animated=1, level=3)
hit(0x1000, 0, level=1)
hit(0x2000, 0, level=3)
hit(0x2000, 1, level=3)
hit(0x1000, 1, level=1)
check('nested owners attributed separately', ns['_st']['mvHits'] == 4
      and ns['_st']['unattributed'] == 0 and outer['mv'] == 2 and inner['mv'] == 2)
# the outer window closes first: out-of-order, and it must be reported
FB.made[0].stop()
check('out-of-order close reported', 'order:swb15-1-ph1' in ns['_st']['status'])
FB.made[1].stop()

# ---------------------------------------------------------------- rollback that fails honestly
reset()
ns['_st']['calls'] = 2                                     # phase 0 forces a write
MEM[(0x1000, 'animated')] = 1
MEM[(0x1000, 'count')] = 5
REGS.update({'$rdi': 0x1000, '$rsi': 0x2000})
STACK['top'] = body_frame(1)
# write 1 (mAnimated = 0) lands, write 2 (the count) raises, and the rollback writes raise too:
# that leaves the memory NOT restored, which is exactly what has to be reported honestly
orig_ex = ex
CALLS = {'n': 0}


def ex_partial(eq):
    CALLS['n'] += 1
    if CALLS['n'] == 2:
        raise RuntimeError('count write failed')
    if CALLS['n'] > 2:
        raise RuntimeError('rollback write failed')
    WRITES.append(eq)
    orig_ex(eq)


g.execute = ex_partial
ns['body_fixture'](g)
g.execute = orig_ex
log = open(LOG).read()
check('failed write reported, not claimed rolled back',
      'rollback FAILED' in log and 'THE ORIGINAL STATE IS NOT RESTORED' in log
      and 'rollback-failed:' in ' '.join(ns['_st']['status'])
      and ns['_st']['rollbackFail'] == 1,
      'memory anim=%s (original 0 was written, rollback failed)'
      % MEM.get((0x1000, 'animated')))
check('failed write counted as an error', ns['_st']['err'] == 1)
for bp in list(FB.made):
    bp.stop()

# ---------------------------------------------------------------- missing return breakpoint
reset()
ns['_st']['calls'] = 2
MEM[(0x1000, 'animated')] = 1
MEM[(0x1000, 'count')] = 5
REGS.update({'$rdi': 0x1000, '$rsi': 0x2000})
STACK['top'] = body_frame(1)
ARM_FAIL['value'] = True
ns['body_fixture'](g)
check('missing return breakpoint -> no write', not WRITES
      and MEM[(0x1000, 'animated')] == 1 and MEM[(0x1000, 'count')] == 5
      and ns['_st']['noRestore'] == 1)
ARM_FAIL['value'] = False

# ---------------------------------------------------------------- summary status
reset()
ns['_summary']()
line = open(LOG).read().splitlines()[-1]
check('clean exit reports status=OK', 'status=OK' in line, line[-60:])
reset()
ns['_st']['calls'] = 1
enter(0x1000, count=5, animated=1)
ns['_summary']()
line = open(LOG).read().splitlines()[-1]
check('pending window fails the exit status',
      'status=FAIL' in line and "'pending': 1" in line)

print()
print('SELFTEST:', 'PASS' if not FAILED else 'FAIL: ' + ', '.join(FAILED))
sys.exit(0 if not FAILED else 1)
