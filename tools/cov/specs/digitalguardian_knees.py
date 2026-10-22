# DigitalGuardian (module 14): fn_14_6E04 (the body turns to the head) and fn_14_10120 (the head's
# projectile attack block) with every knee count. The knee ids are cleared one by one at
# fn_14_6E04's entry (the fight takes hours to break four knees), the head's projectile attack
# (head+0x10D0) is set from the knee count it last saw (head+0x10DC: 2 -> 0, at most 1 -> 1) at
# ProjectileTelegraph's entry, and with no knee seen the head's bit 0x10 of 0x10F4 (its armour gone)
# is set at fn_14_10120's entry. Driver: digitalguardian_knees.con.
# Run: COV_GDB_SCRIPT=tools/cov/cov_force.py BUILD=build-port-cov tools/cov/force.sh \
#        tools/cov/specs/digitalguardian_knees.py <out> 1baa96c2,09_cliff_temple_dark \
#        --con tools/cov/specs/digitalguardian_knees.con --fast --frames 2600
CLASS = 'CDigitalGuardian'
H = '((CDigitalGuardianHead*)$rdi)'
SPEC = [
    ('6E04 the body turns to the head', 'CDigitalGuardian::x6E04_(',
     ['$this->xFB0_', '$this->xFB2_', '$this->xFB4_', '$this->xFB6_'], ''),
    ('10120 the projectile-attack block', 'CDigitalGuardianHead::x10120_(',
     [H + '->x10F4_ & 0x10'], ''),
]
FORCE = [
    ('6E04 three knees', 'CDigitalGuardian::x6E04_(', [], ['$this->xFB0_ = 0xffff'], 500,
     'clears the first knee id (xFB0_)'),
    ('6E04 two knees', 'CDigitalGuardian::x6E04_(', [], ['$this->xFB2_ = 0xffff'], 900,
     'clears the second knee id (xFB2_)'),
    ('6E04 one knee', 'CDigitalGuardian::x6E04_(', [], ['$this->xFB4_ = 0xffff'], 1300,
     'clears the third knee id (xFB4_)'),
    ('6E04 no knee', 'CDigitalGuardian::x6E04_(', [], ['$this->xFB6_ = 0xffff'], 1700,
     'clears the fourth knee id (xFB6_)'),
    ('16F74 the projectile attack', 'CDigitalGuardianHead::ProjectileTelegraph(',
     [H + '->x10DC_', H + '->x10D0_'],
     [H + '->x10D0_ = ' + H + '->x10DC_ == 2 ? 0 : (' + H + '->x10DC_ >= 0 && ' + H +
      '->x10DC_ <= 1 ? 1 : ' + H + '->x10D0_)'], 0,
     'head+0x10D0 = 0 with two knees seen, 1 with one or none'),
    ('10120 the head armour gone', 'CDigitalGuardianHead::x10120_(', [H + '->x10DC_'],
     [H + '->x10F4_ = ' + H + '->x10F4_ | (' + H + '->x10DC_ == 0 ? 0x10 : 0)'], 0,
     'head+0x10F4 bit 0x10 once the head has seen no knee'),
]
