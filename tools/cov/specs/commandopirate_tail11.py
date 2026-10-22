# CommandoPirate (module 9) spec, slice tail11: the two team helpers' arms, counted by callee
# (`calls`), with the team id forced so both the "no team manager" and the "not part of the team"
# exits run (tools/cov/force_ret.py; every write is restored at the call's return).
#
#   COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh
#     tools/cov/specs/commandopirate_tail11.py <out> 3bfa3eff,05_temple_federationsite_dark --fast
#     --frames 3000 --driver 'python3 tools/drivers/commandopirate_cp3.py {port} <out>/drv.txt'
#
# 0x1930 LeaveTeam: the invalid-id exit (0x0000195C), the null manager (0x0000197C, the own id
# forced as the team), the "not part of the team" exit (0x00001998, the old id written back over
# the body's QuitTeam) and the quit (0x000019B4) by the driver's Deactivate/Activate + SetTargetDest,
CLASS = 'CCommandoPirate'
SELF = '$this->mUniqueId.value'
TEAM = '$this->xbc4_teamId.value'

FORCE_RET = [
    # (1, TEAM = itself) is written back at the return, so the body's QuitTeam is undone and the
    # next call takes the "not part of the team" exit; (3, TEAM = the own id) makes ObjectById
    # return a non-manager (the null cast at 0x0000197C).
    # The driver sends Deactivate twice in a row: the first call's id is written back at its
    # return, so the second one finds a valid id whose manager no longer holds the commando
    # (0x00001998). Every third call the own id stands in for the team (0x0000197C, the null cast).
    ('1930 LeaveTeam', 'CCommandoPirate::x1930_LeaveTeam(', 4,
     [(1, [(TEAM, TEAM)]), (3, [(TEAM, SELF)])],
     'the team id kept over the quit (0x00001998) and the own id as the team (0x0000197C)'),
    # The join's two flag arms (0x00001A14 with 0x02 alone, 0x00001A20 with both) and the
    # "already in a team" exit (0x00001A08): the driver's Activate/Deactivate pair keeps the id
    # coming and going.
    ('19D8 JoinTeam', 'CCommandoPirate::x19D8_JoinTeam(', 6,
     [(1, [(TEAM, '0xffff'), ('$this->mData.mFlags0', '$this->mData.mFlags0 | 0x02'),
           ('$this->mData.mFlags1', '$this->mData.mFlags1 | 0x40')]),
      (3, [(TEAM, '0xffff'), ('$this->mData.mFlags0', '$this->mData.mFlags0 | 0x02'),
           ('$this->mData.mFlags1', '$this->mData.mFlags1 & ~0x40')])],
     'no team with the block flags 0x02 and 0x40 (0x00001A20) / 0x02 alone (0x00001A14)'),
]

SPEC = [
    ('0x1930 LeaveTeam', 'CCommandoPirate::x1930_LeaveTeam(', [], 'calls max=20000'),
    ('0x19D8 JoinTeam', 'CCommandoPirate::x19D8_JoinTeam(', [], 'calls max=20000'),
]
