# PuddleSpore (module 51, Phlogus) spec for tools/cov/run.sh; arms come from the coverage build
# (gcov_report.py), the breakpoints only count calls and a few entry values. Driver:
#   BUILD=$PWD/build-port-cov SECS=1800 tools/cov/run.sh tools/cov/specs/puddlespore.py <out> \
#     3dfd2249,cbc72d79 --con tools/cov/specs/puddlespore.con --fast --frames 1900
CLASS = 'CPuddleSpore'
SPEC = [
    ('330C AcceptScriptMsg', 'CPuddleSpore::AcceptScriptMsg(', [], 'max=4000'),
    ('17FC InActive', 'CPuddleSpore::InActive(', ['$rdx'], 'max=4000'),
    ('1758 Active', 'CPuddleSpore::Active(', ['$rdx'], 'max=4000'),
    ('1574 Run', 'CPuddleSpore::Run(', ['$rdx'], 'max=4000'),
    ('135C TurnAround', 'CPuddleSpore::TurnAround(', ['$rdx'], 'max=4000'),
    ('118C GetUp', 'CPuddleSpore::GetUp(', ['$rdx'], 'max=4000'),
    ('0FC0 Attack', 'CPuddleSpore::Attack(', ['$rdx'], 'max=4000'),
    ('2FB0 DoUserAnimEvent', 'CPuddleSpore::DoUserAnimEvent(', ['$rcx'], 'max=4000'),
    ('2B98 KnockOffPlayers', 'CPuddleSpore::KnockOffPlayers(', [], 'max=4000'),
    ('21AC SetState', 'CPuddleSpore::SetState(', ['$rdx'], 'max=4000'),
]
