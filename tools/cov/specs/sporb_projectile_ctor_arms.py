# sporb6: natural projectile constructors; force only derived-class inputs after
# CPatterned has finished. No inferior calls or model ownership copies.
# SPORB6_CTOR_CASE=0..5 force_ret.py, hall, 1200 frames (no driver needed).
import gdb
import math
import os

CLASS='CSporbProjectile'
selected=int(os.environ.get('SPORB6_CTOR_CASE','0'))
cases=[(2.,6.,3.,True,True),(4.,6.,4.,False,True),(3.,6.,3.,True,False),
       (2.,6.,3.,False,False),(float('nan'),6.,float('nan'),False,False),
       (2.,float('nan'),2.,False,False)]
active=None
syms=[]
for line in open(os.environ['COV_NM']):
    raw,_,demangled=line.rstrip().partition('\t')
    words=raw.split()
    if len(words)==3 and words[1] in 'tTW' and '.cold' not in demangled:
        syms.append((words[2],demangled))


def symbol(prefix):
    return next(m for m,d in syms if d.startswith(prefix))


def log(text):
    with open(os.path.join(os.environ['COV_OUT'],'forced.log'),'a') as out:
        out.write(text+'\n')


def assign(expr,value):
    if isinstance(value,float) and math.isnan(value):
        gdb.set_convenience_variable('sp6p_nan',gdb.Value(value))
        value='$sp6p_nan'
    gdb.execute('set var %s = %s' % (expr,value))


class Complete(gdb.FinishBreakpoint):
    def __init__(self,frame,state):
        super().__init__(frame,internal=True)
        self.state=state

    def stop(self):
        global active
        state=self.state
        actor=gdb.parse_and_eval(state['actor']).dereference()
        radius=float(actor['mSphere']['mSphere']['mRadius'])
        expected=cases[selected][2]
        good=(math.isnan(radius) and math.isnan(expected)) or abs(radius-expected)<1.e-5
        half=state['half']
        want=(-half[0],-half[2],-half[1],half[0],half[2],half[1])
        box=actor['mBaseBoundingBox']
        got=tuple(float(box[end]['m'+axis]) for end in ('min','max') for axis in 'XYZ')
        good &= all(abs(a-b)<1.e-5 for a,b in zip(got,want))
        log('F494 case%d: %s radius=%s expected=%s box=%s expected=%s' %
            (selected,'PASS' if good else 'FAIL',radius,expected,got,want))
        for expr,value in state['saved']:
            assign(expr,value)
        # Restore nonfinite forced radius before the normal game update sees it.
        original_radius=max(state['old_half'],state['old_height']*.5)
        assign(state['actor']+'->mSphere.mSphere.mRadius',original_radius)
        assign(state['actor']+'->mBallEscapeRadius',2*original_radius)
        original=state['original_half']
        for end,sign in (('min',-1),('max',1)):
            for axis,value in zip('XYZ',(original[0],original[2],original[1])):
                assign(state['actor']+'->mBaseBoundingBox.'+end+'.m'+axis,sign*value)
        active=None
        return False


class Capture(gdb.Breakpoint):
    def stop(self):
        global active
        actor='((CSporbProjectile*)%d)' % int(gdb.parse_and_eval('$rdi'))
        pattern=gdb.newest_frame().read_var('patternedInfo').referenced_value().address
        pattern='((CPatternedInfo*)%d)' % int(pattern)
        active={'actor':actor,'pattern':pattern,'saved':[],
                'old_half':float(gdb.parse_and_eval(pattern+'->mHalfExtent')),
                'old_height':float(gdb.parse_and_eval(pattern+'->mHeight'))}
        for name,valid in [('ballSpitEffect',cases[selected][3]),('ballEscapeEffect',cases[selected][4])]:
            if not valid:
                gdb.execute('set var %s = 4294967295u' % name)
        Complete(gdb.newest_frame(),active)
        return False


class BaseComplete(gdb.FinishBreakpoint):
    def stop(self):
        if active:
            for field,value in zip(('mHalfExtent','mHeight'),cases[selected][:2]):
                expr=active['pattern']+'->'+field
                active['saved'].append((expr,float(gdb.parse_and_eval(expr))))
                assign(expr,value)
        return False


class Base(gdb.Breakpoint):
    def stop(self):
        if active:
            BaseComplete(gdb.newest_frame(),internal=True)
        return False


class Box(gdb.Breakpoint):
    def stop(self):
        if active and int(gdb.parse_and_eval('$rdi'))==int(gdb.parse_and_eval(active['actor'])):
            box=gdb.parse_and_eval(active['actor']+'->mBaseBoundingBox')
            active['original_half']=tuple((float(box['max']['m'+a])-float(box['min']['m'+a]))*.5 for a in 'XYZ')
            active['half']=(1.,2.,3.)
            for i,axis in enumerate('XYZ'):
                assign(active['actor']+'->mBaseBoundingBox.min.m'+axis,0.)
                assign(active['actor']+'->mBaseBoundingBox.max.m'+axis,2.*(i+1))
        return False


Capture('*'+symbol('CSporbProjectile::CSporbProjectile('),internal=True)
Base('*'+symbol('CPatterned::CPatterned('),internal=True)
Box('*'+symbol('CPhysicsActor::GetBaseBoundingBox('),internal=True)
SPEC=[]
