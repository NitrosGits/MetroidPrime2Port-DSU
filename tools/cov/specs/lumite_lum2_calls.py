# lum2: Lumite arms the natural play (tools/drivers/lumite_natural.py, lumite_states.py) does not reach, by gdb
# calls (the dtr10/dtr11 way). gdb boots the game; a breakpoint at the entry of CLumite::Think sources
# lumite_lum2_<name>.gdb (writes the members an arm needs, calls the body, writes them back); each file runs
# once ($lm_dX flags) and a failing file does not stop the next one. Convenience variables are $lm_* / $t / $m
# (never register names: $al, $si, $dl ... are x86 registers).
#   LUM2_FILES=<out>/x COV_GDB_SCRIPT=$PWD/tools/cov/specs/lumite_lum2_calls.py BUILD=$PWD/build-port-cov \
#     tools/cov/force.sh tools/cov/specs/lumite_lum2_calls.py <out> 42b935e4,0j_sand_hall_dark \
#     --driver "python3 tools/drivers/lumite_natural.py {port} <out>.txt" --fast --frames 6000
# (the driver writes <out>/x.team and x.zones, the uids the team and zone files need).
import os
import gdb

CLASS = 'CLumite'
FORCE = []
HERE = os.path.dirname(os.path.abspath(os.environ['COV_SPEC']))
OUT = os.environ['COV_OUT']

syms = []
for ln in open(os.environ['COV_NM']):
    parts = ln.rstrip('\n').split('\t')
    if len(parts) == 2:
        f = parts[0].split()
        if len(f) == 3 and f[1] in 'tTW':
            syms.append((f[2], parts[1]))


def addr(prefix):
    hits = [m for m, d in syms if d.startswith(prefix) and '.cold' not in d and 'clone' not in d]
    if len(hits) != 1:
        raise gdb.GdbError('%s: %d symbols' % (prefix, len(hits)))
    return '*' + hits[0]


gdb.execute('set $lm_busy = 0')
gdb.execute('set $lm_done = 0')
gdb.execute('set $lm_team = -1')

STEPS = os.environ.get('LUM2_STEPS', 'cbn,basic,sun,zone,trig,knee,hop,more,team2,msg,collide,dead').split(',')

bp = gdb.Breakpoint(addr('CLumite::Think('))
bp.condition = ('$lm_busy == 0 && $lm_done == 0 && ((CLumite*)$rdi)->mActive && ((CLumite*)$rdi)->mAlive '
                '&& ((CLumite*)$rdi)->mElapsedTime > 5.0f')
bp.ignore_count = int(os.environ.get('LUM2_AT', '300'))
body = ''.join('python\ntry:\n  gdb.execute("source %s/lumite_lum2_%s.gdb")\nexcept gdb.error as e:\n  print("ERROR %s", e)\nend\n'
               % (HERE, n, n) for n in STEPS)
bp.commands = ('set logging file %s/lum2.log\nset logging overwrite on\nset logging redirect off\nset logging enabled on\n'
               'set $lm_busy = 1\nset $lm_done = 1\n%sset $lm_busy = 0\nset logging enabled off\ncontinue' % (OUT, body))


class TeamBp(gdb.Breakpoint):
    # a Lumite with no team yet gets the area's team manager id (the driver's <prefix>.team) and joins it:
    # no shipped Lumite is connected to the team manager (GetAssociatedTeamId finds none), so the team arms
    # are forced.
    def stop(self):
        try:
            if int(gdb.convenience_variable('lm_team')) < 0:
                txt = open(os.environ['LUM2_FILES'] + '.team').read().split()
                if not txt:
                    return False
                gdb.execute('set $lm_team = %d' % int(txt[0]))
            t = '((CLumite*)$rdi)'
            return (int(gdb.parse_and_eval(t + '->mTeamAiMgrId.value')) == 0xffff and
                    int(gdb.parse_and_eval(t + '->mActive')) != 0 and int(gdb.parse_and_eval(t + '->mAlive')) != 0 and
                    int(gdb.parse_and_eval('$lm_busy')) == 0)
        except (OSError, gdb.error):
            return False


if 'team' in os.environ.get('LUM2_TEAM', 'team'):
    tb = TeamBp(addr('CLumite::Think('))
    tb.commands = ('set $lm_busy = 1\nset $t = (CLumite*)$rdi\nset $m = (CStateManager*)$rsi\n'
                   'set var $t->mTeamAiMgrId.value = $lm_team\ncall $t->JoinTeam(*$m)\ncall $t->GetTeamRole(*$m)\n'
                   'printf "team joined %p\\n", $t\nset $lm_busy = 0\ncontinue')


# forced return values: `$lm_rf_<k> >= 0` makes the next return of the function carry that value (rax), for the
# team arms (a role Unknown = heckler, five hop actions, a manager the Lumite is not part of).
class RetSetBp(gdb.FinishBreakpoint):
    def __init__(self, frame, v):
        super().__init__(frame, internal=True)
        self.v = v

    def stop(self):
        gdb.execute('set var $rax = %d' % self.v)
        return False

    def out_of_scope(self):
        pass


class RetForce(gdb.Breakpoint):
    def __init__(self, sym, var):
        super().__init__(addr(sym), internal=True)
        self.var = var

    def stop(self):
        v = int(gdb.convenience_variable(self.var))
        if v >= 0:
            try:
                RetSetBp(gdb.newest_frame(), v)
            except (gdb.error, ValueError):
                pass
        return False


for _sym, _var in (('CScriptTeamAiMgr::GetTeamRole(', 'lm_rf_role'), ('CScriptTeamAiMgr::IsPartOfTeam(', 'lm_rf_part'),
                   ('CScriptTeamAiMgr::GetTeamActionCount(', 'lm_rf_acts'),
                   ('CScriptSafeZone::IsHurtful(', 'lm_rf_hurt')):
    gdb.execute('set $%s = -1' % _var)
    RetForce(_sym, _var)


class RetForceAll(RetForce):
    # every copy of an inline-expanded function (the REL modules each carry their own CBodyState::IsDead)
    def __init__(self, address, var):
        gdb.Breakpoint.__init__(self, '*' + address, internal=True)
        self.var = var


for _m, _d in syms:
    if _d.startswith('CBodyState::IsDead(') or _d.startswith('CBSDie::IsDead('):
        RetForceAll(_m, 'lm_rf_dead')
gdb.execute('set $lm_rf_dead = -1')
for _sym, _var in (('CCollisionSurface::IsDegenerate(', 'lm_rf_degen'), ('CAABox::DoBoundsOverlap(', 'lm_rf_ovl'),
                   ('CSafeZoneManager::SphereTouchingWhichSafeZone(', 'lm_rf_szm')):
    gdb.execute('set $%s = -1' % _var)
    RetForce(_sym, _var)


# LoadLumite arms shipped data may never reach (no model, the -1 phase sounds): at the line of the test the local is
# written once (the first Lumite load of the run; the loaded object is dropped for the model arm).
class LoadBp(gdb.Breakpoint):
    def __init__(self, loc, kind, off, val):
        super().__init__(loc, internal=True)
        self.kind, self.off, self.val, self.n = kind, off, val, 0
        self.expr = '%s -0x%x = %d' % (kind, off, val)

    def stop(self):
        self.n += 1
        with open(OUT + '/lum2_load.log', 'a') as fh:
            fh.write('LoadBp hit %d %s\n' % (self.n, self.expr))
        if self.n == 1:
            try:
                gdb.execute('set var %s($rbp - %d) = %d' % ('{char}' if self.kind == 'char_slot' else '{int}', self.off, self.val))
            except gdb.error as e:
                with open(OUT + '/lum2_load.log', 'a') as fh:
                    fh.write('LoadBp %s: %s\n' % (self.expr, e))
        return False


if os.environ.get('LUM2_LOAD', '1') == '1':
    # the tests are compiled to `cmpb $0,-N(%rbp)` after the LdrToModelData call and `mov -N(%rbp),%rXXd` for the two
    # sounds: the breakpoints sit on those instructions and write the stack slots (the source line of the test has no code of its own).
    import re
    _dis = gdb.execute('disassemble ' + addr('LoadLumite(')[1:], to_string=True).split('\n')
    _ins = [re.match(r'\s*(0x[0-9a-f]+) <\+(\d+)>:\s+(.*)', l) for l in _dis]
    _ins = [('(%s + %s)' % (addr('LoadLumite(')[1:], m.group(2)), m.group(3)) for m in _ins if m]
    for _k, (_a, _t) in enumerate(_ins):
        if 'call' in _t and 'LdrToModelData' in _t:
            _m = re.search(r'cmpb\s+\$0x0,-0x([0-9a-f]+)\(%rbp\)', _ins[_k + 1][1])
            LoadBp('*' + _ins[_k + 1][0], 'char_slot', int(_m.group(1), 16), 0)
            _sl = [(a, re.search(r'mov\s+-0x([0-9a-f]+)\(%rbp\),%r\d+d', t)) for a, t in _ins[_k + 2:_k + 12]]
            _sl = [(a, int(m.group(1), 16)) for a, m in _sl if m]
            LoadBp('*' + _sl[0][0], 'int_slot', _sl[0][1], -1)
            LoadBp('*' + _sl[1][0], 'int_slot', _sl[1][1], -1)
            break


# CVector3f::CanBeNormalized forced false for one call made from a given source line of the caller
# ($lm_cbn_line_k = line, $lm_cbn_cnt_k = how many calls from it to force; k = 1..3). The lines of the tests are found by text.
class CbnForce(gdb.Breakpoint):
    def stop(self):
        try:
            fr = gdb.newest_frame().older()
            line = fr.find_sal().line
            if os.environ.get('LUM2_DBG') and any(int(gdb.convenience_variable('lm_cbn_cnt_%d' % k)) > 0 for k in (1, 2, 3)):
                open(OUT + '/cbn.log', 'a').write('cbn call from %s line %d\n' % (fr.name(), line))
            for k in (1, 2, 3):
                if int(gdb.convenience_variable('lm_cbn_cnt_%d' % k)) > 0 and \
                        int(gdb.convenience_variable('lm_cbn_line_%d' % k)) == line:
                    gdb.execute('set $lm_cbn_cnt_%d = $lm_cbn_cnt_%d - 1' % (k, k))
                    RetSetBp(fr.newer(), 0)
                    break
        except (gdb.error, ValueError, AttributeError, TypeError):
            pass
        return False


for _k in (1, 2, 3):
    gdb.execute('set $lm_cbn_cnt_%d = 0' % _k)
    gdb.execute('set $lm_cbn_line_%d = 0' % _k)
CbnForce(addr('CVector3f::CanBeNormalized('), internal=True)
_srcl = open(os.path.join(HERE, '..', '..', '..', 'src/MetroidPrime/Enemies/CLumite.cpp')).read().split('\n')
for _name, _txt in (('while', 'while (!cross.CanBeNormalized()'), ('inner', 'if (mHop.mLaunchDirection.CanBeNormalized())'),
                    ('tozone', 'if (!toZone.CanBeNormalized())'), ('normal', 'if (!normal.CanBeNormalized())'),
                    ('hopdir', 'if (direction.CanBeNormalized())')):
    _ln = [i for i, l in enumerate(_srcl, 1) if _txt in l]
    gdb.execute('set $lm_L_%s = %d' % (_name, (max(_ln) if _name == 'hopdir' else _ln[0]) if _ln else 0))
