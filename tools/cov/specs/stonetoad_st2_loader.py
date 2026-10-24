# st2 loader's second shipped instance: force the optional model empty immediately at
# its validity test. The first instance still constructs normally. This intentionally
# suppresses one temporary model's cleanup; no fabricated stream or shared code changes.
# COV_GDB_SCRIPT=tools/cov/force_ret.py
import gdb
import os

CLASS = 'CStoneToad'
SPEC = []

def log(s):
    with open(os.environ['COV_OUT'] + '/st2-loader.log', 'a') as f:
        f.write(s + '\n')

class Loader(gdb.Breakpoint):
    def __init__(self):
        # In this coverage build the line-597 breakpoint resolves *after* the test;
        # +484 is the cmpb of modelData.m_valid (disassemble /s LoadStoneToad).
        insn = gdb.execute('x/i LoadStoneToad+484', to_string=True)
        assert 'cmpb' in insn and '-0x308(%rbp)' in insn, insn
        super().__init__('*LoadStoneToad+484', internal=True)
        self.n = 0

    def stop(self):
        self.n += 1
        log('loader model test hit %d valid=%s' %
            (self.n, gdb.parse_and_eval('modelData.m_valid')))
        if self.n == 2:
            gdb.execute('set var modelData.m_valid=0')
            log('FORCED second model absent')
        return False

class Dtor(gdb.Breakpoint):
    def __init__(self):
        super().__init__('CStoneToad::~CStoneToad()', internal=True)

    def stop(self):
        log('destructor manager=%s' %
            gdb.parse_and_eval('((CStoneToad*)$rdi)->mCollisionActorManager.mPtr'))
        return False

Loader()
Dtor()
