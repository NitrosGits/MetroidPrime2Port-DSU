# Grenchler (module 27) gren6: FindJumpTarget 4AA4 with every jump point reported in use
# (CScriptAiJumpPoint::GetInUse stubbed to 1 for the direct call). Entrance room driver:
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 \
#     tools/cov/force.sh tools/cov/specs/grenchler_force6.py <out> 3dfd2249,01_swamp_entrance \
#     --con tools/cov/specs/grenchler_gren6b.con --fast --frames 750
CLASS = 'CGrenchler'
FORCE_RET = []
CALLS = [('FindJumpTarget direct calls', 'CGrenchler::PreRender(', '((CGrenchler*)$rdi)->mIsGrappleGuardian == 0', [
    # the room's jump points are active, in this area, and in use: skipped (retail 4C58 beq)
    ([], '$this->FindJumpTarget(*$mgr, 1)', [('CScriptAiJumpPoint::GetInUse(', 1)]),
], 'FindJumpTarget with GetInUse stubbed')]
