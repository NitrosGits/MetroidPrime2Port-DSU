# DigitalGuardian (module 14), dg22: the gun sweep's X-column fallback (fn_14_10798 at 0x10920:
# the locator 0x10A4 forced invalid, with digitalguardian_gun_force.con placing the head on the
# player) and the machine-gun bullet's TryCache-false arm (fn_14_101E0 at 0x1020C..0x10258). The gun's weapon is locked in the ctor, so a fight never misses it; the
# forcing drops the cached pointer and the lock flag at the bullet's entry (it stays uncached for
# the rest of the run, so this run's gcov merges with the natural digitalguardian_gun.con run's).
#
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_gun_force.py \
#        <out> 1baa96c2,09_cliff_temple_dark --con tools/cov/specs/digitalguardian_gun_force.con \
#        --fast --frames 300
CLASS = 'CDigitalGuardianHead'
SPEC = []
FORCE = [
    ('10798 the gun locator is invalid', 'CDigitalGuardianHead::x10798_(', [],
     ['$this->x10A4_.mId = 0xFF'], 0, 'the gun locator (x10A4_) invalid: GetLctrTransform is the head transform'),
    ('101E0 the gun weapon is not cached', 'CDigitalGuardianHead::x101E0_(', [],
     ['$this->xE7C_.mWeaponDescription.mItem = 0', '$this->xE7C_.mWeaponDescription.mLockHeld = 0'],
     0, "clears the gun weapon's cached pointer and lock flag (xE7C_)"),
]
