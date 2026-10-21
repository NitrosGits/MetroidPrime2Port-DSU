# CommandoPirate (module 9) coverage spec for tools/cov/run.sh; the labels are the retail offsets.
# Run: Temple Grounds 05_temple_federationsite_dark (area 59) with the CommandoPirate lane's
# driver (tools/drivers/commandopirate_warpout.py), which wakes the commando and makes the
# noises its Listen override (fn_9_BE4C) has to answer.
CLASS = 'CCommandoPirate'
SPEC = [
    # fn_9_BE4C: the actor hears. The body: the base's flag byte's 0x02 bit (the host's x420_26_),
    # then the noise type - 4 (a path noise) and 0 (a shot) are the two that matter, every other
    # type returns 0. The loc: entries are the lines of the built source, one per arm.
    ('BE4C Listen', 'CCommandoPirate::xBE4C_Listen(', [], 'ret max=400'),
    ('BE4C Listen: the actor is not alive (0x0000BE4C)', 'loc:PortCCommandoPirateHelpers.cpp:36',
     [], 'ret max=400'),
    ('BE4C Listen: a path noise, too far (0x0000BE7C)',
     'loc:PortCCommandoPirateHelpers.cpp:44', [], 'ret max=400'),
    ('BE4C Listen: a path noise, heard (0x0000BF0C)',
     'loc:PortCCommandoPirateHelpers.cpp:46', [], 'ret max=400'),
    ('BE4C Listen: a shot, too far (0x0000BEF4)',
     'loc:PortCCommandoPirateHelpers.cpp:52', [], 'ret max=400'),
    ('BE4C Listen: a shot, the height range (0x0000BF20)',
     'loc:PortCCommandoPirateHelpers.cpp:56', [], 'ret max=400'),
    ('BE4C Listen: a shot, heard (0x0000BF34)',
     'loc:PortCCommandoPirateHelpers.cpp:58', [], 'ret max=400'),
    ('BE4C Listen: another noise type (0x0000BF3C)',
     'loc:PortCCommandoPirateHelpers.cpp:61', [], 'ret max=400'),
]
