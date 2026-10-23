# Tryclops (REL 81) forcing spec for tools/cov/force.sh, Sanctuary Fortress 0f_cliff_hall.
# A held bomb whose object is gone: at the entry of the bomb bodies an unset mBombId is pointed at
# a Tryclops (u047b, not a CBomb), so their "bomb no longer resolves" arms run; Think's dizzy timer
# is cut to 0.01 once it is armed (PathFindEx sets 15), so its expiry arm runs while the ball is held;
# GetUp's Update sees mShotTarget set. Drivers: tools/drivers/tryclops_force.con, tryclops_force2.con.
# Run: BUILD=build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/cov_force.py tools/cov/force.sh \
#      tools/cov/specs/tryclops_force.py <out> 1baa96c2,42c85ad9 --con tools/drivers/tryclops_force.con \
#      --fast --frames 2600   (with GODMODE=1 in the environment)
CLASS = 'CTryclops'
_BOGUS = '$this->mBombId.value = $this->mBombId.value == 0xffff ? 0x047b : $this->mBombId.value'
FORCE = [
    ('0x4078 TargetCover: bomb gone (forced)', 'CTryclops::TargetCover(', ['$rsi'], [_BOGUS], 0,
     'mBombId names a non-bomb'),
    ('0x3778 HearShot: bomb gone (forced)', 'CTryclops::HearShot(', [], [_BOGUS], 0,
     'mBombId names a non-bomb'),
    ('0x2F34 JumpBack: bomb gone (forced)', 'CTryclops::JumpBack(', ['$rsi'], [_BOGUS], 0,
     'mBombId names a non-bomb'),
    ('0x3DA4 InRange: bomb gone (forced)', 'CTryclops::InRange(', [], [_BOGUS], 0,
     'mBombId names a non-bomb'),
    ('0x3814 InPosition: bomb gone (forced)', 'CTryclops::InPosition(', [], [_BOGUS], 0,
     'mBombId names a non-bomb'),
    ('0x21F4 SpotPlayer: bomb gone (forced)', 'CTryclops::SpotPlayer(', [], [_BOGUS], 0,
     'mBombId names a non-bomb'),
    ('0x1B58 Death: bomb gone (forced)', 'CTryclops::Death(', [], [_BOGUS], 0,
     'mBombId names a non-bomb'),
    ('0x4114 Think: dizzy timer expires (forced)', 'CTryclops::Think(', [],
     ['$this->mDizzyTimer = $this->mDizzyTimer > 1 ? 0.01 : $this->mDizzyTimer'], 0,
     'the 15 s hold timer is cut to 0.01'),
    ('0x270C GetUp: target already shot at Update (forced)', 'CTryclops::GetUp(', ['$rdx'],
     ['$this->mShotTarget = $rdx == 1 ? 1 : $this->mShotTarget'], 0,
     'mShotTarget set on an Update'),
]
SPEC = [
    ('0x1B58 Death', 'CTryclops::Death(', [], 'calls'),
]
