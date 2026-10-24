# gb3 actor arms: force.sh + cov_force.py, build-port-cov, Ing smoke driver.
# Temporary active/area/description mutations are restored at Think's return.
import os
import gdb

CLASS = 'CCollisionTracker'
SPEC = []
FORCE = [
    ('27DC emission', 'CCollisionTracker::Think(', [],
     ['$this->mParticleEmissionRateScalar = 1.0'], 0, 'emit live Ing strands'),
]

if not hasattr(gdb, '_gb3_actor_installed'):
    gdb._gb3_actor_installed = True
    symbols = []
    for line in open(os.environ['COV_NM']):
        parts = line.rstrip().split('\t')
        if len(parts) == 2:
            fields = parts[0].split()
            if len(fields) == 3 and fields[1] in 'tTW':
                symbols.append((fields[2], parts[1]))

    def symbol(prefix):
        return next(m for m, d in symbols if d.startswith(prefix) and 'clone' not in d)

    def command(text):
        gdb.execute(text, to_string=True)

    def log(text):
        with open(os.path.join(os.environ['COV_OUT'], 'actor.log'), 'a') as f:
            f.write(text + '\n')

    class Restore(gdb.FinishBreakpoint):
        def __init__(self, expression, value):
            super().__init__(gdb.newest_frame(), internal=True)
            self.write_expr, self.old_value = expression, value

        def stop(self):
            command('set var %s = %s' % (self.write_expr, self.old_value))
            log('restored ' + self.write_expr)
            return False

    def temporary(expression, value):
        old = int(gdb.parse_and_eval(expression))
        command('set var %s = %s' % (expression, value))
        Restore(expression, old)

    class Think(gdb.Breakpoint):
        stage = 0

        def stop(self):
            try:
                actor = '((CCollisionTracker*)%d)' % int(gdb.parse_and_eval('$rdi'))
                mgr = '((CStateManager*)%d)' % int(gdb.parse_and_eval('$rsi'))
                gen = actor + '->mParticleSystem.mPtr'
                if self.stage == 0:
                    temporary(actor + '->mActive', 0)
                    log('27DC inactive')
                elif self.stage in (1, 2):
                    area = mgr + '->mWorld.mPtr->mAreas.mItems[' + actor + '->mAreaId.value].mItem'
                    if self.stage == 1:
                        temporary(area + '->mPhase', 0)
                        log('27DC unloaded area')
                    else:
                        temporary(area + '->mPostConst.mPtr->mOcclusionState', 0)
                        log('27DC occluded area')
                else:
                    count = int(gdb.parse_and_eval(actor + '->mStrands.mCount'))
                    if count < 2:
                        return False
                    if self.stage == 3:
                        expr = gen + '->mLoadedGenDesc->mVEL1'
                        old = int(gdb.parse_and_eval(expr))
                        command('set var ' + expr + ' = 0')
                        Restore(expr, '(CModVectorElement*)%d' % old)
                        log('27DC VEL1 absent on populated strands')
                    else:
                        for index in (0, count - 1):
                            command('set var %s->mStrands.mItems[%d].mLifetime = 0.0' % (actor, index))
                        log('27DC expired first and last strands (swap-and-continue)')
                        self.enabled = False
                self.stage += 1
            except Exception as exc:
                log('27DC FAILED stage %d: %s' % (self.stage, exc))
                self.enabled = False
            return False

    class EmptyParticles(gdb.Breakpoint):
        def stop(self):
            self.enabled = False
            try:
                command('set var particleCount = 0')
                log('27DC zero count after particle creation')
            except Exception as exc:
                log('27DC particle count FAILED: ' + str(exc))
            return False

    class Light(gdb.Breakpoint):
        def stop(self):
            self.enabled = False
            command('set ((CCollisionTracker*)$rdi)->mParticleSystem.mPtr->mLightType = CElementGen::kLT_Custom')
            log('2584 first tracker has a custom light; second retains no-light description')
            return False

    Think('*' + symbol('CCollisionTracker::Think('), internal=True)
    source = os.path.join(os.path.dirname(os.environ['COV_SPEC']),
                          '../../../src/MetroidPrime/Enemies/CCollisionTracker.cpp')
    line = next(i for i, text in enumerate(open(source), 1) if 'if (particleCount != 0)' in text)
    EmptyParticles('CCollisionTracker.cpp:%d' % line, internal=True)
    Light('*' + symbol('CCollisionTracker::AcceptScriptMsg('), internal=True)
