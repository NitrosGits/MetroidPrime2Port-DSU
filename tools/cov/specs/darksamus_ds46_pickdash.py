# ds46: the arms of fn_10_154FC PickDash the rooftop fight does not reach, forced by gdb calls on
# the ds44 harness (darksamus_ds44_calls.py: ForceRet, ds45-scratch, the Think breakpoint), which
# runs tools/cov/specs/darksamus_ds46_pickdash.gdb at the DS44_AT-th CDarkSamus::Think instead of
# its own script. Every member the .gdb writes is written back.
#   COV_GDB_SCRIPT=$PWD/tools/cov/specs/darksamus_ds46_pickdash.py BUILD=$PWD/build-port-cov \
#     MP2_INPUT=LAYER2@400 tools/cov/force.sh tools/cov/specs/darksamus_ds46_pickdash.py <out> \
#     "1baa96c2,10_cliff_rooftop" --con tools/drivers/darksamus_ds43.con --fast --frames 5900
import os

os.environ.setdefault('DS44_GDB', 'darksamus_ds46_pickdash.gdb')
_here = os.path.dirname(os.path.abspath(os.environ['COV_SPEC']))
exec(open(os.path.join(_here, 'darksamus_ds44_calls.py')).read())

# $hval: GetHeightOfPointAboveMesh answers it (0 kR_Success, 1 a failure) while $hgate is set.
ForceRet(addr('CPathFindSearch::GetHeightOfPointAboveMesh('), 'hgate', 'set var $rax = $hval')
# $clrv: GetDashPathClearance answers it while $clrgate is set.
ForceRet(addr('CDarkSamus::GetDashPathClearance('), 'clrgate',
         'set var $xmm0.v4_float[0] = $clrv')


