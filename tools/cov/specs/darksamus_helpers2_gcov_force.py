# The arms the gcov profile shows as untaken and that a member write can reach, one forced call
# each. The run writes its gcov counters into its own directory, so the report merges them with
# the natural run's (tools/cov/gcov_report.py <build> <dir1> <dir2>).
#
# Run: BUILD=<coverage build> tools/cov/force.sh tools/cov/specs/darksamus_helpers2_gcov_force.py
#         <out> "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_rooftop.con --fast
CLASS = 'CDarkSamus'
FORCE = [
    # fn_10_1DD14: the "the contact time is fresh" arm (the contact time written in the future).
    ('1DD14 Separate', 'CDarkSamus::Separate(', ['$this->mContactTime + 0.3 > $this->mElapsedTime'],
     ['$this->mContactTime = $this->mElapsedTime + 10.0'], 1, 'the "the contact time is fresh" arm'),
    # fn_10_1D860: the "deactivate without an additive animation" arm (the animation id is -1).
    ('1D860 UpdateAdditiveAnimation', 'CDarkSamus::UpdateAdditiveAnimation(',
     ['$rdx == 0', '$this->mAdditiveAnim != -1'], ['$this->mAdditiveAnim = -1'], 1,
     'the "deactivate without an animation" arm'),
    # fn_10_1E740: the "the attacker has no cover point" arm (a uid that has no cover point).
    ('1E740 ShouldForgetLastAttacker', 'CDarkSamus::ShouldForgetLastAttacker(',
     ['$this->mLastAttackerUid.value == 65535'], ['$this->mLastAttackerUid.value = 12345'], 1,
     'the "the attacker has no cover point" arm'),
    # fn_10_1EE4C: the "the hit is too recent" arm (the last hit is now) and the "there is no
    # stage" arm (the battle stage uid is invalid).
    ('1EE4C CanReflectHit', 'CDarkSamus::CanReflectHit(', [],
     ['$this->mGuestC0C = $this->mElapsedTime', '$this->mBattleStageUid.value = 65535'], 1,
     'the "the hit is too recent" and the "there is no stage" arms'),
    # fn_10_1C6C8: the "the second scatter info" arm (the scatter shot is the current attack and
    # the second info is the one to hand out).
    ('1C6C8 ProjectileInfo', 'CDarkSamus::ProjectileInfo(', [],
     ['$this->mCurrentAttack = 8', '$this->mScatterUseSecondInfo = 1'], 1,
     'the "the second scatter info" arm'),
]
