# The forcing spec of the DarkSamus helpers2 lane: the arms the rooftop fight cannot reach.
#
# Run: tools/cov/force.sh tools/cov/specs/darksamus_helpers2_force.py <out> "1baa96c2,10_cliff_rooftop"
#         --con tools/cov/specs/darksamus_rooftop.con --fast --frames 6400
#
# Each entry breaks at the port's body, writes the listed members (so the arm is taken) and
# records the arm keys (the natural arms, before the write).  The run's forced.log says what
# was forced; the note labels those arms "forced".
CLASS = 'CDarkSamus'
FORCE = [
    # the glide emitter: the update and the deactivate without an emitter (the emitter handle
    # cleared).  The "the sfx id is invalid" arm is dead on shipped data (the loader's id).
    ('11334 UpdateGlideEmitter', 'CDarkSamus::UpdateGlideEmitter(', ['$rdx == 0'],
     ['$this->mGlideSfx.mID = 0'], 1, 'the "no emitter" arms'),
    # the safe position: the boss without a safe position (the arm that asks IsInUnsafeSpot).
    ('128D8 UpdateSafePosition', 'CDarkSamus::UpdateSafePosition(', ['$this->mHasSafePos != 0'],
     ['$this->mHasSafePos = 0'], 1, 'the "no safe position" arm'),
    # the fade attacks: the fade-out arm (mFadeOut is set by the PickedNormalMissile trigger).
    ('1A554 CollectFadeAttacks', 'CDarkSamus::CollectFadeAttacks(', ['$this->mFadeOut == 1'],
     ['$this->mFadeOut = 1'], 1, 'the fade-out arm'),
    # the damage: the arm that adds the damage (the health has to drop after the activate).
    ('1E178 AccumulateDamage', 'CDarkSamus::AccumulateDamage(', ['$rsi == 0', '$this->mLastHealth > 0.0'],
     ['$this->mLastHealth = 1.0'], 1, 'the "the health is gone" arm'),
    # the attacker list: the "the uid is invalid" arm.  The member is written in the caller
    # (TravelToCoverPoints reads it and hands it to PushAttacker).
    ('1E26C PushAttacker (via the caller)', 'CDarkSamus::TravelToCoverPoints(', [],
     ['$this->mLastAttackerUid.value = 65535'], 1, 'the "invalid uid" arm'),
    # the dash: the "the actor stood still" and "the dash is stuck" arms.
    ('14FE0 PickNextDash', 'CDarkSamus::PickNextDash(', ['$this->mStillTime > 1.2', '$this->mDashStuckCount >= 2'],
     ['$this->mStillTime = 2.0', '$this->mDashStuckCount = 5'], 1, 'the "stood still" and "stuck" arms'),
    # the gun line (fn_10_164C0): the two outcomes of the ray cast.
    ('164C0 IsGunLineClear|ret', 'CDarkSamus::IsGunLineClear(', [], [], 0, 'the two outcomes'),
    # the aim line (fn_10_16418): the two outcomes.
    ('16418 IsPlayerAimLineClear|ret', 'CDarkSamus::IsPlayerAimLineClear(', [], [], 0, 'the two outcomes'),
]
