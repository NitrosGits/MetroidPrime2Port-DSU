# CommandoPirate (module 9) forcing spec, slice cp2: the untaken arms of the cheap triggers and
# states. On the calls an entry's phase picks, the listed members are written at the body's
# entry and written back when that call returns (tools/cov/force_ret.py), so the commando's own
# state is never left forced. SPEC records the answers of the branch-free triggers.
#
# Run (the coverage build; the gcov counters of these runs add to the natural runs'):
#   BUILD=build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh
#     tools/cov/specs/commandopirate_cp2.py <out> 3bfa3eff,05_temple_federationsite_dark --fast
#     --frames 12000 --driver 'python3 tools/drivers/commandopirate_states.py {port}'
#   (and the same with --driver 'python3 tools/drivers/commandopirate_core.py {port} /dev/null x msgs')
CLASS = 'CCommandoPirate'
INVALID = '0xffff'
SELF = '$this->mUniqueId.value'
F0 = '$this->mData.mFlags0'
F1 = '$this->mData.mFlags1'
TGT = '$this->xbca_.value'
C63, C64, C65 = '$this->xc63_', '$this->xc64_', '$this->xc65_'


def ent(label, fn, period, phases, what):
    return (label, 'CCommandoPirate::%s(' % fn, period, phases, what)


FORCE_RET = [
    ent('AA20 SetTargetDest', 'SetTargetDest', 6,
        [(1, [(TGT, INVALID)]), (3, [(C64, C64 + ' | 0x40')])],
        'the target id invalid (the lookup fails, 0x0000AA84); the 0x40 bit of +0xC64 (0x0000AA90)'),
    ent('BD98 ShouldWarpIn', 'ShouldWarpIn', 6,
        [(1, [('$this->x920_', '0')]),
         (3, [('$this->x920_', '-1'), (F1, F1 + ' | 0x80'), (C63, C63 + ' & ~0x20')]),
         (5, [('$this->x920_', '-1'), (F1, F1 + ' | 0x80'), (C63, C63 + ' | 0x20')])],
        'the owner word 0 (0x0000BDA0); -1 with the block flag 0x80 and the +0xC63 0x20 bit clear / set'),
    ent('BCB8 ShouldMeleeAttack', 'ShouldMeleeAttack', 6,
        [(1, [(F0, F0 + ' | 0x02')]), (3, [('$this->xb7c_', '2.0'), (TGT, INVALID)])],
        'the block flag 0x02 (0x0000BCCC); the cooldown over 1 and the target gone (0x0000BD00)'),
    ent('BBC8 ShouldFireEGrenade', 'ShouldFireEGrenade', 8,
        [(1, [(C63, C63 + ' | 0x02'), ("'sEgrenadeObjectCount'", '1')]),
         (3, [(C63, C63 + ' | 0x02'), (F1, F1 + ' | 0x10')]),
         (5, [(C63, C63 + ' | 0x02'), (TGT, INVALID)])],
        'with +0xC63 0x02: a live E-grenade (0x0000BBEC), the block flag 0x10 (0x0000BBF8), the '
        'target gone (0x0000BC30)'),
    ent('BB7C ShouldRetreat', 'ShouldRetreat', 8,
        [(1, [(C63, C63 + ' | 0x40'), (F1, F1 + ' | 0x80')]),
         (3, [(C63, C63 + ' | 0x40'), (F1, F1 + ' & ~0x80'), (F0, F0 + ' | 0x08')]),
         (5, [(C63, C63 + ' | 0x40'), (F1, F1 + ' & ~0x80'), (F0, F0 + ' & ~0x08'),
              ('$this->xbce_.value', SELF)])],
        'the +0xC63 0x40 bit with: the block flag 0x80; the block flag 0x08; neither and a valid '
        '+0xBCE id (the own id)'),
    ent('BB10 ShouldJumpBack', 'ShouldJumpBack', 4, [(1, [(F0, F0 + ' | 0x08')])],
        'the block flag 0x08 (0x0000BB20)'),
    ent('BA20 HasNewTarget', 'HasNewTarget', 6,
        [(1, [(TGT, INVALID)]), (3, [('$this->xbcc_.value', TGT)])],
        'the target gone; the +0xBCC id equal to the target'),
    ent('B9D8 PathShagged', 'PathShagged', 4, [(1, [(C63, C63 + ' | 0x80')])],
        'the +0xC63 0x80 bit (0x0000B9F0)'),
    ent('B990 PathOver', 'PathOver', 4, [(1, [(C65, C65 + ' | 0x80')])],
        'the +0xC65 0x80 bit set (0x0000B9AC)'),
    ent('B76C TooClose', 'TooClose', 4, [(1, [(TGT, INVALID)])], 'the target gone'),
    ent('B690 ShouldDodge', 'ShouldDodge', 4,
        [(1, [('$this->xb64_', '0.0')]), (3, [(C63, C63 + ' | 0x04'), (TGT, INVALID)])],
        'the under-fire distance at +0xB64 under 0.75 (0x0000B730); the target gone (0x0000B6D0)'),
    ent('B660 ShouldArmShield', 'ShouldArmShield', 6,
        [(1, [(C64, C64 + ' | 0x80'), (F1, F1 + ' | 0x20')]),
         (3, [(C64, C64 + ' | 0x81'), (F1, F1 + ' & ~0x20')])],
        'the +0xC64 0x80 bit with the block flag 0x20 (0x0000B674) / with the +0xC64 0x01 bit '
        '(0x0000B67C)'),
    ent('B0F4 ShouldCover', 'ShouldCover', 6,
        [(1, [(F0, F0 + ' | 0x08')]), (3, [(F0, F0 + ' & ~0x08'), (C63, C63 + ' | 0x01')])],
        'the block flag 0x08; the flag clear and the +0xC63 0x01 bit'),
    ent('ADD4 Lurk', 'Lurk', 2, [(0, [(TGT, SELF)])],
        'a valid target id at +0xBCA (the own id) on every other call (0x0000ADEC)'),
    ent('3CBC aim', 'x3CBC_', 8, [(1, [(TGT, INVALID)])], 'the target gone (0x00003CE0)'),
    ent('ABDC FaceTarget', 'FaceTarget', 6,
        [(1, [(TGT, INVALID)]),
         (3, [('$this->xb94_.mX', '$rdx == 1 ? $this->mPosition.mX : $this->xb94_.mX'),
              ('$this->xb94_.mY', '$rdx == 1 ? $this->mPosition.mY : $this->xb94_.mY'),
              ('$this->xb94_.mZ', '$rdx == 1 ? $this->mPosition.mZ : $this->xb94_.mZ')])],
        'the target gone (the activate\'s no-target arm, 0x0000ACB0); on an update the kept aim '
        'position at the actor\'s own (the magnitude test fails, 0x0000AD08)'),
    ent('AE04 Alert', 'Alert', 4,
        [(1, [('!$this->mAnimationState.mState', '$rdx == 1 ? 3 : $this->mAnimationState.mState')]),
         (3, [('$this->mBodyController.mPtr->mLocomotionType', '1')])],
        'on an update the animation state "over" (CanIssueCommand says no, 0x0000AE58); the relaxed '
        'locomotion (the kTT_Two taunt, 0x0000AE90)'),
]


def ret(label, fn, exprs=()):
    return (label, 'CCommandoPirate::%s(' % fn, list(exprs), 'ret max=20000')


SPEC = [
    ret('0xB900 HasAttackPattern', 'HasAttackPattern'),
    ret('0xB1D4 AbortShieldCharge', 'AbortShieldCharge', [C64 + ' & 0x04']),
    ret('0xB0D4 FoundCover', 'FoundCover'),
    ret('0xBAB4 HeardShot', 'HeardShot'),
    ret('0xB1C8 IsAggressive', 'IsAggressive'),
    ret('0xB0B4 ShouldCoverAttack', 'ShouldCoverAttack'),
    ret('0xAFB8 AbortSeekCover', 'AbortSeekCover'),
    ret('0xBCB8 ShouldMeleeAttack', 'ShouldMeleeAttack', [F0 + ' & 2', '$this->xb7c_ > 1', TGT]),
    ret('0xBD98 ShouldWarpIn', 'ShouldWarpIn', ['$this->x920_', F1 + ' & 0x80', C63 + ' & 0x20']),
]
