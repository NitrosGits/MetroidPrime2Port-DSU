# Metroid (module 40) met6 lane: CBabyMetroid is built by LoadMetroidAlpha only when metroidData.xc4_24_ (the baby flag) is set, which no shipped
# placement does; the flag is written for the first load(s) (force.py `loc:` at the `if (metroidData.xc4_24_) {` line) so the baby bodies run.
#   COV_ROOT=$PWD BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 tools/cov/force.sh tools/cov/specs/metroid_force8.py <out> \
#     42b935e4,08_sand_casualpirates --con tools/cov/specs/metroid_force8.con --fast --frames 1500
import os
import re
CLASS = 'CMetroid'
_src = open(os.path.join(os.environ.get('COV_ROOT', '.'), 'src/MetroidPrime/Enemies/CMetroid.cpp')).read().split('\n')
_L = [i + 1 for i, l in enumerate(_src) if re.search(r'^  if \(metroidData\.xc4_24_\) \{', l)][0]
_N = int(os.environ.get('MET_BABIES', '1'))


def _first(n):
    st = {'n': 0}

    def f(gdb):
        st['n'] += 1
        if st['n'] <= n:
            gdb.execute('set var metroidData.xc4_24_ = 1')
    return f


FORCE = [('baby flag', 'loc:CMetroid.cpp:%d' % _L, [], [_first(_N)], 0, 'the first load(s) build a CBabyMetroid')]
