# SwarmBasics (module 80) swb4: the swb3 recipes with both fixes. For
# COV_GDB_SCRIPT=tools/cov/force_ret.py:
#   COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/swarmbasics_force4.py <out> 3dfd2249,0g_swamp_hall \
#     --con tools/cov/specs/swarmbasics_swamp.con --fast --frames 1200
# Fix 1: every FORCE_RET lvalue is a `$this` member (force_ret binds $this to the call's object, so
# the restore at the return writes the same object). No lvalue goes through an argument register: the
# swb3 spec wrote ((CSwarmBasics::CBoid*)$rcx)->mLaunched, whose restore ran in the caller with $rcx
# dead ("restore failed ... 0x100b9"), leaving the boid launched for ever and stalling the game.
# Fix 2: the hardware-light path is forced on one Render call in 600 (period 600, phase 0): with
# 1200 frames that is ~2 frames, so the CActorLights-per-boid cost cannot crawl the run.
CLASS = 'CSwarmBasics'

FORCE_RET = [
    ('5024 all boid movement', 'CSwarmBasics::UpdateAllBoidMovement(', 2,
     [(0, [('$this->mAnimated', '1')])],
     'the mAnimated arm: the animated boid-movement loop'),
    ('3B34 render hardware light', 'CSwarmBasics::Render(', 600,
     [(0, [('$this->mEnableLighting', '1'), ('$this->mUseSoftwareLight', '0')])],
     'the enableLighting && !useSoftwareLight arm (HardwareLight is called), ~2 frames'),
    ('6104 updateboid swarm state', 'CSwarmBasics::UpdateBoid(', 4,
     [(0, [('$this->mLifeTime', '0.0')]), (1, [('$this->mVulnerableToSafeZone', '1')])],
     'the lifetime arm (589) and the safe-zone test (597)'),
]

FORCE_INNER = [
    ('76F4 no waypoint', 'CSwarmBasics::CreateBoid(', 'CSwarmBasics::GetWaypointForState(', 2,
     {0: [('$rax', '0xffff')]}, 'the waypoint nullptr arm (line 437)'),
    ('76F4 no next waypoint', 'CSwarmBasics::CreateBoid(', 'CScriptWaypoint::NextWaypoint(', 2,
     {1: [('$rax', '0xffff')]}, 'the next-waypoint nullptr arm (line 440)'),
    ('76F4 upside-down surface', 'CSwarmBasics::CreateBoid(',
     'CSwarmBasics::FindBestCollisionInBox(', 2,
     {1: [('((CCollisionSurface*)$rax)->mA.mX', '0.0'),
          ('((CCollisionSurface*)$rax)->mA.mY', '0.0'),
          ('((CCollisionSurface*)$rax)->mA.mZ', '0.0'),
          ('((CCollisionSurface*)$rax)->mB.mX', '1.0'),
          ('((CCollisionSurface*)$rax)->mB.mY', '0.0'),
          ('((CCollisionSurface*)$rax)->mB.mZ', '0.0'),
          ('((CCollisionSurface*)$rax)->mC.mX', '0.0'),
          ('((CCollisionSurface*)$rax)->mC.mY', '-1.0'),
          ('((CCollisionSurface*)$rax)->mC.mZ', '0.0')]},
     'the dot(surface normal, z) == -1 arm (line 446) on alternate calls: the triangle is rewritten'
     ' so its normal is -Z'),
]
