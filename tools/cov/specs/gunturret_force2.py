# GunTurret base, per-frame bodies: the arms the fights do not take, forced for one call each (tools/cov/force_ret.py
# restores every write when the call returns). Run on the coverage build:
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=build-port-cov tools/cov/force.sh \
#     tools/cov/specs/gunturret_force2.py <out> 42b935e4,0i_sand_hall \
#     --con tools/cov/specs/gunturret_0i_sand_hall.con --frames 3400 (no --fast)
# then merge <out>/gcov with the natural runs (gcov_report.py, --module GunTurret).
CLASS = 'CGunTurretBase'
NO_TOP = [('$this->mTopId.value', '0xFFFF')]
FORCE_RET = [
    ('3520 UpdateGunOrientation: no top', 'CGunTurretBase::UpdateGunOrientation(', 40, [(1, NO_TOP)],
     'top lookup null'),
    ('616C ShouldAttack: not alert / occluded', 'CGunTurretBase::ShouldAttack(', 40,
     [(1, [('$this->mAlert', '0')]), (2, [('$this->mOccluded', '1')])], 'alert 0 / occluded 1'),
]
