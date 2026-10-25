# fp4e: FlyingPirate (module 22) Think 15DC, CFlyingPirate.cpp:1975: CBurstFire::SetTimeToNextShot's
# `mTimeToNextShot > 0` false after a shot (a burst entry with time 0 and variance 0). A calloc'd SBurst
# (first shot angle 1) replaces mBurstFire.mCurBursts for one call, then every member is written back.
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \\
#     tools/cov/specs/flyingpirate_fp4e.py <out> 1baa96c2,01_cliff_entrance \\
#     --con tools/cov/specs/flyingpirate_fp3_cliff.con --fast --frames 4200
import os
_here = os.path.dirname(os.environ['COV_SPEC'])
for _f in ('flyingpirate_fp3.py', 'flyingpirate_fp4.py'):
    exec(compile(open(os.path.join(_here, _f)).read(), _f, 'exec'))

CLASS = 'CFlyingPirate'
TH = '$this->Think(0.016f, *$mgr)'
BF = '$this->mBurstFire'
BURST = [('$this->mAlive', '1'), ('$this->mBurstTimer', '-1.0'), ('%s.mBurstType' % BF, '0'),
         ('$this->mAimAtTarget', '1')]
NEW = [('%s.mCurBursts' % BF, '(SBurst*)calloc(1, 44)'), ('%s.mAngleIdx' % BF, '-1'),
       ('%s.mTimeToNextShot' % BF, '-1.0'), ('((SBurst*)%s.mCurBursts)->mShotAngles[0]' % BF, '1')]

CALLS = [
    ('Think burst fp4', 'CFlyingPirate::Think(', '((CEntity*)$rdi)->mActive != 0', [
        (BURST + NEW, TH),
    ], 'Think with a shot of burst time 0 forced'),
]
SPEC = []
