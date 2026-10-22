# One-call forcing spec (COV_GDB_SCRIPT=tools/cov/force_ret.py, 02_sand_sandfalls L3) for 10CE4, the
# WallHang waypoint predicate (CSpacePirateWaypointPredicate::IsValid). The wall-hang cover points'
# kSS_Arrived/kSM_Next connections all lead to waypoints, so every other call is asked about the
# invalid id (the host passes the TUniqueId in edx; not restored) and answers false.
# Run (2026-10-22, sp22), the sandfalls wall-hang setup of spacepirate_force5.py:
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov SP_FORCE=Halt SP_STEPS='wait 4;
#   place {cls:CPlayer} -83.1 -72 -9.5; place {000a010e} -78 -96 -10; wait 1; fsm {000a010e} Lurk;
#   wait 25; place {cls:CPlayer} -40.8 -85 15; fsm {000a027a} Lurk; wait 30'
#   tools/cov/force.sh <this> <out> 42b935e4,02_sand_sandfalls --driver
#   'python3 tools/drivers/spacepirate_drive.py {port} 3 1' --frames 10000
import gdb

CLASS = 'CSpacePirate'
SPEC = [
    ('10CE4 IsValid (id)', 'CSpacePirateWaypointPredicate::IsValid(', ['$rdx & 0xffff'], 'ret'),
    ('69AC WallHang (msg, cover)', 'CSpacePirate::WallHang(', ['$rdx', '$this->x900_coverPoint.value'], ''),
]
FORCE_RET = [
    ('10CE4 IsValid invalid id', 'CSpacePirateWaypointPredicate::IsValid(', 2, [
        (0, [('!$rdx', '0xffff')]),
    ], 'every second call asks about the invalid id (no object: false)'),
]
