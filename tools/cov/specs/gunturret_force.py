# GunTurret base: the arms the fights do not take, forced for one call each (tools/cov/force_ret.py
# restores every write when the call returns). Run on the coverage build:
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=build-port-cov tools/cov/force.sh \
#     tools/cov/specs/gunturret_force.py <out> 42b935e4,0i_sand_hall \
#     --con tools/cov/specs/gunturret_0i_sand_hall.con --frames 2400 (no --fast: the per-frame bodies stop gdb each update; SECS=1800)
# then merge <out>/gcov with the natural runs (gcov_report.py, --module GunTurret).
CLASS = 'CGunTurretBase'
NO_TOP = [('$this->mTopId.value', '0xFFFF')]
FORCE_RET = [
    ('48D0 Sleep: no top', 'CGunTurretBase::Sleep(', 1, [(0, NO_TOP)], 'top lookup null'),
    ('483C Patrol: no top', 'CGunTurretBase::Patrol(', 1, [(0, NO_TOP)], 'top lookup null'),
    ('46F4 Attack: no top', 'CGunTurretBase::Attack(', 1, [(0, NO_TOP)], 'top lookup null'),
    ('496C Spawn: no top', 'CGunTurretBase::Spawn(', 1, [(0, NO_TOP)], 'top lookup null'),
    ('2908 ResetAttack: no top', 'CGunTurretBase::ResetAttack(', 2, [(1, NO_TOP)], 'top lookup null'),
    ('60C0 AttackExitDone: destroyed', 'CGunTurretBase::AttackExitDone(', 2,
     [(1, [('$this->mGunDestroyed', '1')])], 'gun destroyed'),
]
