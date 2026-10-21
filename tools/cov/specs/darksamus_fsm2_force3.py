# DarkSamus FSM part two, the third forcing run (gcov, no breakpoints needed): the phazon waypoint
# lookup fn_10_65CC with hints given types 6 / 7 by the console's `hint` (10_cliff_rooftop ships no
# hint of either type), PhazonRise's move arms toward such a hint, GetUp's CanIssueCommand false
# arm, and two shots in flight for fn_10_1BC58.
# Run: BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/darksamus_fsm2_force3.py <out>
#      "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_fsm2_force3.con --fast --frames 2600
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module DarkSamus
CLASS = 'CDarkSamus'
SPEC = [
    ('65CC GetPhazonWaypoint', 'CDarkSamus::GetPhazonWaypoint(', ['$rdx'], ''),
]
