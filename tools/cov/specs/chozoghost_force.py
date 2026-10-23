# ChozoGhost (REL 8) forced run: chozoghost.py's SPEC plus one write at load.  `spawn CHOG` leaves
# the PatternedAITypedef knockbackRules at its default -1; that RULE is missing, reads as zeros and
# its parent rule 0 recurses until the heap runs out (CAiKnockBackMgr in the CPatterned ctor).
# The write gives an unset knockbackRules the RULE 393ca543 (in every Metroid*.pak) instead.
#   COV_GDB_SCRIPT=tools/cov/cov_force.py BUILD=build-port-cov tools/cov/force.sh \
#     tools/cov/specs/chozoghost_force.py <out> 42b935e4,08_sand_casualpirates \
#     --con tools/drivers/chozoghost_forced.con --fast --frames <n>
import os
exec(open(os.path.join(os.path.dirname(os.environ['COV_SPEC']), 'chozoghost.py')).read())
KB = '*(unsigned*)&((SLdrPatternedAITypedef*)$rsi)->knockbackRules'
FORCE = [
    ('LdrToPatternedInfo knockbackRules', 'LdrToPatternedInfo(', [KB],
     ['%s = %s == 0xffffffff ? 0x393ca543 : %s' % (KB, KB, KB)], 0,
     'an unset knockbackRules -> RULE 393ca543 (forced, spawn CHOG)'),
]
