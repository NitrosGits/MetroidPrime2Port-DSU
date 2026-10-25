# fp4c: FlyingPirate (module 22) DoUserAnimEvent 2738 arms (its own run: one CALLS body per gdb stop) that no shipped fight
# reaches, forced on the coverage build by direct calls (CALLS: members written, the body called,
# members written back; a stub makes a callee return at once). The helpers ($fp3_obj, $fp3_uid,
# $fp4_slot) are those of flyingpirate_fp3.py / flyingpirate_fp4.py, loaded from this directory.
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/flyingpirate_fp4c.py <out> 1baa96c2,01_cliff_entrance \
#     --con tools/cov/specs/flyingpirate_fp3_cliff.con --fast --frames 4200
#
import os
_here = os.path.dirname(os.environ['COV_SPEC'])
for _f in ('flyingpirate_fp3.py', 'flyingpirate_fp4.py'):
    exec(compile(open(os.path.join(_here, _f)).read(), _f, 'exec'))

CLASS = 'CFlyingPirate'
TH = '$this->Think(0.016f, *$mgr)'
BC = '$this->mBodyController.mPtr'
BURST = [('$this->mAlive', '1'), ('$this->mBurstTimer', '-1.0'), ('$this->mBurstFire.mBurstType', '0'),
         ('$this->mAimAtTarget', '1')]
LAI = '$fp3_obj($mgr, 4, %d, "CFlyingPirate")'
ANIM = '$this->DoUserAnimEvent(*$mgr, *(CInt32POINode*)$rdx, %d, 0.016f)'
NOTLOADED = [('CObjectReference::IsLoaded(', 0)]
NOCREATE = [('CStateManager::CanCreateProjectile(', 0)]

CALLS = [
    ('DoUserAnimEvent arms fp4', 'CFlyingPirate::DoUserAnimEvent(', '((CEntity*)$rdi)->mActive != 0 && (int)$ecx == 0', [
        ([('$this->mIsAttackingObject', '1'), ('$this->mAttackObjectId.value', '$fp3_uid($fp3_obj($mgr, -1, 0, "CFlyingPirate"))')], ANIM % 0),
        ([('$this->mIsAttackingObject', '0'), ('$this->mIsAquaPirate', '1')], ANIM % 0),
        ([('$this->mIsAttackingObject', '1'), ('$this->mIsAquaPirate', '1')], ANIM % 0),
        ([], ANIM % 0, NOTLOADED),
        ([], ANIM % 0, NOCREATE),
        ([], ANIM % 19),
        ([], ANIM % 7),
    ], 'DoUserAnimEvent with the attack-object / aqua / token-not-loaded / cannot-create / ragdoll / default arms'),
]
SPEC = []
