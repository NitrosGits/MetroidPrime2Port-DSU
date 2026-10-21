# DarkSamus (module 10) proof spec for tools/cov/run.sh; the labels are the retail offsets.
# Run: Sanctuary 10_cliff_rooftop with tools/cov/specs/darksamus_pause.con.
CLASS = 'CDarkSamus'
SPEC = [
    # the BreakPause trigger: the pause is over (guest 0xC30 below the elapsed time 0x8B0) or not
    ('106F8 BreakPause', 'CDarkSamus::BreakPause(', ['$this->mGuestC30 < $this->mElapsedTime'], 'ret'),
    # the Pause state, whose Activate writes 0xC30 (the pause's end time)
    ('10710 Pause', 'CDarkSamus::Pause(', [], 'calls'),
]
