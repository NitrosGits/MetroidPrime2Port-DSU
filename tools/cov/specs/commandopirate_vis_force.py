# CommandoPirate (module 9) coverage + forcing spec for Think (fn_9_C944): the frozen body without a
# rag doll (the bone tracking skipped, 0x0000C9D8..0x0000C9DC) is forced on every 40th call; the
# inactive early return (0x0000C970..0x0000C974) comes from the core driver's Deactivate (msgs).
# Run: COV_GDB_SCRIPT=tools/cov/cov_force.py tools/cov/force.sh tools/cov/specs/commandopirate_vis_force.py
#      <out> 3bfa3eff,05_temple_federationsite_dark --frames 9000
#      --driver 'python3 tools/drivers/commandopirate_core.py {port} /dev/null x msgs'
# force.py does not gate on exprs; the set carries its own condition (frozen, then thawed again).
CLASS = 'CCommandoPirate'
RD = '$this->xbfc_ragDoll.mPtr'
BC = '$this->mBodyController.mPtr'
SPEC = [
    ('0xC944 Think', 'CCommandoPirate::Think(',
     ['$this->mActive', RD + ' == 0', BC + '->mFrozen'], 'calls max=20000'),
]
FORCE = [
    ('C9DC frozen body, no rag doll (forced)', 'CCommandoPirate::Think(',
     [RD + ' == 0'],
     ['$cpv_n = ($_isvoid($cpv_n) ? 0 : $cpv_n) + 1',
      BC + '->mFrozen = (' + RD + ' != 0 ? ' + BC + '->mFrozen : ($cpv_n % 40 == 0 ? 1 : 0))'], 0,
     "every 40th call without a rag doll the body controller is frozen, so Think skips the bone "
     "tracking (0x0000C9D8..0x0000C9DC); the other calls thaw it again"),
]
