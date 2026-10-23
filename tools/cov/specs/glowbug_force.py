# Glowbug (module 26) forcing spec for tools/cov/force.sh, same driver as tools/cov/specs/glowbug.py
# (merge the gcov dirs in gcov_report.py):
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/glowbug_force.py <out> \
#     863fcd72,0i_temple_hall --con tools/cov/specs/glowbug_templehall.con --fast --frames 2600
# No shipped dark area places a GBUG (tools/scan_area_actors.py GBUG), so the dark arms of Death
# and Dead are forced: mIsInDarkWorld set at their entry (AcceptScriptMsg AreaLoaded writes it
# from mgr.GetIsDarkWorld). mOnGround (CPatterned::IsOnGround) stays false on the first Dead
# Update (the death flash not yet played) and is set from the second, so the not-landed arm, the
# break-apart arm and the timer arm all run.
CLASS = 'CGlowbug'
FORCE = [
    ('151C Death dark', 'CGlowbug::Death(', [], ['$this->mIsInDarkWorld = 1', '$this->mAttackPhase = 2'], 0,
     'the DeathGlow arm; phase 2 as when killed mid-attack (only Patrol clears it): Think/Render with mAlive false'),
    ('10D4 Dead dark', 'CGlowbug::Dead(', [],
     ['$this->mIsInDarkWorld = 1', '$this->mOnGround = $this->mDeathFlashPlayed ? 1 : 0'], 0,
     'mIsInDarkWorld: the dark material set, then not landed, break apart (DGNR) and the timer'),
    ('F7C FindAttackTarget ranges', 'CGlowbug::FindAttackTarget(', ['$this->mMinAttackRange', '$this->mMaxAttackRange'], [], 0,
     'the attack range window (read only)'),
]
