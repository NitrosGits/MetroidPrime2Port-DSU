# dsscat forcing spec for tools/cov/force.sh: the two Think arms (fn_10_2918) the rooftop fight
# does not take. A zero dt never reaches a live weapon, and every scatter projectile of the fight
# has been deleted before its lifetime passes 10.0f.
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/darksamus_scat_force.py <out> \
#        "1baa96c2,10_cliff_rooftop" --con tools/drivers/darksamus_scat.con --fast --frames 2000
#      (with MP2_INPUT=LAYER2@400 in the environment)
CLASS = 'CScattershotProjectile'

def CNT(n):
    return '$%s = $_isvoid($%s) ? 1 : $%s + 1' % (n, n, n)

FORCE = [
    ('2918 Think dt/lifetime (forced)', 'CScattershotProjectile::Think(',
     ['$xmm0.v4_float[0] <= 0', '$this->mLifetime > 10.0'],
     [CNT('th'), '$xmm0.v4_float[0] = ($th % 97 == 0) ? 0.0 : $xmm0.v4_float[0]',
      '$this->mLifetime = ($th % 211 == 0) ? 11.0 : $this->mLifetime'], 0,
     'dt = 0 on every 97th call (the early return at 0x00002950) and mLifetime = 11 on every 211th '
     '(the self-delete past 10.0f at 0x000029A0)'),
]
