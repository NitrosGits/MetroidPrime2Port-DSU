# DigitalGuardian (module 14), dg26: the arms the area-53 data and the console cannot reach.
# - fn_14_11FEC / 12B04 with no effect token (xFD0_ / xF14_ unset: the only DGHD placement sets
#   both assets, so the ctor always locks them); the flag is cleared at entry and set back after
#   the effect block.
# - fn_14_13248 with an id that is no ball (the second XDMG of the run's ball): -1, so fn_14_12B04
#   returns at once. The head's other collider, the root, resists every weapon, so it never sends
#   XDMG at no health.
# - AcceptScriptMsg (fn_14_17D38) ACTV/DCTV/XAUD/XDEL with no collider manager (xD70_ is built at
#   XCRT, which AddObject delivers first); nulled at entry for those messages, restored at the next Think.
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg26_force.py \
#        <out> 1baa96c2,09_cliff_temple_dark --driver 'python3 tools/drivers/dg26_slots.py {port} a' \
#        --fast --frames 2400
CLASS = 'CDigitalGuardianHead'
SPEC = []
_MSG = ('(msg.m_msg == kSM_Delete || msg.m_msg == kSM_Activate || msg.m_msg == kSM_Deactivate'
        ' || msg.m_msg == kSM_AIUpdateDisabled)')
FORCE = [
    ('11FEC no effect token', 'CDigitalGuardianHead::x11FEC_(', [],
     ['$dg26_h1 = $rdi', '$this->xFD0_.m_valid = 0'], 0, 'clears xFD0_.m_valid'),
    ('11FEC token back', 'loc:PortCDigitalGuardianHead.cpp:2820', [],
     ['((CDigitalGuardianHead*)$dg26_h1)->xFD0_.m_valid = 1'], 0, 'sets xFD0_.m_valid'),
    ('12B04 no effect token', 'CDigitalGuardianHead::x12B04_(', [],
     ['$dg26_h2 = $rdi', '$this->xF14_.m_valid = 0'], 0, 'clears xF14_.m_valid'),
    ('12B04 token back', 'loc:PortCDigitalGuardianHead.cpp:2858', [],
     ['((CDigitalGuardianHead*)$dg26_h2)->xF14_.m_valid = 1'], 0, 'sets xF14_.m_valid'),
    ('13248 no ball', 'CDigitalGuardianHead::x13248_(', [], ['$rsi = 0xffff'], 1,
     'the id (si) becomes 0xffff'),
    ('17D38 save slot', 'CDigitalGuardianHead::SetupStateMachine(', [], ['$dg26_d70 = 0'], 0,
     'zeroes the gdb save slot'),
    ('17D38 no collider manager', 'CDigitalGuardianHead::AcceptScriptMsg(', ['msg.m_msg'],
     ['$dg26_d70 = $this->xD70_ ? (long)$this->xD70_ : $dg26_d70',
      '$this->xD70_ = %s ? 0 : $this->xD70_' % _MSG], 0, 'nulls xD70_ for ACTV/DCTV/XAUD/XDEL'),
    ('17D38 manager back', 'CDigitalGuardianHead::Think(', [],
     ['$this->xD70_ = $this->xD70_ ? $this->xD70_ : (CCollisionActorManager*)$dg26_d70'], 0,
     'restores xD70_ before the next Think'),
]
