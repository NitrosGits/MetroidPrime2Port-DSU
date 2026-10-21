# tools/cov/force_cov.py: the coverage harness (tools/cov/gdb_cov.py) plus the forcing
# mechanism (tools/cov/force.py, copied from decomp/darksamus-helpers2), so one run gives both
# the arm counts (cov.txt) and the forced writes (forced.txt). The spec file (COV_SPEC) defines
# both SPEC (the coverage entries) and FORCE (the members to write at the entries).
#
#   COV_GDB_SCRIPT=tools/cov/force_cov.py tools/cov/force.sh <spec.py> <out> <world,area> --con <f>
import os

import gdb

SPEC = os.environ['COV_SPEC']
# the spec lives in <root>/tools/cov/specs/, so the root is four levels up
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(SPEC))))

for script in ('tools/cov/gdb_cov.py', 'tools/cov/force.py'):
    path = os.path.join(ROOT, script)
    exec(compile(open(path).read(), path, 'exec'))
