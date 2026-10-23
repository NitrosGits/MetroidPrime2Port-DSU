# sp13 (2026-10-23): does a shipped pirate death reach retail's rag doll creation? Retail's Think
# tail (fn_72_1980, 0x1F08-0x2030) builds a CPirateRagDoll into 0xb30 once the delay timer 0xb2c,
# armed by the BecomeRagDoll anim event (3B28, kUE_BecomeRagDoll) or by a floating corpse (E498),
# has counted down. The arming is read from the coverage build's gcov line counts (the
# kUE_BecomeRagDoll case in PortCSpacePirate.cpp: one per kill; a gdb `loc:` breakpoint there did
# not resolve in the coverage build). This spec counts the GetRagDoll / Touch calls: both read
# 0xb30, which the port never sets (the tail is unported).
# Run: tools/cov/run.sh <this> <out> 42b935e4,07_sand_alamoportalobservation --driver
#   'python3 tools/drivers/spacepirate_drive.py {port} - 0' --fast, SP_STEPS with
#   `layer 3; ...; damage {002a00af} 1000` (the kills).
CLASS = 'CSpacePirate'
SPEC = [
    ('17A0 GetRagDoll', 'CSpacePirate::GetRagDoll(', ['$this->xb30_ != 0'], 'retp'),
    ('E3D8 Touch', 'CSpacePirate::Touch(', ['$this->xb30_ != 0'], 'max=2000'),
]
