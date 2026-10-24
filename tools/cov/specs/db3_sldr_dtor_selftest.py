#!/usr/bin/env python3
"""Mocked-gdb regression test for tools/cov/specs/db3_sldr_dtor.py - no game boot, no gdb.

The fixture attributes invocations from the four member-destructor calls (the empty dtor is inlined):
each member's `this` is the root plus a fixed offset, the frame above the member call is the inlined
`SLdrDestructibleBarrier::~SLdrDestructibleBarrier()`, and the frame above that is
LoadDestructibleBarrier.

  valid            one root, four members, loader frame contains it                     -> OK
  two invocations  two roots                                                            -> OK
  outside frame    root above the loader rbp                                            -> FAIL
  wrong caller     the frame above the inlined dtor is not LoadDestructibleBarrier        -> FAIL
  missing member   one member destructor never seen                                      -> FAIL
  foreign member   a member call with a different inlined caller is not attributed       -> FAIL(never ran)
  no invocation    nothing ran                                                           -> FAIL
  no install       breakpoint creation fails                                             -> FAIL

    python3 tools/cov/specs/db3_sldr_dtor_selftest.py
"""
import hashlib
import os
import pathlib
import sys
import tempfile
import types

SPEC = pathlib.Path(__file__).with_name('db3_sldr_dtor.py')
RSP, RBP = 0x7fffffffc000, 0x7fffffffc900
ROOT = RBP - 0x670
MEM = [('SLdrActorParameters', 0x220), ('SLdrDamageVulnerability', 0xc4),
       ('SLdrHealthInfo', 0xbc), ('SLdrEditorProperties', 0x0)]
LOADER = 'LoadDestructibleBarrier(CStateManager&, CInputStream&, CEntityInfo&)'
INLINE = 'SLdrDestructibleBarrier::~SLdrDestructibleBarrier()'
failures = []


class FakeGdb(types.ModuleType):
    def __init__(self, binary, install_fails=False, rsp=RSP, rbp=RBP, loader=LOADER, inline=INLINE):
        super().__init__('gdb')
        self.binary = binary
        self.install_fails = install_fails
        self.rsp, self.rbp = rsp, rbp
        self.loader, self.inline = loader, inline
        self.member_this = ROOT
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

        class _Exited:
            @staticmethod
            def connect(fn):
                outer.exited.append(fn)

        class _Events:
            exited = _Exited()
        self.error = error
        self.Breakpoint = Breakpoint
        self.events = _Events()

    def current_progspace(self):
        return types.SimpleNamespace(filename=self.binary)

    def newest_frame(self):
        outer = self

        class _F:
            def __init__(self, name, older=None):
                self._name, self._older = name, older

            def name(self):
                return self._name

            def older(self):
                return self._older

            def read_register(self, reg):
                return outer.rsp if reg == 'rsp' else outer.rbp

        loader = _F(outer.loader)
        inline = _F(outer.inline, loader)
        return _F('member', inline)

    def parse_and_eval(self, expr):
        if expr.strip() == '$rdi':
            return self.member_this
        raise self.error('unsupported expression: ' + expr[:40])


def digest(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


def load(gdbmod, outdir):
    os.makedirs(outdir, exist_ok=True)
    os.environ['COV_OUT'] = outdir
    sys.modules['gdb'] = gdbmod
    ns = {'__name__': 'db3_under_test', '__file__': str(SPEC)}
    exec(compile(SPEC.read_text(), str(SPEC), 'exec'), ns)
    return ns


def line(outdir):
    f = pathlib.Path(outdir, 'fixture.txt')
    return [l for l in f.read_text().splitlines() if l.strip()][-1] if f.exists() else ''


def member_bp(g, label):
    return next(b for b in g.Breakpoint.instances if label in b.spec)


def invocation(g, root=ROOT, members=MEM, inline=None):
    if inline is not None:
        g.inline = inline
    for label, off in members:
        g.member_this = root + off
        member_bp(g, label).stop()


def scenario(name, cond, detail=''):
    print(('PASS ' if cond else 'FAIL ') + '%-16s %s' % (name, detail))
    if not cond:
        failures.append(name)


def main():
    tmp = pathlib.Path(tempfile.mkdtemp(prefix='db3-selftest-'))
    binary = tmp / 'metroid_prime2_port'
    binary.write_bytes(b'stand-in binary for the db3 mocked-gdb test\n')

    out = tmp / 'valid'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    invocation(g)
    ns['summarize']()
    l = line(out)
    scenario('valid', 'status=OK' in l and 'invocations=1' in l and 'in_loader_frame=[True]' in l, l)

    out = tmp / 'two'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    invocation(g)
    invocation(g, root=ROOT - 0x100)
    ns['summarize']()
    l = line(out)
    scenario('two invocations', 'status=OK' in l and 'invocations=2' in l, l)

    out = tmp / 'outside'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    invocation(g, root=RBP + 0x1000)
    ns['summarize']()
    l = line(out)
    scenario('outside frame', 'status=FAIL' in l and 'is not inside the loader frame' in l, l)

    out = tmp / 'wrongcaller'
    g = FakeGdb(str(binary), loader='SomeOtherLoader(CStateManager&)')
    ns = load(g, str(out))
    invocation(g)
    ns['summarize']()
    l = line(out)
    scenario('wrong caller', 'status=FAIL' in l and 'not LoadDestructibleBarrier' in l, l)

    out = tmp / 'missingmember'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    invocation(g, members=MEM[:3])
    ns['summarize']()
    l = line(out)
    scenario('missing member', 'status=FAIL' in l and 'SLdrEditorProperties' in l, l)

    out = tmp / 'foreign'
    g = FakeGdb(str(binary), inline='CBarrierChunkGrid::~CBarrierChunkGrid()')
    ns = load(g, str(out))
    invocation(g)
    ns['summarize']()
    l = line(out)
    scenario('foreign member', 'status=FAIL' in l and 'never ran' in l, l)

    out = tmp / 'noinvocation'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    ns['summarize']()
    l = line(out)
    scenario('no invocation', 'status=FAIL' in l and 'never ran' in l, l)

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
