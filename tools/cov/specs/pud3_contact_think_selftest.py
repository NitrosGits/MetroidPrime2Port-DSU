"""Offline pud3 ownership/rollback/oracle tests (mock gdb; no game, no inferior).

    python3 tools/cov/specs/pud3_contact_think_selftest.py
"""
import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import types
import unittest

GC = 1 << 0x25
PLAYER = 1 << 0x20


class Memory:
    def __init__(self):
        self.raw = bytearray(64)
        self.raw[8:16] = (GC | PLAYER).to_bytes(8, 'little')
        self.raw[16:28] = bytes(range(1, 13))
        self.write_error = None

    def read_memory(self, address, size):
        return bytes(self.raw[address:address + size])

    def write_memory(self, address, raw):
        if self.write_error:
            self.write_error(address)
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


class Tests(unittest.TestCase):
    def setUp(self):
        self.mem = Memory()
        self.original = bytes(self.mem.raw)
        self.stack = [object()]
        self.arm_fail = False
        self.out = tempfile.mkdtemp()
        test = self

        class Finish:
            def __init__(self, frame, internal=True):
                if test.arm_fail:
                    raise RuntimeError('cannot arm return')

        self.gdb = types.SimpleNamespace(
            Breakpoint=object, FinishBreakpoint=Finish,
            selected_inferior=lambda: types.SimpleNamespace(num=1),
            selected_thread=lambda: types.SimpleNamespace(global_num=7),
            newest_frame=lambda: Frame(self.stack, self.stack[0]),
            events=types.SimpleNamespace(exited=types.SimpleNamespace(connect=lambda f: None)),
            parse_and_eval=lambda e: 0, current_progspace=lambda: None)
        sys.modules['gdb'] = self.gdb
        os.environ['COV_OUT'] = self.out
        os.environ['PUD3_SELFTEST'] = '1'
        spec = importlib.util.spec_from_file_location(
            'pud3', Path(__file__).with_name('pud3_contact_think.py'))
        self.m = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.m)

    def window(self, kind):
        return self.m.Window(kind, 1, 2, Frame(self.stack, self.stack[0]), (1, 7))

    def gc_snapshot(self, w):
        w.gc_snapshot = self.m.Snapshot([('mat', 8, 8)], self.mem)
        w.forced = 'gc'
        self.mem.raw[8:16] = (PLAYER).to_bytes(8, 'little')

    def test_arm_failure_blocks_everything(self):
        self.arm_fail = True
        with self.assertRaises(RuntimeError):
            self.window('upc')
        self.assertEqual(bytes(self.mem.raw), self.original)

    def test_gc_forced_window_restores_and_counts(self):
        w = self.window('upc')
        self.gc_snapshot(w)
        w.mat = [('R', 0x20), ('A', 0x20)]
        w.cbn = [('nat', 1)]
        self.assertFalse(w.stop())
        self.assertEqual(bytes(self.mem.raw), self.original)
        self.assertEqual((self.m.state['gc_ok'], self.m.state['failed']), (1, 0))

    def test_gc_wrong_sequence_fails_but_restores(self):
        w = self.window('upc')
        self.gc_snapshot(w)
        w.mat = [('R', 0x25), ('R', 0x20), ('A', 0x25), ('A', 0x20)]
        w.cbn = [('nat', 1)]
        self.assertTrue(w.stop())
        self.assertEqual(bytes(self.mem.raw), self.original)
        self.assertEqual((self.m.state['gc_ok'], self.m.state['failed']), (0, 1))

    def test_gc_without_push_is_noeffect_not_evidence(self):
        w = self.window('upc')
        self.gc_snapshot(w)
        self.assertFalse(w.stop())
        self.assertEqual((self.m.state['gc_ok'], self.m.state['upc_noeffect']), (0, 1))

    def test_restore_write_failure_reported(self):
        w = self.window('upc')
        self.gc_snapshot(w)
        self.mem.write_error = lambda a: (_ for _ in ()).throw(RuntimeError('boom'))
        self.assertTrue(w.stop())
        self.assertEqual(self.m.state['failed'], 1)

    def test_unwind_restores_and_fails(self):
        w = self.window('upc')
        self.gc_snapshot(w)
        w.out_of_scope()
        self.assertEqual(bytes(self.mem.raw), self.original)
        self.assertEqual(self.m.state['failed'], 1)
        self.assertFalse(self.m.state['active'].get((1, 7)))

    def test_think_arms(self):
        w = self.window('think')
        w.cbn = [('nat', 1)]
        w.emitter = 1
        self.assertFalse(w.stop())
        self.assertEqual(self.m.state['nat_think'], 1)
        w = self.window('think')
        w.forced = 'cbn'
        w.cbn = [('forced', 0)]
        self.assertFalse(w.stop())
        self.assertEqual((self.m.state['think_ok'], self.m.state['failed']), (1, 0))

    def test_think_forced_with_emitter_or_true_result_fails(self):
        w = self.window('think')
        w.forced = 'cbn'
        w.cbn = [('forced', 0)]
        w.emitter = 1
        self.assertTrue(w.stop())
        w = self.window('think')
        w.forced = 'cbn'
        w.cbn = [('forced', 1)]
        self.assertTrue(w.stop())
        self.assertEqual((self.m.state['think_ok'], self.m.state['failed']), (0, 2))

    def id_window(self, arg=0x8123, ret=0, emitter=0):
        w = self.window('think')
        w.id_snap = self.m.Snapshot([('id', 16, 2)], self.mem)
        w.forced, w.id_stale, w.id_arg, w.id_ret, w.emitter = 'id', 0x8123, arg, ret, emitter
        self.mem.raw[16:18] = (0x8123).to_bytes(2, 'little')
        return w

    def test_think_id_arm_restores_and_counts(self):
        w = self.id_window()
        self.assertFalse(w.stop())
        self.assertEqual(bytes(self.mem.raw), self.original)
        self.assertEqual((self.m.state['id_ok'], self.m.state['failed']), (1, 0))

    def test_think_id_wrong_result_or_branch_fails_but_restores(self):
        for kw in (dict(arg=0x1), dict(ret=5), dict(emitter=1)):
            w = self.id_window(**kw)
            self.assertTrue(w.stop())
            self.assertEqual(bytes(self.mem.raw), self.original)
        self.assertEqual((self.m.state['id_ok'], self.m.state['failed']), (0, 3))

    def test_think_id_unwind_restores(self):
        w = self.id_window()
        w.out_of_scope()
        self.assertEqual(bytes(self.mem.raw), self.original)
        self.assertEqual(self.m.state['failed'], 1)

    def test_upc_cbn_arm(self):
        w = self.window('upc')
        w.forced = 'cbn'
        w.entry_gc = True
        w.cbn = [('forced', 0)]
        w.mat = [('R', 0x25), ('R', 0x20), ('A', 0x25), ('A', 0x20)]
        self.assertFalse(w.stop())
        self.assertEqual(self.m.state['upc_cbn_ok'], 1)

    def test_upc_natural_push_follows_entry_ground_collider_bit(self):
        w = self.window('upc')
        w.entry_gc = False
        w.cbn = [('nat', 1)]
        w.mat = [('R', 0x20), ('A', 0x20)]
        self.assertFalse(w.stop())
        self.assertEqual((self.m.state['nat_upc'], self.m.state['nat_gc_clear']), (1, 1))
        w = self.window('upc')
        w.entry_gc = True
        w.cbn = [('nat', 1)]
        w.mat = [('R', 0x20), ('A', 0x20)]
        self.assertTrue(w.stop())
        self.assertEqual(self.m.state['failed'], 1)

    def test_duplicate_finish_and_nested_entry(self):
        w = self.window('upc')
        w.stop()
        self.assertTrue(w.stop())
        self.window('upc')
        with self.assertRaises(RuntimeError):
            self.window('upc')

    def test_summary_fails_closed(self):
        self.m.summary()
        self.assertTrue(open(os.path.join(self.out, 'pud3.summary.txt')).read().startswith('FAIL'))
        st = self.m.state
        st.update(nat_think=1, think_ok=1, id_ok=1, nat_upc=1, gc_ok=1, upc_cbn_ok=1)
        self.m.summary()
        self.assertTrue(open(os.path.join(self.out, 'pud3.summary.txt')).read().startswith('PASS'))
        st['failed'] = 1
        self.m.summary()
        self.assertTrue(open(os.path.join(self.out, 'pud3.summary.txt')).read().startswith('FAIL'))


if __name__ == '__main__':
    unittest.main()
