# fp4: FlyingPirate (module 22) CoverFind 6040 line 1105, the `TCastToConstPtr< CScriptCoverPoint >`
# of an AiWaypoint-list object that is not a cover point (no shipped area has one beside an FPRT).
# The first slot of the list (kOL_AiWaypoint) is overwritten with the pirate itself for the one
# CoverFind call and written back right after it (CALLS restores each write at the call's return);
# the other 17 slots stay real cover points, so the walk, the closest pick and SetCoverPoint run
# as in the fp3 scenarios. The second call releases the cover point again.
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/flyingpirate_fp4.py <out> 1baa96c2,01_cliff_entrance \
#     --con tools/cov/specs/flyingpirate_fp3_cliff.con --fast --frames 4200
# $fp4_slot(mgr, list) is the address of mEntity of the list's first object (0 when none).
import gdb

CLASS = 'CFlyingPirate'


class Slot(gdb.Function):
    def __init__(self):
        super().__init__('fp4_slot')

    def invoke(self, mgr, lid):
        base = int(mgr) + int(gdb.parse_and_eval('(long)&((CStateManager*)0)->mObjectLists.mData'))
        lst = int(gdb.parse_and_eval('*(unsigned long*)%d' % (base + 16 * int(lid) + 8)))
        if not lst:
            return gdb.Value(0).cast(gdb.lookup_type('unsigned long'))
        first = int(gdb.parse_and_eval('((CObjectList*)%d)->mFirstId' % lst))
        addr = int(gdb.parse_and_eval('(unsigned long)&((CObjectList*)%d)->mObjects[%d].mEntity' % (lst, first)))
        return gdb.Value(addr).cast(gdb.lookup_type('unsigned long'))


Slot()

CF = '$this->CoverFind(*$mgr, *(CTriggerData*)$this)'
LAND = ('CScriptCoverPoint::ShouldLandHere(', 1)
FREE = ('CScriptCoverPoint::GetInUse(', 0)
BIG = ('$this->mData.mMaxCoverDistance', '1000000.0')
SLOT = '*(unsigned long*)$fp4_slot($mgr, 5)'

CALLS = [
    ('CoverFind non-cover', 'CFlyingPirate::Think(',
     '$fp4_slot((CStateManager*)$rsi, 5) != 0', [
         ([(SLOT, '(unsigned long)$this'), BIG], CF, [LAND, FREE]),
         ([], CF),
     ], 'CoverFind with the first AiWaypoint-list slot presenting the pirate (not a cover point)'),
]
SPEC = []
