# Native constructed CGameProjectile/CDamageInfo fixtures; no console shots.
import gdb
import os

CLASS='CShrieker'
SPEC=[]

def ev(s): return gdb.parse_and_eval(s)
def log(s):
    with open(os.environ['COV_OUT']+'/shr-gui1.log','a') as f: f.write(s+'\n')

symbols=[]
for line in open(os.environ['COV_NM']):
    raw,_,name=line.rstrip().partition('\t')
    p=raw.split()
    if len(p)==3 and p[1] in 'tTW' and '.cold' not in name:
        symbols.append((p[2],name))
def sym(prefix,part=None):
    return next(m for m,n in symbols if n.startswith(prefix) and (part is None or part in m))

class Expect(gdb.Function):
    def __init__(self): super().__init__('sg1_expect')
    def invoke(self,v):
        assert bool(v),'Shrieker oracle failed'
        log('oracle PASS')
        return 1
Expect()

class LoaderCheck(gdb.Function):
    def __init__(self): super().__init__('sg1_loader_check')
    def invoke(self,p):
        obj=p.dereference()
        fields={'hostileAccumulatePriority':.1,'pART':4294967295,'projectile':4294967295,'meleeEffect':4294967295,
            'combatVisorMaxVolume':50,'echoVisorMaxVolume':100,'meleeAverageAttackTime':1.,'meleeAttackTimeVariation':.1,
            'meleeRange':4.,'hoverHeight':0.,'missileDeflectionRadius':15.,'missileDeflectRate':1.,'sound_MissileDeflection':-1,
            'dodgeTime':3.,'dodgePercentage':20.,'detectionHeight':5.,'rustleDetectionRadius':15.,'popDetectionRadius':10.,
            'morphballDetectionRadius':5.,'visibilityChangeTime':.5}
        for name,want in fields.items(): assert abs(float(obj[name])-want)<1.e-5,name
        for n,damage,radius,kb in (('damageInfo',10.,4.5,4.),('projectileDamage',5.,0.,3.),('meleeDamage',10.,4.5,4.)):
            d=obj[n]
            assert int(d['dI_WeaponType'])==11 and float(d['dI_Damage'])==damage and float(d['dI_KnockBackPower'])==kb,n
            if n!='projectileDamage': assert float(d['dI_Radius'])==radius,n
        assert all(int(obj['actorInformation']['lighting']['ambientColor']['m'+c])==255 for c in 'RGBA')
        assert [float(obj['missileDeflectionOffset']['m'+c]) for c in 'XYZ']==[0.,0.,-1.5]
        log('all local loader defaults / white ambient oracle PASS')
        return 1
LoaderCheck()

class StaticsCheck(gdb.Function):
    def __init__(self): super().__init__('sg1_statics')
    def invoke(self,actor):
        for table,names in (('skTriggers','PathShagged PlayerEntersProximity PlayerLeavesProximity EnterUprootShriek HasPatrolPath PlayerLeashReached LeavePatrol CloseToStart LineOfSight ShouldFire ShouldMelee ShouldDodge InMaxRange'),
                            ('skStates','Buried BuriedRumbling TargetPlayer UprootShriek PathFind ShriekAttack Bury Patrol ReturnToStart Retreat Melee Dodge Dead')):
            arr=ev("'CShrieker.cpp'::"+table)
            for i,name in enumerate(names.split()):
                entry=arr[i]
                assert entry['mName'].string()==name
                target=int(ev('*(unsigned long*)%d'%int(entry['mFunction'].address)))
                if target&1: target=int(ev('*(unsigned long*)(*(unsigned long*)%d+%d)'%(int(actor),target-1)))
                resolved=gdb.execute('info symbol %d'%target,to_string=True)
                assert 'CShrieker::'+name+'(' in resolved,(name,resolved)
        v=ev("'CShrieker.cpp'::skCloseToStartTolerance")
        assert abs(float(v['mX'])-1.)<1.e-6 and abs(float(v['mY'])-1.)<1.e-6 and abs(float(v['mZ'])-.1)<1.e-6
        for key,want in (('skRootLocatorName','Skeleton_Root'),('skHoverReferenceLocatorName','HoverReference_LCTR')):
            s=ev("'CShrieker.cpp'::"+key)
            assert s['mPtr'].string()==want
        try:
            filt=ev("'CShrieker.cpp'::skUnusedFilter")
            assert int(filt['mInclude']['mValue'])==1<<int(ev('kMT_Solid'))
            assert int(filt['mExclude']['mValue'])==(1<<int(ev('kMT_Player')))|(1<<int(ev('kMT_CollisionActor')))
            assert int(filt['mType'])==int(ev('CMaterialFilter::kFT_IncludeExclude'))
        except gdb.error as error:
            if 'optimized out' not in str(error): raise
            log('unused filter optimized out; verified statically, not a runtime oracle')
        log('static state/trigger table target and tolerance oracle PASS')
        return 1
StaticsCheck()
gdb.set_convenience_variable('sg1_mode',gdb.Value(-1))
gdb.set_convenience_variable('sg1_name',gdb.Value(0))

class StateName(gdb.Breakpoint):
    def stop(self):
        name=int(ev('$sg1_name'))
        if name:
            gdb.execute('set $rax=%d'%name)
            gdb.execute('set $rip=*(unsigned long*)$rsp')
            gdb.execute('set $rsp=$rsp+8')
            log('forced state-name query')
        return False
for m,n in symbols:
    if n.startswith(('TStateMachineState<CPatterned>::GetName(', 'CGenericFSM2State<CPatterned>::GetName(')):
        StateName('*'+m,internal=True)

class BindStateName(gdb.Function):
    def __init__(self): super().__init__('sg1_bind_name')
    def invoke(self,actor):
        machine=int(actor.dereference()['mStateMachine']['mPtr'])
        address=int(ev('((unsigned long*)*(unsigned long*)%d)[10]'%machine))
        log('actual state-name target '+gdb.execute('info symbol %d'%address,to_string=True).strip())
        StateName('*%d'%address,internal=True)
        return 1
BindStateName()

class NearDone(gdb.FinishBreakpoint):
    def __init__(self,p,mode):
        super().__init__(gdb.newest_frame(),internal=True)
        self.p,self.mode=p,mode
    def stop(self):
        ids=[] if self.mode==0 else [65535,int(ev('$sg1_player.value')),int(ev('$sg1_proj->mUniqueId.value'))]
        gdb.execute('set var *(int*)%d=%d'%(self.p,len(ids)))
        for i,uid in enumerate(ids):
            gdb.execute('set var *(unsigned short*)%d=%d'%(self.p+4+2*i,uid))
        return False
class Near(gdb.Breakpoint):
    def stop(self):
        caller=gdb.newest_frame().older()
        mode=int(ev('$sg1_mode'))
        if mode>=0 and caller and 'DeflectMissiles' in (caller.name() or ''):
            NearDone(int(ev('$rsi')),mode)
        return False
for m,n in symbols:
    if n.startswith('CStateManager::BuildNearList('): Near('*'+m,internal=True)

q=[]
def add(w,c,s=None): q.append((w,c,s or []))
add([], '$sg1_bind_name($this)')
add([], '$sg1_statics($this)')
add([], '($sg1_damage=(CDamageInfo*)malloc(sizeof(CDamageInfo)),((void(*)(CDamageInfo*))%s)($sg1_damage),$sg1_damage->mDamage=10.f,$sg1_damage->mRadiusDamageAmount=10.f,$sg1_effect=(CImpactVisorEffect*)malloc(sizeof(CImpactVisorEffect)),((void(*)(CImpactVisorEffect*))%s)($sg1_effect),$sg1_uid=$mgr->AllocateUniqueId(),$sg1_player=$P->mUniqueId,$sg1_invalid=$sg1_player,$sg1_invalid.value=65535,$sg1_proj=(CGameProjectile*)malloc(sizeof(CGameProjectile)),1)'%(sym('CDamageInfo::CDamageInfo()', 'C2'),sym('CImpactVisorEffect::None(')))

ctor=sym('CGameProjectile::CGameProjectile(','C2')
prototype='void(*)(CGameProjectile*,bool,const void*,const void*,int,const void*,int,const void*,TUniqueId,TAreaId,TUniqueId,TUniqueId,unsigned int,bool,const void*,const void*)'
add([], '(((%s)%s)($sg1_proj,false,&$this->mProjectileInfo.mWeaponDescription,&\'CShrieker.cpp\'::skRootLocatorName,kWT_Power,&$this->mTransform,kMT_Player,$sg1_damage,$sg1_uid,$this->mAreaId,$sg1_player,$sg1_invalid,128,false,&$this->mModelData.mPtr->mScale,$sg1_effect),$mgr->AddObject($sg1_proj),$sg1_expect($sg1_proj->mWeaponType==kWT_Power && ($sg1_proj->mProjectileAttribs&128)!=0))'%(prototype,ctor))
add([], '($sg1_patrol=(char*)strdup("Patrol"),$sg1_return=(char*)strdup("ReturnToStart"),$sg1_buried=(char*)strdup("Buried"),1)')

# Real heap loader object and exact default values; no deserializer overwrites.
add([], '($sg1_loader=(SLdrShrieker*)malloc(sizeof(SLdrShrieker)),((void(*)(SLdrShrieker*))%s)($sg1_loader),$sg1_loader_check($sg1_loader),$sg1_expect($sg1_loader->sound_MissileDeflection==-1 && $sg1_loader->editorProperties.unknown_0x5d298a43==3 && $sg1_loader->patterned.health.hI_KnockBackResistance==2.f && $sg1_loader->patterned.echoParameters.isEchoEmitter && $sg1_loader->actorInformation.visor.visorFlags==15 && $sg1_loader->missileDeflectionRadius==15.f),((void(*)(SLdrShrieker*))%s)($sg1_loader),free($sg1_loader),1)'%(sym('SLdrShrieker::SLdrShrieker(','C2'),sym('SLdrShrieker::~SLdrShrieker(','D2')))

health='$this->mHealthInfo.mHealth'
for typ in range(16):
    for combo in (0,128):
        scale=.25 if typ in (4,6) or (typ in (0,1,2,3) and combo) else 0. if typ in (0,1,2,3) else 1.
        writes=[('$this->mBuried','1'),('$this->mPreviousHealth','100.f'),(health,'90.f'),('$this->mData.mHostileAccumulatePriority','0.25f'),('$sg1_proj->mWeaponType',str(typ)),('$sg1_proj->mProjectileAttribs',str(combo))]
        add(writes,'($this->ReactToDamage(*$mgr,$sg1_proj->mUniqueId),$sg1_expect('+health+'==%sf && $this->mShriekTriggered))'%repr(100.-10.*scale))
for buried,delta,sender in ((0,10,'$sg1_proj->mUniqueId'),(1,0,'$sg1_proj->mUniqueId'),(1,-1,'$sg1_proj->mUniqueId'),(1,10,'$sg1_invalid'),(1,10,'$sg1_player')):
    add([('$this->mBuried',str(buried)),('$this->mPreviousHealth',str(90+delta)+'.f'),(health,'90.f')],'$this->ReactToDamage(*$mgr,'+sender+')')
for state in ('$sg1_patrol','$sg1_return','$sg1_buried'):
    for sender in ('$sg1_proj->mUniqueId','$sg1_invalid'):
        call='$this->ReactToDamage(*$mgr,'+sender+')'
        if state!='$sg1_buried' and sender!='$sg1_invalid':
            call='('+call+',$sg1_expect($this->mTargetId.value==$sg1_player.value))'
        add([('$this->mBuried','0'),('$sg1_name',state)],call)
add([('$this->mBuried','1'),('$this->mPreviousHealth','100.f'),(health,'90.f'),('$this->mData.mHostileAccumulatePriority','1.f'),('$sg1_proj->mWeaponType','6')],
    '($this->ReactToDamage(*$mgr,$sg1_proj->mUniqueId),$sg1_expect('+health+'==90.f))')

# Centre and positions are computed from the real actor's aim, not guessed.
add([], '($sg1_center=$this->GetAimPosition(*$mgr,0.f),$sg1_center.mX+=$this->mData.mMissileDeflectionOffset.mX,$sg1_center.mY+=$this->mData.mMissileDeflectionOffset.mY,$sg1_center.mZ+=$this->mData.mMissileDeflectionOffset.mZ,1)')
for mode in (0,1):
    add([('$sg1_mode',str(mode))],'$this->DeflectMissiles(*$mgr)')
for typ,combo,distance,collinear in ((0,0,1,True),(0,128,1,True),(1,128,1,True),(6,0,100,True),(6,0,1,True),(6,0,1,False)):
    writes=[('$sg1_mode','1'),('$sg1_proj->mWeaponType',str(typ)),('$sg1_proj->mProjectileAttribs',str(combo)),('$this->mData.mMissileDeflectionRadius','15.f')]
    for j,c in enumerate('XYZ'):
        writes += [('$sg1_proj->mTransform.m%d3'%j, '$sg1_center.m'+c+('+'+str(distance)+'.f' if c=='X' else ''))]
        writes += [('$sg1_proj->mPosition.m'+c, '$sg1_center.m'+c+('+'+str(distance)+'.f' if c=='X' else ''))]
        writes += [('$sg1_proj->mPreviousPos.m'+c,'$sg1_center.m'+c+('+'+str(distance)+'.f' if c=='X' else '-1.f' if c=='Y' and not collinear else ''))]
    accepted=typ in (0,6) and (typ==6 or combo==128) and distance<15
    call='($sg1_before=$sg1_proj->mProjectile.mLocalXf,$this->DeflectMissiles(*$mgr),$sg1_expect($this->mDeflectedIds.mCount==%d)'%int(accepted)
    if accepted:
        call+=',$sg1_expect($sg1_proj->mMinHomingDist==15.f)'
        equal=' && '.join('$sg1_before.m%d%d==$sg1_proj->mProjectile.mLocalXf.m%d%d'%(i,j,i,j) for i in range(3) for j in range(3))
        call+=',$sg1_expect(%s(%s))'%('' if collinear else '!',equal)
    add(writes,call+',1)')
for timer in ('0.1f','0.75f'):
    add(writes+[('$this->mDeflectedIds.mCount','0'),('$this->mDeflectSfxTimer',timer)],'$this->DeflectMissiles(*$mgr)',[('CSfxManager::IsPlaying(',1)])
add([], '($sg1_mode=-1,$mgr->DeleteObjectRequest($sg1_proj->mUniqueId),free($sg1_damage),free($sg1_effect),free($sg1_patrol),free($sg1_return),free($sg1_buried),1)')
CALLS=[('gui1 Shrieker native fixtures','CShrieker::Think(', '((CStateManager*)$rsi)->mUpdateFrameIdx>350',q,'constructed real projectile, weapon type/combo and collinear cases')]
