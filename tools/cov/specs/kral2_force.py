# Kralee (REL 37) forcing spec for tools/cov/force_ret.py. The whole disc's ten KRAL placements set
# both warp particle ids (tools/scan_kralee_props.py), so CKralee's ctor `!= kInvalidAssetId` arms
# are the ones a load takes; the only arm a placement never takes is LoadKralee's null-modelData
# return, because every placement's patterned.animationInformation.ancs is a real ANCS.
#
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/kral2_force.py <out> 3bfa3eff,0b_temple_hall --con tools/cov/specs/kralee.con \
#     --fast --frames 1300
#
# fn_37_13C LoadKralee `if (!modelData) return nullptr;`: LdrToModelData (ScriptLoader.cpp:446-459)
# returns an empty optional only when both GetResourceTypeById answers are 0. LoadKralee passes
# kInvalidAssetId as the model, so forcing the animation set's ancs (rdx = &animation, ancs at +0)
# to kInvalidAssetId makes both lookups 0 (CResLoader.cpp:208-214 returns 0 for an id in no pak) and
# the arm runs. period 2 / phase 1 forces the first of the area's two placements, so the second
# Kralee still spawns and the driver's commands keep working.
CLASS = 'CKralee'

FORCE_RET = [
    ('13C LoadKralee no model', 'LdrToModelData(', 2,
     [(1, [('*(unsigned int*)$node', '0xffffffff')])],
     'ancs forced invalid for one LdrToModelData call: both resource types 0, so it returns an '
     'empty optional and LoadKralee returns nullptr (the cpp:411 arm)'),
]
