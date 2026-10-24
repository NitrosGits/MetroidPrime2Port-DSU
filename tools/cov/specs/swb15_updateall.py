# SwarmBasics (module 80) swb15: fn_80_5024 CSwarmBasics::UpdateAllBoidMovement.
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swb15_updateall.py <out> \
#     42b935e4,01_sand_entrance --con tools/cov/specs/swarmbasics_sand.con --fast --frames 1200
#
# Why this area: the body is the base-class default of a virtual, and every shipped swarm class but
# CFlyerSwarm overrides it (CBacteriaSwarm, CMetareeSwarm, CIngBlobSwarm, CPlantScarabSwarm,
# CEmperorIngLightSwarm all have their own out-of-line bodies), so only a CFlyerSwarm reaches it -
# through Think's virtual call.  Agon 01_sand_entrance has four of them (u041d, u04ff, u05c4, u05c6).
#
# Fixture, per call of the real body (all writes restored at its return with raw readback):
#   ph0  nothing                     -> the natural mAnimated side
#   ph1  mAnimated = 1               -> the loop runs, MoveBoid once per boid
#   ph2  mAnimated = 1, mBoids.mCount = 0 (only lowered, inside the existing allocation)
#                                    -> the loop's zero-boid side
# The MoveBoid oracle (second entry) then checks every call the body makes while the window is open:
# this/mgr identity, boid pointer == &mBoids[i], delta pointer == &mAdvancementDeltas[i & mask]
# with mask = mModelDatas.size() - 1, and dt == the body's own dt.  MoveBoid runs for real - the
# fixture never replaces or skips it.
CLASS = 'CSwarmBasics'

import os

import gdb

_OUT = os.environ.get('COV_OUT', '/tmp')
_LOG = os.path.join(_OUT, 'swb15_geom.log')

_BOID = '((CSwarmBasics::CBoid*)%d)'
_VEC_U = 'rstl::vector<unsigned int, rstl::rmemory_allocator>'
_VEC_CB = 'rstl::vector<CSwarmBasics::CBoid, rstl::rmemory_allocator>'
_VEC_D = 'rstl::vector<CAdvancementDeltas, rstl::rmemory_allocator>'

_st = {'calls': 0, 'apply': 0, 'err': 0, 'window': None, 'mvCalls': 0, 'mvBad': 0,
       'windows': 0, 'loopOk': 0, 'loopBad': 0, 'restoreFail': 0, 'sz': 0, 'vec': 0}


def _log(msg):
    try:
        with open(_LOG, 'a') as fh:
            fh.write(msg + '\n')
    except Exception:
        pass


def _i(g, e):
    return int(g.parse_and_eval(e))


def _f(g, e):
    return float(g.parse_and_eval(e))


def _sizes(g):
    if not _st['sz']:
        _st['sz'] = _i(g, '(long)sizeof(CSwarmBasics::CBoid)')
        # the delta pointer is &mAdvancementDeltas[i & mask] (asm 0x5070 mulli 0x1c = 28), and
        # GetOffsetDelta() is that struct's first member, so the stride is the struct's size
        _st['vec'] = _i(g, '(long)sizeof(CAdvancementDeltas)')
    return _st['sz'], _st['vec']


class _Restore(gdb.FinishBreakpoint):
    """Puts mAnimated and mBoids.mCount back and reads them back."""

    def __init__(self, frame, info):
        super().__init__(frame, internal=True)
        self.info = info

    def stop(self):
        info = self.info
        try:
            if not info['wrote']:
                _log('RESTORE %s NATURAL (nothing forced, nothing written)' % info['tag'])
            else:
                # the ORIGINAL values (animated0 / count0), never the values the fixture wrote
                gdb.execute('set var ((CSwarmBasics*)%d)->mAnimated = %d'
                            % (info['this'], info['animated0']))
                gdb.execute('set var ((%s*)%d)->mCount = %d'
                            % (_VEC_CB, info['boidsVec'], info['count0']))
                anim = _i(gdb, '((CSwarmBasics*)%d)->mAnimated' % info['this'])
                cnt = _i(gdb, '((%s*)%d)->mCount' % (_VEC_CB, info['boidsVec']))
                same = anim == info['animated0'] and cnt == info['count0']
                if not same:
                    _st['restoreFail'] += 1
                _log('RESTORE %s %s mAnimated %d->%d (readback %d) mBoids.mCount %d->%d'
                     ' (readback %d)'
                     % (info['tag'], 'OK' if same else 'MISMATCH', info['animated0'],
                        info['animated'], anim, info['count0'], info['count'], cnt))
            if _st['window'] is not None:
                w = _st['window']
                want = w['expect']
                good = (w['mv'] == want and w['bad'] == 0)
                if good:
                    _st['loopOk'] += 1
                else:
                    _st['loopBad'] += 1
                _log('MOVE-LOOP %s %s calls=%d expected=%d argMismatches=%d'
                     % (w['tag'], 'OK' if good else 'MISMATCH', w['mv'], want, w['bad']))
                _st['window'] = None
        except Exception as e:
            _st['restoreFail'] += 1
            _log('RESTORE %s FAILED: %s' % (info['tag'], e))
        return False


def body_fixture(g):
    st = _st
    st['calls'] += 1
    try:
        this = _i(g, '$rdi')
        mgr = _i(g, '$rsi')
        dt = _f(g, '$xmm0.v4_float[0]')
        vec = _i(g, '&((CSwarmBasics*)%d)->mBoids' % this)
        count = _i(g, '((%s*)%d)->mCount' % (_VEC_CB, vec))
        cap = _i(g, '((%s*)%d)->mCapacity' % (_VEC_CB, vec))
        items = _i(g, '((%s*)%d)->mItems' % (_VEC_CB, vec))
        animated0 = _i(g, '((CSwarmBasics*)%d)->mAnimated' % this)
        nmodels = _i(g, '((%s*)%d)->mCount'
                     % ('rstl::vector<CModelData, rstl::rmemory_allocator>',
                        _i(g, '&((CSwarmBasics*)%d)->mModelDatas' % this)))
        advVec = _i(g, '&((CSwarmBasics*)%d)->mAdvancementDeltas' % this)
        nadv = _i(g, '((%s*)%d)->mCount' % (_VEC_D, advVec))
        advItems = _i(g, '((%s*)%d)->mItems' % (_VEC_D, advVec))
    except Exception as e:
        if st['err'] < 4:
            _log('entry read failed: %s' % e)
        st['err'] += 1
        return
    # the shipped CFlyerSwarms are animated with 5-7 boids, so the false side must be forced:
    #   ph0 mAnimated = 0 (the early-return side), ph1 mAnimated = 1 with its boids (the loop),
    #   ph2 mAnimated = 1 with mBoids.mCount lowered to 0 (the zero-boid side)
    phase = st['calls'] % 3
    target_anim = 0 if phase == 0 else 1
    target_count = 0 if phase == 2 else count
    wrote = not (target_anim == animated0 and target_count == count)
    if target_anim and target_count > 0:
        if nmodels < 1 or nadv < nmodels or not items or not advItems:
            if st['err'] < 6:
                _log('cannot run the animated loop safely: models=%d adv=%d items=0x%x advItems=0x%x'
                     % (nmodels, nadv, items, advItems))
            st['err'] += 1
            return
    if target_count > cap:
        _log('refusing: target count %d above capacity %d' % (target_count, cap))
        return
    tag = 'swb15-%d-ph%d' % (st['apply'] + 1, phase)
    info = {'this': this, 'boidsVec': vec, 'count': target_count, 'count0': count,
            'animated': target_anim, 'animated0': animated0, 'tag': tag, 'wrote': wrote}
    try:
        _Restore(g.newest_frame(), info)
    except Exception as e:
        _log('%s: restore could not be armed (%s) - no writes' % (tag, e))
        return
    try:
        if wrote:
            g.execute('set var ((CSwarmBasics*)%d)->mAnimated = %d' % (this, target_anim))
            g.execute('set var ((%s*)%d)->mCount = %d' % (_VEC_CB, vec, target_count))
    except Exception as e:
        _log('%s write failed: %s' % (tag, e))
        return
    if wrote:
        st['apply'] += 1
    mask = nmodels - 1 if nmodels > 0 else 0
    st['window'] = {'tag': tag, 'this': this, 'mgr': mgr, 'items': items, 'advItems': advItems,
                    'mask': mask, 'dt': dt, 'expect': target_count if target_anim else 0,
                    'mv': 0, 'bad': 0, 'n': 0}
    st['windows'] += 1
    _log('%s %s swarm=0x%x mAnimated %d->%d mBoids.mCount %d->%d (cap %d) models=%d adv=%d'
         ' mask=%d dt=%.6f expectMoveBoid=%d'
         % (tag, 'FORCED' if wrote else 'NATURAL', this, animated0, target_anim, count,
            target_count, cap, nmodels, nadv, mask, dt, st['window']['expect']))


def moveboid_oracle(g):
    """Check the MoveBoid call the body just made: identity, boid, delta index and dt."""
    st = _st
    w = st['window']
    if w is None:
        return
    try:
        this = _i(g, '$rdi')
        mgr = _i(g, '$rsi')
        boid = _i(g, '$rdx')
        delta = _i(g, '$rcx')
        dt = _f(g, '$xmm0.v4_float[0]')
        szb, szv = _sizes(g)
    except Exception as e:
        w['bad'] += 1
        if w['n'] < 3:
            _log('MOVE-ARG read failed: %s' % e)
        return
    i = w['n']
    want_boid = w['items'] + i * szb
    want_delta = w['advItems'] + (i & w['mask']) * szv
    ok = (this == w['this'] and (not w['mgr'] or mgr == w['mgr']) and boid == want_boid
          and delta == want_delta and dt == w['dt'])
    if not ok:
        w['bad'] += 1
    w['n'] += 1
    w['mv'] += 1
    st['mvCalls'] += 1
    if not ok:
        st['mvBad'] += 1
    if w['n'] <= 3 or not ok:
        _log('MOVE-ARG %s i=%d %s this=0x%x boid=0x%x (want 0x%x) delta=0x%x (want 0x%x) dt=%.6f'
             ' (want %.6f)'
             % (w['tag'], i, 'OK' if ok else 'MISMATCH', this, boid, want_boid, delta, want_delta,
                dt, w['dt']))


def _summary(event=None):
    st = _st
    _log('SUMMARY exit: calls=%d applies=%d windows=%d moveBoidCalls=%d moveBoidArgMismatch=%d'
         ' loopWindowsOk=%d loopWindowsBad=%d restoreFail=%d err=%d'
         % (st['calls'], st['apply'], st['windows'], st['mvCalls'], st['mvBad'], st['loopOk'],
            st['loopBad'], st['restoreFail'], st['err']))


try:
    gdb.events.exited.connect(_summary)
except Exception:
    pass


FORCE = [
    ('5024 body', 'CSwarmBasics::UpdateAllBoidMovement(', [], [body_fixture], 0,
     'ph0 natural mAnimated, ph1 mAnimated = 1 (the loop runs), ph2 mAnimated = 1 with mBoids.mCount'
     ' lowered to 0 inside the existing allocation (the zero-boid side); both restored at the'
     " body's return with readback"),
    ('5024 MoveBoid args', 'CSwarmBasics::MoveBoid(', [], [moveboid_oracle], 0,
     'checks every MoveBoid the body makes while its window is open: this/mgr, boid pointer,'
     ' mAdvancementDeltas[i & mask] and dt; MoveBoid itself runs untouched'),
]
