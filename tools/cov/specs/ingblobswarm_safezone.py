# IngBlobSwarm (module 31) UpdateBoid's safe-zone kill (0xB00 body, the PointIsInSafeZone arm):
# no IBSM placement is near a safe zone, so the flag is set for one UpdateBoid call in 40 and the
# safe-zone test answers true on one call in 4 of those. COV_FORCE=novuln instead clears the flag
# (every placement sets it) for one call in 40. Writes are undone when the call returns.
#   [COV_FORCE=novuln] COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov tools/cov/force.sh \
#     tools/cov/specs/ingblobswarm_safezone.py <out> 1baa96c2,0b_cliff_hall_dark \
#     --con tools/cov/specs/ingblobswarm_cliff.con --fast --frames 2600
import os

CLASS = 'CIngBlobSwarm'
NOVULN = os.environ.get('COV_FORCE') == 'novuln'
FORCE_RET = [
    ('B00 UpdateBoid vulnerable', 'CIngBlobSwarm::UpdateBoid(', 40,
     [(0, [('$this->mVulnerableToSafeZone', '0' if NOVULN else '1')])],
     'mVulnerableToSafeZone %s for the call' % ('cleared' if NOVULN else 'set')),
]
FORCE_INNER = [] if NOVULN else [
    ('B00 UpdateBoid in safe zone', 'CIngBlobSwarm::UpdateBoid(', 'CSafeZoneManager::PointIsInSafeZone(', 4,
     {0: [('$rax', '1')]}, 'PointIsInSafeZone answers true: the boid is killed (KillBoid, no weapon)'),
]
