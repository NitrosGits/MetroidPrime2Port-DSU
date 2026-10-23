# PlantScarabSwarm (module 49) forcing spec for tools/cov/force_ret.py (writes last one call),
# same area and driver as tools/cov/specs/plantscarabswarm.py (merge the gcov dirs in gcov_report.py):
#   BUILD=$PWD/build-port-cov COV_GDB_SCRIPT=tools/cov/force_ret.py tools/cov/force.sh \
#     tools/cov/specs/plantscarabswarm_force.py <out> 3dfd2249,0g_swamp_hall \
#     --con tools/cov/specs/plantscarabswarm_swamp.con --fast --frames 3000
# The data-dependent arms (attackerCount 0, animations -1, isVulnerableToSafeZone) are dead in the
# shipped data (tools/scan_pssm_props.py) and are not forced here.
CLASS = 'CPlantScarabSwarm'
B = '((CSwarmBasics::CBoid*)$rsi)'
FORCE_RET = [
    ('1B64 Think angle', 'CPlantScarabSwarm::Think(', 5, [(0, [('$this->mMaxAttackAngle', '0.0')])],
     'mMaxAttackAngle 0 for one call: an in-range boid fails the attack-angle test'),
    ('12AC PreRenderBoid slot bit', 'CPlantScarabSwarm::PreRenderBoid(', 4,
     [(0, [('!*(unsigned int*)$rdx', '*(unsigned int*)$rdx & ~(1u << $this->mAttackModelBitBase)')])],
     'slot-0 draw bit cleared first (what the retail slot -1 clear does): slot 0 finds it clear'),
    ('1254 RenderBoid slot -1', 'CPlantScarabSwarm::RenderBoid(', 40,
     [(0, [(B + '->mAttacking', '1'), (B + '->mAttackSlot', '-1')])],
     'boid drawn as attacking with slot -1 for one call (the shared attack model)'),
    ('1144 BoidCollidedWithPlayer launched', 'CPlantScarabSwarm::BoidCollidedWithPlayerCallback(', 2,
     [(0, [(B + '->mLaunched', '1')])],
     'the touching boid marked launched for one call: KillBoid instead of the base touch'),
]
