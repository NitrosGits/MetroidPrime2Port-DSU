# DigitalGuardian (module 14), dg33: XRDG fn_14_12D98's zero-damage arm (fcmpo at 0x12E8C: f31 <= 0 skips
# TakeDamage, the echo emitter and the flinch): data-dead, since every head vulnerability entry
# WeaponHurts accepts has a 0.5..2.0 multiplier. The first two hits (charged, normal) run as they
# are; from the third on the computed damage is 0. The driver also deactivates slot 0's ball
# while the morph ball is held there (fn_14_12818's inactive-ball arm).
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg33_head.py \
#        <out> 1baa96c2,09_cliff_temple_dark --driver 'python3 tools/drivers/dg33_head.py {port}' \
#        --fast --frames 2400
CLASS = 'CDigitalGuardianHead'
SPEC = []
FORCE = [
    ('12D98 zero damage', 'loc:PortCDigitalGuardianHead.cpp:609', [], ['damage = 0'], 2,
     'the damage computed as 0'),
]
# fn_14_12818's null ball (ObjectById of 0xDC4 empty, the beq at 0x12950): the ids are the
# XCRT-built colliders, never deleted in a fight; null from the 2900th test on.
FORCE += [
    ('12818 null ball', 'loc:PortCDigitalGuardianHead.cpp:481', [], ['ball = 0'], 2900,
     'the ball collider gone'),
]
