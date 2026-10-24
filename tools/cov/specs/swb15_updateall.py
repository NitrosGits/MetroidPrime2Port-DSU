# SwarmBasics swb15: OFFLINE-CORRECTED, NOT RUNTIME-VALIDATED (2/2 boots exhausted).
# Historical 4325 opens / 4324 closes / 33 overlaps remain unresolved. This fixture
# does not retroactively validate that run or justify fn_80_5024's marker.
#
# Frame.level() is a changing stack depth, NOT an activation identity. Retain the
# real gdb.Frame and compare it with the immediate MoveBoid caller using GDB frame
# equality, guarded by validity, name and inferior/thread identity. Never match by
# this/name/depth alone. Fresh Python wrappers for the same GDB frame compare equal.
# Each window owns its FinishBreakpoint and pre-write snapshots. Duplicate/late
# callbacks cannot replay writes; abnormal close cancels younger windows in LIFO
# order before restoring an older snapshot. Any uncertainty is a failed summary.
CLASS = 'CSwarmBasics'

import os
import gdb

_OUT = os.environ.get('COV_OUT', '/tmp/opencode')
_LOG = os.path.join(_OUT, 'swb15_geom.log')
_BODY = 'CSwarmBasics::UpdateAllBoidMovement'
_MOVE = 'CSwarmBasics::MoveBoid'
_VEC_CB = 'rstl::vector<CSwarmBasics::CBoid, rstl::rmemory_allocator>'
_VEC_D = 'rstl::vector<CAdvancementDeltas, rstl::rmemory_allocator>'
_st = dict(calls=0, apply=0, open={}, order=[], opens=0, closed=0, unwound=0,
           pending=0, dupEntry=0, unattributed=0, mvHits=0, restoreFail=0,
           rollbackFail=0, noRestore=0, refused=0, err=0, sz=0, adv=0,
           ownershipFail=0, orderFail=0, lateFinish=0, poisoned=False, status=[])


def _log(msg):
    try:
        with open(_LOG, 'a') as fh:
            fh.write(msg + '\n')
    except Exception:
        # Missing evidence is a failure, not a successful silent run.
        _st['err'] += 1
        _fail('log-error')


def _fail(reason):
    if reason not in _st['status']:
        _st['status'].append(reason)


def _i(g, expression):
    return int(g.parse_and_eval(expression))


def _f(g, expression):
    return float(g.parse_and_eval(expression))


def _context(g):
    thread = g.selected_thread()
    inferior = g.selected_inferior()
    if thread is None or inferior is None or thread.global_num is None:
        raise RuntimeError('missing inferior/thread')
    return int(inferior.num), int(thread.global_num)


def _frame_info(frame):
    if frame is None or not frame.is_valid():
        raise RuntimeError('missing/invalid frame')
    name, level = frame.name(), frame.level()
    if not name or level is None or level < 0:
        raise RuntimeError('unreadable frame name/level')
    return name, level  # level is diagnostic only, never compared for ownership


def _same_frame(a, b):
    _frame_info(a)
    _frame_info(b)
    return bool(a == b)  # GDB activation identity, not Python `is` or frame depth


def _fields(w):
    return [('anim', '((CSwarmBasics*)%d)->mAnimated' % w['this'], w['animated0']),
            ('count', '((%s*)%d)->mCount' % (_VEC_CB, w['vec']), w['count0'])]


def _restore(w, counter, label):
    if not w['needs_restore']:
        _log('RESTORE %s %s nothing written/already restored' % (w['tag'], label))
        return True
    errors, actual = [], {}
    try:
        if _context(gdb) != w['context']:
            raise RuntimeError('wrong inferior/thread: no unsafe restore writes')
    except Exception as error:
        errors.append(str(error))
    if not errors:
        # Each field is attempted independently, even if a preceding write raises
        # after changing memory. Read both fields independently as well.
        for field, expression, original in _fields(w):
            try:
                gdb.execute('set var %s = %d' % (expression, original))
            except Exception as error:
                errors.append('%s write: %s' % (field, error))
        for field, expression, original in _fields(w):
            try:
                actual[field] = _i(gdb, expression)
                if actual[field] != original:
                    errors.append(field + ' readback mismatch')
            except Exception as error:
                errors.append('%s read: %s' % (field, error))
    good = not errors
    w['needs_restore'] = not good
    if not good:
        _st[counter] += 1
        _st['poisoned'] = True
        _fail(label + '-failed:' + w['tag'])
    _log('RESTORE %s %s %s originals=%s readback=%s errors=%s'
         % (w['tag'], label, 'OK' if good else 'FAILED',
            dict((f, v) for f, e, v in _fields(w)), actual, errors))
    return good


def _settle(w, how):
    w['closed'] = True
    _st['open'].pop(id(w), None)
    _st['order'][:] = [other for other in _st['order'] if other is not w]
    _st['closed' if how == 'return' else 'unwound'] += 1
    if how != 'return':
        _fail(how + ':' + w['tag'])
    try:
        w['breakpoint'].enabled = False
    except Exception as error:
        _st['err'] += 1
        _fail('disable-failed:' + w['tag'])
        _log('FINISH-DISABLE %s FAILED %s' % (w['tag'], error))


class _Restore(gdb.FinishBreakpoint):
    def __init__(self, frame, win):
        super().__init__(frame, internal=True)
        self.win = win

    def _finish(self, how):
        w = self.win
        if w['closed']:
            _st['lateFinish'] += 1
            _fail('late-finish:' + w['tag'])
            _log('LATE-FINISH %s %s ignored, no writes' % (w['tag'], how))
            return False
        positions = [i for i, other in enumerate(_st['order']) if other is w]
        if len(positions) != 1:
            # Corrupt/ambiguous bookkeeping gives no safe snapshot ordering.
            # Do not guess, replay a potentially stale snapshot, or claim restore.
            _st['orderFail'] += 1
            _st['restoreFail'] += 1
            _st['poisoned'] = True
            _fail('ambiguous-finish:' + w['tag'])
            _log('FINISH-FAILED %s ambiguous ordering, no unsafe writes' % w['tag'])
            _settle(w, 'cancelled')
            return False
        ordered = len(positions) == 1 and positions[0] == len(_st['order']) - 1
        if not ordered:
            _st['orderFail'] += 1
            _fail('order:' + w['tag'])
            _log('WINDOW-ORDER %s: cancel younger windows before older restore' % w['tag'])
            # Restoring older first would let a late inner finish reinstate its
            # intermediate snapshot. Cancel/restore inner-to-outer, exactly once.
            younger = _st['order'][positions[0] + 1:] if len(positions) == 1 else []
            for child in reversed(younger):
                _restore(child, 'restoreFail', 'cancel-restore')
                _settle(child, 'cancelled')
        restored = _restore(w, 'restoreFail', 'return' if how == 'return' else 'unwind')
        good = w['usable'] and w['mv'] == w['expect'] and not w['bad']
        if how == 'return' and not good:
            _fail('loop:' + w['tag'])
        _log('MOVE-LOOP %s %s calls=%d expected=%d argMismatches=%d close=%s'
             % (w['tag'], 'OK' if good and how == 'return' else 'FAILED',
                w['mv'], w['expect'], w['bad'], how))
        _settle(w, how)
        return restored and ordered and good and how == 'return' and not _st['poisoned']

    def stop(self):
        # Halt on abnormal close rather than silently running with failed rollback.
        return not self._finish('return')

    def out_of_scope(self):
        _log('WINDOW-UNWOUND ' + self.win['tag'])
        self._finish('unwind')


def body_fixture(g):
    st = _st
    st['calls'] += 1
    if st['poisoned']:
        st['refused'] += 1
        _fail('poisoned-entry')
        return
    try:
        frame = g.newest_frame()
        fname, level = _frame_info(frame)
        context = _context(g)
        if not fname.startswith(_BODY):
            raise RuntimeError('not an UpdateAllBoidMovement frame')
        this, mgr = _i(g, '$rdi'), _i(g, '$rsi')
        for other in st['order']:
            if other['context'] == context and _same_frame(other['frame'], frame):
                st['dupEntry'] += 1
                _fail('dup-entry:' + other['tag'])
                _log('DUPLICATE-ENTRY ' + other['tag'])
                return
        dt = _f(g, '$xmm0.v4_float[0]')
        vec = _i(g, '&((CSwarmBasics*)%d)->mBoids' % this)
        count = _i(g, '((%s*)%d)->mCount' % (_VEC_CB, vec))
        cap = _i(g, '((%s*)%d)->mCapacity' % (_VEC_CB, vec))
        items = _i(g, '((%s*)%d)->mItems' % (_VEC_CB, vec))
        animated0 = _i(g, '((CSwarmBasics*)%d)->mAnimated' % this)
        mvec = _i(g, '&((CSwarmBasics*)%d)->mModelDatas' % this)
        nmodels = _i(g, '((rstl::vector<CModelData, rstl::rmemory_allocator>*)%d)->mCount' % mvec)
        advvec = _i(g, '&((CSwarmBasics*)%d)->mAdvancementDeltas' % this)
        nadv = _i(g, '((%s*)%d)->mCount' % (_VEC_D, advvec))
        advitems = _i(g, '((%s*)%d)->mItems' % (_VEC_D, advvec))
    except Exception as error:
        st['err'] += 1
        _fail('entry-unreadable')
        _log('ENTRY-FAILED no writes: %s' % error)
        return
    phase = st['calls'] % 3
    animated, target_count = (0 if phase == 0 else 1), (0 if phase == 2 else count)
    if count < 0 or count > cap or (animated and target_count and
            (nmodels < 1 or nadv < nmodels or not items or not advitems)):
        st['refused'] += 1
        _fail('refused-input')
        return
    w = dict(tag='swb15-%d-ph%d' % (st['calls'], phase), this=this, mgr=mgr,
             frame=frame, fname=fname, entry_level=level, context=context, vec=vec,
             items=items, advItems=advitems, mask=nmodels - 1 if nmodels else 0,
             dt=dt, expect=target_count if animated else 0, mv=0, bad=0,
             count0=count, animated0=animated0, count=target_count, animated=animated,
             closed=False, usable=True, needs_restore=False, attempted=[])
    try:
        w['breakpoint'] = _Restore(frame, w)  # armed BEFORE the first attempted write
    except Exception as error:
        st['noRestore'] += 1
        _fail('no-restore:' + w['tag'])
        _log('ARM-FAILED %s no writes: %s' % (w['tag'], error))
        return
    st['open'][id(w)] = w
    st['order'].append(w)
    st['opens'] += 1
    if animated != animated0 or target_count != count:
        w['needs_restore'] = True  # even an execute that raises may have mutated memory
        try:
            for field, expression, original in _fields(w):
                value = animated if field == 'anim' else target_count
                w['attempted'].append(field)
                g.execute('set var %s = %d' % (expression, value))
                if _i(g, expression) != value:
                    raise RuntimeError(field + ' forced readback mismatch')
        except Exception as error:
            st['err'] += 1
            w['usable'] = False
            _fail('write-failed:' + w['tag'])
            _log('WRITE-FAILED %s attempted=%s %s' % (w['tag'], w['attempted'], error))
            _restore(w, 'rollbackFail', 'rollback')
            return
        st['apply'] += 1
    _log('%s %s this=0x%x entry_level=%d context=%s expectMoveBoid=%d'
         % (w['tag'], 'FORCED' if w['needs_restore'] else 'NATURAL', this, level,
            context, w['expect']))


def moveboid_oracle(g):
    st = _st
    try:
        this, mgr = _i(g, '$rdi'), _i(g, '$rsi')
        boid, delta = _i(g, '$rdx'), _i(g, '$rcx')
        dt = _f(g, '$xmm0.v4_float[0]')
        top = g.newest_frame()
        topname, _ = _frame_info(top)
        caller = top.older()
        cname, level = _frame_info(caller)
        context = _context(g)
        if not topname.startswith(_MOVE):
            raise RuntimeError('not a MoveBoid frame')
        mine = [w for w in st['order'] if not w['closed'] and w['this'] == this
                and w['context'] == context and w['fname'] == cname
                and _same_frame(w['frame'], caller)]
    except Exception as error:
        st['ownershipFail'] += 1
        st['unattributed'] += 1
        _fail('caller-frame-unreadable')
        _log('MOVE-ARG UNATTRIBUTED unreadable: %s' % error)
        return
    if len(mine) != 1 or st['poisoned'] or not mine[0]['usable']:
        st['unattributed'] += 1
        _fail('move-ambiguous' if len(mine) > 1 else 'move-no-owner')
        _log('MOVE-ARG UNATTRIBUTED this=0x%x caller_level=%d owners=%d' % (this, level, len(mine)))
        return
    w = mine[0]
    try:
        if not st['sz']:
            st['sz'] = _i(g, '(long)sizeof(CSwarmBasics::CBoid)')
            st['adv'] = _i(g, '(long)sizeof(CAdvancementDeltas)')
        i = w['mv']
        want_boid = w['items'] + i * st['sz']
        want_delta = w['advItems'] + (i & w['mask']) * st['adv']
        good = (i < w['expect'] and mgr == w['mgr'] and boid == want_boid
                and delta == want_delta and dt == w['dt'])
    except Exception as error:
        st['unattributed'] += 1
        _fail('move-arg-unreadable')
        _log('MOVE-ARG UNATTRIBUTED argument check: %s' % error)
        return
    w['mv'] += 1
    st['mvHits'] += 1
    if not good:
        w['bad'] += 1
    _log('MOVE-ARG %s i=%d %s entry_level=%d caller_level=%d'
         % (w['tag'], i, 'OK' if good else 'MISMATCH', w['entry_level'], level))


def _summary(event=None):
    st = _st
    st['pending'] = len(st['order'])  # idempotent, not one increment per summary call
    for w in st['order']:
        _fail('pending:' + w['tag'])
    if not st['opens']:
        _fail('no-windows')
    if (len(st['open']) != st['pending'] or
            set(st['open']) != {id(w) for w in st['order']} or
            st['opens'] != st['closed'] + st['unwound'] + st['pending']):
        _fail('window-accounting')
    ok = not st['status'] and not st['pending'] and not st['poisoned']
    reasons = {}
    for reason in st['status']:
        kind = reason.split(':', 1)[0]
        reasons[kind] = reasons.get(kind, 0) + 1
    _log('SUMMARY exit: calls=%d opens=%d closed=%d unwound=%d pending=%d dupEntry=%d'
         ' unattributed=%d moveBoidHits=%d restoreFail=%d rollbackFail=%d noRestore=%d'
         ' refused=%d err=%d ownershipFail=%d orderFail=%d lateFinish=%d status=%s reasons=%s'
         % (st['calls'], st['opens'], st['closed'], st['unwound'], st['pending'], st['dupEntry'],
            st['unattributed'], st['mvHits'], st['restoreFail'], st['rollbackFail'], st['noRestore'],
            st['refused'], st['err'], st['ownershipFail'], st['orderFail'], st['lateFinish'],
            'OK' if ok else 'FAIL', reasons))
    return ok and not st['status']  # also fail if writing the summary log failed


try:
    gdb.events.exited.connect(_summary)
except Exception as error:
    _st['err'] += 1
    _fail('exit-hook-unavailable')

FORCE = [
    ('5024 body', 'CSwarmBasics::UpdateAllBoidMovement(', [], [body_fixture], 0,
     'frame-owned ph0/1/2; original snapshots and return/unwind restore before writes'),
    ('5024 MoveBoid args', 'CSwarmBasics::MoveBoid(', [], [moveboid_oracle], 0,
     'stable immediate-caller activation ownership plus actual boid/delta/dt oracle'),
]
