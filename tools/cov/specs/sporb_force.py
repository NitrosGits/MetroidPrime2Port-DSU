# Sporb (REL 76) forcing spec for tools/cov/force.sh: the grab chain (ball inside, hold, retract
# with the ball, attach, spit, boost escape). In 0e_swamp_hall the grabber's flight (aimed with the
# player's lead) ends on the floor about 2 units from a resting ball, out of the projectile's
# ball radius, so the ball-inside arm of CSporbProjectile::Think (fn_76_DA4C) is reached by
# widening that radius while the projectile is launched; everything after it runs unforced.
# Run: BUILD=build-port-cov COV_GDB_SCRIPT=$PWD/tools/cov/cov_force.py tools/cov/force.sh
#      tools/cov/specs/sporb_force.py <out> 3dfd2249,0e_swamp_hall --con tools/cov/specs/sporb_grab.con --fast --frames 2600
CLASS = 'CSporbProjectile'
FORCE = [
    ('DA4C Think: the ball inside the launched projectile (forced)', 'CSporbProjectile::Think(',
     ['$this->mState', '$this->mSolidPhase'],
     ['$this->mBallEscapeRadius = $this->mState == 0 ? 100 : $this->mBallEscapeRadius'], 0,
     'the morph ball is within the ball radius of the launched projectile, so it turns '
     'ball-inside and the base grabs it'),
]
SPEC = [
    ('UpdateGrabber', 'CSporbBase::UpdateGrabber(', ['((CSporbBase*)$rdi)->mGrabberState'], 'max=6000'),
    ('AttachPlayer', 'CSporbBase::AttachPlayer(', [], ''),
    ('SpitPlayer', 'CSporbBase::SpitPlayer(', [], ''),
]
