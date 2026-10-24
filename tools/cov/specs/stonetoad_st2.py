# st2: native StoneToad calls, using tools/cov/force_ret.py and the coverage build.
# Forced callee answers are explicitly fixtures, not claims about shipped weapon data.
import gdb
import os
import re

CLASS = 'CStoneToad'
SPEC = []  # ApproachWeight is inlined; its own gcov row still records its arms.

# Exact normalized axes make the active target exactly zero as well as positive.
# CVector2f (two floats) is returned in xmm0 under this host's SysV ABI. The shared
# force_ret helper cannot hook an inferior-call dummy frame: those direct calls
# retain their real look vector; ordinary Think calls receive these axis fixtures.
FORCE_INNER = [('B48 exact look axes', 'CStoneToad::UpdateLookAt(',
                'CVector2f::AsNormalized(', 4,
                {i: [('$xmm0.v4_float[0]', str(x) + '.f'),
                     ('$xmm0.v4_float[1]', str(y) + '.f')]
                 for i, (x, y) in enumerate(((0, 1), (1, 0), (-1, 0), (0, -1)))},
                'forced exact look axes: zero and positive active smoothing targets')]
# acos rounding can make a tiny real angle zero; explicitly force that answer too,
# independently of the look-axis fixture, to cover the negative-side zero-target copies.
FORCE_INNER += [('B48 zero angle', 'CStoneToad::UpdateLookAt(',
                 'CVector3f::GetAngleDiff(', 5, {0: [('$xmm0.v4_float[0]', '0.f')]},
                 'forced zero angle, including negative look axes')]

class Expect(gdb.Function):
    def __init__(self):
        super().__init__('st2_expect')

    def invoke(self, value):
        assert bool(value), 'StoneToad oracle failed'
        with open(os.environ['COV_OUT'] + '/st2-oracles.log', 'a') as f:
            f.write('PASS\n')
        return 1

Expect()
queue = []

# gcov does not report the min/max expressions compiled to branchless selects.
# Observe the actual SSE inputs; inline DWARF variables can be optimized out or
# resolve after the select. Offsets are discovered from this build, not assumed.
observed = {}
class Observe(gdb.Breakpoint):
    def __init__(self, offset, label, expr):
        super().__init__('*_ZN10CStoneToad12UpdateLookAtER13CStateManager+%d' % offset,
                         internal=True)
        self.label, self.expr = label, expr

    def stop(self):
        try:
            key = int(gdb.parse_and_eval(self.expr))
        except gdb.error as e:
            key = 'ERROR ' + str(e)
        k = (self.label, str(key))
        observed[k] = observed.get(k, 0) + 1
        return False

disasm = gdb.execute('disassemble CStoneToad::UpdateLookAt', to_string=True)
selects = re.findall(r'<\+(\d+)>:\s+(minss|maxss|cmpltss)\s+%(xmm\d+),%(xmm\d+)', disasm)
assert len(selects) == 17, selects
for offset, op, src, dst in selects:
    Observe(int(offset), '+%s %s' % (offset, op),
            '$%s.v4_float[0] %s $%s.v4_float[0]' %
            (dst, '>' if op == 'maxss' else '<', src))

def dump_observed(event):
    with open(os.environ['COV_OUT'] + '/st2-selects.log', 'w') as f:
        for (label, key), count in sorted(observed.items()):
            f.write('%s key=%s hits=%d\n' % (label, key, count))

gdb.events.exited.connect(dump_observed)

def add(writes, call, stubs=None):
    queue.append((writes, call, stubs or []))

add([], '($st_msg=(CScriptMsg*)calloc(1,sizeof(CScriptMsg)), '
    '$st_proj=(CGameProjectile*)calloc(1,sizeof(CGameProjectile)), '
    '$st_bomb=(CBomb*)calloc(1,sizeof(CBomb)), '
    '$st_power=(CPowerBomb*)calloc(1,sizeof(CPowerBomb)), '
    '$st_msg->mSenderId.value=65535, $st_msg->m_id=$this->mUniqueId, '
    '$st_msg->m_originator.value=65535, $st_msg->m_state=kSS_InvalidState, '
    '$st_anim=$this->mModelData.mPtr->mAnimData.mItem, '
    '$st_col=$this->mCollisionActorManager.mPtr->mJointDescriptions.mItems[0].mActorId, 1)')

# All octants, angle-clamped and small-angle targets, rising/falling/clamped weights.
# Raw transforms are restored by force_ret after each call, before the next game tick.
identity = [('$this->mTransform.m%d%d' % (r, c), '1.f' if r == c else '0.f')
            for r in range(3) for c in range(4)]
for x, y, z in ((-20, 20, -20), (20, 20, -20), (-20, 20, 20), (20, 20, 20),
                (0, 20, 20), (20, 20, 0), (0.1, 100, 3), (-0.1, 100, 3),
                (20, -20, 20), (-20, -20, -20)):
    for weight in ('0.f', '0.01f', '0.5f', '0.99f', '1.01f', '1.5f'):
        writes = identity + [('$P->mTransform.m03', str(x)),
                             ('$P->mTransform.m13', str(y)),
                             ('$P->mTransform.m23', str(z))]
        writes += [('$this->' + field, weight) for field in
                   ('mLookUpWeight', 'mLookDownWeight', 'mLookLeftWeight', 'mLookRightWeight')]
        # Four calls decouple each weight/target fixture from the four-axis phase.
        reset = ','.join('$this->%s=%s' % (field, weight) for field in
                         ('mLookUpWeight', 'mLookDownWeight', 'mLookLeftWeight', 'mLookRightWeight'))
        add(writes, '(' + ','.join([reset + ',$this->UpdateLookAt(*$mgr)'] * 4) + ',1)')

base_stub = [('CPatterned::AcceptScriptMsg(', 0)]
def message(name, writes=None, stubs=None, oracle=None):
    call = '($st_msg->m_msg=kSM_' + name + ', $this->AcceptScriptMsg(*$mgr,*$st_msg)'
    if oracle:
        call += ', $st2_expect(' + oracle + ')'
    call += ', 1)'
    add(writes or [], call, base_stub + (stubs or []))

# Create without animation / without a spatial primitive. Existing manager is preserved;
# neither path allocates new collision actors. Natural Create covers inactive body activation.
message('Create', [('$this->mModelData.mPtr->mAnimData.mItem', '0')])
message('Create', [('$this->mModelData.mPtr', '0')])
message('Create', [('$st_anim->mSpatialPrimitive.m_valid', '0')])
add([('$this->mCollisionActorManager.mPtr', '0')],
    '($st_msg->m_msg=kSM_Create,$this->AcceptScriptMsg(*$mgr,*$st_msg),'
    '$st_new=$this->mCollisionActorManager.mPtr,$st_new->Destroy(*$mgr),'
    '$st_new->~CCollisionActorManager(),free($st_new),'
    '$this->mCollisionActorManager.mPtr=0,1)',
    base_stub + [('CStateManager::ObjectById(', 0)])
message('AIUpdateDisabled', [('$this->mCollisionActorManager.mPtr', '0')])
message('Alert')
message('HitObject')
message('None')

# Approach connection resolving to no waypoint, plus an empty connection list.
message('AreaLoaded', [('$this->mWaypoints.mCount', '0')],
        [('CScriptWaypoint* TCastToPtr<CScriptWaypoint>(', 0)])
message('AreaLoaded', [('$this->mConnections.mCount', '0')])

# Cast results are typed POD read fixtures; only GetCurrentDamageInfo is read, never a
# virtual call or destructor on these unconstructed fixtures. The cast/WeaponHurts helpers
# themselves are covered naturally; forced answers isolate each short-circuit arm here.
cast_names = ['CGameProjectile* TCastToPtr<CGameProjectile>(',
              'CBomb* TCastToPtr<CBomb>(', 'CPowerBomb* TCastToPtr<CPowerBomb>(']
for kind, fixture in enumerate(('$st_proj', '$st_bomb', '$st_power')):
    for hurts in (0, 1):
        stubs = [(name, fixture if i == kind else 0) for i, name in enumerate(cast_names)]
        stubs += [('CDamageVulnerability::WeaponHurts(', hurts)]
        for sender in ('65535', '$st_col.value'):
            message('Damage' if hurts else 'ResistedDamage',
                    [('$st_msg->mSenderId.value', sender), ('$this->mProvoked', '0'),
                     ('$this->mReturnRequested', '1'), ('$this->mProvoker', '-1')], stubs,
                    '$this->mProvoked==%d && $this->mReturnRequested==%d && $this->mProvoker==%d'
                    % (hurts, 1 - hurts, 1 if hurts else -1))

# Changed active flag with no collision manager: real base Activate, not a stub.
add([('$this->mActive', '0'), ('$this->mCollisionActorManager.mPtr', '0')],
    '($st_msg->m_msg=kSM_Activate,$this->AcceptScriptMsg(*$mgr,*$st_msg),$st2_expect($this->mActive),1)')
add([], '(free($st_msg),free($st_proj),free($st_bomb),free($st_power),1)')

CALLS = [('st2 native queue', 'CStoneToad::UpdateLookAt(',
          '((CStateManager*)$rsi)->mUpdateFrameIdx>400', queue,
          'forced look quadrants, weights and message subconditions; native stores checked')]
