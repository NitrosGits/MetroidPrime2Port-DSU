# MediumIng (module 41) states spec (lane mi3) for tools/cov/run.sh; labels are the retail offsets,
# key $rdx = the state's EStateMsg (0 Activate, 1 Update, 2 Deactivate). Natural play driver:
#   tools/cov/run.sh tools/cov/specs/mediuming_states.py <out> 3bfa3eff,02_temple_exploration_dark \
#     --con tools/cov/specs/mediuming_play.con --fast --frames 6000 --env MP2_GODMODE=1
CLASS = 'CMediumIng'
_S = [('7394', 'SubStart'), ('72C4', 'PathFind'), ('71BC', 'FollowAttackPattern'),
      ('70B4', 'FollowApproachPattern'), ('5878', 'MistIn'), ('4B8C', 'MistOut'),
      ('3F3C', 'ArmAttack'), ('3AE4', 'MeleeAttack'), ('37B0', 'MistAttack'), ('5728', 'Jump'),
      ('4FE8', 'Dash'), ('4E1C', 'Taunt'), ('1E78', 'WaitForLocomotion'), ('5724', 'Idle'),
      ('542C', 'SafezoneReaction'), ('5240', 'MoveToSafeZone'), ('2E70', 'BackUp'),
      ('30B0', 'Dead'), ('30AC', 'SelectTargetState')]
SPEC = [('%s %s' % (o, n), 'CMediumIng::%s(' % n, ['$rdx'], 'max=20000') for o, n in _S] + [
    ('698C SelectTarget', 'CMediumIng::SelectTarget(', [], ''),
    ('6880 SetLeashTarget', 'CMediumIng::SetLeashTarget(', [], ''),
    ('2564 SetRetreatDestination', 'CMediumIng::SetRetreatDestination(', [], ''),
    ('2A74 SetMeshPathDestination', 'CMediumIng::SetMeshPathDestination(', [], ''),
    ('261C SetDashDestination', 'CMediumIng::SetDashDestination(', [], ''),
    ('13E4 FindSafeZoneRetreatPoint', 'CMediumIng::FindSafeZoneRetreatPoint(', [], 'ret'),
    ('1BA8 FindClosestRetreatActor', 'CMediumIng::FindClosestRetreatActor(', [], ''),
    ('1CE8 FindClosestDangerActor', 'CMediumIng::FindClosestDangerActor(', [], ''),
    ('4308 ExtendTentacle', 'CMediumIng::ExtendTentacle(', ['$rdx'], ''),
    ('43E8 RetractTentacles', 'CMediumIng::RetractTentacles(', [], ''),
    ('4470 PositionTentacles', 'CMediumIng::PositionTentacles(', [], 'max=4000'),
    ('4598 SetTentaclesActive', 'CMediumIng::SetTentaclesActive(', ['$rdx & 0xff'], ''),
    ('2024 CalculateSeparation', 'CMediumIng::CalculateSeparation(', [], 'max=4000'),
    ('1B18 Death', 'CMediumIng::Death(', ['$this->mAlive'], ''),
    # Death's caller (gdb anchors the regex at the name's start, hence '.*'): ApplyLocalDamage (hp <= 0), a knockback follow-up, CPatterned::Think (pending).
    ('1B18 Death callers', 'CMediumIng::Death(', ['$this->mAlive', '$_any_caller_matches(".*ApplyLocalDamage", 4)',
      '$_any_caller_matches(".*KnockBack", 4)', '$_any_caller_matches(".*CPatterned::Think", 4)',
      '$_any_caller_matches(".*AcceptScriptMsg", 4)'], ''),
    ('6078 UpdateMistState', 'CMediumIng::UpdateMistState(', ['$this->mMistState'], 'max=20000'),
    ('1680 UpdateMistEffect', 'CMediumIng::UpdateMistEffect(', ['$this->mC0'], 'max=20000'),
    ('5E0C IsBlocked', 'CMediumIng::IsBlocked(', [], 'ret max=4000'),
    ('T 9A34 Extend', 'CMediumIngTentacle::Extend(', [], ''),
    ('T 91B0 Touch', 'CMediumIngTentacle::Touch(', ['((CMediumIngTentacle*)$rdi)->mCanDamage'], 'max=4000'),
    ('761C Touch', 'CMediumIng::Touch(', ['$this->mD2', '$this->mD3'], 'max=4000'),
]
