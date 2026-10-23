# DarkTrooper (REL 12) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Temple Grounds 04_temple_unknownenemies layer 2 holds Dark Troopers;
# tools/drivers/darktrooper_unknown.con wakes them, walks the state table, damages and kills them:
#   BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/darktrooper.py <out> 3bfa3eff,04_temple_unknownenemies \
#     --con tools/drivers/darktrooper_unknown.con --fast --frames <n>
#   then tools/cov/gcov_report.py build-port-cov <out>/gcov --module DarkTrooper
CLASS = 'CDarkTrooper'
SPEC = [
    ('Think', 'CDarkTrooper::Think(', [], 'max=4000'),
    ('AcceptScriptMsg', 'CDarkTrooper::AcceptScriptMsg(', [], 'max=4000'),
    ('DoUserAnimEvent', 'CDarkTrooper::DoUserAnimEvent(', ['$rdx'], 'max=400'),
    ('FireSmallShot', 'CDarkTrooper::FireSmallShot(', [], 'max=400'),
    ('FireMissile', 'CDarkTrooper::FireMissile(', [], 'max=400'),
    ('Sleep', 'CDarkTrooper::Sleep(', ['$rdx'], 'max=400'),
    ('Rise', 'CDarkTrooper::Rise(', ['$rdx'], 'max=400'),
    ('Pursue', 'CDarkTrooper::Pursue(', ['$rdx'], 'max=4000'),
    ('SmallShotAttack', 'CDarkTrooper::SmallShotAttack(', ['$rdx'], 'max=4000'),
    ('MissileAttack', 'CDarkTrooper::MissileAttack(', ['$rdx'], 'max=4000'),
    ('MeleeAttack', 'CDarkTrooper::MeleeAttack(', ['$rdx'], 'max=4000'),
    ('Pause', 'CDarkTrooper::Pause(', ['$rdx'], 'max=4000'),
    ('Dead', 'CDarkTrooper::Dead(', ['$rdx'], 'max=400'),
    ('CreateRagDoll', 'CDarkTrooper::CreateRagDoll(', [], 'max=400'),
    ('KnockBack', 'CDarkTrooper::KnockBack(', [], 'max=400'),
    ('Touch', 'CDarkTrooper::Touch(', [], 'max=400'),
]
