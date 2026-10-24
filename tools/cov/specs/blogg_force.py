# Blogg (module 7) forcing spec for tools/cov/force.sh, same area and driver as the blogg drivers
# (merge the gcov dirs in gcov_report.py, this one first):
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/blogg_force.py <out> \
#     3dfd2249,07_swamp_shaft --con tools/cov/specs/blogg_forced.con --fast --frames 2800
# mPlayerId is only written by the script message; every 5th GetPlayer call sees it invalid
# ($rp holds the real id and is written back on the other calls).
CLASS = 'CBlogg'
FORCE = [
    ('464 GetPlayer null', 'CBlogg::GetPlayer(', [],
     ['$gn = $_isvoid($gn) ? 1 : $gn + 1',
      '$rp = ($this->mPlayerId.value != 0xffff) ? $this->mPlayerId.value : ($_isvoid($rp) ? 0xffff : $rp)',
      '$this->mPlayerId.value = ($gn % 5 == 0) ? 0xffff : $rp'], 0,
     'mPlayerId invalid on every 5th call: the no-player arms of the GetPlayer callers'),
    ('718 ShouldEndPursuit time', 'CBlogg::ShouldEndPursuit(', [],
     ['$this->mPlayerPursuitTime = 1.0e9'], 0,
     'pursuit time over the maximum: the || second operand'),
    ('722 ShouldEndBallPursuit time', 'CBlogg::ShouldEndBallPursuit(', [],
     ['$this->mBallPursuitTime = 1.0e9'], 0,
     'ball pursuit time over the maximum'),
    ('746 DetectBall mega', 'CBlogg::DetectBall(', [],
     ['$gd = $_isvoid($gd) ? 1 : $gd + 1', '$this->mIsMegaBlogg = ($gd % 3 == 0) ? 1 : 0'], 0,
     'mIsMegaBlogg set on every 3rd call (0 in every shipped Blogg here): the mega arm'),
]
