# PirateRagDoll (module 50) forcing spec for tools/cov/force.sh. PRD_F picks one set (one run each,
# since every set runs on every hit after `skip`):
#   PRD_F=attach BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/pirateragdoll_force.py <out> \
#     3bfa3eff,0e_temple_hive_hall --frames 12000 \
#     --driver 'python3 tools/drivers/pirateragdoll_drive.py {port} darktrooper 2 30'
# (PRD_F=pins: 3bfa3eff,01_temple_hive_04 and layer `-`, where the DKTR ragdolls have 6 waypoints).
import os

CLASS = 'CPirateRagDoll'
IDS = '((unsigned short*)$this->mWaypointIds.mData)'
ACT = '((bool*)$this->mWaypointActive.mData)'
SETS = {
    'attach': [
        ('217 Update attached', 'CPirateRagDoll::Update(', ['$this->mOver'], ['$this->mActorAttached = 1'], 0,
         'mActorAttached set (pirateattach only reaches a live pirate): the shoulder/neck z fix-ups'),
        ('369 PreRender attached', 'CPirateRagDoll::PreRender(', [], ['$this->mActorAttached = 1'], 0,
         'mActorAttached set: the neck rotation arm'),
        ('400 UpdateImpactSfx no init', 'CPirateRagDoll::UpdateImpactSfx(', [], ['$this->mInitSfx = 0'], 0,
         'mInitSfx clear: the moved-since-last-thud test decides'),
    ],
    'hit': [
        ('259 Update over+hit', 'CPirateRagDoll::Update(', ['$this->mOver'],
         ['$this->mOver ? ($this->mHitByProjectile = 1) : 0'], 0, 'a projectile hit on a settled ragdoll'),
    ],
    'water': [
        ('261 Update over+water', 'CPirateRagDoll::Update(', ['$this->mOver'],
         ['$this->mOver ? ($this->mPrevWaterTop = 1000000.0) : 0'], 0, 'the water top moved under a settled ragdoll'),
    ],
    'impulse': [
        ('268 Update over+impulse', 'CPirateRagDoll::Update(', ['$this->mOver'],
         ['$this->mOver ? ($this->mTorsoImpulse.mX = 50.0) : 0'], 0, 'a torso impulse on a settled ragdoll'),
    ],
    'pins': [
        ('279 Update pins', 'CPirateRagDoll::Update(', ['$this->mOver', '(int)$this->mWaypointIds.mCount'],
         ['$this->mOver ? (' + IDS + '[0] = $this->mActor->mUniqueId.value) : 0',
          '$this->mOver ? (' + IDS + '[1] = 0xffff) : 0',
          '$this->mOver ? (' + ACT + '[1] = 1) : 0',
          '$this->mOver ? (' + IDS + '[2] = 0xffff) : 0',
          '$this->mOver ? (' + ACT + '[2] = 0) : 0',
          '$this->mOver ? (' + IDS + '[3] = $this->mActor->mUniqueId.value) : 0',
          '$this->mOver ? (' + ACT + '[3] = 1) : 0'], 0,
         'settled ragdoll: waypoints 0/3 = the (active) owner actor (3 already marked active), 1/2 = no object (1 marked active)'),
    ],
    'inactive': [
        # the owner actor stands in for an inactive waypoint during one settled Update: inactive
        # at the entry, active again after the activeWaypoints loop (cpp:309, $prda keeps it)
        ('280 Update inactive', 'CPirateRagDoll::Update(', ['$this->mOver'],
         ['$prda = $this->mActor',
          '$this->mOver ? (' + IDS + '[0] = $this->mActor->mUniqueId.value) : 0',
          '$this->mOver ? (' + IDS + '[3] = $this->mActor->mUniqueId.value) : 0',
          '$this->mOver ? (' + ACT + '[0] = 1) : 0',
          '$this->mOver ? (' + ACT + '[3] = 0) : 0',
          '$this->mOver ? ($prda->mActive = 0) : 0'], 0,
         'settled ragdoll: waypoints 0/3 = the owner actor made inactive (0 marked active, 3 not)'),
        ('309 Update reactivate', 'loc:CPirateRagDoll.cpp:309', [], ['$prda->mActive = 1'], 0,
         'the owner actor active again'),
    ],
}
FORCE = SETS[os.environ.get('PRD_F', 'attach')]
