# CommandoPirate (module 9) spec, slice tail13: fn_9_3D64 `x3D64_SetWarpOut` (0xB8) - every guard
# both ways and both argument values.
#
# Retail (archive auto_00_00000000_text.s):
#   0x3D88 lbz 0x420, 0x3D8C extrwi. 1,25 (0x40)  0x3D90 beq .L_00003E00  - not alive: return
#   0x3D94 lbz 0xC65, 0x3D98 extrwi. 1,27 (0x10)  0x3D9C bne .L_00003E00  - the 0x10 bit: return
#   0x3DA0 lwz 0x920, 0x3DA4 cmpwi -1, 0x3DA8 bne .L_00003E00             - not -1: return
#   0x3DAC lbz 0x91D, 0x3DB0 extrwi. 1,24 (0x80)  0x3DB4 beq .L_00003E00  - no 0x80 bit: return
#   0x3DC8 string_l("WarpOut"), 0x3DE8 the vcall of the object at +0x350 (slot 0x20),
#   0x3DF8 rlwimi r0,r31,4,27,27, 0x3DFC stb 0xC63 - the 0x10 bit <- the argument.
#
# The callers (kSM_Escape 0xCF4C with r5=1, kSM_SetToZero 0xCF24 with r5=0) guard on
# `wasActive && (xc63_ & 0x20) == 0`, and kSM_Activate sets 0x20 whenever the block's +0x91D 0x80
# bit is set (0xCD7C) - which is the same bit the body needs. The driver clears 0x20 through the
# port console's `var <id> cp.c63` (a host debug accessor, no guest code) in the same tick as the
# message, so both call sites are reached without forcing anything in the body.
#
# FORCE_RET then writes one guard per call (restored at the call's return), so all four early
# returns run, and the fifth call of each cycle is left natural (the full path):
#   n%5==1 the alive bit 0 (0x3D90), ==2 the +0xC65 0x10 bit (0x3D9C)
#   ==3 +0x920 0 (0x3DA8), ==4 the block's +0x91D 0x80 bit cleared (0x3DB4), ==0 untouched
# The 0x20 bit of the caller's guard is cleared by the driver before each send, so the counters of
# the two sides stay in step.
#
#   COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh
#     tools/cov/specs/commandopirate_tail13.py <out> 1baa96c2,01_cliff_entrance --fast
#     --frames 1200 --driver 'python3 tools/drivers/commandopirate_tail13.py {port} <out>/drv.txt'

CLASS = 'CCommandoPirate'
F1 = '$this->mData.mFlags1'
C63, C65 = '$this->xc63_', '$this->xc65_'

FORCE_RET = [
    ('3D64 guards', 'CCommandoPirate::x3D64_SetWarpOut(', 5,
     [(1, [('$this->mAlive', '0')]),
      (2, [(C65, C65 + ' | 0x10')]),
      (3, [('$this->x920_', '0')]),
      (4, [(F1, F1 + ' & ~0x80')])],
     'the four early returns: the alive bit (0x3D90), +0xC65 0x10 (0x3D9C), +0x920 0 '
     '(0x3DA8), the block flag 0x80 clear (0x3DB4); every 5th call is left natural (0x3DC0..)'),
]

SPEC = [
    # the guard values at the body's entry: the caller's +0xC63 0x20 (wasActive && clear), the
    # alive bit, +0xC65 0x10, +0x920 and the block's +0x91D 0x80
    ('0x3D64 SetWarpOut', 'CCommandoPirate::x3D64_SetWarpOut(',
     [C63 + ' & 0x20', '$this->mAlive', C65 + ' & 0x10', '$this->x920_', F1 + ' & 0x80'],
     'max=200'),
]
