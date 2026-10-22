# CommandoPirate (module 9) forcing spec for slice 30: the arms the area 59 run does not reach.
#
# Run: COV_GDB_SCRIPT=tools/cov/cov_force.py tools/cov/force.sh
#      tools/cov/specs/commandopirate_states2.py <out> swamp,05_swamp_timber!
#      --driver 'python3 tools/drivers/commandopirate_states2.py {port}'
#
# The cover point the WallHang and the Cover states need is the Swamp's first one (u05c8 - the
# driver puts the commando on it and finds it with the console's `objs cover`), and the byte at
# guest this+0xC66's 0x20 bit - which the knock-back helper sets when the shield is up - is
# written for the Dead state's shield effect.
CLASS = 'CCommandoPirate'
FORCE = [
    ('7358 WallHang cover', 'CCommandoPirate::WallHang(', ['$rdx'], ['$this->xbd2_.value = 0x5c8'], 2,
     'xbd2_ = u05c8 (a held cover point) from the 3rd call'),
    ('6DBC Cover cover', 'CCommandoPirate::Cover(', ['$rdx'], ['$this->xbd2_.value = 0x5c8'], 2,
     'xbd2_ = u05c8 (a held cover point) from the 3rd call'),
    ('69C8 Dead shield bit', 'CCommandoPirate::Dead(', ['$rdx'],
     ['$this->xc66_ = $this->xc66_ | 0x20'], 0,
     'xc66_ |= 0x20 (the byte at guest this+0xC66\'s 0x20 bit): the knock-back helper (fn_9_2D0C) '
     'sets it when the shield is up'),
    # The body controller's own state (the WallHang's "the body is in the wall-hang state", the
    # GetUp's "the commando lies on the ground" and the Cover's "the body is already in the cover
    # state") is not written here: the body controller is an rstl::single_ptr
    # (include/MetroidPrime/Enemies/CPatterned.hpp:409) and gdb answers "Could not find
    # operator->" for it - see the note's section 6.7.
]
