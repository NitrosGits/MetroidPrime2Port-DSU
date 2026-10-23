# tail24: the de-rezzed Rezbit (forced with `codecall ... SetDeRezzedState`) and one missile.
# Run: tools/cov/run.sh tools/cov/specs/tail24_rezbit_missile.py <out> 1baa96c2,02_cliff_reactorcore \
#        --con tools/drivers/tail24_rezbit_missile.con --fast --frames 2900
# Result: DeflectMissiles 509 entries, 18 of them with a projectile in the near list and deflected
# (CProjectileWeapon::SetWorldSpaceOrientation 18, line 981 18); UpdateDeflectSfx 0 hits because
# nothing in the tree calls it (see /tmp/opencode/reports/tail24.md).
CLASS = 'CRezbit'
SPEC = [
    ('Think rezState', 'CRezbit::Think(', ['$this->mRezState'], 'max=60000'),
    ('SetDeRezzedState', 'CRezbit::SetDeRezzedState(', [], 'max=200'),
    ('955 DeflectMissiles', 'CRezbit::DeflectMissiles(', [], 'calls max=8000'),
    ('981 inside radius', 'loc:CRezbit.cpp:981', [], 'max=8000'),
    ('947 UpdateDeflectSfx', 'CRezbit::UpdateDeflectSfx(', [], 'max=20000'),
    ('SfxManager IsPlaying', 'CSfxManager::IsPlaying(', ['$rdi'], 'ret max=400000'),
]
