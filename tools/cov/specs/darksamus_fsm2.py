# DarkSamus (module 10) FSM part two (PortCDarkSamusFsm2.cpp) spec for tools/cov/run.sh; the labels
# are the retail offsets. Run: Sanctuary 10_cliff_rooftop with tools/cov/specs/darksamus_fsm2.con.
CLASS = 'CDarkSamus'
HIST = ('($this->mAttackHistoryCount > 0 ? '
        '$this->mAttackHistory[$this->mAttackHistoryCount - 1] : 0xffff)')
SPEC = [
    # the message and the step direction 0xC1C (Update/Deactivate show what Activate picked)
    ('10C9C HeavyHit', 'CDarkSamus::HeavyHit(', ['$rdx', '$this->mGuestC1C'], 'calls'),
    # the message and the last finished attack, which picks the command
    ('10FF4 Interrupt', 'CDarkSamus::Interrupt(', ['$rdx', HIST], 'calls'),
    ('1142C Patrol', 'CDarkSamus::Patrol(', ['$rdx'], 'calls max=2000'),
    ('11544 Taunt', 'CDarkSamus::Taunt(', ['$rdx', '$this->mGuestBE8'], 'calls max=2000'),
    ('16ECC SuperMissileAttack', 'CDarkSamus::SuperMissileAttack(',
     ['$rdx', '$this->mGuestA78 >= $this->mGuestA7C', '$this->mGuestA84'], 'calls max=2000'),
    ('17448 NormalMissileAttack', 'CDarkSamus::NormalMissileAttack(',
     ['$rdx', '$this->mGuestA38 >= $this->mGuestA3C'], 'calls max=2000'),
    ('178E4 MissileDeflect', 'CDarkSamus::MissileDeflect(', ['$rdx', '$this->mGuestC04 != 0'],
     'calls max=2000'),
    ('18EAC GetUp', 'CDarkSamus::GetUp(', ['$rdx'], 'calls max=2000'),
    ('4AC8 PhazonSuck', 'CDarkSamus::PhazonSuck(', ['$rdx'], 'calls max=2000'),
    ('6460 GoToPhazonEnrage', 'CDarkSamus::GoToPhazonEnrage(', ['$rdx'], 'calls max=2000'),
    ('652C GoToPhazonAttack', 'CDarkSamus::GoToPhazonAttack(', ['$rdx'], 'calls max=2000'),
    ('67B8 PhazonMissiles', 'CDarkSamus::PhazonMissiles(', ['$rdx', '$this->mGuestFB8'],
     'calls max=2000'),
    ('6BA0 PhazonFall', 'CDarkSamus::PhazonFall(', ['$rdx'], 'calls max=2000'),
    ('7054 PhazonRise', 'CDarkSamus::PhazonRise(', ['$rdx', '$this->mGuest1044'], 'calls max=2000'),
    ('7BC0 PhazonEnraged', 'CDarkSamus::PhazonEnraged(', ['$rdx'], 'calls max=2000'),
    ('81D4 PhazonAttack', 'CDarkSamus::PhazonAttack(', ['$rdx', '$this->mGuestF5C40'],
     'calls max=2000'),
    ('16B5C BreakNormalMissile', 'CDarkSamus::BreakNormalMissile(', [], 'ret max=3000'),
    ('16B54 BreakSuperMissile', 'CDarkSamus::BreakSuperMissile(', [], 'ret max=3000'),
]
