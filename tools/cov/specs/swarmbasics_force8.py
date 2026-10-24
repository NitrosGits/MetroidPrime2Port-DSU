# SwarmBasics (module 80) swb8: UpdateBoid's last three arms by placing boid 0 relative to the floor.
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swarmbasics_force8.py <out> \
#     42b935e4,01_sand_entrance --con tools/cov/specs/swarmbasics_sand.con --fast --frames 1200
# The boid's transform translation Z is mTransform.m23; each phase moves it from where it is, so the
# boid oscillates around the floor instead of drifting. Pair this run with a force_ret run of
# swarmbasics_force7.py (same area) so 599's safe-zone hit is in the merged profile too.
CLASS = 'CSwarmBasics'
_d = [0]


def floor_boid(m):
    _d[0] += 1
    ph = _d[0] % 3
    b = '((CSwarmBasics*)$rdi)->mBoids.mItems[0]'
    try:
        z = float(gdb.parse_and_eval(b + '.mTransform.m23'))
    except Exception:
        return
    setv = lambda e, v: m.execute('set var %s = %s' % (e, v))
    setv(b + '.mTransform.m23', repr(z + (0.3 if ph == 0 else (-0.5 if ph == 1 else 5.0))))
    setv(b + '.mLaunched', '1' if ph == 0 else '0')
    setv(b + '.mVelocity.mX', '0.0')
    setv(b + '.mVelocity.mY', '0.0')
    setv(b + '.mVelocity.mZ', '-60.0' if ph == 0 else '-4.0')
    setv(b + '.mRemainingLaunchNotOnSurfaceFrames', '0')
    # phase 0: a radius (0.5) that both reaches the floor (probe radius 1.0 > the 0.7 the probe sinks) and leaves
    # speed*dt - boidRadius = 0.5 >= 0, so the loop re-tests and leaves through !found with found true (613).
    setv('((CSwarmBasics*)$rdi)->mBoidRadius', '0.5' if ph == 0 else '1.0')
    setv(b + '.mFramesNotOnSurface', '0')


FORCE = [
    ('6104 boid on the floor', 'CSwarmBasics::UpdateBoid(', [], [floor_boid], 40,
     'phase 0: just above the floor, launched, large velocity (613); phase 1: at/below the floor, not'
     ' launched (656/657 true); phase 2: five units up, not launched (656/657 false)'),
]
