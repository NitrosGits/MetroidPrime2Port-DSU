# DarkSamus (module 10) proof spec for tools/cov/run.sh; the labels are the retail offsets.
# Run: Sanctuary 10_cliff_rooftop with tools/cov/specs/darksamus_states.con.
CLASS = 'CDarkSamus'
# The DoneRolling trigger's arms are forced from the expressions (gdb convenience variables):
# the arms the fight cannot reach are written at the trigger's entry, one per hit.
DR_K = '($_isvoid($dk) ? ($dk = 0) : ($dk = $dk + 1))'
DR_SET = ('($dk == 1 ? ($this->mBoostBallUid.value = 0xffff) : '
          '($dk == 2 ? ($this->mBoostBallUid.value = 1, $this->mGuestEA4 = 3.0) : '
          '($dk == 3 ? ($this->mBoostBallUid.value = 1, $this->mGuestEA4 = 0.0, '
          '            $this->mBoostHintUid.value = 2, $this->mGuestEC8 = 1) : '
          '($dk == 4 ? ($this->mBoostBallUid.value = 1, $this->mGuestEA4 = 0.0, '
          '            $this->mBoostHintUid.value = 2, $this->mGuestEC8 = 0) : '
          '($dk == 5 ? ($this->mBoostBallUid.value = 1, $this->mGuestEA4 = 0.0, '
          '            $this->mBoostHintUid.value = 0xffff, $this->mBoostBallFlag80 = 1) : '
          '($dk == 6 ? ($this->mBoostBallUid.value = 1, $this->mGuestEA4 = 0.0, '
          '            $this->mBoostHintUid.value = 0xffff, $this->mBoostBallFlag80 = 0, '
          '            $this->mGuestEA8 = -1000.0) : '
          '($dk == 7 ? ($this->mBoostBallUid.value = 1, $this->mGuestEA4 = 0.0, '
          '            $this->mBoostHintUid.value = 0xffff, $this->mBoostBallFlag80 = 0, '
          '            $this->mGuestEA8 = 1000.0, $this->mGuestEAC = CVector3f::Zero(), '
          '            $this->mGuestEB8 = 0.0) : '
          '($this->mBoostBallUid.value = 1, $this->mGuestEA4 = 0.0, '
          ' $this->mBoostHintUid.value = 0xffff, $this->mBoostBallFlag80 = 0, '
          ' $this->mGuestEA8 = 1000.0, $this->mGuestEAC = $this->mPosition, '
          ' $this->mGuestEB8 = 0.0))))))))'))
          ' $this->mGuestEB8 = 0.0))))))))')
# the Pause state: the second Activate runs without a fight stage (the battle stage's uid is
# invalidated), which is the arm the retail's own null check at 0x00010770 takes.
FB_K = '($_isvoid($fk) ? ($fk = 0) : ($fk = $fk + 1))'
FB_SET = ('($rdx == 1 ? ($fk == 1 ? ($this->mGuestAB0 = 1) : '
          '($fk == 2 ? ($this->mGuestAB0 = 0, $this->mGuestAB8 = $this->mGuestAB8 | 0x40) : 0)) : 0)')
PAUSE_K = '($rdx == 0 ? ($_isvoid($pk) ? ($pk = 0) : ($pk = $pk + 1)) : 0)'
PAUSE_SET = ('($pk == 1 ? ($this->mBattleStageUid.value = 0xffff) : 0)')
SPEC = [
    # the BreakPause trigger: the pause is over (guest 0xC30 below the elapsed time 0x8B0) or not
    ('106F8 BreakPause', 'CDarkSamus::BreakPause(', ['$this->mGuestC30 < $this->mElapsedTime'], 'ret'),
    # the states of slice 26: the key is the message (0 = Activate, 1 = Update, 2 = Deactivate)
    ('10710 Pause', 'CDarkSamus::Pause(', [PAUSE_K, PAUSE_SET, '$rdx'], 'calls'),
    ('9724 BoostBallFire', 'CDarkSamus::BoostBallFire(', ['$rdx'], 'calls'),
    ('BD40 FadeIn', 'CDarkSamus::FadeIn(', ['$rdx'], 'calls'),
    ('BC54 FadeOut', 'CDarkSamus::FadeOut(', ['$rdx'], 'calls'),
    ('F86C FallFromTheAir', 'CDarkSamus::FallFromTheAir(', ['$rdx'], 'calls'),
    ('7A14 EnrageHitReact', 'CDarkSamus::EnrageHitReact(', ['$rdx'], 'calls'),
    ('6E38 EnragedTravel', 'CDarkSamus::EnragedTravel(', ['$rdx'], 'calls'),
    # the FreezeBeam: the counter 0xAB0 (the exit state command) and the bit 0x40 of 0xAB8 (the
    # turn and the dodge) are written at the Update's entry, one per Update (the Activate clears
    # both, 0x00016D5C and 0x00016D6C).
    ('16CDC FreezeBeam', 'CDarkSamus::FreezeBeam(', [FB_K, FB_SET, '$rdx'], 'calls'),
    # the helpers of slice 27
    ('99B8 StopBoostBallParticles', 'CDarkSamus::StopBoostBallParticles(', [], 'calls'),
    ('6DAC UpdateShieldFromHealth', 'CDarkSamus::UpdateShieldFromHealth(', ['$rdx'], 'calls'),
    ('8870 RunFreezeBeamCmd', 'CDarkSamus::RunFreezeBeamCmd(', ['$rdx'], 'calls'),
    # the DoneRolling trigger: the arms are forced from the expressions above
    ('9314 DoneRolling', 'CDarkSamus::DoneRolling(', [DR_K, DR_SET], 'ret max=3000'),
    # the Arrived trigger (the fight never evaluates it; the arms are forced from a call)
    ('62B4 Arrived', 'CDarkSamus::Arrived(', ['$this->mGuestCD0 == 0'], 'ret max=3000'),
]
