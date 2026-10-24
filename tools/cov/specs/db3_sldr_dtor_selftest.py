#!/usr/bin/env python3
"""Mocked-gdb regression test for tools/cov/specs/db3_sldr_dtor.py - no game boot, no gdb.

The success criterion is a strict consecutive ordered four-member chain (retail order), with the same
root, inferior, thread and frame context inside the chain, frame bounds re-read per member, and no chain
left pending.  This suite drives the real fixture code with a fake gdb:

  complete chain        one ordered four-member chain                                       -> OK
  reused root, twice    two complete chains at the same root                                -> OK, completed=2
  interleaved partial   chain A (2 members) then chain B with all four: union covers 4       -> FAIL
  changed context       the third member arrives with another loader pc                     -> FAIL
  out of order          members 0, then 2                                                   -> FAIL
  duplicate             member 0 twice                                                      -> FAIL
  pending at exit       only three members of the chain                                     -> FAIL
  outside frame         one member's root above the loader rbp                              -> FAIL
  wrong caller          the frame above the inlined dtor is not LoadDestructibleBarrier      -> FAIL
  foreign member call   an inlined frame that is not this dtor: not attributed              -> FAIL(no chain)
  no install            breakpoint creation fails                                           -> FAIL

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
    def __init__(self, binary, install_fails=False, rsp=RSP, rbp=RBP, loader=LOADER, inline=INLINE,
                 loader_pc=0xde1b20, inline_pc=0xde1b20, inferior=1, thread=1):
        super().__init__('gdb')
        self.binary = binary
        self.install_fails = install_fails
        self.rsp, self.rbp = rsp, rbp
        self.loader, self.inline = loader, inline
        self.loader_pc, self.inline_pc = loader_pc, inline_pc
        self.inferior, self.thread = inferior, thread
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

    def selected_inferior(self):
        return types.SimpleNamespace(num=self.inferior)

    def selected_thread(self):
        return types.SimpleNamespace(global_num=self.thread)

    def newest_frame(self):
        outer = self

        class _F:
            def __init__(self, name, pc, older=None):
                self._name, self._pc, self._older = name, pc, older

            def name(self):
                return self._name

            def pc(self):
                return self._pc

            def older(self):
                return self._older

            def read_register(self, reg):
                return outer.rsp if reg == 'rsp' else outer.rbp

        loader = _F(outer.loader, outer.loader_pc)
        inline = _F(outer.inline, outer.inline_pc, loader)
        return _F('member', outer.inline_pc, inline)

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


def member(g, index, root=ROOT):
    label, off = MEM[index]
    g.member_this = root + off
    member_bp(g, label).stop()


def chain(g, root=ROOT, indices=range(len(MEM))):
    for i in indices:
        member(g, i, root)


def scenario(name, cond, detail=''):
    print(('PASS ' if cond else 'FAIL ') + '%-18s %s' % (name, detail))
    if not cond:
        failures.append(name)


def main():
    tmp = pathlib.Path(tempfile.mkdtemp(prefix='db3fix-selftest-'))
    binary = tmp / 'metroid_prime2_port'
    binary.write_bytes(b'stand-in binary for the db3 mocked-gdb test\n')

    out = tmp / 'complete'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    chain(g)
    ns['summarize']()
    l = line(out)
    scenario('complete chain', 'status=OK' in l and 'completed_chains=1' in l
             and 'pending=None' in l, l)

    out = tmp / 'reused'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    chain(g)
    chain(g, root=ROOT)          # same root and same frame context, a second loader invocation
    ns['summarize']()
    l = line(out)
    scenario('reused root, twice', 'status=OK' in l and 'completed_chains=2' in l, l)

    out = tmp / 'interleaved'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    member(g, 0, ROOT)
    member(g, 1, ROOT)
    chain(g, root=ROOT - 0x40)   # another chain, its union covers all four
    ns['summarize']()
    l = line(out)
    scenario('interleaved partial', 'status=FAIL' in l and 'changed context' in l, l)

    out = tmp / 'context'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    member(g, 0)
    member(g, 1)
    g.loader_pc = 0xdeabcd0      # the third member reports another call site
    member(g, 2)
    ns['summarize']()
    l = line(out)
    scenario('changed context', 'status=FAIL' in l and 'changed context' in l, l)

    out = tmp / 'order'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    member(g, 0)
    member(g, 2)
    ns['summarize']()
    l = line(out)
    scenario('out of order', 'status=FAIL' in l and 'out-of-order' in l, l)

    out = tmp / 'duplicate'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    member(g, 0)
    member(g, 0)
    ns['summarize']()
    l = line(out)
    scenario('duplicate', 'status=FAIL' in l and 'duplicate or out-of-order' in l, l)

    out = tmp / 'pending'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    chain(g, indices=(0, 1, 2))
    ns['summarize']()
    l = line(out)
    scenario('pending at exit', 'status=FAIL' in l and 'still pending at exit' in l
             and 'no completed four-member destruction chain' in l, l)

    out = tmp / 'outside'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    g.rbp = ROOT - 0x100          # the root is above the loader frame now
    chain(g)
    ns['summarize']()
    l = line(out)
    scenario('outside frame', 'status=FAIL' in l and 'not inside the loader frame' in l, l)

    out = tmp / 'wrongcaller'
    g = FakeGdb(str(binary), loader='SomeOtherLoader(CStateManager&)')
    ns = load(g, str(out))
    chain(g)
    ns['summarize']()
    l = line(out)
    scenario('wrong caller', 'status=FAIL' in l and 'not LoadDestructibleBarrier' in l, l)

    out = tmp / 'foreign'
    g = FakeGdb(str(binary), inline='CBarrierChunkGrid::~CBarrierChunkGrid()')
    ns = load(g, str(out))
    chain(g)
    ns['summarize']()
    l = line(out)
    scenario('foreign member call', 'status=FAIL' in l and 'no completed four-member' in l, l)

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
