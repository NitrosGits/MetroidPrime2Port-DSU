# One-call forcing spec (COV_GDB_SCRIPT=tools/cov/force_ret.py) for the sp20 bodies: A71C
# ShouldAttack, 3220 ShouldFrenzy, 3D7C SolveGrenadeLaunch, 4030 GetGrenadeAimPoint and 4174
# SpawnGrenade. Every write is restored when the forced call returns, so only that call sees it.
# The driver (07_sand_alamoportalobservation L3) wakes the two launcher troopers 002a00f3/002a00f4,
# fires grenades with `uevent <id> 8` (kUE_Projectile), puts the layer-0 pirates in Attack (their
# shots evaluate ShouldFrenzy) and calls `trigcall <trooper> ShouldAttack`.
# Run (2026-10-22):
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov SP_STEPS=... tools/cov/force.sh
#   <this> <out> 42b935e4,07_sand_alamoportalobservation --driver
#   'python3 tools/drivers/spacepirate_drive.py {port} 3 0' --frames 9000
import gdb

CLASS = 'CSpacePirate'
gdb.execute('set $sp_att = 0')  # the last pirate whose Attack body ran (A71C's dead-attacker force)
gdb.execute('set $sp_pl = 0')   # the player, read at an Attack entry ($rsi is gone at a return)
PL = '((CStateManager*)$rsi)->mPlayers[0]'
SET_PL = ('!$sp_pl', '(long)' + PL)  # written first in a phase, so the restore finds the player
MORPHED = ('((CPlayer*)$sp_pl)->mMorphBallState', '1')
EMPTY_CHARGE = ('CSpacePirate::mChargePlayerList.mCount', '0')
TARGET_PLAYER = ('$this->mAttackTarget.value', PL + '->mUniqueId.value')
FORCE_RET = [
    ('Attack (records $sp_att)', 'CSpacePirate::Attack(', 1, [(0, [('!$sp_att', '$rdi')])],
     'not a forced arm: remembers the attacking pirate'),
    ('A71C ShouldAttack', 'CSpacePirate::ShouldAttack(', 3, [
        (1, [TARGET_PLAYER]),
        (2, [TARGET_PLAYER, ('$this->mAreaId.value', '77')]),
        (0, [TARGET_PLAYER, ('((CSpacePirate*)$sp_att)->mAlive', '0')]),
    ], 'target = the player (1: as is; 2: this pirate in area 77, so every other pirate is in '
       'another area; 0: the last attacking pirate reads as dead)'),
    ('3220 ShouldFrenzy', 'CSpacePirate::ShouldFrenzy(', 6, [
        (1, [('$this->mAttackTarget.value', '65535'), EMPTY_CHARGE]),
        (2, [TARGET_PLAYER, SET_PL, MORPHED]),
        (3, [('$this->xa18_', '1000000.0'), ('$this->xb28_lowHealthFrenzyTimer', '1.0')]),
        (4, [('$this->xa18_', '1000000.0'), ('$this->xb28_lowHealthFrenzyTimer', '0.0')]),
        (5, [('$this->x8f8_24_pendingFrenzyChance', '1')]),
        (0, [('$this->x8f8_24_pendingFrenzyChance', '1'), EMPTY_CHARGE]),
    ], '1: no target, empty charge list; 2: target = the morphed player; 3: low HP, frenzy timer '
       '>= 0.5; 4: low HP, timer 0; 5/0: the frenzy-chance bit (never set in retail: the ctor '
       'clears it, F6E8, and no other store writes 0x8f8 bit 0x80)'),
    ('ADC0 AggressionCheck (fills the charge list)', 'CSpacePirate::AggressionCheck(', 400,
     [(1, [('$this->xa7c_timeNoPlayerLos', '11.0')])],
     'player unseen for 11 s, so the module charge list gets this pirate (3220 charge arm)'),
    # 3D7C SolveGrenadeLaunch and 4030 GetGrenadeAimPoint are static and inlined into
    # SpawnGrenade in the coverage build, so their arms are forced through SpawnGrenade's members.
    ('4174 SpawnGrenade (+4030, 3D7C)', 'CSpacePirate::SpawnGrenade(', 5, [
        (1, [('$this->mAttackTarget.value', '$this->mUniqueId.value')]),
        (2, [TARGET_PLAYER, SET_PL, MORPHED, ('$this->mData.mGrenadeNumBounces', '0')]),
        (3, [TARGET_PLAYER, ('$this->mData.mGrenadeMinLaunchSpeed', '0.1'),
             ('$this->mData.mGrenadeMaxLaunchSpeed', '0.2')]),
        (4, [TARGET_PLAYER, ('$this->mData.mGrenadeMinLaunchSpeed', '200.0'),
             ('$this->mData.mGrenadeMaxLaunchSpeed', '201.0')]),
        (0, [TARGET_PLAYER]),
    ], '1: the pirate targets itself (not a player); 2: morphed player, no bounces; 3/4: launch '
       'speed range below/above every candidate; 0: target = the player (the troopers woken by '
       '`send Activate` have none yet, and SpawnGrenade returns early without one)'),
]
