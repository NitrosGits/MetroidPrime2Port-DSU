# SwarmBasics (module 80) swb11: fn_80_1978 CSwarmBasics::AssignSeekerBoids, the boid-geometry
# arms and the caller's taken/numNeeded/out loop.  force.py, one fixture per call, restored at
# the call's return:
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swarmbasics_seeker.py <out> \
#     3dfd2249,0g_swamp_hall --driver 'python3 tools/drivers/con.py {port} tools/drivers/swb11_seeker.con' \
#     --fast --frames 1200
#
# The caller is the real one (UpdateSeekerTargets -> AssignSeekerBoids(mgr, keptBoids, numLost,
# newBoids) every frame), so `taken`, `numNeeded` and `out` are the caller's own vectors: this spec
# never writes them, never resizes them and never invents a descriptor.  It only moves ONE existing
# element of $this->mBoids for the duration of the call and restores its absolute translation and
# mActive from a finish breakpoint:
#
#   phase 0  the boid is put beyond the orbit distance (camera + 1e6 on X) -> the
#            `delta.MagSquared() > maxDistSq` true side (the `continue`).
#   phase 1  the boid is put exactly at the camera (the camera's own floats) -> delta == 0 ->
#            `CanBeNormalized()` false.
#   phase 2  nothing is moved (the natural swarm), so the seeker lock the driver produces is what
#            decides `taken`: the swarm's own 5 seeker-target actors are real objects, and once the
#            gun locks one (the driver holds the use-item button with the launcher given), the
#            caller's keptBoids is non-empty and the taken-scan arms run.
#
# Camera: mgr->mCameraManagers[0]->mFpCamera->mTransform (GetCameraManager(0),
# GetFirstPersonCamera(), GetTransform() = CActor::mTransform), translation = (m03, m13, m23) of
# CTransform4f.  Names verified against the build's DWARF before the run; the host layout differs
# from retail's, so no retail offset is used.
#
# NOTE on the log label (correction): the swb10 spec's APPLY line labelled a field `boid=` but its
# printf argument was `this`, the swarm actor pointer (see swarmbasics_lockon.py, now labelled
# `swarm=`); the archived /tmp/opencode/swb10 logs keep the old label and are not rewritten. This
# spec logs the swarm as `swarm=` and the moved element as `boid=` (the address of $this->mBoids[idx]).
CLASS = 'CSwarmBasics'

import os

import gdb  # this spec is exec'd by tools/cov/force.py inside gdb's python

_OUT = os.environ.get('COV_OUT', '/tmp')
_LOG = os.path.join(_OUT, 'swb11_geom.log')

_VEC = 'rstl::vector<unsigned int, rstl::rmemory_allocator>'

_st = {'n': 0, 'apply': 0, 'err': 0, 'takenSeen': 0, 'logN': 0}


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
    """Puts the moved boid's absolute translation and mActive back after the call."""

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
            _log('RESTORE %s pos=(%.5f,%.5f,%.5f) active=%d'
                 % (info['tag'], info['pos'][0], info['pos'][1], info['pos'][2], info['active']))
        except Exception as e:
            _log('RESTORE %s FAILED: %s' % (info['tag'], e))
        return False


def assign_fixture(g):
    st = _st
    st['n'] += 1
    try:
        this = _i(g, '$rdi')
        mgr = _i(g, '$rsi')
        taken = _i(g, '$rdx')
        needed = _i(g, '$rcx')
        out = _i(g, '$r8')
        count = _i(g, '((CSwarmBasics*)%d)->mBoids.mCount' % this)
        items = _i(g, '((CSwarmBasics*)%d)->mBoids.mItems' % this)
    except Exception as e:
        if st['err'] < 4:
            _log('entry read failed: %s' % e)
        st['err'] += 1
        return
    tcount = -1
    ocount = -1
    if taken:
        try:
            tcount = _i(g, '((%s*)%d)->mCount' % (_VEC, taken))
        except Exception:
            tcount = -2
    if out:
        try:
            ocount = _i(g, '((%s*)%d)->mCount' % (_VEC, out))
        except Exception:
            ocount = -2
    if tcount > 0:
        if st['takenSeen'] < 5:
            _log('TAKEN non-empty: %d entries, numNeeded=%d out.size=%d' % (tcount, needed, ocount))
        st['takenSeen'] += 1
    phase = st['n'] % 3
    if phase == 2 or count <= 0 or items <= 0:
        if st['logN'] % 100 == 0:
            _log('CALL %d phase=%d taken=%d needed=%d out=%d boids=%d (no move)'
                 % (st['n'], phase, tcount, needed, ocount, count))
        st['logN'] += 1
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
        if st['err'] < 4:
            _log('no first-person camera yet (manager=0x%x camera=0x%x)' % (cam, fp))
        st['err'] += 1
        return
    xf = '((CActor*)%d)->mTransform' % fp
    try:
        cpos = (_f(g, xf + '.m03'), _f(g, xf + '.m13'), _f(g, xf + '.m23'))
    except Exception as e:
        if st['err'] < 4:
            _log('camera transform read failed: %s' % e)
        st['err'] += 1
        return
    idx = 0
    b = '((CSwarmBasics*)%d)->mBoids.mItems[%d]' % (this, idx)
    try:
        pos = (_f(g, b + '.mTransform.m03'), _f(g, b + '.mTransform.m13'),
               _f(g, b + '.mTransform.m23'))
        active = _i(g, b + '.mActive')
        boid = _i(g, '&' + b)
    except Exception as e:
        if st['err'] < 4:
            _log('boid read failed: %s' % e)
        st['err'] += 1
        return
    if phase == 0:
        target = (cpos[0] + 1.0e6, cpos[1], cpos[2])
    else:
        target = cpos
    tag = 'swb11-%d-ph%d' % (st['apply'] + 1, phase)
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
    _log('APPLY %s swarm=0x%x boid=0x%x idx=%d of %d was_active=%d taken=%d needed=%d out=%d'
         ' camera=0x%x cam_pos=(%.5f,%.5f,%.5f) target=(%.5f,%.5f,%.5f) saved=(%.5f,%.5f,%.5f)'
         % (tag, this, boid, idx, count, active, tcount, needed, ocount, fp,
            cpos[0], cpos[1], cpos[2], target[0], target[1], target[2],
            pos[0], pos[1], pos[2]))
    try:
        _Restore(g.newest_frame(), {'b': b, 'pos': pos, 'active': active, 'tag': tag})
    except Exception as e:
        _log('RESTORE %s could not be armed: %s' % (tag, e))


FORCE = [
    ('1978 seeker geometry', 'CSwarmBasics::AssignSeekerBoids(', [], [assign_fixture], 0,
     'one existing boid of the swarm is put beyond the orbit distance (the continue arm) and'
     ' exactly at the camera (CanBeNormalized false), restored when the call returns; the'
     ' taken/numNeeded/out vectors stay the caller\'s own'),
]
