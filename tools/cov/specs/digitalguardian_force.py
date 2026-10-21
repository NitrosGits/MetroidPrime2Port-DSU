# DigitalGuardian (module 14): the coverage spec with the forcing mechanism (tools/cov/force.py,
# copied from decomp/darksamus-helpers2) - the labels are the retail offsets of the helpers this
# lane ports.  The FORCE entries write the members that take the arms the fight cannot reach;
# the gdb side is tools/cov/force_cov.py (the coverage harness plus the forcing).
#
# Run: COV_GDB_SCRIPT=tools/cov/force_cov.py tools/cov/force.sh \
#        tools/cov/specs/digitalguardian_force.py <out> 1baa96c2,09_cliff_temple_dark \
#        --con tools/cov/specs/digitalguardian_area53.con --fast --frames 3000
CLASS = 'CDigitalGuardian'
KNEES = ('($this->xFB0_ != 0xffff ? 1 : 0) + ($this->xFB2_ != 0xffff ? 1 : 0)'
         ' + ($this->xFB4_ != 0xffff ? 1 : 0) + ($this->xFB6_ != 0xffff ? 1 : 0)')
MORPH = ('(((CStateManager*)$rsi)->mPlayers[0]->mSpawnedMorphBallState == CPlayer::kMS_Unmorphed'
         ' ? ((CStateManager*)$rsi)->mPlayers[0]->mMorphBallState : CPlayer::kMS_Unmorphed)')
SPEC = [
    # the helpers the Think calls (the four calls of fn_14_A5BC) and the ones they call
    ('6E04 the body turns to the head', 'CDigitalGuardian::x6E04_(', ['$this->x128C_ & 0x02'],
     'calls'),
    ('6CB0 the aim', 'CDigitalGuardian::x6CB0_(', [], 'calls'),
    ('6C24 the head\'s turn direction', 'CDigitalGuardian::x6C24_(', [], 'calls'),
    ('2358 the toe targets\' sound emitters', 'CDigitalGuardian::x2358_(', [], 'calls'),
    ('38BC the leg-stab, jump and protect timers', 'CDigitalGuardian::x38BC_(', [KNEES, MORPH],
     ''),
    ('3F90 the knee armours follow the knees', 'CDigitalGuardian::x3F90_(', ['$this->xFB2_',
                                                                           '$this->xFB4_'],
     'calls'),
    ('6C74 the attack angle', 'CDigitalGuardian::x6C74_(', ['$this->x11B8_ != 0',
                                                            '$this->x128C_ & 0x40'], ''),
    # the rest of this lane's helpers
    ('5644 the XRDG case', 'CDigitalGuardian::x5644_(', [], 'calls'),
    ('37FC nearest 45 degrees', 'CDigitalGuardian::x37FC_(', [], ''),
    ('11EA8 the bomb slots\' sounds', 'CDigitalGuardianHead::x11EA8_()', [], 'calls'),
    ('11F7C the echo targets are deleted', 'CDigitalGuardianHead::x11F7C_(', [], 'calls'),
    ('10120 the projectile-attack block', 'CDigitalGuardianHead::x10120_(', [], ''),
    ('F374 the retreat speed', 'CDigitalGuardianHead::xF374_()', [], ''),
    ('137A8 the head\'s armour model', 'CDigitalGuardianHead::x137A8_()', [], 'calls'),
]
FORCE = [
    # fn_14_3F90: the second knee id is cleared, so the walk skips that knee (the arm at 0x3FD4)
    ('3F90 an invalid knee id', 'CDigitalGuardian::x3F90_(', [], ['$this->xFB2_ = 0xffff'], 100,
     'clears the second knee id (xFB2_)'),
    # fn_14_3F90: the third knee id names an actor that is not there, so the walk clears it
    # (the arm at 0x4054)
    ('3F90 a knee that is gone', 'CDigitalGuardian::x3F90_(', [], ['$this->xFB4_ = 0x1234'], 200,
     'points the third knee id (xFB4_) at an actor that does not exist'),
    # fn_14_38BC: fewer than four knees, so the protect timer is not the "four knees" arm
    # (the arm at 0x3904)
    ('38BC fewer than four knees', 'CDigitalGuardian::x38BC_(', [], ['$this->xFB2_ = 0xffff'], 100,
     'clears the second knee id (xFB2_)'),
    # fn_14_38BC: the player is morphed, so the protect timer is reset (the arm at 0x393C)
    ('38BC the player is morphed', 'CDigitalGuardian::x38BC_(', [],
     ['((CStateManager*)$rsi)->mPlayers[0]->mMorphBallState = 1'], 300,
     "sets the player's morph state (mMorphBallState) to 1 (morphed)"),
    # fn_14_6C74: the vortex mode (the arm at 0x6C78)
    ('6C74 the vortex mode', 'CDigitalGuardian::x6C74_(', [], ['$this->x11B8_ = 1'], 100,
     'sets the vortex mode (x11B8_) to 1'),
    # fn_14_6C74: the flag byte's bit 6 (the arm at 0x6C90)
    ('6C74 the flag bit', 'CDigitalGuardian::x6C74_(', [], ['$this->x11B8_ = 0',
                                                        '$this->x128C_ = $this->x128C_ | 0x40'], 200,
     "clears the vortex mode and sets the flag byte's bit 6 (x128C_)"),
    # fn_14_2358: a toe target in the flinch state (the arm at 0x247C)
    ('2358 the flinch', 'CDigitalGuardian::x2358_(', [], ['$this->x10DC_[0] = 2'], 100,
     "sets the first toe target's state word (x10DC_) to 2 (the flinch)"),
    # fn_14_6E04: no head (the arm at 0x6E40)
    ('6E04 no head', 'CDigitalGuardian::x6E04_(', [], ['$this->x1082_ = 0xffff'], 100,
     "clears the head's id (x1082_)"),
    # fn_14_6E04: the head is gone, so it detaches (the arm at 0x6E64)
    ('6E04 the head is gone', 'CDigitalGuardian::x6E04_(', [], ['$this->x1082_ = 0x1234'], 200,
     "points the head's id (x1082_) at an actor that does not exist"),
    # fn_14_6CB0: the flag byte's bit 1 is clear, so the aim transform is rebuilt (the arm at
    # 0x6CE0) when the head has no usable direction
    ('6CB0 the flag bit is clear', 'CDigitalGuardian::x6CB0_(', [],
     ['$this->x128C_ = $this->x128C_ & 0xfd'], 100,
     "clears the flag byte's bit 1 (x128C_)"),
    # fn_14_6C24: the guardian's body state is 0, so the head's turn direction does not matter
    # (the arm at 0x6C40)
    ('6C24 the body state is 0', 'CDigitalGuardian::x6C24_(', [],
     ['((CDigitalGuardian*)$rdi)->mBodyController.mPtr->mBodyStateInfo.mState = 0'], 100,
     "sets the body controller's state id (mState) to 0"),
    # fn_14_11EA8: a bomb slot with a sound (the arm at 0x11EDC)
    ('11EA8 a bomb slot with a sound', 'CDigitalGuardianHead::x11EA8_()', [],
     ['((CDigitalGuardianHead*)$rdi)->xF28_[0] = 1'], 0,
     'writes a sound handle into the first bomb slot (xF28_)'),
]
