# CommandoPirate (module 9), slice cp6. The arms come from the coverage build (gcov) and the
# driver tools/drivers/commandopirate_cp6.py, which forces them through the console
# (statecall/trigcall, var cp.*, bodystate, place), each forced value put back in the same tick.
#
# Under gdb (the force run) one more arm: Cover's side (fn_9_6DBC, the attack direction's 0x4 bit
# at 0x00006E50..0x00006E68). Every Swamp cover point has the same bit, so every other
# CScriptCoverPoint::GetAttackDirection call made by CCommandoPirate::Cover answers with the bit
# flipped (the return value only; the cover point is not written).
#   BUILD=build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh
#     tools/cov/specs/commandopirate_cp6.py <out> 'swamp,05_swamp_timber!' --fast --frames 1500
#     --driver 'python3 tools/drivers/commandopirate_cp6.py {port} <out>/drv.txt cover'
import os

CLASS = 'CCommandoPirate'
FORCE_RET = []
SPEC = []

try:
    import gdb
except ImportError:  # read outside gdb
    gdb = None

STATE = {'calls': 0, 'log': 0}


def _log(msg):
    if STATE['log'] < 200:
        STATE['log'] += 1
        with open(os.path.join(os.environ['COV_OUT'], 'forced.log'), 'a') as fh:
            fh.write(msg + '\n')


if gdb is not None:
    class SideRet(gdb.FinishBreakpoint):
        def stop(self):
            old = int(gdb.parse_and_eval('$rax')) & 0xffffffff
            gdb.execute('set $rax = %d' % (old ^ 0x4))
            _log('GetAttackDirection %d -> %d' % (old, old ^ 0x4))
            return False

    class SideBp(gdb.Breakpoint):
        def stop(self):
            # the caller from the return address (the 4-byte leaf has no frame to unwind)
            ret = int(gdb.parse_and_eval('*(unsigned long*)$rsp'))
            name = gdb.execute('info symbol %d' % ret, to_string=True)
            if 'CCommandoPirate::Cover(' not in name:
                return False
            STATE['calls'] += 1
            if STATE['calls'] % 2 == 0:
                try:
                    SideRet(gdb.newest_frame(), internal=True)
                except (gdb.error, ValueError) as e:
                    _log('no GetAttackDirection return: %s' % str(e)[:60])
            return False

    SideBp('CScriptCoverPoint::GetAttackDirection', internal=True)
