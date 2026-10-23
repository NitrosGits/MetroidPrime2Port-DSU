# FishCloud (module 20) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Drivers: tools/cov/specs/fishcloud_*.con, e.g.
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/fishcloud.py <out> \
#     3bfa3eff,06_temple_sacredpath --con tools/cov/specs/fishcloud_sacredpath.con --fast --frames 2600
CLASS = 'CFishCloud'
SPEC = [
    ('22EC Touch', 'CFishCloud::Touch(', [], 'max=4000'),
    ('2B04 KillBoid', 'CFishCloud::KillBoid(', [], 'max=4000'),
    ('1A1C ApplyRotation', 'CFishCloud::ApplyRotation(', ['$rcx & 0xff'], 'max=4000'),
    ('3F2C AddAttractor', 'CFishCloud::AddAttractor(', [], 'ret'),
    ('3A88 AddRepulsor', 'CFishCloud::AddRepulsor(', [], 'ret'),
]
