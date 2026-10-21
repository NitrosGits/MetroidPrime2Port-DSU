# DarkSamus phazon beam (fn_10_6118, 58C0, 8178, 5974) and PhazonAttack (fn_10_81D4), a forcing run
# on the coverage build. The host never fires the attack animation's user events, so the console's
# `uevent` delivers them (27 EventStart makes the beam, 18 EndAction counts a shot, 12 Landing,
# 28 EventStop releases it), and `statecall` runs the state (darksamus_phazonbeam.con). The arms
# the fight cannot pick are written at the entry of UpdatePhazonBeam / PhazonAttack, keyed by the
# statecall's dt:
#   0.0123 the actor onto the player's x/y and its z = 1 + 0xF58 (the offset to the player is zero)
#   0.0124 the actor turned 180 degrees (the angle arms)     0.0126 0xF58 + 500 (the rays miss)
#   0.0127 0xF5C bit 0x40 (PhazonAttack)                       0.0128 the actor's forward straight up
#   0.0136/0.0137/0.0138 the player straight ahead at 40 / 3 / the reach   0.0139 a NaN gun locator
#   0.0140 one beam ray, cast too short to hit   0.0141 PhazonAttack's CanIssueCommand answers yes
#   0.0135 0xF60 a handle no sound has (PhazonAttack's emitter arms)
#   0.0129 0xF58 = 0                                         0.0130 the forward put back
# and the beam's cylinder radius at each of its PointInside calls ($pm the mode, $cyc the call; not $pc, gdb's program counter):
#   0.0125 outside, outside, inside (the aim position)   0.0131 outside, inside (the eye)
#   0.0132 inside (the position)                          0.0133 outside x3
#   0.0134 = 0.0124 + 0.0132 (inside while facing away)
# Run: BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darksamus_phazonbeam.py <out>
#      "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_phazonbeam.con --fast --frames 800
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module DarkSamus
CLASS = 'CDarkSamus'


def key(v):
    return '($xmm0.v4_float[0] > %.5f && $xmm0.v4_float[0] < %.5f)' % (v - 0.00004, v + 0.00004)


P = '((CStateManager*)$rsi)->mPlayers[0]'
T = '$this->mTransform'
# GetTranslation() is the actor's mPosition (guest 0x54), not its transform's column.
Q = '$this->mPosition'
ZERO = ['%s.mX = %s ? %s->mPosition.mX : %s.mX' % (Q, key(0.0123), P, Q),
        '%s.mY = %s ? %s->mPosition.mY : %s.mY' % (Q, key(0.0123), P, Q),
        '%s.mZ = %s ? 1 + $this->mGuestF58 : %s.mZ' % (Q, key(0.0123), Q)]
FLIPK = '(%s || %s)' % (key(0.0124), key(0.0134))
FLIP = ['%s.%s = %s ? -%s.%s : %s.%s' % (T, m, FLIPK, T, m, T, m)
        for m in ('m00', 'm01', 'm10', 'm11')]
# The player straight ahead (the actor moved back along its forward from the player, the beam's
# height at the actor's z): 0.0136 at 40, 0.0137 at 3, 0.0138 at the beam's reach 0xF10.
FACE = []
for v, d in ((0.0136, '40'), (0.0137, '3'), (0.0138, '$this->mGuestF10')):
    FACE += ['%s.mX = %s ? %s->mPosition.mX - %s * %s.m01 : %s.mX' % (Q, key(v), P, d, T, Q),
             '%s.mY = %s ? %s->mPosition.mY - %s * %s.m11 : %s.mY' % (Q, key(v), P, d, T, Q),
             '$this->mGuestF58 = %s ? %s.mZ - 1 : $this->mGuestF58' % (key(v), Q)]
# 0.0139: the gun locator's z NaN, so the beam's LookAt (a unit forward otherwise, in all its
# arms) answers a NaN forward 0xF14; 0.0129 puts the z back.
L = '$this->mGunLctrXf.m23'
FACE += ['$lz = %s ? %s : ($_isvoid($lz) ? 0 : $lz)' % (key(0.0139), L),
         '%s = %s ? 0.0 / 0.0 : %s' % (L, key(0.0139), L),
         '%s = (%s && $lz != 0) ? $lz : %s' % (L, key(0.0129), L)]
HIGH = ['$this->mGuestF58 = %s ? %s.m23 + 500 : $this->mGuestF58' % (key(0.0126), T)]
# 0.0140: one ray (0xF50 = 0.07 before the dt is added) cast 0.001 long, so it misses
# (CGameCollision::RayStaticIntersection reads length 0 as unbounded, so not 0).
RAY = ['$this->mGuestF50 = %s ? 0.07 : $this->mGuestF50' % key(0.0140),
       '$rm = %s ? 1 : 0' % key(0.0140)]
MODE = ['$cyc = 0',
        '$pm = %s ? 1 : %s ? 3 : (%s || %s) ? 4 : %s ? 2 : 0' % (
            key(0.0125), key(0.0131), key(0.0132), key(0.0134), key(0.0133))]
C = '((CCylinder*)$rdi)->mRadius'
CYL = ['$cyc = $cyc + 1',
       '%s = $pm == 1 ? ($cyc < 3 ? 0 : 1000) : $pm == 2 ? 0 : $pm == 3 ? ($cyc < 2 ? 0 : 1000) : '
       '$pm == 4 ? 1000 : %s' % (C, C),
       '$pm = ($cyc >= 3 || $pm == 4 || ($pm == 3 && $cyc == 2)) ? 0 : $pm']
UPK = ['$ci = %s ? 1 : 0' % key(0.0141),
       '$this->mGuestF5C40 = %s ? 1 : $this->mGuestF5C40' % key(0.0127),
       '$this->mGuestF60.mID = %s ? 0x7fffffff : $this->mGuestF60.mID' % key(0.0135)]
for m, v in (('m01', 0), ('m11', 0), ('m21', 1)):
    UPK.append('$s%s = %s ? %s.%s : ($_isvoid($s%s) ? 0 : $s%s)' % (m, key(0.0128), T, m, m, m))
    UPK.append('%s.%s = %s ? %d : %s.%s' % (T, m, key(0.0128), v, T, m))
for m in ('m01', 'm11', 'm21'):
    UPK.append('%s.%s = %s ? $s%s : %s.%s' % (T, m, key(0.0130), m, T, m))
UPK.append('$this->mGuestF58 = %s ? 0 : $this->mGuestF58' % key(0.0129))
FORCE = [
    ('5974 entry', 'CDarkSamus::UpdatePhazonBeam(', [], ZERO + FACE + FLIP + HIGH + RAY + MODE, 0,
     'keyed writes: zero offset, turn, high beam, cylinder mode'),
    ('8178 sfx', 'CDarkSamus::StartPhazonBeamSfx(', [], [
        '$this->mData.mSound = $this->mData.mSound == -1 ? 1 : $this->mData.mSound'], 0,
     'the sound 1 when the data has none (the -1 arm is darksamus_phazonbeam_invalid.py)'),
    ('5974 cylinder', 'CCylinder::PointInside(', [], CYL, 0, 'the beam cylinder radius by mode'),
    ('5974 ray miss', 'CStateManager::RayStaticIntersection(', [],
     ['$xmm0.v4_float[0] = $rm ? 0.001 : $xmm0.v4_float[0]', '$rm = 0'], 0,
     'the beam ray 0.001 long (dt 0.0140)'),
    ('81D4 issue', 'CAnimationState::CanIssueCommand(', [],
     ['((CAnimationState*)$rdi)->mState = $ci ? 1 : ((CAnimationState*)$rdi)->mState',
      '$rdx = $ci ? 0x7f : $rdx', '$ci = 0'], 0,
     'dt 0.0141: Ready and the body not in the asked state (the true arm)'),
    ('81D4 entry', 'CDarkSamus::PhazonAttack(', [], UPK, 0,
     'keyed writes: 0xF5C bit 0x40, forward up / back'),
]
