# WispTentacle (REL 86) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Temple Grounds 0t_temple_hall_dark layer 0 holds six Darkling
# Tentacles (e00240036 at 8.4 -39.3 -33.4); tools/drivers/wisptentacle_hall.con wakes one, walks the
# states, hits and grabs:
#   BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/wisptentacle.py <out> 3bfa3eff,0t_temple_hall_dark \
#     --con tools/drivers/wisptentacle_hall.con --fast --frames <n>
#   then tools/cov/gcov_report.py build-port-cov <out>/gcov --module WispTentacle
CLASS = 'CWispTentacle'
SPEC = [
    ('Think', 'CWispTentacle::Think(', [], 'max=4000'),
    ('AcceptScriptMsg', 'CWispTentacle::AcceptScriptMsg(', [], 'max=4000'),
    ('Sleep', 'CWispTentacle::Sleep(', ['$rdx'], 'max=400'),
    ('Spawn', 'CWispTentacle::Spawn(', ['$rdx'], 'max=4000'),
    ('Patrol', 'CWispTentacle::Patrol(', ['$rdx'], 'max=4000'),
    ('Search', 'CWispTentacle::Search(', ['$rdx'], 'max=4000'),
    ('Attack', 'CWispTentacle::Attack(', ['$rdx'], 'max=4000'),
    ('Withdraw', 'CWispTentacle::Withdraw(', ['$rdx'], 'max=4000'),
    ('Flinch', 'CWispTentacle::Flinch(', ['$rdx'], 'max=4000'),
    ('Grab', 'CWispTentacle::Grab(', ['$rdx'], 'max=4000'),
    ('Pull', 'CWispTentacle::Pull(', ['$rdx'], 'max=4000'),
    ('KnockBack', 'CWispTentacle::KnockBack(', [], 'max=400'),
    ('DoUserAnimEvent', 'CWispTentacle::DoUserAnimEvent(', ['$rdx'], 'max=400'),
    ('GetOrbitPosition', 'CWispTentacle::GetOrbitPosition(', [], 'max=400'),
    ('GetAimPosition', 'CWispTentacle::GetAimPosition(', [], 'max=400'),
]
