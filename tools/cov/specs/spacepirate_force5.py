# Forcing spec (tools/cov/force.sh --driver, 02_sand_sandfalls L3) for CoverBlown AA64. Each force is
# one call by a pirate that holds a cover point (0x900 valid) and is undone at the next CoverBlown
# call (gdb convenience variables only):
#   1. 0x3dc (mMinAttackRange) = 1e6 -> the target is inside the minimum attack range, the trigger
#      stays true without asking the cover point (asm 0xAAA0 `ble`).
#   2. 0x904 (steering speed) = 0 -> a cover that is not blown and a pirate that is not stepping and
#      far from its cover counts as blown (asm 0xAAD8-0xAB40).
#   3. 0x904 = 0 and the model's Y scale = 1e6 -> the same pirate is "near" its cover (the squared
#      distance is under 3 * scaleY), so the cover stays not blown (asm 0xAB40 `ble`).
#   4. 0x904 = 0 and the body state id = 3 (kAS_Step) -> a stepping pirate keeps its cover (asm
#      0xAB04); the state id is put back at the trigger's own TraceCall, before anything else runs.
# The fights never stop a pirate that holds a cover point (x904 is 1 in PathFind/TargetCover).
# Run (2026-10-21), with the sandfalls wall-hang and cover setup:
#   SP_FORCE=Halt SP_STEPS='wait 4; place {cls:CPlayer} -83.1 -72 -9.5; place {000a010e} -78 -96 -10;
#   wait 1; fsm {000a010e} Lurk; wait 25; place {cls:CPlayer} -40.8 -85 15; fsm {000a027a} Lurk; wait 30'
#   tools/cov/force.sh <this> <out> 42b935e4,02_sand_sandfalls --driver
#   'python3 tools/drivers/spacepirate_drive.py {port} 3 1' --frames 10000
CLASS = 'CSpacePirate'
BS = '$this->mBodyController.mPtr->mBodyStateInfo.mState'
SCALE_Y = '$this->mModelData.mPtr->mScale.mY'
FORCE = [
    ('AA64 CoverBlown', 'CSpacePirate::CoverBlown(',
     ['$this->x900_coverPoint.value != 0xffff', '$this->x904_steeringSpeed'],
     ['$cb = $_isvoid($cb) ? 0 : $cb', '$cb_p = $_isvoid($cb_p) ? 0 : $cb_p',
      '$cb_r = $_isvoid($cb_r) ? 0 : $cb_r', '$cb_s = $_isvoid($cb_s) ? 0 : $cb_s',
      '$cb_y = $_isvoid($cb_y) ? 0 : $cb_y',
      # undo the previous call's write
      '$cb == 1 ? (((CSpacePirate*)$cb_p)->mMinAttackRange = $cb_r) : 0',
      '($cb == 2 || $cb == 3 || $cb == 5) ? (((CSpacePirate*)$cb_p)->x904_steeringSpeed = $cb_s) : 0',
      '$cb == 3 ? (((CSpacePirate*)$cb_p)->mModelData.mPtr->mScale.mY = $cb_y) : 0',
      '$cb = $cb == 1 ? 11 : ($cb == 2 ? 12 : ($cb == 3 ? 13 : ($cb == 5 ? 15 : $cb)))',
      '$cb_go = ($this->x900_coverPoint.value != 0xffff && ($cb == 0 || $cb == 11 || $cb == 12 || $cb == 13)) ? 1 : 0',
      '$cb_p = $cb_go ? $this : $cb_p',
      '$cb_r = ($cb_go && $cb == 0) ? $this->mMinAttackRange : $cb_r',
      '$this->mMinAttackRange = ($cb_go && $cb == 0) ? 1000000 : $this->mMinAttackRange',
      '$cb_s = ($cb_go && $cb != 0) ? $this->x904_steeringSpeed : $cb_s',
      '$this->x904_steeringSpeed = ($cb_go && $cb != 0) ? 0 : $this->x904_steeringSpeed',
      '$cb_y = ($cb_go && $cb == 12) ? ' + SCALE_Y + ' : $cb_y',
      SCALE_Y + ' = ($cb_go && $cb == 12) ? 1000000 : ' + SCALE_Y,
      '$cb_b = ($cb_go && $cb == 13) ? ' + BS + ' : ($_isvoid($cb_b) ? 0 : $cb_b)',
      BS + ' = ($cb_go && $cb == 13) ? 3 : ' + BS,
      '$cb = $cb_go ? ($cb == 0 ? 1 : ($cb == 11 ? 2 : ($cb == 12 ? 3 : 4))) : $cb'],
     0, 'min attack range, then steering 0, then that plus scale Y 1e6, then that plus kAS_Step; one call each'),
    ('AA64 CoverBlown (puts the body state back)', '(anonymous namespace)::TraceCall(', [],
     ['$cb = $_isvoid($cb) ? 0 : $cb',
      '$cb == 4 ? (((CSpacePirate*)$cb_p)->mBodyController.mPtr->mBodyStateInfo.mState = $cb_b) : 0',
      '$cb = $cb == 4 ? 5 : $cb'],
     0, 'restores the kAS_Step write (not a forced arm)'),
]
