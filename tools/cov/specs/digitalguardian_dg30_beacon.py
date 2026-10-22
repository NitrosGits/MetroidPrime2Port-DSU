# DigitalGuardian (module 14), dg30 run B: the beacon and render arms the fight cannot reach.
# - fn_14_D664 with the beacon effect id unset (data-dead: the placement sets it), so 0x123C is
#   null: fn_14_17DC (0x1878), fn_14_2A28 (0x2A60), fn_14_28D0's emitter test.
# - fn_14_28D0's second arrival with no sound handle (0x1240 cleared at entry; the driver's echo
#   visor is on, so the 0x7F volume arm runs) and its third with no collision actor (0x10CC unset).
# - fn_14_A3D4 (Render) with mDrawParticles clear after 40 renders (0xA40C).
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg30_beacon.py \
#        <out> 1baa96c2,09_cliff_temple_dark --driver 'python3 tools/drivers/dg30_beacon.py {port} b' \
#        --fast --frames 900
CLASS = 'CDigitalGuardian'
SPEC = []
FORCE = [
    ('D664 no effect', 'CDigitalGuardian::xD664_(', [], ['$rsi = 0xffffffff'], 0,
     'the beacon effect id unset'),
    ('28D0 no sound handle', 'CDigitalGuardian::x28D0_(', [], ['$this->x1240_.mID = 0'], 1,
     'arrival 2+: 0x1240 cleared'),
    ('28D0 no actor', 'CDigitalGuardian::x28D0_(', [], ['$this->x10CC_.value = 0xffff'], 2,
     'arrival 3+: 0x10CC unset'),
    ('A3D4 no particles', 'CDigitalGuardian::Render(', [], ['$this->mDrawParticles = 0'], 40,
     'mDrawParticles cleared'),
]
