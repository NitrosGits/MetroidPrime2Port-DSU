# SwarmBasics (module 80) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Swarms: FSWM Agon 01_sand_entrance L0/L2, PSSM Torvus 0g_swamp_hall L3,
# MSWM Sanctuary 0a_cliff_hall L0, IBSM Temple Grounds 0e_temple_hive_hall L0:
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/swarmbasics.py <out> 42b935e4,01_sand_entrance \
#     --con tools/cov/specs/swarmbasics_sand.con --fast --frames 3000
CLASS = 'CSwarmBasics'
SPEC = [
    ('50B8 Think', 'CSwarmBasics::Think(', [], 'max=4000'),
    ('4A58 UpdatePartition', 'CSwarmBasics::UpdatePartition(', [], 'max=4000'),
    ('220C Touch', 'CSwarmBasics::Touch(', [], 'max=4000'),
]
