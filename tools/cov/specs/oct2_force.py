# OctapedeSegment (REL 46) second pass: the arms an inlined callee's answer decides, forced at that
# callee's return while the body runs (FORCE_INNER of tools/cov/force_ret.py), plus one-call member
# writes (FORCE_RET, restored when the body returns).
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/oct2_force.py <out> 1baa96c2,0p_cliff_hall --con tools/drivers/octapede_cliffhall.con \
#     --fast --frames 3000
CLASS = 'COctapedeSegment'

# fn_46_2924 Think: line 312's `if (mBouncesRemaining <= 0)` true arm (so 313 is read), then line 324's
# `mSegmentState != kSegS_Bouncing` and 348's same test with `!mBrokenApart`.
FORCE_RET = [
    ('2924 Think no bounces', 'COctapedeSegment::Think(', 2,
     [(1, [('$this->mBouncesRemaining', '0')])],
     'mBouncesRemaining 0 so the obstruction test at line 313 is read'),
    ('2924 Think crawling', 'COctapedeSegment::Think(', 4,
     [(3, [('$this->mSegmentState', '0'), ('$this->mBrokenApart', '0')])],
     'not bouncing and not broken apart: lines 324/348 and the death ladder'),
]

# fn_46_2924 Think 0x309 `area.GetOcclusionState() != kOS_Visible`: an occluded area is the arm no
# shipped area gives while a segment thinks (kOS_Visible 0, so 1 is not visible).
# fn_46_14E0 CollidedWith 0x582 `list.GetCount() > 0`: the empty collision list.
# fn_46_30BC AcceptScriptMsg 0x236/0x239: the TCastToConstPtr<COctapedeSegment> null arms.
FORCE_INNER = [
    ('2924 Think occluded', 'COctapedeSegment::Think(', 'CGameArea::GetOcclusionState(', 2,
     {1: [('$rax', '1')]}, 'the area reports a non-visible occlusion state (line 309)'),
    ('14E0 CollidedWith empty list', 'COctapedeSegment::CollidedWith(',
     'CCollisionInfoList::GetCount(', 1, {0: [('$rax', '0')]},
     'an empty collision info list (line 582)'),
    ('30BC AcceptScriptMsg no segment', 'COctapedeSegment::AcceptScriptMsg(',
     'TCastToConstPtr<COctapedeSegment>(', 2, {1: [('$rax', '0')]},
     'the connection target is no COctapedeSegment (lines 236/239)'),
]
