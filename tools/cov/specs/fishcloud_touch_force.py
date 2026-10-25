# FishCloud (module 20) Touch forcing spec for tools/cov/force.sh (fish2, 2026-10-25). Touch(other, mgr): this=rdi, other=rsi.
# Two runs, merged with the natural ones in gcov_report.py:
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/fishcloud_touch_force.py <out> \
#     3dfd2249,01_swamp_entrance --con tools/cov/specs/fishcloud_swampentrance.con --fast --frames 2000   (weapon arms)
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/fishcloud_touch_force.py <out> \
#     3bfa3eff,0k_temple_hall --con tools/cov/specs/fishcloud_morph.con --fast --frames 1000   (ball arms)
CLASS = 'CFishCloud'
_state = {'k': 0}


def _set(gdb, expr):
    gdb.execute('set var ' + expr.replace('$this', '((CFishCloud*)$rdi)'))


def touch(gdb):
    sym = gdb.execute('info symbol *(long*)$rsi', to_string=True)
    if 'Projectile' in sym:
        k = _state['k']
        _state['k'] += 1
        if k == 0:
            # the other weapon already inactive: GetTouchBounds null (698 invalid arm)
            _set(gdb, '((CGameProjectile*)$rsi)->mActive = 0')
            return
        if k == 1:
            _set(gdb, '$this->mBoids.mItems[1].mActive = 0')  # an inactive boid in the loop (702 arm)
        for i in (0, 2):  # boids on the weapon: bounds overlap, KillBoid (705/706)
            for c, m in (('mX', 'm03'), ('mY', 'm13'), ('mZ', 'm23')):
                _set(gdb, '$this->mBoids.mItems[%d].mPos.%s = ((CActor*)$rsi)->mTransform.%s' % (i, c, m))


def ball(gdb):
    # the ball path: boid velocities zero (CanBeNormalized false) on every 16th think, NaN on every 16th+8 (the unordered arm)
    sym = gdb.execute('info symbol *(long*)$rsi', to_string=True)
    if 'CPlayer' not in sym:
        return
    n = int(gdb.parse_and_eval('((CFishCloud*)$rdi)->mThinkCounter')) % 16
    if n in (0, 8):
        val = '0' if n == 0 else '(0.0/0.0)'
        for i in range(0, 40):
            for c in ('mX', 'mY', 'mZ'):
                _set(gdb, '$this->mBoids.mItems[%d].mVel.%s = %s' % (i, c, val))


FORCE = [
    ('22EC Touch weapon', 'CFishCloud::Touch(', [], [touch], 0,
     'weapon touches: inactive weapon, an inactive boid, boids on the weapon (KillBoid)'),
    ('22EC Touch ball', 'CFishCloud::Touch(', [], [ball], 0,
     'player touches: boids with zero and NaN velocity (vel CanBeNormalized false, unordered)'),
]
