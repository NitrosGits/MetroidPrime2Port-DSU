# DigitalGuardian (module 14), dg33: fn_14_B100 (the ctor) with every optional asset unset
# (data-dead: the one DGRD placement, 0x00350060, sets them all): the crippled SCAN (data +0),
# the toe explosion (+0x28), the beacon fx (+0x190), explosion (+0x1F4) and echo hit (+0x1F8),
# so each "id != -1" test (addis/cmplwi 0xffff/beq at 0xB3B0, 0xB6D4, 0xB84C, 0xBF54) and the
# null-generator test after fn_14_D664 take their skip arms, and fn_14_6864 with the five model
# ids unset so the echo-emitter test (0xC01C) skips too.
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg33_ctor.py \
#        <out> 1baa96c2,09_cliff_temple_dark --con tools/smoke/cmd_digig.txt --fast --frames 600
CLASS = 'CDigitalGuardian'
SPEC = []
FORCE = [
    ('B100 no assets', 'CDigitalGuardian::CDigitalGuardian(', [],
     ['data.mScannableInfoCrippled = 0xffffffff', 'data.mToeTargetExplosion = 0xffffffff',
      'data.mTransmissionBeaconFx = 0xffffffff', 'data.mTransmissionBeaconExplosion = 0xffffffff',
      'data.mTransmissionBeaconEchoHit = 0xffffffff'], 0, 'the five optional asset ids unset'),
    ('6864 no models', 'CDigitalGuardian::x6864_(', [],
     ['$this->mData.mLegModel = 0xffffffff', '$this->mData.mShinArmor = 0xffffffff',
      '$this->mData.mKneeArmor = 0xffffffff', '$this->mData.mTransmissionBeacon = 0xffffffff',
      '$this->mData.mToeTargetModel = 0xffffffff'], 0, 'the five model ids unset'),
]
