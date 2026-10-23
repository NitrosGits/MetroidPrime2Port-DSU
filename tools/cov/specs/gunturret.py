# GunTurret (REL 28) on the coverage build: the gcov profile is the evidence; these breakpoints only
# count a few entries. Drivers: tools/cov/specs/gunturret_*.con.
# Run: BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/gunturret.py <out> <world,area>
#      --con tools/cov/specs/gunturret_<x>.con --fast --frames <n>
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module GunTurret
CLASS = 'CGunTurretBase'
SPEC = [
    ('LaunchProjectile 1E00', 'CGunTurretBase::LaunchProjectile(', [], ''),
    ('DestroyGun', 'CGunTurretBase::DestroyGun(', [], ''),
    ('Top Death', 'CGunTurretTop::Death(', [], ''),
]
