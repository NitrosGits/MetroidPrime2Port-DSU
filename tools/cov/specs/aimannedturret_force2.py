# AIMannedTurret (module 1) stubbed-call forcing spec for COV_GDB_SCRIPT=tools/cov/force_ret.py:
#   COV_GDB_SCRIPT=$PWD/tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/aimannedturret_force2.py <out> 42b935e4,06_sand_alamofront \
#     --con tools/drivers/aimannedturret_alamofront.con --fast --frames 14000
# The arms behind a call result: the weapon token reports loaded only after its object streams in
# (Fire 585), and the projectile is never null unless the allocator fails (Fire 596).
CLASS = 'CAIMannedTurret'

FORCE_INNER = [
    ('0CF0 Fire weapon not loaded', 'CAIMannedTurret::Fire(', 'CObjectReference::IsLoaded(', 2,
     {0: [('$rax', '0')]}, 'the mWeaponToken.IsLoaded() false arm (line 585)'),
    ('0CF0 Fire no projectile', 'CAIMannedTurret::Fire(', 'operator new(', 2,
     {1: [('$rax', '0')]}, 'the projectile != nullptr false arm (line 596): a failed allocation'),
]
