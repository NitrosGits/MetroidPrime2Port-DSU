# DigitalGuardian (module 14) body states and codes: which states the fight enters, with the
# state message ($rdx). Run: tools/cov/run.sh tools/cov/specs/digitalguardian_states.py <out>
#   1baa96c2,09_cliff_temple_dark --con tools/cov/specs/digitalguardian_fight.con --fast
CLASS = 'CDigitalGuardian'
SPEC = [
    ('95B8 Idle', 'CDigitalGuardian::Idle(', ['$rdx'], 'max=4000'),
    ('94BC FaceTarget', 'CDigitalGuardian::FaceTarget(', ['$rdx'], 'max=4000'),
    ('8794 ProjectileAttack', 'CDigitalGuardian::ProjectileAttack(', ['$rdx'], 'max=4000'),
    ('8CCC MeleeAttackTelegraph', 'CDigitalGuardian::MeleeAttackTelegraph(', ['$rdx'], 'max=4000'),
    ('89D4 MeleeAttack', 'CDigitalGuardian::MeleeAttack(', ['$rdx'], 'max=4000'),
    ('8580 JumpAttack', 'CDigitalGuardian::JumpAttack(', ['$rdx'], 'max=4000'),
    ('9260 VortexAttack', 'CDigitalGuardian::VortexAttack(', ['$rdx'], 'max=4000'),
    ('7D8C CrippledReaction', 'CDigitalGuardian::CrippledReaction(', ['$rdx'], 'max=4000'),
    ('77EC SetProtectKneesFaceTarget', 'CDigitalGuardian::SetProtectKneesFaceTarget(', [], ''),
    ('7BD4 SetupBossEnergyBar', 'CDigitalGuardian::SetupBossEnergyBar(', [], ''),
    ('7504 SelectBestMeleeAttackLeg', 'CDigitalGuardian::SelectBestMeleeAttackLeg(', [], ''),
    ('73D4 SelectMeleeAttackRange', 'CDigitalGuardian::SelectMeleeAttackRange(', [], ''),
    ('A468 DoUserAnimEvent', 'CDigitalGuardian::DoUserAnimEvent(', [], 'max=4000'),
]
