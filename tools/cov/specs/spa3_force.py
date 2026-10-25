# ScriptPlayerActor (REL 61) forcing spec, spa3: the arms spa2's one-shot writes missed.  Every
# body cycles through its variants on EVERY call (the old value is put back before the next write
# and at a `loc:` line after the arm has read it), so no arm depends on a call number or on one
# object being reachable.  Registers are never restored (a by-value argument is dead after entry).
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/spa3_force.py <out> \
#     42b935e4,03_sand_temple_dark --con tools/drivers/spa3_templedark.con --fast --frames 2600
import os

import gdb

# `this` and the manager are grabbed at a body's entry (rdi/rsi are not them at a `loc:` line of an -O2 body)
ctx = {'this': 0, 'mgr': 0}


class _Expr:
    def __init__(self, fmt, key):
        self.fmt, self.key = fmt, key

    def __add__(self, other):
        return (self.fmt % ctx[self.key]) + other

    def __radd__(self, other):
        return other + (self.fmt % ctx[self.key])


THIS = _Expr('((CScriptPlayerActor*)%d)', 'this')
MGR = _Expr('((CStateManager*)%d)', 'mgr')


def grab(gdb):
    ctx['this'] = int(gdb.parse_and_eval('$rdi'))
    ctx['mgr'] = int(gdb.parse_and_eval('$rsi'))
SRC = 'CScriptPlayerActor.cpp:'


def _log(msg):
    try:
        with open(os.environ['COV_OUT'] + '/forced.log', 'a') as fh:
            fh.write(msg + '\n')
    except Exception:
        pass


def _read(expr):
    try:
        return gdb.parse_and_eval(expr)
    except Exception as e:  # noqa: BLE001
        _log('read failed (%s): %s' % (expr, str(e)[:60]))
        return None


def _val(expr):
    v = _read(expr)
    return None if v is None else int(v)


class Forcer:
    """A body's forcing: `variants[(n-1) % len]` is applied at its n-th entry; `restore` holds
    the old values and is replayed at the next entry and at the body's `loc:` end hook."""

    def __init__(self, variants, name, needs_arm=False):
        self.variants, self.name, self.n, self.restore = variants, name, 0, []
        self.needs_arm, self.armed = needs_arm, False

    def arm(self, gdb):
        # an -O2 line has several code addresses: write on the first hit after the entry only
        self.armed = True

    def put_back(self, gdb):
        while self.restore:
            expr = self.restore.pop()
            try:
                gdb.execute('set var ' + expr)
            except Exception as e:  # noqa: BLE001
                _log('%s restore failed (%s): %s' % (self.name, expr, str(e)[:70]))

    def write(self, gdb, target, new):
        old = _read(target)
        if old is None:
            return False
        try:
            gdb.execute('set var %s = %s' % (target, new))
        except Exception as e:  # noqa: BLE001
            _log('%s set failed (%s): %s' % (self.name, target, str(e)[:70]))
            return False
        self.restore.append('%s = %s' % (target, str(old).split(' ')[0]))
        return True

    def __call__(self, gdb):
        if self.needs_arm:
            if not self.armed:
                return
            self.armed = False
        self.put_back(gdb)
        self.n += 1
        v = self.variants[(self.n - 1) % len(self.variants)]
        if v is not None:
            v(gdb, self)

    def end(self, gdb):
        self.put_back(gdb)


def reg(gdb, name, value):
    try:
        gdb.execute('set var %s = %s' % (name, value))
    except Exception as e:  # noqa: BLE001
        _log('reg set failed (%s): %s' % (name, str(e)[:60]))


def member(m, new):
    return lambda gdb, st: st.write(gdb, THIS + '->' + m, new)


def several(*fns):
    def run(gdb, st):
        for f in fns:
            f(gdb, st)
    return run


# ------------------------------------------------ fn_61_103C GetNextSuitCharIdx (suit in mSuit)
# (flags&2, mSuit): (set, Light) -> the 0x489 true arm; (clear, Dark/Light/Varia) -> the switch.
def flags(setbit):
    def fn(gdb, st):
        cur = _val(THIS + '->mFlags')
        if cur is not None:
            st.write(gdb, THIS + '->mFlags', cur | 2 if setbit else cur & ~2)
    return fn


next_suit = Forcer([several(flags(False), member('mSuit', 2)),    # switch default
                    several(flags(False), member('mSuit', 1)),    # switch kPS_Dark
                    several(flags(False), member('mSuit', 0)),    # switch kPS_Varia
                    several(flags(True), member('mSuit', 2)),     # Light -> Dark
                    several(flags(True), member('mSuit', 1))],    # Dark  -> Varia
                   '103C')


# --------------------------------------------------- fn_61_FDC GetSuitCharIdx(mgr, suit in $rdx)
# every (suit, mCharacterCount) pair: the outer `charIdx >= count` and the inner one.
def suit_pair(suit, count):
    def fn(gdb, st):
        reg(gdb, '$rdx', suit)
        st.write(gdb, THIS + '->mCharacterCount', count)
    return fn


suit_char = Forcer([suit_pair(s, c) for c in (0, 1, 2, 3) for s in (0, 1, 2)] + [None] * 20, 'FDC')


# ------------------------------------------------------------------ site-level pokes
# The arms below test a value an instruction loads (HasModelData: ptr, anim ptr, normal-model byte).
# A `loc:*addr` breakpoint at that test writes the value there (state-aware: the pointer is the one
# the body just loaded), and a second breakpoint at the arm's landing address puts it back, so the
# fake HasNormalModel never reaches a model call.
pend = []
cnt = {}


def wr(expr, new):
    old = _val(expr)
    if old is None:
        return False
    try:
        gdb.execute('set var %s = %s' % (expr, new))
    except Exception as e:  # noqa: BLE001
        _log('poke failed (%s): %s' % (expr, str(e)[:60]))
        return False
    pend.append((expr, old))
    return True


def restore(gdb):
    while pend:
        expr, old = pend.pop()
        try:
            gdb.execute('set var %s = %s' % (expr, old))
        except Exception as e:  # noqa: BLE001
            _log('restore failed (%s): %s' % (expr, str(e)[:60]))


def tick(name, n):
    cnt[name] = cnt.get(name, 0) + 1
    return (cnt[name] - 1) % n


def anim_site(name, reg, mode_n=3):
    """At the `cmpq $0,0x18(reg)` of a HasXModelData: 0 -> anim null + no normal model,
    1 -> anim null + normal model present, else untouched."""
    def fn(gdb):
        k = tick(name, mode_n)
        ptr = _val(reg)
        if not ptr or k > 1:
            return
        wr('*(long*)(%d+0x18)' % ptr, 0)
        wr('*(unsigned char*)(%d+0x40)' % ptr, k)
    return fn


def null_site(name, _unused, reg_set, mode_n=4):
    """At the `test reg,reg` of a HasXModelData: every mode_n-th call sees a null pointer
    (a register write only: the body does not use the register on that arm)."""
    def fn(gdb):
        if tick(name, mode_n) == 0:
            reg(gdb, reg_set, 0)
    return fn


def ret_forced(name, value, n=3):
    """At a `test %al,%al` after a call: every n-th call sees `value`."""
    def fn(gdb):
        if tick(name, n) == 0:
            reg(gdb, '$eax', value)
    return fn


def put_back(gdb):
    restore(gdb)


def grapple_alias(gdb):
    # f7cddd: HasGrappleModelData() reads mGrappleModelData (rax). No run keeps a grapple model while
    # the shadow exists, so alias the (static, anim-less) beam model for the register, and at f7cdff
    # also store it in the on-stack models[2] (rsp+0x80, stored at f7cd80) so RenderShadowBuffer runs
    # with count 3 and a valid CModelData* (no count rewrite).
    if tick('c30alias', 2) == 0:
        return
    this = _val('$rbx')
    beam = _val('*(long*)(%d+0x468)' % this) if this else None
    if beam and _val('$rax') == 0:
        reg(gdb, '$rax', str(beam))
        cnt['c30alias_on'] = beam


def alias_count(gdb):
    beam = cnt.get('c30alias_on')
    if beam:
        cnt['c30alias_on'] = 0
        reg(gdb, '*(long*)($rsp+0x80)', str(beam))  # stack slot, dead after the call: no restore
        cnt['c30models2'] = cnt.get('c30models2', 0) + 1
        _log('C30 models[2]=beam %#x, edx=%d' % (beam, _val('$edx') or -1))
    restore(gdb)


def rsb_entry(gdb):
    n = _val('$edx')
    m = _val('$rcx')
    if m:
        _log('RSB count %s models %s' % (n, [hex(_val('*(long*)(%d+%d)' % (m, 8 * i)) or 0) for i in range(3)]))
    if n == 3:
        cnt['rsb3'] = cnt.get('rsb3', 0) + 1
        _log('RenderShadowBuffer entered with count 3 (hit %d)' % cnt['rsb3'])


# --- PreRender (cov line numbers; see docs/module-analysis/ScriptPlayerActor.md section 8)
PRE = [
    ('C30 suit null', 'loc:*0xf7cd1e', null_site('c30s', None, '$rax')),
    ('C30 suit anim', 'loc:*0xf7cd27', anim_site('c30sa', '$rax')),
    ('C30 gun null', 'loc:*0xf7cd39', null_site('c30g', None, '$rdx')),
    ('C30 gun anim', 'loc:*0xf7cd4b', anim_site('c30ga', '$rdx')),
    ('C30 grapple alias', 'loc:*0xf7cddd', grapple_alias),
    ('C30 grapple anim', 'loc:*0xf7cde6', anim_site('c30ra', '$rax')),
    ('C30 restore f7cd32', 'loc:*0xf7cd32', put_back),
    ('C30 restore f7cd56', 'loc:*0xf7cd56', put_back),
    ('C30 restore f7cdff', 'loc:*0xf7cdff', alias_count),
    ('C30 restore f7ced9', 'loc:*0xf7ced9', put_back),
    ('C30 RenderShadowBuffer count', 'loc:*0x128ce10', rsb_entry),
]

# --- Think
THINK = [
    ('2630 200 anim', 'loc:*0xf7c7e8', anim_site('t200a', '$rax')),
    ('2630 restore f7c7f3', 'loc:*0xf7c7f3', put_back),
    ('2630 restore f7c811', 'loc:*0xf7c811', put_back),
    ('2630 200 cinematic', 'loc:*0xf7c809', ret_forced('t200', 1)),
    ('2630 221 null', 'loc:*0xf7c95c', null_site('t221n', None, '$rdi')),
    ('2630 221 anim', 'loc:*0xf7c96e', anim_site('t221a', '$rdi')),
    ('2630 221 IsLoaded', 'loc:*0xf7c989', ret_forced('t221l', 0)),
    ('2630 227 null', 'loc:*0xf7c6f6', null_site('t227n', None, '$rdi')),
    ('2630 227 anim', 'loc:*0xf7c708', anim_site('t227a', '$rdi')),
    ('2630 227 IsLoaded', 'loc:*0xf7c723', ret_forced('t227l', 0)),
    ('2630 restore f7caff', 'loc:*0xf7caff', put_back),
    ('2630 restore f7c799', 'loc:*0xf7c799', put_back),
    ('2630 restore f7cb20', 'loc:*0xf7cb20', put_back),
    ('2630 restore f7c8d0', 'loc:*0xf7c8d0', put_back),
]


# --- SetupEnvFx (set in $rdx at entry; the world test at f7b116 is the producer)
def envfx_prod(gdb):
    if _val('$rdx') == 0:
        return
    this = _val('$rbx')
    if not this or not _val('*(long*)(%d+0x70)' % this):
        return                                            # no model data: nothing to aim at
    k = (3, 4, 1, 2, 5, 0)[tick('ab8', 6)]
    if k == 5:                                            # mFlags 0x20 (no shadow): testb at f7b11f
        this = _val('$rbx')
        old = _val('*(unsigned char*)(%d+0x4d0)' % this) if this else None
        if old is not None:
            gdb.execute('set var *(unsigned char*)(%d+0x4d0) = %d' % (this, old | 0x20))
            pend2.append(('*(unsigned char*)(%d+0x4d0)' % this, old))
        return
    world = _val('$rax')
    if not world:
        return
    wr('*(int*)(%d+0x130)' % world, 2)                    # kEFX_Rain
    mgrp = _val('$rsi')
    dens = '*(float*)(*(long*)(%d+0x17c8)+0x30)' % mgrp if mgrp else None
    if k == 1 and dens:
        wr(dens, '1.0')
    elif k == 2 and dens:
        wr(dens, '0.0')
    cnt['ab8mode'] = k


def envfx_anim(gdb):
    k = cnt.get('ab8mode', -1)
    ptr = _val('$rax')
    cnt['ab8mode'] = -1
    if ptr and k in (3, 4):
        wr('*(long*)(%d+0x18)' % ptr, 0)
        wr('*(unsigned char*)(%d+0x40)' % ptr, k - 3)


pend2 = []


def put_back2(gdb):
    while pend2:
        expr, old = pend2.pop()
        gdb.execute('set var %s = %s' % (expr, old))


def backup_hold(gdb):
    # after `mDeallocateBackupCountdown = 2` (movl $2,0x4c8(%rbx) at f7a10e): keep the backup alive
    # so the deleting destructor sees the engaged mBackupModelData optional
    this = _val('$rbx')
    if this:
        gdb.execute('set var *(int*)(%d+0x4c8) = 1000000' % this)


OFFL = [
    ('2E3C IsMaxSpeed false', 'loc:*0xf7a06e', lambda gdb: reg(gdb, '$eax', os.environ.get('SPA3_MAXSPEED', '0'))),
    ('2E3C backup hold', 'loc:*0xf7a118', backup_hold),
]

ENV = [
    ('AB8 producer', 'loc:*0xf7b116', envfx_prod),
    ('AB8 anim', 'loc:*0xf7b14d', envfx_anim),
    ('AB8 restore f7b11f', 'loc:*0xf7b11f', put_back),
    ('AB8 restore f7b158', 'loc:*0xf7b158', put_back),
    ('AB8 restore flag f7b128', 'loc:*0xf7b128', put_back2),
]


# -------------------------------------------- fn_61_970 SetIntoStateManager (set in $rdx)
# head -> an unused id (ObjectById null: the `actor &&` arm) / head -> the other chain actor with
# its mIsPlayerActor bit cleared (the cast succeeds, IsPlayerActor() false: the else arm).
seen = []


def sia_remember(gdb, st=None):
    me = _val('$rdi')
    if me is not None and me not in seen:
        seen.append(me)


def sia_null(gdb, st):
    if _val('$rdx') == 0:
        st.write(gdb, MGR + '->mPlayerActorHead.value', 0x3fe)


def sia_not_player(gdb, st):
    me = _val('$rdi')
    head = _val(MGR + '->mPlayerActorHead.value')
    _log('970 not_player: me=%s head=%s seen=%s' % (me, head, seen))
    for other in seen:
        if other != me and _val('((CScriptPlayerActor*)%d)->mUniqueId.value' % other) == head:
            st.write(gdb, '((CScriptPlayerActor*)%d)->mIsPlayerActor' % other, 0)
            return


def sia_all(gdb, st):
    sia_not_player(gdb, st)


sia = Forcer([sia_all, sia_null, sia_all, None], '970')


# ------------------------------------------------ fn_61_C0 LoadPlayerActor (locals of the loader)
load = {'n': 0}


def load_acs(gdb):
    # SPA3_ACS=1 only (the loader then returns nullptr and the actor never exists, so the run
    # that has it cannot also drive the actor)
    load['n'] += 1
    if os.environ.get('SPA3_ACS') == '1':
        reg(gdb, 'sldrThis.animationInformation.ancs', '0xffffffff')


# ------------------------------------------------------- fn_61_2630 Think
# writes at entry; the loading flags are read at lines 213-231, restored at the last line.
think = Forcer([None,
                member('mBeamModelData.mPtr->mAnimData.mItem', 0),   # HasAnimation false at 200
                None,
                member('mModelData.mPtr->mAnimData.mItem', 0),       # !HasModelData at 221 / 227
                None,
                member('mModelData.mPtr', 0),
                None, None], '2630', needs_arm=True)


def think_loading(gdb, st):
    pass


# the binary is a PIE: `*0xADDR` would not relocate, `*symbol+off` does
BASES = [(0x128ce10, '_ZN16CProjectedShadow18RenderShadowBufferER13CStateManageriPKPK10CModelDataPKPK12CTransform4fiRK9CVector3fff'),
         (0xf7ccb0, '_ZN18CScriptPlayerActor9PreRenderER13CStateManager'),
         (0xf7c550, '_ZN18CScriptPlayerActor5ThinkEfR13CStateManager'),
         (0xf7b0f0, '_ZN18CScriptPlayerActor10SetupEnvFxER13CStateManagerb'),
         (0xf79fb0, '_ZN18CScriptPlayerActor21SetupOfflineModelDataEv')]


def _rel(w):
    a = int(w[len('loc:*'):], 16)
    for base, sym in BASES:
        if base <= a < base + 0x800:
            return 'loc:*%s+%d' % (sym, a - base)
    raise ValueError(w)


CLASS = 'CScriptPlayerActor'
def _g(*f):
    return [grab] + list(f)


FORCE = [
    ('103C GetNextSuitCharIdx', 'CScriptPlayerActor::GetNextSuitCharIdx(', [], _g(next_suit), 0,
     'every (flags&2, mSuit) pair'),
    ('103C GetNextSuitCharIdx end', 'loc:' + SRC + '507', [], [next_suit.end], 0, ''),
    ('FDC GetSuitCharIdx', 'CScriptPlayerActor::GetSuitCharIdx(', [], _g(suit_char), 0,
     'every (suit, mCharacterCount) pair'),
    ('970 SetIntoStateManager', 'CScriptPlayerActor::SetIntoStateManager(', [], _g(sia_remember, sia), 0,
     'head -> unused id; other chain actor not a player actor'),
    ('970 SetIntoStateManager end', 'loc:' + SRC + '593', [], [sia.end], 0, ''),
    ('C0 LoadPlayerActor unknown ACS', 'loc:' + SRC + '599', [], [load_acs], 0, ''),
] + [(lb, _rel(w), [], [fn], 0, '') for lb, w, fn in PRE + THINK + ENV + OFFL]

# SPA3_ONLY=offl: just the mBackupModelData pokes (the other forcing starves the suit load that the
# offline path needs)
if os.environ.get('SPA3_ONLY') == 'offl':
    FORCE = [(lb, _rel(w), [], [fn], 0, '') for lb, w, fn in OFFL]
