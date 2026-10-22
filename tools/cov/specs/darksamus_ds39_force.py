# ds39: the arms of the bodied rows that the rooftop fight (tools/drivers/darksamus_ds39.con) does
# not take, forced on the coverage build with tools/cov/force_ret.py as the gdb side:
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_INPUT=LAYER2@400 \
#     tools/cov/force.sh tools/cov/specs/darksamus_ds39_force.py <out> "1baa96c2,10_cliff_rooftop" \
#     --con tools/drivers/darksamus_ds39.con --fast --frames 5700
# FORCE_RET (written at the entry, put back at the return):
#   6DAC UpdateShieldFromHealth  every 2nd call: the health 0.5 (the <= 1.0f arm, 0x00006DFC)
#   6374 FindPathToWaypoint      a successful search on the first waypoint (PathShagged false),
#                                of 1 waypoint (the last one: 0x0000640C) or of 3 (0x00006408 taken)
#   1E740 ShouldForgetLastAttacker  the same two searches (0x0001E7AC / the cover lookup)
#   11C60 UpdateDiveFlight       the dive's target = its start (CanBeNormalized false, 0x00011CD8)
#   21F14 DoProjectileAttackCmd  on Update: the animation state Ready and the asked state 0, not the
#                                body's, so CanIssueCommand answers true (0x00021FB8)
CLASS = 'CDarkSamus'
S = '$this->mPathFindSearch'
SEARCH1 = [(S + '.mResult', '0'), (S + '.mCurWaypoint', '0'), (S + '.mWaypoints.mCount', '1')]
SEARCH3 = [(S + '.mResult', '0'), (S + '.mCurWaypoint', '0'), (S + '.mWaypoints.mCount', '3')]
SPEC = [
    ('1E740 ShouldForgetLastAttacker', 'CDarkSamus::ShouldForgetLastAttacker(',
     ['$this->mLastAttackerUid.value != 0xffff', S + '.mWaypoints.mCount'], 'ret max=2000'),
    ('6374 FindPathToWaypoint', 'CDarkSamus::FindPathToWaypoint(', ['$rdx', S + '.mWaypoints.mCount'],
     'calls max=2000'),
]
FORCE_RET = [
    ('6DAC health 0.5 (forced)', 'CDarkSamus::UpdateShieldFromHealth(', 2,
     [(0, [('$this->mHealthInfo.mHealth', '0.5')])], 'the health at most 1.0f (0x00006DFC)'),
    ('6374 search (forced)', 'CDarkSamus::FindPathToWaypoint(', 3,
     [(1, SEARCH1), (2, SEARCH3)], 'PathShagged false; the last waypoint / not'),
    ('1E740 search (forced)', 'CDarkSamus::ShouldForgetLastAttacker(', 3,
     [(1, SEARCH1), (2, SEARCH3)], 'PathShagged false; the last waypoint / not'),
    ('11C60 target = start (forced)', 'CDarkSamus::UpdateDiveFlight(', 2,
     [(0, [('$this->mDive.mTarget.mX', '$this->mDive.mStart.mX'),
           ('$this->mDive.mTarget.mY', '$this->mDive.mStart.mY'),
           ('$this->mDive.mTarget.mZ', '$this->mDive.mStart.mZ')])],
     'a zero direction (CanBeNormalized false)'),
    ('21F14 Ready (forced)', 'CDarkSamus::DoProjectileAttackCmd(', 1,
     [(0, [('$this->mAnimationState.mState', '$rsi == 1 ? 1 : $this->mAnimationState.mState'),
           ('$rdx', '$rsi == 1 ? 0 : $rdx')])],
     'CanIssueCommand true on Update (0x00021FB8)'),
]
