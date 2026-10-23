# IngSpiderballGuardian (module 35, Spider Guardian) spec for tools/cov/run.sh; arms come from the
# coverage build (gcov_report.py), the breakpoints only count calls and a few entry values. Driver:
#   BUILD=$PWD/build-port-cov SECS=1800 tools/cov/run.sh tools/cov/specs/isbg.py <out> \
#     1baa96c2,06_cliff_spiderballguardian --con tools/cov/specs/isbg.con --fast --frames 4000
CLASS = 'CIngSpiderballGuardian'
SPEC = [
    ('3214 AcceptScriptMsg', 'CIngSpiderballGuardian::AcceptScriptMsg(', [], 'max=4000'),
    ('0F8C ApplyStunDamage', 'CIngSpiderballGuardian::ApplyStunDamage(', ['$this->mState', '$xmm0.v4_float[0]'], 'max=4000'),
    ('1084 SetGuardianState', 'CIngSpiderballGuardian::SetGuardianState(', ['$this->mState', '$rdx', '$this->mPhase'], 'max=4000'),
    ('1D90 SelectNextWaypoint', 'CIngSpiderballGuardian::SelectNextWaypoint(', ['$this->mTargetWaypointId.value'], 'max=40000'),
    ('303C DoUserAnimEvent', 'CIngSpiderballGuardian::DoUserAnimEvent(', ['$rcx'], 'max=4000'),
    ('2DFC Death', 'CIngSpiderballGuardian::Death(', [], ''),
    ('2534 Dead', 'CIngSpiderballGuardian::Dead(', ['$rsi'], 'max=4000'),
    ('296C DamageReaction', 'CIngSpiderballGuardian::DamageReaction(', ['$rsi'], 'max=4000'),
    ('25E4 StunnedReaction', 'CIngSpiderballGuardian::StunnedReaction(', ['$rsi'], 'max=4000'),
    ('2470 BeginNextPuzzleSection', 'CIngSpiderballGuardian::BeginNextPuzzleSection(', ['$this->mPhase'], ''),
]
