# Splitter (module 75) forcing spec for tools/cov/force.sh: the arm the fight cannot reach.
# The head's Listen (fn_75_5F18) asks the base's flag byte at 0x420 first: with the actor dead
# the body returns false at once.  A dead head is deleted, so the noise system never asks a dead
# one - the arm is taken by writing the alive flag at the body's entry (one call).
CLASS = 'CSplitterCommandModule'
FORCE = [
    ('5F18 Listen', 'CSplitterCommandModule::PortListen(', ['$rcx'],
     ['set var $this->mAlive = 0'], 1,
     'the head is not alive: the Listen returns false without asking the distance'),
]
