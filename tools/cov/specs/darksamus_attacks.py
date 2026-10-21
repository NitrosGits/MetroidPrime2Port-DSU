# DarkSamus anim-event attack callees (dsfsm2 slice 8): fn_10_77B8, 13998, 13AB4, 1353C, 133D0,
# 9164 and ProjectileInfo's missile arms. A forcing run on the coverage build; the console's
# `uevent <id> <type> <dt>` (darksamus_attacks.con) delivers each event and the dt less 100 is the
# key, read at the handler's entry:
#   30 every member saved                      99 every member put back (each driver line ends so)
#   1, 2, 4 the attack 1, 2, 4    5 0x40    6 0x80    7 0x100    8 0x20000    9 0x100000    19 none
#   20 the player reads morphed (its 0x38C, kMS_Morphed) until 99
#   21 the actor 1000 units up and 100000 across until 99 (the melee pulse's floor ray misses: the
#      host's ray finds the floor 1000 units down, out of its 20-unit reach)
#   22 the melee record's hit byte cleared (the next DamageOn hits)
#   23 the melee 'PART' tokens and the freeze beam's token read absent until 99
#   24 iceSpreadSound reads -1 (no sound) until 99
# Run: BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darksamus_attacks.py <out>
#      "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_attacks.con --fast --frames 750
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module DarkSamus
CLASS = 'CDarkSamus'


def k(v):
    return '($xmm0.v4_float[0] == %d)' % (100 + v)


PLAYER = '((CStateManager*)$rsi)->mPlayers[0]->mMorphBallState'
MEMBERS = [('$dsa_ca', '$this->mCurrentAttack'), ('$dsa_mo', PLAYER),
           ('$dsa_z', '$this->mTransform.m23'), ('$dsa_x', '$this->mTransform.m03'), ('$dsa_hit', '$this->mMeleeContact.mHit'),
           ('$dsa_v1', '$this->mMeleeAttackFXToken.m_valid'),
           ('$dsa_v2', '$this->mMeleePartToken.m_valid'),
           ('$dsa_v3', '$this->mFreezeBeamToken.m_valid'),
           ('$dsa_snd', '$this->mData.mIceSpreadSound')]
ATTACKS = [(1, 1), (2, 2), (4, 4), (5, 0x40), (6, 0x80), (7, 0x100), (8, 0x20000), (9, 0x100000),
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
    put('$this->mMeleeContact.mHit', V['$this->mMeleeContact.mHit'], (22, 0)),
    put('$this->mMeleeAttackFXToken.m_valid', '$dsa_v1', (23, 0)),
    put('$this->mMeleePartToken.m_valid', '$dsa_v2', (23, 0)),
    put('$this->mFreezeBeamToken.m_valid', '$dsa_v3', (23, 0)),
    put('$this->mData.mIceSpreadSound', '$dsa_snd', (24, -1))]
FORCE = [
    ('1BEFC entry', 'CDarkSamus::DoUserAnimEvent(', ['$xmm0.v4_float[0]', '(int)$rcx', '$this->mCurrentAttack', '(int)$this->mTransform.m23',
      '(int)((CStateManager*)$rsi)->mPlayers[0]->mTransform.m23'],
     SETS, 0, 'the attack and the forced members by the dt key'),
]
