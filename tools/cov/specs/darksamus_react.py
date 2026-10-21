# DarkSamus ShouldReact (fn_10_11158) and AnyIncomingMissiles (fn_10_1BB08), a forcing run on the
# coverage build: `trigcall <id> ShouldReact <arg>` (darksamus_react.con), keyed by the argument
# (the CTriggerData at $rdx) less 100; ShouldReact hands the same data to the four triggers it asks.
#   30 every member saved, the forces off          99 every member put back
#   1 the last heavy hit now (0xC0C, WasHeavyHit true); every other key: no heavy hit (0xC0C = 0)
#   3.. the stage's missile float (its info's +0x24, CanMissileDeflect's "below zero") reads -1.0
#       at the next lookups (written where GetBattleStage casts the object); 99 puts it back
#   4.. the projectile filter (fn_10_1B9B0, CanReflectProjectile) is asked with the type -1 (the
#       `int` at $rcx) instead of 6, so the console's power-beam `shoot` counts as a missile; the
#       stage float doubles as the reflect chance, which the -1.0 above would fail
#   10 0x8, 11 0x40, 12 0x200, 13 0x400, 14 0x10000, 15 0x200000, 16 0xFFFF with the fade flag
#   0xDEC 1, 17 0xFFFF with 0xDEC 0, 18 0x2 (the switch's default): the current attack 0x980
# Run: BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darksamus_react.py <out>
#      "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_react.con --fast --frames 800
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module DarkSamus
CLASS = 'CDarkSamus'


def k(v):
    return '(*(float*)$rdx == %d)' % (100 + v)


def ks(lo, hi):
    return '(*(float*)$rdx >= %d && *(float*)$rdx <= %d)' % (100 + lo, 100 + hi)


NOW = '$this->mElapsedTime'


def keep(var, member):
    return '%s = %s ? %s : ($_isvoid(%s) ? 0 : %s)' % (var, k(30), member, var, var)


def put(member, var, *pairs):
    expr = member
    for v, val in reversed(pairs + ((99, var),)):
        expr = '%s ? %s : (%s)' % (k(v), val, expr)
    return '%s = %s' % (member, expr)


MEMBERS = [('$dsk_c0c', '$this->mGuestC0C'), ('$dsk_ca', '$this->mCurrentAttack'), ('$dsk_fo', '$this->mFadeOut')]
V = dict((m, v) for v, m in MEMBERS)
ATTACKS = [(10, 0x8), (11, 0x40), (12, 0x200), (13, 0x400), (14, 0x10000), (15, 0x200000), (16, 0xFFFF),
           (17, 0xFFFF), (18, 0x2)]
SETS = [keep(v, m) for v, m in MEMBERS] + [
    '$dsk_md = %s ? 0 : (%s ? 1 : (%s ? 2 : ($_isvoid($dsk_md) ? 0 : $dsk_md)))' % (k(30), ks(3, 29), k(99)),
    '$dsk_mds = %s ? 0 : ($_isvoid($dsk_mds) ? 0 : $dsk_mds)' % k(30),
    '$dsk_mdsv = %s ? 0 : ($_isvoid($dsk_mdsv) ? 0 : $dsk_mdsv)' % k(30),
    '$dsk_rf = %s ? 1 : 0' % ks(4, 29),
    put('$this->mGuestC0C', V['$this->mGuestC0C'], (1, NOW), *[(v, 0) for v in range(2, 30)]),
    put('$this->mCurrentAttack', V['$this->mCurrentAttack'], *ATTACKS),
    put('$this->mFadeOut', V['$this->mFadeOut'], (16, 1), (17, 0))]
F = '((CDarkSamusBattleStage*)$rdi)->mStageInfo.missiles.unknown_0x7ee77018'
STAGE = ['$dsk_mds = ($dsk_md == 1 && $dsk_mdsv == 0) ? %s : $dsk_mds' % F,
         '$dsk_mdsv = $dsk_md == 1 ? 1 : $dsk_mdsv',
         '%s = $dsk_md == 1 ? -1.0 : (($dsk_md == 2 && $dsk_mdsv == 1) ? $dsk_mds : %s)' % (F, F),
         '$dsk_mdsv = $dsk_md == 2 ? 0 : $dsk_mdsv',
         '$dsk_md = $dsk_md == 2 ? 0 : $dsk_md']
FORCE = [
    ('11158 entry|ret', 'CDarkSamus::ShouldReact(', ['*(float*)$rdx', '$this->mCurrentAttack'], SETS, 0,
     'the keys above'),
    ('stage cast', 'CDarkSamusBattleStage* TCastToPtr<CDarkSamusBattleStage>(CEntity*)', ['$dsk_md', F],
     STAGE, 0, 'the missile float after keys 3..'),
    ('1B9B0 entry|ret', 'CDarkSamus::CanReflectProjectile(', ['$dsk_rf', '(int)$rcx', '$rdx != 0'],
     ['$rcx = $dsk_rf == 1 ? -1 : $rcx'], 0, 'the type -1 after keys 4..'),
    ('145E4 entry|ret', 'CDarkSamus::CanWalkToPoint(', ['$dsk_rf'], [], 0, 'none (records the answer)'),
    ('1BB08 entry|ret', 'CDarkSamus::AnyIncomingMissiles(', ['*(float*)$rdx'], [], 0, 'none'),
]
