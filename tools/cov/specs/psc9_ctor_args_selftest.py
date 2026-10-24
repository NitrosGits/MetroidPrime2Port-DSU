#!/usr/bin/env python3
"""Mocked-gdb regression test for tools/cov/specs/psc9_ctor_args.py - no game boot, no gdb process.

It loads the real fixture source with a fake `gdb` and drives its actual breakpoints:
  natural        no write, object state matches -> status=OK, state oracle 1/1
  invalidAnim    writes 0xffffffff, restores the ORIGINAL bytes, state matches -> OK
  state mismatch scripted wrong object state       -> FAIL, named reason
  unreadable     the animRes address is unreadable -> FAIL, named reason
  exception      the patch raises inside stop()    -> FAIL, fixture exception
  no hit         installed, never entered          -> FAIL, no ctor hit
  restore read   the write-back is ignored         -> FAIL, restore failure
  wrong binary   digest differs                    -> FAIL, not installed, no breakpoints

    python3 tools/cov/specs/psc9_ctor_args_selftest.py
"""
import hashlib
import os
import pathlib
import re
import sys
import tempfile
import types

SPEC = pathlib.Path(__file__).with_name('psc9_ctor_args.py')
THIS = 0x555557b00000
ANIMRES = 0x7fffffffc100
RSP = 0x7fffffffc800
failures = []


class FakeGdb(types.ModuleType):
    def __init__(self, binary, state, mem=None, write_works=True, raise_addr=False,
                 ignore_restore=False):
        super().__init__('gdb')
        self.binary = binary
        self.state = state          # field name -> value for the object-state oracle
        self.mem = dict(mem or {})  # absolute address -> 32-bit word
        self.write_works = write_works
        self.raise_addr = raise_addr
        self.ignore_restore = ignore_restore
        self.exited = []
        outer = self

        class error(Exception):
            pass

        class Breakpoint:
            instances = []

            def __init__(self, spec, internal=False):
                self.spec, self.internal = spec, internal
                Breakpoint.instances.append(self)

        class FinishBreakpoint:
            instances = []

            def __init__(self, frame, internal=False):
                self.frame, self.internal = frame, internal
                FinishBreakpoint.instances.append(self)

        class _Exited:
            @staticmethod
            def connect(fn):
                outer.exited.append(fn)

        class _Events:
            exited = _Exited()
        self.error = error
        self.Breakpoint = Breakpoint
        self.FinishBreakpoint = FinishBreakpoint
        self.events = _Events()

    def current_progspace(self):
        return types.SimpleNamespace(filename=self.binary)

    def newest_frame(self):
        return object()

    def parse_and_eval(self, expr):
        e = expr.strip()
        if e == '$rdi':
            return THIS
        if e == '$r13':
            return ANIMRES
        if e == '$rsp':
            return RSP
        if e == '(long)&animRes':
            if self.raise_addr:
                raise self.error('no member animRes')
            return ANIMRES
        m = re.match(r'\*\(unsigned int\*\)0x([0-9a-f]+)$', e)
        if m:
            a = int(m.group(1), 16)
            if a not in self.mem:
                raise self.error('Cannot access memory at address 0x%x' % a)
            return self.mem[a]
        for tail, key in (('mAttackModels.mCount', 'mAttackModels'),
                          ('mSharedAttackModel.mPtr != 0', 'mSharedAttackModel'),
                          ('mSharedAttackDeltas.mPtr != 0', 'mSharedAttackDeltas'),
                          ('mAttackModelBitBase', 'mAttackModelBitBase'),
                          ('mSharedAttackBit', 'mSharedAttackBit')):
            if e.endswith(tail) and e.startswith('((CPlantScarabSwarm*)'):
                return self.state[key]
        raise self.error('unsupported expression: ' + e[:60])

    def execute(self, cmd):
        m = re.match(r'set \{unsigned int\}0x([0-9a-f]+) = 0x([0-9a-f]+)$', cmd.strip())
        if m and self.write_works:
            val = int(m.group(2), 16)
            if self.ignore_restore and val != 0xffffffff:
                return None  # a write-back that silently does not take
            self.mem[int(m.group(1), 16)] = val
        return None


def digest(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


NM = '0000000000f4f3b0 t _ZN17CPlantScarabSwarmC1E9TUniqueIdFAKE\tCPlantScarabSwarm::CPlantScarabSwarm(TUniqueId)\n'


def load(gdbmod, outdir, expect_sha=None):
    os.makedirs(outdir, exist_ok=True)
    os.environ['COV_OUT'] = outdir
    os.environ['PSC9_PHASE_START'] = '0'
    nm = pathlib.Path(outdir, 'nm.txt')
    nm.write_text(NM)
    os.environ['COV_NM'] = str(nm)
    sys.modules['gdb'] = gdbmod
    src = SPEC.read_text()
    if expect_sha is not None:
        src = re.sub(r"EVIDENCE_SHA256 = '[0-9a-f]+'", "EVIDENCE_SHA256 = '%s'" % expect_sha, src, count=1)
    ns = {'__name__': 'psc9_spec_under_test', '__file__': str(SPEC)}
    exec(compile(src, str(SPEC), 'exec'), ns)
    return ns


def line(outdir):
    f = pathlib.Path(outdir, 'fixture.txt')
    if not f.exists():
        return ''
    return [l for l in f.read_text().splitlines() if l.strip()][-1]


def onephase(ns, gdbmod):
    """Drive entry breakpoint, the one matching patch breakpoint, then the ctor return."""
    bps = gdbmod.Breakpoint.instances
    entry = next(b for b in bps if '+0x' not in b.spec)
    patch = next((b for b in bps if b.spec.endswith('+0x32e')), None)
    entry.stop()
    if patch is not None:
        patch.stop()
    fin = gdbmod.FinishBreakpoint.instances[-1]
    fin.stop()


def scenario(name, cond, detail=''):
    print(('PASS ' if cond else 'FAIL ') + '%-15s %s' % (name, detail))
    if not cond:
        failures.append(name)


NATURAL = {'mAttackModels': 3, 'mSharedAttackModel': 1, 'mSharedAttackDeltas': 1,
           'mAttackModelBitBase': 3, 'mSharedAttackBit': 1}
SKIPPED = {'mAttackModels': 0, 'mSharedAttackModel': 0, 'mSharedAttackDeltas': 0,
           'mAttackModelBitBase': 0, 'mSharedAttackBit': 0}


def main():
    tmp = pathlib.Path(tempfile.mkdtemp(prefix='psc9-selftest-'))
    binary = tmp / 'metroid_prime2_port'
    binary.write_bytes(b'stand-in binary for the psc9 mocked-gdb test\n')
    sha = digest(binary)

    # 1 natural (phase 0)
    out = tmp / 'natural'
    g = FakeGdb(str(binary), dict(NATURAL), mem={ANIMRES: 0x8bb689b4, RSP + 0x1a8: 2, RSP + 0x1b0: 1})
    ns = load(g, str(out), expect_sha=sha)
    onephase(ns, g)
    ns['summarize']()
    l = line(out)
    scenario('natural', 'status=OK' in l and 'writes=0' in l and 'state_ok=1' in l, l)

    # 2 invalidAnim (phase 1): writes animRes id, restores the original
    out = tmp / 'invalid'
    g = FakeGdb(str(binary), dict(SKIPPED), mem={ANIMRES: 0x8bb689b4, RSP + 0x1a8: 2, RSP + 0x1b0: 1})
    ns = load(g, str(out), expect_sha=sha)
    ns['state']['phase_idx'] = 1
    onephase(ns, g)
    ns['summarize']()
    l = line(out)
    scenario('invalidAnim', 'status=OK' in l and 'writes=1' in l and 'restored=1' in l
             and g.mem[ANIMRES] == 0x8bb689b4, l + ' | animRes=0x%x' % g.mem[ANIMRES])

    # 3 object state mismatch
    out = tmp / 'badstate'
    g = FakeGdb(str(binary), dict(NATURAL), mem={ANIMRES: 0x8bb689b4, RSP + 0x1a8: 2, RSP + 0x1b0: 1})
    ns = load(g, str(out), expect_sha=sha)
    ns['state']['phase_idx'] = 1          # invalidAnim expects the skipped state, script NATURAL
    onephase(ns, g)
    ns['summarize']()
    l = line(out)
    scenario('state mismatch', 'status=FAIL' in l and 'invalidAnim: object state got' in l, l)

    # 4 unreadable animRes address (falls back to r13, which the fake returns) -> make both fail
    out = tmp / 'unreadable'
    g = FakeGdb(str(binary), dict(SKIPPED), mem={ANIMRES: 0x8bb689b4, RSP + 0x1a8: 2, RSP + 0x1b0: 1},
                raise_addr=True)
    ns = load(g, str(out), expect_sha=sha)
    ns['state']['phase_idx'] = 1
    g.parse_and_eval_orig = g.parse_and_eval
    g.parse_and_eval = lambda expr: (_ for _ in ()).throw(g.error('boom')) if expr.strip() in ('$r13',) \
        else g.parse_and_eval_orig(expr)
    onephase(ns, g)
    ns['summarize']()
    l = line(out)
    scenario('unreadable', 'status=FAIL' in l and 'animid address unresolved' in l, l)

    # 5 exception inside the patch
    out = tmp / 'exception'
    g = FakeGdb(str(binary), dict(SKIPPED), mem={ANIMRES: 0x8bb689b4, RSP + 0x1a8: 2, RSP + 0x1b0: 1})
    ns = load(g, str(out), expect_sha=sha)
    ns['state']['phase_idx'] = 1
    ns['rd32'] = lambda a: (_ for _ in ()).throw(RuntimeError('simulated'))
    entry = next(b for b in g.Breakpoint.instances if '+0x' not in b.spec)
    patch = next(b for b in g.Breakpoint.instances if b.spec.endswith('+0x32e'))
    entry.stop()
    patch.stop()
    ns['summarize']()
    l = line(out)
    scenario('exception', 'status=FAIL' in l and 'patch exception' in l, l)

    # 6 no hit
    out = tmp / 'nohit'
    g = FakeGdb(str(binary), dict(NATURAL), mem={})
    ns = load(g, str(out), expect_sha=sha)
    ns['summarize']()
    l = line(out)
    scenario('no hit', 'status=FAIL' in l and 'no ctor hit' in l and 'no ctor return' in l, l)

    # 7 restore readback ignored by the write
    out = tmp / 'restore'
    g = FakeGdb(str(binary), dict(SKIPPED), mem={ANIMRES: 0x8bb689b4, RSP + 0x1a8: 2, RSP + 0x1b0: 1},
                ignore_restore=True)
    ns = load(g, str(out), expect_sha=sha)
    ns['state']['phase_idx'] = 1
    onephase(ns, g)
    ns['summarize']()
    l = line(out)
    scenario('restore failure', 'status=FAIL' in l and 'restore readback mismatch' in l, l)

    # 8 wrong binary
    out = tmp / 'wrongbin'
    g = FakeGdb(str(binary), dict(NATURAL), mem={})
    ns = load(g, str(out))       # keep the archived digest
    ns['summarize']()
    l = line(out)
    scenario('wrong binary', 'status=FAIL' in l and 'BINARY SHA MISMATCH' in l and 'installed=0' in l, l)
    scenario('no bp on mismatch', g.Breakpoint.instances == [],
             'breakpoints: %d' % len(g.Breakpoint.instances))

    print('\n%d scenario failure(s)' % len(failures))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
