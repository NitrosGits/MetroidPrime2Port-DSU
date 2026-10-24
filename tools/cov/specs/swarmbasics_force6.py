# SwarmBasics (module 80) swb6: UpdateBoid's remaining arms and the Render/HardwareLight path.
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swarmbasics_force6.py <out> \
#     3dfd2249,0g_swamp_hall --con tools/cov/specs/swarmbasics_swamp.con --fast --frames 1200
# Every value is written through `$this` (the boid is `$this->mBoids.mItems[0]`, the vector's element
# array), and the outlier list is forged at a `loc:` line *after* UpdatePartition() (line 824), with
# `$this` captured at Think's entry breakpoint - UpdatePartition clears the list later in the body.
CLASS = 'CSwarmBasics'

_swarm = [0]


def cap_this(m):
    try:
        _swarm[0] = int(gdb.parse_and_eval('$rdi'))
    except Exception:
        pass


def forge_outlier(m):
    s = _swarm[0]
    if not s:
        return
    m.execute('set var ((CSwarmBasics*)%d)->mOutlierBoidList = &((CSwarmBasics*)%d)->mBoids.mItems[0]'
              % (s, s))
    m.execute('set var ((CSwarmBasics*)%d)->mBoids.mItems[0].mNext = 0' % s)


# force.py applies every set on every hit after `skip`, so the variation has to live in a callable:
# a six-phase cycle over the launched, lifetime, safe-zone and stuck states.
_n = [0]


def updateboid_states(m):
    _n[0] += 1
    ph = _n[0] % 6
    b = '((CSwarmBasics*)$rdi)'
    setv = lambda e, v: m.execute('set var %s = %s' % (e, v))
    setv('%s->mBoids.mItems[0].mLaunched' % b, '1' if ph in (0, 4, 5) else '0')
    setv('%s->mLifeTime' % b, '1.0' if ph in (1, 4) else '0.0')
    setv('%s->mVulnerableToSafeZone' % b, '1' if ph in (1, 5) else '0')
    setv('%s->mBoids.mItems[0].mLifeTime' % b, '0.0' if ph in (1, 2) else '10.0')
    setv('%s->mBoids.mItems[0].mFramesNotOnSurface' % b, '30' if ph == 3 else '0')
    setv('%s->mBoids.mItems[0].mRemainingLaunchNotOnSurfaceFrames' % b,
         '1' if ph == 4 else '0')


FORCE = [
    # instrumentation: what Render sees, one line per hit in forced.txt
    ('3B34 render probe', 'CSwarmBasics::Render(',
     ['$this->mEnableLighting', '$this->mUseSoftwareLight', '$this->mOutlierBoidList'], [], 0,
     'instrumentation only'),
    ('5024 think capture', 'CSwarmBasics::Think(', [], [cap_this], 0, 'captures $this for the loc:'),
    ('5024 outlier after UpdatePartition', 'loc:CSwarmBasics.cpp:824', [], [forge_outlier], 0,
     'a forged outlier list once UpdatePartition has cleared it'),
    ('6104 updateboid states', 'CSwarmBasics::UpdateBoid(', [], [updateboid_states], 0,
     'six-phase cycle over the launched, lifetime, safe-zone and stuck arms'),
]
