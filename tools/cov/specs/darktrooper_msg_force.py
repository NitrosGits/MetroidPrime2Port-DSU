# dtr8: AcceptScriptMsg / KnockBack operands the fights never reach. Run it with the flotsam driver
# (the hive_hall hanging troopers, tools/drivers/darktrooper_flotsam.con for KnockBack) and the msgs
# driver (tools/drivers/darktrooper_msgs.con for AcceptScriptMsg), 3bfa3eff,0e_temple_hive_hall, cov build:
#   Activate #1 / Alert #1 : mFiresMissiles = 1        -> the SetBossParams arm
#   Damage #1              : mAlive = mTransitioning = 1 -> the early return
#   Damage #2              : mAlive = 1, mTransitioning = 0 -> alive but not transitioning
#   AreaLoaded #1 / #2     : mFlotsam = 0, mIsSleeper = 1 / 0 -> the sleeper and non-sleeper material arms
#   Decrement #2           : ragdoll pointer nulled (restored at the next Think) -> the null ragdoll arm
#   KnockBack #3 / #4      : ragdoll nulled (restored at Think), HP forced -1 / 10 -> HP<=0 and HP>0 arms
CLASS = 'CDarkTrooper'
_n = {}
_saved = {}
H = '((CDarkTrooper*)$rdi)'


def _cnt(k):
    _n[k] = _n.get(k, 0) + 1
    return _n[k]


def _set(gdb, s):
    gdb.execute('set var ' + s)


def _null_ragdoll(gdb):
    this = int(gdb.parse_and_eval('$rdi'))
    ptr = int(gdb.parse_and_eval(H + '->mRagDollHolder.mRagDoll.mPtr'))
    if ptr:
        _saved[this] = ptr
        _set(gdb, H + '->mRagDollHolder.mRagDoll.mPtr = 0')


def _msg(gdb):
    m = int(gdb.parse_and_eval('((CScriptMsg*)$rdx)->m_msg'))
    names = {int(gdb.parse_and_eval(n)): n for n in
             ('kSM_Activate', 'kSM_Alert', 'kSM_Damage', 'kSM_AreaLoaded', 'kSM_Decrement')}
    n = names.get(m)
    if n is None:
        return
    c = _cnt(n)
    if n in ('kSM_Activate', 'kSM_Alert') and c == 1:
        _set(gdb, H + '->mFiresMissiles = 1')
    elif n == 'kSM_Damage' and c == 1:
        _set(gdb, H + '->mAlive = 1')
        _set(gdb, H + '->mTransitioning = 1')
    elif n == 'kSM_Damage' and c == 2:
        _set(gdb, H + '->mAlive = 1')
        _set(gdb, H + '->mTransitioning = 0')
    elif n == 'kSM_AreaLoaded' and c == 1:
        _set(gdb, H + '->mRagDollHolder.mFlotsam = 0')
        _set(gdb, H + '->mIsSleeper = 1')
    elif n == 'kSM_AreaLoaded' and c == 2:
        _set(gdb, H + '->mRagDollHolder.mFlotsam = 0')
        _set(gdb, H + '->mIsSleeper = 0')
    elif n == 'kSM_Decrement' and c == 2:
        _null_ragdoll(gdb)


def _knock(gdb):
    c = _cnt('kb')
    if c in (3, 4):
        _null_ragdoll(gdb)
        _set(gdb, H + '->mRagDollHolder.mFlotsam = 1')
        _set(gdb, '((CAi*)$rdi)->mHealthInfo.mHealth = %s' % ('-1.0f' if c == 3 else '10.0f'))


def _think(gdb):
    this = int(gdb.parse_and_eval('$rdi'))
    ptr = _saved.pop(this, 0)
    if ptr:
        _set(gdb, H + '->mRagDollHolder.mRagDoll.mPtr = %d' % ptr)


FORCE = [
  ('Msg', 'CDarkTrooper::AcceptScriptMsg(', [], [_msg], 0, 'forced: members by message and count'),
  ('KnockBack', 'CDarkTrooper::KnockBack(', [], [_knock], 0, 'forced: flotsam / ragdoll / HP by call count'),
  ('Think', 'CDarkTrooper::Think(', [], [_think], 0, 'restores a nulled ragdoll'),
]
