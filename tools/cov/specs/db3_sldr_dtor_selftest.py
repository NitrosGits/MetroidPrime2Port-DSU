#!/usr/bin/env python3
"""Mocked-gdb regression test for tools/cov/specs/db3_sldr_dtor.py - no game boot, no gdb.

An invocation is owned by guarded frame equality (the LoadDestructibleBarrier frame, gdb Frame
equality, plus inferior/thread, root and rsp/rbp), NOT by pc or frame level: the pc advances between the
four member calls.  Success is exactly the ordered four members inside one owned frame.  OFFLINE ONLY:
this suite drives the real fixture code with a fake gdb whose frames carry an id and a validity flag.

  complete, changing pcs   one chain, pcs/levels change every member                             -> OK
  two invocations         two chains in one frame id (different pcs), then another frame         -> OK, 3
  different frame         third member's loader frame has another id                              -> FAIL
  stale frame             the frame object goes invalid mid-chain                                 -> FAIL
  interleaved             chain A partial (other frame), chain B full: union covers 4             -> FAIL
  recursive               a member 0 of a nested frame starts while the outer chain pends         -> FAIL
  wrong thread / inferior third member on another thread or inferior                              -> FAIL
  different root / bounds same frame, other root or rsp/rbp                                       -> FAIL
  out of order, duplicate, incomplete, pending at exit, outside frame, wrong caller, foreign
  member call, no install                                                                         -> FAIL

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
        self.frame_id = ('loader', RBP)
        self.level = 0
        self.valid = True
        self.frames = []
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

        class _F:  # equality is by frame id across calls, like gdb's frame_id
            def __init__(self, name, pc, level, fid, older=None, valid=True):
                self._name, self._pc, self._older = name, pc, older
                self._level, self.fid, self.valid = level, fid, valid

            def name(self):
                return self._name

            def pc(self):
                return self._pc

            def level(self):
                return self._level

            def older(self):
                return self._older

            def is_valid(self):
                return self.valid

            def __eq__(self, other):
                return hasattr(other, 'fid') and self.fid == other.fid

            def __hash__(self):
                return hash(self.fid)

            def read_register(self, reg):
                return outer.rsp if reg == 'rsp' else outer.rbp

        loader = _F(outer.loader, outer.loader_pc, outer.level + 2, outer.frame_id,
                    valid=outer.valid)
        inline = _F(outer.inline, outer.inline_pc, outer.level + 1, ('inline', outer.frame_id),
                    loader)
        frame = _F('member', outer.inline_pc, outer.level, ('member', outer.inline_pc), inline)
        outer.frames.append(loader)
        return frame

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

    def adv(g, n=1):
        g.loader_pc += 0x10 * n
        g.inline_pc += 0x10 * n

    def advancing_chain(g, root=ROOT, indices=range(len(MEM))):
        for i in indices:
            adv(g)
            g.level = i % 3          # the member frame depth changes too
            member(g, i, root)

    out = tmp / 'complete'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    advancing_chain(g)
    ns['summarize']()
    l = line(out)
    scenario('complete, changing pcs', 'status=OK' in l and 'completed_chains=1' in l
             and 'pending=None' in l and len({f.pc() for f in g.frames}) == 4, l)

    out = tmp / 'reused'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    advancing_chain(g)
    advancing_chain(g)           # a second invocation reusing the same frame id and root
    g.frame_id = ('loader', RBP - 0x2000)
    g.rsp, g.rbp = RSP - 0x2000, RBP - 0x2000
    advancing_chain(g, root=g.rbp - 0x670)   # a third, in another frame
    ns['summarize']()
    l = line(out)
    scenario('three invocations', 'status=OK' in l and 'completed_chains=3' in l, l)

    out = tmp / 'diffframe'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    advancing_chain(g, indices=(0, 1))
    g.frame_id = ('loader', RBP + 0x500)
    advancing_chain(g, indices=(2, 3))
    ns['summarize']()
    l = line(out)
    scenario('different frame', 'status=FAIL' in l and 'different frame' in l
             and 'completed_chains=0' in l, l)

    out = tmp / 'stale'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    advancing_chain(g, indices=(0, 1))
    first = g.frames[0]
    first.valid = False          # the frame that owns the pending chain went stale
    advancing_chain(g, indices=(2, 3))
    ns['summarize']()
    l = line(out)
    scenario('stale frame', 'status=FAIL' in l and 'stale or invalid frame' in l
             and 'completed_chains=0' in l, l)

    out = tmp / 'invalidnow'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    advancing_chain(g, indices=(0, 1))
    g.valid = False              # the current loader frame is invalid
    advancing_chain(g, indices=(2, 3))
    ns['summarize']()
    l = line(out)
    scenario('invalid current frame', 'status=FAIL' in l and 'stale or invalid frame' in l, l)

    out = tmp / 'interleaved'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    advancing_chain(g, indices=(0, 1))
    g.frame_id = ('loader', RBP + 0x500)
    advancing_chain(g)           # another frame's full chain; the union covers all four
    ns['summarize']()
    l = line(out)
    scenario('interleaved', 'status=FAIL' in l and 'different frame' in l, l)

    out = tmp / 'recursive'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    advancing_chain(g, indices=(0, 1))
    g.frame_id = ('loader', RBP - 0x3000)     # nested activation starts while the outer chain pends
    g.rsp, g.rbp = RSP - 0x3000, RBP - 0x3000
    advancing_chain(g, root=g.rbp - 0x670)
    g.frame_id, g.rsp, g.rbp = ('loader', RBP), RSP, RBP
    advancing_chain(g, indices=(2, 3))        # the outer chain resumes: already dropped
    ns['summarize']()
    l = line(out)
    scenario('recursive', 'status=FAIL' in l and 'different frame' in l, l)

    for what, setter in (('thread', lambda g: setattr(g, 'thread', 2)),
                         ('inferior', lambda g: setattr(g, 'inferior', 2))):
        out = tmp / ('wrong' + what)
        g = FakeGdb(str(binary))
        ns = load(g, str(out))
        advancing_chain(g, indices=(0, 1))
        setter(g)
        advancing_chain(g, indices=(2, 3))
        ns['summarize']()
        l = line(out)
        scenario('wrong ' + what, 'status=FAIL' in l and 'different inferior/thread' in l
                 and 'completed_chains=0' in l, l)

    out = tmp / 'diffroot'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    advancing_chain(g, indices=(0, 1))
    advancing_chain(g, root=ROOT - 0x40, indices=(2, 3))
    ns['summarize']()
    l = line(out)
    scenario('different root', 'status=FAIL' in l and 'different root' in l, l)

    out = tmp / 'diffbounds'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    advancing_chain(g, indices=(0, 1))
    g.rsp -= 0x20
    advancing_chain(g, indices=(2, 3))
    ns['summarize']()
    l = line(out)
    scenario('different bounds', 'status=FAIL' in l and 'different frame bounds' in l, l)

    out = tmp / 'incomplete'
    g = FakeGdb(str(binary))
    ns = load(g, str(out))
    advancing_chain(g)
    advancing_chain(g, indices=(0, 1))    # a second invocation that stops after two members
    ns['summarize']()
    l = line(out)
    scenario('incomplete second chain', 'status=FAIL' in l and 'still pending at exit' in l
             and 'completed_chains=1' in l, l)

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
    advancing_chain(g, indices=(0, 1, 2))
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
