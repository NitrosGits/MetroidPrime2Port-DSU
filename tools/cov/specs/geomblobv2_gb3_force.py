# gb3: force Ing blob emission while the owner is corporeal. Counters still come
# from build-port-cov; no production probes or source edits are needed.
# COV_GDB_SCRIPT=tools/cov/cov_force.py BUILD=$PWD/build-port-cov \
#   tools/cov/force.sh tools/cov/specs/geomblobv2_gb3_force.py <out> \
#   1baa96c2,2735236b --frames 8000 --fast
CLASS = 'CCollisionTracker'
SPEC = []
FORCE = [
    ('27DC emission', 'CCollisionTracker::Think(', [],
     ['$this->mParticleEmissionRateScalar = 1.0'], 0,
     'emit strands from the live Ing collision tracker'),
]
