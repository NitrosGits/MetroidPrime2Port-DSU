# Blogg (module 7) call-scoped forcing spec (tools/cov/force_ret.py: each write lasts one call, the
# value is restored when the body returns), same area and driver as blogg_force.py:
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py MP2_CONSOLE_WAIT=120 \
#     tools/cov/force.sh tools/cov/specs/blogg_force2.py <out> 3dfd2249,07_swamp_shaft \
#     --con tools/cov/specs/blogg_forced2.con --fast --frames 3200
# Merge its gcov dir with those of blogg_shaft.con / blogg_guardian.con / blogg_forced.con runs.
# The GetPlayer period is 40: a period of 4 slowed the 8-Blogg area so much the run timed out.
CLASS = 'CBlogg'
FORCE_RET = [
    ('464 GetPlayer null', 'CBlogg::GetPlayer(', 40,
     [(0, [('$this->mPlayerId.value', '0xffff')])],
     'mPlayerId invalid on every 40th call: the no-player arms of the GetPlayer callers'),
    ('718 ShouldEndPursuit', 'CBlogg::ShouldEndPursuit(', 3,
     [(0, [('$this->mMeleePursuitEnded', '1')]),
      (1, [('$this->mPlayerPursuitTime', '1.0e9')])],
     'phase 0: melee pursuit ended (CanMeleeAttack false); phase 1: pursuit time over the maximum'),
    ('722 ShouldEndBallPursuit', 'CBlogg::ShouldEndBallPursuit(', 3,
     [(0, [('$this->mMeleePursuitEnded', '1')]),
      (1, [('$this->mBallPursuitTime', '1.0e9')])],
     'phase 0: melee pursuit ended; phase 1: ball pursuit time over the maximum'),
    ('746 DetectBall', 'CBlogg::DetectBall(', 3,
     [(0, [('$this->mIsMegaBlogg', '1')]),
      (1, [('$this->mCanBite', '0')])],
     'phase 0: mIsMegaBlogg (0 in every shipped Blogg here); phase 1: no bite'),
    ('774 ReleaseHints', 'CBlogg::ReleaseHints(', 2,
     [(0, [('$this->mHintIds.mItems[0].value', '0xffff')])],
     'first hint id invalid: the null-hint arm'),
]
