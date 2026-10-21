# Forcing spec (tools/cov/force.sh --driver, 06_sand_alamofront L2) for ShouldAttack A71C. Cover's
# only transition that names it is LineOfSight->TargetPlayer with the and-links ShouldAttack,
# !ShotAt, Delay 0.5 and RandomDelay 5. The state machine walks a chain from its last link back
# (TStateMachineState::Update), so ShouldAttack runs only after RandomDelay (state time > 5 * the
# state's random) and the Delay pass. The fight's pirate 001d0051 takes cover naturally but leaves
# Cover (OffLine/CoverBlown) well inside those 5 s. The force: on up to three Cover Updates,
# until ShouldAttack is first called, the state time is set to 5.1 (under Cover's Delay 20 ->
# Enraged), so both delays pass on that evaluation.
# Run (2026-10-21):
#   SP_FORCE=Halt tools/cov/force.sh <this> <out> 42b935e4,06_sand_alamofront --driver
#   'python3 tools/drivers/spacepirate_drive.py {port} 2 3' --frames 12000
CLASS = 'CSpacePirate'
SM = '((TStateMachineState<CPatterned>*)$this->mStateMachine.mPtr)'
FORCE = [
    ('A71C ShouldAttack (stops the Cover force)', 'CSpacePirate::ShouldAttack(', [],
     ['$sa = 1'], 0, 'marks ShouldAttack as reached (not a forced arm)'),
    ('A3C0 Cover -> A71C', 'CSpacePirate::Cover(', ['$rdx'],
     ['$sa = $_isvoid($sa) ? 0 : $sa', '$sc = $_isvoid($sc) ? 0 : $sc',
      '$sc_a = ($sa == 0 && $sc < 3 && $rdx == 1) ? 1 : 0',
      SM + '->mTime = $sc_a ? 5.1 : ' + SM + '->mTime',
      '$sc = $sc_a ? $sc + 1 : $sc'],
     0, 'state time = 5.1 on a Cover Update (at most three, until ShouldAttack runs)'),
]
