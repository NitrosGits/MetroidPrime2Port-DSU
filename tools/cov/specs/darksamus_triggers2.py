# DarkSamus triggers, part two (fn_10_16B5C, 16B54, 112AC, 11760, 117A0, FD30, 10698, F800, and the
# wrappers 17CB8, 142CC, 10634, 112E8), a forcing run on the coverage build: the console's
# `trigcall <id> <trigger> <arg>` evaluates each (darksamus_triggers2.con), and the arms the fight
# cannot pick are written at the trigger's entry, keyed by the argument (the CTriggerData at $rdx)
# less 100:
#   30 every member saved                        99 every member put back (each driver line ends so)
#   31 0xA40 = 1, 32 0xA40 = 0 (BreakNormalMissile) 33 0xA80 = 1, 34 0xA80 = 0 (BreakSuperMissile)
#   35 0xC0C = 0, 36 0xC0C = now, 37 0xC0C = now - 1 (WasHeavyHit: no hit, recent, old)
#   38 0x984 = 0x100000, 39 0x984 = 0 (PickedFreezeBeam)
#   40 0xDEC = 1 and 0x984 = 0x200000, 41 0xDEC = 0 and 0x984 = 0x40 (PickedNormalMissile)
#   42 0xD58 = 1, 43 0xD58 = 0, 44 0xD58 = 1 and not alive, 45 alive again (ShouldGetPissedOff)
#   46 0x980 = 0x8000; 47 0x980 = 0, history [0x8000]; 48 history [0], damage 2 > 1;
#   49 no history, damage 0 (TooMuchDamage)
#   50 still 2 s; 51 still 0, wall hit now; 52 wall hit now - 1 (WallCollision)
# OutOfMeleeRange's answers come from `place` (the player 60 and 1 units away).
# Run: BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darksamus_triggers2.py <out>
#      "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_triggers2.con --fast --frames 800
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module DarkSamus
# The saved values are $dsk_* convenience variables: a bare name like $al is an x86 register (AL).
CLASS = 'CDarkSamus'


def k(v):
    return '(*(float*)$rdx == %d)' % (100 + v)


NOW = '$this->mElapsedTime'


def keep(var, member):
    return '%s = %s ? %s : ($_isvoid(%s) ? 0 : %s)' % (var, k(30), member, var, var)


def put(member, var, *pairs):
    expr = member
    for v, val in reversed(pairs + ((99, var),)):
        expr = '%s ? %s : (%s)' % (k(v), val, expr)
    return '%s = %s' % (member, expr)


MEMBERS = [('$dsk_a40', '$this->mGuestA40'), ('$dsk_a80', '$this->mGuestA80'), ('$dsk_c0c', '$this->mGuestC0C'),
           ('$dsk_pk', '$this->mPickedAttack'), ('$dsk_fo', '$this->mFadeOut'), ('$dsk_po', '$this->mPissedOff'),
           ('$dsk_al', '$this->mAlive'), ('$dsk_ca', '$this->mCurrentAttack'),
           ('$dsk_hc', '$this->mAttackHistoryCount'), ('$dsk_h0', '$this->mAttackHistory[0]'),
           ('$dsk_ad', '$this->mAccumulatedDamage'), ('$dsk_dm', '$this->mDamageThreshold'),
           ('$dsk_st', '$this->mStillTime'), ('$dsk_lw', '$this->mLastWallHitTime')]
V = dict((m, v) for v, m in MEMBERS)


def p(member, *pairs):
    return put(member, V[member], *pairs)


SETS = [keep(v, m) for v, m in MEMBERS] + [
    p('$this->mGuestA40', (31, 1), (32, 0)),
    p('$this->mGuestA80', (33, 1), (34, 0)),
    p('$this->mGuestC0C', (35, 0), (36, NOW), (37, NOW + ' - 1')),
    p('$this->mPickedAttack', (38, 0x100000), (39, 0), (40, 0x200000), (41, 0x40)),
    p('$this->mFadeOut', (40, 1), (41, 0)),
    p('$this->mPissedOff', (42, 1), (43, 0), (44, 1)),
    p('$this->mAlive', (44, 0), (45, '$dsk_al')),
    p('$this->mCurrentAttack', (46, 0x8000), (47, 0), (48, 0), (49, 0)),
    p('$this->mAttackHistoryCount', (47, 1), (48, 1), (49, 0)),
    p('$this->mAttackHistory[0]', (47, 0x8000), (48, 0)),
    p('$this->mAccumulatedDamage', (48, 2), (49, 0)),
    p('$this->mDamageThreshold', (48, 1), (49, 1)),
    p('$this->mStillTime', (50, 2), (51, 0), (52, 0)),
    p('$this->mLastWallHitTime', (51, NOW), (52, NOW + ' - 1'))]
A = ['*(float*)$rdx']
FORCE = [
    ('16B5C entry', 'CDarkSamus::BreakNormalMissile(', A, SETS, 0, 'keys 30..32'),
    ('16B54 entry', 'CDarkSamus::BreakSuperMissile(', A, SETS, 0, 'keys 33, 34'),
    ('112AC entry', 'CDarkSamus::WasHeavyHit(', A, SETS, 0, 'keys 35..37'),
    ('11760 entry', 'CDarkSamus::PickedFreezeBeam(', A, SETS, 0, 'keys 38, 39'),
    ('117A0 entry', 'CDarkSamus::PickedNormalMissile(', A, SETS, 0, 'keys 40, 41'),
    ('FD30 entry|ret', 'CDarkSamus::ShouldGetPissedOff(', A + ['$this->mAlive', '$this->mPissedOff'], SETS, 0,
     'keys 42..45'),
    ('10698 entry', 'CDarkSamus::TooMuchDamage(', A, SETS, 0, 'keys 46..49'),
    ('F800 entry', 'CDarkSamus::WallCollision(', A, SETS, 0, 'keys 50..52, 99'),
]
