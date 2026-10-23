# WallWalker (module 85, Mekenobite) spec for tools/cov/run.sh; arms come from the coverage build
# (gcov_report.py), the breakpoints only count calls and a few entry values. Drivers:
# tools/cov/specs/wallwalker_*.con, e.g.
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/wallwalker.py <out> \
#     1baa96c2,09_cliff_temple --con tools/cov/specs/wallwalker_temple.con --fast --frames 3000
CLASS = 'CWallWalker'
SPEC = [
    ('37E0 Patrol', 'CWallWalker::Patrol(', ['$this->mPatrolState', '$rsi'], 'max=20000'),
    ('3A30 AcceptScriptMsg', 'CWallWalker::AcceptScriptMsg(', ['*(int*)$rdx'], 'max=4000'),
    ('3178 ShouldShootProjectile', 'CWallWalker::ShouldShootProjectile(', [], 'ret max=20000'),
    ('18DC LaunchProjectiles', 'CWallWalker::LaunchProjectiles(', ['$this->mNumShots'], ''),
    ('221C DoUserAnimEvent', 'CWallWalker::DoUserAnimEvent(', ['$rdx'], 'max=4000'),
    ('2340 Dead', 'CWallWalker::Dead(', ['$rsi'], ''),
    ('2ABC UpdateCollisionManager', 'CWallWalker::UpdateCollisionManager(', [], 'max=20000'),
    ('3664 LeftLegHitReaction', 'CWallWalker::LeftLegHitReaction(', ['$rsi'], 'max=4000'),
    ('3484 RightLegHitReaction', 'CWallWalker::RightLegHitReaction(', ['$rsi'], 'max=4000'),
    ('32B8 ShootProjectile', 'CWallWalker::ShootProjectile(', ['$rsi'], 'max=4000'),
]
