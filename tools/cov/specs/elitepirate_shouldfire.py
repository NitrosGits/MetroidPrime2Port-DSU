# ElitePirate (module 15) ShouldFire (fn_15_6820 -> fn_15_6618) arms, with the console's
# `trigcall` and `var ep.*` (the timer 0xa34, the launcher uid 0x9fa, the target uid 0xa26, ep.yaw).
# Run: BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/elitepirate_shouldfire.py <out>
#        1baa96c2,b666b655 --con tools/cov/specs/elitepirate_shouldfire.con --fast --frames 1600
# (ElitePirate.md section 11). 0x67F4 needs the con's f760 `material -solid` force (ret=1 on ShouldFire). DynHit names a source line of CGameCollision.cpp: re-check it after edits there.
CLASS = 'CElitePirate'
SPEC = [
    ('0x6618 ShouldFire', 'CElitePirate::ShouldFire(', [], 'ret max=3000'),
    ('0x6C84 ShouldTurn', 'CElitePirate::ShouldTurn(', [], 'ret max=3000'),
]
# the ray's two halves (static world, then the near-list actors), keyed by whether the filter is
# ShouldFire's include Solid|Character (bits 59|33 = 0x0800000200000000; the filter is rcx in all three)
F = '*(unsigned long long *)$rcx == 0x0800000200000000'
SPEC += [
    ('RayInternal', 'CStateManager::RayCollideWorldInternal(', [F], 'ret max=20000'),
    ('RayStatic', 'CGameCollision::RayStaticLineOfSightTest(',
     [F] + ['(%s) ? (int)(((float *)%s)[%d] * 10) : 0' % (F, r, i)
            for r in ('$rsi', '$rdx') for i in range(3)] + ['(%s) ? (int)($xmm0.v4_float[0] * 10) : 0' % F],
     'ret max=20000'),
    ('RayDynamic', 'CGameCollision::RayDynamicLineOfSightTest(', [F], 'ret max=20000'),
]
# which actor blocks a ShouldFire ray in the dynamic half (the uid; u0400 is the player)
SPEC += [
    ('DynHit', 'loc:CGameCollision.cpp:192',
     ['*(unsigned long long *)&filter == 0x0800000200000000',
      '(*(unsigned long long *)&filter == 0x0800000200000000) ? id->value : 0'],
     'max=20000'),
]
