# DigitalGuardian (module 14), dg27: fn_14_DF7C (the DGHD loader) with an empty model (data-dead:
# the one DGHD placement, 0x0035000F, has a model): the ANCS id is made invalid before LdrToModelData (the CMDL is kInvalidAssetId), so the
# loader returns null and the head is never added.
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg27_noload.py \
#        <out> 1baa96c2,09_cliff_temple_dark --fast --frames 600
CLASS = 'CDigitalGuardianHead'
SPEC = []
FORCE = [
    ('DF7C empty model', 'loc:PortCDigitalGuardianHead.cpp:2728', [],
     ['sldrThis.patterned.animationInformation.ancs = 0xffffffff'], 0,
     'no ANCS (the model id is already invalid): LdrToModelData is empty'),
]
