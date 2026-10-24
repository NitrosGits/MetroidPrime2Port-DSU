# Metroid (module 40) met6 lane: CBabyMetroid::Think (fn_40_9C78) arms by direct calls from its own first Think (force_ret CALLS; every write lasts one call).
# CBabyMetroid::TryJoinHive and CMetroid::Think are stubbed (void) so the call tests only Think's own arms. Driver metroid_force8.con (first metroid a baby,
# metroid_force8.py's flag write is NOT needed here: run with force_ret via the `loc:` script instead, see below), so this spec is merged with the flag run:
#   (1) COV_ROOT=$PWD BUILD=... tools/cov/force.sh tools/cov/specs/metroid_force8.py <out> ...  (flag write; force.py)
#   the CALLS need the baby to exist, so the flag is written by the gdb hook below as well (same expression as metroid_force8.py).
#   COV_ROOT=$PWD COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 tools/cov/force.sh tools/cov/specs/metroid_force8b.py <out> \
#     42b935e4,08_sand_casualpirates --con tools/cov/specs/metroid_force8.con --fast --frames 2100
import os
import re
import gdb

CLASS = 'CBabyMetroid'
FORCE_RET = []
_src = open(os.path.join(os.environ.get('COV_ROOT', '.'), 'src/MetroidPrime/Enemies/CMetroid.cpp')).read().split('\n')
_L = [i + 1 for i, l in enumerate(_src) if re.search(r'^  if \(metroidData\.xc4_24_\) \{', l)][0]
_n = {'n': 0}


class _Flag(gdb.Breakpoint):
    def stop(self):
        _n['n'] += 1
        if _n['n'] == 1:
            gdb.execute('set var metroidData.xc4_24_ = 1')
        return False


_Flag('CMetroid.cpp:%d' % _L, internal=True)

COUNTER = ('*(unsigned short*)&$this->xa66_', '1344')            # u0540, a CScriptCounter of the room
NOCOUNTER = ('*(unsigned short*)&$this->xa66_', '65535')
SEEK = [('$this->mHitByPlayerProjectile', '1'), ('$this->mShouldSeekEnergySource', '1')]
NOJOIN = [('CBabyMetroid::TryJoinHive(', 0), ('CMetroid::Think(', 0)]
NEAR = [('$this->mTransform.m03', '$P->mTransform.m03'), ('$this->mTransform.m13', '$P->mTransform.m13'), ('$this->mTransform.m23', '$P->mTransform.m23')]
CALL = '$this->Think(0.016f, *$mgr)'
CALLS = [('baby Think direct calls', 'CBabyMetroid::Think(', '1', [
    (SEEK + [COUNTER], CALL, NOJOIN),                                    # hit while seeking: the counter is told, seeking stops
    (SEEK + [NOCOUNTER], CALL, NOJOIN),                                  # ... the counter is gone
    ([('$this->mHitByPlayerProjectile', '1'), ('$this->mShouldSeekEnergySource', '0')], CALL, NOJOIN),
    ([('$this->mHitByPlayerProjectile', '0')], CALL, NOJOIN),
    ([('$this->xa84_', '100.0'), ('$this->mDodgeCheckTimeInterval', '0.0'), ('$this->mShouldDodge', '0'), ('$this->mChanceToDodge', '2.0')], CALL, NOJOIN),   # dodge roll succeeds
    ([('$this->xa84_', '100.0'), ('$this->mDodgeCheckTimeInterval', '0.0'), ('$this->mShouldDodge', '0'), ('$this->mChanceToDodge', '-1.0')], CALL, NOJOIN),  # ... fails
    ([('$this->xa84_', '100.0'), ('$this->mDodgeCheckTimeInterval', '0.0'), ('$this->mShouldDodge', '1')], CALL, NOJOIN),                                 # already dodging
    ([('$this->mShouldDodge', '0'), ('$this->xa84_', '0.0'), ('$this->mDodgeCheckTimeInterval', '1000.0')], CALL, NOJOIN),                                  # timer short
    (NEAR, CALL, NOJOIN),                                                # on top of the player: contact damage
    ([('$this->mShouldSeekEnergySource', '0'), ('$this->xa78_', '1000.0'), ('$this->xa74_', '1.0')], CALL, NOJOIN),   # hive timer ran out: TryJoinHive
    ([('$this->mShouldSeekEnergySource', '0'), ('$this->xa78_', '0.0'), ('$this->xa74_', '1000.0')], CALL, NOJOIN),
    ([('$this->mShouldSeekEnergySource', '1')], CALL, NOJOIN),
], 'CBabyMetroid::Think: seek / counter, dodge roll, contact damage, hive timer')]
