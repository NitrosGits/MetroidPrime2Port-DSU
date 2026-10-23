# fp2: FlyingPirate (module 22) arms the fights (flyingpirate_*.con) do not take, forced on the
# coverage build. Every FORCE_RET write is undone when the call that saw it returns:
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/flyingpirate_fp2.py <out> 42b935e4,02_sand_sandfalls \
#     --con tools/cov/specs/flyingpirate_fp2.con --fast --frames 4200
# The target swaps (GetTargetPos 663/664) are this spec's own breakpoints, below: the uid of
# another live pirate (seen in Think) goes into mTargetId, on some calls with that pirate's
# mActive cleared for the call.
import os
import gdb

CLASS = 'CFlyingPirate'
SM = "(('TStateMachineState<CPatterned>'*)$this->mStateMachine.mPtr)"
PLAYER = '((CStateManager*)$rsi)->mPlayers[0]'
FORCE_RET = [
    # Dead 1610: a spin-to-death Update whose six seconds are up. Spins in these fights end on a
    # collision before Dead's Update sees them, so the spin flag is forced too (every other
    # Update; the box it sets stays).
    ('Dead spin time 7', 'CFlyingPirate::Dead(', 2,
     [(0, [('$this->mSpinToDeath', '$rdx == 1 ? 1 : $this->mSpinToDeath'),
           (SM + '->mTime', '$rdx == 1 ? 7.0 : ' + SM + '->mTime')])],
     'Update with mSpinToDeath 1, GetTime() 7'),
    # Lurk 1031: mDestPos on the pirate itself, so the turn's delta is not magnitude-safe
    # (the body writes mDestPos itself further down: not restored).
    ('Lurk dest = self', 'CFlyingPirate::Lurk(', 2,
     [(1, [('!$this->mDestPos.mX', '$rdx == 1 ? $this->mPosition.mX : $this->mDestPos.mX'),
           ('!$this->mDestPos.mY', '$rdx == 1 ? $this->mPosition.mY : $this->mDestPos.mY'),
           ('!$this->mDestPos.mZ', '$rdx == 1 ? $this->mPosition.mZ : $this->mDestPos.mZ'),
           ('!$this->mAnimationState.mState', '$rdx == 1 ? 1 : $this->mAnimationState.mState')])],
     'Lurk Update with mDestPos = own translation, kAS_Ready'),
    # ShouldDodge 1365: not attacked lately, but shot at.
    ('ShouldDodge shot at', 'CFlyingPirate::ShouldDodge(', 3,
     [(1, [('$this->mTimeSinceAttacked', '1.0'), ('$this->mTimeSinceShotAt', '0.1')])],
     'mTimeSinceAttacked 1, mTimeSinceShotAt 0.1'),
    # HearPlayer 913: out of hearing range.
    ('HearPlayer deaf', 'CFlyingPirate::HearPlayer(', 2,
     [(1, [('$this->mData.mHearingDistance', '0.0')])], 'mHearingDistance 0'),
    # Taunt 947: a listening pirate in another area.
    ('Taunt other area', 'CFlyingPirate::Taunt(', 2,
     [(1, [('$this->mAreaId.value', '$rdx == 2 ? $this->mAreaId.value + 1 : $this->mAreaId.value')])],
     'own area id + 1 on Deactivate'),
    # Stuck 1570: the search path's result is not kR_Success.
    ('Stuck no path', 'CFlyingPirate::Stuck(', 3,
     [(2, [('$this->mPathFindSearch.mResult', '4')])], 'mPathFindSearch.mResult kR_NoPath'),
    # Attack 1164: aggression already checked on Activate.
    ('Attack checked', 'CFlyingPirate::Attack(', 2,
     [(1, [('$this->mAggressionChecked', '$rdx == 0 ? 1 : $this->mAggressionChecked')])],
     'mAggressionChecked 1 on Activate'),
    # Attack 1183: the pirate stands on the player (delta to the player not magnitude-safe).
    ('Attack on player', 'CFlyingPirate::Attack(', 5,
     [(2, [('$this->mPosition.m%s' % c, '$rdx == 1 ? %s->mPosition.m%s : $this->mPosition.m%s' % (PLAYER, c, c))
           for c in 'XYZ'])],
     'own translation = the player on Update'),
    # CheckFireMissiles 680: the path already blocked at check 1.
    ('CheckFireMissiles blocked', 'CFlyingPirate::CheckFireMissiles(', 1,
     [(0, [('!$this->mMissilePathBlocked',
            '$this->mFireMissilesCheck == 1 && ($this->mUniqueId.value & 1) ? 1 : $this->mMissilePathBlocked')])],
     'mMissilePathBlocked 1 at check 1 (odd uids; kept: the body moves it on)'),
    # Listen 406/407: a detection height range, small and large.
    ('Listen height', 'CFlyingPirate::Listen(', 3,
     [(1, [('$this->mDetectionHeightRange', '1.0')]), (2, [('$this->mDetectionHeightRange', '1000000.0')])],
     'mDetectionHeightRange 1 / 1e6'),
    # AddToTeam 498, RemoveFromTeam 507: the team id names the player (not a team manager).
    ('AddToTeam not a team', 'CFlyingPirate::AddToTeam(', 3,
     [(1, [('$this->mTeamAiMgr.value', PLAYER + '->mUniqueId.value')])], 'mTeamAiMgr = player'),
    ('RemoveFromTeam not a team', 'CFlyingPirate::RemoveFromTeam(', 3,
     [(1, [('$this->mTeamAiMgr.value', PLAYER + '->mUniqueId.value')])], 'mTeamAiMgr = player'),
    # UpdateParticleEffects 427-445, DoUserAnimEvent 1757: an aqua pirate (no shipped one here).
    ('UpdateParticleEffects aqua', 'CFlyingPirate::UpdateParticleEffects(', 5,
     [(3, [('$this->mIsAquaPirate', '1')])], 'mIsAquaPirate 1'),
    ('DoUserAnimEvent aqua', 'CFlyingPirate::DoUserAnimEvent(', 4,
     [(1, [('$this->mIsAquaPirate', '1')]), (3, [('$this->mIsAttackingObject', '1')])],
     'mIsAquaPirate 1 / mIsAttackingObject 1'),
    # InRange 1208/1209: the player inside / outside the ranges.
    ('InRange ranges', 'CFlyingPirate::InRange(', 3,
     [(0, [('$this->mMinAttackRange', '0.0')]),
      (1, [('$this->mMinAttackRange', '1000000.0'), ('$this->mMaxAttackRange', '1000000.0')]),
      (2, [('$this->mMinAttackRange', '1000000.0'), ('$this->mMaxAttackRange', '0.0')])],
     'ranges 0/-, 1e6/1e6 and 1e6/0'),
    # FireProjectile 1703: no object for the target id.
    ('FireProjectile no target', 'CFlyingPirate::FireProjectile(', 4,
     [(1, [('$this->mTargetId.value', '65535')])], 'mTargetId invalid'),
    # FireProjectile 1714: the target 5 units off the gun, 52 / 75 degrees off its forward (the
    # pirate is moved for the call, the player stays put).
    ('FireProjectile close', 'CFlyingPirate::FireProjectile(', 4,
     [(2, [('$this->mTransform.m%d3' % i, '%s->mPosition.m%s - 3.08 * $this->mTransform.m%d1 - 3.94 * $this->mTransform.m%d0'
            % (PLAYER, 'XYZ'[i], i, i)) for i in range(3)]),
      (3, [('$this->mTransform.m%d3' % i, '%s->mPosition.m%s - 1.29 * $this->mTransform.m%d1 - 4.83 * $this->mTransform.m%d0'
            % (PLAYER, 'XYZ'[i], i, i)) for i in range(3)])],
     'gun target ~5 units at 52 / 75 degrees'),
    # KnockBack 705/708/733/742: no vertical movement (a flyer has it set), a missed spiral
    # roll, burn / fade deaths.
    ('KnockBack not vertical', 'CFlyingPirate::KnockBack(', 1,
     [(0, [('$this->mVerticalMovement', '$this->mAlive ? 0 : $this->mVerticalMovement')])],
     'mVerticalMovement 0 (alive)'),
    ('KnockBack spiral roll', 'CFlyingPirate::KnockBack(', 4,
     [(1, [('$this->mData.mSpiralChance', '$this->mAlive ? $this->mData.mSpiralChance : -1.0')]),
      (3, [('$this->mData.mSpiralChance', '$this->mAlive ? $this->mData.mSpiralChance : 101.0')])],
     'mSpiralChance -1 / 101 (dead)'),
    ('KnockBack fade', 'CFlyingPirate::KnockBack(', 3,
     [(2, [('$this->mFadeToDeath', '$this->mAlive ? $this->mFadeToDeath : 1')])], 'mFadeToDeath 1 (dead)'),
    ('KnockBack lagged burn', 'CFlyingPirate::KnockBack(', 2,
     [(0, [('$this->mLaggedBurnDeath', '$this->mAlive ? $this->mLaggedBurnDeath : 1')])],
     'mLaggedBurnDeath 1 (dead)'),
]
FORCE_INNER = [
    ('Stuck base stuck', 'CFlyingPirate::Stuck(', 'CPatterned::Stuck(', 3, {1: [('$rax', '1')]},
     'CPatterned::Stuck answers true'),
    ('Dodge no direction', 'CFlyingPirate::Dodge(', 'CFlyingPirate::GetDodgeDirection(', 1,
     {0: [('$rax', '-1')]}, 'GetDodgeDirection answers kSD_Invalid'),
    ('DoUserAnimEvent no room', 'CFlyingPirate::DoUserAnimEvent(', 'CStateManager::CanCreateProjectile(', 3,
     {2: [('$rax', '0')]}, 'CanCreateProjectile answers false'),
    ('RemoveFromTeam not member', 'CFlyingPirate::RemoveFromTeam(', 'CScriptTeamAiMgr::IsPartOfTeam(', 2,
     {1: [('$rax', '0')]}, 'IsPartOfTeam answers false'),
    ('KnockBack hurled', 'CFlyingPirate::KnockBack(', 'CPatterned::KnockBack(', 2,
     {1: [('$this->mKnockBackController.mActiveParameters.mReaction', '3')]}, 'reaction kAR_Hurled'),
]
SPEC = [
    ('AcceptScriptMsg', 'CFlyingPirate::AcceptScriptMsg(', ['$rsi', '$rdx & 0xffff'], 'max=2000'),
]

OUT = os.environ['COV_OUT']
LOGF = os.path.join(OUT, 'fp2.log')
counts = {}
seen = {}  # uid -> (object address, Think hit number)
think_n = [0]
syms = []
for ln in open(os.environ['COV_NM']):
    parts = ln.rstrip('\n').split('\t')
    if len(parts) == 2:
        f = parts[0].split()
        if len(f) == 3 and f[1] in 'tTW':
            syms.append((f[2], parts[1]))


def addrs(prefix):
    return [m for m, d in syms if d.startswith(prefix) and '.cold' not in d and 'clone' not in d]


def log(key, msg):
    counts[key] = counts.get(key, 0) + 1
    if counts[key] <= 4:
        with open(LOGF, 'a') as fh:
            fh.write('%s #%d: %s\n' % (key, counts[key], msg))
    with open(os.path.join(OUT, 'fp2_counts.txt'), 'w') as fh:
        for k, v in sorted(counts.items()):
            fh.write('%-40s %d\n' % (k, v))


class ThinkBp(gdb.Breakpoint):
    def stop(self):
        think_n[0] += 1
        this = int(gdb.parse_and_eval('$rdi'))
        uid = int(gdb.parse_and_eval('((CFlyingPirate*)%d)->mUniqueId.value' % this))
        seen[uid] = (this, think_n[0])
        return False


class Restore(gdb.FinishBreakpoint):
    def __init__(self, frame, saved):
        super().__init__(frame, internal=True)
        self.saved = saved

    def stop(self):
        for lv, old in reversed(self.saved):
            gdb.execute('set var %s = %s' % (lv, old))
        return False

    def out_of_scope(self):
        pass


class TargetBp(gdb.Breakpoint):
    n = 0

    def stop(self):
        TargetBp.n += 1
        phase = TargetBp.n % 40
        if phase not in (1, 2):
            return False
        this = int(gdb.parse_and_eval('$rdi'))
        me = int(gdb.parse_and_eval('((CFlyingPirate*)%d)->mUniqueId.value' % this))
        others = [(u, p) for u, (p, t) in seen.items() if u != me and think_n[0] - t < 8]
        if not others:
            return False
        uid, ptr = others[0]
        saved = []
        lv = '((CFlyingPirate*)%d)->mTargetId.value' % this
        saved.append((lv, str(int(gdb.parse_and_eval(lv)))))
        gdb.execute('set var %s = %d' % (lv, uid))
        what = 'target = %04x (active)' % uid
        if phase == 2:
            act = '((CEntity*)(CFlyingPirate*)%d)->mActive' % ptr
            saved.append((act, str(int(gdb.parse_and_eval(act)))))
            gdb.execute('set var %s = 0' % act)
            what = 'target = %04x (inactive for the call)' % uid
        Restore(gdb.newest_frame(), saved)
        log('GetTargetPos other phase %d' % phase, what)
        return False


for a in addrs('CFlyingPirate::Think('):
    ThinkBp('*' + a, internal=True)
for a in addrs('CFlyingPirate::GetTargetPos('):
    TargetBp('*' + a, internal=True)

