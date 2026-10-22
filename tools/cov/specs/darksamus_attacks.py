# DarkSamus anim-event attack callees (dsfsm2 slice 8): fn_10_77B8, 13998, 13AB4, 1353C, 133D0,
# 9164 and ProjectileInfo's missile arms. A forcing run on the coverage build; the console's
# `uevent <id> <type> <dt>` (darksamus_attacks.con) delivers each event and the dt less 100 is the
# key, read at the handler's entry:
#   30 every member saved                      99 every member put back (each driver line ends so)
#   1, 2, 4 the attack 1, 2, 4    5 0x40    6 0x80    7 0x100    8 0x20000    9 0x100000    19 none
#   20 the player reads morphed (its 0x38C, kMS_Morphed) until 99
#   21 the actor 1000 units up and 100000 across until 99, its transform and its position 0x54 (the
#      melee pulse's floor ray starts at GetTranslation, the position, and misses)
#   22 the melee record's hit byte cleared (the next DamageOn hits)
#   23 the melee 'PART' tokens and the freeze beam's token read absent until 99
#   24 iceSpreadSound reads -1 (no sound) until 99
# Slice 9, the phazon volley (fn_10_5840 / 50B8 / 4C40):
#   10 the attack 0x800000    11 a shot (0xFB8 0, 0xFB4 3, 0xFCC -1)    12 the super missile (0xFB8 1,
#      0xFB4 2)    13 no missiles left    14 0xFB8 2 (neither)    15 a shot with the clock 0xFCC at 5
#   16 the player orbits the actor until 99    17 the dummy list 0x934 full (12 x 0xFFFE) until 99
#   18 the actor's forward flat (m01 = m11 = 0) until 99    25 the player at the actor's x, y until 99
#   26 the shot's side vector zeroed (loc: at its CanBeNormalized) until 99
#   27 the shot info's token uncached and unlocked until 99    28 the shot info absent until 99
# Run: BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darksamus_attacks.py <out>
#      "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_attacks.con --fast --frames 750
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module DarkSamus
CLASS = 'CDarkSamus'


def k(v):
    return '($xmm0.v4_float[0] == %d)' % (100 + v)


PLAYER = '((CStateManager*)$rsi)->mPlayers[0]->mMorphBallState'
MEMBERS = [('$dsa_ca', '$this->mCurrentAttack'), ('$dsa_mo', PLAYER),
           ('$dsa_z', '$this->mTransform.m23'), ('$dsa_x', '$this->mTransform.m03'),
           ('$dsa_pz', '$this->mPosition.mZ'), ('$dsa_px0', '$this->mPosition.mX'), ('$dsa_hit', '$this->mMeleeContact.mHit'),
           ('$dsa_v1', '$this->mMeleeAttackFXToken.m_valid'),
           ('$dsa_v2', '$this->mMeleePartToken.m_valid'),
           ('$dsa_v3', '$this->mFreezeBeamToken.m_valid'),
           ('$dsa_snd', '$this->mData.mIceSpreadSound')]
PL = '((CStateManager*)$rsi)->mPlayers[0]'
SHOT = '((CProjectileInfo*)&$this->mPhazonShotInfo)->mWeaponDescription'
MEMBERS += [('$dsa_fb4', '$this->mGuestFB4'), ('$dsa_fb8', '$this->mGuestFB8'),
            ('$dsa_fcc', '$this->mGuestFCC'), ('$dsa_orb', PL + '->mOrbitTargetId.value'),
            ('$dsa_g930', '$this->mGuest930'), ('$dsa_g934', '$this->mGuest934'),
            ('$dsa_m01', '$this->mTransform.m01'), ('$dsa_m11', '$this->mTransform.m11'),
            ('$dsa_px', PL + '->mTransform.m03'), ('$dsa_py', PL + '->mTransform.m13'),
            ('$dsa_it', SHOT + '.mItem'), ('$dsa_lk', SHOT + '.mLockHeld'),
            ('$dsa_sv', '$this->mPhazonShotInfo.m_valid')]
ATTACKS = [(10, 0x800000), (1, 1), (2, 2), (4, 4), (5, 0x40), (6, 0x80), (7, 0x100), (8, 0x20000), (9, 0x100000),
           (19, 0)]


def keep(var, member):
    return '%s = %s ? %s : ($_isvoid(%s) ? 0 : %s)' % (var, k(30), member, var, var)


def put(member, var, *pairs):
    expr = member
    for v, val in reversed(pairs + ((99, var),)):
        expr = '%s ? %s : (%s)' % (k(v), val, expr)
    return '%s = %s' % (member, expr)


V = dict((m, v) for v, m in MEMBERS)
SETS = [keep(v, m) for v, m in MEMBERS] + [
    put('$this->mCurrentAttack', V['$this->mCurrentAttack'], *ATTACKS),
    put(PLAYER, V[PLAYER], (20, 1)),
    put('$this->mTransform.m23', V['$this->mTransform.m23'], (21, '$this->mTransform.m23 + 1000')),
    put('$this->mTransform.m03', V['$this->mTransform.m03'], (21, '$this->mTransform.m03 + 100000')),
    put('$this->mPosition.mZ', '$dsa_pz', (21, '$this->mPosition.mZ + 1000')),
    put('$this->mPosition.mX', '$dsa_px0', (21, '$this->mPosition.mX + 100000')),
    put('$this->mMeleeContact.mHit', V['$this->mMeleeContact.mHit'], (22, 0)),
    put('$this->mMeleeAttackFXToken.m_valid', '$dsa_v1', (23, 0)),
    put('$this->mMeleePartToken.m_valid', '$dsa_v2', (23, 0)),
    put('$this->mFreezeBeamToken.m_valid', '$dsa_v3', (23, 0)),
    put('$this->mData.mIceSpreadSound', '$dsa_snd', (24, -1)),
    put('$this->mGuestFB4', '$dsa_fb4', (11, 3), (12, 2), (13, 0), (14, 1), (15, 1)),
    put('$this->mGuestFB8', '$dsa_fb8', (11, 0), (12, 1), (14, 2), (15, 0)),
    put('$this->mGuestFCC', '$dsa_fcc', (11, -1), (15, 5)),
    put(PL + '->mOrbitTargetId.value', '$dsa_orb', (16, '$this->mUniqueId.value')),
    put('$this->mGuest930', '$dsa_g930', (17, 12))] + [
    put('$this->mGuest934[%d]' % i, '$dsa_g934[%d]' % i, (17, 0xFFFE)) for i in range(18)] + [
    put('$this->mTransform.m01', '$dsa_m01', (18, 0)),
    put('$this->mTransform.m11', '$dsa_m11', (18, 0)),
    put(PL + '->mTransform.m03', '$dsa_px', (25, '$this->mPosition.mX')),
    put(PL + '->mTransform.m13', '$dsa_py', (25, '$this->mPosition.mY')),
    put(SHOT + '.mItem', '$dsa_it', (27, 0)),
    put(SHOT + '.mLockHeld', '$dsa_lk', (27, 0)),
    put('$this->mPhazonShotInfo.m_valid', '$dsa_sv', (28, 0)),
    '$dsa_zs = %s ? 1 : (%s ? 0 : ($_isvoid($dsa_zs) ? 0 : $dsa_zs))' % (k(26), k(99))]
import os
import gdb
SRC = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(gdb.current_progspace().filename))),
                   'src/MetroidPrime/Enemies/PortCDarkSamusHelpers.cpp')
SIDE = 1 + [i for i, l in enumerate(open(SRC)) if 'if (side.CanBeNormalized() == true)' in l][0]
FORCE = [
    ('1BEFC entry', 'CDarkSamus::DoUserAnimEvent(', ['$xmm0.v4_float[0]', '(int)$rcx', '$this->mCurrentAttack', '(int)$this->mTransform.m23',
      '(int)((CStateManager*)$rsi)->mPlayers[0]->mTransform.m23'],
     SETS, 0, 'the attack and the forced members by the dt key'),
    ('50B8 side zeroed', 'loc:PortCDarkSamusHelpers.cpp:%d' % SIDE, ['$dsa_zs', 'side.mX', 'side.mY'],
     ['side.mX = $dsa_zs ? 0 : side.mX', 'side.mY = $dsa_zs ? 0 : side.mY'], 0,
     'the random offset zeroed by key 26'),
]
