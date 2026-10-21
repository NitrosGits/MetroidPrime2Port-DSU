# DarkSamus FSM part two: the arms the plain run (darksamus_fsm2.py) cannot reach, forced by gdb
# writes inside the spec's expressions (each once, counted by gdb convenience variables).
# Run: Sanctuary 10_cliff_rooftop with tools/cov/specs/darksamus_fsm2_force.con.
CLASS = 'CDarkSamus'
HIST = ('($this->mAttackHistoryCount > 0 ? '
        '$this->mAttackHistory[$this->mAttackHistoryCount - 1] : 0xffff)')
# Interrupt: hit k (every message runs the switch) gets an empty history (k = 0), then the last
# attack 0x20, 0x100000, 0x200000, 0x800 (k = 1..4)
INT_K = '($_isvoid($ik) ? ($ik = 0) : ($ik = $ik + 1))'
INT_SET = ('($ik == 0 ? ($this->mAttackHistoryCount = 0) : ($ik <= 4 ? '
           '($this->mAttackHistoryCount = 1, $this->mAttackHistory[0] = ($ik == 1 ? 0x20 : '
           '($ik == 2 ? 0x100000 : ($ik == 3 ? 0x200000 : 0x800)))) : 0))')
# Taunt: the first Activate that finds 0xBE8 = 0xA gets 9 instead (the "skip 9" candidate arm)
TAUNT_SET = ('($rdx == 0 && $this->mGuestBE8 == 10 && $_isvoid($tk) ? '
             '($tk = 1, $this->mGuestBE8 = 9) : 0)')
# Taunt activation counter: an Activate row holds the type before the pick, the Deactivate row of
# the same count the type after it (which Range arm ran)
TAUNT_K = '($rdx == 0 ? ($_isvoid($tc) ? ($tc = 0) : ($tc = $tc + 1)) : $tc)'
# NormalMissileAttack: the 30th Update finds the shots fired at the shots to fire (the exit arm)
NM_K = '($rdx == 1 ? ($_isvoid($nk) ? ($nk = 0) : ($nk = $nk + 1)) : -1)'
NM_SET = '($nk == 30 ? ($this->mGuestA38 = $this->mGuestA3C) : 0)'
# SuperMissileAttack: Updates 10..19 with the dodge byte 0xA84 = 1, the 40th with the exit arm
SM_K = '($rdx == 1 ? ($_isvoid($sk) ? ($sk = 0) : ($sk = $sk + 1)) : -1)'
SM_SET = ('($sk == 10 ? ($this->mGuestA84 = 1) : ($sk == 20 ? ($this->mGuestA84 = 0) : '
          '($sk == 40 ? ($this->mGuestA78 = $this->mGuestA7C) : 0)))')
SPEC = [
    ('10FF4 Interrupt', 'CDarkSamus::Interrupt(', [INT_K, INT_SET, '$rdx', HIST], 'calls'),
    ('11544 Taunt', 'CDarkSamus::Taunt(', [TAUNT_K, TAUNT_SET, '$rdx', '$this->mGuestBE8'],
     'calls max=3000'),
    ('17448 NormalMissileAttack', 'CDarkSamus::NormalMissileAttack(',
     [NM_K, NM_SET, '$rdx', '$this->mGuestA38 >= $this->mGuestA3C'], 'calls max=3000'),
    ('16ECC SuperMissileAttack', 'CDarkSamus::SuperMissileAttack(',
     [SM_K, SM_SET, '$rdx', '$this->mGuestA78 >= $this->mGuestA7C', '$this->mGuestA84'],
     'calls max=3000'),
    ('4AC8 PhazonSuck', 'CDarkSamus::PhazonSuck(', ['$rdx'], 'calls'),
    ('8698 RunPhazonTauntCmd', 'CDarkSamus::RunPhazonTauntCmd(', ['$rsi', '$rdx'], 'calls'),
]
