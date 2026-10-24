# Grenchler (module 27) slice gren4: arms no fight reaches, forced for one call each
# (tools/cov/force_ret.py). Entrance driver (u05c4, nav mesh, jump points), merge with the gren3 runs:
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 \
#     tools/cov/force.sh tools/cov/specs/grenchler_force4.py <out> 3dfd2249,01_swamp_entrance \
#     --con tools/cov/specs/grenchler_gren4_ent.con --fast --frames 1100
# x86-64 SysV entry registers: rdi this, esi/edx/ecx/r8d/r9d the CAssetId arguments of the ctors.
CLASS = 'CGrenchler'
INV = '0xffffffff'
REGION1 = '(long)&$this->GetPathArea(*$mgr)->mRegions.mData[1]'
REGION0 = '(long)&$this->GetPathArea(*$mgr)->mRegions.mData[0]'
FORCE_RET = [
    # first construction only (n % 1000 == 1): an invalid asset id, written for good (the id is read once)
    ('SEffectD invalid effect', 'CGrenchler::SEffectD::SEffectD(', 1000, [(1, [('!$esi', INV)])],
     'water splash effect id invalid: no CElementGen'),
    ('SEffectA invalid hitFx+eyeGlow', 'CGrenchler::SEffectA::SEffectA(', 1000,
     [(1, [('!$r8d', INV), ('!$r9d', INV)])],
     'hit effect and eye glow ids invalid: no optional token, no eye CElementGen'),
]
# PathExists(waypoint, target) always fails naturally (110 of 110 hits: +10000 cost); every PathExists inside
# FindJumpPoint answers kR_Success (0), so the second one takes the other arm (no +10000)
FORCE_INNER = [
    ('FindJumpPoint PathExists succeeds', 'CGrenchler::FindJumpPoint(', 'CPathFindSearch::PathExists(', 1,
     {0: [('$rax', '0')]}, 'both PathExists calls of a jump point iteration succeed: no +10000 cost'),
]
CALLS = [('MarkPathRegion direct calls', 'CGrenchler::MarkPathRegion(',
          '((CGrenchler*)$rdi)->mPathFindSearch.mWaypoints.mCount > 1', [
    # every FindClosestRegion answers null: lastRegion null, region null
    ([], '$this->MarkPathRegion(*$mgr)', [('CPFArea::FindClosestRegion(', 0)]),
    # every FindClosestRegion answers region 0: region == lastRegion
    ([], '$this->MarkPathRegion(*$mgr)', [('CPFArea::FindClosestRegion(', REGION0)]),
    # lastRegion = region 0, region = region 1 whose avoidance flags are set: flags != 0
    ([('$this->GetPathArea(*$mgr)->mRegions.mData[1].mData->mAvoidanceFlags', '1')],
     '$this->MarkPathRegion(*$mgr)', [('CPFArea::FindClosestRegion(', [REGION0, REGION1])]),
    # lastRegion = region 0, region = region 1, flags 0: the region is marked
    ([], '$this->MarkPathRegion(*$mgr)', [('CPFArea::FindClosestRegion(', [REGION0, REGION1])]),
], 'MarkPathRegion with FindClosestRegion stubbed')]
