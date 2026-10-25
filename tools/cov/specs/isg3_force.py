# IngSpiderballGuardian (module 35) forcing spec, lane isg3: the arms of DoUserAnimEvent,
# MoveAlongWaypoints and ApplyProximityDamage that the natural fight (isbg.py + isbg.con) cannot
# reach. Every write lasts one call (tools/cov/force_ret.py) except the FORCE_INNER position
# writes, which are the guardian standing where the arm needs it (the body rewrites mPosition itself).
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py SECS=1800 timeout 1900 \
#     tools/cov/force.sh tools/cov/specs/isg3_force.py <out> 1baa96c2,06_cliff_spiderballguardian \
#     --con tools/cov/specs/isbg.con --fast --frames 6000
CLASS = 'CIngSpiderballGuardian'
WP = '((CActor*)$rax)'
UP_ = lambda c, a: '%s->mTransform.m%s2' % (a, c)  # noqa: E731
AT_TARGET = [('$this->mPosition.m%s' % ax,
              '%s->mPosition.m%s + $this->mRadius * %s->mTransform.m%s2' % (WP, ax, WP, c))
             for ax, c in (('X', '0'), ('Y', '1'), ('Z', '2'))]
BELOW = [('$this->mPosition.m%s' % ax,
          '%s->mPosition.m%s + $this->mRadius * %s->mTransform.m%s2 - 5.0 * $this->mTransform.m%s2'
          % (WP, ax, WP, c, c))
         for ax, c in (('X', '0'), ('Y', '1'), ('Z', '2'))]

FORCE_RET = [
    ('303C DoUserAnimEvent: unknown event type', 'CIngSpiderballGuardian::DoUserAnimEvent(', 3,
     [(1, [('$rcx', '126')])], 'event type 126 (not kUE_SoundPlay): handled stays false, CPatterned runs'),
    ('303C DoUserAnimEvent: player in range, weight -1', 'CIngSpiderballGuardian::DoUserAnimEvent(', 3,
     [(2, [('$this->mData.unknown_0x32133b39', '100000.0'), ('*(float*)($node + 0x24)', '-1.0')])],
     'range 100000 and POI weight -1: Random().Range(0,1) <= weight is false, no sound'),
    ('19D0 MoveAlongWaypoints: no waypoint', 'CIngSpiderballGuardian::MoveAlongWaypoints(', 100,
     [(33, [('$this->mTargetWaypointId.value', '0xffff')])], 'target id invalid: the body returns at once'),
    ('19D0 MoveAlongWaypoints: anim speed 0', 'CIngSpiderballGuardian::MoveAlongWaypoints(', 100,
     [(21, [('$this->mAnimSpeed', '0.0')])], 'mAnimSpeed <= FLT_EPSILON: mSpeed is left alone'),
    ('1878 ApplyProximityDamage: damage disabled', 'CIngSpiderballGuardian::ApplyProximityDamage(', 7,
     [(3, [('$this->mProximityDamageEnabled', '0')])], 'the damage bit clear while the effects are on'),
]

FORCE_INNER = [
    ('19D0 MoveAlongWaypoints: at the target', 'CIngSpiderballGuardian::MoveAlongWaypoints(',
     'CStateManager::GetObjectById(', 100, {7: AT_TARGET},
     'mPosition = waypoint + radius * waypoint up: toTarget is zero, IsMagnitudeSafe false'),
    ('19D0 MoveAlongWaypoints: below the target', 'CIngSpiderballGuardian::MoveAlongWaypoints(',
     'CStateManager::GetObjectById(', 100, {13: BELOW},
     'mPosition 5 below the target along the guardian up: direction = its up, right . direction = 0'),
]
