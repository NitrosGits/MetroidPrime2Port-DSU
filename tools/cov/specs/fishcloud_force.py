# FishCloud (module 20) forcing spec for tools/cov/force.sh, same drivers as tools/cov/specs/fishcloud.py
# (merge the gcov dirs in gcov_report.py). Two runs: the attractor (sacredpath) and the repulsors (LandingSite):
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/fishcloud_force.py <out> \
#     3bfa3eff,06_temple_sacredpath --con tools/cov/specs/fishcloud_sacredpath.con --fast --frames 2000
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/fishcloud_force.py <out> \
#     3bfa3eff,01_Temple_LandingSite --con tools/cov/specs/fishcloud_landingsite.con --fast --frames 1800
# The every-32nd-think conditions keep the clouds swimming between the forced frames.
CLASS = 'CFishCloud'
_EVERY = '($this->mThinkCounter % 32 == 0)'
_BOID = '((CFishCloud::CBoid*)$rsi)'
_PT = '((CVector3f*)$rdx)'
FORCE = [
    ('Think random movement', 'CFishCloud::Think(', [],
     ['$this->mRandomMovement = $this->mThinkCounter & 1',
      '$this->mRandomMovementTimer = ($this->mThinkCounter & 2) ? 0.5 : 0.0'], 0,
     'mRandomMovement/mRandomMovementTimer (only the ctor and Think write them, always 0): the random arms'),
    ('ApplyContainment zero velocity', 'CFishCloud::ApplyContainment(', [],
     ['%s->mVel.%s = %s ? 0.0 : %s->mVel.%s' % (_BOID, c, _EVERY, _BOID, c) for c in ('mX', 'mY', 'mZ')], 0,
     'a boid with zero velocity: CanBeNormalized false, then Think\'s zero-speed arm'),
] + [
    ('%s point at boid' % fn, 'CFishCloud::%s(CFishCloud::CBoid&, CVector3f const&' % fn, [],
     ['%s->mPos.%s = %s ? %s->%s : %s->mPos.%s' % (_BOID, c, _EVERY, _PT, c, _BOID, c)
      for c in ('mX', 'mY', 'mZ')], 0,
     'the point on the boid: CanBeNormalized false')
    for fn in ('ApplyCohesion', 'ApplySeparation', 'ApplyAttraction')
] + [
    ('3F2C AddAttractor full', 'CFishCloud::AddAttractor(', [],
     ['$this->mModifierSources.mCapacity = $this->mModifierSources.mCount'], 2,
     'the source vector full from the 3rd call (reserve 10, at most 4 shipped modifiers per cloud)'),
    ('3A88 AddRepulsor full', 'CFishCloud::AddRepulsor(', [],
     ['$this->mModifierSources.mCapacity = $this->mModifierSources.mCount'], 4,
     'the source vector full from the 5th call (reserve 10, at most 4 shipped modifiers per cloud)'),
    ('2BD8 Render inactive', 'CFishCloud::Render(', [],
     ['$this->mBoids.mItems[0].mActive = $this->mThinkCounter > 400 ? 0 : $this->mBoids.mItems[0].mActive',
      '$this->mActive = $this->mThinkCounter > 900 ? 0 : 1'], 0,
     'boid 0 inactive after 400 thinks, the cloud inactive after 900 (Render is only queued while active)'),
]
