# CommandoPirate (module 9) spec: the KnockBack override (fn_9_C300, the vtable slot 0x9C), no forcing.
#
# Run: tools/cov/run.sh tools/cov/specs/commandopirate_c300.py <out>
#      3bfa3eff,05_temple_federationsite_dark --driver 'python3 tools/drivers/commandopirate_c300.py {port}'
#
# The override's two sound arms key on the knock-back manager's active reaction (the guest's word
# at this+0x630 = the manager's +0x10, SReactionParameters::mReaction) being kAR_Hurled (3); the
# dead arm's sound also on the follow-up (+0x14). The loc line is the `if (GetAlive())` right
# after the base's CPatterned::KnockBack, so the keys are the values the guest compares.
CLASS = 'CCommandoPirate'
L = 'loc:PortCCommandoPirate.cpp:'
SPEC = [
    ('0xC300 KnockBack', 'CCommandoPirate::KnockBack(', [], 'calls max=3000'),
    ('0xC3F0 after base: alive, reaction, follow-up', L + '2029',
     ['$this->mAlive', '(int)$this->mKnockBackController.mActiveParameters.mReaction',
      '(int)$this->mKnockBackController.mActiveParameters.mFollowUp'], 'max=3000'),
    ('0xC40C GetUp + hurled sound', L + '2030', [], 'max=3000'),
    ('0xC4B0 death sound', L + '2041', ['(int)$this->mKnockBackController.mActiveParameters.mFollowUp'], 'max=3000'),
]
