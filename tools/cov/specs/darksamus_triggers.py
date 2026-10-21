# DarkSamus phazon triggers (fn_10_4ABC, 6A40, 6A60, 6AE8, 6B78, 7B68, 110E0), a forcing run on the
# coverage build: the console's `trigcall <id> <trigger> <arg>` evaluates each once
# (darksamus_triggers.con), and the arms the fight cannot pick are written at the trigger's entry,
# keyed by the argument (the CTriggerData at $rdx) less 100 (the shipped FSM's arguments are small):
#   1 0xFB0 = 1 (ShouldLeaveSphere's timer arm)     2 0xFB0 = 0, the health 50 (its health arm)
#   3 the health 100, 0x1058 = 1, 0x105C = 0 (false) 4 0x1058 = 0 (true)
#   5 the health 50 (ShouldRiseAgain at once)       6 the health 100, 0x1054 = 200 (true)
#   7 0x1054 = 0 (false)
#   8 0xFC4 = 200 (HitDuringEnrage true)            9 0xFC4 = 0 (false)
#   10 0xFB0 = 1 (ReadyForMissiles false)           11 0xFB0 = 0 (true)
#   20 every member put back (saved at 1)
# ShouldEnragedMelee's two answers come from `place` (the player 1 and 60 units away),
# BreakPhazonAttack's from `uevent 12` (Landing sets 0xF5C bit 0x80) after PhazonAttack's Activate.
# Run: BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darksamus_triggers.py <out>
#      "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_triggers.con --fast --frames 800
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module DarkSamus
CLASS = 'CDarkSamus'


def k(v):
    return '(*(float*)$rdx == %d)' % (100 + v)


HP = '$this->mHealthInfo.mHealth'


def keep(var, member):
    return '%s = %s ? %s : ($_isvoid(%s) ? 0 : %s)' % (var, k(1), member, var, var)


def put(member, *pairs):
    expr = member
    for v, val in reversed(pairs):
        expr = '%s ? %s : (%s)' % (k(v), val, expr)
    return '%s = %s' % (member, expr)


SETS = [keep('$hp', HP), keep('$fb', '$this->mGuestFB0'), keep('$mk', '$this->mPhazonHealthMark'),
        keep('$fc', '$this->mGuestFC4'), keep('$v8', '$this->mGuest1058'),
        keep('$vc', '$this->mGuest105C'),
        put('$this->mGuestFB0', (1, 1), (2, 0), (10, 1), (11, 0), (20, '$fb')),
        put(HP, (2, 50), (3, 100), (4, 100), (5, 50), (6, 100), (7, 100), (8, 100), (20, '$hp')),
        put('$this->mGuest1058', (3, 1), (4, 0), (20, '$v8')),
        put('$this->mGuest105C', (3, 0), (4, 0), (20, '$vc')),
        put('$this->mPhazonHealthMark', (6, 200), (7, 0), (20, '$mk')),
        put('$this->mGuestFC4', (8, 200), (9, 0), (20, '$fc'))]
FORCE = [
    ('6A60 entry', 'CDarkSamus::ShouldLeaveSphere(', ['*(float*)$rdx'], SETS, 0, 'keys 1..4'),
    ('6AE8 entry', 'CDarkSamus::ShouldRiseAgain(', ['*(float*)$rdx'], SETS, 0, 'keys 5..7'),
    ('7B68 entry', 'CDarkSamus::HitDuringEnrage(', ['*(float*)$rdx'], SETS, 0, 'keys 8, 9'),
    ('6A40 entry', 'CDarkSamus::ReadyForMissiles(', ['*(float*)$rdx'], SETS, 0, 'keys 10, 11, 20'),
]
