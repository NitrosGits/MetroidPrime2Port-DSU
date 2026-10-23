# GunTurret top: the arms the fights do not take, forced for one call each (tools/cov/force_ret.py
# restores every write when the call returns). Run on the coverage build:
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=build-port-cov tools/cov/force.sh \
#     tools/cov/specs/gunturret_force_top.py <out> 42b935e4,0i_sand_hall \
#     --con tools/cov/specs/gunturret_0i_sand_hall.con --fast --frames 3400
# then merge <out>/gcov with the natural runs (gcov_report.py, --module GunTurret).
CLASS = 'CGunTurretTop'
FORCE_RET = [
    ('8420 Death: no base', 'CGunTurretTop::Death(', 2, [(1, [('$this->mBaseId.value', '0xFFFF')])],
     'base lookup null'),
    ('8ED4 Sleep: no light', 'CGunTurretTop::Sleep(', 1, [(0, [('$this->mLightId.value', '0xFFFF')])],
     'light lookup null'),
    ('8E38 Patrol: no light', 'CGunTurretTop::Patrol(', 1, [(0, [('$this->mLightId.value', '0xFFFF')])],
     'light lookup null'),
]
