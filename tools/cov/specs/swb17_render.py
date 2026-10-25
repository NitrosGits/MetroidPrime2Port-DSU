# SwarmBasics swb17: CSwarmBasics::Render (fn_80_3B34), HardwareLight (fn_80_4538) and
# PreRenderBoid (fn_80_4194) forcing fixture (tools/cov/force.sh, CLASS below).
#
# Render windows. On every PERIOD-th Render call the fixture owns the call: it retains the real
# gdb.Frame of the Render activation, arms a FinishBreakpoint on it BEFORE any write, snapshots the
# raw values, forces the phase's members, and counts the HardwareLight/SoftwareLight calls whose
# immediate older frame is that very activation (gdb frame equality guarded by validity, name and
# inferior/thread; never this/name/depth alone). At the return it compares the counts with an
# oracle computed from the forced state, restores every value from the stored absolute addresses
# (never the argument registers), reads each one back, and logs OK/FAILED. Any uncertainty (refused
# input, ambiguous ownership, unrestored write, window open at exit) is a FAIL in the SUMMARY line.
# No inferior calls anywhere; stop() only reads and writes memory.
#
# Phases (the outlier head is the first boid with mActive && mInFrustum; its mNext chain is the real
# list tail, no boid link is ever written):
#   A hardware lights   mEnableLighting 1, mUseSoftwareLight 0
#   B software, match   lighting 1, soft 1, mThinkCounter 1   (outlier index 1 matches)
#   C software, miss    lighting 1, soft 1, mThinkCounter 0
#   D lighting off      mEnableLighting 0
#   E outlier outside   lighting 1, soft 0, head mInFrustum 0
#   G outlier inactive  lighting 1, soft 0, head mActive 0
#   F active cleared    as B, and the head's mActive cleared when its SoftwareLight returns
#                       (Render's second `if (boid->mActive)`, line 1227)
# PreRenderBoid windows: every PRB_PERIOD-th call writes mFreezeTimer = 1.0f on the boid argument;
# the oracle is that CachePose is not called from that activation.
CLASS = 'CSwarmBasics'

import os
import struct
import gdb

_OUT = os.environ.get('COV_OUT', '/tmp/opencode')
_LOG = os.path.join(_OUT, 'swb17_render.log')
_RENDER = 'CSwarmBasics::Render('
_HW = 'CSwarmBasics::HardwareLight('
_SW = 'CSwarmBasics::SoftwareLight('
_PRB = 'CSwarmBasics::PreRenderBoid('
_CP = 'CSwarmBasics::CachePose('
PERIOD, OFFSET = 2, 1
PRB_PERIOD, PRB_OFFSET = 12, 3
CBOID = 'CSwarmBasics::CBoid'
FREEZE_OFF = 96  # CBoid::mFreezeTimer (ptype/o on the coverage binary)
FROZEN = 0x3F800000  # 1.0f

# (name, lighting, soft, thinkCounter, headInFrustum, headActive, clearActiveAfterSoftLight, extras)
# extras: 'dl0'    mDisplayList.mPtr = 0 (line 1175 false arm; RenderBoid's TARGET_PC guard returns)
#         'alpha'  GetRenderAlphaBufferAlpha returns 0 to this Render (lines 1166 and 1239)
#         'lights' CGraphics::mLightActive = 1 after DisableAllLights returns (line 1178-1182 arms)
PHASES = [
    ('A', 1, 0, None, None, None, False, ()),
    ('B', 1, 1, 1, None, None, False, ()),
    ('C', 1, 1, 0, None, None, False, ()),
    ('D', 0, None, None, None, None, False, ()),
    ('E', 1, 0, None, 0, None, False, ()),
    ('G', 1, 0, None, None, 0, False, ()),
    ('F', 1, 1, 1, None, None, True, ()),
    ('H', 1, 0, None, None, None, False, ('alpha',)),
    ('LA', 1, 0, None, None, None, False, ('lights',)),
    ('LD', 0, None, None, None, None, False, ('lights',)),
    ('LB', 1, 1, 1, None, None, False, ('lights',)),
]
E_LIGHT_ACTIVE = 'CGraphics::mLightActive'
# Phase N nulls the display list. Only the base CSwarmBasics::RenderBoid (line 1122, TARGET_PC) returns
# on a null list; CPlantScarabSwarm/Metaree/IngBlob/... RenderBoid overrides draw through it and
# crash. SWB17_DL0=1 is for a CFlyerSwarm area (Agon 01_sand_entrance), which uses the base.
if os.environ.get('SWB17_DL0') == '1':
    PHASES.insert(7, ('N', 1, 0, None, None, None, False, ('dl0',)))

_st = dict(render=0, opens=0, closed=0, unwound=0, hw=0, sw=0, unowned=0, refused=0,
           orderFail=0, restoreFail=0, noRestore=0, ownershipFail=0, err=0, dup=0,
           countFail=0, lateFinish=0, softHooks=0, hookFail=0,
           prb=0, prbMode={}, prbOpens=0, prbClosed=0, prbUnwound=0, prbCache=0, prbFail=0,
           open={}, status=[], phaseOk={}, phaseBad={})


# ---- expression builders (one place, so the offline test and the binary dry run use the same text)
def e_this(this, field):
    return '((CSwarmBasics*)%d)->%s' % (this, field)


def e_ptr_raw(this, field):
    return '*(unsigned long*)&((CSwarmBasics*)%d)->%s' % (this, field)


def e_cells(this):
    return '(long)&((CSwarmBasics*)%d)->mPartitionedBoidLists.mData[0]' % this


def e_u64(addr):
    return '*(unsigned long*)(%d)' % addr


def e_u32(addr):
    return '*(unsigned int*)(%d)' % addr


def e_boid(boid, field):
    return '((%s*)%d)->%s' % (CBOID, boid, field)


E_SIZEOF = '(long)sizeof(%s)' % CBOID
E_NEXT_OFF = '(long)&((%s*)0)->mNext' % CBOID


def _log(msg):
    try:
        with open(_LOG, 'a') as fh:
            fh.write(msg + '\n')
    except Exception:
        _st['err'] += 1
        _fail('log-error')


def _fail(reason):
    if reason not in _st['status']:
        _st['status'].append(reason)


def _i(g, expression):
    return int(g.parse_and_eval(expression))


def _context(g):
    thread, inferior = g.selected_thread(), g.selected_inferior()
    if thread is None or inferior is None or thread.global_num is None:
        raise RuntimeError('missing inferior/thread')
    return int(inferior.num), int(thread.global_num)


def _frame_name(frame):
    if frame is None or not frame.is_valid():
        raise RuntimeError('missing/invalid frame')
    name = frame.name()
    if not name:
        raise RuntimeError('unreadable frame name')
    return name


def _same_frame(a, b):
    _frame_name(a)
    _frame_name(b)
    return bool(a == b)  # GDB activation identity


def expected(cell_indices, chain, lighting, soft, think):
    """(hardware, software, cellSoftware) calls of one Render body.

    cell_indices: the non-empty partition cell indices (x + 5y + 25z); chain: the outlier list as
    [(inFrustum, active)] in link order; the outlier index starts at 1 (line 1215)."""
    if not lighting:
        return 0, 0, 0
    hw = sw = cell_sw = 0
    for index in cell_indices:
        if soft:
            if (index & 3) == (think & 3):
                sw += 1
                cell_sw += 1
        else:
            hw += 1
    for n, (in_frustum, active) in enumerate(chain, 1):
        if in_frustum and active:
            if soft:
                if (n & 3) == (think & 3):
                    sw += 1
            else:
                hw += 1
    return hw, sw, cell_sw


def _restore(w, label):
    """Write every original back (each independently), read each back."""
    errors = []
    for expr, orig in reversed(w['snap']):
        try:
            gdb.execute('set var %s = %d' % (expr, orig))
        except Exception as error:
            errors.append('write %s: %s' % (expr, error))
    for expr, orig in w['snap']:
        try:
            if _i(gdb, expr) != orig:
                errors.append('readback %s' % expr)
        except Exception as error:
            errors.append('read %s: %s' % (expr, error))
    w['dirty'] = bool(errors)
    if errors:
        _st['restoreFail'] += 1
        _fail('restore-%s:%s' % (label, w['tag']))
    _log('RESTORE %s %s %s snap=%s errors=%s' % (w['tag'], label, 'FAILED' if errors else 'OK',
                                                 w['snap'], errors))
    return not errors


def _settle(w, how):
    w['closed'] = True
    _st['open'].pop(id(w), None)
    key = 'closed' if how == 'return' else 'unwound'
    _st[key if w['kind'] == 'render' else ('prbClosed' if how == 'return' else 'prbUnwound')] += 1
    if how != 'return':
        _fail(how + ':' + w['tag'])


class _Finish(gdb.FinishBreakpoint):
    def __init__(self, frame, win):
        super().__init__(frame, internal=True)
        self.win = win

    def _done(self, how):
        w = self.win
        if w['closed']:
            _st['lateFinish'] += 1
            _fail('late-finish:' + w['tag'])
            _log('LATE-FINISH %s ignored, no writes' % w['tag'])
            return False
        restored = _restore(w, 'return' if how == 'return' else 'unwind')
        if w['kind'] == 'render':
            good = ((w['gotHw'], w['gotSw']) == (w['expHw'], w['expSw']) and w['hooksOk']
                    and w['need'] <= w['done'] and (not w['clear'] or w['hookDone']))
            if how == 'return' and not good:
                _st['countFail'] += 1
                _fail('count:' + w['tag'])
            ph = w['phase']
            (_st['phaseOk'] if good and how == 'return' else _st['phaseBad'])[ph] = \
                (_st['phaseOk'] if good and how == 'return' else _st['phaseBad']).get(ph, 0) + 1
            _log('WINDOW %s %s hw=%d/%d sw=%d/%d hooks=%s extras=%s/%s close=%s'
                 % (w['tag'], 'OK' if good and how == 'return' else 'FAILED', w['gotHw'], w['expHw'],
                    w['gotSw'], w['expSw'], w['hooksOk'], sorted(w['done']), sorted(w['need']), how))
        else:
            good = w['gotCache'] == 0 and _i(gdb, e_u32(w['mask'])) == w['mask0']
            if how == 'return' and not good:
                _st['prbFail'] += 1
                _fail('prb-cachepose:' + w['tag'])
            _log('WINDOW %s %s cachePose=%d close=%s' % (w['tag'], 'OK' if good else 'FAILED',
                                                        w['gotCache'], how))
        _settle(w, how)
        return restored and good and how == 'return'

    def stop(self):
        self._done('return')
        return False  # a failed window is logged and fails the SUMMARY; the run goes on

    def out_of_scope(self):
        _log('WINDOW-UNWOUND ' + self.win['tag'])
        self._done('unwind')


class _SoftFinish(gdb.FinishBreakpoint):
    """The head's SoftwareLight returned: clear its mActive (phase F), with its own readback."""

    def __init__(self, frame, win):
        super().__init__(frame, internal=True)
        self.win = win

    def stop(self):
        w = self.win
        try:
            if w['closed']:
                raise RuntimeError('window already closed')
            expr = e_boid(w['head'], 'mActive')
            gdb.execute('set var %s = 0' % expr)
            if _i(gdb, expr) != 0:
                raise RuntimeError('readback')
            _st['softHooks'] += 1
            w['hookDone'] = True
        except Exception as error:
            _st['hookFail'] += 1
            w['hooksOk'] = False
            _fail('hook:' + w['tag'])
            _log('SOFT-HOOK %s FAILED %s' % (w['tag'], error))
        return False

    def out_of_scope(self):
        self.win['hooksOk'] = False
        _fail('hook-unwound:' + self.win['tag'])


def _owner(g, kind):
    """The open window whose activation is the immediate caller of the current top frame."""
    top = g.newest_frame()
    _frame_name(top)
    caller = top.older()
    _frame_name(caller)
    context = _context(g)
    mine = [w for w in _st['open'].values() if w['kind'] == kind and not w['closed']
            and w['context'] == context and _same_frame(w['frame'], caller)]
    return mine[0] if len(mine) == 1 else None, len(mine)


def render_body(g):
    st = _st
    st['render'] += 1
    if st['render'] % PERIOD != OFFSET:
        return
    try:
        frame = g.newest_frame()
        fname = _frame_name(frame)
        context = _context(g)
        if not fname.startswith(_RENDER.rstrip('(')):
            raise RuntimeError('not a Render frame')
        this = _i(g, '$rdi')
        for other in st['open'].values():
            if other['context'] == context and _same_frame(other['frame'], frame):
                st['dup'] += 1
                _fail('dup-entry:' + other['tag'])
                return
        ph = PHASES[st['opens'] % len(PHASES)]
        name, lighting, soft, think, head_f, head_a, clear_after, extras = ph
        count = _i(g, e_this(this, 'mBoids.mCount'))
        items = _i(g, e_this(this, 'mBoids.mItems'))
        size = _i(g, E_SIZEOF)
        nextoff = _i(g, E_NEXT_OFF)
        cells = _i(g, e_cells(this))
        if count <= 0 or count > 1024 or not items or size <= 0:
            raise RuntimeError('unusable boid vector')
        head = 0
        for k in range(count):
            b = items + k * size
            if _i(g, e_boid(b, 'mActive')) and _i(g, e_boid(b, 'mInFrustum')):
                head = b
                break
        if not head:
            st['refused'] += 1
            _log('REFUSED render=%d phase=%s: no active in-frustum boid' % (st['render'], name))
            return
        cell_idx = [i for i in range(125) if _i(g, e_u64(cells + 8 * i)) != 0]
        # the chain from the head, with the phase's head flags applied
        chain, seen, b = [], set(), head
        while b:
            if b in seen or len(chain) > count + 1:
                raise RuntimeError('outlier chain loops')
            seen.add(b)
            f, a = _i(g, e_boid(b, 'mInFrustum')), _i(g, e_boid(b, 'mActive'))
            if b == head:
                f = head_f if head_f is not None else f
                a = head_a if head_a is not None else a
            chain.append((f, a))
            b = _i(g, e_u64(b + nextoff))
        tc0 = _i(g, e_this(this, 'mThinkCounter'))
        light0 = _i(g, e_this(this, 'mEnableLighting'))
        soft0 = _i(g, e_this(this, 'mUseSoftwareLight'))
        eff_soft = soft0 if soft is None else soft
        eff_think = tc0 if think is None else think
        exp_hw, exp_sw, cell_sw = expected(cell_idx, chain, lighting, eff_soft, eff_think)
    except Exception as error:
        st['err'] += 1
        _fail('render-entry-unreadable')
        _log('ENTRY-FAILED render=%d no writes: %s' % (st['render'], error))
        return
    w = dict(kind='render', tag='swb17-r%d-%s' % (st['render'], name), phase=name, frame=frame,
             context=context, this=this, head=head, expHw=exp_hw, expSw=exp_sw, cellSw=cell_sw,
             gotHw=0, gotSw=0, hooksOk=True, hookDone=False, clear=clear_after, closed=False,
             extras=extras, need=set(extras), done=set(),
             snap=[], dirty=False)
    writes = [(e_this(this, 'mEnableLighting'), lighting, light0),
              (e_ptr_raw(this, 'mOutlierBoidList'), head, _i(g, e_ptr_raw(this, 'mOutlierBoidList')))]
    if soft is not None:
        writes.append((e_this(this, 'mUseSoftwareLight'), soft, soft0))
    if think is not None:
        writes.append((e_this(this, 'mThinkCounter'), think, tc0))
    if head_f is not None:
        writes.append((e_boid(head, 'mInFrustum'), head_f, _i(g, e_boid(head, 'mInFrustum'))))
    if head_a is not None:
        writes.append((e_boid(head, 'mActive'), head_a, _i(g, e_boid(head, 'mActive'))))
    if 'dl0' in extras:
        writes.append((e_ptr_raw(this, 'mDisplayList.mPtr'), 0,
                       _i(g, e_ptr_raw(this, 'mDisplayList.mPtr'))))
    if clear_after:
        w['snap'].append((e_boid(head, 'mActive'), _i(g, e_boid(head, 'mActive'))))
    try:
        w['finish'] = _Finish(frame, w)  # armed BEFORE the first write
    except Exception as error:
        st['noRestore'] += 1
        _fail('no-restore:' + w['tag'])
        _log('ARM-FAILED %s no writes: %s' % (w['tag'], error))
        return
    st['open'][id(w)] = w
    st['opens'] += 1
    w['snap'] += [(e, orig) for e, v, orig in writes]
    try:
        for expr, value, orig in writes:
            g.execute('set var %s = %d' % (expr, value))
            if _i(g, expr) != value:
                raise RuntimeError('forced readback mismatch ' + expr)
    except Exception as error:
        st['err'] += 1
        _fail('write-failed:' + w['tag'])
        _log('WRITE-FAILED %s %s' % (w['tag'], error))
        _restore(w, 'rollback')  # the window stays open; the finish breakpoint settles it
        return
    _log('%s FORCED this=0x%x head=0x%x cells=%d chain=%d lighting=%d soft=%d think=%d '
         'expHw=%d expSw=%d cellSw=%d' % (w['tag'], this, head, len(cell_idx), len(chain), lighting,
                                          eff_soft, eff_think, exp_hw, exp_sw, cell_sw))


def _light_hit(g, kind):
    st = _st
    try:
        w, n = _owner(g, 'render')
    except Exception as error:
        st['ownershipFail'] += 1
        _fail('caller-frame-unreadable')
        _log('LIGHT-HIT unreadable owner: %s' % error)
        return
    if w is None:
        st['unowned'] += 1
        if n > 1:
            _fail('ambiguous-owner')
        return
    if kind == 'hw':
        w['gotHw'] += 1
        st['hw'] += 1
        return
    w['gotSw'] += 1
    st['sw'] += 1
    # phase F: the first outlier SoftwareLight is call cellSw + 1 (the cells call it first)
    if w['clear'] and w['gotSw'] == w['cellSw'] + 1:
        try:
            _SoftFinish(g.newest_frame(), w)
        except Exception as error:
            st['hookFail'] += 1
            w['hooksOk'] = False
            _fail('hook-arm:' + w['tag'])
            _log('SOFT-HOOK-ARM %s FAILED %s' % (w['tag'], error))


def hw_hit(g):
    _light_hit(g, 'hw')


def sw_hit(g):
    _light_hit(g, 'sw')


class _RegFinish(gdb.FinishBreakpoint):
    """After DisableAllLights / GetRenderAlphaBufferAlpha returned to a window's Render: the write."""

    def __init__(self, frame, win, what):
        super().__init__(frame, internal=True)
        self.win, self.what = win, what

    def stop(self):
        w = self.win
        try:
            if w['closed']:
                raise RuntimeError('window already closed')
            if self.what == 'lights':
                orig = _i(gdb, E_LIGHT_ACTIVE)
                w['snap'].append((E_LIGHT_ACTIVE, orig))
                gdb.execute('set var %s = 1' % E_LIGHT_ACTIVE)
                if _i(gdb, E_LIGHT_ACTIVE) != 1:
                    raise RuntimeError('mLightActive readback')
            else:
                gdb.execute('set var $rax = 0')  # int return value in eax (x86-64 SysV)
                if _i(gdb, '(int)$eax') != 0:
                    raise RuntimeError('eax readback')
            w['done'].add(self.what)
        except Exception as error:
            w['hooksOk'] = False
            _st['hookFail'] += 1
            _fail('hook:%s:%s' % (self.what, w['tag']))
            _log('REG-HOOK %s %s FAILED %s' % (w['tag'], self.what, error))
        return False

    def out_of_scope(self):
        self.win['hooksOk'] = False
        _fail('hook-unwound:' + self.win['tag'])


def _reg_hit(g, what):
    try:
        w, _ = _owner(g, 'render')
        if w is None or what not in w['need'] or what in w['done'] or w.get('armed_' + what):
            return
        w['armed_' + what] = True
        _RegFinish(g.newest_frame(), w, what)
    except Exception as error:
        _st['ownershipFail'] += 1
        _fail('reg-hook-arm')
        _log('REG-HOOK-ARM %s FAILED %s' % (what, error))


def dal_hit(g):
    _reg_hit(g, 'lights')


def alpha_hit(g):
    _reg_hit(g, 'alpha')


def prb_body(g):
    st = _st
    st['prb'] += 1
    n = st['prb'] % PRB_PERIOD
    if n not in (PRB_OFFSET, PRB_OFFSET + 1):
        return
    mode = 'freeze' if n == PRB_OFFSET else 'empty'
    try:
        frame = g.newest_frame()
        if not _frame_name(frame).startswith(_PRB.rstrip('(')):
            raise RuntimeError('not a PreRenderBoid frame')
        context = _context(g)
        this, boid, mask = _i(g, '$rdi'), _i(g, '$rsi'), _i(g, '$rdx')
        mask0 = _i(g, e_u32(mask))
        if mode == 'freeze':
            expr = e_u32(boid + FREEZE_OFF)
            value = FROZEN
        else:
            # an empty model list makes idx = mIndex; only a boid with mIndex > 31 takes the host
            # guard's false side (no model state is read, bit = 0), so refuse any other boid
            if _i(g, e_boid(boid, 'mIndex')) <= 31:
                return
            expr = e_this(this, 'mModelDatas.mCount')
            value = 0
        orig = _i(g, expr)
    except Exception as error:
        st['err'] += 1
        _fail('prb-entry-unreadable')
        _log('PRB-ENTRY-FAILED %s' % error)
        return
    w = dict(kind='prb', tag='swb17-p%d-%s' % (st['prb'], mode), frame=frame, context=context,
             gotCache=0, closed=False, snap=[(expr, orig)], dirty=False, mask=mask, mask0=mask0)
    try:
        w['finish'] = _Finish(frame, w)
    except Exception as error:
        st['noRestore'] += 1
        _fail('no-restore:' + w['tag'])
        _log('ARM-FAILED %s %s' % (w['tag'], error))
        return
    st['open'][id(w)] = w
    st['prbOpens'] += 1
    st['prbMode'][mode] = st['prbMode'].get(mode, 0) + 1
    try:
        g.execute('set var %s = %d' % (expr, value))
        if _i(g, expr) != value:
            raise RuntimeError('forced readback mismatch')
    except Exception as error:
        st['err'] += 1
        _fail('write-failed:' + w['tag'])
        _log('PRB-WRITE-FAILED %s %s' % (w['tag'], error))
        _restore(w, 'rollback')
        return
    _log('%s FORCED boid=0x%x %s 0x%x -> 0x%x' % (w['tag'], boid, expr, orig, value))


def cp_hit(g):
    try:
        w, _ = _owner(g, 'prb')
    except Exception as error:
        _st['ownershipFail'] += 1
        _fail('caller-frame-unreadable')
        _log('CACHEPOSE unreadable owner: %s' % error)
        return
    if w is not None:
        w['gotCache'] += 1
        _st['prbCache'] += 1


def _summary(event=None):
    st = _st
    pending = len(st['open'])
    for w in st['open'].values():
        _fail('pending:' + w['tag'])
    for ph in PHASES:
        if not st['phaseOk'].get(ph[0]):
            _fail('phase-never-ok:' + ph[0])
    if not st['prbOpens']:
        _fail('no-prb-windows')
    if st['opens'] != st['closed'] + st['unwound'] + sum(1 for w in st['open'].values()
                                                       if w['kind'] == 'render'):
        _fail('render-accounting')
    if st['prbOpens'] != st['prbClosed'] + st['prbUnwound'] + sum(1 for w in st['open'].values()
                                                                 if w['kind'] == 'prb'):
        _fail('prb-accounting')
    ok = not st['status'] and not pending
    _log('SUMMARY exit: render=%d opens=%d closed=%d unwound=%d pending=%d hw=%d sw=%d unowned=%d '
         'refused=%d countFail=%d restoreFail=%d noRestore=%d ownershipFail=%d err=%d dup=%d '
         'softHooks=%d hookFail=%d prb=%d prbOpens=%d prbClosed=%d prbUnwound=%d prbCache=%d '
         'prbFail=%d prbMode=%s phaseOk=%s phaseBad=%s status=%s %s'
         % (st['render'], st['opens'], st['closed'], st['unwound'], pending, st['hw'], st['sw'],
            st['unowned'], st['refused'], st['countFail'], st['restoreFail'], st['noRestore'],
            st['ownershipFail'], st['err'], st['dup'], st['softHooks'], st['hookFail'], st['prb'],
            st['prbOpens'], st['prbClosed'], st['prbUnwound'], st['prbCache'], st['prbFail'], st['prbMode'],
            dict(sorted(st['phaseOk'].items())), dict(sorted(st['phaseBad'].items())),
            'OK' if ok else 'FAIL', st['status']))
    return ok


try:
    gdb.events.exited.connect(_summary)
except Exception:
    _st['err'] += 1
    _fail('exit-hook-unavailable')

FORCE = [
    ('DisableAllLights', 'CGraphics::DisableAllLights(', [], [dal_hit], 0,
     'phases LA/LD/LB: mLightActive = 1 once it returns to a window'),
    ('GetRenderAlphaBufferAlpha', 'CActor::GetRenderAlphaBufferAlpha(', [], [alpha_hit], 0,
     'phase H: the result forced to 0 for a window'),
    ('3B34 Render window', _RENDER, [], [render_body], 0,
     'frame-owned phases A/B/C/D/E/G/F; snapshot, forced readback, return/unwind restore'),
    ('4538 HardwareLight', _HW, [], [hw_hit], 0, 'counts calls whose caller is a Render window'),
    ('4194+ SoftwareLight', _SW, [], [sw_hit], 0, 'counts calls; phase F clears the head mActive'),
    ('4194 PreRenderBoid window', _PRB, [], [prb_body], 0,
     'frozen boid window (mFreezeTimer = 1.0f), restored at return'),
    ('4194 CachePose', _CP, [], [cp_hit], 0, 'CachePose must not run from a frozen window'),
]
