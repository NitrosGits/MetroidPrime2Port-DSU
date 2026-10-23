# Shredder (module 68) spec for tools/cov/run.sh; arms come from the coverage build
# (gcov_report.py), the breakpoints only count calls and a few entry values. Drivers:
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/shredder.py <out> \
#     3dfd2249,01_swamp_entrance --con tools/cov/specs/shredder.con --frames 1200
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/shredder.py <out> \
#     3dfd2249,0d_swamp_hall --con tools/cov/specs/shredder_hall.con --frames 1500
CLASS = 'CShredder'
SPEC = [
    ('1EA0 AcceptScriptMsg', 'CShredder::AcceptScriptMsg(', [], 'max=4000'),
    ('1DDC Lurk', 'CShredder::Lurk(', ['$rdx'], 'max=4000'),
    ('1728 Generate', 'CShredder::Generate(', ['$rdx'], 'max=4000'),
    ('14F0 AttackPlayer', 'CShredder::AttackPlayer(', ['$rdx'], 'max=4000'),
    ('12E8 Think', 'CShredder::Think(', [], 'max=4000'),
    ('126C CollidedWith', 'CShredder::CollidedWith(', [], 'max=4000'),
    ('1198 Explode', 'CShredder::Explode(', [], 'max=40'),
    ('10B8 Touch', 'CShredder::Touch(', [], 'max=400'),
    ('0FD4 Listen', 'CShredder::Listen(', ['$rcx'], 'max=400'),
    ('0F80 ShouldWakeUp', 'CShredder::ShouldWakeUp(', [], 'max=4000'),
    ('0EAC IsUnderWater', 'CShredder::IsUnderWater(', [], 'max=4000'),
    ('0D34 ApplySeparation', 'CShredder::ApplySeparation(', [], 'max=4000'),
    ('0BF0 Render', 'CShredder::Render(', [], 'max=40'),
    ('1140 AddReactionAnimation', 'CShredder::AddReactionAnimation(', [], 'max=40'),
]
