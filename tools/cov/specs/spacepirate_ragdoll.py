# sp13/sp14 (2026-10-23): the SpacePirate ragdoll (0xb30, xb30_ragDoll). Retail's Think tail
# (fn_72_1980, 0x1F08-0x2030) builds a CPirateRagDoll once the delay 0xb2c, armed by the
# BecomeRagDoll anim event (3B28) or a floating corpse (E498 kSM_AreaLoaded), has counted down;
# every consumer then takes its ragdoll arm. The arms themselves come from the coverage build's gcov
# profile (tools/cov/gcov_report.py); this spec counts the ragdoll pointer and its primed bit at
# the entries, and GetRagDoll's result.
# Run: tools/cov/run.sh <this> <out> 42b935e4,07_sand_alamoportalobservation --driver
#   'python3 tools/drivers/spacepirate_ragdoll_drive.py {port} 3' --fast (BUILD=build-port-cov);
#   the floating corpses: 42b935e4,08_sand_casualpirates (layer -) and 06_sand_alamofront (layer 2).
CLASS = 'CSpacePirate'
RD = '$this->xb30_ragDoll.mPtr'
PRIMED = '(%s != 0 ? (int)%s->mPrimed : -1)' % (RD, RD)
SPEC = [
    ('17A0 GetRagDoll', 'CSpacePirate::GetRagDoll(', [PRIMED], 'retp'),
    ('17C8 GetTouchBounds', 'CSpacePirate::GetTouchBounds(', [PRIMED], 'max=4000'),
    ('1980 Think', 'CSpacePirate::Think(', [PRIMED, '(int)$this->x8f5_30_floatingCorpse'], 'max=3000'),
    ('E3D8 Touch', 'CSpacePirate::Touch(', [PRIMED], 'max=4000'),
    ('E498 AcceptScriptMsg', 'CSpacePirate::AcceptScriptMsg(', [PRIMED, '(int)$rsi'], 'max=4000'),
]
