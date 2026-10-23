# PirateRagDoll (module 50, prd1): the pirate death ragdoll CPirateRagDoll. The arms come from the
# coverage build's gcov profile (tools/cov/gcov_report.py --module PirateRagDoll); this spec only
# counts the entries with the attached-actor bit, the waypoint pins and the over/hit flags.
# Runs (BUILD=build-port-cov), the SpacePirate ragdoll driver (docs/module-analysis/PirateRagDoll.md):
#   tools/cov/run.sh tools/cov/specs/pirateragdoll.py <out> 42b935e4,07_sand_alamoportalobservation \
#     --driver 'python3 tools/drivers/spacepirate_ragdoll_drive.py {port} 3' --fast
CLASS = 'CPirateRagDoll'
ATT = '(int)$this->mActorAttached'
WP = '(int)($this->mWaypointIds.mCount)'
SPEC = [
    ('1938 ctor', 'CPirateRagDoll::CPirateRagDoll(', ['$rsi != 0'], ''),
    ('1874 Prime', 'CPirateRagDoll::Prime(', [ATT], ''),
    ('E10 Update', 'CPirateRagDoll::Update(', [ATT, WP, '(int)$this->mOver', '(int)$this->mHitByProjectile'],
     'max=4000'),
    ('1610 CheckStatic', 'CPirateRagDoll::CheckStatic(', [WP, '(int)$this->mNoOverTimer'], 'max=4000'),
    ('4F8 PreRender', 'CPirateRagDoll::PreRender(', [ATT], 'max=4000'),
    ('3B4 UpdateImpactSfx', 'CPirateRagDoll::UpdateImpactSfx(', ['(int)$this->mInitSfx'], 'max=4000'),
    ('10C AlignShoulderPad', 'CPirateRagDoll::AlignShoulderPad(', ['$r9 != 0'], 'max=4000'),
]
