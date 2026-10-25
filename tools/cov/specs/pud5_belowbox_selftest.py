#!/usr/bin/env python3
"""Mocked-gdb regression test for tools/cov/specs/pud5_belowbox.py - no game boot, no gdb.

Drives the real fixture with a fake gdb.  The safety contract under test: an attempted position.z write is
recorded BEFORE the write and always restored at the return, whatever the write or its readback did.

  both paths            forced write confirmed + restored, one natural call returns        -> OK
  write then read fail  the store lands, the post-write read raises; rollback and return
                        restore the original; the failure stays reported                   -> FAIL, restored=1
  partial write + raise the store lands and gdb.error follows; same restore guarantees     -> FAIL, restored=1
  rollback failure      readback mismatch and the rollback write does not take; memory
                        keeps the forced value, the return restore also fails              -> FAIL, restored=0
  unreturned natural    an accepted natural call never returns                              -> FAIL
  unreadable forced ret the forced call returns but $rax could not be read                   -> FAIL
  unreadable natural    a natural call returns an unreadable bool                            -> FAIL
  wrong caller          same this, other caller: refused, never completion evidence         -> FAIL
  wrong this            allowed caller, other spore: refused                                -> FAIL
  restore mismatch      the write-back at the return does not take                          -> FAIL
  no natural            only the forced call                                               -> FAIL
  no install            breakpoint creation fails                                           -> FAIL

    python3 tools/cov/specs/pud5_belowbox_selftest.py
"""
import hashlib
import os
import pathlib
import re
import struct
import sys
import tempfile
import types

SPEC = pathlib.Path(__file__).with_name('pud5_belowbox.py')
SPORE = 0x555557a00000
OTHER = 0x555557b00000
POS = 0x555558100000          # rsi: the CVector3f; the edit is its z word at +8
MODE = POS + 8                # the edited word
MODEADDR = 0x555558200000     # rdx: the CWeaponMode (only read for the log)
ORIG = 0x41000000             # 8.0f
MIN_Z, MAX_Z = 1.0, 3.0       # centre z = 2.0
FORCED = struct.unpack('<I', struct.pack('<f', 0.5 * (MIN_Z + MAX_Z) - 1.0e6))[0]
failures = []


class FakeGdb(types.ModuleType):
    def __init__(self, binary, mstate=1, install_fails=False, drop_writes=(),
                 read_fail_after_write=(MODE,), execute_raise_after=False, read_raise=(),
                 drop_restore_writes=False):
        super().__init__('gdb')
        self.binary = binary
        self.mstate = mstate
        self.install_fails = install_fails
        self.drop_writes = set(drop_writes)
        self.read_fail_after_write = set(read_fail_after_write)
        self.execute_raise_after = execute_raise_after
        self.read_raise = set(read_raise)
        self.drop_restore_writes = drop_restore_writes
        self.mem = {MODE: ORIG, MODEADDR: 0}
        self.geom = {'min.z': MIN_Z, 'max.z': MAX_Z, 'm22': 1.0, 'sine': 0.5}
        self.written = set()
        self.failed_once = set()
        self.caller = 'CPuddleSpore::Touch'
        self.this = SPORE
        self.rax = 1
        self.exited = []
        outer = self

        class error(Exception):
            pass

        class Breakpoint:
            instances = []

            def __init__(self, spec, internal=False):
                if outer.install_fails:
                    raise outer.error('No symbol "%s"' % spec)
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
        name = self.caller

        class _Caller:
            def name(self):
                return name

            def older(self):
                return None

        class _Top:
            def name(self):
                return 'frame'

            def older(self):
                return _Caller()

        return _Top()

    def parse_and_eval(self, expr):
        e = expr.strip()
        if e == '$rdi':
            return self.this
        if e == '$rdx':
            return MODEADDR
        if e == '$rsi':
            return POS
        for key, name in (('min.z', 'mTouchBounds.min.z'), ('max.z', 'mTouchBounds.max.z'),
                          ('m22', 'mTransform.m22'), ('sine', 'mData.mHitDetectionSine')):
            if e.endswith('->' + name):
                v = self.geom[key]
                if v is None:
                    raise self.error('Cannot access memory (geometry)')
                return v
        if e == '$rax':
            return self.rax
        m = re.match(r'\*\(unsigned int\*\)0x([0-9a-f]+)$', e)
        if m:
            a = int(m.group(1), 16)
            if a in self.read_raise:
                raise self.error('Cannot access memory at address 0x%x' % a)
            if a in self.read_fail_after_write and a in self.written and a not in self.failed_once:
                self.failed_once.add(a)
                raise self.error('transient read failure at 0x%x' % a)
            if a not in self.mem:
                raise self.error('Cannot access memory at address 0x%x' % a)
            return self.mem[a]
        if e.endswith('->mState'):
            return self.mstate
        raise self.error('unsupported expression: ' + e[:50])

    def execute(self, cmd):
        m = re.match(r'set \{unsigned int\}0x([0-9a-f]+) = 0x([0-9a-f]+)$', cmd.strip())
        if not m:
            return None
        addr, val = int(m.group(1), 16), int(m.group(2), 16)
        restore_write = val != FORCED
        if addr not in self.drop_writes and not (self.drop_restore_writes and restore_write):
            self.mem[addr] = val
            self.written.add(addr)
        if self.execute_raise_after:
            self.execute_raise_after = False
            raise self.error('write fault after the store at 0x%x' % addr)
        return None


def digest(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


def load(gdbmod, outdir):
    os.makedirs(outdir, exist_ok=True)
    os.environ['COV_OUT'] = outdir
    sys.modules['gdb'] = gdbmod
    ns = {'__name__': 'pud5_under_test', '__file__': str(SPEC)}
    exec(compile(SPEC.read_text(), str(SPEC), 'exec'), ns)
    return ns


def line(outdir):
    f = pathlib.Path(outdir, 'fixture.txt')
    return [l for l in f.read_text().splitlines() if l.strip()][-1] if f.exists() else ''


def diag(outdir):
    return pathlib.Path(outdir, 'diag.log').read_text()


def bp(g):
    return g.Breakpoint.instances[0]


def call(g, this=SPORE, caller='CPuddleSpore::Touch', ret=1, do_return=True, mstate=None):
    if mstate is not None:
        g.mstate = mstate
    g.this, g.caller, g.rax = this, caller, ret
    bp(g).stop()
    if do_return and g.FinishBreakpoint.instances:
        g.FinishBreakpoint.instances[-1].stop()


def scenario(name, cond, detail=''):
    print(('PASS ' if cond else 'FAIL ') + '%-20s %s' % (name, detail))
    if not cond:
        failures.append(name)


def main():
    tmp = pathlib.Path(tempfile.mkdtemp(prefix='pud5fix-selftest-'))
    binary = tmp / 'metroid_prime2_port'
    binary.write_bytes(b'stand-in binary for the pud5 mocked-gdb test\n')
    sha = digest(binary)

    # 1 both paths
    out = tmp / 'both'
    g = FakeGdb(str(binary), read_fail_after_write=())
    ns = load(g, str(out))
    call(g, ret=1)
    call(g, ret=0)
    ns['summarize']()
    l = line(out)
    scenario('both paths', 'status=OK' in l and 'attempted=1' in l and 'restored=1' in l
             and 'accepted=2' in l and g.mem[MODE] == ORIG, l)

    # 1b the forced value is the float32 of centre.z - 1e6 and the caller's z is untouched afterwards
    out = tmp / 'value'
    g = FakeGdb(str(binary), read_fail_after_write=())
    ns = load(g, str(out))
    g.this, g.caller, g.rax = SPORE, 'CPuddleSpore::Touch', 1
    bp(g).stop()
    mid = g.mem[MODE]
    g.FinishBreakpoint.instances[-1].stop()
    scenario('forced value', mid == FORCED and struct.unpack('<f', struct.pack('<I', mid))[0] < -9.9e5
             and g.mem[MODE] == ORIG, 'during=0x%x after=0x%x' % (mid, g.mem[MODE]))

    # 1c preconditions: tilted spore / sine above up.z / unreadable geometry -> never written
    for nm, geom, want in (('tilted', {'m22': 0.5}, 'preconditions not met'),
                           ('sine too big', {'sine': 0.95, 'm22': 0.9}, 'preconditions not met'),
                           ('geometry unreadable', {'max.z': None}, 'live geometry unreadable')):
        out = tmp / nm.replace(' ', '')
        g = FakeGdb(str(binary), read_fail_after_write=())
        g.geom.update(geom)
        ns = load(g, str(out))
        call(g, ret=1)
        call(g, ret=0)
        ns['summarize']()
        l, d = line(out), diag(out)
        scenario('precond ' + nm, 'status=FAIL' in l and 'attempted=0' in l
                 and want in d + l and g.mem[MODE] == ORIG
                 and 'forced below-box call never happened' in l, l)

    # 2 write lands, post-write readback raises
    out = tmp / 'readfail'
    g = FakeGdb(str(binary), read_fail_after_write=(MODE,))
    ns = load(g, str(out))
    call(g, ret=1)
    call(g, ret=0)
    ns['summarize']()
    l, d = line(out), diag(out)
    scenario('write then read fail', 'status=FAIL' in l and 'post-write readback unreadable' in d
             and 'restored=1' in l and 'immediate rollback restored the original' in d
             and g.mem[MODE] == ORIG, l)

    # 3 partial write then exception
    out = tmp / 'partialraise'
    g = FakeGdb(str(binary), execute_raise_after=True, read_fail_after_write=())
    ns = load(g, str(out))
    call(g, ret=1)
    call(g, ret=0)
    ns['summarize']()
    l, d = line(out), diag(out)
    scenario('partial write + raise', 'status=FAIL' in l and 'write raised' in d
             and 'restored=1' in l and 'rollback read back the ORIGINAL' in d
             and g.mem[MODE] == ORIG, l)

    # 4 rollback failure: the store lands, the readback raises, and the rollback write is dropped, so
    # the forced value survives and the return restore cannot confirm it either
    out = tmp / 'rollbackfail'
    g = FakeGdb(str(binary), read_fail_after_write=(MODE,), drop_restore_writes=True)
    ns = load(g, str(out))
    call(g, ret=1)
    call(g, ret=0)
    ns['summarize']()
    l, d = line(out), diag(out)
    scenario('rollback failure', 'status=FAIL' in l and 'attempted mutation(s) not restored' in l
             and 'rollback readback' in d and 'restored=0' in l and g.mem[MODE] == FORCED, l)

    # 5 unreturned natural call
    out = tmp / 'unreturnednatural'
    g = FakeGdb(str(binary), read_fail_after_write=())
    ns = load(g, str(out))
    call(g, ret=1)                      # forced call returns
    call(g, ret=0, do_return=False)     # natural call never returns
    ns['summarize']()
    l = line(out)
    scenario('unreturned natural', 'status=FAIL' in l and 'accepted call(s) without a return: [2]' in l
             and 'returned=1' in l, l)

    # 5b unreadable forced return: $rax could not be read, so the skip arm was never observed
    out = tmp / 'unreadableforced'
    g = FakeGdb(str(binary), read_fail_after_write=())
    ns = load(g, str(out))
    call(g, ret=None)                   # forced call, return value unreadable
    call(g, ret=0)                      # natural call with a readable bool
    ns['summarize']()
    l, d = line(out), diag(out)
    scenario('unreadable forced ret', 'status=FAIL' in l
             and 'returned without a readable bool result: [1]' in l
             and 'forced call returned' in l and 'forced_ret=None' in l
             and 'restored=1' in l and g.mem[MODE] == ORIG, l)

    # 5c unreadable natural return: a natural call with no readable bool is not completion evidence
    out = tmp / 'unreadablenatural'
    g = FakeGdb(str(binary), read_fail_after_write=())
    ns = load(g, str(out))
    call(g, ret=1)                      # forced call observed true
    call(g, ret=None)                   # natural call, return value unreadable
    ns['summarize']()
    l = line(out)
    scenario('unreadable natural ret', 'status=FAIL' in l
             and 'returned without a readable bool result: [2]' in l
             and 'no accepted natural call returned a readable bool' in l
             and 'restored=1' in l and g.mem[MODE] == ORIG, l)

    # 6 same this, wrong caller
    out = tmp / 'wrongcaller'
    g = FakeGdb(str(binary), read_fail_after_write=())
    ns = load(g, str(out))
    call(g, caller='CSomeActor::Hit', ret=1)
    ns['summarize']()
    l = line(out)
    scenario('wrong caller', 'status=FAIL' in l and 'refused=1' in l and 'accepted=0' in l
             and 'attempted=0' in l, l)

    # 7 allowed caller, different this
    out = tmp / 'wrongthis'
    g = FakeGdb(str(binary), read_fail_after_write=())
    ns = load(g, str(out))
    call(g, this=SPORE, ret=1)
    call(g, this=OTHER, ret=1)
    ns['summarize']()
    l, d = line(out), diag(out)
    scenario('wrong this', 'refused=1' in l and 'is not the tracked spore' in d, l)

    # 8 restore mismatch: the forced write lands, only the write-back at the return fails
    out = tmp / 'restoremismatch'
    g = FakeGdb(str(binary), read_fail_after_write=(), drop_restore_writes=True)
    ns = load(g, str(out))
    call(g, ret=1)
    call(g, ret=0)
    ns['summarize']()
    l, d = line(out), diag(out)
    scenario('restore mismatch', 'status=FAIL' in l and 'attempted mutation(s) not restored' in l
             and 'restore readback' in d and g.mem[MODE] == FORCED, l)

    # 9 no natural call
    out = tmp / 'nonatural'
    g = FakeGdb(str(binary), read_fail_after_write=())
    ns = load(g, str(out))
    call(g, ret=1)
    ns['summarize']()
    l = line(out)
    scenario('no natural', 'status=FAIL' in l and 'no accepted natural call returned' in l, l)

    # 10 install failure
    out = tmp / 'noinstall'
    g = FakeGdb(str(binary), install_fails=True)
    ns = load(g, str(out))
    ns['summarize']()
    l = line(out)
    scenario('no install', 'status=FAIL' in l and 'install failed' in l and 'installed=0' in l, l)

    print('\n%d scenario failure(s)' % len(failures))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
