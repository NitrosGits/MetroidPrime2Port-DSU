# One-call forcing spec (COV_GDB_SCRIPT=tools/cov/force_ret.py) for sp13: every forced arm of 4174
# SpawnGrenade and of the 4030 GetGrenadeAimPoint / 3D7C SolveGrenadeLaunch it inlines in the
# coverage build, in one run (the phases of spacepirate_force7.py and spacepirate_force8.py without
# their slow ctor / AggressionCheck entries). Every write is restored when the forced call returns.
# The driver (07_sand_alamoportalobservation L3) switches the layer, wakes the two launcher
# troopers 002a00f3/002a00f4 and fires grenades with `uevent <id> 0` (kUE_Projectile).
# Run (2026-10-23):
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov SP_STEPS='layer 3; wait 6;
#   send {002a00f3} Activate; send {002a00f4} Activate; wait 2; press x; wait 3;
#   uevent {002a00f3} 0; wait 1; uevent {002a00f4} 0; wait 1; ...' tools/cov/force.sh <this> <out>
#   42b935e4,07_sand_alamoportalobservation --driver
#   'python3 tools/drivers/spacepirate_drive.py {port} - 0' --frames 16000 --fast
import gdb

CLASS = 'CSpacePirate'
gdb.execute('set $sp_pl = 0')
PL = '((CStateManager*)$rsi)->mPlayers[0]'
SET_PL = ('!$sp_pl', '(long)' + PL)
TARGET_PLAYER = ('$this->mAttackTarget.value', PL + '->mUniqueId.value')
NAN = '(float)(0.0/0.0)'
FORCE_RET = [
    ('4174 SpawnGrenade (+4030, 3D7C)', 'CSpacePirate::SpawnGrenade(', 7, [
        (1, [('$this->mAttackTarget.value', '$this->mUniqueId.value')]),
        (2, [TARGET_PLAYER, SET_PL, ('((CPlayer*)$sp_pl)->mMorphBallState', '1'),
             ('$this->mData.mGrenadeNumBounces', '0')]),
        (3, [TARGET_PLAYER, ('$this->mData.mGrenadeMinLaunchSpeed', '0.1'),
             ('$this->mData.mGrenadeMaxLaunchSpeed', '0.2')]),
        (4, [TARGET_PLAYER, ('$this->mData.mGrenadeMinLaunchSpeed', '200.0'),
             ('$this->mData.mGrenadeMaxLaunchSpeed', '201.0')]),
        (5, [TARGET_PLAYER, SET_PL, ('((CPlayer*)$sp_pl)->mPosition.mX', NAN),
             ('((CPlayer*)$sp_pl)->mPosition.mY', NAN)]),
        (6, [('$this->mAttackTarget.value', '65535')]),
        (0, [TARGET_PLAYER, SET_PL, ('((CPlayer*)$sp_pl)->mMorphBallState', '0')]),
    ], '1: the pirate targets itself (not a player); 2: morphed player, no bounces; 3/4: launch '
       'speed range below/above every candidate; 5: the player at a NaN x/y (the flat aim cannot be '
       'normalised, gun forward instead); 6: no target (65535); 0: target = the unmorphed player (this '
       'area\'s harness player stays morphed: SPEC below reads 1 on every natural call, `press x` or not)'),
]
# Counted after the forcing (gdb_cov.py): the player's morph state and the target each call sees.
SPEC = [
    ('4174 player morph state', 'CSpacePirate::SpawnGrenade(',
     ['((CStateManager*)$rsi)->mPlayers[0]->mMorphBallState', '$this->mAttackTarget.value'], 'max=2000'),
]
