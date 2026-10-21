# CommandoPirate (module 9) forcing spec for tools/cov/force.sh; the labels are the retail offsets.
# The two arms of fn_9_BE4C that the area 59 fight cannot reach are forced here: the height range
# (the base's height range at guest this+0x3d4 is 0 there, so the tie arm runs instead) and the
# not-alive early-out (the alive flag's only writer kills the actor in the same update).
# Run: tools/cov/force.sh tools/cov/specs/commandopirate_force.py <out>
#       temple,05_temple_federationsite_dark --driver "python3 tools/drivers/commandopirate_warpout.py {port}"
CLASS = 'CCommandoPirate'
FORCE = [
    ('BE4C Listen: the height range (forced)', 'CCommandoPirate::xBE4C_Listen(',
     ['$rcx == 0'], ['$this->mDetectionHeightRange = 0.05'], 0,
     "the base's height range at guest this+0x3d4 is not 0, so the arm at 0x0000BF20 runs "
     "(the driver raises the noise by 5 cm, so the noise's height distance is over the range)"),
    ('BE4C Listen: the actor is not alive (forced)', 'CCommandoPirate::xBE4C_Listen(',
     [], ['$this->mAlive = 0'], 5,
     "the base's flag byte at guest this+0x420 has its 0x40 bit clear, so the early-out at "
     "0x0000BE4C runs"),
]
