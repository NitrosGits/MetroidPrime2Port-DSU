# DarkSamus boost ball effects (ds895c): fn_10_9B24 CreateBoostBallEffects, fn_10_99B8
# StopBoostBallParticles and fn_10_8F50 UpdateBoostBallEffects, keyed on the members they test.
# Run: tools/cov/run.sh tools/cov/specs/darksamus_boostfx.py <out> "1baa96c2,10_cliff_rooftop"
#      --con tools/drivers/darksamus_boost_rooftop.con --fast --frames 2200 --env MP2_INPUT=LAYER2@400
CLASS = 'CDarkSamus'

E3C = '($this->mBoostBallSwooshE3C != 0)'
E4C = '($this->mBoostBallSwooshE4C != 0)'
GEN = '($this->mBoostBallElementGen.%s != 0)'

SPEC = [
    ('9B24', 'CDarkSamus::CreateBoostBallEffects(',
     ['$this->mBoostBallSwooshTokenE40.m_valid', '$this->mBoostBallSwooshTokenE50.m_valid', E3C, E4C], ''),
    ('99B8', 'CDarkSamus::StopBoostBallParticles(', [E3C, E4C], ''),
    ('8F50', 'CDarkSamus::UpdateBoostBallEffects(',
     [GEN % 'mItem', E3C, E4C, '$this->mAlive', '$this->mBoostBallFlag40'], 'max=20000'),
]

# The forcing runs (tools/drivers/darksamus_boostfx.con; COV_GDB_SCRIPT=tools/cov/cov_force.py also
# counts SPEC): DSB_MODE=1 (the default) has the second spawn read the trail swoosh token absent
# (9B24's 0xE48 arm); DSB_MODE=2 has it read the second token absent (the 0xE58 arm, then 8F50
# with 0xE3C and no 0xE4C, alive and after the kill).
#   DSB_MODE=2 COV_GDB_SCRIPT=$PWD/tools/cov/cov_force.py tools/cov/force.sh
#   tools/cov/specs/darksamus_boostfx.py <out> "1baa96c2,10_cliff_rooftop"
#   --con tools/drivers/darksamus_boostfx.con --fast --frames 1700   (MP2_INPUT=LAYER2@400 set)
import os
_TOKEN = 'mBoostBallSwooshTokenE50' if os.environ.get('DSB_MODE') == '2' else 'mBoostBallSwooshTokenE40'
FORCE = [
    ('9B24', 'CDarkSamus::CreateBoostBallEffects(', ['$this->mBoostBallSwooshTokenE40.m_valid', '$this->mBoostBallSwooshTokenE50.m_valid'],
     ['$dsb_n = $_isvoid($dsb_n) ? 1 : $dsb_n + 1',
      '$dsb_n == 2 ? ($this->%s.m_valid = 0) : 0' % _TOKEN],
     0, 'spawn 2: %s read absent' % _TOKEN),
]
