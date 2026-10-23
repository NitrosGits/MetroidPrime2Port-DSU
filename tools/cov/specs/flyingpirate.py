# FlyingPirate (module 22) coverage. Arms come from the gcov build (build-port-cov); these gdb
# entries only log the states and ids a driver reaches.
# Run: BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/flyingpirate.py <out>
#      42b935e4,08_sand_casualpirates --con tools/cov/specs/flyingpirate_<x>.con --fast
CLASS = 'CFlyingPirate'
SPEC = [
    ('Think', 'CFlyingPirate::Think(', [], 'max=20'),
    ('AcceptScriptMsg', 'CFlyingPirate::AcceptScriptMsg(', ['$rsi', '$rdx & 0xffff'], 'max=2000'),
    ('Death', 'CFlyingPirate::Death(', [], ''),
]
