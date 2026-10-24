# PlantScarabSwarm (module 49) psc3 read-only probe for tools/cov/run.sh: records the live state the
# ctor built (shared attack model / state / deltas, the two bit numbers) and, per PreRenderBoid call,
# the boid's attack flags and whether the draw mask carries the shared bit.  Nothing is written.
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/psc3_plant_probe.py <out> \
#     3dfd2249,0g_swamp_hall --con tools/cov/specs/psc3_plant_0g.con --fast --frames 1200
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/psc3_plant_probe.py <out> \
#     3dfd2249,ok_swamp_hall --con tools/cov/specs/psc3_plant_ok.con --fast --frames 1200
CLASS = 'CPlantScarabSwarm'
P = '((CPlantScarabSwarm*)$rdi)'
B = '((CSwarmBasics::CBoid*)$rsi)'
SPEC = [
    ('1B64 Think model', 'CPlantScarabSwarm::Think(', [
        P + '->mAttackerCount',
        P + '->mAttackModels.mCount',
        P + '->mModelDatas.mCount',
        P + '->mSharedAttackModel.mPtr != 0',
        P + '->mSharedAttackState.mPtr != 0',
        P + '->mSharedAttackDeltas.mPtr != 0',
        P + '->mSharedAttackBit',
        P + '->mAttackModelBitBase',
    ], 'max=4000'),
    # cpp:214-235: 214 the attacking test, 216 slot == -1, 218 the shared draw bit (the arm psc1
    # could not reach), 229 the indexed slot bit.
    ('12AC PreRenderBoid', 'CPlantScarabSwarm::PreRenderBoid(', [
        P + '->mSharedAttackBit',
        B + '->mActive',
        B + '->mAttacking',
        B + '->mAttackSlot',
        '(*(unsigned int*)$rdx & (1u << ((CPlantScarabSwarm*)$rdi)->mSharedAttackBit)) != 0',
        '(*(unsigned int*)$rdx & (1u << ((CPlantScarabSwarm*)$rdi)->mAttackModelBitBase)) != 0',
        P + '->mSharedAttackState.mPtr != 0',
    ], 'max=60000'),
]
