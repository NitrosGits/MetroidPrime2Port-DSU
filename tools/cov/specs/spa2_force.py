# ScriptPlayerActor (REL 61) forcing spec: the data/state arms of the eight bodies spa1 left
# unmarked. One call per write, the old value put back at the next entry of the same body.
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/spa2_force.py <out> \
#     42b935e4,03_sand_temple_dark --con tools/drivers/spa2_templedark.con --fast --frames 2200
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/spa2_force.py <out> \
#     3bfa3eff,01_temple_hive03_dark --con tools/drivers/spa_hive03dark.con --fast --frames 1400
import os

import gdb

THIS = '((CScriptPlayerActor*)$rdi)'
MGR = '((CStateManager*)$rsi)'


def _log(msg):
    try:
        with open(os.environ['COV_OUT'] + '/forced.log', 'a') as fh:
            fh.write(msg + '\n')
    except Exception:
        pass


def _set(gdb, expr):
    try:
        gdb.execute('set var ' + expr)
    except Exception as e:  # noqa: BLE001
        _log('set failed (%s): %s' % (expr, str(e)[:70]))


def _read(expr):
    try:
        return gdb.parse_and_eval(expr)
    except Exception as e:  # noqa: BLE001
        _log('read failed (%s): %s' % (expr, str(e)[:60]))
        return None


def _val(expr):
    v = _read(expr)
    return None if v is None else int(v)


class OneShot:
    """A body's forcing: `hits` maps an entry count to the writes made on that call."""

    def __init__(self, hits, what=''):
        self.n = 0
        self.hits = hits
        self.what = what
        self.restore = []
        self.regs = []

    def __call__(self, gdb):
        for expr in self.restore:
            _set(gdb, expr)
        self.restore = []
        for reg, old in self.regs:
            _set(gdb, '%s = %s' % (reg, old))
        self.regs = []
        self.n += 1
        fn = self.hits.get(self.n)
        if fn is not None:
            fn(gdb, self)


def _save(body, member, new):
    def fn(gdb, st):
        old = _read(THIS + '->' + member)
        if old is None:
            return
        st.restore.append('%s->%s = %s' % (THIS, member, str(old)))
        _set(gdb, '%s->%s = %s' % (THIS, member, new))
    return fn


# ------------------------------------------------ fn_61_103C GetNextSuitCharIdx
# 0x489's `mSuit == kPS_Light` arm (kPS_Varia 0, kPS_Dark 1, kPS_Light 2) and the switch's
# kPS_Varia / default arms.
def _next_suit_light(gdb, st):
    _save(st, 'mFlags', str(_val(THIS + '->mFlags') | 2))(gdb, st)
    _save(st, 'mSuit', '2')(gdb, st)


def _next_suit_varia(gdb, st):
    _save(st, 'mFlags', str(_val(THIS + '->mFlags') & ~2))(gdb, st)
    _save(st, 'mSuit', '0')(gdb, st)


def _next_suit_default(gdb, st):
    _save(st, 'mFlags', str(_val(THIS + '->mFlags') & ~2))(gdb, st)
    _save(st, 'mSuit', '2')(gdb, st)


def _next_suit_dark(gdb, st):
    _save(st, 'mFlags', str(_val(THIS + '->mFlags') & ~2))(gdb, st)
    _save(st, 'mSuit', '1')(gdb, st)   # kPS_Dark -> the switch's Dark case


next_suit = OneShot({2: _next_suit_light, 3: _next_suit_varia, 4: _next_suit_default,
                     5: _next_suit_dark, 6: _next_suit_light, 7: _next_suit_dark})


# --------------------------------------------------- fn_61_FDC GetSuitCharIdx
# 0x512's outer `charIdx >= mCharacterCount` and 0x514's inner one: a count of 0 takes both true,
# a count of 1 with the suit forced to kPS_Varia (0) takes the outer true and the inner false.
def _suit_count0(gdb, st):
    _save(st, 'mCharacterCount', '0')(gdb, st)


def _suit_arg(gdb, st, suit, count):
    cur = _val('$rdx')
    if cur is not None:
        st.regs.append(('$rdx', str(cur)))
        _set(gdb, '$rdx = %d' % suit)
    _save(st, 'mCharacterCount', str(count))(gdb, st)


def _suit_varia1(gdb, st):
    _suit_arg(gdb, st, 0, 1)   # suit Varia, one character: outer true only if renderSuit > 0


def _suit_dark2(gdb, st):
    _suit_arg(gdb, st, 1, 2)   # suit Dark, two characters


def _suit_light1(gdb, st):
    _suit_arg(gdb, st, 2, 1)


suit_char = OneShot({3: _suit_count0, 6: _suit_varia1, 12: _suit_dark2, 20: _suit_light1,
                     30: _suit_varia1, 50: _suit_dark2, 80: _suit_light1})


# ------------------------------------------------------- fn_61_C30 PreRender
# 0x530 `mProjectedShadow.get() != nullptr && HasModelData() && HasGunModelData()` and 0x535's
# `HasGrappleModelData() ? 3 : 2`.
def _pre_no_shadow(gdb, st):
    _save(st, 'mProjectedShadow.mPtr', '0')(gdb, st)


def _pre_no_gun(gdb, st):
    _save(st, 'mBeamModelData.mPtr', '0')(gdb, st)


pre_render = OneShot({3: _pre_no_shadow, 6: _pre_no_gun, 40: _pre_no_gun})


# ------------------------------------------------ fn_61_AB8 SetupEnvFx (set in $rdx)
# 0x553's `!(mFlags & 0x20)` arm and the rain block (0x554: the world's needed env fx, the
# manager's rain magnitude).
def _env_no_shadow_flag(gdb, st):
    _save(st, 'mFlags', str(_val(THIS + '->mFlags') | 0x20))(gdb, st)


def _env_rain(gdb, st):
    fx = _read(MGR + '->mWorld.mPtr->mNeededEnvFx')
    if fx is not None:
        st.restore.append('((CStateManager*)$rsi)->mWorld.mPtr->mNeededEnvFx = %d' % int(fx))
        _set(gdb, '((CStateManager*)$rsi)->mWorld.mPtr->mNeededEnvFx = 2')  # kEFX_Rain
    dens = _read(MGR + '->mEnvFxManager->mFxDensity')
    if dens is not None:
        st.restore.append('((CStateManager*)$rsi)->mEnvFxManager->mFxDensity = %.9g'
                          % float(dens))
        _set(gdb, '((CStateManager*)$rsi)->mEnvFxManager->mFxDensity = 1.0')


def _env_rain_set(gdb, st):
    if _val('$rdx') == 1:
        _env_rain(gdb, st)


setup_envfx = OneShot({3: _env_no_shadow_flag, 5: _env_rain_set, 8: _env_rain_set,
                       12: _env_rain_set, 16: _env_rain_set})


# -------------------------------------------- fn_61_970 SetIntoStateManager
# 0x580's `actor &&` and `actor->IsPlayerActor()` arms, forced on the loop's locals.
_sia = {'n': 0}


def sia_actor_null(gdb):
    # loc: line 579: an invalid head id makes ObjectById answer null, so the cast at 579 does
    _sia['n'] += 1
    if _sia['n'] == 2:
        _set(gdb, 'id.value = 0xffff')


_sia_seen = []


def sia_not_player(gdb):
    # loc: line 580: clear the *other* player actor's mIsPlayerActor bit, so the walk from the
    # head reaches it, the cast succeeds and IsPlayerActor() answers false (the else at 583)
    _sia['n'] += 1
    me = _val('$rdi')
    if me is not None and me not in _sia_seen:
        _sia_seen.append(me)
    if _sia['n'] == 5:
        for other in _sia_seen:
            if other != me:
                st = _val('((CScriptActor*)%d)->mIsPlayerActor' % other)
                if st is not None:
                    _set(gdb, '((CScriptActor*)%d)->mIsPlayerActor = 0' % other)
                break


# ------------------------------------------------ fn_61_C0 LoadPlayerActor
_load = {'acs': 0, 'solid': 0, 'gun': 0, 'gun2': 0}


def load_acs(gdb):
    # loc: line 600: an unknown ACS file makes GetResourceTypeById answer 0
    _load['acs'] += 1
    if _load['acs'] == 2:
        _set(gdb, 'sldrThis.animationInformation.ancs = 0xffffffff')


def load_solid(gdb):
    # loc: line 613: the isSolid material arm
    _load['solid'] += 1
    if _load['solid'] == 2:
        _set(gdb, 'sldrThis.isSolid = 1')


def load_gun_override(gdb):
    # loc: line 621: the switch's default arm (a render-gun override with no beam)
    _load['gun'] += 1
    if _load['gun'] in (2, 4, 6, 8):
        _set(gdb, 'sldrThis.renderGunOverride = 7')


def load_gun_before(gdb):
    # loc: line 620, one line earlier, so the value is written before the switch reads it
    _load['gun2'] += 1
    if _load['gun2'] in (3, 5, 7):
        _set(gdb, 'sldrThis.renderGunOverride = 7')


# ------------------------------------------------------- fn_61_2630 Think
def _think_beam(gdb, st):
    _save(st, 'mUsePlayerBeamModel', '1')(gdb, st)
    _save(st, 'mBeamModelData.mPtr', '0')(gdb, st)


def _think_beam_invalid(gdb, st):
    _save(st, 'mUsePlayerBeamModel', '0')(gdb, st)
    _save(st, 'mBeam', '-1')(gdb, st)


def _think_grapple_loading(gdb, st):
    _save(st, 'mGrappleModelLoading', '1')(gdb, st)


def _think_loading_block(gdb, st):
    # 0x221/0x226: no loading flag set and the model not loaded yet
    for m in ('mSuitModelLoading', 'mBeamModelLoading', 'mGrappleModelLoading', 'mLoading'):
        _save(st, m, '0')(gdb, st)
    _save(st, 'mSentArrived', '0')(gdb, st)


def _think_area_tracking(gdb, st):
    # 0x228: the area-tracking decrement
    for m in ('mSuitModelLoading', 'mBeamModelLoading', 'mGrappleModelLoading'):
        _save(st, m, '0')(gdb, st)
    _save(st, 'mLoading', '1')(gdb, st)
    _save(st, 'mAreaTrackingLoad', '1')(gdb, st)
    _save(st, 'mSentArrived', '0')(gdb, st)


def _think_flags(gdb, st):
    # 0x185: the suit-swap and no-recheck flags skip the char-index block
    _save(st, 'mFlags', str(_val(THIS + '->mFlags') | 1))(gdb, st)


think = OneShot({5: _think_beam, 15: _think_beam_invalid, 25: _think_grapple_loading,
                 40: _think_loading_block, 60: _think_area_tracking, 90: _think_flags})


CLASS = 'CScriptPlayerActor'
FORCE = [
    ('103C GetNextSuitCharIdx', 'CScriptPlayerActor::GetNextSuitCharIdx(', [], [next_suit], 0,
     'mFlags 0x2 with the Light suit, then Varia, then Light: the 0x489 and switch arms'),
    ('FDC GetSuitCharIdx', 'CScriptPlayerActor::GetSuitCharIdx(', [], [suit_char], 0,
     'mCharacterCount 0 (both count arms) then 1 with suit 0 (the outer arm, inner false)'),
    ('C30 PreRender', 'CScriptPlayerActor::PreRender(', [], [pre_render], 0,
     'shadow null, gun model data null, grapple data aliased to the beam data (the ?: arm)'),
    ('AB8 SetupEnvFx', 'CScriptPlayerActor::SetupEnvFx(', [], [setup_envfx], 0,
     'mFlags 0x20 (no shadow) and the world set to kEFX_Rain with a rain magnitude'),
    ('2630 Think', 'CScriptPlayerActor::Think(', [], [think], 0,
     'use-player-beam with no beam data, invalid beam, grapple loading, the loading blocks'),
    ('970 SetIntoStateManager', 'loc:CScriptPlayerActor.cpp:580', [], [sia_actor_null], 0, ''),
    ('970 SetIntoStateManager not a player actor', 'loc:CScriptPlayerActor.cpp:580', [],
     [sia_not_player], 0, ''),
    ('C0 LoadPlayerActor unknown ACS', 'loc:CScriptPlayerActor.cpp:599', [], [load_acs], 0, ''),
    ('C0 LoadPlayerActor gun override (620)', 'loc:CScriptPlayerActor.cpp:620', [],
     [load_gun_before], 0, ''),
    ('C0 LoadPlayerActor solid', 'loc:CScriptPlayerActor.cpp:613', [], [load_solid], 0, ''),
    ('C0 LoadPlayerActor gun override', 'loc:CScriptPlayerActor.cpp:621', [],
     [load_gun_override], 0, ''),
]
