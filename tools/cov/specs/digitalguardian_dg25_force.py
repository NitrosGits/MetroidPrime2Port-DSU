# DigitalGuardian (module 14), dg25: EjectPlayerFromBombPit's (fn_14_14058) no-push arm, taken
# when the push direction (the X column, either way, plus the Z column) is not magnitude-safe
# (clrlwi./beq at 0x1413C). The head's columns are orthonormal, so the sum is zeroed at the test.
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg25_force.py \
#        <out> 1baa96c2,09_cliff_temple_dark --con tools/cov/specs/digitalguardian_dg25.con \
#        --fast --frames 450
CLASS = 'CDigitalGuardianHead'
SPEC = []
FORCE = [
    ('14058 the push direction is unsafe', 'loc:PortCDigitalGuardianHead.cpp:2543', [],
     ['dir.mX = 0', 'dir.mY = 0', 'dir.mZ = 0'], 0, 'zeroes the push direction (dir)'),
]
