# dsscat: the Dark Samus REL's CScattershotProjectile (src/MetroidPrime/Enemies/PortCScattershotProjectile.cpp).
#   BUILD=$PWD/build-port-cov MP2_INPUT=LAYER2@400 tools/cov/run.sh tools/cov/specs/darksamus_scat.py <out> \
#     "1baa96c2,10_cliff_rooftop" --con tools/drivers/darksamus_scat.con --fast --frames 2000
# The arms themselves come from the gcov report of the same run (tools/cov/gcov_report.py
# build-port-cov <out>/gcov --module DarkSamus); this spec counts the calls per key.
CLASS = 'CScattershotProjectile'

SPEC = [
    ('C98 AddShot', 'CScattershotProjectile::AddShot(',
     ['*(int*)&$this->mShots', '*(int*)&$this->mShots % 17 == 0', '*(int*)&$this->mShots % 13 == 0',
      '*(int*)&$this->mShots % 5 == 0'], ''),
    ('2C CheckPlayerHit', 'CScattershotProjectile::CheckPlayerHit(', [], 'calls max=4000'),
    ('324 Sweep', 'CScattershotProjectile::Sweep(', [], 'calls'),
    ('1D8C SpawnHitExplosion', 'CScattershotProjectile::SpawnHitExplosion(',
     ['((float*)$rdx)[6] < 2.0'], 'calls'),
    ('213C PlayHitSfx', 'CScattershotProjectile::PlayHitSfx(', [], 'calls'),
    ('2918 Think', 'CScattershotProjectile::Think(', ['$xmm0.v4_float[0] <= 0', '$this->mLifetime > 10.0'], 'max=4000'),
]
