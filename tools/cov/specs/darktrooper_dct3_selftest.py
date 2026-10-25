"""Mocked selftest for tools/cov/specs/darktrooper_dct3.py (no game, no gdb binary).

A fake `gdb` module is installed, so the spec's own _install() runs against a synthetic show: two
or three DarkTrooper constructions, one of them forced. The test drives the very breakpoint classes
the real run uses (Entry / FirstCompare / BestEntry / BestReturn / Arm / CtorFinish), with a fake
inferior memory, and then checks the artefacts the run would leave behind:

  - symbol selection, parse_force, arm_edges, expected_sleeper, force_step, checks_for (pure part)
  - one word written for the chosen uid only, read back, and restored at the ctor's return
  - dct3_table.txt / dct3_checks.txt contents: every edge counted, no failed check

usage: python3 tools/cov/specs/darktrooper_dct3_selftest.py
"""
import atexit
import importlib.util
import os
import pathlib
import sys
import tempfile
import types

HERE = pathlib.Path(__file__).resolve().parent


# ----------------------------------------------------------------- fake gdb
class GdbError(Exception):
    pass


class Value(int):
    def __new__(cls, v):
        return int.__new__(cls, v)


MEM = {}


class _Bytes:
    def __init__(self, raw):
        self.raw = raw

    def tobytes(self):
        return self.raw


class Inferior:
    def read_memory(self, addr, size):
        return _Bytes(bytes(MEM.get(addr + i, 0) for i in range(size)))

    def write_memory(self, addr, data):
        for i, b in enumerate(data):
            MEM[addr + i] = b


class Frame:
    def __init__(self, regs, name=None, older=None):
        self.regs, self._name, self._older = regs, name, older

    def read_register(self, n):
        return self.regs[n]

    def name(self):
        return self._name

    def older(self):
        return self._older


class Breakpoint:
    made = []

    def __init__(self, spec, internal=False, **kw):
        self.spec, self.internal = spec, internal
        Breakpoint.made.append(self)

    def stop(self):
        return False


class FinishBreakpoint:
    made = []

    def __init__(self, frame, internal=False, **kw):
        self.frame = frame
        FinishBreakpoint.made.append(self)

    def stop(self):
        return False


CURRENT = {'frame': None}
ENV = {}


def _parse(expr):
    expr = expr.strip()
    if expr.startswith('$') and CURRENT['frame'] is not None:
        regs = CURRENT['frame'].regs
        if expr[1:] in regs:
            return Value(regs[expr[1:]])
    if expr in ENV:
        return Value(ENV[expr])
    raise GdbError('cannot evaluate %r' % expr)


FAKE = types.SimpleNamespace(
    error=GdbError, Breakpoint=Breakpoint, FinishBreakpoint=FinishBreakpoint,
    Value=Value, parse_and_eval=_parse, newest_frame=lambda: CURRENT['frame'],
    selected_inferior=lambda: Inferior(),
    TYPE_CODE_PTR=1, TYPE_CODE_ENUM=2, TYPE_CODE_BOOL=3, TYPE_CODE_INT=4,
)


def load_spec(outdir, force):
    os.environ['COV_OUT'] = str(outdir)
    os.environ['COV_NM'] = str(outdir / 'nm.txt')
    os.environ.pop('DCT3_FORCE', None)
    if force:
        os.environ['DCT3_FORCE'] = force
    (outdir / 'nm.txt').write_text(
        '0000000000d4bbf0 t _ZN12CDarkTrooperC1E9TUniqueIdEE\tCDarkTrooper::CDarkTrooper(TUniqueId)\n'
        '0000000000d4bbf0 t _ZN12CDarkTrooperC2E9TUniqueIdEE\tCDarkTrooper::CDarkTrooper(TUniqueId)\n'
        '0000000000d5009a t _ZN12CDarkTrooperC1E9TUniqueIdEE.cold\t'
        'CDarkTrooper::CDarkTrooper(TUniqueId) [clone .cold]\n'
        '0000000000f11d40 T _ZNK12CPASDatabase17FindBestAnimationERK16CPASAnimParmDatai\t'
        'CPASDatabase::FindBestAnimation(CPASAnimParmData const&, int) const\n'
        '0000000000f11e70 T _ZNK12CPASDatabase17FindBestAnimationERK16CPASAnimParmDataR9CRandom16i\t'
        'CPASDatabase::FindBestAnimation(CPASAnimParmData const&, CRandom16&, int) const\n')
    atexit._clear()  # drop handlers from a previous scenario
    sys.modules['gdb'] = FAKE
    spec_path = HERE / 'darktrooper_dct3.py'
    mod = types.ModuleType('dct3_spec')
    mod.__file__ = str(spec_path)
    code = compile(spec_path.read_text(), str(spec_path), 'exec')
    exec(code, mod.__dict__)
    return mod


def bp(cls, spec_prefix):
    for b in Breakpoint.made:
        if isinstance(b, cls) and getattr(b, 'spec', '').startswith(spec_prefix):
            return b
    raise AssertionError('no %s breakpoint at %s' % (cls.__name__, spec_prefix))


def finish(prefix):
    for f in reversed(FinishBreakpoint.made):
        if prefix in type(f).__name__:
            return f
    raise AssertionError('no finish breakpoint %s' % prefix)


def put(addr, size, value):
    for i in range(size):
        MEM[addr + i] = (value >> (8 * i)) & 0xFF


def main():
    failures = []

    def ok(name, cond, detail=''):
        print('%s %s %s' % ('PASS' if cond else 'FAIL', name, detail))
        if not cond:
            failures.append(name)

    tmp = pathlib.Path(tempfile.mkdtemp(prefix='dct3-selftest-'))
    mod = load_spec(tmp, force='5:E1')

    # ---- pure part
    nm = open(tmp / 'nm.txt').readlines()
    ok('ctor symbols: C1/C2 aliases collapse to one', len(mod.ctor_symbols(nm)) == 1)
    nm2 = nm + ['0000000000d60000 t _ZN12CDarkTrooperC1E9TUniqueIdEE2\t'
                'CDarkTrooper::CDarkTrooper(TUniqueId)\n']
    ok('ctor symbols: a distinct address is kept', len(mod.ctor_symbols(nm2)) == 2)
    ok('ctor symbols: .cold dropped', all('.cold' not in s for s in mod.ctor_symbols(nm)))
    ok('best symbol: two-arg overload only',
       mod.best_symbols(open(tmp / 'nm.txt').readlines()) ==
       ['_ZNK12CPASDatabase17FindBestAnimationERK16CPASAnimParmDatai'])
    ok('parse_force E1', mod.parse_force('5:E1') == (5, 'E1'))
    ok('parse_force junk', mod.parse_force('nope') is None and mod.parse_force('') is None)
    ok('force_step uid filter',
       mod.force_step((5, 'E1'), {'uid': 6}, 1, 13) is None)
    ok('force_step call filter',
       mod.force_step((5, 'E1'), {'uid': 5}, 2, 13) is None and
       mod.force_step((5, 'E2'), {'uid': 5}, 1, 13) is None)
    ok('force_step hit', mod.force_step((5, 'E1'), {'uid': 5}, 1, 13) == 13)
    rec = {'uid': 5, 'anim': 13, 'best': [13], 'arms': {'E1true', 'E3true', 'E4true'}, 'scan': 0x51d4ba83,
           'flotsam': 1, 'final': {'sleeper': True, 'loco': 7, 'scaninfo': 0x1000,
                                   'visor_byte': 0, 'highlight': 0}}
    e = mod.arm_edges(rec)
    ok('arm_edges E1true/E1false',
       e['E1true'] and not e['E1false'] and not e['E2false'], str(e))
    ok('expected_sleeper first arm', mod.expected_sleeper(rec) == (True, 7))
    ok('expected_sleeper second arm',
       mod.expected_sleeper({'anim': 11, 'best': [13, 11]}) == (True, 9))
    ok('expected_sleeper none', mod.expected_sleeper({'anim': 4, 'best': [13, 15]}) == (False, None))
    ok('expected_sleeper uses the forced word',
       mod.expected_sleeper({'anim': 11, 'best': [13],
                             'forced': [{'edge': 'E1', 'write': 13}]}) == (True, 7))
    ok('checks_for a good record', all(ok_ for _, ok_, _ in mod.checks_for(rec)))
    bad = dict(rec, final={'sleeper': False, 'loco': 0, 'scaninfo': 0, 'visor_byte': 0})
    ok('checks_for catches a wrong arm',
       not all(ok_ for _, ok_, _ in mod.checks_for(bad)))

    # ---- mocked show: uid 5 forced (E1), uid 9 natural both-false, uid 11 natural E2
    entry = bp(mod.API['Entry'], '*_ZN12CDarkTrooperC')
    first = bp(mod.API['FirstCompare'], 'CDarkTrooper.cpp:149')
    for b in Breakpoint.made:
        if type(b).__name__ == 'BestEntry':
            beste = b
    arms = [b for b in Breakpoint.made if type(b).__name__ == 'Arm']
    arm_by_edge = {b.edge: b for b in arms}
    ok('all four arm breakpoints', sorted(arm_by_edge) == ['E1true', 'E2true', 'E3true', 'E4true'])

    def ctor_frame(this, rsp):
        return Frame({'rbx': this, 'rsp': rsp, 'rdi': this},
                     name='CDarkTrooper::CDarkTrooper(TUniqueId)')

    def construct(uid, this, rsp, anim, scan, flotsam, bests, taken, final):
        ENV['(unsigned int)uid.value'] = uid
        CURRENT['frame'] = ctor_frame(this, rsp)
        entry.stop()
        put(rsp + mod.OFF_ANIM, 4, anim)
        put(rsp + mod.OFF_SCAN, 4, scan)
        put(rsp + mod.OFF_FLOTSAM, 1, flotsam)
        first.stop()
        ctor = CURRENT['frame']
        for index in bests:
            callee = Frame({'rax': ((index & 0xFFFFFFFF) << 32) | 1},
                           name='CPASDatabase::FindBestAnimation(CPASAnimParmData const&, int) const')
            callee._older = ctor
            CURRENT['frame'] = callee
            beste.stop()
            FinishBreakpoint.made[-1].stop()
        CURRENT['frame'] = ctor
        for edge in taken:
            arm_by_edge[edge].stop()
        base = this
        put(base + mod.OFF_MIS_SLEEPER, 1, 1 if final['sleeper'] else 0)
        put(base + mod.OFF_LOCO, 4, final['loco'])
        put(base + mod.OFF_SCANINFO, 8, final['scaninfo'])
        put(base + mod.OFF_VISOR_BYTE, 1, final['visor_byte'])
        CURRENT['frame'] = ctor
        ctor_finish = [f for f in FinishBreakpoint.made
                       if type(f).__name__ == 'CtorFinish' and f.rec['uid'] == uid][-1]
        ctor_finish.stop()
        return ctor_finish

    cf5 = construct(5, 0x1000, 0x8000, anim=11, scan=0x51D4BA83, flotsam=1, bests=[13],
                    taken=['E1true', 'E3true', 'E4true'],
                    final={'sleeper': True, 'loco': 7, 'scaninfo': 0x2000,
                                             'visor_byte': 0x00})
    forced = cf5.rec['forced']
    ok('one force record', len(forced) == 1, str(forced))
    ok('force wrote the index', forced and forced[0]['write'] == 13)
    ok('force readback', forced and forced[0]['readback'] == 13)
    ok('force restored', forced and forced[0]['restored'])
    ok('slot restored to the original word',
       sum(MEM.get(0x81A0 + i, 0) << (8 * i) for i in range(4)) == 11)

    construct(9, 0x2000, 0x9000, anim=4, scan=0xFFFFFFFF, flotsam=0, bests=[13, 15], taken=[],
              final={'sleeper': False, 'loco': 0, 'scaninfo': 0, 'visor_byte': 0x02})
    construct(11, 0x3000, 0xA000, anim=15, scan=0x51D4BA83, flotsam=0, bests=[13, 15],
              taken=['E2true', 'E3true'],
              final={'sleeper': True, 'loco': 9, 'scaninfo': 0x3000, 'visor_byte': 0x02})

    atexit._run_exitfuncs()
    table = (tmp / 'dct3_table.txt').read_text()
    checks = (tmp / 'dct3_checks.txt').read_text()
    ok('table has three objects', '3 constructed DarkTroopers' in table, table.splitlines()[0])
    ok('every edge counted', 'edges with no object: -' in checks,
       [l for l in checks.splitlines() if 'edges with' in l][0])
    ok('no failed check', 'FAILED 0 of' in checks,
       [l for l in checks.splitlines() if l.startswith('FAILED')][0])
    ok('force uid only', 'force uid=5 E1' in checks and 'force uid=9' not in checks)
    ok('E1false counted for uid 9', 'uid=9 ' in table and 'E1false=1' in table)

    atexit._clear()
    sys.modules.pop('gdb', None)
    print('\n%d failure(s); artefacts in %s' % (len(failures), tmp))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
