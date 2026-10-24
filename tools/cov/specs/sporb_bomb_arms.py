# sporb6: guardian ShootPowerBomb/CreatePowerBomb, real bombs and emitter-slot arms.
# force_ret.py; sporb_pbg.con; 3dfd2249,11_swamp_bloggtank_dark; 3200 frames.
import gdb
import math
import os

CLASS = 'CSporbBase'
for key, value in [('bomb','(CSporbPowerBomb*)0'),('trace','0'),
                   ('created','0'),('added','0')]:
    gdb.execute('set $sp6b_%s = %s' % (key,value))


def log(text):
    with open(os.path.join(os.environ['COV_OUT'],'forced.log'),'a') as out:
        out.write(text+'\n')


syms = []
for line in open(os.environ['COV_NM']):
    raw, _, demangled = line.rstrip().partition('\t')
    fields = raw.split()
    if len(fields) == 3 and fields[1] in 'tTW' and '.cold' not in demangled:
        syms.append((fields[2],demangled))


def symbol(prefix):
    return next(m for m,d in syms if d.startswith(prefix))


class BombReturn(gdb.FinishBreakpoint):
    def stop(self):
        gdb.execute('set $sp6b_bomb=(CSporbPowerBomb*)$rax')
        return False


class Bomb(gdb.Breakpoint):
    def stop(self):
        if int(gdb.parse_and_eval('$sp6b_trace')):
            gdb.execute('set $sp6b_created=$sp6b_created+1')
            BombReturn(gdb.newest_frame(),internal=True)
        return False


Bomb('*'+symbol('CSporbBase::CreatePowerBomb('),internal=True)


class Add(gdb.Breakpoint):
    def stop(self):
        if int(gdb.parse_and_eval('$sp6b_trace')):
            gdb.execute('set $sp6b_added=$sp6b_added+1')
        return False


Add('*'+symbol('CStateManager::AddObject('),internal=True)


scenarios, labels = [], []
token = '$this->mProjectileInfo.mWeaponDescription'
emitter = '((CSporbBase::SPowerBombEmitter*)$this->mPowerBombEmitters.mData)'


class Check(gdb.Function):
    def __init__(self):
        super().__init__('sp6b_check')

    def invoke(self,index,passed):
        good = bool(passed)
        log('sporb6 bomb #%d %s: %s created=%s added=%s bomb=%s' %
            (int(index),labels[int(index)],'PASS' if good else 'FAIL',
             gdb.parse_and_eval('$sp6b_created'),gdb.parse_and_eval('$sp6b_added'),
             gdb.parse_and_eval('$sp6b_bomb')))
        if not good:
            raise gdb.GdbError('Sporb bomb differs from retail')
        return 1


class Velocity(gdb.Function):
    def __init__(self):
        super().__init__('sp6b_velocity')

    def invoke(self):
        bomb = gdb.parse_and_eval('$sp6b_bomb').dereference()
        # height=2, gravity=9.81, displacement=(3,4,0): flight=2*sqrt(4/g).
        time = 2*math.sqrt(4/9.81)
        tick = float(gdb.parse_and_eval("'CSporbBase::ShootPowerBomb(float, CVector3f const&, CStateManager&, int, CVector3f const&)::sTickTime'"))
        want = [3/time*tick,4/time*tick,.5*9.81*time*tick]
        got = [float(bomb['mProjectile']['mVelocity']['m'+a]) for a in 'XYZ']
        gravity = [float(bomb['mProjectile']['mGravity']['m'+a]) for a in 'XYZ']
        good = all(abs(a-b)<2.e-5 for a,b in zip(got+gravity,want+[0,0,-9.81*tick]))
        log('44BC ballistic velocity/gravity: %s got=%s / %s' % ('PASS' if good else 'FAIL',got,gravity))
        return int(good)


Check()
Velocity()
saved = [(emitter+'[%d].%s' % (i,f)) for i in range(4) for f in ('mHandle.mID','mProjectileId.value')]
baseline = [(s,s) for s in saved] + [(token+'.mItem',token+'.mItem'),
            ('$sp6b_trace','1'),('$sp6b_created','0'),('$sp6b_added','0'),('$sp6b_bomb','0'),
            ('$this->mStages.mCount','0'),('$this->mAimTarget.mX','$this->mPosition.mX + 3.f'),
            ('$this->mAimTarget.mY','$this->mPosition.mY + 4.f'),
            ('$this->mAimTarget.mZ','$this->mPosition.mZ')]


def shoot(label,writes=(),count=1000,created=1,added=1,check='1',stubs=()):
    n=len(scenarios)
    scenarios.append((baseline+list(writes),
        '($this->ShootPowerBomb(2.f,$this->mPosition,*$mgr,%d,$this->mAimTarget),'
        '$sp6b_check(%d,$sp6b_created==%d && $sp6b_added==%d && (%s)))' %
        (count,n,created,added,check),list(stubs)))
    labels.append(label)


shoot('4548 cached, projectile quota zero',count=0,created=0,added=0)
shoot('4560 unlocked',[(token+'.mItem','0'),(token+'.mLockHeld','0')],created=0,added=0)
shoot('4570 locked but unloaded',[(token+'.mItem','0'),(token+'.mLockHeld','1'),
      (token+'.mObjRef->mObject','0')],created=0,added=0)
shoot('4574 cache loaded token',[(token+'.mItem','0'),(token+'.mLockHeld','1')],check='$sp6b_velocity()')
# Host rs_new is throwing new (CMemory.hpp), so its allocation-failure path does
# not return. Force the caller's nullable result guard; successful callees are real.
shoot('4694 null CreatePowerBomb result',added=0,
      check='$sp6b_bomb == 0',stubs=[('CSporbBase::CreatePowerBomb(','0')])
for layout in ('empty','free-first','busy-handles','busy-ids','mixed-last'):
    writes=[('$this->mPowerBombEmitters.mCount','0' if layout=='empty' else '4')]
    for i in range(4):
        handle=1 if layout=='busy-handles' or (layout=='mixed-last' and i==0) else 0
        uid='65535' if layout in ('empty','free-first','busy-handles') or i==3 else '$this->mUniqueId.value'
        writes += [(emitter+'[%d].mHandle.mID'%i,str(handle)),
                   (emitter+'[%d].mProjectileId.value'%i,uid)]
    check='$sp6b_velocity()'
    if layout in ('free-first','mixed-last'):
        index=0 if layout=='free-first' else 3
        check += ' && '+emitter+'[%d].mProjectileId.value==$sp6b_bomb->mUniqueId.value'%index
    shoot('4F30-4F6C '+layout,writes,check=check)
CALLS=[('sporb6 bombs','CSporbBase::Think(',
        '((CSporbBase*)$rdi)->mIsPowerBombGuardian && ((CSporbBase*)$rdi)->mState==2 '
        '&& ((CSporbBase*)$rdi)->mTopId.value!=65535',scenarios,
        '; '.join('%d=%s'%x for x in enumerate(labels)))]
SPEC=[]
