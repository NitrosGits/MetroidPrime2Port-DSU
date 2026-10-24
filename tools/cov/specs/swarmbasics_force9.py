# SwarmBasics (module 80) swb9: UpdateBoid's last arm, 613:3/4 - the launched `while` loop's
# re-test with `found` already true.  force.py (persistent writes, no restore):
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swarmbasics_force9.py <out> \
#     3dfd2249,0g_swamp_hall --con tools/cov/specs/swarmbasics_swamp.con --fast --frames 1200
# Merge the run's gcov/ with the swb8 profiles (the 599 safe-zone arm comes from
# swarmbasics_force7.py there) before gcov_report.py: this spec's own run leaves the falling exit
# of the same loop (613:1/4) untaken, that merge leaves this one untaken; together, ALL-ARMS.
#
# The fixture, all of it measured at the entry of every UpdateBoid call, nothing stubbed:
#   * the boid argument is taken from the entry register that holds an element of mBoids (the
#     $rcx/$r8/$r9 check below), never from $this->mBoids.mItems[0] and never from a later, stale
#     register: Think's boid loop and its outlier loop pass different objects, and
#     CPlantScarabSwarm/CMetareeSwarm override UpdateBoid and forward their own boid.
#   * a boid is "on a surface" when its own mSurface triangle (a real CCollisionSurface that a
#     real FindBestSurface returned and 654 stored) has the boid's translation within 2*mBoidRadius
#     of its plane AND the projection of that translation inside the triangle (the three sign
#     tests of PointOnSurface, recomputed here from the vertices) AND the boid took the
#     non-launched path last frame (mFramesNotOnSurface == 0, mLaunched == 0).
#   * that absolute position and triangle are cached per boid.  A later call re-writes the SAVED
#     ABSOLUTE position (never a delta: swb8 moved m23 by +0.3/-0.5/+5 every call, which is a
#     +4.8 drift per cycle, not a non-drifting placement) and only while the boid is still within
#     0.05 of it, so the CAreaCollisionCache Think built for this boid still covers the point.
#   * speed = 4*r/dt and the probe scale s = h/(8r), so speed*dt = 4r (>= 2r: the loop's `distance`
#     survives `distance -= boidRadius`) and the probe point lands h/2 over the same triangle, so
#     it is inside 2r of the plane and inside the triangle.  FindBestSurface is the real one on
#     the real octree cache and has to return true on its own; nothing sets `found`.
# Every capture and apply is logged to <out>/swb9_geom.log with the boid pointer, the register it
# came from, dt, speed, radius, the vertices/normal, the predicted point and its height over the
# plane.  Each apply arms a one-shot breakpoint on FindBestSurface, so the very next call - the
# loop's own - has its real return value and returned triangle logged as PROBE-RESULT.
CLASS = 'CSwarmBasics'

import os

import gdb  # the spec is exec'd by tools/cov/force.py inside gdb's python

_OUT = os.environ.get('COV_OUT', '/tmp')
_GEOM = os.path.join(_OUT, 'swb9_geom.log')

_st = {'sizeof': 0, 'n': 0, 'abi': 0, 'apply': 0, 'fs': 0, 'probe_bp': None, 'cap': {}}


def _log(msg):
    with open(_GEOM, 'a') as fh:
        fh.write(msg + '\n')


def _f(g, expr):
    return float(g.parse_and_eval(expr))


def _i(g, expr):
    return int(g.parse_and_eval(expr))


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _plane(a, b, c):
    """(unit normal, its length) of the triangle, cross(B-A, C-A) as CPlane does."""
    n = _cross(_sub(b, a), _sub(c, a))
    ln = (n[0] * n[0] + n[1] * n[1] + n[2] * n[2]) ** 0.5
    if ln < 1e-6:
        return None, ln
    return (n[0] / ln, n[1] / ln, n[2] / ln), ln


def _inside(a, b, c, n, p):
    """PointOnSurface's three sign tests, recomputed from the vertices."""
    d = _dot(_sub(p, a), n)
    q = (p[0] - d * n[0], p[1] - d * n[1], p[2] - d * n[2])
    verts = (a, b, c)
    for i in range(3):
        nxt = i + 2
        prev = nxt if nxt == 2 else nxt - 3
        edge2 = _sub(verts[prev], verts[i])
        edge1 = _sub(q, verts[i])
        if _dot(n, _cross(edge1, edge2)) < 0.0:
            return False
    return True


def _addr(v):
    a = v.address
    return int(a) if a is not None else int(v)


def _fs_addr():
    """The runtime address of CSwarmBasics::FindBestSurface (the run's nm table for its name)."""
    if _st['fs']:
        return _st['fs']
    try:
        _st['fs'] = int(gdb.parse_and_eval('(void*)&CSwarmBasics::FindBestSurface'))
        return _st['fs']
    except Exception:
        pass
    try:
        for ln in open(os.environ['COV_NM']):
            parts = ln.rstrip('\n').split('\t')
            if len(parts) == 2 and 'CSwarmBasics::FindBestSurface(' in parts[1]:
                _st['fs'] = int(gdb.parse_and_eval("(void*)&'%s'" % parts[1]))
                return _st['fs']
    except Exception as e:
        _log('FindBestSurface address not found: %s' % e)
    return 0


class _ProbeRet(gdb.FinishBreakpoint):
    def __init__(self, frame, info):
        super().__init__(frame, internal=True)
        self.info = info

    def stop(self):
        info = self.info
        try:
            rax = int(gdb.parse_and_eval('$rax')) & 0xFF
        except Exception:
            rax = -1
        try:
            extra = ''
            if rax == 1 and info['out']:
                # The surface the real FindBestSurface wrote to `out`, and how far the probe
                # point is from its plane.
                s = '((CCollisionSurface*)%d)' % info['out']
                a = tuple(_f(gdb, s + '.m' + v + '.m' + c) for v in 'ABC' for c in 'XYZ')
                n, ln = _plane(a[0:3], a[3:6], a[6:9])
                if n is not None:
                    extra = ' surface_h=%.5f surface_normal=(%.4f,%.4f,%.4f)' % (
                        _dot(_sub(info['pos'], a[0:3]), n), n[0], n[1], n[2])
            _log('PROBE-RESULT %s found=%d predicted=(%.5f,%.5f,%.5f) radius=%.5f%s' %
                 (info['tag'], rax, info['pos'][0], info['pos'][1], info['pos'][2],
                  info['radius'], extra))
        except Exception as e:
            _log('PROBE-RESULT %s log failed: %s' % (info['tag'], e))
        return False


class _ProbeOneShot(gdb.Breakpoint):
    """Armed at an apply: takes the return value of the loop's own FindBestSurface call."""

    def __init__(self, addr, tag, predicted, radius):
        super().__init__('*0x%x' % addr, internal=True)
        self.silent = True
        self.tag, self.predicted, self.radius = tag, predicted, radius

    def stop(self):
        self.enabled = False
        try:
            fr = gdb.newest_frame()
            out = 0
            try:
                out = _addr(fr.read_var('out'))
            except Exception:
                try:
                    out = _i(gdb, '$rdx')
                except Exception:
                    out = 0
            _ProbeRet(fr, {'tag': self.tag, 'pos': self.predicted, 'radius': self.radius,
                           'out': out})
        except Exception as e:
            _log('PROBE %s: arm failed (%s)' % (self.tag, e))
        return False


def _arm_probe(tag, predicted, radius):
    addr = _fs_addr()
    if not addr:
        return
    bp = _st['probe_bp']
    if bp is None:
        try:
            _st['probe_bp'] = _ProbeOneShot(addr, tag, predicted, radius)
        except Exception as e:
            _log('PROBE %s: could not arm (%s)' % (tag, e))
        return
    bp.tag, bp.predicted, bp.radius = tag, predicted, radius
    bp.enabled = True


def _boid_arg(g, items, count):
    """The CBoid& argument: the entry register that holds an element of $this->mBoids."""
    if _st['sizeof'] == 0:
        _st['sizeof'] = _i(g, 'sizeof(CSwarmBasics::CBoid)')
    sz = _st['sizeof']
    if items == 0 or sz <= 0:
        return None, None, None
    for reg in ('$rcx', '$r8', '$r9'):
        try:
            v = _i(g, reg)
        except Exception:
            continue
        off = v - items
        if v and off >= 0 and off % sz == 0 and off // sz < count:
            return v, off // sz, reg
    return None, None, None


def launch_on_real_floor(g):
    """Re-launch a boid that is measurably on a real surface, from its saved absolute position."""
    st = _st
    st['n'] += 1
    try:
        this = _i(g, '$rdi')
        dt = _f(g, '$xmm0.v4_float[0]')
        count = _i(g, '((CSwarmBasics*)%d)->mBoids.mCount' % this)
        items = _i(g, '((CSwarmBasics*)%d)->mBoids.mItems' % this)
        r = _f(g, '((CSwarmBasics*)%d)->mBoidRadius' % this)
    except Exception as e:
        if st['abi'] < 5:
            _log('read failed: %s' % e)
        st['abi'] += 1
        return
    boid, idx, reg = _boid_arg(g, items, count)
    if boid is None:
        if st['abi'] < 5:
            _log('no boid argument: rcx=0x%x r8=0x%x r9=0x%x items=0x%x count=%d sizeof=%d'
                 % (_i(g, '$rcx'), _i(g, '$r8'), _i(g, '$r9'), items, count, st['sizeof']))
        st['abi'] += 1
        return
    if not (0.0 < dt < 0.5) or not (0.01 < r < 100.0):
        if st['abi'] < 8:
            _log('unexpected dt=%.6f radius=%.4f (%s) - no writes' % (dt, r, reg))
        st['abi'] += 1
        return
    b = '((CSwarmBasics*)%d)->mBoids.mItems[%d]' % (this, idx)
    try:
        pos = (_f(g, b + '.mTransform.m03'), _f(g, b + '.mTransform.m13'),
               _f(g, b + '.mTransform.m23'))
        launched = _i(g, b + '.mLaunched')
        frames = _i(g, b + '.mFramesNotOnSurface')
        a = tuple(_f(g, b + '.mSurface.m' + v + '.m' + c) for v in 'ABC' for c in 'XYZ')
    except Exception as e:
        if st['abi'] < 12:
            _log('boid read failed (0x%x idx %d): %s' % (boid, idx, e))
        st['abi'] += 1
        return
    a3 = (a[0:3], a[3:6], a[6:9])
    n, ln = _plane(*a3)
    if n is None:
        return
    h = _dot(_sub(pos, a3[0]), n)
    key = (this, idx)
    cap = st['cap'].get(key)
    if launched == 0 and frames == 0 and 0.01 < h < 2.0 * r and _inside(a3[0], a3[1], a3[2], n, pos):
        if cap is None or max(abs(pos[0] - cap['pos'][0]), abs(pos[1] - cap['pos'][1]),
                              abs(pos[2] - cap['pos'][2])) > 0.05:
            _log('CAPTURE 0x%x idx=%d reg=%s dt=%.6f r=%.4f pos=(%.5f,%.5f,%.5f) h=%.5f'
                 ' tri=(%.4f,%.4f,%.4f)(%.4f,%.4f,%.4f)(%.4f,%.4f,%.4f) normal=(%.5f,%.5f,%.5f)'
                 % (boid, idx, reg, dt, r, pos[0], pos[1], pos[2], h,
                    a3[0][0], a3[0][1], a3[0][2], a3[1][0], a3[1][1], a3[1][2],
                    a3[2][0], a3[2][1], a3[2][2], n[0], n[1], n[2]))
        cap = {'pos': pos, 'tri': a3, 'n': n, 'h': h, 'r': r}
        st['cap'][key] = cap
    if cap is None:
        return
    saved, nr = cap['pos'], cap['n']
    if max(abs(pos[0] - saved[0]), abs(pos[1] - saved[1]), abs(pos[2] - saved[2])) > 0.05:
        return
    speed = 4.0 * r / dt
    s = min(max(cap['h'] / (2.0 * dt * speed), 0.0), 0.5)
    move = s * dt * speed
    predicted = (saved[0] - move * nr[0], saved[1] - move * nr[1], saved[2] - move * nr[2])
    pred_h = _dot(_sub(predicted, cap['tri'][0]), nr)
    tag = 'swb9-%d' % (st['apply'] + 1)
    try:
        setv = lambda e, v: g.execute('set var %s = %s' % (e, v))
        setv('((CSwarmBasics*)%d)->mSurfaceProbeScale' % this, repr(s))
        setv(b + '.mTransform.m03', repr(saved[0]))
        setv(b + '.mTransform.m13', repr(saved[1]))
        setv(b + '.mTransform.m23', repr(saved[2]))
        setv(b + '.mVelocity.mX', repr(-speed * nr[0]))
        setv(b + '.mVelocity.mY', repr(-speed * nr[1]))
        setv(b + '.mVelocity.mZ', repr(-speed * nr[2]))
        setv(b + '.mLaunched', '1')
        setv(b + '.mRemainingLaunchNotOnSurfaceFrames', '0')
    except Exception as e:
        _log('APPLY %s write failed: %s' % (tag, e))
        return
    st['apply'] += 1
    _log('APPLY %s 0x%x idx=%d reg=%s dt=%.6f r=%.4f speed=%.3f scale=%.5f'
         ' pos=(%.5f,%.5f,%.5f) predicted=(%.5f,%.5f,%.5f) predicted_h=%.5f'
         ' speed_dt=%.4f minus_radius=%.4f'
         % (tag, boid, idx, reg, dt, r, speed, s, saved[0], saved[1], saved[2],
            predicted[0], predicted[1], predicted[2], pred_h, speed * dt, speed * dt - r))
    _arm_probe(tag, predicted, 2.0 * r)


FORCE = [
    ('6104 swb9 relaunch from a measured surface', 'CSwarmBasics::UpdateBoid(', [],
     [launch_on_real_floor], 0,
     'a boid that is measurably on a real triangle is put back at its saved absolute position'
     ' with speed*dt = 4r and the probe scale that keeps the probe h/2 over the same triangle:'
     ' the loop finds the surface on its first iteration and, since 4r - r >= 0, re-tests and'
     ' exits through !found with found true (613:3/4)'),
]
