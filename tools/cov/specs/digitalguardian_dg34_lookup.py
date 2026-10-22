# DigitalGuardian (module 14), dg34: fn_14_3300's calls and the current anim each TakeOff/Landing lookup
# asks about (CAnimData::GetTimeOfUserEventForAnimation(this, anim, type): $rsi anim, $rdx type).
# Run with tools/cov/specs/digitalguardian_dg34_3300.con (see there).
CLASS = 'CDigitalGuardian'
SPEC = [
    ('3300', 'CDigitalGuardian::x3300_(', [], 'calls'),
    ('3300 lookup', 'CAnimData::GetTimeOfUserEventForAnimation(', ['$rsi', '$rdx'], 'caller=x3300_'),
]
