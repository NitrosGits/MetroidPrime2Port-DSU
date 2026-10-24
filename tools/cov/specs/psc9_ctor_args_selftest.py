#!/usr/bin/env python3
"""Mocked-gdb regression test for tools/cov/specs/psc9_ctor_args.py - no game boot, no gdb process.

It loads the real fixture source with a fake `gdb` and drives its real breakpoints, so the restoration
contract is exercised: obligations are registered before the write, retained when the write or its
readback misbehaves, restored per call at the return, and every gap fails the summary.

  natural                 no write, state matches                -> OK
  invalidAnim             write + exact restore + state          -> OK
  readback fails          post-write read unreadable, return OK  -> FAIL for the readback, restored=1
  mutate then raise       write applied, then gdb.error          -> FAIL, obligation restored anyway
  second call unfinished  first returns, second never does       -> FAIL, call 2 unreturned/restored=1
  no FinishBreakpoint     arming fails, no mutation permitted    -> FAIL, attempted=0
  unreadable restore      the return cannot read the slot back   -> FAIL, obligation unrestored
  pending obligations     a patched call that never returns      -> FAIL, unreturned + not restored
  state mismatch          wrong object state                     -> FAIL
  wrong binary            digest differs                         -> FAIL, not installed, no breakpoints

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
SLOT_INTO = RSP + 0x1a8
SLOT_ATTACK = RSP + 0x1b0
failures = []
NM = ('0000000000f4f3b0 t _ZN17CPlantScarabSwarmC1E9TUniqueIdFAKE'
      '\tCPlantScarabSwarm::CPlantScarabSwarm(TUniqueId)\n')


class FakeGdb(types.ModuleType):
    def __init__(self, binary, obj, mem=None, finish_fails=False, execute_raise_after=False,
                 read_fail_after_write=(), unreadable=(), drop_writes=()):
        super().__init__('gdb')
        self.binary = binary
        self.obj = obj
        self.mem = dict(mem or {})
        self.finish_fails = finish_fails
        self.execute_raise_after = execute_raise_after
        self.read_fail_after_write = set(read_fail_after_write)
        self.unreadable = set(unreadable)
        self.drop_writes = set(drop_writes)
        self.written = set()
        self.failed_once = set()
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
                if outer.finish_fails:
                    raise outer.error('no finish breakpoint available')
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
            return ANIMRES
        m = re.match(r'\*\(unsigned int\*\)0x([0-9a-f]+)$', e)
        if m:
            a = int(m.group(1), 16)
            if a in self.unreadable:
                raise self.error('Cannot access memory at address 0x%x' % a)
            if a in self.read_fail_after_write and a in self.written and a not in self.failed_once:
                self.failed_once.add(a)
                raise self.error('transient read failure at 0x%x' % a)
            if a not in self.mem:
                raise self.error('Cannot access memory at address 0x%x' % a)
            return self.mem[a]
        for tail, key in (('mAttackModels.mCount', 'mAttackModels'),
                          ('mSharedAttackModel.mPtr != 0', 'mSharedAttackModel'),
                          ('mSharedAttackDeltas.mPtr != 0', 'mSharedAttackDeltas'),
                          ('mAttackModelBitBase', 'mAttackModelBitBase'),
                          ('mSharedAttackBit', 'mSharedAttackBit')):
            if e.endswith(tail) and e.startswith('((CPlantScarabSwarm*)'):
                return self.obj[key]
        raise self.error('unsupported expression: ' + e[:60])

    def execute(self, cmd):
        m = re.match(r'set \{unsigned int\}0x([0-9a-f]+) = 0x([0-9a-f]+)$', cmd.strip())
        if not m:
            return None
        addr, val = int(m.group(1), 16), int(m.group(2), 16)
        if addr not in self.drop_writes:
            self.mem[addr] = val
            self.written.add(addr)
        if self.execute_raise_after:
            self.execute_raise_after = False
            raise self.error('write fault after the store at 0x%x' % addr)
        return None


NATURAL = {'mAttackModels': 3, 'mSharedAttackModel': 1, 'mSharedAttackDeltas': 1,
           'mAttackModelBitBase': 3, 'mSharedAttackBit': 1}
SKIPPED = {'mAttackModels': 0, 'mSharedAttackModel': 0, 'mSharedAttackDeltas': 0,
           'mAttackModelBitBase': 0, 'mSharedAttackBit': 0}


def digest(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


def load(gdbmod, outdir, expect_sha=None, phase_start=0):
    os.makedirs(outdir, exist_ok=True)
    os.environ['COV_OUT'] = outdir
    os.environ['PSC9_PHASE_START'] = str(phase_start)
    nm = pathlib.Path(outdir, 'nm.txt')
    nm.write_text(NM)
    os.environ['COV_NM'] = str(nm)
    sys.modules['gdb'] = gdbmod
    src = SPEC.read_text()
    if expect_sha is not None:
        src = re.sub(r"EVIDENCE_SHA256 = '[0-9a-f]+'", "EVIDENCE_SHA256 = '%s'" % expect_sha,
                     src, count=1)
    ns = {'__name__': 'psc9_spec_under_test', '__file__': str(SPEC)}
    exec(compile(src, str(SPEC), 'exec'), ns)
    return ns


def line(outdir):
    f = pathlib.Path(outdir, 'fixture.txt')
    return [l for l in f.read_text().splitlines() if l.strip()][-1] if f.exists() else ''


def bps(g, suffix):
    return [b for b in g.Breakpoint.instances if suffix is None and '+0x' not in b.spec] or \
           [b for b in g.Breakpoint.instances if suffix and b.spec.endswith(suffix)]


def entry_bp(g):
    return next(b for b in g.Breakpoint.instances if '+0x' not in b.spec)


def patch_bp(g, suffix):
    return next(b for b in g.Breakpoint.instances if b.spec.endswith(suffix))


def run_call(ns, g, patch_suffix='+0x32e', do_return=True):
    entry_bp(g).stop()
    patch_bp(g, patch_suffix).stop()
    if do_return and g.FinishBreakpoint.instances:
        g.FinishBreakpoint.instances[-1].stop()


def scenario(name, cond, detail=''):
    print(('PASS ' if cond else 'FAIL ') + '%-22s %s' % (name, detail))
    if not cond:
        failures.append(name)


def main():
    tmp = pathlib.Path(tempfile.mkdtemp(prefix='psc9-selftest-'))
    binary = tmp / 'metroid_prime2_port'
    binary.write_bytes(b'stand-in binary for the psc9 mocked-gdb test\n')
    sha = digest(binary)
    base_mem = {ANIMRES: 0x8bb689b4, SLOT_INTO: 2, SLOT_ATTACK: 1}

    # 1 natural
    out = tmp / 'natural'
    g = FakeGdb(str(binary), dict(NATURAL), base_mem)
    ns = load(g, str(out), expect_sha=sha, phase_start=0)
    run_call(ns, g)
    ns['summarize']()
    l = line(out)
    scenario('natural', 'status=OK' in l and 'attempted=0' in l and 'state_ok=1' in l, l)

    # 2 invalidAnim
    out = tmp / 'invalid'
    g = FakeGdb(str(binary), dict(SKIPPED), base_mem)
    ns = load(g, str(out), expect_sha=sha, phase_start=1)
    run_call(ns, g)
    ns['summarize']()
    l = line(out)
    scenario('invalidAnim', 'status=OK' in l and 'attempted=1' in l and 'restored=1' in l
             and 'unrestored=0' in l and g.mem[ANIMRES] == 0x8bb689b4, l)

    # 3 post-write readback fails, return still restores
    out = tmp / 'readback'
    g = FakeGdb(str(binary), dict(SKIPPED), base_mem, read_fail_after_write=(ANIMRES,))
    ns = load(g, str(out), expect_sha=sha, phase_start=1)
    run_call(ns, g)
    ns['summarize']()
    l = line(out)
    scenario('readback fails', 'status=FAIL' in l and 'post-write readback' in l
             and 'restored=1' in l and 'unrestored=0' in l, l)

    # 4 write mutates then raises
    out = tmp / 'mutate-raise'
    g = FakeGdb(str(binary), dict(SKIPPED), base_mem, execute_raise_after=True)
    ns = load(g, str(out), expect_sha=sha, phase_start=1)
    run_call(ns, g)
    ns['summarize']()
    l = line(out)
    scenario('mutate then raise', 'status=FAIL' in l and 'write to animRes.mAncsId' in l
             and 'restored=1' in l and 'unrestored=0' in l, l)

    # 5 second call unfinished after a successful first
    out = tmp / 'second-open'
    g = FakeGdb(str(binary), dict(SKIPPED), base_mem)
    ns = load(g, str(out), expect_sha=sha, phase_start=1)
    run_call(ns, g)
    run_call(ns, g, patch_suffix='+0x342', do_return=False)
    ns['summarize']()
    l = line(out)
    scenario('second call open', 'status=FAIL' in l and 'unreturned ctor call(s): [2]' in l
             and 'attempted write(s) not restored' in l and 'restored=1' in l, l)

    # 6 missing FinishBreakpoint: no mutation permitted
    out = tmp / 'no-finish'
    g = FakeGdb(str(binary), dict(SKIPPED), base_mem, finish_fails=True)
    ns = load(g, str(out), expect_sha=sha, phase_start=1)
    run_call(ns, g)
    ns['summarize']()
    l = line(out)
    scenario('no finish bp', 'status=FAIL' in l and 'no FinishBreakpoint' in l and 'attempted=0' in l
             and g.mem[ANIMRES] == 0x8bb689b4, l)

    # 7 unreadable restore readback
    out = tmp / 'unreadable-restore'
    g = FakeGdb(str(binary), dict(SKIPPED), base_mem)
    ns = load(g, str(out), expect_sha=sha, phase_start=1)
    entry_bp(g).stop()
    patch_bp(g, '+0x32e').stop()
    g.unreadable.add(ANIMRES)          # the return can no longer read the slot back
    g.FinishBreakpoint.instances[-1].stop()
    ns['summarize']()
    l = line(out)
    scenario('unreadable restore', 'status=FAIL' in l and 'unreadable' in l and 'restored=0' in l
             and 'unrestored=1' in l, l)

    # 8 pending obligations: patched call that never returns
    out = tmp / 'pending'
    g = FakeGdb(str(binary), dict(SKIPPED), base_mem)
    ns = load(g, str(out), expect_sha=sha, phase_start=1)
    run_call(ns, g, do_return=False)
    ns['summarize']()
    l = line(out)
    scenario('pending obligations', 'status=FAIL' in l and 'unreturned' in l
             and 'not restored' in l, l)

    # 9 object state mismatch
    out = tmp / 'badstate'
    g = FakeGdb(str(binary), dict(NATURAL), base_mem)
    ns = load(g, str(out), expect_sha=sha, phase_start=1)
    run_call(ns, g)
    ns['summarize']()
    l = line(out)
    scenario('state mismatch', 'status=FAIL' in l and 'object state got' in l, l)

    # 10 wrong binary
    out = tmp / 'wrongbin'
    g = FakeGdb(str(binary), dict(NATURAL), base_mem)
    ns = load(g, str(out))            # keeps the archived digest
    ns['summarize']()
    l = line(out)
    scenario('wrong binary', 'status=FAIL' in l and 'BINARY SHA MISMATCH' in l and 'installed=0' in l, l)
    scenario('no bp on mismatch', g.Breakpoint.instances == [],
             'breakpoints: %d' % len(g.Breakpoint.instances))

    print('\n%d scenario failure(s)' % len(failures))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
