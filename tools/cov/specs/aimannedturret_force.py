# AIMannedTurret (module 1) forcing spec for tools/cov/force.sh, same area and driver as
# tools/cov/specs/aimannedturret.py:
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/aimannedturret_force.py <out> \
#     42b935e4,06_sand_alamofront --con tools/drivers/aimannedturret_alamofront.con --fast --frames 14000
CLASS = 'CAIMannedTurret'

# The arms that need state the fight does not produce:
#  - UpdateAim's `if (snap)` arm: both callers pass `li r5,0` (Patrol 0x2C34, Attack 0x2EFC), so the
#    argument is written at the body entry ($rdx, third argument).
#  - the |dot| >= 1e-4 arms and the elevation arms: mAimDirection is written at line 401 (after the
#    body has read the turret transform, before the two dot tests), cycling a vertical, an opposite
#    and a zero-2d direction.
#  - Attack's inlined Limit(dot, 1) clamp (line 278): a non-unit mAimDirection makes the dot exceed 1.
_n = [0]


def aim_dir(m):
    _n[0] += 1
    mode = (_n[0] // 2) % 3
    m.execute('set var $this->mAimDirection.mX = %s' % ('-1.0' if mode == 1 else '0.0'))
    m.execute('set var $this->mAimDirection.mY = 0.0')
    m.execute('set var $this->mAimDirection.mZ = %s' % ('-1.0' if mode == 1 else
                                                        ('1.0' if mode == 0 else '0.0')))


def aim_scale(m):
    # x2 in place: the direction is unchanged but the dot with the rider's unit forward can pass 1
    for c in ('mX', 'mY', 'mZ'):
        m.execute('set var $this->mAimDirection.%s = $this->mAimDirection.%s * 2.0' % (c, c))


FORCE = [
    ('2C6C Attack limit clamp', 'loc:CAIMannedTurret.cpp:278', [], [aim_scale], 4,
     'the inlined CMath::Limit(dot, 1) clamp arm'),
    ('1F28 UpdateAim snapped', 'CAIMannedTurret::UpdateAim(', [], ['$rdx = 1'], 0,
     'the snap arm (both callers pass false)'),
    ('1F28 UpdateAim aim direction', 'loc:CAIMannedTurret.cpp:401', [], [aim_dir], 2,
     'the |dot| >= 1e-4 false arm, the horizontal rotation limits and the elevation arms'),
]
