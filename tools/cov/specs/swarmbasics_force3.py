# SwarmBasics (module 80) swb3 recipes, for COV_GDB_SCRIPT=tools/cov/force_ret.py (every write is
# undone when the body returns):
#   COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/swarmbasics_force3.py <out> 3dfd2249,0g_swamp_hall \
#     --con tools/cov/specs/swarmbasics_swamp.con --fast --frames 3000
# (run it in the swamp_hall and cliff_hall areas too: only those swarms have boids).
CLASS = 'CSwarmBasics'

_SURF = '$' + 'rax'  # the hidden return pointer of a by-value CCollisionSurface

FORCE_RET = [
    ('5024 all boid movement', 'CSwarmBasics::UpdateAllBoidMovement(', 2,
     [(0, [('$this->mAnimated', '1')])],
     'the mAnimated arm (line 880): the animated boid-movement loop'),
    ('3B34 render hardware light', 'CSwarmBasics::Render(', 2,
     [(1, [('$this->mEnableLighting', '1'), ('$this->mUseSoftwareLight', '0')])],
     'the enableLighting && !useSoftwareLight arm: HardwareLight is called (line 1230)'),
    ('6104 updateboid states', 'CSwarmBasics::UpdateBoid(', 4,
     [(0, [('((CSwarmBasics::CBoid*)$rcx)->mLaunched', '1')]),
      (1, [('$this->mVulnerableToSafeZone', '1')]),
      (2, [('((CSwarmBasics::CBoid*)$rcx)->mFramesNotOnSurface', '30')]),
      (3, [('$this->mLifeTime', '0.0')])],
     'the launched path (604-641), the safe-zone test (597), the no-surface death (639) and the'
     ' lifetime arm (589)'),
]

FORCE_INNER = [
    ('76F4 no waypoint', 'CSwarmBasics::CreateBoid(', 'CSwarmBasics::GetWaypointForState(', 2,
     {0: [('$rax', '0xffff')]}, 'the waypoint nullptr arm (line 437)'),
    ('76F4 no next waypoint', 'CSwarmBasics::CreateBoid(', 'CScriptWaypoint::NextWaypoint(', 2,
     {1: [('$rax', '0xffff')]}, 'the next-waypoint nullptr arm (line 440)'),
    ('76F4 upside-down surface', 'CSwarmBasics::CreateBoid(',
     'CSwarmBasics::FindBestCollisionInBox(', 1,
     {0: [('((CCollisionSurface*)$rax)->mA.mX', '0.0'),
          ('((CCollisionSurface*)$rax)->mA.mY', '0.0'),
          ('((CCollisionSurface*)$rax)->mA.mZ', '0.0'),
          ('((CCollisionSurface*)$rax)->mB.mX', '1.0'),
          ('((CCollisionSurface*)$rax)->mB.mY', '0.0'),
          ('((CCollisionSurface*)$rax)->mB.mZ', '0.0'),
          ('((CCollisionSurface*)$rax)->mC.mX', '0.0'),
          ('((CCollisionSurface*)$rax)->mC.mY', '-1.0'),
          ('((CCollisionSurface*)$rax)->mC.mZ', '0.0')]},
     'the dot(surface normal, z) == -1 arm (line 446): the triangle is rewritten so its normal is -Z'),
]
