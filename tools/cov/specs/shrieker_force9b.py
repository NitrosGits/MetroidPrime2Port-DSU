# Shrieker (module 69) gren9 lane: LoadShrieker 154 and CShrieker ctor 62C0 arms the shipped data never takes, written into the
# loader's locals (force.py `loc:`): load 1 model data invalid (no Shrieker), load 2 pART invalid (no shriek effect), load 3
# meleeEffect invalid (no melee effect). The m_valid compare has no line-table entry: break at the first `cmpb $0,-N(%rbp)` after
# the LdrToModelData call (as grenchler_force6b.py).
#   COV_ROOT=$PWD BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 tools/cov/force.sh tools/cov/specs/shrieker_force9b.py <out> \
#     3dfd2249,02_swamp_greatbridge --con tools/cov/specs/shrieker_force9.con --fast --frames 700
import os
import re
import subprocess
CLASS = 'CShrieker'
_src = open(os.path.join(os.environ.get('COV_ROOT', '.'), 'src/MetroidPrime/Enemies/CShrieker.cpp')).read().split('\n')
_D = [i + 1 for i, l in enumerate(_src) if re.search(r'^  const CShriekerData data\(', l)][0]


def _model_check():
    nm = [l.rstrip('\n').split('\t') for l in open(os.environ['COV_NM'])]
    addr = [int(a.split()[0], 16) for a, d in nm if len(a.split()) == 3 and d.startswith('LoadShrieker(')][0]
    exe = os.path.join(os.environ.get('BUILD', 'build-port-cov'), 'metroid_prime2_port')
    dis = subprocess.run(['objdump', '-d', '--no-show-raw-insn', '--start-address=%d' % addr, '--stop-address=%d' % (addr + 0x800), exe],
                         capture_output=True, text=True).stdout.split('\n')
    seen = False
    for l in dis:
        if 'LdrToModelData' in l:
            seen = True
        elif seen and re.search(r'cmpb\s+\$0x0,-0x[0-9a-f]+\(%rbp\)', l):
            return int(l.split(':')[0].strip(), 16) - addr
    raise RuntimeError('no m_valid compare')


def _once(*exprs, skip=0):
    st = {'n': 0}

    def f(gdb):
        st['n'] += 1
        if st['n'] == 1 + skip:
            for e in exprs:
                gdb.execute('set var ' + e)
    return f


FORCE = [
    ('LoadShrieker model invalid', "loc:*'LoadShrieker(CStateManager&, CInputStream&, CEntityInfo&)'+%d" % _model_check(), [],
     [_once('modelData.m_valid = 0')], 0, 'the first load returns nullptr'),
    ('CShrieker shriek effect invalid', 'loc:CShrieker.cpp:%d' % _D, [], [_once('sldrThis.pART = 4294967295')], 0, 'first surviving load: no shriek effect'),
    ('CShrieker melee effect invalid', 'loc:CShrieker.cpp:%d' % _D, [], [_once('sldrThis.meleeEffect = 4294967295', skip=1)], 0,
     'second surviving load: no melee effect'),
]
