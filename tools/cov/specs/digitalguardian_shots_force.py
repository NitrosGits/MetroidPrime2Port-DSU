# DigitalGuardian (module 14), dg20: the spawners' TryCache-false arms (fn_14_10CA0 at
# 0x10CC8..0x10D18, fn_14_10A2C at 0x10A58..0x10AA8). The weapons are locked and loaded from
# the ctor on, so a fight never misses one; the forcing drops the cached pointer and the lock
# flag at each spawner's entry (the lock is not taken again: these weapons stay uncached for the
# rest of the run, so the run is a separate one whose gcov merges with digitalguardian_shots.con's).
#
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_shots_force.py \
#        <out> 1baa96c2,09_cliff_temple_dark --con tools/cov/specs/digitalguardian_shots.con \
#        --fast --frames 420
CLASS = 'CDigitalGuardianHead'
SPEC = []
FORCE = [
    ('10CA0 the charge weapon is not cached', 'CDigitalGuardianHead::x10CA0_(', [],
     ['$this->xE2C_.mWeaponDescription.mItem = 0', '$this->xE2C_.mWeaponDescription.mLockHeld = 0'],
     0, "clears the charge weapon's cached pointer and lock flag (xE2C_)"),
    ('10A2C the missile weapon is not cached', 'CDigitalGuardianHead::x10A2C_(', [],
     ['$this->xE54_.mWeaponDescription.mItem = 0', '$this->xE54_.mWeaponDescription.mLockHeld = 0'],
     0, "clears the missile weapon's cached pointer and lock flag (xE54_)"),
]
