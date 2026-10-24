# dtr4: darktrooper_missile_force.py plus line 412 of FireMissile: the local `forward` has its x,y zeroed
# at that line (after the third FireMissile hit), so CanBeNormalized() is false: the gun transform DY column pointing straight up/down.
import os
ns = {}
_p = os.path.join(os.path.dirname(os.environ['COV_SPEC']), 'darktrooper_missile_force.py')
exec(compile(open(_p).read(), _p, 'exec'), ns)
CLASS = ns['CLASS']
FORCE = ns['FORCE'] + [('FireMissile412', 'loc:CDarkTrooper.cpp:412', [], ['forward.mX = 0', 'forward.mY = 0'], 3,
                        'the gun transform DY column pointing straight up/down')]
