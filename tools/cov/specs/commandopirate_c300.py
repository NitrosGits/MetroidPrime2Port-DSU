# CommandoPirate (module 9) forcing spec: the KnockBack override (fn_9_C300, the vtable slot 0x9C).
#
# Run: the game with MP2_ALLOW_PTRACE=1 (the game opts in to attaches), then
#      COV_GDB_SCRIPT=tools/cov/commandopirate_force_after.py tools/cov/force.sh --attach <pid>
#      tools/cov/specs/commandopirate_c300.py <out>
#
# The override's dead arm wants the actor dead (the console's `kill` does that) and the knock-back
# manager's active reaction flags (the word at guest this+0x630) at 3.  The flags are 1|2 = 3 only
# for the frozen reaction, and the death takes the frozen state with it, so they are written on the
# base's CPatterned::KnockBack return (the gdb side tools/cov/commandopirate_force_after.py).
# The module's own knock-back helper only runs when the byte at guest this+0xC65 has its 0x04 or
# 0x08 bit, so the driver puts the commando in the ArmShield state first.
CLASS = 'CCommandoPirate'
FORCE = [
    ('C300 flags on return', 'CCommandoPirate::KnockBack(', [],
     ['$this->mKnockBackController.mActiveParameters.mFlags = 3'], 0,
     'the knock-back manager\'s active reaction flags: a dead actor cannot hold the frozen '
     'reaction (the death takes the frozen state with it), so the flags are written to 3 on the '
     'base\'s return'),
]

# The arms of the body, counted in the same run.
SPEC = [
    ('0xC300 KnockBack', 'CCommandoPirate::KnockBack(', [], 'calls max=3000'),
    ('0xC300 KnockBack: GetUp', 'loc:PortCCommandoPirate.cpp:1974', [], 'max=3000'),
    ('0xC300 KnockBack: death sound', 'loc:PortCCommandoPirate.cpp:1984', [], 'max=3000'),
]
