# fp4d: FlyingPirate (module 22) Think 15DC, the water max-Z compare (CFlyingPirate.cpp:1922) in both
# directions on 08_sand_casualpirates (flyingpirate_fp4_casual.con). The compare reads GetTranslation()
# (CActor::mPosition), not mTransform.m23 (the fp3 water scenarios wrote the wrong member).
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \\
#     tools/cov/specs/flyingpirate_fp4d.py <out> 42b935e4,08_sand_casualpirates \\
#     --con tools/cov/specs/flyingpirate_fp4_casual.con --fast --frames 2000
import os
_here = os.path.dirname(os.environ['COV_SPEC'])
for _f in ('flyingpirate_fp3.py', 'flyingpirate_fp4.py'):
    exec(compile(open(os.path.join(_here, _f)).read(), _f, 'exec'))

CLASS = 'CFlyingPirate'
TH = '$this->Think(0.016f, *$mgr)'
STUB = [('CActor::InFluidId(', '$fp3_uid(%s)' % WATER)]

CALLS = [
    ('Think water fp4', 'CFlyingPirate::Think(', '((CEntity*)$rdi)->mActive != 0', [
        # the pirate far below the water's top: max Z > 2 + height, pending death set
        ([('$this->mPendingDeath', '0'), ('$this->mPosition.mZ', '-1000000.0')], TH, STUB),
        # far above it: the compare false
        ([('$this->mPendingDeath', '0'), ('$this->mPosition.mZ', '1000000.0')], TH, STUB),
    ], 'Think with the water max-Z compare forced both ways'),
]
SPEC = []
