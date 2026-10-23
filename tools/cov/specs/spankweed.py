# SpankWeed (REL 73) forced run; arms come from the coverage build (gcov_report.py), the breakpoints
# only count calls.  Shipped data has no SPNK placement (tools/scan_spnk_props.py) and retail
# registers no SpankWeed state or trigger, so the driver `spawn SPNK`s with a borrowed SpacePirate
# body in Agon Wastes 08_sand_casualpirates.  Writes: an unset knockbackRules -> RULE 393ca543 (as
# chozoghost_force.py), and mIsHiding cleared at Think entry (the eye block retail never reaches).
#   COV_GDB_SCRIPT=tools/cov/cov_force.py BUILD=build-port-cov tools/cov/force.sh \
#     tools/cov/specs/spankweed.py <out> 42b935e4,08_sand_casualpirates \
#     --con tools/drivers/spankweed_forced.con --fast --frames 900
CLASS = 'CSpankWeed'
SPEC = [
    ('1EE4 AcceptScriptMsg', 'CSpankWeed::AcceptScriptMsg(', [], 'max=4000'),
    ('1D48 Think', 'CSpankWeed::Think(', [], 'max=4000'),
    ('0F14 GetAimPosition', 'CSpankWeed::GetAimPosition(', [], 'max=400'),
    ('0DAC GetOrbitPosition', 'CSpankWeed::GetOrbitPosition(', [], 'max=400'),
]
KB = '*(unsigned*)&((SLdrPatternedAITypedef*)$rsi)->knockbackRules'
HIDE = '((CSpankWeed*)$rdi)->mIsHiding'
FORCE = [
    ('LdrToPatternedInfo knockbackRules', 'LdrToPatternedInfo(', [KB],
     ['%s = %s == 0xffffffff ? 0x393ca543 : %s' % (KB, KB, KB)], 0,
     'an unset knockbackRules -> RULE 393ca543 (forced, spawn SPNK)'),
    ('Think !mIsHiding', 'CSpankWeed::Think(', [HIDE], ['%s = 0' % HIDE], 200,
     'the eye block (forced: retail never clears mIsHiding)'),
]
