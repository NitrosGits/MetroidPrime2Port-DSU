# CommandoPirate forcing spec: the KnockBack override's dead arm (the vtable slot 0x9C).
# Run: tools/cov/force.sh tools/cov/specs/commandopirate_knockback.py <out> 3bfa3eff,645ef243
#      --driver "python3 tools/drivers/commandopirate_dead.py {port} <out>.drv"
#
# The override's dead arm wants two things at once: the actor dead (the console's `kill` does that
# - it zeroes the health and runs the actor's Death, which clears the alive flag at guest
# this+0x420) and the knock-back manager's active reaction flags at 3 (the word at guest
# this+0x630, the manager's +0x10).  The flags are 1|2 = 3 only for the frozen reaction, and
# the death takes the frozen state with it, so a dead actor never has them.  They are written to
# 3 at the override's entry for the one call the driver makes after the kill.
CLASS = 'CCommandoPirate'
FORCE = [
    ('C300 flags', 'CCommandoPirate::KnockBack(', [],
     ['$this->mKnockBackController.mActiveParameters.mFlags = 3'], 1,
     'the knock-back manager\'s active reaction flags: a dead actor cannot hold the frozen '
     'reaction (the death takes the frozen state with it), so the flags are written to 3 for the '
     'one call the driver makes after the kill'),
]
