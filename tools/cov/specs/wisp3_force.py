# WispTentacle (REL 86), wisp3: forced arms of Think, UpdateGrabbedPirate, AcceptScriptMsg and the ctor.
# Instruction-level breakpoints (`loc:*SYMBOL+off`) write registers/members just before a test; every
# force is one-shot per listed hit and logged to <out>/forced.log. Offsets are for build-port-cov at
# 92bcb4775 (re-check with `gdb -batch -ex 'disassemble /s SYMBOL'` after a rebuild).
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/wisp3_force.py <out> \
#     3bfa3eff,0t_temple_hall_dark --con tools/drivers/wisp3_hall.con --fast --frames 7900
import os

import gdb

THIS = '((CWispTentacle*)$rdi)'


def _log(msg):
    try:
        with open(os.environ['COV_OUT'] + '/forced.log', 'a') as fh:
            fh.write(msg + '\n')
    except Exception:
        pass


def _set(g, expr):
    try:
        g.execute('set var ' + expr)
    except Exception as e:  # noqa: BLE001
        _log('set failed (%s): %s' % (expr, str(e)[:70]))


def _read(expr):
    try:
        return gdb.parse_and_eval(expr)
    except Exception as e:  # noqa: BLE001
        _log('read failed (%s): %s' % (expr, str(e)[:60]))
        return None


def _sym(demangled_prefix):
    for ln in open(os.environ['COV_NM']):
        parts = ln.rstrip('\n').split('\t')
        if len(parts) == 2:
            f = parts[0].split()
            if len(f) == 3 and f[1] in 'tTW' and parts[1].startswith(demangled_prefix) \
                    and '.cold' not in parts[1] and 'clone' not in parts[1]:
                return f[2]
    return None


def _save(st, member, value, base=THIS):
    old = _read(base + '->' + member)
    if old is None:
        return
    st.restore.append('%s->%s = %s' % (base, member, str(old)))
    _set(gdb, '%s->%s = %s' % (base, member, value))


class OneShot:
    def __init__(self, hits):
        self.n = 0
        self.hits = hits
        self.restore = []

    def __call__(self, g):
        for expr in self.restore:
            _set(g, expr)
        self.restore = []
        self.n += 1
        fn = self.hits.get(self.n)
        if fn is not None:
            fn(g, self)


# ---- Think (fn_86_13D4): 558 inactive arm, 590 mPirateCaptured with no pirate (UpdateGrabbedPirate's 479)
think = OneShot({4: lambda g, st: _save(st, 'mActive', '0'),
                 9: lambda g, st: _save(st, 'mPirateCaptured', '1'),
                 15: lambda g, st: _save(st, 'mActive', '0'),
                 25: lambda g, st: _save(st, 'mPirateCaptured', '1')})

# 576/585 `GetLocatorSegId(..).val() != 0xff` false arms, first uncached Think only.
_loc = {'n1': 0, 'n2': 0}


def think_lockon(g):
    _loc['n1'] += 1
    if _loc['n1'] == 1:
        _set(g, '$r13 = 0xff')
        _log('13D4 lockOnSeg forced to 0xff (576)')


def think_armend(g):
    _loc['n2'] += 1
    if _loc['n2'] == 1:
        _set(g, '*(unsigned char*)($rsp+0x60) = 0xff')
        _log('13D4 armEndSeg forced to 0xff (585)')


# ---- AcceptScriptMsg (fn_86_2A18)
def _msg():
    m = _read('((CScriptMsg*)$rdx)->m_msg')
    return None if m is None else int(m) & 0xFFFFFFFF


_lights = {'restore': None, 'n': 0}


def as_lights(g):
    # 242 kSM_Activate's `if (ActorLights())` false arm (Create's 211 is followed by an unguarded
    # ActorLights()-> use, so only Activate is forced)
    if _lights['restore'] is not None:
        for e in _lights['restore']:
            _set(g, e)
        _lights['restore'] = None
    if _msg() == 0x41435456:
        _lights['n'] += 1
        if _lights['n'] in (1, 3):
            item = _read(THIS + '->mActorLights.mPtr')
            if item is None:
                return
            _lights['restore'] = ['%s->mActorLights.mPtr = %s' % (THIS, str(item))]
            _set(g, '%s->mActorLights.mPtr = 0' % THIS)
            _log('2A18 Activate with ActorLights() null (242)')


_coll = {'restore': None, 'addr': None}


def as_null_collision(g):
    m = _msg()
    if m == 0x58415544:
        if _coll['restore'] is None:
            old = _read(THIS + '->mCollisionManager.mPtr')
            if old is not None:
                _coll['restore'] = str(old)
                _coll['addr'] = int(_read('$rdi'))
                _set(g, '%s->mCollisionManager.mPtr = 0' % THIS)
                _log('2A18 AIUpdateDisabled with mCollisionManager null (253)')
    elif _coll['restore'] is not None:
        think_restore_collision(g)


_touch = {'addr': None, 'old': None, 'hit': 0, 'dmg': 0}
BOMB_UID = 0x6ab  # `objs CBomb` in tools/drivers/wisp3_hall.con's bomb window (ids repeat run to run)


def _last_touched(g, value_expr):
    # rewrite the sender CCollisionActor's mLastTouched (HitObject/Damage read it back), undone at the next entry
    sender = _read('((CScriptMsg*)$rdx)->mSenderId.value')
    if sender is None:
        return
    try:
        g.execute('set $ent = ((void*(*)(void*,unsigned short))_ZN13CStateManager10ObjectByIdE9TUniqueId)($rsi, %d)'
                  % int(sender))
        addr = int(_read('(long)$ent'))
        if addr == 0:
            return
        _touch['old'] = int(_read('((CCollisionActor*)%d)->mLastTouched.value' % addr))
        _touch['addr'] = addr
        _set(g, '((CCollisionActor*)%d)->mLastTouched.value = %s' % (addr, value_expr))
    except Exception as e:  # noqa: BLE001
        _log('lastTouched failed: %s' % str(e)[:80])


def as_touch(g):
    if _touch['addr'] is not None:
        _set(g, '((CCollisionActor*)%d)->mLastTouched.value = %d' % (_touch['addr'], _touch['old']))
        _touch['addr'] = None
    m = _msg()
    if m == 0x58484954:  # HitObject: the sender "touched" the player
        _touch['hit'] += 1
        _last_touched(g, '((CStateManager*)$rsi)->mPlayers[0]->mUniqueId.value')
        _log('2A18 HitObject sender mLastTouched forced to the player (225)')
    elif m == 0x58444d47:  # Damage: it "touched" a bomb
        _touch['dmg'] += 1
        _last_touched(g, str(BOMB_UID))
        _log('2A18 Damage sender mLastTouched forced to bomb %#x (266)' % BOMB_UID)


def think_restore_collision(g):
    # Think dereferences the manager unguarded: put it back before any body runs
    if _coll['restore'] is not None:
        _set(g, '((CWispTentacle*)%d)->mCollisionManager.mPtr = %s' % (_coll['addr'], _coll['restore']))
        _coll['restore'] = None


# ---- ctor (fn_86_3628): instruction-level forces
_c = {'portal': 0, 'this': None, 'old': None, 'inf': 0, 'eps': 0}


def ctor_portal(g):
    _c['portal'] += 1
    if _c['portal'] == 1:
        rbx = int(_read('$rbx'))
        _c['this'] = rbx
        _c['old'] = int(_read('*(unsigned char*)($rbx+0x8a9)'))
        _set(g, '*(unsigned char*)($rbx+0x8a9) = 1')
        _log('3628 mSpawnFromPortal forced 1 (110)')


def ctor_join(g):
    if _c['this'] is not None:
        _set(g, '*(unsigned char*)(%d+0x8a9) = %d' % (_c['this'], _c['old']))
        _c['this'] = None


def ctor_inf(g):
    _c['inf'] += 1
    if _c['inf'] in (1, 5):
        _set(g, '$al = 0')
        _log('3628 CCharAnimTime != Infinity forced false (124)')


def ctor_eps(g):
    _c['eps'] += 1
    if _c['eps'] in (1, 4):
        _set(g, '$xmm0.v4_float[0] = 0')
        _log('3628 best.first forced 0 (127)')


CLASS = 'CWispTentacle'
_ctor = _sym('CWispTentacle::CWispTentacle(')
_think = _sym('CWispTentacle::Think(')
FORCE = [
    ('13D4 Think inactive/captured', 'CWispTentacle::Think(', [], [think], 0,
     'mActive clear (558), mPirateCaptured set with no pirate (590 -> 479)'),
    ('13D4 Think restore', 'CWispTentacle::Think(', [], [think_restore_collision], 0, 'restore'),
    ('13D4 Think lockon', 'loc:*' + _think + '+0x2a1', [], [think_lockon], 0, 'lockOnSeg 0xff'),
    ('13D4 Think armend', 'loc:*' + _think + '+0x2f4', [], [think_armend], 0, 'armEndSeg 0xff'),
    ('2A18 lights', 'CWispTentacle::AcceptScriptMsg(', [], [as_lights, as_null_collision], 0,
     'Activate without lights, AIUpdateDisabled without manager'),
    ('3628 portal', 'loc:*' + _ctor + '+0x243', [], [ctor_portal], 0, 'portal arm'),
    ('3628 join', 'loc:*' + _ctor + '+0x392', [], [ctor_join], 0, 'restore'),
    ('3628 inf', 'loc:*' + _ctor + '+0x4e3', [], [ctor_inf], 0, 'time == Infinity'),
    ('3628 eps', 'loc:*' + _ctor + '+0x544', [], [ctor_eps], 0, 'best <= eps'),
]
