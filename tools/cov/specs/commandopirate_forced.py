# CommandoPirate (module 9) forcing spec: the missing answer of the one-answer triggers, and
# the KnockBack override's dead arm.
#
# Run: COV_GDB_SCRIPT=tools/cov/cov_force.py BUILD=build-port-cov tools/cov/force.sh
#      tools/cov/specs/commandopirate_forced.py <out> 3bfa3eff,645ef243
#      --driver "python3 tools/drivers/commandopirate_dead.py {port} <out>.drv"
#
# The twelve triggers below answer one way in the natural runs; each FORCE entry writes the
# member its missing answer needs, at the trigger's entry, so the other answer runs too (the
# counters of this run and of the natural one add up).
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
]

# The arms of the same bodies, counted in the same run.
SPEC = [
    ('0xAFB8 AbortSeekCover', 'CCommandoPirate::AbortSeekCover(', [], 'ret max=3000'),
    ('0xB0B4 ShouldCoverAttack', 'CCommandoPirate::ShouldCoverAttack(', [], 'ret max=3000'),
    ('0xB0D4 FoundCover', 'CCommandoPirate::FoundCover(', [], 'ret max=3000'),
    ('0xB1A8 FoundJumpPoint', 'CCommandoPirate::FoundJumpPoint(', [], 'ret max=3000'),
    ('0xB1C8 IsAggressive', 'CCommandoPirate::IsAggressive(', [], 'ret max=3000'),
    ('0xB1D4 AbortShieldCharge', 'CCommandoPirate::AbortShieldCharge(', [], 'ret max=3000'),
    ('0xBAB4 HeardShot', 'CCommandoPirate::HeardShot(', [], 'ret max=3000'),
    ('0xBAD0 UnderFire', 'CCommandoPirate::UnderFire(', [], 'ret max=3000'),
    ('0xC300 KnockBack', 'CCommandoPirate::KnockBack(', [], 'calls max=3000'),
    ('0xC300 KnockBack: GetUp', 'loc:PortCCommandoPirate.cpp:1987', [], 'max=3000'),
    ('0xC300 KnockBack: death sound', 'loc:PortCCommandoPirate.cpp:1997', [], 'max=3000'),
]
