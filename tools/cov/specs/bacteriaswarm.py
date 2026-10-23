# BacteriaSwarm (module 6) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Swarm: BSWM e003d0023, Temple Grounds dark 04_temple_unknownenemies_dark L0:
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/bacteriaswarm.py <out> \
#     3bfa3eff,04_temple_unknownenemies_dark --con tools/cov/specs/bacteriaswarm_temple.con --fast --frames 3200
CLASS = 'CBacteriaSwarm'
SPEC = [
    ('3570 Think', 'CBacteriaSwarm::Think(', [], 'max=4000'),
    ('4734 UpdateBoid', 'CBacteriaSwarm::UpdateBoid(', [], 'max=4000'),
    ('12E0 KillBoid', 'CBacteriaSwarm::KillBoid(', [], 'max=4000'),
]
