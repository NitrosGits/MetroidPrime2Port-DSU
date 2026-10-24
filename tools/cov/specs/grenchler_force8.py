# Grenchler (module 27) gren8: CGrenchler ctor 11580 arms the shipped data never takes, written into LoadGrenchler's locals at the
# `return rs_new CGrenchler(` statement (force.py `loc:`; the three entrance Grenchlers load in turn):
#   load 1: tailDestroyedHealth = -5 (retail 0x11xxx `mTailHealth < 0`), load 2: tailDestroyedHealth = 1e9 (> HP: clamped),
#   load 3: ingPossessionData.isAnEncounter = 1 (the 0x200 pathfinding flag) and shallowWaterRing = invalid (SEffectD without its asset).
#   BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 COV_ROOT=$PWD tools/cov/force.sh tools/cov/specs/grenchler_force8.py <out> \
#     3dfd2249,01_swamp_entrance --con tools/cov/specs/grenchler_gren6c.con --fast --frames 700
import os
import re
CLASS = 'CGrenchler'
_src = open(os.path.join(os.environ.get('COV_ROOT', '.'), 'src/MetroidPrime/Enemies/CGrenchler.cpp')).read().split('\n')
_L = [i + 1 for i, l in enumerate(_src) if re.search(r'^  return rs_new CGrenchler\(', l)][0]


def _seq(*steps):
    st = {'n': 0}

    def f(gdb):
        i = st['n']
        st['n'] += 1
        if i < len(steps):
            for e in steps[i]:
                gdb.execute('set var ' + e)
    return f


FORCE = [('CGrenchler ctor inputs', 'loc:CGrenchler.cpp:%d' % _L, [],
          [_seq(['sldrThis.tailDestroyedHealth = -5.0'], ['sldrThis.tailDestroyedHealth = 1000000000.0'],
                ['sldrThis.ingPossessionData.isAnEncounter = 1', 'sldrThis.shallowWaterRing = 4294967295'])], 0,
          'load 1 negative tail health, load 2 tail health above HP, load 3 encounter flag and no shallow-water ring')]
