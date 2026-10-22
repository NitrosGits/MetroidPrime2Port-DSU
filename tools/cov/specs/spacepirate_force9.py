# One-call forcing spec (COV_GDB_SCRIPT=tools/cov/force_ret.py) for CE30 GetStrafeDir's
# other-area skip: every pirate of the alamo area stands in this pirate's area, so one call in ten
# reads this pirate as being in area 77 (restored when the call returns).
# The SPEC entry logs LaunchGrenade's message and team id (the driver's statecalls, fn_72_58D8).
# Run (2026-10-22, sp22):
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh <this> <out>
#   42b935e4,07_sand_alamoportalobservation --driver
#   'python3 tools/drivers/spacepirate_strafe.py {port} 2' --frames 15000
import gdb

SPEC = [
    ('58D8 LaunchGrenade (msg, team id)', 'CSpacePirate::LaunchGrenade(',
     ['$rdx', '$this->xba0_teamAiMgrId.value'], ''),
]

CLASS = 'CSpacePirate'
FORCE_RET = [
    ('CE30 GetStrafeDir other area', 'CSpacePirate::GetStrafeDir(', 10, [
        (1, [('$this->mAreaId.value', '77')]),
    ], 'this pirate in area 77 for the call, so every other pirate is in another area'),
]
