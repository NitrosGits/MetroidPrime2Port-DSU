# SwarmBasics (module 80) swb10: fn_80_1C78 CSwarmBasics::FindBestLockOnIndex, the boid-geometry
# arms of its loop.  force.py, one fixture per call, everything restored when the call returns:
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swarmbasics_lockon.py <out> \
#     3dfd2249,0g_swamp_hall --con tools/cov/specs/swarmbasics_swamp.con --fast --frames 1200
#
# At the entry of every FindBestLockOnIndex call (this = $rdi, mgr = $rsi - the function's only
# argument), one existing element of $this->mBoids is used as the candidate and its ABSOLUTE
# translation is written for the duration of the call, then restored by a finish breakpoint:
#
#   phase 0  the boid's translation is set to the camera's own (exact same floats), so
#            delta = 0.0f -> MagSquared 0 <= maxDistanceSq -> CanBeNormalized() false (the arm the
#            module note lists as never taken: "a boid exactly at the camera").
#   phase 1  translation = camera + 1e6 on X -> delta.MagSquared() > maxDistanceSq -> `continue`.
#   phase 2  translation = camera + 1.0 * the camera's own right vector -> normalizable, dot = 0
#            with the camera forward, i.e. not > maxDot (0.5f) -> the `dot > maxDot` false side.
#
# The camera is read the way the body reads it: mgr->mCameraManagers[0]->mFpCamera->mTransform
# (CStateManager.hpp GetCameraManager(0), CCameraManager.hpp GetFirstPersonCamera(), and
# GetTransform() = CActor::mTransform), then GetTranslation() = (m03, m13, m23),
# GetForward() = (m01, m11, m21), GetRight() = (m00, m10, m20) from CTransform4f.hpp.  Names and
# the chain were checked against the build's DWARF (`ptype`/`p &((T*)0)->member`) before any run;
# no retail offset is used, the host layout differs from it.
#
# The candidate element is the first element that is already mActive, else element 0 (and then
# mActive is set for the call and restored afterwards): a boid-state write, no array/count/model
# write and no condition stub.  Every other element is left as it is, so the loop still sees both
# the active and the inactive side of its `if (it->mActive)`.
CLASS = 'CSwarmBasics'

import os

import gdb  # this spec is exec'd by tools/cov/force.py inside gdb's python

_OUT = os.environ.get('COV_OUT', '/tmp')
_LOG = os.path.join(_OUT, 'swb10_geom.log')

_st = {'n': 0, 'apply': 0, 'err': 0, 'noCam': 0, 'noBoid': 0}


def _log(msg):
    try:
        with open(_LOG, 'a') as fh:
            fh.write(msg + '\n')
    except Exception:
        pass


def _f(g, expr):
    return float(g.parse_and_eval(expr))


def _i(g, expr):
    return int(g.parse_and_eval(expr))


class _Restore(gdb.FinishBreakpoint):
    """Puts the candidate boid's absolute translation (and mActive) back after the call."""

    def __init__(self, frame, info):
        super().__init__(frame, internal=True)
        self.info = info

    def stop(self):
        info = self.info
        try:
            b = info['b']
            gdb.execute('set var %s.mTransform.m03 = %r' % (b, info['pos'][0]))
            gdb.execute('set var %s.mTransform.m13 = %r' % (b, info['pos'][1]))
            gdb.execute('set var %s.mTransform.m23 = %r' % (b, info['pos'][2]))
            gdb.execute('set var %s.mActive = %d' % (b, info['active']))
            try:
                ret = int(gdb.parse_and_eval('$rax'))
            except Exception:
                ret = -99
            _log('RESTORE %s idx=%d pos=(%.5f,%.5f,%.5f) active=%d ret=%d'
                 % (info['tag'], info['idx'], info['pos'][0], info['pos'][1], info['pos'][2],
                    info['active'], ret))
        except Exception as e:
            _log('RESTORE %s FAILED: %s' % (info['tag'], e))
        return False


def lockon_fixture(g):
    """Move one existing boid of this swarm to the geometry the loop arm needs, for one call."""
    st = _st
    st['n'] += 1
    try:
        this = _i(g, '$rdi')
        mgr = _i(g, '$rsi')
        count = _i(g, '((CSwarmBasics*)%d)->mBoids.mCount' % this)
        items = _i(g, '((CSwarmBasics*)%d)->mBoids.mItems' % this)
    except Exception as e:
        if st['err'] < 4:
            _log('entry read failed: %s' % e)
        st['err'] += 1
        return
    if this <= 0 or mgr <= 0 or count <= 0 or items <= 0:
        if st['noBoid'] < 4:
            _log('swarm 0x%x has no boids (count=%d items=0x%x) - nothing to move'
                 % (this, count, items))
        st['noBoid'] += 1
        return
    try:
        cam = _i(g, '(long)((CStateManager*)%d)->mCameraManagers[0]' % mgr)
        fp = _i(g, '(long)((CCameraManager*)%d)->mFpCamera' % cam) if cam else 0
    except Exception as e:
        if st['err'] < 4:
            _log('camera chain failed: %s' % e)
        st['err'] += 1
        return
    if not cam or not fp:
        if st['noCam'] < 4:
            _log('no first-person camera yet (manager=0x%x camera=0x%x)' % (cam, fp))
        st['noCam'] += 1
        return
    xf = '((CActor*)%d)->mTransform' % fp
    try:
        cpos = (_f(g, xf + '.m03'), _f(g, xf + '.m13'), _f(g, xf + '.m23'))
        right = (_f(g, xf + '.m00'), _f(g, xf + '.m10'), _f(g, xf + '.m20'))
    except Exception as e:
        if st['err'] < 4:
            _log('camera transform read failed: %s' % e)
        st['err'] += 1
        return
    # the candidate: an element that is already active, else element 0 (activated for the call)
    idx = None
    active = 0
    try:
        for k in range(count):
            if _i(g, '((CSwarmBasics*)%d)->mBoids.mItems[%d].mActive' % (this, k)) != 0:
                idx = k
                active = 1
                break
        if idx is None:
            idx = 0
            active = 0
        b = '((CSwarmBasics*)%d)->mBoids.mItems[%d]' % (this, idx)
        pos = (_f(g, b + '.mTransform.m03'), _f(g, b + '.mTransform.m13'),
               _f(g, b + '.mTransform.m23'))
    except Exception as e:
        if st['err'] < 4:
            _log('boid read failed: %s' % e)
        st['err'] += 1
        return
    phase = st['apply'] % 3
    if phase == 0:
        target = cpos
    elif phase == 1:
        target = (cpos[0] + 1.0e6, cpos[1], cpos[2])
    else:
        target = (cpos[0] + right[0], cpos[1] + right[1], cpos[2] + right[2])
    tag = 'swb10-%d-ph%d' % (st['apply'] + 1, phase)
    try:
        setv = lambda e, v: g.execute('set var %s = %s' % (e, v))
        setv(b + '.mTransform.m03', repr(target[0]))
        setv(b + '.mTransform.m13', repr(target[1]))
        setv(b + '.mTransform.m23', repr(target[2]))
        setv(b + '.mActive', '1')
    except Exception as e:
        _log('APPLY %s write failed: %s' % (tag, e))
        return
    st['apply'] += 1
    _log('APPLY %s boid=0x%x idx=%d of %d was_active=%d camera=0x%x cam_pos=(%.5f,%.5f,%.5f)'
         ' target=(%.5f,%.5f,%.5f) saved=(%.5f,%.5f,%.5f)'
         % (tag, this, idx, count, active, fp, cpos[0], cpos[1], cpos[2],
            target[0], target[1], target[2], pos[0], pos[1], pos[2]))
    try:
        _Restore(g.newest_frame(), {'b': b, 'idx': idx, 'pos': pos, 'active': active,
                                    'tag': tag})
    except Exception as e:
        _log('RESTORE %s could not be armed: %s' % (tag, e))


FORCE = [
    ('1C78 lock-on geometry', 'CSwarmBasics::FindBestLockOnIndex(', [], [lockon_fixture], 0,
     'one existing boid of the swarm is put exactly at the camera (delta 0: CanBeNormalized'
     ' false), beyond the orbit distance (the continue arm) and at the camera\'s right (a'
     ' normalizable delta with dot 0 <= maxDot), restored when the call returns'),
]
