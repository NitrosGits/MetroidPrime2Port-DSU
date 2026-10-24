"""cg2: natural spawned-CHOG PreRender calls with owned, byte-exact fixtures.

Use default force.py. No loader override: console `rules` supplies the safe RULE.
Snapshots precede arming, arming precedes attempted writes. No production probes.
"""
import gdb
import hashlib
import json
import os

CLASS = 'CChozoGhost'
FORCE = []
# name, alive, red, alpha, active visor (combat0 / echo1)
CASES = [('normal',1,0,19,0), ('white-alpha',0,0,64,0),
         ('red64-live',1,64,29,0), ('red127-echo',0,127,128,1),
         ('red128-echo',0,128,255,1), ('red255-zero-alpha',0,255,0,0),
         ('red1',0,1,254,0)]
state = dict(index=0, active={}, failed=0, restores=0, ignored=0)

def log(event, **values):
    with open(os.path.join(os.environ['COV_OUT'],'cg2.jsonl'),'a') as f:
        f.write(json.dumps(dict(event=event,**values),sort_keys=True)+'\n')

def ev(expression):
    return gdb.parse_and_eval(expression)

def iv(expression):
    return int(ev(expression))

def check(condition, text):
    if not condition:
        raise RuntimeError(text)

def context():
    thread=gdb.selected_thread()
    check(thread is not None,'missing thread')
    inferior=gdb.selected_inferior()
    check(inferior is not None and inferior.num is not None and thread.global_num is not None,
          'unreadable inferior/thread identity')
    return inferior.num, thread.global_num

def failure(text):
    state['failed']+=1
    log('FAIL',reason=str(text))

def same_frame(a,b):
    check(a is not None and b is not None and a.is_valid() and b.is_valid(),'invalid frame')
    return bool(a==b)

def bit_storage(typ, name):
    """DWARF bit positions, not retail offsets or a guessed host uint unit."""
    found=[]
    for field in typ.strip_typedefs().fields():
        if field.name==name:
            check(field.bitpos is not None and field.bitsize>0,'not a concrete bitfield')
            found.append((field.bitpos//8, (field.bitpos%8+field.bitsize+7)//8))
        elif field.is_base_class:
            try:
                offset,size=bit_storage(field.type,name)
            except LookupError:
                continue
            check(field.bitpos is not None and field.bitpos%8==0,'unsupported base layout')
            found.append((field.bitpos//8+offset,size))
    if not found:
        raise LookupError(name)
    check(len(found)==1,'ambiguous bitfield '+name)
    return found[0]

class Snapshot:
    def __init__(self, regions, memory):
        self.memory=memory
        self.saved=[]
        seen=set()
        for label,address,size in regions:
            if (address,size) not in seen:
                self.saved.append((label,address,bytes(memory.read_memory(address,size))))
                seen.add((address,size))

    def originals(self):
        return [dict(label=l,address=a,original=b.hex()) for l,a,b in self.saved]

    def restore(self):
        errors=[]
        # Attempt every write and every read independently, including post-write errors.
        for label,address,raw in reversed(self.saved):
            try:self.memory.write_memory(address,raw)
            except Exception as error:errors.append(label+' write: '+str(error))
        rows=[]
        for label,address,raw in self.saved:
            try:
                actual=bytes(self.memory.read_memory(address,len(raw)))
                rows.append(dict(label=label,address=address,original=raw.hex(),readback=actual.hex()))
                if actual!=raw:errors.append(label+' readback mismatch')
            except Exception as error:errors.append(label+' read: '+str(error))
        return rows,errors

def regions(this,S,P):
    result=[]
    for name in ('mAlive','mDrawParticles','mCalculateLighting'):
        offset,size=bit_storage(gdb.lookup_type('CChozoGhost'),name)
        result.append((name+' storage',this+offset,size))
    for expression in (S+'mColor',S+'mDrawFlags','*'+S+'mActorLights.mPtr',P+'mCurrentVisor'):
        value=ev(expression)
        check(value.address is not None,'addressable region '+expression)
        result.append((expression,int(value.address),value.type.sizeof))
    return result

def rgba(expression):
    return tuple(iv(expression+'.m'+c) for c in 'RGBA')

def expected(case):
    name,alive,red,alpha,visor=case
    alpha=255 if alive else alpha
    blend=alpha<255 or red!=0
    value=max(255-2*red,0)
    return (5 if blend else 0), ((255,value,value,alpha) if red else (255,255,255,alpha))

def visual(w):
    S=w.S
    mode,color=expected(w.case)
    actual=dict(mode=iv(S+'mDrawFlags.mBlendMode'),material=iv(S+'mDrawFlags.mMatSetIdx'),
                flags=iv(S+'mDrawFlags.mFlags'),color=rgba(S+'mDrawFlags.mColor'),
                particles=iv(S+'mDrawParticles'),calculate=iv(S+'mCalculateLighting'))
    check(actual['mode']==mode and actual['material']==0 and actual['flags']==3,'model flags')
    check(actual['color']==color,'model RGBA')
    echo=w.case[4]==1
    check(actual['particles']==int(not echo) and actual['calculate']==int(not echo),'visor outputs')
    if echo:
        check(rgba(S+'mActorLights.mPtr->mAmbientColor')==(255,255,255,255),'echo white ambient')
    return actual

class Window(gdb.FinishBreakpoint):
    def __init__(self,this,mgr,case,S,P,frame,ctx,snapshot):
        self.this,self.mgr,self.case,self.S,self.P=this,mgr,case,S,P
        self.owner,self.ctx,self.snapshot=frame,ctx,snapshot
        self.closed=False
        self.visor=[]
        self.base=self.bone=0
        self.attempted=False
        super().__init__(frame,internal=True)  # MUST succeed before mutation
        check(ctx not in state['active'],'nested owned PreRender')
        state['active'][ctx]=self
        log('ARM',case=case[0],frame_level=frame.level(),context=ctx,
            this=this,originals=snapshot.originals())

    def restore(self):
        check(context()==self.ctx,'restore context mismatch: no unsafe writes')
        check(not self.closed and state['active'].get(self.ctx) is self,
              'restore ownership mismatch: no stale writes')
        rows,errors=self.snapshot.restore()
        log('RESTORE',case=self.case[0],fields=rows,errors=errors)
        state['restores']+=1
        check(not errors,'restore failed '+str(errors))

    def apply(self):
        name,alive,red,alpha,visor=self.case
        writes=[(self.S+'mAlive',alive),(self.P+'mCurrentVisor',visor)]
        writes += [(self.S+'mColor.m'+axis,value) for axis,value in zip('RGBA',(red,71,93,alpha))]
        try:
            for expression,value in writes:
                self.attempted=True  # execute can change memory and then raise
                gdb.execute('set var %s = %d'%(expression,value),to_string=True)
                check(iv(expression)==value,'forced readback mismatch '+expression)
        except Exception:
            self.restore()
            raise

    def stop(self):
        if self.closed:
            failure('duplicate finish; no stale writes')
            return True
        good=False
        try:
            check(state['active'].get(self.ctx) is self,'finish window ownership')
            actual=visual(self)
            check(self.base==1 and self.bone==1,'base/bone calls must complete')
            check(self.visor==[self.case[4],self.case[4]],'two actual active-visor results')
            log('RESULT',case=self.case[0],output=actual,base=self.base,bone=self.bone,visor=self.visor)
            good=True
        except Exception as error:
            failure(error)
        finally:
            try:self.restore()
            except Exception as error:
                failure(error);good=False
            self.closed=True
            if state['active'].get(self.ctx) is self:state['active'].pop(self.ctx)
        if good:
            state['index']+=1
        return not good

    def out_of_scope(self):
        if self.closed:
            failure('late unwind; no writes')
            return
        failure('PreRender unwind '+self.case[0])
        try:self.restore()
        except Exception as error:failure(error)
        self.closed=True
        if state['active'].get(self.ctx) is self:state['active'].pop(self.ctx)

def owned():
    ctx=context()
    w=state['active'].get(ctx)
    if w is None:return None
    caller=gdb.newest_frame().older()
    if not same_frame(w.owner,caller):
        state['ignored']+=1
        return None
    return w

def abort(error):
    failure(error)
    for w in list(state['active'].values()):
        try:w.restore()
        except Exception as restore_error:failure(restore_error)
    return True

class Entry(gdb.Breakpoint):
    def stop(self):
        if state['failed']:return True
        if state['index']==len(CASES):return False
        try:
            this,mgr=iv('$rdi'),iv('$rsi')
            ctx=context();frame=gdb.newest_frame()
            check(frame is not None and frame.is_valid(),'entry frame')
            check(ctx not in state['active'],'recursive entry rejected before writes')
            S='((CChozoGhost*)%d)->'%this
            P='((CStateManager*)%d)->mPlayerStates[0]->'%mgr
            # No invented lights, model or animation; first normal call may create lights.
            if not iv(S+'mActorLights.mPtr'):return False
            check(iv(S+'mModelData.mPtr->mAnimData.mItem')!=0,'real animation')
            check(iv(S+'mBodyController.mPtr')!=0,'real body controller')
            dynamic=str(ev('((CEntity*)%d)'%this).dynamic_type)
            check(dynamic=='CChozoGhost *','concrete ghost dynamic type '+dynamic)
            if state['index']==0:
                log('TARGET',this=this,dynamic_type=dynamic,editor=iv(S+'mEditorId.value'),
                    uid=iv(S+'mUniqueId.value'))
            snap=Snapshot(regions(this,S,P),gdb.selected_inferior())
            w=Window(this,mgr,CASES[state['index']],S,P,frame,ctx,snap)
            w.apply()
            return False
        except Exception as error:return abort(error)

class VisorReturn(gdb.FinishBreakpoint):
    def __init__(self,w):
        super().__init__(gdb.newest_frame(),internal=True);self.w=w
    def stop(self):
        try:
            check(state['active'].get(context()) is self.w and not self.w.closed,'visor ownership')
            check(same_frame(gdb.newest_frame(),self.w.owner),'visor return owner')
            self.w.visor.append(iv('$eax'))
            return False
        except Exception as error:return abort(error)
    def out_of_scope(self):
        failure('active-visor callee unwound')

class Trace(gdb.Breakpoint):
    def __init__(self,symbol,kind):
        super().__init__('*'+symbol,internal=True);self.kind=kind
    def stop(self):
        try:
            w=owned()
            if w is None:return False
            if self.kind=='visor':
                check(iv('$rdi')==iv(w.P[:-2]),'actual player-state argument')
                VisorReturn(w)
            elif self.kind=='base':
                check(iv('$rdi')==w.this and iv('$rsi')==w.mgr,'base arguments')
                log('BEFORE_BASE',case=w.case[0],output=visual(w));w.base+=1
            else:
                S=w.S
                check(iv('$rdi')==iv('&'+S+'mBoneTracking') and iv('$rsi')==w.mgr,'bone owner/manager')
                check(iv('$rdx')==iv(S+'mModelData.mPtr->mAnimData.mItem'),'bone animation')
                check(iv('$rcx')==iv('&'+S+'mTransform'),'bone transform')
                check(iv('$r9')==iv(S+'mBodyController.mPtr'),'bone body controller')
                for axis in 'XYZ':
                    check(float(ev('((CVector3f*)%d)->m'%iv('$r8')+axis))==float(ev(S+'mModelData.mPtr->mScale.m'+axis)),'bone scale')
                w.bone+=1
                log('BONE_CALL',case=w.case[0])
            return False
        except Exception as error:return abort(error)

def symbol(prefix):
    matches=[]
    for line in open(os.environ['COV_NM']):
        raw,dem=line.rstrip('\n').split('\t');parts=raw.split()
        if len(parts)==3 and parts[1] in 'tTW' and dem.startswith(prefix) and 'clone' not in dem:
            matches.append(parts[2])
    check(len(matches)==1,'one native symbol '+prefix)
    return matches[0]

def summary(event):
    ok=state['index']==len(CASES) and not state['active'] and not state['failed']
    log('EXIT',ok=ok,completed=state['index'],pending=len(state['active']),
        failed=state['failed'],restores=state['restores'],ignored_nested=state['ignored'])
    if not ok:failure('incomplete cg2 run')

def install():
    binary=gdb.current_progspace().filename
    with open(binary,'rb') as f:digest=hashlib.file_digest(f,'sha256').hexdigest()
    log('BINARY',path=binary,sha256=digest)
    Entry('*'+symbol('CChozoGhost::PreRender('),internal=True)
    Trace(symbol('CPlayerState::GetActiveVisor('),'visor')
    Trace(symbol('CActor::PreRender('),'base')
    Trace(symbol('CBoneTracking::PreRender(CStateManager const&, CAnimData&, CTransform4f const&, CVector3f const&, CBodyController const&)'),'bone')
    gdb.events.exited.connect(summary)

if os.environ.get('CG2_SELFTEST')!='1':install()
