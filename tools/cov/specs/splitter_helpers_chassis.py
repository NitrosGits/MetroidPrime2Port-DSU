# Splitter (module 75) proof spec for tools/cov/run.sh: the helpers lane's chassis bodies. The
# labels are the retail offsets (fn_75_XXXX); the arms are the host's own conditions.
# Run: 04_cliff_contraption layer 2 with tools/cov/specs/splitter_helpers.con.
CLASS = 'CSplitterMainChassis'
SPEC = [
    # fn_75_E99C: the chassis's CanBeIngPossessed ends with false when one of its own three flags
    # is set (the byte at 0xD41's 0x80 bit, the byte at 0xD40's 0x01 bit and the byte at 0xD41's
    # 0x40 bit).
    ('E99C CanBeIngPossessed', 'CSplitterMainChassis::PortCanBeIngPossessed(',
     ['$this->mFlagsD41 & 0x80', '$this->mFlagsD40 & 0x01', '$this->mFlagsD41 & 0x40'], 'ret'),
    # fn_75_E810: the chassis's SetAttackTarget keeps the id of a CIngSnatchingSwarm target.
    ('E810 SetAttackTarget', 'CSplitterMainChassis::PortSetAttackTarget(', ['$rdx & 0xffff'], 'calls'),
    # fn_75_E90C: the chassis's SetIngPossessed (the bool form) passes the possession on to the
    # head it holds.
    ('E90C SetIngPossessed', 'CSplitterMainChassis::PortSetIngPossessed(',
     ['$this->mCommandModuleId.value != 0xffff'], 'calls'),
]
