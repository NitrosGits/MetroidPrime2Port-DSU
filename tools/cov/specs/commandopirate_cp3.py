# CommandoPirate (module 9) forcing spec, slice cp3: the triggers no shipped FSM2 state evaluates
# (the driver calls them with `trigcall`), the Lurk/Alert/SetJumpDest/SetCoverDest/JumpBack arms
# and the team helpers. Every write lasts one call (tools/cov/force_ret.py); SPEC records the
# answers of the branch-free triggers.
#
# Run (the coverage build; the gcov counters of these runs add to the natural runs'):
#   BUILD=build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh
#     tools/cov/specs/commandopirate_cp3.py <out> 3bfa3eff,05_temple_federationsite_dark --fast
#     --frames 9000 --driver 'python3 tools/drivers/commandopirate_cp3.py {port} <out>/drv.txt'
#   (and the same in 'swamp,05_swamp_timber!', the area with cover points)
CLASS = 'CCommandoPirate'
INVALID = '0xffff'
SELF = '$this->mUniqueId.value'
F0 = '$this->mData.mFlags0'
F1 = '$this->mData.mFlags1'
TGT = '$this->xbca_.value'
TEAM = '$this->xbc4_teamId.value'
C64, C66 = '$this->xc64_', '$this->xc66_'
ON_ACT = lambda lv, v: (lv, '$rdx == 0 ? %s : %s' % (v, lv))  # noqa: E731


def ent(label, fn, period, phases, what):
    return (label, 'CCommandoPirate::%s(' % fn, period, phases, what)


FORCE_RET = [
    ent('BAB4 HeardShot', 'HeardShot', 2, [(1, [('$this->xb60_', '0.0')])],
        'the heard-shot distance under 1.5 (0x0000BAC0)'),
    ent('B0B4 ShouldCoverAttack', 'ShouldCoverAttack', 2,
        [(1, [('$this->xb5c_', '-1.0')]), (0, [('$this->xb5c_', '1.0')])],
        '+0xB5C at most 0 / above 0 (0x0000B0C0)'),
    ent('AFB8 AbortSeekCover', 'AbortSeekCover', 2,
        [(1, [('$this->xb8c_', '1.0'), ('$this->xb90_', '0.0')]),
         (0, [('$this->xb8c_', '0.0'), ('$this->xb90_', '1.0')])],
        '+0xB8C at least / under +0xB90 (0x0000AFC0)'),
    ent('B1D4 AbortShieldCharge', 'AbortShieldCharge', 2,
        [(1, [(C64, C64 + ' | 0x04')]), (0, [(C64, C64 + ' & ~0x04')])],
        'the +0xC64 0x04 bit set / clear (0x0000B1D8)'),
    ent('B1C8 IsAggressive', 'IsAggressive', 2,
        [(1, [(C64, C64 + ' | 0x40')]), (0, [(C64, C64 + ' & ~0x40')])],
        'the +0xC64 0x40 bit set / clear (0x0000B1CC)'),
    ent('ADD4 Lurk', 'Lurk', 2, [(1, [ON_ACT(TGT, INVALID)])],
        'on an activate the target id kInvalidUniqueId (the beqlr at 0x0000ADEC)'),
    ent('AE04 Alert', 'Alert', 3,
        [(1, [('$this->mBodyController.mPtr->mLocomotionType',
               '$rdx == 1 ? 1 : $this->mBodyController.mPtr->mLocomotionType')])],
        'on an update the relaxed locomotion (the cmpwi at 0x0000AE70)'),
    ent('A8E8 SetJumpDest', 'SetJumpDest', 2, [(1, [ON_ACT('$this->xbd0_.value', SELF)])],
        'the jump point id valid (the own id, an actor: 0x0000A92C)'),
    ent('A7A8 SetCoverDest', 'SetCoverDest', 2, [(1, [ON_ACT(C66, C66 + ' | 0x40')])],
        'the +0xC66 0x40 bit already set (0x0000A808)'),
    ent('8B70 JumpBack', 'JumpBack', 4, [(1, [ON_ACT(TGT, INVALID)]), (3, [ON_ACT(TGT, INVALID)])],
        'on an activate the target gone (fn_9_3CBC fails, 0x0000BBC4 falls through)'),
    ent('19D8 JoinTeam', 'x19D8_JoinTeam', 6,
        [(1, [(TEAM, INVALID), (F0, F0 + ' | 0x02'), (F1, F1 + ' | 0x40')]),
         (3, [(TEAM, INVALID), (F0, F0 + ' | 0x02'), (F1, F1 + ' & ~0x40')])],
        'no team with the block flags 0x02 and 0x40 (0x00001A20) / 0x02 alone (0x00001A14)'),
    # The first call is written back after the body's QuitTeam: the id stays while the commando
    # is out, so the next call takes the "not part of the team" arm (0x00001998).
    ent('1930 LeaveTeam', 'x1930_LeaveTeam', 5,
        [(1, [(TEAM, TEAM)]), (3, [(TEAM, SELF)])],
        'the team id kept over the quit (the next call: 0x00001998); the own id, not a team '
        '(0x0000197C)'),
]


# The avoidance's other-area arm (0x00001AFC): no commando area has a listening AI of another
# area loaded beside it, so the actor's own area id is written for the call.
FORCE_RET.append(ent('1A9C AvoidActors', 'x1A9C_AvoidActors', 3,
                     [(1, [('$this->mAreaId.value', '$this->mAreaId.value + 1')])],
                     'the own area id moved by one: every other actor is in another area'))


# The cover triggers' cover arm: the commando lets its cover point go again before the driver's
# trigcall (xbd2_ is kInvalidUniqueId at every call), so the Swamp's first cover point u05c8
# (05_swamp_timber!'s `objs cover`; in another area the id is no cover point and the null arm runs)
# is written for every other call.
COVER = '0x05c8'
FORCE_RET += [
    ent('B168 ShouldCrouch', 'ShouldCrouch', 2, [(1, [('$this->xbd2_.value', COVER)])],
        'the cover point u05c8 held (0x0000B184 falls through to the ShouldCrouch call)'),
    ent('B114 ShouldWallHang', 'ShouldWallHang', 2, [(1, [('$this->xbd2_.value', COVER)])],
        'the cover point u05c8 held (0x0000B134 falls through to the ShouldWallHang call)'),
]


def ret(label, fn, exprs=()):
    return (label, 'CCommandoPirate::%s(' % fn, list(exprs), 'ret max=20000')


SPEC = [
    ret('0xBAB4 HeardShot', 'HeardShot'),
    ret('0xB0B4 ShouldCoverAttack', 'ShouldCoverAttack'),
    ret('0xAFB8 AbortSeekCover', 'AbortSeekCover'),
    ret('0xB1D4 AbortShieldCharge', 'AbortShieldCharge'),
    ret('0xB1C8 IsAggressive', 'IsAggressive'),
    ret('0xB168 ShouldCrouch', 'ShouldCrouch', ['$this->xbd2_.value']),
    ret('0xB114 ShouldWallHang', 'ShouldWallHang', ['$this->xbd2_.value']),
]
