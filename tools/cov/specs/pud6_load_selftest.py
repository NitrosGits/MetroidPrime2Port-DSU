#!/usr/bin/env python3
"""Mocked-gdb regression test for tools/cov/specs/pud6_load.py - no game boot, no gdb.

Contract under test: the one edit (ancs word for PUD6_ARM=null, flag byte for PUD6_ARM=flag) is recorded
BEFORE the write, rolled back on a failed write/readback, restored and read back at the restore site or
(safety net) at the return; the summary fails closed.

    python3 tools/cov/specs/pud6_load_selftest.py
"""
import hashlib
import os
import pathlib
import re
import sys
import tempfile
import types

SPEC = pathlib.Path(__file__).with_name('pud6_load.py')
RBP = 0x7ffe00001000
ANCS = RBP - 0x4d0          # rcx at the LdrToModelData call
FLAG = RBP - 0x3e8          # the unknown_0x5cdc877d byte
VALID = RBP - 0x308         # the optional's valid byte
ANCS_ORIG = 0x12345678
INSN = {748: 'call   0x1 <LdrToModelData>', 753: 'cmpb   $0x0,-0x308(%rbp)',
        1171: 'cmpb   $0x0,-0x3e8(%rbp)', 1178: 'je     0x2 <x+2256>'}
failures = []


class FakeGdb(types.ModuleType):
    def __init__(self, binary, flag=1, valid_after=0, install_fails=False, drop_writes=(),
                 raise_after_write=False, drop_restore=False, bad_insn=None):
        super().__init__('gdb')
        self.binary, self.install_fails = binary, install_fails
        self.drop_writes, self.raise_after_write = set(drop_writes), raise_after_write
        self.drop_restore = drop_restore
        self.mem = {ANCS: ANCS_ORIG, FLAG: flag, VALID: valid_after}
        self.bad_insn = bad_insn
        self.rax = 0
        self.rcx = ANCS
        self.exited = []
        self.written = []
        outer = self

        class error(Exception):
            pass

        class Breakpoint:
            instances = []

            def __init__(self, spec, internal=False):
                if outer.install_fails:
                    raise outer.error('No symbol "%s"' % spec)
                self.spec = spec
                Breakpoint.instances.append(self)

        class FinishBreakpoint:
            instances = []

            def __init__(self, frame, internal=False):
                FinishBreakpoint.instances.append(self)

        class _Exited:
            @staticmethod
            def connect(fn):
                outer.exited.append(fn)

        class _Events:
            exited = _Exited()
        self.error, self.Breakpoint, self.FinishBreakpoint = error, Breakpoint, FinishBreakpoint
        self.events = _Events()

    def current_progspace(self):
        return types.SimpleNamespace(filename=self.binary)

    def newest_frame(self):
        return object()

    def parse_and_eval(self, expr):
        e = expr.strip()
        if e == '$rbp':
            return RBP
        if e == '$rcx':
            return self.rcx
        if e == '$rax':
            if self.rax is None:
                raise self.error('rax unavailable')
            return self.rax
        m = re.match(r'\*\((unsigned int|unsigned char)\*\)0x([0-9a-f]+)$', e)
        if m:
            a = int(m.group(2), 16)
            if a not in self.mem:
                raise self.error('Cannot access memory at address 0x%x' % a)
            return self.mem[a]
        raise self.error('unsupported expression: ' + e)

    def execute(self, cmd, to_string=False):
        m = re.match(r'x/i \*\S+\+(\d+)$', cmd.strip())
        if m:
            off = int(m.group(1))
            return INSN[off] if self.bad_insn != off else 'nop'
        m = re.match(r'set \{(unsigned int|unsigned char)\}0x([0-9a-f]+) = 0x([0-9a-f]+)$', cmd.strip())
        if not m:
            return None
        addr, val = int(m.group(2), 16), int(m.group(3), 16)
        restoring = val in (ANCS_ORIG, 0, 1) and addr in self.written
        if addr not in self.drop_writes and not (self.drop_restore and restoring):
            self.mem[addr] = val
        self.written.append(addr)
        if self.raise_after_write and len(self.written) == 1:
            raise self.error('write fault after the store at 0x%x' % addr)
        return None


def load(gdbmod, outdir, arm):
    os.makedirs(outdir, exist_ok=True)
    os.environ['COV_OUT'] = outdir
    os.environ['PUD6_ARM'] = arm
    sys.modules['gdb'] = gdbmod
    ns = {'__name__': 'pud6_under_test', '__file__': str(SPEC)}
    exec(compile(SPEC.read_text(), str(SPEC), 'exec'), ns)
    return ns


def line(outdir):
    f = pathlib.Path(outdir, 'fixture.txt')
    return [l for l in f.read_text().splitlines() if l.strip()][-1] if f.exists() else ''


def diag(outdir):
    return pathlib.Path(outdir, 'diag.log').read_text()


def bps(g):
    d = {}
    for b in g.Breakpoint.instances:
        d[b.spec.rsplit('+', 1)[-1] if '+' in b.spec else 'entry'] = b
    return d


def run_load(g, ret, restore_site=True, do_return=True):
    """One LoadPuddleSpore: entry, write site, restore site, return."""
    b = bps(g)
    arm_w, arm_r = ('748', '753') if '748' in b else ('1171', '1178')
    b['entry'].stop()
    fin = g.FinishBreakpoint.instances[-1]
    b[arm_w].stop()
    if restore_site:
        b[arm_r].stop()
    g.rax = ret
    if do_return:
        fin.stop()


def scenario(name, cond, detail=''):
    print(('PASS ' if cond else 'FAIL ') + '%-24s %s' % (name, detail))
    if not cond:
        failures.append(name)


def main():
    tmp = pathlib.Path(tempfile.mkdtemp(prefix='pud6fix-selftest-'))
    binary = tmp / 'bin'
    binary.write_bytes(b'stand-in binary\n')

    # null arm, forced load returns nullptr, second load an entity
    out = tmp / 'null_ok'
    g = FakeGdb(str(binary))
    ns = load(g, str(out), 'null')
    run_load(g, 0)
    g.mem[VALID] = 1
    run_load(g, 0x5555)
    ns['summarize']()
    l = line(out)
    scenario('null ok', 'status=OK' in l and 'arm=null' in l and 'forced=1' in l and 'restored=1' in l
             and g.mem[ANCS] == ANCS_ORIG and 'FORCED ancs word' in diag(out)
             and ('0xdead0001') in diag(out), l)

    # null arm: forced load returned an entity (unresolvable id did not empty the optional)
    out = tmp / 'null_ret'
    g = FakeGdb(str(binary), valid_after=1)
    ns = load(g, str(out), 'null')
    run_load(g, 0x4444)
    run_load(g, 0x5555)
    ns['summarize']()
    l = line(out)
    scenario('null not null', 'status=FAIL' in l and 'must return nullptr' in l, l)

    # flag arm ok
    out = tmp / 'flag_ok'
    g = FakeGdb(str(binary), flag=1)
    ns = load(g, str(out), 'flag')
    seen = []
    b = bps(g)
    b['entry'].stop()
    fin = g.FinishBreakpoint.instances[-1]
    b['1171'].stop()
    seen.append(g.mem[FLAG])
    b['1178'].stop()
    g.rax = 0x4444
    fin.stop()
    run_load(g, 0x5555)
    ns['summarize']()
    l = line(out)
    scenario('flag ok', 'status=OK' in l and seen == [0] and g.mem[FLAG] == 1 and 'restored=1' in l, l)

    # flag arm from 0: flips to 1
    out = tmp / 'flag_zero'
    g = FakeGdb(str(binary), flag=0)
    ns = load(g, str(out), 'flag')
    run_load(g, 0x4444)
    run_load(g, 0x5555)
    ns['summarize']()
    scenario('flag from zero', 'status=OK' in line(out) and g.mem[FLAG] == 0, line(out))

    # flag byte not a bool: nothing written
    out = tmp / 'flag_junk'
    g = FakeGdb(str(binary), flag=7)
    ns = load(g, str(out), 'flag')
    run_load(g, 0x4444)
    run_load(g, 0x5555)
    ns['summarize']()
    l = line(out)
    scenario('flag junk', 'status=FAIL' in l and 'not a bool' in diag(out) and g.mem[FLAG] == 7
             and 'forced=0' in l, l)

    # wrong rcx: nothing written
    out = tmp / 'rcx'
    g = FakeGdb(str(binary))
    g.rcx = ANCS + 8
    ns = load(g, str(out), 'null')
    run_load(g, 0)
    run_load(g, 0x5555)
    ns['summarize']()
    l = line(out)
    scenario('wrong rcx', 'status=FAIL' in l and 'forced=0' in l and g.mem[ANCS] == ANCS_ORIG, l)

    # write raises after the store: immediate rollback, still reported
    out = tmp / 'raise'
    g = FakeGdb(str(binary), raise_after_write=True)
    ns = load(g, str(out), 'null')
    run_load(g, 0)
    run_load(g, 0x5555)
    ns['summarize']()
    l = line(out)
    scenario('write raise', 'status=FAIL' in l and 'write raised' in diag(out) and 'restored=1' in l
             and g.mem[ANCS] == ANCS_ORIG, l)

    # write dropped: readback mismatch, rollback restores the (unchanged) original
    out = tmp / 'drop'
    g = FakeGdb(str(binary), drop_writes=(ANCS,))
    ns = load(g, str(out), 'null')
    run_load(g, 0)
    run_load(g, 0x5555)
    ns['summarize']()
    l = line(out)
    scenario('write dropped', 'status=FAIL' in l and 'read back' in diag(out) and g.mem[ANCS] == ANCS_ORIG, l)

    # restore never lands: not restored, fails closed
    out = tmp / 'norestore'
    g = FakeGdb(str(binary), drop_restore=True)
    ns = load(g, str(out), 'null')
    run_load(g, 0)
    run_load(g, 0x5555)
    ns['summarize']()
    l = line(out)
    scenario('restore fails', 'status=FAIL' in l and 'not restored' in l and g.mem[ANCS] == 0xDEAD0001, l)

    # restore site never hit: the return safety net restores
    out = tmp / 'safetynet'
    g = FakeGdb(str(binary))
    ns = load(g, str(out), 'null')
    run_load(g, 0, restore_site=False)
    run_load(g, 0x5555)
    ns['summarize']()
    l = line(out)
    scenario('return safety net', 'return safety net' in diag(out) and 'restored=1' in l
             and g.mem[ANCS] == ANCS_ORIG, l)

    # forced load never returns
    out = tmp / 'unreturned'
    g = FakeGdb(str(binary))
    ns = load(g, str(out), 'null')
    run_load(g, 0, do_return=False)
    run_load(g, 0x5555)
    ns['summarize']()
    l = line(out)
    scenario('unreturned', 'status=FAIL' in l and 'without a return: [1]' in l, l)

    # unreadable return value
    out = tmp / 'rax'
    g = FakeGdb(str(binary))
    ns = load(g, str(out), 'null')
    run_load(g, None)
    run_load(g, 0x5555)
    ns['summarize']()
    scenario('unreadable rax', 'status=FAIL' in line(out) and 'unreadable' in line(out), line(out))

    # a single forced load (the driver's area makes only one): OK
    out = tmp / 'nonatural'
    g = FakeGdb(str(binary))
    ns = load(g, str(out), 'null')
    run_load(g, 0)
    ns['summarize']()
    scenario('single load ok', 'status=OK' in line(out) and 'natural_entities=0' in line(out), line(out))

    # install failures: breakpoint, bad insn, bad arm
    out = tmp / 'noinstall'
    g = FakeGdb(str(binary), install_fails=True)
    ns = load(g, str(out), 'null')
    ns['summarize']()
    scenario('no install', 'status=FAIL' in line(out) and 'installed=0' in line(out), line(out))
    out = tmp / 'badinsn'
    g = FakeGdb(str(binary), bad_insn=1171)
    ns = load(g, str(out), 'flag')
    ns['summarize']()
    scenario('site mismatch', 'status=FAIL' in line(out) and 'installed=0' in line(out)
             and not g.Breakpoint.instances, line(out))
    out = tmp / 'badarm'
    g = FakeGdb(str(binary))
    ns = load(g, str(out), 'zzz')
    ns['summarize']()
    scenario('bad arm', 'status=FAIL' in line(out) and 'installed=0' in line(out), line(out))

    print('\n%d scenario failure(s)' % len(failures))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
