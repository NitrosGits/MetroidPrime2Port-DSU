# DigitalGuardian (module 14), dg32: the head's render without particles (fn_14_17B28's
# mDrawParticles clear, 0x17B58: set naturally, cleared from the 20th render on) and
# GetAimPosition's dt > 0 arm (fn_14_17890, fcmpo/ble at 0x178E4) from its 30th call on, if no
# caller takes it naturally. And the head's flag 0x10 (armour destroyed) in fn_14_174F4 (extrwi.
# 1, 27 at 0x1758C): the scan display only draws the head for a few frames once the scan has
# downloaded, so the flag is set from the 5th call on (it stays set; the run ends soon after).
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg32_render.py \
#        <out> 1baa96c2,09_cliff_temple_dark --driver 'python3 tools/drivers/dg32_head.py {port}' \
#        --fast --frames 900
CLASS = 'CDigitalGuardianHead'
SPEC = []
FORCE = [
    ('17B28 no particles', 'CDigitalGuardianHead::Render(', [],
     ['$this->mDrawParticles = 0'], 20, 'mDrawParticles clear'),
    ('17890 dt > 0', 'CDigitalGuardianHead::GetAimPosition(', [], ['dt = 0.5'], 30,
     'dt = 0.5'),
    ('174F4 armour destroyed', 'CDigitalGuardianHead::ScanVisorRender(', [],
     ['$this->x10F4_ |= 0x10'], 4, 'flag 0x10 set'),
]
