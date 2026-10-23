# PillBug (module 48) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints count the state bodies per EStateMsg ($rdx) and the triggers' results. Drivers:
# tools/cov/specs/pillbug_*.con, e.g.
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/pillbug.py <out> \
#     42b935e4,0b_sand_hall --con tools/cov/specs/pillbug_sandhall.con --fast --frames 3000
CLASS = 'CPillBug'
SPEC = [
    ('21D4 Patrol', 'CPillBug::Patrol(', ['$rdx'], 'max=4000'),
    ('2014 FastPatrol', 'CPillBug::FastPatrol(', ['$rdx'], 'max=4000'),
    ('1A58 ReturnToPath', 'CPillBug::ReturnToPath(', ['$rdx'], 'max=4000'),
    ('1768 Fall', 'CPillBug::Fall(', ['$rdx'], 'max=4000'),
    ('1598 Injured', 'CPillBug::Injured(', ['$rdx'], 'max=4000'),
    ('1458 Turn', 'CPillBug::Turn(', ['$rdx'], 'max=4000'),
    ('1020 Wander', 'CPillBug::Wander(', ['$rdx'], 'max=4000'),
    ('BD8 Pause', 'CPillBug::Pause(', ['$rdx'], 'max=4000'),
    ('274C CloseToPath', 'CPillBug::CloseToPath(', [], 'ret max=4000'),
    ('2968 HasReturnPath', 'CPillBug::HasReturnPath(', [], 'ret max=4000'),
    ('3134 BuildPath', 'CPillBug::BuildPath(', [], ''),
    ('3840 AcceptScriptMsg', 'CPillBug::AcceptScriptMsg(', [], 'max=4000'),
]
