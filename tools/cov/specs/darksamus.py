# DarkSamus (module 10) proof spec for tools/cov/run.sh; the labels are the retail offsets.
# Run: Sanctuary 10_cliff_rooftop with tools/cov/specs/darksamus_pause.con or
# tools/cov/specs/darksamus_states.con.
CLASS = 'CDarkSamus'
SPEC = [
    # the BreakPause trigger: the pause is over (guest 0xC30 below the elapsed time 0x8B0) or not
    ('106F8 BreakPause', 'CDarkSamus::BreakPause(', ['$this->mGuestC30 < $this->mElapsedTime'], 'ret'),
    # the Pause state, whose Activate writes 0xC30 (the pause's end time)
    ('10710 Pause', 'CDarkSamus::Pause(', ['$rdx'], 'calls'),
    # the states of slice 26: the key is the message (0 = Activate, 1 = Update, 2 = Deactivate)
    ('9724 BoostBallFire', 'CDarkSamus::BoostBallFire(', ['$rdx'], 'calls'),
    ('BD40 FadeIn', 'CDarkSamus::FadeIn(', ['$rdx'], 'calls'),
    ('BC54 FadeOut', 'CDarkSamus::FadeOut(', ['$rdx'], 'calls'),
    ('F86C FallFromTheAir', 'CDarkSamus::FallFromTheAir(', ['$rdx'], 'calls'),
    ('7A14 EnrageHitReact', 'CDarkSamus::EnrageHitReact(', ['$rdx'], 'calls'),
    ('6E38 EnragedTravel', 'CDarkSamus::EnragedTravel(', ['$rdx'], 'calls'),
    ('16CDC FreezeBeam', 'CDarkSamus::FreezeBeam(', ['$rdx'], 'calls'),
    # the two triggers: the key is the branch that decides, the result is the bool
    ('62B4 Arrived', 'CDarkSamus::Arrived(', ['$this->mGuestCD0 == 0'], 'ret max=3000'),
    ('9314 DoneRolling', 'CDarkSamus::DoneRolling(', ['$this->mBoostBallUid != 0xffff'], 'ret max=3000'),
]
