# DigitalGuardian (module 14), dg30 run A: fn_14_6864 with the five model ids unset (data-dead:
# the one DGRD placement, 0x00350060, sets all five), so every model optional stays empty and the
# render/beacon bodies take their "no model" arms: fn_14_1994 (0x19B8), fn_14_18B4 (0x18E4),
# fn_14_17DC (0x1808), fn_14_2A28 (0x2A6C), fn_14_1700 (port-only guard).
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg30_models.py \
#        <out> 1baa96c2,09_cliff_temple_dark --driver 'python3 tools/drivers/dg30_beacon.py {port} a' \
#        --fast --frames 700
CLASS = 'CDigitalGuardian'
SPEC = []
FORCE = [
    ('6864 no models', 'CDigitalGuardian::x6864_(', [],
     ['$this->mData.mLegModel = 0xffffffff', '$this->mData.mShinArmor = 0xffffffff',
      '$this->mData.mKneeArmor = 0xffffffff', '$this->mData.mTransmissionBeacon = 0xffffffff',
      '$this->mData.mToeTargetModel = 0xffffffff'], 0, 'the five model ids unset'),
]
