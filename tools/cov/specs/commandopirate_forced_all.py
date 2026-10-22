# CommandoPirate (module 9) forcing spec: the missing answer of the six one-answer triggers, and
# the KnockBack override's dead arm (fn_9_C300, the vtable slot 0x9C).
#
# Run: the game with MP2_ALLOW_PTRACE=1 (the game opts in to attaches), then
#      COV_GDB_SCRIPT=tools/cov/commandopirate_force_after.py tools/cov/force.sh --attach <pid>
#      tools/cov/specs/commandopirate_forced_all.py <out>
#
# The six triggers answer one way in the natural runs (the console's trigcall says which); each
# FORCE entry writes the member the missing answer needs, at the trigger's entry, so the other
# answer runs too.  The gdb side (tools/cov/commandopirate_force_after.py) also writes the knock-back
# manager's active reaction flags to 3 on the base's CPatterned::KnockBack return, and the alive
# flag is cleared at the override's entry (the console's damage does nothing on a dead commando).
CLASS = 'CCommandoPirate'
FORCE = [

    # fn_9_AFB8 (AbortSeekCover): the answer is "the float at guest this+0xB8C is at least the
    # float at +0xB90"; the natural run answers true, the missing answer is false.
    ('AFB8 AbortSeekCover', 'CCommandoPirate::AbortSeekCover(', [], ['$this->xb8c_ = -1.0'], 0,
     'the float at guest this+0xB8C below the one at +0xB90: the answer becomes false'),
    # fn_9_B0B4 (ShouldCoverAttack): the answer is "the float at guest this+0xB5C is at most 0";
    # the natural run answers true, the missing answer is false.
    ('B0B4 ShouldCoverAttack', 'CCommandoPirate::ShouldCoverAttack(', [], ['$this->xb5c_ = 1.0'], 0,
     'the float at guest this+0xB5C above 0: the answer becomes false'),
    # fn_9_B0D4 (FoundCover): the answer is "the cover point id at guest this+0xBD2 is valid"; the
    # natural run answers false, the missing answer is true.
    ('B0D4 FoundCover', 'CCommandoPirate::FoundCover(', [], ['$this->xbd2_.value = 1'], 0,
     'a valid cover point id at guest this+0xBD2: the answer becomes true'),
    # fn_9_B1A8 (FoundJumpPoint): the answer is "the jump point id at guest this+0xBD0 is valid";
    # the natural run answers false, the missing answer is true.
    ('B1A8 FoundJumpPoint', 'CCommandoPirate::FoundJumpPoint(', [], ['$this->xbd0_.value = 1'], 0,
     'a valid jump point id at guest this+0xBD0: the answer becomes true'),
    # fn_9_B1C8 (IsAggressive): the answer is "the byte at guest this+0xC64 has its 0x40 bit".
    ('B1C8 IsAggressive', 'CCommandoPirate::IsAggressive(', [],
     ['$this->xc64_ = 0x40'], 0,
     'the 0x40 bit of the byte at guest this+0xC64: the answer becomes true'),
    # fn_9_B1D4 (AbortShieldCharge): the answer is "the byte at guest this+0xC64 has its 0x04 bit".
    ('B1D4 AbortShieldCharge', 'CCommandoPirate::AbortShieldCharge(', [],
     ['$this->xc64_ = 0x04'], 0,
     'the 0x04 bit of the byte at guest this+0xC64: the answer becomes true'),
    # fn_9_BAB4 (HeardShot): the answer is "the float at guest this+0xB60 is below 1.5"; the
    # natural run answers false, the missing answer is true.
    ('BAB4 HeardShot', 'CCommandoPirate::HeardShot(', [], ['$this->xb60_ = 0.0'], 0,
     'the float at guest this+0xB60 below 1.5: the answer becomes true'),
    # fn_9_BAD0 (UnderFire): the answer is "the float at guest this+0xB64 is below 0.75"; the
    # natural run answers false, the missing answer is true.
    ('BAD0 UnderFire', 'CCommandoPirate::UnderFire(', [], ['$this->xb64_ = 0.0'], 0,
     'the float at guest this+0xB64 below 0.75: the answer becomes true'),
, 'CCommandoPirate::KnockBack(', [], ['$this->mAlive = 0'], 0,
     'the base\'s alive flag: the override takes its dead arm when the flag is clear, and the '
     'console\'s damage does nothing on a dead commando'),
]
]

# The arms of the same bodies, counted in the same run (no 'ret': a FinishBreakpoint fails in an
# attached gdb - the answers come from the console's trigcall).
SPEC = [
    ('0xAFB8 AbortSeekCover', 'CCommandoPirate::AbortSeekCover(', [], 'calls max=3000'),
    ('0xB0B4 ShouldCoverAttack', 'CCommandoPirate::ShouldCoverAttack(', [], 'calls max=3000'),
    ('0xB0D4 FoundCover', 'CCommandoPirate::FoundCover(', [], 'calls max=3000'),
    ('0xB1A8 FoundJumpPoint', 'CCommandoPirate::FoundJumpPoint(', [], 'calls max=3000'),
    ('0xB1C8 IsAggressive', 'CCommandoPirate::IsAggressive(', [], 'calls max=3000'),
    ('0xB1D4 AbortShieldCharge', 'CCommandoPirate::AbortShieldCharge(', [], 'calls max=3000'),
    ('0xBAB4 HeardShot', 'CCommandoPirate::HeardShot(', [], 'calls max=3000'),
    ('0xBAD0 UnderFire', 'CCommandoPirate::UnderFire(', [], 'calls max=3000'),
    ('0xC300 KnockBack', 'CCommandoPirate::KnockBack(', [], 'calls max=3000'),
    ('0xC300 KnockBack: GetUp', 'loc:PortCCommandoPirate.cpp:1974', [], 'max=3000'),
    ('0xC300 KnockBack: death sound', 'loc:PortCCommandoPirate.cpp:1984', [], 'max=3000'),
]
