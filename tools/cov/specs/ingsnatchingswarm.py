# IngSnatchingSwarm (module 33) spec for tools/cov/run.sh; the arms come from the coverage build
# (gcov_report.py --module IngSnatchingSwarm), the breakpoints only count calls per state message.
# Swarm: Temple Grounds 10_temple_pathofeyes layer 2, GENR e80e018c (TIMR e80e0189) generates ISSW
# 80e018b next to three Space Pirates:
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/ingsnatchingswarm.py <out> \
#     3bfa3eff,10_temple_pathofeyes --con tools/cov/specs/ingsnatchingswarm_eyes.con --fast --frames 3000
CLASS = 'CIngSnatchingSwarm'
SPEC = [
    ('2AB4 Think', 'CIngSnatchingSwarm::Think(', [], 'max=4000'),
    ('3010 ExitPortal', 'CIngSnatchingSwarm::ExitPortal(', ['$rdx'], 'max=4000'),
    ('2E78 FollowArcPath', 'CIngSnatchingSwarm::FollowArcPath(', ['$rdx'], 'max=4000'),
    ('2DB0 DiveToTarget', 'CIngSnatchingSwarm::DiveToTarget(', ['$rdx'], 'max=4000'),
    ('2B3C SnatchTarget', 'CIngSnatchingSwarm::SnatchTarget(', ['$rdx'], 'max=4000'),
    ('3360 Dead', 'CIngSnatchingSwarm::Dead(', ['$rdx'], 'max=4000'),
    ('SelectTarget', 'CIngSnatchingSwarm::SelectTarget(', [], ''),
    ('SetTargetDest', 'CIngSnatchingSwarm::SetTargetDest(', [], ''),
    ('SetPlayerDest', 'CIngSnatchingSwarm::SetPlayerDest(', [], ''),
    ('SetPortalSwarmPath', 'CIngSnatchingSwarm::SetPortalSwarmPath(', [], ''),
    ('DamagePlayer', 'CIngSnatchingSwarm::DamagePlayer(', [], ''),
    ('Explode', 'CIngSnatchingSwarm::Explode(', [], ''),
]
