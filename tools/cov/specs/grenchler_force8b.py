# gren8 evidence (not coverage): log each loaded Grenchler's grapple swoosh / beam-particle asset ids and guardian flag at the
# `return rs_new CGrenchler(` statement; answers whether the entrance Grenchlers have a swoosh asset (`gren.swoosh` segfault question).
#   COV_ROOT=$PWD BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 tools/cov/force.sh tools/cov/specs/grenchler_force8b.py <out> <world,area> --con <con> --fast --frames 600
import os
import re
CLASS = 'CGrenchler'
_src = open(os.path.join(os.environ.get('COV_ROOT', '.'), 'src/MetroidPrime/Enemies/CGrenchler.cpp')).read().split('\n')
_L = [i + 1 for i, l in enumerate(_src) if re.search(r'^  return rs_new CGrenchler\(', l)][0]


def _log(gdb):
    v = lambda e: gdb.parse_and_eval(e)
    with open(os.path.join(os.environ['COV_OUT'], 'g8b.log'), 'a') as fh:
        fh.write('load: guardian=%s grappleSwoosh=%#x grappleBeamPart=%#x\n' % (
            int(v('sldrThis.isGrappleGuardian')), int(v('sldrThis.grappleSwoosh')) & 0xffffffff,
            int(v('sldrThis.grappleBeamPart')) & 0xffffffff))


FORCE = [('Grenchler loads', 'loc:CGrenchler.cpp:%d' % _L, [], [_log], 0, 'log asset ids')]
