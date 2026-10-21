# CommandoPirate (module 9) core spec for tools/cov/run.sh; labels are the retail offsets, loc lines
# are src/MetroidPrime/Enemies/PortCCommandoPirateCore.cpp's. States key on $rdx (the EStateMsg).
# Run: tools/drivers/commandopirate_core.py in 3bfa3eff,05_temple_federationsite_dark (area 59) and
# in the Swamp's 05_swamp_timber! with its `cover` mode (the only commando areas with cover points).
CLASS = 'CCommandoPirate'
C = 'loc:PortCCommandoPirateCore.cpp:'
SPEC = [
    ('0 GetSearchPath', 'CCommandoPirate::GetSearchPath(', [], 'max=50'),
    ('8 IsListening', 'CCommandoPirate::IsListening(', [], 'max=50'),
    ('10 GetGravityConstant', 'CCommandoPirate::GetGravityConstant(', [], 'max=50'),
    ('68 GetModelAlphau8', 'CCommandoPirate::GetModelAlphau8(', [], 'max=50'),
    ('70 GetPointSearchPath', 'CCommandoPirate::GetPointSearchPath(', [], 'max=50'),
    ('78 GetRagDoll', 'CCommandoPirate::GetRagDoll(', [], 'max=50'),
    ('80 GetAttackTarget', 'CCommandoPirate::GetAttackTarget(', [], 'max=50'),
    ('A4 CanBeShot', 'CCommandoPirate::CanBeShot(', [], 'max=50'),
    ('AC GetOrigin', 'CCommandoPirate::GetOrigin(', [], 'max=50'),
    ('D474 ProjectileInfo', 'CCommandoPirate::ProjectileInfo(', [], 'max=50'),
    ('BDE0 SetupStateMachine', 'CCommandoPirate::SetupStateMachine(', [], 'calls'),
    ('C4FC AddToRenderer', 'CCommandoPirate::AddToRenderer(', [], 'calls max=50'),
    ('C2CC GetDamageVulnerability', 'CCommandoPirate::GetDamageVulnerability(',
     ['($this->xc65_ & 4) != 0'], 'max=4000'),
    # Listen: type ($rcx) and alive; the two "heard" lines
    ('BE4C Listen', 'CCommandoPirate::Listen(', ['$rcx', '$this->mAlive'], 'ret'),
    ('BE4C Listen: type 4 heard', C + '88', [], ''),
    ('BE4C Listen: type 0 heard', C + '104', [], ''),
    ('BE4C Listen: height range', 'loc:PortCCommandoPirateCore.cpp:100', ['this->mDetectionHeightRange != 0'], ''),
    ('AFD4 CoverBlown', 'CCommandoPirate::CoverBlown(', ['$this->xbd2_.value != 0xffff'], 'ret calls'),
    ('A4BC MeleeAttack', 'CCommandoPirate::MeleeAttack(', ['$rdx', '$this->xbc4_teamId.value != 0xffff'], 'calls'),
    ('A2EC EGrenadeAttack', 'CCommandoPirate::EGrenadeAttack(', ['$rdx'], 'calls max=2000'),
    ('A110 PostEGrenadeAttack', 'CCommandoPirate::PostEGrenadeAttack(', ['$rdx'], 'calls max=2000'),
    ('A110 PostEGrenadeAttack: pause over', C + '243', [], ''),
    ('A110 PostEGrenadeAttack: no aim', C + '247', [], ''),
    ('A110 PostEGrenadeAttack: no target', C + '238', [], ''),
    ('9BB4 JetBoost', 'CCommandoPirate::JetBoost(', ['$rdx', '$this->mAnimationState.mState'], 'calls max=2000'),
    ('9BB4 JetBoost: turning', C + '304', [], ''),
    ('9064 WarpIn', 'CCommandoPirate::WarpIn(', ['$rdx', '$this->x920_'], 'calls max=3000'),
    ('9064 WarpIn: waypoint kept', C + '369', [], ''),
    ('9064 WarpIn: no collision', C + '386', [], ''),
    ('9064 WarpIn: anim time kept', C + '395', [], ''),
    ('9064 WarpIn: fade clamped', C + '399', [], ''),
    ('9064 WarpIn: fade', C + '401', [], 'max=2000'),
    ('7724 Crouch', 'CCommandoPirate::Crouch(', ['$rdx', '$this->xbd2_.value != 0xffff'], 'calls max=2000'),
    ('6DBC Cover', 'CCommandoPirate::Cover(', ['$rdx', '$this->xbd2_.value != 0xffff'], 'calls max=2000'),
    ('6DBC Cover: slide', C + '482', [], ''),
    ('6C68 CoverAttack', 'CCommandoPirate::CoverAttack(', ['$rdx', '$this->xbd2_.value != 0xffff'], 'calls max=2000'),
    ('6C68 CoverAttack: slide', C + '516', [], ''),
    ('6A40 GetUp', 'CCommandoPirate::GetUp(', ['$rdx'], 'calls max=2000'),
    ('6A40 GetUp: pending death', C + '543', [], ''),
]
