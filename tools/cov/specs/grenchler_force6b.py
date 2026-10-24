# Grenchler (module 27) gren6: LoadGrenchler 0168 arms the shipped data never takes, written into the
# loader's locals at the line of the test (force.py `loc:`, one write per arm, the first load that reaches it):
#   tailDestroyedSound == -1, tailHitSound == -1, and the model-data-invalid early return (m_valid cleared:
#   that one Grenchler is not created). Driver tools/cov/specs/grenchler_gren6c.con (01_swamp_entrance).
#   BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 tools/cov/force.sh tools/cov/specs/grenchler_force6b.py <out> \
#     3dfd2249,01_swamp_entrance --con tools/cov/specs/grenchler_gren6c.con --fast --frames 600
import re
CLASS = 'CGrenchler'
_src = open(__import__('os').path.join(__import__('os').environ.get('COV_ROOT', '.'),
                                       'src/MetroidPrime/Enemies/CGrenchler.cpp')).read().split('\n')


def _line(pat):
    return [i + 1 for i, l in enumerate(_src) if re.search(pat, l)][0]


def _once(expr):
    st = {'n': 0}

    def f(gdb):
        if st['n'] == 0:
            st['n'] = 1
            gdb.execute('set var ' + expr)
    return f


_L = _line(r'const ushort tailDestroyedSound = sldrThis\.tailDestroyedSound == -1')
_H = _line(r'const ushort tailHitSound =$')


def _model_check_addr():
    # `if (!modelData)` has no line-table entry of its own (the cmpb of m_valid follows the call
    # to LdrToModelData and belongs to no line): break at the first `cmpb $0x0,-N(%rbp)` after that call.
    import os, subprocess
    nm = [l.rstrip('\n').split('\t') for l in open(os.environ['COV_NM'])]
    addr = [int(a.split()[0], 16) for a, d in nm if len(a.split()) == 3 and d.startswith('LoadGrenchler(')][0]
    exe = os.path.join(os.environ.get('BUILD', 'build-port-cov'), 'metroid_prime2_port')
    dis = subprocess.run(['objdump', '-d', '--no-show-raw-insn', '--start-address=%d' % addr,
                          '--stop-address=%d' % (addr + 0x800), exe], capture_output=True, text=True).stdout.split('\n')
    seen = False
    for l in dis:
        if 'LdrToModelData' in l:
            seen = True
        elif seen and re.search(r'cmpb\s+\$0x0,-0x[0-9a-f]+\(%rbp\)', l):
            return int(l.split(':')[0].strip(), 16) - addr
    raise RuntimeError('no m_valid compare')
FORCE = [
    ('LoadGrenchler tailDestroyedSound -1', 'loc:CGrenchler.cpp:%d' % _L, [], [_once('sldrThis.tailDestroyedSound = -1')], 0,
     'the first load takes the unset-sound arm'),
    ('LoadGrenchler tailHitSound -1', 'loc:CGrenchler.cpp:%d' % _H, [], [_once('sldrThis.tailHitSound = -1')], 0,
     'the first load takes the unset-sound arm'),
    ('LoadGrenchler model invalid', "loc:*'LoadGrenchler(CStateManager&, CInputStream&, CEntityInfo&)'+%d" % _model_check_addr(), [], [_once('modelData.m_valid = 0')], 0,
     'the first load returns nullptr (no Grenchler)'),
]
