# SwarmBasics (module 80) swb15 (hardened): fn_80_5024 CSwarmBasics::UpdateAllBoidMovement.
#
# NOT RUNTIME-TESTED. The swb15 evidence run used the earlier singleton-window version of this
# fixture, whose raw log could not account for all windows (see docs/module-analysis/SwarmBasics.md,
# "swb15"): 4325 opens, 4324 closes, one tag (swb15-2861-ph2) opened and restored but never closed,
# 33 opens that landed while another window was open, and one window (swb15-2819-ph1) that counted 8
# MoveBoid calls for 7 boids with i=6 repeated. This version removes every mechanism that could
# produce those artefacts; it has only been exercised by the mock suite in
# /tmp/opencode/swb15-reconcile/mock/.
#
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swb15_updateall.py <out> \
#     42b935e4,01_sand_entrance --con tools/cov/specs/swarmbasics_sand.con --fast --frames 1200
#
# Per-call ownership: every body entry gets its own window record, keyed by the gdb frame it was
# armed on, and its own restore/close breakpoint bound to that record - there is no shared "current
# window" any callback can overwrite. The MoveBoid oracle attributes a hit only to the innermost open
# window whose `this` matches the incoming $rdi, and anything else (no match, several matches, a hit
# outside any window) is counted and logged as unattributed instead of being folded into a window.
# Closing out of order, a duplicate entry for the same frame, an unwind past a window and a window
# still open at exit are all counted and make the exit status FAIL.
#
# Phases (the shipped CFlyerSwarms are animated with 5-7 boids, so the false side has to be forced):
#   ph0 mAnimated = 0   -> the early-return side
#   ph1 mAnimated = 1   -> the loop, MoveBoid once per boid
#   ph2 mAnimated = 1 with mBoids.mCount lowered to 0 (inside the existing allocation) -> zero boids
# Every write is restored at the body's return from the values read before it, then read back.
CLASS = 'CSwarmBasics'

import os

import gdb

_OUT = os.environ.get('COV_OUT', '/tmp')
_LOG = os.path.join(_OUT, 'swb15_geom.log')

_VEC_CB = 'rstl::vector<CSwarmBasics::CBoid, rstl::rmemory_allocator>'
_VEC_D = 'rstl::vector<CAdvancementDeltas, rstl::rmemory_allocator>'

_st = {
    'calls': 0, 'apply': 0, 'open': {}, 'order': [], 'opens': 0, 'closed': 0, 'unwound': 0,
    'pending': 0, 'dupEntry': 0, 'unattributed': 0, 'mvHits': 0, 'restoreFail': 0,
    'noRestore': 0, 'refused': 0, 'err': 0, 'sz': 0, 'adv': 0, 'status': [],
}


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
        _st['adv'] = _i(g, '(long)sizeof(CAdvancementDeltas)')
    return _st['sz'], _st['adv']


class _Restore(gdb.FinishBreakpoint):
    """Bound to ONE window record: restores that window's writes and closes only that window."""

    def __init__(self, frame, win):
        super().__init__(frame, internal=True)
        self.win = win

    def _close(self, how):
        w = self.win
        _st['open'].pop(id(w), None)
        if w['closed']:
            return
        w['closed'] = True
        if how == 'return':
            _st['closed'] += 1
        else:
            _st['unwound'] += 1
            _st['status'].append('unwind:' + w['tag'])
        # out-of-order close: another window opened after this one and is still open
        if _st['order'] and _st['order'][-1] is not w:
            _st['status'].append('order:' + w['tag'])
            _log('WINDOW-ORDER %s closed while %s was still open'
                 % (w['tag'], _st['order'][-1]['tag']))
        if _st['order'] and _st['order'][-1] is w:
            _st['order'].pop()
        elif w in _st['order']:
            _st['order'].remove(w)

    def stop(self):
        w = self.win
        try:
            if w['wrote']:
                # the values read before the write, never the ones the fixture wrote
                gdb.execute('set var ((CSwarmBasics*)%d)->mAnimated = %d'
                            % (w['this'], w['animated0']))
                gdb.execute('set var ((%s*)%d)->mCount = %d'
                            % (_VEC_CB, w['boidsVec'], w['count0']))
                anim = _i(gdb, '((CSwarmBasics*)%d)->mAnimated' % w['this'])
                cnt = _i(gdb, '((%s*)%d)->mCount' % (_VEC_CB, w['boidsVec']))
                same = anim == w['animated0'] and cnt == w['count0']
                if not same:
                    _st['restoreFail'] += 1
                    _st['status'].append('restore:' + w['tag'])
                _log('RESTORE %s %s mAnimated %d->%d (readback %d) mBoids.mCount %d->%d'
                     ' (readback %d)'
                     % (w['tag'], 'OK' if same else 'MISMATCH', w['animated0'], w['animated'],
                        anim, w['count0'], w['count'], cnt))
            else:
                _log('RESTORE %s NATURAL (nothing forced, nothing written)' % w['tag'])
            good = (w['mv'] == w['expect'] and w['bad'] == 0)
            if not good:
                _st['status'].append('loop:' + w['tag'])
            _log('MOVE-LOOP %s %s calls=%d expected=%d argMismatches=%d'
                 % (w['tag'], 'OK' if good else 'MISMATCH', w['mv'], w['expect'], w['bad']))
        except Exception as e:
            _st['restoreFail'] += 1
            _st['status'].append('restore-exc:' + w['tag'])
            _log('RESTORE %s FAILED: %s' % (w['tag'], e))
        self._close('return')
        return False

    def out_of_scope(self):
        _st['status'].append('out-of-scope:' + self.win['tag'])
        _log('WINDOW-UNWOUND %s: the frame was discarded without a normal return' % self.win['tag'])
        self._close('unwind')


def body_fixture(g):
    st = _st
    st['calls'] += 1
    try:
        frame = g.newest_frame()
        this = _i(g, '$rdi')
        mgr = _i(g, '$rsi')
        dt = _f(g, '$xmm0.v4_float[0]')
        vec = _i(g, '&((CSwarmBasics*)%d)->mBoids' % this)
        count = _i(g, '((%s*)%d)->mCount' % (_VEC_CB, vec))
        cap = _i(g, '((%s*)%d)->mCapacity' % (_VEC_CB, vec))
        items = _i(g, '((%s*)%d)->mItems' % (_VEC_CB, vec))
        animated0 = _i(g, '((CSwarmBasics*)%d)->mAnimated' % this)
        mvec = _i(g, '&((CSwarmBasics*)%d)->mModelDatas' % this)
        nmodels = _i(g, '((rstl::vector<CModelData, rstl::rmemory_allocator>*)%d)->mCount' % mvec)
        advVec = _i(g, '&((CSwarmBasics*)%d)->mAdvancementDeltas' % this)
        nadv = _i(g, '((%s*)%d)->mCount' % (_VEC_D, advVec))
        advItems = _i(g, '((%s*)%d)->mItems' % (_VEC_D, advVec))
    except Exception as e:
        if st['err'] < 4:
            _log('entry read failed: %s' % e)
        st['err'] += 1
        return
    phase = st['calls'] % 3
    target_anim = 0 if phase == 0 else 1
    target_count = 0 if phase == 2 else count
    wrote = not (target_anim == animated0 and target_count == count)
    if wrote and target_anim and target_count > 0:
        if nmodels < 1 or nadv < nmodels or not items or not advItems:
            st['refused'] += 1
            st['status'].append('refused-models')
            if st['refused'] <= 3:
                _log('refused: models=%d adv=%d items=0x%x advItems=0x%x' % (nmodels, nadv, items,
                                                                            advItems))
            return
    if target_count > cap:
        st['refused'] += 1
        st['status'].append('refused-cap')
        _log('refused: target count %d above capacity %d' % (target_count, cap))
        return
    tag = 'swb15-%d-ph%d' % (st['calls'], phase)
    win = {'tag': tag, 'this': this, 'mgr': mgr, 'frame': frame, 'vec': vec, 'items': items,
           'advItems': advItems, 'mask': nmodels - 1 if nmodels > 0 else 0, 'dt': dt,
           'expect': target_count if target_anim else 0, 'mv': 0, 'bad': 0, 'wrote': wrote,
           'count0': count, 'animated0': animated0, 'count': target_count,
           'animated': target_anim, 'closed': False, 'writes': []}
    if any(w['this'] == this and w['frame'] == frame for w in st['open'].values()):
        st['dupEntry'] += 1
        st['status'].append('dup-entry:' + tag)
        _log('DUPLICATE-ENTRY %s for swarm=0x%x on the same frame' % (tag, this))
        return
    try:
        _Restore(frame, win)
    except Exception as e:
        st['noRestore'] += 1
        st['status'].append('no-restore:' + tag)
        _log('%s: restore could not be armed (%s) - no writes' % (tag, e))
        return
    st['open'][id(win)] = win
    st['order'].append(win)
    st['opens'] += 1
    if wrote:
        try:
            g.execute('set var ((CSwarmBasics*)%d)->mAnimated = %d' % (this, target_anim))
            win['writes'].append(('anim', target_anim))
            g.execute('set var ((%s*)%d)->mCount = %d' % (_VEC_CB, vec, target_count))
            win['writes'].append(('count', target_count))
        except Exception as e:
            # a partial write must be undone here and now
            try:
                g.execute('set var ((CSwarmBasics*)%d)->mAnimated = %d' % (this, animated0))
                g.execute('set var ((%s*)%d)->mCount = %d' % (_VEC_CB, vec, count))
            except Exception:
                pass
            st['err'] += 1
            st['status'].append('write-failed:' + tag)
            _log('%s write failed after %d field(s): %s - rolled back' % (tag, len(win['writes']),
                                                                         e))
            return
        st['apply'] += 1
    _log('%s %s swarm=0x%x mAnimated %d->%d mBoids.mCount %d->%d (cap %d) models=%d adv=%d mask=%d'
         ' dt=%.6f expectMoveBoid=%d'
         % (tag, 'FORCED' if wrote else 'NATURAL', this, animated0, target_anim, count,
            target_count, cap, nmodels, nadv, win['mask'], dt, win['expect']))


def moveboid_oracle(g):
    st = _st
    try:
        this = _i(g, '$rdi')
        mgr = _i(g, '$rsi')
        boid = _i(g, '$rdx')
        delta = _i(g, '$rcx')
        dt = _f(g, '$xmm0.v4_float[0]')
        szb, sza = _sizes(g)
    except Exception as e:
        st['unattributed'] += 1
        st['status'].append('move-arg-unreadable')
        return
    # ownership: the innermost open window for this caller; anything else is not folded into a window
    mine = [w for w in st['order'] if w['this'] == this and not w['closed']]
    if len(mine) != 1:
        st['unattributed'] += 1
        st['status'].append('move-unattributed' if not mine else 'move-ambiguous')
        if st['unattributed'] <= 5:
            _log('MOVE-ARG UNATTRIBUTED this=0x%x boid=0x%x open=%d (%s)'
                 % (this, boid, len(mine), ','.join(w['tag'] for w in mine)))
        return
    w = mine[0]
    i = w['mv']
    want_boid = w['items'] + i * szb
    want_delta = w['advItems'] + (i & w['mask']) * sza
    ok = (mgr == w['mgr'] and boid == want_boid and delta == want_delta and dt == w['dt'])
    w['mv'] += 1
    st['mvHits'] += 1
    if not ok:
        w['bad'] += 1
    if w['mv'] <= 3 or not ok:
        _log('MOVE-ARG %s i=%d %s this=0x%x boid=0x%x (want 0x%x) delta=0x%x (want 0x%x) dt=%.6f'
             ' (want %.6f)'
             % (w['tag'], i, 'OK' if ok else 'MISMATCH', this, boid, want_boid, delta, want_delta,
                dt, w['dt']))


def _summary(event=None):
    st = _st
    for w in list(st['order']):
        st['pending'] += 1
        st['status'].append('pending:' + w['tag'])
        _log('WINDOW-PENDING %s at exit (never closed)' % w['tag'])
    reasons = {}
    for s in st['status']:
        reasons[s.split(':', 1)[0]] = reasons.get(s.split(':', 1)[0], 0) + 1
    ok = (st['pending'] == 0 and st['dupEntry'] == 0 and st['unattributed'] == 0
          and st['restoreFail'] == 0 and st['noRestore'] == 0 and st['err'] == 0
          and st['refused'] == 0 and not any(s.startswith(('order:', 'loop:', 'out-of-scope:',
                                                           'restore:')) for s in st['status']))
    _log('SUMMARY exit: calls=%d opens=%d closed=%d unwound=%d pending=%d dupEntry=%d'
         ' unattributed=%d moveBoidHits=%d restoreFail=%d noRestore=%d refused=%d err=%d'
         ' status=%s reasons=%s'
         % (st['calls'], st['opens'], st['closed'], st['unwound'], st['pending'], st['dupEntry'],
            st['unattributed'], st['mvHits'], st['restoreFail'], st['noRestore'], st['refused'],
            st['err'], 'OK' if ok else 'FAIL', reasons or '{}'))


try:
    gdb.events.exited.connect(_summary)
except Exception:
    pass


FORCE = [
    ('5024 body', 'CSwarmBasics::UpdateAllBoidMovement(', [], [body_fixture], 0,
     'per-call window: ph0 mAnimated = 0, ph1 mAnimated = 1 (the loop), ph2 mAnimated = 1 with'
     ' mBoids.mCount lowered to 0; each restored at its own return with readback'),
    ('5024 MoveBoid args', 'CSwarmBasics::MoveBoid(', [], [moveboid_oracle], 0,
     'attributes every MoveBoid hit to the one open window for that caller and checks boid,'
     ' mAdvancementDeltas[i & mask] and dt; MoveBoid runs untouched'),
]
