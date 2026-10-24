#!/usr/bin/env python3
"""Mocked-gdb regression test for tools/cov/specs/mfly2_douseranimevent.py - no game boot, no gdb.

Loads the real fixture with a fake `gdb` and drives its breakpoints, so both arms and every failure
path are exercised statically:

  both arms     call 1 forced FadeIn (no base hit, returns), call 2 natural (base hit, returns) -> OK
  forced base   the forced call still reaches CPatterned::DoUserAnimEvent           -> FAIL
  no forced     MFORCE_CALL beyond the observed calls                              -> FAIL
  unreturned    the forced call never returns                                      -> FAIL
  ecx restore   the restore readback does not match                                -> FAIL
  no natural    every call was forced (no natural base call)                       -> FAIL
  bad type      a type value outside the enum range (wrong ABI slot)               -> FAIL
  no install    breakpoint creation fails                                          -> FAIL

    python3 tools/cov/specs/mfly2_douseranimevent_selftest.py
"""
import hashlib
import os
import pathlib
import re
import sys
import tempfile
import types

SPEC = pathlib.Path(__file__).with_name('mfly2_douseranimevent.py')
failures = []


class FakeGdb(types.ModuleType):
    def __init__(self, binary, ecx, caller='CMysteryFlyer::DoUserAnimEvent', install_fails=False,
                 restore_works=True):
        super().__init__('gdb')
        self.binary = binary
        self.ecx = ecx
        self.caller = caller
        self.install_fails = install_fails
        self.restore_works = restore_works
        self.exited = []
        outer = self

        class error(Exception):
            pass

        class Breakpoint:
            instances = []

            def __init__(self, spec, internal=False):
                if outer.install_fails:
                    raise outer.error('No symbol "%s" in current context.' % spec)
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
                return 'gdb-frame'

            def older(self):
                return _Caller()

        return _Top()

    def parse_and_eval(self, expr):
        e = expr.strip()
        if e == '$ecx':
            return self.ecx
        raise self.error('unsupported expression: ' + e[:40])

    def execute(self, cmd):
        m = re.match(r'set \$ecx = (-?\d+)$', cmd.strip())
        if m:
            val = int(m.group(1))
            if val == 14 or self.restore_works:
                self.ecx = val
        return None


def digest(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


def load(gdbmod, outdir, force_call=1):
    os.makedirs(outdir, exist_ok=True)
    os.environ['COV_OUT'] = outdir
    os.environ['MFORCE_CALL'] = str(force_call)
    sys.modules['gdb'] = gdbmod
    ns = {'__name__': 'mfly2_under_test', '__file__': str(SPEC)}
    exec(compile(SPEC.read_text(), str(SPEC), 'exec'), ns)
    return ns


def line(outdir):
    f = pathlib.Path(outdir, 'fixture.txt')
    return [l for l in f.read_text().splitlines() if l.strip()][-1] if f.exists() else ''


def bps(g):
    return g.Breakpoint.instances


def entry(g):
    return next(b for b in bps(g) if 'CMysteryFlyer' in b.spec)


def base(g):
    return next(b for b in bps(g) if 'CPatterned' in b.spec)


def do_call(g, ecx, base_too=False, caller='CMysteryFlyer::DoUserAnimEvent', do_return=True):
    g.ecx = ecx
    entry(g).stop()
    if base_too:
        g.caller = caller
        base(g).stop()
    if do_return and g.FinishBreakpoint.instances:
        g.FinishBreakpoint.instances[-1].stop()


def scenario(name, cond, detail=''):
    print(('PASS ' if cond else 'FAIL ') + '%-14s %s' % (name, detail))
    if not cond:
        failures.append(name)


def main():
    tmp = pathlib.Path(tempfile.mkdtemp(prefix='mfly2-selftest-'))
    binary = tmp / 'metroid_prime2_port'
    binary.write_bytes(b'stand-in binary for the mfly2 mocked-gdb test\n')
    sha = digest(binary)

    # 1 both arms
    out = tmp / 'both'
    g = FakeGdb(str(binary), 0)
    ns = load(g, str(out))
    do_call(g, 2)                 # natural: type 2, reaches the base
    do_call(g, 6, base_too=True)  # the next call is natural too in this scenario? keep FORCE_CALL=1
    ns['summarize']()
    l = line(out)
    # call 1 was forced (FadeIn), call 2 natural with a base hit
    scenario('both arms', 'status=OK' in l and 'forced=1' in l and 'forced_base_hits=0' in l
             and 'natural_returned=1' in l and 'base_from_this=1' in l, l)

    # 2 forced call still reaches the base
    out = tmp / 'forcedbase'
    g = FakeGdb(str(binary), 0)
    ns = load(g, str(out))
    do_call(g, 2, base_too=True)     # call 1 (forced) wrongly reaches the base
    do_call(g, 6, base_too=True)     # natural call
    ns['summarize']()
    l = line(out)
    scenario('forced base', 'status=FAIL' in l and 'still reached the CPatterned base' in l, l)

    # 3 the forced call never happens
    out = tmp / 'noforced'
    g = FakeGdb(str(binary), 0)
    ns = load(g, str(out), force_call=5)
    do_call(g, 2, base_too=True)
    do_call(g, 6, base_too=True)
    ns['summarize']()
    l = line(out)
    scenario('no forced', 'status=FAIL' in l and 'never happened' in l, l)

    # 4 forced call unreturned
    out = tmp / 'unreturned'
    g = FakeGdb(str(binary), 0)
    ns = load(g, str(out))
    do_call(g, 2, do_return=False)
    ns['summarize']()
    l = line(out)
    scenario('unreturned', 'status=FAIL' in l and 'unreturned call(s): [1]' in l, l)

    # 5 ecx restore readback mismatch
    out = tmp / 'restore'
    g = FakeGdb(str(binary), 0, restore_works=False)
    ns = load(g, str(out))
    do_call(g, 2)
    do_call(g, 6, base_too=True)
    ns['summarize']()
    l = line(out)
    scenario('ecx restore', 'status=FAIL' in l and 'ecx not restored' in l, l)

    # 6 no natural call (every call forced)
    out = tmp / 'nonatural'
    g = FakeGdb(str(binary), 0)
    ns = load(g, str(out))
    do_call(g, 2)
    ns['summarize']()
    l = line(out)
    scenario('no natural', 'status=FAIL' in l and 'no natural' in l, l)

    # 7 implausible type value (wrong ABI slot)
    out = tmp / 'badtype'
    g = FakeGdb(str(binary), 0)
    ns = load(g, str(out))
    do_call(g, 1000, base_too=True)
    do_call(g, 6, base_too=True)
    ns['summarize']()
    l = line(out)
    scenario('bad type', 'status=FAIL' in l and 'not a plausible EUserEventType' in l, l)

    # 8 install failure
    out = tmp / 'noinstall'
    g = FakeGdb(str(binary), 0, install_fails=True)
    ns = load(g, str(out))
    ns['summarize']()
    l = line(out)
    scenario('no install', 'status=FAIL' in l and 'install failed' in l and 'installed=0' in l, l)

    print('\n%d scenario failure(s)' % len(failures))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
