# ScriptSafeZone (REL 66) on the coverage build: the gcov profile is the evidence; these breakpoints
# only count entries. Drivers: tools/cov/specs/safezone_*.con.
# Run: BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/safezone.py <out> <world,area>
#      --con tools/cov/specs/safezone_<x>.con --fast --frames <n>
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module ScriptSafeZone
CLASS = 'CScriptSafeZone'
SPEC = [
    ('Zone HandleProjectile', 'CScriptSafeZone::HandleProjectile(', [], ''),
    ('Zone SetZoneType', 'CScriptSafeZone::SetZoneType(', [], ''),
    ('Crystal SetState', 'CScriptSafeZoneCrystal::SetState(', [], ''),
    ('Crystal Touch', 'CScriptSafeZoneCrystal::Touch(', [], ''),
]
