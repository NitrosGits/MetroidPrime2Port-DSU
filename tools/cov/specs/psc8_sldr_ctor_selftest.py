#!/usr/bin/env python3
"""Mocked-gdb regression test for tools/cov/specs/psc8_sldr_ctor.py - no game boot, no gdb process.

It loads the fixture source with a fake `gdb` module and drives its real code paths:
  1 valid        23 fields read and matching -> status=OK, fields_ok=23/23
  2 mismatch     one field reads a wrong value -> status=FAIL, fields_bad=1, mismatch reason
  3 unreadable   one field's address/memory raises gdb.error -> status=FAIL, unread list, reason
  4 exception    the reader raises inside stop() -> caught, status=FAIL, "fixture exception"
  5 no hit       installed but never stopped -> status=FAIL, "no breakpoint hit"
  6 wrong binary the binary hash differs -> the fixture refuses to install, status=FAIL,
                 "BINARY SHA MISMATCH", and no breakpoint object is created

    python3 tools/cov/specs/psc8_sldr_ctor_selftest.py
"""
import hashlib
import os
import pathlib
import re
import sys
import tempfile
import types

SPEC = pathlib.Path(__file__).with_name('psc8_sldr_ctor.py')
OBJ = 0x7fffffffc2f0
RBP = OBJ + 0x450
failures = []


class FakeGdb(types.ModuleType):
    def __init__(self, binary, field_values, addr_fail=(), mem_fail=(), obj_ok=True):
        super().__init__('gdb')
        self.binary = binary
        self.values = dict(field_values)
        self.addr_fail = set(addr_fail)
        self.mem_fail = set(mem_fail)
        self.obj_ok = obj_ok
        self.addr_of = {}
        self.exited = []
        outer = self

        class error(Exception):
            pass

        class Breakpoint:
            instances = []

            def __init__(self, spec, internal=False):
                self.spec, self.internal = spec, internal
                Breakpoint.instances.append(self)

        class _Exited:
            @staticmethod
            def connect(fn):
                outer.exited.append(fn)

        class _Events:
            exited = _Exited()

        self.error = error
        self.Breakpoint = Breakpoint
        self.events = _Events()
        self._next = 0x10000

    def current_progspace(self):
        return types.SimpleNamespace(filename=self.binary)

    def _addr(self, suffix):
        if suffix not in self.addr_of:
            self.addr_of[suffix] = self._next
            self._next += 8
        return self.addr_of[suffix]

    def parse_and_eval(self, expr):
        e = expr.strip()
        if e == '(long)&sldrThis':
            if self.obj_ok:
                return OBJ
            raise self.error('Cannot access memory at address 0x0')
        if e == '$rbp':
            return RBP
        m = re.match(r'\(long\)&\(\(SLdrPlantScarabSwarm\*\)0x[0-9a-f]+\)->(.+)$', e)
        if m:
            suffix = m.group(1)
            if suffix in self.addr_fail:
                raise self.error('There is no member named ' + suffix)
            return self._addr(suffix)
        m = re.match(r'\*\((unsigned int|unsigned char)\*\)0x([0-9a-f]+)$', e)
        if m:
            addr = int(m.group(2), 16)
            suffix = next((s for s, a in self.addr_of.items() if a == addr), None)
            if suffix is None or addr in self.mem_fail:
                raise self.error('Cannot access memory at address 0x%x' % addr)
            return self.values[suffix]
        raise self.error('unsupported expression: ' + e[:60])

    def execute(self, cmd):
        return None


def digest(path):
    h = hashlib.sha256()
    h.update(pathlib.Path(path).read_bytes())
    return h.hexdigest()


def load(gdbmod, outdir, expect_sha=None):
    """Exec the spec with the fake gdb; expect_sha rewrites the bound digest (test harness only)."""
    os.makedirs(outdir, exist_ok=True)
    os.environ['COV_OUT'] = outdir
    sys.modules['gdb'] = gdbmod
    src = SPEC.read_text()
    if expect_sha is not None:
        src = re.sub(r"EVIDENCE_SHA256 = '[0-9a-f]+'", "EVIDENCE_SHA256 = '%s'" % expect_sha, src, count=1)
    ns = {'__name__': 'psc8_spec_under_test', '__file__': str(SPEC)}
    exec(compile(src, str(SPEC), 'exec'), ns)
    return ns


def summary_line(outdir):
    lines = [l for l in pathlib.Path(outdir, 'fixture.txt').read_text().splitlines() if l.strip()]
    return lines[-1] if lines else ''


def scenario(name, cond, detail=''):
    if cond:
        print('PASS %-14s %s' % (name, detail))
    else:
        print('FAIL %-14s %s' % (name, detail))
        failures.append(name)


def main():
    tmp = pathlib.Path(tempfile.mkdtemp(prefix='psc8-selftest-'))
    binary = tmp / 'metroid_prime2_port'
    binary.write_bytes(b'a stand-in binary for the mocked-gdb test\n')
    sha = digest(binary)

    oracle_values = None
    # the expected bits come from the spec itself, so the test follows the oracle table
    ns_probe = load(FakeGdb(str(binary), {}), str(tmp / 'probe'), expect_sha=sha)
    oracle_values = {suffix: want for suffix, _w, want in ns_probe['ORACLE']}

    # 1 valid
    out = tmp / 'valid'
    ns = load(FakeGdb(str(binary), oracle_values), str(out), expect_sha=sha)
    g = sys.modules['gdb']
    ns['Bp']().stop()
    ns['on_exit'](None)
    line = summary_line(out)
    scenario('valid', 'status=OK' in line and 'fields_ok=23/23' in line and 'fields_bad=0' in line, line)

    # 2 mismatch
    out = tmp / 'mismatch'
    bad = dict(oracle_values)
    bad['grenadeBounceSound'] = 0            # upstream value instead of the TARGET_PC -1
    ns = load(FakeGdb(str(binary), bad), str(out), expect_sha=sha)
    ns['Bp']().stop()
    ns['on_exit'](None)
    line = summary_line(out)
    scenario('mismatch', 'status=FAIL' in line and 'fields_bad=1' in line and 'mismatch' in line, line)

    # 3 unreadable address, then unreadable memory
    for how, kwargs in (('unreadable', {'addr_fail': ('grenadeEffect',)}),
                        ('unreadable', {'mem_fail': (0x10000 + 8 * 3,)})):
        out = tmp / ('unread%d' % len(failures))
        gdbmod = FakeGdb(str(binary), oracle_values, **kwargs)
        ns = load(gdbmod, str(out), expect_sha=sha)
        if 'mem_fail' in kwargs:  # resolve addresses first so the memory address is known
            gdbmod._addr('grenadeEffect')
            ns['Bp']().stop()
        else:
            ns['Bp']().stop()
        ns['on_exit'](None)
        line = summary_line(out)
        scenario('unreadable', 'status=FAIL' in line and 'unreadable' in line
                 and 'fields_ok=22/23' in line, line)

    # 4 exception raised inside the reader
    out = tmp / 'exception'
    ns = load(FakeGdb(str(binary), oracle_values), str(out), expect_sha=sha)
    boom = ns['read_field']

    def explode(*a, **k):
        raise RuntimeError('simulated reader failure')
    ns['read_field'] = explode
    ns['Bp']().stop()
    ns['on_exit'](None)
    line = summary_line(out)
    ns['read_field'] = boom
    scenario('exception', 'status=FAIL' in line and 'fixture exception' in line, line)

    # 5 installed but never hit
    out = tmp / 'nohit'
    ns = load(FakeGdb(str(binary), oracle_values), str(out), expect_sha=sha)
    ns['on_exit'](None)
    line = summary_line(out)
    scenario('no hit', 'status=FAIL' in line and 'no breakpoint hit' in line
             and 'no object read' in line, line)

    # 6 binary mismatch: the real digest stays, the stand-in binary differs
    out = tmp / 'wrongbinary'
    gdbmod = FakeGdb(str(binary), oracle_values)
    ns = load(gdbmod, str(out))  # no expect_sha rewrite: the spec keeps the archived digest
    ns['on_exit'](None)
    line = summary_line(out)
    scenario('wrong binary', 'status=FAIL' in line and 'BINARY SHA MISMATCH' in line
             and 'installed=0' in line, line)
    scenario('no bp on mismatch', gdbmod.Breakpoint.instances == [],
             'breakpoints created: %d' % len(gdbmod.Breakpoint.instances))
    # ... and a missing binary path fails closed too
    out = tmp / 'nopath'
    gdbmod = FakeGdb(None, oracle_values)
    ns = load(gdbmod, str(out))
    ns['on_exit'](None)
    line = summary_line(out)
    scenario('no binary path', 'status=FAIL' in line and 'no binary path' in line, line)

    print('\n%d scenario failure(s)' % len(failures))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
