# CommandoPirate (module 9) forcing spec: the KnockBack override's dead arm (fn_9_C300, the
# vtable slot 0x9C).
#
# Run: the game with MP2_ALLOW_PTRACE=1 (the game opts in to attaches), then
#      COV_GDB_SCRIPT=tools/cov/commandopirate_force_after.py tools/cov/force.sh --attach <pid>
#      tools/cov/specs/commandopirate_c300_dead.py <out>
#
# The override takes its dead arm when the base's alive flag (the byte at guest this+0x420's 0x40
# bit) is clear.  The console's `damage` does nothing on a dead commando (its health is 0), so the
# flag is written at the override's entry instead.  The reaction flags (the word at guest
# this+0x630) are 1|2 = 3 only for the frozen reaction, and the death takes the frozen state with
# it, so the gdb side (tools/cov/commandopirate_force_after.py) writes them to 3 on the base's
# CPatterned::KnockBack return.  The module's own knock-back helper only runs when the byte at
# guest this+0xC65 has its 0x04 or 0x08 bit, so the driver puts the commando in the ArmShield
# state first.
CLASS = 'CCommandoPirate'
FORCE = [
    ('C300 dead', 'CCommandoPirate::KnockBack(', [], ['$this->mAlive = 0'], 0,
     'the base\'s alive flag: the override takes its dead arm when the flag is clear, and the '
     'console\'s damage does nothing on a dead commando'),
]

# The arms of the body, counted in the same run.
SPEC = [
    ('0xC300 KnockBack', 'CCommandoPirate::KnockBack(', [], 'calls max=3000'),
    ('0xC300 KnockBack: GetUp', 'loc:PortCCommandoPirate.cpp:1974', [], 'max=3000'),
    ('0xC300 KnockBack: death sound', 'loc:PortCCommandoPirate.cpp:1984', [], 'max=3000'),
]
