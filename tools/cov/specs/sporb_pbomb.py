# Sporb (REL 76) CSporbPowerBomb on the coverage build: the gcov profile is the evidence; these
# breakpoints count the bomb's impacts and its blast states. Driver: tools/cov/specs/sporb_pbomb.con.
# Run: BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/sporb_pbomb.py <out>
#      3dfd2249,11_swamp_bloggtank_dark --con tools/cov/specs/sporb_pbomb.con --fast --frames 2000
CLASS = 'CSporbBase'
SPEC = [
    ('Shoot', 'CSporbBase::ShootPowerBomb(', ['(int)$this->mTimerScaleMaxed'], ''),
    ('11D44 ResolveCollisionWithActor', 'CSporbPowerBomb::ResolveCollisionWithActor(', [], ''),
    ('11DB8 ResolveCollisionWithWorld', 'CSporbPowerBomb::ResolveCollisionWithWorld(', [], ''),
    ('11E24 Think', 'CSporbPowerBomb::Think(', ['((CSporbPowerBomb*)$rdi)->mState', '((CSporbPowerBomb*)$rdi)->mDamageWaitTimer > 0'], 'max=20000'),
]
