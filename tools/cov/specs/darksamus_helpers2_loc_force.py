# The arms the rooftop fight cannot reach, one forced call each: the members are written at the
# entry of the body, and the arm lines (or the entry's arm keys) record what the force produced.
#
# Run: tools/cov/force.sh tools/cov/specs/darksamus_helpers2_loc_force.py <out>
#         "1baa96c2,10_cliff_rooftop" --con tools/cov/specs/darksamus_rooftop.con --fast --frames 6400
CLASS = 'CDarkSamus'
F = 'loc:PortCDarkSamusFsm.cpp:'
FORCE = [
    # fn_10_1DD14: the "the contact time is fresh" arm (the contact time is the elapsed time of
    # the last contact, so writing it in the future makes the test true).
    ('1DD14 Separate', 'CDarkSamus::Separate(', ['$this->mContactTime + 0.3 > $this->mElapsedTime'],
     ['$this->mContactTime = $this->mElapsedTime + 10.0'], 1, 'the "the contact time is fresh" arm'),
    ('1DD14 Separate: the contact time is fresh', F + '822', [], [], 0, ''),
    ('1DD14 Separate: the repulsion is weak', F + '825', [], [], 0, ''),
    # fn_10_1D860: the "deactivate without an additive animation" arm (the animation id is -1).
    ('1D860 UpdateAdditiveAnimation', 'CDarkSamus::UpdateAdditiveAnimation(',
     ['$rdx == 0', '$this->mAdditiveAnim != -1'], ['$this->mAdditiveAnim = -1'], 1,
     'the "deactivate without an animation" arm'),
    # fn_10_1C6C8: the "the second scatter info" arm (the flag that picks the second info).
    ('1C6C8 ProjectileInfo', 'CDarkSamus::ProjectileInfo(', [],
     ['$this->mScatterUseSecondInfo = 1'], 1, 'the "the second scatter info" arm'),
]
