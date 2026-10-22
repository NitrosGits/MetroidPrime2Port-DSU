# DigitalGuardian (module 14), dg33: fn_14_1B8 (the DGRD loader) with an empty model (data-dead:
# the shipped DGRD, 0x00350060, has a model): the ANCS id is made invalid before LdrToModelData
# (the CMDL is kInvalidAssetId), so the loader returns null and the body is never added.
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg33_noload.py \
#        <out> 1baa96c2,09_cliff_temple_dark --fast --frames 600
CLASS = 'CDigitalGuardian'
SPEC = []
FORCE = [
    ('1B8 empty model', 'loc:PortCDigitalGuardian.cpp:3251', [],
     ['sldrThis.patterned.animationInformation.ancs = 0xffffffff'], 0,
     'no ANCS (the model id is already invalid): LdrToModelData is empty'),
]
