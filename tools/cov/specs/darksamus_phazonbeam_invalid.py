# DarkSamus phazon beam, the invalid-asset arms: PhazonVisorToken (fn_10_21104) and the beam's hit
# effect token (fn_10_45F8 at 0x00004604) are built from kInvalidAssetId (the ids written at their
# entry, before the actor's constructor runs them), and fn_10_8178's sfx is -1. The beam then runs
# with the cylinder forced empty (dt 0.0133, as in darksamus_phazonbeam.py): no damage, so no visor
# effect, but every ray that hits takes the empty-token arm.
# Run: BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darksamus_phazonbeam_invalid.py <out>
#      "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_phazonbeam_invalid.con --fast
#      --frames 760, then tools/cov/gcov_report.py build-port-cov <out>/gcov --module DarkSamus
CLASS = 'CDarkSamus'
K = '($xmm0.v4_float[0] > 0.01326 && $xmm0.v4_float[0] < 0.01334)'
FORCE = [
    ('21104 invalid', 'CDarkSamus::PhazonVisorToken(', ['$rsi'], ['$rsi = 0xffffffff'], 0,
     'the visor token id kInvalidAssetId'),
    ('45F8 invalid', 'CDarkSamus::PhazonBeamHitToken(', ['$rsi'], ['$rsi = 0xffffffff'], 0,
     'the hit effect id kInvalidAssetId'),
    ('8178 sfx -1', 'CDarkSamus::StartPhazonBeamSfx(', [], ['$this->mData.mSound = -1'], 0,
     'the sound -1'),
    ('5974 entry', 'CDarkSamus::UpdatePhazonBeam(', [], ['$cyc = 0', '$pm = %s ? 2 : 0' % K], 0,
     'the cylinder empty for dt 0.0133'),
    ('5974 cylinder', 'CCylinder::PointInside(', [],
     ['$cyc = $cyc + 1', '((CCylinder*)$rdi)->mRadius = $pm == 2 ? 0 : ((CCylinder*)$rdi)->mRadius',
      '$pm = $cyc >= 3 ? 0 : $pm'], 0, 'the beam cylinder radius 0'),
]
