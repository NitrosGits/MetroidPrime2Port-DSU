# CommandoPirate (module 9) forcing spec: the CoverFind state's search radius.
# Run: tools/cov/force.sh tools/cov/specs/commandopirate_wallhang.py <out> 1baa96c2,01_cliff_entrance
#      --con tools/cov/specs/commandopirate_wallhang.con
#
# The Cliff's entrance holds the four cover points and the six commandoes, and the CoverFind
# state (fn_9_709C) finds none of them: the block's search radius is 50 units (the block's
# +0x54) and the three cover points nearest the commando are 62, 97 and 108 units away, so the
# radius rules them out (the fourth is inside the radius but the path find does not reach it).
# The radius is forced to 120 for the CoverFind call the driver makes, so the 62-unit point can
# be taken and the WallHang state's cover point path runs.
CLASS = 'CCommandoPirate'
FORCE = [
    ('CF radius', 'CCommandoPirate::CoverFind(', [], ['$this->mData.mSearchRadius = 120.0f'], 0,
     'the cover search radius: the block says 50, the nearest cover point in 01_cliff_entrance is '
     '62 units away (the next are 97 and 108), so the radius is what keeps the commando from '
     'taking one'),
    # what the WallHang state sees: the cover point the module holds (the id at guest this+0xBD2)
    # and the cover point's forward (guest this+0xBA0, the zero vector when there is none).
    ('WH cover', 'CCommandoPirate::WallHang(', ['$this->xbd2_.value != 0', '$this->xba0_.mX != 0.0f', '$this->x924_'],
     0, 'the cover point the commando holds and its forward vector'),
]
