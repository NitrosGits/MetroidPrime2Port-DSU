# GunTurret base: AttackExitDone with the gun destroyed (fn_28_60C0's early `return true`), forced for
# one call each (tools/cov/force_ret.py restores the write when the call returns):
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=build-port-cov tools/cov/force.sh \
#     tools/cov/specs/gunturret_force3.py <out> 42b935e4,0i_sand_hall \
#     --con tools/cov/specs/gunturret_force3.con --fast --frames 1200
CLASS = 'CGunTurretBase'
FORCE_RET = [
    ('60C0 AttackExitDone: destroyed', 'CGunTurretBase::AttackExitDone(', 2,
     [(1, [('$this->mGunDestroyed', '1')])], 'gun destroyed'),
]
