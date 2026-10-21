# gdb Python helper for tools/drivers/spacepirate_cov.sh: entry breakpoints on SpacePirate port
# functions that never stop the game. Each hit records an "arm key" (the listed expressions,
# evaluated on the entry registers, plus the return value for `ret` entries) and the counts are
# written as JSON when the inferior exits. Nothing in the game is changed, so no probe is needed.
#
# Env: SP_COV_NM (an `nm` dump: "<addr> <type> <mangled>\t<demangled>" lines, made by the runner),
# SP_COV_OUT (the JSON file), SP_COV_MAX (hits kept per breakpoint, default 400).
import json
import os

import gdb

THIS = '((CSpacePirate*)$rdi)'
# (label, demangled prefix, [expressions on the entry registers], flags)
#   flags: 'ret' records the low byte of the return value (bool results),
#          'caller=<s>' only counts hits whose caller frame's name contains <s>.
SPEC = [
    ('0 IsScanVisorSelfRender', 'CSpacePirate::IsScanVisorSelfRender(', [], ''),
    ('8 GetSearchPath', 'CSpacePirate::GetSearchPath(', [], ''),
    ('10 IsListening', 'CSpacePirate::IsListening(', [], ''),
    ('18 GetAttackTarget', 'CSpacePirate::GetAttackTarget(', [], ''),
    ('24 GetGravityConstant', 'CSpacePirate::GetGravityConstant(', [], ''),
    ('140 loader', 'PortLoadSpacePirate(', [], ''),
    ('17A0 GetRagDoll', 'CSpacePirate::GetRagDoll(', [], ''),
    ('18BC TryToBeCaptured', 'CSpacePirate::TryToBeCaptured(', [], ''),
    ('3220 ShouldFrenzy', 'CSpacePirate::ShouldFrenzy(', [], 'ret'),
    ('4F0C ProjectileInfo', 'CSpacePirate::ProjectileInfo(', [THIS + '->mProjectileInfo.m_valid'], ''),
    ('51C8 TrooperWarpOut', 'CSpacePirate::TrooperWarpOut(', [], ''),
    ('5610 Captured', 'CSpacePirate::Captured(', ['$rdx'], ''),
    ('58D8 LaunchGrenade', 'CSpacePirate::LaunchGrenade(', ['$rdx'], ''),
    ('5BF4 Deactivate', 'CSpacePirate::Deactivate(', ['$rdx'], ''),
    ('6328 BreakAttack', 'CSpacePirate::BreakAttack(', [], 'ret'),
    ('686C ShouldJumpBack', 'CSpacePirate::ShouldJumpBack(',
     [THIS + '->x8f4_28_noShuffleCloseCheck', THIS + '->xbac_holdPositionTime > 6'], 'ret'),
    ('689C AnimOver', 'CSpacePirate::AnimOver(', [THIS + '->x8f7_29_inWallHang'], 'ret'),
    ('69AC WallHang', 'CSpacePirate::WallHang(', ['$rdx'], ''),
    ('7078 WarpIn', 'CSpacePirate::WarpIn(', ['$rdx'], ''),
    ('77D0 ShouldWarpIn', 'CSpacePirate::ShouldWarpIn(',
     [THIS + '->xc04_warpPoint', THIS + '->x8f6_24_trooper', THIS + '->x8fa_26_'], 'ret'),
    ('8464 TurnAround', 'CSpacePirate::TurnAround(', ['$rdx'], ''),
    ('8670 Shuffle', 'CSpacePirate::Shuffle(', ['$rdx'], ''),
    ('8A84 InRange', 'CSpacePirate::InRange(', [], 'ret'),
    ('9478 Approach', 'CSpacePirate::Approach(', ['$rdx'], ''),
    ('98D8 Crouch', 'CSpacePirate::Crouch(', ['$rdx', THIS + '->x900_coverPoint.value'], ''),
    ('99F0 DoubleSnap', 'CSpacePirate::DoubleSnap(', ['$rdx'], ''),
    ('A310 CoverAttack', 'CSpacePirate::CoverAttack(', ['$rdx'], ''),
    ('A3C0 Cover', 'CSpacePirate::Cover(', ['$rdx'], ''),
    ('A71C ShouldAttack', 'CSpacePirate::ShouldAttack(', [], 'ret'),
    ('A8D0 CoveringFire', 'CSpacePirate::CoveringFire(', [], 'ret'),
    ('A988 CoverNearlyBlown', 'CSpacePirate::CoverNearlyBlown(', [], 'ret'),
    ('AA64 CoverBlown', 'CSpacePirate::CoverBlown(', [], 'ret'),
    ('ADB4 CoverCheck', 'CSpacePirate::CoverCheck(', [], 'ret'),
    ('B3D4 GetUp', 'CSpacePirate::GetUp(', ['$rdx'], ''),
    ('BBB0 Halt', 'CSpacePirate::Halt(', ['$rdx'], ''),
    ('C1A4 Generate', 'CSpacePirate::Generate(', ['$rdx'], ''),
    ('C468 PatternShagged', 'CSpacePirate::PatternShagged(', [], 'ret'),
    ('C488 Stuck', 'CSpacePirate::Stuck(', [], 'ret'),
    ('C488 Stuck: CPatterned::Stuck', 'CPatterned::Stuck(', [], 'ret caller=CSpacePirate::Stuck'),
    ('C488 Stuck: PathShagged', 'CPatterned::PathShagged(', [], 'ret caller=CSpacePirate::Stuck'),
    ('C9DC SetNonCinematicCollision', 'CSpacePirate::SetNonCinematicCollision(', [], ''),
    ('CA94 SetCinematicCollision', 'CSpacePirate::SetCinematicCollision(', [], ''),
    ('CE30 GetStrafeDir', 'CSpacePirate::GetStrafeDir(', [], ''),
    ('DE5C UpdateTeamTarget', 'CSpacePirate::UpdateTeamTarget(', [THIS + '->xba0_teamAiMgrId.value'], ''),
    ('DFD4 SquadJoin', 'CSpacePirate::SquadJoin(', [], ''),
    ('E090 GetOrigin', 'CSpacePirate::GetOrigin(', [], ''),
    ('E3D8 Touch', 'CSpacePirate::Touch(', [], ''),
    ('EDEC ChooseTarget', 'CSpacePirate::ChooseTarget(', [], ''),
    ('EFD4 ChooseTeamTarget', 'CSpacePirate::ChooseTeamTarget(', [], ''),
    ('F49C ctor', 'CSpacePirate::CSpacePirate(', [], ''),
    ('2EA8/1854/others: GetTouchBounds', 'CSpacePirate::GetTouchBounds(', [], ''),
]

MAX = int(os.environ.get('SP_COV_MAX', '400'))
results = {}


def record(label, key):
    r = results.setdefault(label, {'hits': 0, 'arms': {}})
    r['hits'] += 1
    k = repr(key)
    r['arms'][k] = r['arms'].get(k, 0) + 1


def ev(expr):
    try:
        v = gdb.parse_and_eval(expr)
        return int(v)
    except gdb.error as e:
        return 'err:' + str(e)[:40]


class RetBp(gdb.FinishBreakpoint):
    def __init__(self, frame, label, key):
        super().__init__(frame, internal=True)
        self.label, self.key = label, key

    def stop(self):
        try:
            rv = int(gdb.parse_and_eval('$rax')) & 0xff
        except gdb.error:
            rv = '?'
        record(self.label, self.key + ('ret', rv))
        return False

    def out_of_scope(self):
        record(self.label, self.key + ('ret', 'lost'))


class EntryBp(gdb.Breakpoint):
    def __init__(self, spec, label, exprs, flags):
        super().__init__(spec, internal=False)
        self.label, self.exprs, self.flags, self.n = label, exprs, flags, 0
        self.caller = None
        for f in flags.split():
            if f.startswith('caller='):
                self.caller = f[7:]

    def stop(self):
        if self.caller is not None:
            older = gdb.newest_frame().older()
            if older is None or self.caller not in (older.name() or ''):
                return False
        self.n += 1
        if self.n > MAX:
            self.enabled = False
            return False
        key = tuple(ev(e) for e in self.exprs)
        if 'ret' in self.flags.split():
            try:
                RetBp(gdb.newest_frame(), self.label, key)
            except (gdb.error, ValueError):
                record(self.label, key + ('ret', 'nofinish'))
        else:
            record(self.label, key)
        return False


def install():
    syms = []
    for ln in open(os.environ['SP_COV_NM']):
        parts = ln.rstrip('\n').split('\t')
        if len(parts) != 2:
            continue
        addr_type_mangled, demangled = parts
        f = addr_type_mangled.split()
        if len(f) == 3 and f[1] in 'tTW':
            syms.append((f[2], demangled))
    for label, prefix, exprs, flags in SPEC:
        hits = [m for m, d in syms if d.startswith(prefix) and '.cold' not in d and 'clone' not in d]
        if not hits:
            results[label] = {'hits': 0, 'arms': {}, 'note': 'no symbol'}
            continue
        for m in hits:
            EntryBp('*' + m, label, exprs, flags)
        results.setdefault(label, {'hits': 0, 'arms': {}})


def on_exit(event):
    with open(os.environ['SP_COV_OUT'], 'w') as fh:
        json.dump(results, fh, indent=1, sort_keys=True)


gdb.events.exited.connect(on_exit)
install()
