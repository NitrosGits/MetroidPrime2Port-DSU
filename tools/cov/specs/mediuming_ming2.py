# MediumIng (module 41) ming2: gcov-only run (no gdb breakpoints); the driver mediuming_trig.con forces
# the trigger arms with `var <id> ming.<field>` + `trigcall`. Run on a coverage build:
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/mediuming_ming2.py <out> \
#     3bfa3eff,02_temple_exploration_dark --con tools/cov/specs/mediuming_trig.con --fast --frames 900
CLASS = 'CMediumIng'
SPEC = []
