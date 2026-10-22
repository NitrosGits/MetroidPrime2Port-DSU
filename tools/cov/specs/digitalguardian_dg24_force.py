# DigitalGuardian (module 14), dg24: Stage3Start's (fn_14_149F8) fallback to the head's own Y
# column, taken when the IS06/FOLW waypoint's Y column is not magnitude-safe (0x14AB0). The only
# head placement's waypoint has an orthonormal transform, so the column is zeroed at the test.
# EnableDynamicCollision's (fn_14_14828) dead-ball arm (HP <= 0.0, 0x14938): from the fifth ball
# visit on, the ball's health is zeroed at the test. The no-collider arms of 14828 and 14664 (0xD70
# null, 0x148B4/0x146F4): from the second call on, 0xD70 is nulled at the entry and put back at
# the filter line that follows the block (the head's Think updates it unconditionally).
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg24_force.py \
#        <out> 1baa96c2,09_cliff_temple_dark --con tools/cov/specs/digitalguardian_dg24.con \
#        --fast --frames 400
CLASS = 'CDigitalGuardianHead'
SPEC = []
FORCE = [
    ('149F8 the waypoint column is unsafe', 'loc:PortCDigitalGuardianHead.cpp:2359', [],
     ['dir.mX = 0', 'dir.mY = 0', 'dir.mZ = 0'], 0, "zeroes Stage3Start's waypoint column (dir)"),
    ('14828 a ball at no health', 'loc:PortCDigitalGuardianHead.cpp:2385', [],
     ['ball->mHealthInfo.mHealth = 0'], 4, "zeroes the ball's health (mHealthInfo.mHealth)"),
    ('14828 no collider manager', 'CDigitalGuardianHead::EnableDynamicCollision(', [],
     ['$dg24_head = $rdi', '$dg24_d70 = $this->xD70_', '$this->xD70_ = 0'], 1, 'nulls xD70_'),
    ('14828 manager back', 'loc:PortCDigitalGuardianHead.cpp:2392', [],
     ['((CDigitalGuardianHead*)$dg24_head)->xD70_ = $dg24_d70'], 1, 'restores xD70_'),
    ('14664 no collider manager', 'CDigitalGuardianHead::DisableDynamicCollision(', [],
     ['$dg24_head = $rdi', '$dg24_d70 = $this->xD70_', '$this->xD70_ = 0'], 1, 'nulls xD70_'),
    ('14664 manager back', 'loc:PortCDigitalGuardianHead.cpp:2421', [],
     ['((CDigitalGuardianHead*)$dg24_head)->xD70_ = $dg24_d70'], 1, 'restores xD70_'),
]
