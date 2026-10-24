# Splinter (module 74) spl6 lane, force_ret.py spec.  What the natural fights (splinter_all.sh) cannot reach:
#  * a Mega Splinter (no shipped placement is one): a gdb breakpoint after the loader's `.inc` writes `sldrThis.isMegaSplinter = 1` on the SPL_MEGA-th load
#    (default the first); the spit projectile asset is invalid and the projectile constructor crashes, so CPatterned::LaunchProjectile answers null when
#    called from a DoUserAnimEvent;
#  * DoUserAnimEvent: the event type (rcx) cycles over 0 / 12 / 37 / 38 / 39 / 1 on the first twelve calls (SPL_ROT rotates the cycle);
#  * LoadSplinter: SPL_NOMODEL=<n> (the n-th load has no animation set: the loader returns null), SPL_SM2=<n> (the n-th load gets a state machine 2);
#  * ExplodeSac: SPL_PART=1 gives every load a particle effect (pART := gibParticles);
#  * AcceptScriptMsg: Launching with the jump phase 2 / not 2 (launched straight up), AreaLoaded with an invalid team id and a sac on a cable (when the
#    message has a sender), the CIngSnatchingSwarm cast answering its argument (swarm sender);
#  * CALLS from a teamed Splinter's SPL_GN-th Think (default 300): ExplodeSac, CanEvade (also with mIsDarkWorld forced), AttackPointIsLegal, IsAttackBlocked,
#    FindHideHint, HasClearPathTo, PathFind, and AcceptScriptMsg with a hand-made CScriptMsg (AreaLoaded / Delete / Deactivate / Create / Damage);
#  * CALLS from the 200th Think of the real Mega Splinter: Think (ing possession, snatched, boss id), PathFind Activate, AttackPointIsLegal.
# Runs (each on build-port-cov, all `COV_ROOT=$PWD COV_GDB_SCRIPT=tools/cov/force_ret.py MP2_ALLOW_PTRACE=1 tools/cov/force.sh tools/cov/specs/splinter_spl6.py <out>
#   3bfa3eff,02_temple_exploration --con tools/cov/specs/splinter_spl6.con --fast --frames 3400`, then gcov_report.py with the natural runs FIRST):
#   Mega:   SPL_MEGA=1 SPL_GN=999999 SPL_PART=1                     (four of them with SPL_ROT 0..3 and SPL_MEGA 1 / 2 / 99 for the event types)
#   direct: SPL_MEGA=99 SPL_NOMGA=1 SPL_PART=1                        (the CALLS from the teamed Splinter)
#   loader: SPL_NOMODEL=3 SPL_MEGA=99 / SPL_SM2=2 SPL_MEGA=99
# splinter_spl6.con adds eight attack-block hints (type 24) in a ring and four hide hints (type 25, one inactive) to the temple fight, plus the messages.
# Order matters for gcov_report.py: the first directory must hold the full set of gcda (a force run alone loses arms in the merge).
import os
import re
import gdb

CLASS = 'CSplinter'
# CSplinter::DoUserAnimEvent: the event type (rcx) cycles over Projectile / Landing / 37 / 38 / 39 / 1 (in the jump table, no case) on successive calls, so the Mega and the plain Splinters both see every case.
_TYPES = [0, 12, 37, 38, 39, 1]
FORCE_RET = [('DoUserAnimEvent types', 'CSplinter::DoUserAnimEvent(', 12,
              [(i, [('$rcx', str(_TYPES[(i + int(os.environ.get('SPL_ROT', '0'))) % 6]))]) for i in range(12)], 'user event type cycled; Mega and plain splinters')]
# AcceptScriptMsg (every call; the writes last one call): Launching alternates the jump phase 2 / 0, AreaLoaded sees an invalid team id and a sac on a cable.
_MSG = '((CScriptMsg*)$rdx)->m_msg'
FORCE_RET += [
    ('AcceptScriptMsg Launching phase', 'CSplinter::AcceptScriptMsg(', 1,
     [(0, [('$this->mJump.mPhase', '%s == 0x584c4155 ? (($sp6l = $sp6l + 1) %% 2 ? 2 : 0) : $this->mJump.mPhase' % _MSG)] +
          # a launch from where the splinter stands, straight up (a garbage start point would throw it out of the world)
          [('$this->mJump.mStart.m%s' % a, '%s == 0x584c4155 ? $this->mPosition.m%s : $this->mJump.mStart.m%s' % (_MSG, a, a)) for a in 'XYZ'] +
          [('$this->mJump.mHeight', '%s == 0x584c4155 ? 2.0 : $this->mJump.mHeight' % _MSG)])], 'Launching with the jump phase 2 and not 2, launched upward'),
    ('AcceptScriptMsg AreaLoaded', 'CSplinter::AcceptScriptMsg(', 1,
     [(0, [('$this->mTeamAiMgrId.value', '%s == 0x58414c44 && ((CScriptMsg*)$rdx)->mSenderId.value != 65535 ? 65535 : $this->mTeamAiMgrId.value' % _MSG),
           ('$this->mSac.mSacOnCable', '%s == 0x58414c44 && ((CScriptMsg*)$rdx)->mSenderId.value != 65535 ? %d : $this->mSac.mSacOnCable' % (_MSG, 0 if os.environ.get('SPL_NOSAC') else 1))])], 'AreaLoaded: team id invalid, sac on a cable'),
]
if os.environ.get('SPL_NOMSG'):
    del FORCE_RET[1:]
gdb.execute('set $sp6l = 0')
gdb.execute('set $sp6n = 0')
gdb.execute('set $sp6k = 0')
gdb.execute('set $sp6b = 0')
gdb.execute('set $sp6m = 0')
CALLS = []
_src = open(os.path.join(os.environ.get('COV_ROOT', '.'), 'src/MetroidPrime/Enemies/CSplinter.cpp')).read().split('\n')
_L = [i + 1 for i, l in enumerate(_src) if re.search(r'^  if \(sldrThis\.patterned\.stateMachine2 != kInvalidAssetId\) \{', l)][0]
_n = {'n': 0}


class _Mega(gdb.Breakpoint):
    def stop(self):
        _n['n'] += 1
        if _n['n'] == int(os.environ.get('SPL_MEGA', '1')):
            gdb.execute('set var sldrThis.isMegaSplinter = 1')
        return False


_Mega('CSplinter.cpp:%d' % _L, internal=True)


# LoadSplinter's other two arms: SPL_NOMODEL=<n> makes the n-th Splinter load have no animation set (LdrToModelData answers an empty optional: the loader
# returns null); SPL_SM2=<n> gives the n-th load a state machine 2 (stateMachine2 := stateMachine; no shipped placement has one).
_M = [i + 1 for i, l in enumerate(_src) if re.search(r'^  rstl::optional_object< CModelData > modelData\(', l)][0]
_k = {'nm': 0, 'sm': 0}


class _NoModel(gdb.Breakpoint):
    def stop(self):
        _k['nm'] += 1
        if _k['nm'] == int(os.environ.get('SPL_NOMODEL', '0')):
            gdb.execute('set var sldrThis.patterned.animationInformation.ancs = 4294967295')
        return False


class _Sm2(gdb.Breakpoint):
    def stop(self):
        _k['sm'] += 1
        if _k['sm'] == int(os.environ.get('SPL_SM2', '0')):
            gdb.execute('set var sldrThis.patterned.stateMachine2 = sldrThis.patterned.stateMachine')
        return False


_NoModel('CSplinter.cpp:%d' % _M, internal=True)
_Sm2('CSplinter.cpp:%d' % _L, internal=True)


def _syms(prefix):
    out = []
    for ln in open(os.environ['COV_NM']):
        parts = ln.rstrip('\n').split('\t')
        if len(parts) == 2 and parts[1].startswith(prefix) and '.cold' not in parts[1] and 'clone' not in parts[1]:
            f = parts[0].split()
            if len(f) == 3 and f[1] in 'tTW':
                out.append(f[2])
    return out


class _LaunchStub(gdb.Breakpoint):
    def stop(self):
        caller = gdb.newest_frame().older()
        if caller is not None and 'DoUserAnimEvent' in (caller.name() or ''):
            gdb.execute('set $rax = 0')
            gdb.execute('set $rip = *(unsigned long*)$rsp')
            gdb.execute('set $rsp = $rsp + 8')
        return False


for _ls in _syms('CPatterned::LaunchProjectile('):
    _LaunchStub('*' + _ls, internal=True)


class _SwarmCast(gdb.Breakpoint):
    # TCastToPtr<CIngSnatchingSwarm>(CEntity*) answers its argument inside AcceptScriptMsg (no swarm is in the area; the body only reads its id)
    def stop(self):
        caller = gdb.newest_frame().older()
        if caller is not None and 'AcceptScriptMsg' in (caller.name() or ''):
            gdb.execute('set $rax = $rdi')
            gdb.execute('set $rip = *(unsigned long*)$rsp')
            gdb.execute('set $rsp = $rsp + 8')
        return False


for _ls in _syms('CIngSnatchingSwarm* TCastToPtr<CIngSnatchingSwarm>('):
    _SwarmCast('*' + _ls, internal=True)


class _Part(gdb.Breakpoint):
    # SPL_PART=1: every Splinter load gets a particle effect (pART := gibParticles) so ExplodeSac's `mParticle.mEffect.valid()` arm can run
    def stop(self):
        if os.environ.get('SPL_PART'):
            gdb.execute('set var sldrThis.pART = sldrThis.patterned.gibParticles')
        return False


_Part('CSplinter.cpp:%d' % _L, internal=True)

# ---- direct calls (force_ret CALLS), from a Splinter's Think. Writes last one call. -------------------------------------------------
_B = ('$sp6b', '0')                                    # (unused: AttackPointIsLegal ignores its CTriggerData, any bytes will do)
_TD = '*(CTriggerData*)$this'
_X, _Y = '$this->mPosition.mX', '$this->mPosition.mY'
_NOTID = '65535'


def _pos(dx, dy):
    return [(_X, '%s + %s' % (_X, dx)), (_Y, '%s + %s' % (_Y, dy))]


_dirs = [0, 1, 2, 3, 7]
_G = []
# ExplodeSac: holding / not, grabbed invalid / the player / an id no object has
_G += [([_B, ('$this->mSac.mHoldingSac', '0')], '$this->ExplodeSac(*$mgr)', []),
       ([_B, ('$this->mSac.mHoldingSac', '1'), ('$this->mSac.mGrabbedId.value', _NOTID)], '$this->ExplodeSac(*$mgr)', []),
       ([_B, ('$this->mSac.mHoldingSac', '1'), ('$this->mSac.mGrabbedId.value', '$P->mUniqueId.value')], '$this->ExplodeSac(*$mgr)', []),
       ([_B, ('$this->mSac.mHoldingSac', '1'), ('$this->mSac.mGrabbedId.value', '3000')], '$this->ExplodeSac(*$mgr)', []),
       ([_B, ('$this->mSac.mHoldingSac', '1'), ('$this->mSac.mGrabbedId.value', '$P->mUniqueId.value'), ('$this->mParticle.mEffect.m_valid', '0')], '$this->ExplodeSac(*$mgr)', [])]
# CanEvade: every direction where the splinter is, 1000 units away along four axes (the team-centre distance arms), and an unknown direction
_G += [([], '$this->CanEvade(*$mgr, (CSplinter::EEvadeDirection)%d)' % d, []) for d in _dirs]
for dx, dy in ((1000, 0), (0, 1000), (-1000, 0), (0, -1000)):
    _G += [(_pos(dx, dy), '$this->CanEvade(*$mgr, (CSplinter::EEvadeDirection)%d)' % d, []) for d in range(4)]
# the dark-world arms in the light-world area: mIsDarkWorld forced for the call, the ing-possession and the safe-zone answers stubbed
_GC = [([('$mgr->mIsDarkWorld', '1')], '$this->CanEvade(*$mgr, (CSplinter::EEvadeDirection)%d)' % d, [('CPatterned::IsIngPossessed(', 1), ('CSafeZoneManager::PointIsInHurtfulSafeZone(', h)]) for d in (0, 1) for h in (0, 1)]
_G += _GC
_G += [([], '$this->CanEvade(*$mgr, (CSplinter::EEvadeDirection)0)', [('CPatterned::IsIngPossessed(', 1), ('CSafeZoneManager::PointIsInHurtfulSafeZone(', 1)]),
       ([], '$this->CanEvade(*$mgr, (CSplinter::EEvadeDirection)0)', [('CPatterned::IsIngPossessed(', 1), ('CSafeZoneManager::PointIsInHurtfulSafeZone(', 0)])]
# AttackPointIsLegal: player in another area; the retry counter high with the attack not blocked / blocked; blocked with no retries; mega arm
_G += [([_B, ('$this->mAreaId.value', '$this->mAreaId.value + 1')], '$this->AttackPointIsLegal(*$mgr, %s)' % _TD, []),
       ([_B, ('$this->mUnknown8e0', '10')], '$this->AttackPointIsLegal(*$mgr, %s)' % _TD, [('CSplinter::IsAttackBlocked(', 0)]),
       ([_B, ('$this->mUnknown8e0', '10')], '$this->AttackPointIsLegal(*$mgr, %s)' % _TD, [('CSplinter::IsAttackBlocked(', 1)]),
       ([_B, ('$this->mUnknown8e0', '0')], '$this->AttackPointIsLegal(*$mgr, %s)' % _TD, [('CSplinter::IsAttackBlocked(', 1)]),
       ([_B, ('$this->mUnknown8e0', '0')], '$this->AttackPointIsLegal(*$mgr, %s)' % _TD, [('CSplinter::IsMega(', 1)])]
# IsAttackBlocked: the splinter in the middle of the hint ring (yaw 0 / 90 / 180 / 270), 1000 units from it
_C = [('$this->mPosition.mX', '205.0'), ('$this->mPosition.mY', '-215.0'), ('$this->mPosition.mZ', '-40.0')]
for yaw in ((1, 0), (0, 1), (-1, 0), (0, -1)):
    _G += [(_C + [('$this->mTransform.m01', str(float(yaw[0]))), ('$this->mTransform.m11', str(float(yaw[1])))], '$this->IsAttackBlocked(*$mgr)', [])]
_G += [(_pos(1000, 0), '$this->IsAttackBlocked(*$mgr)', []),
       ([('$this->mAreaId.value', '$this->mAreaId.value + 1')], '$this->IsAttackBlocked(*$mgr)', [])]
# FindHideHint: hint in use / free sequences and path answers
_seqs = [[0] * 16, [1] * 16, [1, 0] * 8, [0, 1] * 8, [0, 0, 1, 1] * 4, [1, 1, 0, 0] * 4, [0, 1, 1, 0] * 4]
for q in _seqs:
    _G += [([], '$this->FindHideHint(*$mgr)', [('CScriptAIHint::GetInUse(', q)])]
_G += [([], '$this->FindHideHint(*$mgr)', [('CScriptAIHint::GetInUse(', q), ('CPathFindSearch::PathExists(', 0)]) for q in ([0, 0, 1, 1] * 4, [0, 0, 1, 0] * 4, [0, 0, 0, 1] * 4)]
for pe in (0, 1, 2):
    _G += [([], '$this->FindHideHint(*$mgr)', [('CScriptAIHint::GetInUse(', [0] * 16), ('CPathFindSearch::PathExists(', pe)])]
_G += [([('$this->mAreaId.value', '$this->mAreaId.value + 1')], '$this->FindHideHint(*$mgr)', [])]
# HasClearPathTo (the static `filter` guard) at the player
_G += [([], '$this->HasClearPathTo(*$mgr, *(CVector3f*)&$P->mPosition)', [])]
# PathFind: Update with the stuck timer off, the jump check due and FindJump answering true / false
for fj in (1, 0):
    _G += [([('$this->mStuckDeadline', '0.0'), ('$this->mUnknown944', '1.0')], '$this->PathFind(*$mgr, (EStateMsg)1, 0.016f)', [('CSplinter::FindJump(', fj)])]

# AcceptScriptMsg by direct call (a hand-made CScriptMsg): AreaLoaded (an active, teamed splinter), Delete, Deactivate, Create, Damage; last, they take the splinter out of its team
_MB = ('$sp6m', '(long)((void*(*)(unsigned long))malloc)(64)')
for _m in (0x58414c44, 0x5844454c, 0x44435456, 0x58435254, 0x58444d47):
    _G += [([_MB, ('((CScriptMsg*)$sp6m)->mSenderId.value', '65535'), ('((CScriptMsg*)$sp6m)->m_id.value', '$this->mUniqueId.value'),
             ('((CScriptMsg*)$sp6m)->m_msg', '%d' % _m)], '$this->AcceptScriptMsg(*$mgr, *(CScriptMsg*)$sp6m)', [])]
_MGA = []
# the real Mega splinter: Think with the ing-possession blend high / low, snatched or not, and the boss id already this splinter's
_MGA += [([('$this->mIngPossessionBlend', '1.0')], '$this->Think(0.016f, *$mgr)', [('CPatterned::IsIngPossessed(', 1), ('CPatterned::IsBeingSnatched(', 0)]),
         ([('$this->mIngPossessionBlend', '1.0')], '$this->Think(0.016f, *$mgr)', [('CPatterned::IsIngPossessed(', 1), ('CPatterned::IsBeingSnatched(', 0)]),
         ([('$this->mIngPossessionBlend', '1.0')], '$this->Think(0.016f, *$mgr)', [('CPatterned::IsIngPossessed(', 1), ('CPatterned::IsBeingSnatched(', 1)]),
         ([('$this->mIngPossessionBlend', '0.0')], '$this->Think(0.016f, *$mgr)', [('CPatterned::IsIngPossessed(', 1), ('CPatterned::IsBeingSnatched(', 0)]),
         ([], '$this->Think(0.016f, *$mgr)', [('CPatterned::IsIngPossessed(', 0)]),
         ([], '$this->PathFind(*$mgr, (EStateMsg)0, 0.016f)', []),
         ([_B, ('$this->mUnknown8e0', '0')], '$this->AttackPointIsLegal(*$mgr, %s)' % _TD, [])]

CALLS = [('Splinter direct calls', 'CSplinter::Think(', '((CSplinter*)$rdi)->mTeamAiMgrId.value != 65535 && ($sp6n = $sp6n + 1) >= %s' % os.environ.get('SPL_GN', '300'), ([] if os.environ.get('SPL_NOG') else _G), 'ExplodeSac / CanEvade / AttackPointIsLegal / IsAttackBlocked / FindHideHint / HasClearPathTo / PathFind arms'),
         ('Mega Splinter direct calls', 'CSplinter::Think(', '((CSplinter*)$rdi)->mSpit.m_valid && ($sp6k = $sp6k + 1) >= 200', [] if os.environ.get('SPL_NOMGA') else _MGA,
          'Think / PathFind / AttackPointIsLegal on the Mega Splinter')]
