# DarkCommando (module 3) forcing spec for tools/cov/force.sh: the EMP grenade's duration.
# The shipped grenades all carry an EMP duration over 0 and the area's fight lets each one run to
# its end, so the `eMPDuration > 0` false arm of fn_3_8BEC CDarkCommandoGrenade::Think never
# runs; here the duration is 100 for the first 0.3 s after the explosion (the not-yet-expired
# arm) and 0 afterwards (the `> 0` false arm, then the delete).
# Run (coverage build, the melee driver's FireEMP part throws the grenades):
#   BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darkcommando_emp_force.py <out> \
#     3bfa3eff,06_temple_sacredpath_dark --driver 'python3 tools/drivers/darkcommando_melee.py {port} <out>/melee.log' \
#     --frames 9000 --env MP2_GODMODE=1
CLASS = 'CDarkCommandoGrenade'
FORCE = [
    ('8BEC Grenade::Think: EMP duration (forced)', 'CDarkCommandoGrenade::Think(',
     ['$this->mEMPTime < 0.3'],
     ['$this->mData.eMPDuration = $this->mEMPTime < 0.3 ? 100.0 : 0.0'], 0,
     'eMPDuration 100 until 0.3 s after the explosion, then 0'),
]
