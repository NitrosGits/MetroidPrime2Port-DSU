# Shrieker (module 69) gren9 lane: direct calls with stubbed callees (tools/cov/force_ret.py CALLS) for the null-target,
# team-role and timer arms of the predicate bodies. Greatbridge room, no driver needed beyond `god 1`:
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 tools/cov/force.sh \
#     tools/cov/specs/shrieker_force9.py <out> 3dfd2249,02_swamp_greatbridge --con tools/cov/specs/shrieker_force9.con --fast --frames 700
import os
import gdb

gdb.execute('set $g9_h = 0')


def _sym(prefix):
    for ln in open(os.environ['COV_NM']):
        parts = ln.rstrip('\n').split('\t')
        if len(parts) == 2 and parts[1].startswith(prefix) and '.cold' not in parts[1] and 'clone' not in parts[1]:
            f = parts[0].split()
            if len(f) == 3 and f[1] in 'tTW':
                return f[2]


class _Fin(gdb.FinishBreakpoint):
    def __init__(self, frame, ptr):
        super().__init__(frame, internal=True)
        self.ptr = ptr

    def stop(self):
        gdb.execute('set var *(float*)%d = 2.0' % self.ptr)
        gdb.execute('set $rax = 1')
        return False

    def out_of_scope(self):
        pass


class _HeightHook(gdb.Breakpoint):
    # $g9_h 1: GetHeightAboveMesh(float&) reports success with height 2.0 (|height| > 0.5)
    def stop(self):
        if int(gdb.parse_and_eval('$g9_h')) == 1:
            _Fin(gdb.newest_frame(), int(gdb.parse_and_eval('$rsi')))
        return False


_hs = _sym('CShrieker::GetHeightAboveMesh(')
if _hs:
    _HeightHook('*' + _hs, internal=True)

CLASS = 'CShrieker'
FORCE_RET = []
PLAYER = ('*(unsigned short*)&$this->mTargetId', '*(unsigned short*)&$P->mUniqueId')
SELF = ('*(unsigned short*)&$this->mTargetId', '*(unsigned short*)&$this->mUniqueId')
NOTARGET = ('*(unsigned short*)&$this->mTargetId', '65535')
NOTEAM = ('*(unsigned short*)&$this->mTeamAiMgrId', '65535')
TEAM = ('*(unsigned short*)&$this->mTeamAiMgrId', '*(unsigned short*)&$this->mUniqueId')
T0 = [('$this->mMeleeTimer', '0.0'), ('$this->mFireTimer', '0.0')]
TD = '*(CTriggerData*)$this'
ROLE = 'CScriptTeamAiMgr::GetTeamAiRole('
ROLE_INITIAL = (ROLE, '(long)calloc(1, 64)')              # role value 0 = kTAR_Initial
ROLE_OTHER = (ROLE, '(long)memset((void*)calloc(1, 64), 1, 64)')
ATTACK = 'CScriptTeamAiMgr::StartAttack('
HEIGHT = 'CShrieker::GetHeightAboveMesh('
CALLS = [('Shrieker predicates direct calls', 'CShrieker::Think(', '1', [
    # IsFacingTarget / InMaxRange / FaceTarget without a target
    ([NOTARGET], '$this->IsFacingTarget(*$mgr)', []),
    ([NOTARGET], '$this->InMaxRange(*$mgr, %s)' % TD, []),
    ([NOTARGET], '$this->FaceTarget(*$mgr, 0.016f)', []),
    ([SELF], '$this->FaceTarget(*$mgr, 0.016f)', []),                                        # target is itself: zero direction
    ([PLAYER, ('$g9_h', '1'), ('$this->mChasing', '0')], '$this->FaceTarget(*$mgr, 0.016f)', []),   # height 2.0 above the mesh
    ([PLAYER, ('$this->mChasing', '0')], '$this->FaceTarget(*$mgr, 0.016f)', [(HEIGHT, 0)]),
    ([PLAYER, ('$this->mChasing', '1')], '$this->FaceTarget(*$mgr, 0.016f)', [(HEIGHT, 1)]),
    ([PLAYER, ('$this->mChasing', '0')], '$this->FaceTarget(*$mgr, 0.016f)', [(HEIGHT, 1)]),
    # ShouldMelee
    (T0 + [SELF, NOTEAM, ('$this->mData.mMeleeRangeSquared', '1000000000.0')], '$this->ShouldMelee(*$mgr, %s)' % TD, [ROLE_INITIAL]),
    (T0 + [SELF, NOTEAM, ('$this->mData.mMeleeRangeSquared', '1000000000.0')], '$this->ShouldMelee(*$mgr, %s)' % TD, [(ROLE, 0)]),
    (T0 + [SELF, TEAM, ('$this->mData.mMeleeRangeSquared', '1000000000.0')], '$this->ShouldMelee(*$mgr, %s)' % TD, [ROLE_OTHER, (ATTACK, 1)]),
    (T0 + [SELF, TEAM, ('$this->mData.mMeleeRangeSquared', '1000000000.0')], '$this->ShouldMelee(*$mgr, %s)' % TD, [ROLE_OTHER, (ATTACK, 0)]),
    (T0 + [SELF, NOTEAM, ('$this->mData.mMeleeRangeSquared', '0.0')], '$this->ShouldMelee(*$mgr, %s)' % TD, [(ROLE, 0)]),
    (T0 + [NOTEAM, NOTARGET], '$this->ShouldMelee(*$mgr, %s)' % TD, [(ROLE, 0)]),
    ([('$this->mMeleeTimer', '5.0')], '$this->ShouldMelee(*$mgr, %s)' % TD, [(ROLE, 0)]),
    # ShouldFire
    (T0 + [NOTEAM], '$this->ShouldFire(*$mgr, %s)' % TD, [ROLE_INITIAL, (HEIGHT, 1)]),
    (T0 + [NOTEAM], '$this->ShouldFire(*$mgr, %s)' % TD, [(ROLE, 0), (HEIGHT, 1)]),
    (T0 + [NOTEAM], '$this->ShouldFire(*$mgr, %s)' % TD, [(ROLE, 0), (HEIGHT, 0)]),
    (T0 + [TEAM], '$this->ShouldFire(*$mgr, %s)' % TD, [ROLE_OTHER, (HEIGHT, 1), (ATTACK, 1)]),
    (T0 + [TEAM], '$this->ShouldFire(*$mgr, %s)' % TD, [ROLE_OTHER, (HEIGHT, 1), (ATTACK, 0)]),
    ([('$this->mFireTimer', '5.0')], '$this->ShouldFire(*$mgr, %s)' % TD, [(ROLE, 0)]),
    # ShouldDodge: path blocked arm toggles the side
    ([('$this->mDodgeTimer', '0.0'), ('$this->mData.mDodgePercentage', '1000.0')], '$this->ShouldDodge(*$mgr, %s)' % TD,
     [('CPathFindSearch::PathExists(', 1)]),
    # FindTarget: no target / a target id with no object / separating
    ([NOTARGET], '$this->FindTarget(*$mgr)', []),
    ([('*(unsigned short*)&$this->mTargetId', '64515')], '$this->FindTarget(*$mgr)', []),
    ([SELF, ('$this->mSeparating', '1')], '$this->FindTarget(*$mgr)', []),
    ([SELF, ('$this->mSeparating', '0')], '$this->FindTarget(*$mgr)', []),
    # CloseToStart: far in x, in y (x near), in z (xy near), all near
    ([('$this->mStartPosition.mX', '$this->mTransform.m03 + 1000.0')], '$this->CloseToStart(*$mgr, %s)' % TD, []),
    ([('$this->mStartPosition.mX', '$this->mTransform.m03'), ('$this->mStartPosition.mY', '$this->mTransform.m13 + 1000.0')],
     '$this->CloseToStart(*$mgr, %s)' % TD, []),
    ([('$this->mStartPosition.mX', '$this->mTransform.m03'), ('$this->mStartPosition.mY', '$this->mTransform.m13'),
      ('$this->mStartPosition.mZ', '$this->mTransform.m23 + 1000.0')], '$this->CloseToStart(*$mgr, %s)' % TD, []),
    ([('$this->mStartPosition.mX', '$this->mTransform.m03'), ('$this->mStartPosition.mY', '$this->mTransform.m13'),
      ('$this->mStartPosition.mZ', '$this->mTransform.m23')], '$this->CloseToStart(*$mgr, %s)' % TD, []),
], 'Shrieker predicates: null target, team roles, StartAttack, timers, dodge, start distance')]

CALLS.append(('Shrieker Think / KnockBack direct calls', 'CShrieker::Think(', '1', [
    # buried, shriek not triggered: DetectPlayer only unless both scripted flags are set
    ([('$this->mBuried', '1'), ('$this->mShriekTriggered', '0'), ('$this->mScriptedProximity', '0'), ('$this->mScriptedAlert', '0')],
     '$this->Think(0.016f, *$mgr)', []),
    ([('$this->mBuried', '1'), ('$this->mShriekTriggered', '0'), ('$this->mScriptedProximity', '1'), ('$this->mScriptedAlert', '0')],
     '$this->Think(0.016f, *$mgr)', []),
    ([('$this->mBuried', '1'), ('$this->mShriekTriggered', '0'), ('$this->mScriptedProximity', '1'), ('$this->mScriptedAlert', '1')],
     '$this->Think(0.016f, *$mgr)', []),
    # a second knock back after the death knock back started
    ([('$this->mBuried', '0'), ('$this->mAlive', '0'), ('$this->mDeathKnockBackStarted', '1')],
     '$this->KnockBack(*$mgr, *(CKnockBackInfo*)calloc(1, 256))', [('CPatterned::KnockBack(', 0)]),
], 'Shrieker Think scripted flags, KnockBack after the death knock back'))
