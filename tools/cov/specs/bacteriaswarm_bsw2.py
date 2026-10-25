# BacteriaSwarm bsw2, used with COV_GDB_SCRIPT=tools/cov/force_ret.py.
# Run once normally (native call queue + triangle-result fixture), then once with
# BSW2_CTOR_SKIP=1 (actual constructor stack/reference arguments set to invalid ids).
import gdb
import os

CLASS = 'CBacteriaSwarm'
SPEC = []
SKIP = os.environ.get('BSW2_CTOR_SKIP') == '1'

def log(text):
    with open(os.environ['COV_OUT'] + '/bsw2.log', 'a') as f:
        f.write(text + '\n')

class Expect(gdb.Function):
    def __init__(self):
        super().__init__('bsw2_expect')

    def invoke(self, value):
        assert bool(value), 'bsw2 field/return oracle failed'
        log('ORACLE PASS')
        return 1

Expect()

class CtorExit(gdb.FinishBreakpoint):
    def __init__(self, this, data, death, scan, scan_id):
        super().__init__(gdb.newest_frame(), internal=True)
        self.this, self.data, self.death = this, data, death
        self.scan, self.scan_id = scan, scan_id

    def stop(self):
        this = '((CBacteriaSwarm*)%d)' % self.this
        death = int(gdb.parse_and_eval(this + '->mDeathParticleGenerator.mPtr'))
        boid = int(gdb.parse_and_eval(this + '->mBoidParticleGenerator.mPtr'))
        log('CTOR EXIT skip=%d death=%#x boid=%#x' % (SKIP, death, boid))
        if SKIP:
            assert death == 0 and boid == 0
            gdb.execute('set var ((CBasicSwarmData*)%d)->mDeathParticleEffect=%d' %
                        (self.data, self.death))
            gdb.execute('set var ((CStaticRes*)%d)->mCmdlId=%d' % (self.scan, self.scan_id))
            assert int(gdb.parse_and_eval(this + '->mScanModelData.mAnimData.mItem')) == 0
            assert not bool(gdb.parse_and_eval(this + '->mScanModelData.mNormalModel.m_valid'))
            log('CTOR reference arguments restored; absent model and both generator oracles PASS')
        return False

class Ctor(gdb.Breakpoint):
    def stop(self):
        this = int(gdb.parse_and_eval('$rdi'))
        data = int(gdb.parse_and_eval('&data'))
        death = int(gdb.parse_and_eval('data.mDeathParticleEffect'))
        particle = int(gdb.parse_and_eval('particleEffect'))
        scan = int(gdb.parse_and_eval('&scanModel'))
        scan_id = int(gdb.parse_and_eval('scanModel.mCmdlId'))
        log('CTOR ENTRY this=%#x data=%#x death=%#x particle=%#x scan=%#x' %
            (this, data, death, particle, scan_id))
        CtorExit(this, data, death, scan, scan_id)
        if SKIP:
            gdb.execute('set var data.mDeathParticleEffect=4294967295')
            gdb.execute('set var particleEffect=4294967295')
            gdb.execute('set var scanModel.mCmdlId=4294967295')
            log('FORCED ctor death/scan reference members and boid stack argument to invalid ids')
        return False

ctor_symbols = []
for line in open(os.environ['COV_NM']):
    parts = line.rstrip('\n').split('\t')
    if len(parts) == 2 and parts[1].startswith('CBacteriaSwarm::CBacteriaSwarm('):
        raw = parts[0].split()
        if len(raw) == 3 and raw[1] in 'tTW' and 'C2E' in raw[2] and '.cold' not in raw[2]:
            ctor_symbols.append(raw[2])
assert len(ctor_symbols) == 1, ctor_symbols
Ctor('*' + ctor_symbols[0], internal=True)

queue = []
def add(writes, call, stubs=None):
    queue.append((writes, call, stubs or []))

add([], '($bs_cam=$mgr->mCameraManagers[0]->mFpCamera, '
    '$bs_zone=(CScriptSafeZone*)calloc(1,sizeof(CScriptSafeZone)), '
    '$bs_ahead=(CVector3f*)calloc(1,sizeof(CVector3f)), '
    '$bs_near=(rstl::reserved_vector<CBacteriaSwarm::CBoid*,50>*)calloc(1,sizeof(rstl::reserved_vector<CBacteriaSwarm::CBoid*,50>)), '
    '$bs_zone->mScale.mX=10.f,1)')

# Real camera geometry; one real element, count and all written members restored on return.
B = '$this->mBoids.mItems[0].'
camera = '$bs_cam->mTransform.'
for tag, count, active, axis, distance, result in (
        ('empty', 0, 0, None, 0, -1), ('inactive', 1, 0, None, 0, -1),
        ('camera coincident', 1, 1, None, 0, -1), ('far', 1, 1, 0, 1000000, -1),
        ('right', 1, 1, 0, 1, -1), ('behind', 1, 1, 1, -1, -1),
        ('forward', 1, 1, 1, 1, 0)):
    writes = [('$this->mBoids.mCount', str(count)), (B + 'mActive', str(active))]
    for row in range(3):
        value = camera + 'm%d3' % row
        if axis is not None:
            value += '+(%s)*%s' % (distance, camera + 'm%d%d' % (row, axis))
        writes.append((B + 'mTransform.m%d3' % row, value))
    add(writes, '$bsw2_expect($this->FindBestLockOnIndex(*$mgr)==%d)' % result)

# Typed read-only safe-zone fixture: no virtual call or destructor on unconstructed storage.
for tag, zone_id, object_result, radius, touching, inside in (
        ('no zone', 65535, 0, 10, 0, 0), ('missing object', 0, 0, 10, 0, 0),
        ('inside', 0, '$bs_zone', 10, 1, 1), ('outside', 0, '$bs_zone', 1, 1, 0),
        ('boundary', 0, '$bs_zone', 5, 1, 0)):
    writes = [(B + 'mTransform.m03', '5.f'), (B + 'mTransform.m13', '0.f'),
              (B + 'mTransform.m23', '0.f'), (B + 'mTouchingSafeZone', '0'),
              (B + 'mInSafeZone', '0'), ('$this->mSafeZoneAvoidance', '2.f'),
              ('$bs_zone->mScale.mX', str(radius))]
    call = '($bs_ahead->mX=0.f,$bs_ahead->mY=0.f,$bs_ahead->mZ=0.f,'
    call += '$this->ApplySafeZoneAvoidance(*$mgr,$this->mBoids.mItems[0],*$bs_near,*$bs_ahead),'
    call += '$bsw2_expect(%smTouchingSafeZone==%d && %smInSafeZone==%d && $bs_ahead->mX==%d.f && $bs_ahead->mY==0.f && $bs_ahead->mZ==0.f))' % (B, touching, B, inside, 2 if touching else 0)
    add(writes, call, [('CSafeZoneManager::SphereTouchingWhichSafeZone(', zone_id),
                       ('CStateManager::GetObjectById(', object_result)])
add([], '(free($bs_zone),free($bs_ahead),free($bs_near),1)')

CALLS = [] if SKIP else [('bsw2 native queue', 'CBacteriaSwarm::Think(',
    '((CStateManager*)$rsi)->mUpdateFrameIdx>500 && ((CBacteriaSwarm*)$rdi)->mBoids.mCount>0',
    queue, 'real camera/boid geometry and safe-zone callee-answer fixtures')]

# One returned triangle is enough. Hooking every triangle in every surface search
# makes a frame take minutes, so this fixture disables itself after one real call.
# The sret pointer in rax names only the caller's temporary, never the collision tree.
class TriangleExit(gdb.FinishBreakpoint):
    def stop(self):
        pointer = int(gdb.parse_and_eval('$rax'))
        lv = '((CCollisionSurface*)%d)->mFlags' % pointer
        old = int(gdb.parse_and_eval(lv))
        gdb.execute('set var %s=%d' % (lv, old | 0x10000000))
        log('TRIANGLE RESULT forced ptr=%#x flags=%#x -> %#x' %
            (pointer, old, old | 0x10000000))
        return False

class TriangleEntry(gdb.Breakpoint):
    def stop(self):
        caller = gdb.newest_frame().older()
        if caller and (caller.name() or '').startswith('CBacteriaSwarm::FindBestSurface'):
            TriangleExit(gdb.newest_frame(), internal=True)
            self.enabled = False
        return False

if not SKIP:
    TriangleEntry('*_ZNK23CCollisionPrimitiveData11GetTriangleEj', internal=True)
