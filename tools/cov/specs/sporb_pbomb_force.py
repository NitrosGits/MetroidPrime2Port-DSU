# Sporb (REL 76) forcing spec for tools/cov/force.sh: CSporbPowerBomb::Think's TestRayDamage-false
# arms (fn_76_11E24: 0x12038 player with the wait timer out, 0x12088 player inside the wait time,
# 0x1218C other actors). Nothing in the guardian's arena stands between a blast and the player,
# so after the first ray test (which passes and starts the wait timer) every ray test starts 500
# units lower, under the floor, and fails. CStateManager::TestRayDamage(position = rsi): the
# write lands in Think's `center`, which nothing reads after a failed test.
# Run: BUILD=build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/cov_force.py tools/cov/force.sh
#      tools/cov/specs/sporb_pbomb_force.py <out> 3dfd2249,11_swamp_bloggtank_dark
#      --con tools/cov/specs/sporb_pbomb.con --fast --frames 2000
CLASS = 'CStateManager'
FORCE = [
    ('11E24 Think: blast ray test fails (forced)', 'CStateManager::TestRayDamage(',
     ['$rdx != 0'], ['((float*)$rsi)[2] = ((float*)$rsi)[2] - 500'], 1,
     'the blast centre is moved 500 below the floor, so the ray to the damagee is blocked'),
]
SPEC = [
    ('11E24 Think', 'CSporbPowerBomb::Think(', ['((CSporbPowerBomb*)$rdi)->mState', '((CSporbPowerBomb*)$rdi)->mDamageWaitTimer > 0'], 'max=20000'),
]
