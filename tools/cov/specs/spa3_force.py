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


# ------------------------------------------------------- fn_61_C30 PreRender (writes at line 530)
def pre(*parts):
    return several(*parts)


def cyc_model(m):
    return lambda gdb, st: st.write(gdb, THIS + '->' + m, 0)


pre_render = Forcer([None,
                     None,
                     member('mModelData.mPtr', 0),
                     member('mModelData.mPtr->mAnimData.mItem', 0),
                     member('mBeamModelData.mPtr', 0),
                     member('mBeamModelData.mPtr->mAnimData.mItem', 0),
                     member('mGrappleModelData.mPtr', 0),
                     member('mGrappleModelData.mPtr->mAnimData.mItem', 0),
                     None, None], 'C30')


# ------------------------------------------------ fn_61_AB8 SetupEnvFx (set in $rdx)
def env_rain(gdb, st):
    if _val('$rdx') == 1:
        st.write(gdb, MGR + '->mWorld.mPtr->mNeededEnvFx', 2)      # kEFX_Rain
        st.write(gdb, MGR + '->mEnvFxManager->mFxDensity', '1.0')


def env_rain_zero(gdb, st):
    if _val('$rdx') == 1:
        st.write(gdb, MGR + '->mWorld.mPtr->mNeededEnvFx', 2)
        st.write(gdb, MGR + '->mEnvFxManager->mFxDensity', '0.0')


def env_rain_nomodel(gdb, st):
    if _val('$rdx') == 1:
        st.write(gdb, MGR + '->mWorld.mPtr->mNeededEnvFx', 2)
        st.write(gdb, MGR + '->mEnvFxManager->mFxDensity', '1.0')
        st.write(gdb, THIS + '->mModelData.mPtr', 0)


def env_noshadow(gdb, st):
    st.write(gdb, THIS + '->mFlags', (_val(THIS + '->mFlags') or 0) | 0x20)


def env_rain_noanim(gdb, st):
    if _val('$rdx') == 1:
        st.write(gdb, MGR + '->mWorld.mPtr->mNeededEnvFx', 2)
        st.write(gdb, MGR + '->mEnvFxManager->mFxDensity', '1.0')
        st.write(gdb, THIS + '->mModelData.mPtr->mAnimData.mItem', 0)


class GatedForcer(Forcer):
    """Advances its cycle only on calls whose `set` argument ($rdx) is 1."""

    def __call__(self, gdb):
        self.put_back(gdb)
        if _val('$rdx') != 1:
            return
        self.n += 1
        v = self.variants[(self.n - 1) % len(self.variants)]
        if v is not None:
            v(gdb, self)


setup_envfx = GatedForcer([env_rain, env_rain_zero, env_rain_nomodel, env_rain_noanim,
                           env_noshadow, None], 'AB8')


# -------------------------------------------- fn_61_970 SetIntoStateManager (set in $rdx)
# head -> an unused id (ObjectById null: the `actor &&` arm) / head -> the other chain actor with
# its mIsPlayerActor bit cleared (the cast succeeds, IsPlayerActor() false: the else arm).
seen = []


def sia_remember(gdb, st):
    me = _val('$rdi')
    if me is not None and me not in seen:
        seen.append(me)


def sia_null(gdb, st):
    if _val('$rdx') == 0:
        st.write(gdb, MGR + '->mPlayerActorHead.value', 0x3fe)


def sia_not_player(gdb, st):
    me = _val('$rdi')
    head = _val(MGR + '->mPlayerActorHead.value')
    for other in seen:
        if other != me and _val('((CScriptPlayerActor*)%d)->mUniqueId.value' % other) == head:
            st.write(gdb, '((CScriptPlayerActor*)%d)->mIsPlayerActor' % other, 0)
            return


def sia_all(gdb, st):
    sia_remember(gdb, st)
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


CLASS = 'CScriptPlayerActor'
def _g(*f):
    return [grab] + list(f)


FORCE = [
    ('103C GetNextSuitCharIdx', 'CScriptPlayerActor::GetNextSuitCharIdx(', [], _g(next_suit), 0,
     'every (flags&2, mSuit) pair'),
    ('103C GetNextSuitCharIdx end', 'loc:' + SRC + '507', [], [next_suit.end], 0, ''),
    ('FDC GetSuitCharIdx', 'CScriptPlayerActor::GetSuitCharIdx(', [], _g(suit_char), 0,
     'every (suit, mCharacterCount) pair'),
    ('C30 PreRender entry', 'CScriptPlayerActor::PreRender(', [], [grab, pre_render], 0, ''),
    ('C30 PreRender end', 'loc:' + SRC + '538', [], [pre_render.end], 0, ''),
    ('AB8 SetupEnvFx', 'CScriptPlayerActor::SetupEnvFx(', [], _g(setup_envfx), 0,
     'rain, rain with magnitude 0, rain without model data, mFlags 0x20'),
    ('AB8 SetupEnvFx end', 'loc:' + SRC + '561', [], [setup_envfx.end], 0, ''),
    ('970 SetIntoStateManager', 'CScriptPlayerActor::SetIntoStateManager(', [], _g(sia), 0,
     'head -> unused id; other chain actor not a player actor'),
    ('970 SetIntoStateManager end', 'loc:' + SRC + '593', [], [sia.end], 0, ''),
    ('C0 LoadPlayerActor unknown ACS', 'loc:' + SRC + '599', [], [load_acs], 0, ''),
    ('2630 Think entry', 'CScriptPlayerActor::Think(', [], [grab, think.arm], 0, ''),
    ('2630 Think', 'loc:' + SRC + '200', [], [think], 0, 'beam / model data nulled at 200'),
    ('2630 Think end', 'loc:' + SRC + '239', [], [think.end], 0, ''),
]
