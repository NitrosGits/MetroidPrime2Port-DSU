# CommandoPirate forcing spec: the IsFacingTarget trigger's "the target has no aim position" arm
# (the early return at 0x0000B860).  The trigger's target id (the id at guest this+0xBCA, the
# port's xbca_) is written to an invalid id at the trigger's entry, so the module's helper
# x3CBC_ cannot find the target and the early arm runs.
CLASS = 'CCommandoPirate'
FORCE = [
    ('IsFacingTarget target', 'CCommandoPirate::IsFacingTarget(', [], ['$this->xbca_ = 0'], 0,
     'the trigger target id: an invalid id makes the module helper x3CBC_ fail, so the early '
     'return (the target has no aim position) runs'),
]
