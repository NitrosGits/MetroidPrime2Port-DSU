# OctapedeSegment (REL 46) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Sanctuary Fortress 0p_cliff_hall layer 0 holds 16 segments (two
# Octapedes); tools/drivers/octapede_cliffhall.con walks, kills and freezes them:
#   BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/octapedesegment.py <out> 1baa96c2,0p_cliff_hall \
#     --con tools/drivers/octapede_cliffhall.con --fast --frames <n>
#   then tools/cov/gcov_report.py build-port-cov <out>/gcov --module OctapedeSegment
CLASS = 'COctapedeSegment'
SPEC = [
    ('Think', 'COctapedeSegment::Think(', [], 'max=4000'),
    ('AcceptScriptMsg', 'COctapedeSegment::AcceptScriptMsg(', [], 'max=4000'),
    ('Patrol', 'COctapedeSegment::Patrol(', ['$rdx'], 'max=4000'),
    ('Dead', 'COctapedeSegment::Dead(', ['$rdx'], 'max=4000'),
    ('Death', 'COctapedeSegment::Death(', [], 'max=400'),
    ('Bounce', 'COctapedeSegment::Bounce(', [], 'max=4000'),
    ('CollidedWith', 'COctapedeSegment::CollidedWith(', [], 'max=4000'),
    ('Explode', 'COctapedeSegment::Explode(', [], 'max=400'),
    ('Freeze', 'COctapedeSegment::Freeze(', [], 'max=400'),
    ('Burn', 'COctapedeSegment::Burn(', [], 'max=400'),
    ('DoUserAnimEvent', 'COctapedeSegment::DoUserAnimEvent(', ['$rdx'], 'max=400'),
]
