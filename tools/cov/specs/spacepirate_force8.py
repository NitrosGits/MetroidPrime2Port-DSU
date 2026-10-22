# One-call forcing spec (COV_GDB_SCRIPT=tools/cov/force_ret.py) for the sp21 bodies: the ctor
# F49C and 4174 SpawnGrenade's flat-aim arm. Every write is restored when the forced call returns.
# The ctor's arguments past r9 are on the stack at its entry (x86-64 SysV): actParms $rsp+8,
# pInfo $rsp+16, data $rsp+24. Those are the loader's per-object temporaries, so the writes are
# not restored ('!': $rsp differs at the return). C1 and C2 share one address, so each ctor call
# hits two breakpoints with the same phase; the writes are idempotent.
# The driver (07_sand_alamoportalobservation L3) fires grenades with `uevent <trooper> 0`.
# Run (2026-10-22):
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov SP_STEPS='press x; wait 3;
#   uevent {002a00f3} 0; wait 1; uevent {002a00f4} 0; ...' tools/cov/force.sh <this> <out>
#   42b935e4,07_sand_alamoportalobservation --driver
#   'python3 tools/drivers/spacepirate_drive.py {port} 3 0' --frames 9000
import gdb

CLASS = 'CSpacePirate'
gdb.execute('set $sp_ctor = 0')  # set by a forced ctor: its HasBodyState(kAS_AdditiveAim) reads false
gdb.execute('set $sp_pl = 0')
ACT = '((CActorParameters*)*(long*)($rsp+8))'
INFO = '((CPatternedInfo*)*(long*)($rsp+16))'
DATA = '((CSpacePirateData*)*(long*)($rsp+24))'
PL = '((CStateManager*)$rsi)->mPlayers[0]'
TARGET_PLAYER = ('$this->mAttackTarget.value', PL + '->mUniqueId.value')
FORCE_RET = [
    ('F49C ctor', 'CSpacePirate::CSpacePirate(', 3, [
        (1, [('!' + DATA + '->mFlags', DATA + '->mFlags | 0x10020'),
             ('!' + DATA + '->mProjectile', '0xffffffff')]),
        (2, [('!' + INFO + '->mEchoParameters.mIsEchoEmitter', '0'),
             ('!' + ACT + '->mLighting.mMakeLights', '0'), ('!$sp_ctor', '1')]),
    ], '1: onlyAttackInRange (0x20) + floatingCorpse (0x10000) data flags, no projectile; 2: pInfo '
       'not an echo emitter (every placement is one: the PatternedAI default), actParms without '
       'lights, and the body reads as having no AdditiveAim state (melee)'),
    ('F49C HasBodyState(0x16) false', 'CBodyController::HasBodyState(', 1, [
        (0, [('!$rsi', '$sp_ctor && $rsi == 22 ? 99 : $rsi'),
             ('!$sp_ctor', '$rsi == 99 ? 0 : $sp_ctor')]),
    ], 'only the call from a phase-2 ctor: state 22 is asked as 99 (no such state)'),
    ('4174 SpawnGrenade flat aim', 'CSpacePirate::SpawnGrenade(', 3, [
        (1, [TARGET_PLAYER, ('!$sp_pl', '(long)' + PL),
             ('((CPlayer*)$sp_pl)->mPosition.mX', '(float)(0.0/0.0)'),
             ('((CPlayer*)$sp_pl)->mPosition.mY', '(float)(0.0/0.0)')]),
        (2, [('$this->mAttackTarget.value', '65535')]),
        (0, [TARGET_PLAYER]),
    ], '1: target = the player with a NaN x/y mPosition (GetTranslation) for this call, so the flat '
       'aim cannot be normalised (gun forward instead); 2: no target; 0: target = the player'),
]
