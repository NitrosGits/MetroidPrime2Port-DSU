# PlantScarabSwarm (module 49) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py),
# the breakpoints only count calls. Swarm: PSSM u049e, Torvus Bog 0g_swamp_hall L3:
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/plantscarabswarm.py <out> \
#     3dfd2249,0g_swamp_hall --con tools/cov/specs/plantscarabswarm_swamp.con --fast --frames 3000
CLASS = 'CPlantScarabSwarm'
SPEC = [
    ('1B64 Think', 'CPlantScarabSwarm::Think(', [], 'max=4000'),
    ('C6C SpawnGrenades', 'CPlantScarabSwarm::SpawnGrenades(', [], 'max=4000'),
    ('10D0 KillBoid', 'CPlantScarabSwarm::KillBoid(', [], 'max=4000'),
]
