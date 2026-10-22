# CommandoPirate (module 9), slice cp9: the arms come from the coverage build's gcov counters
# (tools/cov/gcov_report.py build-port-cov <out>/gcov --module CommandoPirate); this spec only adds
# the one forced arm the console cannot reach and counts the entries. Driver:
#   BUILD=build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh
#     tools/cov/specs/commandopirate_cp9.py <out> 'swamp,05_swamp_timber!' --fast --frames 9000
#     --driver 'python3 tools/drivers/commandopirate_cp9.py {port} <out>/drv.txt'
# Forced (put back at the return): a fake jump point's timer (CScriptAiJumpPoint +0x184,
# mTimeRemaining) on GetInUse's calls 0 mod 3, so JumpPointFind (fn_9_9F24) sees a jump point in
# use (0x00009FFC..0x0000A004); the host's jump point has no code that sets it in use.
CLASS = 'CCommandoPirate'
FORCE_RET = [
    ('9F24 jump point in use', 'CScriptAiJumpPoint::GetInUse(', 3,
     [(0, [('$this->mTimeRemaining', '1.0')])], 'mTimeRemaining = 1 on calls 0 mod 3'),
]
SPEC = [
    ('9F24 JumpPointFind entry: msg', 'CCommandoPirate::JumpPointFind(', ['$rdx'], 'max=400'),
    ('9BB4 JetBoost entry: msg', 'CCommandoPirate::JetBoost(', ['$rdx'], 'max=400'),
    ('GetInUse', 'CScriptAiJumpPoint::GetInUse(', [], 'ret max=400'),
]
