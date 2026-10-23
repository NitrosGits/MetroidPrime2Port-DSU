# MysteryFlyer (module 45, War Wasp) spec for tools/cov/run.sh; arms come from the coverage build
# (gcov_report.py), the breakpoints only count calls and a few entry values. Driver:
#   BUILD=$PWD/build-port-cov SECS=1800 tools/cov/run.sh tools/cov/specs/mysteryflyer.py <out> \
#     3bfa3eff,0e_temple_hall --con tools/cov/specs/mysteryflyer.con --fast --frames 3000
CLASS = 'CMysteryFlyer'
SPEC = [
    ('23B8 AcceptScriptMsg', 'CMysteryFlyer::AcceptScriptMsg(', [], 'max=4000'),
    ('1E14 Generate', 'CMysteryFlyer::Generate(', ['$rdx'], 'max=4000'),
    ('1BEC GoToPatrol', 'CMysteryFlyer::GoToPatrol(', ['$rdx'], 'max=4000'),
    ('1B24 Patrol', 'CMysteryFlyer::Patrol(', ['$rdx'], 'max=4000'),
    ('1A04 FollowAttackPath', 'CMysteryFlyer::FollowAttackPath(', ['$rdx'], 'max=4000'),
    ('1998 Hover', 'CMysteryFlyer::Hover(', ['$rdx'], 'max=4000'),
    ('1850 Approach', 'CMysteryFlyer::Approach(', ['$rdx'], 'max=4000'),
    ('162C Attack', 'CMysteryFlyer::Attack(', ['$rdx'], 'max=4000'),
    ('13F0 Dodge', 'CMysteryFlyer::Dodge(', ['$rdx'], 'max=4000'),
    ('15F4 Dead', 'CMysteryFlyer::Dead(', ['$rdx'], 'max=4000'),
    ('0C10 JoinTeam', 'CMysteryFlyer::JoinTeam(', [], 'max=4000'),
    ('0B68 QuitTeam', 'CMysteryFlyer::QuitTeam(', [], 'max=4000'),
    ('237C DoUserAnimEvent', 'CMysteryFlyer::DoUserAnimEvent(', [], 'max=4000'),
]
