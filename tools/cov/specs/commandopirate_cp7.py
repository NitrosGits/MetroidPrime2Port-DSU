# CommandoPirate (module 9), slice cp7: the waypoint predicate (fn_9_F6DC / fn_9_F69C), the shield
# block's constructor (fn_9_EF80) and the E-grenade's aim point (fn_9_52BC), driven by
# tools/drivers/commandopirate_cp7.py in the Swamp (05_swamp_timber!).
#   COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh tools/cov/specs/commandopirate_cp7.py
#     <out> 'swamp,05_swamp_timber!' --fast --frames 4000
#     --driver 'python3 tools/drivers/commandopirate_cp7.py {port} <out>/drv.txt'
# Forced (each write lasts one call): the predicate gets kInvalidUniqueId on every other call (its
# false answer; every shipped ARRV/NEXT link leads to a waypoint); 52BC's player is morphed on
# calls 1 mod 3 and the target is the commando itself (not a CPlayer, and close) on calls 2 mod 3.
CLASS = 'CCommandoPirate'
try:  # the player's address for the morph write, so the restore at the return finds it again
    import gdb
    gdb.execute('set $cp7_pl = 0')
except ImportError:  # read outside gdb
    pass
PRED = '(anonymous namespace)::CCommandoPirateWaypointPredicate::IsValid('
FORCE_RET = [
    ('F6DC invalid id', PRED, 2, [(0, [('!$rdx', '0xffff')])], 'id = kInvalidUniqueId on even calls'),
    ('52BC morphed / self', 'CCommandoPirate::x52BC_GetGrenadeTarget(', 3,
     [(1, [('!$cp7_pl', '$rdx'), ('((CPlayer*)$cp7_pl)->mMorphBallState', '1')]), (2, [('!$rdx', '$rdi')])],
     'the player morphed on calls 1 mod 3, the target = the commando on calls 2 mod 3'),
]
SPEC = [
    ('F6DC/F69C IsValid', PRED, ['($rdx & 0xffff) == 0xffff'], 'ret max=2000'),
    # the loader's shieldData (the inlined ctor has no line of its own) and the next statement
    ('EF80 shield block (loader)', 'loc:PortCCommandoPirate.cpp:4796', [], ''),
    ('EF80 shield block done', 'loc:PortCCommandoPirate.cpp:4818', [], ''),
    ('52BC entry', 'CCommandoPirate::x52BC_GetGrenadeTarget(', ['$rdx == $rdi'], 'max=200'),
    ('52BC unmorphed player', 'loc:PortCCommandoPirateCore.cpp:761', [], 'max=200'),
    ('52BC far (> 6)', 'loc:PortCCommandoPirateCore.cpp:766', [], 'max=200'),
]
