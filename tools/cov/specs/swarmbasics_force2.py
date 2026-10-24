# SwarmBasics (module 80) boid-path forcing spec for tools/cov/force.sh, same drivers as
# tools/cov/specs/swarmbasics.py (use the swamp/cliff/hive areas: only those swarms create boids):
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swarmbasics_force2.py <out> \
#     3dfd2249,0g_swamp_hall --con tools/cov/specs/swarmbasics_swamp.con --fast --frames 3000
# The frozen-boid and unlit arms of the boid draw path: `boid` is the first argument ($rsi) of both
# PreRenderBoid/DrawBoidSkinnedModel/RenderBoid, `$this` is the swarm.
CLASS = 'CSwarmBasics'

FORCE = [
    ('4038 DrawBoid frozen', 'CSwarmBasics::DrawBoidSkinnedModel(',
     [], ['((CSwarmBasics::CBoid*)$rsi)->mFreezeTimer = 1.0'], 0,
     'the mFreezeTimer > 0 arm (line 1136) and the lerp'),
    ('4038 DrawBoid lighting off', 'CSwarmBasics::DrawBoidSkinnedModel(',
     [], ['$this->mEnableLighting = false'], 2,
     'the mEnableLighting false arm (line 1140)'),
    ('4194 PreRenderBoid frozen', 'CSwarmBasics::PreRenderBoid(',
     [], ['((CSwarmBasics::CBoid*)$rsi)->mFreezeTimer = 1.0'], 0,
     'the !(mFreezeTimer > 0) false arm (line 1068)'),
    ('4128 RenderBoid frozen', 'CSwarmBasics::RenderBoid(',
     [], ['((CSwarmBasics::CBoid*)$rsi)->mFreezeTimer = 1.0'], 0,
     'the frozen boid draw arm'),
]
