# DarkSamus phazon beam, the invalid-asset arms: PhazonVisorToken (fn_10_21104) and the beam's hit
# effect token (fn_10_45F8 at 0x00004604) are built from kInvalidAssetId (the ids written at their
# entry, before the actor's constructor runs them; both return through a hidden pointer in rdi, so
# the id is rsi), and fn_10_8178's sfx is -1. The beam is made and released but never updated:
# its rays would read the empty hit token, which retail (and the port) read unchecked.
# Run: BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darksamus_phazonbeam_invalid.py <out>
#      "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_phazonbeam_invalid.con --fast
#      --frames 760, then tools/cov/gcov_report.py build-port-cov <out>/gcov --module DarkSamus
CLASS = 'CDarkSamus'
FORCE = [
    ('21104 invalid', 'CDarkSamus::PhazonVisorToken(', ['$rsi'], ['$rsi = 0xffffffff'], 0,
     'the visor token id kInvalidAssetId'),
    ('45F8 invalid', 'CDarkSamus::PhazonBeamHitToken(', ['$rsi'], ['$rsi = 0xffffffff'], 0,
     'the hit effect id kInvalidAssetId'),
    ('8178 sfx -1', 'CDarkSamus::StartPhazonBeamSfx(', [], ['$this->mData.mSound = -1'], 0,
     'the sound -1'),
]
