#!/usr/bin/env python3
"""Mocked-gdb regression test for tools/cov/specs/pud2_weaponhitsweakspot.py - no game boot, no gdb.

Drives the real fixture logic with a fake gdb:
  both paths        open call forced to CWeaponMode() (restored, returns true) + a natural call -> OK
  forced false      the forced call returns false (weak spot still hit)                       -> FAIL
  wrong caller      a call from another caller on the same this is refused                   -> FAIL
  wrong this        a call from an allowed caller on another spore is refused                -> FAIL
  restore mismatch  the write-back does not take                                              -> FAIL
  unreturned        the forced call never returns                                             -> FAIL
  no natural        only the forced call happens                                              -> FAIL
  no install        breakpoint creation fails                                                 -> FAIL

    python3 tools/cov/specs/pud2_weaponhitsweakspot_selftest.py
"""
import hashlib
import os
import pathlib
import re
import sys
import tempfile
import types

SPEC = pathlib.Path(__file__).with_name('pud2_weaponhitsweakspot.py')
SPORE = 0x555557a00000
OTHER = 0x555557b00000
MODE = 0x555558100000
failures = []


class FakeGdb(types.ModuleType):
    def __init__(self, binary, mstate=1, install_fails=False, restore_works=True):
        super().__init__('gdb')
        self.binary = binary
        self.mstate = mstate
        self.install_fails = install_fails
        self.restore_works = restore_works
        self.mem = {}
        self.caller = 'CPuddleSpore::Touch'
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
            return MODE
        if e == '$rax':
            return self.rax
        m = re.match(r'\*\(unsigned int\*\)0x([0-9a-f]+)$', e)
        if m:
            a = int(m.group(1), 16)
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
        if val != 0xffff and not self.restore_works:
            return None            # the write-back silently does not take
        self.mem[addr] = val
        return None


def digest(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


def load(gdbmod, outdir):
    os.makedirs(outdir, exist_ok=True)
    os.environ['COV_OUT'] = outdir
    sys.modules['gdb'] = gdbmod
    ns = {'__name__': 'pud2_under_test', '__file__': str(SPEC)}
    exec(compile(SPEC.read_text(), str(SPEC), 'exec'), ns)
    return ns


def line(outdir):
    f = pathlib.Path(outdir, 'fixture.txt')
    return [l for l in f.read_text().splitlines() if l.strip()][-1] if f.exists() else ''


def bp(g):
    return g.Breakpoint.instances[0]


def call(g, this=SPORE, caller='CPuddleSpore::Touch', ret=1, do_return=True):
    g.this, g.caller, g.rax = this, caller, ret
    bp(g).stop()
    if do_return and g.FinishBreakpoint.instances:
        g.FinishBreakpoint.instances[-1].stop()


def scenario(name, cond, detail=''):
    print(('PASS ' if cond else 'FAIL ') + '%-16s %s' % (name, detail))
    if not cond:
        failures.append(name)


def main():
    tmp = pathlib.Path(tempfile.mkdtemp(prefix='pud2-selftest-'))
    binary = tmp / 'metroid_prime2_port'
    binary.write_bytes(b'stand-in binary for the pud2 mocked-gdb test\n')
    sha = digest(binary)

    # 1 both paths: call 1 forced (returns true), call 2 natural (returns false: weak spot hit)
    out = tmp / 'both'
    g = FakeGdb(str(binary), mstate=1)
    g.mem[MODE] = 0x00000000
    ns = load(g, str(out))
    call(g, ret=1)
    call(g, ret=0)
    ns['summarize']()
    l = line(out)
    scenario('both paths', 'status=OK' in l and 'forced=1' in l and 'forced_ret=0x1' in l
             and 'natural=1' in l and g.mem[MODE] == 0x00000000, l)

    # 2 forced call returns false
    out = tmp / 'forcedfalse'
    g = FakeGdb(str(binary), mstate=1)
    g.mem[MODE] = 0x00000001
    ns = load(g, str(out))
    call(g, ret=0)
    call(g, ret=0)
    ns['summarize']()
    l = line(out)
    scenario('forced false', 'status=FAIL' in l and 'must return true' in l, l)

    # 3 same this, wrong caller
    out = tmp / 'wrongcaller'
    g = FakeGdb(str(binary), mstate=1)
    g.mem[MODE] = 0x00000000
    ns = load(g, str(out))
    call(g, caller='CSomeActor::Hit', ret=1)
    ns['summarize']()
    l = line(out)
    scenario('wrong caller', 'status=FAIL' in l and 'never happened' in l and 'refused=1' in l, l)

    # 4 allowed caller, different this
    out = tmp / 'wrongthis'
    g = FakeGdb(str(binary), mstate=1)
    g.mem[MODE] = 0x00000000
    ns = load(g, str(out))
    call(g, this=SPORE, ret=1)      # first call tracks SPORE as natural? force is armed at open
    # reset: reload so the tracked instance is SPORE and the forced call already happened
    out2 = tmp / 'wrongthis2'
    g2 = FakeGdb(str(binary), mstate=1)
    g2.mem[MODE] = 0x00000000
    ns2 = load(g2, str(out2))
    call(g2, this=SPORE, do_return=False)   # tracked=SPORE, armed+forced on SPORE
    g2.FinishBreakpoint.instances[-1].stop()
    call(g2, this=OTHER, ret=1)             # allowed caller, other spore -> refused
    ns2['summarize']()
    l = line(out2)
    scenario('wrong this', 'refused=1' in l and 'not the tracked spore' in pathlib.Path(out2, 'diag.log').read_text(), l)

    # 5 restore readback mismatch
    out = tmp / 'restore'
    g = FakeGdb(str(binary), mstate=1, restore_works=False)
    g.mem[MODE] = 0x00000000
    ns = load(g, str(out))
    call(g, ret=1)
    call(g, ret=0)
    ns['summarize']()
    l = line(out)
    scenario('restore mismatch', 'status=FAIL' in l and 'not restored' in l, l)

    # 6 forced call unreturned
    out = tmp / 'unreturned'
    g = FakeGdb(str(binary), mstate=1)
    g.mem[MODE] = 0x00000000
    ns = load(g, str(out))
    call(g, ret=1, do_return=False)
    ns['summarize']()
    l = line(out)
    scenario('unreturned', 'status=FAIL' in l and 'without a return' in l, l)

    # 7 no natural call
    out = tmp / 'nonatural'
    g = FakeGdb(str(binary), mstate=1)
    g.mem[MODE] = 0x00000000
    ns = load(g, str(out))
    call(g, ret=1)
    ns['summarize']()
    l = line(out)
    scenario('no natural', 'status=FAIL' in l and 'no natural call' in l, l)

    # 8 install failure
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
