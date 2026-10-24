# Kralee (REL 37) forcing spec for tools/cov/force_ret.py: writes live for exactly one call (restored
# at its return), and a callee's answer is forced at its return for the arms an inline test decides.
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/kral1_force.py <out> 3bfa3eff,0b_temple_hall --con tools/cov/specs/kralee.con \
#     --fast --frames 1300
CLASS = 'CKralee'

# fn_37_244C: 0x2x `if (mWarpInParticleEffect != kInvalidAssetId)` / the out one (lines 87/90): both
# shipped Kralees carry valid ids, so the empty-id arm needs the member cleared at the body entry.
# fn_37_20DC Patrol 0x329 `if (*waypoint != kInvalidUniqueId)`: the no-waypoint arm needs
# mCurrentWaypointId invalid (line 325 then looks the connection up).
# fn_37_13C LoadKralee 0x411 `if (!modelData)`: the loader's optional cleared while the body runs.
FORCE_RET = [
    ('244C warp-in particle id', 'CKralee::CKralee(', 2,
     [(1, [('$this->mWarpInParticleEffect', '0x12345678')])],
     'a valid warp-in particle id arm at line 87 (the shipped ones are invalid)'),
    ('244C warp-out particle id', 'CKralee::CKralee(', 2,
     [(0, [('$this->mWarpOutParticleEffect', '0x12345678')])],
     'a valid warp-out particle id arm at line 90 (the shipped ones are invalid)'),
    ('244C both particle ids', 'CKralee::CKralee(', 4,
     [(3, [('$this->mWarpInParticleEffect', '0x12345678'),
           ('$this->mWarpOutParticleEffect', '0x12345678')])],
     'both particle ids valid on one ctor call'),
    ('20DC Patrol no waypoint', 'CKralee::Patrol(', 4,
     [(1, [('$this->mCurrentWaypointId.value', '0xffff')])],
     'mCurrentWaypointId invalid: the no-waypoint arm at line 329'),
    ('13C LoadKralee no model', 'LoadKralee(', 2,
     [(1, [('modelData.m_valid', '0')])],
     'the modelData optional cleared: the return-null arm at line 411'),
]

# fn_37_1DB8 WarpOut 0x364 `if (mAnimationState.CanIssueCommand(...))`: the true arm needs the answer
# from the inline-reached CTryCommand::ShouldRetry, forced at its return while WarpOut runs.
FORCE_INNER = [
    ('1DB8 WarpOut can issue', 'CKralee::WarpOut(', 'CAnimationState::CanIssueCommand(', 1,
     {0: [('$rax', '1')]}, 'CanIssueCommand answers true: the generate command arm at line 364'),
    ('20DC Patrol no connection', 'CKralee::Patrol(', 'CPatterned::GetConnectedObject(', 1,
     {0: [('$rax', '0xffff')]},
     'GetConnectedObject answers kInvalidUniqueId: the no-waypoint arm at line 329'),
]
