# Kralee (module 37) forcing spec for tools/cov/force.sh, same area as tools/cov/specs/kralee.py
# (merge the two gcov dirs in gcov_report.py):
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/kralee_force.py <out> \
#     3bfa3eff,0b_temple_hall --con tools/cov/specs/kralee_force.con --fast --frames 1300
CLASS = 'CKralee'
FORCE = [
    ('1BA0 AcceptScriptMsg timers', 'CKralee::AcceptScriptMsg(', [],
     ['$this->mVisibleTime = 1.5', '$this->mInvisibleTime = 1.0'], 0,
     'visibleTime/invisibleTime > 0 (both shipped 0B Kralees have 0): the Create duration arms '
     'and the Think visible/invisible timers run'),
    ('Think disableMove', 'CKralee::Think(', [],
     ['$this->mDisableMove = $this->mThinkCounter == 300 ? 1 : '
      '($this->mThinkCounter == 301 ? 0 : $this->mDisableMove)'], 0,
     'mDisableMove for one Think: AlignToFloor skipped'),
    ('Think alignToFloor', 'CKralee::Think(', [],
     ['$this->mAlignToFloor = $this->mThinkCounter == 400 ? 0 : '
      '($this->mThinkCounter == 401 ? 1 : $this->mAlignToFloor)'], 0,
     'mAlignToFloor clear for one Think: SetMovable(true), AlignToFloor skipped'),
    ('Think frozen', 'CKralee::Think(', [],
     ['$this->mBodyController.mPtr->mFrozenDur = $this->mThinkCounter == 500 ? 1.0 : '
      '($this->mThinkCounter == 501 ? 0.0 : $this->mBodyController.mPtr->mFrozenDur)'], 0,
     'body frozen for one Think (GetPercentageFrozen != 0): AlignToFloor skipped'),
    ('Think occluded', 'CKralee::Think(', [],
     ['((CStateManager*)$rsi)->mWorld.mPtr->mAreas.mItems[$this->mAreaId.value].mItem'
      '->mPostConst.mPtr->mOcclusionState = $this->mThinkCounter == 600 ? 0 : '
      '($this->mThinkCounter == 601 ? 1 : ((CStateManager*)$rsi)->mWorld.mPtr->mAreas'
      '.mItems[$this->mAreaId.value].mItem->mPostConst.mPtr->mOcclusionState)'], 0,
     'the Kralee\'s area occluded for one Think (only 0B is loaded, so no neighbour to move to): '
     'mPlayerObstructed, SetMovable(false), early return'),
]
