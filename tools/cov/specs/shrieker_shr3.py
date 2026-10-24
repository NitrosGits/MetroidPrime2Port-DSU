# Shrieker (module 69) shr3 lane: direct calls with stubs / hooks for PathFind, ShouldDodge, ApplyMeleeDamage, and the projectile arm of
# DoUserAnimEvent (the LaunchProjectile result forced null at its return). Driver tools/cov/specs/shrieker_shr3.con (greatbridge, 1700 frames):
#   COV_GDB_SCRIPT=tools/cov/force_ret.py BUILD=$PWD/build-port-cov MP2_ALLOW_PTRACE=1 tools/cov/force.sh tools/cov/specs/shrieker_shr3.py <out> \
#     3dfd2249,02_swamp_greatbridge --con tools/cov/specs/shrieker_shr3.con --fast --frames 1700
import os
import gdb

CLASS = 'CShrieker'
FORCE_RET = []
gdb.execute('set $g3_m = 0')
gdb.execute('set $g3_mode = 3')   # 3: BuildNearList calls made by ApplyMeleeDamage get one bogus id, those made by ShouldDodge a non-empty list (natural runs only: both need the state they run in)


def _syms(prefix):
    out = []
    for ln in open(os.environ['COV_NM']):
        parts = ln.rstrip('\n').split('\t')
        if len(parts) == 2 and parts[1].startswith(prefix) and '.cold' not in parts[1] and 'clone' not in parts[1]:
            f = parts[0].split()
            if len(f) == 3 and f[1] in 'tTW':
                out.append(f[2])
    return out


class _Fin(gdb.FinishBreakpoint):
    def __init__(self, frame, lst, mode):
        super().__init__(frame, internal=True)
        self.lst, self.mode = lst, mode

    def stop(self):
        gdb.execute('set var *(int*)%d = 1' % self.lst)                  # reserved_vector mCount := 1
        if self.mode == 2:
            gdb.execute('set var *(unsigned short*)%d = 64515' % (self.lst + 4))  # data[0]: a version-63 id, no object
        return False

    def out_of_scope(self):
        pass


class _NearHook(gdb.Breakpoint):
    # $g3_mode 1: BuildNearList returns one id (ShouldDodge: a non-empty near list); 2: the id has no object (ApplyMeleeDamage)
    def stop(self):
        mode = int(gdb.parse_and_eval('$g3_mode'))
        if mode == 3:
            caller = gdb.newest_frame().older()
            cn = (caller.name() or '') if caller is not None else ''
            mode = 2 if 'ApplyMeleeDamage' in cn else (1 if 'ShouldDodge' in cn else 0)
        with open(os.path.join(os.environ['COV_OUT'], 'shr3.log'), 'a') as fh:
            fh.write('BuildNearList mode %d caller %s\n' % (mode, (gdb.newest_frame().older().name() or '?')[:60]))
        if mode:
            _Fin(gdb.newest_frame(), int(gdb.parse_and_eval('$rsi')), mode)
        return False


for _s in _syms('CStateManager::BuildNearList('):
    _NearHook('*' + _s, internal=True)

NOTARGET = ('*(unsigned short*)&$this->mTargetId', '65535')
PLAYER = ('*(unsigned short*)&$this->mTargetId', '*(unsigned short*)&$P->mUniqueId')
TD = '*(CTriggerData*)$this'
CALLS = [('Shrieker shr3 direct calls', 'CShrieker::Think(', '1', [
    ([NOTARGET], '$this->PathFind(*$mgr, 1, 0.016f)', []),
    ([PLAYER], '$this->PathFind(*$mgr, 1, 0.016f)', [('CPatterned::PathShagged(', 1)]),
    ([('$this->mDodgeTimer', '0.0'), ('$this->mData.mDodgePercentage', '1000.0'), ('$g3_mode', '1')], '$this->ShouldDodge(*$mgr, %s)' % TD,
     [('CPathFindSearch::PathExists(', 0), ('CStateManager::RayCollideWorld(CVector3f const&, CVector3f const&, CMaterialFilter const&, CActor const*)', 1)]),
    ([('$this->mDodgeTimer', '0.0'), ('$this->mData.mDodgePercentage', '1000.0')], '$this->ShouldDodge(*$mgr, %s)' % TD,
     [('CPathFindSearch::PathExists(', 0), ('CStateManager::RayCollideWorld(CVector3f const&, CVector3f const&, CMaterialFilter const&, CActor const*)', 0)]),
    # Launching on a dying body that was already launched
    ([('$g3_m', '(long)calloc(1, 32)'), ('*(int*)($g3_m + 8)', '%d' % 0x584c4155), ('$this->mAlive', '0'), ('$this->mDying', '1'), ('$this->mLaunched', '1')],
     '$this->AcceptScriptMsg(*$mgr, *(CScriptMsg*)$g3_m)', []),
], 'PathFind without a target / path shagged, ShouldDodge with a non-empty near list, ApplyMeleeDamage with a non-actor near id')]
FORCE_INNER = [('LaunchProjectile null', 'CShrieker::DoUserAnimEvent(', 'CPatterned::LaunchProjectile(', 1, {0: [('$rax', '0')]},
                'the projectile event gets no projectile back')]
