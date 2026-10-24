# dtr11: the DarkTrooper arms the fights cannot reach, forced by gdb calls (the ds43 way): gdb boots
# the game; breakpoints with a `commands` list source darktrooper_dtr11_*.gdb, which write the
# members each arm needs, call the body and write every member back. Each script runs once
# (the $dN flags); a nested call never re-enters (the breakpoint conditions test $busy).
#   COV_GDB_SCRIPT=$PWD/tools/cov/specs/darktrooper_dtr11_calls.py BUILD=$PWD/build-port-cov \
#     tools/cov/force.sh tools/cov/specs/darktrooper_dtr11_calls.py <out> 3bfa3eff,04_temple_unknownenemies \
#     --con tools/drivers/darktrooper_unknown.con --fast --frames 5200
# CPatterned::GetScannableObjectInfo answers 1 / 0 while $scan is 1 / 2 (Dead's scannable arm).
import os
import gdb

CLASS = 'CDarkTrooper'
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


for v in ('busy', 'dS', 'scan', 'pnull', 'skiplp', 'dE'):
    gdb.execute('set $%s = 0' % v)


class RetSet(gdb.FinishBreakpoint):
    def stop(self):
        s = int(gdb.convenience_variable('scan'))
        gdb.execute('set var $rax = %d' % (1 if s == 1 else 0))
        return False

    def out_of_scope(self):
        pass


class ScanBp(gdb.Breakpoint):
    def stop(self):
        with open(OUT + '/scan.log', 'a') as fh:
            fh.write('scan hit %s\n' % gdb.convenience_variable('scan'))
        if int(gdb.convenience_variable('scan')) != 0:
            RetSet(gdb.newest_frame(), internal=True)
        return False


ScanBp(addr('CPatterned::GetScannableObjectInfo('), internal=True)


class PathRet(gdb.FinishBreakpoint):
    def stop(self):
        gdb.execute('set var $rax = 0')
        return False

    def out_of_scope(self):
        pass


class PathBp(gdb.Breakpoint):
    def stop(self):
        if int(gdb.convenience_variable('pnull')) == 1:
            PathRet(gdb.newest_frame(), internal=True)
        return False


PathBp(addr('CDarkTrooper::GetPathArea('), internal=True)


def script(spec, name, cond, ignore=0):
    bp = gdb.Breakpoint(addr(spec))
    bp.condition = cond
    bp.ignore_count = ignore
    bp.commands = ('set logging file %s/dtr11_%s.log\nset logging overwrite on\n'
                   'set logging redirect off\nset logging enabled on\n'
                   'set $busy = 1\npython\ntry:\n  gdb.execute("source %s/darktrooper_dtr11_%s.gdb")\n'
                   'except gdb.error as e:\n  print("ERROR", e)\nend\nset $busy = 0\n'
                   'set logging enabled off\ncontinue' % (OUT, name, HERE, name))


class LaunchBp(gdb.Breakpoint):
    # $skiplp == 1: CPatterned::LaunchProjectile returns at once (its projectile info is the forced one-valid
    # optional with no asset), as if by `ret`.
    def stop(self):
        if int(gdb.convenience_variable('skiplp')) == 1:
            gdb.execute('set $rip = *(unsigned long*)$rsp')
            gdb.execute('set $rsp = $rsp + 8')
            gdb.execute('set $rax = 0')
        return False


LaunchBp(addr('CPatterned::LaunchProjectile('), internal=True)

script('CDarkTrooper::Think(', 'fss', '$busy == 0 && $dS == 0', int(os.environ.get('DTR11_AT', '300')))
script('CDarkTrooper::ThinkRagDoll(', 'rend', '$busy == 0 && $dE == 0')
