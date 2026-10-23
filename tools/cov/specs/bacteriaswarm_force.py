# BacteriaSwarm (module 6) forcing spec for tools/cov/force.sh, same area and driver as
# tools/cov/specs/bacteriaswarm.py (merge the two gcov dirs in gcov_report.py):
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/bacteriaswarm_force.py <out> \
#     3bfa3eff,04_temple_unknownenemies_dark --con tools/cov/specs/bacteriaswarm_temple.con --fast --frames 3200
CLASS = 'CBacteriaSwarm'
FORCE = [
    ('3A8 AcceptScriptMsg spawnInstantly', 'CBacteriaSwarm::AcceptScriptMsg(', [],
     ['$this->mSpawnInstantly = 1'], 0,
     'mSpawnInstantly set (every shipped BSWM has 0): the AreaLoaded spawn loop runs'),
    ('touch radius', 'CBacteriaSwarm::Touch(', [], ['$this->mPlayerTouchRadius = 0.0'], 0,
     'mPlayerTouchRadius 0: Touch falls back to mTouchRadius'),
    ('12E0 KillBoid chances', 'CBacteriaSwarm::KillBoid(', [],
     ['$xmm0.v4_float[0] = 0.0', '$xmm1.v4_float[0] = 0.0'], 0,
     'deathRattleChance and deadChance 0 (Touch passes 1.0): neither message is sent'),
    ('3570 Think particle gens', 'CBacteriaSwarm::Think(', [],
     ['$this->mDeathParticleGenerator.mPtr = 0', '$this->mBoidParticleGenerator.mPtr = 0'], 2600,
     'both particle generators null from the 2601st Think (leaked): the no-generator arms'),
]
