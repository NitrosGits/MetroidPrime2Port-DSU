# WispTentacle (REL 86) forcing spec: the data/state arms of the bodies wisp1 left unmarked.
# One call per write, the old value put back at the next entry of the same body.
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/wisp2_force.py <out> \
#     3bfa3eff,0t_temple_hall_dark --con tools/drivers/wisp2_hall.con --fast --frames 6200
import os

import gdb

THIS = '((CWispTentacle*)$rdi)'


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


def _save(st, member, value, base=THIS):
    old = _read(base + '->' + member)
    if old is None:
        return
    st.restore.append('%s->%s = %s' % (base, member, str(old)))
    _set(gdb, '%s->%s = %s' % (base, member, value))


class OneShot:
    def __init__(self, hits, what=''):
        self.n = 0
        self.hits = hits
        self.what = what
        self.restore = []

    def __call__(self, gdb):
        for expr in self.restore:
            _set(gdb, expr)
        self.restore = []
        self.n += 1
        fn = self.hits.get(self.n)
        if fn is not None:
            fn(gdb, self)


# ----------------------------------------------------- fn_86_1780 PreRender
# 0x542's `if (mSpawnFromPortal)`: no shipped placement spawns from a portal.
pre_render = OneShot({2: lambda gdb, st: _save(st, 'mSpawnFromPortal', '1'),
                      5: lambda gdb, st: _save(st, 'mSpawnFromPortal', '1'),
                      9: lambda gdb, st: _save(st, 'mSpawnFromPortal', '1')})


# ---------------------------------------------------------- fn_86_13D4 Think
# 0x557's inactive arm, 0x589's `mPirateCaptured` arm (UpdateGrabbedPirate then runs and returns at
# its own 0x479 because mPirateId is no pirate), and 0x575/0x584's locator-not-found arms.
def _think_inactive(gdb, st):
    _save(st, 'mActive', '0')


def _think_captured(gdb, st):
    _save(st, 'mPirateCaptured', '1')


think = OneShot({4: _think_inactive, 9: _think_captured, 15: _think_inactive,
                 25: _think_captured})


def think_lockon(gdb):
    # loc: 575 `if (lockOnSeg.val() != 0xff)`: the negative arm (the model has no such locator)
    think_lockon.n += 1
    if think_lockon.n in (2, 4, 6):
        _set(gdb, 'lockOnSeg.mId = 255')


think_lockon.n = 0


def think_armend(gdb):
    # loc: 584, the same for the arm-end locator
    think_armend.n += 1
    if think_armend.n in (2, 4, 6):
        _set(gdb, 'armEndSeg.mId = 255')


think_armend.n = 0


# --------------------------------------------------- fn_86_2A18 AcceptScriptMsg
# 0x211/0x242 `if (ActorLights())` false arm, 0x253 `if (mCollisionManager.get())` false arm on the
# AIUpdateDisabled message, and the Damage gate (0x259) with the body out of the sleep states.
def _as_lights(gdb, st):
    # the auto_ptr is two words: the pointer and its has-value flag
    item = _read(THIS + '->mActorLights.mItem')
    flags = _read(THIS + '->mActorLights.mHas')
    if item is None or flags is None:
        return
    st.restore.append('%s->mActorLights.mItem = %s' % (THIS, str(item)))
    st.restore.append('%s->mActorLights.mHas = %s' % (THIS, str(flags)))
    _set(gdb, '%s->mActorLights.mItem = 0' % THIS)
    _set(gdb, '%s->mActorLights.mHas = 0' % THIS)


accept_msg = OneShot({3: _as_lights, 12: _as_lights, 30: _as_lights})

_coll = {'restore': None}


def as_null_collision(gdb):
    # 0x3448 (AIUpdateDisabled = "XAUD" = 0x58415544): the manager is null for exactly that message
    msg = _read('((CScriptMsg*)$rdx)->m_msg')
    if msg is None:
        return
    if int(msg) & 0xFFFFFFFF == 0x58415544:
        if _coll['restore'] is None:
            old = _read(THIS + '->mCollisionManager.mPtr')
            if old is not None:
                _coll['restore'] = str(old)
                _set(gdb, '%s->mCollisionManager.mPtr = 0' % THIS)
    elif _coll['restore'] is not None:
        _set(gdb, '%s->mCollisionManager.mPtr = %s' % (THIS, _coll['restore']))
        _coll['restore'] = None


# -------------------------------------------------- fn_86_3628 the constructor
# 0x3xxx `if (mSpawnFromPortal)` (line 110): the portal arm of the placement.
ctor = OneShot({1: lambda gdb, st: _save(st, 'mSpawnFromPortal', '1')})


# ------------------------------------------- fn_86_13D4 Think's locator arms
# 0x575/0x584 test `GetLocatorSegId(name).val() != 0xff`. The CSegId is a register local the -O2 build
# keeps in a register, so the value is forced at its source: a return breakpoint on the call that
# produces it, restricted to calls made from Think.
_loc = {'on': False, 'installed': False}


class _LocRet(gdb.FinishBreakpoint):
    def __init__(self, frame):
        super().__init__(frame, internal=True)

    def stop(self):
        try:
            gdb.execute('set $rax = 0xff')
        except Exception:  # noqa: BLE001
            pass
        return False


class _LocBp(gdb.Breakpoint):
    def __init__(self):
        super().__init__('CAnimData::GetLocatorSegId(', internal=True)

    def stop(self):
        if not _loc['on']:
            return False
        try:
            caller = gdb.newest_frame().older()
            name = caller.name() or '' if caller is not None else ''
        except Exception:  # noqa: BLE001
            name = ''
        if 'CWispTentacle::Think' in name:
            try:
                _LocRet(gdb.newest_frame())
            except Exception:  # noqa: BLE001
                pass
        return False


def think_locators(gdb):
    if not _loc['installed']:
        _LocBp()
        _loc['installed'] = True
        _log('13D4 GetLocatorSegId return forced to 0xff (Think only)')
    _loc['on'] = True


CLASS = 'CWispTentacle'
FORCE = [
    ('1780 PreRender portal', 'CWispTentacle::PreRender(', [], [pre_render], 0,
     'mSpawnFromPortal set: the model-flag arm at 0x542'),
    ('13D4 Think inactive/captured', 'CWispTentacle::Think(', [], [think], 0,
     'mActive clear (0x557) and mPirateCaptured set (0x589)'),
    ('13D4 Think locator returns', 'CWispTentacle::Think(', [], [think_locators], 0,
     'GetLocatorSegId answers 0xff: the locator-not-found arms at 0x575/0x584'),
]
