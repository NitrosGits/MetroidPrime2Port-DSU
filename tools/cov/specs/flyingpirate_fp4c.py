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
# TCachedToken::IsLoaded is `mItem != nullptr || mObjRef->mObject != nullptr` (both inline)
TOK = '$this->mAltProjectileInfo1.mWeaponDescription'
NOTLOADED = [('%s.mItem' % TOK, '0'), ('%s.mObjRef->mObject' % TOK, '0')]
TOK2 = '$this->mAltProjectileInfo2.mWeaponDescription'
NOTLOADED2 = [('%s.mItem' % TOK2, '0'), ('%s.mObjRef->mObject' % TOK2, '0')]
ATK = [('$this->mIsAttackingObject', '1'), ('$this->mAttackObjectId.value', '$fp3_uid($fp3_obj($mgr, -1, 0, "CFlyingPirate"))')]
NOCREATE = [('CStateManager::CanCreateProjectile(', 0)]

CALLS = [
    ('DoUserAnimEvent arms fp4', 'CFlyingPirate::DoUserAnimEvent(', '((CEntity*)$rdi)->mActive != 0 && (int)$ecx == 0', [
        ([('$this->mIsAttackingObject', '1'), ('$this->mAttackObjectId.value', '$fp3_uid($fp3_obj($mgr, -1, 0, "CFlyingPirate"))')], ANIM % 0),
        ([('$this->mIsAttackingObject', '0'), ('$this->mIsAquaPirate', '1')], ANIM % 0),
        ([('$this->mIsAttackingObject', '1'), ('$this->mIsAquaPirate', '1')], ANIM % 0),
        (NOTLOADED, ANIM % 0),
        # cached item set (no shipped fight ever caches the token: mItem is 0 at every natural call):
        # the left operand of `||` true, on mAltProjectileInfo1 and mAltProjectileInfo2
        ([('%s.mItem' % TOK, '(CWeaponDescription*)%s.mObjRef->mObject' % TOK)], ANIM % 0),
        (ATK + [('%s.mItem' % TOK2, '(CWeaponDescription*)%s.mObjRef->mObject' % TOK2)], ANIM % 0),
        # cached item null, the object reference loaded: `||` takes its right operand true
        ([NOTLOADED[0]], ANIM % 0),
        ([], ANIM % 0, NOCREATE),
        # the attack-object arm reads mAltProjectileInfo2: the same token / can-create arms on it
        (ATK + NOTLOADED2, ANIM % 0),
        (ATK + [NOTLOADED2[0]], ANIM % 0),
        (ATK, ANIM % 0, NOCREATE),
        (ATK + [NOTLOADED2[0]], ANIM % 0, NOCREATE),
        ([], ANIM % 19),
        ([], ANIM % 7),
    ], 'DoUserAnimEvent with the attack-object / aqua / token-not-loaded / cannot-create / ragdoll / default arms'),
]
SPEC = []
