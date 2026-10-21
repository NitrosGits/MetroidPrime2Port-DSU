# DigitalGuardian (module 14) helpers spec for tools/cov/run.sh; the labels are the retail
# offsets of the helpers this lane ports (src/MetroidPrime/Enemies/PortCDigitalGuardianHelpers.cpp).
# Run: tools/cov/run.sh tools/cov/specs/digitalguardian.py <out> 1baa96c2,09_cliff_temple_dark
#       --con tools/cov/specs/digitalguardian_area53.con
CLASS = 'CDigitalGuardian'
SPEC = [
    # sanity entries: bodies the port already had and that run in the area (the triggers
    # evaluate x705C_ every frame, the Think builds the collision managers)
    ('SANITY 705C the knee count', 'CDigitalGuardian::x705C_(', [], ''),
    ('SANITY 676C the collision managers', 'CDigitalGuardian::x676C_(', [], ''),
    ('SANITY 3E88 the knee armours flag', 'CDigitalGuardian::x3E88_(', [], ''),
    # the body's helpers
    ('37FC nearest 45 degrees', 'CDigitalGuardian::x37FC_(', [], ''),
    ('6C74 the attack angle', 'CDigitalGuardian::x6C74_(', [], ''),
    ('3F90 the knee armours follow the knees', 'CDigitalGuardian::x3F90_(', [], ''),
    ('38BC the leg-stab, jump and protect timers', 'CDigitalGuardian::x38BC_(', [], ''),
    ('2358 the toe targets\' sound emitters', 'CDigitalGuardian::x2358_(', [], 'calls'),
    ('5644 the XRDG case', 'CDigitalGuardian::x5644_(', [], 'calls'),
    ('6C24 the head\'s turn direction', 'CDigitalGuardian::x6C24_(', [], ''),
    ('6CB0 the aim', 'CDigitalGuardian::x6CB0_(', [], 'calls'),
    ('6E04 the body turns to the head', 'CDigitalGuardian::x6E04_(', [], 'calls'),
    # the head's helpers
    ('10120 the projectile-attack block', 'CDigitalGuardianHead::x10120_(', [], ''),
    ('F374 the retreat speed', 'CDigitalGuardianHead::xF374_(', [], ''),
    ('11EA8 the bomb slots\' sounds', 'CDigitalGuardianHead::x11EA8_(', [], 'calls'),
    ('11F7C the echo targets are deleted', 'CDigitalGuardianHead::x11F7C_(', [], 'calls'),
    ('137A8 the head\'s armour model', 'CDigitalGuardianHead::x137A8_(', [], 'calls'),
]
