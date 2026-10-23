# ds10: the CScattershotProjectile::Render arms (fn_10_E7C, PortCScattershotProjectile.cpp) the
# rooftop fight does not take, forced on the coverage build:
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_INPUT=LAYER2@400 \
#     tools/cov/force.sh tools/cov/specs/darksamus_ds10_e7c.py <out> "1baa96c2,10_cliff_rooftop" \
#     --con tools/drivers/darksamus_scat.con --fast --frames 2000
# Render, put back at the return:
#   4k    B1RT / B2RT = the B1SE / B2SE elements (the data has no rotation element)
#   4k+1  the colour and rotation elements null (B1CL B1RT B2CL B2RT TSCL TECL: grey, 0.0f)
#   4k+2  the three textures null (no billboard, no trail)
#   4k+3  the RB1A / RB2A / RTLA bits flipped (the other blend factor) and the last shot at the
#         view point (the trail's side vector is zero: the `ble` at 0x0000195C taken)
# BillboardAxes every 3rd call: the rotation 30.0f (RotateLocalY; the data's B1RT/B2RT are 0).
CLASS = 'CScattershotProjectile'
DESC = '$this->mDesc'
SH = '((CScattershotProjectile::SShot*)$this->mShots.mData)[$this->mShots.mCount - 1]'
VIEW = 'CGraphics::mViewMatrix'

FORCE_RET = [
    ('E7C Render arms (forced)', 'CScattershotProjectile::Render(', 4,
     [(0, [('%s->mB1RT' % DESC, '%s->mB1SE' % DESC), ('%s->mB2RT' % DESC, '%s->mB2SE' % DESC)]),
      (1, [('%s->m%s' % (DESC, e), '0') for e in ('B1CL', 'B1RT', 'B2CL', 'B2RT', 'TSCL', 'TECL')]),
      (2, [('%s->m%s' % (DESC, e), '0') for e in ('B1TX', 'B2TX', 'TTEX')]),
      (3, [('%s->m%s' % (DESC, e), '!%s->m%s' % (DESC, e)) for e in ('RB1A', 'RB2A', 'RTLA')] +
          [('%s.mPos.m%s' % (SH, c), '%s.m%s3' % (VIEW, r)) for c, r in zip('XYZ', '012')])],
     '4k B1RT/B2RT = B1SE/B2SE; 4k+1 elements null; 4k+2 textures null; 4k+3 blend bits flipped, last shot at the view'),
    ('E7C BillboardAxes rotation (forced)', '(anonymous namespace)::BillboardAxes(', 3,
     [(1, [('!$xmm1.v4_float[0]', '30.0')])], 'rotation 30.0f'),
]
