# Tryclops (REL 81), Sanctuary Fortress 0f_cliff_hall (1baa96c2,42c85ad9). Line/branch coverage
# comes from build-port-cov (gcov_report.py --module Tryclops); these entries only log the state flow.
# Run: BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/tryclops.py <out> 1baa96c2,42c85ad9 \
#        --con tools/drivers/tryclops_<grab|struggle|bomb9|beam2|direct>.con --fast --frames 2600 --env GODMODE=1
CLASS = 'CTryclops'
SPEC = [
    ('0x4114 Think', 'CTryclops::Think(', [], 'max=50'),
    ('0x1B58 Death', 'CTryclops::Death(', [], 'calls'),
    ('0x0F90 BallCloseToCollision', 'CTryclops::BallCloseToCollision(', [], 'ret'),
]
