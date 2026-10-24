# Lumite (module 39) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Agon Wastes 0a_sand_hall / 0j_sand_hall_dark hold Lumites
# (02_sand_sandfalls layer 5, 08_sand_casualpirates_dark layer 3, Temple Grounds 0q_temple_hall layer 2);
# tools/drivers/lumite_states.py walks the state table, damages and kills them:
#   BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/lumite.py <out> 42b935e4,0a_sand_hall \
#     --driver 'python3 tools/drivers/lumite_states.py {port} <out>/states.log' --fast --frames 6000
CLASS = 'CLumite'
SPEC = [
    ('52C0 Think', 'CLumite::Think(', [], 'max=4000'),
    ('5D50 AcceptScriptMsg', 'CLumite::AcceptScriptMsg(', [], 'max=4000'),
    ('3E4C FacingHint', 'CLumite::FacingHint(', [], 'ret max=2000'),
    ('2244 BuildHopCandidates', 'CLumite::BuildHopCandidates(', [], ''),
    ('12A8 GetGravityConstant', 'CLumite::GetGravityConstant(', [], 'max=100'),
]
