# ds43: the DarkSamus arms the rooftop fight (tools/drivers/darksamus_ds43.con) cannot reach,
# forced by gdb calls (the ds42 way): gdb boots the game, and at the DS43_AT-th (default 4600)
# CDarkSamus::Think it runs tools/cov/specs/darksamus_ds43_calls.gdb, which writes the members
# each arm needs, calls the body and writes every member back. The ring-path breakpoint below
# makes CDarkSamus::IsRingPathClear answer false while $ring0 is 1 (FindDiveLanding's
# exhausted loop).
#   COV_GDB_SCRIPT=$PWD/tools/cov/specs/darksamus_ds43_calls.py BUILD=$PWD/build-port-cov \
#     MP2_INPUT=LAYER2@400 tools/cov/force.sh tools/cov/specs/darksamus_ds43_calls.py <out> \
#     "1baa96c2,10_cliff_rooftop" --con tools/drivers/darksamus_ds43.con --fast --frames 5900
import os
import gdb

CLASS = 'CDarkSamus'
FORCE = []
HERE = os.path.dirname(os.path.abspath(os.environ['COV_SPEC']))
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


class RetFalse(gdb.FinishBreakpoint):
    def stop(self):
        gdb.execute('set var $rax = 0')
        return False

    def out_of_scope(self):
        pass


class RingBp(gdb.Breakpoint):
    def stop(self):
        flag = gdb.convenience_variable('ring0')
        if flag is not None and int(flag) == 1:
            RetFalse(gdb.newest_frame(), internal=True)
        return False


RingBp(addr('CDarkSamus::IsRingPathClear('), internal=True)


def gate(name):
    v = gdb.convenience_variable(name)
    return v is not None and int(v) == 1


class OnFinish(gdb.FinishBreakpoint):
    def __init__(self, frame, cmd):
        super().__init__(frame, internal=True)
        self.cmd = cmd

    def stop(self):
        if callable(self.cmd):
            self.cmd()
        else:
            gdb.execute(self.cmd)
        return False

    def out_of_scope(self):
        pass


class ForceRet(gdb.Breakpoint):
    """While the convenience variable `gate` is 1, run `cmd` when the function returns."""

    def __init__(self, spec, gate_name, cmd):
        super().__init__(spec, internal=True)
        self.gate_name = gate_name
        self.cmd = cmd

    def stop(self):
        if gate(self.gate_name):
            OnFinish(gdb.newest_frame(), self.cmd)
        return False


class Capture(gdb.Breakpoint):
    """While `gate` is 1, keep `expr` (read at the function entry) in the variable `var`."""

    def __init__(self, spec, gate_name, var, expr):
        super().__init__(spec, internal=True)
        self.gate_name, self.var, self.expr = gate_name, var, expr

    def stop(self):
        if gate(self.gate_name):
            gdb.set_convenience_variable(self.var, int(gdb.parse_and_eval(self.expr)))
        return False


# fn_10_1D860: a FindBestAnimation score of 0 (the pair's float is the low half of rax).
ForceRet(addr('CPASDatabase::FindBestAnimation(CPASAnimParmData const&, CRandom16&'), 'fba0',
         'set var $rax = $rax & 0xffffffff00000000')
# fn_10_B6C4: no object for any uid; a chain with an inactive hint; the first hint it looks up.
ForceRet(addr('CStateManager::GetObjectById('), 'gnull', 'set var $rax = 0')
def keep_first_object():
    if int(gdb.convenience_variable('hp')) == 0:
        gdb.set_convenience_variable('hp', int(gdb.parse_and_eval('$rax')))


ForceRet(addr('CStateManager::GetObjectById('), 'caphint', keep_first_object)
ForceRet(addr('CDarkSamus::BoostChainHasInactiveHint('), 'bchi', 'set var $rax = 1')
# fn_10_1E3C0: the last cover point scored. fn_10_150DC: no cover point; a pick of weight 0.
Capture(addr('CDarkSamus::ScoreCoverPoint('), 'capcov', 'cov', '$rsi')
ForceRet(addr('CDarkSamus::FindBestCoverPoint('), 'fbc0', 'set var $rax = 0xffff')
ForceRet(addr('CDarkSamus::PickDash('), 'pd0', 'set var $xmm0.v4_float[0] = 0')
think = gdb.Breakpoint(addr('CDarkSamus::Think('))
think.ignore_count = int(os.environ.get('DS43_AT', '4600'))
think.commands = ('set logging file %s/ds43_calls.log\nset logging overwrite on\n'
                  'set logging redirect off\nset logging enabled on\n'
                  'source %s/darksamus_ds43_calls.gdb\nset logging enabled off\n'
                  'delete %d\ncontinue' % (OUT, HERE, think.number))
