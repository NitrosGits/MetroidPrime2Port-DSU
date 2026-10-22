# CIng fn_29_3684 (LeaveTeam) arm coverage, tools/cov/run.sh spec (lane tail7).
#
# The body is a file-static helper, so the arms are counted at the code the compiler emitted for
# each test rather than at a source line (an optimised `return;` has no line of its own: gdb
# resolves `loc:PortCIng.cpp:<line>` to the next statement). The sites are symbol-relative, so gdb
# re-resolves them after the PIE relocates; the offsets are the ones this build's disassembly shows
# (gdb `disassemble '_ZL9LeaveTeamR13CStateManagerR9TUniqueIdS1_'`):
#
#   +16   je +104    the id at 0xd6a is kInvalidUniqueId   (retail 0x36AC cmp / 0x36B0 beq)
#   +49   je +93     TCastToPtr gave no CScriptTeamAiMgr   (retail 0x36CC mr. / 0x36D0 beq)
#   +65   je +93     IsPartOfTeam said no                  (retail 0x36E8 clrlwi. / 0x36EC beq)
#   +67   mov 0x8(%rsp),%rdi   the QuitTeam (0x3704) + id reset (0x3710) arm
#   +104  ret        the invalid-id arm's own return
#
# `$eflags & 0x40` is ZF at a `je`: 1 = taken (that arm returns), 0 = falls through. Driver:
# tools/drivers/ing_team.py over Sand World 02_sand_sandfalls_dark layer 5 (the two ing spots
# there carry the ACTV/PLAY connection to the CScriptTeamAiMgr 0x14190161).
CLASS = 'CIng'
FN = '_ZL9LeaveTeamR13CStateManagerR9TUniqueIdS1_'
SPEC = [
    ('3684 LeaveTeam', 'LeaveTeam(CStateManager&, TUniqueId&, TUniqueId)', [], 'calls max=40'),
    ('3684 @0x36B0 id is invalid', 'loc:*%s+16' % FN, ['$eflags & 0x40'], 'max=40'),
    ('3684 @0x36D0 object is no team', 'loc:*%s+49' % FN, ['$eflags & 0x40'], 'max=40'),
    ('3684 @0x36EC not part of the team', 'loc:*%s+65' % FN, ['$eflags & 0x40'], 'max=40'),
    ('3684 @0x3704 QuitTeam arm', 'loc:*%s+67' % FN, [], 'max=40'),
    ('3684 @0x36B0 invalid-id return', 'loc:*%s+104' % FN, [], 'max=40'),
]
