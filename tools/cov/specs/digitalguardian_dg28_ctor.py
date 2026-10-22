# DigitalGuardian (module 14), dg28: fn_14_18264 (the head's ctor) with every optional asset unset
# (data-dead: the one DGHD placement, 0x0035000F, names all seven): the four 'PART' ids (mData, read
# at 0x18418/0x18590/0x1870C/0x18904) and the three 'SCAN' ids (the loader's data, 0x18E70/0x18F38/
# 0x19000) become kInvalidAssetId before the first test, so every "not set" arm runs.
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg28_ctor.py \
#        <out> 1baa96c2,09_cliff_temple_dark --fast --frames 600
CLASS = 'CDigitalGuardianHead'
SPEC = []
FORCE = [
    ('18264 no assets', 'loc:PortCDigitalGuardianHead.cpp:237', [],
     ['this->mData.dropShieldsFx = 0xffffffff', 'this->mData.pART = 0xffffffff',
      'this->mData.bombSlotDestroyedFx = 0xffffffff',
      'this->mData.echoTargetDestroyedFx = 0xffffffff',
      'data.scannableInfoShieldHead = 0xffffffff', 'data.scannableInfoStunnedHead = 0xffffffff',
      'data.scannableInfoFinalHead = 0xffffffff'], 0,
     'the four PART and three SCAN ids unset'),
]
