# Sporb (REL 76) on the coverage build: the gcov profile is the evidence; these breakpoints only
# count the grab chain's entries. Drivers: tools/cov/specs/sporb_*.con.
# Run: BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/sporb.py <out> <world,area>
#      --con tools/cov/specs/sporb_<x>.con --fast --frames <n>
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module Sporb
CLASS = 'CSporbBase'
SPEC = [
    ('UpdateGrabber 71AC', 'CSporbBase::UpdateGrabber(', ['$this->mGrabberState', '(int)$this->mMaxGrabRange', '(int)$this->mMinGrabRange'], 'max=4000'),
    ('AttachPlayer', 'CSporbBase::AttachPlayer(', [], ''),
    ('SpitPlayer', 'CSporbBase::SpitPlayer(', [], ''),
    ('ShootPowerBomb', 'CSporbBase::ShootPowerBomb(', [], ''),
    ('Projectile Think', 'CSporbProjectile::Think(', ['((CSporbProjectile*)$rdi)->mState', '((CSporbProjectile*)$rdi)->mSolidPhase'], 'max=8000'),
    ('Projectile CollidedWith', 'CSporbProjectile::CollidedWith(', [], ''),
]
