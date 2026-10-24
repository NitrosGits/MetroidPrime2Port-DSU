# SwarmBasics (module 80) forcing spec for tools/cov/force.sh, same areas/drivers as
# tools/cov/specs/swarmbasics.py:
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swarmbasics_force.py <out> \
#     3dfd2249,01_swamp_entrance --con tools/cov/specs/swarmbasics_entrance.con --fast --frames 3000
# The boid paths (CreateBoid 0/36, UpdateBoid 0/85 in the natural runs) only run once the generation
# loop fires: both forcing entries make the swarms of these areas spawn and then keep their boids.
CLASS = 'CSwarmBasics'

FORCE = [
    ('50B8 generation on', 'CSwarmBasics::Think(', [],
     ['$this->x4f0_30_ = true', '$this->mMaxCreatedBoids = 24', '$this->mBoidGenRate = 2.0',
      '$this->mBoidGenCooldownTimer = -1.0'], 20,
     'the generation loop runs on every Think: CreateBoid, then the boid update path'),
    ('07E4 UpdateEffects history', 'CSwarmBasics::UpdateEffects(', [], ['$this->mBoidGenCooldownTimer = -1.0'], 0,
     'keeps the boid count up for the animation/particle arms'),
]
