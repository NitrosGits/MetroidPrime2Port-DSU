# Metroid (module 40) met6 lane: direct calls (force_ret CALLS) of CMetroid bodies from the first CMetroid::Think of a regular metroid (08_sand_casualpirates,
# metroid_force8c.con: layers 6+7 only). Every write lasts one call. Stubs make the callee answer a fixed value for the call.
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 tools/cov/force.sh tools/cov/specs/metroid_force8c.py <out> \
#     42b935e4,08_sand_casualpirates --con tools/cov/specs/metroid_force8c.con --fast --frames 700
import os
import gdb

gdb.execute('set $m6_t = 0')
gdb.execute('set $m6_p = 0')


def _sym(prefix):
    for ln in open(os.environ['COV_NM']):
        parts = ln.rstrip('\n').split('\t')
        if len(parts) == 2 and parts[1].startswith(prefix) and '.cold' not in parts[1] and 'clone' not in parts[1]:
            f = parts[0].split()
            if len(f) == 3 and f[1] in 'tTW':
                return f[2]


class _Fin(gdb.FinishBreakpoint):
    def __init__(self, frame, val):
        super().__init__(frame, internal=True)
        self.val = val

    def stop(self):
        gdb.execute('set $xmm0.v4_float[0] = %s' % self.val)
        return False

    def out_of_scope(self):
        pass


class _TimeHook(gdb.Breakpoint):
    # $m6_t 1: GetAnimTimeRemaining answers 0.0, 2: answers 5.0
    def stop(self):
        m = int(gdb.parse_and_eval('$m6_t'))
        if m:
            _Fin(gdb.newest_frame(), '0.0' if m == 1 else '5.0')
        return False


_ts = _sym('CBodyController::GetAnimTimeRemaining(')
if _ts:
    _TimeHook('*' + _ts, internal=True)


class _PosFin(gdb.FinishBreakpoint):
    def __init__(self, frame, me):
        super().__init__(frame, internal=True)
        self.me = me

    def stop(self):
        # a 12-byte CVector3f comes back in xmm0 (x, y) and xmm1 (z)
        gdb.execute('set $xmm0.v4_float[0] = ((CMetroid*)%d)->mTransform.m03' % self.me)
        gdb.execute('set $xmm0.v4_float[1] = ((CMetroid*)%d)->mTransform.m13' % self.me)
        gdb.execute('set $xmm1.v4_float[0] = ((CMetroid*)%d)->mTransform.m23' % self.me)
        return False

    def out_of_scope(self):
        pass


class _PosHook(gdb.Breakpoint):
    # $m6_p 1: GetAttackTargetPos answers this metroid's own translation (attackDelta == 0)
    def stop(self):
        if int(gdb.parse_and_eval('$m6_p')):
            _PosFin(gdb.newest_frame(), int(gdb.parse_and_eval('$rdi')))
        return False


_ps = _sym('CMetroid::GetAttackTargetPos(')
if _ps:
    _PosHook('*' + _ps, internal=True)

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
    # Generate state One: animation time remaining 0 (the Over arm); state Two while frozen
    ([ST(ONE), BS(12), ('$m6_t', '1')], GN(1), []),
    ([ST(ONE), BS(12), ('$m6_t', '2')], GN(1), []),
    ([ST(TWO), BS(12), ('$this->mBodyController.mPtr->mFrozen', '1')], GN(1), []),
    # Attack Update state Two: the player held by this metroid but in another area / in this area / held by nobody
    ([ST(TWO), PLAYER, ('*(unsigned short*)&$P->mAttachedActor', '*(unsigned short*)&$this->mUniqueId'), ('$P->mAreaId.value', '$P->mAreaId.value + 1')], AT(1), [('CMetroid::DetachFromTarget(', 0)]),
    ([ST(TWO), PLAYER, ('*(unsigned short*)&$P->mAttachedActor', '*(unsigned short*)&$this->mUniqueId'), BS(9)], AT(1), [('CMetroid::DetachFromTarget(', 0)]),
    ([ST(TWO), PLAYER, ('*(unsigned short*)&$P->mAttachedActor', '65535'), BS(9)], AT(1), [('CMetroid::DetachFromTarget(', 0)]),
    # InAttackPosition
    ([PLAYER, ('$this->mAreaId.value', '$this->mAreaId.value + 1')], '$this->InAttackPosition(*$mgr, %s)' % TD, []),
    ([PLAYER], '$this->InAttackPosition(*$mgr, %s)' % TD, [('CMetroid::IsPlayerInFluid(', 1)]),
    ([PLAYER, ('$P->mMorphBallState', '0'), ('$this->xa40_28_', '0')], '$this->InAttackPosition(*$mgr, %s)' % TD, [('CMetroid::IsPlayerInFluid(', 0)]),
    ([PLAYER, ('$P->mMorphBallState', '0'), ('$this->xa40_28_', '1')], '$this->InAttackPosition(*$mgr, %s)' % TD, [('CMetroid::IsPlayerInFluid(', 0)]),
    ([PLAYER, ('$P->mMorphBallState', '1')], '$this->InAttackPosition(*$mgr, %s)' % TD, [('CMetroid::IsPlayerInFluid(', 0)]),
    ([PLAYER, ('$P->mMorphBallState', '1'), ('$P->mTransform.m03', '$this->mTransform.m03 + $this->mTransform.m01'), ('$P->mTransform.m13', '$this->mTransform.m13 + $this->mTransform.m11'),
      ('$P->mTransform.m23', '$this->mTransform.m23 - 0.1'), ('$this->x7c0_.mX', '$this->mTransform.m03'), ('$this->x7c0_.mY', '$this->mTransform.m13'),
      ('$this->x7c0_.mZ', '$this->mTransform.m23'), ('$this->mMaxAttackRange', '100.0')], '$this->InAttackPosition(*$mgr, %s)' % TD, [('CMetroid::IsPlayerInFluid(', 0)]),
    ([PLAYER, ('$P->mMorphBallState', '1'), ('$P->mTransform.m03', '$this->mTransform.m03 + $this->mTransform.m01'), ('$P->mTransform.m13', '$this->mTransform.m13 + $this->mTransform.m11'),
      ('$P->mTransform.m23', '$this->mTransform.m23 + 5.0'), ('$this->x7c0_.mX', '$this->mTransform.m03'), ('$this->x7c0_.mY', '$this->mTransform.m13'),
      ('$this->x7c0_.mZ', '$this->mTransform.m23'), ('$this->mMaxAttackRange', '100.0')], '$this->InAttackPosition(*$mgr, %s)' % TD, [('CMetroid::IsPlayerInFluid(', 0)]),
    ([PLAYER, ('$P->mMorphBallState', '1'), ('$this->mMaxAttackRange', '0.0')], '$this->InAttackPosition(*$mgr, %s)' % TD, [('CMetroid::IsPlayerInFluid(', 0)]),
    ([PLAYER, ('$m6_p', '1'), ('$P->mMorphBallState', '1'), ('$P->mTransform.m03', '$this->mTransform.m03 + $this->mTransform.m01'), ('$P->mTransform.m13', '$this->mTransform.m13 + $this->mTransform.m11'),
      ('$P->mTransform.m23', '$this->mTransform.m23 - 0.1'), ('$this->x7c0_.mX', '$this->mTransform.m03'), ('$this->x7c0_.mY', '$this->mTransform.m13'),
      ('$this->x7c0_.mZ', '$this->mTransform.m23'), ('$this->mMaxAttackRange', '100.0')], '$this->InAttackPosition(*$mgr, %s)' % TD, [('CMetroid::IsPlayerInFluid(', 0)]),
    # ComputeSuckPlayerPosRot while the player morphs / unmorphs (transition factor 0.9, 0.1, no duration)
    ([('$P->mMorphBallState', '2')], '$this->ComputeSuckPlayerPosRot(*$P, *$mgr, *(CVector3f*)calloc(1, 16), *(CQuaternion*)calloc(1, 32))', []),
    ([('$P->mMorphBallState', '3'), ('$P->mMorphDuration', '1.0'), ('$P->mMorphTime', '0.9')], '$this->ComputeSuckPlayerPosRot(*$P, *$mgr, *(CVector3f*)calloc(1, 16), *(CQuaternion*)calloc(1, 32))', []),
    ([('$P->mMorphBallState', '3'), ('$P->mMorphDuration', '1.0'), ('$P->mMorphTime', '0.1')], '$this->ComputeSuckPlayerPosRot(*$P, *$mgr, *(CVector3f*)calloc(1, 16), *(CQuaternion*)calloc(1, 32))', []),
    ([('$P->mMorphBallState', '3'), ('$P->mMorphDuration', '0.0')], '$this->ComputeSuckPlayerPosRot(*$P, *$mgr, *(CVector3f*)calloc(1, 16), *(CQuaternion*)calloc(1, 32))', []),
], 'CMetroid ApplySeparationBehavior, ComputeSuckPiratePosRot, TelegraphAttack, Generate, Attack arms')]
