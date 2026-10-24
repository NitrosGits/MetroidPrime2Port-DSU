# AIMannedTurret (module 1) spec for tools/cov/run.sh; the arms come from the coverage build
# (gcov_report.py --module AIMannedTurret), the breakpoints only count calls per body.
# Turret AI e001d0055, Agon 06_sand_alamofront layer 2 (amt1's driver):
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/aimannedturret.py <out> \
#     42b935e4,06_sand_alamofront --con tools/drivers/aimannedturret_alamofront.con --fast --frames 14000
CLASS = 'CAIMannedTurret'
SPEC = [
    ('CF0 Fire', 'CAIMannedTurret::Fire(', [], 'max=4000'),
    ('1F28 UpdateAim', 'CAIMannedTurret::UpdateAim(', ['$rdx', '$rcx & 0xff'], 'max=8000'),
    ('2C6C Attack', 'CAIMannedTurret::Attack(', ['$rdx'], 'max=4000'),
    ('2AA8 Patrol', 'CAIMannedTurret::Patrol(', ['$rdx'], 'max=4000'),
]
