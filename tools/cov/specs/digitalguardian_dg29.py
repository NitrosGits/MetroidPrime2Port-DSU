# DigitalGuardian (module 14), dg29: no gdb breakpoints; the runs are gcov runs on build-port-cov
# (tools/cov/gcov_report.py ... --module DigitalGuardian). Drivers: tools/drivers/dg29_body.py
# (the body's triggers, codes, vortex, toes and beacon through `var <id> dg.*`) and
# tools/cov/specs/digitalguardian_msgs.con.
# Run: BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/digitalguardian_dg29.py <out> \
#        1baa96c2,09_cliff_temple_dark --driver 'python3 tools/drivers/dg29_body.py {port}' \
#        --fast --frames 2500
CLASS = 'CDigitalGuardian'
SPEC = []
