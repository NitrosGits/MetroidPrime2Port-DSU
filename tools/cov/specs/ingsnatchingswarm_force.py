# IngSnatchingSwarm (module 33) forcing spec for COV_GDB_SCRIPT=tools/cov/force_ret.py, same area
# and driver as tools/cov/specs/ingsnatchingswarm.py:
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/ingsnatchingswarm_force.py <out> 3bfa3eff,10_temple_pathofeyes \
#     --con tools/cov/specs/ingsnatchingswarm_pathofeyes.con --fast --frames 2400
# Every write is undone when the body returns (force_ret), so the arc still flies normally between hits.
CLASS = 'CIngSnatchingSwarm'

FORCE_RET = [
    ('1908 CalculateArcLength no arc', 'CIngSnatchingSwarm::CalculateArcLength(', 2,
     [(0, [('$this->mArcPoints.mCount', '0')])],
     'mArcPoints.size() != 4 (the arc is only set by SetPortalSwarmPath)'),
    ('17C8 FollowArc arms', 'CIngSnatchingSwarm::FollowArc(', 4,
     [(0, [('$this->mArcSpeedScale', '0.0')]),
      (1, [('$this->mArcPoints.mCount', '0')]),
      (2, [('$this->mArcLength', '0.001')]),
      (3, [('$xmm0.v4_float[0]', '1e6')])],
     'size != 4 / speed <= 0 / a tiny duration (t >= 1 at the arc end) / time past the end'),
]

FORCE_INNER = [
    ('29A8 SelectTarget not possessable', 'CIngSnatchingSwarm::SelectTarget(',
     'CPatterned::CanBeIngPossessed(', 2, {0: [('$rax', '0')]},
     'the candidate filter false arm (line 475)'),
    ('29A8 SelectTarget possessed', 'CIngSnatchingSwarm::SelectTarget(',
     'CPatterned::IsIngPossessed(', 2, {1: [('$rax', '1')]},
     'the IsIngPossessed ? 1 : 0 arm (line 476)'),
    ('0F7C UpdateParticles max speed', 'CIngSnatchingSwarm::UpdateParticles(',
     'CMain::IsMaxSpeed(', 2, {0: [('$rax', '1')]},
     'the IsMaxSpeed true arm (line 704)'),
]
