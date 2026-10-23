# IngSpaceJumpGuardian (REL 34) spec for tools/cov/run.sh; arms come from the coverage build
# (gcov_report.py --module IngSpaceJumpGuardian), the breakpoints only count calls and state msgs.
# Agon Wastes 02_sand_sandfalls_dark (42b935e4, area 25) layer 2 places the Jump Guardian
# (editor 001900e9, inactive). tools/drivers/ingspacejumpguardian_sandfalls.con wakes it and
# walks the fight:
#   BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/ingspacejumpguardian.py <out> \
#     42b935e4,02_sand_sandfalls_dark --con tools/drivers/ingspacejumpguardian_sandfalls.con --fast
#   then tools/cov/gcov_report.py build-port-cov <out>/gcov --module IngSpaceJumpGuardian
CLASS = 'CIngSpaceJumpGuardian'
_S = ['Start', 'Dead', 'FollowIntroPattern', 'FaceTarget', 'Jump', 'Taunt', 'MiniPortalAttack',
      'PathFind', 'SteerToDest']
SPEC = [('AcceptScriptMsg', 'CIngSpaceJumpGuardian::AcceptScriptMsg(', [], 'max=4000'),
        ('DoUserAnimEvent', 'CIngSpaceJumpGuardian::DoUserAnimEvent(', ['$rcx'], 'max=4000')]
SPEC += [(s, 'CIngSpaceJumpGuardian::%s(' % s, ['$rdx'], 'max=4000') for s in _S]
SPEC += [(f, 'CIngSpaceJumpGuardian::%s(' % f, [], 'max=400') for f in
         ['SelectJumpTarget', 'FilterJumpPoints', 'FindBestMiniPortals', 'ComputeJumpVelocity',
          'CollisionDamage', 'TouchDamage', 'SpawnShockWave', 'CameraShake', 'Listen']]
