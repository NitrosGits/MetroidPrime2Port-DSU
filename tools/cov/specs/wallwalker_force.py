# WallWalker (module 85) forcing spec for tools/cov/force.sh, same area as tools/cov/specs/wallwalker.py
# with its own driver (every walker is killed; merge the gcov dirs in gcov_report.py):
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/wallwalker_force.py <out> \
#     1baa96c2,09_cliff_temple --con tools/cov/specs/wallwalker_force.con --fast --frames 3400  (SECS=1800: the default 900 s is too short)
CLASS = 'CWallWalker'
FORCE = [
    ('18DC LaunchProjectiles 8 shots', 'CWallWalker::LaunchProjectiles(', ['$this->mNumShots'],
     ['$this->mNumShots = 8'], 0,
     'mNumShots 8 (> the 4 live AI projectiles CanCreateProjectile allows): the false arm at 408'),
    ('3178 ShouldShootProjectile shots queued', 'CWallWalker::ShouldShootProjectile(', [],
     ['$this->mNumShots = 2'], 0,
     'mNumShots > 0 (SetNumberShots is only called by the FSM): the early true arm'),
    ('221C DoUserAnimEvent other type', 'CWallWalker::DoUserAnimEvent(', ['$rcx'],
     ['$rcx = 2'], 4,
     'from the 5th event the type is kUE_LoopedSoundStop: the not-handled arm to CPatterned'),
    ('2340 Dead burning', 'CWallWalker::Dead(', [],
     ['$this->mBurning = ($this->mEditorId.value & 0x3ffffff) == 0x25009e ? 1 : $this->mBurning',
      '$this->mIngPossessionTarget = ($this->mEditorId.value & 0x3ffffff) == 0x250156 ? 1.0 : '
      '$this->mIngPossessionTarget',
      '$this->mIngPossessionBlend = ($this->mEditorId.value & 0x3ffffff) == 0x250156 ? 1.0 : '
      '$this->mIngPossessionBlend',
      '$this->mHealthInfo.mDeathWeaponMode.mWeaponType = ($this->mEditorId.value & '
      '0x3ffffff) == 0x25014e || ($this->mEditorId.value & 0x3ffffff) == 0x250156 ? 1 : '
      '$this->mHealthInfo.mDeathWeaponMode.mWeaponType',
      '$this->mHealthInfo.mDeathWeaponMode.mCharged = ($this->mEditorId.value & '
      '0x3ffffff) == 0x25014e ? 1 : (($this->mEditorId.value & 0x3ffffff) == 0x250156 ? 0 : '
      '$this->mHealthInfo.mDeathWeaponMode.mCharged)'], 0,
     'e0025009e burning when it dies: the burning arms of Dead (356) and Think (317); e00250156 '
     'Ing-possessed (target and blend; Death clears the target) and killed by an uncharged dark shot: '
     'the uncharged-dark and DarkXDamage arms (320, 322); e0025014e '
     'killed by a charged dark shot (the console hurt has no charge flag): the IceXDamage arm (320)'),
]
