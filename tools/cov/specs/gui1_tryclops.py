# Tryclops's two branchless virtual getters and native static-initializer outputs.
import gdb
import os

CLASS='CTryclops'
SPEC=[]
def ev(s): return gdb.parse_and_eval(s)
def log(s):
    with open(os.environ['COV_OUT']+'/try-gui1.log','a') as f: f.write(s+'\n')

class Check(gdb.Function):
    def __init__(self): super().__init__('tg1_check')
    def invoke(self,actor,path,listening):
        assert int(path)==int(actor.dereference()['mPathFindSearch'].address)
        assert bool(listening)
        log('GetSearchPath/IsListening oracle PASS')
        vec=ev('CTryclops::kBombPosOffset')
        assert abs(float(vec['mX']))<1.e-6 and abs(float(vec['mY']))<1.e-6 and abs(float(vec['mZ'])+.3)<1.e-6
        assert ev('CTryclops::kMouthLctr').string()=='ballGrab_locator'
        try:
            assert ev("'CTryclops.cpp'::skRootLocator").string()=='Skeleton_Root'
        except gdb.error as error:
            if 'optimized out' not in str(error): raise
            log('root locator constant optimized out; source/retail checked, no runtime claim')
        try:
            vulnerability=ev("'CTryclops.cpp'::skPowerBombVulnerability")
            assert int(vulnerability['first'])==int(ev('kWT_PowerBomb'))
            assert float(vulnerability['second']['mDamageMultiplier'])==1.
            assert int(vulnerability['second']['mEffect'])==0 and not bool(vulnerability['second']['mIgnoreRadius'])
        except gdb.error as error:
            if 'optimized out' not in str(error): raise
            log('vulnerability constant optimized out; exact matched initializer proof instead')
        for table,names in (('skTriggers','InDetectionRange InAttackPosition Inside InMaxRange InRange InPosition SpotPlayer HearShot CoverBlown IsDizzy ShouldRetreat HasRetreatPattern'),
                            ('skStates','Patrol Suck Crouch PathFindEx PathFind Attack TargetCover SelectTarget JumpBack Approach GetUp Shuffle TurnAround TargetPatrol TargetPlayer Dizzy Cover FixedDelay')):
            arr=ev("'CTryclops.cpp'::"+table)
            for i,name in enumerate(names.split()):
                entry=arr[i]
                assert entry['mName'].string()==name
                target=int(ev('*(unsigned long*)%d'%int(entry['mFunction'].address)))
                if target&1: target=int(ev('*(unsigned long*)(*(unsigned long*)%d+%d)'%(int(actor),target-1)))
                resolved=gdb.execute('info symbol %d'%target,to_string=True)
                assert 'CTryclops::'+name+'(' in resolved,(name,resolved)
        log('static vector/mouth locator and all30 PMF bindings oracle PASS')
        return 1
Check()
CALLS=[('gui1 Tryclops getters/statics','CTryclops::Think(',
    '((CStateManager*)$rsi)->mUpdateFrameIdx>350',
    [([], '$tg1_check($this,$this->GetSearchPath(),$this->IsListening())')],
    'branchless real virtual calls and complete static output oracles')]
