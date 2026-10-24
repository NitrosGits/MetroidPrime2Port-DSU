# Grenchler (module 27) gren7: forced arms of UpdateBeamTrace 32C4 (and the helpers of the later bodies), entrance room.
# force_ret.py CALLS direct calls with stubs (CanBeNormalized lists), plus two spec-local gdb hooks:
#   $g7_mode 1: every RayDynamicIntersection call after the first (the player ray) returns hitId = a version-63 id (no object);
#   $g7_mode 2: the same calls return hitId = this Grenchler, and CDamageVulnerability::GetVulnerability returns mEffect = 3 (rax high half) (immune).
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 \
#     tools/cov/force.sh tools/cov/specs/grenchler_force7.py <out> 3dfd2249,01_swamp_entrance \
#     --con tools/cov/specs/grenchler_gren6c.con --fast --frames 600
import os
import gdb

CLASS = 'CGrenchler'
FORCE_RET = []
gdb.execute('set $g7_mode = 0')
gdb.execute('set $g7_n = 0')
gdb.execute('set $g7_uid = 0')
_G7_LOG = os.path.join(os.environ.get('COV_OUT', '/tmp'), 'g7.log')


def _g7log(msg):
    with open(_G7_LOG, 'a') as fh:
        fh.write(msg + '\n')


def _sym(prefix):
    for ln in open(os.environ['COV_NM']):
        parts = ln.rstrip('\n').split('\t')
        if len(parts) == 2 and parts[1].startswith(prefix) and '.cold' not in parts[1] and 'clone' not in parts[1]:
            f = parts[0].split()
            if len(f) == 3 and f[1] in 'tTW':
                return f[2]
    return None


class _Write(gdb.FinishBreakpoint):
    def __init__(self, frame, expr):
        super().__init__(frame, internal=True)
        self.expr = expr

    def stop(self):
        try:
            gdb.execute('set var ' + self.expr)
        except gdb.error as e:
            _g7log('write failed %s: %s' % (self.expr, str(e)[:80]))
        return False

    def out_of_scope(self):
        pass


class _RayHook(gdb.Breakpoint):
    def stop(self):
        mode = int(gdb.parse_and_eval('$g7_mode'))
        if mode == 0:
            return False
        n = int(gdb.parse_and_eval('$g7_n'))
        gdb.execute('set $g7_n = %d' % (n + 1))
        if n >= 1:  # the loop rays; call 0 is the player ray
            val = 63 * 1024 + 3 if mode == 1 else int(gdb.parse_and_eval('$g7_uid'))
            ptr = int(gdb.parse_and_eval('$rdx'))
            _g7log('ray call %d mode %d: hitId at %#x := %d' % (n, mode, ptr, val))
            _Write(gdb.newest_frame(), '*(unsigned short*)%d = %d' % (ptr, val))
        return False


class _VulnHook(gdb.Breakpoint):
    def stop(self):
        if int(gdb.parse_and_eval('$g7_mode')) == 2:
            # CWeaponTypeVulnerability {float, int effect} comes back in rax (effect = the high half)
            _g7log('GetVulnerability: mEffect := 3 in the returned rax')
            _Write(gdb.newest_frame(), '$rax = ($rax & 0xffffffff) | (3UL << 32)')
        return False


_ray = _sym('CGameCollision::RayDynamicIntersection(')
_vul = _sym('CDamageVulnerability::GetVulnerability(')
if _ray and _vul:
    _RayHook('*' + _ray, internal=True)
    _VulnHook('*' + _vul, internal=True)
else:
    _g7log('hook symbols missing: %s %s' % (_ray, _vul))

_T0 = [('$this->mEffectC.mTraceState', '0'), ('$this->mEffectC.mBeamLength', '0.5')]
_CALL = '$this->UpdateBeamTrace(*$mgr)'
CALLS = [('UpdateBeamTrace direct calls', 'CGrenchler::PreRender(',
          '((CGrenchler*)$rdi)->mIsGrappleGuardian == 0', [
    ([('$this->mEffectC.mTraceState', '1')], _CALL, []),                       # player-ray block and static block skipped
    (_T0, _CALL, [('CVector3f::CanBeNormalized(', [0, 1, 1])]),                 # beam target on the horn: forward used
    (_T0, _CALL, [('CVector3f::CanBeNormalized(', [1, 0, 1])]),                 # sideA not normalisable
    (_T0, _CALL, [('CVector3f::CanBeNormalized(', [1, 1, 0])]),                 # sideB not normalisable
    (_T0 + [('$g7_mode', '1'), ('$g7_n', '0'), ('$g7_uid', '*(unsigned short*)&$this->mUniqueId')], _CALL, []),  # ray hits an id with no object
    (_T0 + [('$g7_mode', '2'), ('$g7_n', '0'), ('$g7_uid', '*(unsigned short*)&$this->mUniqueId')], _CALL, []),  # ray hits an actor, immune
], 'UpdateBeamTrace: trace state 1, zero-length beam target, vertical beam sides, hit id with no actor, immune hit')]

_GP = '$this->GrapplePull(*$mgr, 1, 0.016f)'
_NOPULL = ('CGrenchler::PullPlayer(', 0)
_LAST_AT_PLAYER = [('$this->mEffectC.mLastHornPosition.mX', '$P->mTransform.m03'),
                   ('$this->mEffectC.mLastHornPosition.mY', '$P->mTransform.m13'),
                   ('$this->mEffectC.mLastHornPosition.mZ', '$P->mTransform.m23')]
CALLS.append(('GrapplePull direct calls', 'CGrenchler::PreRender(', '((CGrenchler*)$rdi)->mIsGrappleGuardian == 0', [
    ([], '$this->GrapplePull(*$mgr, 3, 0.016f)', []),                           # switch default
    ([], _GP, [('CVector3f::CanBeNormalized(', 0), _NOPULL]),                    # horn straight above the player: no pull
    (_LAST_AT_PLAYER, _GP, [_NOPULL]),                                         # moved away since last frame, no backward input
    (_LAST_AT_PLAYER + [('$P->mBackwardInput', '0.5')], _GP, [_NOPULL]),          # moved away, backing off
    ([('$this->mEffectC.mLastHornPosition.mX', '100000.0'), ('$P->mBackwardInput', '0.5')], _GP, [_NOPULL]),  # coming closer, backing off
], 'GrapplePull Update: msg default, no pull direction, horn moved away with and without backward input, closer with backward input'))
