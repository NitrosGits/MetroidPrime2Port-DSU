# DigitalGuardian (module 14), dg33: fn_14_308C's non-waypoint arm (the TCastToPtr<CScriptWaypoint>
# null test, beq at 0x3114): data-dead, every IS02/FOLW link of the area-53 body (00350017/1E/63/6A)
# is a waypoint and FindConnectedObjects lists only loaded ids; from the 5th test on (after the first ShouldJumpAttack's four) the cast
# answers null. The beacon and fn_14_308C's inactive waypoint come from the .con.
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg33_beacon.py \
#        <out> 1baa96c2,09_cliff_temple_dark --con tools/cov/specs/digitalguardian_dg33_beacon.con \
#        --fast --frames 600
CLASS = 'CDigitalGuardian'
SPEC = []
FORCE = [
    ('308C not a waypoint', 'loc:PortCDigitalGuardian.cpp:1838', [], ['waypoint = 0'], 4,
     'the connected object is no waypoint'),
]
