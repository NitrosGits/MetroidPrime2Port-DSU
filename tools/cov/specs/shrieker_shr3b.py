# Shrieker shr3 lane: every loaded Shrieker without a shriek effect (pART invalid at the loader), so DoUserAnimEvent's `mEffectDescs[i].valid()`
# false arm runs when the shriek event fires. Driver shrieker_shr3.con.
#   COV_ROOT=$PWD BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 tools/cov/force.sh tools/cov/specs/shrieker_shr3b.py <out> 3dfd2249,02_swamp_greatbridge \
#     --con tools/cov/specs/shrieker_shr3.con --fast --frames 1700
import os
import re
CLASS = 'CShrieker'
_src = open(os.path.join(os.environ.get('COV_ROOT', '.'), 'src/MetroidPrime/Enemies/CShrieker.cpp')).read().split('\n')
_D = [i + 1 for i, l in enumerate(_src) if re.search(r'^  const CShriekerData data\(', l)][0]
FORCE = [('shriek effect invalid', 'loc:CShrieker.cpp:%d' % _D, [], ['sldrThis.pART = 4294967295'], 0, 'no shriek effect on any Shrieker')]
