# dtr4: darktrooper_missile_force.py plus line 408 of FireMissile: the local `toTarget` has its x,y zeroed
# at that line (after the third FireMissile hit), so CanBeNormalized() is false: toTarget degenerate (gun straight above/below the jittered target: no random draw can give exactly 0).
import os
ns = {}
_p = os.path.join(os.path.dirname(os.environ['COV_SPEC']), 'darktrooper_missile_force.py')
exec(compile(open(_p).read(), _p, 'exec'), ns)
CLASS = ns['CLASS']
FORCE = ns['FORCE'] + [('FireMissile408', 'loc:CDarkTrooper.cpp:408', [], ['toTarget.mX = 0', 'toTarget.mY = 0'], 3,
                        'toTarget degenerate (gun straight above/below the jittered target: no random draw can give exactly 0)')]
