# SwarmBasics (module 80) swb8: UpdateBoid's last three arms by moving boid 0's translation Z.
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swarmbasics_force8.py <out> \
#     3dfd2249,0g_swamp_hall --con tools/cov/specs/swarmbasics_swamp.con --fast --frames 1200
#   (the eight runs of the swb8 report used 3dfd2249,0g_swamp_hall and 1baa96c2,0a_cliff_hall)
# The boid's transform translation Z is mTransform.m23.  The three phase offsets are ADDED to whatever
# the current value is, so they are cumulative and net +4.8 per three-call cycle: this is a drifting
# placement, not a floor-relative one, and it cannot keep the boid on a floor.  It also writes
# $this->mBoids.mItems[0] - boid 0 of this swarm, which is not necessarily the object the call is
# updating (Think's outlier loop and the CPlantScarabSwarm/CMetareeSwarm overrides pass other boids);
# swarmbasics_force9.py takes the argument from the entry register instead.
# Pair this run with a force_ret run of swarmbasics_force7.py (same area) so 599's safe-zone hit is in
# the merged profile too.
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
    # phase 0's claim does not hold: with mSurfaceProbeScale 1.5 the probe point is a whole dt*|v|
    # (1.0) below the plane while the probe radius is 2*mBoidRadius = 1.0, and FindBestSurface needs
    # |plane height| < radius strictly, so it never finds a surface here and 613 stays untaken.
    # (swb9: speed*dt = 4r with a probe scale that keeps the probe point h/2 over the real triangle.)
    setv('((CSwarmBasics*)$rdi)->mBoidRadius', '0.5' if ph == 0 else '1.0')
    setv(b + '.mFramesNotOnSurface', '0')


FORCE = [
    ('6104 boid on the floor', 'CSwarmBasics::UpdateBoid(', [], [floor_boid], 40,
     'phase 0: just above the floor, launched, large velocity (613 attempted, not reached); phase 1:'
     ' at/below the floor, not launched (656/657 true); phase 2: five units up, not launched (656/657'
     ' false)'),
]
