# SpacePirate (module 72) coverage spec for the stateless helper lane
# (tools/cov/run.sh with tools/cov/specs/spacepirate_helpers.py; the driver is
# tools/drivers/spacepirate_helpers_drive.py). It is the module's own spec
# (decomp/spacepirate:tools/cov/specs/spacepirate.py) plus the helper half's bodies; the labels
# are the retail offsets.
CLASS = 'CSpacePirate'
SPEC = [
    ('0 IsScanVisorSelfRender', 'CSpacePirate::IsScanVisorSelfRender(', [], ''),
    ('8 GetSearchPath', 'CSpacePirate::GetSearchPath(', [], ''),
    ('10 IsListening', 'CSpacePirate::IsListening(', [], ''),
    ('18 GetAttackTarget', 'CSpacePirate::GetAttackTarget(', [], ''),
    ('24 GetGravityConstant', 'CSpacePirate::GetGravityConstant(', [], ''),
    ('140 loader', 'PortLoadSpacePirate(', [], ''),
    ('17A0 GetRagDoll', 'CSpacePirate::GetRagDoll(', [], ''),
    ('18BC TryToBeCaptured', 'CSpacePirate::TryToBeCaptured(', [], ''),
    ('3220 ShouldFrenzy', 'CSpacePirate::ShouldFrenzy(', [], 'ret'),
    ('4F0C ProjectileInfo', 'CSpacePirate::ProjectileInfo(', ['$this->mProjectileInfo.m_valid'], ''),
    ('51C8 TrooperWarpOut', 'CSpacePirate::TrooperWarpOut(', [], ''),
    ('5610 Captured', 'CSpacePirate::Captured(', ['$rdx'], ''),
    ('58D8 LaunchGrenade', 'CSpacePirate::LaunchGrenade(', ['$rdx'], ''),
    ('5BF4 Deactivate', 'CSpacePirate::Deactivate(', ['$rdx'], ''),
    ('6328 BreakAttack', 'CSpacePirate::BreakAttack(', [], 'ret'),
    ('686C ShouldJumpBack', 'CSpacePirate::ShouldJumpBack(',
     ['$this->x8f4_28_noShuffleCloseCheck', '$this->xbac_holdPositionTime > 6'], 'ret'),
    ('689C AnimOver', 'CSpacePirate::AnimOver(', ['$this->x8f7_29_inWallHang'], 'ret'),
    # the wall-hang arm (branchless in retail and on the host): entries keyed by the bit, uncapped
    # because the capped entry above fills long before a forced WallHang
    ('689C AnimOver: in wall hang', 'CSpacePirate::AnimOver(', ['$this->x8f7_29_inWallHang'], 'max=1000000'),
    ('69AC WallHang', 'CSpacePirate::WallHang(', ['$rdx'], ''),
    ('7078 WarpIn', 'CSpacePirate::WarpIn(', ['$rdx'], ''),
    ('77D0 ShouldWarpIn', 'CSpacePirate::ShouldWarpIn(',
     ['$this->xc04_warpPoint', '$this->x8f6_24_trooper', '$this->x8fa_26_'], 'ret'),
    ('8464 TurnAround', 'CSpacePirate::TurnAround(', ['$rdx'], ''),
    ('8670 Shuffle', 'CSpacePirate::Shuffle(', ['$rdx'], ''),
    ('8A84 InRange', 'CSpacePirate::InRange(', [], 'ret'),
    ('9478 Approach', 'CSpacePirate::Approach(', ['$rdx'], ''),
    ('98D8 Crouch', 'CSpacePirate::Crouch(', ['$rdx', '$this->x900_coverPoint.value'], ''),
    ('99F0 DoubleSnap', 'CSpacePirate::DoubleSnap(', ['$rdx'], ''),
    ('A310 CoverAttack', 'CSpacePirate::CoverAttack(', ['$rdx'], ''),
    ('A3C0 Cover', 'CSpacePirate::Cover(', ['$rdx'], ''),
    ('A71C ShouldAttack', 'CSpacePirate::ShouldAttack(', [], 'ret'),
    ('A8D0 CoveringFire', 'CSpacePirate::CoveringFire(', [], 'ret'),
    ('A988 CoverNearlyBlown', 'CSpacePirate::CoverNearlyBlown(', [], 'ret'),
    ('AA64 CoverBlown', 'CSpacePirate::CoverBlown(', [], 'ret'),
    ('ADB4 CoverCheck', 'CSpacePirate::CoverCheck(', [], 'ret'),
    ('B3D4 GetUp', 'CSpacePirate::GetUp(', ['$rdx'], ''),
    ('BBB0 Halt', 'CSpacePirate::Halt(', ['$rdx'], ''),
    ('C1A4 Generate', 'CSpacePirate::Generate(', ['$rdx'], ''),
    ('C468 PatternShagged', 'CSpacePirate::PatternShagged(', [], 'ret'),
    ('C488 Stuck', 'CSpacePirate::Stuck(', [], 'ret'),
    ('C488 Stuck: CPatterned::Stuck', 'CPatterned::Stuck(', [], 'ret caller=CSpacePirate::Stuck'),
    ('C488 Stuck: PathShagged', 'CPatterned::PathShagged(', [], 'ret caller=CSpacePirate::Stuck'),
    ('C9DC SetNonCinematicCollision', 'CSpacePirate::SetNonCinematicCollision(', [], ''),
    ('CA94 SetCinematicCollision', 'CSpacePirate::SetCinematicCollision(', [], ''),
    ('CE30 GetStrafeDir', 'CSpacePirate::GetStrafeDir(', [], ''),
    # DE5C/DFD4/EFD4 end in tail calls (jmp), so their arms are read off the call sites ('calls'):
    # the cast-null arm is the TCastToPtr count minus the count of the call that follows it
    ('DE5C UpdateTeamTarget', 'CSpacePirate::UpdateTeamTarget(', ['$this->xba0_teamAiMgrId.value != 0xffff'], 'calls max=20000'),
    ('DFD4 SquadJoin', 'CSpacePirate::SquadJoin(', ['$this->xba0_teamAiMgrId.value != 0xffff', '$this->x8f4_27_melee'], 'calls'),
    ('EFD4 ChooseTeamTarget', 'CSpacePirate::ChooseTeamTarget(', ['$this->xba0_teamAiMgrId.value != 0xffff'], 'calls'),
    ('E090 GetOrigin', 'CSpacePirate::GetOrigin(', [], ''),
    ('E3D8 Touch', 'CSpacePirate::Touch(', [], ''),
    ('EDEC ChooseTarget', 'CSpacePirate::ChooseTarget(', [], ''),
    ('F49C ctor', 'CSpacePirate::CSpacePirate(', [], ''),
    ('2EA8/1854/others: GetTouchBounds', 'CSpacePirate::GetTouchBounds(', [], ''),

# --- the stateless helper half (PortCSpacePirateHelpers.cpp) ---
    ('0 IsScanVisorSelfRender: helper', 'CSpacePirate::IsScanVisorSelfRender(', [], 'ret'),
    ('18 GetAttackTarget: helper', 'CSpacePirate::GetAttackTarget(', [], 'ret'),
    ('24 GetGravityConstant: helper', 'CSpacePirate::GetGravityConstant(', [], ''),
    ('17A0 GetRagDoll: helper', 'CSpacePirate::GetRagDoll(', [], 'retp'),
    ('1854 GetActorTouchBounds', 'CSpacePirate::GetActorTouchBounds(', [], ''),
    ('2EA8 UpdateSfxEmitter', 'CSpacePirate::UpdateSfxEmitter(', [], ''),
    ('3220 ShouldFrenzy: helper', 'CSpacePirate::ShouldFrenzy(', [], 'ret'),
    ('51C8 TrooperWarpOut: helper', 'CSpacePirate::TrooperWarpOut(', ['$rdx'], ''),
    ('C9DC SetNonCinematicCollision: helper', 'CSpacePirate::SetNonCinematicCollision(', [], ''),
    ('CA94 SetCinematicCollision: helper', 'CSpacePirate::SetCinematicCollision(', [], ''),
    ('CE30 GetStrafeDir: helper', 'CSpacePirate::GetStrafeDir(', [], ''),
    ('D608 ShuffleRandomVector', 'CSpacePirate::ShuffleRandomVector(', [], ''),
    ('E3D8 Touch: helper', 'CSpacePirate::Touch(', [], ''),
    ('EDEC ChooseTarget: helper', 'CSpacePirate::ChooseTarget(', [], ''),
    ('EFD4 ChooseTeamTarget: helper', 'CSpacePirate::ChooseTeamTarget(', [], 'calls'),
    ('4174 SpawnGrenade: helper', 'CSpacePirate::SpawnGrenade(', [], ''),
    ('18BC TryToBeCaptured: helper', 'CSpacePirate::TryToBeCaptured(', [], 'ret'),
    # The same bodies with the call sites counted, so the arms that end in a call are visible
    # (the 'calls' flag counts every call/jmp site in the body by its callee).  On this build the
    # call-site breakpoints of one label also count the calls of the neighbouring functions, so
    # these numbers are only trustworthy in a run whose spec holds the one entry (that is how the
    # emitter's arms were counted).
    ('2EA8 UpdateSfxEmitter: calls', 'CSpacePirate::UpdateSfxEmitter(', ['$this->xc00_.mID != 0'],
     'calls max=1000000'),
    ('3220 ShouldFrenzy: calls', 'CSpacePirate::ShouldFrenzy(', [], 'calls max=1000000'),
    ('51C8 TrooperWarpOut: calls', 'CSpacePirate::TrooperWarpOut(', ['$rdx'], 'calls max=1000000'),
    ('CE30 GetStrafeDir: calls', 'CSpacePirate::GetStrafeDir(', [], 'calls max=1000000'),
    ('4174 SpawnGrenade: calls', 'CSpacePirate::SpawnGrenade(', [], 'calls max=1000000'),
    ('EDEC ChooseTarget: calls', 'CSpacePirate::ChooseTarget(', [], 'calls max=1000000'),
    ('EFD4 ChooseTeamTarget: calls', 'CSpacePirate::ChooseTeamTarget(', [], 'calls max=1000000'),
    ('D608 ShuffleRandomVector: calls', 'CSpacePirate::ShuffleRandomVector(', [], 'calls max=1000000'),
]
