# dtr4: darktrooper_missile_force.py plus the line-of-sight arm of FireMissile (390, the
# `mLineOfSightTracker.HasLineOfSight()` operand): the tracker's bit is cleared at the entry of
# FireMissile, as if a wall stood between the gun and the player (no blocked line of sight is
# reachable on the open Temple Grounds floor). Same driver and command as the force spec.
import os
ns = {}
_p = os.path.join(os.path.dirname(os.environ['COV_SPEC']), 'darktrooper_missile_force.py')
exec(compile(open(_p).read(), _p, 'exec'), ns)
CLASS = ns['CLASS']
FORCE = [(l, w, e, s + (['$this->mLineOfSightTracker.mHasLineOfSight = 0'] if l == 'FireMissile' else []), k, n)
         for (l, w, e, s, k, n) in ns['FORCE']]
