# SwarmBasics (module 80) swb7: UpdateBoid's last nine arms. For
# COV_GDB_SCRIPT=tools/cov/force_ret.py (every write is undone when the call returns):
#   COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/swarmbasics_force7.py <out> 3dfd2249,0g_swamp_hall \
#     --con tools/cov/specs/swarmbasics_swamp.con --fast --frames 1200
# Only `$this`-relative lvalues (the boid is $this->mBoids.mItems[0]) so force_ret can restore them at
# the call's return; no loc: breakpoints, no mNext write - those are what stopped Render in swb6.
CLASS = 'CSwarmBasics'

_B = '$this->mBoids.mItems[0].'
_S = '$this->'


def phase(launched, vz, rem, frames, looped, vuln):
    return [(0, [(_B + 'mLaunched', launched),
                 (_B + 'mVelocity.mX', '0.0'),
                 (_B + 'mVelocity.mY', '0.0'),
                 (_B + 'mVelocity.mZ', vz),
                 (_B + 'mRemainingLaunchNotOnSurfaceFrames', rem),
                 (_B + 'mFramesNotOnSurface', frames),
                 (_B + 'mHasLoopedSound', looped),
                 (_S + 'mVulnerableToSafeZone', vuln)])]


# Twelve phases: a fast launched boid (the while loop iterates and !found runs), a launched boid whose
# remaining-launch counter is 1 (617 false, 636 true), a slow launched boid that lands (found true),
# the >= 30 non-launched death with and without the looped sound, and the falling non-launched path.
FORCE_RET = [
    ('6104 ph0 fast launch', 'CSwarmBasics::UpdateBoid(', 12, phase('1', '-20.0', '0', '0', '0', '0'),
     'the launched loop with several iterations, found false (635)'),
    ('6104 ph1 launch counter', 'CSwarmBasics::UpdateBoid(', 12,
     [(1, [(_B + 'mLaunched', '1'), (_B + 'mVelocity.mZ', '-40.0'),
           (_B + 'mRemainingLaunchNotOnSurfaceFrames', '1'),
           (_B + 'mFramesNotOnSurface', '0'), (_B + 'mHasLoopedSound', '0'),
           (_S + 'mVulnerableToSafeZone', '0')])],
     'the remaining-launch false arm (617) and the counter arms (636/637)'),
    ('6104 ph2 landing launch', 'CSwarmBasics::UpdateBoid(', 12,
     [(2, [(_B + 'mLaunched', '1'), (_B + 'mVelocity.mZ', '-4.0'),
           (_B + 'mRemainingLaunchNotOnSurfaceFrames', '0'),
           (_B + 'mFramesNotOnSurface', '0'), (_B + 'mHasLoopedSound', '0'),
           (_S + 'mVulnerableToSafeZone', '0')])],
     'a launched boid that lands: found true, mLaunched cleared, the collided callback'),
    ('6104 ph3 stuck with sound', 'CSwarmBasics::UpdateBoid(', 12,
     [(3, [(_B + 'mLaunched', '0'), (_B + 'mVelocity.mZ', '-4.0'),
           (_B + 'mFramesNotOnSurface', '30'), (_B + 'mHasLoopedSound', '1'),
           (_S + 'mVulnerableToSafeZone', '0')])],
     'the >= 30 frames death with a looped sound (640/641/642/643)'),
    ('6104 ph4 stuck no sound', 'CSwarmBasics::UpdateBoid(', 12,
     [(4, [(_B + 'mLaunched', '0'), (_B + 'mVelocity.mZ', '-4.0'),
           (_B + 'mFramesNotOnSurface', '30'), (_B + 'mHasLoopedSound', '0'),
           (_S + 'mVulnerableToSafeZone', '0')])],
     'the same death without the sound (642 false)'),
    ('6104 ph5 falling', 'CSwarmBasics::UpdateBoid(', 12,
     [(5, [(_B + 'mLaunched', '0'), (_B + 'mVelocity.mZ', '-6.0'),
           (_B + 'mFramesNotOnSurface', '0'), (_B + 'mHasLoopedSound', '0'),
           (_S + 'mVulnerableToSafeZone', '0')])],
     'the falling non-launched path (645-670: 653/656/657)'),
    ('6104 ph6 safe zone', 'CSwarmBasics::UpdateBoid(', 12,
     [(6, [(_B + 'mLaunched', '0'), (_B + 'mVelocity.mZ', '-1.0'),
           (_B + 'mFramesNotOnSurface', '0'), (_B + 'mHasLoopedSound', '0'),
           (_S + 'mVulnerableToSafeZone', '1')])],
     'the safe-zone test with mVulnerableToSafeZone on (599 via the stub below)'),
    ('6104 ph7 lifetime', 'CSwarmBasics::UpdateBoid(', 12,
     [(7, [(_S + 'mLifeTime', '1.0'), (_B + 'mLifeTime', '0.0'), (_B + 'mLaunched', '0'),
           (_B + 'mVelocity.mZ', '-4.0'), (_B + 'mFramesNotOnSurface', '0')])],
     'the mLifeTime > 0 arm and the boid lifetime <= 0 arm (590/592)'),
    ('6104 ph8 no lifetime', 'CSwarmBasics::UpdateBoid(', 12,
     [(8, [(_S + 'mLifeTime', '0.0'), (_B + 'mLifeTime', '10.0'), (_B + 'mLaunched', '0'),
           (_B + 'mVelocity.mZ', '-4.0'), (_B + 'mFramesNotOnSurface', '0')])],
     'the same two arms from the other side (590/592 false)'),
    ('6104 ph9 tiny radius launch', 'CSwarmBasics::UpdateBoid(', 12,
     [(9, [(_S + 'mBoidRadius', '0.001'), (_B + 'mLaunched', '1'), (_B + 'mVelocity.mZ', '-400.0'),
           (_B + 'mRemainingLaunchNotOnSurfaceFrames', '0'), (_B + 'mFramesNotOnSurface', '0')])],
     'a launched boid with a tiny radius: several iterations, so a surface found early leaves the'
     ' loop through the !found test with found true (613), and the falling radius test is false'),
    ('6104 ph10 lifetime false', 'CSwarmBasics::UpdateBoid(', 12,
     [(10, [(_S + 'mLifeTime', '1.0'), (_B + 'mLifeTime', '10.0'), (_B + 'mLaunched', '0'),
            (_B + 'mVelocity.mZ', '-4.0'), (_B + 'mFramesNotOnSurface', '0')])],
     'the boid lifetime > 0 arm inside the mLifeTime arm (592 false)'),
    ('6104 ph11 floor radius', 'CSwarmBasics::UpdateBoid(', 12,
     [(11, [(_S + 'mBoidRadius', '1000.0'), (_S + 'mLifeTime', '0.0'), (_B + 'mLaunched', '0'),
            (_B + 'mVelocity.mZ', '-4.0'), (_B + 'mFramesNotOnSurface', '0')])],
     'the falling path with a huge boid radius: the surface height test (656/657)'),
]

FORCE_INNER = [
    ('6104 ph6 safe zone hit', 'CSwarmBasics::UpdateBoid(',
     'CSafeZoneManager::PointIsInSafeZone(', 4, {0: [('$rax', '1')]},
     'the safe-zone hit arm (599): KillBoid and return'),
]
