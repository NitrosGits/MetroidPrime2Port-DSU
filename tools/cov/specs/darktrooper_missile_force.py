# DarkTrooper missile arms (dtr3): no shipped trooper in the areas the harness reaches carries the
# missile flag (mFiresMissiles, byte 0x928 bit 0x40) or a missile projectile, so the flag is written at
# the entry of Think and AcceptScriptMsg, and ProjectileInfo() (called by LaunchProjectile from
# FireMissile) answers with the small-shot projectile (mAttackState 5). The fight is
# tools/drivers/darktrooper_unknown.con. Every hit is logged to forced.log.
#   BUILD=build-port-cov tools/cov/force.sh tools/cov/specs/darktrooper_missile_force.py <out> \
#     3bfa3eff,04_temple_unknownenemies --con tools/drivers/darktrooper_unknown.con --fast --frames 5200
CLASS = 'CDarkTrooper'
FORCE = [
    ('AcceptScriptMsg', 'CDarkTrooper::AcceptScriptMsg(', ['$this->mFiresMissiles'], ['$this->mFiresMissiles = 1'], 0,
     'mFiresMissiles on (no shipped missile trooper loads)'),
    ('Think', 'CDarkTrooper::Think(', ['$this->mFiresMissiles'],
     ['$this->mFiresMissiles = 1', '$this->mMissileAttack.mProjectile.m_valid = 0'], 0,
     'mFiresMissiles on (no shipped missile trooper loads); the empty missile projectile stays invalid outside FireMissile'),
    ('FireMissile', 'CDarkTrooper::FireMissile(', ['$this->mFiresMissiles'],
     ['$this->mMissileAttack.mProjectile.m_valid = 1'], 0,
     'the missile projectile optional reads valid for the gate (its data is never read: ProjectileInfo below)'),
    ('ProjectileInfo', 'CDarkTrooper::ProjectileInfo(', ['$this->mAttackState'],
     ['$this->mAttackState = ($this->mAttackState == 1 ? 5 : $this->mAttackState)'], 0,
     'the missile attack launches the small-shot projectile (the missile one holds no asset)'),
]
