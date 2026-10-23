# CommandoPirate (module 9) spec, slice tail12: every reader of the loader's block flag bytes
# (mData.mFlags0 = the block's +0x15C, mData.mFlags1 = +0x15D) after review26 re-decoded the
# packing (asm 0xEED4..0xEF5C: flags0 bit i = the "sound" word's bit i -> 0x80>>i, flags1 bits
# 8..11). The shipped words 0x80 / 0x300 / 0x114 / 0x104 give flags0 0x01 / 0x00 / 0x28 / 0x20
# and flags1 0x00 / 0xC0 / 0x80 / 0x80 (nine CMDO placements in three areas; the scan is in the
# module note, section "tail12").
#
# FORCE_RET toggles each reader's flag bit(s) on alternate calls (tools/cov/force_ret.py restores
# them at the call's return), so both arms of every reader run in one game even where the shipped
# data cannot set the bit; the SPEC entries record the flag value at the body's entry and the
# return value (bool readers) or the call each arm ends in.
#
#   COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh
#     tools/cov/specs/commandopirate_tail12.py <out> 1baa96c2,01_cliff_entrance --fast
#     --frames 1000 --driver 'python3 tools/drivers/commandopirate_tail12.py {port} <out>/drv.txt'
import os

CLASS = 'CCommandoPirate'
F0 = '$this->mData.mFlags0'
F1 = '$this->mData.mFlags1'
C63, C64, C65 = '$this->xc63_', '$this->xc64_', '$this->xc65_'


def ent(label, fn, period, phases, what):
    return (label, 'CCommandoPirate::%s(' % fn, period, phases, what)


FORCE_RET = [
    # 0xE258 (ctor): the flag is an argument, not a branch; not forced (the tooling writes at a
    # body's entry, where mData is not built yet), recorded by the SPEC loc entry below.
    # 0x4F58 (x4D84_UpdateAim): behind CanAim() == false with +0xC64 0x10 set.
    ent('4D84 aim flag 0x10', 'x4D84_UpdateAim', 4,
        [(1, [(C65, C65 + ' | 0x01'), (C64, C64 + ' | 0x10'), (F0, F0 + ' | 0x10')]),
         (3, [(C65, C65 + ' | 0x01'), (C64, C64 + ' | 0x10'), (F0, F0 + ' & ~0x10')])],
        'the block flag 0x10 set (the return at 0x00004F60) / clear (the idle at 0x00004F84)'),
    ent('4CA4 aim gate flag 0x10', 'x4CA4_CanAim', 4,
        [(1, [(C64, C64 + ' | 0x10'), (F0, F0 + ' | 0x10')]),
         (3, [(C64, C64 + ' | 0x10'), (F0, F0 + ' & ~0x10')])],
        'the block flag 0x10 at 0x00004CEC'),
    ent('CB40 message flags', 'AcceptScriptMsg', 4,
        [(1, [(F1, F1 + ' | 0x80'), (F0, F0 + ' | 0x01')]),
         (3, [(F1, F1 + ' & ~0x80'), (F0, F0 + ' & ~0x01')])],
        'the Activate block flag (0x0000CD3C) and the AreaLoaded block flag (0x0000CE04)'),
    ('3E1C rag doll flags', 'CCommandoPirate::x3E1C_UpdateRagDoll(', 2,
     [(1, [(F0, F0 + ' | 0x01')]), (0, [(F0, F0 + ' & ~0x01')])],
     'the rag doll created with the flags 3 (0x000040A0) / 0'),
    ('C300 knock-back flag 0x01', 'CCommandoPirate::KnockBack(', 2,
     [(1, [(F0, '$this->xbfc_ragDoll.mPtr != 0 ? $this->mData.mFlags0 | 0x01 : '
              '$this->mData.mFlags0')]),
      (0, [(F0, '$this->xbfc_ragDoll.mPtr != 0 ? $this->mData.mFlags0 & ~0x01 : '
              '$this->mData.mFlags0')])],
     'the torso arm (0x0000C324) / the rag-doll-only exit (0x0000C3B0)'),
    ent('BD98 ShouldWarpIn flag 0x80', 'ShouldWarpIn', 4,
        [(1, [('$this->x920_', '-1'), (F1, F1 + ' | 0x80'), (C63, C63 + ' | 0x20')]),
         (3, [('$this->x920_', '-1'), (F1, F1 + ' & ~0x80'), (C63, C63 + ' | 0x20')])],
        'the block flag 0x80 (0x0000BDAC)'),
    ent('BCB8 ShouldMeleeAttack flag 0x02', 'ShouldMeleeAttack', 4,
        [(1, [(F0, F0 + ' | 0x02')]), (3, [(F0, F0 + ' & ~0x02')])],
        'the block flag 0x02 (0x0000BCD0)'),
    ent('BBC8 ShouldFireEGrenade flag 0x10', 'ShouldFireEGrenade', 5,
        [(1, [(C63, C63 + ' | 0x02'), ('sEgrenadeObjectCount', '0'), (F1, F1 + ' | 0x10')]),
         (3, [(C63, C63 + ' | 0x02'), ('sEgrenadeObjectCount', '0'), (F1, F1 + ' & ~0x10')])],
        'the block flag 0x10 (0x0000BBFC)'),
    ent('BB7C ShouldRetreat flags', 'ShouldRetreat', 5,
        [(1, [(C63, C63 + ' | 0x40'), (F1, F1 + ' | 0x80'), (F0, F0 + ' | 0x08')]),
         (3, [(C63, C63 + ' | 0x40'), (F1, F1 + ' & ~0x80'), (F0, F0 + ' & ~0x08')])],
        'the block flags 0x80 and 0x08 (0x0000BB8C, 0x0000BB98)'),
    ent('BB10 ShouldJumpBack flag 0x08', 'ShouldJumpBack', 4,
        [(1, [(F0, F0 + ' | 0x08')]), (3, [(F0, F0 + ' & ~0x08')])],
        'the block flag 0x08 (0x0000BB20)'),
    ent('BB04 ShouldAmbush flag 0x80', 'ShouldAmbush', 4,
        [(1, [(F0, F0 + ' | 0x80')]), (3, [(F0, F0 + ' & ~0x80')])],
        'the block flag 0x80 (0x0000BB08)'),
    ent('BAF8 BreakAmbush flag 0x40', 'BreakAmbush', 4,
        [(1, [(F0, F0 + ' | 0x40')]), (3, [(F0, F0 + ' & ~0x40')])],
        'the block flag 0x40 (0x0000BAFC)'),
    ent('B690 ShouldDodge flag 0x20', 'ShouldDodge', 4,
        [(1, [(C63, C63 + ' | 0x04'), (F0, F0 + ' | 0x20')]),
         (3, [(C63, C63 + ' | 0x04'), (F0, F0 + ' & ~0x20')])],
        'the block flag 0x20 (0x0000B6A8)'),
    ent('B660 ShouldArmShield flag 0x20', 'ShouldArmShield', 4,
        [(1, [(C64, C64 + ' | 0x80'), (F1, F1 + ' | 0x20')]),
         (3, [(C64, C64 + ' | 0x80'), (F1, F1 + ' & ~0x20')])],
        'the block flag 0x20 at 0x0000B674'),
    ent('B0F4 ShouldCover flag 0x08', 'ShouldCover', 4,
        [(1, [(C63, C63 + ' | 0x01'), (F0, F0 + ' | 0x08')]),
         (3, [(C63, C63 + ' | 0x01'), (F0, F0 + ' & ~0x08')])],
        'the block flag 0x08 at 0x0000B0F8'),
    ent('19D8 JoinTeam flags', 'x19D8_JoinTeam', 4,
        [(1, [('$this->xbc4_teamId.value', '0xffff'), (F0, F0 + ' | 0x02'), (F1, F1 + ' | 0x40')]),
         (3, [('$this->xbc4_teamId.value', '0xffff'), (F0, F0 + ' | 0x02'),
              (F1, F1 + ' & ~0x40')])],
        'the 0x02 and 0x40 bits together (0x00001A20) / 0x02 alone (0x00001A14)'),
    ent('9AAC PathFind flag 0x08', 'PathFind', 4,
        [(1, [(C65, C65 + ' | 0x80'), (F0, F0 + ' | 0x08')]),
         (3, [(C65, C65 + ' | 0x80'), (F0, F0 + ' & ~0x08')])],
        'the block flag 0x08 at 0x00009B58'),
    ent('3D64 SetWarpOut flag 0x80', 'x3D64_SetWarpOut', 4,
        [(1, [('$this->x920_', '-1'), (C65, C65 + ' & ~0x10'), (C63, C63 + ' & ~0x20'),
              (F1, F1 + ' | 0x80')]),
         (3, [('$this->x920_', '-1'), (C65, C65 + ' & ~0x10'), (C63, C63 + ' & ~0x20'),
              (F1, F1 + ' & ~0x80')])],
        'the block flag 0x80 (0x00003DAC) / clear (0x00003DB4)'),
    ent('B250 ShouldShieldCharge flag 0x40', 'ShouldShieldCharge', 4,
        [(1, [(C64, C64 + ' | 0x40'), (F1, F1 + ' | 0x40')]),
         (3, [(C64, C64 + ' | 0x40'), (F1, F1 + ' & ~0x40')])],
        'the block flag 0x40 (0x0000B2C0)'),
]

SPEC = [
    # the ctor's argument (the flag is read once per instance, at line 314)
    ('0xD904 ctor knock-back physics', 'loc:PortCCommandoPirate.cpp:314',
     ['this->mData.mFlags0 & 0x04'], 'max=200'),
    # the readers that answer with a value: the flag value at the body's entry and the result
    ('0xBD98 ShouldWarpIn', 'CCommandoPirate::ShouldWarpIn(',
     ['$this->x920_', F1 + ' & 0x80', C63 + ' & 0x20'], 'ret max=20000'),
    ('0xBCB8 ShouldMeleeAttack', 'CCommandoPirate::ShouldMeleeAttack(',
     [F0 + ' & 0x02'], 'ret max=20000'),
    ('0xBBC8 ShouldFireEGrenade', 'CCommandoPirate::ShouldFireEGrenade(',
     [F1 + ' & 0x10'], 'ret max=20000'),
    ('0xBB7C ShouldRetreat', 'CCommandoPirate::ShouldRetreat(',
     [C63 + ' & 0x40', F1 + ' & 0x80', F0 + ' & 0x08'], 'ret max=20000'),
    ('0xBB10 ShouldJumpBack', 'CCommandoPirate::ShouldJumpBack(',
     [F0 + ' & 0x08'], 'ret max=20000'),
    ('0xBB04 ShouldAmbush', 'CCommandoPirate::ShouldAmbush(',
     [F0 + ' & 0x80'], 'ret max=20000'),
    ('0xBAF8 BreakAmbush', 'CCommandoPirate::BreakAmbush(',
     [F0 + ' & 0x40'], 'ret max=20000'),
    ('0xB690 ShouldDodge', 'CCommandoPirate::ShouldDodge(',
     [F0 + ' & 0x20'], 'ret max=20000'),
    ('0xB660 ShouldArmShield', 'CCommandoPirate::ShouldArmShield(',
     [C64 + ' & 0x80', F1 + ' & 0x20'], 'ret max=20000'),
    ('0xB0F4 ShouldCover', 'CCommandoPirate::ShouldCover(',
     [F0 + ' & 0x08', C63 + ' & 0x01'], 'ret max=20000'),
    ('0xB250 ShouldShieldCharge (unmarked)', 'CCommandoPirate::ShouldShieldCharge(',
     [C64 + ' & 0x40', F1 + ' & 0x40'], 'ret max=20000'),
    # the readers whose arm ends in a branch or a call: the flag value at the body's entry
    ('0x4D84 UpdateAim', 'CCommandoPirate::x4D84_UpdateAim(',
     [F0 + ' & 0x10', '$this->xc64_ & 0x10'], 'max=20000'),
    ('0x4CA4 CanAim', 'CCommandoPirate::x4CA4_CanAim(',
     [F0 + ' & 0x10', '$this->xc64_ & 0x10'], 'ret max=20000'),
    ('0xCB40 AcceptScriptMsg', 'CCommandoPirate::AcceptScriptMsg(',
     [F0 + ' & 0x01', F1 + ' & 0x80'], 'max=20000'),
    ('0x3E1C UpdateRagDoll', 'CCommandoPirate::x3E1C_UpdateRagDoll(',
     [F0 + ' & 0x01', '$this->xbfc_ragDoll.mPtr != 0'], 'max=20000'),
    ('0xC300 KnockBack', 'CCommandoPirate::KnockBack(',
     [F0 + ' & 0x01', '$this->xbfc_ragDoll.mPtr != 0'], 'max=20000'),
    ('0x19D8 JoinTeam', 'CCommandoPirate::x19D8_JoinTeam(',
     [F0 + ' & 0x02', F1 + ' & 0x40', '$this->xbc4_teamId.value'], 'max=20000'),
    ('0x9AAC PathFind', 'CCommandoPirate::PathFind(',
     [F0 + ' & 0x08', C65 + ' & 0x80'], 'max=20000'),
    ('0x3D64 SetWarpOut', 'CCommandoPirate::x3D64_SetWarpOut(',
     [F1 + ' & 0x80', '$this->x920_'], 'max=20000'),
]
