# CommandoPirate (module 9) coverage spec: the 45 ported but unmarked state-machine bodies
# (the `fsm-trigger`/`ported` rows of docs/module-analysis/CommandoPirate_functions.tsv).
# A trigger is recorded with its return value (the true and the false arm), a state with the
# calls it makes (an arm that ends in a call shows as its own key).
# Runs: tools/cov/run.sh tools/cov/specs/commandopirate_triggers.py <out> <world,area>
#        --con tools/cov/specs/commandopirate_triggers_temple.con   (area 59)
#        --con tools/cov/specs/commandopirate_triggers_cliff.con    (the Cliff's entrance)
CLASS = 'CCommandoPirate'
SPEC = [
    ('0x69C8 Dead', 'CCommandoPirate::Dead(', [], 'calls max=3000'),
    ('0x7358 WallHang', 'CCommandoPirate::WallHang(', [], 'calls max=3000'),
    ('0x786C RestoreOrientation', 'CCommandoPirate::RestoreOrientation(', [], 'calls max=3000'),
    ('0x7A98 ScriptedShieldCharge', 'CCommandoPirate::ScriptedShieldCharge(', [], 'calls max=3000'),
    ('0x8934 Dodge', 'CCommandoPirate::Dodge(', [], 'calls max=3000'),
    ('0x8B70 JumpBack', 'CCommandoPirate::JumpBack(', [], 'calls max=3000'),
    ('0x961C FollowAttackPattern', 'CCommandoPirate::FollowAttackPattern(', [], 'calls max=3000'),
    ('0x9AAC PathFind', 'CCommandoPirate::PathFind(', [], 'calls max=3000'),
    ('0xA7A8 SetCoverDest', 'CCommandoPirate::SetCoverDest(', [], 'calls max=3000'),
    ('0xA8E8 SetJumpDest', 'CCommandoPirate::SetJumpDest(', [], 'calls max=3000'),
    ('0xA97C SetRetreatDest', 'CCommandoPirate::SetRetreatDest(', [], 'calls max=3000'),
    ('0xAB08 SelectTarget', 'CCommandoPirate::SelectTarget(', [], 'calls max=3000'),
    ('0xABDC FaceTarget', 'CCommandoPirate::FaceTarget(', [], 'calls max=3000'),
    ('0xAE04 Alert', 'CCommandoPirate::Alert(', [], 'calls max=3000'),
    ('0xAFB8 AbortSeekCover', 'CCommandoPirate::AbortSeekCover(', [], 'ret max=3000'),
    ('0xB0B4 ShouldCoverAttack', 'CCommandoPirate::ShouldCoverAttack(', [], 'ret max=3000'),
    ('0xB0D4 FoundCover', 'CCommandoPirate::FoundCover(', [], 'ret max=3000'),
    ('0xB0F4 ShouldCover', 'CCommandoPirate::ShouldCover(', [], 'ret max=3000'),
    ('0xB114 ShouldWallHang', 'CCommandoPirate::ShouldWallHang(', [], 'ret max=3000'),
    ('0xB168 ShouldCrouch', 'CCommandoPirate::ShouldCrouch(', [], 'ret max=3000'),
    ('0xB1A8 FoundJumpPoint', 'CCommandoPirate::FoundJumpPoint(', [], 'ret max=3000'),
    ('0xB1C8 IsAggressive', 'CCommandoPirate::IsAggressive(', [], 'ret max=3000'),
    ('0xB1D4 AbortShieldCharge', 'CCommandoPirate::AbortShieldCharge(', [], 'ret max=3000'),
    ('0xB250 ShouldShieldCharge', 'CCommandoPirate::ShouldShieldCharge(', [], 'ret max=3000'),
    ('0xB660 ShouldArmShield', 'CCommandoPirate::ShouldArmShield(', [], 'ret max=3000'),
    ('0xB690 ShouldDodge', 'CCommandoPirate::ShouldDodge(', [], 'ret max=3000'),
    ('0xB76C TooClose', 'CCommandoPirate::TooClose(', [], 'ret max=3000'),
    ('0xB7FC AttackPatternOver', 'CCommandoPirate::AttackPatternOver(', [], 'ret max=3000'),
    ('0xB818 IsFacingTarget', 'CCommandoPirate::IsFacingTarget(', [], 'ret max=3000'),
    # the early return: the target has no aim position (the x3CBC_ call failed)
    ('0xB818 IsFacingTarget: no aim', 'loc:PortCCommandoPirate.cpp:2242', [], 'max=3000'),
    # the KnockBack override (the vtable slot 0x9C): the "GetUp" state and the two sounds
    ('0xC300 KnockBack', 'CCommandoPirate::KnockBack(', [], 'calls max=3000'),
    ('0xC300 KnockBack: GetUp', 'loc:PortCCommandoPirate.cpp:1945', [], 'max=3000'),
    ('0xC300 KnockBack: death sound', 'loc:PortCCommandoPirate.cpp:1955', [], 'max=3000'),
    ('0xB900 HasAttackPattern', 'CCommandoPirate::HasAttackPattern(', [], 'ret max=3000'),
    ('0xB990 PathOver', 'CCommandoPirate::PathOver(', [], 'ret max=3000'),
    ('0xB9D8 PathShagged', 'CCommandoPirate::PathShagged(', [], 'ret max=3000'),
    ('0xBA20 HasNewTarget', 'CCommandoPirate::HasNewTarget(', [], 'ret max=3000'),
    ('0xBA78 HasTarget', 'CCommandoPirate::HasTarget(', [], 'ret max=3000'),
    ('0xBAB4 HeardShot', 'CCommandoPirate::HeardShot(', [], 'ret max=3000'),
    ('0xBAD0 UnderFire', 'CCommandoPirate::UnderFire(', [], 'ret max=3000'),
    ('0xBAEC HasLineOfSight', 'CCommandoPirate::HasLineOfSight(', [], 'ret max=3000'),
    ('0xBAF8 BreakAmbush', 'CCommandoPirate::BreakAmbush(', [], 'ret max=3000'),
    ('0xBB04 ShouldAmbush', 'CCommandoPirate::ShouldAmbush(', [], 'ret max=3000'),
    ('0xBB10 ShouldJumpBack', 'CCommandoPirate::ShouldJumpBack(', [], 'ret max=3000'),
    ('0xBB7C ShouldRetreat', 'CCommandoPirate::ShouldRetreat(', [], 'ret max=3000'),
    ('0xBBC8 ShouldFireEGrenade', 'CCommandoPirate::ShouldFireEGrenade(', [], 'ret max=3000'),
    ('0xBCB8 ShouldMeleeAttack', 'CCommandoPirate::ShouldMeleeAttack(', [], 'ret max=3000'),
    ('0xBD98 ShouldWarpIn', 'CCommandoPirate::ShouldWarpIn(', [], 'ret max=3000'),
    ('0xBDCC StateOver', 'CCommandoPirate::StateOver(', [], 'ret max=3000'),
]
