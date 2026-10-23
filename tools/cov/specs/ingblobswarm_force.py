# IngBlobSwarm (module 31) forcing specs for tools/cov/force.sh; arms come from the coverage build
# (gcov_report.py). Sanctuary 0b_cliff_hall_dark L4 (attackerCount 5, attackTimer 0.5):
#   BUILD=$PWD/build-port-cov COV_FORCE=timer tools/cov/force.sh tools/cov/specs/ingblobswarm_force.py <out> \
#     1baa96c2,0b_cliff_hall_dark --con tools/cov/specs/ingblobswarm_cliff.con --fast --frames 2600
#   (COV_FORCE=other for the second set)
import os

CLASS = 'CIngBlobSwarm'
WHICH = os.environ.get('COV_FORCE', 'timer')
if WHICH == 'timer':
    FORCE = [
        ('1744 Think attackTimer', 'CIngBlobSwarm::Think(', [], ['$this->mAttackTimer = 0.0'], 0,
         'attackTimer 0 (every placement has 0.5): several boids start an attack in one Think, so the '
         'slots run out (attackers >= count, slot in use, no free slot) and a winding-up boid can '
         'touch the player (KillBoid with a slot)'),
    ]
else:
    FORCE = [
        ('1744 Think attackerCount', 'CIngBlobSwarm::Think(', [], ['$this->mAttackerCount = 0'], 0,
         'attackerCount 0 (every placement has 1 or 5): Think returns after the base Think, no boid '
         'attacks, so an un-launched boid touches the player'),
        ('6A4 KillBoid attackSlot', 'CIngBlobSwarm::KillBoid(', [],
         ['((CSwarmBasics::CBoid*)$rsi)->mAttackSlot = 0'], 0,
         'the dying boid holds slot 0 (a boid shot or touched during its ~10-tick wind-up): '
         'the slot is freed and the attack flags cleared'),
    ]
