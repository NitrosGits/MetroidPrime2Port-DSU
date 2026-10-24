# OctapedeSegment (REL 46) forcing spec: the member-decided arms of the bodies oct1 left unmarked.
# One call per write, the old value put back at the next entry of the same body.
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/oct1_force.py <out> \
#     1baa96c2,0p_cliff_hall --con tools/drivers/octapede_cliffhall.con --fast --frames 3000
import os

import gdb

THIS = '((COctapedeSegment*)$rdi)'


def _log(msg):
    try:
        with open(os.environ['COV_OUT'] + '/forced.log', 'a') as fh:
            fh.write(msg + '\n')
    except Exception:
        pass


def _set(gdb, expr):
    try:
        gdb.execute('set var ' + expr)
    except Exception as e:  # noqa: BLE001
        _log('set failed (%s): %s' % (expr, str(e)[:70]))


def _read(expr):
    try:
        return gdb.parse_and_eval(expr)
    except Exception as e:  # noqa: BLE001
        _log('read failed (%s): %s' % (expr, str(e)[:60]))
        return None


def _save(st, member, value):
    old = _read(THIS + '->' + member)
    if old is None:
        return
    st.restore.append('%s->%s = %s' % (THIS, member, str(old)))
    _set(gdb, '%s->%s = %s' % (THIS, member, value))


class OneShot:
    def __init__(self, hits):
        self.n = 0
        self.hits = hits
        self.restore = []

    def __call__(self, gdb):
        for expr in self.restore:
            _set(gdb, expr)
        self.restore = []
        self.n += 1
        fn = self.hits.get(self.n)
        if fn is not None:
            fn(gdb, self)


# ------------------------------------------------- fn_46_30BC AcceptScriptMsg
# 0x281's `mSegmentState == kSegS_Bouncing && mBouncesRemaining > 0` early return (kSegS_Bouncing 2,
# ESegmentState kSegS_Idle 0); 0x248's `mHeadSegmentId != GetUniqueId() && mChildSegmentIds.size() > 0`.
def _asg_bouncing(gdb, st):
    _save(st, 'mSegmentState', '2')
    _save(st, 'mBouncesRemaining', '2')


def _asg_head_self(gdb, st):
    _save(st, 'mHeadSegmentId.value', str(int(_read(THIS + '->mUniqueId.value'))))


accept_msg = OneShot({20: _asg_bouncing, 40: _asg_bouncing, 60: _asg_head_self, 90: _asg_head_self})


# -------------------------------------------------- fn_46_14E0 CollidedWith
# 0x572's `if (mBrokenApart)` false arm and 0x585's `... && mBouncesRemaining > 0` false arm.
def _col_whole(gdb, st):
    _save(st, 'mBrokenApart', '0')


def _col_no_bounce(gdb, st):
    _save(st, 'mBrokenApart', '1')
    _save(st, 'mBouncesRemaining', '0')


collided = OneShot({3: _col_whole, 8: _col_no_bounce, 20: _col_no_bounce, 40: _col_whole})


# ------------------------------------------------------- fn_46_2924 Think
# 0x313's `if (mPlayerObstructed)` arm (the occluded early return) and 0x312's
# `if (mBouncesRemaining <= 0)` true arm.
def _think_obstructed(gdb, st):
    _save(st, 'mBouncesRemaining', '0')
    _save(st, 'mPlayerObstructed', '1')


def _think_not_obstructed(gdb, st):
    _save(st, 'mBouncesRemaining', '0')
    _save(st, 'mPlayerObstructed', '0')


think = OneShot({6: _think_obstructed, 12: _think_not_obstructed, 24: _think_obstructed,
                 48: _think_not_obstructed})


CLASS = 'COctapedeSegment'
FORCE = [
    ('30BC AcceptScriptMsg bouncing', 'COctapedeSegment::AcceptScriptMsg(', [], [accept_msg], 0,
     'mSegmentState = kSegS_Bouncing with bounces left (the Landed early return) and the head = self'),
    ('14E0 CollidedWith broken-apart', 'COctapedeSegment::CollidedWith(', [], [collided], 0,
     'mBrokenApart cleared (0x572 false) and bounces exhausted (0x585 false)'),
    ('2924 Think obstruction', 'COctapedeSegment::Think(', [], [think], 0,
     'mBouncesRemaining 0 with mPlayerObstructed set and clear (0x312 true, 0x313 both arms)'),
]
