"""Offline sz2 tests: the retail-asm model, the snapshot/rollback and the ownership rules.
Run: python3 -I tools/cov/specs/sz2_selftest.py   (no game, no gdb inferior)."""
import importlib.util
import os
from pathlib import Path
import sys
import types
import unittest


class Memory:
    def __init__(self):
        self.raw = bytearray(range(128))
        self.writes = []
        self.fail_at = None

    def read_memory(self, address, size):
        return bytes(self.raw[address:address + size])

    def write_memory(self, address, raw):
        self.writes.append(address)
        if self.fail_at is not None and len(self.writes) == self.fail_at:
            raise RuntimeError('write failed')
        self.raw[address:address + len(raw)] = raw


class Frame:
    def __init__(self, stack, token):
        self.stack, self.token = stack, token

    def is_valid(self):
        return self.token in self.stack

    def older(self):
        i = self.stack.index(self.token) + 1
        return Frame(self.stack, self.stack[i]) if i < len(self.stack) else None

    def __eq__(self, other):
        return isinstance(other, Frame) and self.token is other.token


XF = ([1, 0, 0, 3.5, 0, 1, 0, -2.25, 0, 0, 1, 7.0], (0.5, 1.0, -0.125))
TR = (3.5, -2.25, 7.0)


class Tests(unittest.TestCase):
    def setUp(self):
        self.mem = Memory()
        self.original = bytes(self.mem.raw)
        self.stack = [object(), object()]
        self.ctx = [1, 7]
        self.arm_fail = False
        test = self

        class Finish:
            def __init__(self, frame, internal=True):
                if test.arm_fail:
                    raise RuntimeError('cannot arm return')
        self.gdb = types.SimpleNamespace(
            Breakpoint=object, FinishBreakpoint=Finish,
            selected_inferior=lambda: types.SimpleNamespace(num=self.ctx[0], pid=4242),
            selected_thread=lambda: types.SimpleNamespace(global_num=self.ctx[1]),
            newest_frame=lambda: Frame(self.stack, self.stack[0]))
        sys.modules['gdb'] = self.gdb
        old = os.environ.get('SZ2_SELFTEST')
        os.environ['SZ2_SELFTEST'] = '1'
        try:
            spec = importlib.util.spec_from_file_location('sz2', Path(__file__).with_name('sz2_think.py'))
            self.m = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(self.m)
        finally:
            if old is None:
                os.environ.pop('SZ2_SELFTEST')
            else:
                os.environ['SZ2_SELFTEST'] = old
        self.events = []
        self.m.log = lambda event, **kw: self.events.append((event, kw))

    def case(self, name):
        c = dict(self.m.D)
        c.update(dict(self.m.CASES)[name])
        return c

    def run_model(self, name):
        return self.m.model(self.case(name), 5, TR, XF)

    # ---- model, hand-computed from the retail asm / quoted arms
    def test_grow_and_clamp(self):
        final, calls, acc = self.run_model('dark-grow')   # 0.5 + 0.25/2 = 0.625
        self.assertEqual(final['act'], 0.625)
        self.assertEqual(calls[0], ('base_think', 0.25))
        self.assertIn(('update_manager',), calls)
        final, calls, _ = self.run_model('dark-grow-clamp')
        self.assertEqual(final['act'], 1.0)

    def test_full_shell(self):
        final, calls, _ = self.run_model('dark-full-shell-decay')  # 0.5 - 0.125
        self.assertEqual(final['shell'], 0.375)
        self.assertNotIn(('update_manager',), calls)
        final, _, _ = self.run_model('dark-full-shell-clamp')
        self.assertEqual(final['shell'], 0.0)

    def test_light_world_clears(self):
        final, calls, _ = self.run_model('light-off-clears')
        self.assertEqual(final['act'], 0.0)
        self.assertIn(('update_manager',), calls)
        _, calls, _ = self.run_model('light-off-zero')
        self.assertNotIn(('update_manager',), calls)

    def test_inactive_fade(self):
        final, calls, _ = self.run_model('inactive-fade')   # 0.5 - 0.25/4
        self.assertEqual(final['act'], 0.4375)
        self.assertNotIn(('set_zone_type', 0), calls)
        final, calls, _ = self.run_model('inactive-fade-end-echo')
        self.assertEqual(final['act'], 0.0)
        self.assertEqual(calls[1:4], [('set_zone_type', 0), ('lowpass', 0), ('update_manager',)])

    def test_lifetime(self):
        final, calls, _ = self.run_model('life-tick')
        self.assertEqual(final['lt'], 1.25)
        _, calls, _ = self.run_model('life-expired-active-zero')
        self.assertEqual(calls[-3:], [('send_deactivate',), ('remove_zone',), ('delete_object',)])
        _, calls, _ = self.run_model('life-expired-active-act')
        self.assertIn(('send_deactivate',), calls)
        self.assertNotIn(('remove_zone',), calls)
        _, calls, _ = self.run_model('life-expired-inactive-act')
        self.assertNotIn(('send_deactivate',), calls)

    def test_moved_arms(self):
        final, calls, _ = self.run_model('moved-obst-light')
        self.assertEqual(final['dirty'], 0)
        i = calls.index(('objbyid', 0x123))
        self.assertEqual(calls[i - 2:i], [('obstruction', 0), ('obstruction', 1)])
        t = [c for c in calls if c[0] == 'set_transform'][0][1]
        self.assertEqual((t[3], t[7], t[11]), (4.0, -1.25, 6.875))
        self.assertEqual(t[0], 1.0)
        _, calls, _ = self.run_model('mobile-notdirty')
        self.assertFalse([c for c in calls if c[0] in ('objbyid', 'obstruction')])
        _, calls, _ = self.run_model('inactive-mobile-dirty')
        self.assertFalse([c for c in calls if c[0] in ('objbyid', 'obstruction')])
        self.assertEqual(self.run_model('inactive-mobile-dirty')[0]['dirty'], 1)
        _, calls, _ = self.run_model('moved-wrongtype')
        self.assertNotIn('set_transform', [c[0] for c in calls])

    def test_echo_and_emitter(self):
        _, calls, _ = self.run_model('echo-zone')
        self.assertIn(('inform', TR, 6), calls)
        _, calls, _ = self.run_model('emitter-live')
        self.assertIn(('update_emitter', 0.25), calls)
        _, calls, _ = self.run_model('emitter-zero')
        self.assertIn(('emitter_off',), calls)

    def test_camera(self):
        final, calls, _ = self.run_model('inside-enter')
        self.assertEqual(final['pci'], 1)
        self.assertEqual(calls[-4:], [('send_msgs', 'max'), ('fog', 'in'), ('inside_filter',), ('lowpass', 1)])
        _, calls, _ = self.run_model('inside-enter-ign-cine')
        self.assertIn(('cine_query', 1), calls)
        self.assertNotIn(('inside_filter',), calls)
        final, calls, _ = self.run_model('inside-enter-ign-nocine')
        self.assertIn(('cine_query', 0), calls)
        self.assertIn(('inside_filter',), calls)
        final, calls, _ = self.run_model('enter-blocked-fog')
        self.assertEqual(calls[-2:], [('fog', 'in'), ('lowpass', 1)])
        self.assertEqual(final['fog'], 0)
        _, calls, _ = self.run_model('enter-fog')
        self.assertEqual(sum(1 for c in calls if c[0] == 'fog'), 1)  # the block clears the dirty flag
        _, calls, _ = self.run_model('fog-dirty-outside')
        self.assertEqual(calls[-2:], [('fog', 'out'), ('lowpass', 0)])

    def test_every_branch_has_a_case(self):
        seen = set()
        for name, _ in self.m.CASES:
            for c in self.run_model(name)[1]:
                seen.add(c[0] if len(c) < 2 or c[0] not in ('send_msgs', 'fog', 'lowpass', 'cine_query', 'obstruction') else c[:2])
        for want in ('update_manager', 'emitter_off', 'update_emitter', 'inform', 'send_deactivate', 'remove_zone',
                     'delete_object', 'set_zone_type', 'set_transform', 'objbyid', 'cast_light', 'inside_filter',
                     ('send_msgs', 'max'), ('send_msgs', 'zero'), ('fog', 'in'), ('fog', 'out'),
                     ('cine_query', 0), ('cine_query', 1), ('obstruction', 0), ('obstruction', 1)):
            self.assertIn(want, seen)

    def test_names_unique(self):
        names = [n for n, _ in self.m.CASES]
        self.assertEqual(len(names), len(set(names)))

    # ---- snapshot / ownership
    def snap(self):
        return self.m.Snapshot([('a', 8, 4), ('b', 16, 4), ('c', 32, 8)], self.mem)

    def window(self):
        c = self.case('dark-grow')
        return self.m.Window(0x1000, 0x2000, ('dark-grow', c), tuple(self.ctx), Frame(self.stack, self.stack[0]),
                             self.snap(), dict(uid=5, trans=TR, xform=XF))

    def test_snapshot_restores_exactly(self):
        s = self.snap()
        self.mem.write_memory(8, b'\xff' * 4)
        self.mem.write_memory(32, b'\xee' * 8)
        rows, errors = s.restore()
        self.assertFalse(errors)
        self.assertEqual(bytes(self.mem.raw), self.original)
        self.assertTrue(all(r['original'] == r['readback'] for r in rows))

    def test_restore_readback_mismatch_reported(self):
        s = self.snap()
        orig_write = self.mem.write_memory
        self.mem.write_memory = lambda a, r: None   # a write that silently does not land
        self.mem.raw[8] = 99
        rows, errors = s.restore()
        self.assertTrue(errors)

    def test_window_arm_failure_no_mutation(self):
        self.arm_fail = True
        with self.assertRaises(RuntimeError):
            self.window()
        self.assertFalse(self.mem.writes)
        self.assertFalse(self.m.state['active'])

    def test_unwind_restores_and_fails(self):
        w = self.window()
        self.mem.write_memory(8, b'\xff' * 4)
        w.out_of_scope()
        self.assertEqual(bytes(self.mem.raw), self.original)
        self.assertEqual((self.m.state['failed'], self.m.state['restores']), (1, 1))
        self.assertFalse(self.m.state['active'])

    def test_context_mismatch_blocks_writes(self):
        w = self.window()
        self.ctx[1] = 9
        before = len(self.mem.writes)
        with self.assertRaises(RuntimeError):
            w.restore()
        self.assertEqual(len(self.mem.writes), before)

    def test_owned_requires_frame_equality(self):
        w = self.window()
        self.assertIsNone(self.m.owned())           # newest.older() is stack[1], owner is stack[0]
        self.assertEqual(self.m.state['ignored'], 1)
        self.stack.insert(0, object())                # a callee frame whose caller is the owner
        self.assertIs(self.m.owned(), w)
        self.ctx[1] = 9
        self.assertIsNone(self.m.owned())             # another thread: not ours

    def test_duplicate_stop_has_no_writes(self):
        w = self.window()
        w.closed = True
        before = len(self.mem.writes)
        self.assertTrue(w.stop())
        self.assertEqual(len(self.mem.writes), before)

    def apply_window(self, name):
        """apply() against the fake memory: pointer slot at 40 holds a 'natural' emitter, scratch at 64"""
        mem = self.mem
        mem.num = self.ctx[0]
        mem.raw[40:48] = (0x7777).to_bytes(8, 'little')
        m = self.m
        m.state['scratch'] = 64
        reg = self.fake_xmm()
        m.ev = lambda e: reg['v'] if e == '$xmm0.v4_float[0]' else (40 if e.startswith('&') else 0)
        m.iv = lambda e: int.from_bytes(mem.raw[40:48], 'little')
        m.zone_expr = lambda this, member: member
        m.write_float = m.write_int = m.write_bit = lambda *a, **k: None
        m.read_bit = lambda *a: 0
        self.gdb.lookup_type = lambda n: None
        self.gdb.selected_inferior = lambda: mem
        w = m.Window(0x1000, 0x2000, (name, self.case(name)), tuple(self.ctx), Frame(self.stack, self.stack[0]),
                     m.Snapshot([('mEchoEmitter.mPtr', 40, 8), ('scratch page', 64, 16)], mem),
                     dict(uid=5, trans=TR, xform=XF, light_id=0x123))
        return w

    def test_natural_emitter_hidden_then_restored(self):
        w = self.apply_window('dark-grow')            # emit=0: the natural pointer is nulled
        w.apply()
        self.assertEqual(bytes(self.mem.raw[40:48]), bytes(8))
        w.restore()
        self.assertEqual(int.from_bytes(self.mem.raw[40:48], 'little'), 0x7777)
        self.assertEqual(bytes(self.mem.raw[64:80]), self.original[64:80])

    def test_natural_emitter_replaced_by_fake_page(self):
        w = self.apply_window('emitter-live')        # emit=1: the pointer is the scratch page
        self.m.iv = lambda e: int.from_bytes(self.mem.raw[40:48], 'little')
        w.apply()
        self.assertEqual(int.from_bytes(self.mem.raw[40:48], 'little'), 64)
        w.restore()
        self.assertEqual(int.from_bytes(self.mem.raw[40:48], 'little'), 0x7777)

    def test_every_case_has_no_emitter_guard(self):
        with open(Path(__file__).with_name('sz2_think.py')) as f:
            self.assertNotIn("state['skipped_emitter'] += 1\n                return False", f.read())

    def fake_xmm(self):
        """a fake $xmm0 float lane: gdb.execute('set var $xmm0.v4_float[0] = x') and ev('$xmm0...')"""
        import struct
        reg = {'v': 0.0166666675}
        def execute(cmd):
            assert cmd.startswith('set var $xmm0.v4_float[0] = ')
            reg['v'] = struct.unpack('<f', struct.pack('<f', float(cmd.split('= ')[1])))[0]
        self.gdb.execute = execute
        self.m.ev = lambda e: reg['v'] if e == '$xmm0.v4_float[0]' else 0
        return reg

    def test_dt_written_to_xmm0_with_readback(self):
        reg = self.fake_xmm()
        before, after = self.m.write_dt(0.25)
        self.assertEqual((before, after, reg['v']), (0.0166666675, 0.25, 0.25))
        self.m.write_dt(0.1)                       # not representable: the readback is the float32
        self.assertEqual(reg['v'], self.m.f32(0.1))

    def test_dt_readback_mismatch_raises(self):
        reg = self.fake_xmm()
        self.gdb.execute = lambda cmd: None        # the write does not land
        with self.assertRaises(RuntimeError):
            self.m.write_dt(0.25)

    def test_apply_writes_case_dt_and_judges_against_it(self):
        w = self.apply_window('inactive-fade')
        w.apply()
        reg = {'v': w.dt_written}
        self.assertEqual((reg['v'], w.dt_written), (0.25, 0.25))
        self.assertEqual(self.run_model('inactive-fade')[1][0], ('base_think', reg['v']))

    def test_every_case_dt_is_written_not_natural(self):
        for name, _ in self.m.CASES:
            self.assertNotEqual(self.case(name)['dt'], 0.0166666675)

    def test_decay_cases_cross_zero_at_case_dt(self):
        for name in ('inactive-fade-end', 'inactive-fade-end-echo', 'emitter-inactive-fade'):
            final, calls, _ = self.run_model(name)
            self.assertEqual(final['act'], 0.0, name)
            self.assertIn(('lowpass', 0), calls, name)

    def test_finished_window_is_retired(self):
        w = self.window()
        w.enabled = True
        w.judge = lambda: ({}, [], [])
        w.stop()
        self.assertFalse(w.enabled)
        self.assertTrue(w.closed)

    def test_stall_dump_is_gdb_side_and_kills(self):
        import tempfile
        cmds = []
        self.gdb.execute = lambda c, to_string=False: cmds.append(c) or ('out of ' + c if to_string else None)
        switched = []
        fifo = types.SimpleNamespace(name='Aurora FIFO pro', switch=lambda: switched.append(1))
        other = types.SimpleNamespace(name='Main thread', switch=lambda: switched.append(2))
        self.gdb.selected_inferior = lambda: types.SimpleNamespace(threads=lambda: [other, fifo], pid=os.getpid())
        self.m.iv = lambda e: 0
        self.m.stall.update(want=True, this=0x40, restore=('c', [{'label': 'mActivation'}], []))
        with tempfile.TemporaryDirectory() as d:
            os.environ['COV_OUT'] = d
            self.m.stall_dump(None)
            with open(os.path.join(d, 'stall.txt')) as f:
                text = f.read()
        self.assertIn('thread apply all bt 30', cmds)
        self.assertEqual(cmds[-1], 'kill')
        self.assertIn('mActivation', text)
        self.assertIn('print *(CScriptSafeZone*)64', cmds)
        self.assertEqual(switched, [1])  # only the FIFO worker is inspected
        self.assertIn('info locals', cmds)
        self.assertIn('frame function aurora::gx::fifo::(anonymous namespace)::worker_main', cmds)
        for name in self.m.FIFO_STATE:  # the render-side state is dumped too
            self.assertIn("print '%s'" % name, cmds)
        self.assertFalse(self.m.stall['want'])
        cmds.clear()
        self.m.stall_dump(None)  # not requested: a normal stop is left alone
        self.assertEqual(cmds, [])

    def test_stall_interrupt_once(self):
        cmds = []
        self.gdb.execute = lambda c, to_string=False: cmds.append(c)
        self.m.stall.update(fired=False, want=False)
        self.m.stall_interrupt()
        self.m.stall_interrupt()
        self.assertEqual(cmds, ['interrupt'])
        self.assertTrue(self.m.stall['want'])

    def test_stall_watch_posts_only_when_idle(self):
        posted = []
        self.gdb.post_event = posted.append
        self.m.time.sleep = lambda s: None
        self.m.state['eligible'] = 5
        self.m.stall.update(fired=False, last=self.m.time.monotonic())
        ticks = iter([0, 0, 1])

        def sleep(s):
            if next(ticks) == 1:
                self.m.stall['last'] -= 1000
        self.m.time.sleep = sleep
        self.m.stall_watch()
        self.assertEqual(posted, [self.m.stall_interrupt])

    def test_stall_sample_reads_proc_tasks(self):
        import tempfile
        self.m.time.sleep = lambda s: None
        lines = self.m.task_states(os.getpid())
        self.assertTrue(lines and lines[0].split()[1] in 'RSDTtZ')
        with tempfile.TemporaryDirectory() as d:
            os.environ['COV_OUT'] = d
            self.m.stall_sample(os.getpid())
            text = open(os.path.join(d, 'stall_pre.txt')).read()
        self.assertEqual(text.count('### sample'), 2)

    def test_log_has_a_monotonic_time(self):
        with open(Path(__file__).with_name('sz2_think.py')) as f:
            self.assertIn("t=round(time.monotonic(), 3)", f.read())


if __name__ == '__main__':
    unittest.main()
