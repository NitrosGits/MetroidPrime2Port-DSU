# Glowbug (module 26, Mechanism: Lightflyer) spec for tools/cov/run.sh; arms come from the coverage
# build (gcov_report.py), the breakpoints only count calls and a few entry values. Driver:
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/glowbug.py <out> \
#     863fcd72,0i_temple_hall --con tools/cov/specs/glowbug_templehall.con --fast --frames 2600
CLASS = 'CGlowbug'
SPEC = [
    ('1F68 AcceptScriptMsg', 'CGlowbug::AcceptScriptMsg(', [], 'max=4000'),
    ('1AE4 Think', 'CGlowbug::Think(', [], 'max=4000'),
    ('18F8 Patrol', 'CGlowbug::Patrol(', ['$rdx'], 'max=4000'),
    ('1740 AttackTelegraph', 'CGlowbug::AttackTelegraph(', ['$rdx'], 'max=4000'),
    ('16FC Attack', 'CGlowbug::Attack(', ['$rdx'], 'max=4000'),
    ('10D4 Dead', 'CGlowbug::Dead(', ['$rdx', '$this->mIsInDarkWorld'], 'max=4000'),
    ('151C Death', 'CGlowbug::Death(', ['$this->mIsInDarkWorld'], 'max=4000'),
    ('16A4 ShouldAttack', 'CGlowbug::ShouldAttack(', [], 'ret max=4000'),
    ('160C AttackTelegraphDelay', 'CGlowbug::AttackTelegraphDelay(', [], 'ret max=4000'),
    ('1658 AttackFinished', 'CGlowbug::AttackFinished(', [], 'ret max=4000'),
    ('0E50 ScanVisorRender', 'CGlowbug::ScanVisorRender(', [], 'max=4000'),
]
