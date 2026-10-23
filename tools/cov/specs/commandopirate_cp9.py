# CommandoPirate (module 9), slice cp9: the arms come from the coverage build's gcov counters
# (tools/cov/gcov_report.py build-port-cov <out>/gcov --module CommandoPirate); this spec only adds
# the one forced arm the console cannot reach and counts the entries. Driver:
#   BUILD=build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh
#     tools/cov/specs/commandopirate_cp9.py <out> 'swamp,05_swamp_timber!' --fast --frames 60000
#     --driver 'python3 tools/drivers/commandopirate_cp9.py {port} <out>/drv.txt'
# Forced (put back at the return): a fake jump point's timer (CScriptAiJumpPoint +0x184,
# mTimeRemaining) on GetInUse's calls 0 mod 3, so JumpPointFind (fn_9_9F24) sees a jump point in
# use (0x00009FFC..0x0000A004); the host's jump point has no code that sets it in use.
# Forced at a callee's return (FORCE_INNER): ShouldShieldCharge's (fn_9_B250) three world rays,
# clear unless forced (the calls 1, 3 and 6 of every nine answer blocked: the first ray, then the
# second, then the third, then none - the driver's `shieldcharge` block makes four calls that
# reach the rays); the knock-back helper's (fn_9_2D0C) reaction after its KnockBack (0x00002E38)
# on every second call, 3 (the 2..4 band, 0x00002E50/0x00002E58); ShouldShieldCharge's team
# answer (CanStartAttack, 0x0000B3D8) true from its second call (no swamp team has the commando);
# the knock-back helper's ray under the commando (0x00002FC4) clear while the driver holds xb54 at
# 4242 (the last commando's final shots: the death and the damage source, 0x00003068).
CLASS = 'CCommandoPirate'
FORCE_RET = [
    ('9F24 jump point in use', 'CScriptAiJumpPoint::GetInUse(', 3,
     [(0, [('((CScriptAiJumpPoint*)$this)->mTimeRemaining', '1.0')])], 'mTimeRemaining = 1 on calls 0 mod 3'),
]
RAY = 'CStateManager::RayCollideWorld('
FORCE_INNER = [
    ('B250 rays', 'CCommandoPirate::ShouldShieldCharge(', RAY, 9,
     {1: [('$rax', '0')], 2: [('$rax', '1')], 3: [('$rax', '0')], 4: [('$rax', '1')],
      5: [('$rax', '1')], 6: [('$rax', '0')], 7: [('$rax', '1')], 8: [('$rax', '1')],
      0: [('$rax', '1')]}, 'ray answers F | T F | T T F | T T T'),
    ('B250 team says yes', 'CCommandoPirate::ShouldShieldCharge(', 'CScriptTeamAiMgr::CanStartAttack(',
     1000, {p: [('$rax', '1')] for p in range(2, 1000)},
     'CanStartAttack true from its second call (the first: no team, false)'),
    ('2D0C ray clear', 'CCommandoPirate::x2D0C_KnockBack(', RAY, 1,
     {0: [('$rax', '$this->xb54_ == 4242 ? 1 : $rax')]},
     'the ray under the commando clear while xb54 is 4242 (the driver\'s last shots)'),
    ('2D0C reaction', 'CCommandoPirate::x2D0C_KnockBack(', 'CCommandoPirate::KnockBack(', 2,
     {1: [('$this->mKnockBackController.mActiveParameters.mReaction', '3')]},
     'reaction 3 after the knock-back'),
]
SPEC = [
    ('9F24 JumpPointFind entry: msg', 'CCommandoPirate::JumpPointFind(', ['$rdx'], 'max=400'),
    ('9BB4 JetBoost entry: msg', 'CCommandoPirate::JetBoost(', ['$rdx'], 'max=400'),
    ('GetInUse', 'CScriptAiJumpPoint::GetInUse(', [], 'ret max=400'),
    ('B250 ShouldShieldCharge', 'CCommandoPirate::ShouldShieldCharge(', [], 'ret max=4000'),
]
