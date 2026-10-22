# DigitalGuardian (module 14): the coverage spec with the forcing mechanism (tools/cov/force.py,
# copied from the decomp/darksamus-helpers2 branch) - the labels are the retail offsets of the
# helpers this lane ports.  The FORCE entries write the members that take the arms the fight
# cannot reach; the gdb side is tools/cov/cov_force.py (the coverage harness plus the forcing).
#
# Run: COV_GDB_SCRIPT=tools/cov/cov_force.py tools/cov/force.sh \
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
    # fn_14_6C74: the flag byte's bit 6 (the arm at 0x6C90). The vortex mode has to be cleared
    # again, or the first arm would win.
    ('6C74 the flag bit', 'CDigitalGuardian::x6C74_(', [], ['$this->x11B8_ = 0',
                                                        '$this->x128C_ = $this->x128C_ | 0x40'], 200,
     "clears the vortex mode and sets the flag byte's bit 6 (x128C_)"),
    # fn_14_2358: the first toe target has no sound emitter (the else arm at 0x23C4), its hit
    # timer is still positive (the else arm at 0x2450) and it is not flinching (the else arm at
    # 0x247C).
    ('2358 the first toe target', 'CDigitalGuardian::x2358_(', [],
     ['$this->x10F0_[0] = 0', '$this->x1158_[0] = 100.f', '$this->x10DC_[0] = 1'], 100,
     "clears the first toe target's sound handle and sets its timer and state word"),
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
    # fn_14_137A8: the armour id is 0xFFFE, so no model is built (the else arm at 0x137C4)
    ('137A8 no armour', 'CDigitalGuardianHead::x137A8_()', [],
     ['((CDigitalGuardianHead*)$rdi)->mData.headArmor = 0xfffe'], 0,
     "sets the armour's asset id (mData.headArmor) to 0xFFFE"),
    # fn_14_37FC: the five values of the table, one call each (the arms at 0x381C, 0x383C,
    # 0x385C, 0x387C and 0x389C). The calls hang on at fn_14_3E88's entry, which the "ACTV"
    # message reaches once.
    ('37FC the five values', 'CDigitalGuardian::x3E88_(', [],
     ['$tmp = ((CDigitalGuardian*)$rdi)->x37FC_(0.f)',
      '$tmp = ((CDigitalGuardian*)$rdi)->x37FC_(0.7853982f)',
      '$tmp = ((CDigitalGuardian*)$rdi)->x37FC_(1.5707964f)',
      '$tmp = ((CDigitalGuardian*)$rdi)->x37FC_(2.3561945f)',
      '$tmp = ((CDigitalGuardian*)$rdi)->x37FC_(3.1415927f)'], 0,
     'calls fn_14_37FC with the five table values'),
    # fn_14_F374: the head's lives left are 2 (the first arm at 0xF378) and 1 (the else arm).
    ('F374 the lives left', 'CDigitalGuardianHead::x11F7C_(', [],
     ['$tmp = ((CDigitalGuardianHead*)$rdi)->xF374_()',
      '((CDigitalGuardianHead*)$rdi)->x10E4_ = 1',
      '$tmp = ((CDigitalGuardianHead*)$rdi)->xF374_()'], 0,
     "calls fn_14_F374 with two lives left and with one"),
    # fn_14_5644: the sender is a toe target collider (the arms past the first early-out). The
    # other arms need their own run: a member written here stays written.
    ('5644 a toe target collider', 'CDigitalGuardian::x5644_(', [],
     ['$rdx = ((CDigitalGuardian*)$rdi)->x1078_[0]'], 1,
     "points the message's sender at the first toe target collider"),
]
