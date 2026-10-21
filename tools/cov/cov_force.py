# gdb side of a forcing run that also counts arms: tools/cov/force.py (the spec's FORCE list)
# and then tools/cov/gdb_cov.py (its SPEC list), each in its own namespace. force.py's
# breakpoints are made first, so at a shared entry gdb runs its write before gdb_cov.py
# evaluates the arm key - the key shows the forced value.
#   COV_GDB_SCRIPT=tools/cov/cov_force.py tools/cov/force.sh <spec> <out> <world,area> ...
import os

HERE = os.path.dirname(os.path.abspath(__file__)) if '__file__' in globals() else 'tools/cov'
for name in ('force.py', 'gdb_cov.py'):
    path = os.path.join(HERE, name)
    exec(compile(open(path).read(), path, 'exec'), {'__name__': '__main__', '__file__': path})
