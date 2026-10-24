# Metroid (module 40) met6 lane: direct calls (force_ret CALLS) of CMetroid bodies from the first CMetroid::Think of a regular metroid (08_sand_casualpirates,
# metroid_force8c.con: layers 6+7 only). Every write lasts one call. Stubs make the callee answer a fixed value for the call.
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 tools/cov/force.sh tools/cov/specs/metroid_force8c.py <out> \
#     42b935e4,08_sand_casualpirates --con tools/cov/specs/metroid_force8c.con --fast --frames 700
CLASS = 'CMetroid'
FORCE_RET = []
PLAYER = ('*(unsigned short*)&$this->mAttackTarget', '*(unsigned short*)&$P->mUniqueId')
SELF = ('*(unsigned short*)&$this->mAttackTarget', '*(unsigned short*)&$this->mUniqueId')
NOTARGET = ('*(unsigned short*)&$this->mAttackTarget', '65535')
BOGUS = ('*(unsigned short*)&$this->mAttackTarget', '64515')
TD = '*(CTriggerData*)$this'
ZERO, ONE, TWO, OVER = 0, 1, 2, 3   # CMetroid::EAIState values (kAiState_Zero..)
BS = lambda n: ('*(int*)&$this->mBodyController.mPtr->mBodyStateInfo.mState', str(n))
def ST(n):
    return ('*(int*)&$this->mState', str(n))
def AT(msg, n=None):
    return '$this->Attack(*$mgr, %d, 0.016f)' % msg
def TG(msg):
    return '$this->TelegraphAttack(*$mgr, %d, 0.016f)' % msg
def GN(msg):
    return '$this->Generate(*$mgr, %d, 0.016f)' % msg
BUF = '*(CVector3f*)calloc(1, 16), *(CQuaternion*)calloc(1, 32)'
CALLS = [('CMetroid direct calls', 'CMetroid::Think(', '1', [
    # ApplySeparationBehavior with this metroid's area id changed: every other body is in another area
    ([('$this->mAreaId.value', '$this->mAreaId.value + 1')], '$this->ApplySeparationBehavior(*$mgr)', []),
    # ComputeSuckPiratePosRot with no target actor
    ([NOTARGET], '$this->ComputeSuckPiratePosRot(*$mgr, %s)' % BUF, []),
    # TelegraphAttack Update: state Zero with the timer left / run out, with / without a target and speed
    ([ST(ZERO), ('$this->mTelegraphAttackTime', '100.0'), NOTARGET], TG(1), []),
    ([ST(ZERO), ('$this->mTelegraphAttackTime', '100.0'), BOGUS], TG(1), []),
    ([ST(ZERO), ('$this->mTelegraphAttackTime', '100.0'), SELF], TG(1), []),
    ([ST(ZERO), ('$this->mTelegraphAttackTime', '100.0'), PLAYER], TG(1), []),
    ([ST(ZERO), ('$this->mTelegraphAttackTime', '-1.0'), ('$this->mSpeed', '0.0'), PLAYER], TG(1), []),
    ([ST(ZERO), ('$this->mTelegraphAttackTime', '-1.0'), ('$this->mSpeed', '2.0'), PLAYER], TG(1), []),
    ([ST(ONE)], TG(1), []),
    ([ST(TWO)], TG(1), []),
    ([ST(OVER)], TG(1), []),
    ([], TG(2), [('CMetroid::Attacked(', 1)]),
    ([], TG(2), [('CMetroid::Attacked(', 0), ('CMetroid::PatternShagged(', 1)]),
    ([], TG(2), [('CMetroid::Attacked(', 0), ('CMetroid::PatternShagged(', 0)]),
    # Generate: explosion growth energy reached, the animation states
    ([('$this->mGrowthEnergy', '100000000.0')], GN(0), [('CPatterned::MassiveDeath(', 0)]),
    ([('$this->mGrowthEnergy', '0.0')], GN(0), []),
    ([ST(ONE), BS(12)], GN(1), []),
    ([ST(ONE), BS(0)], GN(1), [('CMetroid::Attacked(', 1)]),
    ([ST(ONE), BS(0)], GN(1), [('CMetroid::Attacked(', 0)]),
    ([ST(TWO), BS(0)], GN(1), []),
    ([ST(TWO), BS(12), ('$this->mGrowthDuration', '1.0')], GN(1), [('CMetroid::Attacked(', 1)]),
    ([ST(TWO), BS(12), ('$this->mGrowthDuration', '1.0')], GN(1), [('CMetroid::Attacked(', 0)]),
    # Attack Deactivate with the random source unavailable
    ([('$mgr->mRandomAvailable', '0')], AT(2), [('CScriptTeamAiMgr::EndAttack(', 0)]),
], 'CMetroid ApplySeparationBehavior, ComputeSuckPiratePosRot, TelegraphAttack, Generate, Attack arms')]
