# Metroid (module 40), slice met4: the arms of UpdateAttackTarget (fn_40_1080), ShouldReleaseFromTarget
# (fn_40_2EC8) and SuckEnergyFromTarget (fn_40_2864) that no fight reaches: a target that is gone,
# is not a CActor of the kinds the metroid picks, a player that is in a fluid / in screw-attack
# ball state / dead / morphing, a pirate whose energy is drained or that is dead, a target without
# a health info.  Every write lasts one call (force_ret.py restores it at the return).
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov \
#     tools/cov/force.sh tools/cov/specs/metroid_force6.py <out> 42b935e4,08_sand_casualpirates \
#     --con tools/cov/specs/metroid_force6.con --fast --frames 1200
# The CALLS entry hooks CMetroid::Think once `$pir` (a CSpacePirate) and `$sa` (a CScriptActor)
# were seen, then calls the three functions directly with stubbed IsPirateValidTarget /
# IsObjectInHurtfulSafeZone / CSpacePirate::GetHealthInfo results (force_ret.py CallBp).
# The null-health-info arm uses a pirate whose GetHealthInfo is stubbed to null.
CLASS = 'CMetroid'
try:  # the captured objects stay 0 until the first sighting
    import gdb
    gdb.execute('set $pir = 0')
    gdb.execute('set $sa = 0')
except ImportError:  # read outside gdb
    pass

MT = '$this->mAttackTarget.value'
PL = '$P->mUniqueId.value'
PIRU = '((CSpacePirate*)$pir)->mUniqueId.value'
SAU = '((CScriptActor*)$sa)->mUniqueId.value'
SELFU = '$this->mUniqueId.value'
STATE = '$this->mAttackState'
BODY = '((CPatterned*)$pir)->mBodyController.mPtr->mBodyStateInfo'
DIE = '((CBSDie*)%s.mStates.mItems[4])->mIsDead' % BODY

FORCE_RET = [
    ('capture pirate|once', 'CSpacePirate::AcceptScriptMsg(', 1, [(0, [('!$pir', '$rdi')])], 'a pirate'),
    ('capture scriptactor|once', 'CScriptActor::AcceptScriptMsg(', 1, [(0, [('!$sa', '$rdi')])], 'a script actor'),
]

PIRVALID = 'CMetroid::IsPirateValidTarget('
SAFE = 'CSafeZoneManager::IsObjectInHurtfulSafeZone('
UPD = '$this->UpdateAttackTarget(*$mgr)'
REL = '$this->ShouldReleaseFromTarget(*$mgr)'
SUCK = '$this->SuckEnergyFromTarget(0.1f, *$mgr)'
FRZ = '((CBodyController*)$this->mBodyController.mPtr)->mFrozen'
S2 = (STATE, '2')
UNMORPH = [('$P->mSpawnedMorphBallState', '0'), ('$P->mMorphBallState', '0')]
WHEN = '$pir != 0 && $sa != 0'

CALLS = [('met4 direct calls', 'CMetroid::Think(', WHEN, [
    # UpdateAttackTarget (+0x28 cast, +0x58 attached actor, +0x70 pirate cast)
    ([(MT, '0x0fff')], UPD),
    ([(MT, SELFU)], UPD),
    ([(MT, PL), ('$P->mAttachedActor.value', '1')], UPD),
    ([(MT, PL), ('$P->mAttachedActor.value', '1')], UPD, [(PIRVALID, 0)]),
    ([(MT, PL), ('$P->mAttachedActor.value', '0')], UPD),
    ([(MT, PIRU)], UPD, [(PIRVALID, 0)]),
    ([(MT, PIRU)], UPD, [(PIRVALID, 1)]),
    # ShouldReleaseFromTarget
    ([(FRZ, '1')], REL),
    ([(MT, PL), ('$this->mEnergyDrained', '1000000.0')], REL),
    ([(MT, PL), ('$P->mFluidIds.mCount', '1')], REL),
    ([(MT, PL), ('$P->mMorphBall.mPtr->mBallState', '4')], REL),
    ([(MT, PL)], REL, [(SAFE, 1)]),
    ([(MT, PL), ('$P->mPlayerState->mAlive', '0')], REL),
    ([(MT, PL)], REL),
    ([(MT, PIRU), ('((CSpacePirate*)$pir)->mAllEnergyDrained', '1')], REL),
    ([(MT, PIRU), ('%s.mState' % BODY, '4'), (DIE, '1')], REL),
    ([(MT, PIRU)], REL),
    ([(MT, SELFU)], REL),
    ([(MT, '0xffff')], REL),
    # SuckEnergyFromTarget, forced into attack state 2
    ([S2, (MT, '0x0fff')], SUCK),
    ([S2, (MT, PIRU)], SUCK, [('CSpacePirate::GetHealthInfo(', 0)]),
    ([S2, (MT, SELFU)], SUCK),
    ([S2, (MT, SELFU), ('$this->mHealthInfo.mHealth', '0.0')], SUCK),
    ([S2, (MT, PL), ('$P->mSpawnedMorphBallState', '1')], SUCK),
    ([S2, (MT, PL)] + UNMORPH[:1] + [('$P->mMorphBallState', '1')], SUCK),
    ([S2, (MT, PL)] + UNMORPH + [('$P->mStaticTimer', '0.0')], SUCK),
    ([S2, (MT, PL)] + UNMORPH + [('$P->mStaticTimer', '5.0')], SUCK),
    ([S2, (MT, PL), ('$P->mMorphBallState', '2')], SUCK),
    ([S2, (MT, PL), ('$P->mMorphBallState', '3')], SUCK),
    ([S2, (MT, PIRU)], SUCK),
], 'direct calls with the forced scenario members')]
