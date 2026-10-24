# rez6: why does `send <rezbit> Create` crash in CBSFall::Start? Logs every CBodyController::Activate / CBSFall::Start.
#   COV_GDB_SCRIPT=<this> BUILD=$PWD/build-port-cov SECS=300 tools/cov/force.sh <this> <out> 1baa96c2,02_cliff_reactorcore --con tools/drivers/rezbit_create.con --fast --frames 1200
import os
import gdb
CLASS = 'CRezbit'
FORCE = []
OUT = os.environ['COV_OUT']
syms = []
for ln in open(os.environ['COV_NM']):
    parts = ln.rstrip('\n').split('\t')
    if len(parts) == 2:
        f = parts[0].split()
        if len(f) == 3 and f[1] in 'tTW':
            syms.append((f[2], parts[1]))


def addr(prefix):
    hits = [m for m, d in syms if d.startswith(prefix) and '.cold' not in d and 'clone' not in d]
    if len(hits) != 1:
        raise gdb.GdbError('%s: %d symbols' % (prefix, len(hits)))
    return '*' + hits[0]


def log(m):
    with open(OUT + '/dbg.log', 'a') as fh:
        fh.write(m + '\n')


class Act(gdb.Breakpoint):
    def stop(self):
        try:
            bc = gdb.parse_and_eval('(CBodyController*)$rdi')
            log('Activate this=%s state=%d active=%d cur=%s' % (bc, int(gdb.parse_and_eval('$edx')), int(bc['mActive']),
                                                              bc['mBodyStateInfo']['mState']))
        except gdb.error as e:
            log('Activate err %s' % e)
        return False


class Fall(gdb.Breakpoint):
    def stop(self):
        try:
            bc = gdb.parse_and_eval('(CBodyController*)$rsi')
            cmd = bc['mCmdMgr'].address.cast(gdb.lookup_type('CBodyStateCmdMgr').pointer())
            log('Fall::Start bc=%s delivered[KnockDown]=%s' % (bc, cmd['mDeliveredCommands']['mData'][int(gdb.parse_and_eval('(int)kBSC_KnockDown'))]))
            log('  default state %s' % gdb.parse_and_eval('(int)((CBodyController*)$rsi)->GetPASDatabase().GetDefaultState()'))
        except gdb.error as e:
            log('Fall err %s' % e)
        return False


Act(addr('CBodyController::Activate('))
Fall(addr('CBSFall::Start('))
